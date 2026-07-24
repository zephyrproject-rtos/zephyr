/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal hardware-debugpoint model and core interface.
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_DEBUGPOINT_INTERNAL_H_
#define ZEPHYR_INCLUDE_DEBUG_DEBUGPOINT_INTERNAL_H_

#include <zephyr/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

enum z_debugpoint_type {
	Z_DEBUGPOINT_WATCH_READ,
	Z_DEBUGPOINT_WATCH_WRITE,
	Z_DEBUGPOINT_WATCH_RW,
	Z_DEBUGPOINT_BREAKPOINT,
	Z_DEBUGPOINT_TYPE_COUNT,
};

enum z_debugpoint_timing {
	Z_DEBUGPOINT_TIMING_UNKNOWN,
	Z_DEBUGPOINT_TIMING_BEFORE,
	Z_DEBUGPOINT_TIMING_AFTER,
};

/** Opaque identity for one installed debugpoint. */
typedef uint64_t z_debugpoint_handle_t;

#define Z_DEBUGPOINT_HANDLE_INVALID ((z_debugpoint_handle_t)0)

struct arch_esf;
struct z_debugpoint_config;

struct z_debugpoint_event {
	void *pc;
	void *access_addr;
	bool access_addr_valid;
	size_t access_size;
	enum z_debugpoint_type type;
	enum z_debugpoint_timing timing;
	bool rearm_required;
	const struct arch_esf *esf;
};

typedef void (*z_debugpoint_cb_t)(const struct z_debugpoint_config *config,
				  const struct z_debugpoint_event *event,
				  void *user_data);

/** Consumer-independent description of one logical hardware debugpoint. */
struct z_debugpoint_config {
	enum z_debugpoint_type type;
	void *addr;
	size_t size;
	z_debugpoint_cb_t callback;
	void *user_data;
};

int z_debugpoint_add(const struct z_debugpoint_config *config,
		     z_debugpoint_handle_t *handle);
int z_debugpoint_remove(z_debugpoint_handle_t handle);
bool z_debugpoint_in_callback(void);

/*
 * Report a hardware hit. If re-arming is required, the backend must already
 * have disabled this logical point on the current CPU and marked it inactive
 * in backend state.
 */
void z_debugpoint_hit(z_debugpoint_handle_t handle,
		      const struct z_debugpoint_event *event);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_DEBUGPOINT_INTERNAL_H_ */
