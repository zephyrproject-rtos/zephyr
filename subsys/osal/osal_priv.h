/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_OSAL_OSAL_PRIV_H_
#define ZEPHYR_SUBSYS_OSAL_OSAL_PRIV_H_

#include <stddef.h>

/*
 * Control-block allocation policy. Handles (k_mutex, k_sem, k_msgq, ...) are
 * allocated through these so the policy lives in one place: dynamic (k_malloc)
 * when CONFIG_OSAL_DYNAMIC_ALLOC is set, otherwise from fixed pools.
 */
void *osal_cb_alloc(size_t size);
void osal_cb_free(void *ptr);

#endif /* ZEPHYR_SUBSYS_OSAL_OSAL_PRIV_H_ */
