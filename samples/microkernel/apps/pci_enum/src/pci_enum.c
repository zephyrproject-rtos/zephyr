/* pci_enum.c - PCI Enumeration print-out application */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <zephyr.h>

#include <stdint.h>
#include <pci/pci.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

void pci_enumerate(void)
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

#ifdef CONFIG_MICROKERNEL


static int done = 0;

void task_enum_pci(void)
{
	if (done) {
		task_yield();
	}

	pci_enumerate();

	done = 1;
}

#else /* CONFIG_NANOKERNEL */

void main(void)
{
	pci_enumerate();
}

#endif /* CONFIG_MICROKERNEL */
