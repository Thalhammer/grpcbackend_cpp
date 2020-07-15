#include <grpcbackend/http/mw/rewrite.h>
#include <grpcbackend/http/mw/connection_forward.h>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
                rewrite::rewrite(std::function<std::string(connection_ptr)> f)
                    : func(f)
                {}
                // Geerbt über middleware
                void rewrite::on_request(connection_ptr con, std::function<void(connection_ptr)> next) {
                    auto res = func(con);
                    if (!res.empty()) {
                        class my_connection : public connection_forward {
                            std::string _resource;
                        public:
                            my_connection(connection_ptr con, const std::string& resource)
                                : connection_forward(con), _resource(resource)
                            { }
                            const std::string& get_resource() const override { return _resource; }
                        };
                        auto m = std::make_shared<my_connection>(con, res);
                        next(m);
                    }
                    else
                        next(con);
                }
			}
		}
	}
}