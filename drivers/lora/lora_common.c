/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>

static void lora_ed_discard(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(size);
	ARG_UNUSED(rssi);
	ARG_UNUSED(snr);
	ARG_UNUSED(user_data);
}

int lora_energy_detect(const struct device *dev, int16_t rssi_threshold, k_timeout_t duration)
{
	k_timepoint_t expiry;
	bool busy = false;
	int ret, stop;

	if (K_TIMEOUT_EQ(duration, K_NO_WAIT) || K_TIMEOUT_EQ(duration, K_FOREVER)) {
		return -EINVAL;
	}

	ret = lora_recv_async(dev, lora_ed_discard, NULL);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_USEC(CONFIG_LORA_RSSI_SETTLE_US));

	expiry = sys_timepoint_calc(duration);

	while (!sys_timepoint_expired(expiry)) {
		int16_t rssi;

		ret = lora_rssi(dev, &rssi);
		if (ret < 0) {
			break;
		}

		if (rssi >= rssi_threshold) {
			busy = true;
			break;
		}

		k_sleep(K_USEC(CONFIG_LORA_ENERGY_DETECT_SAMPLE_INTERVAL_US));
	}

	stop = lora_recv_async(dev, NULL, NULL);
	if (stop < 0) {
		return stop;
	}

	if (ret < 0) {
		return ret;
	}

	return busy ? 1 : 0;
}
