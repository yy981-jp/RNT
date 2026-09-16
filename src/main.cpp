#include <iostream>

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

	Texter text(ctx.entries);
	// auto debug = resolvePath(ctx.entries[33].id, ctx.entries).string();
	// printf("%s\n",debug.c_str() );
	std::cout << text.getText();
}
