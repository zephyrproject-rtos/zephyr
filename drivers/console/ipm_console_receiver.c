/* ipm_console.c - Console messages to/from another processor */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <ring_buffer.h>
#include <misc/printk.h>
#include <stdio.h>
#include <ipm.h>
#include <console/ipm_console.h>
#include <misc/__assert.h>
#include <atomic.h>

static void ipm_console_thread(void *arg1, void *arg2, void *arg3)
{
	u8_t size32;
	u16_t type;
	int ret;
	k_spinlock_key_t key;
	struct device *d;
	const struct ipm_console_receiver_config_info *config_info;
	struct ipm_console_receiver_runtime_data *driver_data;
	int pos, end;
	atomic_t *target;

	d = (struct device *)arg1;
	driver_data = d->driver_data;
	config_info = d->config->config_info;
	ARG_UNUSED(arg2);
	size32 = 0U;
	pos = 0;
	target = config_info->print_num;

	while (1) {
		k_sem_take(&driver_data->sem, K_FOREVER);

		key = k_spin_lock(&driver_data->rb_spinlock);
		ret = ring_buf_item_get(&driver_data->rb, &type,
					(u8_t *)&config_info->line_buf[pos],
					NULL, &size32);
		k_spin_unlock(&driver_data->rb_spinlock, key);
		if (ret) {
			/* Shouldn't ever happen... */
			printk("ipm console ring buffer error: %d\n", ret);
			size32 = 0U;
			continue;
		}

		end = 0;
		if (config_info->line_buf[pos] == '\n' ||
		    pos == config_info->lb_size - 2) {
			if (pos != config_info->lb_size - 2) {
				config_info->line_buf[pos] = '\0';
				end = 1;
			} else {
				config_info->line_buf[pos + 1] = '\0';
			}
			if (config_info->flags & IPM_CONSOLE_PRINTK) {
				printk("%s: '%s'\n", d->config->name,
				       config_info->line_buf);
			}
			if (config_info->flags & IPM_CONSOLE_STDOUT) {
				printf("%s: '%s'\n", d->config->name,
				       config_info->line_buf);
			}
			if (end == 1) {
				atomic_dec(target);
			}
			pos = 0;
		} else {
			++pos;
		}

	}
}

static void ipm_console_receive_callback(void *context, u32_t id,
					 volatile void *data)
{
	struct device *d;
	struct ipm_console_receiver_runtime_data *driver_data;
	int ret;
	k_spinlock_key_t key;

	ARG_UNUSED(data);
	d = context;
	driver_data = d->driver_data;

	key = k_spin_lock(&driver_data->rb_spinlock);
	/* Should always be at least one free buffer slot */
	ret = ring_buf_item_put(&driver_data->rb, 0, id, NULL, 0);
	k_spin_unlock(&driver_data->rb_spinlock, key);
	__ASSERT(ret == 0, "Failed to insert data into ring buffer");
	k_sem_give(&driver_data->sem);

	while (ring_buf_space_get(&driver_data->rb) == 0) {
	}
}


int ipm_console_receiver_init(struct device *d)
{
	const struct ipm_console_receiver_config_info *config_info =
		d->config->config_info;
	struct ipm_console_receiver_runtime_data *driver_data = d->driver_data;
	struct device *ipm;

	ipm = device_get_binding(config_info->bind_to);

	if (!ipm) {
		printk("unable to bind IPM console receiver to '%s'\n",
		       config_info->bind_to);
		return -EINVAL;
	}

	if (ipm_max_id_val_get(ipm) < 0xFF) {
		printk("IPM driver %s doesn't support 8-bit id values",
		       config_info->bind_to);
		return -EINVAL;
	}

	driver_data->ipm_device = ipm;
	driver_data->channel_disabled = 0;
	k_sem_init(&driver_data->sem, 0, UINT_MAX);
	ring_buf_init(&driver_data->rb, config_info->rb_size32,
		      config_info->ring_buf_data);

	ipm_register_callback(ipm, ipm_console_receive_callback, d);

	k_thread_create(&driver_data->rx_thread, config_info->thread_stack,
			CONFIG_IPM_CONSOLE_STACK_SIZE, ipm_console_thread, d,
			NULL, NULL, K_PRIO_COOP(IPM_CONSOLE_PRI), 0, 0);
	ipm_set_enabled(ipm, 1);

	return 0;
}
