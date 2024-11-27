/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if CONFIG_SOC_NRF54H20_GLOBAL_HSFLL_MIN_FREQ_64_MHZ
#define GLOBAL_HSFLL_MIN_FREQ_HZ 64000000
#elif CONFIG_SOC_NRF54H20_GLOBAL_HSFLL_MIN_FREQ_128_MHZ
#define GLOBAL_HSFLL_MIN_FREQ_HZ 128000000
#elif CONFIG_SOC_NRF54H20_GLOBAL_HSFLL_MIN_FREQ_256_MHZ
#define GLOBAL_HSFLL_MIN_FREQ_HZ 256000000
#else
#define GLOBAL_HSFLL_MIN_FREQ_HZ 320000000
#endif

#define GLOBAL_HSFLL_REQUEST_TIMEOUT_US \
	(CONFIG_SOC_NRF54H20_GLOBAL_HSFLL_TIMEOUT_MS * USEC_PER_SEC)

static struct onoff_client cli;
static const struct device *global_hsfll = DEVICE_DT_GET(DT_NODELABEL(hsfll120));
static const struct nrf_clock_spec spec = {
	.frequency = GLOBAL_HSFLL_MIN_FREQ_HZ,
};

static int nordicsemi_nrf54h_global_hsfll_init(void)
{
	int ret;
	int res;
	bool completed;

	sys_notify_init_spinwait(&cli.notify);

	ret = nrf_clock_control_request(global_hsfll, &spec, &cli);
	if (ret) {
		return ret;
	}

	res = -EIO;
	completed = WAIT_FOR(sys_notify_fetch_result(&cli.notify, &res) == 0,
			     GLOBAL_HSFLL_REQUEST_TIMEOUT_US,
			     k_msleep(1));

	if (!completed) {
		LOG_ERR("%s request timed out", "Restrict global HSFLL");
		return -EIO;
	}

	if (res) {
		LOG_ERR("%s request failed: (res=%i)", "Restrict global HSFLL", res);
		return res;
	}

	LOG_INF("%s to %uHz", "Restrict global HSFLL", GLOBAL_HSFLL_MIN_FREQ_HZ);
	return 0;
}

SYS_INIT(nordicsemi_nrf54h_global_hsfll_init, POST_KERNEL, 99);
