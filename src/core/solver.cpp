#include <core/solver.h>

#include <algorithm>


void Solver::diff() {
	for (size_t i = 0; i < before.size(); i++) {
		const Entry& a = before[i];
		const Entry& b = changed[i];

		if (a.isDir ^ b.isDir) throw std::runtime_error(
			"Changing a directory to a file, or vice versa, is not supported."
		);

		if (
			(a.parent != b.parent) ||
			(a.name != b.name)
		) changedId.push_back(a.id);

	}
}

void Solver::check_collide() {
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
			throw std::runtime_error("Final Path collision.");
	}
}

/// @brief 現在位置の占有者から依存関係を作る
void Solver::gen_dep() {
	dependencies.assign(entry_size,EntryId::INVALID());
	for (EntryId id: changedId) {
		const Entry& e = changed[id];
		const Entry& beforeEntry = before[id];

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
		dependencies[id] = owner;
	}
}

void Solver::solveDep() {
	std::vector<bool> processed(entry_size, false);
	// 高速化のため
	fsOperates.reserve(entry_size);

	for (const EntryId& targetId: changedId) {
		DepTrace trace = traceDep(targetId, processed);

		if (trace.looped()) solveLoop(trace);
		else solveChain(trace.chain);

	}
}

DepTrace Solver::traceDep(EntryId targetId, std::vector<bool>& processed) {
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

void Solver::solveChain(const std::vector<EntryId>& chain) {
	for (auto& e: chain) {
		fsOperates.emplace_back(FsOperate{
			.from = solvePath(before,e),
			.to = solvePath(changed,e)
		});
	}
}

void Solver::solveLoop(const DepTrace& trace) {
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

std::vector<FsOperate>& Solver::solve() {
	try {
		check_collide();
		gen_dep();
		solveDep();
	} catch (const std::runtime_error& e) {
		throw std::runtime_error(std::string{"Error: Solver: "} + e.what());
	}
	
	return fsOperates;
}
