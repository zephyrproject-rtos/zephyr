/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_DEBUG_TRACING_BOTTOMS_GENERIC_CTF_BOTTOM_H
#define SUBSYS_DEBUG_TRACING_BOTTOMS_GENERIC_CTF_BOTTOM_H

#include <stdio.h>
#include <debug/tracing_format.h>

/*
 * Gather fields to a contiguous event-packet, then atomically emit.
 * Used by middle-layer.
 */
#define CTF_BOTTOM_FIELDS(...)						    \
{									    \
	TRACING_DATA(__VA_ARGS__);                                          \
}

#define CTF_BOTTOM_LOCK()         { /* empty */ }
#define CTF_BOTTOM_UNLOCK()       { /* empty */ }

#define CTF_BOTTOM_TIMESTAMPED_INTERNALLY

void ctf_bottom_configure(void);

void ctf_bottom_start(void);

#endif
