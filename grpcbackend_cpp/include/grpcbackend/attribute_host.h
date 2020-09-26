#pragma once
#include <grpcbackend/attribute.h>
#include <memory>
#include <map>
#include <typeindex>

namespace thalhammer {
	namespace grpcbackend {
		// Base class of all attributes used to make dynamic_pointer_case work
		class attribute_host {
		public:
			using attribute_map = std::map<std::type_index, std::shared_ptr<attribute>>;
			virtual ~attribute_host() {}
			virtual const attribute_map& get_attributes() const = 0;

			template<typename T>
			typename std::enable_if<std::is_base_of<attribute, T>::value, void>::type
				set_attribute(std::shared_ptr<T> attr) {
				attribute_map& map = get_attributes();
				map[typeid(T)] = attr;
			}

			template<typename T>
			typename std::enable_if<std::is_base_of<attribute, T>::value, bool>::type
				has_attribute() const {
				return get_attributes().count(typeid(T)) != 0;
			}

			template<typename T>
			typename std::enable_if<std::is_base_of<attribute, T>::value, std::shared_ptr<T>>::type
				get_attribute() const {
				if (!has_attribute<T>())
					return nullptr;
				return std::dynamic_pointer_cast<T>(get_attributes().at(typeid(T)));
			}
		protected:
			virtual attribute_map& get_attributes() = 0;
		};
	}
}