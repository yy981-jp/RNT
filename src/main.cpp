#include <core/dir.h>
#include <core/texter.h>
#include <core/config.h>
#include <core/solver.h>
#include <util/ui.h>

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


	std::vector<Entry> entOrig;
	std::vector<Entry> entChanged;

	{
		Ctx ctx{};
		dir(ctx, target);

		entOrig = std::move( ctx.entries );
	}
	if (entOrig.empty()) {
		std::cerr << "There are no work items.";
		return 0;
	}

	Texter texter(entOrig);

	
	for (int loop = 0; loop < 50; loop++) {

		try {
		
			// Ents ent;
			{

				// ユーザーに操作させる
				entChanged.clear();
				entChanged.swap( texter.edit(config,target) );
				const std::string& stat = texter.status();
				if (!stat.empty()) throw std::runtime_error("Error: Texter: " + stat);

			}

			std::vector<FsOperate> fsOp;
			{
				Solver solver(entOrig, entChanged, target);
				fsOp = std::move(solver.solve());
			}
			
			for (const auto& e: fsOp) {
				printf("%s   ->   %s\n", e.from.string().c_str(), e.to.string().c_str());
			}

			if ( !choice("Do you want to confirm this change?") ) {
				std::cout << "The operation was interrupted.\n";
				return 0;
			}

			for (const auto& e: fsOp) {
				fs::rename(e.from,e.to);
			}

			std::cout << "Done.\n";

			break;

		} catch (const std::runtime_error& e) {
			// error
			std::cerr << e.what() << "\n";
			std::cout << "Press Enter to continue.";
			std::cin.get();
		}


	}

}
