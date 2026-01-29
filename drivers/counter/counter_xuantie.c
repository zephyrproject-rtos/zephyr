/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief COUNTER driver for the XuanTie xiaohui&smartl FPGA
 */

#define DT_DRV_COMPAT xuantie_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(xuantie_counter, CONFIG_COUNTER_LOG_LEVEL);

static int counter_xuantie_timer_get_value(const struct device *timer_dev, uint32_t *ticks);

/* clang-format off */

/*timer register offsets */
#define LOADCOUNT_OFST           0x0
#define CURRENTVAL_OFST          0x4
#define CONTROLREG_OFST          0x8
#define EOI_OFST                 0xc
#define INTSTAT_OFST             0x10

/* free running mode value */
#define FREE_RUNNING_MODE_VAL 0xFFFFFFFFUL

/* timer control flags */
#define TIMER_CONTROL_ENABLE_BIT    0
#define TIMER_MODE_BIT              1
#define TIMER_INTR_MASK_BIT         2

/* timer modes */
#define USER_DEFINED_MODE       1
#define FREE_RUNNING_MODE       0

/* clang-format no */

#define DEV_CFG(_dev) ((const struct counter_xuantie_timer_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct counter_xuantie_timer_drv_data *const)(_dev)->data)

/* Device Configuration */
struct counter_xuantie_timer_config {
	struct counter_config_info info;

	DEVICE_MMIO_NAMED_ROM(timer_mmio);

	/* clock frequency of timer */
	uint32_t freq;

	/* interrupt config function ptr */
	void (*irq_config)(void);
};

/* Driver data */
struct counter_xuantie_timer_drv_data {
	/* mmio address mapping info */
	DEVICE_MMIO_NAMED_RAM(timer_mmio);

	uint32_t freq;

	/* spin lock to protect user data */
	struct k_spinlock lock;
	/* top callback function */
	counter_top_callback_t top_cb;
	/* alarm callback function */
	counter_alarm_callback_t alarm_cb;
	/* private user data */
	void *prv_data;
};

static void counter_xuantie_timer_irq_handler(const struct device *timer_dev)
{
	uint32_t ticks = 0;
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);
	k_spinlock_key_t key;
	counter_alarm_callback_t alarm_cb = data->alarm_cb;

	/* read EOI register to clear interrupt flag */
	sys_read32(reg_base + EOI_OFST);

	counter_xuantie_timer_get_value(timer_dev, &ticks);

	key = k_spin_lock(&data->lock);

	/* In case of alarm, mask interrupt and disable the callback. User
	 * can configure the alarm in same context within callback function.
	 */
	if (data->alarm_cb) {
		sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_INTR_MASK_BIT);

		data->alarm_cb = NULL;
		alarm_cb(timer_dev, 0, ticks, data->prv_data);

	} else if (data->top_cb) {
		data->top_cb(timer_dev, data->prv_data);
	}

	k_spin_unlock(&data->lock, key);
}


/**
 * @brief Start counter device in free running mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */

static int counter_xuantie_timer_start(const struct device *dev)
{
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, timer_mmio);

	/* disable timer before starting in free-running mode */
	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);

	/* starting timer in free running mode */
	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_MODE_BIT);
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_INTR_MASK_BIT);
	sys_write32(FREE_RUNNING_MODE_VAL, reg_base + LOADCOUNT_OFST);

	/* enable timer */
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);
	return 0;
}

/**
 * @brief Stop counter device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
int counter_xuantie_timer_disable(const struct device *dev)
{
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, timer_mmio);

	/* stop timer */
	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);
	return 0;
}


/**
 * @brief Function to retrieve current top value.
 *
 * @param[in]  timer_dev  Pointer to the device structure for the driver instance.
 *
 * @return Top value.
 */
static uint32_t counter_xuantie_timer_get_top_value(const struct device *timer_dev)
{
	uint32_t top_val = 0;
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);

	/* get the current top value from load count register */
	top_val = sys_read32(reg_base + LOADCOUNT_OFST);

	return top_val;
}

/**
 * @brief Get current counter value.
 * @param timer_dev Pointer to the device structure for the driver instance.
 * @param ticks Pointer to where to store the current counter value
 *
 * @retval 0 If successful.
 */
static int counter_xuantie_timer_get_value(const struct device *timer_dev, uint32_t *ticks)
{
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);

	/* current value of the currentdis value register */
	*ticks = sys_read32(reg_base + CURRENTVAL_OFST);

	return 0;
}

/**
 * @brief Set counter top value.
 * @param timer_dev		Pointer to the device structure for the driver instance.
 * @param top_cfg		Configuration. Cannot be NULL.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported (top value cannot be updated without reset).
 * @retval -EBUSY if any alarm is active.
 * @retval -ETIME if @ref COUNTER_TOP_CFG_DONT_RESET was set and new top value
 *		  is smaller than current counter value (counter counting up).
 */
static int counter_xuantie_timer_set_top_value(const struct device *timer_dev,
					const struct counter_top_cfg *top_cfg)
{
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);
	k_spinlock_key_t key;

	if (top_cfg == NULL) {
		LOG_ERR("Invalid top value configuration");
		return -EINVAL;
	}

	/* top value cannot be updated without reset */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		LOG_ERR("Updating top value without reset is not supported");
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	/* top value cannot be updated if the alarm is active */
	if (data->alarm_cb) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Top value cannot be updated, alarm is active!");
		return -EBUSY;
	}

	if (!top_cfg->callback) {
		/* mask an interrupt if callback is not passed */
		sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_INTR_MASK_BIT);
	} else {
		/* unmask interrupt if callback is passed */
		sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_INTR_MASK_BIT);
	}

	data->top_cb = top_cfg->callback;
	data->prv_data = top_cfg->user_data;

	/* top value can be loaded only when timer is stopped and re-enabled */
	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);

	/* configuring timer in user-defined mode */
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_MODE_BIT);

	/* set new top value */
	sys_write32(top_cfg->ticks, reg_base + LOADCOUNT_OFST);
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);

	k_spin_unlock(&data->lock, key);

	return 0;
}

/**
 * @brief Set a single shot alarm on a channel.
 *
 * @param timer_dev	Pointer to the device structure for the driver instance.
 * @param chan_id	Channel ID.
 * @param alarm_cfg	Alarm configuration.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if request is not supported.
 * @retval -EINVAL if alarm settings are invalid.
 * @retval -EBUSY  if alarm is already active.
 */
static int counter_xuantie_timer_set_alarm(const struct device *timer_dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);
	k_spinlock_key_t key;

	if (alarm_cfg == NULL) {
		LOG_ERR("Invalid alarm configuration");
		return -EINVAL;
	}

	/* Alarm callback is mandatory */
	if (!alarm_cfg->callback) {
		LOG_ERR("Alarm callback function cannot be null");
		return -EINVAL;
	}

	/* absolute alarm is not supported as interrupts are triggered
	 * only when the counter reaches 0(downcounter)
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		LOG_ERR("Absolute alarm is not supported");
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	/* check if alarm is already active */
	if (data->alarm_cb != NULL) {
		LOG_ERR("Alarm is already active\n");
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm_cb = alarm_cfg->callback;
	data->prv_data = alarm_cfg->user_data;

	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);

	/* start timer in user-defined mode */
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_MODE_BIT);
	sys_clear_bit(reg_base + CONTROLREG_OFST, TIMER_INTR_MASK_BIT);

	sys_write32(alarm_cfg->ticks, reg_base + LOADCOUNT_OFST);
	sys_set_bit(reg_base + CONTROLREG_OFST, TIMER_CONTROL_ENABLE_BIT);

	k_spin_unlock(&data->lock, key);

	return 0;
}

/**
 * @brief Cancel an alarm on a channel.
 *
 * @param timer_dev	Pointer to the device structure for the driver instance.
 * @param chan_id	Channel ID.
 *
 * @retval 0 If successful.
 */

static int counter_xuantie_timer_cancel_alarm(const struct device *timer_dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);
	volatile uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(timer_dev, timer_mmio);
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	sys_write32(0, reg_base + CONTROLREG_OFST);

	data->alarm_cb = NULL;
	data->prv_data = NULL;

	k_spin_unlock(&data->lock, key);

	return 0;
}

/**
 * @brief Function to get counter frequency.
 *
 * @param[in]  timer_dev Pointer to the device structure for the driver instance.
 *
 * @return Frequency of the counter in Hz, or zero if the counter does
 * not have a fixed frequency.
 */

uint32_t counter_xuantie_timer_get_freq(const struct device *timer_dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);

	return data->freq;
#else
	const struct counter_xuantie_timer_config *config = DEV_CFG(timer_dev);

	return config->freq;
#endif
}

static const struct counter_driver_api dw_timer_driver_api = {
	.start = counter_xuantie_timer_start,
	.stop = counter_xuantie_timer_disable,
	.get_value = counter_xuantie_timer_get_value,
	.set_top_value = counter_xuantie_timer_set_top_value,
	.get_top_value = counter_xuantie_timer_get_top_value,
	.set_alarm = counter_xuantie_timer_set_alarm,
	.cancel_alarm = counter_xuantie_timer_cancel_alarm,
	.get_freq = counter_xuantie_timer_get_freq,
};


static int counter_xuantie_timer_init(const struct device *timer_dev)
{
	DEVICE_MMIO_NAMED_MAP(timer_dev, timer_mmio, K_MEM_CACHE_NONE);
	const struct counter_xuantie_timer_config *timer_config = DEV_CFG(timer_dev);
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks) || DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	int ret;
#endif

	/*
	 * get clock rate from clock_frequency property if valid,
	 * otherwise, get clock rate from clock manager
	 */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clocks)
	struct counter_xuantie_timer_drv_data *const data = DEV_DATA(timer_dev);

	if (!device_is_ready(timer_config->clk_dev)) {
		LOG_ERR("clock controller device not ready");
		return -ENODEV;
	}
	ret = clock_control_get_rate(timer_config->clk_dev,
					timer_config->clkid, &data->freq);
	if (ret != 0) {
		LOG_ERR("Unable to get clock rate: err:%d", ret);
		return ret;
	}
#endif

	/* Reset timer only if reset controller driver is supported */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)
	if (timer_config->reset.dev != NULL) {
		if (!device_is_ready(timer_config->reset.dev)) {
			LOG_ERR("Reset controller device not ready");
			return -ENODEV;
		}

		ret = reset_line_toggle(timer_config->reset.dev, timer_config->reset.id);
		if (ret != 0) {
			LOG_ERR("Timer reset failed");
			return ret;
		}
	}
#endif

	timer_config->irq_config();

	return 0;
}

#define DW_SNPS_TIMER_CLOCK_RATE_INIT(inst)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency),				\
		(										\
			.freq = DT_INST_PROP(inst, clock_frequency),				\
		),										\
		(										\
			.freq = 0,								\
			.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),			\
			.clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid),	\
		)										\
	)

#define DW_SNPS_TIMER_SNPS_RESET_SPEC_INIT(inst)				\
	.reset = RESET_DT_SPEC_INST_GET(inst),					\

#define CREATE_DW_TIMER_DEV(inst)						\
	static void counter_xuantie_timer_irq_config_##inst(void); \
	static struct counter_xuantie_timer_drv_data timer_data_##inst;		\
	static const struct counter_xuantie_timer_config timer_config_##inst = {	\
		DEVICE_MMIO_NAMED_ROM_INIT(timer_mmio, DT_DRV_INST(inst)),	\
		DW_SNPS_TIMER_CLOCK_RATE_INIT(inst)				\
		.info = {							\
					.max_top_value = UINT32_MAX,		\
					.channels = 1,				\
		},								\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets),			\
			(DW_SNPS_TIMER_SNPS_RESET_SPEC_INIT(inst)))		\
		.irq_config = counter_xuantie_timer_irq_config_##inst,		\
	};									\
	DEVICE_DT_INST_DEFINE(inst,						\
			counter_xuantie_timer_init,					\
			NULL, &timer_data_##inst,				\
			&timer_config_##inst, POST_KERNEL,			\
			CONFIG_COUNTER_INIT_PRIORITY,				\
			&dw_timer_driver_api);					\
	static void counter_xuantie_timer_irq_config_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
				DT_INST_IRQ(inst, priority),			\
				counter_xuantie_timer_irq_handler,			\
				DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
	}

DT_INST_FOREACH_STATUS_OKAY(CREATE_DW_TIMER_DEV);
