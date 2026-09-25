#include <core/dir.h>
#include <core/texter.h>
#include <core/config.h>
#include <core/solver.h>
#include <core/fs.h>

#include <iostream>


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
	fs::current_path(target);


	RNT_Dir rd(target);


	for (int loop = 0; loop < 50; loop++) {

		try {
		
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

			std::vector<FsOperate> fsOp;
			{
				Solver solver(ent.orig, ent.changed, target);
				fsOp = std::move(solver.solve());
			}
			
			for (const auto& e: fsOp) {
				printf("%s   ->   %s\n", e.from.string().c_str(), e.to.string().c_str());
		
				fs::rename(e.from,e.to);
			}

			break;

		} catch (const std::runtime_error& e) {
			// error
			std::cerr << e.what() << "\n";
			std::cout << "Press Enter to continue";
			std::cin.get();
		}


	}

}
