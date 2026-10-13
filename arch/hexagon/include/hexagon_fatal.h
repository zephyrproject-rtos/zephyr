/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_FATAL_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_FATAL_H_

#include <zephyr/toolchain.h>
#include <zephyr/types.h>

struct arch_esf;
struct event_context;

FUNC_NORETURN void z_hexagon_fatal_error(unsigned int reason);
FUNC_NORETURN void z_hexagon_fatal_error_ctx(unsigned int reason,
					      unsigned int event_type,
					      const struct event_context *ctx);

#ifdef CONFIG_DEBUG_COREDUMP
void z_hexagon_coredump_set_fault_sp(uintptr_t sp);
#endif

bool z_hexagon_event_frame_fp_get(const struct event_context *ctx,
					   uint32_t *fp);

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_FATAL_H_ */
