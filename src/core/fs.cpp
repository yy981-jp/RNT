#include <core/fs.h>

#include <system_error>
#include <stdexcept>


namespace {
	fs::file_status symlinkStatus(const fs::path& path) {
		std::error_code ec;
		const fs::file_status status = fs::symlink_status(path, ec);

		if (ec == std::errc::no_such_file_or_directory)
			return fs::file_status(fs::file_type::not_found);
		if (ec)
			throw fs::filesystem_error("cannot inspect path", path, ec);

		return status;
	}

	bool pathExists(const fs::path& path) {
		const fs::file_type type = symlinkStatus(path).type();
		return type != fs::file_type::not_found && type != fs::file_type::none;
	}

	bool isDirectoryPath(const fs::path& path) {
		// Do not follow symlinks when deciding whether a merge is safe.
		return symlinkStatus(path).type() == fs::file_type::directory;
	}

	[[noreturn]] void throwCollision(const fs::path& source, const fs::path& destination) {
		throw fs::filesystem_error(
			"cannot merge: destination entry already exists",
			source,
			destination,
			std::make_error_code(std::errc::file_exists)
		);
	}

	void validateMerge(const fs::path& source, const fs::path& destination) {
		for (const auto& entry: fs::directory_iterator(source)) {
			const fs::path sourceChild = entry.path();
			const fs::path destinationChild = destination / sourceChild.filename();

			if (!pathExists(destinationChild))
				continue;

			if (!isDirectoryPath(sourceChild) || !isDirectoryPath(destinationChild))
				throwCollision(sourceChild, destinationChild);

			validateMerge(sourceChild, destinationChild);
		}
	}

	void mergeDirectoryContents(const fs::path& source, const fs::path& destination) {
		std::vector<fs::path> children;
		for (const auto& entry: fs::directory_iterator(source))
			children.push_back(entry.path());

		for (const fs::path& sourceChild: children) {
			const fs::path destinationChild = destination / sourceChild.filename();

			if (pathExists(destinationChild)) {
				if (!isDirectoryPath(sourceChild) || !isDirectoryPath(destinationChild))
					throwCollision(sourceChild, destinationChild);

				// The recursive call removes sourceChild after merging its contents.
				mergeDirectoryContents(sourceChild, destinationChild);
				continue;
			}

			fs::rename(sourceChild, destinationChild);
		}

		fs::remove(source);
	}
}


FsExecutor::FsExecutor(const std::vector<Entry>& before) {
	current.resize(before.size());

	for (const Entry& e: before) {
		const size_t id = static_cast<size_t>(e.id.value);
		if (id >= current.size()) throw std::runtime_error("FsExecutor: invalid EntryId.");

		current[id] = Location{
			.parent = e.parent,
			.name = e.name,
			.isDir = e.isDir
		};
	}
}

EntryId FsExecutor::canonicalId(EntryId id) const {
	for (size_t i = 0; i < current.size(); ++i) {
		if (!id.isValid()) break;

		const size_t index = static_cast<size_t>(id.value);
		if (index >= current.size())
			throw std::runtime_error("FsExecutor: EntryId is out of range.");

		const EntryId next = current[index].aliasOf;
		if (!next.isValid()) return id;
		id = next;
	}

	throw std::runtime_error("FsExecutor: directory alias cycle.");
}

fs::path FsExecutor::currentPath(EntryId id) const {
	id = canonicalId(id);
	if (!id.isValid()) throw std::runtime_error("FsExecutor: invalid EntryId.");

	const size_t index = static_cast<size_t>(id.value);
	if (index >= current.size()) throw std::runtime_error("FsExecutor: EntryId is out of range.");

	const Location& location = current[index];
	if (location.fixedPath.has_value()) return *location.fixedPath;

	if (location.parent.isValid())
		return currentPath(location.parent) / location.name;

	return location.name;
}

EntryId FsExecutor::findDirectoryAt(const fs::path& path, EntryId excluding) const {
	const fs::path normalized = path.lexically_normal();
	const EntryId excluded = excluding.isValid() ? canonicalId(excluding) : EntryId::INVALID();

	for (size_t i = 0; i < current.size(); ++i) {
		const EntryId candidate{static_cast<uint64_t>(i)};
		const EntryId canonical = canonicalId(candidate);
		if (canonical != candidate || canonical == excluded)
			continue;

		if (!current[canonical.value].isDir)
			continue;

		if (currentPath(canonical).lexically_normal() == normalized)
			return canonical;
	}

	return EntryId::INVALID();
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
	location.aliasOf = EntryId::INVALID();

	if (op.targetPath.has_value()) {
		location.parent = EntryId::INVALID();
		location.name.clear();
		location.fixedPath = op.targetPath;
		return;
	}

	location.parent = op.parent;
	location.name = op.name;
	location.isDir = op.isDir;
	location.fixedPath.reset();
}

void FsExecutor::alias(EntryId source, EntryId destination) {
	const EntryId target = canonicalId(destination);
	if (canonicalId(source) == target)
		return;

	Location& location = current[static_cast<size_t>(source.value)];
	location.parent = EntryId::INVALID();
	location.name.clear();
	location.fixedPath.reset();
	location.aliasOf = target;
	location.isDir = true;
}

std::pair<fs::path, fs::path> FsExecutor::preview(const FsOperate& op) {
	const auto paths = resolve(op);

	if (!op.targetPath.has_value() && op.isDir && paths.first.lexically_normal() != paths.second.lexically_normal()) {
		const EntryId target = findDirectoryAt(paths.second, op.id);
		if (target.isValid()) {
			alias(op.id, target);
			return paths;
		}
	}

	advance(op);
	return paths;
}

void FsExecutor::apply(const FsOperate& op) {
	const auto [from, to] = resolve(op);

	if (from.lexically_normal() == to.lexically_normal()) {
		advance(op);
		return;
	}

	if (!pathExists(from))
		throw fs::filesystem_error(
			"cannot rename: source does not exist",
			from,
			std::make_error_code(std::errc::no_such_file_or_directory)
		);

	if (pathExists(to)) {
		if (op.isDir && isDirectoryPath(from) && isDirectoryPath(to)) {
			// Find the logical target first. If it is represented by an Entry,
			// alias the merged source to it so later moves of the target are followed.
			const EntryId target = findDirectoryAt(to, op.id);

			// Validate the whole merge before moving anything, so conflicts inside
			// a nested directory do not leave this merge half-applied.
			validateMerge(from, to);
			mergeDirectoryContents(from, to);

			if (target.isValid())
				alias(op.id, target);
			else
				advance(op);

			return;
		}

		throwCollision(from, to);
	}

	fs::rename(from, to);
	advance(op);
}
