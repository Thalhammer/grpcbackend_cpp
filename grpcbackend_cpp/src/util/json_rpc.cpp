#include <grpcbackend/util/json_rpc.h>

namespace thalhammer {
    namespace grpcbackend {
        namespace util {
            struct my_call_context : public json_rpc::call_context {
                picojson::object m_request;
                bool m_is_batched;
                bool m_prefer_sync;
                std::shared_ptr<void> m_context;
                std::function<void(const picojson::value&)> m_responsefn;
            };

            void json_rpc::call_context::respond(picojson::value result)
            {
                auto impl = static_cast<my_call_context*>(this);
                // NOTE: responsefn should be the last call as it might call delete on this object
                if(is_notification()) return impl->m_responsefn(picojson::value());
                picojson::object msg;
                msg["jsonrpc"] = picojson::value("2.0");
                msg["id"] = impl->m_request["id"];
                msg["result"] = result;
                // NOTE: responsefn should be the last call as it might call delete on this object
                return impl->m_responsefn(picojson::value(msg));
            }

            void json_rpc::call_context::error(int code, const std::string& message, picojson::value data)
            {
                auto impl = static_cast<my_call_context*>(this);
                // NOTE: responsefn should be the last call as it might call delete on this object
                if(is_notification()) return impl->m_responsefn(picojson::value());
                picojson::object error;
                error["code"] = picojson::value((int64_t)code);
                error["message"] = picojson::value(message);
                if(!data.is<picojson::null>()) error["data"] = data;
                picojson::object msg;
                msg["jsonrpc"] = picojson::value("2.0");
                msg["id"] = impl->m_request["id"];
                msg["error"] = picojson::value(error);
                // NOTE: responsefn should be the last call as it might call delete on this object
                return impl->m_responsefn(picojson::value(msg));
            }

            const picojson::value& json_rpc::call_context::get_id() const noexcept
            {
                const static picojson::value empty_id;
                auto impl = static_cast<const my_call_context*>(this);
                if(impl->m_request.count("id") != 0) return impl->m_request.at("id");
                return empty_id;
            }

            bool json_rpc::call_context::is_notification() const noexcept
            {
                auto impl = static_cast<const my_call_context*>(this);
                return impl->m_request.count("id") == 0;
            }

            bool json_rpc::call_context::is_batched() const noexcept
            {
                auto impl = static_cast<const my_call_context*>(this);
                return impl->m_is_batched;
            }

            const picojson::value& json_rpc::call_context::get_params() const noexcept
            {
                const static picojson::value empty_params;
                auto impl = static_cast<const my_call_context*>(this);
                if(impl->m_request.count("params") != 0) return impl->m_request.at("params");
                return empty_params;
            }

            const std::string& json_rpc::call_context::get_method() const noexcept
            {
                auto impl = static_cast<const my_call_context*>(this);
                return impl->m_request.at("method").get<std::string>();
            }

            bool json_rpc::call_context::prefer_sync() const noexcept
            {
                auto impl = static_cast<const my_call_context*>(this);
                return impl->m_prefer_sync;
            }

            std::shared_ptr<void> json_rpc::call_context::get_context() const noexcept
            {
                auto impl = static_cast<const my_call_context*>(this);
                return impl->m_context;
            }

            void json_rpc::handle_message(const std::string& msg, std::function<void(const std::string&)> responsefn, std::shared_ptr<void> context, bool prefer_sync) const
            {
                picojson::value msgval;
                auto err = picojson::parse(msgval, msg);
                if(!err.empty()) {
                    picojson::object obj;
                    picojson::object error;
                    error["code"] = picojson::value((int64_t)ERROR_PARSE_ERROR);
                    error["message"] = picojson::value("Failed to parse json");
                    obj["error"] = picojson::value(error);
                    obj["jsonrpc"] = picojson::value("2.0");
                    return responsefn(picojson::value(obj).serialize());
                }
                if(msgval.is<picojson::object>()) {
                    auto ctx = std::make_shared<my_call_context>();
                    ctx->m_is_batched = false;
                    ctx->m_request = msgval.get<picojson::object>();
                    ctx->m_prefer_sync = prefer_sync;
                    ctx->m_context = context;
                    ctx->m_responsefn = [responsefn](const picojson::value& obj){
                        if(obj.is<picojson::null>()) return;
                        responsefn(obj.serialize());
                    };
                    handle_call(ctx);
                } else if(msgval.is<picojson::array>()) {
                    struct call_context_array {
                        std::vector<my_call_context> contexts;
                        std::map<size_t, picojson::value> result_map;
                        std::function<void(const std::string&)> responsefn;

                        void handle_result(size_t ctx, const picojson::value& value) {
                            result_map[ctx] = value;
                            if(result_map.size() >= contexts.size()) {
                                picojson::array results;
                                for(size_t i = 0; i < contexts.size(); i++) {
                                    if(contexts[i].is_notification()) continue;
                                    auto& entry = result_map.at(i);
                                    if(entry.is<picojson::object>()) results.push_back(entry);
                                }
                                responsefn(picojson::value(results).serialize());
                            }
                        }
                    };
                    auto ctx = std::make_shared<call_context_array>();
                    ctx->responsefn = responsefn;
                    for(auto& req : msgval.get<picojson::array>()) {
                        ctx->contexts.push_back({});
                        ctx->contexts.back().m_is_batched = true;
                        ctx->contexts.back().m_request = req.get<picojson::object>();
                        ctx->contexts.back().m_prefer_sync = prefer_sync;
                        ctx->contexts.back().m_context = context;
                        ctx->contexts.back().m_responsefn = [idx = ctx->contexts.size()-1, pctx = ctx.get()](const picojson::value& obj){
                            pctx->handle_result(idx, obj);
                        };
                    }
                    for(auto& e : ctx->contexts) {
                        handle_call(std::shared_ptr<call_context>(ctx, &e));
                    }
                } else {
                    picojson::object obj;
                    picojson::object error;
                    error["code"] = picojson::value((int64_t)ERROR_INVALID_REQUEST);
                    error["message"] = picojson::value("Not a valid request");
                    obj["error"] = picojson::value(error);
                    obj["jsonrpc"] = picojson::value("2.0");
                    return responsefn(picojson::value(obj).serialize());
                }
            }

            void json_rpc::register_method(const std::string& name, handler_function_t fn)
            {
                if(m_handlers.count(name) != 0)
                    throw std::runtime_error("function already registered");
                m_handlers[name] = fn;
            }

            void json_rpc::handle_call(std::shared_ptr<call_context> ctx) const
            {
                auto& fn = ctx->get_method();
                auto it = m_handlers.find(fn);
                if(it == m_handlers.end()) {
                    return ctx->error(ERROR_METHOD_NOT_FOUND, "Could not find the requested method");
                } else {
                    return it->second(ctx);
                }
            }
        }
    }
}