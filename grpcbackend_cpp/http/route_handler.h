#pragma once
#include "request.h"
#include "response.h"
#include <set>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class route_handler {
			public:
				typedef std::shared_ptr<route_handler> ptr;
				virtual ~route_handler() {}

				virtual std::set<std::string> get_methods() = 0;
				virtual std::set<std::string> get_routes() = 0;
				virtual void handle_request(request& req, response& resp) = 0;
			};
			using route_handler_ptr = route_handler::ptr;
		}
	}
}