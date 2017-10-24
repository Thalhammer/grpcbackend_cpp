#pragma once
#include "../route_handler.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace route {
				template<typename Func>
				class function_route_handler : public route_handler {
					std::set<std::string> methods;
					std::set<std::string> routes;
					Func handler;
				public:
					function_route_handler(Func phandler, const std::string& proute, const std::string& pmethod = "*")
						: methods({ pmethod }), routes({ proute }), handler(phandler)
					{}
					function_route_handler(Func phandler, std::set<std::string> proutes, std::set<std::string> pmethods = { "*" })
						: methods(pmethods), routes(proutes), handler(phandler)
					{}
					virtual std::set<std::string> get_methods() override { return methods; }
					virtual std::set<std::string> get_routes() override { return routes; }
					virtual void handle_request(request& req, response& resp) override {
						handler(req, resp);
					}
				};
			}
		}
	}
}