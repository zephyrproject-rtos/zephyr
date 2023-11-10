/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/acpi/acpi.h>

ZTEST(acpi, test_mcfg_table)
{
	struct acpi_mcfg *mcfg;

	mcfg = acpi_table_get("MCFG", 0);

	zassert_not_null(mcfg, "Failed to get MCFG table");
}

#if ACPI_DT_HAS_HID(DT_NODELABEL(pcie0))
ZTEST(acpi, test_dev_enum)
{
	struct acpi_dev *dev;
	ACPI_RESOURCE *res_lst;
	int ret;

	dev = acpi_device_get(ACPI_DT_HID(DT_NODELABEL(pcie0)), ACPI_DT_UID(DT_NODELABEL(pcie0)));

	zassert_not_null(dev, "Failed to get acpi device with given HID");

	ret = acpi_current_resource_get(dev->path, &res_lst);

	zassert_ok(ret, "Failed to get current resource setting");
}
#endif

#if ACPI_DT_HAS_HID(DT_NODELABEL(rtc))
ZTEST(acpi, test_resource_enum)
{
	struct acpi_dev *dev;
	struct acpi_irq_resource irq_res;
	struct acpi_mmio_resource mmio_res;
	int ret;

	dev = acpi_device_get(ACPI_DT_HID(DT_NODELABEL(rtc)), ACPI_DT_UID(DT_NODELABEL(rtc)));

	zassert_not_null(dev, "Failed to get acpi device with given HID");

	ret = acpi_device_mmio_get(dev, &mmio_res);

	zassert_ok(ret, "Failed to get MMIO resources");

	ret = acpi_device_irq_get(dev, &irq_res);

	zassert_ok(ret, "Failed to get IRQ resources");
}
#endif

ZTEST_SUITE(acpi, NULL, NULL, NULL, NULL, NULL);
