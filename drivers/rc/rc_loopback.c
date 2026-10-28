/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rc_loopback

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#define RC_LOOPBACK_TIMEOUT_MIN_US 1000U
#define RC_LOOPBACK_TIMEOUT_MAX_US 1000000U
#define RC_LOOPBACK_CARRIER_MIN_HZ 20000U
#define RC_LOOPBACK_CARRIER_MAX_HZ 60000U

struct rc_loopback_config {
	bool hw_rx_timeout;
	bool rx_only;
	bool tx_echo;
};

struct rc_loopback_data {
	struct rc_dev_data rc;
	atomic_t rx_enabled;
	atomic_t rx_timeout_us;
#ifdef CONFIG_RC_TX
	struct k_spinlock tx_lock;
	uint32_t tx_words[CONFIG_RC_LOOPBACK_TX_WORDS];
	size_t tx_count;
	uint32_t tx_total;
	uint32_t carrier_hz;
	uint8_t duty_pct;
	atomic_t tx_hold;
	struct k_sem tx_gate;
#endif
};

static int rc_loopback_get_caps(const struct device *dev, struct rc_caps *caps)
{
	const struct rc_loopback_config *config = dev->config;

	*caps = (struct rc_caps){
		.roles = RC_ROLE_RAW_RX,
		.rx_resolution_ns = 1000U,
	};

#ifdef CONFIG_RC_TX
	if (!config->rx_only) {
		caps->roles |= RC_ROLE_RAW_TX;
		caps->carrier_min_hz = RC_LOOPBACK_CARRIER_MIN_HZ;
		caps->carrier_max_hz = RC_LOOPBACK_CARRIER_MAX_HZ;
		caps->max_tx_words = CONFIG_RC_LOOPBACK_TX_WORDS;
	}
#endif

	if (config->hw_rx_timeout) {
		caps->rx_timeout_min_us = RC_LOOPBACK_TIMEOUT_MIN_US;
		caps->rx_timeout_max_us = RC_LOOPBACK_TIMEOUT_MAX_US;
	}

	return 0;
}

static int rc_loopback_rx_enable(const struct device *dev, bool enable)
{
	struct rc_loopback_data *data = dev->data;

	atomic_set(&data->rx_enabled, enable ? 1 : 0);

	return 0;
}

bool rc_loopback_rx_is_enabled(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;

	return atomic_get(&data->rx_enabled) != 0;
}

static int rc_loopback_set_rx_timeout(const struct device *dev, uint32_t us)
{
	struct rc_loopback_data *data = dev->data;

	atomic_set(&data->rx_timeout_us, (atomic_val_t)us);

	return 0;
}

uint32_t rc_loopback_rx_timeout_us(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;

	return (uint32_t)atomic_get(&data->rx_timeout_us);
}

int rc_loopback_inject(const struct device *dev, const uint32_t *words, size_t count)
{
	if (!rc_loopback_rx_is_enabled(dev)) {
		return -EAGAIN;
	}

	for (size_t i = 0; i < count; i++) {
		int ret = rc_raw_store(dev, words[i]);

		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#ifdef CONFIG_RC_TX
static int rc_loopback_set_carrier(const struct device *dev, uint32_t hz, uint8_t duty_pct)
{
	struct rc_loopback_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);

	data->carrier_hz = hz;
	data->duty_pct = duty_pct;
	k_spin_unlock(&data->tx_lock, key);

	return 0;
}

static int rc_loopback_tx(const struct device *dev, const uint32_t *words, size_t count)
{
	const struct rc_loopback_config *config = dev->config;
	struct rc_loopback_data *data = dev->data;
	k_spinlock_key_t key;

	if (atomic_get(&data->tx_hold) != 0) {
		(void)k_sem_take(&data->tx_gate, K_FOREVER);
	}

	key = k_spin_lock(&data->tx_lock);
	data->tx_count = MIN(count, ARRAY_SIZE(data->tx_words));
	memcpy(data->tx_words, words, data->tx_count * sizeof(words[0]));
	data->tx_total++;
	k_spin_unlock(&data->tx_lock, key);

	if (config->tx_echo && rc_loopback_rx_is_enabled(dev)) {
		for (size_t i = 0; i < count; i++) {
			(void)rc_raw_store(dev, words[i]);
		}
	}

	return 0;
}

size_t rc_loopback_tx_get(const struct device *dev, uint32_t *words, size_t max, uint32_t *hz,
			  uint8_t *duty_pct)
{
	struct rc_loopback_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);
	size_t count = MIN(data->tx_count, max);

	memcpy(words, data->tx_words, count * sizeof(words[0]));
	*hz = data->carrier_hz;
	*duty_pct = data->duty_pct;
	k_spin_unlock(&data->tx_lock, key);

	return count;
}

uint32_t rc_loopback_tx_total(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->tx_lock);
	uint32_t total = data->tx_total;

	k_spin_unlock(&data->tx_lock, key);

	return total;
}

void rc_loopback_tx_hold(const struct device *dev, bool hold)
{
	struct rc_loopback_data *data = dev->data;

	if (hold) {
		k_sem_reset(&data->tx_gate);
		atomic_set(&data->tx_hold, 1);
	} else {
		atomic_set(&data->tx_hold, 0);
		k_sem_give(&data->tx_gate);
	}
}

static int rc_loopback_init(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;

	k_sem_init(&data->tx_gate, 0, 1);

	return 0;
}
#endif

static DEVICE_API(rc, rc_loopback_api) = {
	.get_caps = rc_loopback_get_caps,
	.rx_enable = rc_loopback_rx_enable,
#ifdef CONFIG_RC_TX
	.set_carrier = rc_loopback_set_carrier,
	.tx = rc_loopback_tx,
#endif
};

static DEVICE_API(rc, rc_loopback_hw_timeout_api) = {
	.get_caps = rc_loopback_get_caps,
	.rx_enable = rc_loopback_rx_enable,
	.set_rx_timeout = rc_loopback_set_rx_timeout,
#ifdef CONFIG_RC_TX
	.set_carrier = rc_loopback_set_carrier,
	.tx = rc_loopback_tx,
#endif
};

#define RC_LOOPBACK_API(inst)                                                                      \
	COND_CODE_1(DT_INST_PROP(inst, hw_rx_timeout), (&rc_loopback_hw_timeout_api),              \
		    (&rc_loopback_api))

#define RC_LOOPBACK_INIT COND_CODE_1(CONFIG_RC_TX, (rc_loopback_init), (NULL))

#define RC_LOOPBACK_DEFINE(inst)                                                                   \
	static const struct rc_loopback_config rc_loopback_config_##inst = {                       \
		.hw_rx_timeout = DT_INST_PROP(inst, hw_rx_timeout),                                \
		.rx_only = DT_INST_PROP(inst, rx_only),                                            \
		.tx_echo = DT_INST_PROP(inst, tx_echo),                                            \
	};                                                                                         \
	static struct rc_loopback_data rc_loopback_data_##inst;                                    \
	RC_DEVICE_DT_INST_DEFINE(inst, RC_LOOPBACK_INIT, NULL, &rc_loopback_data_##inst,           \
				 &rc_loopback_config_##inst, POST_KERNEL, CONFIG_RC_INIT_PRIORITY, \
				 RC_LOOPBACK_API(inst));

DT_INST_FOREACH_STATUS_OKAY(RC_LOOPBACK_DEFINE)
