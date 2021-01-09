#include <iostream>
#include <string>
#include <stdexcept>

#include "utilities/flags/flags.hpp"
#include "simulation/simulation.hpp"

int main(int argc, char** argv) {

	int error = 0;
	FlagOptions flags;

	error = parse_flags(argc, argv, flags);

	if (error != 0) {
		print_usage();
		return 1;
	}

	try {
		Simulation simulation(flags);
		simulation.run();
	} catch (...) {
		try {
			std::exception_ptr eptr = std::current_exception(); // capture
			std::rethrow_exception(eptr);
		} catch(const std::exception &e) {
			std::cerr << "[Exception] " << e.what() << std::endl;
		}
		print_usage();
		return 1;
	}

	return error;
}
