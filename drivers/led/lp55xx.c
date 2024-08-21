/*
 * Copyright (c) 2018 Workaround GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lp5562

/**
 * @file
 * @brief LP5562 LED driver
 *
 * The LP5562 is a 4-channel LED driver that communicates over I2C. The four
 * channels are expected to be connected to a red, green, blue and white LED.
 * Each LED can be driven by two different sources.
 *
 * 1. The brightness of each LED can be configured directly by setting a
 * register that drives the PWM of the connected LED.
 *
 * 2. A program can be transferred to the driver and run by one of the three
 * available execution engines. Up to 16 commands can be defined in each
 * program. Possible commands are:
 *   - Set the brightness.
 *   - Fade the brightness over time.
 *   - Loop parts of the program or the whole program.
 *   - Add delays.
 *   - Synchronize between the engines.
 *
 * After the program has been transferred, it can run infinitely without
 * communication between the host MCU and the driver.
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp5562);

/* Registers */
#define LP5562_ENABLE             0x00
#define LP5562_OP_MODE            0x01
#define LP5562_CONFIG             0x08
#define LP5562_ENG1_PC            0x09
#define LP5562_ENG2_PC            0x0A
#define LP5562_ENG3_PC            0x0B
#define LP5562_STATUS             0x0C
#define LP5562_RESET              0x0D
#define LP5562_PROG_MEM_ENG1_BASE 0x10
#define LP5562_PROG_MEM_ENG2_BASE 0x30
#define LP5562_PROG_MEM_ENG3_BASE 0x50

/*
 * The wait command has six bits for the number of steps (max 63) with up to
 * 15.6ms per step if the prescaler is set to 1. We round the step length
 * however to 16ms for easier handling, so the maximum blinking period is
 * therefore (16 * 63) = 1008ms. We round it down to 1000ms to be on the safe
 * side.
 */
#define LP5562_MAX_BLINK_PERIOD 1000
/*
 * The minimum waiting period is 0.49ms with the prescaler set to 0 and one
 * step. We round up to a full millisecond.
 */
#define LP5562_MIN_BLINK_PERIOD 1

/* Brightness limits in percent */
#define LP5562_MIN_BRIGHTNESS 0
#define LP5562_MAX_BRIGHTNESS 100

/* Output current limits in 0.1 mA */
#define LP5562_MIN_CURRENT_SETTING 0
#define LP5562_MAX_CURRENT_SETTING 255

/* Values for ENABLE register. */
#define LP5562_ENABLE_CHIP_EN_MASK (1 << 6)
#define LP5562_ENABLE_CHIP_EN_SET  (1 << 6)
#define LP5562_ENABLE_CHIP_EN_CLR  (0 << 6)
#define LP5562_ENABLE_LOG_EN       (1 << 7)

/* Values for CONFIG register. */
#define LP5562_CONFIG_EXTERNAL_CLOCK         0x00
#define LP5562_CONFIG_INTERNAL_CLOCK         0x01
#define LP5562_CONFIG_CLOCK_AUTOMATIC_SELECT 0x02
#define LP5562_CONFIG_PWRSAVE_EN             (1 << 5)
/* Enable 558 Hz frequency for PWM. Default is 256. */
#define LP5562_CONFIG_PWM_HW_FREQ_558        (1 << 6)

/* Values for execution engine programs. */
#define LP5562_PROG_COMMAND_SET_PWM (1 << 6)
#define LP5562_PROG_COMMAND_RAMP_TIME(prescale, step_time) \
	(((prescale) << 6) | (step_time))
#define LP5562_PROG_COMMAND_STEP_COUNT(fade_direction, count) \
	(((fade_direction) << 7) | (count))

/* Helper definitions. */
#define LP5562_PROG_MAX_COMMANDS 16
#define LP5562_MASK              0x03

/*
 * Each channel can be driven by directly assigning a value between 0 and 255 to
 * it to drive the PWM or by one of the three execution engines that can be
 * programmed for custom lighting patterns in order to reduce the I2C traffic
 * for repetitive patterns.
 */
enum lp5562_led_sources {
	LP5562_SOURCE_PWM,
	LP5562_SOURCE_ENGINE_1,
	LP5562_SOURCE_ENGINE_2,
	LP5562_SOURCE_ENGINE_3,

	LP5562_SOURCE_COUNT,
};

/* Operational modes of the execution engines. */
enum lp5562_engine_op_modes {
	LP5562_OP_MODE_DISABLED = 0x00,
	LP5562_OP_MODE_LOAD = 0x01,
	LP5562_OP_MODE_RUN = 0x02,
	LP5562_OP_MODE_DIRECT_CTRL = 0x03,
};

/* Execution state of the engines. */
enum lp5562_engine_exec_states {
	LP5562_ENGINE_MODE_HOLD = 0x00,
	LP5562_ENGINE_MODE_STEP = 0x01,
	LP5562_ENGINE_MODE_RUN = 0x02,
	LP5562_ENGINE_MODE_EXEC = 0x03,
};

/* Fading directions for programs executed by the engines. */
enum lp5562_engine_fade_dirs {
	LP5562_FADE_UP = 0x00,
	LP5562_FADE_DOWN = 0x01,
};

struct lp5562_interface {
	/* color_id to register_mapping */
	uint8_t pwm_reg_map[4];
	uint8_t current_reg_map[4];
	/* API to support engine handling */
	int (*get_available_engine)(const struct device *dev,
				    enum lp5562_led_sources *engine);
	int (*get_led_source)(const struct device *dev, uint8_t color_id,
			      enum lp5562_led_sources *source);
	int (*set_led_source)(const struct device *dev, uint8_t color_id,
			      enum lp5562_led_sources source);
};

struct lp5562_config {
	struct i2c_dt_spec bus;
	uint8_t wrgb_current[4];
	struct gpio_dt_spec enable_gpio;
	const struct lp5562_interface *iface;
};

/*
 * @brief Get the base address for programs of the given execution engine.
 *
 * @param engine    Engine the base address is requested for.
 * @param base_addr Pointer to the base address.
 *
 * @retval 0       On success.
 * @retval -EINVAL If a source is given that is not a valid engine.
 */
static int lp5562_get_engine_ram_base_addr(enum lp5562_led_sources engine,
					   uint8_t *base_addr)
{
	switch (engine) {
	case LP5562_SOURCE_ENGINE_1:
		*base_addr = LP5562_PROG_MEM_ENG1_BASE;
		break;
	case LP5562_SOURCE_ENGINE_2:
		*base_addr = LP5562_PROG_MEM_ENG2_BASE;
		break;
	case LP5562_SOURCE_ENGINE_3:
		*base_addr = LP5562_PROG_MEM_ENG3_BASE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * @brief Helper to get the register bit shift for the execution engines.
 *
 * The engine with the highest index is placed on the lowest two bits in the
 * OP_MODE and ENABLE registers.
 *
 * @param engine Engine the shift is requested for.
 * @param shift  Pointer to the shift value.
 *
 * @retval 0       On success.
 * @retval -EINVAL If a source is given that is not a valid engine.
 */
static int lp5562_get_engine_reg_shift(enum lp5562_led_sources engine,
				       uint8_t *shift)
{
	switch (engine) {
	case LP5562_SOURCE_ENGINE_1:
		*shift = 4U;
		break;
	case LP5562_SOURCE_ENGINE_2:
		*shift = 2U;
		break;
	case LP5562_SOURCE_ENGINE_3:
		*shift = 0U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * @brief Convert a time in milliseconds to a combination of prescale and
 *	step_time for the execution engine programs.
 *
 * This function expects the given time in milliseconds to be in the allowed
 * range the device can handle (0ms to 1000ms).
 *
 * @param ms        Time to be converted in milliseconds [0..1000].
 * @param prescale  Pointer to the prescale value.
 * @param step_time Pointer to the step_time value.
 */
static void lp5562_ms_to_prescale_and_step(uint32_t ms,
					   uint8_t *prescale, uint8_t *step_time)
{
	/*
	 * One step with the prescaler set to 0 takes 0.49ms. The max value for
	 * step_time is 63, so we just double the millisecond value. That way
	 * the step_time value never goes above the allowed 63.
	 */
	if (ms < 31) {
		*prescale = 0U;
		*step_time = ms << 1;

		return;
	}

	/*
	 * With a prescaler value set to 1 one step takes 15.6ms. So by dividing
	 * through 16 we get a decent enough result with low effort.
	 */
	*prescale = 1U;
	*step_time = ms >> 4;

	return;
}

#if CONFIG_DT_HAS_TI_LP5562_ENABLED

#define LP5562_B_PWM              0x02
#define LP5562_G_PWM              0x03
#define LP5562_R_PWM              0x04
#define LP5562_W_PWM              0x0E
#define LP5562_B_CURRENT          0x05
#define LP5562_G_CURRENT          0x06
#define LP5562_R_CURRENT          0x07
#define LP5562_W_CURRENT          0x0F
#define LP5562_LED_MAP            0x70

/*
 * @brief Assign a source to the given LED color_id.
 *
 * @param dev     LP5562 device.
 * @param color_id LED color_id the source is assigned to.
 * @param source  Source for the color_id.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_set_led_source(const struct device *dev,
				 uint8_t color_id,
				 enum lp5562_led_sources source)
{
	const struct lp5562_config *config = dev->config;
	/* LP5562 uses WRGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);

	if (i2c_reg_update_byte_dt(&config->bus, LP5562_LED_MAP,
				   LP5562_MASK << bit_pos,
				   source << bit_pos)) {
		LOG_ERR("Failed to set LED[%d] source=%d.", color_id, source);
		return -EIO;
	}

	return 0;
}

/*
 * @brief Get the assigned source of the given LED color_id.
 *
 * @param dev     LP5562 device.
 * @param color_id Requested LED color_id.
 * @param source  Pointer to the source of the color_id.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_get_led_source(const struct device *dev,
				 uint8_t color_id,
				 enum lp5562_led_sources *source)
{
	const struct lp5562_config *config = dev->config;
	uint8_t led_map;
	/* LP5562 uses WRGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);

	if (i2c_reg_read_byte_dt(&config->bus, LP5562_LED_MAP, &led_map)) {
		LOG_ERR("Failed to get LED[%d] source.", color_id);
		return -EIO;
	}

	*source = (led_map >> bit_pos) & LP5562_MASK;

	return 0;
}

/*
 * @brief Request whether an engine is currently running.
 *
 * @param dev    LP5562 device.
 * @param engine Engine to check.
 *
 * @return Indication of the engine execution state.
 *
 * @retval true  If the engine is currently running.
 * @retval false If the engine is not running or an error occurred.
 */
static bool lp5562_is_engine_executing(const struct device *dev,
				       enum lp5562_led_sources engine)
{
	const struct lp5562_config *config = dev->config;
	uint8_t enabled, shift;
	int ret;

	ret = lp5562_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return false;
	}

	if (i2c_reg_read_byte_dt(&config->bus, LP5562_ENABLE, &enabled)) {
		LOG_ERR("Failed to read ENABLE register.");
		return false;
	}

	enabled = (enabled >> shift) & LP5562_MASK;

	if (enabled == LP5562_ENGINE_MODE_RUN) {
		return true;
	}

	return false;
}

/*
 * @brief Get an available execution engine that is currently unused.
 *
 * @param dev    LP5562 device.
 * @param engine Pointer to the engine ID.
 *
 * @retval 0       On success.
 * @retval -ENODEV If all engines are busy.
 */
static int lp5562_get_available_engine(const struct device *dev,
				       enum lp5562_led_sources *engine)
{
	enum lp5562_led_sources src;

	for (src = LP5562_SOURCE_ENGINE_1; src < LP5562_SOURCE_COUNT; src++) {
		if (!lp5562_is_engine_executing(dev, src)) {
			LOG_DBG("Available engine: %d", src);
			*engine = src;
			return 0;
		}
	}

	LOG_ERR("No unused engine available");

	return -ENODEV;
}

/* LP5562 Interface definitions */
struct lp5562_interface lp5562_iface = {
	.pwm_reg_map = {LP5562_W_PWM, LP5562_R_PWM, LP5562_G_PWM, LP5562_B_PWM,},
	.current_reg_map = {LP5562_W_CURRENT, LP5562_R_CURRENT, LP5562_G_CURRENT, LP5562_B_CURRENT},
	.get_available_engine = lp5562_get_available_engine,
	.get_led_source = lp5562_get_led_source,
	.set_led_source = lp5562_set_led_source,
};

#endif /* CONFIG_DT_HAS_TI_LP5562_ENABLED */

/*
 * @brief Set an register shifted for the given execution engine.
 *
 * @param dev    LP5562 device.
 * @param engine Engine the value is shifted for.
 * @param reg    Register address to set.
 * @param val    Value to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_set_engine_reg(const struct device *dev,
				 enum lp5562_led_sources engine,
				 uint8_t reg, uint8_t val)
{
	const struct lp5562_config *config = dev->config;
	uint8_t shift;
	int ret;

	ret = lp5562_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return ret;
	}

	if (i2c_reg_update_byte_dt(&config->bus, reg, LP5562_MASK << shift,
				   val << shift)) {
		return -EIO;
	}

	return 0;
}

/*
 * @brief Set the operational mode of the given engine.
 *
 * @param dev    LP5562 device.
 * @param engine Engine the operational mode is changed for.
 * @param mode   Mode to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp5562_set_engine_op_mode(const struct device *dev,
					    enum lp5562_led_sources engine,
					    enum lp5562_engine_op_modes mode)
{
	return lp5562_set_engine_reg(dev, engine, LP5562_OP_MODE, mode);
}

/*
 * @brief Set the execution state of the given engine.
 *
 * @param dev    LP5562 device.
 * @param engine Engine the execution state is changed for.
 * @param state  State to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp5562_set_engine_exec_state(const struct device *dev,
					       enum lp5562_led_sources engine,
					       enum lp5562_engine_exec_states state)
{
	int ret;

	ret = lp5562_set_engine_reg(dev, engine, LP5562_ENABLE, state);

	/*
	 * Delay between consecutive I2C writes to
	 * ENABLE register (00h) need to be longer than 488μs (typ.).
	 */
	k_sleep(K_MSEC(1));

	return ret;
}

/*
 * @brief Start the execution of the program of the given engine.
 *
 * @param dev    LP5562 device.
 * @param engine Engine that is started.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp5562_start_program_exec(const struct device *dev,
					    enum lp5562_led_sources engine)
{
	if (lp5562_set_engine_op_mode(dev, engine, LP5562_OP_MODE_RUN)) {
		return -EIO;
	}

	return lp5562_set_engine_exec_state(dev, engine,
					    LP5562_ENGINE_MODE_RUN);
}

/*
 * @brief Stop the execution of the program of the given engine.
 *
 * @param dev    LP5562 device.
 * @param engine Engine that is stopped.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp5562_stop_program_exec(const struct device *dev,
					   enum lp5562_led_sources engine)
{
	if (lp5562_set_engine_op_mode(dev, engine, LP5562_OP_MODE_DISABLED)) {
		return -EIO;
	}

	return lp5562_set_engine_exec_state(dev, engine,
					    LP5562_ENGINE_MODE_HOLD);
}

static int lp5562_enter_pwm_mode(const struct device *dev, uint32_t led)
{
	const struct lp5562_config *config = dev->config;
	const struct lp5562_interface *iface = config->iface;
	int ret;
	enum lp5562_led_sources source;

	/* query current led source */
	ret = iface->get_led_source(dev, led, &source);
	if (ret) {
		return ret;
	}

	/* if source is linked to engine stop it, and switch to PWM */
	if (source != LP5562_SOURCE_PWM) {
		ret = lp5562_stop_program_exec(dev, source);
		if (ret) {
			LOG_ERR("Failed to stop engine=%d.", source);
			return ret;
		}
		ret = iface->set_led_source(dev, led, LP5562_SOURCE_PWM);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

static int lp5562_enter_engine_mode(const struct device *dev, uint32_t led,
				    enum lp5562_led_sources *engine)
{
	const struct lp5562_config *config = dev->config;
	const struct lp5562_interface *iface = config->iface;
	int ret;

	/* query current led source */
	ret = iface->get_led_source(dev, led, engine);
	if (ret) {
		return ret;
	}

	/* if source is PWM, and identify available engine and link it */
	if (*engine == LP5562_SOURCE_PWM) {
		ret = iface->get_available_engine(dev, engine);
		if (ret) {
			return ret;
		}
		ret = iface->set_led_source(dev, led, *engine);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

/*
 * @brief Program a command to the memory of the given execution engine.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine that is programmed.
 * @param command_index Index of the command that is programmed.
 * @param command_msb   Most significant byte of the command.
 * @param command_lsb   Least significant byte of the command.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the given command index is out of range or an invalid
 *		   engine is passed.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp5562_program_command(const struct device *dev,
				  enum lp5562_led_sources engine,
				  uint8_t command_index,
				  uint8_t command_msb,
				  uint8_t command_lsb)
{
	const struct lp5562_config *config = dev->config;
	uint8_t prog_base_addr;
	int ret;

	if (command_index >= LP5562_PROG_MAX_COMMANDS) {
		return -EINVAL;
	}

	ret = lp5562_get_engine_ram_base_addr(engine, &prog_base_addr);
	if (ret) {
		LOG_ERR("Failed to get base RAM address.");
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus,
				  prog_base_addr + (command_index << 1),
				  command_msb)) {
		LOG_ERR("Failed to update LED.");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->bus,
				  prog_base_addr + (command_index << 1) + 1,
				  command_lsb)) {
		LOG_ERR("Failed to update LED.");
		return -EIO;
	}

	return 0;
}

/*
 * @brief Program a command to set a fixed brightness to the given engine.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 * @param brightness    Brightness to be set for the LED in percent.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the passed arguments are invalid or out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp5562_program_set_brightness(const struct device *dev,
					 enum lp5562_led_sources engine,
					 uint8_t command_index,
					 uint8_t brightness)
{
	uint8_t val;

	if ((brightness < LP5562_MIN_BRIGHTNESS) ||
			(brightness > LP5562_MAX_BRIGHTNESS)) {
		return -EINVAL;
	}

	val = (brightness * 0xFF) / LP5562_MAX_BRIGHTNESS;

	return lp5562_program_command(dev, engine, command_index,
			LP5562_PROG_COMMAND_SET_PWM, val);
}

/*
 * @brief Program a command to ramp the brightness over time.
 *
 * In each step the PWM value is increased or decreased by 1/255th until the
 * maximum or minimum value is reached or step_count steps have been done.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 * @param time_per_step Time each step takes in milliseconds.
 * @param step_count    Number of steps to perform.
 * @param fade_dir      Direction of the ramp (in-/decrease brightness).
 *
 * @retval 0       On success.
 * @retval -EINVAL If the passed arguments are invalid or out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp5562_program_ramp(const struct device *dev,
			       enum lp5562_led_sources engine,
			       uint8_t command_index,
			       uint32_t time_per_step,
			       uint8_t step_count,
			       enum lp5562_engine_fade_dirs fade_dir)
{
	uint8_t prescale, step_time;

	if ((time_per_step < LP5562_MIN_BLINK_PERIOD) ||
			(time_per_step > LP5562_MAX_BLINK_PERIOD)) {
		return -EINVAL;
	}

	lp5562_ms_to_prescale_and_step(time_per_step,
			&prescale, &step_time);

	return lp5562_program_command(dev, engine, command_index,
			LP5562_PROG_COMMAND_RAMP_TIME(prescale, step_time),
			LP5562_PROG_COMMAND_STEP_COUNT(fade_dir, step_count));
}

/*
 * @brief Program a command to do nothing for the given time.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 * @param time          Time to do nothing in milliseconds.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the passed arguments are invalid or out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static inline int lp5562_program_wait(const struct device *dev,
				      enum lp5562_led_sources engine,
				      uint8_t command_index,
				      uint32_t time)
{
	/*
	 * A wait command is a ramp with the step_count set to 0. The fading
	 * direction does not matter in this case.
	 */
	return lp5562_program_ramp(dev, engine, command_index,
			time, 0, LP5562_FADE_UP);
}

/*
 * @brief Program a command to go back to the beginning of the program.
 *
 * Can be used at the end of a program to loop it infinitely.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the given command index is out of range or an invalid
 *		   engine is passed.
 * @retval -EIO    If the underlying I2C call fails.
 */
static inline int lp5562_program_go_to_start(const struct device *dev,
					     enum lp5562_led_sources engine,
					     uint8_t command_index)
{
	return lp5562_program_command(dev, engine, command_index, 0x00, 0x00);
}

static int lp5562_led_set_pwm_brightness(const struct device *dev, uint32_t color_id,
					 uint8_t value)
{
	const struct lp5562_config *config = dev->config;
	const struct lp5562_interface *iface = config->iface;
	uint8_t val = (value * 0xFF) / LP5562_MAX_BRIGHTNESS;

	if (color_id > LED_COLOR_ID_BLUE) {
		return -EINVAL;
	}

	if (i2c_reg_write_byte_dt(&config->bus, iface->pwm_reg_map[color_id], val)) {
		LOG_ERR("LED PWM write failed");
		return -EIO;
	}

	return 0;
}

/*
 * @brief Change the brightness of a running blink program.
 *
 * We know that the current program executes a blinking pattern
 * consisting of following commands:
 *
 * - set_brightness high
 * - wait on_delay
 * - set_brightness low
 * - wait off_delay
 * - return to start
 *
 * In order to change the brightness during blinking, we overwrite only
 * the first command and start execution again.
 *
 * @param dev           LP5562 device.
 * @param engine        Engine running the blinking program.
 * @param brightness_on New brightness value.
 *
 * @retval 0       On Success.
 * @retval -EINVAL If the engine ID or brightness is out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp5562_update_blinking_brightness(const struct device *dev,
					     enum lp5562_led_sources engine,
					     uint8_t brightness_on)
{
	int ret;

	ret = lp5562_stop_program_exec(dev, engine);
	if (ret) {
		return ret;
	}

	ret = lp5562_set_engine_op_mode(dev, engine, LP5562_OP_MODE_LOAD);
	if (ret) {
		return ret;
	}


	ret = lp5562_program_set_brightness(dev, engine, 0, brightness_on);
	if (ret) {
		return ret;
	}

	ret = lp5562_start_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to execute program.");
		return ret;
	}

	return 0;
}

static int lp5562_led_blink(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	int ret;
	enum lp5562_led_sources engine;
	uint8_t command_index = 0U;

	ret = lp5562_enter_engine_mode(dev, led, &engine);
	if (ret) {
		return ret;
	}

	ret = lp5562_set_engine_op_mode(dev, engine, LP5562_OP_MODE_LOAD);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_set_brightness(dev, engine, command_index,
			LP5562_MAX_BRIGHTNESS);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_wait(dev, engine, ++command_index, delay_on);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_set_brightness(dev, engine, ++command_index,
			LP5562_MIN_BRIGHTNESS);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_wait(dev, engine, ++command_index, delay_off);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_go_to_start(dev, engine, ++command_index);
	if (ret) {
		return ret;
	}

	ret = lp5562_start_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to execute program.");
		return ret;
	}

	return 0;
}

static int lp5562_led_set_brightness(const struct device *dev, uint32_t led,
				     uint8_t value)
{
	const struct lp5562_config *config = dev->config;
	const struct lp5562_interface *iface = config->iface;
	int ret;
	enum lp5562_led_sources current_source;

	if ((value < LP5562_MIN_BRIGHTNESS) ||
			(value > LP5562_MAX_BRIGHTNESS)) {
		return -EINVAL;
	}

	ret = iface->get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}

	if (current_source == LP5562_SOURCE_PWM) {
		return lp5562_led_set_pwm_brightness(dev, led, value);
	} else {
		return lp5562_update_blinking_brightness(dev, current_source, value);
	}
}

static inline int lp5562_led_on(const struct device *dev, uint32_t led)
{
	int ret = lp5562_enter_pwm_mode(dev, led);

	if (ret) {
		return ret;
	}
	return lp5562_led_set_pwm_brightness(dev, led, LP5562_MAX_BRIGHTNESS);
}

static inline int lp5562_led_off(const struct device *dev, uint32_t led)
{
	int ret = lp5562_enter_pwm_mode(dev, led);

	if (ret) {
		return ret;
	}
	return lp5562_led_set_pwm_brightness(dev, led, LP5562_MIN_BRIGHTNESS);
}

static int lp5562_led_update_current(const struct device *dev)
{
	const struct lp5562_config *config = dev->config;
	const struct lp5562_interface *iface = config->iface;
	const uint8_t *current = config->wrgb_current;
	int ret;

	for (uint8_t color_id = LED_COLOR_ID_WHITE; color_id <= LED_COLOR_ID_BLUE; color_id++) {
		ret = i2c_reg_write_byte_dt(&config->bus, iface->current_reg_map[color_id],
					    current[color_id]);
		if (ret) {
			LOG_ERR("Failed to set current of color %d", color_id);
			return ret;
		}
	}

	return 0;
}

static int lp5562_enable(const struct device *dev, bool soft_reset)
{
	const struct lp5562_config *config = dev->config;
	const struct gpio_dt_spec *enable_gpio = &config->enable_gpio;
	int err = 0;

	/* If ENABLE_GPIO control is enabled, we need to assert ENABLE_GPIO first. */
	if (enable_gpio->port != NULL) {
		err = gpio_pin_set_dt(enable_gpio, 1);
		if (err) {
			LOG_ERR("%s: failed to set enable GPIO 1", dev->name);
			return err;
		}
		/*
		 * The I2C host should allow at least 1ms before sending data to
		 * the LP5562 after the rising edge of the enable line.
		 * So let's wait for 1 ms.
		 */
		k_sleep(K_MSEC(1));
	}

	if (soft_reset) {
		/* Reset all internal registers to have a deterministic state. */
		err = i2c_reg_write_byte_dt(&config->bus, LP5562_RESET, 0xFF);
		if (err) {
			LOG_ERR("%s: failed to soft-reset device", dev->name);
			return err;
		}
	}

	/* Set en bit in LP5562_ENABLE register. */
	err = i2c_reg_update_byte_dt(&config->bus, LP5562_ENABLE, LP5562_ENABLE_CHIP_EN_MASK,
				     LP5562_ENABLE_CHIP_EN_SET);
	if (err) {
		LOG_ERR("%s: failed to set EN Bit in ENABLE register", dev->name);
		return err;
	}
	/* Allow 500 µs delay after setting chip_en bit to '1'. */
	k_sleep(K_USEC(500));
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lp5562_disable(const struct device *dev)
{
	const struct lp5562_config *config = dev->config;
	const struct gpio_dt_spec *enable_gpio = &config->enable_gpio;
	int err = 0;

	/* clear en bit in register configurations */
	err = i2c_reg_update_byte_dt(&config->bus, LP5562_ENABLE, LP5562_ENABLE_CHIP_EN_MASK,
				     LP5562_ENABLE_CHIP_EN_CLR);
	if (err) {
		LOG_ERR("%s: failed to clear EN Bit in ENABLE register", dev->name);
		return err;
	}

	/* if gpio control is enabled, we can de-assert EN_GPIO now */
	if (enable_gpio->port != NULL) {
		err = gpio_pin_set_dt(enable_gpio, 0);
		if (err) {
			LOG_ERR("%s: failed to set enable GPIO to 0", dev->name);
			return err;
		}
	}
	return 0;
}
#endif

static int lp5562_led_init(const struct device *dev)
{
	const struct lp5562_config *config = dev->config;
	const struct gpio_dt_spec *enable_gpio = &config->enable_gpio;
	int ret;

	if (enable_gpio->port != NULL) {
		if (!gpio_is_ready_dt(enable_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(enable_gpio, GPIO_OUTPUT);
		if (ret) {
			LOG_ERR("LP5562 Enable GPIO Config failed");
			return ret;
		}
	}

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ret = lp5562_enable(dev, true);
	if (ret) {
		return ret;
	}

	ret = lp5562_led_update_current(dev);
	if (ret) {
		LOG_ERR("Setting current setting LP5562 LED chip failed.");
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus, LP5562_CONFIG,
				  (LP5562_CONFIG_INTERNAL_CLOCK |
				   LP5562_CONFIG_PWRSAVE_EN))) {
		LOG_ERR("Configuring LP5562 LED chip failed.");
		return -EIO;
	}

	for (uint8_t color_id = LED_COLOR_ID_WHITE; color_id <= LED_COLOR_ID_BLUE; color_id++) {
		ret = lp5562_led_off(dev, color_id);
		if (ret) {
			LOG_ERR("Failed to set default state");
			return ret;
		}
	}

	return 0;
}

static const struct led_driver_api lp5562_led_api = {
	.blink = lp5562_led_blink,
	.set_brightness = lp5562_led_set_brightness,
	.on = lp5562_led_on,
	.off = lp5562_led_off,
};

#ifdef CONFIG_PM_DEVICE
static int lp5562_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return lp5562_disable(dev);
	case PM_DEVICE_ACTION_RESUME:
		return lp5562_enable(dev, false);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define LP5562_DEFINE(id)						\
	BUILD_ASSERT(DT_INST_PROP(id, red_output_current) <= LP5562_MAX_CURRENT_SETTING,\
		"Red channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP(id, green_output_current) <= LP5562_MAX_CURRENT_SETTING,\
		"Green channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP(id, blue_output_current) <= LP5562_MAX_CURRENT_SETTING,\
		"Blue channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP(id, white_output_current) <= LP5562_MAX_CURRENT_SETTING,\
		"White channel current must be between 0 and 25.5 mA.");	\
	static const struct lp5562_config lp5562_config_##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),			\
		.wrgb_current = {					\
			DT_INST_PROP(id, white_output_current),		\
			DT_INST_PROP(id, red_output_current),		\
			DT_INST_PROP(id, green_output_current),		\
			DT_INST_PROP(id, blue_output_current),		\
		},							\
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(id, enable_gpios, {0}),	\
		.iface = &lp5562_iface,					\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(id, lp5562_pm_action);			\
									\
	DEVICE_DT_INST_DEFINE(id, &lp5562_led_init, PM_DEVICE_DT_INST_GET(id),	\
			NULL,						\
			&lp5562_config_##id, POST_KERNEL,		\
			CONFIG_LED_INIT_PRIORITY,			\
			&lp5562_led_api);				\

DT_INST_FOREACH_STATUS_OKAY(LP5562_DEFINE)
