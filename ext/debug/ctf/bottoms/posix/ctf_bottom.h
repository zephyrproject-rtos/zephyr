/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CTF_BOTTOM_POSIX_H
#define _CTF_BOTTOM_POSIX_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/types.h>
#include <ctf_map.h>


/* Obtain a field's size at compile-time.
 * Internal to this bottom-layer.
 */
#define CTF_BOTTOM_INTERNAL_FIELD_SIZE(x)      + sizeof(x)

/* Append a field to current event-packet.
 * Internal to this bottom-layer.
 */
#define CTF_BOTTOM_INTERNAL_FIELD_APPEND(x)			\
	{							\
		memcpy(epacket_cursor, &(x), sizeof(x));	\
		epacket_cursor += sizeof(x);			\
	}

/* Gather fields to a contiguous event-packet, then atomically emit.
 * Used by middle-layer.
 */
#define CTF_BOTTOM_FIELDS(...)							\
	{									\
		u8_t epacket[							\
			0 MAP(CTF_BOTTOM_INTERNAL_FIELD_SIZE, ##__VA_ARGS__)	\
		];								\
		u8_t *epacket_cursor = &epacket[0];				\
										\
		MAP(CTF_BOTTOM_INTERNAL_FIELD_APPEND, ##__VA_ARGS__)		\
		ctf_bottom_emit(epacket, sizeof(epacket));			\
	}

/* No need for locking when ctf_bottom_emit does POSIX fwrite(3) which is thread safe.
 * Used by middle-layer.
 */
#define CTF_BOTTOM_LOCK		{ /* empty */ }
#define CTF_BOTTOM_UNLOCK	{ /* empty */ }

/* On native_posix board, the code must sample time by itself.
 * Used by middle-layer.
 */
#define CTF_BOTTOM_TIMESTAMPED_INTERNALLY


typedef struct {
	const char *pathname;
	FILE *ostream;
} ctf_bottom_ctx_t;

extern ctf_bottom_ctx_t ctf_bottom;


/* Configure initializes ctf_bottom context and opens the IO channel */
void ctf_bottom_configure();

/* Start a new trace stream */
void ctf_bottom_start();

/* Emit IO in system-specifc way */
static inline void ctf_bottom_emit(const void *ptr, size_t size)
{
	/* Simplest possible example is atomic fwrite */
	fwrite(ptr, size, 1, ctf_bottom.ostream);
}

#endif /* _CTF_BOTTOM_POSIX_H */
