/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/devicetree.h>

/**
 * @brief Define data structure for clock management callback
 *
 * This macro defines the data structures used for a clock management callback.
 * It will be called from the generic clock management code. However, only
 * drivers will actually reference this data structure, so unused definitions
 * will be discarded by the linker.
 */
#define CLOCK_CALLBACK_SLIST_DEFINE(clock_id)                                   \
	/* Callback sys_slist. Linker will discard this if unused. */           \
	sys_slist_t Z_GENERIC_SECTION(.clock_callback_##clock_id)               \
		clock_callback_##clock_id;

/* Define all clock callback linked lists */
DT_FOREACH_CLOCK_ID(CLOCK_CALLBACK_SLIST_DEFINE)
