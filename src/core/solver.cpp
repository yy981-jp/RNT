#include <core/solver.h>

#include <algorithm>
#include <unordered_map>


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
	for (const auto& e: before)
		beforeLocation[{e.parent, e.name}] = e.id;

	std::unordered_map<fs::path, EntryId> changedLocation;

	for (const auto& e: changed) {
		const fs::path path = solvePath(changed, e.id).lexically_normal();
		auto [it, inserted] = changedLocation.emplace(path, e.id);

		if (!inserted) {
			const Entry& owner = changed[it->second];
			if (!owner.isDir || !e.isDir)
				throw std::runtime_error("Final Path collision: " + path.string());
		}
	}
}

/// @brief 現在位置の占有者から依存関係を作る
void Solver::gen_dep() {
	dependencies.assign(entry_size,EntryId::INVALID());

	for (EntryId id: changedId) {
		const Entry& e = changed[id];

		Location target{
			e.parent,
			e.name
		};

		auto it = beforeLocation.find(target);

		if (it == beforeLocation.end())
			continue;

		const EntryId owner = it->second;

		// 自分自身なら依存不要
		if (owner == id)
			continue;

		// ownerが移動した後でentryを移動する必要がある
		dependencies[id] = owner;
	}

	// If multiple directories have the same final path, make every other
	// directory wait for a stable merge target. Prefer the entry already at
	// that path, then one whose name does not change.
	std::unordered_map<fs::path, std::vector<EntryId>> finalDirectories;
	for (const Entry& e: changed) {
		if (!e.isDir) continue;
		finalDirectories[solvePath(changed, e.id).lexically_normal()].push_back(e.id);
	}

	for (const auto& [path, ids]: finalDirectories) {
		if (ids.size() < 2) continue;

		auto score = [&](EntryId id) {
			const Entry& old = before[id];
			const Entry& now = changed[id];
			if (solvePath(before, id).lexically_normal() == path)
				return 0;
			if (old.name == now.name)
				return 1;
			return 2;
		};

		EntryId winner = ids.front();
		for (EntryId id: ids) {
			if (score(id) < score(winner))
				winner = id;
		}

		for (EntryId id: ids) {
			if (id == winner)
				continue;
			dependencies[id] = winner;
		}
	}
}

void Solver::solveDep() {
	std::vector<bool> processed(entry_size, false);
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
		if (processed[curId])
			break;

		processed[curId] = true;
		result.chain.push_back(curId);

		const EntryId& dependOn = dependencies[curId];

		if (!dependOn.isValid())
			break;

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
	for (const auto& e: chain) {
		fsOperates.emplace_back(FsOperate{
			.id = e,
			.parent = changed[e].parent,
			.name = changed[e].name,
			.isDir = changed[e].isDir
		});
	}
}

void Solver::solveLoop(const DepTrace& trace) {
	const auto& chain = trace.chain;
	const size_t loopSize = trace.loopSize;

	const EntryId& edge = chain.front();
	const fs::path tmp = createEvPath(edge);

	fsOperates.emplace_back(FsOperate{
		.id = edge,
		.isDir = changed[edge].isDir,
		.targetPath = tmp
	});

	for (size_t i = 1; i < loopSize; ++i) {
		const EntryId& e = chain[i];

		fsOperates.emplace_back(FsOperate{
			.id = e,
			.parent = changed[e].parent,
			.name = changed[e].name,
			.isDir = changed[e].isDir
		});
	}

	fsOperates.emplace_back(FsOperate{
		.id = edge,
		.parent = changed[edge].parent,
		.name = changed[edge].name,
		.isDir = changed[edge].isDir
	});

	// loopの外側
	for (size_t i = loopSize; i < chain.size(); ++i) {
		const EntryId& e = chain[i];

		fsOperates.emplace_back(FsOperate{
			.id = e,
			.parent = changed[e].parent,
			.name = changed[e].name,
			.isDir = changed[e].isDir
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
