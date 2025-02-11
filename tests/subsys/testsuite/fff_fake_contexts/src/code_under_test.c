/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h> /* NULL */
#include <zephyr/called_API.h>
#include <zephyr/code_under_test.h>


int code_under_test(void)
{
	int result = 0;

	for (int i = 0; i < 2; ++i) {
		const struct called_API_info *called_API = NULL;

		result = called_API_open(&called_API);
		if (result != 0) {
			break;
		}

		result = called_API_close(called_API);
		if (result != 0) {
			break;
		}

	}

	return result;
}
