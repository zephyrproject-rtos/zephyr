/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_pmw3610

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_pmw3610.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_pmw3610, CONFIG_INPUT_LOG_LEVEL);

/* Page 0 */
#define PMW3610_PROD_ID		0x00
#define PMW3610_REV_ID		0x01
#define PMW3610_MOTION		0x02
#define PMW3610_DELTA_X_L	0x03
#define PMW3610_DELTA_Y_L	0x04
#define PMW3610_DELTA_XY_H	0x05
#define PMW3610_PERFORMANCE	0x11
#define PMW3610_BURST_READ	0x12
#define PMW3610_RUN_DOWNSHIFT	0x1b
#define PMW3610_REST1_RATE	0x1c
#define PMW3610_REST1_DOWNSHIFT	0x1d
#define PMW3610_OBSERVATION1	0x2d
#define PMW3610_SMART_MODE	0x32
#define PMW3610_POWER_UP_RESET	0x3a
#define PMW3610_SHUTDOWN	0x3b
#define PMW3610_SPI_CLK_ON_REQ	0x41
#define PWM3610_SPI_PAGE0	0x7f

/* Page 1 */
#define PMW3610_RES_STEP	0x05
#define PWM3610_SPI_PAGE1	0x7f

/* Burst register offsets */
#define BURST_MOTION		0
#define BURST_DELTA_X_L		1
#define BURST_DELTA_Y_L		2
#define BURST_DELTA_XY_H	3
#define BURST_SQUAL		4
#define BURST_SHUTTER_HI	5
#define BURST_SHUTTER_LO	6

#define BURST_DATA_LEN_NORMAL	(BURST_DELTA_XY_H + 1)
#define BURST_DATA_LEN_SMART	(BURST_SHUTTER_LO + 1)
#define BURST_DATA_LEN_MAX	MAX(BURST_DATA_LEN_NORMAL, BURST_DATA_LEN_SMART)

/* Init sequence values */
#define OBSERVATION1_INIT_MASK 0x0f
#define PERFORMANCE_INIT 0x0d
#define RUN_DOWNSHIFT_INIT 0x04
#define REST1_RATE_INIT 0x04
#define REST1_DOWNSHIFT_INIT 0x0f

#define PRODUCT_ID_PMW3610 0x3e
#define SPI_WRITE BIT(7)
#define MOTION_STATUS_MOTION BIT(7)
#define SPI_CLOCK_ON_REQ_ON 0xba
#define SPI_CLOCK_ON_REQ_OFF 0xb5
#define RES_STEP_INV_X_BIT 6
#define RES_STEP_INV_Y_BIT 5
#define RES_STEP_RES_MASK 0x1f
#define PERFORMANCE_FMODE_MASK (0x0f << 4)
#define PERFORMANCE_FMODE_NORMAL (0x00 << 4)
#define PERFORMANCE_FMODE_FORCE_AWAKE (0x0f << 4)
#define POWER_UP_RESET 0x5a
#define POWER_UP_WAKEUP 0x96
#define SHUTDOWN_ENABLE 0xe7
#define SPI_PAGE0_1 0xff
#define SPI_PAGE1_0 0x00
#define SHUTTER_SMART_THRESHOLD 45
#define SMART_MODE_ENABLE 0x00
#define SMART_MODE_DISABLE 0x80

#define PMW3610_DATA_SIZE_BITS 12

#define RESET_DELAY_MS 10
#define INIT_OBSERVATION_DELAY_MS 10
#define CLOCK_ON_DELAY_US 300

#define RES_STEP 200
#define RES_MIN 200
#define RES_MAX 3200

struct pmw3610_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec motion_gpio;
	struct gpio_dt_spec reset_gpio;
	uint16_t axis_x;
	uint16_t axis_y;
	int16_t res_cpi;
	bool invert_x;
	bool invert_y;
	bool force_awake;
	bool smart_mode;
};

struct pmw3610_data {
	const struct device *dev;
	struct k_work motion_work;
	struct gpio_callback motion_cb;
	bool smart_flag;
};

static int pmw3610_read(const struct device *dev,
			uint8_t addr, uint8_t *value, uint8_t len)
{
	const struct pmw3610_config *cfg = dev->config;

	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = sizeof(addr),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf[] = {
		{
			.buf = NULL,
			.len = sizeof(addr),
		},
		{
			.buf = value,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int pmw3610_read_reg(const struct device *dev, uint8_t addr, uint8_t *value)
{
	return pmw3610_read(dev, addr, value, 1);
}

static int pmw3610_write_reg(const struct device *dev, uint8_t addr, uint8_t value)
{
	const struct pmw3610_config *cfg = dev->config;

	uint8_t write_buf[] = {addr | SPI_WRITE, value};
	const struct spi_buf tx_buf = {
		.buf = write_buf,
		.len = sizeof(write_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(&cfg->spi, &tx);
}

static int pmw3610_spi_clk_on(const struct device *dev)
{
	int ret;

	ret = pmw3610_write_reg(dev, PMW3610_SPI_CLK_ON_REQ, SPI_CLOCK_ON_REQ_ON);

	k_sleep(K_USEC(CLOCK_ON_DELAY_US));

	return ret;
}

static int pmw3610_spi_clk_off(const struct device *dev)
{
	return pmw3610_write_reg(dev, PMW3610_SPI_CLK_ON_REQ, SPI_CLOCK_ON_REQ_OFF);
}

static void pmw3610_motion_work_handler(struct k_work *work)
{
	struct pmw3610_data *data = CONTAINER_OF(
			work, struct pmw3610_data, motion_work);
	const struct device *dev = data->dev;
	const struct pmw3610_config *cfg = dev->config;
	uint8_t burst_data[BURST_DATA_LEN_MAX];
	uint8_t burst_data_len;
	int32_t x, y;
	int ret;

	if (cfg->smart_mode) {
		burst_data_len = BURST_DATA_LEN_SMART;
	} else {
		burst_data_len = BURST_DATA_LEN_NORMAL;
	}

	ret = pmw3610_read(dev, PMW3610_BURST_READ, burst_data, burst_data_len);
	if (ret < 0) {
		return;
	}

	if ((burst_data[BURST_MOTION] & MOTION_STATUS_MOTION) == 0x00) {
		return;
	}

	x = ((burst_data[BURST_DELTA_XY_H] << 4) & 0xf00) | burst_data[BURST_DELTA_X_L];
	y = ((burst_data[BURST_DELTA_XY_H] << 8) & 0xf00) | burst_data[BURST_DELTA_Y_L];

	x = sign_extend(x, PMW3610_DATA_SIZE_BITS - 1);
	y = sign_extend(y, PMW3610_DATA_SIZE_BITS - 1);

	input_report_rel(data->dev, cfg->axis_x, x, false, K_FOREVER);
	input_report_rel(data->dev, cfg->axis_y, y, true, K_FOREVER);

	if (cfg->smart_mode) {
		uint16_t shutter_val = sys_get_be16(&burst_data[BURST_SHUTTER_HI]);

		if (data->smart_flag && shutter_val < SHUTTER_SMART_THRESHOLD) {
			pmw3610_spi_clk_on(dev);

			ret = pmw3610_write_reg(dev, PMW3610_SMART_MODE, SMART_MODE_ENABLE);
			if (ret < 0) {
				return;
			}

			pmw3610_spi_clk_off(dev);

			data->smart_flag = false;
		} else if (!data->smart_flag && shutter_val > SHUTTER_SMART_THRESHOLD) {
			pmw3610_spi_clk_on(dev);

			ret = pmw3610_write_reg(dev, PMW3610_SMART_MODE, SMART_MODE_DISABLE);
			if (ret < 0) {
				return;
			}

			pmw3610_spi_clk_off(dev);

			data->smart_flag = true;
		}
	}
}

static void pmw3610_motion_handler(const struct device *gpio_dev,
				   struct gpio_callback *cb,
				   uint32_t pins)
{
	struct pmw3610_data *data = CONTAINER_OF(
			cb, struct pmw3610_data, motion_cb);

	k_work_submit(&data->motion_work);
}

int pmw3610_set_resolution(const struct device *dev, uint16_t res_cpi)
{
	uint8_t val;
	int ret;

	if (!IN_RANGE(res_cpi, RES_MIN, RES_MAX)) {
		LOG_ERR("res_cpi out of range: %d", res_cpi);
		return -EINVAL;
	}

	ret = pmw3610_spi_clk_on(dev);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PWM3610_SPI_PAGE0, SPI_PAGE0_1);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_read_reg(dev, PMW3610_RES_STEP, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~RES_STEP_RES_MASK;
	val |= res_cpi / RES_STEP;

	ret = pmw3610_write_reg(dev, PMW3610_RES_STEP, val);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PWM3610_SPI_PAGE1, SPI_PAGE1_0);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_spi_clk_off(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int pmw3610_force_awake(const struct device *dev, bool enable)
{
	uint8_t val;
	int ret;

	ret = pmw3610_read_reg(dev, PMW3610_PERFORMANCE, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~PERFORMANCE_FMODE_MASK;
	if (enable) {
		val |= PERFORMANCE_FMODE_FORCE_AWAKE;
	} else {
		val |= PERFORMANCE_FMODE_NORMAL;
	}

	ret = pmw3610_spi_clk_on(dev);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PMW3610_PERFORMANCE, val);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_spi_clk_off(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int pmw3610_configure(const struct device *dev)
{
	const struct pmw3610_config *cfg = dev->config;
	uint8_t val;
	int ret;

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("%s is not ready", cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Reset pin configuration failed: %d", ret);
			return ret;
		}

		k_sleep(K_MSEC(RESET_DELAY_MS));

		gpio_pin_set_dt(&cfg->reset_gpio, 0);

		k_sleep(K_MSEC(RESET_DELAY_MS));
	} else {
		ret = pmw3610_write_reg(dev, PMW3610_POWER_UP_RESET, POWER_UP_RESET);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(RESET_DELAY_MS));
	}

	ret = pmw3610_read_reg(dev, PMW3610_PROD_ID, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != PRODUCT_ID_PMW3610) {
		LOG_ERR("Invalid product id: %02x", val);
		return -ENOTSUP;
	}

	/* Power-up init sequence */
	ret = pmw3610_spi_clk_on(dev);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PMW3610_OBSERVATION1, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(INIT_OBSERVATION_DELAY_MS));

	ret = pmw3610_read_reg(dev, PMW3610_OBSERVATION1, &val);
	if (ret < 0) {
		return ret;
	}

	if ((val & OBSERVATION1_INIT_MASK) != OBSERVATION1_INIT_MASK) {
		LOG_ERR("Unexpected OBSERVATION1 value: %02x", val);
		return -EINVAL;
	}

	for (uint8_t reg = PMW3610_MOTION; reg <= PMW3610_DELTA_XY_H; reg++) {
		ret = pmw3610_read_reg(dev, reg, &val);
		if (ret < 0) {
			return ret;
		}
	}

	ret = pmw3610_write_reg(dev, PMW3610_PERFORMANCE, PERFORMANCE_INIT);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PMW3610_RUN_DOWNSHIFT, RUN_DOWNSHIFT_INIT);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PMW3610_REST1_RATE, REST1_RATE_INIT);
	if (ret < 0) {
		return ret;
	}

	ret = pmw3610_write_reg(dev, PMW3610_REST1_DOWNSHIFT, REST1_DOWNSHIFT_INIT);
	if (ret < 0) {
		return ret;
	}

	/* Configuration */

	if (cfg->invert_x || cfg->invert_y) {
		ret = pmw3610_write_reg(dev, PWM3610_SPI_PAGE0, SPI_PAGE0_1);
		if (ret < 0) {
			return ret;
		}

		ret = pmw3610_read_reg(dev, PMW3610_RES_STEP, &val);
		if (ret < 0) {
			return ret;
		}

		WRITE_BIT(val, RES_STEP_INV_X_BIT, cfg->invert_x);
		WRITE_BIT(val, RES_STEP_INV_Y_BIT, cfg->invert_y);

		ret = pmw3610_write_reg(dev, PMW3610_RES_STEP, val);
		if (ret < 0) {
			return ret;
		}

		ret = pmw3610_write_reg(dev, PWM3610_SPI_PAGE1, SPI_PAGE1_0);
		if (ret < 0) {
			return ret;
		}

	}

	ret = pmw3610_spi_clk_off(dev);
	if (ret < 0) {
		return ret;
	}

	/* The remaining functions call spi_clk_on/off independently. */

	if (cfg->res_cpi > 0) {
		pmw3610_set_resolution(dev, cfg->res_cpi);
	}

	pmw3610_force_awake(dev, cfg->force_awake);

	return 0;
}

static int pmw3610_init(const struct device *dev)
{
	const struct pmw3610_config *cfg = dev->config;
	struct pmw3610_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("%s is not ready", cfg->spi.bus->name);
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init(&data->motion_work, pmw3610_motion_work_handler);

	if (!gpio_is_ready_dt(&cfg->motion_gpio)) {
		LOG_ERR("%s is not ready", cfg->motion_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->motion_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Motion pin configuration failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->motion_cb, pmw3610_motion_handler,
			   BIT(cfg->motion_gpio.pin));

	ret = gpio_add_callback_dt(&cfg->motion_gpio, &data->motion_cb);
	if (ret < 0) {
		LOG_ERR("Could not set motion callback: %d", ret);
		return ret;
	}

	ret = pmw3610_configure(dev);
	if (ret != 0) {
		LOG_ERR("Device configuration failed: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->motion_gpio,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Motion interrupt configuration failed: %d", ret);
		return ret;
	}

	ret = pm_device_runtime_enable(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime power management: %d", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int pmw3610_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pmw3610_write_reg(dev, PMW3610_SHUTDOWN, SHUTDOWN_ENABLE);
		if (ret < 0) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = pmw3610_write_reg(dev, PMW3610_POWER_UP_RESET, POWER_UP_WAKEUP);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define PMW3610_SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | \
			  SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define PMW3610_INIT(n)								\
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP_OR(n, res_cpi, RES_MIN),		\
			      RES_MIN, RES_MAX), "invalid res-cpi");		\
										\
	static const struct pmw3610_config pmw3610_cfg_##n = {			\
		.spi = SPI_DT_SPEC_INST_GET(n, PMW3610_SPI_MODE, 0),		\
		.motion_gpio = GPIO_DT_SPEC_INST_GET(n, motion_gpios),		\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),	\
		.axis_x = DT_INST_PROP(n, zephyr_axis_x),			\
		.axis_y = DT_INST_PROP(n, zephyr_axis_y),			\
		.res_cpi = DT_INST_PROP_OR(n, res_cpi, -1),			\
		.invert_x = DT_INST_PROP(n, invert_x),				\
		.invert_y = DT_INST_PROP(n, invert_y),				\
		.force_awake = DT_INST_PROP(n, force_awake),			\
		.smart_mode = DT_INST_PROP(n, smart_mode),			\
	};									\
										\
	static struct pmw3610_data pmw3610_data_##n;				\
										\
	PM_DEVICE_DT_INST_DEFINE(n, pmw3610_pm_action);				\
										\
	DEVICE_DT_INST_DEFINE(n, pmw3610_init, PM_DEVICE_DT_INST_GET(n),	\
			      &pmw3610_data_##n, &pmw3610_cfg_##n,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PMW3610_INIT)
