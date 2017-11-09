#include "router.h"
#include <vector>
#include <regex>
#include <ttl/string_util.h>
#include "route_params.h"
#include "mw/rewrite.h"
#include "route/filesystem_route_handler.h"
#include "mw/logger.h"
#include "mw/mime_type.h"
#include "notfound_exception.h"

using namespace std::string_literals;

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			struct route_entry {
				std::regex regex;
				std::vector<std::string> keys;
				route_handler_ptr handler;
				std::string uri;
			};

			struct router::routing_info {
				std::multimap<pos, middleware_ptr> middleware;
				std::map<std::string, std::vector<route_entry>> routes;
				bool debug_mode;
				notfound_handler_t notfound_handler;
				error_handler_t error_handler;
			};

			router::router()
			{
				auto info = std::make_shared<routing_info>();
				info->debug_mode = false;
				std::atomic_store(&_routing, info);
			}

			router& router::route(route_handler_ptr handler)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				for (auto& method : handler->get_methods()) {
					for (auto& uri : handler->get_routes()) {
						route_entry entry;
						auto parts = parse_route(uri);
						entry.uri = uri;
						entry.regex = std::regex(build_route_regex(parts, entry.keys));
						entry.handler = handler;
						// Modify
						info->routes[method].push_back(entry);
					}
				}
				// Update class instance
				std::atomic_store(&_routing, info);

				return *this;
			}

			router& router::use(middleware_ptr m, pos p)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				// Modify
				info->middleware.insert({ p, m });
				// Copy to class instance
				std::atomic_store(&_routing, info);

				return *this;
			}

			router& router::set_debug_mode(bool b)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				// Modify
				info->debug_mode = b;
				// Copy to class instance
				std::atomic_store(&_routing, info);

				return *this;
			}

			router& router::notfound(notfound_handler_t fn)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				// Modify
				info->notfound_handler = fn;
				// Copy to class instance
				std::atomic_store(&_routing, info);

				return *this;
			}

			router& router::error(error_handler_t fn)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				// Modify
				info->error_handler = fn;
				// Copy to class instance
				std::atomic_store(&_routing, info);

				return *this;
			}

			void router::handle_request(request & req, response & resp)
			{
				std::shared_ptr<const routing_info> info = std::atomic_load(&_routing);
				if (!info->middleware.empty()) {
					auto it = info->middleware.cbegin();
					auto end = info->middleware.cend();

					std::function<void(request&, response&)> next = [&it, end, this, &next, info](request& req, response& resp) -> void {
						if (it != end) {
							auto mw = it->second;
							it++;
							mw->handle_request(req, resp, next);
						}
						else {
							this->do_routing(req, resp, info);
						}
					};

					next(req, resp);
				}
				else {
					do_routing(req, resp, info);
				}
			}

			router& router::rewrite(std::function<std::string(request&, response&)> fn, pos p)
			{
				return this->use(std::make_shared<mw::rewrite>(fn), p);
			}

			router& router::rewrite(std::regex reg, std::string fmt, pos p)
			{
				return this->rewrite([reg, fmt](request& req, response&) -> std::string {
					std::smatch match;
					if(std::regex_match(req.get_resource(), match, reg))
					{
						return match.format(fmt);
					}
					return "";
				}, p);
			}

			router& router::serve_dir(const std::string& uri, const std::string& dir, bool allow_listing)
			{
				return this->route(std::make_shared<http::route::filesystem_route_handler>(dir, allow_listing, uri + "{fs_path:.*}"));
			}

			router& router::log_requests(thalhammer::logger& stream, pos p)
			{
				return this->use(std::make_shared<http::mw::logger>(stream), p);
			}

			router& router::mime_detector(const std::string& mimefile, pos p)
			{
				return this->use(std::make_shared<http::mw::mime_type>(mimefile), p);
			}

			void router::do_routing(request & req, response & resp, std::shared_ptr<const routing_info> info)
			{
				auto& method = req.get_method();
				auto& uri = req.get_parsed_uri().get_path();

				auto& map = info->routes;

				if (map.count(method) == 0) {
					if(info->notfound_handler)
						info->notfound_handler(req, resp);
					else resp.set_status(404);
					return;
				}
				auto& entries = map.at(method);
				route_handler_ptr handler;
				std::smatch matches;
				for (auto& e : entries) {
					if (std::regex_match(uri, matches, e.regex)) {
						handler = e.handler;
						std::shared_ptr<route_params> params;
						if (req.has_attribute<route_params>()) params = req.get_attribute<route_params>();
						else {
							params = std::make_shared<route_params>();
							req.set_attribute(params);
						}

						if (matches.size() != e.keys.size() + 1) {
							if (info->debug_mode)
								resp.set_header("X-Exception", "Regex size check failed");
							if(info->error_handler) info->error_handler(req, resp, "Internal error", nullptr);
							else resp.set_status(500, "Internal error");
							return;
						}

						for (size_t i = 0; i < e.keys.size(); i++) {
							params->set_param(e.keys[i], matches[i + 1].str());
						}
						params->set_selected_route(e.uri);
						break;
					}
				}

				if (!handler) {
					if(info->notfound_handler)
						info->notfound_handler(req, resp);
					else resp.set_status(404);
					return;
				}

				try {
					handler->handle_request(req, resp);
				}
				catch (const notfound_exception& ex) {
					if(info->notfound_handler)
						info->notfound_handler(req, resp);
					else resp.set_status(404);
				}
				catch (...) {
					if (info->debug_mode) {
						try {
							throw;
						}
						catch (std::exception& e) {
							resp.set_header("X-Exception", "std::exception("s + e.what() + ")");
						}
						catch (...) {
							resp.set_header("X-Exception", "unknown");
						}
					}
					if(info->error_handler) info->error_handler(req, resp, "Internal error", std::current_exception());
					else resp.set_status(500, "Internal error");
				}
			}

			struct router::route_match_info {
				enum class type_t {
					equals,
					match_all,
					regex
				};
				type_t type;
				std::string param_name;
				std::string equals_str;
			};

			std::vector<router::route_match_info> router::parse_route(const std::string & uri)
			{
				auto parts = string::split(uri, std::string("/"));
				std::vector<router::route_match_info> res;
				for (auto& part : parts) {
					route_match_info info;
					if (part[0] == '{' && part[part.size() - 1] == '}') {
						part = part.substr(1, part.size() - 2);
						auto pos = part.find(':');
						if (pos == std::string::npos) {
							info.type = route_match_info::type_t::match_all;
						}
						else {
							info.type = route_match_info::type_t::regex;
							// Also set equals_string to allow combining
							info.equals_str = part.substr(pos + 1);
						}
						info.param_name = part.substr(0, pos);
					}
					else if (part[0] == ':') {
						info.type = route_match_info::type_t::match_all;
						info.param_name = part.substr(1);
					}
					else {
						info.type = route_match_info::type_t::equals;
						info.equals_str = part;
					}
					res.push_back(info);
				}
				return res;
			}

			std::string router::build_route_regex(const std::vector<route_match_info>& info, std::vector<std::string>& keys)
			{
				std::vector<std::string> parts;
				for (auto& e : info) {
					if (e.type == route_match_info::type_t::equals) {
						parts.push_back(e.equals_str);
					}
					else if (e.type == route_match_info::type_t::match_all) {
						if (e.param_name.empty())
							parts.push_back("(?:[^\\/]+?)");
						else {
							parts.push_back("([^\\/]+?)");
							keys.push_back(e.param_name);
						}
					}
					else {
						if (e.param_name.empty())
							parts.push_back("(?:" + e.equals_str + "?)");
						else {
							parts.push_back("("s + e.equals_str + "?)"s);
							keys.push_back(e.param_name);
						}
					}
				}

				std::string res = "^\\/" + string::join(parts, "\\/"s) + "$";
				return res;
			}

			route_params::~route_params()
			{
			}

			const std::string & route_params::get_selected_route() const
			{
				return selected_route;
			}

			void route_params::set_selected_route(const std::string & route)
			{
				selected_route = route;
			}

			bool route_params::has_param(const std::string & key) const
			{
				return params.count(key) != 0;
			}

			const std::string & route_params::get_param(const std::string & key) const
			{
				static const std::string empty = "";
				if (has_param(key))
					return params.at(key);
				else return empty;
			}

			void route_params::set_param(const std::string & key, const std::string & val)
			{
				params[key] = val;
			}
		}
	}
}