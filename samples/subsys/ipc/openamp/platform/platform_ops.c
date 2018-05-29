/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implement the HIL layer required by OpenAMP
 */

#include <zephyr.h>
#include <ipm.h>

#include <openamp/open_amp.h>
#include "platform.h"

static K_SEM_DEFINE(data_sem, 0, 1);
static struct device *ipm_handle;

static void platform_ipm_callback(void *context, u32_t id, volatile void *data)
{
	k_sem_give(&data_sem);
}

static int enable_interrupt(struct proc_intr *intr)
{
	return ipm_set_enabled(ipm_handle, 1);
}

static void notify(struct hil_proc *proc, struct proc_intr *intr_info)
{
	u32_t dummy_data = 0x12345678; /* Some data must be provided */

	ipm_send(ipm_handle, 0, 0, &dummy_data, sizeof(dummy_data));
}

static int boot_cpu(struct hil_proc *proc, unsigned int load_addr)
{
	return -1;
}

static void shutdown_cpu(struct hil_proc *proc)
{
}

static int poll(struct hil_proc *proc, int nonblock)
{
	int status = k_sem_take(&data_sem, nonblock ? K_NO_WAIT : K_FOREVER);

	if (status == 0) {
		hil_notified(proc, 0xffffffff);
	}

	return status;
}

static struct metal_io_region *alloc_shm(struct hil_proc *proc,
					 metal_phys_addr_t physical,
					 size_t size,
					 struct metal_device **device)
{
	int status = metal_device_open("generic", SHM_DEVICE_NAME, device);

	if (status != 0) {
		return NULL;
	}

	return metal_device_io_region(*device, 0);
}

static void release_shm(struct hil_proc *proc, struct metal_device *device,
			struct metal_io_region *io)
{
	metal_device_close(device);
}

static int initialize(struct hil_proc *proc)
{
	ipm_handle = device_get_binding("MAILBOX_0");
	if (!ipm_handle) {
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);
	return 0;
}

static void release(struct hil_proc *proc)
{
	ipm_set_enabled(ipm_handle, 0);
}

struct hil_platform_ops platform_ops = {
	.enable_interrupt = enable_interrupt,
	.notify = notify,
	.boot_cpu = boot_cpu,
	.shutdown_cpu = shutdown_cpu,
	.poll = poll,
	.alloc_shm = alloc_shm,
	.release_shm = release_shm,
	.initialize = initialize,
	.release = release
};
