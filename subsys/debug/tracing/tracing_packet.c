/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/atomic.h>
#include <tracing_packet.h>
#include <debug/tracing_core.h>

#define PACKET_SIZE sizeof(struct tracing_packet)
#define NUM_OF_PACKETS (CONFIG_TRACING_BUFFER_SIZE / PACKET_SIZE)

static atomic_t dropped_num;

static struct k_mem_slab tracing_packet_pool;
static u8_t __noinit __aligned(sizeof(void *))
		tracing_packet_pool_buf[CONFIG_TRACING_BUFFER_SIZE];

/*
 * Return true if interrupts were locked in current context
 */
static bool is_irq_locked(void)
{
	unsigned int key = arch_irq_lock();
	bool ret = arch_irq_unlocked(key);

	arch_irq_unlock(key);
	return ret;
}

static struct tracing_packet *tracing_packet_try_realloc(void)
{
	int ret = 0;
	bool has_more;
	struct tracing_packet *packet = NULL;

	do {
		has_more = tracing_packet_try_free();

		/*
		 * must be K_NO_WAIT here because
		 * context switch will also request packet
		 */
		ret = k_mem_slab_alloc(&tracing_packet_pool,
					(void **)&packet, K_NO_WAIT);

	} while (ret != 0 && has_more);

	if (packet == NULL) {
		atomic_inc(&dropped_num);
	}

	return packet;
}

void tracing_packet_pool_init(void)
{
	k_mem_slab_init(&tracing_packet_pool,
		tracing_packet_pool_buf, PACKET_SIZE, NUM_OF_PACKETS);
}

struct tracing_packet *tracing_packet_alloc(void)
{
	int ret = 0;
	struct tracing_packet *packet = NULL;

	/*
	 * use K_NO_WAIT here to make sure
	 * tracing doesn't impact code execution
	 */
	ret = k_mem_slab_alloc(&tracing_packet_pool,
				(void **)&packet, K_NO_WAIT);

	if (ret != 0 && !k_is_in_isr() && !is_irq_locked()) {
		packet = tracing_packet_try_realloc();
	}

	return packet;
}

void tracing_packet_free(struct tracing_packet *packet)
{
	if (packet != NULL) {
		k_mem_slab_free(&tracing_packet_pool, (void **)&packet);
	}
}
