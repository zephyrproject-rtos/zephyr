/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Shared P4WQ pool for element-level work parallelism.
 *
 * Provides a shared P4WQ (Pooled Parallel Preemptible Priority-based Work Queue)
 * pool that elements can use to offload and parallelize work items
 * (e.g., tile-based AI inference, DCT transforms, etc.).
 *
 * Elements use the native Zephyr P4WQ API directly (k_p4wq_submit, k_p4wq_wait)
 * with the shared @ref mp_p4wq instance, without wrapping the API.
 *
 * Example usage in an element with parallel tile processing:
 *
 * @code
 * #include <zephyr/mp/core/mp_workqueue.h>
 *
 * static void tile_handler(struct k_p4wq_work *work) {
 *     // process one tile...
 * }
 *
 * static bool my_chainfn(struct mp_pad *pad, struct net_buf *in,
 *                        struct net_buf **out) {
 *     struct k_p4wq_work tile_work[4];
 *
 *     for (int i = 0; i < 4; i++) {
 *         tile_work[i].handler = tile_handler;
 *         tile_work[i].priority = 7;
 *         tile_work[i].deadline = 0;
 *         tile_work[i].sync = true;
 *         k_sem_init(&tile_work[i].done_sem, 0, 1);
 *         k_p4wq_submit(mp_p4wq, &tile_work[i]);
 *     }
 *     for (int i = 0; i < 4; i++) {
 *         k_p4wq_wait(&tile_work[i], K_FOREVER);
 *     }
 *     // assemble output...
 *     *out = result;
 *     return true;
 * }
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_WORKQUEUE_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_WORKQUEUE_H_

/**
 * @defgroup mp_workqueue Workqueues
 * @ingroup mp_core
 * @brief Shared P4WQ pool for element-level work parallelism
 * @{
 */

#include <zephyr/sys/p4wq.h>

/**
 * @brief Shared MP P4WQ pool instance
 *
 * A process-wide P4WQ pool shared by all elements that need to parallelize
 * internal work.
 */
extern struct k_p4wq *const mp_p4wq;

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_WORKQUEUE_H_ */
