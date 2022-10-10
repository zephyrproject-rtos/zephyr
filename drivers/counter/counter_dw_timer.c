/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_dw_timers

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

LOG_MODULE_REGISTER(dw_timer, CONFIG_COUNTER_LOG_LEVEL);

/* DW APB timer register offsets */
#define LOADCOUNT_OFST           0x0
#define CURRENTVAL_OFST          0x4
#define CONTROLREG_OFST          0x8
#define EOI_OFST                 0xc
#define INTSTAT_OFST             0x10
#define TIMERSINTSTAT_OFST       0xa0
#define TIMERSEOI_OFST           0xa4
#define TIMERSRAWINTSTAT_OFST    0xa8
#define TIMERSCOMPVERSION_OFST   0xac

/* DW APB timer control flags */
#define TIMER_CONTROL_ENABLE    0
#define TIMER_MODE              1
#define TIMER_INTR_MASK         2

/* DW APB timer modes */
#define USER_DEFINED_MODE       1
#define FREE_RUNNING_MODE       0

#define GET_DEV_CONFIG(_dev)	((struct dw_timer_config *const)(_dev)->config)
#define GET_DEV_DATA(_dev)		((struct dw_timer_drv_data *const)(_dev)->data)

/* conflict requirement of DEVICE_MMIO_ROM and struct counter_config_info to    *
 * be a first member of config structure of driver                              *
 */
#define DEVICE_MMIO_ROM_OBJ(dev) (((struct dw_timer_config *)(dev->config))->_mmio)

#ifdef DEVICE_MMIO_IS_IN_RAM
#define DEVICE_MMIO_GET_TIMER(dev) DEVICE_MMIO_GET(dev)

#define DEVICE_MMIO_MAP_TIMER(dev, flags) \
	device_map(DEVICE_MMIO_RAM_PTR(dev), \
			DEVICE_MMIO_ROM_OBJ(dev).phys_addr, \
			DEVICE_MMIO_ROM_OBJ(dev).size, \
			(flags))
#else
#define DEVICE_MMIO_GET_TIMER(dev) (uintptr_t)(DEVICE_MMIO_ROM_OBJ(dev).addr)

#define DEVICE_MMIO_MAP_TIMER(dev, flags)
#endif	/* DEVICE_MMIO_IS_IN_RAM */

struct dw_timer_config {
	/* counter_config_info needs to be first */
	struct counter_config_info info;

	DEVICE_MMIO_ROM;
	uint32_t freq;
	char *clock_drv;
	uint32_t clkid;
	const struct reset_dt_spec reset;
	void (*irq_config)(void);
};

struct dw_timer_drv_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
	counter_top_callback_t top_cb;
	counter_alarm_callback_t alarm_cb;
	void *prv_data;
};

static void dw_timer_irq_handler(const struct device *timer_dev)
{
	struct dw_timer_drv_data *const data = GET_DEV_DATA(timer_dev);
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(timer_dev);
	uint32_t ticks = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->alarm_cb) {
		ticks = sys_read32(reg_base + LOADCOUNT_OFST);

		data->alarm_cb(timer_dev, 0, ticks, data->prv_data);
		data->alarm_cb = NULL;

		sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE);
	} else if (data->top_cb)
		data->top_cb(timer_dev, data->prv_data);

	k_spin_unlock(&data->lock, key);

	sys_read32(reg_base + EOI_OFST);
}

static int dw_timer_start(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(dev);

	if (sys_test_bit(reg_base + CONTROLREG_OFST,
					TIMER_CONTROL_ENABLE)) {
		LOG_ERR("Timer is already running");
		return -EBUSY;
	}

	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_MODE);
	sys_write32(~0, reg_base + LOADCOUNT_OFST);

	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE);
	return 0;
}

int dw_timer_disable(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(dev);

	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE);
	return 0;
}

static uint32_t dw_timer_get_top_value(const struct device *timer_dev)
{
	struct dw_timer_config *const timer_config = GET_DEV_CONFIG(timer_dev);
	uint32_t top_val = 0;

	top_val = timer_config->info.max_top_value;

	return top_val;
}

static int dw_timer_get_value(const struct device *timer_dev, uint32_t *ticks)
{
	uint32_t curr_val = 0, load_value = 0;
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(timer_dev);

	curr_val = sys_read32(reg_base + CURRENTVAL_OFST);
	load_value = sys_read32(reg_base + LOADCOUNT_OFST);

	*ticks = load_value - curr_val;
	return 0;
}

static int dw_timer_set_top_value(const struct device *timer_dev,
					const struct counter_top_cfg *top_cfg)
{
	struct dw_timer_drv_data *const data = GET_DEV_DATA(timer_dev);
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(timer_dev);
	k_spinlock_key_t key;

	/* Timer is already running. */
	if (sys_test_bit(reg_base + CONTROLREG_OFST,
					TIMER_CONTROL_ENABLE)) {
		LOG_ERR("Timer is already running.");
		return -EBUSY;
	}

	if (!top_cfg->callback) {
		LOG_ERR("Top config callback function is missing");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	data->top_cb = top_cfg->callback;
	data->prv_data = top_cfg->user_data;

	k_spin_unlock(&data->lock, key);

	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_MODE);
	sys_write32(top_cfg->ticks, reg_base + LOADCOUNT_OFST);

	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE);

	return 0;
}

static int dw_timer_set_alarm(const struct device *timer_dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	struct dw_timer_drv_data *const data = GET_DEV_DATA(timer_dev);
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(timer_dev);
	k_spinlock_key_t key;

	/* is timer running already? */
	if (sys_test_bit(reg_base + CONTROLREG_OFST,
					TIMER_CONTROL_ENABLE)) {
		LOG_ERR("Timer is already running!");
		return -EBUSY;
	}

	if (!alarm_cfg->callback) {
		LOG_ERR("Alarm config callback function is missing");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	data->alarm_cb = alarm_cfg->callback;
	data->prv_data = alarm_cfg->user_data;

	k_spin_unlock(&data->lock, key);

	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_MODE);
	sys_write32(alarm_cfg->ticks, reg_base + LOADCOUNT_OFST);

	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE);

	return 0;
}

static int dw_timer_cancel_alarm(const struct device *timer_dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	uintptr_t reg_base = DEVICE_MMIO_GET_TIMER(timer_dev);
	struct dw_timer_drv_data *const data = GET_DEV_DATA(timer_dev);
	k_spinlock_key_t key;

	sys_write32(0, reg_base + CONTROLREG_OFST);

	key = k_spin_lock(&data->lock);

	data->alarm_cb = NULL;
	data->prv_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

uint32_t dw_timer_get_freq(const struct device *timer_dev)
{
	struct dw_timer_config *const timer_config = GET_DEV_CONFIG(timer_dev);
	uint32_t freq = 0;
	const struct device *clk_dev;

	freq = timer_config->freq;

	/*
	 * Set clock rate from clock_frequency property if valid,
	 * otherwise, get clock rate from clock manager
	 */
	if (freq == 0U) {
		clk_dev = device_get_binding(timer_config->clock_drv);
		if (!clk_dev) {
			LOG_ERR("%s: device not found", timer_config->clock_drv);
			return -ENODEV;
		}

		clock_control_get_rate(clk_dev,
			(clock_control_subsys_t) &timer_config->clkid, &freq);
	}

	return freq;
}

static const struct counter_driver_api dw_timer_driver_api = {
	.start = dw_timer_start,
	.stop = dw_timer_disable,
	.get_value = dw_timer_get_value,
	.set_top_value = dw_timer_set_top_value,
	.get_top_value = dw_timer_get_top_value,
	.set_alarm = dw_timer_set_alarm,
	.cancel_alarm = dw_timer_cancel_alarm,
	.get_freq = dw_timer_get_freq,
};

static int dw_timer_init(const struct device *timer_dev)
{
	DEVICE_MMIO_MAP_TIMER(timer_dev, K_MEM_CACHE_NONE);
	struct dw_timer_config *const timer_config = GET_DEV_CONFIG(timer_dev);

	if (!device_is_ready(timer_config->reset.dev)) {
		LOG_ERR("Reset device node not found");
		return -ENODEV;
	}

	reset_line_assert(timer_config->reset.dev, timer_config->reset.id);
	reset_line_deassert(timer_config->reset.dev, timer_config->reset.id);

	timer_config->irq_config();

	return 0;
}

#define DW_SNPS_TIMER_CLOCK_RATE_INIT(inst) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency), \
		( \
			.freq = DT_INST_PROP(inst, clock_frequency), \
			.clock_drv = NULL, \
			.clkid = 0, \
		), \
		( \
			.freq = 0, \
			.clock_drv = DT_LABEL(DT_INST_PHANDLE(inst, clocks)), \
			.clkid = DT_INST_PHA_BY_IDX(inst, clocks, 0, clkid), \
		) \
	)

#define CREATE_DW_TIMER_DEV(inst) \
	static void dw_timer_config_##inst(void); \
	static struct dw_timer_drv_data timer_data_##inst; \
	static struct dw_timer_config timer_config_##inst = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)), \
		DW_SNPS_TIMER_CLOCK_RATE_INIT(inst) \
		.info = { \
					.max_top_value = UINT32_MAX, \
					.channels = 1, \
		}, \
		.reset = RESET_DT_SPEC_INST_GET(inst), \
		.irq_config = dw_timer_config_##inst, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, \
			dw_timer_init, \
			NULL, &timer_data_##inst, \
			&timer_config_##inst, POST_KERNEL, \
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
			&dw_timer_driver_api); \
	static void dw_timer_config_##inst(void) \
	{ \
		IRQ_CONNECT(DT_INST_IRQN(inst), \
				DT_INST_IRQ(inst, priority), \
				dw_timer_irq_handler, \
				DEVICE_DT_INST_GET(inst), 0); \
		irq_enable(DT_INST_IRQN(inst)); \
	}

DT_INST_FOREACH_STATUS_OKAY(CREATE_DW_TIMER_DEV);
