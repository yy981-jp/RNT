#pragma once
#include <def/entry.h>
#include <util/fs.h>

#include <unordered_map>


struct DepTrace {
	// 実行順に並んだEntry
	std::vector<EntryId> chain;

	// loop部分のEntry数
	size_t loopSize = 0;

	bool looped() const {
		return loopSize != 0;
	}
};

struct Location {
	EntryId parent;
	std::string name;

	bool operator==(const Location&) const = default;
};

struct LocationHash {
	size_t operator()(const Location& loc) const {
		size_t h1 = std::hash<uint64_t>{}(loc.parent);
		size_t h2 = std::hash<std::string>{}(loc.name);

		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
	}
};


class Solver {
	const std::vector<Entry> &before, &changed;
	const size_t entry_size;
	const fs::path targetDir;

	bool ok = false;
	std::string errorMsg;

	std::vector<EntryId> changedId;

	std::unordered_map<Location, EntryId, LocationHash> beforeLocation;

	std::vector<FsOperate> fsOperates;

	/*
		dependencies[a].push_back(b);
		aを実行するにはbの処理を先に行わなければならない
	*/
	std::vector<EntryId> dependencies;

	fs::path createEvPath(const EntryId& id) {
		return getTempRoot(targetDir) / "ev" / std::to_string(id);
	}


	/// @brief 変更のあるEntryを取り出す
	void diff();

	/// @brief 最終状態の一意性を検証
	void check_collide();

	/// @brief 現在位置の占有者から依存関係を作る
	void gen_dep();

	/// @brief fs命令を完成させる
	void solveDep();
	DepTrace traceDep(EntryId targetId, std::vector<bool>& processed);
	void solveChain(const std::vector<EntryId>& chain);
	void solveLoop(const DepTrace& trace);

public:
	Solver(const std::vector<Entry>& before, const std::vector<Entry>& changed, const fs::path targetDir):
		before(before), changed(changed), entry_size(before.size()), targetDir(targetDir) {
			diff();
		}

	bool isChanged() {
		return changedId.empty();
	}
	std::vector<FsOperate>& solve();
};
