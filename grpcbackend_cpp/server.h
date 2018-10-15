#pragma once
#include <ttl/logger.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>

namespace grpc {
	class Server;
	class ServerBuilder;
}

namespace thalhammer {
	namespace grpcbackend {
		class handler;
		namespace http { class router; }
		namespace websocket { class hub; }
		class server
		{
			std::shared_ptr<ttl::logger> log;
			std::unique_ptr<http::router> router;
			std::unique_ptr<websocket::hub> hub;
			std::unique_ptr<handler> http_service;

			struct cqcontext;
			std::atomic<bool> exit;
			std::vector<cqcontext> cqs;
			std::unique_ptr<::grpc::ServerBuilder> builder;

			std::unique_ptr<::grpc::Server> mserver;
		public:
			struct options {
				std::shared_ptr<ttl::logger> log;
				size_t num_worker_threads;
			};

			explicit server(std::ostream& logstream = std::cout);
			explicit server(ttl::logger& l);
			explicit server(std::shared_ptr<ttl::logger> l);
			explicit server(options opts);
			server(const server& other) = delete;
			server& operator=(const server& other) = delete;
			~server();

			void start_server();
			void shutdown_server(std::chrono::milliseconds max_wait = std::chrono::milliseconds(10000));

			::grpc::ServerBuilder& get_builder() { return *builder; }

			ttl::logger& get_logger() { return *log; }
			http::router& get_router() { return *router; }
			websocket::hub& get_wshub() { return *hub; }
			handler& get_handler() { return *http_service; }
		};
	}
}