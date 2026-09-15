#pragma once
#include <vector>

#include <def/entity.h>


class BaseNum {
	// RNT 予約文字
	static const std::vector<std::string> reserved;

public:
	static size_t size();
	static std::string encode(uint64_t num);
	static uint64_t decode(std::string_view str);
};


class Texter {
	const std::vector<Entry>& entries;

public:
	Texter(const std::vector<Entry>& entries): entries(entries) {}

	std::string getText();
};
