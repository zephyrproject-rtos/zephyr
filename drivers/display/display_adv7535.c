/*
 * Copyright (C) 2026, Savoir-faire Linux, Inc.
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adv7535

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define ADV7535_DTS_TEST_PATTERN_DISABLE            -1
#define ADV7535_DTS_TEST_PATTERN_COLOR_BARS         0
#define ADV7535_DTS_TEST_PATTERN_GRAYSCALE_GRADIENT 1

#define ADV7535_I2C_EDID_ADDR_DEFAULT   0x3f
#define ADV7535_I2C_PACKET_ADDR_DEFAULT 0x38
#define ADV7535_I2C_CEC_ADDR_DEFAULT    0x3c
#define ADV7535_I2C_FIXED_ADDR_DEFAULT  0x00

#define ADV7535_REG_EDID_ADDR       0x43
#define ADV7535_REG_PACKET_MEM_ADDR 0x45
#define ADV7535_REG_CEC_ADDR        0xe1
#define ADV7535_REG_FIXED_ADDR      0xf9

#define ADV7535_REG_REVISION 0x00

#define ADV7535_REG_CEC_ID_LB 0x01
#define ADV7535_REG_CEC_ID_HB 0x00

#define ADV7535_REG_POWER 0x41

#define ADV7535_POWER_DOWN BIT(6) /* 0 = Powered up; 1 = Powered down */

#define ADV7535_REG_ENABLE_0 0x40

#define ADV7535_ENABLE_0_PACKET_SPARE_1 BIT(0)
#define ADV7535_ENABLE_0_PACKET_SPARE_2 BIT(1)
#define ADV7535_ENABLE_0_PACKET_GM      BIT(2)
#define ADV7535_ENABLE_0_PACKET_ISRC    BIT(3)
#define ADV7535_ENABLE_0_PACKET_ACP     BIT(4)
#define ADV7535_ENABLE_0_PACKET_MPEG    BIT(5)
#define ADV7535_ENABLE_0_PACKET_SPD     BIT(6)
#define ADV7535_ENABLE_0_PACKET_GC      BIT(7)

#define ADV7535_REG_ENABLE_1 0x44

#define ADV7535_ENABLE_1_PACKET_MEM_READ_MODE BIT(0)
/* Bits 1 and 2 are reserved and should not be used */
#define ADV7535_ENABLE_1_AUDIO_INFO_FRAME     BIT(3)
#define ADV7535_ENABLE_1_AVI_INFO_FRAME       BIT(4)
#define ADV7535_ENABLE_1_PACKET_AUDIO_SAMPLE  BIT(5)
#define ADV7535_ENABLE_1_PACKET_N_CTS         BIT(6)
/* Bit 7 is reserved and should not be used */

#define ADV7535_REG_CEC_POWER_DOWN 0xe2

#define ADV7535_CEC_POWER_DOWN BIT(0)

#define ADV7535_REG_CEC_INT_ENABLE 0x92
#define ADV7535_REG_INT_ENABLE_0   0x94
#define ADV7535_REG_INT_ENABLE_1   0x95

#define ADV7535_REG_CEC_INT 0x93
#define ADV7535_REG_INT_0   0x96
#define ADV7535_REG_INT_1   0x97

#define ADV7535_INT_0_HPD             BIT(7)
#define ADV7535_INT_0_MONITOR_SENSE   BIT(6)
#define ADV7535_INT_0_VSYNC           BIT(5)
#define ADV7535_INT_0_AUDIO_FIFO_FULL BIT(4)
/* BIT(3) is reserved */
#define ADV7535_INT_0_EDID_READY      BIT(2)
#define ADV7535_INT_0_HDCP_AUTH       BIT(1)
/* BIT(0) is reserved */

#define ADV7535_INT_1_DCC_ERR             BIT(7)
#define ADV7535_INT_1_BKSV                BIT(6)
#define ADV7535_INT_1_TX_READY            BIT(5)
#define ADV7535_INT_1_TX_ARBITRATION_LOST BIT(4)
#define ADV7535_INT_1_TX_RETRY_TIMEOUT    BIT(3)
#define ADV7535_INT_1_RX_READY_3          BIT(2)
#define ADV7535_INT_1_RX_READY_2          BIT(1)
#define ADV7535_INT_1_RX_READY_1          BIT(0)

#define ADV7535_REG_PORT_STATE 0x42

#define ADV7535_PORT_STATE_HPD           BIT(6)
#define ADV7535_PORT_STATE_MONITOR_SENSE BIT(5)

#define ADV7535_REG_CEC_TEST_PATTERN 0x55

#define ADV7535_TEST_PATTERN_DISABLE            0x00
#define ADV7535_TEST_PATTERN_COLOR_BARS         0x80
#define ADV7535_TEST_PATTERN_GRAYSCALE_GRADIENT 0xA0

#define ADV7535_REG_HDMI_OPTS 0xaf

#define ADV7535_HDMI_OPTS_MODE      BIT(1)
#define ADV7535_HDMI_OPTS_MODE_DVI  0
#define ADV7535_HDMI_OPTS_MODE_HDMI 1

#define ADV7535_REG_TIMING_START  0x28
#define ADV7535_TIMING_REGS_COUNT 16

#define ADV7535_TIMING_MAX 0xfff

LOG_MODULE_REGISTER(adv7535, CONFIG_DISPLAY_LOG_LEVEL);

BUILD_ASSERT(CONFIG_ADV7535_INIT_PRIORITY > CONFIG_MIPI_DSI_INIT_PRIORITY,
	     "ADV7535 must initialize after the MIPI DSI host");

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
	bool sink_present;
};

enum adv7535_reg_map {
	ADV7535_MAIN,
	ADV7535_PACKET,
	ADV7535_EDID,
	ADV7535_CEC
};

struct reg_val_pair {
	uint8_t reg, val;
};

static const struct reg_val_pair adv7535_fixed_registers[] = {
	{0x16, 0x20}, {0x9a, 0xe0}, {0xba, 0x70}, {0xde, 0x82}, {0xe4, 0x40}, {0xe5, 0x80},
};

static const struct reg_val_pair adv7535_cec_fixed_registers[] = {
	{0x15, 0xd0}, {0x17, 0xd0}, {0x24, 0x20}, {0x57, 0x11}, {0x05, 0xc8},
};

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
	const struct adv7535_config *config = dev->config;
	const struct device *i2c_dev = config->i2c.bus;
	uint8_t i2c_addr = adv7535_reg_map_to_addr(dev, reg_map);
	int ret;

	ret = i2c_reg_update_byte(i2c_dev, i2c_addr, reg, bit, val ? 0xFFU : 0U);
	if (ret < 0) {
		LOG_ERR("Could not update I2C reg 0x%02x, address 0x%02x", reg, i2c_addr);
	}

	return ret;
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
			     ADV7535_INT_0_MONITOR_SENSE | ADV7535_INT_0_HPD);
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

	const uint8_t timings[ADV7535_TIMING_REGS_COUNT] = {
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
	struct adv7535_data *data = dev->data;
	int ret = 0;
	uint8_t port_state;
	bool is_connected;

	ret = adv7535_read(dev, ADV7535_MAIN, ADV7535_REG_PORT_STATE, &port_state);
	if (ret) {
		return ret;
	}

	is_connected = (port_state & ADV7535_PORT_STATE_HPD) &&
		       (port_state & ADV7535_PORT_STATE_MONITOR_SENSE);

	if (is_connected && !data->sink_present) {
		data->sink_present = true;
		ret = adv7535_power_up(dev);
		if (ret) {
			LOG_ERR("Failed powering up");
		}
		LOG_INF("Connect detected");
	}

	if (!is_connected && data->sink_present) {
		data->sink_present = false;
		ret = adv7535_power_down(dev);
		if (ret) {
			LOG_ERR("Failed powering down");
		}
		LOG_INF("Disconnect detected");
	}

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

			if (data->int_0_reg & (ADV7535_INT_0_MONITOR_SENSE | ADV7535_INT_0_HPD)) {
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

static int adv7535_get_id(const struct device *dev)
{
	uint8_t high_byte, low_byte;
	int ret;

	ret = adv7535_read(dev, ADV7535_CEC, ADV7535_REG_CEC_ID_HB, &high_byte);
	if (ret) {
		return ret;
	}

	ret = adv7535_read(dev, ADV7535_CEC, ADV7535_REG_CEC_ID_LB, &low_byte);
	if (ret) {
		return ret;
	}

	return ((uint16_t)high_byte << 8U) | low_byte;
}

static int adv7535_init(const struct device *dev)
{
	const struct adv7535_config *config = dev->config;
	int ret;
	uint8_t rev;
	int id;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device %s not ready!", config->i2c.bus->name);
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

	id = adv7535_get_id(dev);
	if (id < 0) {
		return id;
	} else if (id != 0x7533 && id != 0x7535) {
		LOG_ERR("Unexpected chip ID: %x", id);
		return -ENODEV;
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

	ret = adv7535_read(dev, ADV7535_MAIN, ADV7535_REG_REVISION, &rev);
	if (!ret) {
		LOG_INF("ADV7535 initialized. ID: %x; Revision: 0x%02x", id, rev);
	}

	return 0;

error:
	adv7535_remove_int_callback(dev);
	return ret;
}

#define ADV7535_DSI(inst)    DT_INST_PHANDLE(inst, dsi_device)
#define ADV7535_TIMING(inst) DT_PHANDLE(ADV7535_DSI(inst), display_timing)
#define ADV7535_PANEL(inst)  DT_PARENT(ADV7535_TIMING(inst))

#define ADV7535_IS_PD_ACTIVE_LOW(inst) (DT_INST_GPIO_FLAGS(inst, pd_gpios) & GPIO_ACTIVE_LOW)

#define ADV7535_DT_GET_I2C_ADDR(inst) DT_INST_REG_ADDR(inst)

#define ADV7535_IS_PD_AND_I2C_ADDR_VALID(inst)                                                     \
	((ADV7535_DT_GET_I2C_ADDR(inst) == 0x39 && !(ADV7535_IS_PD_ACTIVE_LOW(inst))) ||           \
	 (ADV7535_DT_GET_I2C_ADDR(inst) == 0x3d && (ADV7535_IS_PD_ACTIVE_LOW(inst))))

#define ADV7535_VALIDATE_PD_AND_I2C_ADDR(inst)                                                     \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, pd_gpios),                                          \
	(BUILD_ASSERT((ADV7535_IS_PD_AND_I2C_ADDR_VALID(inst)),                                    \
		"ADV7535 I2C address does not match pd-gpios polarity. "                           \
		"0x39 requres active high, 0x3d requres active low."                               \
	      ))                                                                                   \
	);

#define ADV7535_VALIDATE_HORIZONTAL_TIMING(inst)                                                   \
	BUILD_ASSERT(DT_PROP(ADV7535_PANEL(inst), width) +                                         \
				     DT_PROP(ADV7535_TIMING(inst), hsync_len) +                    \
				     DT_PROP(ADV7535_TIMING(inst), hfront_porch) +                 \
				     DT_PROP(ADV7535_TIMING(inst), hback_porch) <=                 \
			     ADV7535_TIMING_MAX,                                                   \
		     "ADV7535 horizontal total exceeds 12 bits");

#define ADV7535_VALIDATE_VERTICAL_TIMING(inst)                                                     \
	BUILD_ASSERT(DT_PROP(ADV7535_PANEL(inst), height) +                                        \
				     DT_PROP(ADV7535_TIMING(inst), vsync_len) +                    \
				     DT_PROP(ADV7535_TIMING(inst), vfront_porch) +                 \
				     DT_PROP(ADV7535_TIMING(inst), vback_porch) <=                 \
			     ADV7535_TIMING_MAX,                                                   \
		     "ADV7535 vertical total exceeds 12 bits");

#define ADV7535_DEFINE(inst)                                                                       \
	static K_KERNEL_STACK_DEFINE(drv_stack_##inst, CONFIG_ADV7535_THREAD_STACK_SIZE);          \
	static struct k_thread drv_stack_data_##inst;                                              \
                                                                                                   \
	static void adv7535_make_thread_##inst(const struct device *dev)                           \
	{                                                                                          \
		k_thread_create(&drv_stack_data_##inst, drv_stack_##inst,                          \
				K_KERNEL_STACK_SIZEOF(drv_stack_##inst), adv7535_thread,           \
				(void *)dev, NULL, NULL, CONFIG_ADV7535_THREAD_PRIORITY, 0,        \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&drv_stack_data_##inst, "adv7535_" STRINGIFY(inst));             \
	}                                                                                          \
                                                                                                   \
	static const struct adv7535_config config_##inst = {                                       \
		.mipi_dsi_host = DEVICE_DT_GET(DT_PARENT(ADV7535_DSI(inst))),                      \
		.make_thread = adv7535_make_thread_##inst,                                         \
		.channel = DT_REG_ADDR(ADV7535_DSI(inst)),                                         \
		.data_lanes = DT_PROP_BY_IDX(ADV7535_DSI(inst), data_lanes, 0),                    \
		.test_pattern = DT_PROP_OR(ADV7535_DSI(inst), test_pattern, -1),                   \
		.i2c =                                                                             \
			{                                                                          \
				.bus = DEVICE_DT_GET(DT_INST_PARENT(inst)),                        \
				.main_addr = DT_INST_REG_ADDR(inst),                               \
				.edid_addr = DT_INST_PROP_OR(inst, adi_edid_addr,                  \
							     ADV7535_I2C_EDID_ADDR_DEFAULT),       \
				.packet_addr = DT_INST_PROP_OR(inst, adi_packet_addr,              \
							       ADV7535_I2C_PACKET_ADDR_DEFAULT),   \
				.cec_addr = DT_INST_PROP_OR(inst, adi_cec_addr,                    \
							    ADV7535_I2C_CEC_ADDR_DEFAULT),         \
				.fixed_addr = DT_INST_PROP_OR(inst, adi_fixed_addr,                \
							      ADV7535_I2C_FIXED_ADDR_DEFAULT),     \
			},                                                                         \
		.dt_pd = GPIO_DT_SPEC_INST_GET_OR(inst, pd_gpios, {0}),                            \
		.dt_int = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                  \
		.display_timings = {                                                               \
			.hactive = DT_PROP(ADV7535_PANEL(inst), width),                            \
			.hsync = DT_PROP(ADV7535_TIMING(inst), hsync_len),                         \
			.hfp = DT_PROP(ADV7535_TIMING(inst), hfront_porch),                        \
			.hbp = DT_PROP(ADV7535_TIMING(inst), hback_porch),                         \
			.vactive = DT_PROP(ADV7535_PANEL(inst), height),                           \
			.vsync = DT_PROP(ADV7535_TIMING(inst), vsync_len),                         \
			.vfp = DT_PROP(ADV7535_TIMING(inst), vfront_porch),                        \
			.vbp = DT_PROP(ADV7535_TIMING(inst), vback_porch),                         \
		}};                                                                                \
	static struct adv7535_data data_##inst = {                                                 \
		.irq_sem = Z_SEM_INITIALIZER(data_##inst.irq_sem, 0, 1)};                          \
	DEVICE_DT_INST_DEFINE(inst, adv7535_init, NULL, &data_##inst, &config_##inst, POST_KERNEL, \
			      CONFIG_ADV7535_INIT_PRIORITY, NULL);                                 \
	ADV7535_VALIDATE_PD_AND_I2C_ADDR(inst)                                                     \
	ADV7535_VALIDATE_HORIZONTAL_TIMING(inst)                                                   \
	ADV7535_VALIDATE_VERTICAL_TIMING(inst)

DT_INST_FOREACH_STATUS_OKAY(ADV7535_DEFINE)
