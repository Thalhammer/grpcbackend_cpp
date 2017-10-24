#pragma once
#include "../route_handler.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace route {
				class filesystem_route_handler : public route_handler {
					std::set<std::string> methods;
					std::set<std::string> routes;
					std::string path;
					bool allow_listing;

					static std::string last_modified(time_t date);
					static std::string format_size(std::streamsize size);
				public:
					filesystem_route_handler(const std::string& ppath, bool pallow_listing, const std::string& proute, const std::string& pmethod = "GET")
						: methods({ pmethod }), routes({ proute }), path(ppath), allow_listing(pallow_listing)
					{}
					filesystem_route_handler(const std::string& ppath, bool pallow_listing, std::set<std::string> proutes, std::set<std::string> pmethods = { "GET" })
						: methods(pmethods), routes(proutes), path(ppath), allow_listing(pallow_listing)
					{}
					virtual std::set<std::string> get_methods() override { return methods; }
					virtual std::set<std::string> get_routes() override { return routes; }
					virtual void handle_request(request& req, response& resp) override;
				};
			}
		}
	}
}