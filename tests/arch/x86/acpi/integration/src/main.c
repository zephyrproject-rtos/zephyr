/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/acpi/acpi.h>

#define APCI_TEST_DEV ACPI_DT_HAS_HID(DT_ALIAS(acpi_dev))

#if APCI_TEST_DEV
#define DEV_HID ACPI_DT_HID(DT_ALIAS(acpi_dev))
#define DEV_UID ACPI_DT_UID(DT_ALIAS(acpi_dev))
#else
#define DEV_HID NULL
#define DEV_UID NULL
#endif

ZTEST(acpi, test_mcfg_table)
{
	struct acpi_mcfg *mcfg;

	mcfg = acpi_table_get("MCFG", 0);

	zassert_not_null(mcfg, "Failed to get MCFG table");
}

ZTEST(acpi, test_dev_enum)
{
	struct acpi_dev *dev;
	ACPI_RESOURCE *res_lst;
	int ret;

	Z_TEST_SKIP_IFNDEF(APCI_TEST_DEV);

	dev = acpi_device_get(DEV_HID, DEV_UID);

	zassert_not_null(dev, "Failed to get acpi device with given HID");

	ret = acpi_current_resource_get(dev->path, &res_lst);

	zassert_ok(ret, "Failed to get current resource setting");
}

ZTEST(acpi, test_resource_enum)
{
	struct acpi_dev *dev;
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	uint16_t irqs[CONFIG_ACPI_IRQ_VECTOR_MAX];
	struct acpi_reg_base reg_base[CONFIG_ACPI_MMIO_ENTRIES_MAX];
	int ret;

	Z_TEST_SKIP_IFNDEF(APCI_TEST_DEV);

	dev = acpi_device_get(DEV_HID, DEV_UID);

	zassert_not_null(dev, "Failed to get acpi device with given HID");

	mmio_res.mmio_max = ARRAY_SIZE(reg_base);
	mmio_res.reg_base = reg_base;
	ret = acpi_device_mmio_get(dev, &mmio_res);

	zassert_ok(ret, "Failed to get MMIO resources");

	irq_res.irq_vector_max = ARRAY_SIZE(irqs);
	irq_res.irqs = irqs;
	ret = acpi_device_irq_get(dev, &irq_res);

	zassert_ok(ret, "Failed to get IRQ resources");
}

ZTEST_SUITE(acpi, NULL, NULL, NULL, NULL, NULL);
