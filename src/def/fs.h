#pragma once

#include <filesystem>

namespace fs = std::filesystem;

struct FsOperate {
	fs::path from, to;
};
