#include "mime_type.h"
#include "response_forward.h"
#include <magic.h>
#include <mutex>

/**
 * Wrapper around libmagic to make it RAII compatible and threadsafe
 */
class magic {
    mutable std::mutex m_mtx;
    magic_t m_cookie;
public:
    magic()
    {
        m_cookie = magic_open(MAGIC_MIME);
        if(m_cookie == nullptr) throw std::runtime_error("failed to init magic library");
        if (magic_load(m_cookie, NULL) != 0) {
            std::string msg = "cannot load magic database - ";
            msg += magic_error(m_cookie);
            magic_close(m_cookie);
            throw std::runtime_error(msg);
        }
    }
    
    ~magic()
    {
        if(m_cookie) magic_close(m_cookie);
    }
    
    magic(const magic&) = delete;
    magic(magic&&) = delete;
    magic& operator=(const magic&) = delete;
    magic& operator=(magic&&) = delete;

    std::string check_file(const std::string& filename) const {
        std::unique_lock<std::mutex> lck(m_mtx);
        auto r = magic_file(m_cookie, filename.c_str());
        if(r == nullptr) return "";
        return r;
    }

    std::string check_buffer(const void* buf, size_t len) const {
        std::unique_lock<std::mutex> lck(m_mtx);
        auto r = magic_buffer(m_cookie, buf, len);
        if(r == nullptr) return "";
        return r;
    }
};

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
                            void set_header(const std::string& key, const std::string& value, bool replace = false) override {
                                response_forward::set_header(key, value, replace);
                                std::string data = key;
                                std::transform(data.begin(), data.end(), data.begin(), ::tolower);
                                if (data == "content-type")
                                    type_set = true;
                            }
                            // Try to detect content type before sending headers
                            std::ostream& get_ostream() override {
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