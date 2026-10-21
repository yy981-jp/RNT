#include <core/fs.h>

#include <stdexcept>


FsExecutor::FsExecutor(const std::vector<Entry>& before) {
	current.resize(before.size());

	for (const Entry& e: before) {
		const size_t id = static_cast<size_t>(e.id.value);
		if (id >= current.size()) throw std::runtime_error("FsExecutor: invalid EntryId.");

		current[id] = Location{
			.parent = e.parent,
			.name = e.name
		};
	}
}

fs::path FsExecutor::currentPath(EntryId id) const {
	if (!id.isValid()) throw std::runtime_error("FsExecutor: invalid EntryId.");

	const size_t index = static_cast<size_t>(id.value);
	if (index >= current.size()) throw std::runtime_error("FsExecutor: EntryId is out of range.");

	const Location& location = current[index];
	if (location.fixedPath.has_value()) return *location.fixedPath;

	if (location.parent.isValid())
		return currentPath(location.parent) / location.name;

	return location.name;
}

std::pair<fs::path, fs::path> FsExecutor::resolve(const FsOperate& op) const {
	const fs::path from = currentPath(op.id);

	fs::path to;
	if (op.targetPath.has_value()) {
		to = *op.targetPath;
	} else if (op.parent.isValid()) {
		to = currentPath(op.parent) / op.name;
	} else {
		to = op.name;
	}

	return {from, to};
}

void FsExecutor::advance(const FsOperate& op) {
	const size_t index = static_cast<size_t>(op.id.value);
	if (index >= current.size()) throw std::runtime_error("FsExecutor: EntryId is out of range.");

	Location& location = current[index];

	if (op.targetPath.has_value()) {
		location.parent = EntryId::INVALID();
		location.name.clear();
		location.fixedPath = op.targetPath;
		return;
	}

	location.parent = op.parent;
	location.name = op.name;
	location.fixedPath.reset();
}

std::pair<fs::path, fs::path> FsExecutor::preview(const FsOperate& op) {
	const auto paths = resolve(op);
	advance(op);
	return paths;
}

void FsExecutor::apply(const FsOperate& op) {
	const auto [from, to] = resolve(op);
	fs::rename(from, to);
	advance(op);
}
