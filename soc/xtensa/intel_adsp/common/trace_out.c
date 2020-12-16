/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <adsp/cache.h>
#include <soc/shim.h>

/* Simple output driver for the trace window of an ADSP device used
 * for communication with the host processor as a shared memory
 * region.  The protocol uses an array of 64-byte "slots", each of
 * which is prefixed by a 16 bit magic number followed by a sequential
 * ID number.  The remaining bytes are a (potentially nul-terminated)
 * string containing output data.
 *
 * IMPORTANT NOTE on cache coherence: the shared memory window is in
 * HP-SRAM.  Each DSP core has an L1 cache that is incoherent (!) from
 * the perspective of the other cores.  To handle this, we take care
 * to access all memory through the uncached window into HP-SRAM at
 * 0x9xxxxxxx and not the L1-cached mapping of the same memory at
 * 0xBxxxxxxx.
 */

#define SLOT_SIZE 64
#define SLOT_MAGIC 0x55aa

#define NSLOTS (SRAM_TRACE_SIZE / SLOT_SIZE)
#define MSGSZ (SLOT_SIZE - sizeof(struct slot_hdr))

/* Translates a SRAM pointer into an address of the same memory in the
 * uncached region from 0x80000000-0x9fffffff
 */
#define UNCACHED_PTR(p) ((void *)(((int)p) & ~0x20000000))

struct slot_hdr {
	uint16_t magic;
	uint16_t id;
};

struct slot {
	struct slot_hdr hdr;
	char msg[MSGSZ];
};

struct metadata {
	struct k_spinlock lock;
	bool initialized;
	uint32_t curr_slot;   /* To which slot are we writing? */
	uint32_t n_bytes;     /* How many bytes buffered in curr_slot */
};

/* Give it a cache line all its own! */
static __aligned(64) union {
	struct metadata meta;
	uint32_t cache_pad[16];
} data_rec;

#define data ((volatile struct metadata *)UNCACHED_PTR(&data_rec.meta))

static inline struct slot *slot(int i)
{
	struct slot *slots = UNCACHED_PTR(SRAM_TRACE_BASE);

	return &slots[i];
}

static int slot_incr(int s)
{
	return (s + 1) % NSLOTS;
}

void intel_adsp_trace_out(int8_t *str, size_t len)
{
	if (len == 0) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock((void *)&data->lock);

	if (!data->initialized) {
		slot(0)->hdr.magic = 0;
		slot(0)->hdr.id = 0;
		data->curr_slot = data->n_bytes = 0;
		data->initialized = 1;
	}

	/* We work with a local copy of the global data for
	 * performance reasons (The memory behind the "data" pointer
	 * is uncached and volatile!) and put it back at the end.
	 */
	uint32_t curr_slot = data->curr_slot;
	uint32_t n_bytes = data->n_bytes;

	for (size_t i = 0; i < len; i++) {
		int8_t c = str[i];
		struct slot *s = slot(curr_slot);

		s->msg[n_bytes++] = c;

		/* Are we done with this slot?  Terminate it and flag
		 * it for consumption on the other side
		 */
		if (c == '\n' || n_bytes >= MSGSZ) {
			if (n_bytes < MSGSZ) {
				s->msg[n_bytes] = 0;
			}

			/* Make sure the next slot has a magic number
			 * (so the reader can distinguish between
			 * no-new-data and system-reset), but does NOT
			 * have the correct successor ID (so can never
			 * be picked up as valid data).  We'll
			 * increment it later when we terminate that
			 * slot.
			 */
			int next_slot = slot_incr(curr_slot);
			uint16_t new_id = s->hdr.id + 1;

			slot(next_slot)->hdr.id = new_id;
			slot(next_slot)->hdr.magic = SLOT_MAGIC;
			slot(next_slot)->msg[0] = 0;

			s->hdr.id = new_id;
			s->hdr.magic = SLOT_MAGIC;

			curr_slot = next_slot;
			n_bytes = 0;
		}
	}

	data->curr_slot = curr_slot;
	data->n_bytes = n_bytes;
	k_spin_unlock((void *)&data->lock, key);
}

int arch_printk_char_out(int c)
{
	int8_t s = c;

	intel_adsp_trace_out(&s, 1);
	return 0;
}
