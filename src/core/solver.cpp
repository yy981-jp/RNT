#include <core/solver.h>


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
