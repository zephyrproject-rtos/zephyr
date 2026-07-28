/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xen_watchdog

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/xen/sched.h>

LOG_MODULE_REGISTER(wdt_xen, CONFIG_WDT_LOG_LEVEL);

#define XEN_WDT_MAX_CHANNELS		2U

enum xen_wdt_channel_flags {
	XEN_WDT_CHANNEL_INSTALLED,
	XEN_WDT_CHANNEL_ACTIVE,
};

enum xen_wdt_data_flags {
	XEN_WDT_SETUP,
};

/* Per-channel state for one Xen domain watchdog timer slot. */
struct xen_wdt_channel {
	/* Xen-assigned watchdog timer ID; zero means no Xen timer exists yet. */
	uint32_t xen_id;
	/* Timeout programmed into Xen for this channel, in seconds. */
	uint32_t timeout_sec;
	/* Number of feed calls currently using this Xen timer. */
	atomic_t feeds;
	/* Per-channel installed/active state. */
	atomic_t flags;
};

/* Mutable driver state shared by all watchdog API operations. */
struct xen_wdt_data {
	/* Serializes watchdog setup and teardown. */
	struct k_sem lock;
	/* Zephyr channel ID is the index into this Xen watchdog channel array. */
	struct xen_wdt_channel channels[XEN_WDT_MAX_CHANNELS];
	/* Device-wide setup state. */
	atomic_t flags;
};

struct xen_wdt_config {
	unsigned int num_channels;
};

/* Convert Zephyr's millisecond timeout into Xen's second-based timeout. */
static uint32_t xen_wdt_timeout_to_sec(uint32_t timeout_ms)
{
	uint64_t timeout_sec = DIV_ROUND_UP((uint64_t)timeout_ms, MSEC_PER_SEC);

	return (uint32_t)timeout_sec;
}

/* Arm every installed Zephyr channel as a Xen domain watchdog timer. */
static int xen_wdt_start_locked(struct xen_wdt_data *data, unsigned int num_channels)
{
	bool installed = false;
	int ret;

	for (unsigned int i = 0; i < num_channels; i++) {
		if (atomic_test_bit(&data->channels[i].flags, XEN_WDT_CHANNEL_INSTALLED)) {
			installed = true;
			break;
		}
	}

	if (!installed) {
		LOG_WRN("setup requested without installed timeouts");
		ret = -EINVAL;
		return ret;
	}

	for (unsigned int i = 0; i < num_channels; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];

		if (!atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED)) {
			continue;
		}

		channel->xen_id = 0U;
		ret = xen_sched_watchdog(&channel->xen_id, channel->timeout_sec);
		if (ret < 0) {
			LOG_WRN("failed to create Xen watchdog channel %u: %d", i, ret);
			goto cleanup;
		}
	}

	for (unsigned int i = 0; i < num_channels; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];

		if (atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED)) {
			atomic_set_bit(&channel->flags, XEN_WDT_CHANNEL_ACTIVE);
		}
	}

	atomic_set_bit(&data->flags, XEN_WDT_SETUP);
	ret = 0;
	return ret;

cleanup:
	for (unsigned int i = 0; i < num_channels; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];
		int cleanup_ret;

		if (channel->xen_id == 0U) {
			continue;
		}

		cleanup_ret = xen_sched_watchdog(&channel->xen_id, 0U);
		if (cleanup_ret < 0) {
			LOG_WRN("failed to clean up Xen watchdog channel %u: %d", i, cleanup_ret);
		}
		channel->xen_id = 0U;
	}

	return ret;
}

/* Destroy active Xen watchdog timers. Optionally make installed channels reusable. */
static int xen_wdt_stop_locked(struct xen_wdt_data *data, unsigned int num_channels,
			       bool clear_installed)
{
	int first_ret = 0;

	for (unsigned int i = 0; i < num_channels; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];
		int call_ret;

		if (!atomic_test_and_clear_bit(&channel->flags, XEN_WDT_CHANNEL_ACTIVE)) {
			continue;
		}

		while (atomic_get(&channel->feeds) != 0) {
			k_yield();
		}

		call_ret = xen_sched_watchdog(&channel->xen_id, 0U);
		if (call_ret < 0) {
			LOG_WRN("failed to destroy Xen watchdog channel %u: %d", i, call_ret);
			if (first_ret == 0) {
				first_ret = call_ret;
			}
			atomic_set_bit(&channel->flags, XEN_WDT_CHANNEL_ACTIVE);
			continue;
		}

		channel->xen_id = 0U;
		if (clear_installed) {
			atomic_clear_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED);
			channel->timeout_sec = 0U;
		}
	}

	if (first_ret < 0) {
		/* Successfully destroyed channels stay cleared; failed ones remain active. */
		return first_ret;
	}

	if (clear_installed) {
		atomic_clear_bit(&data->flags, XEN_WDT_SETUP);
	}
	return 0;
}

/* Arm every installed Zephyr channel as a Xen domain watchdog timer. */
static int xen_wdt_setup(const struct device *dev, uint8_t options)
{
	const struct xen_wdt_config *config = dev->config;
	struct xen_wdt_data *data = dev->data;
	int ret;

	/* Xen owns the timer, so Zephyr pause options cannot be represented. */
	if (options != 0U) {
		LOG_ERR("setup options 0x%x are not supported", options);
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (atomic_test_bit(&data->flags, XEN_WDT_SETUP)) {
		LOG_WRN("watchdog is already set up");
		ret = -EBUSY;
		goto out;
	}

	ret = xen_wdt_start_locked(data, config->num_channels);

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Destroy active Xen watchdog timers and make installed channels reusable. */
static int xen_wdt_disable(const struct device *dev)
{
	const struct xen_wdt_config *config = dev->config;
	struct xen_wdt_data *data = dev->data;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	if (!atomic_test_bit(&data->flags, XEN_WDT_SETUP)) {
		LOG_WRN("disable requested before setup");
		ret = -EFAULT;
		goto out;
	}

	ret = xen_wdt_stop_locked(data, config->num_channels, true);

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Reserve one free channel before setup; disable clears channels for reuse. */
static int xen_wdt_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	const struct xen_wdt_config *config = dev->config;
	struct xen_wdt_data *data = dev->data;
	int ret;

	if (cfg == NULL || cfg->window.max == 0U) {
		LOG_WRN("invalid timeout configuration");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (atomic_test_bit(&data->flags, XEN_WDT_SETUP)) {
		LOG_WRN("timeout install requested after setup");
		ret = -EBUSY;
		goto out;
	}

	/* Xen expires the whole domain, so only SoC reset semantics apply. */
	if (cfg->callback != NULL || cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_WRN("callback or flags 0x%x are not supported", cfg->flags);
		ret = -ENOTSUP;
		goto out;
	}

	if (cfg->window.min != 0U) {
		LOG_WRN("windowed timeouts are not supported");
		ret = -EINVAL;
		goto out;
	}

	if (cfg->window.max < MSEC_PER_SEC) {
		LOG_WRN("timeout must be at least one second");
		ret = -EINVAL;
		goto out;
	}

	for (unsigned int i = 0; i < config->num_channels; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];

		if (atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED)) {
			continue;
		}

		channel->timeout_sec = xen_wdt_timeout_to_sec(cfg->window.max);
		atomic_set_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED);
		ret = i;
		goto out;
	}

	LOG_WRN("no free Xen watchdog channels");
	ret = -ENOMEM;

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Refresh one active Xen watchdog timer without stalling behind another call. */
static int xen_wdt_feed(const struct device *dev, int channel_id)
{
	const struct xen_wdt_config *config = dev->config;
	struct xen_wdt_data *data = dev->data;
	struct xen_wdt_channel *channel;
	int ret;

	if (channel_id < 0 || (unsigned int)channel_id >= config->num_channels) {
		LOG_WRN("invalid feed channel %d", channel_id);
		return -EINVAL;
	}

	channel = &data->channels[channel_id];
	if (!atomic_test_bit(&data->flags, XEN_WDT_SETUP) ||
	    !atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_INSTALLED)) {
		LOG_WRN("feed channel %d has no installed timeout", channel_id);
		return -EINVAL;
	}

	if (!atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_ACTIVE)) {
		LOG_WRN("feed channel %d is not active", channel_id);
		return -EINVAL;
	}

	atomic_inc(&channel->feeds);
	if (!atomic_test_bit(&channel->flags, XEN_WDT_CHANNEL_ACTIVE)) {
		atomic_dec(&channel->feeds);
		LOG_WRN("feed channel %d is not active", channel_id);
		return -EINVAL;
	}

	ret = xen_sched_watchdog(&channel->xen_id, channel->timeout_sec);
	atomic_dec(&channel->feeds);
	if (ret < 0) {
		LOG_WRN("failed to feed Xen watchdog channel %d: %d", channel_id, ret);
	}

	return ret;
}

/* Prepare synchronization state before the watchdog device is used. */
static int xen_wdt_init(const struct device *dev)
{
	struct xen_wdt_data *data = dev->data;

	return k_sem_init(&data->lock, 1, 1);
}

/* Zephyr watchdog API dispatch table for xen,watchdog devices. */
static DEVICE_API(wdt, xen_wdt_api) = {
	.setup = xen_wdt_setup,
	.disable = xen_wdt_disable,
	.install_timeout = xen_wdt_install_timeout,
	.feed = xen_wdt_feed,
};

/* Instantiate per-devicetree-node state and bind it to the watchdog API. */
#define XEN_WDT_INIT(inst)							\
	static struct xen_wdt_data xen_wdt_data_##inst;			\
	static const struct xen_wdt_config xen_wdt_config_##inst = {	\
		.num_channels = DT_INST_PROP(inst, num_channels),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, xen_wdt_init, NULL,			\
			      &xen_wdt_data_##inst,				\
			      &xen_wdt_config_##inst, POST_KERNEL,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &xen_wdt_api);

DT_INST_FOREACH_STATUS_OKAY(XEN_WDT_INIT)
