/*
 * Copyright (c) 2025 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_DOM0_VERSION_H__
#define __XEN_DOM0_VERSION_H__
#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/version.h>
#include <zephyr/xen/public/xen.h>

/**
 * Get Xen hypervisor version integer encoded information
 *
 * @return Xen version on success, negative errno on error
 */
int xen_version(void);

/**
 * Get Xen hypervisor extra version string
 *
 * @param extra - buffer to store the extra version string
 * @param len - maximum length of the buffer
 * @return 0 on success, negative errno on error
 */
int xen_version_extraversion(char *extra, int len);

#endif /* __XEN_DOM0_VERSION_H__ */
