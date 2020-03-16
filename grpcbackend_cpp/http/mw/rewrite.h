#pragma once
#include "../middleware.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				class rewrite : public middleware {
					std::function<std::string(connection_ptr)> func;
				public:
					explicit rewrite(std::function<std::string(connection_ptr)> f);
					// Geerbt über middleware
					virtual void on_request(connection_ptr, std::function<void(connection_ptr)> next) override;
				};
			}
		}
	}
}