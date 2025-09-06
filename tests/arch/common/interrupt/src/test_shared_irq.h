/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SHARED_IRQ_H__
#define __TEST_SHARED_IRQ_H__

#include <zephyr/ztest.h>
#include <zephyr/interrupt_util.h>

#define IRQ_PRIORITY 1
#define TEST_VECTOR_SIZE 10
#define TEST_INVALID_IDX 0xcafebabe
#define TEST_DUMMY_ISR_VAL 0xdeadbeef
#define TEST_INVALID_IRQ 0xcafebabe

#if defined(CONFIG_RISCV_HAS_CLIC)
#define IRQ_FLAGS 1 /* rising edge */
#else
#define IRQ_FLAGS 0
#endif

#define ISR_DEFINE(name)				\
static inline void name(const void *data)		\
{							\
	int idx = POINTER_TO_INT(data);			\
	test_vector[idx] = result_vector[idx];		\
}							\

static uint32_t test_vector[TEST_VECTOR_SIZE] = {
};

static uint32_t result_vector[TEST_VECTOR_SIZE] = {
	0xdeadbeef,
	0xcafebabe,
	0x1234cafe,
};

ISR_DEFINE(test_isr_0);
ISR_DEFINE(test_isr_1);
ISR_DEFINE(test_isr_2);

static inline bool client_exists_at_index(void (*routine)(const void *arg),
					  void *arg, int irq, size_t idx)
{
	size_t i;
	const struct z_shared_isr_table_entry *shared_entry;
	const struct _isr_table_entry *client;

	shared_entry = &z_shared_sw_isr_table[irq];

	if (idx == TEST_INVALID_IDX) {
		for (i = 0; i < shared_entry->client_num; i++) {
			client = &shared_entry->clients[i];

			if (client->isr == routine && client->arg == arg) {
				return true;
			}
		}
	} else {
		if (shared_entry->client_num <= idx) {
			return false;
		}

		client = &shared_entry->clients[idx];

		return client->isr == routine && client->arg == arg;
	}

	return false;
}

#endif /* __TEST_SHARED_IRQ_H__ */
