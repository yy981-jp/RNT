#include <iostream>
#include <fstream>

#include <core/dir.h>
#include <core/texter.h>
#include <core/launcher.h>
#include <core/config.h>


struct SDLApp {
	SDLApp() { SDL_Init(0); }
	~SDLApp() { SDL_Quit(); }
};


int main(int argc, char *argv[]) {
	SDLApp sdlapp;

	Config config((getDataPath() / "config.json").string());

	fs::path target;
	if (argc < 2) target = fs::current_path();
	else target = fs::path(argv[1]);

	Ctx ctx{};
	dir(ctx, target);

	Texter texter(ctx.entries);

	// ユーザーに操作させる
	std::vector<Entry> changedEntries = texter.edit(config, target);

	

}
