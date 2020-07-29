#include <google/protobuf/compiler/plugin.h>
#include "grpcbackend_server_generator.h"
#include "typescript_client_generator.h"
#include <ttl/string_util.h>

class MainCodeGenerator : public google::protobuf::compiler::CodeGenerator {
	public:
		MainCodeGenerator() {}
		virtual ~MainCodeGenerator() {}
		virtual bool Generate(
			const google::protobuf::FileDescriptor* file,
			const std::string& parameter,
			google::protobuf::compiler::GeneratorContext* context,
			std::string* error) const
		{
			auto params = ttl::string::split<std::string>(parameter, ";", 2);
			if(params[0].empty()) params = { "grpcbackend_server", "" };
			if(params.size() == 1) params.push_back("");
			if(params[0] == "grpcbackend_server") {
				GrpcbackendServerCodeGenerator gen;
				return gen.Generate(file, params[1], context, error);
			} else if(params[0] == "typescript_client") {
				TypescriptClientCodeGenerator gen;
				return gen.Generate(file, params[1], context, error);
			} else {
				*error="Generator " + params[0] + " not found";
				return false;
			}
		}
};

int main(int argc, char** argv)
{
	MainCodeGenerator generator;
	return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}

