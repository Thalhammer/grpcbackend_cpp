#include "logger.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				logger::logger(ttl::logger& log, ttl::loglevel lvl, ttl::logmodule module)
                    : _log(log), _lvl(lvl), _module(module)
                {}

				void logger::handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) {
					_log(_lvl, _module.name)
						<< req.get_method() << " "
						<< req.get_resource() << " "
						<< req.get_protocol() << " "
						<< (req.is_encrypted() ? "tls" : "plain") << " "
						<< req.get_client_ip() << ":" << req.get_client_port();
					next(req, resp);
				}
			}
		}
	}
}