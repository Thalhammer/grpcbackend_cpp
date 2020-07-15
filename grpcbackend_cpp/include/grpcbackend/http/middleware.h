#pragma once
#include "connection.h"
#include <functional>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class middleware {
			public:
				typedef std::shared_ptr<middleware> ptr;
				virtual ~middleware() {}
				virtual void on_request(connection_ptr con, std::function<void(connection_ptr)> next) = 0;
			};
			using middleware_ptr = middleware::ptr;
		}
	}
}