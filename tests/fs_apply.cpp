#include <core/fs.h>
#include <core/solver.h>
#include <util/fs.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


namespace {
	void require(bool condition, const std::string& message) {
		if (!condition) throw std::runtime_error(message);
	}

	std::string readFile(const fs::path& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Couldn't open test file: " + path.string());

		return std::string(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()
		);
	}

	void test_parent_move_and_child_rename(const fs::path& workspace) {
		fs::create_directories(workspace / "orig1");
		fs::create_directories(workspace / "root");
		{
			std::ofstream file(workspace / "orig1" / "example.txt", std::ios::binary);
			file << "payload";
		}

		const std::vector<Entry> before = {
			{.id = EntryId{0}, .parent = EntryId::INVALID(), .name = "orig1", .depth = 0, .isDir = true},
			{.id = EntryId{1}, .parent = EntryId{0}, .name = "example.txt", .depth = 1, .isDir = false},
			{.id = EntryId{2}, .parent = EntryId::INVALID(), .name = "root", .depth = 0, .isDir = true}
		};

		const std::vector<Entry> changed = {
			{.id = EntryId{0}, .parent = EntryId{2}, .name = "orig1", .depth = 1, .isDir = true},
			{.id = EntryId{1}, .parent = EntryId::INVALID(), .name = "changed.txt", .depth = 0, .isDir = false},
			{.id = EntryId{2}, .parent = EntryId::INVALID(), .name = "root", .depth = 0, .isDir = true}
		};

		const fs::path previous = fs::current_path();
		fs::current_path(workspace);

		{
			RNT_Dir temp(workspace);
			Solver solver(before, changed, workspace);
			const std::vector<FsOperate> operations = solver.solve();
			require(operations.size() == 2, "Expected two filesystem operations.");

			FsExecutor preview(before);
			const auto first = preview.preview(operations[0]);
			const auto second = preview.preview(operations[1]);

			require(first.first == fs::path("orig1"), "Unexpected source for directory move.");
			require(first.second == fs::path("root/orig1"), "Unexpected destination for directory move.");
			require(second.first == fs::path("root/orig1/example.txt"), "Child source wasn't resolved after moving its parent.");
			require(second.second == fs::path("changed.txt"), "Unexpected destination for child rename.");

			FsExecutor executor(before);
			for (const auto& operation: operations) executor.apply(operation);
		}

		fs::current_path(previous);
		require(!fs::exists(workspace / "orig1"), "Original directory should have been moved.");
		require(fs::exists(workspace / "root" / "orig1"), "Moved directory should exist under root.");
		require(readFile(workspace / "changed.txt") == "payload", "Moved file contents changed.");
		require(!fs::exists(workspace / "root" / "orig1" / "example.txt"), "Moved file should no longer be in the old directory.");
	}

	void test_cyclic_rename(const fs::path& workspace) {
		fs::create_directories(workspace);
		{
			std::ofstream(workspace / "A", std::ios::binary) << "from A";
			std::ofstream(workspace / "B", std::ios::binary) << "from B";
		}

		const std::vector<Entry> before = {
			{.id = EntryId{0}, .parent = EntryId::INVALID(), .name = "A", .depth = 0, .isDir = false},
			{.id = EntryId{1}, .parent = EntryId::INVALID(), .name = "B", .depth = 0, .isDir = false}
		};

		std::vector<Entry> changed = before;
		changed[0].name = "B";
		changed[1].name = "A";

		const fs::path previous = fs::current_path();
		fs::current_path(workspace);

		{
			RNT_Dir temp(workspace);
			Solver solver(before, changed, workspace);
			const std::vector<FsOperate> operations = solver.solve();

			FsExecutor executor(before);
			for (const auto& operation: operations) executor.apply(operation);
		}

		fs::current_path(previous);
		require(readFile(workspace / "A") == "from B", "Cyclic rename produced an incorrect A file.");
		require(readFile(workspace / "B") == "from A", "Cyclic rename produced an incorrect B file.");
	}
}


int main() {
	const fs::path previous = fs::current_path();
	const fs::path workspace = fs::temp_directory_path() / "rnt-fs-apply-tests";

	try {
		fs::remove_all(workspace);
		fs::create_directories(workspace);
		test_parent_move_and_child_rename(workspace / "parent-move");
		test_cyclic_rename(workspace / "cycle");
		fs::current_path(previous);
		fs::remove_all(workspace);
		std::cout << "RNT filesystem operation tests passed.\n";
		return 0;
	} catch (const std::exception& e) {
		fs::current_path(previous);
		fs::remove_all(workspace);
		std::cerr << e.what() << "\n";
		return 1;
	}
}
