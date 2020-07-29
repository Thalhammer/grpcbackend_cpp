#pragma once
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>

class TypescriptClientCodeGenerator : public google::protobuf::compiler::CodeGenerator {
	public:
		TypescriptClientCodeGenerator() {}
		virtual ~TypescriptClientCodeGenerator() {}
		virtual bool Generate(
			const google::protobuf::FileDescriptor* file,
			const std::string& parameter,
			google::protobuf::compiler::GeneratorContext* context,
			std::string* error) const;
};