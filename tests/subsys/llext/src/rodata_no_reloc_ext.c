/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test extension for CONFIG_LLEXT_RODATA_NO_RELOC feature.
 */

#include <stdint.h>
#include <zephyr/llext/llext.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

/* Exported by the main test to provide ELF buffer location */
extern const void *rodata_no_reloc_ext_ptr;
extern const size_t rodata_no_reloc_ext_size;

/* rodata with a relocation - forces .rodata to be relocated */
Z_GENERIC_SECTION(.rodata)
static const void *relocated_data = &printk;

/* rodata with no relocation - should stay in place */
LLEXT_RODATA_NO_RELOC
static const uint32_t noreloc_data = 0x12345678;

void test_entry(void)
{
	uintptr_t elf_start = (uintptr_t)rodata_no_reloc_ext_ptr;
	uintptr_t elf_end = elf_start + rodata_no_reloc_ext_size;
	uintptr_t noreloc_addr = (uintptr_t)&noreloc_data;
	uintptr_t relocated_addr = (uintptr_t)&relocated_data;

	printk("noreloc at %p, relocated at %p, ELF: %p - %p\n",
	       (void *)noreloc_addr, (void *)relocated_addr,
	       (void *)elf_start, (void *)elf_end);

	/* Verify data is correct */
	zassert_equal(noreloc_data, 0x12345678, "noreloc_data value mismatch");

	/* noreloc_data should stay in original ELF buffer */
	zassert_true(noreloc_addr >= elf_start && noreloc_addr < elf_end,
		     "noreloc_data should remain in ELF buffer");

	/* relocated_data should be outside ELF buffer */
	zassert_true(relocated_addr < elf_start || relocated_addr >= elf_end,
		     "relocated_data should be outside ELF buffer");
}
EXPORT_SYMBOL(test_entry);
