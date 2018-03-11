#include "filesystem.h"
#include <ttl/mmap.h>
#undef max // Windows defines max and screws up numeric_limits::max
#include <cassert>
#include <cstring>
#include <ttl/io/zip_reader.h>
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

		static std::string zip_fix_path(const std::string& path) {
			auto p = path;
			if(p[0] == '.' && p[1] == '/')
				p = p.substr(2);
			else if(p[0] == '/')
				p = p.substr(1);
			for(size_t i = 1; i < p.size(); i++) {
				if(p[i-1] == '/' && p[i] == '/')
					p.erase(p.begin() + i);
			}
			return p;
		}

		zipfilesystem::zipfilesystem(const std::string& path, bool precache)
			: resdata(std::make_unique<thalhammer::mmap>(path))
		{
			// Open executable resources
			if(!resdata->is_valid())
				throw std::runtime_error("failed to mmap executable");
			zip = std::make_unique<thalhammer::io::zip_reader>(resdata->data(), resdata->size());
			if(precache)
				zip->uncompress();
		}

		zipfilesystem::~zipfilesystem() {
		}

		bool zipfilesystem::exists(const std::string& path) {
			auto p = zip_fix_path(path);
			return zip->has_file(p) || zip->has_file(p + "/");
		}

		grpcbackend::fs_entry zipfilesystem::get_file(const std::string& path) {
			auto p = zip_fix_path(path);
			
			if(!zip->has_file(p))
				p = p + "/";

			if(!zip->has_file(p))
				throw std::runtime_error("file not found");

			auto idx = zip->find_by_path(p);
			auto& entry = zip->get_entry(idx);

			fs_entry me;
			me.name = entry.get_name();
			me.type = entry.is_directory() ? fs_filetype::directory : fs_filetype::file;
			me.lmodified = entry.get_last_modified();
			me.size = entry.get_header().uncompressed_size;
			if(me.is_file())
			{
				auto pos = me.name.find_last_of('/');
				if(pos != std::string::npos)
					me.file_name = me.name.substr(pos + 1);
				else me.file_name = me.name;
			}
			return me;
		}
		
		std::set<fs_entry> zipfilesystem::get_files(const std::string& path) {
			auto e = this->get_file(path);
			if(!e.is_dir())
				throw std::runtime_error("path is not a directory");
			
			std::set<fs_entry> res;
			for(auto& file : zip->get_files()) {
				if(file == e.name || file.substr(0, e.name.size()) != e.name)
					continue;

				auto tfile = file.substr(e.name.size());
				auto pos = tfile.find('/');
				if(pos != tfile.size() - 1 && pos != std::string::npos)
					continue;

				auto idx = zip->find_by_path(file);
				auto& entry = zip->get_entry(idx);

				fs_entry me;
				me.name = entry.get_name();
				me.type = entry.is_directory() ? fs_filetype::directory : fs_filetype::file;
				me.lmodified = entry.get_last_modified();
				me.size = entry.get_header().uncompressed_size;
				if(me.is_file())
				{
					me.file_name = tfile;
				} else if(me.is_dir()) {
					me.file_name = tfile.substr(0, tfile.size() - 1);
				}
				res.insert(me);
			}
			return res;
		}
		
		std::unique_ptr<std::istream> zipfilesystem::open_file(const std::string& path) {
			auto p = zip_fix_path(path);
			
			if(!zip->has_file(p))
				throw std::runtime_error("file not found");
			
			auto idx = zip->find_by_path(p);
			auto& entry = zip->get_entry(idx);

			return entry.open_stream(true);
		}
	}
}