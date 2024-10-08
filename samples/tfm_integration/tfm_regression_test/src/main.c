/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "non_secure_suites.h"

int main(void)
{
	ns_reg_test_start();

	return 0;
}
