/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_ISR_TABLES_LOCAL_DECLARATION));
BUILD_ASSERT(!IS_ENABLED(CONFIG_GEN_IRQ_VECTOR_TABLE));

/* Do not reference the manual table here: that would hide missing LTO roots. */
int main(void)
{
	return 0;
}
