#include "server.h"
#include "handler.h"
#include "websocket/hub.h"
#include <grpc++/server_builder.h>
#include <grpc++/server.h>

namespace thalhammer {
	namespace grpcbackend {
		struct server::cqcontext{
			std::thread th;
			std::unique_ptr<::grpc::ServerCompletionQueue> cq;
		};

		server::server(std::ostream & logstream)
			: server(std::make_shared<logger>(logstream))
		{}

		server::server(logger& l)
			: server(std::shared_ptr<logger>(&l, [](logger*){}))
		{}

		server::server(std::shared_ptr<logger> l)
			: server(options{ l, 0 })
		{}

		server::server(options opts)
			: log(opts.log), exit(false)
		{
			if (opts.num_worker_threads == 0)
				opts.num_worker_threads = std::thread::hardware_concurrency();
			hub = std::make_unique<websocket::hub>(*log);
			router = std::make_unique<http::router>();
			http_service = std::make_unique<handler>(*router, *hub, *log);
			builder = std::make_unique<::grpc::ServerBuilder>();
			builder->RegisterService(http_service.get());
			for (size_t i = 0; i < opts.num_worker_threads; i++) {
				auto cq = builder->AddCompletionQueue(false);
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

			mserver = builder->BuildAndStart();

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
