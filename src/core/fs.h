#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <def/entry.h>


struct FsOperate {
	EntryId id;
	EntryId parent = EntryId::INVALID();
	std::string name;
	bool isDir = false;
	std::optional<fs::path> targetPath;
};


class FsExecutor {
	struct Location {
		EntryId parent = EntryId::INVALID();
		std::string name;
		bool isDir = false;
		EntryId aliasOf = EntryId::INVALID();
		std::optional<fs::path> fixedPath;
	};

	std::vector<Location> current;

	EntryId canonicalId(EntryId id) const;
	EntryId findDirectoryAt(const fs::path& path, EntryId excluding) const;
	fs::path currentPath(EntryId id) const;
	std::pair<fs::path, fs::path> resolve(const FsOperate& op) const;
	void advance(const FsOperate& op);
	void alias(EntryId source, EntryId destination);

public:
	explicit FsExecutor(const std::vector<Entry>& before);

	/// @brief 次の操作を仮想的に適用し、表示用の元パスと先パスを返す
	std::pair<fs::path, fs::path> preview(const FsOperate& op);

	/// @brief 次の操作を実ファイルシステムに適用する
	void apply(const FsOperate& op);
};
