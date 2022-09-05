/*
 * Copyright (c) 2022 Huawei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file C++ exceptions demo.
 */

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class my_exception : public std::runtime_error
{
public:

	explicit my_exception(const std::string& what)
		: std::runtime_error(what)
	{
	}
};

} /* namespace */

int main(void)
{
	try {
		throw my_exception("foobar");
	} catch (const std::runtime_error& err) {
		std::cout << "Exception caught: " << err.what() << std::endl;
	}

	return 0;
}
