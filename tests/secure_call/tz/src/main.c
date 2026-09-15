/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Non-Secure integration test: exercises the three __secure_call shapes by
 * crossing the TrustZone boundary into the Secure world via CMSE veneers.
 * Output lines are read by pytest/test_tz_calls.py to verify correctness.
 */

#include <zephyr/kernel.h>
#include <zephyr/secure_call.h>
#include "test_calls.h"

int main(void)
{
	printk("NS: starting secure call integration test\n");

	/* sc_add: two scalar args, non-void return */
	int sum = sc_add(3, 4);

	printk("SC_ADD: %d\n", sum);

	/* sc_nop: zero args, non-void return */
	int nop_ret = sc_nop();

	printk("SC_NOP: %d\n", nop_ret);

	/* sc_fill: void return, pointer + size args */
	uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	int fill_ok;

	sc_fill(buf, sizeof(buf));
	fill_ok = (buf[0] == 0 && buf[1] == 1 && buf[2] == 2 && buf[3] == 3);
	printk("SC_FILL: %s\n", fill_ok ? "OK" : "FAIL");

	printk("NS: test complete\n");
	return 0;
}
