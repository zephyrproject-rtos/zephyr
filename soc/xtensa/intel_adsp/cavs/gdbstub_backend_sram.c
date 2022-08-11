/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0

This implements a backend for Zephyr's GDB stub. Since a serial connection is
not available, we need to implement a GDB communication system that uses an
alternative medium. This uses two circular buffers in shared SRAM within the
DEBUG region, similar to the closed-source FW implementation. The SOF kernel
driver will expose a DebugFS file which allows the GDB client to communicate
with Zephyr's stub through these buffers. Additionally, Cavstool.py can expose
these buffers via a pty for debugging without the kernel driver.
*/

#include <soc.h>
#include <adsp_memory.h>

#define RING_SIZE 128
struct gdb_sram_ring {
	unsigned char head;
	/* Fill spaces are to ensure head/tail are in their own cache line */
	unsigned char fill1[63];
	unsigned char tail;
	unsigned char fill2[63];
	unsigned char data[RING_SIZE];
};
static inline unsigned int ring_next_head(const volatile struct gdb_sram_ring *ring)
{
	return (ring->head + 1) & (RING_SIZE - 1);
}

static inline unsigned int ring_next_tail(const volatile struct gdb_sram_ring *ring)
{
	return (ring->tail + 1) & (RING_SIZE - 1);
}

static inline int ring_have_space(const volatile struct gdb_sram_ring *ring)
{
	return ring_next_head(ring) != ring->tail;
}

static inline int ring_have_data(const volatile struct gdb_sram_ring *ring)
{
	return ring->head != ring->tail;
}

#define RX_UNCACHED (void *) HP_SRAM_WIN2_BASE
#define TX_UNCACHED (void *) (HP_SRAM_WIN2_BASE + sizeof(struct gdb_sram_ring))
static volatile struct gdb_sram_ring *rx;
static volatile struct gdb_sram_ring *tx;

void z_gdb_backend_init(void)
{
	rx = z_soc_uncached_ptr(RX_UNCACHED);
	tx = z_soc_uncached_ptr(TX_UNCACHED);
	rx->head = rx->tail = 0;
	tx->head = tx->tail = 0;
}

void z_gdb_putchar(unsigned char c)
{
	while (!ring_have_space(tx)) {
	}

	tx->data[tx->head] = c;
	tx->head = ring_next_head(tx);
}

unsigned char z_gdb_getchar(void)
{
	unsigned char v;

	while (!ring_have_data(rx)) {
	}

	v = rx->data[rx->tail];
	rx->tail = ring_next_tail(rx);
	return v;
}
