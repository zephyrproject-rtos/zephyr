/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Lontium LT8912B MIPI DSI to HDMI bridge. The bridge is
 * configured over I2C (three consecutive addresses: main, CEC/DSI and
 * AVI register pages) and converts a MIPI DSI video mode stream into
 * HDMI output. Register sequences follow the esp_lcd_lt8912b vendor
 * driver.
 */

#define DT_DRV_COMPAT lontium_lt8912b

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lt8912b, CONFIG_DISPLAY_LOG_LEVEL);

struct lt8912b_config {
	struct i2c_dt_spec i2c_main;
	struct i2c_dt_spec i2c_cec;
	struct i2c_dt_spec i2c_avi;
	const struct device *mipi_dsi;
	uint8_t channel;
	uint8_t num_of_lanes;
	struct gpio_dt_spec reset_gpio;
	/* Video timings, taken from the display controller node */
	uint16_t hact;
	uint16_t vact;
	uint16_t hsync_len;
	uint16_t hback_porch;
	uint16_t hfront_porch;
	uint16_t vsync_len;
	uint16_t vback_porch;
	uint16_t vfront_porch;
	uint8_t hsync_active;
	uint8_t vsync_active;
};

struct lt8912b_reg_val {
	uint8_t reg;
	uint8_t val;
};

static const struct lt8912b_reg_val lt8912b_digital_clock_en[] = {
	{0x02, 0xf7}, {0x08, 0xff}, {0x09, 0xff}, {0x0a, 0xff}, {0x0b, 0x7c}, {0x0c, 0xff},
	{0x42, 0x04}, /* per the Linux driver init sequence */
};

static const struct lt8912b_reg_val lt8912b_tx_analog[] = {
	{0x31, 0xe1}, {0x32, 0xe1}, {0x33, 0x0c}, {0x37, 0x00}, {0x38, 0x22}, {0x60, 0x82},
};

static const struct lt8912b_reg_val lt8912b_cbus_analog[] = {
	{0x39, 0x45}, {0x3a, 0x00}, {0x3b, 0x00},
};

static const struct lt8912b_reg_val lt8912b_hdmi_pll_analog[] = {
	{0x44, 0x31}, {0x55, 0x44}, {0x57, 0x01}, {0x5a, 0x02},
};

static const struct lt8912b_reg_val lt8912b_dds_config[] = {
	{0x4e, 0x93}, {0x4f, 0x3e}, {0x50, 0x29}, {0x51, 0x80}, {0x1e, 0x4f}, {0x1f, 0x5e},
	{0x20, 0x01}, {0x21, 0x2c}, {0x22, 0x01}, {0x23, 0xfa}, {0x24, 0x00}, {0x25, 0xc8},
	{0x26, 0x00}, {0x27, 0x5e}, {0x28, 0x01}, {0x29, 0x2c}, {0x2a, 0x01}, {0x2b, 0xfa},
	{0x2c, 0x00}, {0x2d, 0xc8}, {0x2e, 0x00}, {0x42, 0x64}, {0x43, 0x00}, {0x44, 0x04},
	{0x45, 0x00}, {0x46, 0x59}, {0x47, 0x00}, {0x48, 0xf2}, {0x49, 0x06}, {0x4a, 0x00},
	{0x4b, 0x72}, {0x4c, 0x45}, {0x4d, 0x00}, {0x52, 0x08}, {0x53, 0x00}, {0x54, 0xb2},
	{0x55, 0x00}, {0x56, 0xe4}, {0x57, 0x0d}, {0x58, 0x00}, {0x59, 0xe4}, {0x5a, 0x8a},
	{0x5b, 0x00}, {0x5c, 0x34}, {0x51, 0x00},
};

static const struct lt8912b_reg_val lt8912b_audio_iis_en[] = {
	{0x06, 0x08}, {0x07, 0xf0}, {0x34, 0xd2}, {0x0f, 0x2b},
};

static const struct lt8912b_reg_val lt8912b_lvds_bypass[] = {
	{0x44, 0x30}, {0x51, 0x05}, {0x50, 0x24}, {0x51, 0x2d}, {0x52, 0x04}, {0x69, 0x0e},
	{0x69, 0x8e}, {0x6a, 0x00}, {0x6c, 0xb8}, {0x6b, 0x51}, {0x04, 0xfb}, {0x04, 0xff},
	{0x7f, 0x00}, {0xa8, 0x13},
};

static int lt8912b_write_array(const struct i2c_dt_spec *i2c, const struct lt8912b_reg_val *seq,
			       size_t len)
{
	int ret;

	for (size_t i = 0; i < len; i++) {
		ret = i2c_reg_write_byte_dt(i2c, seq[i].reg, seq[i].val);
		if (ret < 0) {
			LOG_ERR("I2C write 0x%02x=0x%02x to 0x%02x failed (%d)", seq[i].reg,
				seq[i].val, i2c->addr, ret);
			return ret;
		}
	}

	return 0;
}

static int lt8912b_video_setup(const struct lt8912b_config *config)
{
	const struct i2c_dt_spec *cec = &config->i2c_cec;
	uint16_t htotal = config->hact + config->hsync_len + config->hback_porch +
			  config->hfront_porch;
	uint16_t vtotal = config->vact + config->vsync_len + config->vback_porch +
			  config->vfront_porch;
	const struct lt8912b_reg_val seq[] = {
		{0x18, config->hsync_len & 0xff},
		{0x19, config->vsync_len & 0xff},
		{0x1c, config->hact & 0xff},
		{0x1d, config->hact >> 8},
		{0x2f, 0x0c}, /* FIFO buffer length */
		{0x34, htotal & 0xff},
		{0x35, htotal >> 8},
		{0x36, vtotal & 0xff},
		{0x37, vtotal >> 8},
		{0x38, config->vback_porch & 0xff},
		{0x39, config->vback_porch >> 8},
		{0x3a, config->vfront_porch & 0xff},
		{0x3b, config->vfront_porch >> 8},
		{0x3c, config->hback_porch & 0xff},
		{0x3d, config->hback_porch >> 8},
		{0x3e, config->hfront_porch & 0xff},
		{0x3f, config->hfront_porch >> 8},
	};

	return lt8912b_write_array(cec, seq, ARRAY_SIZE(seq));
}

static int lt8912b_avi_infoframe(const struct lt8912b_config *config)
{
	/* 1 = 4:3, 2 = 16:9 */
	uint8_t aspect_ratio = (config->hact * 9 == config->vact * 16) ? 2 : 1;
	/* CEA VIC so that TVs recognize the mode (0 = non-CEA/DMT) */
	uint8_t vic = 0;

	if (config->hact == 1280 && config->vact == 720) {
		vic = 4; /* 720p60 */
	}
	uint8_t sync_polarity = (config->hsync_active ? 0x02 : 0x00) |
				(config->vsync_active ? 0x01 : 0x00);
	uint8_t pb2 = (aspect_ratio << 4) + 0x08;
	uint8_t pb4 = vic;
	uint8_t pb0 = ((pb2 + pb4) <= 0x5f) ? (0x5f - pb2 - pb4) : (0x15f - pb2 - pb4);
	int ret;

	/* Enable null packet */
	ret = i2c_reg_write_byte_dt(&config->i2c_avi, 0x3c, 0x41);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0xab, sync_polarity);
	if (ret < 0) {
		return ret;
	}

	const struct lt8912b_reg_val avi[] = {
		{0x43, pb0},  /* checksum */
		{0x44, 0x10}, /* PB1: RGB888 */
		{0x45, pb2},
		{0x46, 0x00},
		{0x47, pb4},
	};

	return lt8912b_write_array(&config->i2c_avi, avi, ARRAY_SIZE(avi));
}

static int lt8912b_mipi_rx_logic_reset(const struct lt8912b_config *config)
{
	int ret;

	/* MIPI RX reset */
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0x03, 0x7f);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0x03, 0xff);
	if (ret < 0) {
		return ret;
	}

	/* DDS reset */
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0x05, 0xfb);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);

	return i2c_reg_write_byte_dt(&config->i2c_main, 0x05, 0xff);
}

static int lt8912b_hdmi_output(const struct lt8912b_config *config, bool on);

/* Cycle all digital clock gates and block resets.
 *
 * Known limitation: the chip has no reset line on these boards and is
 * kept half-alive through the HDMI HPD line (5 V from the sink) when
 * the board is power cycled with the cable attached. That wedges an
 * internal clock domain in a way no register sequence reliably clears
 * (reset pulses, clock gate cycling and DSI clock removal were all
 * tried; the Linux driver requires a physical reset line for the same
 * reason). Recovery requires a power cycle with the HDMI cable
 * detached, or wiring the chip's reset pad to a GPIO.
 */
static int lt8912b_full_reset(const struct lt8912b_config *config)
{
	static const struct lt8912b_reg_val clocks_off[] = {
		{0x02, 0x00}, {0x08, 0x00}, {0x09, 0x00}, {0x0a, 0x00}, {0x0b, 0x00},
		{0x0c, 0x00}, {0x03, 0x00}, {0x05, 0x00},
	};
	static const struct lt8912b_reg_val clocks_on[] = {
		{0x02, 0xf7}, {0x08, 0xff}, {0x09, 0xff}, {0x0a, 0xff}, {0x0b, 0x7c},
		{0x0c, 0xff}, {0x03, 0xff}, {0x05, 0xff},
	};
	int ret;

	ret = lt8912b_write_array(&config->i2c_main, clocks_off, ARRAY_SIZE(clocks_off));
	if (ret < 0) {
		return ret;
	}
	k_msleep(20);
	ret = lt8912b_write_array(&config->i2c_main, clocks_on, ARRAY_SIZE(clocks_on));
	if (ret < 0) {
		return ret;
	}
	k_msleep(20);

	return 0;
}

static int lt8912b_configure(const struct lt8912b_config *config)
{
	uint8_t lane_reg;
	int ret;

	ret = lt8912b_write_array(&config->i2c_main, lt8912b_digital_clock_en,
				  ARRAY_SIZE(lt8912b_digital_clock_en));
	ret |= lt8912b_write_array(&config->i2c_main, lt8912b_tx_analog,
				   ARRAY_SIZE(lt8912b_tx_analog));
	ret |= lt8912b_write_array(&config->i2c_main, lt8912b_cbus_analog,
				   ARRAY_SIZE(lt8912b_cbus_analog));
	ret |= lt8912b_write_array(&config->i2c_main, lt8912b_hdmi_pll_analog,
				   ARRAY_SIZE(lt8912b_hdmi_pll_analog));
	if (ret < 0) {
		return -EIO;
	}

	/* MIPI analog: no P/N swap, EQ settings */
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0x3e, 0xd6);
	ret |= i2c_reg_write_byte_dt(&config->i2c_main, 0x3f, 0xd4);
	ret |= i2c_reg_write_byte_dt(&config->i2c_main, 0x41, 0x3c);
	if (ret < 0) {
		return -EIO;
	}

	/* MIPI basic: 0x00 = 4 lanes, 0x01 = 1 lane, 0x02 = 2 lanes, 0x03 = 3 lanes */
	lane_reg = config->num_of_lanes & 0x03;
	const struct lt8912b_reg_val mipi_basic[] = {
		{0x10, 0x01}, /* term enable */
		{0x11, 0x10}, /* settle */
		{0x12, 0x04}, /* trail (per the Linux driver) */
		{0x13, lane_reg},
		{0x14, 0x00}, /* debug mux */
		{0x15, 0x00}, /* no lane swap */
		{0x1a, 0x03}, /* hshift */
		{0x1b, 0x03}, /* vshift */
	};
	ret = lt8912b_write_array(&config->i2c_cec, mipi_basic, ARRAY_SIZE(mipi_basic));
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_write_array(&config->i2c_cec, lt8912b_dds_config,
				  ARRAY_SIZE(lt8912b_dds_config));
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_video_setup(config);
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_avi_infoframe(config);
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_mipi_rx_logic_reset(config);
	if (ret < 0) {
		return -EIO;
	}

	/* HDMI mode (with audio infrastructure), not DVI */
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0xb2, 0x01);
	ret |= lt8912b_write_array(&config->i2c_avi, lt8912b_audio_iis_en,
				   ARRAY_SIZE(lt8912b_audio_iis_en));
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_write_array(&config->i2c_main, lt8912b_lvds_bypass,
				  ARRAY_SIZE(lt8912b_lvds_bypass));
	if (ret < 0) {
		return -EIO;
	}

	/* LVDS output off, HDMI output on */
	ret = i2c_reg_write_byte_dt(&config->i2c_main, 0x44, 0x31);
	ret |= lt8912b_hdmi_output(config, true);
	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

/* The LT8912B DDS only locks onto the MIPI DSI video stream if its RX
 * logic is reset while the stream is present. Video streaming starts
 * lazily (on first display write), so poll the bridge's MIPI input
 * detection and re-synchronize whenever a video stream (re)appears.
 */
static struct k_work_delayable lt8912b_sync_work;
static const struct device *lt8912b_sync_dev;
static bool lt8912b_output_on;
static uint8_t lt8912b_stable_polls;

#define LT8912B_ENABLE_DEBOUNCE_POLLS 4

static int lt8912b_hdmi_output(const struct lt8912b_config *config, bool on)
{
	return i2c_reg_write_byte_dt(&config->i2c_main, 0x33, on ? 0x0e : 0x0c);
}

static void lt8912b_sync_work_fn(struct k_work *work)
{
	const struct lt8912b_config *config = lt8912b_sync_dev->config;
	uint8_t h_low = 0xff;
	uint8_t h_high = 0xff;
	uint8_t hpd = 0;
	bool video_present;
	bool sink_present;

	i2c_reg_read_byte_dt(&config->i2c_main, 0x9c, &h_low);
	i2c_reg_read_byte_dt(&config->i2c_main, 0x9d, &h_high);
	i2c_reg_read_byte_dt(&config->i2c_main, 0xc1, &hpd);
	video_present = !(h_low == 0xff && h_high == 0xff);
	sink_present = (hpd & 0x80) == 0x80;

	static bool last_sink_present;

	if (sink_present && !last_sink_present && lt8912b_output_on) {
		/* Sink (re)connected while streaming: re-lock the DDS */
		lt8912b_mipi_rx_logic_reset(config);
		LOG_INF("HDMI sink connected, MIPI RX re-synchronized");
	}
	last_sink_present = sink_present;

	if (video_present && !lt8912b_output_on) {
		if (++lt8912b_stable_polls >= LT8912B_ENABLE_DEBOUNCE_POLLS) {
			/* The DDS only locks when its reset runs while
			 * the video stream is present.
			 */
			lt8912b_mipi_rx_logic_reset(config);
			lt8912b_output_on = true;
			LOG_INF("video stream detected, MIPI RX re-synchronized");
		}
	} else if (!video_present) {
		lt8912b_stable_polls = 0;
		if (lt8912b_output_on) {
			lt8912b_output_on = false;
			LOG_INF("video stream lost");
		}
	}

	k_work_schedule(&lt8912b_sync_work, K_MSEC(200));
}

static int lt8912b_init(const struct device *dev)
{
	const struct lt8912b_config *config = dev->config;
	struct mipi_dsi_device mdev = {0};
	uint8_t id[2] = {0};
	uint8_t hpd = 0;
	int ret;

	if (!device_is_ready(config->mipi_dsi)) {
		LOG_ERR("MIPI DSI host not ready");
		return -ENODEV;
	}

	if (!i2c_is_ready_dt(&config->i2c_main)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_main, 0x00, &id[0]);
	ret |= i2c_reg_read_byte_dt(&config->i2c_main, 0x01, &id[1]);
	if (ret < 0) {
		LOG_ERR("Could not read chip ID, check I2C wiring");
		return -EIO;
	}
	LOG_INF("LT8912B chip ID: 0x%02x 0x%02x", id[0], id[1]);

	ret = lt8912b_full_reset(config);
	if (ret < 0) {
		return -EIO;
	}

	ret = lt8912b_configure(config);
	if (ret < 0) {
		return ret;
	}

	/* Attaching starts the video stream, so the bridge is configured
	 * beforehand and its DDS is re-synchronized on the live stream by
	 * the polling work afterwards.
	 */
	mdev.data_lanes = config->num_of_lanes;
	mdev.pixfmt = MIPI_DSI_PIXFMT_RGB888;
	/* The DDS derives its clock from the stream, so the link has to
	 * stay in high speed through the blanking periods.
	 */
	mdev.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_VIDEO_HFP | MIPI_DSI_MODE_VIDEO_HBP |
			  MIPI_DSI_MODE_VIDEO_HSA;
	mdev.timings.hactive = config->hact;
	mdev.timings.hsync = config->hsync_len;
	mdev.timings.hbp = config->hback_porch;
	mdev.timings.hfp = config->hfront_porch;
	mdev.timings.vactive = config->vact;
	mdev.timings.vsync = config->vsync_len;
	mdev.timings.vbp = config->vback_porch;
	mdev.timings.vfp = config->vfront_porch;

	ret = mipi_dsi_attach(config->mipi_dsi, config->channel, &mdev);
	if (ret < 0) {
		LOG_ERR("Could not attach to MIPI DSI host (%d)", ret);
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c_main, 0xc1, &hpd);
	if (ret == 0 && (hpd & 0x80) == 0x80) {
		LOG_INF("HDMI monitor connected");
	} else {
		LOG_WRN("No HDMI monitor detected (hot-plug supported after boot)");
	}

	lt8912b_sync_dev = dev;
	k_work_init_delayable(&lt8912b_sync_work, lt8912b_sync_work_fn);
	k_work_schedule(&lt8912b_sync_work, K_MSEC(200));

	return 0;
}

#define LT8912B_TIMINGS_NODE(id) DT_INST_CHILD(id, display_timings)

#define LT8912B_DEFINE(id)                                                                         \
	static const struct lt8912b_config lt8912b_config_##id = {                                 \
		.i2c_main = I2C_DT_SPEC_INST_GET(id),                                              \
		.i2c_cec = {.bus = DEVICE_DT_GET(DT_INST_BUS(id)),                                 \
			    .addr = DT_INST_REG_ADDR(id) + 1},                                     \
		.i2c_avi = {.bus = DEVICE_DT_GET(DT_INST_BUS(id)),                                 \
			    .addr = DT_INST_REG_ADDR(id) + 2},                                     \
		.mipi_dsi = DEVICE_DT_GET(DT_INST_PHANDLE(id, mipi_dsi)),                          \
		.channel = 0,                                                                      \
		.num_of_lanes = DT_INST_PROP_BY_IDX(id, data_lanes, 0),                            \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(id, reset_gpios, {0}),                      \
		.hact = DT_INST_PROP(id, width),                                                   \
		.vact = DT_INST_PROP(id, height),                                                  \
		.hsync_len = DT_PROP(LT8912B_TIMINGS_NODE(id), hsync_len),                         \
		.hback_porch = DT_PROP(LT8912B_TIMINGS_NODE(id), hback_porch),                     \
		.hfront_porch = DT_PROP(LT8912B_TIMINGS_NODE(id), hfront_porch),                   \
		.vsync_len = DT_PROP(LT8912B_TIMINGS_NODE(id), vsync_len),                         \
		.vback_porch = DT_PROP(LT8912B_TIMINGS_NODE(id), vback_porch),                     \
		.vfront_porch = DT_PROP(LT8912B_TIMINGS_NODE(id), vfront_porch),                   \
		.hsync_active = DT_PROP(LT8912B_TIMINGS_NODE(id), hsync_active),                   \
		.vsync_active = DT_PROP(LT8912B_TIMINGS_NODE(id), vsync_active),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &lt8912b_init, NULL, NULL, &lt8912b_config_##id, POST_KERNEL,    \
			      CONFIG_APPLICATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(LT8912B_DEFINE)
