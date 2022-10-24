#include <gulachek/gtree.hpp>
#include <gulachek/gtree/encoding/string.hpp>
#include <iostream>
#include <string>
#include <sstream>

namespace gt = gulachek::gtree;

int main()
{
	// first step is to ack
	std::string ack;
	if (auto err = gt::write(std::cout, ack))
	{
		std::cerr << "Error sending ack" << err << std::endl;
		return 1;
	}

	std::string msg;
	if (auto err = gt::read(std::cin, &msg))
	{
		std::cerr << "Error reading msg " << err << std::endl;
		return 1;
	}

	std::ostringstream ss;
	ss << "echo: " << msg;
	if (auto err = gt::write(std::cout, ss.str()))
	{
		std::cerr << "Error writing msg " << err << std::endl;
		return 1;
	}

	return 0;
}
