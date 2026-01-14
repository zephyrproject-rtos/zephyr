/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdlib.h>
#include "tests.h"

#ifndef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/backends/intel_adsp_host_ipc.h>
#endif

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE

static volatile uint32_t old_host_dt;

static bool clock_msg(const struct device *dev, void *arg, uint32_t data, uint32_t ext_data)
{
	*(uint32_t *)arg = data;
	return true;
}

#else /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

extern struct ipc_ept host_ipc_ept;

struct intel_adsp_ipc_ept_priv_data test_priv_data;

void clock_ipc_receive_cb(const void *data, size_t len, void *priv)
{
	struct intel_adsp_ipc_ept_priv_data *tpd =
		(struct intel_adsp_ipc_ept_priv_data *)priv;
	const uint32_t *msg = (const uint32_t *)data;

	zassert_equal(len, sizeof(uint32_t) * 2, "unexpected IPC message length");
	zassert_not_null(data, "IPC payload pointer is NULL");

	/* Store returned timestamp from the extended payload word. */
	tpd->priv = (void *)(uintptr_t)msg[1];
}

struct ipc_ept_cfg clock_ipc_ept_cfg = {
	.name = "host_ipc_ept",
	.cb = {
		.received = clock_ipc_receive_cb,
	},
	.priv = (void *)&test_priv_data,
};

extern int intel_adsp_ipc_send_message(const struct device *dev, uint32_t data, uint32_t ext_data);

#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

ZTEST(intel_adsp, test_clock_calibrate)
{
	uint32_t cyc0, cyc1, hz, diff;
	volatile uint32_t *host_dt;

#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	host_dt = &old_host_dt;
#else
	int ret;

	host_dt = (volatile uint32_t *)&test_priv_data.priv;

	ret = ipc_service_register_endpoint(INTEL_ADSP_IPC_HOST_DEV, &host_ipc_ept,
					    &clock_ipc_ept_cfg);
	zassert_equal(ret, 0, "cannot register IPC endpoint");
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

	/* Prime the host script's timestamp */
	cyc0 = k_cycle_get_32();
	intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_TIMESTAMP, 0);

	k_msleep(1000);
	*host_dt = 0;
#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, clock_msg, (void *)host_dt);
#endif /* CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE */

	/* Now do it again, but with a handler to catch the result */
	cyc1 = k_cycle_get_32();
	intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_TIMESTAMP, 0);
	AWAIT(*host_dt != 0);
#ifdef CONFIG_INTEL_ADSP_IPC_OLD_INTERFACE
	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, NULL, NULL);
#else
	ret = ipc_service_deregister_endpoint(&host_ipc_ept);
	zassert_equal(ret, 0, "cannot de-register IPC endpoint");
#endif

	hz = 1000000ULL * (cyc1 - cyc0) / *host_dt;
	printk("CLOCK: %lld Hz\n", (1000000ULL * (cyc1 - cyc0)) / *host_dt);

	/* Make sure we're within 1% of spec */
	diff = abs((int32_t)(hz - CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC));
	zassert_true((hz / MIN(1, diff)) > 100, "clock rate wrong");
}
