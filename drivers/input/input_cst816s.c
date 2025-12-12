/*
 * Copyright (c) 2021 Qingsong Gou <gouqs@hotmail.com>
 * Copyright (c) 2022 Jakob Krantz <mail@jakobkrantz.se>
 * Copyright (c) 2023 Daniel Kampert <kontakt@daniel-kampert.de>
 * Copyright (c) 2025 ZSWatch Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hynitron_cst816s

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/dt-bindings/input/cst816s-gesture-codes.h>

LOG_MODULE_REGISTER(cst816s, CONFIG_INPUT_LOG_LEVEL);

#define CST816S_CHIP_ID1 0xB4
#define CST816S_CHIP_ID2 0xB5
#define CST816S_CHIP_ID3 0xB6

#define CST816S_REG_DATA                0x00
#define CST816S_REG_GESTURE_ID          0x01
#define CST816S_REG_FINGER_NUM          0x02
#define CST816S_REG_XPOS_H              0x03
#define CST816S_REG_XPOS_L              0x04
#define CST816S_REG_YPOS_H              0x05
#define CST816S_REG_YPOS_L              0x06
#define CST816S_REG_BPC0H               0xB0
#define CST816S_REG_BPC0L               0xB1
#define CST816S_REG_BPC1H               0xB2
#define CST816S_REG_BPC1L               0xB3
#define CST816S_REG_POWER_MODE          0xA5
#define CST816S_REG_SLEEP_MODE          0xE5
#define CST816S_REG_CHIP_ID             0xA7
#define CST816S_REG_PROJ_ID             0xA8
#define CST816S_REG_FW_VERSION          0xA9
#define CST816S_REG_MOTION_MASK         0xEC
#define CST816S_REG_IRQ_PULSE_WIDTH     0xED
#define CST816S_REG_NOR_SCAN_PER        0xEE
#define CST816S_REG_MOTION_S1_ANGLE     0xEF
#define CST816S_REG_LP_SCAN_RAW1H       0xF0
#define CST816S_REG_LP_SCAN_RAW1L       0xF1
#define CST816S_REG_LP_SCAN_RAW2H       0xF2
#define CST816S_REG_LP_SCAN_RAW2L       0xF3
#define CST816S_REG_LP_AUTO_WAKEUP_TIME 0xF4
#define CST816S_REG_LP_SCAN_TH          0xF5
#define CST816S_REG_LP_SCAN_WIN         0xF6
#define CST816S_REG_LP_SCAN_FREQ        0xF7
#define CST816S_REG_LP_SCAN_I_DAC       0xF8
#define CST816S_REG_AUTOSLEEP_TIME      0xF9
#define CST816S_REG_IRQ_CTL             0xFA
#define CST816S_REG_DEBOUNCE_TIME       0xFB
#define CST816S_REG_LONG_PRESS_TIME     0xFC
#define CST816S_REG_IOCTL               0xFD
#define CST816S_REG_DIS_AUTO_SLEEP      0xFE

#define CST816S_MOTION_EN_CON_LR BIT(2)
#define CST816S_MOTION_EN_CON_UR BIT(1)
#define CST816S_MOTION_EN_DCLICK BIT(0)

#define CST816S_IRQ_EN_TEST   BIT(7)
#define CST816S_IRQ_EN_TOUCH  BIT(6)
#define CST816S_IRQ_EN_CHANGE BIT(5)
#define CST816S_IRQ_EN_MOTION BIT(4)
#define CST816S_IRQ_ONCE_WLP  BIT(0)

#define CST816S_IOCTL_SOFT_RTS BIT(2)
#define CST816S_IOCTL_IIC_OD   BIT(1)
#define CST816S_IOCTL_EN_1V8   BIT(0)

#define CST816S_POWER_MODE_SLEEP        (0x03)
#define CST816S_POWER_MODE_EXPERIMENTAL (0x05)

#define CST816S_EVENT_BITS_POS (0x06)

#define CST816S_RESET_DELAY (5)  /* in ms */
#define CST816S_WAIT_DELAY  (50) /* in ms */

#define EVENT_PRESS_DOWN 0x00U
#define EVENT_LIFT_UP    0x01U
#define EVENT_CONTACT    0x02U
#define EVENT_NONE       0x03U

/** cst816s configuration (DT). */
struct cst816s_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec rst_gpio;
#ifdef CONFIG_INPUT_CST816S_INTERRUPT
	const struct gpio_dt_spec int_gpio;
#endif

#ifdef CONFIG_PM_DEVICE
	/** cst816s power management profile. */
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

/** cst816s data. */
struct cst816s_data {
	/** Device pointer. */
	const struct device *dev;
	/** Work queue (for deferred read). */
	struct k_work work;

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
	/** Interrupt GPIO callback. */
	struct gpio_callback int_gpio_cb;
#else
	/** Timer (polling mode). */
	struct k_timer timer;
#endif
};

static int cst816s_process(const struct device *dev)
{
	const struct cst816s_config *cfg = dev->config;
	int ret;
	uint8_t event;
	uint16_t row, col;
	bool pressed;
	uint16_t x;
	uint16_t y;

#ifdef CONFIG_INPUT_CST816S_EV_DEVICE
	uint8_t gesture;

	ret = i2c_burst_read_dt(&cfg->i2c, CST816S_REG_GESTURE_ID, &gesture, sizeof(gesture));
	if (ret < 0) {
		LOG_ERR("Could not read gesture-ID data (%d)", ret);
		return ret;
	}
#endif

	ret = i2c_burst_read_dt(&cfg->i2c, CST816S_REG_XPOS_H, (uint8_t *)&x, sizeof(x));
	if (ret < 0) {
		LOG_ERR("Could not read x data (%d)", ret);
		return ret;
	}

	ret = i2c_burst_read_dt(&cfg->i2c, CST816S_REG_YPOS_H, (uint8_t *)&y, sizeof(y));
	if (ret < 0) {
		LOG_ERR("Could not read y data (%d)", ret);
		return ret;
	}
	col = sys_be16_to_cpu(x) & 0x0fff;
	row = sys_be16_to_cpu(y) & 0x0fff;

	event = (x & 0xff) >> CST816S_EVENT_BITS_POS;
	pressed = (event == EVENT_PRESS_DOWN) || (event == EVENT_CONTACT);

	LOG_DBG("event: %d, row: %d, col: %d", event, row, col);

	if (pressed) {
		input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}

#ifdef CONFIG_INPUT_CST816S_EV_DEVICE
	/* Also put the custom touch gestures into the input queue,
	 * some applications may want to process it
	 */

	LOG_DBG("gesture: %d", gesture);

	if (gesture != CST816S_GESTURE_CODE_NONE) {
		input_report(dev, INPUT_EV_DEVICE, (uint16_t)gesture, 0, true, K_FOREVER);
	}
#endif

	return ret;
}

static void cst816s_work_handler(struct k_work *work)
{
	struct cst816s_data *data = CONTAINER_OF(work, struct cst816s_data, work);

	cst816s_process(data->dev);
}

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
static void cst816s_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct cst816s_data *data = CONTAINER_OF(cb, struct cst816s_data, int_gpio_cb);

	k_work_submit(&data->work);
}
#else
static void cst816s_timer_handler(struct k_timer *timer)
{
	struct cst816s_data *data = CONTAINER_OF(timer, struct cst816s_data, timer);

	k_work_submit(&data->work);
}
#endif

static void cst816s_chip_reset(const struct device *dev)
{
	const struct cst816s_config *config = dev->config;
	int ret;

	if (gpio_is_ready_dt(&config->rst_gpio)) {
		ret = gpio_pin_configure_dt(&config->rst_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO pin (%d)", ret);
			return;
		}
		k_msleep(CST816S_RESET_DELAY);
		gpio_pin_set_dt(&config->rst_gpio, 0);
		k_msleep(CST816S_WAIT_DELAY);
	}
}

static int cst816s_chip_init(const struct device *dev)
{
	uint8_t irq_mask;
	const struct cst816s_config *cfg = dev->config;
	int ret;
	uint8_t chip_id;

	cst816s_chip_reset(dev);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, CST816S_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id (%d)", ret);
		return ret;
	}

	if ((chip_id != CST816S_CHIP_ID1) && (chip_id != CST816S_CHIP_ID2) &&
	    (chip_id != CST816S_CHIP_ID3)) {
		LOG_ERR("Wrong chip id: returned 0x%x", chip_id);
		return -ENODEV;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, CST816S_REG_MOTION_MASK, CST816S_MOTION_EN_DCLICK,
				     CST816S_MOTION_EN_DCLICK);
	if (ret < 0) {
		LOG_ERR("Could not enable double-click motion mask (%d)", ret);
		return ret;
	}

#ifdef CONFIG_INPUT_CST816S_EV_DEVICE
	irq_mask = CST816S_IRQ_EN_TOUCH | CST816S_IRQ_EN_CHANGE | CST816S_IRQ_EN_MOTION;
#else
	irq_mask = CST816S_IRQ_EN_TOUCH | CST816S_IRQ_EN_CHANGE;
#endif

	ret = i2c_reg_update_byte_dt(&cfg->i2c, CST816S_REG_IRQ_CTL, irq_mask, irq_mask);
	if (ret < 0) {
		LOG_ERR("Could not enable irq (%d)", ret);
		return ret;
	}
	return 0;
}

static int cst816s_init(const struct device *dev)
{
	struct cst816s_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_work_init(&data->work, cst816s_work_handler);

	ret = cst816s_chip_init(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_INPUT_CST816S_INTERRUPT
	const struct cst816s_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
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

	gpio_init_callback(&data->int_gpio_cb, cst816s_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback (%d)", ret);
		return ret;
	}
#else
	k_timer_init(&data->timer, cst816s_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_CST816S_PERIOD),
		      K_MSEC(CONFIG_INPUT_CST816S_PERIOD));
#endif

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int cst816s_apply_profile(const struct cst816s_config *cfg)
{
	int ret;

	ret = i2c_burst_write_dt(&cfg->i2c, CST816S_REG_LP_AUTO_WAKEUP_TIME,
				 (uint8_t *)&cfg->lp_profile, sizeof(cfg->lp_profile));
	if (ret) {
		LOG_WRN("Write power profile failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int cst816s_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;
	const struct cst816s_config *cfg = dev->config;

	/* For some reason the CST816S does not respond to I2C commands after we use the standby
	 * profile. Workaround for now is to just always reset it before we change power modes.
	 */
	cst816s_chip_reset(dev);
	ret = cst816s_chip_init(dev);
	if (ret < 0) {
		LOG_ERR("Chip init failed during PM action (%d)", ret);
		return ret;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	/* For PM TURN_ON means device is in suspend mode, hence apply suspend profile. */
	case PM_DEVICE_ACTION_TURN_ON:
		ret = cst816s_apply_profile(cfg);
		if (ret < 0) {
			LOG_WRN("Could not apply suspend profile (%d)", ret);
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, CST816S_REG_DIS_AUTO_SLEEP, 0x00);
		if (ret < 0) {
			LOG_WRN("Could not enable auto sleep (%d)", ret);
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		/* Put into Deep Sleep mode. */
		ret = i2c_reg_write_byte_dt(&cfg->i2c, CST816S_REG_SLEEP_MODE,
					    CST816S_POWER_MODE_SLEEP);
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
#define CST816S_DEFINE(index)             \
	IF_ENABLED(CONFIG_PM_DEVICE, (                                    \
		BUILD_ASSERT(DT_INST_PROP(index, scan_th) >= 1 &&              \
				DT_INST_PROP(index, scan_th) <= 255,           \
				"scan_th must be >= 1 and <= 255");            \
		BUILD_ASSERT(DT_INST_PROP(index, scan_freq) >= 1 &&            \
				DT_INST_PROP(index, scan_freq) <= 255,         \
				"scan_freq must be >= 1 and <= 255");          \
		BUILD_ASSERT(DT_INST_PROP(index, scan_win) <= 255,           \
				"scan_win must be <= 255");             \
		BUILD_ASSERT(DT_INST_PROP(index, scan_i_dac) >= 1 &&           \
				DT_INST_PROP(index, scan_i_dac) <= 255,        \
				"scan_i_dac must be >= 1 and <= 255");         \
	))                                                                         \
	static struct cst816s_data cst816s_data_##index; \
	static const struct cst816s_config cst816s_config_##index = { \
		.i2c = I2C_DT_SPEC_INST_GET(index), \
		.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(index, rst_gpios, {}), \
		IF_ENABLED(CONFIG_INPUT_CST816S_INTERRUPT, \
				(.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),))  \
				 IF_ENABLED(CONFIG_PM_DEVICE, \
				(.lp_profile = {                                             \
					.auto_wake_time_min = DT_INST_PROP(index, auto_wake_time), \
					.scan_th = DT_INST_PROP(index, scan_th),     \
					.scan_win = DT_INST_PROP(index, scan_win),   \
					.scan_freq = DT_INST_PROP(index, scan_freq), \
					.scan_i_dac = DT_INST_PROP(index, scan_i_dac), \
					.auto_sleep_time_s = DT_INST_PROP(index, auto_sleep_time), \
			},)) }; \
                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(index, cst816s_pm_action);          \
                                                                                           \
	DEVICE_DT_INST_DEFINE(index, cst816s_init, PM_DEVICE_DT_INST_GET(index), \
			      &cst816s_data_##index, &cst816s_config_##index, POST_KERNEL, \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CST816S_DEFINE)
/* clang-format on */
