#pragma once
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

class GrpcbackendServerCodeGenerator : public google::protobuf::compiler::CodeGenerator {
	public:
		GrpcbackendServerCodeGenerator() {}
		virtual ~GrpcbackendServerCodeGenerator() {}
		virtual bool Generate(
			const google::protobuf::FileDescriptor* file,
			const std::string& parameter,
			google::protobuf::compiler::GeneratorContext* context,
			std::string* error) const;
};