/*
 * Copyright (c) 2021 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hynitron_cst816s

#include <drivers/kscan.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(cst816s, CONFIG_KSCAN_LOG_LEVEL);

#define CST816S_CHIP_ID                 0xB4

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

#define CST816S_MOTION_EN_CON_LR      (1<<2)
#define CST816S_MOTION_EN_CON_UR      (1<<1)
#define CST816S_MOTION_EN_DCLICK      (1<<0)

#define CST816S_IRQ_EN_TEST           (1<<7)
#define CST816S_IRQ_EN_TOUCH          (1<<6)
#define CST816S_IRQ_EN_CHANGE         (1<<5)
#define CST816S_IRQ_EN_MOTION         (1<<4)
#define CST816S_IRQ_ONCE_WLP          (1<<0)

#define CST816S_IOCTL_SOFT_RTS        (1<<2)
#define CST816S_IOCTL_IIC_OD          (1<<1)
#define CST816S_IOCTL_EN_1V8          (1<<0)

#define CST816S_POWER_MODE_SLEEP          (0x03)
#define CST816S_POWER_MODE_EXPERIMENTAL   (0x05)

#define EVENT_PRESS_DOWN	0x00U
#define EVENT_LIFT_UP		0x01U
#define EVENT_CONTACT		0x02U
#define EVENT_NONE		0x03U

/** GPIO DT information. */
struct gpio_dt_info {
	/** Port. */
	const char *port;
	/** Pin. */
	gpio_pin_t pin;
	/** Flags. */
	gpio_dt_flags_t flags;
};

/** cst816s configuration (DT). */
struct cst816s_config {
	char *i2c_bus;
	uint8_t i2c_addr;
	struct gpio_dt_info rst_gpio;
#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
	struct gpio_dt_info int_gpio;
#endif
};

/** cst816s data. */
struct cst816s_data {
	/** Device pointer. */
	const struct device *dev;
	/** I2C controller device. */
	const struct device *i2c;
	/** KSCAN Callback. */
	kscan_callback_t callback;
	/** Work queue (for deferred read). */
	struct k_work work;
	const struct device *rst_gpio;
#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
	/** Interrupt GPIO controller. */
	const struct device *int_gpio;

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
	struct cst816s_data *data = dev->data;

	int r;

	uint8_t event;
	uint16_t row, col;
	bool pressed;
	uint8_t buf[9];

	r = i2c_burst_read(data->i2c, cfg->i2c_addr,
				CST816S_REG_DATA, buf, 9);
	if (r < 0) {
		LOG_ERR("Could not read data");
		return r;
	}

	col = ((buf[CST816S_REG_XPOS_H] & 0x0f) << 8) | buf[CST816S_REG_XPOS_L];
	row = ((buf[CST816S_REG_YPOS_H] & 0x0f) << 8) | buf[CST816S_REG_YPOS_L];

	event = buf[CST816S_REG_XPOS_H] >> 6;
	pressed = (event == EVENT_PRESS_DOWN) || (event == EVENT_CONTACT);

	LOG_DBG("event: %d, row: %d, col: %d", event, row, col);

	if (data->callback)
		data->callback(dev, row, col, pressed);

	return 0;
}

static void cst816s_work_handler(struct k_work *work)
{
	struct cst816s_data *data = CONTAINER_OF(work, struct cst816s_data, work);

	cst816s_process(data->dev);
}

#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
static void cst816s_isr_handler(const struct device *dev,
			       struct gpio_callback *cb, uint32_t pins)
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

static int cst816s_configure(const struct device *dev,
			    kscan_callback_t callback)
{
	struct cst816s_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int cst816s_enable_callback(const struct device *dev)
{
	struct cst816s_data *data = dev->data;

#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
	gpio_add_callback(data->int_gpio, &data->int_gpio_cb);
#else
	k_timer_start(&data->timer, K_MSEC(CONFIG_KSCAN_cst816s_PERIOD),
		      K_MSEC(CONFIG_KSCAN_cst816s_PERIOD));
#endif

	return 0;
}

static int cst816s_disable_callback(const struct device *dev)
{
	struct cst816s_data *data = dev->data;

#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
	gpio_remove_callback(data->int_gpio, &data->int_gpio_cb);
#else
	k_timer_stop(&data->timer);
#endif

	return 0;
}

#if DT_INST_NODE_HAS_PROP(0, rst_gpios)
static void cst816s_chip_reset(const struct device* dev)
{
	const struct cst816s_config *cfg = dev->config;
	struct cst816s_data *data = dev->data;

	gpio_pin_set_raw(data->rst_gpio, cfg->rst_gpio.pin, 0);
	k_msleep(5);
	gpio_pin_set_raw(data->rst_gpio, cfg->rst_gpio.pin, 1);
	k_msleep(50);
}
#endif

static int cst816s_chip_init(const struct device *dev)
{
	const struct cst816s_config *cfg = dev->config;
	struct cst816s_data *drv_data = dev->data;
	int ret = 0;
	uint8_t chip_id = 0;

#if DT_INST_NODE_HAS_PROP(0, rst_gpios)
	cst816s_chip_reset(dev);
#endif
	ret = i2c_reg_read_byte(drv_data->i2c, cfg->i2c_addr,
				CST816S_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("failed reading chip id");
		return ret;
	}

	if (chip_id != CST816S_CHIP_ID) {
		LOG_ERR("CST816S wrong chip id: returned 0x%x", chip_id);
		return -ENODEV;
	}

	ret = i2c_reg_update_byte(drv_data->i2c, cfg->i2c_addr,
				CST816S_REG_IRQ_CTL,
				CST816S_IRQ_EN_TOUCH | CST816S_IRQ_EN_CHANGE,
				CST816S_IRQ_EN_TOUCH | CST816S_IRQ_EN_CHANGE);
	if (ret < 0) {
		LOG_ERR("Could not enable irq");
		return ret;
	}
	return ret;
}

static int cst816s_init(const struct device *dev)
{
	const struct cst816s_config *config = dev->config;
	struct cst816s_data *data = dev->data;

	data->i2c = device_get_binding(config->i2c_bus);
	if (!data->i2c) {
		LOG_ERR("Could not find I2C controller");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->work, cst816s_work_handler);

#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
	int r;

	data->int_gpio = device_get_binding(config->int_gpio.port);
	if (!data->int_gpio) {
		LOG_ERR("Could not find interrupt GPIO controller");
		return -ENODEV;
	}

	r = gpio_pin_configure(data->int_gpio, config->int_gpio.pin,
			       config->int_gpio.flags | GPIO_INPUT);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin");
		return r;
	}

	r = gpio_pin_interrupt_configure(data->int_gpio, config->int_gpio.pin,
					GPIO_INT_EDGE_TO_ACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt.");
		return r;
	}

	gpio_init_callback(&data->int_gpio_cb, cst816s_isr_handler,
			   BIT(config->int_gpio.pin));
#else
	k_timer_init(&data->timer, cst816s_timer_handler, NULL);
#endif

#if DT_INST_NODE_HAS_PROP(0, rst_gpios)
	data->rst_gpio = device_get_binding(config->rst_gpio.port);
	if (!data->rst_gpio) {
		LOG_ERR("Could not find reset GPIO controller");
		return -ENODEV;
	}

	r = gpio_pin_configure(data->rst_gpio, config->rst_gpio.pin,
			config->int_gpio.flags | GPIO_OUTPUT_INACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure reset GPIO pin");
		return r;
	}
#endif

	return cst816s_chip_init(dev);
}

static const struct kscan_driver_api cst816s_driver_api = {
	.config = cst816s_configure,
	.enable_callback = cst816s_enable_callback,
	.disable_callback = cst816s_disable_callback,
};

#define DT_INST_GPIO(index, gpio_pha)					       \
	{								       \
		DT_INST_GPIO_LABEL(index, gpio_pha),			       \
		DT_INST_GPIO_PIN(index, gpio_pha),			       \
		DT_INST_GPIO_FLAGS(index, gpio_pha),			       \
	}

#ifdef CONFIG_KSCAN_CST816S_INTERRUPT
#define CST816S_DEFINE_CONFIG(index)					       \
	static const struct cst816s_config cst816s_config_##index = {	       \
		.i2c_bus = DT_INST_BUS_LABEL(index),			       \
		.i2c_addr = DT_INST_REG_ADDR(index),			       \
		.int_gpio = DT_INST_GPIO(index, irq_gpios),		       \
		.rst_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(index, rst_gpios), \
					(DT_INST_GPIO(index, rst_gpios)), ())		    \
	}
#else
#define CST816S_DEFINE_CONFIG(index)					       \
	static const struct cst816s_config cst816s_config_##index = {	       \
		.i2c_bus = DT_INST_BUS_LABEL(index),			       \
		.i2c_address = DT_INST_REG_ADDR(index)			       \
		.rst_gpio = COND_CODE_1(DT_INST_NODE_HAS_PROP(index, rst_gpios), \
					(DT_INST_GPIO(index, rst_gpios)), ())		    \
	}
#endif

#define CST816S_INIT(index)                                                     \
	CST816S_DEFINE_CONFIG(index);					       \
	static struct cst816s_data cst816s_data_##index;			       \
	DEVICE_DT_INST_DEFINE(index, cst816s_init, NULL,			       \
			    &cst816s_data_##index, &cst816s_config_##index,      \
			    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,	       \
			    &cst816s_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CST816S_INIT)
