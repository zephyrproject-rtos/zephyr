/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_zynqmp_rtc

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_xlnx_zynqmp_rtc, CONFIG_COUNTER_LOG_LEVEL);

#define XLNX_RTC_SET_TIME_WR 0x00
#define XLNX_RTC_SET_TIME_RD 0x04
#define XLNX_RTC_CALIB_WR    0x08
#define XLNX_RTC_CALIB_RD    0x0C
#define XLNX_RTC_CUR_TIME    0x10
#define XLNX_RTC_ALRM        0x18
#define XLNX_RTC_INT_STS     0x20
#define XLNX_RTC_INT_EN      0x28
#define XLNX_RTC_INT_DIS     0x2C
#define XLNX_RTC_CTL         0x40

#define XLNX_RTC_INT_SECS BIT(0)
#define XLNX_RTC_INT_ALRM BIT(1)

#define XLNX_RTC_OSC_EN BIT(24)
/* Setting this bit enables the RTC; clearing it disables the RTC */
#define XLNX_RTC_EN     BIT(31)

#define XLNX_RTC_MAX_TCK_MASK      0xFFFFU
#define XLNX_RTC_FRACTN_EN         BIT(20)
#define XLNX_RTC_FRACTN_DATA_MASK  0xF0000U
#define XLNX_RTC_FRACTN_DATA_SHIFT 16
#define XLNX_RTC_FRACTN_MAX_TICKS  16

#define XLNX_RTC_PPB 1000000000LL

struct xlnx_rtc_config {
	struct counter_config_info counter_info;

	DEVICE_MMIO_NAMED_ROM(mmio);

	uint32_t clock_frequency;
	void (*irq_config)(void);
};

struct xlnx_rtc_data {
	DEVICE_MMIO_NAMED_RAM(mmio);

	counter_alarm_callback_t alarm_cb;
	void *alarm_user_data;
	uint16_t max_tick;
};

#define DEV_CFG(dev)  ((const struct xlnx_rtc_config *)(dev)->config)
#define DEV_DATA(dev) ((struct xlnx_rtc_data *)(dev)->data)

static inline uint32_t xlnx_rtc_read(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, mmio) + offset);
}

static inline void xlnx_rtc_write(const struct device *dev, uint32_t offset, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_NAMED_GET(dev, mmio) + offset);
}

static int xlnx_rtc_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -ENOTSUP;
}

static int xlnx_rtc_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -ENOTSUP;
}

static int xlnx_rtc_get_value(const struct device *dev, uint32_t *ticks)
{
	uint32_t status;

	status = xlnx_rtc_read(dev, XLNX_RTC_INT_STS);

	if (status & XLNX_RTC_INT_SECS) {
		*ticks = xlnx_rtc_read(dev, XLNX_RTC_CUR_TIME);
	} else {
		*ticks = xlnx_rtc_read(dev, XLNX_RTC_SET_TIME_RD) - 1;
	}

	return 0;
}

static int xlnx_rtc_set_value(const struct device *dev, uint32_t ticks)
{
	/*
	 * Hardware latches the written value into CUR_TIME after ~1 second.
	 * Write ticks + 1 so that CUR_TIME reads the intended value once
	 * the seconds counter advances.
	 */
	xlnx_rtc_write(dev, XLNX_RTC_SET_TIME_WR, ticks + 1);
	xlnx_rtc_write(dev, XLNX_RTC_INT_STS, XLNX_RTC_INT_SECS);

	return 0;
}

static int xlnx_rtc_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct xlnx_rtc_config *cfg = dev->config;
	struct xlnx_rtc_data *data = dev->data;
	uint32_t alarm_ticks;
	uint32_t now;

	if (chan_id != 0) {
		return -EINVAL;
	}

	if (data->alarm_cb != NULL) {
		return -EBUSY;
	}

	if (alarm_cfg->ticks > cfg->counter_info.max_top_value) {
		return -EINVAL;
	}

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		alarm_ticks = alarm_cfg->ticks;
	} else {
		xlnx_rtc_get_value(dev, &now);
		alarm_ticks = now + alarm_cfg->ticks;
	}

	xlnx_rtc_get_value(dev, &now);
	if (alarm_ticks <= now) {
		return -ETIME;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	xlnx_rtc_write(dev, XLNX_RTC_ALRM, alarm_ticks);

	/* Clear any stale alarm status before enabling */
	xlnx_rtc_write(dev, XLNX_RTC_INT_STS, XLNX_RTC_INT_ALRM);
	xlnx_rtc_write(dev, XLNX_RTC_INT_EN, XLNX_RTC_INT_ALRM);

	return 0;
}

static int xlnx_rtc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct xlnx_rtc_data *data = dev->data;

	if (chan_id != 0) {
		return -EINVAL;
	}

	xlnx_rtc_write(dev, XLNX_RTC_INT_DIS, XLNX_RTC_INT_ALRM);
	data->alarm_cb = NULL;
	data->alarm_user_data = NULL;

	return 0;
}

static uint32_t xlnx_rtc_get_top_value(const struct device *dev)
{
	const struct xlnx_rtc_config *cfg = dev->config;

	return cfg->counter_info.max_top_value;
}

static int xlnx_rtc_set_top_value(const struct device *dev, const struct counter_top_cfg *top_cfg)
{
	const struct xlnx_rtc_config *cfg = dev->config;

	if (top_cfg->ticks != cfg->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t xlnx_rtc_get_pending_int(const struct device *dev)
{
	return (xlnx_rtc_read(dev, XLNX_RTC_INT_STS) & (XLNX_RTC_INT_SECS | XLNX_RTC_INT_ALRM)) !=
	       0;
}

#if defined(CONFIG_COUNTER_CALIBRATION)
static int xlnx_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	struct xlnx_rtc_data *data = dev->data;
	int32_t fract_data = 0;
	int32_t fract_offset;
	int32_t tick_mult;
	uint32_t calibval;
	int32_t max_tick;
	int32_t freq;

	freq = data->max_tick;
	tick_mult = DIV_ROUND_CLOSEST(XLNX_RTC_PPB, freq);

	max_tick = calibration / tick_mult;
	fract_offset = calibration - (max_tick * tick_mult);

	if (((int32_t)freq + max_tick) > (int32_t)XLNX_RTC_MAX_TCK_MASK ||
	    ((int32_t)freq + max_tick) < 1) {
		return -EINVAL;
	}

	if (fract_offset) {
		int32_t fract_part = DIV_ROUND_UP(tick_mult, XLNX_RTC_FRACTN_MAX_TICKS);

		fract_data = fract_offset / fract_part;
		if (fract_offset < 0 && fract_data) {
			max_tick--;
			fract_data += XLNX_RTC_FRACTN_MAX_TICKS;
		}
	}

	calibval = (uint32_t)(max_tick + freq);

	if (fract_data) {
		calibval |=
			XLNX_RTC_FRACTN_EN | ((uint32_t)fract_data << XLNX_RTC_FRACTN_DATA_SHIFT);
	}

	xlnx_rtc_write(dev, XLNX_RTC_CALIB_WR, calibval);

	return 0;
}

static int xlnx_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	struct xlnx_rtc_data *data = dev->data;
	int32_t offset_val;
	uint32_t calibval;
	int32_t tick_mult;
	int32_t max_tick;
	int32_t freq;

	freq = data->max_tick;
	tick_mult = DIV_ROUND_CLOSEST(XLNX_RTC_PPB, freq);

	calibval = xlnx_rtc_read(dev, XLNX_RTC_CALIB_RD);
	max_tick = calibval & XLNX_RTC_MAX_TCK_MASK;
	offset_val = (max_tick - freq) * tick_mult;

	if (calibval & XLNX_RTC_FRACTN_EN) {
		uint32_t fract_data =
			(calibval & XLNX_RTC_FRACTN_DATA_MASK) >> XLNX_RTC_FRACTN_DATA_SHIFT;
		uint32_t fract_part = DIV_ROUND_UP(tick_mult, XLNX_RTC_FRACTN_MAX_TICKS);

		offset_val += (int32_t)(fract_part * fract_data);
	}

	*calibration = offset_val;

	return 0;
}
#endif /* CONFIG_COUNTER_CALIBRATION */

static void xlnx_rtc_alarm_isr(const struct device *dev)
{
	struct xlnx_rtc_data *data = dev->data;
	counter_alarm_callback_t cb;
	uint32_t ticks;

	xlnx_rtc_write(dev, XLNX_RTC_INT_DIS, XLNX_RTC_INT_ALRM);
	xlnx_rtc_write(dev, XLNX_RTC_INT_STS, XLNX_RTC_INT_ALRM);

	cb = data->alarm_cb;
	data->alarm_cb = NULL;

	if (cb) {
		xlnx_rtc_get_value(dev, &ticks);
		cb(dev, 0, ticks, data->alarm_user_data);
	}
}

static int xlnx_rtc_init(const struct device *dev)
{
	const struct xlnx_rtc_config *cfg = dev->config;
	struct xlnx_rtc_data *data = dev->data;
	uint32_t ctrl;
	uint32_t freq;

	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);

	freq = cfg->clock_frequency - 1;
	if (freq > XLNX_RTC_MAX_TCK_MASK) {
		LOG_ERR("Invalid clock frequency: %u", cfg->clock_frequency);
		return -EINVAL;
	}
	data->max_tick = (uint16_t)freq;

	/* Seed calibration register if it has not been programmed */
	if (xlnx_rtc_read(dev, XLNX_RTC_CALIB_RD) == 0) {
		xlnx_rtc_write(dev, XLNX_RTC_CALIB_WR, freq);
	}

	/* Enable crystal oscillator and RTC counter */
	ctrl = xlnx_rtc_read(dev, XLNX_RTC_CTL);
	ctrl |= XLNX_RTC_OSC_EN | XLNX_RTC_EN;
	xlnx_rtc_write(dev, XLNX_RTC_CTL, ctrl);

	/* Clear pending interrupt status */
	xlnx_rtc_write(dev, XLNX_RTC_INT_STS, XLNX_RTC_INT_SECS | XLNX_RTC_INT_ALRM);

	/* Disable all interrupts */
	xlnx_rtc_write(dev, XLNX_RTC_INT_DIS, XLNX_RTC_INT_SECS | XLNX_RTC_INT_ALRM);

	cfg->irq_config();

	return 0;
}

static DEVICE_API(counter, xlnx_rtc_api) = {
	.start = xlnx_rtc_start,
	.stop = xlnx_rtc_stop,
	.get_value = xlnx_rtc_get_value,
	.set_value = xlnx_rtc_set_value,
	.set_alarm = xlnx_rtc_set_alarm,
	.cancel_alarm = xlnx_rtc_cancel_alarm,
	.set_top_value = xlnx_rtc_set_top_value,
	.get_pending_int = xlnx_rtc_get_pending_int,
	.get_top_value = xlnx_rtc_get_top_value,
#if defined(CONFIG_COUNTER_CALIBRATION)
	.set_calibration = xlnx_rtc_set_calibration,
	.get_calibration = xlnx_rtc_get_calibration,
#endif
};

#define XLNX_RTC_INIT(inst)                                                                        \
	static void xlnx_rtc_irq_config_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, alarm, irq),                                 \
			    DT_INST_IRQ_BY_NAME(inst, alarm, priority), xlnx_rtc_alarm_isr,        \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, alarm, irq));                                 \
	}                                                                                          \
	static struct xlnx_rtc_data xlnx_rtc_data_##inst;                                          \
	static const struct xlnx_rtc_config xlnx_rtc_config_##inst = {                             \
		.counter_info =                                                                    \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 1,                                                         \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(inst)),                               \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
		.irq_config = xlnx_rtc_irq_config_##inst,                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, xlnx_rtc_init, NULL, &xlnx_rtc_data_##inst,                    \
			      &xlnx_rtc_config_##inst, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,  \
			      &xlnx_rtc_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_RTC_INIT)
