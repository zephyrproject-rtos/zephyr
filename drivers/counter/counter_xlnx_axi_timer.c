/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_timer_1_00_a

#include <device.h>
#include <drivers/counter.h>
#include <sys/sys_io.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(xlnx_axi_timer, CONFIG_COUNTER_LOG_LEVEL);

/* AXI Timer v2.0 registers offsets (See Xilinx PG079 for details) */
#define TCSR0_OFFSET 0x00
#define TLR0_OFFSET  0x04
#define TCR0_OFFSET  0x08
#define TCSR1_OFFSET 0x10
#define TLR1_OFFSET  0x14
#define TCR1_OFFSET  0x18

/* TCSRx bit definitions */
#define TCSR_MDT   BIT(0)
#define TCSR_UDT   BIT(1)
#define TCSR_GENT  BIT(2)
#define TCSR_CAPT  BIT(3)
#define TCSR_ARHT  BIT(4)
#define TCSR_LOAD  BIT(5)
#define TCSR_ENIT  BIT(6)
#define TCSR_ENT   BIT(7)
#define TCSR_TINT  BIT(8)
#define TCSR_PWMA  BIT(9)
#define TCSR_ENALL BIT(10)
#define TCSR_CASC  BIT(11)

/* 1st timer used as main timer in auto-reload, count-down. generate mode */
#define TCSR0_DEFAULT (TCSR_ENIT | TCSR_ARHT | TCSR_GENT | TCSR_UDT)

/* 2nd timer (if available) used as alarm timer in count-down, generate mode */
#define TCSR1_DEFAULT (TCSR_ENIT | TCSR_GENT | TCSR_UDT)

struct xlnx_axi_timer_config {
	struct counter_config_info info;
	mm_reg_t base;
	void (*irq_config_func)(const struct device *dev);
};

struct xlnx_axi_timer_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

static inline uint32_t xlnx_axi_timer_read32(const struct device *dev,
					     mm_reg_t offset)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void xlnx_axi_timer_write32(const struct device *dev,
					  uint32_t value,
					  mm_reg_t offset)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static int xlnx_axi_timer_start(const struct device *dev)
{
	const struct xlnx_axi_timer_data *data = dev->data;
	uint32_t tcsr = TCSR0_DEFAULT | TCSR_ENT;

	LOG_DBG("starting timer");

	if (data->alarm_callback) {
		/* Start both timers synchronously */
		tcsr |= TCSR_ENALL;
	}

	xlnx_axi_timer_write32(dev, tcsr, TCSR0_OFFSET);

	return 0;
}

static int xlnx_axi_timer_stop(const struct device *dev)
{
	const struct xlnx_axi_timer_config *config = dev->config;
	unsigned int key;

	LOG_DBG("stopping timer");

	key = irq_lock();

	/* The timers cannot be stopped synchronously */
	if (config->info.channels > 0) {
		xlnx_axi_timer_write32(dev, TCSR1_DEFAULT, TCSR1_OFFSET);
	}
	xlnx_axi_timer_write32(dev, TCSR0_DEFAULT, TCSR0_OFFSET);

	irq_unlock(key);

	return 0;
}

static int xlnx_axi_timer_get_value(const struct device *dev, uint32_t *ticks)
{

	*ticks = xlnx_axi_timer_read32(dev, TCR0_OFFSET);

	return 0;
}

static int xlnx_axi_timer_set_alarm(const struct device *dev, uint8_t chan_id,
				    const struct counter_alarm_cfg *cfg)
{
	struct xlnx_axi_timer_data *data = dev->data;
	unsigned int key;
	uint32_t tcsr;

	ARG_UNUSED(chan_id);

	if (cfg->callback == NULL) {
		return -EINVAL;
	}

	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	if (cfg->ticks > xlnx_axi_timer_read32(dev, TLR0_OFFSET)) {
		return -EINVAL;
	}

	if (cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		/*
		 * Since two different timers (with the same clock signal) are
		 * used for main timer and alarm timer we cannot support
		 * absolute alarms in a reliable way.
		 */
		return -ENOTSUP;
	}

	LOG_DBG("triggering alarm in 0x%08x ticks", cfg->ticks);

	/* Load alarm timer */
	xlnx_axi_timer_write32(dev, cfg->ticks, TLR1_OFFSET);
	xlnx_axi_timer_write32(dev, TCSR1_DEFAULT | TCSR_LOAD, TCSR1_OFFSET);

	key = irq_lock();

	data->alarm_callback = cfg->callback;
	data->alarm_user_data = cfg->user_data;

	/* Enable alarm timer only if main timer already enabled */
	tcsr = xlnx_axi_timer_read32(dev, TCSR0_OFFSET);
	tcsr &= TCSR_ENT;
	xlnx_axi_timer_write32(dev, TCSR1_DEFAULT | tcsr, TCSR1_OFFSET);

	irq_unlock(key);

	return 0;
}

static int xlnx_axi_timer_cancel_alarm(const struct device *dev,
				       uint8_t chan_id)
{
	struct xlnx_axi_timer_data *data = dev->data;

	ARG_UNUSED(chan_id);

	LOG_DBG("cancelling alarm");

	xlnx_axi_timer_write32(dev, TCSR1_DEFAULT, TCSR1_OFFSET);
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;

	return 0;
}

static int xlnx_axi_timer_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	struct xlnx_axi_timer_data *data = dev->data;
	bool reload = true;
	uint32_t tcsr;
	uint32_t now;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	LOG_DBG("setting top value to 0x%08x", cfg->ticks);

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		reload = false;

		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			now = xlnx_axi_timer_read32(dev, TCR0_OFFSET);
			reload = cfg->ticks < now;
		}
	}

	tcsr = xlnx_axi_timer_read32(dev, TCSR0_OFFSET);
	if ((tcsr & TCSR_ENT) == 0U) {
		/* Timer not enabled, force reload of new top value */
		reload = true;
	}

	xlnx_axi_timer_write32(dev, cfg->ticks, TLR0_OFFSET);

	if (reload) {
		xlnx_axi_timer_write32(dev, tcsr | TCSR_LOAD, TCSR0_OFFSET);
		xlnx_axi_timer_write32(dev, tcsr, TCSR0_OFFSET);
	}

	return 0;
}

static uint32_t xlnx_axi_timer_get_pending_int(const struct device *dev)
{
	const struct xlnx_axi_timer_config *config = dev->config;
	uint32_t pending = 0;
	uint32_t tcsr;

	tcsr = xlnx_axi_timer_read32(dev, TCSR0_OFFSET);
	if (tcsr & TCSR_TINT) {
		pending = 1;
	}

	if (config->info.channels > 0) {
		tcsr = xlnx_axi_timer_read32(dev, TCSR1_OFFSET);
		if (tcsr & TCSR_TINT) {
			pending = 1;
		}
	}

	LOG_DBG("%sinterrupt pending", pending ? "" : "no ");

	return pending;
}

static uint32_t xlnx_axi_timer_get_top_value(const struct device *dev)
{
	return xlnx_axi_timer_read32(dev, TLR0_OFFSET);
}

static uint32_t xlnx_axi_timer_get_max_relative_alarm(const struct device *dev)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	return config->info.max_top_value;
}

static void xlnx_axi_timer_isr(const struct device *dev)
{
	struct xlnx_axi_timer_data *data = dev->data;
	counter_alarm_callback_t alarm_cb;
	uint32_t tcsr;
	uint32_t now;

	tcsr = xlnx_axi_timer_read32(dev, TCSR1_OFFSET);
	if (tcsr & TCSR_TINT) {
		xlnx_axi_timer_write32(dev, TCSR1_DEFAULT | TCSR_TINT,
				       TCSR1_OFFSET);

		if (data->alarm_callback) {
			now = xlnx_axi_timer_read32(dev, TCR0_OFFSET);
			alarm_cb = data->alarm_callback;
			data->alarm_callback = NULL;

			alarm_cb(dev, 0, now, data->alarm_user_data);
		}
	}

	tcsr = xlnx_axi_timer_read32(dev, TCSR0_OFFSET);
	if (tcsr & TCSR_TINT) {
		xlnx_axi_timer_write32(dev, tcsr, TCSR0_OFFSET);

		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}
	}
}

static int xlnx_axi_timer_init(const struct device *dev)
{
	const struct xlnx_axi_timer_config *config = dev->config;

	LOG_DBG("max top value = 0x%08x", config->info.max_top_value);
	LOG_DBG("freqency = %d", config->info.freq);
	LOG_DBG("channels = %d", config->info.channels);

	xlnx_axi_timer_write32(dev, config->info.max_top_value, TLR0_OFFSET);
	xlnx_axi_timer_write32(dev, TCSR0_DEFAULT | TCSR_LOAD, TCSR0_OFFSET);

	if (config->info.channels > 0) {
		xlnx_axi_timer_write32(dev, TCSR1_DEFAULT, TCSR1_OFFSET);
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api xlnx_axi_timer_driver_api = {
	.start = xlnx_axi_timer_start,
	.stop = xlnx_axi_timer_stop,
	.get_value = xlnx_axi_timer_get_value,
	.set_alarm = xlnx_axi_timer_set_alarm,
	.cancel_alarm = xlnx_axi_timer_cancel_alarm,
	.set_top_value = xlnx_axi_timer_set_top_value,
	.get_pending_int = xlnx_axi_timer_get_pending_int,
	.get_top_value = xlnx_axi_timer_get_top_value,
	.get_max_relative_alarm = xlnx_axi_timer_get_max_relative_alarm,
};

#define XLNX_AXI_TIMER_INIT(n)						\
	static void xlnx_axi_timer_config_func_##n(const struct device *dev); \
									\
	static struct xlnx_axi_timer_config xlnx_axi_timer_config_##n = { \
		.info = {						\
			.max_top_value =				\
			GENMASK(DT_INST_PROP(n, xlnx_count_width) - 1, 0), \
			.freq = DT_INST_PROP(n, clock_frequency),	\
			.flags = 0,					\
			.channels =					\
			COND_CODE_1(DT_INST_PROP(n, xlnx_one_timer_only), \
				    (0), (1)),				\
		},							\
		.base = DT_INST_REG_ADDR(n),				\
		.irq_config_func = xlnx_axi_timer_config_func_##n,	\
	};								\
									\
	static struct xlnx_axi_timer_data xlnx_axi_timer_data_##n;	\
									\
	DEVICE_DT_INST_DEFINE(n, &xlnx_axi_timer_init,			\
			device_pm_control_nop,				\
			&xlnx_axi_timer_data_##n,			\
			&xlnx_axi_timer_config_##n,			\
			POST_KERNEL,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&xlnx_axi_timer_driver_api);			\
									\
	static void xlnx_axi_timer_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    xlnx_axi_timer_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(XLNX_AXI_TIMER_INIT)
