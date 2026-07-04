/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/p4wq.h>

#include <zephyr/mp/core/mp_workqueue.h>

K_P4WQ_DEFINE(_mp_p4wq_pool, CONFIG_MP_WORKQUEUE_THREADS_NUM, CONFIG_MP_WORKQUEUE_STACK_SIZE);

struct k_p4wq *const mp_p4wq = &_mp_p4wq_pool;
