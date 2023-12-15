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

		ACPI_DMAR_DEVICE_SCOPE unit0_ds0;
		ACPI_DMAR_PCI_PATH unit0_ds0_path0;

		ACPI_DMAR_DEVICE_SCOPE unit0_ds1;
		ACPI_DMAR_PCI_PATH unit0_ds1_path0;
	} unit0;

	/* Hardware Unit 1 */
	struct {
		ACPI_DMAR_HARDWARE_UNIT header;

		ACPI_DMAR_DEVICE_SCOPE unit1_ds0;
		ACPI_DMAR_PCI_PATH unit1_ds0_path0;

		ACPI_DMAR_DEVICE_SCOPE unit1_ds1;
		ACPI_DMAR_PCI_PATH unit1_ds1_path0;
	} unit1;
};

static struct DMAR dmar0 = {
	.header.Header.Length = sizeof(struct DMAR),

	.unit0.header.Header.Length = sizeof(dmar0.unit0),
	.unit1.header.Header.Length = sizeof(dmar0.unit1),
};

static void dmar_initialize(struct DMAR *dmar)
{
	dmar->unit0.header.Header.Length = sizeof(dmar->unit0);
	dmar->unit1.header.Header.Length = sizeof(dmar->unit1);
}

static void dmar_clear(struct DMAR *dmar)
{
	dmar->unit0.header.Header.Length = 0;
	dmar->unit1.header.Header.Length = 0;
}

ZTEST(lib_acpi, test_nop)
{
}

static void count_subtables(ACPI_DMAR_HEADER *subtable, void *arg)
{
	uint8_t *count = arg;

	(*count)++;
}

ZTEST(lib_acpi, test_dmar_foreach)
{
	uint8_t count = 0;

	dmar_initialize(&dmar0);

	acpi_dmar_foreach_subtable((void *)&dmar0, count_subtables, &count);
	zassert_equal(count, 2);

	TC_PRINT("Counted %u hardware units\n", count);
}

ZTEST(lib_acpi, test_dmar_foreach_invalid_unit_size)
{
	uint8_t count = 0;

	dmar_clear(&dmar0);
	expect_assert();

	acpi_dmar_foreach_subtable((void *)&dmar0, count_subtables, &count);
}

ZTEST_SUITE(lib_acpi, NULL, NULL, NULL, NULL, NULL);
