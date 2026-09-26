#pragma once
#include <vector>
#include <stack>

#include <def/entry.h>
#include <def/fs.h>


struct Ctx {
	std::vector<Entry> entries;
	std::stack<EntryId> parents;
	int depth = 0;
	EntryId cid{0};
};

void dir(Ctx& ctx, const fs::path& targetDir);
