#include "logger.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				logger::logger(ttl::logger& log, ttl::loglevel lvl, ttl::logmodule module)
                    : _log(log), _lvl(lvl), _module(module)
                {}

				void logger::on_request(connection_ptr con, std::function<void(connection_ptr)> next) {
					_log(_lvl, _module.name)
						<< con->get_method() << " "
						<< con->get_resource() << " "
						<< con->get_protocol() << " "
						<< (con->is_encrypted() ? "tls" : "plain") << " "
						<< con->get_client_ip() << ":" << con->get_client_port();
					next(con);
				}
			}
		}
	}
}