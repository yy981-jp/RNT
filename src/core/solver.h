#pragma once
#include <def/entry.h>

#include <unordered_map>


enum class OpType {
	rename, move,
};

struct Location {
	EntryId parent;
	std::string name;

	bool operator==(const Location&) const = default;
};

struct LocationHash {
	size_t operator()(const Location& loc) const {
		size_t h1 = std::hash<uint64_t>{}(loc.parent.value);
		size_t h2 = std::hash<std::string>{}(loc.name);

		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};


class Solver {
	const std::vector<Entry> &before, &changed;

	bool ok = false;
	std::string errorMsg;

	std::vector<EntryId> changedId;

	// beforeのパス → EntryId
	std::unordered_map<fs::path, EntryId> beforePath;
	// changedのパス → EntryId
	std::unordered_map<fs::path, EntryId> changedPath;

	std::unordered_map<EntryId, EntryId> dependency;

	void error(const std::string& str) {
		errorMsg = str;
		ok = false;
	}

	void genPathMap() {
		for (const auto& e: before) {
			beforePath[solvePath(before, e.id)];
		}
		for (const auto& e: changed) {
			changedPath[solvePath(changed, e.id)];
		}
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

	void solve() {
		std::unordered_map<fs::path, EntryId> beforePath;

		for (const Entry& e: before) {
			beforePath[solvePath(before, e.id)] = e.id;
		}

		for (const Entry& e: changed) {
			if (
				e.parent == before[e.id.value].parent &&
				e.name == before[e.id.value].name
			) continue;

			fs::path after = solvePath(changed, e.id);

			auto it = beforePath.find(after);

			if (it == beforePath.end()) {
				// 現在のentriesには存在しない
				// → 外部ファイルとの衝突かもしれない
				if (fs::exists(after))
					return error("Target path already exists.");

				continue;
			}

			if (it->second == e.id) {
				// 自分自身
				continue;
			}

			dependency[e.id] = it->second;
		}
	}	
};
