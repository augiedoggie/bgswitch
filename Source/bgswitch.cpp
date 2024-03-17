#include "BackgroundManager.h"
#include "argparse.hpp"

#include <Application.h>
#include <InterfaceDefs.h>
#include <iostream>


void
_print_subhelp(argparse::ArgumentParser& program)
{
	if (program.is_subcommand_used("list"))
		std::cerr << program.at<argparse::ArgumentParser>("list") << std::endl;
	else if (program.is_subcommand_used("set"))
		std::cerr << program.at<argparse::ArgumentParser>("set") << std::endl;
	else if (program.is_subcommand_used("clear"))
		std::cerr << program.at<argparse::ArgumentParser>("clear") << std::endl;
	else
		std::cerr << program;
}

int
_check_workspace(int workspace)
{
	if (workspace < 1 || workspace > count_workspaces()) {
		std::cerr << "invalid workspace # specified" << std::endl;
		exit(1);
	}
	return workspace;
}


int
main(int argc, char** argv)
{
	argparse::ArgumentParser program(argv[0], "1.0", argparse::default_arguments::help);
	program.add_description("Get/Set workspace backgrounds.");
	program.add_argument("-a", "--all")
		.help("Get/Set/Clear all workspaces at once")
		.default_value(false)
		.implicit_value(true);
	program.add_argument("-w", "--workspace")
		.help("The workspace # to get/set/clear, otherwise use the current workspace")
		.scan<'i', int>()
		.default_value(-1);
	program.add_argument("-v", "--verbose")
		.help("Print extra output to screen")
		.default_value(false)
		.implicit_value(true);
	program.add_argument("-d", "--debug")
		.help("Print debugging output to screen")
		.default_value(false)
		.implicit_value(true);

	argparse::ArgumentParser list_command("list", "1.0", argparse::default_arguments::help);
	list_command.add_description("List background information");

	argparse::ArgumentParser set_command("set", "1.0", argparse::default_arguments::help);
	set_command.add_description("Set workspace background");
	set_command.add_argument("file")
		.help("Path to the image file")
		.required();

	argparse::ArgumentParser clear_command("clear", "1.0", argparse::default_arguments::help);
	clear_command.add_description("Clear workspace background");


	program.add_subparser(list_command);
	program.add_subparser(set_command);
	program.add_subparser(clear_command);

	try {
		program.parse_args(argc, argv);
	} catch (const std::runtime_error& err) {
		std::cerr << err.what() << std::endl;
		_print_subhelp(program);
		return 1;
	} catch (const std::invalid_argument& err) {
		std::cerr << "invalid argument: " << err.what() << std::endl;
		_print_subhelp(program);
		return 1;
	}

	BackgroundManager manager(NULL);

	if (manager.InitCheck() != B_OK)
		return 1;

	if (program["debug"] == true)
		manager.DumpBackgroundMessage();

	bool verbose = (program["verbose"] == true);

	// a BApplication is needed for count_workspaces(), doesn't need to be running
	BApplication App("application/x-vnd.cpr.bgswitch");

	int32 maxWorkspace = count_workspaces();
	int32 workspace = 1;

	if (program["all"] == false) {
		workspace = program.get<int>("workspace");
		if (workspace == -1)
			workspace = current_workspace() + 1;
		else
			_check_workspace(workspace);

		maxWorkspace = workspace;
	}

	if (program.is_subcommand_used("list")) {
		for (int32 x = workspace; x <= maxWorkspace; x++) {
			manager.DumpBackground(x, verbose);
			if (verbose) std::cout << std::endl;
		}
		return 0;
	} else if (program.is_subcommand_used("set")) {
		std::string imagePath = set_command.get<std::string>("file");
		status_t result = B_OK;
		for (int32 x = workspace; x <= maxWorkspace && result == B_OK; x++) {
			if (verbose)
				std::cout << "setting: " << workspace << " -> " << imagePath << std::endl;

			result = manager.SetBackground(imagePath.c_str(), workspace, verbose);
		}

		if (result == B_OK)
			manager.SendTrackerMessage();

		return 0;
	} else if (program.is_subcommand_used("clear")) {
		status_t result = B_OK;
		for (int32 x = workspace; x <= maxWorkspace && result == B_OK; x++) {
			if (verbose)
				std::cout << "clearing: " << workspace << std::endl;

			result = manager.ClearBackground(workspace, false, verbose);
		}

		if (result == B_OK)
			manager.SendTrackerMessage();

		return 0;
	} else if (program["debug"] != true)
		// only print the help if we don't have a command and aren't asked to dump the message
		std::cout << program << std::endl;

	return 1;
}
