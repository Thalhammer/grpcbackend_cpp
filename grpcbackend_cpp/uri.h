#pragma once
#include <string>
#include <unordered_map>
#include <set>

namespace thalhammer {
	namespace grpcbackend {
		class uri {
			std::string _path;
			std::string _fname;
			std::string _ext;
			std::unordered_multimap<std::string, std::string> _query;
		public:
			explicit uri(const std::string& uri);

			// Full path including filename and extension
			const std::string& get_path() const { return _path; }
			// Only filename including extention
			const std::string& get_filename() const { return _fname; }
			// Only file extension
			const std::string& get_extension() const { return _ext; }
			
			// Get first occurence of param
			std::string get_param(const std::string& key) const {
					auto it = _query.find(key);
					if (it == _query.cend())
					{
						return "";
					}
					else return it->second;
			}
			// Get all occurences of param
			std::set<std::string> get_params(const std::string& key) const {
					std::set<std::string> res;
					auto range = _query.equal_range(key);
					for (auto it = range.first; it != range.second; it++) {
						res.insert(it->second);
					}
					return res;
			}

			// URL Decode a string
			static std::string url_decode(const std::string& str);
			// URL Encode a string
			static std::string url_encode(const std::string& str);
			// Parse formdata
			static std::unordered_multimap<std::string, std::string> parse_formdata(const std::string& data);
		};

		class invalid_uri_exception : std::runtime_error {
		public:
			invalid_uri_exception()
				:std::runtime_error("Invalid uri")
			{}
		};
	}
}