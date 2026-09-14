#pragma once
#include <vector>
#include <algorithm>
#include <format>
#include <stdexcept>

#include <def/entity.h>


class BaseNum {
	// RNT 予約文字
	inline static const std::vector<std::string> reserved = {
		"\\", ":", "*", "?", "<", ">", "|"
	};

public:

	static size_t size() {
		return reserved.size();
	}

	static std::string encode(uint64_t num) {
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

	static uint64_t decode(std::string_view str) {
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

};


class Texter {
	const std::vector<Entry>& entries;

public:
	Texter(const std::vector<Entry>& entries): entries(entries) {}

	std::string getText() {
		std::string result;
		// あらかじめ大きなメモリ領域を一括で確保しておく
		result.reserve(entries.size() * 64);

		for (const auto& e: entries) {
			// インデントの追加
			result.append(e.depth * 2, ' ');
			
			// 一時オブジェクトを作らず直接 result へ追加
			result.push_back('"');
			result.append(e.name);
			result.push_back('"');
			if (e.isDir) result.push_back('/');
			
			result.append("		  ");
			result.append(BaseNum::encode(e.id.value));
			result.push_back('\n');
		}

		return result;
	}
};