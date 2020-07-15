#pragma once
#include <string>
#include <set>
#include <istream>
#include <memory>

namespace ttl {
	// Forward declaration to ttl
	class mmap;
	namespace io { class zip_reader; }
}
namespace thalhammer {
	namespace grpcbackend {
		enum class fs_filetype {
			file,
			directory,
			unknown
		};
		struct fs_entry {
			std::string name;
			std::string file_name;
			time_t lmodified;
			uintmax_t size;
			fs_filetype type;
			bool is_dir() const { return type == fs_filetype::directory; }
			bool is_file() const { return type == fs_filetype::file; }

			bool operator<(const fs_entry& rhs) const {
				return this->name < rhs.name;
			}
		};
		struct ifilesystem {
			virtual ~ifilesystem() {}
			virtual bool exists(const std::string& path) = 0;
			virtual fs_entry get_file(const std::string& path) = 0;
			virtual std::set<fs_entry> get_files(const std::string& path) = 0;
			virtual std::unique_ptr<std::istream> open_file(const std::string& path) = 0;
		};
		struct osfilesystem : ifilesystem {
			bool exists(const std::string& path) override;
			fs_entry get_file(const std::string& path) override;
			std::set<fs_entry> get_files(const std::string& path) override;
			std::unique_ptr<std::istream> open_file(const std::string& path) override;
		};
		class zipfilesystem : public ifilesystem {
			std::unique_ptr<ttl::mmap> resdata;
			std::unique_ptr<ttl::io::zip_reader> zip;
		public:
			zipfilesystem(const std::string& path, bool precache = true);
			~zipfilesystem();
			bool exists(const std::string& path) override;
			grpcbackend::fs_entry get_file(const std::string& path) override;
			std::set<grpcbackend::fs_entry> get_files(const std::string& path) override;
			std::unique_ptr<std::istream> open_file(const std::string& path) override;
		};
	}
}