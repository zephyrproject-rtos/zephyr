/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 EPAM Systems
 */

#ifndef ZEPHYR_XEN_FLASK_H_
#define ZEPHYR_XEN_FLASK_H_

#include <xen/public/xsm/flask_op.h>

/**
 * @brief convert context string to numerical SID
 *
 * This function issues the FLASK_CONTEXT_TO_SID hypercall to obtain
 * Security ID that corresponds to given security context in loaded policy.
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
 * @retval 0      Permissive Mode
 * @retval 1      Enforcing Mode
 * @retval -errno Negative errno code on failure.
 */
int flask_getenforce(void);

#endif /* ZEPHYR_XEN_FLASK_H_ */
