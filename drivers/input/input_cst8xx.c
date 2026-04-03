/*
 * Copyright (c) 2021 Qingsong Gou <gouqs@hotmail.com>
 * Copyright (c) 2022 Jakob Krantz <mail@jakobkrantz.se>
 * Copyright (c) 2023 Daniel Kampert <kontakt@daniel-kampert.de>
 * Copyright (c) 2025 ZSWatch Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hynitron_cst8xx

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/dt-bindings/input/cst8xx-gesture-codes.h>

LOG_MODULE_REGISTER(cst8xx, CONFIG_INPUT_LOG_LEVEL);

#define CST816S_CHIP_ID 0xB4
#define CST816T_CHIP_ID 0xB5
#define CST816D_CHIP_ID 0xB6
#define CST820_CHIP_ID  0xB7

#define CST8XX_REG_DATA                0x00
#define CST8XX_REG_GESTURE_ID          0x01
#define CST8XX_REG_FINGER_NUM          0x02
#define CST8XX_REG_XPOS_H              0x03
#define CST8XX_REG_XPOS_L              0x04
#define CST8XX_REG_YPOS_H              0x05
#define CST8XX_REG_YPOS_L              0x06
#define CST8XX_REG_BPC0H               0xB0
#define CST8XX_REG_BPC0L               0xB1
#define CST8XX_REG_BPC1H               0xB2
#define CST8XX_REG_BPC1L               0xB3
#define CST8XX_REG_POWER_MODE          0xA5
#define CST8XX_REG_SLEEP_MODE          0xE5
#define CST8XX_REG_CHIP_ID             0xA7
#define CST8XX_REG_PROJ_ID             0xA8
#define CST8XX_REG_FW_VERSION          0xA9
#define CST8XX_REG_MOTION_MASK         0xEC
#define CST8XX_REG_IRQ_PULSE_WIDTH     0xED
#define CST8XX_REG_NOR_SCAN_PER        0xEE
#define CST8XX_REG_MOTION_S1_ANGLE     0xEF
#define CST8XX_REG_LP_SCAN_RAW1H       0xF0
#define CST8XX_REG_LP_SCAN_RAW1L       0xF1
#define CST8XX_REG_LP_SCAN_RAW2H       0xF2
#define CST8XX_REG_LP_SCAN_RAW2L       0xF3
#define CST8XX_REG_LP_AUTO_WAKEUP_TIME 0xF4
#define CST8XX_REG_LP_SCAN_TH          0xF5
#define CST8XX_REG_LP_SCAN_WIN         0xF6
#define CST8XX_REG_LP_SCAN_FREQ        0xF7
#define CST8XX_REG_LP_SCAN_I_DAC       0xF8
#define CST8XX_REG_AUTOSLEEP_TIME      0xF9
#define CST8XX_REG_IRQ_CTL             0xFA
#define CST8XX_REG_DEBOUNCE_TIME       0xFB
#define CST8XX_REG_LONG_PRESS_TIME     0xFC
#define CST8XX_REG_IOCTL               0xFD
#define CST8XX_REG_DIS_AUTO_SLEEP      0xFE

#define CST8XX_MOTION_EN_CON_LR BIT(2)
#define CST8XX_MOTION_EN_CON_UR BIT(1)
#define CST8XX_MOTION_EN_DCLICK BIT(0)

#define CST8XX_IRQ_EN_TEST   BIT(7)
#define CST8XX_IRQ_EN_TOUCH  BIT(6)
#define CST8XX_IRQ_EN_CHANGE BIT(5)
#define CST8XX_IRQ_EN_MOTION BIT(4)
#define CST8XX_IRQ_ONCE_WLP  BIT(0)

#define CST8XX_IOCTL_SOFT_RTS BIT(2)
#define CST8XX_IOCTL_IIC_OD   BIT(1)
#define CST8XX_IOCTL_EN_1V8   BIT(0)

#define CST8XX_POWER_MODE_SLEEP        (0x03)
#define CST8XX_POWER_MODE_EXPERIMENTAL (0x05)

#define CST8XX_EVENT_BITS_POS (0x06)

#define CST8XX_RESET_DELAY (5)  /* in ms */
#define CST8XX_WAIT_DELAY  (50) /* in ms */

#define EVENT_PRESS_DOWN 0x00U
#define EVENT_LIFT_UP    0x01U
#define EVENT_CONTACT    0x02U
#define EVENT_NONE       0x03U

/** cst8xx configuration (DT). */
struct cst8xx_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec rst_gpio;
#ifdef CONFIG_INPUT_CST8XX_INTERRUPT
	const struct gpio_dt_spec int_gpio;
#endif

#ifdef CONFIG_PM_DEVICE
	/** cst8xx power management profile. */
	struct __packed {
		uint8_t auto_wake_time_min; /**< Auto-recalibration period during low-power mode */
		uint8_t scan_th;            /**< Low-power scan wake-up threshold */
		uint8_t scan_win;           /**< Measurement range for low-power scan */
		uint8_t scan_freq;          /**< Frequency for low-power scan */
		uint8_t scan_i_dac;         /**< Current for low-power scan */
		uint8_t auto_sleep_time_s; /**< Time of inactivity before entering low-power mode */
	} lp_profile;
#endif
};

/** cst8xx data. */
struct cst8xx_data {
	/** Device pointer. */
	const struct device *dev;
	/** Work queue (for deferred read). */
	struct k_work work;

#ifdef CONFIG_INPUT_CST8XX_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
};

/* NOTE: This results in reliable low-power operation with good
 * wake sensitivity. Consumes about 80uA in suspend mode. Can probably be tuned more to reduce power
 * further while keeping good wake sensitivity.
 */

static int cst8xx_process(const struct device *dev)
{
	const struct cst8xx_config *cfg = dev->config;
	int ret;
	uint8_t event;
	uint16_t row, col;
	bool pressed;
	uint16_t x;
	uint16_t y;

#ifdef CONFIG_INPUT_CST8XX_EV_DEVICE
	uint8_t gesture;

	ret = i2c_burst_read_dt(&cfg->i2c, CST8XX_REG_GESTURE_ID, &gesture, sizeof(gesture));
	if (ret < 0) {
		LOG_ERR("Could not read gesture-ID data (%d)", ret);
		return ret;
	}
#endif

	ret = i2c_burst_read_dt(&cfg->i2c, CST8XX_REG_XPOS_H, (uint8_t *)&x, sizeof(x));
	if (ret < 0) {
		LOG_ERR("Could not read x data (%d)", ret);
		return ret;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, CST8XX_REG_YPOS_H, (uint8_t *)&y, sizeof(y));
	if (ret < 0) {
		LOG_ERR("Could not read y data (%d)", ret);
		return ret;
	}
	col = sys_be16_to_cpu(x) & 0x0fff;
	row = sys_be16_to_cpu(y) & 0x0fff;

	event = (x & 0xff) >> CST8XX_EVENT_BITS_POS;
	pressed = (event == EVENT_PRESS_DOWN) || (event == EVENT_CONTACT);

	LOG_DBG("event: %d, row: %d, col: %d", event, row, col);

	if (pressed) {
		input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}

#ifdef CONFIG_INPUT_CST8XX_EV_DEVICE
	/* Also put the custom touch gestures into the input queue,
	 * some applications may want to process it
	 */

	LOG_DBG("gesture: %d", gesture);

	if (gesture != CST8XX_GESTURE_CODE_NONE) {
		input_report(dev, INPUT_EV_DEVICE, (uint16_t)gesture, 0, true, K_FOREVER);
	}
#endif

	return ret;
}

static void cst8xx_work_handler(struct k_work *work)
{
	struct cst8xx_data *data = CONTAINER_OF(work, struct cst8xx_data, work);

	cst8xx_process(data->dev);
}

#ifdef CONFIG_INPUT_CST8XX_INTERRUPT
static void cst8xx_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cst8xx_data *data = CONTAINER_OF(cb, struct cst8xx_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void cst8xx_timer_handler(struct k_timer *timer)
{
	struct cst8xx_data *data = CONTAINER_OF(timer, struct cst8xx_data, timer);

	k_work_submit(&data->work);
}
#endif

static void cst8xx_chip_reset(const struct device *dev)
{
	const struct cst8xx_config *config = dev->config;
	int ret;

	if (gpio_is_ready_dt(&config->rst_gpio)) {
		ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO pin (%d)", ret);
			return;
		}
		k_msleep(CST8XX_RESET_DELAY);
		gpio_pin_set_dt(&config->rst_gpio, 0);
		k_msleep(CST8XX_WAIT_DELAY);
	}
}

static int cst8xx_chip_init(const struct device *dev)
{
	uint8_t irq_mask;
	const struct cst8xx_config *cfg = dev->config;
	int ret;
	uint8_t chip_id;

	cst8xx_chip_reset(dev);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, CST8XX_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id (%d)", ret);
		return ret;
	}

	switch (chip_id) {
	case CST816S_CHIP_ID:
	case CST816T_CHIP_ID:
	case CST816D_CHIP_ID:
	case CST820_CHIP_ID:
		/* Valid chip IDs */
		break;

	default:
		LOG_ERR("CST8XXX unsupported chip id: returned 0x%x", chip_id);
		return -ENODEV;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, CST8XX_REG_MOTION_MASK, CST8XX_MOTION_EN_DCLICK,
				     CST8XX_MOTION_EN_DCLICK);
	if (ret < 0) {
		LOG_ERR("Could not enable double-click motion mask (%d)", ret);
		return ret;
	}

#ifdef CONFIG_INPUT_CST8XX_EV_DEVICE
	irq_mask = CST8XX_IRQ_EN_TOUCH | CST8XX_IRQ_EN_CHANGE | CST8XX_IRQ_EN_MOTION;
#else
	irq_mask = CST8XX_IRQ_EN_TOUCH | CST8XX_IRQ_EN_CHANGE;
#endif

	ret = i2c_reg_update_byte_dt(&cfg->i2c, CST8XX_REG_IRQ_CTL, irq_mask, irq_mask);
	if (ret < 0) {
		LOG_ERR("Could not enable irq (%d)", ret);
		return ret;
	}
	return 0;
}

static int cst8xx_init(const struct device *dev)
{
	struct cst8xx_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_work_init(&data->work, cst8xx_work_handler);

	ret = cst8xx_chip_init(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_INPUT_CST8XX_INTERRUPT
	const struct cst8xx_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(config->int_gpio.port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, cst8xx_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback (%d)", ret);
		return ret;
	}
#else
	k_timer_init(&data->timer, cst8xx_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_CST8XX_PERIOD),
		      K_MSEC(CONFIG_INPUT_CST8XX_PERIOD));
#endif

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int cst8xx_apply_profile(const struct cst8xx_config *cfg)
{
	int ret;

	ret = i2c_burst_write_dt(&cfg->i2c, CST8XX_REG_LP_AUTO_WAKEUP_TIME,
				 (const uint8_t *)&cfg->lp_profile, sizeof(cfg->lp_profile));
	if (ret) {
		LOG_WRN("Write power profile failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int cst8xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;
	const struct cst8xx_config *cfg = dev->config;

	/* For some reason the CST816S does not respond to I2C commands after we use the standby
	 * profile. Workaround for now is to just always reset it before we change power modes.
	 */
	cst8xx_chip_reset(dev);
	ret = cst8xx_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Chip init failed during PM action (%d)", ret);
		return ret;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	/* For PM TURN_ON means device is in suspend mode, hence apply suspend profile. */
	case PM_DEVICE_ACTION_TURN_ON:
		ret = cst8xx_apply_profile(cfg);
		if (ret < 0) {
			LOG_WRN("Could not apply suspend profile (%d)", ret);
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, CST8XX_REG_DIS_AUTO_SLEEP, 0x00);
		if (ret < 0) {
			LOG_WRN("Could not enable auto sleep (%d)", ret);
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/* Put into Deep Sleep mode. */
		ret = i2c_reg_write_byte_dt(&cfg->i2c, CST8XX_REG_SLEEP_MODE,
					    CST8XX_POWER_MODE_SLEEP);
		if (ret < 0) {
			LOG_WRN("Could not enter deep sleep mode (%d)", ret);
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* Nothing to do, device is already reset above. */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

/* clang-format off */
#define CST8XX_DEFINE(index)                                                                       \
	IF_ENABLED(CONFIG_PM_DEVICE, (                                                             \
		BUILD_ASSERT(DT_INST_PROP(index, scan_th) >= 1 &&                                  \
				DT_INST_PROP(index, scan_th) <= 255,                               \
				"scan_th must be >= 1 and <= 255");                                \
		BUILD_ASSERT(DT_INST_PROP(index, scan_freq) >= 1 &&                                \
				DT_INST_PROP(index, scan_freq) <= 255,                             \
				"scan_freq must be >= 1 and <= 255");                              \
		BUILD_ASSERT(DT_INST_PROP(index, scan_win) <= 255,                                 \
				"scan_win must be <= 255");                                        \
		BUILD_ASSERT(DT_INST_PROP(index, scan_i_dac) >= 1 &&                               \
				DT_INST_PROP(index, scan_i_dac) <= 255,                            \
				"scan_i_dac must be >= 1 and <= 255");                             \
	))                                                                                         \
	static struct cst8xx_data cst8xx_data_##index;                                             \
	static const struct cst8xx_config cst8xx_config_##index = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(index, rst_gpios, {}),                        \
		IF_ENABLED(CONFIG_INPUT_CST8XX_INTERRUPT,                                          \
				(.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),))            \
				 IF_ENABLED(CONFIG_PM_DEVICE,                                      \
				(.lp_profile = {                                                   \
					.auto_wake_time_min = DT_INST_PROP(index, auto_wake_time), \
					.scan_th = DT_INST_PROP(index, scan_th),                   \
					.scan_win = DT_INST_PROP(index, scan_win),                 \
					.scan_freq = DT_INST_PROP(index, scan_freq),               \
					.scan_i_dac = DT_INST_PROP(index, scan_i_dac),             \
					.auto_sleep_time_s = DT_INST_PROP(index, auto_sleep_time), \
			},)) };                                                                    \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(index, cst8xx_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, cst8xx_init, PM_DEVICE_DT_INST_GET(index),                    \
			      &cst8xx_data_##index, &cst8xx_config_##index, POST_KERNEL,           \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CST8XX_DEFINE)
/* clang-format on */
