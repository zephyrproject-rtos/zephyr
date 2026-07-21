/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_rtc

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(counter_bflb_rtc, CONFIG_COUNTER_LOG_LEVEL);

#include <bouffalolab/common/rtc_reg.h>

/*
 * Bouffalo Lab RTC is a 40-bit counter in the HBN (hibernate) block.
 * It runs from a 32,768 Hz oscillator (always-on domain).
 *
 * Latch-and-read: Set bit 31 of RTC_TIME_H, clear it, then read.
 * We expose it as a 32-bit counter to Zephyr (lower 32 bits).
 * At 32768 Hz, 32-bit overflow = ~36 hours.
 *
 * Alarm support is deferred: HBN_OUT0 IRQ is shared with powerctrl.
 */

#define RTC_FREQ 32768U

struct counter_bflb_rtc_config {
	struct counter_config_info info;
	uintptr_t base;
};

static int counter_bflb_rtc_start(const struct device *dev)
{
	const struct counter_bflb_rtc_config *cfg = dev->config;
	uint32_t tmp;
	unsigned int key;

	/* Enable RTC counter in HBN_CTL. */
	key = irq_lock();
	tmp = sys_read32(cfg->base + HBN_CTL_OFFSET);
	tmp |= HBN_RTC_ENABLE;
	sys_write32(tmp, cfg->base + HBN_CTL_OFFSET);
	irq_unlock(key);

	return 0;
}

static int counter_bflb_rtc_stop(const struct device *dev)
{
	const struct counter_bflb_rtc_config *cfg = dev->config;
	uint32_t tmp;
	unsigned int key;

	/* Disable RTC counter in HBN_CTL. */
	key = irq_lock();
	tmp = sys_read32(cfg->base + HBN_CTL_OFFSET);
	tmp &= ~HBN_RTC_ENABLE;
	sys_write32(tmp, cfg->base + HBN_CTL_OFFSET);
	irq_unlock(key);

	return 0;
}

static int counter_bflb_rtc_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_bflb_rtc_config *cfg = dev->config;
	uint32_t tmp;

	unsigned int key;

	/* Latch current RTC value: set bit 31 of RTC_TIME_H, then clear it. */
	key = irq_lock();
	tmp = sys_read32(cfg->base + HBN_RTC_TIME_H_OFFSET);
	tmp |= HBN_RTC_TIME_LATCH;
	sys_write32(tmp, cfg->base + HBN_RTC_TIME_H_OFFSET);

	tmp &= ~HBN_RTC_TIME_LATCH;
	sys_write32(tmp, cfg->base + HBN_RTC_TIME_H_OFFSET);

	/* Read latched value (lower 32 bits) */
	*ticks = sys_read32(cfg->base + HBN_RTC_TIME_L_OFFSET);
	irq_unlock(key);

	return 0;
}

static uint32_t counter_bflb_rtc_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UINT32_MAX;
}

static uint32_t counter_bflb_rtc_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t counter_bflb_rtc_get_freq(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RTC_FREQ;
}

static int counter_bflb_rtc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* RTC is already running if bootloader enabled it.
	 * We don't touch it during init - start() will enable if needed.
	 */
	return 0;
}

static DEVICE_API(counter, counter_bflb_rtc_api) = {
	.start = counter_bflb_rtc_start,
	.stop = counter_bflb_rtc_stop,
	.get_value = counter_bflb_rtc_get_value,
	.get_top_value = counter_bflb_rtc_get_top_value,
	.get_pending_int = counter_bflb_rtc_get_pending_int,
	.get_freq = counter_bflb_rtc_get_freq,
};

#define COUNTER_BFLB_RTC_INIT(n)                                                                   \
	static const struct counter_bflb_rtc_config counter_bflb_rtc_config_##n = {                \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.channels = 0,                                                     \
				.freq = RTC_FREQ,                                                  \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, counter_bflb_rtc_init, NULL, NULL, &counter_bflb_rtc_config_##n,  \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, &counter_bflb_rtc_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_BFLB_RTC_INIT)
