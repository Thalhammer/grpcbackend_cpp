#pragma once
#include <ttl/logger.h>
#include "../middleware.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				class logger : public middleware {
					thalhammer::logger& _log;
					thalhammer::loglevel _lvl;
					thalhammer::logmodule _module;
				public:
					logger(thalhammer::logger& log, thalhammer::loglevel lvl = thalhammer::loglevel::DEBUG, thalhammer::logmodule module = thalhammer::logmodule("http"));
					// Geerbt über middleware
					virtual void handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) override;
				};
			}
		}
	}
}