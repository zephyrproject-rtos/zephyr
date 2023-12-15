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

ZTEST(acpi, test_irq_routing_table)
{
	static ACPI_PCI_ROUTING_TABLE irq_prt_table[CONFIG_ACPI_MAX_PRT_ENTRY];
	int status;

	status = acpi_get_irq_routing_table(CONFIG_ACPI_PRT_BUS_NAME,
					    irq_prt_table, ARRAY_SIZE(irq_prt_table));
	zassert_ok(status, "Failed to get PRT");
}

ZTEST_SUITE(acpi, NULL, NULL, NULL, NULL, NULL);
