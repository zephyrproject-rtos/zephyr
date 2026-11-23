/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tstmr

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>

#include <fsl_clock.h>

#if defined(CONFIG_COUNTER_MCUX_TSTMR_SYSCTR_BACKEND)
#include <fsl_sysctr.h>
#endif

#define TSTMR_MAX_TICKS     ((1ULL << 56) - 1ULL)

#define TSTMR_LOW_OFFSET    0x0U
#define TSTMR_HIGH_OFFSET   0x4U
#define TSTMR_HIGH_MASK     0x00FFFFFFU

struct mcux_tstmr_config {
	struct counter_config_info info;
	mem_addr_t base;
	/* Clock feeding the underlying SYS_CTR. NULL when no 'clocks' phandle. */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct mcux_tstmr_data {
	uint32_t freq;
};

static uint64_t mcux_tstmr_read_timestamp(mem_addr_t base)
{
	unsigned int key;
	uint32_t low_first;
	uint32_t high_first;
	uint32_t low_second;
	uint32_t high_second;

	key = irq_lock();

	low_first = sys_read32(base + TSTMR_LOW_OFFSET);
	barrier_dmem_fence_full();
	high_first = sys_read32(base + TSTMR_HIGH_OFFSET) & TSTMR_HIGH_MASK;
	barrier_dmem_fence_full();
	low_second = sys_read32(base + TSTMR_LOW_OFFSET);
	barrier_dmem_fence_full();
	high_second = sys_read32(base + TSTMR_HIGH_OFFSET) & TSTMR_HIGH_MASK;

	irq_unlock(key);

	if (low_second < low_first) {
		return ((uint64_t)high_second << 32) | low_second;
	}

	return ((uint64_t)high_first << 32) | low_first;
}

static int mcux_tstmr_start(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_COUNTER_MCUX_TSTMR_SYSCTR_BACKEND)
	if ((SYS_CTR_CONTROL->CNTCR & SYS_CTR_CONTROL_CNTCR_EN_MASK) == 0U) {
		SYS_CTR_CONTROL->CNTCR |= SYS_CTR_CONTROL_CNTCR_EN_MASK;
	}
#endif /* CONFIG_COUNTER_MCUX_TSTMR_SYSCTR_BACKEND */

	return 0;
}

static int mcux_tstmr_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int mcux_tstmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_tstmr_config *config = dev->config;

	*ticks = (uint32_t)mcux_tstmr_read_timestamp(config->base);

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int mcux_tstmr_get_value_64(const struct device *dev, uint64_t *ticks)
{
	const struct mcux_tstmr_config *config = dev->config;

	*ticks = mcux_tstmr_read_timestamp(config->base);

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int mcux_tstmr_set_alarm(const struct device *dev, uint8_t chan_id,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);

	return -ENOTSUP;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int mcux_tstmr_set_alarm_64(const struct device *dev, uint8_t chan_id,
					  const struct counter_alarm_cfg_64 *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);

	return -ENOTSUP;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int mcux_tstmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);

	return -ENOTSUP;
}

static int mcux_tstmr_set_top_value(const struct device *dev,
				    const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int mcux_tstmr_set_top_value_64(const struct device *dev,
				       const struct counter_top_cfg_64 *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static uint32_t mcux_tstmr_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t mcux_tstmr_get_top_value(const struct device *dev)
{
	const struct mcux_tstmr_config *config = dev->config;

	return config->info.max_top_value;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static uint64_t mcux_tstmr_get_top_value_64(const struct device *dev)
{
	ARG_UNUSED(dev);

	return TSTMR_MAX_TICKS;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static uint32_t mcux_tstmr_get_freq(const struct device *dev)
{
	const struct mcux_tstmr_data *data = dev->data;

#if defined(CONFIG_COUNTER_MCUX_TSTMR_SYSCTR_BACKEND)
		if ((SYS_CTR_CONTROL->CNTCR & SYS_CTR_CONTROL_CNTCR_EN_MASK) == 0U) {
			return 0U;
		}
#endif /* CONFIG_COUNTER_MCUX_TSTMR_SYSCTR_BACKEND */

	return data->freq;
}

static int mcux_tstmr_init(const struct device *dev)
{
	const struct mcux_tstmr_config *config = dev->config;
	struct mcux_tstmr_data *data = dev->data;

	if (config->clock_dev != NULL) {
		uint32_t rate;
		int ret;

		if (!device_is_ready(config->clock_dev)) {
			return -ENODEV;
		}

		/*
		 * Enable the SYS_CTR clock gate once, here, so counter_start()
		 * only sets the count-enable and never toggles the gate (a gate
		 * toggle re-syncs the clock domains and slows the first reads).
		 */
		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret != 0) {
			return ret;
		}

		ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &rate);
		if (ret != 0) {
			return ret;
		}

		data->freq = rate;
	} else {
		/* No 'clocks' phandle: rate comes from the clock-frequency prop. */
		data->freq = config->info.freq;
	}

	if (data->freq == 0U) {
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(counter, mcux_tstmr_driver_api) = {
	.start = mcux_tstmr_start,
	.stop = mcux_tstmr_stop,
	.get_value = mcux_tstmr_get_value,
	.set_alarm = mcux_tstmr_set_alarm,
	.cancel_alarm = mcux_tstmr_cancel_alarm,
	.set_top_value = mcux_tstmr_set_top_value,
	.get_pending_int = mcux_tstmr_get_pending_int,
	.get_top_value = mcux_tstmr_get_top_value,
	.get_freq = mcux_tstmr_get_freq,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.get_value_64 = mcux_tstmr_get_value_64,
	.set_alarm_64 = mcux_tstmr_set_alarm_64,
	.get_top_value_64 = mcux_tstmr_get_top_value_64,
	.set_top_value_64 = mcux_tstmr_set_top_value_64,
#endif /* CONFIG_COUNTER_64BITS_TICKS */
};

#define MCUX_TSTMR_CLOCK_DEV(n)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),                                       \
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL))

#define MCUX_TSTMR_CLOCK_SUBSYS(n)                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),                                       \
		    ((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)), (NULL))

#define MCUX_TSTMR_DEVICE(n)                                                               \
	static struct mcux_tstmr_data mcux_tstmr_data_##n;                                  \
	static const struct mcux_tstmr_config mcux_tstmr_config_##n = {                      \
		.info = {                                                                      \
			.freq = DT_INST_PROP_OR(n, clock_frequency, 0),                         \
			IF_ENABLED(CONFIG_COUNTER_64BITS_TICKS,                                 \
				(.max_top_value_64 = TSTMR_MAX_TICKS,))                           \
			IF_DISABLED(CONFIG_COUNTER_64BITS_TICKS,                                \
				(.max_top_value = UINT32_MAX,))                                    \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,                                  \
			.channels = 0,                                                           \
		},                                                                              \
		.base = DT_INST_REG_ADDR(n),                                                    \
		.clock_dev = MCUX_TSTMR_CLOCK_DEV(n),                                           \
		.clock_subsys = MCUX_TSTMR_CLOCK_SUBSYS(n),                                     \
	};                                                                                 \
	DEVICE_DT_INST_DEFINE(n, mcux_tstmr_init, NULL, &mcux_tstmr_data_##n,               \
			      &mcux_tstmr_config_##n, POST_KERNEL,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &mcux_tstmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_TSTMR_DEVICE)
