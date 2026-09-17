#pragma once
#include <def/entry.h>

#include <unordered_map>


enum class OpType {
	rename, move,
};

class Solver {
	const std::vector<Entry> &before, &changed;

	bool ok = false;
	std::string errorMsg;

	std::vector<EntryId> changedId;


	void error(const std::string& str) {
		errorMsg = str;
		ok = false;
	}

public:
	Solver(const std::vector<Entry>& before, const std::vector<Entry>& changed):
		before(before), changed(changed) {}

	void debug() {
		for (auto e: changedId) printf("%llu, ", e.value);
		printf("\n");
	}

	void diff() {
		for (size_t i = 0; i < before.size(); i++) {
			const Entry& a = before[i];
			const Entry& b = changed[i];

			if (a.isDir ^ b.isDir) return error(
				"Changing a directory to a file, or vice versa, is not supported."
			);

			if (
				(a.parent != b.parent) ||
				(a.name != b.name)
			) changedId.push_back(a.id);

		}
	}
};
