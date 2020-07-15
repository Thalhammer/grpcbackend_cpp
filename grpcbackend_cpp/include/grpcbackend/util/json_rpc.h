#pragma once
#include <grpcbackend/picojson.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace thalhammer {
    namespace grpcbackend {
        namespace util {
            struct json_rpc_call_context {
                void respond(picojson::value result);
                void respond(picojson::object obj) { return respond(picojson::value(obj)); }
                void respond(picojson::array arr) { return respond(picojson::value(arr)); }
                void error(int code, const std::string& message, picojson::value data = {});

                const picojson::value& get_id() const noexcept;
                bool is_notification() const noexcept;
                bool is_batched() const noexcept;
                const picojson::value& get_params() const noexcept;
                const std::string& get_method() const noexcept;

                bool prefer_sync() const noexcept;
                std::shared_ptr<void> get_context() const noexcept;
            };
            class json_rpc {
            public:
                enum error {
                    ERROR_PARSE_ERROR = -32700,
                    ERROR_INVALID_REQUEST = -32600,
                    ERROR_METHOD_NOT_FOUND = -32601,
                    ERROR_INVALID_PARAMS = -32602,
                    ERROR_INTERNAL = -32603,
                    ERROR_SERVER_START = -32000,
                    ERROR_SERVER_END = -32099
                };

                typedef json_rpc_call_context call_context;
                
                typedef std::function<void(std::shared_ptr<call_context>)> handler_function_t;

                void handle_message(const std::string& msg, std::function<void(const std::string&)> responsefn, std::shared_ptr<void> context = nullptr, bool prefer_sync = false) const;

                void register_method(const std::string& name, handler_function_t fn);
            private:
                std::unordered_map<std::string, handler_function_t> m_handlers;

                void handle_call(std::shared_ptr<call_context> ctx) const;
            };
        }
    }
}