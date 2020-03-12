#include "rewrite.h"
#include "request_forward.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
                rewrite::rewrite(std::function<std::string(request&, response&)> f)
                    : func(f)
                {}
                // Geerbt über middleware
                void rewrite::handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) {
                    auto res = func(req, resp);
                    if (!res.empty()) {
                        class my_request : public request_forward {
                            std::string _resource;
                        public:
                            my_request(request& porig, const std::string& resource)
                                : request_forward(porig), _resource(resource)
                            { }
                            const std::string& get_resource() const override { return _resource; }
                        };
                        my_request m(req, res);
                        next(m, resp);
                    }
                    else
                        next(req, resp);
                }
			}
		}
	}
}