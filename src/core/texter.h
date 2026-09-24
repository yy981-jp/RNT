#pragma once
#include <vector>

#include <core/config.h>
#include <core/dir.h>
#include <def/entry.h>


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
	std::vector<Entry> changedEntries;

	size_t idIndentWidth = 0;

	bool ok = false;
	std::string errorMsg;
	size_t line_pos = 0;

	void error(const std::string& str) {
		errorMsg = std::format("Line %d:  %s", line_pos+1, str.c_str());
		ok = false;
	}

	void solve(std::string_view str);

public:
	Texter(const std::vector<Entry>& entries): entries(entries) {}

	std::string getText();
	std::vector<Entry>& edit(Config& config, const fs::path& target);

	std::string status() {
		if (ok) return {};
		return errorMsg;
	}
};
