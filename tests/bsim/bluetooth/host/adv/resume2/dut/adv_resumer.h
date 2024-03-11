/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

typedef int adv_starter_t(void);

/**
 * adv_starter reference must be Sync. The reference is dropped
 * when @ref adv_resumer_stop returns.
 */
int adv_resumer_start(adv_starter_t *adv_starter);

/**
 * Stops the resumer. Also calls @ref bt_le_adv_stop.
 *
 * This function is synchronized with @ref adv_resumer_start and
 * the resume mechanism. After this function returns, the @c
 * adv_starter provided to @ref adv_resumer_start will not
 * invoked, and it is safe to modify global variabled accessed
 * by @c adv_starter.
 */
int adv_resumer_stop(void);
