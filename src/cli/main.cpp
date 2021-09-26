#include "iostream"
#include "optional"
#include <exception>

#include "cli.h"
#include "compare.h"

static std::string HELP_MESSAGE = R"(
ohhey is a program to use facial recognition to authorize things in linux.

A subcommand must be supplied.

Available subcommands are (compare).
)";

void printHelp() {
	std::cerr << HELP_MESSAGE;
}

int main(int argc, char* argv[]) {

    std::vector<std::string> argVec(argv, argv+argc);
	argVec.erase(argVec.begin()); // Remove command name.

	if (argVec.empty() || argVec[0] == "-h" || argVec[0] == "--help") {
		printHelp();
		return 1;
	}

	Cli::BaseCommand cmd;
	if (argVec[0] == Cli::CMD_COMPARE) {
		cmd = Compare::run;
	} else {
		std::cerr << "Received unknown subcommand \"" << argVec[0] << "\"." << std::endl;
		printHelp();
		return 1;
	}

	try {
		cmd(argVec);
	} catch (TCLAP::ArgException &e) {
		std::cerr << "Received invalid arguments: " << e.what() << std::endl;
		return 1;
	} catch (std::exception &e) {
		std::cerr << "Failed to run command: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
