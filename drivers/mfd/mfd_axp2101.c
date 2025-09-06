/*
 * Copyright (c) 2025 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

LOG_MODULE_REGISTER(mfd_axp2101, CONFIG_MFD_LOG_LEVEL);

/* Helper macro to enable IRQ management from the device-tree. */
#if DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(x_powers_axp2101, int_gpios)
#define MFD_AXP2101_INTERRUPT	1
#endif

struct mfd_axp2101_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
};

#if MFD_AXP2101_INTERRUPT
struct mfd_axp2101_data {
	struct gpio_callback gpio_cb;
	const struct device *dev;
	struct k_work work;
};
#endif /* MFD_AXP2101_INTERRUPT */

/* Registers and (some) corresponding values */
#define AXP2101_REG_CHIP_ID		0x03U
	#define AXP2101_CHIP_ID				0x4AU
#define AXP2101_REG_IRQ_ENABLE_0	0x40U
#define AXP2101_REG_IRQ_ENABLE_1	0x41U
	#define AXP2101_IRQ_ENABLE_1_PWR_ON_NEG_EDGE_IRQ	BIT(1)
	#define AXP2101_IRQ_ENABLE_1_PWR_ON_POS_EDGE_IRQ	BIT(0)
#define AXP2101_REG_IRQ_ENABLE_2	0x42U
#define AXP2101_REG_IRQ_STATUS_0	0x48U
#define AXP2101_REG_IRQ_STATUS_1	0x49U
	#define AXP2101_IRQ_STATUS_1_PWR_ON_NEG_EDGE_IRQ	BIT(1)
	#define AXP2101_IRQ_STATUS_1_PWR_ON_POS_EDGE_IRQ	BIT(0)
#define AXP2101_REG_IRQ_STATUS_2	0x4AU

#if MFD_AXP2101_INTERRUPT

enum irq_status_reg_idx {
	AXP2101_IRQ_STATUS_0_IDX = 0,
	AXP2101_IRQ_STATUS_1_IDX,
	AXP2101_IRQ_STATUS_2_IDX,
	AXP2101_IRQ_STATUS_REG_COUNT
};

static const uint8_t axp2101_dflt_irq_enable[] = {
	/* AXP2101_REG_IRQ_ENABLE_0 */
	0x00,

	/* AXP2101_REG_IRQ_ENABLE_1 */
#if CONFIG_MFD_AXP2101_POWER_BUTTON
	AXP2101_IRQ_ENABLE_1_PWR_ON_NEG_EDGE_IRQ |
	AXP2101_IRQ_ENABLE_1_PWR_ON_POS_EDGE_IRQ,
#else
	0x00,
#endif

	/* AXP2101_REG_IRQ_ENABLE_2 */
	0x00,
};

static inline int axp2101_irq_read_and_clear(const struct i2c_dt_spec *i2c,
					     uint8_t irq_status[AXP2101_IRQ_STATUS_REG_COUNT])
{
	int ret;

	ret = i2c_burst_read_dt(i2c, AXP2101_REG_IRQ_STATUS_0, irq_status,
				AXP2101_IRQ_STATUS_REG_COUNT);
	if (ret < 0) {
		LOG_ERR("Failed to read IRQ status registers");
		return ret;
	}

	/* Writing a '1' on a status bit which is already set clears it. Therefore
	 * to clear all (and only) the bits that have just been dumped we write
	 * the read values back.
	 */
	ret = i2c_burst_write_dt(i2c, AXP2101_REG_IRQ_STATUS_0, irq_status,
				 AXP2101_IRQ_STATUS_REG_COUNT);
	if (ret < 0) {
		LOG_ERR("Failed to clear IRQ status registers");
		return ret;
	}

	return 0;
}

static void axp2101_k_work_handler(struct k_work *work)
{
	struct mfd_axp2101_data *data = CONTAINER_OF(work, struct mfd_axp2101_data, work);
	const struct device *dev = data->dev;
	const struct mfd_axp2101_config *config = dev->config;
	uint8_t irq_status_regs[AXP2101_IRQ_STATUS_REG_COUNT];
	int ret;

	ret = axp2101_irq_read_and_clear(&config->i2c, irq_status_regs);
	if (ret < 0) {
		goto exit;
	}

#if CONFIG_MFD_AXP2101_POWER_BUTTON
	if (irq_status_regs[AXP2101_IRQ_STATUS_1_IDX] & AXP2101_IRQ_STATUS_1_PWR_ON_NEG_EDGE_IRQ) {
		ret = input_report_key(dev, INPUT_KEY_POWER, true, true, K_FOREVER);
	}
	if (irq_status_regs[AXP2101_IRQ_STATUS_1_IDX] & AXP2101_IRQ_STATUS_1_PWR_ON_POS_EDGE_IRQ) {
		ret = input_report_key(dev, INPUT_KEY_POWER, false, true, K_FOREVER);
	}
	if (ret < 0) {
		LOG_ERR("Failed to send input event");
	}
#endif /* CONFIG_MFD_AXP2101_POWER_BUTTON */

exit:
	/* Resubmit work if interrupt is still active */
	if (gpio_pin_get_dt(&config->int_gpio) != 0) {
		k_work_submit(&data->work);
	}
}

static void axp2101_interrupt_callback(const struct device *gpio_dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct mfd_axp2101_data *data = CONTAINER_OF(cb, struct mfd_axp2101_data, gpio_cb);

	ARG_UNUSED(gpio_dev);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

static int mfd_axp2101_configure_irq(const struct device *dev)
{
	const struct mfd_axp2101_config *config = dev->config;
	struct mfd_axp2101_data *data = dev->data;
	uint8_t dummy_buffer[] = {0xFF, 0xFF, 0xFF};
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_WRN("Interrupt GPIO not ready");
		return -EIO;
	};

	k_work_init(&data->work, axp2101_k_work_handler);
	data->dev = dev;

	/* Enable only selected interrupts (most are enabled by default) */
	ret = i2c_burst_write_dt(&config->i2c, AXP2101_REG_IRQ_ENABLE_0,
				 axp2101_dflt_irq_enable,
				 ARRAY_SIZE(axp2101_dflt_irq_enable));
	if (ret < 0) {
		LOG_ERR("Failed to configure enabled IRQs");
		return ret;
	}

	/* Clear any pending interrupt (if any) */
	ret = i2c_burst_write_dt(&config->i2c, AXP2101_REG_IRQ_STATUS_0,
				dummy_buffer, ARRAY_SIZE(dummy_buffer));
	if (ret < 0) {
		LOG_ERR("Failed to clear IRQ status registers");
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, axp2101_interrupt_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure GPIO interrupt: %d", ret);
		return ret;
	}

	/* Manually kick the work the 1st time if IRQ line is already asserted. */
	if (gpio_pin_get_dt(&config->int_gpio) != 0) {
		k_work_submit(&data->work);
	}

	return 0;
}

#endif /* MFD_AXP2101_INTERRUPT */

static int mfd_axp2101_init(const struct device *dev)
{
	const struct mfd_axp2101_config *config = dev->config;
	uint8_t chip_id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Check if axp2101 chip is available */
	ret = i2c_reg_read_byte_dt(&config->i2c, AXP2101_REG_CHIP_ID, &chip_id);
	if (ret < 0) {
		return ret;
	}
	if (chip_id != AXP2101_CHIP_ID) {
		LOG_ERR("Invalid Chip detected (%d)", chip_id);
		return -EINVAL;
	}

#if MFD_AXP2101_INTERRUPT
	ret = mfd_axp2101_configure_irq(dev);
	if (ret != 0) {
		return ret;
	}
#endif /* MFD_AXP2101_INTERRUPT */

	return 0;
}

#define MFD_AXP2101_DEFINE(node)                                                                   \
	static const struct mfd_axp2101_config config##node = {                                    \
		.i2c = I2C_DT_SPEC_GET(node),                                                      \
		.int_gpio = GPIO_DT_SPEC_GET_OR(node, int_gpios, {0})                              \
	};                                                                                         \
                                                                                                   \
	IF_ENABLED(MFD_AXP2101_INTERRUPT, (static struct mfd_axp2101_data data##node;))            \
                                                                                                   \
	DEVICE_DT_DEFINE(node, mfd_axp2101_init, NULL,                                             \
			 COND_CODE_1(MFD_AXP2101_INTERRUPT, (&data##node), (NULL)),                \
			 &config##node, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_FOREACH_STATUS_OKAY(x_powers_axp2101, MFD_AXP2101_DEFINE);
