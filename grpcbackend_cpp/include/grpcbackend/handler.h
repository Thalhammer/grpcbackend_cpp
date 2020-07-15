#pragma once
#include "handler.grpc.pb.h"
#include "http/router.h"
#include "websocket/con_handler.h"
#include "http/con_handler.h"
#include <atomic>

namespace thalhammer {
	namespace grpcbackend {
		class handler
		: public ::thalhammer::http::Handler::AsyncService
		{
			websocket::con_handler& _ws_handler;
			http::con_handler& _http_handler;
			ttl::logger& _logger;
		public:
			handler(websocket::con_handler& ws_handler, http::con_handler& http_handler, ttl::logger& logger);
			virtual ~handler();

			ttl::logger& get_logger() { return _logger; }

			void async_task(::grpc::ServerCompletionQueue* cq);

			//virtual ::grpc::Status Handle(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream);
		};
	}
}