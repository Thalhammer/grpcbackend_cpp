#pragma once
#include "middleware.h"
#include "route/function_route_handler.h"
#include <vector>
#include <mutex>
#include <regex>
#include <exception>

namespace thalhammer {
	class logger;
	namespace grpcbackend {
		namespace http {
			class router {
			public:
				enum class pos {
					BEGIN=0,
					DEFAULT=10,
					LAST=20
				};

				typedef std::function<void(request&, response&)> notfound_handler_t;
				typedef std::function<void(request&, response&, const std::string&, std::exception_ptr)> error_handler_t;

				router();
				router& route(route_handler_ptr handler);
				router& use(middleware_ptr m, pos p = pos::DEFAULT);
				router& set_debug_mode(bool b);
				router& notfound(notfound_handler_t fn);
				router& error(error_handler_t fn);

				void handle_request(request& req, response& resp);

				// Shortcut methods for default middleware/routehandlers
				// Add a function handler
				template<typename T>
				router& route(const std::string& method, const std::string& uri, T handler) {
					return route(std::make_shared<route::function_route_handler<T>>(handler, uri, method));
				}
				// Add a rewrite middleware
				router& rewrite(std::function<std::string(request&, response&)> fn, pos p = pos::DEFAULT);
				router& rewrite(std::regex reg, std::string fmt, pos p = pos::DEFAULT);
				router& rewrite(const std::string& reg, std::string fmt, pos p = pos::DEFAULT) {
					return this->rewrite(std::regex(reg), fmt, p);
				}
				// Serve a static directory
				router& serve_dir(const std::string& uri, const std::string& dir, bool allow_listing = true);
				// Add logging middleware for specified stream
				router& log_requests(thalhammer::logger& stream, pos p = pos::BEGIN);
				// Add content-type header based on uri file extension
				router& mime_detector(const std::string& mimefile = "mime.types", pos p = pos::BEGIN);
			private:
				// Routing information
				struct routing_info;
				// Route matching information
				struct route_match_info;
				// This only prevents concurrent modification.
				// Worker threads do not need to lock it because we read and write the shared_ptr using atomic operations
				std::mutex _routing_update_mtx;
				std::shared_ptr<routing_info> _routing;
				static void do_routing(request& req, response& resp, std::shared_ptr<const routing_info> info);
				static std::vector<route_match_info> parse_route(const std::string& uri);
				static std::string build_route_regex(const std::vector<route_match_info>& info, std::vector<std::string>& keys);
			};
		}
	}
}