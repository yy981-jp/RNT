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

	std::unordered_map<Location, EntryId, LocationHash> beforeLocation;

	/*
		dependencies[a.value].push_back(b);
		aを実行するにはbの処理を先に行わなければならない
	*/
	std::vector<std::vector<EntryId>> dependencies;

	void error(const std::string& str) {
		errorMsg = str;
		ok = false;
	}

	/// @brief 最終状態の一意性を検証
	void check_collide() {
		for (const auto& e: before) {
			beforeLocation[{e.parent, e.name}] = e.id;
		}

		std::unordered_map<Location, EntryId, LocationHash> changedLocation;

		for (const auto& e: changed) {
			auto [it, inserted] = changedLocation.emplace(
				Location{e.parent, e.name},
				e.id
			);

			if (!inserted)
				return error("Final Path collision.");
		}
	}

	/// @brief 現在位置の占有者から依存関係を作る
	void gen_dep() {
		for (EntryId id: changedId) {
			const Entry& e = changed[id.value];
			const Entry& beforeEntry = before[id.value];

			Location target{
				e.parent,
				e.name
			};

			auto it = beforeLocation.find(target);

			if (it == beforeLocation.end())
				continue;

			EntryId owner = it->second;

			// 自分自身なら依存不要
			if (owner == id)
				continue;

			// ownerが移動した後でentryを移動する必要がある
			dependencies[id.value].push_back(owner);
		}
	}

	/// @brief 変更のあるEntryを取り出す
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


public:
	Solver(const std::vector<Entry>& before, const std::vector<Entry>& changed):
		before(before), changed(changed) {}

	void debug() {
		for (auto e: changedId) printf("%llu, ", e.value);
		printf("\n");
	}

	void solve() {
		diff();
		if (!ok)
			return;

		check_collide();
		if (!ok)
			return;

		gen_dep();
		if (!ok)
			return;

		// TODO: dependency graphから実行順序を決める
	}

};
