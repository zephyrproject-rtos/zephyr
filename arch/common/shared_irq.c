/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sw_isr_common.h"

#include <zephyr/sw_isr_table.h>
#include <zephyr/spinlock.h>

/* an interrupt line can be considered shared only if there's
 * at least 2 clients using it. As such, enforce the fact that
 * the maximum number of allowed clients should be at least 2.
 */
BUILD_ASSERT(CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS >= 2,
	     "maximum number of clients should be at least 2");

void z_shared_isr(const void *data)
{
	size_t i;
	const struct z_shared_isr_table_entry *entry;
	const struct _isr_table_entry *client;

	entry = data;

	for (i = 0; i < entry->client_num; i++) {
		client = &entry->clients[i];

		if (client->isr) {
			client->isr(client->arg);
		}
	}
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS

static struct k_spinlock lock;

void z_isr_install(unsigned int irq, void (*routine)(const void *),
		   const void *param)
{
	struct z_shared_isr_table_entry *shared_entry;
	struct _isr_table_entry *entry;
	struct _isr_table_entry *client;
	unsigned int table_idx;
	int i;
	k_spinlock_key_t key;

	table_idx = z_get_sw_isr_table_idx(irq);

	/* check for out of bounds table index */
	if (table_idx >= CONFIG_NUM_IRQS) {
		return;
	}

	shared_entry = &z_shared_sw_isr_table[table_idx];
	entry = &_sw_isr_table[table_idx];

	key = k_spin_lock(&lock);

	/* have we reached the client limit? */
	__ASSERT(shared_entry->client_num < CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS,
		 "reached maximum number of clients");

	if (entry->isr == z_irq_spurious) {
		/* this is the first time a ISR/arg pair is registered
		 * for INTID => no need to share it.
		 */
		entry->isr = routine;
		entry->arg = param;

		k_spin_unlock(&lock, key);

		return;
	} else if (entry->isr != z_shared_isr) {
		/* INTID is being used by another ISR/arg pair.
		 * Push back the ISR/arg pair registered in _sw_isr_table
		 * to the list of clients and hijack the pair stored in
		 * _sw_isr_table with our own z_shared_isr/shared_entry pair.
		 */
		shared_entry->clients[shared_entry->client_num].isr = entry->isr;
		shared_entry->clients[shared_entry->client_num].arg = entry->arg;

		shared_entry->client_num++;

		entry->isr = z_shared_isr;
		entry->arg = shared_entry;
	}

	/* don't register the same ISR/arg pair multiple times */
	for (i = 0; i < shared_entry->client_num; i++) {
		client = &shared_entry->clients[i];

		__ASSERT(client->isr != routine && client->arg != param,
			 "trying to register duplicate ISR/arg pair");
	}

	shared_entry->clients[shared_entry->client_num].isr = routine;
	shared_entry->clients[shared_entry->client_num].arg = param;
	shared_entry->client_num++;

	k_spin_unlock(&lock, key);
}

static void swap_client_data(struct _isr_table_entry *a,
			     struct _isr_table_entry *b)
{
	struct _isr_table_entry tmp;

	tmp.arg = a->arg;
	tmp.isr = a->isr;

	a->arg = b->arg;
	a->isr = b->isr;

	b->arg = tmp.arg;
	b->isr = tmp.isr;
}

static void shared_irq_remove_client(struct z_shared_isr_table_entry *shared_entry,
				     int client_idx, unsigned int table_idx)
{
	int i;

	shared_entry->clients[client_idx].isr = NULL;
	shared_entry->clients[client_idx].arg = NULL;

	/* push back the removed client to the end of the client list */
	for (i = client_idx; i <= (int)shared_entry->client_num - 2; i++) {
		swap_client_data(&shared_entry->clients[i],
				 &shared_entry->clients[i + 1]);
	}

	shared_entry->client_num--;

	/* "unshare" interrupt if there will be a single client left */
	if (shared_entry->client_num == 1) {
		_sw_isr_table[table_idx].isr = shared_entry->clients[0].isr;
		_sw_isr_table[table_idx].arg = shared_entry->clients[0].arg;

		shared_entry->clients[0].isr = NULL;
		shared_entry->clients[0].arg = NULL;

		shared_entry->client_num--;
	}
}

int __weak arch_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
				       void (*routine)(const void *parameter),
				       const void *parameter, uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

	return z_isr_uninstall(irq, routine, parameter);
}

int z_isr_uninstall(unsigned int irq,
		    void (*routine)(const void *),
		    const void *parameter)
{
	struct z_shared_isr_table_entry *shared_entry;
	struct _isr_table_entry *entry;
	struct _isr_table_entry *client;
	unsigned int table_idx;
	size_t i;
	k_spinlock_key_t key;

	table_idx = z_get_sw_isr_table_idx(irq);

	/* check for out of bounds table index */
	if (table_idx >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

	shared_entry = &z_shared_sw_isr_table[table_idx];
	entry = &_sw_isr_table[table_idx];

	key = k_spin_lock(&lock);

	/* note: it's important that we remove the ISR/arg pair even if
	 * the IRQ line is not being shared because z_isr_install() will
	 * not overwrite it unless the _sw_isr_table entry for the given
	 * IRQ line contains the default pair which is z_irq_spurious/NULL.
	 */
	if (!shared_entry->client_num) {
		if (entry->isr == routine && entry->arg == parameter) {
			entry->isr = z_irq_spurious;
			entry->arg = NULL;
		}

		goto out_unlock;
	}

	for (i = 0; i < shared_entry->client_num; i++) {
		client = &shared_entry->clients[i];

		if (client->isr == routine && client->arg == parameter) {
			/* note: this is the only match we're going to get */
			shared_irq_remove_client(shared_entry, i, table_idx);
			goto out_unlock;
		}
	}

out_unlock:
	k_spin_unlock(&lock, key);
	return 0;
}

#endif /* CONFIG_DYNAMIC_INTERRUPTS */
