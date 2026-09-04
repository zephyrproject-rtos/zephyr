/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_wdt_mss

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_mchp_mss, CONFIG_WDT_LOG_LEVEL);

#define CORE_WDT_REFRESH (0x00)
#define CORE_WDT_CONTROL (0x04)
#define CORE_WDT_STATUS  (0x08)
#define CORE_WDT_TIME    (0x0C)
#define CORE_WDT_MSVP    (0x10)
#define CORE_WDT_TRIGGER (0x14)
#define CORE_WDT_FORCE   (0x18)

#define REFRESH_WDOGREFRESH_KEY (0xDEADC0DE)

#define CONTROL_ENABLE_FORBIDDEN_MASK BIT(4)
#define CONTROL_ACTIVE_SLEEP_MASK     BIT(3)
#define CONTROL_INTEN_SLEEP_MASK      BIT(2)
#define CONTROL_INTEN_TRIG_MASK       BIT(1)
#define CONTROL_INTEN_MSVP_MASK       BIT(0)

#define STATUS_DEVRST_MASK       BIT(5)
#define STATUS_LOCKED_MASK       BIT(4)
#define STATUS_TRIGGERED_MASK    BIT(3)
#define STATUS_FORBIDDEN_MASK    BIT(2)
#define STATUS_WDOG_TRIPPED_MASK BIT(1)
#define STATUS_MVRP_TRIPPED_MASK BIT(0)

#define TIME_WDOGTIME_MASK BIT_MASK(24)
#define TIME_WDOGTIME_MAX  TIME_WDOGTIME_MASK

#define MSVP_WDOGMVRP_MASK BIT_MASK(24)

#define TRIGGER_WDOGTRIGGER_MASK BIT_MASK(12)

#define FORCE_WDOGVALUE_IMMEDIATE_RESET (0x0000000C)

struct mss_wdt_config {
	bool reset_capable;
	uintptr_t wdt_base_addr;
	uint32_t wdt_clk;

	void (*irq_config_func)(const struct device *dev);
	unsigned int mvrp_irq;
	unsigned int tout_irq;
};

struct mss_wdt_data {
	bool running;
	bool already_programmed;

	wdt_callback_t callback;
	bool callback_enabled;
	struct k_spinlock lock;
};

static int wdt_mchp_mss_setup(const struct device *dev, uint8_t options)
{
	const struct mss_wdt_config *cfg = dev->config;
	struct mss_wdt_data *data = dev->data;
	uint32_t regval;
	int retval = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->running) {
		retval = -EBUSY;
		goto cleanup;
	}

	if (!data->already_programmed) {
		/* The wdg, once started, can never be stopped.
		 * Refuse to start one that is still holding its
		 * reset-value timeout.
		 */
		retval = -EINVAL;
		goto cleanup;
	}

	/* MSS WDG is always halted when in debug mode. */
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) == 0) {
		retval = -ENOTSUP;
		goto cleanup;
	}

	regval = sys_read32(cfg->wdt_base_addr + CORE_WDT_CONTROL);
	regval &= ~(CONTROL_ENABLE_FORBIDDEN_MASK);

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) != 0) {
		regval &= ~CONTROL_ACTIVE_SLEEP_MASK;
	} else {
		regval |= CONTROL_ACTIVE_SLEEP_MASK;
	}

	sys_write32(regval, cfg->wdt_base_addr + CORE_WDT_CONTROL);

	/* start the countdown */
	sys_write32(REFRESH_WDOGREFRESH_KEY, cfg->wdt_base_addr + CORE_WDT_REFRESH);

	data->running = true;

cleanup:

	k_spin_unlock(&data->lock, key);

	return retval;
}

static int wdt_mchp_mss_disable(const struct device *dev)
{
	struct mss_wdt_data *data = dev->data;
	int retval = -EPERM;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!data->running) {
		retval = -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return retval;
}

static int wdt_mchp_mss_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *devcfg = dev->config;
	uint32_t wdog_time;
	uint64_t wdog_time_raw;
	uint32_t wdog_mvrp;
	uint64_t wdog_min_raw;
	uint32_t regval;

	int retval = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->already_programmed) {
		/* The only modification a user would be allowed to do
		 * would be to update the callback if the timer's already been
		 * setup for that.
		 */
		if (data->callback_enabled) {
			data->callback = cfg->callback;
			retval = 0;
		} else {
			retval = -EBUSY;
		}
		goto cleanup;
	}

	if ((sys_read32(devcfg->wdt_base_addr + CORE_WDT_STATUS) & STATUS_LOCKED_MASK) != 0) {
		retval = -EBUSY;
		goto cleanup;
	}

#if defined(CONFIG_WDT_MULTISTAGE)
	if (cfg->next != NULL) {
		retval = -ENOTSUP;
		goto cleanup;
	}
#endif
	if (cfg->flags & WDT_FLAG_RESET_CPU_CORE) {
		/* No MSS watchdog is able to just reset a HART */
		retval = -ENOTSUP;
		goto cleanup;
	}

	if (((cfg->flags & WDT_FLAG_RESET_MASK) != WDT_FLAG_RESET_NONE) != devcfg->reset_capable) {
		/* Only WDT0 is able to reset the SoC, and it's actually not possible
		 * to configure it not to do so.
		 */
		retval = -ENOTSUP;
		goto cleanup;
	}

	if (cfg->window.min != 0U && (cfg->window.min > cfg->window.max)) {
		retval = -EINVAL;
		goto cleanup;
	}

	/* cfg->window.max maps to the WDOG_TIME register */
	wdog_time_raw = ((uint64_t)cfg->window.max * devcfg->wdt_clk) / 1000;
	if ((wdog_time_raw > (uint64_t)TIME_WDOGTIME_MAX) || wdog_time_raw == 0) {
		retval = -EINVAL;
		goto cleanup;
	}
	wdog_time = (uint32_t)wdog_time_raw;

	/* cfg->window.min can instead be mapped to MVRP value */
	if (cfg->window.min > 0) {
		wdog_min_raw = ((uint64_t)cfg->window.min * devcfg->wdt_clk) / 1000;
		if (wdog_min_raw == 0 || wdog_min_raw >= (uint64_t)wdog_time) {
			retval = -EINVAL;
			goto cleanup;
		}
		wdog_mvrp = wdog_time - (uint32_t)wdog_min_raw;
	} else {
		wdog_mvrp = wdog_time;
	}

	/* MVRP and TRIGGER registers will be locked by writing to TIME, so they
	 * get programmed first. Trigger is left disabled since it's used for
	 * an (unsupported) INT -> RST pattern.
	 */
	sys_write32(wdog_mvrp, devcfg->wdt_base_addr + CORE_WDT_MSVP);
	sys_write32(0, devcfg->wdt_base_addr + CORE_WDT_TRIGGER);
	sys_write32(wdog_time, devcfg->wdt_base_addr + CORE_WDT_TIME);

	data->callback = cfg->callback;

	/* Enable interrupts only if a callback is enabled */
	if (data->callback) {
		data->callback_enabled = true;
		regval = sys_read32(devcfg->wdt_base_addr + CORE_WDT_CONTROL);
		regval |= CONTROL_INTEN_SLEEP_MASK;
		irq_enable(devcfg->tout_irq);
		if (wdog_mvrp > 0) {
			regval |= CONTROL_INTEN_MSVP_MASK;
			irq_enable(devcfg->mvrp_irq);
		}

		sys_write32(regval, devcfg->wdt_base_addr + CORE_WDT_CONTROL);
	}

	data->already_programmed = true;

cleanup:
	k_spin_unlock(&data->lock, key);

	return retval;
}

static int wdt_mchp_mss_feed(const struct device *dev, int channel_id)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *cfg = dev->config;
	int retval = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (channel_id != 0) {
		retval = -EINVAL;
		goto cleanup;
	}

	if (!data->running) {
		retval = -EINVAL;
		goto cleanup;
	}

	sys_write32(REFRESH_WDOGREFRESH_KEY, cfg->wdt_base_addr + CORE_WDT_REFRESH);

cleanup:
	k_spin_unlock(&data->lock, key);

	return retval;
}

static void wdt_mchp_mss_mvrp_isr(const struct device *dev)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *cfg = dev->config;
	uint32_t status = sys_read32(cfg->wdt_base_addr + CORE_WDT_STATUS);
	wdt_callback_t cb;
	k_spinlock_key_t key;

	if (status & STATUS_MVRP_TRIPPED_MASK) {
		sys_write32(STATUS_MVRP_TRIPPED_MASK, cfg->wdt_base_addr + CORE_WDT_STATUS);

		key = k_spin_lock(&data->lock);
		cb = data->callback;
		k_spin_unlock(&data->lock, key);

		if (cb != NULL) {
			cb(dev, 0);
		}
	}
}

static void wdt_mchp_mss_trig_isr(const struct device *dev)
{
	struct mss_wdt_data *data = dev->data;
	const struct mss_wdt_config *cfg = dev->config;
	uint32_t status = sys_read32(cfg->wdt_base_addr + CORE_WDT_STATUS);
	wdt_callback_t cb;
	k_spinlock_key_t key;

	if (status & STATUS_WDOG_TRIPPED_MASK) {
		sys_write32(STATUS_WDOG_TRIPPED_MASK, cfg->wdt_base_addr + CORE_WDT_STATUS);

		key = k_spin_lock(&data->lock);
		cb = data->callback;
		k_spin_unlock(&data->lock, key);

		if (cb != NULL) {
			cb(dev, 0);
		}
	}
}

static int wdt_mchp_mss_init(const struct device *dev)
{
	const struct mss_wdt_config *cfg = dev->config;
	struct mss_wdt_data *data = dev->data;
	uint32_t regval;

	if (cfg->wdt_clk == 0) {
		return -EINVAL;
	}

	/* Check STATUS.LOCKED bit. The watchdog can only be programmed once,
	 * so installing timeouts from user code won't be effective.
	 */
	regval = sys_read32(cfg->wdt_base_addr + CORE_WDT_STATUS);
	data->already_programmed = (regval & STATUS_LOCKED_MASK) != 0;

	cfg->irq_config_func(dev);

	return 0;
}

static DEVICE_API(wdt, wdt_mchp_mss_driver_api) = {
	.setup = wdt_mchp_mss_setup,
	.disable = wdt_mchp_mss_disable,
	.install_timeout = wdt_mchp_mss_install_timeout,
	.feed = wdt_mchp_mss_feed,
};

/* DTS order of interrupts is {MVRP, TOUT} for each watchdog */
/* DTS order of interrupts is {MVRP, TOUT} for each watchdog */
#define WDT_MCHP_MSS_MVRP_IRQ_CONNECT(idx)                                                         \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(idx, 0), DT_INST_IRQ_BY_IDX(idx, 0, priority),             \
		    wdt_mchp_mss_mvrp_isr, DEVICE_DT_INST_GET(idx), 0);

#define WDT_MCHP_MSS_TRIG_IRQ_CONNECT(idx)                                                         \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(idx, 1), DT_INST_IRQ_BY_IDX(idx, 1, priority),             \
		    wdt_mchp_mss_trig_isr, DEVICE_DT_INST_GET(idx), 0);

#define WDT_MCHP_MSS_WDT_DEVICE(idx)                                                               \
	static void wdt_mchp_irq_config_##idx(const struct device *dev)                            \
	{                                                                                          \
		WDT_MCHP_MSS_MVRP_IRQ_CONNECT(idx);                                                \
		WDT_MCHP_MSS_TRIG_IRQ_CONNECT(idx);                                                \
	}                                                                                          \
	static const struct mss_wdt_config wdt_##idx##_config = {                                  \
		.reset_capable = DT_INST_PROP(idx, reset_capable),                                 \
		.wdt_base_addr = DT_INST_REG_ADDR(idx),                                            \
		.wdt_clk = DT_INST_PROP(idx, clock_frequency) >> 8,                                \
		.irq_config_func = wdt_mchp_irq_config_##idx,                                      \
		.mvrp_irq = DT_INST_IRQN_BY_IDX(idx, 0),                                           \
		.tout_irq = DT_INST_IRQN_BY_IDX(idx, 1)};                                          \
                                                                                                   \
	static struct mss_wdt_data wdt_##idx##_data = {.running = false,                           \
						       .already_programmed = false,                \
						       .callback = NULL,                           \
						       .callback_enabled = false};                 \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_NODELABEL(watchdog##idx), wdt_mchp_mss_init, NULL, &wdt_##idx##_data,  \
			 &wdt_##idx##_config, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,    \
			 &wdt_mchp_mss_driver_api)

DT_INST_FOREACH_STATUS_OKAY(WDT_MCHP_MSS_WDT_DEVICE);
