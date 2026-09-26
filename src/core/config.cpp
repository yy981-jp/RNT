#include <core/config.h>
#include <util/ui.h>

#include <iostream>


void Config::init() {
	// editor
	{
		std::cout << "[If you want to use an editor that is not on the list, please add it to rnt.cfg.json.]\n";
		
		int i = 1;
		const auto& editors = baseConfig.at("editor");
		for (const auto& e: editors) {
			std::cout << i << ". " << e["name"].get<std::string>() << "\n";
			i++;
		}

		int choice;
		std::cout << "Which text editor should I use? ";
		std::cin >> choice;

		config["editor"] = choice - 1;

		std::cout << "\n";
	}

	// arg 
	{
		config["useCliArg"] = choice("Do you want to use CLI arguments?");
		
		std::cout << "\n";
	}

	save();

	std::cout << "Setup completed.\n"
				 "If you want to change the settings, please edit or delete the settings file.\n"
			  << "settings file: " << configPath << "\n";	

	exit(0);
}
