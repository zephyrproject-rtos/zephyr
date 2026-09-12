/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/arch/fwargs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/boot_fdt.h>
#include <zephyr/xen/fdt.h>

static uint8_t xen_fdt[CONFIG_XEN_FDT_MAX_SIZE] __aligned(8);
static uint32_t xen_fdt_size;

int xen_fdt_get(const uint8_t **fdt, uint32_t *fdt_size)
{
	if ((fdt == NULL) || (fdt_size == NULL)) {
		return -EINVAL;
	}

	if (xen_fdt_size == 0U) {
		return -ENOENT;
	}

	*fdt = xen_fdt;
	*fdt_size = xen_fdt_size;

	return 0;
}

void fwargs_handler_hook(const uintptr_t *args, size_t argc)
{
	int ret;

	if (argc == 0U) {
		k_panic();
	}

	ret = sys_boot_fdt_copy(xen_fdt, sizeof(xen_fdt),
				(const uint8_t *)args[0], &xen_fdt_size);
	if (ret != 0) {
		k_panic();
	}
}
