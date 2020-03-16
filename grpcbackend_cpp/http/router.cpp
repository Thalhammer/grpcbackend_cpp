#include "router.h"
#include <vector>
#include <regex>
#include <ttl/string_util.h>
#include "route_params.h"
#include "mw/rewrite.h"
#include "mw/logger.h"
#include "mw/mime_type.h"
#include "route/filesystem_route_handler.h"
#include "http_exception.h"

using namespace std::string_literals;

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			struct router::route_entry {
				std::regex regex;
				std::vector<std::string> keys;
				std::vector<middleware_ptr> middleware;
				route_handler_ptr handler;
				std::string uri;
			};

			struct router::routing_info {
				std::multimap<pos, middleware_ptr> middleware;
				std::map<std::string, std::vector<std::shared_ptr<route_entry>>> routes;
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

			router& router::route(route_handler_ptr handler, std::vector<middleware_ptr> mw)
			{
				std::lock_guard<std::mutex> lck(_routing_update_mtx);
				// Get local pointer
				std::shared_ptr<const routing_info> pinfo = std::atomic_load(&_routing);
				// Copy it
				auto info = std::make_shared<routing_info>(*pinfo);
				for (auto& method : handler->get_methods()) {
					for (auto& uri : handler->get_routes()) {
						std::shared_ptr<route_entry> entry = std::make_shared<route_entry>();
						auto parts = parse_route(uri);
						entry->uri = uri;
						entry->regex = std::regex(build_route_regex(parts, entry->keys));
						entry->handler = handler;
						entry->middleware = mw;
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

			// TODO: Do we need exception handling in here ?
			void router::mw_helper_global(connection_ptr con, std::shared_ptr<const routing_info> info, std::multimap<pos, middleware_ptr>::const_iterator it) {
				if (it != info->middleware.cend()) {
					auto mw = it->second;
					it++;
					mw->on_request(con, std::bind(&router::mw_helper_global, std::placeholders::_1, info, it));
				}
				else {
					do_routing(con, info);
				}
			}

			void router::mw_helper_local(connection_ptr con, std::shared_ptr<const route_entry> entry, std::vector<middleware_ptr>::const_iterator it) {
				if (it != entry->middleware.cend()) {
					auto mw = *it;
					it++;
					mw->on_request(con, std::bind(&router::mw_helper_local, std::placeholders::_1, entry, it));
				}
				else {
					entry->handler->on_request(con);
				}
			}

			void router::on_request(connection_ptr con)
			{
				std::shared_ptr<const routing_info> info = std::atomic_load(&_routing);
				auto params = std::make_shared<route_params>();
				con->set_attribute(params);
				params->info = info;
				try {
					if (!info->middleware.empty()) {
						mw_helper_global(con, info, info->middleware.cbegin());
					}
					else {
						do_routing(con, info);
					}
				} catch(...) {
					handle_exception(con, std::current_exception());
				}
			}

			router& router::rewrite(std::function<std::string(connection_ptr con)> fn, pos p)
			{
				return this->use(std::make_shared<mw::rewrite>(fn), p);
			}

			router& router::rewrite(std::regex reg, std::string fmt, pos p)
			{
				return this->rewrite([reg, fmt](connection_ptr con) -> std::string {
					std::smatch match;
					if(std::regex_match(con->get_resource(), match, reg))
					{
						return match.format(fmt);
					}
					return "";
				}, p);
			}

			router& router::serve_dir(const std::string& uri, const std::string& dir, bool allow_listing)
			{
				return this->serve_dir(uri, dir, allow_listing, std::make_shared<osfilesystem>());
			}

			router& router::serve_dir(const std::string& uri, const std::string& dir, bool allow_listing, std::shared_ptr<ifilesystem> fs) {
				return this->route(std::make_shared<http::route::filesystem_route_handler>(dir, allow_listing, uri + "{fs_path:.*}", fs));
			}

			router& router::log_requests(ttl::logger& stream, pos p)
			{
				return this->use(std::make_shared<http::mw::logger>(stream), p);
			}

			router& router::mime_detector(const std::string& mimefile, pos p)
			{
				return this->use(std::make_shared<http::mw::mime_type>(mimefile), p);
			}

			router& router::mime_detector(std::istream& stream, pos p)
			{
				return this->use(std::make_shared<http::mw::mime_type>(stream), p);
			}

			void router::handle_exception(connection_ptr con, std::exception_ptr ptr)
			{
				auto params = con->get_attribute<route_params>();
				if (!params) std::rethrow_exception(ptr);
				try {
					std::rethrow_exception(ptr);
				}
				catch (const http_exception& ex) {
					if(ex.get_code() == 404) {
						if(params->info->notfound_handler)
							params->info->notfound_handler(con);
						else con->set_status(404);
					} else if(ex.get_code() == 500) {
						if(params->info->error_handler) params->info->error_handler(con, ex.get_message(), std::current_exception());
						else con->set_status(500, ex.get_message());
					} else {
						con->set_status(ex.get_code(), ex.get_message());
					}
				}
				catch (std::exception& e) {
					if(params->info->debug_mode) con->set_header("X-Exception", "std::exception("s + e.what() + ")");
					if(params->info->error_handler) params->info->error_handler(con, e.what(), ptr);
					else con->set_status(500, "Internal error");
				}
				catch (...) {
					if(params->info->debug_mode) con->set_header("X-Exception", "unknown");
					if(params->info->error_handler) params->info->error_handler(con, "Internal error", ptr);
					else con->set_status(500, "Internal error");
				}
				if(!con->is_finished()) con->end();
			}

			void router::do_routing(connection_ptr con, std::shared_ptr<const routing_info> info)
			{
				auto& method = con->get_method();
				auto& uri = con->get_parsed_uri().get_path();

				auto& map = info->routes;

				if (map.count(method) == 0) {
					if(info->notfound_handler)
						info->notfound_handler(con);
					else con->set_status(404);
					return;
				}
				auto& entries = map.at(method);
				std::shared_ptr<const route_entry> entry;
				std::smatch matches;
				for (auto& e : entries) {
					if (std::regex_match(uri, matches, e->regex)) {
						entry = e;
						std::shared_ptr<route_params> params = con->get_attribute<route_params>();
						if(!params) {
							params = std::make_shared<route_params>();
							con->set_attribute(params);
						}

						if (matches.size() != e->keys.size() + 1) {
							if (info->debug_mode)
								con->set_header("X-Exception", "Regex size check failed");
							if(info->error_handler) info->error_handler(con, "Internal error", nullptr);
							else con->set_status(500, "Internal error");
							return;
						}

						for (size_t i = 0; i < e->keys.size(); i++) {
							params->params[e->keys[i]] = matches[i + 1].str();
						}

						params->info = info;
						params->entry = e;
						break;
					}
				}

				if (!entry) {
					if(info->notfound_handler)
						info->notfound_handler(con);
					else con->set_status(404);
					return;
				}

				try {
					if (!entry->middleware.empty()) {
						// Execute middleware chain
						mw_helper_local(con, entry, entry->middleware.cbegin());
					}
					else {
						entry->handler->on_request(con);
					}
				} catch(...) {
					handle_exception(con, std::current_exception());
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
				auto parts = ttl::string::split(uri, std::string("/"));
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

				std::string res = "^\\/" + ttl::string::join(parts, "\\/"s) + "$";
				return res;
			}

			const std::string & route_params::get_selected_route() const
			{
				return entry->uri;
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
		}
	}
}