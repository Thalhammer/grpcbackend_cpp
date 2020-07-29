#include "grpcbackend_server_generator.h"
#include "jsonrpc.pb.h"
#include "helpers.h"

#include <iostream>
#include <sstream>
#include <algorithm>

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/stl_util.h>
#include <google/protobuf/compiler/plugin.pb.h>

#include <ttl/string_util.h>

static void GenerateHeaderHelpers(std::ostream& ss) {
	ss << "namespace thalhammer {\n";
	ss << "namespace grpcbackend {\n";
	ss << "namespace util {\n";
	ss << "\ttemplate<typename TReq, typename TRes>\n";
	ss << "\tclass json_rpc_query {\n";
	ss << "\t\tstd::shared_ptr<thalhammer::grpcbackend::util::json_rpc_call_context> m_context;\n";
	ss << "\t\tTReq m_request;\n";
	ss << "\tpublic:\n";
	ss << "\t\tjson_rpc_query(std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_call_context> ctx)\n";
	ss << "\t\t\t: m_context(ctx)\n";
	ss << "\t\t{\n";
	ss << "\t\t\tm_request.from_json(m_context->get_params());\n";
	ss << "\t\t}\n";
	ss << "\t\tvoid respond(const TRes& result) {\n";
	ss << "\t\t\tm_context->respond(result.to_json());\n";
	ss << "\t\t}\n";
	ss << "\t\tvoid error(int code, const std::string& message, picojson::value data = {}) {\n";
	ss << "\t\t\tm_context->error(code, message, data);\n";
	ss << "\t\t}\n";
	ss << "\t\n";
	ss << "\t\tconst picojson::value& get_id() const noexcept { return m_context->get_id(); }\n";
	ss << "\t\tbool is_notification() const noexcept { return m_context->is_notification(); }\n";
	ss << "\t\tbool is_batched() const noexcept { return m_context->is_batched(); }\n";
	ss << "\t\tconst std::string& get_method() const noexcept { return m_context->get_method(); }\n";
	ss << "\t\tconst TReq& get_request() const noexcept { return m_request; }\n";
	ss << "\t\n";
	ss << "\t\tbool prefer_sync() const noexcept { return m_context->prefer_sync(); }\n";
	ss << "\t\tstd::shared_ptr<void> get_context() const noexcept { return m_context->get_context(); }\n";
	ss << "\t};\n";
	ss << "}\n";
	ss << "}\n";
	ss << "}\n";
	ss << "\n";
}

static void GenerateHeaderEnum(std::ostream& ss, const google::protobuf::EnumDescriptor* e) {
	ss << "enum class " << get_clean_name(e) << " : int {\n";
	for(int i = 0; i < e->value_count(); i++) {
		auto val = e->value(i);
		ss << "\t" << val->name() << " = " << val->number() << ",\n";
	}
	ss << "};\n\n";
}

static void GenerateHeaderMessage(std::ostream& ss, const google::protobuf::Descriptor* m) {
	ss << "struct " << get_clean_name(m) << " {\n";
	if((m->enum_type_count() + m->nested_type_count()) > 0) {
		for (int i = 0; i < m->enum_type_count(); i++) {
			ss << "\ttypedef " << get_clean_name(m->enum_type(i)) << " " << m->enum_type(i)->name() << ";\n";
		}
		for (int i = 0; i < m->nested_type_count(); i++) {
			ss << "\ttypedef " << get_clean_name(m->nested_type(i)) << " " << m->nested_type(i)->name() << ";\n";
		}
	}
	ss << "\n";

	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		ss << "\t";
		switch(fd->type()) {
		case google::protobuf::FieldDescriptor::TYPE_BOOL:
			ss << "bool"; break;
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			ss << "std::string"; break;
		case google::protobuf::FieldDescriptor::TYPE_ENUM:
			ss << fd->enum_type()->name(); break;
		case google::protobuf::FieldDescriptor::TYPE_GROUP:
			throw std::runtime_error(std::string("not implemented ") + fd->type_name());
		case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
			ss << fd->message_type()->name(); break;
		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
			ss << "double"; break;
		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			ss << "float"; break;
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
			ss << "int32_t"; break;
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
			ss << "int64_t"; break;
		case google::protobuf::FieldDescriptor::TYPE_UINT32:
			ss << "uint32_t"; break;
		case google::protobuf::FieldDescriptor::TYPE_UINT64:
			ss << "uint64_t"; break;
		case google::protobuf::FieldDescriptor::TYPE_STRING:
			ss << "std::string"; break;
		}
		ss << " " << fd->name() << ";\n";
	}
	ss << "\n";
	ss << "\tpicojson::value to_json() const;\n";
	ss << "\tvoid from_json(const picojson::value& val);\n";
	ss << "};\n\n";
}

static void GenerateHeaderService(std::ostream& ss, const google::protobuf::ServiceDescriptor* service) {
	ss << "class " << service->name() << " {\n";
	ss << "protected:\n";
	ss << "\tvoid register_service(thalhammer::grpcbackend::util::json_rpc& rpc);\n";
	ss << "\n";
	for(int m = 0; m < service->method_count(); m++) {
		auto method = service->method(m);
		if(method->server_streaming() || method->client_streaming())
			throw std::runtime_error("jsonrpc does not support streaming calls");
		auto input_type = method->input_type();
		auto output_type = method->output_type();
		ss << "\tvirtual void " << method->name() << "(std::shared_ptr<thalhammer::grpcbackend::util::json_rpc_query<"
			<< get_clean_name(input_type) << "," << get_clean_name(output_type) << ">> context) = 0;\n";
	}
	ss << "};\n";
}

static bool GenerateHeader(const google::protobuf::FileDescriptor* file, const std::string& parameter, google::protobuf::compiler::GeneratorContext* context, std::string* error) 
{
	std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(file->name() + ".h"));
	google::protobuf::io::Printer printer(output.get(), '$');

	std::stringstream ss;

	/* Getting protobuf compiler version */
	google::protobuf::compiler::Version ver;
	context->GetCompilerVersion(&ver);
	ss << "/*\n";
	ss << " *  Generated by grpcbackend_jsonrpc on protobuf " << ver.major() << "." << ver.minor() << "." << ver.patch();
	if(!ver.suffix().empty()) ss << "-" << ver.suffix();
	ss << "\n";
	ss << " */\n";
	ss << "#include <grpcbackend/util/json_rpc.h>\n";

	GenerateHeaderHelpers(ss);

	std::vector<std::string> namespaces;
	if(!file->package().empty()) namespaces = ttl::string::split<std::string>(file->package(), ".");
	for(auto& e : namespaces) ss << "namespace " << e << " {\n";
	ss << "\n";

	for (auto e : get_all_enums(file))
		GenerateHeaderEnum(ss, e);
	
	for (auto m : get_all_messages(file))
		GenerateHeaderMessage(ss, m);

	for (int s = 0; s < file->service_count(); s++)
	{
		auto service = file->service(s);
		GenerateHeaderService(ss, service);
	}

	ss << "\n";
	for(auto& e : namespaces) ss << "} /* namespace " << e << " */\n";
	ss << "\n";

	printer.PrintRaw(ss.str());
	if (printer.failed()) 
	{
		*error = "JsonRPCApiCodeGenerator detected write error.";
		return false;
	}
	return true;
}

static void GenerateBodyHelpers(std::ostream& ss) {
	// Emit helper functions for base64
	ss << "static std::string base64_encode(const std::string& in) {\n";
	ss << "\tconst int pl = 3*in.size()/4;\n";
	ss << "\tstd::string output(pl + 1, '\\0'); //+1 for the terminating null that EVP_EncodeBlock adds on\n";
	ss << "\tconst auto ol = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(const_cast<char*>(output.data())), reinterpret_cast<const unsigned char *>(in.data()), in.size());\n";
	ss << "\tif (pl != ol) throw std::runtime_error(\"base64_encode predicted \" + std::to_string(pl) + \" but we got \" + std::to_string(ol));\n";
	ss << "\treturn output;\n";
	ss << "}\n";
	ss << "\n";
	ss << "static std::string base64_decode(const std::string& in) {\n";
	ss << "\tconst int pl = 4*((in.size()+2)/3);\n";
	ss << "\tstd::string output(pl + 1, '\\0'); //+1 for the terminating null that EVP_EncodeBlock adds on\n";
	ss << "\tconst auto ol = EVP_DecodeBlock(reinterpret_cast<unsigned char *>(const_cast<char*>(output.data())), reinterpret_cast<const unsigned char *>(in.data()), in.size());\n";
	ss << "\tif (pl != ol) throw std::runtime_error(\"base64_decode predicted \" + std::to_string(pl) + \" but we got \" + std::to_string(ol));\n";
	ss << "\treturn output;\n";
	ss << "}\n";
}

static void GenerateBodyEnum(std::ostream& ss, const google::protobuf::EnumDescriptor* e) {
	auto name = get_clean_name(e);
	ss << "static " << name << " string_to_enum_" << name << "(const std::string& str) {\n";
	for(int i = 0; i < e->value_count(); i++) {
		auto val = e->value(i);
		ss << "\tif(str == \"" << val->name() << "\") return " << name << "::" << val->name() << ";\n";
	}
	ss << "\tthrow std::runtime_error(\"invalid value for enum\");\n";
	ss << "}\n\n";
}

static void GenerateBodyToJson(std::ostream& ss, const google::protobuf::Descriptor* m) {
	ss << "picojson::value " << get_clean_name(m) << "::to_json() const {\n";
	ss << "\tpicojson::object obj;\n";
	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		auto name = (fd->has_json_name() && fd->type() != fd->TYPE_ENUM) ? fd->json_name() : fd->name();
		switch(fd->type()) {
		case google::protobuf::FieldDescriptor::TYPE_BOOL:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(" << fd->name() << "));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(base64_encode(" << fd->name() << ")));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_ENUM:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(static_cast<int64_t>(" << fd->name() << ")));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_GROUP:
			throw std::runtime_error(std::string("not implemented ") + fd->type_name());
		case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
			ss << "\tobj.emplace(\"" << name << "\", " << fd->name() << ".to_json());\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(" << fd->name() << "));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
		case google::protobuf::FieldDescriptor::TYPE_UINT32:
		case google::protobuf::FieldDescriptor::TYPE_UINT64:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(static_cast<int64_t>(" << fd->name() << ")));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_STRING:
			ss << "\tobj.emplace(\"" << name << "\", picojson::value(" << fd->name() << "));\n"; break;
		default:
			throw std::logic_error("invalid type");
		}
	}
	ss << "\treturn picojson::value(obj);\n";
	ss << "}\n\n";
}

static void GenerateBodyFromJson(std::ostream& ss, const google::protobuf::Descriptor* m) {
	ss << "void " << get_clean_name(m) << "::from_json(const picojson::value& val) {\n";
	ss << "\tif(!val.is<picojson::object>()) throw std::runtime_error(\"invalid json supplied\");\n";
	ss << "\tauto& obj = val.get<picojson::object>();\n";
	ss << "\n";
	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		auto name = (fd->has_json_name() && fd->type() != fd->TYPE_ENUM) ? fd->json_name() : fd->name();
		if(opts.is_optional()) {
			ss << "\tif(obj.count(\"" << name << "\") > 0) ";
		} else {
			ss << "\tif(obj.count(\"" << name << "\") == 0)\n";
			ss << "\t\tthrow std::runtime_error(\"json missing required member " << name << "\");\n";
		}
		ss << "\t{\n";
		switch(fd->type()) {
		case google::protobuf::FieldDescriptor::TYPE_BOOL:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<bool>()) throw std::runtime_error(\"" << name << " must be a bool\");\n";
			ss << "\t\tthis->" << fd->name() << " = obj.at(\"" << name << "\").get<bool>();\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<std::string>()) throw std::runtime_error(\"" << name << " must be a string\");\n";
			ss << "\t\tthis->" << fd->name() << " = base64_decode(obj.at(\"" << name << "\").get<std::string>());\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_ENUM:
			ss << "\t\tif(obj.at(\"" << name << "\").is<int64_t>())\n";
			ss << "\t\t\tthis->" << fd->name() << " = static_cast<" << get_clean_name(fd->enum_type()) << ">(obj.at(\"" << name << "\").get<int64_t>());\n";
			ss << "\t\telse if(obj.at(\"" << name << "\").is<std::string>())\n";
			ss << "\t\t\tthis->" << fd->name() << " = string_to_enum_" << get_clean_name(fd->enum_type()) << "(obj.at(\"" << name << "\").get<std::string>());\n";
			ss << "\t\telse throw std::runtime_error(\"" << name << " must be a number or string\");\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_GROUP:
			throw std::runtime_error(std::string("not implemented ") + fd->type_name());
		case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<picojson::object>()) throw std::runtime_error(\"" << name << " must be an object\");\n";
			ss << "\t\t" << fd->name() << ".from_json(obj.at(\"" << name << "\"));\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<double>()) throw std::runtime_error(\"" << name << " must be a number\");\n";
			ss << "\t\tthis->" << fd->name() << " = obj.at(\"" << name << "\").get<double>();\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
		case google::protobuf::FieldDescriptor::TYPE_UINT32:
		case google::protobuf::FieldDescriptor::TYPE_UINT64:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<int64_t>()) throw std::runtime_error(\"" << name << " must be a number\");\n";
			ss << "\t\tthis->" << fd->name() << " = obj.at(\"" << name << "\").get<int64_t>();\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_STRING:
			ss << "\t\tif(!obj.at(\"" << name << "\").is<std::string>()) throw std::runtime_error(\"" << name << " must be a string\");\n";
			ss << "\t\tthis->" << fd->name() << " = obj.at(\"" << name << "\").get<std::string>();\n"; break;
		}
		ss << "\t}\n";
	}
	ss << "}\n\n";
}

static void GenerateBodyRegisterService(std::ostream& ss, const google::protobuf::ServiceDescriptor* service) {
	auto& service_opts = service->options().GetExtension(json_rpc_service);

	std::string basename;
	if(service_opts.basename().empty()) {
		basename =  service->file()->package();
		if(!basename.empty()) basename += ".";
		basename += service->name() + ".";
	} else {
		basename = service_opts.basename().empty() + ".";
	}

	ss << "void " << service->name() << "::register_service(thalhammer::grpcbackend::util::json_rpc& rpc) {\n";
	for(int m = 0; m < service->method_count(); m++) {
		auto method = service->method(m);
		auto input_type = method->input_type();
		auto output_type = method->output_type();
		ss << "\trpc.register_method(\"" << basename << method->name() << "\", [this](std::shared_ptr<thalhammer::grpcbackend::util::json_rpc::call_context> ctx) {\n";
		ss << "\t\tstd::shared_ptr<thalhammer::grpcbackend::util::json_rpc_query<" << get_clean_name(input_type) << ", " << get_clean_name(output_type) << ">> mctx;\n";
		ss << "\t\ttry {\n";
		ss << "\t\t\tmctx = std::make_shared<thalhammer::grpcbackend::util::json_rpc_query<" << get_clean_name(input_type) << ", " << get_clean_name(output_type) << ">>(ctx);\n";
		ss << "\t\t} catch(const std::exception& e) {\n";
		ss << "\t\t\treturn ctx->error(thalhammer::grpcbackend::util::json_rpc::ERROR_INVALID_PARAMS, e.what());\n";
		ss << "\t\t}\n";
		ss << "\t\ttry {\n";
		ss << "\t\t\tthis->" << method->name() << "(mctx);\n";
		ss << "\t\t} catch(const std::exception& e) {\n";
		ss << "\t\t\treturn ctx->error(thalhammer::grpcbackend::util::json_rpc::ERROR_INTERNAL, e.what());\n";
		ss << "\t\t}\n";
		ss << "\t});\n";
	}
	ss << "}\n\n";
}

static bool GenerateBody(const google::protobuf::FileDescriptor* file, const std::string& parameter, google::protobuf::compiler::GeneratorContext* context, std::string* error) 
{
	std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(file->name() + ".cpp"));
	google::protobuf::io::Printer printer(output.get(), '$');

	std::stringstream ss;

	google::protobuf::compiler::Version ver;
	context->GetCompilerVersion(&ver);
	ss << "/*\n";
	ss << " *  Generated by json_rpc_api_plugin on protobuf " << ver.major() << "." << ver.minor() << "." << ver.patch();
	if(!ver.suffix().empty()) ss << "-" << ver.suffix();
	ss << "\n";
	ss << " */\n";
	ss << "#include \"" << file->name() << ".h\"\n";
	ss << "#include <grpcbackend/util/json_rpc.h>\n";
	ss << "#include <openssl/evp.h>\n";
	ss << "\n";

	GenerateBodyHelpers(ss);

	// Convert package to C++ namespaces
	std::vector<std::string> namespaces;
	if(!file->package().empty()) namespaces = ttl::string::split<std::string>(file->package(), ".");
	for(auto& e : namespaces) ss << "namespace " << e << " {\n";
	ss << "\n";

	// Generate enum related functions
	for (auto e : get_all_enums(file))
		GenerateBodyEnum(ss, e);
	
	// Generate message related functions
	for (auto m : get_all_messages(file)) 
	{
		GenerateBodyToJson(ss, m);
		GenerateBodyFromJson(ss, m);
	}

	// Generate service registry
	for (int s = 0; s < file->service_count(); s++)
	{
		auto service = file->service(s);
		GenerateBodyRegisterService(ss, service);
	}

	// Closing namespaces
	for(auto& e : namespaces) ss << "} /* " << e << " */\n";
	ss << "\n";

	printer.PrintRaw(ss.str());
	if (printer.failed()) 
	{
		*error = "JsonRPCApiCodeGenerator detected write error.";
		return false;
	}
	return true;
}

bool GrpcbackendServerCodeGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter, google::protobuf::compiler::GeneratorContext* context, std::string* error)  const
{
	try {
		return GenerateHeader(file, parameter, context, error)
			&& GenerateBody(file, parameter, context, error);
	} catch(const std::exception& e) {
		*error = e.what();
		return false;
	}
}

