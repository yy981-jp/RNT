#include <filesystem>
#include <string>
#include <vector>
#include <stack>
#include <cstdint>

namespace fs = std::filesystem;

// disable_recursion_pending


using EntryId = uint64_t;

struct Entry {
	EntryId id;
	std::string name;
};


int main() {
	fs::path cd = fs::current_path();

	std::vector<Entry> entries;
	std::stack<EntryId> parents;
	int lastNest = 0;
	for (const auto& e: fs::recursive_directory_iterator(cd)) {

	}
}
