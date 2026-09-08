/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adv7535

#include "zephyr/drivers/gpio.h"
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "display_adi_adv7535.h"

LOG_MODULE_REGISTER(adi_adv7535, CONFIG_DISPLAY_LOG_LEVEL);

struct display_timings {
	uint16_t hactive;
	uint16_t hsync;
	uint16_t hfp;
	uint16_t hbp;
	uint16_t vactive;
	uint16_t vsync;
	uint16_t vfp;
	uint16_t vbp;
};

struct adv7535_i2c_conf {
	uint8_t main_addr;
	uint8_t edid_addr;
	uint8_t packet_addr;
	uint8_t cec_addr;
	uint8_t fixed_addr;
	const struct device *bus;
};

struct adv7535_config {
	const struct device *mipi_dsi_host;
	void (*make_thread)(const struct device *dev);
	uint8_t channel;
	uint8_t data_lanes;
	int8_t test_pattern;
	struct adv7535_i2c_conf i2c;
	struct gpio_dt_spec dt_pd;
	struct gpio_dt_spec dt_int;
	struct display_timings display_timings;
};

struct adv7535_data {
	struct gpio_callback int_gpio_cb;
	struct k_sem irq_sem;
	uint8_t int_0_reg;
	uint8_t int_1_reg;
};

static bool adv7535_i2c_bus_ready(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;

	return device_is_ready(config->i2c.bus);
}

static const char *adv7535_i2c_bus_name(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;

	return config->i2c.bus->name;
}

static inline uint8_t adv7535_reg_map_to_addr(const struct device *dev,
					      enum adv7535_reg_map reg_map)
{
	const struct adv7535_config *config = dev->config;

	switch (reg_map) {
	case ADV7535_PACKET:
		return config->i2c.packet_addr;
	case ADV7535_EDID:
		return config->i2c.edid_addr;
	case ADV7535_CEC:
		return config->i2c.cec_addr;
	case ADV7535_MAIN:
	default:
		return config->i2c.main_addr;
	}
}

static int adv7535_write(const struct device *dev, enum adv7535_reg_map reg_map, uint8_t reg,
			 uint8_t val)
{
	const struct adv7535_config *config = dev->config;
	const struct device *i2c_dev = config->i2c.bus;
	int ret = 0;
	uint8_t i2c_addr = adv7535_reg_map_to_addr(dev, reg_map);
	uint8_t buf[2] = {reg, val};

	ret = i2c_write(i2c_dev, buf, ARRAY_SIZE(buf), i2c_addr);
	if (ret) {
		LOG_ERR("Could not write to address 0x%02x, register 0x%02x, value 0x%02x",
			i2c_addr, reg, val);
	}

	return ret;
}

static int adv7535_read(const struct device *dev, enum adv7535_reg_map reg_map, uint8_t reg,
			uint8_t *buf)
{
	const struct adv7535_config *config = dev->config;
	const struct device *i2c_dev = config->i2c.bus;
	int ret = 0;
	uint8_t i2c_addr = adv7535_reg_map_to_addr(dev, reg_map);

	ret = i2c_write_read(i2c_dev, i2c_addr, &reg, 1, buf, 1);
	if (ret) {
		LOG_ERR("Could not read address 0x%02x, register 0x%02x", i2c_addr, reg);
	}

	return ret;
}

static int adv7535_write_bit(const struct device *dev, enum adv7535_reg_map reg_map, uint8_t reg,
			     uint8_t bit, bool val)
{
	int ret;
	uint8_t buf;

	ret = adv7535_read(dev, reg_map, reg, &buf);
	if (ret) {
		return ret;
	}

	buf &= ~bit;
	buf |= val ? bit : 0;

	return adv7535_write(dev, reg_map, reg, buf);
}

static int adv7535_read_bit(const struct device *dev, enum adv7535_reg_map reg_map, uint8_t reg,
			    uint8_t bit, bool *buf)
{
	int ret;
	uint8_t byte_buf;

	ret = adv7535_read(dev, reg_map, reg, &byte_buf);
	if (ret) {
		return ret;
	}

	byte_buf &= bit;
	*buf = (bool)(byte_buf);

	return 0;
}

static int adv7535_burst_write(const struct device *dev, enum adv7535_reg_map reg_map,
			       uint8_t start_reg, const uint8_t *data, uint32_t num_bytes)
{
	const struct adv7535_config *config = dev->config;
	const struct device *i2c_dev = config->i2c.bus;
	int ret = 0;
	uint8_t i2c_addr = adv7535_reg_map_to_addr(dev, reg_map);

	ret = i2c_burst_write(i2c_dev, i2c_addr, start_reg, data, num_bytes);
	if (ret) {
		LOG_ERR("Failed during a burst I2C write to address 0x%02x starting from register "
			"0x%02x",
			i2c_addr, start_reg);
	}

	return ret;
}

static int adv7535_set_fixed_registers(const struct device *dev)
{
	int ret = 0;

	ARRAY_FOR_EACH(adv7535_fixed_registers, i) {
		ret = adv7535_write(dev, ADV7535_MAIN, adv7535_fixed_registers[i].reg,
				    adv7535_fixed_registers[i].val);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int adv7535_set_cec_fixed_registers(const struct device *dev)
{
	int ret = 0;

	ARRAY_FOR_EACH(adv7535_cec_fixed_registers, i) {
		ret = adv7535_write(dev, ADV7535_CEC, adv7535_cec_fixed_registers[i].reg,
				    adv7535_cec_fixed_registers[i].val);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int adv7535_enable_interrupts(const struct device *dev)
{
	/* Enable hdmi connect/disconnect detection */
	return adv7535_write(dev, ADV7535_MAIN, ADV7535_REG_INT_ENABLE_0,
			     ADV7535_INT_0_MONITOR_SENSE);
}

static int adv7535_set_test_pattern(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	uint8_t val;

	switch (config->test_pattern) {
	case ADV7535_DTS_TEST_PATTERN_DISABLE:
		val = ADV7535_TEST_PATTERN_DISABLE;
		break;
	case ADV7535_DTS_TEST_PATTERN_COLOR_BARS:
		val = ADV7535_TEST_PATTERN_COLOR_BARS;
		break;
	case ADV7535_DTS_TEST_PATTERN_GRAYSCALE_GRADIENT:
		val = ADV7535_TEST_PATTERN_GRAYSCALE_GRADIENT;
		break;
	default:
		return -EINVAL;
	}

	return adv7535_write(dev, ADV7535_CEC, ADV7535_REG_CEC_TEST_PATTERN, val);
}

static int adv7535_enable_dsi_clock(const struct device *dev)
{
	int ret;

	/* Enable DSI LP oscillator and DSI bias clock */
	ret = adv7535_write_bit(dev, ADV7535_CEC, 0x03, BIT(1), 0);
	if (ret) {
		return ret;
	}

	/* Reset internal timing generator */
	ret = adv7535_write(dev, ADV7535_CEC, 0x27, 0xcb);
	if (ret) {
		return ret;
	}
	ret = adv7535_write(dev, ADV7535_CEC, 0x27, 0x8b);
	if (ret) {
		return ret;
	}
	return adv7535_write(dev, ADV7535_CEC, 0x27, 0xcb);
}

static int adv7535_configure_dsi_lanes(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	int ret = 0;
	/* Assumes 2 <= data_lanes <= 4 is true */
	const uint8_t clock_div_by_lanes = 12 / config->data_lanes;

	/* Set the number of dsi lanes */
	ret = adv7535_write(dev, ADV7535_CEC, 0x1C, config->data_lanes << 4);
	if (ret) {
		return ret;
	}

	/* Set pixel clock divider */
	return adv7535_write(dev, ADV7535_CEC, 0x16, clock_div_by_lanes << 3);
}

static int adv7535_configure_dsi_timings(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	const struct display_timings *dp = &config->display_timings;
	uint16_t htotal, vtotal;

	htotal = dp->hactive + dp->hsync + dp->hfp + dp->hbp;
	vtotal = dp->vactive + dp->vsync + dp->vfp + dp->vbp;

	const uint8_t timings[] = {
		/* Horizontal Timings */
		htotal >> 4,
		htotal << 4,
		dp->hsync >> 4,
		dp->hsync << 4,
		dp->hfp >> 4,
		dp->hfp << 4,
		dp->hbp >> 4,
		dp->hbp << 4,

		/* Vertical Timings */
		vtotal >> 4,
		vtotal << 4,
		dp->vsync >> 4,
		dp->vsync << 4,
		dp->vfp >> 4,
		dp->vfp << 4,
		dp->vbp >> 4,
		dp->vbp << 4,
	};

	return adv7535_burst_write(dev, ADV7535_CEC, ADV7535_REG_TIMING_START, timings,
				   ARRAY_SIZE(timings));
}

static int adv7535_select_transmitter_mode(const struct device *dev)
{
	/*
	 * Use DVI mode while the driver supports only basic video output.
	 * The driver does not currently read EDID or generate HDMI InfoFrames
	 * or audio packets. DVI-compatible video can also be displayed by HDMI
	 * sinks. This will need to be set properly when HDMI video metadata,
	 * audio, or sink detection is added.
	 */
	return adv7535_write_bit(dev, ADV7535_MAIN, ADV7535_REG_HDMI_OPTS, ADV7535_HDMI_OPTS_MODE,
				 ADV7535_HDMI_OPTS_MODE_DVI);
}

static int adv7535_enable_output(const struct device *dev)
{
	return adv7535_write_bit(dev, ADV7535_CEC, 0x03, BIT(7), 1);
}

static int adv7535_configure(const struct device *dev)
{
	int ret;

	ret = adv7535_enable_interrupts(dev);
	if (ret) {
		LOG_ERR("Failed to enable interrupts");
		return ret;
	}

	ret = adv7535_set_fixed_registers(dev);
	if (ret) {
		LOG_ERR("Failed to set fixed main registers");
		return ret;
	}

	ret = adv7535_enable_dsi_clock(dev);
	if (ret) {
		LOG_ERR("Failed to enable and reset DSI clock");
		return ret;
	}

	ret = adv7535_configure_dsi_lanes(dev);
	if (ret) {
		LOG_ERR("Failed to configure DSI lanes settings");
		return ret;
	}

	ret = adv7535_configure_dsi_timings(dev);
	if (ret) {
		LOG_ERR("Failed to configure DSI timings");
		return ret;
	}

	ret = adv7535_set_cec_fixed_registers(dev);
	if (ret) {
		LOG_ERR("Failed to set fixed cec registers");
		return ret;
	}

	ret = adv7535_select_transmitter_mode(dev);
	if (ret) {
		LOG_ERR("Failed to select transmitter mode");
		return ret;
	}

	ret = adv7535_enable_output(dev);
	if (ret) {
		LOG_ERR("Failed to enable output");
		return ret;
	}

	ret = adv7535_set_test_pattern(dev);
	if (ret) {
		LOG_ERR("Failed to configure test pattern");
		return ret;
	}

	return 0;
}

static int adv7535_power_up(const struct device *dev)
{
	int ret;

	ret = adv7535_write_bit(dev, ADV7535_MAIN, ADV7535_REG_POWER, ADV7535_POWER_DOWN, 0);
	if (ret) {
		return ret;
	}

	return adv7535_configure(dev);
}

static int adv7535_power_down(const struct device *dev)
{
	int ret;

	ret = adv7535_write_bit(dev, ADV7535_MAIN, ADV7535_REG_POWER, ADV7535_POWER_DOWN, 1);

	return ret;
}

static int adv7535_attach_to_mipi_dsi_host(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	const struct display_timings *dt = &config->display_timings;
	struct mipi_dsi_device mdev = {0};
	int ret;

	mdev.timings.hactive = dt->hactive;
	mdev.timings.hsync = dt->hsync;
	mdev.timings.hfp = dt->hfp;
	mdev.timings.hbp = dt->hbp;

	mdev.timings.vactive = dt->vactive;
	mdev.timings.vsync = dt->vsync;
	mdev.timings.vfp = dt->vfp;
	mdev.timings.vbp = dt->vbp;

	mdev.data_lanes = config->data_lanes;
	mdev.pixfmt = MIPI_DSI_PIXFMT_RGB888;

	mdev.mode_flags =
		MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE | MIPI_DSI_MODE_VIDEO_HSE;

	if (!device_is_ready(config->mipi_dsi_host)) {
		LOG_ERR("MIPI DSI Host not ready");
		return -EINVAL;
	}

	ret = mipi_dsi_attach(config->mipi_dsi_host, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI-DSI host");
	}
	return ret;
}

static int adv7535_set_i2c_addresses(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	struct reg_val_pair addresses[] = {
		{ADV7535_REG_EDID_ADDR, config->i2c.edid_addr},
		{ADV7535_REG_PACKET_MEM_ADDR, config->i2c.packet_addr},
		{ADV7535_REG_CEC_ADDR, config->i2c.cec_addr},
		{ADV7535_REG_FIXED_ADDR, config->i2c.fixed_addr},
	};
	int ret = 0;

	/* NOTE: Main address is set by the state on the Power Down pin during power up */

	ARRAY_FOR_EACH(addresses, i) {
		ret = adv7535_write(dev, ADV7535_MAIN, addresses[i].reg, addresses[i].val << 1);
		if (ret) {
			return ret;
		}
	}

	return ret;
}

static int adv7535_configure_rst_gpio(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	int ret = 0;

	if (config->dt_pd.port) {
		if (!gpio_is_ready_dt(&config->dt_pd)) {
			LOG_ERR("GPIO device %s not ready", config->dt_pd.port->name);
			return -EIO;
		}

		ret = gpio_pin_configure_dt(&config->dt_pd, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to configure GPIO pin %u", config->dt_pd.pin);
			return ret;
		}
	}

	return ret;
}

static int adv7535_probe_and_handle_monitor_sense(const struct device *dev)
{
	bool is_connected;
	int ret = 0;

	ret = adv7535_read_bit(dev, ADV7535_MAIN, ADV7535_REG_PORT_STATE,
			       ADV7535_MONITOR_SENSE_STATE, &is_connected);
	if (ret) {
		return ret;
	}

	if (is_connected) {
		ret = adv7535_power_up(dev);
		if (ret) {
			LOG_ERR("Failed powering up");
		}
	} else {
		ret = adv7535_power_down(dev);
		if (ret) {
			LOG_ERR("Failed powering down");
		}
	}

	LOG_INF("%s detected", is_connected ? "Connect" : "Disconnect");

	return ret;
}

static int adv7535_update_int(const struct device *dev)
{
	struct adv7535_data *data = dev->data;
	int ret = 0;

	ret = adv7535_read(dev, ADV7535_MAIN, ADV7535_REG_INT_0, &data->int_0_reg);
	if (!ret) {
		ret = adv7535_read(dev, ADV7535_MAIN, ADV7535_REG_INT_1, &data->int_1_reg);
	}

	if (ret) {
		LOG_ERR("Failed to read interrupt registers");
	}
	return ret;
}

static int adv7535_clear_int(const struct device *dev)
{
	struct adv7535_data *data = dev->data;
	int ret = 0;

	ret = adv7535_write(dev, ADV7535_MAIN, ADV7535_REG_INT_0, data->int_0_reg);
	if (!ret) {
		ret = adv7535_write(dev, ADV7535_MAIN, ADV7535_REG_INT_1, data->int_1_reg);
	}

	if (ret) {
		LOG_ERR("Failed to clear interrupt registers");
	}
	return ret;
}

static void adv7535_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct device *dev = p1;
	const struct adv7535_config *config = dev->config;
	struct adv7535_data *data = dev->data;
	bool non_recoverable_failure = false;

	LOG_INF("ADV7535 Thread started");

	while (!non_recoverable_failure) {
		int ret;

		k_sem_take(&data->irq_sem, K_FOREVER);

		LOG_DBG("Interrupt!");

		do {
			if (adv7535_update_int(dev)) {
				non_recoverable_failure = true;
				break;
			}

			if (data->int_0_reg & ADV7535_INT_0_MONITOR_SENSE) {
				ret = adv7535_probe_and_handle_monitor_sense(dev);
				if (ret) {
					LOG_ERR("Failed to handle a hotplug event");
				}
			}

			if (adv7535_clear_int(dev)) {
				non_recoverable_failure = true;
				break;
			}

			/* We check interrupt gpio again to make sure a new interrupt did not occur,
			 * while we were handling the current one.
			 *
			 * ADV7535 signals a presence of a interrupt by pulling interrupt pin low,
			 * it goes to high only while all interrupts have been resolved.
			 *
			 * Since not all boards support GPIO_INT_LEVEL_LOW or GPIO_INT_LEVEL_ACTIVE,
			 * we use GPIO_INT_EDGE_TO_ACTIVE and just recheck interrupt gpio state.
			 */
		} while (gpio_pin_get_dt(&config->dt_int) == 1);
	}

	LOG_ERR("A interrupt error occurred. New interrupts will not be handeled");
}

static void adv7535_int_gpio_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adv7535_data *data = CONTAINER_OF(cb, struct adv7535_data, int_gpio_cb);

	k_sem_give(&data->irq_sem);
}

static int adv7535_remove_int_callback(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	struct adv7535_data *data = dev->data;

	return gpio_remove_callback_dt(&config->dt_int, &data->int_gpio_cb);
}

static int adv7535_configure_int_gpio(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	struct adv7535_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->dt_int)) {
		LOG_ERR("GPIO device %s not ready", config->dt_int.port->name);
		return -EIO;
	}

	gpio_init_callback(&data->int_gpio_cb, adv7535_int_gpio_cb, BIT(config->dt_int.pin));

	ret = gpio_add_callback_dt(&config->dt_int, &data->int_gpio_cb);
	if (ret) {
		goto error;
	}

	ret = gpio_pin_configure_dt(&config->dt_int, GPIO_INPUT);
	if (ret) {
		goto error;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->dt_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		goto error;
	}

	return 0;

error:
	adv7535_remove_int_callback(dev);
	return ret;
}

static int adv7535_disable_and_clear_all_interrupts(const struct device *dev)
{
	int ret;

	uint8_t int_enable_regs[] = {
		ADV7535_REG_CEC_INT_ENABLE,
		ADV7535_REG_INT_ENABLE_0,
		ADV7535_REG_INT_ENABLE_1,
	};

	uint8_t int_regs[] = {ADV7535_REG_INT_0, ADV7535_REG_INT_1, ADV7535_REG_CEC_INT};

	ARRAY_FOR_EACH(int_enable_regs, i) {
		ret = adv7535_write(dev, ADV7535_MAIN, int_enable_regs[i], 0);
		if (ret) {
			return ret;
		}
	}

	ARRAY_FOR_EACH(int_regs, i) {
		ret = adv7535_write(dev, ADV7535_MAIN, int_regs[i], 0xff);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int adv7535_reset(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	int ret = 0;

	if (config->dt_pd.port) {
		ret = gpio_pin_set_dt(&config->dt_pd, 1);
		k_msleep(5);
		ret |= gpio_pin_set_dt(&config->dt_pd, 0);
		LOG_INF("Reset using Power Down pin");
	} else {
		LOG_INF("Powerd Down pin was not provided, skipping reset.");
	}

	return ret;
}

static int adv7535_init(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	int ret;
	uint8_t revision;

	if (!adv7535_i2c_bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", adv7535_i2c_bus_name(dev));
		return -EINVAL;
	}

	ret = adv7535_configure_rst_gpio(dev);
	if (ret) {
		LOG_ERR("Failed configuring reset GPIO");
		return ret;
	}

	ret = adv7535_reset(dev);
	if (ret) {
		LOG_ERR("Failed resetting the device");
		return ret;
	}

	ret = adv7535_set_i2c_addresses(dev);
	if (ret) {
		LOG_ERR("Failed setting the I2C addresses");
		return ret;
	}

	ret = adv7535_disable_and_clear_all_interrupts(dev);
	if (ret) {
		LOG_ERR("Failed disabling interrupts");
		return ret;
	}

	ret = adv7535_configure_int_gpio(dev);
	if (ret) {
		LOG_ERR("Failed configuring interrupt GPIO");
		goto error;
	}

	ret = adv7535_enable_interrupts(dev);
	if (ret) {
		LOG_ERR("Failed enabling interrupts");
		goto error;
	}

	ret = adv7535_power_down(dev);
	if (ret) {
		LOG_ERR("Failed to power off ADV7535");
		goto error;
	}

	ret = adv7535_set_fixed_registers(dev);
	if (ret) {
		LOG_ERR("Failed to set fixed registers");
		goto error;
	}

	ret = adv7535_set_cec_fixed_registers(dev);
	if (ret) {
		LOG_ERR("Failed to set CEC fixed registers");
		goto error;
	}

	ret = adv7535_attach_to_mipi_dsi_host(dev);
	if (ret) {
		LOG_ERR("Failed to attach to MIPI DSI host");
		goto error;
	}

	ret = adv7535_probe_and_handle_monitor_sense(dev);
	if (ret) {
		LOG_ERR("Failed initial probing of sink presence");
	}

	config->make_thread(dev);

	ret = adv7535_read(dev, ADV7535_MAIN, ADV7535_REG_REVISION, &revision);
	if (!ret) {
		LOG_INF("ADV7535 initialized. Chip Revision: 0x%02x", revision);
	}

	return 0;

error:
	adv7535_remove_int_callback(dev);
	return ret;
}

#define ADV7535_IS_PD_ACTIVE_LOW(id) (DT_INST_GPIO_FLAGS(id, pd_gpios) & GPIO_ACTIVE_LOW)

#define ADV7535_DT_GET_I2C_ADDR(id) DT_INST_PROP(id, i2c_addr)

#define ADV7535_IS_PD_AND_I2C_ADDR_VALID(id)                                                       \
	((ADV7535_DT_GET_I2C_ADDR(id) == 0x39 && !(ADV7535_IS_PD_ACTIVE_LOW(id))) ||               \
	 (ADV7535_DT_GET_I2C_ADDR(id) == 0x3d && (ADV7535_IS_PD_ACTIVE_LOW(id))))

#define ADV7535_VALIDATE_PD_AND_I2C_ADDR(id)                                                       \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(id, pd_gpios),                                            \
	(BUILD_ASSERT((ADV7535_IS_PD_AND_I2C_ADDR_VALID(id)),                                      \
		"ADV7535 I2C address does not match pd-gpios polarity. "                           \
		"0x39 requres active high, 0x3d requres active low."                               \
	      ))                                                                                   \
	);

#define ADV7535_DEFINE(id)                                                                         \
	static K_KERNEL_STACK_DEFINE(drv_stack_##id, CONFIG_ADV7535_THREAD_STACK_SIZE);            \
	static struct k_thread drv_stack_data_##id;                                                \
                                                                                                   \
	static void adv7535_make_thread_##id(const struct device *dev)                             \
	{                                                                                          \
		k_thread_create(&drv_stack_data_##id, drv_stack_##id,                              \
				K_KERNEL_STACK_SIZEOF(drv_stack_##id), adv7535_thread,             \
				(void *)dev, NULL, NULL, CONFIG_ADV7535_THREAD_PRIORITY, 0,        \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&drv_stack_data_##id, "adv7535_" STRINGIFY(id));                 \
	}                                                                                          \
                                                                                                   \
	static const struct adv7535_config config_##id = {                                         \
		.mipi_dsi_host = DEVICE_DT_GET(DT_INST_PARENT(id)),                                \
		.make_thread = adv7535_make_thread_##id,                                           \
		.channel = DT_INST_REG_ADDR(id),                                                   \
		.data_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                              \
		.test_pattern = DT_INST_PROP_OR(id, test_pattern, -1),                             \
		.i2c =                                                                             \
			{                                                                          \
				.bus = DEVICE_DT_GET(DT_INST_PHANDLE(id, i2c)),                    \
				.main_addr = DT_INST_PROP(id, i2c_addr),                           \
				.edid_addr = DT_INST_PROP_OR(id, edid_addr,                        \
							     ADV7535_I2C_EDID_ADDR_DEFAULT),       \
				.packet_addr = DT_INST_PROP_OR(id, packet_addr,                    \
							       ADV7535_I2C_PACKET_ADDR_DEFAULT),   \
				.cec_addr = DT_INST_PROP_OR(id, cec_addr,                          \
							    ADV7535_I2C_CEC_ADDR_DEFAULT),         \
				.fixed_addr = DT_INST_PROP_OR(id, fixed_addr,                      \
							      ADV7535_I2C_FIXED_ADDR_DEFAULT),     \
			},                                                                         \
		.dt_pd = GPIO_DT_SPEC_INST_GET_OR(id, pd_gpios, {0}),                              \
		.dt_int = GPIO_DT_SPEC_INST_GET(id, int_gpios),                                    \
		.display_timings = {                                                               \
			.hactive = DT_INST_PROP(id, hactive),                                      \
			.hsync = DT_INST_PROP(id, hsync),                                          \
			.hfp = DT_INST_PROP(id, hfp),                                              \
			.hbp = DT_INST_PROP(id, hbp),                                              \
			.vactive = DT_INST_PROP(id, vactive),                                      \
			.vsync = DT_INST_PROP(id, vsync),                                          \
			.vfp = DT_INST_PROP(id, vfp),                                              \
			.vbp = DT_INST_PROP(id, vbp),                                              \
		}};                                                                                \
	static struct adv7535_data data_##id = {                                                   \
		.irq_sem = Z_SEM_INITIALIZER(data_##id.irq_sem, 0, 1)};                            \
	DEVICE_DT_INST_DEFINE(id, adv7535_init, NULL, &data_##id, &config_##id, POST_KERNEL,       \
			      CONFIG_DISPLAY_INIT_PRIORITY, NULL);                                 \
	ADV7535_VALIDATE_PD_AND_I2C_ADDR(id)

DT_INST_FOREACH_STATUS_OKAY(ADV7535_DEFINE)
