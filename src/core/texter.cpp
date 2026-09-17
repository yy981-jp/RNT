#include <core/texter.h>
#include <core/launcher.h>
#include <util/json.h>

#include <algorithm>
#include <stdexcept>
#include <fstream>


namespace {
	constexpr int MARGIN = 2, TABSIZE = 4;
}





const std::vector<std::string> BaseNum::reserved = {
	"\\", ":", "*", "?", "<", ">", "|"
};

size_t BaseNum::size() {
	return reserved.size();
}

std::string BaseNum::encode(uint64_t num) {
	const size_t base = reserved.size();

	if (num == 0)
		return std::string(1, reserved[0][0]);

	std::string result;
	result.reserve(22);

	while (num > 0) {
		result.push_back(reserved[num % base][0]);
		num /= base;
	}

	std::reverse(result.begin(), result.end());
	return result;
}

uint64_t BaseNum::decode(std::string_view str) {
	uint64_t result = 0;

	for (char c : str) {
		auto it = std::ranges::find_if(reserved, [c](const auto& s) {
			return s[0] == c;
		});

		if (it == reserved.end())
			throw std::invalid_argument("invalid BaseNum character");

		const auto digit = static_cast<uint64_t>(
			std::distance(reserved.begin(), it)
		);

		result = result * reserved.size() + digit;
	}

	return result;
}





std::string Texter::getText() {
	std::string result;
	// あらかじめ大きなメモリ領域を一括で確保しておく
	result.reserve(entries.size() * 64);

	idIndentWidth = BaseNum::encode(entries.back().id.value).size() / TABSIZE;

	for (const auto& e: entries) {
		// id
		std::string id = BaseNum::encode(e.id.value);
		size_t idSize = id.size();
		result.append(id);

		// id indent
		int tab_num = ( idIndentWidth - (idSize / TABSIZE) ) + MARGIN;
		result.append(tab_num, '\t');

		// file indent
		// result.push_back('|');
		result.append(e.depth, '\t');
		
		// name
		result.push_back('"');
		result.append(e.name);
		result.push_back('"');
		if (e.isDir) result.push_back('/');
		
		result.push_back('\n');
	}

	return result;
}


std::vector<Entry>& Texter::edit(Config& config, const fs::path& target) {
	std::string text = getText();

	fs::path tempF = fs::temp_directory_path() / std::format("RNT-{}.txt", std::hash<fs::path>{}(target));
	{
		std::ofstream ofs(tempF);
		if (!ofs) throw std::runtime_error("RNT couldn't open temp file. (ofs)");
		ofs << text;
		ofs.flush();
	}

	json te = config.getSys()["editor"][config.get().at("editor").get<int>()];
	launchTextEditor(te["path"].get<std::string>(), te["arg"].get<std::string>(), tempF.string());

	text.clear();
	{
		std::ifstream ifs(tempF);
		if (!ifs) throw std::runtime_error("RNT couldn't open temp file. (ifs)");

		text.assign(
			std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>()
		);
	}

	// solve
	changedEntries.resize(entries.size());
	solve(text);
	
	return changedEntries;
}


void Texter::solve(std::string_view str) {
	ok = true;
	line_pos = 0;

	std::stack<EntryId> parents;
	std::vector<size_t> entryToIdx;
	entryToIdx.reserve(entries.size());
	

	for (auto e: str | std::views::split('\n')) {
		auto first = std::ranges::begin(e);
		auto last = std::ranges::end(e);
		std::string_view line(first, last);

		if (line.empty()) continue;

		// EntryId
		size_t id_end = line.find('\t');
		if (id_end == std::string_view::npos) return error("The end of EntryId could not be found.");
		EntryId id;
		id.value = BaseNum::decode(line.substr(0, id_end));

		// name
		size_t name_begin = line.find('"');
		if (name_begin == std::string_view::npos) return error("The begin of name could not be found.");

		size_t name_end = line.find_last_of('"');
		if (name_end == std::string_view::npos || name_end <= name_begin)
			return error("The end of name could not be found.");

		std::string_view name =
			line.substr(name_begin + 1, name_end - name_begin - 1);
		if (name.empty()) return error("Name is empty");

		// depth
		int id_tab = idIndentWidth - static_cast<int>(id_end / TABSIZE) + MARGIN;
		int depth = static_cast<int>(name_begin - id_end - id_tab);

		// printf("%llu: %d\n", id.value, depth);

		/*
			現在のdepth以上の親は、
			現在のEntryのparentにはならない

			例:
			    A/  depth 0
			        B/  depth 1
			            C  depth 2
			            D  depth 2
			        E  depth 1

			Dを処理するとき parents = [A,B]
			Eを処理するとき Bは同じdepth1なのでpopする
		*/
		while (!parents.empty()) {
			EntryId parent_id = parents.top();

			if (changedEntries[parent_id.value].depth < depth) break;

			parents.pop();
		}

		EntryId parent =
			parents.empty()
				? EntryId::INVALID()
				: parents.top();

		Entry ent_data{
			.id = id,
			.parent = parent,
			.name = std::string{name},
			.depth = depth
		};

		auto& ent = changedEntries[id.value];
		ent = std::move(ent_data);

		// dir判定
		if (line[name_end] == '/') {
			ent.isDir = true;
			parents.push(ent.id);
		}

		line_pos++;
	}

	if (line_pos != entries.size()) return error(
		std::format(
			"The number of entries does not match the number before the change."
			" (%d -> %d)", entries.size(), line_pos)
	);
}
