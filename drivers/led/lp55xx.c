/*
 * Copyright (c) 2018 Workaround GmbH
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LP55XX LED driver
 *
 * The LP55XX is a 4-channel LED driver that communicates over I2C. The four
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
LOG_MODULE_REGISTER(lp55xx);

/* Registers */
#define LP55XX_ENABLE             0x00
#define LP55XX_OP_MODE            0x01
#define LP55XX_CONFIG             0x08
#define LP55XX_ENG1_PC            0x09
#define LP55XX_ENG2_PC            0x0A
#define LP55XX_ENG3_PC            0x0B
#define LP55XX_STATUS             0x0C
#define LP55XX_RESET              0x0D
#define LP55XX_PROG_MEM_ENG1_BASE 0x10
#define LP55XX_PROG_MEM_ENG2_BASE 0x30
#define LP55XX_PROG_MEM_ENG3_BASE 0x50
#define LP55XX_PROG_MEM_SIZE      0x20

/*
 * The wait command has six bits for the number of steps (max 63) with up to
 * 15.6ms per step if the prescaler is set to 1. We round the step length
 * however to 16ms for easier handling, so the maximum blinking period is
 * therefore (16 * 63) = 1008ms. We round it down to 1000ms to be on the safe
 * side.
 */
#define LP55XX_MAX_BLINK_PERIOD 1000
/*
 * The minimum waiting period is 0.49ms with the prescaler set to 0 and one
 * step. We round up to a full millisecond.
 */
#define LP55XX_MIN_BLINK_PERIOD 1

/* Brightness limits in percent */
#define LP55XX_MIN_BRIGHTNESS 0
#define LP55XX_MAX_BRIGHTNESS 100

/* Output current limits in 0.1 mA */
#define LP55XX_MIN_CURRENT_SETTING 0
#define LP55XX_MAX_CURRENT_SETTING 255

/* Values for ENABLE register. */
#define LP55XX_ENABLE_CHIP_EN_MASK (1 << 6)
#define LP55XX_ENABLE_CHIP_EN_SET  (1 << 6)
#define LP55XX_ENABLE_CHIP_EN_CLR  (0 << 6)
#define LP55XX_ENABLE_LOG_EN       (1 << 7)

/* Values for CONFIG register. */
#define LP55XX_CONFIG_EXTERNAL_CLOCK         0x00
#define LP55XX_CONFIG_INTERNAL_CLOCK         0x01
#define LP55XX_CONFIG_CLOCK_AUTOMATIC_SELECT 0x02
#define LP55XX_CONFIG_PWRSAVE_EN             (1 << 5)
/* Enable 558 Hz frequency for PWM. Default is 256. */
#define LP55XX_CONFIG_PWM_HW_FREQ_558        (1 << 6)

/* Values for execution engine programs. */
#define LP55XX_PROG_CMD_BITS_SET_PWM (1 << 6)
#define LP55XX_PROG_CMD_BITS_PSC_0 (0 << 6)
#define LP55XX_PROG_CMD_BITS_PSC_1 (1 << 6)

/* Helper definitions. */
#define LP55XX_PROG_MAX_COMMANDS 16
#define LP55XX_MASK              0x03

/* Values for execution engine programs. */
#define LP55XX_SET_PWM_ENG_CMD_PAIR(pwm) {LP55XX_PROG_CMD_BITS_SET_PWM, (pwm)}

/*
 * The max value for step_time is 63,
 * step_delay is 15.6ms if pre-scaler is set to 1, else 0.49ms.
 *
 * If required delay is less than 31, then we just double the
 * millisecond value to get step_time with pre-scaler 0 that is 0.49ms per step.
 * Else we need to use pre-scaler where one step takes 15.6ms. And divide delay
 * by 16 to get an approximate step time. (integer division is used for simplicity)
 */
#define LP55XX_BUSY_WAIT_ENG_CMD_PAIR(delay)					\
	{(((delay) < 31)							\
		? (LP55XX_PROG_CMD_BITS_PSC_0 | (((delay) << 1) & 0x3F))	\
		: (LP55XX_PROG_CMD_BITS_PSC_1 | (((delay) >> 4) & 0x3F))),	\
	0x00}

#define LP55XX_GO_TO_START_ENG_CMD_PAIR() {0, 0}

/*
 * Each channel can be driven by directly assigning a value between 0 and 255 to
 * it to drive the PWM or by one of the three execution engines that can be
 * programmed for custom lighting patterns in order to reduce the I2C traffic
 * for repetitive patterns.
 */
enum lp55xx_led_sources {
	LP55XX_SOURCE_PWM,
	LP55XX_SOURCE_ENGINE_1,
	LP55XX_SOURCE_ENGINE_2,
	LP55XX_SOURCE_ENGINE_3,

	LP55XX_SOURCE_COUNT,
};

/* Operational modes of the execution engines. */
enum lp55xx_engine_op_modes {
	LP55XX_OP_MODE_DISABLED = 0x00,
	LP55XX_OP_MODE_LOAD = 0x01,
	LP55XX_OP_MODE_RUN = 0x02,
	LP55XX_OP_MODE_DIRECT_CTRL = 0x03,
};

/* Execution state of the engines. */
enum lp55xx_engine_exec_states {
	LP55XX_ENGINE_MODE_HOLD = 0x00,
	LP55XX_ENGINE_MODE_STEP = 0x01,
	LP55XX_ENGINE_MODE_RUN = 0x02,
	LP55XX_ENGINE_MODE_EXEC = 0x03,
};

/* Fading directions for programs executed by the engines. */
enum lp55xx_engine_fade_dirs {
	LP55XX_FADE_UP = 0x00,
	LP55XX_FADE_DOWN = 0x01,
};

struct lp55xx_interface {
	uint8_t cfg_reg_init;
	/* bit mask to denote color supported by the interface */
	uint32_t supported_colors;
	/* color_id to register_mapping */
	uint8_t pwm_reg_map[4];
	uint8_t current_reg_map[4];
	/* API to support engine handling */
	int (*get_available_engine)(const struct device *dev, uint8_t color_id,
				    enum lp55xx_led_sources *engine);
	int (*get_led_source)(const struct device *dev, uint8_t color_id,
			      enum lp55xx_led_sources *source);
	int (*set_led_source)(const struct device *dev, uint8_t color_id,
			      enum lp55xx_led_sources source);
};

struct lp55xx_config {
	struct i2c_dt_spec bus;
	uint8_t wrgb_current[4];
	struct gpio_dt_spec enable_gpio;
	const struct lp55xx_interface *iface;
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
static int lp55xx_get_engine_ram_base_addr(enum lp55xx_led_sources engine,
					   uint8_t *base_addr)
{
	switch (engine) {
	case LP55XX_SOURCE_ENGINE_1:
		*base_addr = LP55XX_PROG_MEM_ENG1_BASE;
		break;
	case LP55XX_SOURCE_ENGINE_2:
		*base_addr = LP55XX_PROG_MEM_ENG2_BASE;
		break;
	case LP55XX_SOURCE_ENGINE_3:
		*base_addr = LP55XX_PROG_MEM_ENG3_BASE;
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
static int lp55xx_get_engine_reg_shift(enum lp55xx_led_sources engine,
				       uint8_t *shift)
{
	switch (engine) {
	case LP55XX_SOURCE_ENGINE_1:
		*shift = 4U;
		break;
	case LP55XX_SOURCE_ENGINE_2:
		*shift = 2U;
		break;
	case LP55XX_SOURCE_ENGINE_3:
		*shift = 0U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
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
 * @param dev     LP55XX device.
 * @param color_id LED color_id the source is assigned to.
 * @param source  Source for the color_id.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_set_led_source(const struct device *dev,
				 uint8_t color_id,
				 enum lp55xx_led_sources source)
{
	const struct lp55xx_config *config = dev->config;
	/* LP5562 uses WRGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);

	if (i2c_reg_update_byte_dt(&config->bus, LP5562_LED_MAP,
				   LP55XX_MASK << bit_pos,
				   source << bit_pos)) {
		LOG_ERR("Failed to set LED[%d] source=%d.", color_id, source);
		return -EIO;
	}

	return 0;
}

/*
 * @brief Get the assigned source of the given LED color_id.
 *
 * @param dev     LP55XX device.
 * @param color_id Requested LED color_id.
 * @param source  Pointer to the source of the color_id.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5562_get_led_source(const struct device *dev,
				 uint8_t color_id,
				 enum lp55xx_led_sources *source)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t led_map;
	/* LP5562 uses WRGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);

	if (i2c_reg_read_byte_dt(&config->bus, LP5562_LED_MAP, &led_map)) {
		LOG_ERR("Failed to get LED[%d] source.", color_id);
		return -EIO;
	}

	*source = (led_map >> bit_pos) & LP55XX_MASK;

	return 0;
}

/*
 * @brief Request whether an engine is currently running.
 *
 * @param dev    LP55XX device.
 * @param engine Engine to check.
 *
 * @return Indication of the engine execution state.
 *
 * @retval true  If the engine is currently running.
 * @retval false If the engine is not running or an error occurred.
 */
static bool lp5562_is_engine_executing(const struct device *dev,
				       enum lp55xx_led_sources engine)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t enabled, shift;
	int ret;

	ret = lp55xx_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return false;
	}

	if (i2c_reg_read_byte_dt(&config->bus, LP55XX_ENABLE, &enabled)) {
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
 * @param dev    LP55XX device.
 * @param engine Pointer to the engine ID.
 *
 * @retval 0       On success.
 * @retval -ENODEV If all engines are busy.
 */
static int lp5562_get_available_engine(const struct device *dev, uint8_t color_id,
				       enum lp55xx_led_sources *engine)
{
	enum lp55xx_led_sources src;

	for (src = LP55XX_SOURCE_ENGINE_1; src < LP55XX_SOURCE_COUNT; src++) {
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
struct lp55xx_interface lp55xx_lp5562_iface = {
	.cfg_reg_init = (LP55XX_CONFIG_INTERNAL_CLOCK |
			 LP55XX_CONFIG_PWRSAVE_EN),
	.supported_colors = BIT(LED_COLOR_ID_WHITE) |
			    BIT(LED_COLOR_ID_RED) |
			    BIT(LED_COLOR_ID_GREEN) |
			    BIT(LED_COLOR_ID_BLUE),
	.pwm_reg_map = {LP5562_W_PWM, LP5562_R_PWM, LP5562_G_PWM, LP5562_B_PWM,},
	.current_reg_map = {LP5562_W_CURRENT, LP5562_R_CURRENT, LP5562_G_CURRENT, LP5562_B_CURRENT},
	.get_available_engine = lp5562_get_available_engine,
	.get_led_source = lp5562_get_led_source,
	.set_led_source = lp5562_set_led_source,
};

#endif /* CONFIG_DT_HAS_TI_LP5562_ENABLED */

#if CONFIG_DT_HAS_TI_LP5521_ENABLED

/* LP5521 specific registers indexes */
#define LP5521_R_PWM              0x02
#define LP5521_G_PWM              0x03
#define LP5521_B_PWM              0x04
#define LP5521_R_CURRENT          0x05
#define LP5521_G_CURRENT          0x06
#define LP5521_B_CURRENT          0x07
#define LP5521_CP_MODE_AUTO_MASK  0x18

/*
 * @brief Get engine corresponding to the color_id
 *
 * @param dev    LP5521 device.
 * @param engine Pointer to the engine ID.
 *
 * @retval 0       On success.
 * @retval -EINVAL invalid color_id.
 */
static int lp5521_get_available_engine(const struct device *dev, uint8_t color_id,
					enum lp55xx_led_sources *engine)
{
	if ((color_id >= LED_COLOR_ID_RED) && (color_id <= LED_COLOR_ID_BLUE)) {
		/* color id and engine id are same */
		*engine = color_id;
		return 0;
	}

	LOG_ERR("No unused engine available");
	return -EINVAL;
}

/*
 * @brief Assign a source to the given LED channel.
 *
 * @param dev     LP5521 device.
 * @param channel LED channel the source is assigned to.
 * @param source  Source for the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5521_set_led_source(const struct device *dev, uint8_t color_id,
				 enum lp55xx_led_sources source)
{
	const struct lp55xx_config *config = dev->config;
	/* LP5521 uses RGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);

	if (source == LP55XX_SOURCE_PWM) {
		return i2c_reg_update_byte_dt(&config->bus, LP55XX_OP_MODE,
						LP55XX_MASK << bit_pos,
						LP55XX_OP_MODE_DIRECT_CTRL << bit_pos);
	}

	return 0;
}

/*
 * @brief Get the assigned source of the given LED channel.
 *
 * @param dev     LP5521 device.
 * @param channel Requested LED channel.
 * @param source  Pointer to the source of the channel.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp5521_get_led_source(const struct device *dev, uint8_t color_id,
				 enum lp55xx_led_sources *source)
{
	const struct lp55xx_config *config = dev->config;
	/* LP5521 uses RGB, but ID is BGRW so invert it */
	uint8_t bit_pos = ((LED_COLOR_ID_BLUE - (color_id)) << 1);
	uint8_t op_mode;

	if (i2c_reg_read_byte_dt(&config->bus, LP55XX_OP_MODE, &op_mode)) {
		LOG_ERR("Failed to read OP MODE register.");
		return false;
	}

	if (((op_mode >> bit_pos) & LP55XX_MASK) != LP55XX_OP_MODE_DIRECT_CTRL) {
		return lp5521_get_available_engine(dev, color_id, source);
	}
	*source = LP55XX_SOURCE_PWM;

	return 0;
}

/* LP5521 Interface definitions */
struct lp55xx_interface lp55xx_lp5521_iface = {
	.cfg_reg_init = (LP55XX_CONFIG_INTERNAL_CLOCK |
			 LP55XX_CONFIG_PWRSAVE_EN |
			 LP5521_CP_MODE_AUTO_MASK),
	.supported_colors = BIT(LED_COLOR_ID_RED) |
			    BIT(LED_COLOR_ID_GREEN) |
			    BIT(LED_COLOR_ID_BLUE),
	.pwm_reg_map = {0, LP5521_R_PWM, LP5521_G_PWM, LP5521_B_PWM},
	.current_reg_map = {0, LP5521_R_CURRENT, LP5521_G_CURRENT, LP5521_B_CURRENT},
	.get_available_engine = lp5521_get_available_engine,
	.get_led_source = lp5521_get_led_source,
	.set_led_source = lp5521_set_led_source,
};

#endif /* CONFIG_DT_HAS_TI_LP5521_ENABLED */

/*
 * @brief Set an register shifted for the given execution engine.
 *
 * @param dev    LP55XX device.
 * @param engine Engine the value is shifted for.
 * @param reg    Register address to set.
 * @param val    Value to set.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static int lp55xx_set_engine_reg(const struct device *dev,
				 enum lp55xx_led_sources engine,
				 uint8_t reg, uint8_t val)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t shift;
	int ret;

	ret = lp55xx_get_engine_reg_shift(engine, &shift);
	if (ret) {
		return ret;
	}

	if (i2c_reg_update_byte_dt(&config->bus, reg, LP55XX_MASK << shift,
				   val << shift)) {
		return -EIO;
	}

	return 0;
}

/*
 * @brief Set the operational mode of the given engine.
 *
 * @param dev    LP55XX device.
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
	return lp55xx_set_engine_reg(dev, engine, LP55XX_OP_MODE, mode);
}

/*
 * @brief Set the execution state of the given engine.
 *
 * @param dev    LP55XX device.
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

	ret = lp55xx_set_engine_reg(dev, engine, LP55XX_ENABLE, state);

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
 * @param dev    LP55XX device.
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

	return lp55xx_set_engine_exec_state(dev, engine,
					    LP55XX_ENGINE_MODE_RUN);
}

/*
 * @brief Stop the execution of the program of the given engine.
 *
 * @param dev    LP55XX device.
 * @param engine Engine that is stopped.
 *
 * @retval 0    On success.
 * @retval -EIO If the underlying I2C call fails.
 */
static inline int lp55xx_stop_program_exec(const struct device *dev,
					   enum lp55xx_led_sources engine)
{
	if (lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_DISABLED)) {
		return -EIO;
	}

	return lp55xx_set_engine_exec_state(dev, engine,
					    LP55XX_ENGINE_MODE_HOLD);
}

static int lp55xx_enter_pwm_mode(const struct device *dev, uint32_t led)
{
	const struct lp55xx_config *config = dev->config;
	const struct lp55xx_interface *iface = config->iface;
	int ret;
	enum lp55xx_led_sources source;

	/* query current led source */
	ret = iface->get_led_source(dev, led, &source);
	if (ret) {
		return ret;
	}

	/* if source is linked to engine stop it, and switch to PWM */
	if (source != LP55XX_SOURCE_PWM) {
		ret = lp55xx_stop_program_exec(dev, source);
		if (ret) {
			LOG_ERR("Failed to stop engine=%d.", source);
			return ret;
		}
		ret = iface->set_led_source(dev, led, LP55XX_SOURCE_PWM);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

static int lp55xx_enter_engine_mode(const struct device *dev, uint32_t color_id,
				    enum lp55xx_led_sources *engine)
{
	const struct lp55xx_config *config = dev->config;
	const struct lp55xx_interface *iface = config->iface;
	int ret;

	/* query current led source */
	ret = iface->get_led_source(dev, color_id, engine);
	if (ret) {
		return ret;
	}

	/* if source is PWM, and identify available engine and link it */
	if (*engine == LP55XX_SOURCE_PWM) {
		ret = iface->get_available_engine(dev, color_id, engine);
		if (ret) {
			return ret;
		}
		ret = iface->set_led_source(dev, color_id, *engine);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

static inline int lp55xx_load_and_run_program(const struct device *dev,
					      enum lp55xx_led_sources engine,
					      uint8_t *program_code,
					      uint8_t program_size)
{
	const struct lp55xx_config *config = dev->config;
	uint8_t prog_base_addr;
	int ret;

	if ((program_code == NULL) ||
	    (program_size == 0) ||
	    (program_size > LP55XX_PROG_MEM_SIZE)) {
		return -EINVAL;
	}
	ret = lp55xx_get_engine_ram_base_addr(engine, &prog_base_addr);
	if (ret) {
		LOG_ERR("Failed to get base RAM address.");
		return ret;
	}

	ret = lp55xx_stop_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to stop ENG Exec.");
		return ret;
	}

	ret = lp55xx_set_engine_op_mode(dev, engine, LP55XX_OP_MODE_LOAD);
	if (ret) {
		LOG_ERR("Failed to switch ENG to LOAD mode");
		return ret;
	}

	ret = i2c_burst_write_dt(&config->bus, prog_base_addr, program_code, program_size);
	if (ret) {
		LOG_ERR("Failed to upload program");
		return ret;
	}

	ret = lp55xx_start_program_exec(dev, engine);
	if (ret) {
		LOG_ERR("Failed to execute program.");
		return ret;
	}

	return 0;
}

static int lp55xx_led_set_pwm_brightness(const struct device *dev, uint32_t color_id,
					 uint8_t value)
{
	const struct lp55xx_config *config = dev->config;
	const struct lp55xx_interface *iface = config->iface;
	uint8_t val = (value * 0xFF) / LP55XX_MAX_BRIGHTNESS;

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
 * @param dev           LP55XX device.
 * @param engine        Engine running the blinking program.
 * @param brightness_on New brightness value.
 *
 * @retval 0       On Success.
 * @retval -EINVAL If the engine ID or brightness is out of range.
 * @retval -EIO    If the underlying I2C call fails.
 */
static int lp55xx_update_blinking_brightness(const struct device *dev,
					     enum lp55xx_led_sources engine,
					     uint8_t brightness_on)
{
	uint8_t pwm_value = (brightness_on * 0xFF) / LP55XX_MAX_BRIGHTNESS;

	uint8_t program_code[][2] = {
		LP55XX_SET_PWM_ENG_CMD_PAIR(pwm_value),
	};

	return lp55xx_load_and_run_program(dev, engine,
					   (uint8_t *) program_code,
					   sizeof(program_code));
}

static int lp55xx_led_blink(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	int ret;
	enum lp55xx_led_sources engine;

	ret = lp55xx_enter_engine_mode(dev, led, &engine);
	if (ret) {
		return ret;
	}

	uint8_t program_code[][2] = {
		/* set to maximum brightness */
		LP55XX_SET_PWM_ENG_CMD_PAIR(LP55XX_MAX_BRIGHTNESS),
		/* wait for on period */
		LP55XX_BUSY_WAIT_ENG_CMD_PAIR(delay_on),
		/* set to minimum brightness */
		LP55XX_SET_PWM_ENG_CMD_PAIR(LP55XX_MIN_BRIGHTNESS),
		/* wait for off period */
		LP55XX_BUSY_WAIT_ENG_CMD_PAIR(delay_off),
		/* restart */
		LP55XX_GO_TO_START_ENG_CMD_PAIR(),
	};

	return lp55xx_load_and_run_program(dev, engine,
					   (uint8_t *) program_code,
					   sizeof(program_code));
}

static int lp55xx_led_set_brightness(const struct device *dev, uint32_t led,
				     uint8_t value)
{
	const struct lp55xx_config *config = dev->config;
	const struct lp55xx_interface *iface = config->iface;
	int ret;
	enum lp55xx_led_sources current_source;

	if ((value < LP55XX_MIN_BRIGHTNESS) ||
			(value > LP55XX_MAX_BRIGHTNESS)) {
		return -EINVAL;
	}

	ret = iface->get_led_source(dev, led, &current_source);
	if (ret) {
		return ret;
	}

	if (current_source == LP55XX_SOURCE_PWM) {
		return lp55xx_led_set_pwm_brightness(dev, led, value);
	} else {
		return lp55xx_update_blinking_brightness(dev, current_source, value);
	}
}

static inline int lp55xx_led_on(const struct device *dev, uint32_t led)
{
	int ret = lp55xx_enter_pwm_mode(dev, led);

	if (ret) {
		return ret;
	}
	return lp55xx_led_set_pwm_brightness(dev, led, LP55XX_MAX_BRIGHTNESS);
}

static inline int lp55xx_led_off(const struct device *dev, uint32_t led)
{
	int ret = lp55xx_enter_pwm_mode(dev, led);

	if (ret) {
		return ret;
	}
	return lp55xx_led_set_pwm_brightness(dev, led, LP55XX_MIN_BRIGHTNESS);
}

static int lp55xx_enable(const struct device *dev, bool soft_reset)
{
	const struct lp55xx_config *config = dev->config;
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
		 * the LP55XX after the rising edge of the enable line.
		 * So let's wait for 1 ms.
		 */
		k_sleep(K_MSEC(1));
	}

	if (soft_reset) {
		/* Reset all internal registers to have a deterministic state. */
		err = i2c_reg_write_byte_dt(&config->bus, LP55XX_RESET, 0xFF);
		if (err) {
			LOG_ERR("%s: failed to soft-reset device", dev->name);
			return err;
		}
	}

	/* Set en bit in LP55XX_ENABLE register. */
	err = i2c_reg_update_byte_dt(&config->bus, LP55XX_ENABLE, LP55XX_ENABLE_CHIP_EN_MASK,
				     LP55XX_ENABLE_CHIP_EN_SET);
	if (err) {
		LOG_ERR("%s: failed to set EN Bit in ENABLE register", dev->name);
		return err;
	}
	/* Allow 500 µs delay after setting chip_en bit to '1'. */
	k_sleep(K_USEC(500));
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lp55xx_disable(const struct device *dev)
{
	const struct lp55xx_config *config = dev->config;
	const struct gpio_dt_spec *enable_gpio = &config->enable_gpio;
	int err = 0;

	/* clear en bit in register configurations */
	err = i2c_reg_update_byte_dt(&config->bus, LP55XX_ENABLE, LP55XX_ENABLE_CHIP_EN_MASK,
				     LP55XX_ENABLE_CHIP_EN_CLR);
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

static int lp55xx_led_init(const struct device *dev)
{
	const struct lp55xx_config *config = dev->config;
	const struct lp55xx_interface *iface = config->iface;
	const struct gpio_dt_spec *enable_gpio = &config->enable_gpio;
	int ret;

	if (enable_gpio->port != NULL) {
		if (!gpio_is_ready_dt(enable_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(enable_gpio, GPIO_OUTPUT);
		if (ret) {
			LOG_ERR("LP55XX Enable GPIO Config failed");
			return ret;
		}
	}

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	ret = lp55xx_enable(dev, true);
	if (ret) {
		return ret;
	}

	if (i2c_reg_write_byte_dt(&config->bus, LP55XX_CONFIG, iface->cfg_reg_init)) {
		LOG_ERR("Configuring LP55XX LED chip failed.");
		return -EIO;
	}

	for (uint8_t color_id = LED_COLOR_ID_WHITE; color_id <= LED_COLOR_ID_BLUE; color_id++) {
		if (iface->supported_colors & BIT(color_id)) {
			/* update current configuration */
			ret = i2c_reg_write_byte_dt(&config->bus, iface->current_reg_map[color_id],
						    config->wrgb_current[color_id]);
			if (ret) {
				LOG_ERR("Failed to set current of color %d", color_id);
				return ret;
			}
			/* turn off */
			ret = lp55xx_led_off(dev, color_id);
			if (ret) {
				LOG_ERR("Failed to set default state");
				return ret;
			}
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

#ifdef CONFIG_PM_DEVICE
static int lp55xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return lp55xx_disable(dev);
	case PM_DEVICE_ACTION_RESUME:
		return lp55xx_enable(dev, false);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define LP55XX_DEFINE(id, name)						\
	BUILD_ASSERT(DT_INST_PROP(id, red_output_current) <= LP55XX_MAX_CURRENT_SETTING,\
		"Red channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP(id, green_output_current) <= LP55XX_MAX_CURRENT_SETTING,\
		"Green channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP(id, blue_output_current) <= LP55XX_MAX_CURRENT_SETTING,\
		"Blue channel current must be between 0 and 25.5 mA.");	\
	BUILD_ASSERT(DT_INST_PROP_OR(id, white_output_current, 0) <= LP55XX_MAX_CURRENT_SETTING,\
		"White channel current must be between 0 and 25.5 mA.");	\
	static const struct lp55xx_config lp55xx_config_##name##id = {	\
		.bus = I2C_DT_SPEC_INST_GET(id),			\
		.wrgb_current = {					\
			DT_INST_PROP_OR(id, white_output_current, 0),	\
			DT_INST_PROP(id, red_output_current),		\
			DT_INST_PROP(id, green_output_current),		\
			DT_INST_PROP(id, blue_output_current),		\
		},							\
		.enable_gpio = GPIO_DT_SPEC_INST_GET_OR(id, enable_gpios, {0}),	\
		.iface = &lp55xx_##name##_iface,			\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(id, lp55xx_pm_action);			\
									\
	DEVICE_DT_INST_DEFINE(id, &lp55xx_led_init, PM_DEVICE_DT_INST_GET(id),	\
			NULL,						\
			&lp55xx_config_##name##id, POST_KERNEL,		\
			CONFIG_LED_INIT_PRIORITY,			\
			&lp55xx_led_api);				\

#if CONFIG_DT_HAS_TI_LP5562_ENABLED

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5562
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP55XX_DEFINE, lp5562)

#endif /* CONFIG_DT_HAS_TI_LP5562_ENABLED */

#if CONFIG_DT_HAS_TI_LP5521_ENABLED

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lp5521
DT_INST_FOREACH_STATUS_OKAY_VARGS(LP55XX_DEFINE, lp5521)

#endif /* CONFIG_DT_HAS_TI_LP5521_ENABLED */
