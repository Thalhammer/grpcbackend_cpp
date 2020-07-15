#pragma once
#include "connection.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class con_handler
			{
			public:
				virtual ~con_handler() {}

				virtual void on_request(connection_ptr con) = 0;
			};
		}
	}
}