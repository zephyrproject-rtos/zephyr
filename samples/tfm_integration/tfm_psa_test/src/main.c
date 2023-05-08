/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* Run the PSA test suite */
void psa_test(void);

__attribute__((noreturn))
int main(void)
{
#ifdef CONFIG_TFM_PSA_TEST_NONE
	#error "No PSA test suite set. Use Kconfig to enable a test suite.\n"
#else
	psa_test();
#endif

	for (;;) {
	}
}
