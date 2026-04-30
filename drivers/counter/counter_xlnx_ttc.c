/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_ttc_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_ttc, CONFIG_COUNTER_LOG_LEVEL);

/* ==================== REGISTER DEFINITIONS ==================== */

/* Clock Control Register offsets */
#define TTC_CLOCK_CONTROL_1         0x00
#define TTC_CLOCK_CONTROL_2         0x04
#define TTC_CLOCK_CONTROL_3         0x08

/* Counter Control Register offsets */
#define TTC_COUNTER_CONTROL_1       0x0C
#define TTC_COUNTER_CONTROL_2       0x10
#define TTC_COUNTER_CONTROL_3       0x14

/* Counter Value Register offsets (read-only) */
#define TTC_COUNTER_VALUE_1         0x18
#define TTC_COUNTER_VALUE_2         0x1C
#define TTC_COUNTER_VALUE_3         0x20

/* Interval Counter Register offsets */
#define TTC_INTERVAL_COUNTER_1      0x24
#define TTC_INTERVAL_COUNTER_2      0x28
#define TTC_INTERVAL_COUNTER_3      0x2C

/* Match Register offsets */
#define TTC_MATCH_1_COUNTER_1       0x30
#define TTC_MATCH_1_COUNTER_2       0x34
#define TTC_MATCH_1_COUNTER_3       0x38

#define TTC_MATCH_2_COUNTER_1       0x3C
#define TTC_MATCH_2_COUNTER_2       0x40
#define TTC_MATCH_2_COUNTER_3       0x44

#define TTC_MATCH_3_COUNTER_1       0x48
#define TTC_MATCH_3_COUNTER_2       0x4C
#define TTC_MATCH_3_COUNTER_3       0x50

/* Interrupt Register offsets */
#define TTC_INTERRUPT_REGISTER_1    0x54
#define TTC_INTERRUPT_REGISTER_2    0x58
#define TTC_INTERRUPT_REGISTER_3    0x5C

/* Interrupt Enable Register offsets */
#define TTC_INTERRUPT_ENABLE_1      0x60
#define TTC_INTERRUPT_ENABLE_2      0x64
#define TTC_INTERRUPT_ENABLE_3      0x68

/* Event Control Register offsets */
#define TTC_EVENT_CONTROL_TIMER_1   0x6C
#define TTC_EVENT_CONTROL_TIMER_2   0x70
#define TTC_EVENT_CONTROL_TIMER_3   0x74

/* Event Register offsets */
#define TTC_EVENT_REGISTER_1        0x78
#define TTC_EVENT_REGISTER_2        0x7C
#define TTC_EVENT_REGISTER_3        0x80

/* Clock Control Register bit fields */
#define TTC_CLK_CTRL_PS_EN          BIT(0)
#define TTC_CLK_CTRL_PS_V_MASK      GENMASK(4, 1)
#define TTC_CLK_CTRL_PS_V_SHIFT     1
#define TTC_CLK_CTRL_C_SRC          BIT(5)
#define TTC_CLK_CTRL_EX_E           BIT(6)

/* Counter Control Register bit fields */
#define TTC_CNT_CTRL_DIS            BIT(0)
#define TTC_CNT_CTRL_INT            BIT(1)
#define TTC_CNT_CTRL_DEC            BIT(2)
#define TTC_CNT_CTRL_MATCH          BIT(3)
#define TTC_CNT_CTRL_RST            BIT(4)
#define TTC_CNT_CTRL_WAVE_EN        BIT(5)
#define TTC_CNT_CTRL_WAVE_POL       BIT(6)

/* Interrupt bit fields */
#define TTC_INT_IV                  BIT(0)
#define TTC_INT_M1                  BIT(1)
#define TTC_INT_M2                  BIT(2)
#define TTC_INT_M3                  BIT(3)
#define TTC_INT_OV                  BIT(4)
#define TTC_INT_EV                  BIT(5)

/* Constants */
#define TTC_MAX_CHANNELS            3
#define TTC_MAX_PRESCALER           15

/* Total number of TTC counter instances across all TTC IP blocks */
#define TTC_NUM_INSTANCES           DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
#define TTC_INSTANCE_STORAGE_SIZE   MAX(TTC_NUM_INSTANCES, 1)

/* Global device array for shared interrupt handling */
static const struct device *ttc_devices[TTC_INSTANCE_STORAGE_SIZE];

/*
 * Runtime tracking of registered IRQs to avoid duplicate registration.
 * Each entry stores an IRQ number that has been connected, or 0 if unused.
 * This allows shared IRQs across TTC instances to be registered only once.
 */
static uint32_t ttc_registered_irqs[TTC_INSTANCE_STORAGE_SIZE];
static uint8_t ttc_registered_irq_count;

/* ==================== DATA STRUCTURES ==================== */

/**
 * @brief TTC device configuration structure
 */
struct ttc_config {
	struct counter_config_info info;
	uintptr_t base;
	uint32_t clock_freq;
	uint32_t irq_num;
	uint32_t irq_priority;
	uint8_t prescaler;
	uint8_t device_idx; /* Unique index across all TTC instances (0 to TTC_NUM_INSTANCES-1) */
	bool prescaler_present;
};

/**
 * @brief TTC device runtime data structure
 */
struct ttc_data {
	counter_alarm_callback_t alarm_callbacks[TTC_MAX_CHANNELS];
	counter_top_callback_t top_callback;
	void *alarm_user_data[TTC_MAX_CHANNELS];
	void *top_user_data;
	uint32_t top_value;
	uint32_t guard_period;
	atomic_t late_alarm_pending;
	bool guard_period_set;
};

/* ==================== HELPER FUNCTIONS ==================== */

/**
 * @brief Calculate effective frequency after prescaling
 * @param dev Pointer to device structure
 * @return Effective counter frequency in Hz
 */
static uint32_t ttc_get_effective_frequency(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	uint32_t divisor = 1;

	if (config->prescaler_present) {
		divisor = 1U << (config->prescaler + 1);
	}

	return config->clock_freq / divisor;
}

/**
 * @brief Get match register offset for channel
 * @param channel Channel identifier (0-2)
 * @return Match register offset
 */
static uint32_t ttc_get_match_register(uint8_t channel)
{
	uint32_t timer1_matches[3] = {
		TTC_MATCH_1_COUNTER_1,
		TTC_MATCH_2_COUNTER_1,
		TTC_MATCH_3_COUNTER_1
	};

	if (channel >= TTC_MAX_CHANNELS) {
		return 0;
	}
	return timer1_matches[channel];
}

/**
 * @brief Get interrupt bit mask for channel
 * @param channel Channel identifier (0-2)
 * @return Interrupt bit mask
 */
static uint32_t ttc_get_interrupt_bit(uint8_t channel)
{
	uint32_t bits[3] = { TTC_INT_M1, TTC_INT_M2, TTC_INT_M3 };

	if (channel >= TTC_MAX_CHANNELS) {
		return 0;
	}

	return bits[channel];
}

/**
 * @brief Trigger software interrupt for late alarm
 *
 * Sets a software flag and triggers interrupt via GIC to handle late alarms.
 */
static void ttc_set_alarm_pending(const struct device *dev, uint8_t chan_id)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;

	atomic_or(&data->late_alarm_pending, BIT(chan_id));
	arm_gic_irq_set_pending(config->irq_num);
}

/* ==================== COUNTER API IMPLEMENTATION ==================== */

/**
 * @brief Start the counter
 * @param dev Pointer to device structure
 * @return 0 on success
 */
static int ttc_start(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	uint32_t ctrl_val;

	ctrl_val = sys_read32(config->base + TTC_COUNTER_CONTROL_1);
	ctrl_val |= TTC_CNT_CTRL_RST;
	ctrl_val &= ~TTC_CNT_CTRL_DIS;
	sys_write32(ctrl_val, config->base + TTC_COUNTER_CONTROL_1);

	LOG_DBG("Started TTC timer");

	return 0;
}

/**
 * @brief Stop the counter
 * @param dev Pointer to device structure
 * @return 0 on success
 */
static int ttc_stop(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	uint32_t ctrl_val;

	ctrl_val = sys_read32(config->base + TTC_COUNTER_CONTROL_1);
	ctrl_val |= TTC_CNT_CTRL_DIS;
	sys_write32(ctrl_val, config->base + TTC_COUNTER_CONTROL_1);

	LOG_DBG("Stopped TTC timer");

	return 0;
}

/**
 * @brief Get current counter value
 * @param dev Pointer to device structure
 * @param ticks Pointer to store current tick count
 * @return 0 on success, negative errno otherwise
 */
static int ttc_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct ttc_config *config = dev->config;

	if (!ticks) {
		return -EINVAL;
	}

	*ticks = sys_read32(config->base + TTC_COUNTER_VALUE_1);

	return 0;
}

/**
 * @brief Get current top value
 * @param dev Pointer to device structure
 * @return Current top value or maximum count value if not set
 */
static uint32_t ttc_get_top_value(const struct device *dev)
{
	struct ttc_data *data = dev->data;
	const struct ttc_config *config = dev->config;

	return data->top_value ? data->top_value : config->info.max_top_value;
}

/**
 * @brief Set top value (interval) for counter
 * @param dev Pointer to device structure
 * @param cfg Top value configuration
 * @return 0 on success, negative errno otherwise
 */
static int ttc_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t ctrl_val;
	uint32_t int_en_val;

	if (!cfg) {
		return -EINVAL;
	}

	int_en_val = sys_read32(config->base + TTC_INTERRUPT_ENABLE_1);

	if (cfg->ticks == 0 || cfg->ticks > config->info.max_top_value) {
		LOG_ERR("Invalid top value %u", cfg->ticks);
		return -EINVAL;
	}

	LOG_DBG("Setting top value to %u", cfg->ticks);

	sys_write32(cfg->ticks, config->base + TTC_INTERVAL_COUNTER_1);
	ctrl_val = sys_read32(config->base + TTC_COUNTER_CONTROL_1);
	ctrl_val |= TTC_CNT_CTRL_INT;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		ctrl_val |= TTC_CNT_CTRL_RST;
	}

	sys_write32(ctrl_val, config->base + TTC_COUNTER_CONTROL_1);

	data->top_value = cfg->ticks;
	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (cfg->callback) {
		int_en_val |= TTC_INT_IV;
		sys_write32(int_en_val, config->base + TTC_INTERRUPT_ENABLE_1);
	}

	return 0;
}

/**
 * @brief Get counter frequency
 * @param dev Pointer to device structure
 * @return Counter frequency in Hz
 */
static uint32_t ttc_get_freq(const struct device *dev)
{
	return ttc_get_effective_frequency(dev);
}

static int ttc_compute_alarm_ticks(const struct device *dev,
				   const struct counter_alarm_cfg *alarm_cfg,
				   uint32_t current_ticks, uint32_t *alarm_ticks)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	bool is_absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0;

	if (!is_absolute) {
		uint32_t limit = data->top_value > 0 ? data->top_value :
			config->info.max_top_value;

		if (ticks > limit) {
			LOG_ERR("Relative alarm ticks %u > limit %u", ticks, limit);
			return -EINVAL;
		}

		ticks += current_ticks;
		if (data->top_value > 0 && ticks > data->top_value) {
			ticks %= (data->top_value + 1);
		}
	} else if (data->top_value > 0 && ticks > data->top_value) {
		LOG_ERR("Alarm ticks %u > top value %u", ticks, data->top_value);
		return -EINVAL;
	}

	*alarm_ticks = ticks;
	return 0;
}

static bool ttc_is_alarm_late(const struct device *dev, uint32_t current_ticks,
			      uint32_t alarm_ticks)
{
	struct ttc_data *data = dev->data;
	uint32_t top = ttc_get_top_value(dev);
	uint32_t max_rel_val = data->guard_period_set ? (top - data->guard_period) : top;
	uint32_t target = (alarm_ticks == 0) ? top : (alarm_ticks - 1);
	uint32_t diff;

	if (current_ticks <= target) {
		diff = target - current_ticks;
	} else {
		diff = target + top + 1 - current_ticks;
	}

	return (diff > max_rel_val) || (alarm_ticks == current_ticks);
}

static int ttc_handle_late_alarm(const struct device *dev, uint8_t chan_id,
				 const struct counter_alarm_cfg *alarm_cfg,
				 uint32_t current_ticks, uint32_t alarm_ticks,
				 bool *program_match)
{
	struct ttc_data *data = dev->data;
	bool is_absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0;
	bool expire_when_late = (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0;
	bool same_tick = alarm_ticks == current_ticks;

	*program_match = true;

	if (!ttc_is_alarm_late(dev, current_ticks, alarm_ticks)) {
		return 0;
	}

	if (expire_when_late || same_tick) {
		data->alarm_callbacks[chan_id] = alarm_cfg->callback;
		data->alarm_user_data[chan_id] = alarm_cfg->user_data;
		ttc_set_alarm_pending(dev, chan_id);
		*program_match = false;
		return is_absolute ? -ETIME : 0;
	}

	if (is_absolute) {
		*program_match = false;
		return -ETIME;
	}

	return 0;
}

/**
 * @brief Cancel alarm on a channel
 * @param dev Pointer to device structure
 * @param chan_id Channel identifier (0-2)
 * @return 0 on success, negative errno otherwise
 */
static int ttc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t int_en_val;

	if (chan_id >= TTC_MAX_CHANNELS) {
		return -ENOTSUP;
	}

	LOG_DBG("Cancelling alarm %d", chan_id);

	data->alarm_callbacks[chan_id] = NULL;
	data->alarm_user_data[chan_id] = NULL;

	int_en_val = sys_read32(config->base + TTC_INTERRUPT_ENABLE_1);
	int_en_val &= ~ttc_get_interrupt_bit(chan_id);
	sys_write32(int_en_val, config->base + TTC_INTERRUPT_ENABLE_1);

	return 0;
}

/**
 * @brief Set alarm on a channel
 */
static int ttc_set_alarm(const struct device *dev, uint8_t chan_id,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t match_offset;
	uint32_t alarm_ticks;
	uint32_t current_ticks;
	uint32_t int_en_val;
	uint32_t ctrl_val;
	bool program_match;
	int ret;

	if (chan_id >= TTC_MAX_CHANNELS) {
		LOG_ERR("Invalid channel %d", chan_id);
		return -ENOTSUP;
	}

	if (!alarm_cfg || !alarm_cfg->callback) {
		return -EINVAL;
	}

	/* Get current counter value - needed for both relative and absolute alarms */
	ret = ttc_get_value(dev, &current_ticks);
	if (ret != 0) {
		return ret;
	}

	ret = ttc_compute_alarm_ticks(dev, alarm_cfg, current_ticks, &alarm_ticks);
	if (ret != 0) {
		return ret;
	}

	ret = ttc_handle_late_alarm(dev, chan_id, alarm_cfg, current_ticks,
				    alarm_ticks, &program_match);
	if (ret != 0 || !program_match) {
		return ret;
	}

	match_offset = ttc_get_match_register(chan_id);
	if (match_offset == 0) {
		return -ENOTSUP;
	}

	/* Write match register */
	sys_write32(alarm_ticks, config->base + match_offset);

	data->alarm_callbacks[chan_id] = alarm_cfg->callback;
	data->alarm_user_data[chan_id] = alarm_cfg->user_data;

	ctrl_val = sys_read32(config->base + TTC_COUNTER_CONTROL_1);
	ctrl_val |= TTC_CNT_CTRL_MATCH;
	sys_write32(ctrl_val, config->base + TTC_COUNTER_CONTROL_1);

	/* Normal case - enable interrupt */
	int_en_val = sys_read32(config->base + TTC_INTERRUPT_ENABLE_1);
	int_en_val |= ttc_get_interrupt_bit(chan_id);
	sys_write32(int_en_val, config->base + TTC_INTERRUPT_ENABLE_1);

	return 0;
}

/**
 * @brief Set guard period for late alarm detection
 */
static int ttc_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	struct ttc_data *data = dev->data;

	ARG_UNUSED(flags);

	if (ticks > ttc_get_top_value(dev)) {
		return -EINVAL;
	}

	data->guard_period = ticks;
	data->guard_period_set = true;

	return 0;
}

/**
 * @brief Get guard period
 * @param dev Pointer to device structure
 * @param flags Guard period flags
 * @return Guard period in ticks or 0 if not set
 */
static uint32_t ttc_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct ttc_data *data = dev->data;

	if (flags != COUNTER_GUARD_PERIOD_LATE_TO_SET || !data->guard_period_set) {
		return 0;
	}

	return data->guard_period;
}

/**
 * @brief Get pending interrupt status
 * @param dev Pointer to device structure
 * @return 1 if interrupt pending, 0 otherwise
 */
static uint32_t ttc_get_pending_int(const struct device *dev)
{
	const struct ttc_config *config = dev->config;

	return sys_read32(config->base + TTC_INTERRUPT_REGISTER_1) ? 1 : 0;
}

/* ==================== INTERRUPT HANDLER ==================== */

/**
 * @brief Process interrupts for a single TTC timer
 * @param dev Pointer to device structure
 * @param int_status Interrupt status register value (already read by ISR)
 */
static void ttc_process_timer_interrupt(const struct device *dev, uint32_t int_status)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t current_ticks;
	uint32_t int_en_val;

	/* Check for software-triggered late alarms */
	uint32_t sw_pending = atomic_and(&data->late_alarm_pending, 0);

	/* Handle interval interrupt */
	if ((int_status & TTC_INT_IV) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

	/* Handle match interrupts (hardware or software-triggered) */
	for (int i = 0; i < TTC_MAX_CHANNELS; i++) {
		uint32_t match_bit = ttc_get_interrupt_bit(i);
		bool hw_pending = (int_status & match_bit) != 0;
		bool sw_late_alarm = (sw_pending & BIT(i)) != 0;

		if ((hw_pending || sw_late_alarm) && data->alarm_callbacks[i]) {
			int ret = ttc_get_value(dev, &current_ticks);

			if (ret != 0) {
				LOG_ERR("Failed to read TTC value, chan %d: %d", i, ret);
				continue;
			}

			data->alarm_callbacks[i](dev, i, current_ticks,
						data->alarm_user_data[i]);

			int_en_val = sys_read32(config->base + TTC_INTERRUPT_ENABLE_1);
			int_en_val &= ~match_bit;
			sys_write32(int_en_val, config->base + TTC_INTERRUPT_ENABLE_1);

			data->alarm_callbacks[i] = NULL;
			data->alarm_user_data[i] = NULL;
		}
	}
}

/**
 * @brief TTC Interrupt Service Routine
 *
 * Checks all timer interrupt status registers within the TTC IP block.
 * For shared interrupts, processes all pending timers.
 * For dedicated interrupts, only the triggered timer will have status set.
 *
 * @param dev Pointer to device structure
 */
static void ttc_isr(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	uint32_t irq_num = config->irq_num;
	uint32_t int_status;
	/* Iterate through all registered TTC counter instances */
	for (uint8_t idx = 0; idx < TTC_NUM_INSTANCES; idx++) {
		const struct device *timer_dev = ttc_devices[idx];

		if (timer_dev == NULL) {
			continue;
		}
		const struct ttc_config *timer_cfg = timer_dev->config;
		/* Only process devices belonging to the same IRQ */
		if (timer_cfg->irq_num != irq_num) {
			continue;
		}
		int_status = sys_read32(timer_cfg->base + TTC_INTERRUPT_REGISTER_1);
		/*
		 * Process timer interrupt even if int_status is 0, because there might be
		 * software-triggered late alarms pending (via arm_gic_irq_set_pending).
		 */
		ttc_process_timer_interrupt(timer_dev, int_status);
	}
}

/* ==================== DRIVER API STRUCTURE ==================== */

static const struct counter_driver_api ttc_driver_api = {
	.start = ttc_start,
	.stop = ttc_stop,
	.get_value = ttc_get_value,
	.set_alarm = ttc_set_alarm,
	.cancel_alarm = ttc_cancel_alarm,
	.set_top_value = ttc_set_top_value,
	.get_pending_int = ttc_get_pending_int,
	.get_top_value = ttc_get_top_value,
	.get_guard_period = ttc_get_guard_period,
	.set_guard_period = ttc_set_guard_period,
	.get_freq = ttc_get_freq,
};

/* ==================== DEVICE INITIALIZATION ==================== */

/**
 * @brief Check if an IRQ has already been registered
 * @param irq_num IRQ number to check
 * @return true if already registered, false otherwise
 */
static bool ttc_irq_is_registered(uint32_t irq_num)
{
	if (TTC_NUM_INSTANCES == 0) {
		return false;
	}

	for (uint8_t i = 0; i < ttc_registered_irq_count; i++) {
		if (ttc_registered_irqs[i] == irq_num) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Register an IRQ as connected
 * @param irq_num IRQ number to register
 */
static void ttc_irq_mark_registered(uint32_t irq_num)
{
	if (TTC_NUM_INSTANCES == 0) {
		return;
	}

	if (ttc_registered_irq_count < TTC_NUM_INSTANCES) {
		ttc_registered_irqs[ttc_registered_irq_count++] = irq_num;
	}
}

/**
 * @brief Initialize TTC timer device
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno otherwise
 */
static int ttc_init(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t clk_ctrl_val;
	uint32_t ctrl_val;

	/* Register this device in global array for shared interrupt handling */
	if (config->device_idx < TTC_NUM_INSTANCES) {
		ttc_devices[config->device_idx] = dev;
	}

	memset(data, 0, sizeof(*data));
	clk_ctrl_val = 0;

	if (config->prescaler_present && config->prescaler <= TTC_MAX_PRESCALER) {
		clk_ctrl_val |= TTC_CLK_CTRL_PS_EN;
		clk_ctrl_val |= (config->prescaler << TTC_CLK_CTRL_PS_V_SHIFT) &
			TTC_CLK_CTRL_PS_V_MASK;
	}

	sys_write32(clk_ctrl_val, config->base + TTC_CLOCK_CONTROL_1);
	ctrl_val = TTC_CNT_CTRL_DIS | TTC_CNT_CTRL_RST;
	sys_write32(ctrl_val, config->base + TTC_COUNTER_CONTROL_1);

	(void)sys_read32(config->base + TTC_INTERRUPT_REGISTER_1);
	sys_write32(0, config->base + TTC_INTERRUPT_ENABLE_1);

	/*
	 * Connect IRQ dynamically only if not already registered.
	 * This handles shared IRQs across multiple TTC counter instances.
	 */
	if (!ttc_irq_is_registered(config->irq_num)) {
		irq_connect_dynamic(config->irq_num, config->irq_priority,
				    (void (*)(const void *))ttc_isr, dev, 0);
		irq_enable(config->irq_num);
		ttc_irq_mark_registered(config->irq_num);
	}

	return 0;
}

/* ==================== DEVICE INSTANTIATION ==================== */

/*  if prescale is enabled, the count rate is divided by 2^(N+1) */
#define PRESCALER_DIV(n) \
	COND_CODE_1(                                     \
		DT_INST_NODE_HAS_PROP(n, prescaler),         \
		(BIT(DT_INST_PROP(n, prescaler) + 1)),       \
		(1U)                                         \
		)

#define TTC_DEVICE_INIT(n)							\
	static struct ttc_data ttc_data_##n;					\
										\
										\
	static const struct ttc_config ttc_config_##n = {			\
		.base = DT_INST_REG_ADDR(n),					\
		.clock_freq = DT_INST_PROP(n, clock_frequency),			\
		.prescaler_present = DT_INST_NODE_HAS_PROP(n, prescaler),       \
		.prescaler = DT_INST_PROP_OR(n, prescaler, 0),			\
		.irq_num = DT_INST_IRQN(n),					\
		.irq_priority = DT_INST_IRQ(n, priority),			\
		.device_idx = n,						\
		.info = {							\
			.max_top_value = GENMASK(DT_INST_PROP(n, timer_width) - 1, 0), \
			.freq = DT_INST_PROP(n, clock_frequency) /		\
				PRESCALER_DIV(n),                               \
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = TTC_MAX_CHANNELS,				\
		},								\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, ttc_init, NULL,				\
			      &ttc_data_##n, &ttc_config_##n,			\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,	\
			      &ttc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TTC_DEVICE_INIT)
