#pragma once
#include <string>
#include <map>
#include <memory>
#include <typeindex>
#include "../attribute.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class response {
			protected:
				std::map<std::type_index, std::shared_ptr<attribute>> _attributes;
			public:
				virtual ~response() {}

				virtual void set_status(int code, const std::string& message = "") = 0;
				virtual void set_header(const std::string& key, const std::string& value, bool replace = false) = 0;
				virtual std::ostream& get_ostream() = 0;

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, void>::type
					set_attribute(std::shared_ptr<T> attr) {
					_attributes[typeid(T)] = attr;
				}

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, bool>::type
					has_attribute() const {
					return _attributes.count(typeid(T)) != 0;
				}

				template<typename T>
				typename std::enable_if<std::is_base_of<attribute, T>::value, std::shared_ptr<T>>::type
					get_attribute() const {
					if (!has_attribute<T>())
						return nullptr;
					return std::dynamic_pointer_cast<T>(_attributes.at(typeid(T)));
				}

				const std::map<std::type_index, std::shared_ptr<attribute>>& get_attributes() const {
					return _attributes;
				}
			};
		}
	}
}