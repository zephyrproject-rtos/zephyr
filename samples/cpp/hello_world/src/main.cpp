/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#ifdef __cplusplus
extern "C" {
	#include <libfoo.h>
}
#endif

int main(void)
{
	std::cout << "result: " << foo_sample(2) << std::endl;
	std::cout << "Hello, C++ world! " << CONFIG_BOARD << std::endl;
	return 0;
}
