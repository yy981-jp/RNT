#pragma once
#include <def/entry.h>
#include <util/fs.h>

#include <unordered_map>
#include <algorithm>


// TODO:
namespace {
	void solveChain(const std::vector<EntryId>& id) {}
	void solveLoop(const std::vector<EntryId>& id) {}
}


struct FsOperate {
	fs::path from, to;
};


enum class TraceResult {
	complete,
	loop,
	processed,
};

struct DepTrace {
	std::vector<EntryId> chain;
	TraceResult result;
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
	const size_t entry_size;
	const fs::path targetDir;

	bool ok = false;
	std::string errorMsg;

	std::vector<EntryId> changedId;

	std::unordered_map<Location, EntryId, LocationHash> beforeLocation;

	std::vector<FsOperate> fsOperates;

	/*
		dependencies[a.value].push_back(b);
		aを実行するにはbの処理を先に行わなければならない
	*/
	std::vector<EntryId> dependencies;

	fs::path createEvPath(const EntryId& id) {
		return getTempRoot(targetDir) / "ev" / std::to_string(id);
	}


	void error(const std::string& str) {
		errorMsg = str;
		ok = false;
	}

	/// @brief 変更のあるEntryを取り出す
	void diff();

	/// @brief 最終状態の一意性を検証
	void check_collide();

	/// @brief 現在位置の占有者から依存関係を作る
	void gen_dep();


	DepTrace traceDep(EntryId targetId) {
		std::vector<bool> processed(entry_size);
		DepTrace result;
		EntryId curId = targetId;

		while (curId.isValid()) {
			if (processed[curId]) {
				result.result = TraceResult::processed;
				break;
			}

			const EntryId& dependOn = dependencies[curId];

			if (!dependOn.isValid()) {
				result.result = TraceResult::complete;
				break;
			}

			if (std::ranges::contains(result.chain, dependOn)) {
				result.result = TraceResult::loop;
				break;
			}

			result.chain.push_back(curId);
			curId = dependOn;
		}

		return result;
	}

	/// @brief fs命令を完成させる
	void solveDep() {
		std::vector<bool> processed(entry_size);

		for (const EntryId& targetId: changedId) {
			DepTrace trace = traceDep(targetId);

			switch (trace.result) {
			case TraceResult::complete:
				solveChain(trace.chain);
				break;

			case TraceResult::loop:
				solveLoop(trace.chain);
				break;

			case TraceResult::processed:
				solveChain(trace.chain);
				break;
			}
		}
	}

public:
	Solver(const std::vector<Entry>& before, const std::vector<Entry>& changed, const fs::path targetDir):
		before(before), changed(changed), entry_size(before.size()), targetDir(targetDir) {}

	void debug() {
		for (auto e: changedId) printf("%llu, ", e.value);
		printf("\n");
	}

	std::vector<FsOperate>& solve() {
		diff();
		check_collide();
		gen_dep();
		solveDep();
		return fsOperates;
	}

};
