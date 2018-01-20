#include "server.h"

namespace thalhammer {
	namespace grpcbackend {
		
		server::server(std::ostream & logstream)
			: server(std::make_shared<logger>(logstream))
		{}

		server::server(logger& l)
			: server(std::shared_ptr<logger>(&l, [](logger*){}))
		{}

		server::server(std::shared_ptr<logger> l)
			: log(l), exit(false)
		{
			hub = std::make_unique<websocket::hub>(*l);
			router = std::make_unique<http::router>();
			http_service = std::make_unique<handler>(*router, *hub, *log);
			builder.RegisterService(http_service.get());
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
			if (exit)
				throw std::logic_error("start_server called after shutdown_server");

			mserver = builder.BuildAndStart();

			if (!mserver) {
				throw std::runtime_error("Failed to init server");
			}

			for (auto& cq : cqs) {
				cq.th = std::thread([&]() {
					get_logger()(loglevel::TRACE, "grpc_server") << "CQ thread start";
					http_service->async_task(cq.cq.get());
					get_logger()(loglevel::TRACE, "grpc_server") << "CQ thread exit";
				});
			}
		}

		void server::shutdown_server(std::chrono::milliseconds max_wait)
		{
			if(!exit.exchange(true)) {
				hub->close_all();
				if(mserver)
					mserver->Shutdown(std::chrono::system_clock::now() + max_wait);
				for (auto& cq : cqs) {
					if (cq.cq)
						cq.cq->Shutdown();
					if (cq.th.joinable())
						cq.th.join();
				}
				cqs.clear();
				http_service.reset();
				hub.reset();
				router.reset();
			}
		}
	}
}
