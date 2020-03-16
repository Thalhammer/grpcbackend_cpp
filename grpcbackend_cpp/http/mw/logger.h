#pragma once
#include <ttl/logger.h>
#include "../middleware.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				class logger : public middleware {
					ttl::logger& _log;
					ttl::loglevel _lvl;
					ttl::logmodule _module;
				public:
					logger(ttl::logger& log, ttl::loglevel lvl = ttl::loglevel::DEBUG, ttl::logmodule module = ttl::logmodule("http"));
					// Geerbt über middleware
					virtual void on_request(connection_ptr con, std::function<void(connection_ptr)> next) override;
				};
			}
		}
	}
}