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
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/xen/sched.h>

LOG_MODULE_REGISTER(wdt_xen, CONFIG_WDT_LOG_LEVEL);

#define XEN_WDT_MAX_CHANNELS		CONFIG_WDT_XEN_CHANNELS
#define XEN_WDT_MIN_TIMEOUT_SEC		1U

/* Per-channel state for one Xen domain watchdog timer slot. */
struct xen_wdt_channel {
	/* Xen-assigned watchdog timer ID; zero means no Xen timer exists yet. */
	uint32_t xen_id;
	/* Timeout programmed into Xen for this channel, in seconds. */
	uint32_t timeout_sec;
	/* Channel was reserved by wdt_install_timeout(). */
	bool installed;
	/* Xen timer was created by wdt_setup() and can be fed or destroyed. */
	bool active;
};

/* Mutable driver state shared by all watchdog API operations. */
struct xen_wdt_data {
	/* Serializes channel state changes and matching Xen hypercalls. */
	struct k_sem lock;
	/* Zephyr channel ID is the index into this Xen watchdog channel array. */
	struct xen_wdt_channel channels[XEN_WDT_MAX_CHANNELS];
	/* At least one installed channel has been armed through Xen. */
	bool setup;
	/* Watchdog timers were active before a device PM suspend transition. */
	bool suspended;
};

/* Convert Zephyr's millisecond timeout into Xen's second-based timeout. */
static uint32_t xen_wdt_timeout_to_sec(uint32_t timeout_ms)
{
	uint64_t timeout_sec = DIV_ROUND_UP((uint64_t)timeout_ms, MSEC_PER_SEC);

	return (uint32_t)MAX(timeout_sec, (uint64_t)XEN_WDT_MIN_TIMEOUT_SEC);
}

/* Arm every installed Zephyr channel as a Xen domain watchdog timer. */
static int xen_wdt_start_locked(struct xen_wdt_data *data)
{
	bool installed = false;
	int ret;

	for (unsigned int i = 0; i < XEN_WDT_MAX_CHANNELS; i++) {
		if (data->channels[i].installed) {
			installed = true;
			break;
		}
	}

	if (!installed) {
		LOG_WRN("setup requested without installed timeouts");
		ret = -EINVAL;
		return ret;
	}

	for (unsigned int i = 0; i < XEN_WDT_MAX_CHANNELS; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];

		if (!channel->installed) {
			continue;
		}

		channel->xen_id = 0U;
		ret = xen_sched_watchdog(&channel->xen_id, channel->timeout_sec);
		if (ret < 0) {
			LOG_WRN("failed to create Xen watchdog channel %u: %d", i, ret);
			goto cleanup;
		}

		channel->active = true;
	}

	data->setup = true;
	ret = 0;
	return ret;

cleanup:
	for (unsigned int i = 0; i < XEN_WDT_MAX_CHANNELS; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];
		int cleanup_ret;

		if (!channel->active) {
			continue;
		}

		cleanup_ret = xen_sched_watchdog(&channel->xen_id, 0U);
		if (cleanup_ret < 0) {
			LOG_WRN("failed to clean up Xen watchdog channel %u: %d", i, cleanup_ret);
		}
		channel->xen_id = 0U;
		channel->active = false;
	}

	return ret;
}

/* Destroy active Xen watchdog timers. Optionally make installed channels reusable. */
static int xen_wdt_stop_locked(struct xen_wdt_data *data, bool clear_installed)
{
	int first_ret = 0;

	for (unsigned int i = 0; i < XEN_WDT_MAX_CHANNELS; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];
		int call_ret;

		if (!channel->active) {
			continue;
		}

		call_ret = xen_sched_watchdog(&channel->xen_id, 0U);
		if (call_ret < 0) {
			LOG_WRN("failed to destroy Xen watchdog channel %u: %d", i, call_ret);
			if (first_ret == 0) {
				first_ret = call_ret;
			}
			continue;
		}

		channel->xen_id = 0U;
		channel->active = false;
		if (clear_installed) {
			channel->installed = false;
			channel->timeout_sec = 0U;
		}
	}

	if (first_ret < 0) {
		/* Successfully destroyed channels stay cleared; failed ones remain active. */
		return first_ret;
	}

	data->setup = false;
	return 0;
}

/* Arm every installed Zephyr channel as a Xen domain watchdog timer. */
static int xen_wdt_setup(const struct device *dev, uint8_t options)
{
	struct xen_wdt_data *data = dev->data;
	int ret;

	/* Xen owns the timer, so Zephyr pause options cannot be represented. */
	if (options != 0U) {
		LOG_ERR("setup options 0x%x are not supported", options);
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->setup) {
		LOG_WRN("watchdog is already set up");
		ret = -EBUSY;
		goto out;
	}

	ret = xen_wdt_start_locked(data);

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Destroy active Xen watchdog timers and make installed channels reusable. */
static int xen_wdt_disable(const struct device *dev)
{
	struct xen_wdt_data *data = dev->data;
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	if (!data->setup) {
		LOG_WRN("disable requested before setup");
		ret = -EFAULT;
		goto out;
	}

	ret = xen_wdt_stop_locked(data, true);
	if (ret == 0) {
		data->suspended = false;
	}

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Reserve one free channel before setup; disable clears channels for reuse. */
static int xen_wdt_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	struct xen_wdt_data *data = dev->data;
	int ret;

	if (cfg == NULL || cfg->window.max == 0U) {
		LOG_WRN("invalid timeout configuration");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->setup) {
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

	for (unsigned int i = 0; i < XEN_WDT_MAX_CHANNELS; i++) {
		struct xen_wdt_channel *channel = &data->channels[i];

		if (channel->installed) {
			continue;
		}

		channel->timeout_sec = xen_wdt_timeout_to_sec(cfg->window.max);
		channel->installed = true;
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
	struct xen_wdt_data *data = dev->data;
	struct xen_wdt_channel *channel;
	int ret;

	if (channel_id < 0 || channel_id >= XEN_WDT_MAX_CHANNELS) {
		LOG_WRN("invalid feed channel %d", channel_id);
		return -EINVAL;
	}

	if (k_sem_take(&data->lock, K_NO_WAIT) < 0) {
		LOG_WRN("feed channel %d would stall", channel_id);
		return -EAGAIN;
	}

	channel = &data->channels[channel_id];
	if (!channel->installed) {
		LOG_WRN("feed channel %d has no installed timeout", channel_id);
		ret = -EINVAL;
		goto out;
	}

	if (!channel->active) {
		LOG_WRN("feed channel %d is not active", channel_id);
		ret = -EFAULT;
		goto out;
	}

	ret = xen_sched_watchdog(&channel->xen_id, channel->timeout_sec);
	if (ret < 0) {
		LOG_WRN("failed to feed Xen watchdog channel %d: %d", channel_id, ret);
	}

out:
	k_sem_give(&data->lock);
	return ret;
}

/* Prepare synchronization state before the watchdog device is used. */
static int xen_wdt_init(const struct device *dev)
{
	struct xen_wdt_data *data = dev->data;

	return k_sem_init(&data->lock, 1, 1);
}

#ifdef CONFIG_PM_DEVICE
static int xen_wdt_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	struct xen_wdt_data *data = dev->data;
	int ret = 0;

	if (k_sem_take(&data->lock, K_NO_WAIT) < 0) {
		LOG_WRN("PM action %d would stall", action);
		return -EBUSY;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (data->setup) {
			ret = xen_wdt_stop_locked(data, false);
			if (ret == 0) {
				data->suspended = true;
			}
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		if (data->suspended) {
			ret = xen_wdt_start_locked(data);
			if (ret == 0) {
				data->suspended = false;
			}
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_sem_give(&data->lock);
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

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
										\
	PM_DEVICE_DT_INST_DEFINE(inst, xen_wdt_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(inst, xen_wdt_init,			\
			      PM_DEVICE_DT_INST_GET(inst),			\
			      &xen_wdt_data_##inst, NULL, POST_KERNEL,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &xen_wdt_api);

DT_INST_FOREACH_STATUS_OKAY(XEN_WDT_INIT)
