/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void)
{
#if defined(CONFIG_TEST_M68K_FATAL_ILLEGAL_INSTRUCTION)
	__asm__ volatile (".word 0x4afc");
#else
	__asm__ volatile ("trap #0");
#endif

	CODE_UNREACHABLE;
}
