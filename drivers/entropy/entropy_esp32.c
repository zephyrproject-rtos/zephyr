/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_trng

#include <string.h>
#include <soc/rtc.h>
#include <soc/wdev_reg.h>
#include <esp_system.h>
#include <soc.h>
#include <esp_cpu.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>
#if defined(SOC_RNG_CLOCK_IS_INDEPENDENT)
#include <zephyr/drivers/clock_control.h>
#endif

LOG_MODULE_REGISTER(entropy, CONFIG_ENTROPY_LOG_LEVEL);

static inline uint32_t entropy_esp32_get_u32(void)
{
	/* The PRNG which implements WDEV_RANDOM register gets 2 bits
	 * of extra entropy from a hardware randomness source every APB clock cycle
	 * (provided WiFi or BT are enabled). To make sure entropy is not drained
	 * faster than it is added, this function needs to wait for at least 16 APB
	 * clock cycles after reading previous word. This implementation may actually
	 * wait a bit longer due to extra time spent in arithmetic and branch statements.
	 */

	uint32_t cpu_to_apb_freq_ratio =
		esp_clk_cpu_freq() / esp_clk_apb_freq();

	static uint32_t last_ccount;
	uint32_t ccount;

	do {
		ccount = esp_cpu_get_cycle_count();
	} while (ccount - last_ccount < cpu_to_apb_freq_ratio * 16);
	last_ccount = ccount;

	return REG_READ(WDEV_RND_REG);
}

static int entropy_esp32_get_entropy(const struct device *dev, uint8_t *buf,
				     uint16_t len)
{
	assert(buf != NULL);
	uint8_t *buf_bytes = buf;

	while (len > 0) {
		uint32_t word = entropy_esp32_get_u32();
		uint32_t to_copy = MIN(sizeof(word), len);

		memcpy(buf_bytes, &word, to_copy);
		buf_bytes += to_copy;
		len -= to_copy;
	}

	return 0;
}

static int entropy_esp32_init(const struct device *dev)
{
	int ret = 0;

#if defined(SOC_RNG_CLOCK_IS_INDEPENDENT)
	const struct device *clock_dev =
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(trng0)));
	clock_control_subsys_t clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(trng0), offset);

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(clock_dev, clock_subsys);

	if (ret != 0) {
		LOG_ERR("Error enabling TRNG clock");
	}
#else
	/* clock initialization handled by clock manager */
#endif

	return ret;
}

static DEVICE_API(entropy, entropy_esp32_api_funcs) = {
	.get_entropy = entropy_esp32_get_entropy
};

DEVICE_DT_INST_DEFINE(0,
		    entropy_esp32_init, NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		    &entropy_esp32_api_funcs);
