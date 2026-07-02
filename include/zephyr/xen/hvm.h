/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen HVM operations.
 * @ingroup xen_hvm
 */

#ifndef __XEN_HVM_H__
#define __XEN_HVM_H__

#include <zephyr/xen/public/hvm/hvm_op.h>
#include <zephyr/xen/public/hvm/params.h>

#include <zephyr/kernel.h>

/**
 * @defgroup xen_hvm Xen HVM operations
 * @ingroup xen_support
 * @brief Configure Xen HVM guest parameters through HVM hypercalls.
 * @{
 */

/**
 * @brief Set an HVM parameter for a Xen domain.
 *
 * @param idx HVM parameter selector, typically one of the ``HVM_PARAM_*`` constants.
 * @param domid Target Xen domain identifier.
 * @param value Parameter value to store.
 *
 * @return 0 on success, negative errno value on failure.
 */
int hvm_set_parameter(int idx, int domid, uint64_t value);

/**
 * @brief Get an HVM parameter from a Xen domain.
 *
 * @param idx HVM parameter selector, typically one of the ``HVM_PARAM_*`` constants.
 * @param domid Target Xen domain identifier.
 * @param[out] value Pointer to the buffer that receives the parameter value.
 *
 * @return 0 on success, negative errno value on failure.
 */
int hvm_get_parameter(int idx, int domid, uint64_t *value);

/** @} */

#endif /* __XEN_HVM_H__ */
