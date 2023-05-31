/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/version.h>
#include <zephyr/xen/public/xen.h>
#include <string.h>

int xen_version(void)
{
	return HYPERVISOR_xen_version(XENVER_version, NULL);
}

int xen_version_extraversion(char *extra, int len)
{
	if (!extra || len < XEN_EXTRAVERSION_LEN) {
		return -EINVAL;
	}
	memset(extra, 0, len);
	return HYPERVISOR_xen_version(XENVER_extraversion, extra);
}
