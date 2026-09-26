#pragma once

#include <y9inc/string.h>

#include <string>
#include <iostream>


inline bool choice(std::string_view str) {
	std::cout << str << " [Y,N]: ";

	std::string in;
	std::cin >> in;

	bool flag = false;
	if (is_or(in, "Y", "y", "Yes", "yes")) flag = true;
	else if (is_or(in, "N", "n", "No", "no")) flag = false;
	else throw std::runtime_error("Please answer with Y(es) or N(o).");

	return flag;
}
