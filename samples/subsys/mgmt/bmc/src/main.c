/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>

LOG_MODULE_REGISTER(bmc_sample, LOG_LEVEL_INF);

int main(void)
{
	int ret;

	LOG_INF("Starting BMC sample");

	ret = bmc_init();
	if (ret != 0) {
		LOG_ERR("BMC init failed (%d)", ret);
		return ret;
	}

	LOG_INF("BMC sample is running");

	for (;;) {
		k_sleep(K_FOREVER);
	}
}
