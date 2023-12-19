/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <inttypes.h>

#include "mock.h"

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

ZTEST(lib_acpi, test_dmar_foreach_subtable_invalid_unit_size_big)
{
	ACPI_DMAR_HARDWARE_UNIT *hu = &dmar0.unit1.header;

	dmar_initialize(&dmar0);

	/* Set invalid hardware unit size */
	hu->Header.Length = sizeof(dmar0.unit1) + 1;

	expect_assert();

	/* Expect assert, use fake void function as a callback */
	acpi_dmar_foreach_subtable((void *)&dmar0, subtable_nop, NULL);

	zassert_unreachable("Missed assert catch");
}

static void count_devscopes(ACPI_DMAR_DEVICE_SCOPE *devscope, void *arg)
{
	uint8_t *count = arg;

	(*count)++;
}

FAKE_VOID_FUNC(devscope_nop, ACPI_DMAR_DEVICE_SCOPE *, void *);

ZTEST(lib_acpi, test_dmar_foreach_devscope)
{
	ACPI_DMAR_HARDWARE_UNIT *hu = &dmar0.unit0.header;
	uint8_t count = 0;

	dmar_initialize(&dmar0);

	acpi_dmar_foreach_devscope(hu, count_devscopes, &count);
	zassert_equal(count, 2);

	TC_PRINT("Counted %u device scopes\n", count);
}

ZTEST(lib_acpi, test_dmar_foreach_devscope_invalid_unit_size)
{
	ACPI_DMAR_HARDWARE_UNIT *hu = &dmar0.unit0.header;

	dmar_initialize(&dmar0);

	/* Set invalid hardware unit size */
	hu->Header.Length = 0;

	expect_assert();

	/* Expect assert, use fake void function as a callback */
	acpi_dmar_foreach_devscope(hu, devscope_nop, NULL);

	zassert_unreachable("Missed assert catch");
}

ZTEST(lib_acpi, test_dmar_foreach_devscope_invalid_devscope_size)
{
	ACPI_DMAR_HARDWARE_UNIT *hu = &dmar0.unit0.header;
	ACPI_DMAR_DEVICE_SCOPE *devscope = &dmar0.unit0.ds0.header;

	dmar_initialize(&dmar0);

	/* Set invalid device scope size */
	devscope->Length = 0;

	expect_assert();

	/* Expect assert, use fake void function as a callback */
	acpi_dmar_foreach_devscope(hu, devscope_nop, NULL);

	zassert_unreachable("Missed assert catch");
}

/**
 * Redefine AcpiGetTable to provide our static table
 */
DECLARE_FAKE_VALUE_FUNC(ACPI_STATUS, AcpiGetTable, char *, UINT32,
			struct acpi_table_header **);
static ACPI_STATUS dmar_custom_get_table(char *Signature, UINT32 Instance,
				  ACPI_TABLE_HEADER **OutTable)
{
	*OutTable = (ACPI_TABLE_HEADER *)&dmar0;

	return AE_OK;
}

ZTEST(lib_acpi, test_dmar_ioapic_get)
{
	union acpi_dmar_id fake_path = {
		.bits.bus = 0xab,
		.bits.device = 0xc,
		.bits.function = 0b101,
	};
	uint16_t ioapic;
	int ret;

	dmar_initialize(&dmar0);

	/* Set IOAPIC device scope */
	dmar0.unit1.ds1.header.EntryType = ACPI_DMAR_SCOPE_TYPE_IOAPIC;

	/* Set some arbitrary Bus and PCI path */
	dmar0.unit1.ds1.header.Bus = fake_path.bits.bus;
	dmar0.unit1.ds1.path0.Device = fake_path.bits.device;
	dmar0.unit1.ds1.path0.Function = fake_path.bits.function;

	/* Return our dmar0 table */
	AcpiGetTable_fake.custom_fake = dmar_custom_get_table;

	zassert_equal(AcpiGetTable_fake.call_count, 0);

	ret = acpi_dmar_ioapic_get(&ioapic);
	zassert_ok(ret, "Failed getting ioapic");

	/* Verify AcpiGetTable called */
	zassert_equal(AcpiGetTable_fake.call_count, 1);

	zassert_equal(ioapic, fake_path.raw, "Got wrong ioapic");

	TC_PRINT("Found ioapic id 0x%x\n", ioapic);
}

static void test_before(void *data)
{
	ASSERT_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(lib_acpi, NULL, NULL, test_before, NULL, NULL);
