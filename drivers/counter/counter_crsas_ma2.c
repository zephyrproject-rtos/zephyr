/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_crsas_ma2_counter

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(crsas_ma2_counter, CONFIG_COUNTER_LOG_LEVEL);

/* Register Offsets within CNTCTLBase (Control Frame) */
#define CNTCR_OFFSET    0x000 /* Counter Control Register*/
#define CNTCV_LO_OFFSET 0x008 /* Counter Value Low 32-bits*/
#define CNTCV_HI_OFFSET 0x00C /* Counter Value High 32-bits (0x008 + 4)*/
#define CNTSCR_OFFSET   0x010 /* Counter Scaling Register*/

/* CNTCR Bitfields */
#define CNTCR_EN   0 /* Enable bit */
#define CNTCR_HDBG 1 /* Halt-on-debug bit */

#define SCALING_SHIFT 24

struct counter_crsas_ma2_config {
	struct counter_config_info info;
	uintptr_t base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t scaling_factor; /* 8.24 fixed point */
	bool halt_on_debug;
};

struct counter_crsas_ma2_data {
	uint32_t freq; /* this is scaled */
};

static int counter_crsas_ma2_start(const struct device *dev)
{
	const struct counter_crsas_ma2_config *cfg = dev->config;

	sys_set_bit(cfg->base + CNTCR_OFFSET, CNTCR_EN);
	return 0;
}

static int counter_crsas_ma2_stop(const struct device *dev)
{
#if defined(CONFIG_COUNTER_CRSAS_MA2_TIMER)
	LOG_WRN_ONCE("ARM CRSAS-MA2 System Counter can't be stopped while System Timer "
		     "is enabled");
	return -ENOTSUP;
#else
	const struct counter_crsas_ma2_config *cfg = dev->config;

	sys_clear_bit(cfg->base + CNTCR_OFFSET, CNTCR_EN);
	return 0;
#endif
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int counter_crsas_ma2_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct counter_crsas_ma2_config *cfg = dev->config;
	uint32_t hi, lo, temp;

	hi = sys_read32(cfg->base + CNTCV_HI_OFFSET);
	do {
		temp = hi;
		lo = sys_read32(cfg->base + CNTCV_LO_OFFSET);
		hi = sys_read32(cfg->base + CNTCV_HI_OFFSET);
	} while (hi != temp);

	*ticks = ((uint64_t)hi << 32) | lo;
	return 0;
}
#endif

static int counter_crsas_ma2_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_crsas_ma2_config *cfg = dev->config;

	/* just return the lo register value */
	*ticks = sys_read32(cfg->base + CNTCV_LO_OFFSET);
	return 0;
}

static uint32_t counter_crsas_ma2_get_freq(const struct device *dev)
{
	struct counter_crsas_ma2_data *data = dev->data;

	return data->freq;
}

static int counter_crsas_ma2_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static uint32_t counter_crsas_ma2_get_top_value(const struct device *dev)
{
	const struct counter_crsas_ma2_config *config = dev->config;

	return config->info.max_top_value;
}

static int counter_crsas_ma2_set_alarm(const struct device *dev, uint8_t chan_id,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	return -ENOTSUP;
}

static int counter_crsas_ma2_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	return -ENOTSUP;
}

static uint32_t counter_crsas_ma2_get_pending_int(const struct device *dev)
{
	return 0;
}

static int counter_crsas_ma2_init(const struct device *dev)
{
	const struct counter_crsas_ma2_config *cfg = dev->config;
	struct counter_crsas_ma2_data *data = dev->data;
	uint32_t clk_freq;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	/* The counter must be disabled when scaling factor is being written */
	sys_write32(0, cfg->base + CNTCR_OFFSET);

	sys_write32(cfg->scaling_factor, cfg->base + CNTSCR_OFFSET);

	err = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &clk_freq);
	if (err < 0) {
		return err;
	}

	/* Scaled frequency = (BaseClock * ScalingFactor) >> 24 */
	data->freq = (uint32_t)(((uint64_t)clk_freq * cfg->scaling_factor) >> SCALING_SHIFT);

	/* Enable. We presume if this device is enabled in the device tree,
	 * it's because some peripheral like ARM CRSAS-MA2 System Timer or Watchdog is desired,
	 * so we want to start the counter.
	 */
	sys_write32(BIT(CNTCR_EN), cfg->base + CNTCR_OFFSET);

	if (cfg->halt_on_debug) {
		sys_set_bit(cfg->base + CNTCR_OFFSET, CNTCR_HDBG);
	}

	return err;
}

static DEVICE_API(counter, counter_crsas_ma2_api) = {
	.start = counter_crsas_ma2_start,
	.stop = counter_crsas_ma2_stop,
	.get_value = counter_crsas_ma2_get_value,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = counter_crsas_ma2_get_value_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
	.set_alarm = counter_crsas_ma2_set_alarm,
	.cancel_alarm = counter_crsas_ma2_cancel_alarm,
	.set_top_value = counter_crsas_ma2_set_top_value,
	.get_pending_int = counter_crsas_ma2_get_pending_int,
	.get_top_value = counter_crsas_ma2_get_top_value,
	.get_freq = counter_crsas_ma2_get_freq,
};

#define COUNTER_CRSAS_MA2_DEVICE(n)                                                                \
	static struct counter_crsas_ma2_data counter_crsas_ma2_data_##n;                           \
	static const struct counter_crsas_ma2_config counter_crsas_ma2_cfg_##n = {                 \
		.info =                                                                            \
			{                                                                          \
				/* API limits this to 32 bits, though HW has a 64-bit counter */   \
				.max_top_value = UINT32_MAX,                                       \
				.freq = 0, /* not used, we implement the get_freq API */           \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 0, /* no alarms supported */                           \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clkid),             \
		.scaling_factor = DT_INST_PROP(n, scaling_factor),                                 \
		.halt_on_debug = DT_INST_PROP(n, halt_on_debug),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, counter_crsas_ma2_init, NULL, &counter_crsas_ma2_data_##n,        \
			      &counter_crsas_ma2_cfg_##n, POST_KERNEL,                             \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_crsas_ma2_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_CRSAS_MA2_DEVICE)
