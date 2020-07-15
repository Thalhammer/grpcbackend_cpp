#pragma once

namespace thalhammer {
	namespace grpcbackend {
		// Base class of all attributes used to make dynamic_pointer_case work
		class attribute {
		public:
			attribute() {}
			virtual ~attribute() {}
		};
	}
}