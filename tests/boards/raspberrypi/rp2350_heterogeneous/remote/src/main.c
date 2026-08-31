/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>

#include <rp2350_heterogeneous_test.h>

static volatile struct shared_status *const status = (void *)SHARED_STATUS_ADDR;
static const struct device *ipm;

static void mailbox_callback(const struct device *dev, void *user_data, uint32_t id,
			     volatile void *data)
{
	uint32_t response;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	ARG_UNUSED(id);

	if (data == NULL) {
		return;
	}

	response = *(volatile uint32_t *)data ^ MAILBOX_RESPONSE_XOR;
	if (ipm_send(ipm, 0, 0, &response, sizeof(response)) == 0) {
		status->mailbox_responses++;
	}
}

int main(void)
{
	ipm = DEVICE_DT_GET(DT_NODELABEL(ipm_mbox));
	if (!device_is_ready(ipm)) {
		return 0;
	}

	status->counter = 0U;
	status->mailbox_ready = 0U;
	status->mailbox_responses = 0U;
	status->hart_id = arch_proc_id();
	status->magic = SHARED_MAGIC;
	ipm_register_callback(ipm, mailbox_callback, NULL);
	if (ipm_set_enabled(ipm, 1) != 0) {
		return 0;
	}
	barrier_sync_synchronize();
	status->mailbox_ready = 1U;

	for (;;) {
		status->counter++;
		k_sleep(K_MSEC(25));
	}
}
