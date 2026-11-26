/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_HVX_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_HVX_H_

#include <stdbool.h>
#include <zephyr/kernel.h>

#define HEXAGON_HVX_VLEN 128U

/*
 * Cause 0x16 handler. Returns true if the faulting packet can run again,
 * false if HVX can't be used here and the exception is fatal.
 */
bool z_hexagon_hvx_trap(void);

/* Free @p thread's context and saved state, interrupts must be locked */
void z_hexagon_hvx_thread_release(struct k_thread *thread);

/* Called from z_hexagon_arch_switch() after _current is the new thread */
void z_hexagon_hvx_switch(void);

/* In hvx_context.S */
void z_hexagon_hvx_save(struct _hvx_context *buf);
void z_hexagon_hvx_restore(const struct _hvx_context *buf);
void z_hexagon_hvx_zero(void);

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_HVX_H_ */
