#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <def/fs.h>


struct EntryId {
	uint64_t value = 0;

	static EntryId INVALID() { return {UINT64_MAX}; }
	bool isValid() const { return value != INVALID(); }

	auto operator<=>(const EntryId&) const = default;
	EntryId operator++(int) {
		EntryId b = *this;
		value++;
		return b;
	}
	operator size_t() const { return value; }
};

struct Entry {
	EntryId id;
	EntryId parent;
	std::string name;
	int depth;
	bool isDir = false;
};

template<>
struct std::hash<EntryId> {
	size_t operator()(const EntryId& id) const noexcept {
		return std::hash<uint64_t>{}(id);
	}
};


inline fs::path solvePath(const std::vector<Entry>& entries, EntryId target) {
	std::vector<EntryId> chain;

	chain.reserve( entries[target].depth + 1 );

	for (auto id = target; id.isValid(); id = entries[id].parent) {
		chain.push_back( id );
	}

	fs::path path;

	for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
		path /= entries[it->value].name;
	}

	return path;
}
