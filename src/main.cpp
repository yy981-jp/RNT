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

	Texter text(ctx.entries);
	// auto debug = resolvePath(ctx.entries[33].id, ctx.entries).string();
	// printf("%s\n",debug.c_str() );
	
	fs::path tempF = fs::temp_directory_path() / std::format("RNT-{}.txt", std::hash<fs::path>{}(target));
	std::ofstream ofs(tempF);
	{
		if (!ofs) throw std::runtime_error("RNT couldn't open temp file.");
		ofs << text.getText();
		ofs.flush();
	}

	json te = config.getSys()["editor"][config.get().at("editor").get<int>()];
	launchTextEditor(te["path"].get<std::string>(), te["arg"].get<std::string>(), tempF.string());
}
