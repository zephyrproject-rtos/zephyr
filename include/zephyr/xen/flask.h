/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen XSM FLASK operations.
 * @ingroup xen_xsm_flask
 */


#ifndef ZEPHYR_XEN_FLASK_H_
#define ZEPHYR_XEN_FLASK_H_

#include <xen/public/xsm/flask_op.h>

/**
 * @defgroup xen_xsm_flask Xen XSM FLASK
 * @ingroup xen_support
 * @brief Load Xen security policy, manage FLASK enforcing mode, convert
 * context to SID, manage ocontexts and other FLASK-related tasks.
 * @{
 */

/**
 * @brief convert context string to numerical SID
 *
 * This function issues the FLASK_CONTEXT_TO_SID hypercall to obtain
 * Security ID that corresponds to given security context in loaded policy.
 *
 * @kconfig_dep{CONFIG_XEN_FLASK}
 *
 * @param buf	context string
 * @param size	length of context string
 * @param sid	output pointer to receive corresponding SID.
 *
 * @retval 0      On success
 * @retval -errno Negative errno code on failure.
 */
int flask_context_to_sid(char *buf, uint32_t size, uint32_t *sid);

/**
 * @brief get current FLASK enforcing mode
 *
 * This function issues the FLASK_GETENFORCE hypercall to obtain
 * current enforcing mode.
 *
 * @kconfig_dep{CONFIG_XEN_FLASK}
 *
 * @retval 0      Permissive Mode
 * @retval 1      Enforcing Mode
 * @retval -errno Negative errno code on failure.
 */
int flask_getenforce(void);

/** @} */

#endif /* ZEPHYR_XEN_FLASK_H_ */
