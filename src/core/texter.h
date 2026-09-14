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
		if (num == 0)
			return reserved[0];

		std::string result;

		while (num > 0) {
			result += reserved[num % reserved.size()];
			num /= reserved.size();
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
		
		for (const auto& e: entries) {
			// std::format("%s", )
		}
	}
};
