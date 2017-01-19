/* pci_enum.c - PCI Enumeration print-out application */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <pci/pci.h>

static void pci_enumerate(void)
{
	struct pci_dev_info info = {
		.function = PCI_FUNCTION_ANY,
		.bar = PCI_BAR_ANY,
	};

	pci_bus_scan_init();

	while (pci_bus_scan(&info)) {
		pci_show(&info);
		info.class_type = 0;
		info.vendor_id = 0;
		info.device_id = 0;
		info.function = PCI_FUNCTION_ANY;
		info.bar = PCI_BAR_ANY;
	}
}

void test_main(void)
{
	ztest_test_suite(pci_test, ztest_unit_test(pci_enumerate));
	ztest_run_test_suite(pci_test);
}
