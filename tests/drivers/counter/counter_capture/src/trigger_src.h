/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COUNTER_CAPTURE_TEST_TRIGGER_SRC_H_
#define COUNTER_CAPTURE_TEST_TRIGGER_SRC_H_

#include <stddef.h>
#include <zephyr/drivers/counter.h>

/*
 * Capture channels under test. Defined in capture_test.c and shared with the
 * trigger-source backend (one of src/gpio_src.c or src/counter_capture_emul_src.c
 * is compiled, selected by CMake).
 */
extern const struct counter_capture_dt_spec capture_dt_specs[];

/*
 * Trigger-source backend interface. A capture edge is produced by driving a
 * level on the capture input of channel @p idx (index into capture_dt_specs).
 */

/** One-time per-channel setup. Returns 0 on success or a negative errno. */
int trigger_src_setup(size_t idx);

/** Read the current driven level (0/1). Returns a negative errno on error. */
int trigger_src_get(size_t idx);

/** Drive @p level on the capture input, producing an edge. Returns 0 or errno. */
int trigger_src_set(size_t idx, int level);

#endif /* COUNTER_CAPTURE_TEST_TRIGGER_SRC_H_ */
