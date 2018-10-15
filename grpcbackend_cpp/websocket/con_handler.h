#pragma once
#include "connection.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace websocket {
			class con_handler
			{
			public:
				virtual ~con_handler() {}

				virtual void on_connect(connection_ptr con) = 0;
				virtual void on_message(connection_ptr con, bool bin, const std::string& msg) = 0;
				virtual void on_disconnect(connection_ptr con) = 0;
			};
		}
	}
}