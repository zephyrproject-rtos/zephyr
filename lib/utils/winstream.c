/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <zephyr/sys/winstream.h>

/* This code may be used (e.g. for trace/logging) in very early
 * environments where the standard library isn't available yet.
 * Implement a simple memcpy() as an option.
 */
#ifndef CONFIG_WINSTREAM_STDLIB_MEMCOPY
# define MEMCPY(dst, src, n) \
	do { for (int i = 0; i < (n); i++) { (dst)[i] = (src)[i]; } } while (0)
#else
# include <string.h>
# define MEMCPY memcpy
#endif

/* These are just compiler barriers now.  Zephyr doesn't currently
 * have a framework for hardware memory ordering, and all our targets
 * (arm64 excepted) either promise firm ordering (x86) or are in-order
 * cores (everything else).  But these are marked for future
 * enhancement.
 */
#define READ_BARRIER()  __asm__ volatile("" ::: "memory")
#define WRITE_BARRIER() __asm__ volatile("")

static uint32_t idx_mod(struct sys_winstream *ws, uint32_t idx)
{
	return idx >= ws->len ? idx - ws->len : idx;
}

/* Computes modular a - b, assuming a and b are in [0:len) */
static uint32_t idx_sub(struct sys_winstream *ws, uint32_t a, uint32_t b)
{
	return idx_mod(ws, a + (ws->len - b));
}

void sys_winstream_write(struct sys_winstream *ws,
			 const char *data, uint32_t len)
{
	uint32_t len0 = len, suffix;
	uint32_t start = ws->start, end = ws->end, seq = ws->seq;

	/* Overflow: if we're truncating then just reset the buffer.
	 * (Max bytes buffered is actually len-1 because start==end is
	 * reserved to mean "empty")
	 */
	if (len > ws->len - 1) {
		start = end;
		len = ws->len - 1;
	}

	/* Make room in the buffer by advancing start first (note same
	 * len-1 from above)
	 */
	len = MIN(len, ws->len);
	if (seq != 0) {
		uint32_t avail = (ws->len - 1) - idx_sub(ws, end, start);

		if (len > avail) {
			ws->start = idx_mod(ws, start + (len - avail));
			WRITE_BARRIER();
		}
	}

	/* Had to truncate? */
	if (len < len0) {
		ws->start = end;
		data += len0 - len;
	}

	suffix = MIN(len, ws->len - end);
	MEMCPY(&ws->data[end], data, suffix);
	if (len > suffix) {
		MEMCPY(&ws->data[0], data + suffix, len - suffix);
	}

	ws->end = idx_mod(ws, end + len);
	ws->seq += len0; /* seq represents dropped bytes too! */
	WRITE_BARRIER();
}

uint32_t sys_winstream_read(struct sys_winstream *ws,
			    uint32_t *seq, char *buf, uint32_t buflen)
{
	uint32_t seq0 = *seq, start, end, wseq, len, behind, copy, suffix;

	do {
		start = ws->start; end = ws->end; wseq = ws->seq;
		READ_BARRIER();

		/* No change in buffer state or empty initial stream are easy */
		if (*seq == wseq || start == end) {
			*seq = wseq;
			return 0;
		}

		/* Underflow: we're trying to read from a spot farther
		 * back than start.  We dropped some bytes, so cheat
		 * and just drop them all to catch up.
		 */
		behind = wseq - *seq;
		if (behind > idx_sub(ws, ws->end, ws->start)) {
			*seq = wseq;
			return 0;
		}

		/* Copy data */
		copy = idx_sub(ws, ws->end, behind);
		len = MIN(buflen, behind);
		suffix = MIN(len, ws->len - copy);
		MEMCPY(buf, &ws->data[copy], suffix);
		if (len > suffix) {
			MEMCPY(buf + suffix, &ws->data[0], len - suffix);
		}
		*seq = seq0 + len;

		/* Check vs. the state we initially read and repeat if
		 * needed.  This can't loop forever even if the other
		 * side is stuck spamming writes: we'll run out of
		 * buffer space and exit via the underflow condition.
		 */
		READ_BARRIER();
	} while (start != ws->start || wseq != ws->seq);

	return len;
}
