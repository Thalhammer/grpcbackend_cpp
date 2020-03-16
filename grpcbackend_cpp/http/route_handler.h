#pragma once
#include "con_handler.h"
#include <set>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class route_handler : public con_handler {
			public:
				typedef std::shared_ptr<route_handler> ptr;
				virtual ~route_handler() {}

				virtual std::set<std::string> get_methods() = 0;
				virtual std::set<std::string> get_routes() = 0;
			};
			using route_handler_ptr = route_handler::ptr;
		}
	}
}