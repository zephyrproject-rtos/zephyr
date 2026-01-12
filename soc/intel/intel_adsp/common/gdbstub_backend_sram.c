/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This implements a backend for Zephyr's GDB stub. Since a serial connection is
 * not available, we need to implement a GDB communication system that uses an
 * alternative medium. This uses two circular buffers in shared SRAM within the
 * DEBUG region, similar to the closed-source FW implementation. The SOF kernel
 * driver will expose a DebugFS file which allows the GDB client to communicate
 * with Zephyr's stub through these buffers. Additionally, Cavstool.py can expose
 * these buffers via a pty for debugging without the kernel driver.
 */

#include <soc.h>
#include <adsp_debug_window.h>
#include <adsp_memory.h>
#include <mem_window.h>
#include <zephyr/cache.h>

#define RING_SIZE 512
#if CONFIG_SOC_INTEL_CAVS_V25
#define SOF_GDB_WINDOW_OFFSET 1024
#elif CONFIG_SOC_INTEL_ACE15_MTPM || CONFIG_SOC_INTEL_ACE20_LNL || CONFIG_SOC_INTEL_ACE30 ||       \
	CONFIG_SOC_INTEL_ACE40
/*
 * MTL has 2 usable slots in debug window, which is more than 1 slot on TGL, but
 * still slot 0 is always used for logging, slot 1 is assigned to shell
 */
#define SOF_GDB_WINDOW_OFFSET 1024
#endif

struct gdb_sram_ring {
	unsigned int head;
	/* Fill spaces are to ensure head/tail are in their own cache line */
	unsigned char fill1[60];
	unsigned int tail;
	unsigned char fill2[60];
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

static volatile struct gdb_sram_ring *rx;
static volatile struct gdb_sram_ring *tx;

#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
static int gdb_get_debug_slot(void)
{
	struct adsp_dw_desc slot_desc = { .type = ADSP_DW_SLOT_GDB_STUB, };
	size_t slot_size;
	char *slot_addr;

	if (rx && tx) {
		/* Slot already allocated */
		return 0;
	}

	/* Force use the partial page0 slot */
	slot_addr = (char *)adsp_dw_request_slot(&slot_desc, &slot_size);
	if (!slot_addr) {
		return -ENOMEM;
	}

	rx = sys_cache_uncached_ptr_get(slot_addr);
	tx = sys_cache_uncached_ptr_get(slot_addr + sizeof(struct gdb_sram_ring));

	return 0;
}
#else
#define RX_UNCACHED (uint8_t *) (HP_SRAM_WIN2_BASE + SOF_GDB_WINDOW_OFFSET)
#define TX_UNCACHED (uint8_t *) (RX_UNCACHED + sizeof(struct gdb_sram_ring))
#endif

int z_gdb_backend_init(void)
{
#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
	int ret = gdb_get_debug_slot();

	if (ret) {
		return ret;
	}
#else
	__ASSERT_NO_MSG(sizeof(ADSP_DW->descs) <= SOF_GDB_WINDOW_OFFSET);

	rx = sys_cache_uncached_ptr_get(RX_UNCACHED);
	tx = sys_cache_uncached_ptr_get(TX_UNCACHED);
#endif

	rx->head = rx->tail = 0;
	tx->head = tx->tail = 0;

	return 0;
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

static int gdbstub_backend_sram_init(void)
{
#ifdef CONFIG_INTEL_ADSP_DEBUG_SLOT_MANAGER
	return gdb_get_debug_slot();
#else
	ADSP_DW->descs[ADSP_DW_SLOT_NUM_GDB].type = ADSP_DW_SLOT_GDB_STUB;
#endif

	return 0;
}

SYS_INIT(gdbstub_backend_sram_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
