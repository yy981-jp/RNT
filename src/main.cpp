#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <stack>
#include <cstdint>

namespace fs = std::filesystem;

// disable_recursion_pending


struct EntryId {
	uint64_t value = 0;

	static EntryId INVALID() { return {UINT64_MAX}; }
	bool isValid() { return value != INVALID().value; }

	auto operator<=>(const EntryId&) const = default;
	EntryId operator++(int) {
		EntryId b = *this;
		value++;
		return b;
	}
};

struct Entry {
	EntryId id;
	EntryId parent;
	std::string name;
};

struct Ctx {
	std::vector<Entry> entries;
	std::stack<EntryId> parents;
	int depth = 0;
	EntryId cid{};
};

void dir(Ctx& ctx, const fs::path& targetDir) {
	for (const auto& e: fs::directory_iterator(targetDir)) {
		Entry ent{
			.id = ctx.cid++,
			.parent = ctx.parents.empty()? EntryId::INVALID() : ctx.parents.top(),
			.name = e.path().filename().string(),
		};
		ctx.entries.push_back(ent);

		if (e.is_directory()) {
			ctx.parents.push(ent.id);
			dir(ctx, e.path());
			ctx.parents.pop();
		}


	}
}


int main(int argc, char *argv[]) {
	fs::path target;
	if (argc < 2) target = fs::current_path();
	else target = fs::path(argv[1]);


	Ctx ctx{};
	dir(ctx, target);

	for (const auto& e: ctx.entries) {
		printf("%llu-%llu:\t%s\n", e.id.value, e.parent.value, e.name.c_str());
	}
}
