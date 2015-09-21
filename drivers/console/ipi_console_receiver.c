/* ipi_console.c - Console messages to/from another processor */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <misc/ring_buffer.h>
#include <misc/printk.h>
#include <stdio.h>
#include <ipi.h>
#include <console/ipi_console.h>

static void ipi_console_fiber(int arg1, int arg2)
{
	uint8_t size32;
	uint16_t type;
	int ret;
	struct device *d;
	struct ipi_console_receiver_config_info *config_info;
	struct ipi_console_receiver_runtime_data *driver_data;
	int pos;

	d = (struct device *)arg1;
	driver_data = d->driver_data;
	config_info = d->config->config_info;
	ARG_UNUSED(arg2);
	size32 = 0;
	pos = 0;

	while (1) {
		nano_fiber_sem_take_wait(&driver_data->sem);

		ret = sys_ring_buf_get(&driver_data->rb, &type,
				       (uint8_t *)&config_info->line_buf[pos],
				       NULL, &size32);
		if (ret) {
			/* Shouldn't ever happen... */
			printk("ipi console ring buffer error: %d\n", ret);
			size32 = 0;
			continue;
		}

		if (config_info->line_buf[pos] == '\n' ||
		    pos == config_info->lb_size - 2)
		{
			if (pos != config_info->lb_size - 2) {
				config_info->line_buf[pos] = '\0';
			} else {
				config_info->line_buf[pos + 1] = '\0';
			}
			if (config_info->flags & IPI_CONSOLE_PRINTK) {
				printk("%s: '%s'\n", d->config->name,
				       config_info->line_buf);
			}
			if (config_info->flags & IPI_CONSOLE_STDOUT) {
				printf("%s: '%s'\n", d->config->name,
				       config_info->line_buf);
			}
			pos = 0;
		} else {
			++pos;
		}
	}
}

static void ipi_console_receive_callback(void *context, uint32_t id,
					 volatile void *data)
{
	struct device *d;
	struct ipi_console_receiver_runtime_data *driver_data;

	ARG_UNUSED(data);
	d = context;
	driver_data = d->driver_data;
	if (!sys_ring_buf_put(&driver_data->rb, 0, id, NULL, 0)) {
		nano_isr_sem_give(&driver_data->sem);
	}
}


int ipi_console_receiver_init(struct device *d)
{
	struct ipi_console_receiver_config_info *config_info =
		d->config->config_info;
	struct ipi_console_receiver_runtime_data *driver_data = d->driver_data;
	struct device *ipi;

	ipi = device_get_binding(config_info->bind_to);

	if (!ipi) {
		printk("unable to bind IPI console receiver to '%s'\n",
		       __func__, config_info->bind_to);
		return DEV_INVALID_CONF;
	}

	if (ipi_max_id_val_get(ipi) < 0xFF) {
		printk("IPI driver %s doesn't support 8-bit id values",
		       config_info->bind_to);
		return DEV_INVALID_CONF;
	}

	nano_sem_init(&driver_data->sem);
	sys_ring_buf_init(&driver_data->rb, config_info->rb_size32,
			  config_info->ring_buf_data);

	ipi_register_callback(ipi, ipi_console_receive_callback, d);

	task_fiber_start(config_info->fiber_stack, IPI_CONSOLE_STACK_SIZE,
			 ipi_console_fiber, (int)d, 0,
			 IPI_CONSOLE_PRI, 0);
	ipi_set_enabled(ipi, 1);

	return DEV_OK;
}

