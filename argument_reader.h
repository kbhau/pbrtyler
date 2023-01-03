#pragma once

#include <string>


bool get_argument_flag(std::string flag, int argc, char** argv)
{
	std::string curr = "";

	for (int i=0; i<argc; ++i) {
		curr = argv[i];
		if (curr == flag) {
			return true;
		}
	}

	return false;
}


std::string get_argument_value(std::string option, int argc, char** argv)
{
	std::string curr = "";

	for (int i=0; i<argc-1; ++i) {
		curr = argv[i];
		if (curr == option) {
			return argv[i+1];
		}
	}

	return "Argument error.";
}

