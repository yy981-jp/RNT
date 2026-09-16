#pragma once

#include <def/fs.h>
#include <util/json.h>

#include <SDL3/SDL.h>


inline fs::path getDataPath() {
	char* path = SDL_GetPrefPath("yy981", "RNT");
	if (!path)
		throw std::runtime_error(SDL_GetError());

	fs::path result = path;
	SDL_free(path);
	return result;
}


class Config {
	json config;
	json baseConfig;
	std::string configPath;

	void init();

public:
	Config(const std::string& path): configPath(path) {
		baseConfig = readJson("system.json");
		if (!fs::exists(path) || fs::is_empty(path)) init();
		config = readJson(path);
		if (config.empty()) init();
	}

	json& get() { return config; }
	void save() { writeJson(config, configPath); }

};
