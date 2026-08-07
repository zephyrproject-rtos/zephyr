/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h> /* NULL */
#include <zephyr/called_api.h>
#include <zephyr/code_under_test.h>

int code_under_test(void)
{
	int result = 0;

	for (int i = 0; i < 2; ++i) {
		const struct called_api_info *called_api = NULL;

		result = called_api_open(&called_api);
		if (result != 0) {
			break;
		}

		result = called_api_close(called_api);
		if (result != 0) {
			break;
		}
	}

	return result;
}
