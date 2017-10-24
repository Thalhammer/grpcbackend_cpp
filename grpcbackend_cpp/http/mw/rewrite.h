#pragma once
#include "../middleware.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				class rewrite : public middleware {
					std::function<std::string(request&, response&)> func;
				public:
					rewrite(std::function<std::string(request&, response&)> f);
					// Geerbt über middleware
					virtual void handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) override;
				};
			}
		}
	}
}