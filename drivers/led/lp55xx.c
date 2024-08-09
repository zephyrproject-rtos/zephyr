/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP55XX LED driver
 *
 * This LP55XX driver currently supports LP5521 and LP5562 which is a 3 and 4 channel LED driver
 * respectively that communicates over I2C.
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "lp55xx.h"

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lp55XX);

#include "led_context.h"

struct lp55xx_registers {
	uint8_t enable_reg;
	uint8_t op_mode_reg;
	uint8_t blue_pwm_reg;
	uint8_t green_pwm_reg;
	uint8_t red_pwm_reg;
	uint8_t white_pwm_reg;
	uint8_t blue_curr_reg;
	uint8_t green_curr_reg;
	uint8_t red_curr_reg;
	uint8_t white_curr_reg;
	uint8_t config_reg;
	uint8_t blue_engine_pc_reg;
	uint8_t green_engine_pc_reg;
	uint8_t red_engine_pc_reg;
	uint8_t status_reg;
	uint8_t reset_reg;
	uint8_t blue_prog_mem_base_reg;
	uint8_t green_prog_mem_base_reg;
	uint8_t red_prog_mem_base_reg;
	uint8_t led_map_reg;
};

#ifdef CONFIG_DT_HAS_TI_LP5562_ENABLED
static const struct lp55xx_registers lp5562_reg = {
	.enable_reg = LP5562_ENABLE,
	.op_mode_reg = LP5562_OP_MODE,
	.blue_pwm_reg = LP5562_B_PWM,
	.green_pwm_reg = LP5562_G_PWM,
	.red_pwm_reg = LP5562_R_PWM,
	.white_pwm_reg = LP5562_W_PWM,
	.blue_curr_reg = LP5562_B_CURRENT,
	.green_curr_reg = LP5562_G_CURRENT,
	.red_curr_reg = LP5562_R_CURRENT,
	.white_curr_reg = LP5562_W_CURRENT,
	.config_reg = LP5562_CONFIG,
	.blue_engine_pc_reg = LP5562_ENG1_PC,
	.green_engine_pc_reg = LP5562_ENG2_PC,
	.red_engine_pc_reg = LP5562_ENG3_PC,
	.status_reg = LP5562_STATUS,
	.reset_reg = LP5562_RESET,
	.blue_prog_mem_base_reg = LP5562_PROG_MEM_ENG1_BASE,
	.green_prog_mem_base_reg = LP5562_PROG_MEM_ENG2_BASE,
	.red_prog_mem_base_reg = LP5562_PROG_MEM_ENG3_BASE,
	.led_map_reg = LP5562_LED_MAP,
};
#endif

#ifdef CONFIG_DT_HAS_TI_LP5521_ENABLED
static const struct lp55xx_registers lp5521_reg = {
	.enable_reg = LP5521_ENABLE,
	.op_mode_reg = LP5521_OP_MODE,
	.red_pwm_reg = LP5521_R_PWM,
	.green_pwm_reg = LP5521_G_PWM,
	.blue_pwm_reg = LP5521_B_PWM,
	.white_pwm_reg = 0xFF, /* Not applicable for LP5521 */
	.red_curr_reg = LP5521_R_CURRENT,
	.green_curr_reg = LP5521_G_CURRENT,
	.blue_curr_reg = LP5521_B_CURRENT,
	.white_curr_reg = 0xFF, /* Not applicable for LP5521 */
	.config_reg = LP5521_CONFIG,
	.red_engine_pc_reg = LP5521_R_PC,
	.green_engine_pc_reg = LP5521_G_PC,
	.blue_engine_pc_reg = LP5521_B_PC,
	.status_reg = LP5521_STATUS,
	.reset_reg = LP5521_RESET,
	.blue_prog_mem_base_reg = LP5521_PROG_MEM_BLUE_ENG_BASE,
	.green_prog_mem_base_reg = LP5521_PROG_MEM_GREEN_ENG_BASE,
	.red_prog_mem_base_reg = LP5521_PROG_MEM_RED_ENG_BASE,
	.led_map_reg = 0xFF, /* Not applicable for LP5521 */
};
#endif

struct lp55xx_config {
	uint8_t r_current;
	uint8_t g_current;
	uint8_t b_current;
	uint8_t w_current;
	uint8_t total_leds;
	uint16_t id;
	struct i2c_dt_spec bus;
	const struct lp55xx_registers *registers;
	const struct gpio_dt_spec en_pin;
};

struct lp55xx_data {
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
static int lp55xx_get_pwm_reg(const struct lp55xx_config *config, enum lp55xx_led_channels channel,
			      uint8_t *reg)
{
	switch (channel) {
	case LP55XX_CHANNEL_W:
		*reg = config->registers->white_pwm_reg;
		break;
	case LP55XX_CHANNEL_R:
		*reg = config->registers->red_pwm_reg;
		break;
	case LP55XX_CHANNEL_G:
		*reg = config->registers->green_pwm_reg;
		break;
	case LP55XX_CHANNEL_B:
		*reg = config->registers->blue_pwm_reg;
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
static int lp55xx_get_engine_ram_base_addr(const struct lp55xx_config *config,
					   enum lp55xx_led_sources engine, uint8_t *base_addr)
{
	if (config->id == LP5562_ID) {
		switch (engine) {
		case LP5562_SOURCE_ENGINE_1:
			*base_addr = config->registers->blue_prog_mem_base_reg;
			break;
		case LP5562_SOURCE_ENGINE_2:
			*base_addr = config->registers->green_prog_mem_base_reg;
			;
			break;
		case LP5562_SOURCE_ENGINE_3:
			*base_addr = config->registers->red_prog_mem_base_reg;
			break;
		default:
			return -EINVAL;
		}
	} else if (config->id == LP5521_ID) {
		switch (engine) {
		case LP5562_SOURCE_ENGINE_1:
			*base_addr = config->registers->red_prog_mem_base_reg;
			break;
		case LP5562_SOURCE_ENGINE_2:
			*base_addr = config->registers->green_prog_mem_base_reg;
			;
			break;
		case LP5562_SOURCE_ENGINE_3:
			*base_addr = config->registers->blue_prog_mem_base_reg;
			;
			break;
		default:
			return -EINVAL;
		}
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
static int lp55xx_get_engine_reg_shift(enum lp55xx_led_sources engine, uint8_t *shift)
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
static void lp55xx_ms_to_prescale_and_step(struct led_data *data, uint32_t ms, uint8_t *prescale,
					   uint8_t *step_time)
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
}

/*
 * @brief Assign a source to the given LED channel.
 *
 * @param dev     LP55xx device.
 * @param channel LED channel the source is assigned to.
 * @param source  Source for the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp55xx_set_led_source(const struct device *dev, enum lp55xx_led_channels channel,
				 enum lp55xx_led_sources source)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t reg_addrs = 0;

	if (config->id == LP5562_ID) {
		reg_addrs = config->registers->led_map_reg;
	} else if (config->id == LP5521_ID) {
		reg_addrs = config->registers->op_mode_reg;
	}

	if (i2c_reg_update_byte_dt(&config->bus, reg_addrs, LP55XX_CHANNEL_MASK(channel),
				   source << (channel << 1))) {
		LOG_ERR("LED reg update failed.");
		return -EIO;
	}
	k_msleep(1); /* Must wait for next i2c write on OP_MODE reg */
	return 0;
}

/*
 * @brief Get the assigned source of the given LED channel.
 *
 * @param dev     LP55xx device.
 * @param channel Requested LED channel.
 * @param source  Pointer to the source of the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp55xx_get_led_source(const struct device *dev, enum lp55xx_led_channels channel,
				 enum lp55xx_led_sources *source)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t led_map;

	if (config->id == LP5562_ID) {
		if (i2c_reg_read_byte_dt(&config->bus, config->registers->led_map_reg, &led_map)) {
			return -EIO;
		}
		*source = (led_map >> (channel << 1)) & LP55XX_MASK;
	} else if (config->id == LP5521_ID) {
		/* Source is fixed in LP5521 */
		switch (channel) {
		case LP55XX_CHANNEL_B:
			*source = LP5562_SOURCE_ENGINE_3;
			break;
		case LP55XX_CHANNEL_R:
			*source = LP5562_SOURCE_ENGINE_1;
			break;
		case LP55XX_CHANNEL_G:
			*source = LP5562_SOURCE_ENGINE_2;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * @brief Request whether an engine is currently running.
 *
 * @param dev    LP55xx device.
 * @param engine Engine to check.
 *
 * @return Indication of the engine execution state.
 *
 * @retval true  If the engine is currently running.
 * @retval false If the engine is not running or an error occurred.
 */
static bool lp55xx_is_engine_executing(const struct device *dev, enum lp55xx_led_sources engine)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t enabled, shift;
	int ret;

	ret = lp55xx_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return false;
	}

	if (i2c_reg_read_byte_dt(&config->bus, config->registers->enable_reg, &enabled)) {
		LOG_ERR("Failed to read ENABLE register.");
		return false;
	}

	enabled = (enabled >> shift) & LP55XX_MASK;

	if (enabled == LP55XX_ENGINE_MODE_RUN) {
		return true;
	}

	return false;
}

/*
 * @brief Get an available execution engine that is currently unused.
 *
 * @param dev    LP55xx device.
 * @param engine Pointer to the engine ID.
 *
 * @retval 0       On success.
 * @retval -ENODEV If all engines are busy.
 */
static int lp55xx_get_available_engine(const struct device *dev, enum lp55xx_led_sources *engine,
				       uint32_t led)
{
	enum lp55xx_led_sources src;
	const struct lp55xx_config *config = dev->config;

	if (config->id == LP5521_ID) {
		/* This IC has fixed or non-mappable engines */
		switch (led) {
		case LP55XX_CHANNEL_B:
			*engine = LP5562_SOURCE_ENGINE_3;
			break;
		case LP55XX_CHANNEL_G:
			*engine = LP5562_SOURCE_ENGINE_2;
			break;
		case LP55XX_CHANNEL_R:
			*engine = LP5562_SOURCE_ENGINE_1;
			break;
		default:
			return -ENODEV;
		}
		return 0;
	}

	for (src = LP5562_SOURCE_ENGINE_1; src < LP5562_SOURCE_COUNT; src++) {
		if (!lp55xx_is_engine_executing(dev, src)) {
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
 * @param dev    LP55xx device.
 * @param engine Engine the value is shifted for.
 * @param reg    Register address to set.
 * @param val    Value to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp55xx_set_engine_reg(const struct device *dev, enum lp55xx_led_sources engine,
				 uint8_t reg, uint8_t val)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t shift;
	int ret;

	ret = lp55xx_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return ret;
	}

	if (i2c_reg_update_byte_dt(&config->bus, reg, LP55XX_MASK << shift, val << shift)) {
		return -EIO;
	}

	return 0;
}

/*
 * @brief Set the operational mode of the given engine.
 *
 * @param dev    LP55xx device.
 * @param engine Engine the operational mode is changed for.
 * @param mode   Mode to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp55xx_set_engine_op_mode(const struct device *dev,
					    enum lp55xx_led_sources engine,
					    enum lp55xx_engine_op_modes mode)
{
	const struct lp55xx_config *config = dev->config;

	return lp55xx_set_engine_reg(dev, engine, config->registers->op_mode_reg, mode);
}

/*
 * @brief Set the execution state of the given engine.
 *
 * @param dev    LP55xx device.
 * @param engine Engine the execution state is changed for.
 * @param state  State to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp55xx_set_engine_exec_state(const struct device *dev,
					       enum lp55xx_led_sources engine,
					       enum lp55xx_engine_exec_states state)
{
	int ret;
	const struct lp55xx_config *config = dev->config;

	ret = lp55xx_set_engine_reg(dev, engine, config->registers->enable_reg, state);

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
 * @param dev    LP55xx device.
 * @param engine Engine that is started.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp55xx_start_program_exec(const struct device *dev,
					    enum lp55xx_led_sources engine)
{
	if (lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_RUN)) {
		return -EIO;
	}

	return lp55xx_set_engine_exec_state(dev, engine, LP55XX_ENGINE_MODE_RUN);
}

/*
 * @brief Stop the execution of the program of the given engine.
 *
 * @param dev    LP55xx device.
 * @param engine Engine that is stopped.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp55xx_stop_program_exec(const struct device *dev, enum lp55xx_led_sources engine)
{
	if (lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_DISABLED)) {
		return -EIO;
	}

	return lp55xx_set_engine_exec_state(dev, engine, LP55XX_ENGINE_MODE_HOLD);
}

/*
 * @brief Program a command to the memory of the given execution engine.
 *
 * @param dev           LP55xx device.
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
static int lp55xx_program_command(const struct device *dev, enum lp55xx_led_sources engine,
				  uint8_t command_index, uint8_t command_msb, uint8_t command_lsb)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t prog_base_addr = 0;
	int ret;

	if (command_index >= LP5562_PROG_MAX_COMMANDS) {
		return -EINVAL;
	}

	ret = lp55xx_get_engine_ram_base_addr(config, engine, &prog_base_addr);
	if (ret) {
		LOG_ERR("Failed to get base RAM address.");
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus, prog_base_addr + (command_index << 1),
				  command_msb)) {
		LOG_ERR("Failed to update LED.");
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->bus, prog_base_addr + (command_index << 1) + 1,
				  command_lsb)) {
		LOG_ERR("Failed to update LED.");
		return -EIO;
	}

	return 0;
}

/*
 * @brief Program a command to set a fixed brightness to the given engine.
 *
 * @param dev           LP55xx device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 * @param brightness    Brightness to be set for the LED in percent.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the passed arguments are invalid or out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp55xx_program_set_brightness(const struct device *dev, enum lp55xx_led_sources engine,
					 uint8_t command_index, uint8_t brightness)
{
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t val;

	if ((brightness < dev_data->min_brightness) || (brightness > dev_data->max_brightness)) {
		return -EINVAL;
	}

	val = (brightness * 0xFF) / dev_data->max_brightness;

	return lp55xx_program_command(dev, engine, command_index, LP5562_PROG_COMMAND_SET_PWM, val);
}

/*
 * @brief Program a command to ramp the brightness over time.
 *
 * In each step the PWM value is increased or decreased by 1/255th until the
 * maximum or minimum value is reached or step_count steps have been done.
 *
 * @param dev           LP55xx device.
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
static int lp55xx_program_ramp(const struct device *dev, enum lp55xx_led_sources engine,
			       uint8_t command_index, uint32_t time_per_step, uint8_t step_count,
			       enum lp55xx_engine_fade_dirs fade_dir)
{
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	uint8_t prescale, step_time;

	if ((time_per_step < dev_data->min_period) || (time_per_step > dev_data->max_period)) {
		return -EINVAL;
	}

	lp55xx_ms_to_prescale_and_step(dev_data, time_per_step, &prescale, &step_time);

	return lp55xx_program_command(dev, engine, command_index,
				      LP5562_PROG_COMMAND_RAMP_TIME(prescale, step_time),
				      LP5562_PROG_COMMAND_STEP_COUNT(fade_dir, step_count));
}

/*
 * @brief Program a command to do nothing for the given time.
 *
 * @param dev           LP55xx device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 * @param time          Time to do nothing in milliseconds.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the passed arguments are invalid or out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static inline int lp55xx_program_wait(const struct device *dev, enum lp55xx_led_sources engine,
				      uint8_t command_index, uint32_t time)
{
	/*
	 * A wait command is a ramp with the step_count set to 0. The fading
	 * direction does not matter in this case.
	 */
	return lp55xx_program_ramp(dev, engine, command_index, time, 0, LP55XX_FADE_UP);
}

/*
 * @brief Program a command to go back to the beginning of the program.
 *
 * Can be used at the end of a program to loop it infinitely.
 *
 * @param dev           LP55xx device.
 * @param engine        Engine to be programmed.
 * @param command_index Index of the command in the program sequence.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the given command index is out of range or an invalid
 *		   engine is passed.
 * @retval -EIO    If the underlying I2C call fails.
 */
static inline int lp55xx_program_go_to_start(const struct device *dev,
					     enum lp55xx_led_sources engine, uint8_t command_index)
{
	return lp55xx_program_command(dev, engine, command_index, 0x00, 0x00);
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
 * @param dev           LP55xx device.
 * @param engine        Engine running the blinking program.
 * @param brightness_on New brightness value.
 *
 * @retval 0       On Success.
 * @retval -EINVAL If the engine ID or brightness is out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp55xx_update_blinking_brightness(const struct device *dev,
					     enum lp55xx_led_sources engine, uint8_t brightness_on)
{
	int ret;

	ret = lp55xx_stop_program_exec(dev, engine);
	if (ret) {
		return ret;
	}

	ret = lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_LOAD);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_set_brightness(dev, engine, 0, brightness_on);
	if (ret) {
		return ret;
	}

	ret = lp55xx_start_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to execute program.");
		return ret;
	}

	return 0;
}

static int lp55xx_led_blink(const struct device *dev, uint32_t led, uint32_t delay_on,
			    uint32_t delay_off)
{
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	const struct lp55xx_config *config = dev->config;
	int ret;
	enum lp55xx_led_sources engine;
	uint8_t command_index = 0U;

	if (led >= config->total_leds) {
		return -EINVAL;
	}

	ret = lp55xx_get_available_engine(dev, &engine, led);
	if (ret) {
		return ret;
	}

	ret = lp55xx_set_led_source(dev, led, engine);
	if (ret) {
		LOG_ERR("Failed to set LED source.");
		return ret;
	}

	ret = lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_LOAD);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_set_brightness(dev, engine, command_index, dev_data->max_brightness);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_wait(dev, engine, ++command_index, delay_on);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_set_brightness(dev, engine, ++command_index, dev_data->min_brightness);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_wait(dev, engine, ++command_index, delay_off);
	if (ret) {
		return ret;
	}

	ret = lp55xx_program_go_to_start(dev, engine, ++command_index);
	if (ret) {
		return ret;
	}

	ret = lp55xx_start_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to execute program.");
		return ret;
	}

	return 0;
}

static int lp55xx_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct lp55xx_config *config = dev->config;
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	int ret;
	uint8_t val, reg;
	enum lp55xx_led_sources current_source;

	if ((value < dev_data->min_brightness) || (value > dev_data->max_brightness) ||
	    (led >= config->total_leds)) {
		return -EINVAL;
	}

	ret = lp55xx_get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}
	if (current_source != LP5562_SOURCE_PWM) {
		if (lp55xx_is_engine_executing(dev, current_source)) {
			/*
			 * LED is blinking currently. Restart the blinking with
			 * the passed brightness.
			 */
			return lp55xx_update_blinking_brightness(dev, current_source, value);
		}
		if (config->id == LP5562_ID) {
			ret = lp55xx_set_led_source(dev, led, LP5562_SOURCE_PWM);
		} else if (config->id == LP5521_ID) {
			ret = lp55xx_set_led_source(
				dev, led, (enum lp55xx_led_sources)LP55XX_OP_MODE_DIRECT_CTRL);
		}
		if (ret) {
			return ret;
		}
	}

	val = (value * 0xFF) / dev_data->max_brightness;

	ret = lp55xx_get_pwm_reg(config, led, &reg);
	if (ret) {
		return ret;
	}
	if (i2c_reg_write_byte_dt(&config->bus, reg, val)) {
		LOG_ERR("LED write failed");
		return -EIO;
	}

	return 0;
}

static inline int lp55xx_led_on(const struct device *dev, uint32_t led)
{
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	const struct lp55xx_config *config = dev->config;

	if (led >= config->total_leds) {
		return -EINVAL;
	}
	return lp55xx_led_set_brightness(dev, led, dev_data->max_brightness);
}

static inline int lp55xx_led_off(const struct device *dev, uint32_t led)
{
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	const struct lp55xx_config *config = dev->config;
	int ret;
	enum lp55xx_led_sources current_source;

	if (led >= config->total_leds) {
		return -EINVAL;
	}

	ret = lp55xx_get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}

	if (current_source != LP5562_SOURCE_PWM) {
		ret = lp55xx_stop_program_exec(dev, current_source);
		if (ret) {
			return ret;
		}
	}

	return lp55xx_led_set_brightness(dev, led, dev_data->min_brightness);
}

static int lp55xx_led_update_current(const struct device *dev)
{
	const struct lp55xx_config *config = dev->config;
	int ret;
	uint8_t tx_buf[4];

	if (config->id == LP5562_ID) {
		uint8_t buf[4] = {config->registers->blue_curr_reg, config->b_current,
				  config->g_current, config->r_current};
		memcpy(tx_buf, buf, sizeof(buf));
	} else if (config->id == LP5521_ID) {
		uint8_t buf[4] = {config->registers->red_curr_reg, config->r_current,
				  config->g_current, config->b_current};
		memcpy(tx_buf, buf, sizeof(buf));
	}

	ret = i2c_write_dt(&config->bus, tx_buf, sizeof(tx_buf));
	if (ret) {
		return ret;
	}
	if (config->id == LP5562_ID) {
		ret = i2c_reg_write_byte_dt(&config->bus, config->registers->white_curr_reg,
					    config->w_current);
	}

	return ret;
}

static int lp55xx_led_init(const struct device *dev)
{
	const struct lp55xx_config *config = dev->config;
	struct lp55xx_data *data = dev->data;
	struct led_data *dev_data = &data->dev_data;
	const struct gpio_dt_spec *en_pin = &config->en_pin;
	int ret;

	if (en_pin != 0) {
		if (!device_is_ready(en_pin->port)) {
			return -ENODEV;
		}
		if (gpio_pin_configure_dt(en_pin, GPIO_OUTPUT_ACTIVE)) {
			return -ENODEV;
		}
		if (gpio_pin_set_dt(en_pin, true)) {
			return -ENODEV;
		}
		k_msleep(5);
	}

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	/* Hardware specific limits */
	if (config->id == LP5521_ID) {
		dev_data->min_period = LP5521_MIN_BLINK_PERIOD;
		dev_data->max_period = LP5521_MAX_BLINK_PERIOD;
		dev_data->min_brightness = LP5521_MIN_BRIGHTNESS;
		dev_data->max_brightness = LP5521_MAX_BRIGHTNESS;
	} else if (config->id == LP5562_ID) {
		dev_data->min_period = LP5562_MIN_BLINK_PERIOD;
		dev_data->max_period = LP5562_MAX_BLINK_PERIOD;
		dev_data->min_brightness = LP5562_MIN_BRIGHTNESS;
		dev_data->max_brightness = LP5562_MAX_BRIGHTNESS;
	}

	ret = lp55xx_led_update_current(dev);
	if (ret) {
		LOG_ERR("Setting current setting LP5562 LED chip failed,err:%d", ret);
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus, config->registers->enable_reg,
				  LP5562_ENABLE_CHIP_EN)) {
		LOG_ERR("Enabling LP5562 LED chip failed.");
		return -EIO;
	}

	if (config->id == LP5521_ID) {
		if (i2c_reg_write_byte_dt(&config->bus, config->registers->config_reg, 0x11)) {
			LOG_ERR("Configuring LP5562 LED chip failed.");
			return -EIO;
		}
		if (i2c_reg_write_byte_dt(&config->bus, config->registers->op_mode_reg, 0x3f)) {
			LOG_ERR("Disabling all engines failed.");
			return -EIO;
		}

	} else if (config->id == LP5562_ID) {
		if (i2c_reg_write_byte_dt(
			    &config->bus, config->registers->config_reg,
			    (LP5562_CONFIG_INTERNAL_CLOCK | LP5562_CONFIG_PWRSAVE_EN))) {
			LOG_ERR("Configuring LP5562 LED chip failed.");
			return -EIO;
		}
		if (i2c_reg_write_byte_dt(&config->bus, config->registers->op_mode_reg, 0x00)) {
			LOG_ERR("Disabling all engines failed.");
			return -EIO;
		}
		if (i2c_reg_write_byte_dt(&config->bus, config->registers->led_map_reg, 0x00)) {
			LOG_ERR("Setting all LEDs to manual control failed.");
			return -EIO;
		}
	}

	return 0;
}

static const struct led_driver_api lp55xx_led_api = {
	.blink = lp55xx_led_blink,
	.set_brightness = lp55xx_led_set_brightness,
	.on = lp55xx_led_on,
	.off = lp55xx_led_off,
};

#define LP55XX_DEFINE(n, idx)                                                                      \
	BUILD_ASSERT(DT_INST_PROP(n, red_output_current) <= LP##idx##_MAX_CURRENT_SETTING,         \
		     "Red channel current is above max limit.");                                   \
	BUILD_ASSERT(DT_INST_PROP(n, green_output_current) <= LP##idx##_MAX_CURRENT_SETTING,       \
		     "Green channel current is above max limit.");                                 \
	BUILD_ASSERT(DT_INST_PROP(n, blue_output_current) <= LP##idx##_MAX_CURRENT_SETTING,        \
		     "Blue channel current is above max limit.");                                  \
	BUILD_ASSERT(DT_INST_PROP_OR(n, white_output_current, 0) <= LP##idx##_MAX_CURRENT_SETTING, \
		     "White channel current is above max limit.");                                 \
                                                                                                   \
	static const struct lp55xx_config lp##idx##_config_##n = {                                 \
		.id = idx,                                                                         \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.r_current = DT_INST_PROP(n, red_output_current),                                  \
		.g_current = DT_INST_PROP(n, green_output_current),                                \
		.b_current = DT_INST_PROP(n, blue_output_current),                                 \
		.w_current = DT_INST_PROP_OR(n, white_output_current, 0),                          \
		.registers = &lp##idx##_reg,                                                       \
		.en_pin = GPIO_DT_SPEC_INST_GET_OR(n, en_gpios, {0}),                              \
		.total_leds = LP##idx##_MAX_LEDS,                                                  \
	};                                                                                         \
	struct lp55xx_data lp##idx##_data_##n;                                                     \
	DEVICE_DT_INST_DEFINE(n, &lp55xx_led_init, NULL, &lp##idx##_data_##n,                      \
			      &lp##idx##_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,        \
			      &lp55xx_led_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5562
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP55XX_DEFINE, LP5562_ID)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5521
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP55XX_DEFINE, LP5521_ID)
