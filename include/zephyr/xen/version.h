/*
 * Copyright (c) 2025 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen version queries.
 * @ingroup xen_version_queries
 */

#ifndef __XEN_VERSION_H__
#define __XEN_VERSION_H__

#include <zephyr/xen/generic.h>
#include <zephyr/xen/public/version.h>
#include <zephyr/xen/public/xen.h>

/**
 * @defgroup xen_version_queries Xen version queries
 * @ingroup xen_support
 * @brief Query the Xen hypervisor version exposed to the current domain.
 * @{
 */

/**
 * @brief Get the Xen version encoded as major and minor numbers.
 *
 * The returned integer uses the Xen ``major:minor`` encoding defined by
 * ``XENVER_version``.
 *
 * @return Encoded Xen version on success, negative errno value on failure.
 */
int xen_version(void);

/**
 * @brief Get the Xen extra version string.
 *
 * The destination buffer is cleared before the hypercall runs.
 *
 * @param[out] extra Destination buffer for the extra version string.
 * @param len Size of @p extra in bytes. It must be at least
 * ``XEN_EXTRAVERSION_LEN``.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p extra is ``NULL`` or @p len is too small.
 */
int xen_version_extraversion(char *extra, int len);

/** @} */

#endif /* __XEN_VERSION_H__ */
