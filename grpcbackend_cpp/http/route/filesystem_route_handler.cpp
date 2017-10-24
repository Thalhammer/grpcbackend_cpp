#include "filesystem_route_handler.h"
#include "../route_params.h"
#include <vector>
#include <ttl/string_util.h>
#include <boost/filesystem.hpp>
#include <fstream>
#include <date/date.h>
#include <iomanip>

using namespace std::string_literals;
namespace fs = boost::filesystem;

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

				void filesystem_route_handler::handle_request(request & req, response & resp)
				{
					if (!req.has_attribute<route_params>())
					{
						throw std::logic_error("Missing route_params attribute");
					}
					auto params = req.get_attribute<route_params>();
					if (!params->has_param("fs_path")) {
						throw std::logic_error("Missing fs_path param");
					}

					auto fs_path = fs::path(path + params->get_param("fs_path"));
					if (fs::exists(fs_path)) {
						if (fs::is_regular_file(fs_path)) {
							auto lmodified = last_modified(fs::last_write_time(fs_path));
							if (lmodified == req.get_header("If-Modified-Since")) {
								resp.set_status(304, "Not modified");
								resp.set_header("Last-Modified", lmodified);
							}
							else {
								std::ifstream file(fs_path.native(), std::ios::binary | std::ios::ate);
								if (file.good()) {
									std::streamsize size = file.tellg();
									file.seekg(0, std::ios::beg);
									resp.set_status(200, "OK");
									resp.set_header("Content-Length", std::to_string(size));
									resp.set_header("Last-Modified", lmodified);
									auto& stream = resp.get_ostream();

									stream << file.rdbuf();
									stream.rdbuf()->pubsync();
								}
								else {
									resp.set_status(404, "Not found");
									resp.get_ostream() << "Not found";
								}
							}
						}
						else if (allow_listing && fs::is_directory(fs_path)) {
							resp.set_status(200, "OK");
							auto& stream = resp.get_ostream();
							stream << "<!DOCTYPE html>";
							stream << "<html><head><meta charset=\"utf-8\"><title>Index of " << req.get_resource() << "</title></head><body>";
							stream << "<h1>Index of " << req.get_resource() << "</h1>";
							stream << "<table><tr><th></th><th>Name</th><th>Last modified</th><th>Size</th></tr>";
							stream << "<tr><td colspan=4><hr></td><tr>";
							for (auto& entry : fs::directory_iterator(fs_path)) {
								if (entry.path().filename() != ".." && entry.path().filename() != "." && fs::exists(entry.path())) {
									auto lmodified = std::chrono::system_clock::from_time_t(fs::last_write_time(entry.path()));
									auto days = date::floor<date::days>(lmodified);
									auto date = date::year_month_day{ days };
									auto time = date::make_time(lmodified - days);
									bool is_dir = fs::is_directory(entry.path());
									std::string fname = entry.path().filename().string();
									std::string link = "./" + fname;
									stream
										<< "<tr><td>"
										<< (is_dir ? "[DIR ]" : "[FILE]")
										<< "</td><td><a href=\""
										<< link
										<< "\">"
										<< fname
										<< "</a></td><td>"
										<< date << " " << std::setw(2) << std::setfill('0') << time.hours().count() << ":" << std::setw(2) << std::setfill('0') << time.minutes().count()
										<< "</td><td>"
										<< format_size(is_dir ? 0 : fs::file_size(entry.path()))
										<< "</td></tr>";
								}
							}
							stream << "</table></body></html>";
						}
						else {
							resp.set_status(404, "Not found");
							resp.get_ostream() << "Not found";
						}
					}
					else {
						resp.set_status(404, "Not found");
						resp.get_ostream() << "Not found";
					}
				}
			}
		}
	}
}