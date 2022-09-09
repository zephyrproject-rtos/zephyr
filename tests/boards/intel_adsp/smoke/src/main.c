/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdlib.h>
#include "tests.h"

intel_adsp_ipc_handler_t ipm_handler;

static bool clock_msg(const struct device *dev, void *arg,
		      uint32_t data, uint32_t ext_data)
{
	*(uint32_t *)arg = data;
	return true;
}

ZTEST(intel_adsp, test_clock_calibrate)
{
	static volatile uint32_t host_dt;
	uint32_t cyc0, cyc1, hz, diff;

	/* Prime the host script's timestamp */
	cyc0 = k_cycle_get_32();
	intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_TIMESTAMP, 0);

	k_msleep(1000);
	host_dt = 0;
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, clock_msg, (void *)&host_dt);

	/* Now do it again, but with a handler to catch the result */
	cyc1 = k_cycle_get_32();
	intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_TIMESTAMP, 0);
	AWAIT(host_dt != 0);
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);

	hz = 1000000ULL * (cyc1 - cyc0) / host_dt;
	printk("CLOCK: %lld Hz\n", (1000000ULL * (cyc1 - cyc0)) / host_dt);

	/* Make sure we're within 1% of spec */
	diff = abs(hz - CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	zassert_true((hz / MIN(1, diff)) > 100, "clock rate wrong");
}

static void *intel_adsp_setup(void)
{
	struct intel_adsp_ipc_data *devdata = INTEL_ADSP_IPC_HOST_DEV->data;

	ipm_handler = devdata->handle_message;

	return NULL;
}

static void intel_adsp_teardown(void *data)
{
	/* Wait a bit so the python script on host is ready to receive
	 * IPC messages. An IPC message could be used instead of a timer,
	 * but expecting IPC to be working on a test suite that is going
	 * to test IPC may not be indicated.
	 */
	k_msleep(1000);
}

ZTEST_SUITE(intel_adsp, NULL, intel_adsp_setup, NULL, NULL,
		intel_adsp_teardown);

ZTEST_SUITE(intel_adsp_boot, NULL, intel_adsp_setup, NULL, NULL, NULL);
