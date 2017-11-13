#include "server.h"

namespace thalhammer {
	namespace grpcbackend {
		server::server(std::ostream & logstream)
			: logger(logstream), http_service(router, hub), exit(false)
		{
			builder.RegisterService(&http_service);
			for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
				auto cq = builder.AddCompletionQueue(false);
				cqs.push_back({ std::thread(), std::move(cq) });
			}
		}

		server::~server()
		{
			this->shutdown_server();
		}

		void server::start_server()
		{
			mserver = builder.BuildAndStart();

			if (!mserver) {
				throw std::runtime_error("Failed to init server");
			}

			for (auto& cq : cqs) {
				cq.th = std::thread([&]() {
					http_service.async_task(cq.cq.get(), exit);
				});
			}
		}

		void server::shutdown_server()
		{
			if(!exit.exchange(true)) {
				mserver->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(10));
				for (auto& cq : cqs) {
					if (cq.th.joinable())
						cq.th.join();
					if (cq.cq)
						cq.cq->Shutdown();
				}
			}
		}
	}
}
