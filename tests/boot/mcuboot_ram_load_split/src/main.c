/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>

static volatile uint32_t initialized_words[] = {
	0x11223344,
	0x55667788,
	0xa5a5a5a5,
	0x5a5a5a5a,
};
static volatile uint32_t zeroed_words[4];
static __noinit volatile uint32_t preserved_word;

__ramfunc static uint32_t ram_function(void)
{
	return initialized_words[2];
}

int main(void)
{
	static const uint32_t expected[] = {
		0x11223344,
		0x55667788,
		0xa5a5a5a5,
		0x5a5a5a5a,
	};

	for (size_t i = 0; i < ARRAY_SIZE(expected); ++i) {
		if (initialized_words[i] != expected[i] || zeroed_words[i] != 0U) {
			printk("FAIL: split RAM-load data\n");
			return 1;
		}
	}

	preserved_word = 0xc001c0de;
	if (preserved_word != 0xc001c0de || ram_function() != expected[2]) {
		printk("FAIL: split RAM-load special section\n");
		return 1;
	}

	printk("PASS: split RAM-load initialized data\n");
	return 0;
}
