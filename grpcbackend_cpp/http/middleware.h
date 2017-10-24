#pragma once
#include "request.h"
#include "response.h"
#include <functional>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class middleware {
			public:
				typedef std::shared_ptr<middleware> ptr;
				virtual void handle_request(request& req, response& resp, std::function<void(request&, response&)>& next) = 0;
			};
			using middleware_ptr = middleware::ptr;
		}
	}
}