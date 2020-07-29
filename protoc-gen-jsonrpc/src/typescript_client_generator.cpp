#include "typescript_client_generator.h"
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

static void GenerateModuleHelpers(std::ostream& ss) {
	ss << "interface IJsonRPCClient {\n";
	ss << "\tcall(method: string, params? : any[] | object) : Promise<any>;\n";
	ss << "\tnotify(method: string, params?: any[] | object): void;\n";
	ss << "}\n";
	ss << "interface ILogger {\n";
	ss << "\terror(scope: string, message: string, ...data: any[]);\n";
	ss << "\tinfo(scope: string, message: string, ...data: any[]);\n";
	ss << "\twarn(scope: string, message: string, ...data: any[]);\n";
	ss << "}\n";
}

static void GenerateModuleEnum(std::ostream& ss, const google::protobuf::EnumDescriptor* e) {
	ss << "enum " << get_clean_name(e) << " {\n";
	for(int i = 0; i < e->value_count(); i++) {
		auto val = e->value(i);
		ss << "\t" << val->name() << " = " << val->number() << ",\n";
	}
	ss << "}\n\n";
}

static void GenerateModuleMessage(std::ostream& ss, const google::protobuf::Descriptor* m) {
	ss << "class " << get_clean_name(m) << " {\n";

	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		ss << "\t" << fd->name() << " : ";
		switch(fd->type()) {
		case google::protobuf::FieldDescriptor::TYPE_BOOL:
			ss << "boolean;\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_BYTES:
			ss << "ArrayBuffer;\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_ENUM:
			ss << get_clean_name(fd->enum_type()) + ";\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_GROUP:
			throw std::runtime_error(std::string("not implemented ") + fd->type_name());
		case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
			ss << get_clean_name(fd->message_type()) + ";\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
		case google::protobuf::FieldDescriptor::TYPE_FLOAT:
		case google::protobuf::FieldDescriptor::TYPE_FIXED32:
		case google::protobuf::FieldDescriptor::TYPE_INT32:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
		case google::protobuf::FieldDescriptor::TYPE_SINT32:
		case google::protobuf::FieldDescriptor::TYPE_FIXED64:
		case google::protobuf::FieldDescriptor::TYPE_INT64:
		case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
		case google::protobuf::FieldDescriptor::TYPE_SINT64:
		case google::protobuf::FieldDescriptor::TYPE_UINT32:
		case google::protobuf::FieldDescriptor::TYPE_UINT64:
			ss << "number;\n"; break;
		case google::protobuf::FieldDescriptor::TYPE_STRING:
			ss << "string;\n"; break;
		}
	}
	ss << "\n";
	ss << "\tverify() : void {\n";
	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		if(!opts.is_optional()) {
			ss << "\t\tif(this." << fd->name() << " === undefined || this." << fd->name() << " === null)\n";
			ss << "\t\t\tthrow new Error(\"missing required member " << fd->name() << "\");\n"; 
		}
	}
	ss << "\t}\n";
	ss << "\tto_json() : object {\n";
	ss << "\t\tthis.verify();\n";
	ss << "\t\tlet result : any = {};\n";
	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		auto name = (fd->has_json_name() && fd->type() != fd->TYPE_ENUM) ? fd->json_name() : fd->name();
		if(fd->type() == fd->TYPE_BYTES) {
			ss << "\t\tresult[\"" << name << "\"] = btoa(String.fromCharCode.apply(null, new Uint8Array(this." << fd->name() << ")));\n";
		} else ss << "\t\tresult[\"" << name << "\"] = this." << fd->name() << ";\n";
	}
	ss << "\t\treturn result;\n";
	ss << "\t}\n";
	ss << "\tfrom_json(val : any) : void {\n";
	for (int f = 0; f < m->field_count(); f++)
	{
		auto fd = m->field(f);
		auto& opts = fd->options().GetExtension(json_rpc_field);
		auto name = (fd->has_json_name() && fd->type() != fd->TYPE_ENUM) ? fd->json_name() : fd->name();
		if(fd->type() == fd->TYPE_BYTES) {
			ss << "\t\tthis." << fd->name() << " = Uint8Array.from(atob(val[\"" << name << "\"]), c => c.charCodeAt(0));\n";
		} else ss << "\t\tthis." << fd->name() << " = val[\"" << name << "\"];\n";
	}
	ss << "\t\tthis.verify();\n";
	ss << "\t}\n";
	ss << "}\n\n";
}

static void GenerateModuleService(std::ostream& ss, const google::protobuf::ServiceDescriptor* service) {
	auto& service_opts = service->options().GetExtension(json_rpc_service);
	std::string basename;
	if(service_opts.basename().empty()) {
		basename =  service->file()->package();
		if(!basename.empty()) basename += ".";
		basename += service->name() + ".";
	} else {
		basename = service_opts.basename().empty() + ".";
	}

	ss << "export class " << service->name() << " {\n";
	ss << "\tconstructor(private rpc: IJsonRPCClient, private logger?: ILogger) {}\n";
	for(int m = 0; m < service->method_count(); m++) {
		auto method = service->method(m);
		if(method->server_streaming() || method->client_streaming())
			throw std::runtime_error("jsonrpc does not support streaming calls");
		auto input_type = method->input_type();
		auto output_type = method->output_type();
		ss << "\tpublic " << method->name() << "(request : " << get_clean_name(input_type) << ") : Promise<" << get_clean_name(output_type) << "> {\n";
        ss << "\t\tif(this.logger) this.logger.info(\"" << service->name() << "\", \"Calling " << service->name() << "." << method->name() << "\", request.to_json());\n";
        ss << "\t\treturn this.rpc.call(\"" << basename << method->name() << "\", request.to_json())\n";
		ss << "\t\t\t.then((res) => {\n";
		ss << "\t\t\t\tlet result = new " << get_clean_name(output_type) << "();\n";
		ss << "\t\t\t\tresult.from_json(res);\n";
		ss << "\t\t\t\treturn result;\n";
		ss << "\t\t\t});\n";
		ss << "\t}\n";
	}
	ss << "}\n";
}

static bool GenerateModule(const google::protobuf::FileDescriptor* file, const std::string& parameter, google::protobuf::compiler::GeneratorContext* context, std::string* error) 
{
	auto filepath = ttl::string::join(ttl::string::split<std::string>(file->package(), "."), "/");
	std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(filepath + "/" + file->name() + ".ts"));
	google::protobuf::io::Printer printer(output.get(), '$');

	std::stringstream ss;

	/* Getting protobuf compiler version */
	google::protobuf::compiler::Version ver;
	context->GetCompilerVersion(&ver);
	ss << "/*\n";
	ss << " *  Generated by json_rpc on protobuf " << ver.major() << "." << ver.minor() << "." << ver.patch();
	if(!ver.suffix().empty()) ss << "-" << ver.suffix();
	ss << "\n";
	ss << " */\n";

	GenerateModuleHelpers(ss);

	ss << "\n";

	for (auto e : get_all_enums(file))
		GenerateModuleEnum(ss, e);

	ss << "\n";
	
	for (auto m : get_all_messages(file))
		GenerateModuleMessage(ss, m);

	ss << "\n";

	for (int s = 0; s < file->service_count(); s++)
	{
		auto service = file->service(s);
		GenerateModuleService(ss, service);
	}

	printer.PrintRaw(ss.str());
	if (printer.failed()) 
	{
		*error = "JsonRPCApiCodeGenerator detected write error.";
		return false;
	}
	return true;
}

bool TypescriptClientCodeGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter, google::protobuf::compiler::GeneratorContext* context, std::string* error)  const
{
	try {
		return GenerateModule(file, parameter, context, error);
	} catch(const std::exception& e) {
		*error = e.what();
		return false;
	}
}

