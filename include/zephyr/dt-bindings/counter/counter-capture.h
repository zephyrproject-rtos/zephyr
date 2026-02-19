/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Counter capture flags for use in devicetree.
 * @ingroup counter_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_COUNTER_COUNTER_CAPTURE_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_COUNTER_COUNTER_CAPTURE_H_

/**
 * @addtogroup counter_interface
 * @{
 */

/**
 * @name Counter capture flags
 * The `COUNTER_CAPTURE_*` flags are used to configure capture edge selection
 * with capture_callback_set().
 *
 * The flags are on the lower 8bits of the counter_capture_flags_t.
 * @{
 */

/** Capture on rising edge. */
#define COUNTER_CAPTURE_RISING_EDGE (1U << 0)

/** Capture on falling edge. */
#define COUNTER_CAPTURE_FALLING_EDGE (1U << 1)

/** Capture on both rising and falling edges. */
#define COUNTER_CAPTURE_BOTH_EDGES (COUNTER_CAPTURE_RISING_EDGE | COUNTER_CAPTURE_FALLING_EDGE)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_COUNTER_COUNTER_CAPTURE_H_ */
