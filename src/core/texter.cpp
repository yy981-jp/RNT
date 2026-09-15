#include <core/texter.h>

#include <algorithm>
#include <stdexcept>


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

	const size_t maxIndent = BaseNum::encode(entries.back().id.value).size() / 4;

	for (const auto& e: entries) {
		// id
		std::string id = BaseNum::encode(e.id.value);
		size_t idSize = id.size();
		result.append(id);

		// id indent
		int tab_num = ( maxIndent - (idSize / 4) ) + 2;
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
