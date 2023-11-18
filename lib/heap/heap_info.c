/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include "heap.h"

/*
 * Print heap info for debugging / analysis purpose
 */
static void heap_print_info(struct z_heap *h, bool dump_chunks)
{
	int i, nb_buckets = bucket_idx(h, h->end_chunk) + 1;
	size_t free_bytes, allocated_bytes, total, overhead;

	printk("Heap at %p contains %d units in %d buckets\n\n",
	       chunk_buf(h), h->end_chunk, nb_buckets);

	printk("  bucket#    min units        total      largest      largest\n"
	       "             threshold       chunks      (units)      (bytes)\n"
	       "  -----------------------------------------------------------\n");
	for (i = 0; i < nb_buckets; i++) {
		chunkid_t first = h->buckets[i].next;
		chunksz_t largest = 0;
		int count = 0;

		if (first) {
			chunkid_t curr = first;

			do {
				count++;
				largest = MAX(largest, chunk_size(h, curr));
				curr = next_free_chunk(h, curr);
			} while (curr != first);
		}
		if (count) {
			printk("%9d %12d %12d %12d %12zd\n",
			       i, (1 << i) - 1 + min_chunk_size(h), count,
			       largest, chunksz_to_bytes(h, largest));
		}
	}

	if (dump_chunks) {
		printk("\nChunk dump:\n");
		for (chunkid_t c = 0; ; c = right_chunk(h, c)) {
			printk("chunk %4d: [%c] size=%-4d left=%-4d right=%d\n",
			       c,
			       chunk_used(h, c) ? '*'
			       : solo_free_header(h, c) ? '.'
			       : '-',
			       chunk_size(h, c),
			       left_chunk(h, c),
			       right_chunk(h, c));
			if (c == h->end_chunk) {
				break;
			}
		}
	}

	get_alloc_info(h, &allocated_bytes, &free_bytes);
	/* The end marker chunk has a header. It is part of the overhead. */
	total = h->end_chunk * CHUNK_UNIT + chunk_header_bytes(h);
	overhead = total - free_bytes - allocated_bytes;
	printk("\n%zd free bytes, %zd allocated bytes, overhead = %zd bytes (%zd.%zd%%)\n",
	       free_bytes, allocated_bytes, overhead,
	       (1000 * overhead + total/2) / total / 10,
	       (1000 * overhead + total/2) / total % 10);
}

void sys_heap_print_info(struct sys_heap *heap, bool dump_chunks)
{
	heap_print_info(heap->heap, dump_chunks);
}
