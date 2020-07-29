#pragma once
#include <google/protobuf/descriptor.pb.h>

inline std::vector<const google::protobuf::EnumDescriptor*> get_all_enums(const google::protobuf::Descriptor* d) {
	std::vector<const google::protobuf::EnumDescriptor*> res;
	for(int i=0; i<d->enum_type_count(); i++) res.push_back(d->enum_type(i));

	for(int i=0; i<d->nested_type_count(); i++) {
		auto r = get_all_enums(d->nested_type(i));
		for(auto e : r) res.push_back(e);
	}

	return res;
}

inline std::vector<const google::protobuf::EnumDescriptor*> get_all_enums(const google::protobuf::FileDescriptor* d) {
	std::vector<const google::protobuf::EnumDescriptor*> res;
	for(int i=0; i<d->enum_type_count(); i++) res.push_back(d->enum_type(i));

	for(int i=0; i<d->message_type_count(); i++) {
		auto r = get_all_enums(d->message_type(i));
		for(auto e : r) res.push_back(e);
	}

	return res;
}

inline std::vector<const google::protobuf::Descriptor*> get_all_messages(const google::protobuf::Descriptor* d) {
	std::vector<const google::protobuf::Descriptor*> res;

	for(int i=0; i<d->nested_type_count(); i++) {
		auto r = get_all_messages(d->nested_type(i));
		for(auto e : r) res.push_back(e);
	}
	res.push_back(d);

	return res;
}

inline std::vector<const google::protobuf::Descriptor*> get_all_messages(const google::protobuf::FileDescriptor* d) {
	std::vector<const google::protobuf::Descriptor*> res;

	for(int i=0; i<d->message_type_count(); i++) {
		auto r = get_all_messages(d->message_type(i));
		for(auto e : r) res.push_back(e);
	}

	return res;
}

template<typename T>
inline std::string get_clean_name(const T* e) {
	auto name = e->name();
	for(auto parent = e->containing_type(); parent; parent = parent->containing_type()) name = parent->name() + "_" + name;
	return name;
}