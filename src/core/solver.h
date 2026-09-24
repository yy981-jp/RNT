#pragma once
#include <def/entry.h>
#include <util/fs.h>

#include <unordered_map>
#include <algorithm>


struct FsOperate {
	fs::path from, to;
};


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


	DepTrace traceDep(EntryId targetId, std::vector<bool>& processed) {
		DepTrace result;
		EntryId curId = targetId;

		while (curId.isValid()) {
			if (processed[curId]) {
				break;
			}

			processed[curId] = true;
			result.chain.push_back(curId);

			const EntryId& dependOn = dependencies[curId];

			// 依存関係の終端
			if (!dependOn.isValid()) break;

			auto it = std::ranges::find(result.chain, dependOn);

			if (it != result.chain.end()) {
				const size_t findPos = it - result.chain.begin();
				result.loopSize = result.chain.size() - findPos;
				break;
			}

			curId = dependOn;
		}

		std::ranges::reverse(result.chain);
		return result;
	}

	/// @brief fs命令を完成させる
	void solveDep() {
		std::vector<bool> processed(entry_size, false);
		// 高速化のため
		fsOperates.reserve(entry_size);

		for (const EntryId& targetId: changedId) {
			DepTrace trace = traceDep(targetId, processed);

			if (trace.looped()) solveLoop(trace);
			else solveChain(trace.chain);

		}
	}


	void solveChain(const std::vector<EntryId>& chain) {
		for (auto& e: chain) {
			fsOperates.emplace_back(FsOperate{
				.from = solvePath(before,e),
				.to = solvePath(changed,e)
			});
		}
	}

	void solveLoop(const DepTrace& trace) {
		const auto& chain = trace.chain;
		const size_t loopSize = trace.loopSize;

		const EntryId& edge = chain.front();
		fs::path tmp = createEvPath(edge);

		fsOperates.emplace_back(FsOperate{
			.from = solvePath(before, edge),
			.to = tmp,
		});

		for (size_t i = 1; i < loopSize; ++i) {
			const EntryId& e = chain[i];

			fsOperates.emplace_back(FsOperate{
				.from = solvePath(before, e),
				.to = solvePath(changed, e),
			});
		}

		fsOperates.emplace_back(FsOperate{
			.from = tmp,
			.to = solvePath(changed, edge)
		});

		// loopの外側
		for (size_t i = loopSize; i < chain.size(); ++i) {
			const EntryId& e = chain[i];

			fsOperates.emplace_back(FsOperate{
				.from = solvePath(before, e),
				.to = solvePath(changed, e),
			});
		}
	}


public:
	Solver(const std::vector<Entry>& before, const std::vector<Entry>& changed, const fs::path targetDir):
		before(before), changed(changed), entry_size(before.size()), targetDir(targetDir) {}

	void debug() {
		for (auto e: changedId) printf("%llu, ", e);
		printf("\n");
	}

	std::vector<FsOperate>& solve() {
		try {
			diff();
			check_collide();
			gen_dep();
			solveDep();
		} catch (const std::runtime_error& e) {
			throw std::runtime_error(std::string{"Error: Solver: "} + e.what());
		}
		
		return fsOperates;
	}

};
