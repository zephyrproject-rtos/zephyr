/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_timer

#include <errno.h>
#include <ip_identification.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_timer, CONFIG_COUNTER_LOG_LEVEL);

#define TIMER_AESC_INFO		0x00
#define TIMER_AESC_IRQ_PENDING	0x04
#define TIMER_AESC_IRQ_MASK	0x08
#define TIMER_AESC_CONTROL	0x0c
#define TIMER_AESC_PRESCALER	0x10
#define TIMER_AESC_COUNTER	0x14
#define TIMER_AESC_RELOAD	0x18
#define TIMER_AESC_COMPARE	0x1c

#define TIMER_AESC_INFO_COUNT		GENMASK(7, 0)
#define TIMER_AESC_INFO_CHANNELS	GENMASK(15, 8)
#define TIMER_AESC_INFO_WIDTH		GENMASK(23, 16)
#define TIMER_AESC_INFO_PRESCALER	GENMASK(31, 24)

#define TIMER_AESC_CONTROL_ENABLE	BIT(0)
#define TIMER_AESC_CONTROL_MODE		GENMASK(2, 1)
#define TIMER_AESC_MODE_PERIODIC	1U

#define TIMER_AESC_IRQ_OVERFLOW		BIT(0)
#define TIMER_AESC_IRQ_COMPARE		BIT(1)

#define TIMER_AESC_ALARM_CHANNELS	1U

struct counter_aesc_config {
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t clock_frequency;
	uint32_t resolution;
	uint32_t prescaler;
	void (*irq_config)(const struct device *dev);
};

struct counter_aesc_data {
	DEVICE_MMIO_NAMED_RAM(mmio);

	uintptr_t reg_base;
	uint32_t freq;
	struct k_spinlock lock;
	counter_top_callback_t top_cb;
	void *top_user_data;
	counter_alarm_callback_t alarm_cb;
	void *alarm_user_data;
};

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) ((const struct counter_aesc_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct counter_aesc_data *)(_dev)->data)

static void counter_aesc_set_mask(struct counter_aesc_data *data, uint32_t bits,
				  bool enable)
{
	uint32_t mask = sys_read32(data->reg_base + TIMER_AESC_IRQ_MASK);

	if (enable) {
		/* Pending bits latch while masked; drop stale events first. */
		sys_write32(bits, data->reg_base + TIMER_AESC_IRQ_PENDING);
		mask |= bits;
	} else {
		mask &= ~bits;
	}
	sys_write32(mask, data->reg_base + TIMER_AESC_IRQ_MASK);
}

/* The counter register only accepts writes while the timer is stopped. */
static void counter_aesc_reset_counter(struct counter_aesc_data *data)
{
	uint32_t control = sys_read32(data->reg_base + TIMER_AESC_CONTROL);

	sys_write32(control & ~TIMER_AESC_CONTROL_ENABLE,
		    data->reg_base + TIMER_AESC_CONTROL);
	sys_write32(0, data->reg_base + TIMER_AESC_COUNTER);
	sys_write32(control, data->reg_base + TIMER_AESC_CONTROL);
}

static int counter_aesc_start(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;

	sys_write32(TIMER_AESC_CONTROL_ENABLE |
		    FIELD_PREP(TIMER_AESC_CONTROL_MODE, TIMER_AESC_MODE_PERIODIC),
		    data->reg_base + TIMER_AESC_CONTROL);

	return 0;
}

static int counter_aesc_stop(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;

	sys_write32(FIELD_PREP(TIMER_AESC_CONTROL_MODE, TIMER_AESC_MODE_PERIODIC),
		    data->reg_base + TIMER_AESC_CONTROL);

	return 0;
}

static int counter_aesc_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_aesc_data *data = dev->data;

	*ticks = sys_read32(data->reg_base + TIMER_AESC_COUNTER);

	return 0;
}

static uint32_t counter_aesc_get_top_value(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;

	return sys_read32(data->reg_base + TIMER_AESC_RELOAD);
}

static int counter_aesc_set_alarm(const struct device *dev, uint8_t chan_id,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_aesc_data *data = dev->data;
	uint32_t top = counter_aesc_get_top_value(dev);
	uint32_t target;
	k_spinlock_key_t key;
	int ret = 0;

	ARG_UNUSED(chan_id);

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_cb != NULL) {
		ret = -EBUSY;
		goto unlock;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U) {
		target = alarm_cfg->ticks;
	} else {
		target = sys_read32(data->reg_base + TIMER_AESC_COUNTER);
		target = (target > top - alarm_cfg->ticks) ?
			 target - (top - alarm_cfg->ticks) - 1U :
			 target + alarm_cfg->ticks;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	sys_write32(target, data->reg_base + TIMER_AESC_COMPARE);
	counter_aesc_set_mask(data, TIMER_AESC_IRQ_COMPARE, true);

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int counter_aesc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct counter_aesc_data *data = dev->data;
	k_spinlock_key_t key;

	ARG_UNUSED(chan_id);

	key = k_spin_lock(&data->lock);
	counter_aesc_set_mask(data, TIMER_AESC_IRQ_COMPARE, false);
	data->alarm_cb = NULL;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int counter_aesc_set_top_value(const struct device *dev,
				      const struct counter_top_cfg *cfg)
{
	const struct counter_aesc_config *config = dev->config;
	struct counter_aesc_data *data = dev->data;
	k_spinlock_key_t key;
	int ret = 0;

	if (cfg->ticks > config->info.max_top_value) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (data->alarm_cb != NULL) {
		ret = -EBUSY;
		goto unlock;
	}

	counter_aesc_set_mask(data, TIMER_AESC_IRQ_OVERFLOW, false);
	data->top_cb = cfg->callback;
	data->top_user_data = cfg->user_data;

	sys_write32(cfg->ticks, data->reg_base + TIMER_AESC_RELOAD);

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
		counter_aesc_reset_counter(data);
	} else if (sys_read32(data->reg_base + TIMER_AESC_COUNTER) > cfg->ticks) {
		ret = -ETIME;
		if ((cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) != 0U) {
			counter_aesc_reset_counter(data);
		}
	}

	if (data->top_cb != NULL) {
		counter_aesc_set_mask(data, TIMER_AESC_IRQ_OVERFLOW, true);
	}

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static uint32_t counter_aesc_get_pending_int(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;
	uint32_t pending = sys_read32(data->reg_base + TIMER_AESC_IRQ_PENDING);

	pending &= sys_read32(data->reg_base + TIMER_AESC_IRQ_MASK);

	return (pending != 0U) ? 1U : 0U;
}

static uint32_t counter_aesc_get_freq(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;

	return data->freq;
}

static void counter_aesc_isr(const struct device *dev)
{
	struct counter_aesc_data *data = dev->data;
	counter_alarm_callback_t alarm_cb = NULL;
	void *alarm_user_data = NULL;
	k_spinlock_key_t key;
	uint32_t pending;
	uint32_t ticks;

	key = k_spin_lock(&data->lock);

	pending = sys_read32(data->reg_base + TIMER_AESC_IRQ_PENDING) &
		  sys_read32(data->reg_base + TIMER_AESC_IRQ_MASK);
	sys_write32(pending, data->reg_base + TIMER_AESC_IRQ_PENDING);

	if ((pending & TIMER_AESC_IRQ_COMPARE) != 0U) {
		counter_aesc_set_mask(data, TIMER_AESC_IRQ_COMPARE, false);
		alarm_cb = data->alarm_cb;
		alarm_user_data = data->alarm_user_data;
		data->alarm_cb = NULL;
	}

	k_spin_unlock(&data->lock, key);

	if (((pending & TIMER_AESC_IRQ_OVERFLOW) != 0U) && (data->top_cb != NULL)) {
		data->top_cb(dev, data->top_user_data);
	}

	if (alarm_cb != NULL) {
		ticks = sys_read32(data->reg_base + TIMER_AESC_COUNTER);
		alarm_cb(dev, 0, ticks, alarm_user_data);
	}
}

static int counter_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, mmio, K_MEM_CACHE_NONE);
	volatile uintptr_t *base_addr = (volatile uintptr_t *)DEVICE_MMIO_NAMED_GET(dev, mmio);
	const struct counter_aesc_config *config = dev->config;
	struct counter_aesc_data *data = dev->data;
	uint32_t clk_freq;
	uint32_t info;
	int ret;

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", data->reg_base);

	info = sys_read32(data->reg_base + TIMER_AESC_INFO);
	if (FIELD_GET(TIMER_AESC_INFO_WIDTH, info) != config->resolution) {
		LOG_ERR("resolution %u does not match counter width %lu",
			config->resolution, FIELD_GET(TIMER_AESC_INFO_WIDTH, info));
		return -EINVAL;
	}
	if ((FIELD_GET(TIMER_AESC_INFO_PRESCALER, info) < 32U) &&
	    (config->prescaler >= BIT(FIELD_GET(TIMER_AESC_INFO_PRESCALER, info)))) {
		LOG_ERR("prescaler %u exceeds %lu bits", config->prescaler,
			FIELD_GET(TIMER_AESC_INFO_PRESCALER, info));
		return -EINVAL;
	}

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("clock controller not ready");
			return -ENODEV;
		}
		ret = clock_control_get_rate(config->clock_dev, config->clock_subsys,
					     &clk_freq);
		if (ret < 0) {
			LOG_ERR("failed to get clock rate");
			return ret;
		}
	} else {
		clk_freq = config->clock_frequency;
	}

	if (clk_freq == 0U) {
		LOG_ERR("no input clock frequency");
		return -EINVAL;
	}
	data->freq = clk_freq / (config->prescaler + 1U);

	sys_write32(0, data->reg_base + TIMER_AESC_IRQ_MASK);
	counter_aesc_stop(dev);
	sys_write32(config->prescaler, data->reg_base + TIMER_AESC_PRESCALER);
	sys_write32(config->info.max_top_value, data->reg_base + TIMER_AESC_RELOAD);
	sys_write32(0, data->reg_base + TIMER_AESC_COUNTER);
	sys_write32(TIMER_AESC_IRQ_OVERFLOW | TIMER_AESC_IRQ_COMPARE,
		    data->reg_base + TIMER_AESC_IRQ_PENDING);

	config->irq_config(dev);

	return 0;
}

static DEVICE_API(counter, counter_aesc_driver_api) = {
	.start = counter_aesc_start,
	.stop = counter_aesc_stop,
	.get_value = counter_aesc_get_value,
	.set_alarm = counter_aesc_set_alarm,
	.cancel_alarm = counter_aesc_cancel_alarm,
	.set_top_value = counter_aesc_set_top_value,
	.get_pending_int = counter_aesc_get_pending_int,
	.get_top_value = counter_aesc_get_top_value,
	.get_freq = counter_aesc_get_freq,
};

#define AESC_TIMER_CLOCK_CFG(no)					      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(no, clocks),			      \
		(.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(no)),	      \
		 .clock_subsys = (clock_control_subsys_t)		      \
			DT_INST_CLOCKS_CELL(no, domain),),		      \
		(.clock_frequency = DT_INST_PROP_OR(no, clock_frequency, 0),))

#define AESC_TIMER_INIT(no)						      \
	BUILD_ASSERT(DT_INST_PROP(no, resolution) >= 1 &&		      \
		     DT_INST_PROP(no, resolution) <= 32,		      \
		     "resolution must be 1..32");			      \
	static void counter_aesc_irq_config_##no(const struct device *dev)    \
	{								      \
		IRQ_CONNECT(DT_INST_IRQN(no),				      \
			    DT_INST_IRQ(no, priority),			      \
			    counter_aesc_isr,				      \
			    DEVICE_DT_INST_GET(no),			      \
			    0);						      \
		irq_enable(DT_INST_IRQN(no));				      \
	}								      \
	static struct counter_aesc_data counter_aesc_dev_data_##no;	      \
	static const struct counter_aesc_config counter_aesc_dev_cfg_##no = { \
		.info = {						      \
			.max_top_value = (uint32_t)BIT64_MASK(		      \
				DT_INST_PROP(no, resolution)),		      \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		      \
			.channels = TIMER_AESC_ALARM_CHANNELS,		      \
		},							      \
		DEVICE_MMIO_NAMED_ROM_INIT(mmio, DT_DRV_INST(no)),	      \
		AESC_TIMER_CLOCK_CFG(no)				      \
		.resolution = DT_INST_PROP(no, resolution),		      \
		.prescaler = DT_INST_PROP(no, prescaler),		      \
		.irq_config = counter_aesc_irq_config_##no,		      \
	};								      \
	DEVICE_DT_INST_DEFINE(no,					      \
			      counter_aesc_init,			      \
			      NULL,					      \
			      &counter_aesc_dev_data_##no,		      \
			      &counter_aesc_dev_cfg_##no,		      \
			      PRE_KERNEL_1,				      \
			      CONFIG_COUNTER_INIT_PRIORITY,		      \
			      &counter_aesc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AESC_TIMER_INIT)
