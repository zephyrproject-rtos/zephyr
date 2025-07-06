/*
 * Copyright 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/symtab.h>
#include <zephyr/ztest.h>

const struct symtab_info *symtab;

static void *setup(void)
{
	symtab = symtab_get();

	return NULL;
}

ZTEST_SUITE(test_symtab, NULL, setup, NULL, NULL, NULL);

ZTEST(test_symtab, test_size)
{
	zassert_true(symtab->length > 0);
}

ZTEST(test_symtab, test_symtab_find_symbol_name)
{
	extern int main(void);
	const uintptr_t first_addr = symtab->first_addr;
	const uintptr_t last_addr = first_addr + symtab->entries[symtab->length - 1].offset;
	uint32_t offset;
	const char *symbol_name;

	zassert_between_inclusive((uintptr_t)main, first_addr, last_addr,
				  "No valid address found for `main()`");

	/* Find the name of functions with `symtab_find_symbol_name()` */
	offset = -1;
	symbol_name = symtab_find_symbol_name((uintptr_t)main, &offset);
	zassert_str_equal(symbol_name, "main");
	zassert_equal(offset, 0);

	/* Do a few more just for fun */
	symbol_name = symtab_find_symbol_name((uintptr_t)strcmp, NULL);
	zassert_str_equal(symbol_name, "strcmp");

	symbol_name = symtab_find_symbol_name((uintptr_t)symtab_find_symbol_name, NULL);
	zassert_str_equal(symbol_name, "symtab_find_symbol_name");

	symbol_name = symtab_find_symbol_name((uintptr_t)test_main, NULL);
	zassert_str_equal(symbol_name, "test_main");

	symbol_name = symtab_find_symbol_name((uintptr_t)setup, NULL);
	zassert_str_equal(symbol_name, "setup");
}

/**
 * This testsuite tests the following position in the symbol table
 *
 *
 *                  [SYMBOL ADDR]       |      Name                   Offset
 *     before first-->    .             |       "?"          (not found) 0x0
 *            first-->  0x100           |   <first>                      0x0
 *                      0x101           |   <first>                      0x1
 *                        .             |
 *                        .             |
 *                        .             |
 *             last-->  0x300           |    <last>                      0x0
 *       after last-->  0x301           |    <last>                      0x1
 *                        .             |
 *                      0x310 (dummy)   |       "?"          (not found) 0x0
 *                        .             |
 *      after dummy-->  0x342           |       "?"          (not found) 0x0
 */

ZTEST(test_symtab, test_before_first)
{
	const uintptr_t first_addr = symtab->first_addr;
	const char *symbol_name;
	uint32_t offset;

	/* No symbol should appear before first_addr, but make sure that first_addr != 0 */
	if (first_addr > 0) {
		offset = -1;
		symbol_name = symtab_find_symbol_name(first_addr - 1, &offset);
		zassert_str_equal(symbol_name, "?");
		zassert_equal(offset, 0);
	} else {
		ztest_test_skip();
	}
}

ZTEST(test_symtab, test_first)
{
	const uintptr_t first_addr = symtab->first_addr;
	const char *symbol_name;
	uint32_t offset;

	offset = -1;
	symbol_name = symtab_find_symbol_name(first_addr, &offset);
	zassert_str_equal(symbol_name, symtab->entries[0].name);
	zassert_equal(offset, 0);

	if ((symtab->entries[0].offset + 1) != symtab->entries[1].offset) {
		offset = -1;
		symbol_name = symtab_find_symbol_name(first_addr + 1, &offset);
		zassert_str_equal(symbol_name, symtab->entries[0].name);
		zassert_equal(offset, 1);
	}
}

ZTEST(test_symtab, test_last)
{
	const uintptr_t first_addr = symtab->first_addr;
	const int last_idx = symtab->length - 1;
	const uintptr_t last_addr = first_addr + symtab->entries[last_idx].offset;
	const char *symbol_name;
	uint32_t offset;

	offset = -1;
	symbol_name = symtab_find_symbol_name(last_addr, &offset);
	zassert_str_equal(symbol_name, symtab->entries[last_idx].name);
	zassert_equal(offset, 0);
}

ZTEST(test_symtab, test_after_last)
{
	const uintptr_t first_addr = symtab->first_addr;
	const uintptr_t last_offset = symtab->entries[symtab->length - 1].offset;
	const uintptr_t last_addr = first_addr + last_offset;
	const char *symbol_name;
	uint32_t offset;

	/* Test `offset` output with last symbol */
	if (last_offset + 0x1 != symtab->entries[symtab->length].offset) {
		offset = -1;
		symbol_name = symtab_find_symbol_name(last_addr + 0x1, &offset);
		zassert_str_equal(symbol_name,
				  symtab->entries[symtab->length - 1].name);
		zassert_equal(offset, 0x1);
	} else {
		ztest_test_skip();
	}
}

ZTEST(test_symtab, test_after_dummy)
{
	const uintptr_t first_addr = symtab->first_addr;
	const uintptr_t last_dummy_offset = symtab->entries[symtab->length].offset;
	const uintptr_t last_dummy_addr = first_addr + last_dummy_offset;
	const char *symbol_name;
	uint32_t offset;

	/* Test `offset` output with dummy symbol (after last dymbol) */
	offset = -1;
	symbol_name = symtab_find_symbol_name(last_dummy_addr + 0x42, &offset);
	zassert_str_equal(symbol_name, "?");
	zassert_equal(offset, 0);
}
