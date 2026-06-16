/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_RING_BUFFER_CLAIM
#include <zephyr/sys/internal/ring_buffer_claim.h>
#else
#include <zephyr/sys/internal/ring_buffer_slim.h>
#endif
