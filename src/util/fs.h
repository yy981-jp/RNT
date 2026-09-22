#pragma once
#include <def/fs.h>

inline fs::path getTempRoot(const fs::path targetDir) {
	return fs::temp_directory_path() / std::format("RNT-{}", std::hash<fs::path>{}(targetDir));
}

struct RNT_Dir {
	const fs::path targetDir;

	RNT_Dir(const fs::path targetDir): targetDir(targetDir) {
		fs::create_directories( getTempRoot(targetDir) / "ev" );
	}
	~RNT_Dir() {
		fs::remove_all(targetDir);
	}
};
