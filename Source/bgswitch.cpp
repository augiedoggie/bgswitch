#include "BackgroundManager.h"
#include "argparse.hpp"

#include <Application.h>
#include <InterfaceDefs.h>
#include <be_apps/Tracker/Background.h>
#include <iostream>


void
_print_subhelp(argparse::ArgumentParser& parser)
{
	if (parser.is_subcommand_used("list"))
		std::cerr << parser.at<argparse::ArgumentParser>("list") << std::endl;
	else if (parser.is_subcommand_used("set"))
		std::cerr << parser.at<argparse::ArgumentParser>("set") << std::endl;
	else if (parser.is_subcommand_used("clear"))
		std::cerr << parser.at<argparse::ArgumentParser>("clear") << std::endl;
	else if (parser.is_subcommand_used("reset"))
		std::cerr << parser.at<argparse::ArgumentParser>("reset") << std::endl;
	else
		std::cerr << parser;
}


int32
_check_workspace(int32 workspace)
{
	if (workspace < 0 || workspace > count_workspaces()) {
		std::cerr << "Error: invalid workspace # specified" << std::endl;
		exit(1);
	}
	return workspace;
}


int
main(int argc, char** argv)
{
	argparse::ArgumentParser programParser(argv[0], "1.0", argparse::default_arguments::help);
	programParser.add_description("Get/Set workspace backgrounds");
	programParser.add_epilog("Project page with examples: https://github.com/augiedoggie/bgswitch");
	programParser.add_argument("-a", "--all")
		.help("Modify all workspaces at once")
		.default_value(false)
		.implicit_value(true);
	programParser.add_argument("-w", "--workspace")
		.help("The workspace # to modify, otherwise use the current workspace")
		.scan<'i', int>();
	programParser.add_argument("-v", "--verbose")
		.help("Print extra output to screen")
		.default_value(false)
		.implicit_value(true);
	programParser.add_argument("-d", "--debug")
		.help("Print debugging output to screen")
		.default_value(false)
		.implicit_value(true);

	argparse::ArgumentParser listParser("list", "1.0", argparse::default_arguments::help);
	listParser.add_description("List background information");

	argparse::ArgumentParser setParser("set", "1.0", argparse::default_arguments::help);
	setParser.add_description("Set workspace background options");
	setParser.add_epilog("Specify one or more of the file/mode/text/offset options");
	setParser.add_argument("-f", "--file")
		.help("Path to the image file (ex. -f /path/to/file.jpg)")
		.default_value(std::string{});
	setParser.add_argument("-m", "--mode")
		.help("Placement mode 1=Manual/2=Center/3=Scale/4=Tile (ex. -m 3)")
		.scan<'i', int>();
	setParser.add_argument("-t", "--text")
		.help("Enable text outline")
		.default_value(false)
		.implicit_value(true);
	setParser.add_argument("-n", "--notext")
		.help("Disable text outline")
		.default_value(false)
		.implicit_value(true);
	setParser.add_argument("-o", "--offset")
		.help("X/Y offset in manual placement mode, separated by a space (ex. -o 200 400)")
		.scan<'i', int>()
		.nargs(2);

	argparse::ArgumentParser clearParser("clear", "1.0", argparse::default_arguments::help);
	clearParser.add_description("Make background empty (same effect as: set -f \"\")");

	argparse::ArgumentParser resetParser("reset", "1.0", argparse::default_arguments::help);
	resetParser.add_description("Reset background to global default");

	programParser.add_subparser(listParser);
	programParser.add_subparser(setParser);
	programParser.add_subparser(clearParser);
	programParser.add_subparser(resetParser);

	try {
		programParser.parse_args(argc, argv);
	} catch (const std::exception& err) {
		std::cerr << "Error: " << err.what() << std::endl;
		_print_subhelp(programParser);
		return 1;
	}

	BackgroundManager bgManager(nullptr);

	if (bgManager.InitCheck() != B_OK)
		return 1;

	if (programParser["debug"] == true)
		bgManager.PrintToStream();

	bool verbose = (programParser["verbose"] == true);

	// a BApplication is needed for count_workspaces(), doesn't need to be running
	BApplication App("application/x-vnd.cpr.bgswitch");

	int32 maxWorkspace = count_workspaces();
	int32 workspace = 1;

	if (programParser["all"] == false) {
		if (programParser.is_used("workspace")) {
			workspace = programParser.get<int>("workspace");
			_check_workspace(workspace);
		} else
			workspace = current_workspace() + 1;

		// if we're net setting all workspaces then set them equal so the for loops below only run one loop
		maxWorkspace = workspace;
	} else if (programParser.is_used("workspace")) {
		std::cerr << "Error: only one of -w/--workspace and -a/--all may be used" << std::endl;
		std::cerr << programParser << std::endl;
		return 1;
	}

	if (programParser.is_subcommand_used("list")) {
		// when listing all show the global defaults too
		if (verbose && programParser.is_used("all"))
			workspace = 0;

		for (int32 x = workspace; x <= maxWorkspace; x++) {
			bgManager.PrintBackgroundToStream(x, verbose);
			if (verbose)
				std::cout << std::endl;
		}
		return 0;
	} else if (programParser.is_subcommand_used("set")) {
		// option sanity checking
		if (setParser.is_used("text") && setParser.is_used("notext")) {
			std::cerr << "Error: only one of -t/--text and -n/--notext may be used" << std::endl;
			std::cout << setParser << std::endl;
			return 1;
		}

		// more option sanity checking
		if (!setParser.is_used("file") && !setParser.is_used("mode")
				&& !setParser.is_used("text") && !setParser.is_used("notext")
				&& !setParser.is_used("offset")) {
			std::cerr << "Error: must specify one of the file/mode/text/notext/offset options" << std::endl;
			std::cerr << setParser << std::endl;
			return 1;
		}

		for (int32 x = workspace; x <= maxWorkspace; x++) {
			if (setParser.is_used("file")) {
				std::string imagePath = setParser.get<std::string>("file");
				if (bgManager.SetBackground(imagePath.c_str(), x, verbose) != B_OK)
					return 1;
			}

			if (setParser.is_used("text") && bgManager.SetOutline(true, x, verbose) != B_OK)
				return 1;

			if (setParser.is_used("notext") && bgManager.SetOutline(false, x, verbose) != B_OK)
				return 1;

			if (setParser.is_used("offset")) {
				auto offset = setParser.get<std::vector<int>>("offset");
				if (bgManager.SetOffset(offset[0], offset[1], x, verbose) != B_OK)
					return 1;
			}

			if (setParser.is_used("mode")) {
				int32 mode = 0;
				switch (setParser.get<int>("mode")) {
					case 1: mode = B_BACKGROUND_MODE_USE_ORIGIN; break;
					case 2: mode = B_BACKGROUND_MODE_CENTERED; break;
					case 3: mode = B_BACKGROUND_MODE_SCALED; break;
					case 4: mode = B_BACKGROUND_MODE_TILED; break;
					default:
						std::cerr << "Error: invalid placement mode, must be one of 1/2/3/4" << std::endl;
						std::cerr << setParser << std::endl;
						return 1;
				}
				if (bgManager.SetPlacement(mode, x, verbose) != B_OK)
					return 1;
			}
		}

		bgManager.Flush();
		return 0;
	} else if (programParser.is_subcommand_used("clear") || programParser.is_subcommand_used("reset")) {
		if (workspace == 0 && programParser.is_subcommand_used("clear")) {
			std::cerr << "Error: unable to reset the global workspace" << std::endl;
			return 1;
		}

		for (int32 x = workspace; x <= maxWorkspace; x++) {
			if (programParser.is_subcommand_used("reset"))
				bgManager.ResetWorkspace(x, verbose);
			else
				bgManager.SetBackground(nullptr, x, verbose);
		}

		bgManager.Flush();
		return 0;
	} else if (programParser["debug"] != true)
		// only print the help if we don't have a command and aren't asked to dump the message
		std::cout << programParser << std::endl;

	return 1;
}
