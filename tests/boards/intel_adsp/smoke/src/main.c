/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <ztest.h>
#include <stdlib.h>
#include "tests.h"

cavs_ipc_handler_t ipm_handler;

static bool clock_msg(const struct device *dev, void *arg,
		      uint32_t data, uint32_t ext_data)
{
	*(uint32_t *)arg = data;
	return true;
}

void test_clock_calibrate(void)
{
	static volatile uint32_t host_dt;
	uint32_t cyc0, cyc1, hz, diff;

	/* Prime the host script's timestamp */
	cyc0 = k_cycle_get_32();
	cavs_ipc_send_message(CAVS_HOST_DEV, IPCCMD_TIMESTAMP, 0);

	k_msleep(1000);
	host_dt = 0;
	cavs_ipc_set_message_handler(CAVS_HOST_DEV, clock_msg, (void *)&host_dt);

	/* Now do it again, but with a handler to catch the result */
	cyc1 = k_cycle_get_32();
	cavs_ipc_send_message(CAVS_HOST_DEV, IPCCMD_TIMESTAMP, 0);
	AWAIT(host_dt != 0);
	cavs_ipc_set_message_handler(CAVS_HOST_DEV, NULL, NULL);

	hz = 1000000ULL * (cyc1 - cyc0) / host_dt;
	printk("CLOCK: %lld Hz\n", (1000000ULL * (cyc1 - cyc0)) / host_dt);

	/* Make sure we're within 1% of spec */
	diff = abs(hz - CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	zassert_true((hz / MIN(1, diff)) > 100, "clock rate wrong");
}

void test_main(void)
{
	struct cavs_ipc_data *devdata = CAVS_HOST_DEV->data;

	ipm_handler = devdata->handle_message;

	ztest_test_suite(intel_adsp,
			 ztest_unit_test(test_smp_boot_delay),
			 ztest_unit_test(test_cpu_halt),
			 ztest_unit_test(test_post_boot_ipi),
			 ztest_unit_test(test_cpu_behavior),
			 ztest_unit_test(test_host_ipc),
			 ztest_unit_test(test_clock_calibrate),
			 ztest_unit_test(test_ipm_cavs_host)
			 );

	ztest_run_test_suite(intel_adsp);
}
