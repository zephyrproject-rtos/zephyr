/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sil_sii9022a

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(sii9022a, CONFIG_DISPLAY_LOG_LEVEL);

/* TPI timing registers */
#define SII9022A_REG_PIXCLK_LSB         0x00  /* Pixel clock, 10 kHz units, LSB */
#define SII9022A_REG_PIXCLK_MSB         0x01  /* Pixel clock MSB                */
#define SII9022A_REG_VFREQ_LSB          0x02  /* Vert. freq., integer Hz, LSB   */
#define SII9022A_REG_VFREQ_MSB          0x03  /* Vert. freq. MSB                */
#define SII9022A_REG_PIXELS_LSB         0x04  /* Total H pixels (htotal) LSB    */
#define SII9022A_REG_PIXELS_MSB         0x05  /* Total H pixels MSB             */
#define SII9022A_REG_LINES_LSB          0x06  /* Total V lines  (vtotal) LSB    */
#define SII9022A_REG_LINES_MSB          0x07  /* Total V lines  MSB             */
#define SII9022A_REG_INPUT_BUS          0x08  /* Input bus + pixel repetition   */
#define SII9022A_REG_EMB_SYNC_H         0x09  /* Embedded sync H blank          */
#define SII9022A_REG_OUTPUT_FMT         0x0A  /* Output format / sync type      */
#define SII9022A_REG_YC_MUX             0x0B  /* YC mux mode                    */

/* TMDS / power registers */
#define SII9022A_REG_TMDS_CTRL          0x1A  /* TMDS output control            */
#define SII9022A_REG_POWER_STATE        0x1E  /* TX power state (D0–D3)         */

/* Chip identification registers */
#define SII9022A_REG_CHIP_ID            0x1B  /* Device ID                      */
#define SII9022A_REG_CHIP_PROD_REV      0x1C  /* Device production revision     */
#define SII9022A_REG_TPI_REV            0x1D  /* TPI revision                   */
#define SII9022A_REG_HDCP_REV           0x30  /* HDCP revision                  */

/* Sync configuration and polarity */
#define SII9022A_REG_SYNC_CFG           0x60  /* Sync generator configuration   */
#define SII9022A_REG_DE_DELAY_LSB       0x62  /* DE delay / H-bit-to-H-sync LSB */
#define SII9022A_REG_SYNCPOL            0x63  /* Sync polarity + DE delay MSB   */
#define SII9022A_REG_DE_TOP             0x64  /* DE top (V blanking top)        */
#define SII9022A_REG_DE_CNT_LSB         0x66  /* DE count LSB (H active pixels) */
#define SII9022A_REG_DE_CNT_MSB         0x67  /* DE count MSB                   */
#define SII9022A_REG_DE_LINE_LSB        0x68  /* DE line LSB (V active lines)   */
#define SII9022A_REG_DE_LINE_MSB        0x69  /* DE line MSB                    */

/* TPI AVI InfoFrame registers (written in HDMI output mode) */
#define SII9022A_REG_AVI_TYPE           0x0C  /* InfoFrame type = 0x82       */
#define SII9022A_REG_AVI_VER            0x0D  /* InfoFrame version = 0x02    */
#define SII9022A_REG_AVI_LEN            0x0E  /* InfoFrame length = 0x0D     */
#define SII9022A_REG_AVI_CKSUM          0x0F  /* InfoFrame checksum          */
#define SII9022A_REG_AVI_DB1            0x10  /* DB1: colorspace (Y1Y0)      */
#define SII9022A_REG_AVI_DB2            0x11  /* DB2: colorimetry / AR       */
#define SII9022A_REG_AVI_DB3            0x12  /* DB3: quantisation range     */
#define SII9022A_REG_AVI_DB4            0x13  /* DB4: VIC                    */
#define SII9022A_REG_AVI_DB5            0x14  /* DB5: pixel repetition       */
/* DB6-DB13 (bar data, typically zero) live at 0x15-0x1C                      */
#define SII9022A_REG_AVI_DB_LAST        0x1C  /* last data byte register     */

/* TPI enable / control registers */
#define SII9022A_REG_TPI_CTRL1          0xBC
#define SII9022A_REG_TPI_CTRL2          0xBD
#define SII9022A_REG_TPI_ENABLE         0xBE

/* Hardware option / reset trigger */
#define SII9022A_REG_HW_OPT             0xC7

/* Input bus config (reg 0x08) */
#define SII9022A_BUS_CLK_EDGE_BIT       BIT(4) /* 1=rising edge, 0=falling edge  */

/* TMDS control (reg 0x1A) — System Control Data */
#define SII9022A_TMDS_OUTPUT_HDMI_BIT  BIT(0) /* 1 = HDMI output, 0 = DVI output */
#define SII9022A_TMDS_AV_MUTE_BIT      BIT(3) /* 1 = AV mute (blank screen)      */
#define SII9022A_TMDS_PD_CLK_BIT       BIT(4) /* 1 = power down TMDS clock       */

/* Power state (reg 0x1E) bits[1:0]: 00 = D0 (normal), 01/10/11 = power down */
#define SII9022A_POWER_D0_MASK          0x03

/* Sync config (reg 0x60): 0x04 = external sync with separate DE/HSYNC/VSYNC */
#define SII9022A_SYNC_CFG_EXTERNAL      0x04

/* Sync polarity (reg 0x63): bit6 = DE active-high enable */
#define SII9022A_SYNCPOL_DE_EN          BIT(6)

/* Output format values for reg 0x0A — external sync mode */
#define SII9022A_OFMT_EXT_HDMI_RGB      0x00
#define SII9022A_OFMT_EXT_HDMI_YUV444   0x11
#define SII9022A_OFMT_EXT_HDMI_YUV422   0x12
#define SII9022A_OFMT_EXT_DVI_RGB       0x13

/* SII9022A output format selector (matches DT output-format values). */
enum sii9022a_output_fmt {
	/** HDMI output, RGB colour space */
	SII9022A_OUTPUT_FMT_HDMI_RGB    = 0,
	/** HDMI output, YUV 4:4:4 colour space */
	SII9022A_OUTPUT_FMT_HDMI_YUV444 = 1,
	/** HDMI output, YUV 4:2:2 colour space */
	SII9022A_OUTPUT_FMT_HDMI_YUV422 = 2,
	/** DVI output, RGB colour space */
	SII9022A_OUTPUT_FMT_DVI_RGB     = 3,
};

/* Driver config */
struct sii9022a_config {
	struct i2c_dt_spec bus;
	uint8_t output_format;  /* enum sii9022a_output_fmt value from DT */
	bool    clk_falling;    /* true = latch pixels on falling clock edge */

	/* Video timing — read at compile time from the display controller
	 * DT node via the display-controller phandle on this node. */
	uint32_t pixel_clk_hz;
	uint16_t hactive;
	uint16_t vactive;
	uint16_t hfp;
	uint16_t hbp;
	uint16_t hsync;
	uint16_t vfp;
	uint16_t vbp;
	uint16_t vsync;
};

/* Driver runtime data */
struct sii9022a_data {
	/* Shadow copies of writeable registers — updated as we go */
	uint8_t sync_cfg_reg;      /* current value of reg 0x60   */
	uint8_t sync_polarity_reg; /* current value of reg 0x63   */
	uint8_t in_bus_cfg;        /* current value of reg 0x08   */
	bool    is_rgb_output;     /* true when output is RGB      */

	/* Chip identification (read at init) */
	uint8_t device_id;
	uint8_t device_prod_rev_id;
	uint8_t tpi_rev_id;
	uint8_t hdcp_rev_tpi;
};

/* Low-level I2C helpers */
static int sii9022a_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct sii9022a_config *cfg = dev->config;
	uint8_t buf[2] = {reg, val};

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

static int sii9022a_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct sii9022a_config *cfg = dev->config;

	return i2c_write_read_dt(&cfg->bus, &reg, 1, val, 1);
}

/* Read-modify-write: (reg & ~mask) | (set_bits & mask) */
static int sii9022a_reg_rmw(const struct device *dev, uint8_t reg,
			    uint8_t mask, uint8_t set_bits)
{
	uint8_t val;
	int ret;

	ret = sii9022a_reg_read(dev, reg, &val);
	if (ret) {
		return ret;
	}
	val = (val & ~mask) | (set_bits & mask);
	return sii9022a_reg_write(dev, reg, val);
}

/* BridgeSii9022a_reset: trigger a device soft-reset */
static int sii9022a_reset(const struct device *dev)
{
	return sii9022a_reg_write(dev, SII9022A_REG_HW_OPT, 0x00);
}

/* BridgeSii9022a_getHdmiChipId: read and log chip identification registers */
static int sii9022a_get_chip_id(const struct device *dev)
{
	struct sii9022a_data *data = dev->data;
	int ret = 0;

	ret |= sii9022a_reg_read(dev, SII9022A_REG_CHIP_ID,       &data->device_id);
	ret |= sii9022a_reg_read(dev, SII9022A_REG_CHIP_PROD_REV, &data->device_prod_rev_id);
	ret |= sii9022a_reg_read(dev, SII9022A_REG_TPI_REV,       &data->tpi_rev_id);
	ret |= sii9022a_reg_read(dev, SII9022A_REG_HDCP_REV,      &data->hdcp_rev_tpi);

	if (ret == 0) {
		LOG_INF("SII9022A: device_id=0x%02x prod_rev=0x%02x "
			"tpi_rev=0x%02x hdcp_rev=0x%02x",
			data->device_id, data->device_prod_rev_id,
			data->tpi_rev_id, data->hdcp_rev_tpi);
	}
	return ret;
}

/* BridgeSii9022a_powerUpTransmitter: bring TX out of power-down (D0 state) */
static int sii9022a_power_up_tx(const struct device *dev)
{
	/* Clear bits[1:0] of reg 0x1E → power state D0 (normal operation) */
	return sii9022a_reg_rmw(dev, SII9022A_REG_POWER_STATE,
				SII9022A_POWER_D0_MASK, 0x00);
}

/* BridgeSii9022a_enableDevice: enable TPI register access */
static int sii9022a_enable_device(const struct device *dev)
{
	int ret;

	ret  = sii9022a_reg_write(dev, SII9022A_REG_TPI_CTRL1, 0x01);
	ret |= sii9022a_reg_write(dev, SII9022A_REG_TPI_CTRL2, 0x82);
	if (ret) {
		return ret;
	}
	/* Set bit0 of reg 0xBE to enable TPI register access */
	return sii9022a_reg_rmw(dev, SII9022A_REG_TPI_ENABLE, BIT(0), BIT(0));
}

/* BridgeSii9022a_configureInputBus: set input bus width and clock edge */
static int sii9022a_configure_input_bus(const struct device *dev)
{
	const struct sii9022a_data *data = dev->data;

	return sii9022a_reg_write(dev, SII9022A_REG_INPUT_BUS, data->in_bus_cfg);
}

/* BridgeSii9022a_configureYcMuxMode: disable YC multiplexed input mode */
static int sii9022a_configure_yc_mux(const struct device *dev)
{
	return sii9022a_reg_write(dev, SII9022A_REG_YC_MUX, 0x00);
}

/* BridgeSii9022a_configureSyncMode: write sync configuration register */
static int sii9022a_configure_sync_mode(const struct device *dev)
{
	const struct sii9022a_data *data = dev->data;

	return sii9022a_reg_write(dev, SII9022A_REG_SYNC_CFG, data->sync_cfg_reg);
}

/* BridgeSii9022a_prgmExtSyncTimingInfo */
static int sii9022a_prgm_ext_sync_timing(const struct device *dev,
					  uint16_t de_delay,
					  uint16_t de_top,
					  uint16_t de_cnt,
					  uint16_t de_line,
					  uint16_t pix_clk,
					  uint16_t v_freq,
					  uint16_t pixels,
					  uint16_t lines,
					  uint8_t  out_fmt_reg)
{
	struct sii9022a_data *data = dev->data;
	int ret = 0;

	/* Encode the high 2 bits of DE delay into sync polarity register [1:0] */
	data->sync_polarity_reg &= (uint8_t)~0x03;
	data->sync_polarity_reg |= (uint8_t)((de_delay & 0x300) >> 8);

	/* Ext-sync DE geometry registers (0x62–0x69) */
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_DELAY_LSB,
				  (uint8_t)(de_delay & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_SYNCPOL,
				  data->sync_polarity_reg);
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_TOP,
				  (uint8_t)(de_top & 0xFF));
	/* Note: register 0x65 (field2Offset) is unused in ext-sync mode */
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_CNT_LSB,
				  (uint8_t)(de_cnt & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_CNT_MSB,
				  (uint8_t)((de_cnt & 0xF00) >> 8));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_LINE_LSB,
				  (uint8_t)(de_line & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_DE_LINE_MSB,
				  (uint8_t)((de_line & 0x700) >> 8));

	/* Timing / clock registers (0x00–0x07, 0x09, 0x0A) */
	ret |= sii9022a_reg_write(dev, SII9022A_REG_PIXCLK_LSB,
				  (uint8_t)(pix_clk & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_PIXCLK_MSB,
				  (uint8_t)((pix_clk & 0xFF00) >> 8));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_VFREQ_LSB,
				  (uint8_t)(v_freq & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_VFREQ_MSB,
				  (uint8_t)((v_freq & 0xFF00) >> 8));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_PIXELS_LSB,
				  (uint8_t)(pixels & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_PIXELS_MSB,
				  (uint8_t)((pixels & 0xFF00) >> 8));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_LINES_LSB,
				  (uint8_t)(lines & 0xFF));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_LINES_MSB,
				  (uint8_t)((lines & 0xFF00) >> 8));
	ret |= sii9022a_reg_write(dev, SII9022A_REG_EMB_SYNC_H, 0x00);
	ret |= sii9022a_reg_write(dev, SII9022A_REG_OUTPUT_FMT, out_fmt_reg);

	return ret;
}

/* Internal mode-programming functions (called from bridge vtable below) */
static int sii9022a_set_mode_ext_sync(const struct device *dev,
			       uint16_t hactive, uint16_t vactive,
			       uint16_t hfp,    uint16_t hbp,  uint16_t hsync,
			       uint16_t vfp,    uint16_t vbp,  uint16_t vsync,
			       uint32_t pixel_clk_hz)
{
	const struct sii9022a_config *cfg = dev->config;
	struct sii9022a_data *data = dev->data;
	int ret;

	uint16_t htotal   = hactive + hfp + hbp + hsync;
	uint16_t vtotal   = vactive + vfp + vbp + vsync;

	/*
	 * DE delay = left border width = hbp + hsync
	 * (number of pixel clocks from start of line to first active pixel)
	 */
	uint16_t de_delay = hbp + hsync;

	/*
	 * DE top = top border height = vbp + vsync
	 * (number of lines from start of frame to first active line)
	 */
	uint16_t de_top   = vbp + vsync;

	/* Pixel clock in units of 10 kHz */
	uint16_t pix_clk  = (uint16_t)(pixel_clk_hz / 10000);

	/* Vertical frequency in integer Hz. */
	uint16_t v_freq   = (uint16_t)(pixel_clk_hz /
				       ((uint32_t)htotal * (uint32_t)vtotal));

	uint8_t out_fmt;

	/* Set external sync mode */
	data->sync_cfg_reg = SII9022A_SYNC_CFG_EXTERNAL;

	/* Resolve output format register value and RGB flag */
	switch ((enum sii9022a_output_fmt)cfg->output_format) {
	case SII9022A_OUTPUT_FMT_HDMI_RGB:
		out_fmt = SII9022A_OFMT_EXT_HDMI_RGB;
		data->is_rgb_output = true;
		break;
	case SII9022A_OUTPUT_FMT_HDMI_YUV444:
		out_fmt = SII9022A_OFMT_EXT_HDMI_YUV444;
		data->is_rgb_output = false;
		break;
	case SII9022A_OUTPUT_FMT_HDMI_YUV422:
		out_fmt = SII9022A_OFMT_EXT_HDMI_YUV422;
		data->is_rgb_output = false;
		break;
	case SII9022A_OUTPUT_FMT_DVI_RGB:
		out_fmt = SII9022A_OFMT_EXT_DVI_RGB;
		data->is_rgb_output = true;
		break;
	default:
		LOG_ERR("Unsupported output format %u", cfg->output_format);
		return -EINVAL;
	}

	/* Configure sync mode register */
	ret = sii9022a_configure_sync_mode(dev);
	if (ret) {
		LOG_ERR("Configure sync mode failed: %d", ret);
		return ret;
	}

	/* Program external-sync timing registers */
	ret = sii9022a_prgm_ext_sync_timing(dev,
					    de_delay, de_top,
					    hactive,  vactive,
					    pix_clk,  v_freq,
					    htotal,   vtotal,
					    out_fmt);
	if (ret) {
		LOG_ERR("Program ext-sync timing failed: %d", ret);
		return ret;
	}

	LOG_INF("SII9022A mode set: %ux%u @ %u Hz (pix_clk=%u kHz)",
		hactive, vactive, v_freq, pix_clk * 10);

	return 0;
}

static int sii9022a_start_output(const struct device *dev)
{
	const struct sii9022a_config *cfg = dev->config;

	uint8_t hdmi_bit = (cfg->output_format != SII9022A_OUTPUT_FMT_DVI_RGB)
			   ? SII9022A_TMDS_OUTPUT_HDMI_BIT : 0;

	return sii9022a_reg_rmw(dev, SII9022A_REG_TMDS_CTRL,
				SII9022A_TMDS_PD_CLK_BIT  |
				SII9022A_TMDS_AV_MUTE_BIT |
				SII9022A_TMDS_OUTPUT_HDMI_BIT,
				hdmi_bit);
}

static int sii9022a_init(const struct device *dev)
{
	const struct sii9022a_config *cfg = dev->config;
	struct sii9022a_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Initialise register shadow values */
	data->sync_cfg_reg      = SII9022A_SYNC_CFG_EXTERNAL;
	data->sync_polarity_reg = SII9022A_SYNCPOL_DE_EN; /* DE active-high */
	data->in_bus_cfg = 0x70;
	if (cfg->clk_falling) {
		data->in_bus_cfg &= (uint8_t)~SII9022A_BUS_CLK_EDGE_BIT;
	}

	/* Reset */
	ret = sii9022a_reset(dev);
	if (ret) {
		LOG_ERR("Reset failed: %d", ret);
		return ret;
	}

	/* Read and log chip ID */
	ret = sii9022a_get_chip_id(dev);
	if (ret) {
		LOG_ERR("Chip ID read failed: %d", ret);
		return ret;
	}

	/* Power up TMDS transmitter */
	ret = sii9022a_power_up_tx(dev);
	if (ret) {
		LOG_ERR("Power-up TX failed: %d", ret);
		return ret;
	}

	/* Enable TPI register access */
	ret = sii9022a_enable_device(dev);
	if (ret) {
		LOG_ERR("Enable device failed: %d", ret);
		return ret;
	}

	/* Configure input data bus */
	ret = sii9022a_configure_input_bus(dev);
	if (ret) {
		LOG_ERR("Configure input bus failed: %d", ret);
		return ret;
	}

	/* Disable YC multiplexed input mode */
	ret = sii9022a_configure_yc_mux(dev);
	if (ret) {
		LOG_ERR("Configure YC mux failed: %d", ret);
		return ret;
	}

	/* Set sync mode to external (separate DE/HSYNC/VSYNC) */
	ret = sii9022a_configure_sync_mode(dev);
	if (ret) {
		LOG_ERR("Configure sync mode failed: %d", ret);
		return ret;
	}

	/* Program external-sync timing registers */
	ret = sii9022a_set_mode_ext_sync(dev,
					 cfg->hactive, cfg->vactive,
					 cfg->hfp, cfg->hbp, cfg->hsync,
					 cfg->vfp, cfg->vbp, cfg->vsync,
					 cfg->pixel_clk_hz);
	if (ret) {
		LOG_ERR("Set mode failed: %d", ret);
		return ret;
	}

	/* Enable TMDS output */
	ret = sii9022a_start_output(dev);
	if (ret) {
		LOG_ERR("Start output failed: %d", ret);
		return ret;
	}

	LOG_INF("SII9022A ready (%ux%u @ %u Hz, bus %s addr 0x%02x, %s edge, fmt %u)",
		cfg->hactive, cfg->vactive, cfg->pixel_clk_hz,
		cfg->bus.bus->name, cfg->bus.addr,
		cfg->clk_falling ? "falling" : "rising",
		cfg->output_format);
	return 0;
}

/* Device tree instantiation */
#define SII9022A_DSS_PROP(n, prop) 						\
	DT_PROP(DT_INST_PHANDLE(n, display_controller), prop)

#define SII9022A_DSS_TIMINGS_PROP(n, prop)                                      \
	DT_PROP(DT_CHILD(DT_INST_PHANDLE(n, display_controller),           \
		display_timings), prop)

#define SII9022A_DEVICE_DEFINE(n)						\
	static struct sii9022a_data sii9022a_data_##n;				\
										\
	static const struct sii9022a_config sii9022a_config_##n = {		\
		.bus           = I2C_DT_SPEC_INST_GET(n),			\
		.output_format = DT_INST_PROP(n, output_format),		\
		.clk_falling   = DT_INST_PROP(n, clk_falling),			\
		/* Video timing — sourced from the display controller node */	\
		.pixel_clk_hz  = SII9022A_DSS_TIMINGS_PROP(n, clock_frequency), \
		.hactive       = SII9022A_DSS_PROP(n, width),			\
		.vactive       = SII9022A_DSS_PROP(n, height),			\
		.hfp  	       = SII9022A_DSS_TIMINGS_PROP(n, hfront_porch),    \
		.hbp  	       = SII9022A_DSS_TIMINGS_PROP(n, hback_porch),     \
		.hsync         = SII9022A_DSS_TIMINGS_PROP(n, hsync_len),       \
		.vfp  	       = SII9022A_DSS_TIMINGS_PROP(n, vfront_porch),    \
		.vbp  	       = SII9022A_DSS_TIMINGS_PROP(n, vback_porch),     \
		.vsync         = SII9022A_DSS_TIMINGS_PROP(n, vsync_len),       \
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      sii9022a_init,					\
			      NULL,						\
			      &sii9022a_data_##n,				\
			      &sii9022a_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_SII9022A_INIT_PRIORITY,			\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(SII9022A_DEVICE_DEFINE)
