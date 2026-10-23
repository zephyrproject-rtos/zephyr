/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/ztest.h>

#define BSS_MARKER 0x12345678U

static volatile uint32_t bss_marker;

void __memmap_init(void);

/*
 * QEMU's ELF loader supplies zeroed BSS before startup. Seed a marker through
 * the hook immediately before crt1.S clears BSS to detect an additional clear.
 */
void __memmap_init(void)
{
	bss_marker = BSS_MARKER;
}

ZTEST(bss_clear, test_startup_bss_clear)
{
	uint32_t expected = IS_ENABLED(CONFIG_SKIP_BSS_CLEAR) ? BSS_MARKER : 0U;

	zassert_equal(bss_marker, expected, "Unexpected startup BSS contents");
}

ZTEST_SUITE(bss_clear, NULL, NULL, NULL, NULL, NULL);
