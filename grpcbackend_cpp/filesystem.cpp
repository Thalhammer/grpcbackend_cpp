#include "filesystem.h"
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

namespace thalhammer {
	namespace grpcbackend {
		bool osfilesystem::exists(const std::string& path) {
			return fs::exists(path);
		}

		fs_entry osfilesystem::get_file(const std::string& path) {
			if(!exists(path))
				throw std::runtime_error("file not found");

			auto bpath = fs::canonical(path);
			auto status = fs::status(path);
			fs_entry entry;
			switch(status.type()) {
				case fs::file_type::directory_file:
					entry.type = fs_filetype::directory; break;
				case fs::file_type::regular_file:
					entry.type = fs_filetype::file; break;
				default:
					entry.type = fs_filetype::unknown; break;
			}
			entry.name = bpath.string();
			entry.file_name = bpath.has_filename() ? bpath.filename().string() : "";
			entry.lmodified = fs::last_write_time(bpath);
			if(entry.is_file())
				entry.size = fs::file_size(bpath);
			
			return std::move(entry);
		}

		std::set<fs_entry> osfilesystem::get_files(const std::string& path) {
			std::set<fs_entry> res;
			for(auto& entry : fs::directory_iterator(path)) {
				fs_entry me;
				me.file_name = entry.path().has_filename() ? entry.path().filename().string() : "";
				me.lmodified = fs::last_write_time(entry.path());
				me.name = entry.path().string();
				me.size = fs::file_size(entry.path());
				switch(entry.status().type()) {
					case fs::file_type::directory_file:
						me.type = fs_filetype::directory; break;
					case fs::file_type::regular_file:
						me.type = fs_filetype::file; break;
					default:
						me.type = fs_filetype::unknown; break;
				}
				if(me.is_file())
					me.size = fs::file_size(entry.path());
				res.insert(me);
			}
			return res;
		}

		std::unique_ptr<std::istream> osfilesystem::open_file(const std::string& path) {
			return std::make_unique<std::ifstream>(path, std::ios::binary | std::ios::ate);
		}
	}
}