#include <core/dir.h>
#include <core/texter.h>
#include <core/launcher.h>
#include <core/config.h>
#include <core/solver.h>


struct SDLApp {
	SDLApp() { SDL_Init(0); }
	~SDLApp() { SDL_Quit(); }
};


struct Ents {
	std::vector<Entry> orig, changed;
};

int main(int argc, char *argv[]) {
	SDLApp sdlapp;

	Config config((getDataPath() / "config.json").string());

	fs::path target;
	if (argc < 2) target = fs::current_path();
	else target = fs::path(argv[1]);


	Ents ent;
	{

		Ctx ctx{};
		dir(ctx, target);

		ent.orig = std::move( ctx.entries );

		Texter texter(ent.orig);

		// ユーザーに操作させる
		ent.changed = std::move( texter.edit(config,target) );
		const std::string& stat = texter.status();
		if (!stat.empty()) throw std::runtime_error("Error: Texter: " + stat);

	}

	Solver solver(ent.orig, ent.changed);
	solver.diff();
	solver.debug();
}
