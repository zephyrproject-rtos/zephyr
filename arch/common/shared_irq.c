/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/spinlock.h>

struct shared_irq_client {
	void (*routine)(const void *arg);
	const void *arg;
};

struct shared_irq_data {
	bool enabled;
	struct shared_irq_client clients[CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS];
	uint32_t client_num;
	struct k_spinlock lock;
};

static struct shared_irq_data shared_irq_table[CONFIG_NUM_IRQS];

void shared_irq_isr(const void *data)
{
	int i;
	const struct shared_irq_data *irq_data = data;

	for (i = 0; i < irq_data->client_num; i++)
		/* note: due to how the IRQ disconnect function works
		 * this should be impossible but it's better to safe than
		 * sorry I guess.
		 */
		if (irq_data->clients[i].routine)
			irq_data->clients[i].routine(irq_data->clients[i].arg);
}

void z_isr_install(unsigned int irq, void (*routine)(const void *),
		const void *param)
{
	struct shared_irq_data *irq_data;
	unsigned int table_idx;
	int i;
	k_spinlock_key_t key;

	table_idx = irq - CONFIG_GEN_IRQ_START_VECTOR;

	/* check for out of bounds table index */
	if (table_idx >= CONFIG_NUM_IRQS)
		return;

	irq_data = &shared_irq_table[table_idx];

	/* seems like we've reached the limit of clients */
	/* TODO: is there a better way of letting the user know this is the case? */
	if (irq_data->client_num == CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS)
		return;

	key = k_spin_lock(&irq_data->lock);

	if (!irq_data->enabled) {
		_sw_isr_table[table_idx].arg = irq_data;
		_sw_isr_table[table_idx].isr = shared_irq_isr;

		irq_data->enabled = true;
	}

	/* don't register the same routine/arg pair again */
	for (i = 0; i < irq_data->client_num; i++) {
		if (irq_data->clients[i].routine == routine
		    && irq_data->clients[i].arg == param) {
			k_spin_unlock(&irq_data->lock, key);
			return;
		}
	}


	irq_data->clients[irq_data->client_num].routine = routine;
	irq_data->clients[irq_data->client_num].arg = param;
	irq_data->client_num++;

	k_spin_unlock(&irq_data->lock, key);
}

static void swap_client_data(struct shared_irq_client *a,
			     struct shared_irq_client *b)
{
	struct shared_irq_client tmp;

	tmp.arg = a->arg;
	tmp.routine = a->routine;

	a->arg = b->arg;
	a->routine = b->routine;

	b->arg = tmp.arg;
	b->routine = tmp.routine;
}

static inline void shared_irq_remove_client(struct shared_irq_data *irq_data,
					    int client_idx)
{
	int i;

	irq_data->clients[client_idx].routine = NULL;
	irq_data->clients[client_idx].arg = NULL;

	/* push back the NULL client data to the end of the array to avoid
	 * having holes in the shared IRQ table/
	 */
	for (i = client_idx; i <= (int)irq_data->client_num - 2; i++)
		swap_client_data(&irq_data->clients[i],
				 &irq_data->clients[i + 1]);

}

int arch_irq_disconnect_dynamic(unsigned int irq,
				unsigned int priority,
				void (*routine)(const void *),
				const void *parameter,
				uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	struct shared_irq_data *irq_data;
	unsigned int table_idx;
	int i;
	k_spinlock_key_t key;

	table_idx = irq - CONFIG_GEN_IRQ_START_VECTOR;

	/* check for out of bounds table index */
	if (table_idx >= CONFIG_NUM_IRQS)
		return -EINVAL;

	irq_data = &shared_irq_table[table_idx];

	if (!irq_data->client_num)
		return 0;

	key = k_spin_lock(&irq_data->lock);

	for (i = 0; i < irq_data->client_num; i++) {
		if (irq_data->clients[i].routine == routine
		    && irq_data->clients[i].arg == parameter) {
			shared_irq_remove_client(irq_data, i);
			irq_data->client_num--;
			if (!irq_data->client_num)
				irq_data->enabled = false;
			/* note: you can't have duplicate routine/arg pairs so this
			 * is the only match we're going to get.
			 */
			goto out_unlock;
		}
	}

out_unlock:
	k_spin_unlock(&irq_data->lock, key);
	return 0;
}
