/*
 * Copyright (c) 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/iterable_sections.h>

struct test_ram {
	int i;
};

struct test_ram_named {
	int i;
};

struct test_ram_numeric {
	int i;
};

#define CHECK_BIT 0x80

/* declare in random order to check that the linker is sorting by name */
STRUCT_SECTION_ITERABLE(test_ram, ram3) = {0x03};
STRUCT_SECTION_ITERABLE(test_ram, ram2) = {0x02};
STRUCT_SECTION_ITERABLE(test_ram, ram4) = {0x04};
STRUCT_SECTION_ITERABLE(test_ram, ram1) = {0x01};

#define RAM_EXPECT 0x01020304

/* iterable section items can also be static */
static const STRUCT_SECTION_ITERABLE_ALTERNATE(test_ram2, test_ram, ram5) = {RAM_EXPECT};

/* declare in random order to check that the linker is sorting by custom name */
const STRUCT_SECTION_ITERABLE_NAMED(test_ram_named, A, ram6) = {0x01};
const STRUCT_SECTION_ITERABLE_NAMED(test_ram_named, C, ram7) = {0x03};
const STRUCT_SECTION_ITERABLE_NAMED(test_ram_named, D, ram8) = {0x04};
const STRUCT_SECTION_ITERABLE_NAMED(test_ram_named, B, ram9) = {0x02};

/* declare in random order to check that the linker is sorting numerically */
const STRUCT_SECTION_ITERABLE(test_ram_numeric, ramn_1) = {0x01};
const STRUCT_SECTION_ITERABLE(test_ram_numeric, ramn_10) = {0x03};
const STRUCT_SECTION_ITERABLE(test_ram_numeric, ramn_11) = {0x04};
const STRUCT_SECTION_ITERABLE(test_ram_numeric, ramn_3) = {0x02};

/**
 *
 * @brief Test iterable in read write section.
 *
 */
ZTEST(iterable_sections, test_ram)
{
	int out = 0;

	STRUCT_SECTION_FOREACH(test_ram, t) {
		out = (out << 8) | t->i;
		t->i |= CHECK_BIT;
	}

	zassert_equal(out, RAM_EXPECT, "Check value incorrect (got: 0x%08x)", out);

	zassert_equal(ram1.i & CHECK_BIT, CHECK_BIT,
		      "ram1.i check bit incorrect (got: 0x%x)", ram1.i);
	zassert_equal(ram2.i & CHECK_BIT, CHECK_BIT,
		      "ram2.i check bit incorrect (got: 0x%x)", ram2.i);
	zassert_equal(ram3.i & CHECK_BIT, CHECK_BIT,
		      "ram3.i check bit incorrect (got: 0x%x)", ram3.i);
	zassert_equal(ram4.i & CHECK_BIT, CHECK_BIT,
		      "ram4.i check bit incorrect (got: 0x%x)", ram4.i);

	out = 0;
	STRUCT_SECTION_FOREACH_ALTERNATE(test_ram2, test_ram, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, RAM_EXPECT, "Check value incorrect (got: 0x%08x)", out);

	out = 0;
	STRUCT_SECTION_FOREACH(test_ram_named, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, RAM_EXPECT, "Check value incorrect (got: 0x%x)", out);

	out = 0;
	STRUCT_SECTION_FOREACH(test_ram_numeric, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, RAM_EXPECT, "Check value incorrect (got: 0x%x)", out);
}

struct test_rom {
	int i;
};

struct test_rom_named {
	int i;
};

struct test_rom_numeric {
	int i;
};

/* declare in random order to check that the linker is sorting by name */
const STRUCT_SECTION_ITERABLE(test_rom, rom1) = {0x10};
const STRUCT_SECTION_ITERABLE(test_rom, rom3) = {0x30};
const STRUCT_SECTION_ITERABLE(test_rom, rom4) = {0x40};
const STRUCT_SECTION_ITERABLE(test_rom, rom2) = {0x20};

#define ROM_EXPECT 0x10203040

/* iterable section items can also be static */
static const STRUCT_SECTION_ITERABLE_ALTERNATE(test_rom2, test_rom, rom5) = {ROM_EXPECT};

/* declare in random order to check that the linker is sorting by custom name */
const STRUCT_SECTION_ITERABLE_NAMED(test_rom_named, A, rom6) = {0x10};
const STRUCT_SECTION_ITERABLE_NAMED(test_rom_named, C, rom7) = {0x30};
const STRUCT_SECTION_ITERABLE_NAMED(test_rom_named, D, rom8) = {0x40};
const STRUCT_SECTION_ITERABLE_NAMED(test_rom_named, B, rom9) = {0x20};

/* declare in random order to check that the linker is sorting numerically */
const STRUCT_SECTION_ITERABLE(test_rom_numeric, romn_1) = {0x10};
const STRUCT_SECTION_ITERABLE(test_rom_numeric, romn_10) = {0x30};
const STRUCT_SECTION_ITERABLE(test_rom_numeric, romn_11) = {0x40};
const STRUCT_SECTION_ITERABLE(test_rom_numeric, romn_3) = {0x20};

/**
 *
 * @brief Test iterable in read only section.
 *
 */
ZTEST(iterable_sections, test_rom)
{
	int out = 0;

	STRUCT_SECTION_FOREACH(test_rom, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, ROM_EXPECT, "Check value incorrect (got: 0x%x)", out);

	out = 0;
	STRUCT_SECTION_FOREACH_ALTERNATE(test_rom2, test_rom, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, ROM_EXPECT, "Check value incorrect (got: 0x%x)", out);

	out = 0;
	STRUCT_SECTION_FOREACH(test_rom_named, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, ROM_EXPECT, "Check value incorrect (got: 0x%x)", out);

	out = 0;
	STRUCT_SECTION_FOREACH(test_rom_numeric, t) {
		out = (out << 8) | t->i;
	}

	zassert_equal(out, ROM_EXPECT, "Check value incorrect (got: 0x%x)", out);
}

ZTEST_SUITE(iterable_sections, NULL, NULL, NULL, NULL, NULL);
