/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/dom0/sysctl.h>
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/xen.h>

static int do_sysctl(xen_sysctl_t *sysctl)
{
	sysctl->interface_version = XEN_SYSCTL_INTERFACE_VERSION;
	return HYPERVISOR_sysctl(sysctl);
}

int xen_sysctl_physinfo(struct xen_sysctl_physinfo *info)
{
	xen_sysctl_t sysctl = {
		.cmd = XEN_SYSCTL_physinfo,
	};
	int ret;

	ret = do_sysctl(&sysctl);
	if (ret < 0) {
		return ret;
	}
	*info = sysctl.u.physinfo;
	return ret;
}

int xen_sysctl_getdomaininfo(struct xen_domctl_getdomaininfo *domaininfo,
			     uint16_t first, uint16_t num)
{
	xen_sysctl_t sysctl = {
		.cmd = XEN_SYSCTL_getdomaininfolist,
		.u.getdomaininfolist.first_domain = first,
		.u.getdomaininfolist.max_domains  = num,
		.u.getdomaininfolist.buffer.p  = domaininfo,
	};
	int ret;

	ret = do_sysctl(&sysctl);
	if (ret < 0) {
		return ret;
	}
	return sysctl.u.getdomaininfolist.num_domains;
}
