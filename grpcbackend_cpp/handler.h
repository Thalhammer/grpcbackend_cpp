#pragma once
#include "handler.grpc.pb.h"
#include "http/router.h"
#include "websocket/con_handler.h"
#include <atomic>

namespace thalhammer {
	namespace grpcbackend {
		class handler
		: public ::thalhammer::http::Handler::WithAsyncMethod_HandleWebSocket<::thalhammer::http::Handler::Service>
		{
			http::router& _route;
			websocket::con_handler& _ws_handler;
			ttl::logger& _logger;
		public:
			handler(http::router& route, websocket::con_handler& ws_handler, ttl::logger& logger);
			virtual ~handler();

			ttl::logger& get_logger() { return _logger; }

			void async_task(::grpc::ServerCompletionQueue* cq);

			virtual ::grpc::Status Handle(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::thalhammer::http::HandleResponse, ::thalhammer::http::HandleRequest>* stream);
		};
	}
}