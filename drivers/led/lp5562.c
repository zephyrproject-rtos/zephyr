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
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp5562);

#include "led_context.h"

/* Registers */
#define LP5562_ENABLE             0x00
#define LP5562_OP_MODE            0x01
#define LP5562_B_PWM              0x02
#define LP5562_G_PWM              0x03
#define LP5562_R_PWM              0x04
#define LP5562_B_CURRENT          0x05
#define LP5562_G_CURRENT          0x06
#define LP5562_R_CURRENT          0x07
#define LP5562_CONFIG             0x08
#define LP5562_ENG1_PC            0x09
#define LP5562_ENG2_PC            0x0A
#define LP5562_ENG3_PC            0x0B
#define LP5562_STATUS             0x0C
#define LP5562_RESET              0x0D
#define LP5562_W_PWM              0x0E
#define LP5562_W_CURRENT          0x0F
#define LP5562_PROG_MEM_ENG1_BASE 0x10
#define LP5562_PROG_MEM_ENG2_BASE 0x30
#define LP5562_PROG_MEM_ENG3_BASE 0x50
#define LP5562_LED_MAP            0x70

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

/* Values for ENABLE register. */
#define LP5562_ENABLE_CHIP_EN (1 << 6)
#define LP5562_ENABLE_LOG_EN  (1 << 7)

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
#define LP5562_CHANNEL_MASK(channel) ((LP5562_MASK) << (channel << 1))

/*
 * Available channels. There are four LED channels usable with the LP5562. While
 * they can be mapped to LEDs of any color, the driver's typical application is
 * with a red, a green, a blue and a white LED. Since the data sheet's
 * nomenclature uses RGBW, we keep it that way.
 */
enum lp5562_led_channels {
	LP5562_CHANNEL_B,
	LP5562_CHANNEL_G,
	LP5562_CHANNEL_R,
	LP5562_CHANNEL_W,

	LP5562_CHANNEL_COUNT,
};

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

struct lp5562_config {
	struct i2c_dt_spec bus;
};

struct lp5562_data {
	struct led_data dev_data;
};

/*
 * @brief Get the register for the given LED channel used to directly write a
 *	brightness value instead of using the execution engines.
 *
 * @param channel LED channel.
 * @param reg     Pointer to the register address.
 *
 * @retval 0       On success.
 * @retval -EINVAL If an invalid channel is given.
 */
static int lp5562_get_pwm_reg(enum lp5562_led_channels channel, uint8_t *reg)
{
	switch (channel) {
	case LP5562_CHANNEL_W:
		*reg = LP5562_W_PWM;
		break;
	case LP5562_CHANNEL_R:
		*reg = LP5562_R_PWM;
		break;
	case LP5562_CHANNEL_G:
		*reg = LP5562_G_PWM;
		break;
	case LP5562_CHANNEL_B:
		*reg = LP5562_B_PWM;
		break;
	default:
		LOG_ERR("Invalid channel given.");
		return -EINVAL;
	}

	return 0;
}

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
 * @param data      Capabilities of the driver.
 * @param ms        Time to be converted in milliseconds [0..1000].
 * @param prescale  Pointer to the prescale value.
 * @param step_time Pointer to the step_time value.
 */
static void lp5562_ms_to_prescale_and_step(struct led_data *data, uint32_t ms,
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

/*
 * @brief Assign a source to the given LED channel.
 *
 * @param dev     LP5562 device.
 * @param channel LED channel the source is assigned to.
 * @param source  Source for the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_set_led_source(const struct device *dev,
				 enum lp5562_led_channels channel,
				 enum lp5562_led_sources source)
{
	const struct lp5562_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->bus, LP5562_LED_MAP,
				   LP5562_CHANNEL_MASK(channel),
				   source << (channel << 1))) {
		LOG_ERR("LED reg update failed.");
		return -EIO;
	}

	return 0;
}

/*
 * @brief Get the assigned source of the given LED channel.
 *
 * @param dev     LP5562 device.
 * @param channel Requested LED channel.
 * @param source  Pointer to the source of the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_get_led_source(const struct device *dev,
				 enum lp5562_led_channels channel,
				 enum lp5562_led_sources *source)
{
	const struct lp5562_config *config = dev->config;
	uint8_t led_map;

	if (i2c_reg_read_byte_dt(&config->bus, LP5562_LED_MAP, &led_map)) {
		return -EIO;
	}

	*source = (led_map >> (channel << 1)) & LP5562_MASK;

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
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t val;

	if ((brightness < dev_data->min_brightness) ||
			(brightness > dev_data->max_brightness)) {
		return -EINVAL;
	}

	val = (brightness * 0xFF) / dev_data->max_brightness;

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
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t prescale, step_time;

	if ((time_per_step < dev_data->min_period) ||
			(time_per_step > dev_data->max_period)) {
		return -EINVAL;
	}

	lp5562_ms_to_prescale_and_step(dev_data, time_per_step,
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
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	int ret;
	enum lp5562_led_sources engine;
	uint8_t command_index = 0U;

	ret = lp5562_get_available_engine(dev, &engine);
	if (ret) {
		return ret;
	}

	ret = lp5562_set_led_source(dev, led, engine);
	if (ret) {
		LOG_ERR("Failed to set LED source.");
		return ret;
	}

	ret = lp5562_set_engine_op_mode(dev, engine, LP5562_OP_MODE_LOAD);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_set_brightness(dev, engine, command_index,
			dev_data->max_brightness);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_wait(dev, engine, ++command_index, delay_on);
	if (ret) {
		return ret;
	}

	ret = lp5562_program_set_brightness(dev, engine, ++command_index,
			dev_data->min_brightness);
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
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	int ret;
	uint8_t val, reg;
	enum lp5562_led_sources current_source;

	if ((value < dev_data->min_brightness) ||
			(value > dev_data->max_brightness)) {
		return -EINVAL;
	}

	ret = lp5562_get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}

	if (current_source != LP5562_SOURCE_PWM) {
		if (lp5562_is_engine_executing(dev, current_source)) {
			/*
			 * LED is blinking currently. Restart the blinking with
			 * the passed brightness.
			 */
			return lp5562_update_blinking_brightness(dev,
					current_source, value);
		}

		ret = lp5562_set_led_source(dev, led, LP5562_SOURCE_PWM);
		if (ret) {
			return ret;
		}
	}

	val = (value * 0xFF) / dev_data->max_brightness;

	ret = lp5562_get_pwm_reg(led, &reg);
	if (ret) {
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus, reg, val)) {
		LOG_ERR("LED write failed");
		return -EIO;
	}

	return 0;
}

static inline int lp5562_led_on(const struct device *dev, uint32_t led)
{
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	return lp5562_led_set_brightness(dev, led, dev_data->max_brightness);
}

static inline int lp5562_led_off(const struct device *dev, uint32_t led)
{
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	int ret;
	enum lp5562_led_sources current_source;

	ret = lp5562_get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}

	if (current_source != LP5562_SOURCE_PWM) {
		ret = lp5562_stop_program_exec(dev, current_source);
		if (ret) {
			return ret;
		}
	}

	return lp5562_led_set_brightness(dev, led, dev_data->min_brightness);
}

static int lp5562_led_init(const struct device *dev)
{
	const struct lp5562_config *config = dev->config;
	struct lp5562_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	/* Hardware specific limits */
	dev_data->min_period = LP5562_MIN_BLINK_PERIOD;
	dev_data->max_period = LP5562_MAX_BLINK_PERIOD;
	dev_data->min_brightness = LP5562_MIN_BRIGHTNESS;
	dev_data->max_brightness = LP5562_MAX_BRIGHTNESS;

	if (i2c_reg_write_byte_dt(&config->bus, LP5562_ENABLE,
				  LP5562_ENABLE_CHIP_EN)) {
		LOG_ERR("Enabling LP5562 LED chip failed.");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->bus, LP5562_CONFIG,
				  (LP5562_CONFIG_INTERNAL_CLOCK |
				   LP5562_CONFIG_PWRSAVE_EN))) {
		LOG_ERR("Configuring LP5562 LED chip failed.");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->bus, LP5562_OP_MODE, 0x00)) {
		LOG_ERR("Disabling all engines failed.");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->bus, LP5562_LED_MAP, 0x00)) {
		LOG_ERR("Setting all LEDs to manual control failed.");
		return -EIO;
	}

	return 0;
}

static const struct led_driver_api lp5562_led_api = {
	.blink = lp5562_led_blink,
	.set_brightness = lp5562_led_set_brightness,
	.on = lp5562_led_on,
	.off = lp5562_led_off,
};

#define LP5562_DEFINE(id)						\
	static const struct lp5562_config lp5562_config_##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),			\
	};								\
									\
	struct lp5562_data lp5562_data_##id;				\
	DEVICE_DT_INST_DEFINE(id, &lp5562_led_init, NULL,		\
			&lp5562_data_##id,				\
			&lp5562_config_##id, POST_KERNEL,		\
			CONFIG_LED_INIT_PRIORITY,			\
			&lp5562_led_api);				\

DT_INST_FOREACH_STATUS_OKAY(LP5562_DEFINE)
