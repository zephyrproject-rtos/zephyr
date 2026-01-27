/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_ttc_counter

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/gic.h>

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
#define TTC_MAX_INSTANCES           3

/* Global device array for shared interrupt handling */
static const struct device *ttc_devices[TTC_MAX_INSTANCES];

/* ==================== DATA STRUCTURES ==================== */

/**
 * @brief TTC device configuration structure
 */
struct ttc_config {
	struct counter_config_info info;
	void (*irq_config)(const struct device *dev);
	uintptr_t base;
	uint32_t clock_freq;
	uint32_t irq_num;
	uint8_t timer_id;
	bool prescaler_present;
	uint8_t prescaler;
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
 * @brief Calculate register offset for specific timer
 * @param timer_id Timer identifier (0-2)
 * @param base_offset Base register offset
 * @return Absolute register offset
 */
static uint32_t ttc_get_register_offset(uint8_t timer_id, uint32_t base_offset)
{
	return base_offset + (timer_id * 4);
}

/**
 * @brief Read TTC register
 * @param dev Pointer to device structure
 * @param offset Register offset
 * @return Register value
 */
static inline uint32_t ttc_read_reg(const struct device *dev, uint32_t offset)
{
	const struct ttc_config *config = dev->config;

	return sys_read32(config->base + offset);
}

/**
 * @brief Write TTC register
 * @param dev Pointer to device structure
 * @param offset Register offset
 * @param value Value to write
 */
static inline void ttc_write_reg(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct ttc_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

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
 * @param timer_id Timer identifier (0-2)
 * @param channel Channel identifier (0-2)
 * @return Match register offset
 */
static uint32_t ttc_get_match_register(uint8_t timer_id, uint8_t channel)
{
	uint32_t timer1_matches[3] = {
		TTC_MATCH_1_COUNTER_1,
		TTC_MATCH_2_COUNTER_1,
		TTC_MATCH_3_COUNTER_1
	};

	if (channel >= TTC_MAX_CHANNELS) {
		return 0;
	}

	return timer1_matches[channel] + (timer_id * 4);
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
	uint32_t ctrl_offset;
	uint32_t ctrl_val;

	ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_CONTROL_1);
	ctrl_val = ttc_read_reg(dev, ctrl_offset);
	ctrl_val |= TTC_CNT_CTRL_RST;
	ctrl_val &= ~TTC_CNT_CTRL_DIS;
	ttc_write_reg(dev, ctrl_offset, ctrl_val);

	LOG_DBG("Started TTC timer %d", config->timer_id);

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
	uint32_t ctrl_offset;
	uint32_t ctrl_val;

	ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_CONTROL_1);
	ctrl_val = ttc_read_reg(dev, ctrl_offset);
	ctrl_val |= TTC_CNT_CTRL_DIS;
	ttc_write_reg(dev, ctrl_offset, ctrl_val);

	LOG_DBG("Stopped TTC timer %d", config->timer_id);

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
	uint32_t value_offset;

	if (!ticks) {
		return -EINVAL;
	}

	value_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_VALUE_1);
	*ticks = ttc_read_reg(dev, value_offset);

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
	uint32_t interval_offset;
	uint32_t ctrl_offset;
	uint32_t int_en_offset;
	uint32_t ctrl_val;
	uint32_t int_en_val;

	if (!cfg) {
		return -EINVAL;
	}

	int_en_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_ENABLE_1);
	int_en_val = ttc_read_reg(dev, int_en_offset);

	if (cfg->ticks == 0 || cfg->ticks > config->info.max_top_value) {
		LOG_ERR("Invalid top value %u", cfg->ticks);
		return -EINVAL;
	}

	LOG_DBG("Setting top value to %u", cfg->ticks);

	interval_offset = ttc_get_register_offset(config->timer_id, TTC_INTERVAL_COUNTER_1);
	ttc_write_reg(dev, interval_offset, cfg->ticks);
	ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_CONTROL_1);
	ctrl_val = ttc_read_reg(dev, ctrl_offset);
	ctrl_val |= TTC_CNT_CTRL_INT;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		ctrl_val |= TTC_CNT_CTRL_RST;
	}

	ttc_write_reg(dev, ctrl_offset, ctrl_val);

	data->top_value = cfg->ticks;
	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (cfg->callback) {
		int_en_val |= TTC_INT_IV;
		ttc_write_reg(dev, int_en_offset, int_en_val);
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
	uint32_t int_en_offset;
	uint32_t int_en_val;

	if (chan_id >= TTC_MAX_CHANNELS) {
		return -ENOTSUP;
	}

	LOG_DBG("Cancelling alarm %d", chan_id);

	data->alarm_callbacks[chan_id] = NULL;
	data->alarm_user_data[chan_id] = NULL;

	int_en_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_ENABLE_1);
	int_en_val = ttc_read_reg(dev, int_en_offset);
	int_en_val &= ~ttc_get_interrupt_bit(chan_id);
	ttc_write_reg(dev, int_en_offset, int_en_val);

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
	uint32_t int_en_offset;
	uint32_t ctrl_offset;
	uint32_t alarm_ticks;
	uint32_t current_ticks;
	uint32_t int_en_val;
	uint32_t ctrl_val;
	int ret;

	if (chan_id >= TTC_MAX_CHANNELS) {
		LOG_ERR("Invalid channel %d", chan_id);
		return -ENOTSUP;
	}

	if (!alarm_cfg || !alarm_cfg->callback) {
		return -EINVAL;
	}

	alarm_ticks = alarm_cfg->ticks;

	/* Get current counter value - needed for both relative and absolute alarms */
	ret = ttc_get_value(dev, &current_ticks);
	if (ret != 0) {
		return ret;
	}

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		/* Relative alarm - validate against top value if set, else max value */
		uint32_t limit = data->top_value > 0 ? data->top_value :
			config->info.max_top_value;
		if (alarm_cfg->ticks > limit) {
			LOG_ERR("Relative alarm ticks %u > limit %u", alarm_cfg->ticks, limit);
			return -EINVAL;
		}

		alarm_ticks += current_ticks;

		/* Handle wrap-around for relative alarms */
		if (data->top_value > 0 && alarm_ticks > data->top_value) {
			alarm_ticks %= (data->top_value + 1);
		}
	} else {
		/* Absolute alarm - validate against current top value */
		if (data->top_value > 0 && alarm_cfg->ticks > data->top_value) {
			LOG_ERR("Alarm ticks %u > top value %u",
				alarm_cfg->ticks, data->top_value);
			return -EINVAL;
		}
	}

	/*
	 * Check for late alarms before writing match register.
	 * An alarm is late if the distance from current to (alarm - 1) exceeds max_rel_val.
	 */
	uint32_t top = ttc_get_top_value(dev);
	uint32_t max_rel_val = data->guard_period_set ?
		(top - data->guard_period) : top;
	uint32_t target = (alarm_ticks == 0) ? top : (alarm_ticks - 1);
	uint32_t diff;

	/* Calculate distance from current_ticks to target (with wraparound) */
	if (current_ticks <= target) {
		diff = target - current_ticks;
	} else {
		diff = target + top + 1 - current_ticks;
	}

	/* Check if alarm is already late or matches current value */
	if (diff > max_rel_val || alarm_ticks == current_ticks) {
		int err = 0;
		bool is_absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
		bool expire_when_late = alarm_cfg->flags &
			COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
		if (is_absolute) {
			err = -ETIME;
		}

		/* Trigger interrupt immediately if alarm has EXPIRE_WHEN_LATE flag
		 * or if alarm_ticks == current_ticks (hardware won't fire)
		 */
		bool irq_on_late = expire_when_late || (alarm_ticks == current_ticks);

		if (irq_on_late) {
			data->alarm_callbacks[chan_id] = alarm_cfg->callback;
			data->alarm_user_data[chan_id] = alarm_cfg->user_data;
			ttc_set_alarm_pending(dev, chan_id);
			return err;
		}
		/* Absolute alarms without EXPIRE_WHEN_LATE return error */
		if (is_absolute) {
			return err;
		}
		/* Relative alarms without EXPIRE_WHEN_LATE continue to set match register
		 * (alarm will fire on next cycle when counter wraps)
		 */
	}

	match_offset = ttc_get_match_register(config->timer_id, chan_id);
	if (match_offset == 0) {
		return -ENOTSUP;
	}

	/* Write match register */
	ttc_write_reg(dev, match_offset, alarm_ticks);

	data->alarm_callbacks[chan_id] = alarm_cfg->callback;
	data->alarm_user_data[chan_id] = alarm_cfg->user_data;

	ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_CONTROL_1);
	ctrl_val = ttc_read_reg(dev, ctrl_offset);
	ctrl_val |= TTC_CNT_CTRL_MATCH;
	ttc_write_reg(dev, ctrl_offset, ctrl_val);

	/* Normal case - enable interrupt */
	int_en_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_ENABLE_1);
	int_en_val = ttc_read_reg(dev, int_en_offset);
	int_en_val |= ttc_get_interrupt_bit(chan_id);
	ttc_write_reg(dev, int_en_offset, int_en_val);

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
	uint32_t int_reg_offset;

	int_reg_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_REGISTER_1);
	return ttc_read_reg(dev, int_reg_offset) ? 1 : 0;
}

/* ==================== INTERRUPT HANDLER ==================== */

/**
 * @brief Process interrupts for a single TTC timer
 * @param dev Pointer to device structure
 * @param timer_id Timer ID (0-2) within the TTC IP
 * @param int_status Interrupt status register value (already read by ISR)
 */
static void ttc_process_timer_interrupt(const struct device *dev, uint8_t timer_id,
					uint32_t int_status)
{
	struct ttc_data *data = dev->data;
	uint32_t current_ticks;
	uint32_t int_en_offset;
	uint32_t int_en_val;

	/* Check for software-triggered late alarms */
	uint32_t sw_pending = atomic_and(&data->late_alarm_pending, 0);

	/* Handle interval interrupt */
	if (int_status & TTC_INT_IV) {
		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}
	}

	/* Handle match interrupts (hardware or software-triggered) */
	for (int i = 0; i < TTC_MAX_CHANNELS; i++) {
		uint32_t match_bit = ttc_get_interrupt_bit(i);
		bool hw_pending = (int_status & match_bit) != 0;
		bool sw_late_alarm = (sw_pending & BIT(i)) != 0;

		if ((hw_pending || sw_late_alarm) && data->alarm_callbacks[i]) {
			ttc_get_value(dev, &current_ticks);
			data->alarm_callbacks[i](dev, i, current_ticks,
						data->alarm_user_data[i]);

			int_en_offset = ttc_get_register_offset(timer_id, TTC_INTERRUPT_ENABLE_1);
			int_en_val = ttc_read_reg(dev, int_en_offset);
			int_en_val &= ~match_bit;
			ttc_write_reg(dev, int_en_offset, int_en_val);

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
	uintptr_t base = config->base;
	uint32_t int_reg_offset;
	uint32_t int_status;

	for (uint8_t timer_id = 0; timer_id < TTC_MAX_INSTANCES; timer_id++) {
		const struct device *timer_dev = ttc_devices[timer_id];

		if (timer_dev == NULL) {
			continue;
		}
		const struct ttc_config *timer_cfg = timer_dev->config;

		if (timer_cfg->base != base) {
			continue;
		}
		int_reg_offset = ttc_get_register_offset(timer_cfg->timer_id,
							 TTC_INTERRUPT_REGISTER_1);
		int_status = sys_read32(base + int_reg_offset);
		/*
		 * Process timer interrupt even if int_status is 0, because there might be
		 * software-triggered late alarms pending (via arm_gic_irq_set_pending).
		 */
		ttc_process_timer_interrupt(timer_dev, timer_cfg->timer_id, int_status);
	}
}

/* ==================== DRIVER API STRUCTURE ==================== */

static const struct counter_driver_api ttc_driver_api = {
	.start = ttc_start,
	.stop = ttc_stop,
	.get_value = ttc_get_value,
	.get_value_64 = NULL,
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
 * @brief Initialize TTC timer device
 * @param dev Pointer to device structure
 * @return 0 on success, negative errno otherwise
 */
static int ttc_init(const struct device *dev)
{
	const struct ttc_config *config = dev->config;
	struct ttc_data *data = dev->data;
	uint32_t clk_ctrl_offset;
	uint32_t ctrl_offset;
	uint32_t int_reg_offset;
	uint32_t int_en_offset;
	uint32_t clk_ctrl_val;
	uint32_t ctrl_val;

	/* Register this device in global array for shared interrupt handling */
	if (config->timer_id < TTC_MAX_INSTANCES) {
		ttc_devices[config->timer_id] = dev;
	}

	memset(data, 0, sizeof(*data));
	data->top_value = 0;
	clk_ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_CLOCK_CONTROL_1);
	clk_ctrl_val = 0;

	if (config->prescaler_present) {
		if (config->prescaler >= 0 && config->prescaler <= TTC_MAX_PRESCALER) {
			clk_ctrl_val |= TTC_CLK_CTRL_PS_EN;
			clk_ctrl_val |= (config->prescaler << TTC_CLK_CTRL_PS_V_SHIFT) &
				TTC_CLK_CTRL_PS_V_MASK;
		}
	}

	ttc_write_reg(dev, clk_ctrl_offset, clk_ctrl_val);

	ctrl_offset = ttc_get_register_offset(config->timer_id, TTC_COUNTER_CONTROL_1);
	ctrl_val = TTC_CNT_CTRL_DIS | TTC_CNT_CTRL_RST;
	ttc_write_reg(dev, ctrl_offset, ctrl_val);

	int_reg_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_REGISTER_1);
	(void)ttc_read_reg(dev, int_reg_offset);

	int_en_offset = ttc_get_register_offset(config->timer_id, TTC_INTERRUPT_ENABLE_1);
	ttc_write_reg(dev, int_en_offset, 0);

	config->irq_config(dev);

	return 0;
}

/* ==================== DEVICE INSTANTIATION ==================== */

/*
 * Check if instance n shares IRQ with instance 0 (only when both enabled)
 * Returns 1 if n should NOT connect (IRQ shared with instance 0)
 */
#define TTC_IRQ_SHARED_WITH_INST0(n) \
	UTIL_AND(DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0)), \
	UTIL_AND(UTIL_NOT(IS_EQ(n, 0)), \
		 IS_EQ(DT_INST_IRQN(n), DT_INST_IRQN(0))))

/*
 * Check if instance n shares IRQ with instance 1 (only when both enabled)
 * Returns 1 if n should NOT connect (IRQ shared with instance 1)
 */
#define TTC_IRQ_SHARED_WITH_INST1(n) \
	UTIL_AND(DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1)), \
	UTIL_AND(IS_EQ(n, 2), \
		 IS_EQ(DT_INST_IRQN(n), DT_INST_IRQN(1))))

/*
 * Returns 1 if instance n should connect its IRQ, 0 otherwise
 * Instance 0 always connects. Other instances only connect if they have
 * a different IRQ number than lower-numbered instances.
 */
#define TTC_SHOULD_CONNECT_IRQ(n) \
	UTIL_OR(IS_EQ(n, 0), \
		UTIL_NOT(UTIL_OR(TTC_IRQ_SHARED_WITH_INST0(n), \
				 TTC_IRQ_SHARED_WITH_INST1(n))))
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
	static void ttc_irq_config_##n(const struct device *dev)		\
	{									\
		COND_CODE_1(TTC_SHOULD_CONNECT_IRQ(n), (			\
			IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
				    ttc_isr, DEVICE_DT_INST_GET(n), 0);		\
			irq_enable(DT_INST_IRQN(n));				\
		), ())								\
	}									\
										\
	static const struct ttc_config ttc_config_##n = {			\
		.base = DT_INST_REG_ADDR(n),					\
		.clock_freq = DT_INST_PROP(n, clock_frequency),			\
		.timer_id = DT_INST_PROP(n, timer_id),				\
		.prescaler_present = DT_INST_NODE_HAS_PROP(n, prescaler),       \
		.prescaler = DT_INST_PROP_OR(n, prescaler, 0),			\
		.irq_num = DT_INST_IRQN(n),					\
		.irq_config = ttc_irq_config_##n,				\
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
