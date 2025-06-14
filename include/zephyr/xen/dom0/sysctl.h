/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Xen System Control Interface
 */

#ifndef __XEN_DOM0_SYSCTL_H__
#define __XEN_DOM0_SYSCTL_H__

#include <xen/public/xen.h>
#include <xen/public/sysctl.h>

#include <zephyr/xen/generic.h>

/**
 * @brief Retrieves information about the host system.
 *
 * @param[out] info A pointer to a `struct xen_sysctl_physinfo` structure where the
 *             retrieved information will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int xen_sysctl_physinfo(struct xen_sysctl_physinfo *info);

/**
 * @brief Retrieves information about Xen domains.
 *
 * @param[out] domaininfo A pointer to the `xen_domctl_getdomaininfo` structure
 *                        to store the retrieved domain information.
 * @param first The first domain ID to retrieve information for.
 * @param num The maximum number of domains to retrieve information for.
 * @return 0 on success, or a negative error code on failure.
 */
int xen_sysctl_getdomaininfo(struct xen_domctl_getdomaininfo *domaininfo,
			     uint16_t first, uint16_t num);

#endif /* __XEN_DOM0_SYSCTL_H__ */
