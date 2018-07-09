#include "uri.h"

namespace thalhammer {
	namespace grpcbackend {
		uri::uri(const std::string& uri) {
			if(uri.size() == 0 || uri[0] != '/')
				throw invalid_uri_exception();
			auto pos = uri.find_first_of('?');
			if (pos != std::string::npos) {
				_path = uri.substr(0, pos);
				auto offset = pos;
				do {
					offset += 1;
					pos = uri.find('&', offset);
					auto part = uri.substr(offset, pos - offset);
					if(!part.empty()) {
						auto pos_eq = part.find('=');
						if (pos_eq != std::string::npos)
							_query.insert({ part.substr(0, pos_eq), part.substr(pos_eq + 1) });
						else _query.insert({ part, "" });
					}
					offset = pos;
				} while (offset < uri.size());
			}
			else {
				_path = uri;
			}
			pos = _path.find_last_of('/');
			if (pos != std::string::npos) {
				_fname = _path.substr(pos + 1);
				pos = _fname.find_last_of('.');
				if (pos != std::string::npos)
					_ext = _fname.substr(pos + 1);
			}
		}

		static char parse_hexchar(char c) {
			if(c >= 'A' && c <= 'F')
				return (c - 'A') + 10;
			if(c >= 'a' && c <= 'f')
				return (c - 'a') + 10;
			if(c >= '0' && c <= '9')
				return c - '0';
			throw invalid_uri_exception();
		}

		static char parse_hexnum(char c1, char c2) {
			auto r1 = parse_hexchar(c1);
			auto r2 = parse_hexchar(c2);
			return ((r1 << 4) & 0xf0) | (r2 & 0x0f);
		}

		std::string uri::url_decode(const std::string& str) {
			std::string res;
			for(size_t i=0; i<str.size(); i++) {
				if(str[i] != '%')
					res += str[i];
				else {
					if(str.size() - i < 2)
						throw invalid_uri_exception();
					res += parse_hexnum(str[i+1], str[i+2]);
					i += 2;
				}
			}
			return res;
		}

		std::string uri::url_encode(const std::string& str) {
			static const char * const table = "0123456789abcdef";
			std::string res;
			for(char c : str) {
				if((c >= 'A' && c <= 'Z')
				|| (c >= 'a' && c <= 'z')
				|| (c >= '0' && c <= '9')
				|| c == '-' || c == '_' || c == '.' || c == '~') {
					res += c;
				} else {
					res += '%';
					res += table[(c>>4)&0x0f];
					res += table[c&0x0f];
				}
			}
			return res;
		}

		std::unordered_multimap<std::string, std::string> uri::parse_formdata(const std::string& data) {
			std::unordered_multimap<std::string, std::string> res;
			size_t offset = 0;
			do {
				auto pos = data.find('&', offset);
				auto part = data.substr(offset, pos == std::string::npos ? std::string::npos : pos - offset);
				if(!part.empty()) {
					auto pos_eq = part.find('=');
					if (pos_eq != std::string::npos)
						res.insert({ url_decode(part.substr(0, pos_eq)), url_decode(part.substr(pos_eq + 1)) });
					else res.insert({ url_decode(part), "" });
				}
				if(pos == std::string::npos) break;
				offset = pos + 1;
			} while (offset < data.size());
			return res;
		}
	}
}