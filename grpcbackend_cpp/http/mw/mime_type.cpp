#include "mime_type.h"
#include "response_forward.h"

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace mw {
				mime_type::mime_type(const std::string& filename) {
                    if (!_mime.load_file(filename))
                        throw std::runtime_error("Failed to load mime file");
                }
				mime_type::mime_type(std::istream& stream) {
					if (!_mime.load_stream(stream))
						throw std::runtime_error("Failed to load mime stream");
				}
                // Geerbt über middleware
                void mime_type::handle_request(request & req, response & resp, std::function<void(request&, response&)>& next) {
                    auto ext = req.get_parsed_uri().get_extension();
                    if (!ext.empty() && _mime.has_type(ext)) {
                        class my_response : public response_forward {
                            const std::string& mime;
                            bool type_set = false;
                        public:
                            my_response(response& porig, const std::string& pmime) : response_forward(porig), mime(pmime) {}
                            virtual void set_header(const std::string& key, const std::string& value, bool replace = false) {
                                response_forward::set_header(key, value, replace);
                                std::string data = key;
                                std::transform(data.begin(), data.end(), data.begin(), ::tolower);
                                if (data == "content-type")
                                    type_set = true;
                            }
                            // Try to detect content type before sending headers
                            virtual std::ostream& get_ostream() {
                                if (!type_set) {
                                    response_forward::set_header("Content-Type", mime);
                                    type_set = true;
                                }
                                return response_forward::get_ostream();
                            }
                        };
                        my_response res(resp, _mime.get_type(ext));
                        next(req, res);
                    }
                    else {
                        next(req, resp);
                    }
                }
			}
		}
	}
}