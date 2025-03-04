#include <Windows.h>
#include <iostream>

#include "./utils/perms.hpp"
#include "./modules/systeminfo.hpp"
#include "./modules/memorydevice.hpp"
#include "./modules/monitoredid.hpp"

const char end = '\n';

static void awaitInput(const char* message) {
	std::cout << message << end;
	std::cout << "press any key to exit" << end;
	std::cin.get();
}

static void processResult(std::string& result) {
	size_t pos = result.find(": ");

	if (pos != std::string::npos) {
		std::string status = result.substr(0, pos);
		std::string message = result.substr(pos + 2);

		if (status == "failed") {
			std::cout << "an error occurred: " << message << end;
		}
		else if (status == "success") {
			std::cout << "success: " << message << end;
		}
	}
	else {
		awaitInput("unexpected formatting error");
		std::exit(EXIT_FAILURE);
	}
}

static void processChoice(std::string choice) {
	int parsedChoice;
	std::string result;

	try {
		parsedChoice = stoi(choice);
	}
	catch (...) { parsedChoice = 0; }

	system("cls");

	switch (parsedChoice) {
		case 0:
			std::cout << "invalid input" << end;
			break;

		case 1:
			std::cout << "spoofing system info..." << end;

			result = systeminfo::init();
			processResult(result);
			break;

		case 2:
			std::cout << "spoofing memory device..." << end;

			result = memorydevice::init();
			processResult(result);
			break;

		case 3:
			std::cout << "spoofing monitor EDID..." << end;

			result = monitoredid::init();
			processResult(result);
			break;
	}
}

int main() {
	if (!perms::isAdmin()) {
		if (!perms::promptAdmin())
			awaitInput("request for admin failed");

		if (!perms::isAdmin())
			std::cout << "admin privileges are required to use this" << end;

		return EXIT_FAILURE;
	}

	std::cout << end;
	std::cout << "select method" << end;

	std::cout << "[1] spoof system info" << end;
	std::cout << "[2] spoof memory device" << end;
	std::cout << "[3] spoof monitor EDID" << end;
	std::cout << end;

	std::string choice;
	std::cout << "[>] choice: ";
	std::cin >> choice;

	processChoice(choice);

	std::cout << end;

	awaitInput("");
	return EXIT_SUCCESS;
}
