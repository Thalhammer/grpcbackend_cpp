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
	}
}