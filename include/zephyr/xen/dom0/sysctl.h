/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_DOM0_SYSCTL_H__
#define __XEN_DOM0_SYSCTL_H__
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/sysctl.h>
#include <zephyr/xen/public/xen.h>

int xen_sysctl_physinfo(struct xen_sysctl_physinfo *info);
int xen_sysctl_getdomaininfo(struct xen_domctl_getdomaininfo *domaininfo,
			     uint16_t first, uint16_t num);

#endif /* __XEN_DOM0_SYSCTL_H__ */
