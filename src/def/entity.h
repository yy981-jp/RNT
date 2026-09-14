#pragma once
#include <cstdint>
#include <string>


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
	int depth;
};
