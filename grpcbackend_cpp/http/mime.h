#pragma once
#include <string>
#include <vector>
#include <ttl/string_util.h>
#include <fstream>
#include <unordered_map>

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			class mime {
				std::unordered_map<std::string, std::string> mime_lookup;
			public:
				bool load_file(const std::string& file) {
					std::ifstream stream(file);
					if (!stream.good())
						return false;
					return this->load_stream(stream);
				}
				bool load_stream(std::istream& stream) {
					using namespace std::string_literals;
					std::string line;
					size_t l = 0;
					while (std::getline(stream, line)) {
						l++;
						ttl::string::trim(line);
						if (line[0] == '#')
							continue;
						auto pos = line.find_first_of(" \t");
						if (pos == std::string::npos)
							return false;
						auto type = line.substr(0, pos);
						line = line.substr(pos);
						ttl::string::ltrim(line);

						for (auto& ext : ttl::string::split(line, " "s)) {
							ttl::string::trim(ext);
							if (ext.empty())
								continue;
							mime_lookup[ext] = type;
						}
					}
					return true;
				}

				bool has_type(const std::string& ext) const {
					return mime_lookup.count(ext) != 0;
				}

				const std::string& get_type(const std::string& ext) const {
					static const std::string unknown = "application/binary";
					if (has_type(ext))
						return mime_lookup.at(ext);
					return unknown;
				}
			};
		}
	}
}