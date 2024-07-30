/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT intel_tco_wdt

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_tco, CONFIG_WDT_LOG_LEVEL);

#define BASE(d)                ((struct tco_config *)(d)->config)->base

#define TCO_RLD(d)             (BASE(d) + 0x00) /* TCO Timer Reload/Curr. Value */
#define TCO_DAT_IN(d)          (BASE(d) + 0x02) /* TCO Data In Register */
#define TCO_DAT_OUT(d)         (BASE(d) + 0x03) /* TCO Data Out Register */
#define TCO1_STS(d)            (BASE(d) + 0x04) /* TCO1 Status Register */
#define TCO2_STS(d)            (BASE(d) + 0x06) /* TCO2 Status Register */
#define TCO1_CNT(d)            (BASE(d) + 0x08) /* TCO1 Control Register */
#define TCO2_CNT(d)            (BASE(d) + 0x0a) /* TCO2 Control Register */
#define TCO_MSG(d)             (BASE(d) + 0x0c) /* TCO Message Registers */
#define TCO_WDSTATUS(d)        (BASE(d) + 0x0e) /* TCO Watchdog Status Register */
#define TCO_TMR(d)             (BASE(d) + 0x12) /* TCO Timer Register */

/* TCO1_STS bits */
#define STS_NMI2SMI            BIT(0)
#define STS_OS_TCO_SMI         BIT(1)
#define STS_TCO_INT            BIT(2)
#define STS_TIMEOUT            BIT(3)
#define STS_NEWCENTURY         BIT(7)
#define STS_BIOSWR             BIT(8)
#define STS_CPUSCI             BIT(9)
#define STS_CPUSMI             BIT(10)
#define STS_CPUSERR            BIT(12)
#define STS_SLVSEL             BIT(13)

/* TCO2_STS bits */
#define STS_INTRD_DET          BIT(0)
#define STS_SECOND_TO          BIT(1)
#define STS_NRSTRAP            BIT(2)
#define STS_SMLINK_SLAVE_SMI   BIT(3)

/* TCO1_CNT bits */
#define CNT_NR_MSUS            BIT(0)
#define CNT_NMI_NOW            BIT(8)
#define CNT_NMI2SMI_EN         BIT(9)
#define CNT_TCO_TMR_HALT       BIT(11)
#define CNT_TCO_LOCK           BIT(12)

/* TCO_TMR bits */
#define TMR_TCOTMR             BIT_MASK(10)
#define TMR_MIN                0x04
#define TMR_MAX                0x3f

struct tco_data {
	struct k_spinlock lock;
	bool no_reboot;
};

struct tco_config {
	io_port_t base;
};

static int set_no_reboot(const struct device *dev, bool set)
{
	uint16_t val, newval;

	val = sys_in16(TCO1_CNT(dev));

	if (set) {
		val |= CNT_NR_MSUS;
	} else {
		val &= ~CNT_NR_MSUS;
	}

	sys_out16(val, TCO1_CNT(dev));
	newval = sys_in16(TCO1_CNT(dev));

	if (val != newval) {
		return -EIO;
	}

	return 0;
}

static int tco_setup(const struct device *dev, uint8_t options)
{
	struct tco_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t val;
	int err;

	key = k_spin_lock(&data->lock);

	err = set_no_reboot(dev, data->no_reboot);
	if (err) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Failed to update no_reboot bit (err %d)", err);
		return err;
	}

	/* Reload the timer */
	sys_out16(0x01, TCO_RLD(dev));

	/* Enable the timer to start counting by clearing the TCO_TMR_HALT field */
	val = sys_in16(TCO1_CNT(dev));
	val &= ~CNT_TCO_TMR_HALT;
	sys_out16(val, TCO1_CNT(dev));
	val = sys_in16(TCO1_CNT(dev));

	k_spin_unlock(&data->lock, key);

	if ((val & CNT_TCO_TMR_HALT) == CNT_TCO_TMR_HALT) {
		return -EIO;
	}

	return 0;
}

static int tco_disable(const struct device *dev)
{
	struct tco_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t val;

	key = k_spin_lock(&data->lock);

	/* Set the TCO_TMR_HALT field so that the timer gets halted */
	val = sys_in16(TCO1_CNT(dev));
	val |= CNT_TCO_TMR_HALT;
	sys_out16(val, TCO1_CNT(dev));
	val = sys_in16(TCO1_CNT(dev));

	set_no_reboot(dev, true);

	k_spin_unlock(&data->lock, key);

	if ((val & CNT_TCO_TMR_HALT) == 0) {
		return -EIO;
	}

	return 0;
}

static uint16_t msec_to_ticks(uint32_t msec)
{
	/* Convert from milliseconds to timer ticks. The timer is clocked at
	 * approximately 0.6 seconds.
	 */
	return ((msec / MSEC_PER_SEC) * 10) / 6;
}

static int tco_install_timeout(const struct device *dev,
			       const struct wdt_timeout_cfg *cfg)
{
	struct tco_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t val, ticks;

	/* TCO watchdog doesn't support windowed timeouts */
	if (cfg->window.min != 0) {
		return -EINVAL;
	}

	/* No callback support */
	if (cfg->callback != NULL) {
		return -ENOTSUP;
	}

	ticks = msec_to_ticks(cfg->window.max);

	LOG_DBG("window.max %u -> ticks %u", cfg->window.max, ticks);

	if (ticks < TMR_MIN || ticks > TMR_MAX) {
		return -EINVAL;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		data->no_reboot = false;
		break;
	case WDT_FLAG_RESET_NONE:
		data->no_reboot = true;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
		LOG_ERR("CPU-only reset not supported");
		return -ENOTSUP;
	default:
		LOG_ERR("Unknown watchdog configuration flags");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Set the TCO_TMR field. This value is loaded into the timer each time
	 * the TCO_RLD register is written.
	 */
	val = sys_in16(TCO_TMR(dev));
	val &= ~TMR_TCOTMR;
	val |= ticks;
	sys_out16(val, TCO_TMR(dev));
	val = sys_in16(TCO_TMR(dev));

	k_spin_unlock(&data->lock, key);

	if ((val & TMR_TCOTMR) != ticks) {
		LOG_ERR("val %u", val);
		return -EIO;
	}

	return 0;
}

static int tco_feed(const struct device *dev, int channel_id)
{
	struct tco_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	/* TCORLD: Writing any value to this register will reload the timer to
	 * prevent the timeout.
	 */
	sys_out16(0x01, TCO_RLD(dev));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static const struct wdt_driver_api tco_driver_api = {
	.setup = tco_setup,
	.disable = tco_disable,
	.install_timeout = tco_install_timeout,
	.feed = tco_feed,
};

static int wdt_init(const struct device *dev)
{
	const struct tco_config *config = dev->config;
	struct tco_data *data = dev->data;
	k_spinlock_key_t key;

	LOG_DBG("Using 0x%04x as TCOBA", config->base);

	key = k_spin_lock(&data->lock);

	sys_out16(STS_TIMEOUT, TCO1_STS(dev)); /* Clear the Time Out Status bit */
	sys_out16(STS_SECOND_TO, TCO2_STS(dev)); /* Clear SECOND_TO_STS bit */

	set_no_reboot(dev, data->no_reboot);

	k_spin_unlock(&data->lock, key);

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		tco_disable(dev);
	}

	return 0;
}

static struct tco_data wdt_data = {
};

static const struct tco_config wdt_config = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, wdt_init, NULL, &wdt_data, &wdt_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &tco_driver_api);
