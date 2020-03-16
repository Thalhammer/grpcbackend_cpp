#include "filesystem_route_handler.h"
#include "../route_params.h"
#include <vector>
#include <ttl/string_util.h>
#include <date/date.h>
#include <iomanip>

using namespace std::string_literals;

namespace thalhammer {
	namespace grpcbackend {
		namespace http {
			namespace route {
				std::string filesystem_route_handler::last_modified(time_t tdate)
				{
					auto lmodified = std::chrono::system_clock::from_time_t(tdate);
					auto days = date::floor<date::days>(lmodified);
					auto date = date::year_month_day{ days };
					auto time = date::make_time(lmodified - days);
					auto day = date::weekday{ days };
					std::stringstream ss;
					ss << day << ", " << date.day() << " " << date.month() << " " << date.year() << " " << time.hours().count() << ":" << time.minutes().count() << ":" << time.seconds().count() << " GMT";
					return ss.str();
				}

				std::string filesystem_route_handler::format_size(std::streamsize size)
				{
					static const std::string ext = "KMGTPEZY";
					size_t mult = 0;
					while (size >= 1024) {
						size /= 1024;
						mult++;
					}
					if (mult != 0) {
						return std::to_string(size) + ext[mult - 1];
					}
					else return std::to_string(size);
				}

				struct fs_params : public attribute {
					std::unique_ptr<std::istream> stream;
				};

				void filesystem_route_handler::on_request(connection_ptr con)
				{
					con->skip_body([this](connection_ptr con, bool){
						
						auto params = con->get_attribute<route_params>();
						if (!params)
							throw std::logic_error("Missing route_params attribute");
						if (!params->has_param("fs_path"))
							throw std::logic_error("Missing fs_path param");
						
						if (fs->exists(path + params->get_param("fs_path"))) {
							auto p = fs->get_file(path + params->get_param("fs_path"));
							if (p.is_file()) {
								auto lmodified = last_modified(p.lmodified);
								if (lmodified == con->get_header("If-Modified-Since")) {
									con->set_status(304, "Not modified");
									con->set_header("Last-Modified", lmodified);
									con->end();
								}
								else {
									// TODO: Prevent allocation
									auto stream = fs->open_file(p.name);
									if (stream->good()) {
										stream->seekg(0, std::ios::beg);
										con->set_status(200, "OK");
										con->set_header("Content-Length", std::to_string(p.size));
										con->set_header("Last-Modified", lmodified);
										auto str = stream.release();
										con->send_body(*str, [str](http::connection_ptr con, bool ok){
											if(str) delete str;
											con->end();
										});
									}
									else {
										con->set_status(404, "Not found");
										con->end();
									}
								}
							}
							else if (allow_listing && p.is_dir()) {
								con->set_status(200, "OK");
								std::stringstream stream;
								stream << "<!DOCTYPE html>";
								stream << "<html><head><meta charset=\"utf-8\"><title>Index of " << con->get_resource() << "</title></head><body>";
								stream << "<h1>Index of " << con->get_resource() << "</h1>";
								stream << "<table><tr><th></th><th>Name</th><th>Last modified</th><th>Size</th></tr>";
								stream << "<tr><td colspan=4><hr></td><tr>";
								for (auto& entry : fs->get_files(p.name)) {
									if(entry.type == fs_filetype::unknown)
										continue;

									auto lmodified = std::chrono::system_clock::from_time_t(entry.lmodified);
									auto days = date::floor<date::days>(lmodified);
									auto date = date::year_month_day{ days };
									auto time = date::make_time(lmodified - days);
									std::string fname = entry.file_name;
									std::string link = "./" + fname;
									stream
										<< "<tr><td>"
										<< (entry.is_dir() ? "[DIR ]" : "[FILE]")
										<< "</td><td><a href=\""
										<< link
										<< "\">"
										<< fname
										<< "</a></td><td>"
										<< date << " " << std::setw(2) << std::setfill('0') << time.hours().count() << ":" << std::setw(2) << std::setfill('0') << time.minutes().count()
										<< "</td><td>"
										<< format_size(entry.is_dir() ? 0 : entry.size)
										<< "</td></tr>";
								}
								stream << "</table></body></html>";
								con->end(stream.str(), [](connection_ptr con, bool ok){});
							}
							else {
								con->set_status(404, "Not found");
								con->end();
							}
						}
						else {
							con->set_status(404, "Not found");
							con->end();
						}
					});
				}
			}
		}
	}
}