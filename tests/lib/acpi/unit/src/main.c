/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <inttypes.h>

#include "mock.h"

#define CONFIG_ACPI_DEV_MAX 1
#define _AcpiModuleName ""

#include <zephyr/acpi/acpi.h>
#include <lib/acpi/acpi.c>
#include "assert.h"

#include <zephyr/fff.h>
DEFINE_FFF_GLOBALS;

struct DMAR {
	ACPI_TABLE_DMAR header;

	/* Hardware Unit 0 */
	struct {
		ACPI_DMAR_HARDWARE_UNIT header;

		struct {
			ACPI_DMAR_DEVICE_SCOPE header;
			ACPI_DMAR_PCI_PATH path0;
		} ds0;

		struct {
			ACPI_DMAR_DEVICE_SCOPE header;
			ACPI_DMAR_PCI_PATH path0;
		} ds1;
	} unit0;

	/* Hardware Unit 1 */
	struct {
		ACPI_DMAR_HARDWARE_UNIT header;

		struct {
			ACPI_DMAR_DEVICE_SCOPE header;
			ACPI_DMAR_PCI_PATH path0;
		} ds0;

		struct {
			ACPI_DMAR_DEVICE_SCOPE header;
			ACPI_DMAR_PCI_PATH path0;
		} ds1;
	} unit1;
};

static struct DMAR dmar0;

static void dmar_initialize(struct DMAR *dmar)
{
	dmar->header.Header.Length = sizeof(struct DMAR);

	dmar->unit0.header.Header.Length = sizeof(dmar->unit0);
	dmar->unit0.ds0.header.Length = sizeof(dmar->unit0.ds0);
	dmar->unit0.ds1.header.Length = sizeof(dmar->unit0.ds1);

	dmar->unit1.header.Header.Length = sizeof(dmar->unit1);
	dmar->unit1.ds0.header.Length = sizeof(dmar->unit1.ds0);
	dmar->unit1.ds1.header.Length = sizeof(dmar->unit1.ds1);
}

ZTEST(lib_acpi, test_nop)
{
}

static void count_subtables(ACPI_DMAR_HEADER *subtable, void *arg)
{
	uint8_t *count = arg;

	(*count)++;
}

FAKE_VOID_FUNC(subtable_nop, ACPI_DMAR_HEADER *, void *);

ZTEST(lib_acpi, test_dmar_foreach_subtable)
{
	uint8_t count = 0;

	dmar_initialize(&dmar0);

	acpi_dmar_foreach_subtable((void *)&dmar0, count_subtables, &count);
	zassert_equal(count, 2);

	TC_PRINT("Counted %u hardware units\n", count);
}

ZTEST(lib_acpi, test_dmar_foreach_subtable_invalid_unit_size_zero)
{
	ACPI_DMAR_HARDWARE_UNIT *hu = &dmar0.unit1.header;

	dmar_initialize(&dmar0);

	/* Set invalid hardware unit size */
	hu->Header.Length = 0;

	expect_assert();

	/* Expect assert, use fake void function as a callback */
	acpi_dmar_foreach_subtable((void *)&dmar0, subtable_nop, NULL);

	zassert_unreachable("Missed assert catch");
}

ZTEST_SUITE(lib_acpi, NULL, NULL, NULL, NULL, NULL);
