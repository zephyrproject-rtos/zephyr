/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Microchip SAMA7G5 MIPI CSI-2 host controller.
 *
 * The controller embeds a MIPI D-PHY receiver with up to two data lanes. It
 * receives the CSI-2 packet stream from an image sensor and forwards it on its
 * internal IDI output to the CSI-2 Demux Controller (CSI2DC):
 *
 *   sensor --CSI-2--> csi2host --IDI--> csi2dc --parallel--> isc --> memory
 *
 * The D-PHY has no register bank of its own: it is configured through the
 * analog configuration interface exposed by the CSI_PHY_TEST_CTRL0/1 registers.
 * Only the high-speed bit rate code has to be programmed, which is derived from
 * the link frequency advertised by the sensor.
 */

#define DT_DRV_COMPAT microchip_sama7g5_csi2host

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>
#include <soc.h>

#include "video_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_mchp_sam_csi2host, CONFIG_VIDEO_LOG_LEVEL);

/* D-PHY analog configuration interface */
#define CSI2HOST_DPHY_HSFREQRANGE_ADDR 0x44U

/* The configuration clock of the D-PHY must stay within this range */
#define CSI2HOST_CFGCLK_MIN_HZ MHZ(17)
#define CSI2HOST_CFGCLK_MAX_HZ MHZ(27)

/* Devicetree did not provide a high-speed bit rate code for the D-PHY */
#define CSI2HOST_BITRATE_CODE_AUTO 0xffff

/* Stop state expected on the clock lane and on every enabled data lane */
#define CSI2HOST_STOPSTATE(lanes)                                                                  \
	(CSI_PHY_STOPSTATE_PHY_STOPSTATECLK_Msk | GENMASK((lanes) - 1, 0))

#define CSI2HOST_STOPSTATE_TIMEOUT_US 10000

/*
 * High-speed bit rate code programmed in the D-PHY HSFREQRANGE field, indexed by
 * the maximum lane data rate it covers. The code sits in bits [6:1] of the
 * analog configuration register at address 0x44.
 */
struct csi2host_hsfreq {
	uint16_t max_mbps;
	uint8_t code;
};

static const struct csi2host_hsfreq csi2host_hsfreq_table[] = {
	{80, 0x00},   {90, 0x10},   {100, 0x20},  {110, 0x30},  {120, 0x01},  {130, 0x11},
	{140, 0x21},  {150, 0x31},  {160, 0x02},  {170, 0x12},  {180, 0x22},  {190, 0x32},
	{205, 0x03},  {220, 0x13},  {235, 0x23},  {250, 0x33},  {275, 0x04},  {300, 0x14},
	{325, 0x05},  {350, 0x15},  {400, 0x25},  {450, 0x06},  {500, 0x16},  {550, 0x07},
	{600, 0x17},  {650, 0x08},  {700, 0x18},  {750, 0x09},  {800, 0x19},  {850, 0x29},
	{900, 0x39},  {950, 0x0a},  {1000, 0x1a}, {1050, 0x2a}, {1100, 0x3a}, {1150, 0x0b},
	{1200, 0x1b}, {1250, 0x2b}, {1300, 0x3b}, {1350, 0x0c}, {1400, 0x1c}, {1450, 0x2c},
	{1500, 0x3c},
};

struct csi2host_config {
	csi_registers_t *regs;
	const struct device *source_dev;
	const struct sam_clk_cfg *clock_cfg;
	unsigned int irq;
	uint16_t bitrate_code;
	uint32_t dphy_freq_kbps;
	uint8_t num_clocks;
	uint32_t phyclk_rate;
	uint8_t num_lanes;
};

struct csi2host_data {
	uint32_t phy_fatal_errors;
	uint32_t pkt_fatal_errors;
	uint32_t frame_fatal_errors;
	uint32_t phy_errors;
	uint32_t pkt_errors;
};

/*
 * Analog configuration interface of the D-PHY. The address is presented while
 * TESTEN is set and sampled on the falling edge of TESTCLK; the data byte is
 * presented with TESTEN cleared and sampled on the following rising edge.
 */
static void csi2host_dphy_write(csi_registers_t *regs, uint8_t addr, uint8_t data)
{
	regs->CSI_PHY_TEST_CTRL0 = CSI_PHY_TEST_CTRL0_PHY_TESTCLK_Msk;
	regs->CSI_PHY_TEST_CTRL1 = CSI_PHY_TEST_CTRL1_PHY_TESTDIN(addr) |
				   CSI_PHY_TEST_CTRL1_PHY_TESTEN_Msk;
	regs->CSI_PHY_TEST_CTRL0 = 0;

	regs->CSI_PHY_TEST_CTRL1 = CSI_PHY_TEST_CTRL1_PHY_TESTDIN(data);
	regs->CSI_PHY_TEST_CTRL0 = CSI_PHY_TEST_CTRL0_PHY_TESTCLK_Msk;
	regs->CSI_PHY_TEST_CTRL0 = 0;
}

/* Read back one analog configuration register. Only valid while the D-PHY is held down. */
static uint8_t csi2host_dphy_read(csi_registers_t *regs, uint8_t addr)
{
	regs->CSI_PHY_TEST_CTRL1 = CSI_PHY_TEST_CTRL1_PHY_TESTEN_Msk;
	regs->CSI_PHY_TEST_CTRL0 = CSI_PHY_TEST_CTRL0_PHY_TESTCLK_Msk;
	regs->CSI_PHY_TEST_CTRL1 = CSI_PHY_TEST_CTRL1_PHY_TESTDIN(addr) |
				   CSI_PHY_TEST_CTRL1_PHY_TESTEN_Msk;
	regs->CSI_PHY_TEST_CTRL0 = 0;
	regs->CSI_PHY_TEST_CTRL1 = 0;

	return (uint8_t)((regs->CSI_PHY_TEST_CTRL1 & CSI_PHY_TEST_CTRL1_PHY_TESTDOUT_Msk) >>
			 CSI_PHY_TEST_CTRL1_PHY_TESTDOUT_Pos);
}

static void csi2host_dphy_power_down(csi_registers_t *regs)
{
	regs->CSI_DPHY_RSTZ = 0;
	regs->CSI_PHY_TEST_CTRL0 = 0;
	regs->CSI_PHY_SHUTDOWNZ = 0;
}

static uint8_t csi2host_hsfreq_code(uint32_t mbps)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(csi2host_hsfreq_table) - 1; i++) {
		if (mbps <= csi2host_hsfreq_table[i].max_mbps) {
			break;
		}
	}

	return csi2host_hsfreq_table[i].code;
}

/*
 * The receiver is tuned for the rate the source reports, which is what the ranges
 * describe. Devicetree can pin a different rate for a source whose reported rate
 * sits awkwardly on a range boundary.
 */
static int csi2host_dphy_power_up(const struct device *dev, uint32_t mbps)
{
	const struct csi2host_config *cfg = dev->config;
	csi_registers_t *regs = cfg->regs;
	uint32_t tuned_mbps = cfg->dphy_freq_kbps ? cfg->dphy_freq_kbps / 1000 : mbps;
	uint8_t code;
	uint8_t readback;

	if (cfg->bitrate_code == CSI2HOST_BITRATE_CODE_AUTO) {
		code = csi2host_hsfreq_code(tuned_mbps) << 1;
	} else {
		code = (uint8_t)cfg->bitrate_code;
	}

	/* Hold the D-PHY in shutdown and reset while its analog part is set up */
	csi2host_dphy_power_down(regs);

	/* Clear the analog configuration before programming the bit rate range */
	regs->CSI_PHY_TEST_CTRL0 = CSI_PHY_TEST_CTRL0_PHY_TESTCLR_Msk;
	regs->CSI_PHY_TEST_CTRL0 = 0;

	csi2host_dphy_write(regs, CSI2HOST_DPHY_HSFREQRANGE_ADDR, code);
	readback = csi2host_dphy_read(regs, CSI2HOST_DPHY_HSFREQRANGE_ADDR);

	LOG_DBG("source sends %u Mbps per lane, D-PHY tuned for %u Mbps, HS bit rate code "
		"0x%02x, read back 0x%02x",
		mbps, tuned_mbps, code, readback);

	/*
	 * Reading back leaves the test interface still addressing the same register but
	 * presenting a data byte of zero, and the rising TESTCLK edge below is the edge
	 * that latches data. Present the code once more so that edge is harmless.
	 */
	csi2host_dphy_write(regs, CSI2HOST_DPHY_HSFREQRANGE_ADDR, code);

	/* TESTCLK stays high while the D-PHY runs */
	regs->CSI_PHY_TEST_CTRL0 = CSI_PHY_TEST_CTRL0_PHY_TESTCLK_Msk;
	regs->CSI_PHY_SHUTDOWNZ = CSI_PHY_SHUTDOWNZ_PHY_SHUTDOWNZ_Msk;
	k_busy_wait(200);
	regs->CSI_DPHY_RSTZ = CSI_DPHY_RSTZ_DPHY_RSTZ_Msk;

	return 0;
}

static void csi2host_report_stopstate(const struct device *dev)
{
	const struct csi2host_config *cfg = dev->config;
	uint32_t expected = CSI2HOST_STOPSTATE(cfg->num_lanes);
	uint32_t state = 0;

	for (int wait_us = 0; wait_us < CSI2HOST_STOPSTATE_TIMEOUT_US; wait_us += 100) {
		state = cfg->regs->CSI_PHY_STOPSTATE;
		if ((state & expected) == expected) {
			return;
		}
		k_busy_wait(100);
	}

	LOG_WRN("D-PHY lanes not in stop state (0x%08x, expected 0x%08x)", state, expected);
}

static int csi2host_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct csi2host_config *cfg = dev->config;

	/* The controller is transparent to the format, it only carries packets */
	return video_set_format(cfg->source_dev, fmt);
}

static int csi2host_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct csi2host_config *cfg = dev->config;

	return video_get_format(cfg->source_dev, fmt);
}

static int csi2host_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct csi2host_config *cfg = dev->config;

	return video_get_caps(cfg->source_dev, caps);
}

static int csi2host_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct csi2host_config *cfg = dev->config;
	struct csi2host_data *data = dev->data;
	csi_registers_t *regs = cfg->regs;
	struct video_format fmt;
	int64_t link_freq;
	int ret;

	if (!enable) {
		ret = video_stream_stop(cfg->source_dev, type);

		regs->CSI_INT_MSK_PHY_FATAL = 0;
		regs->CSI_INT_MSK_PKT_FATAL = 0;
		regs->CSI_INT_MSK_FRAME_FATAL = 0;
		regs->CSI_INT_MSK_PHY = 0;
		regs->CSI_INT_MSK_PKT = 0;
		irq_disable(cfg->irq);

		regs->CSI_N_LANES = 0;
		regs->CSI_CSI2_RESETN = 0;
		csi2host_dphy_power_down(regs);

		if (data->phy_fatal_errors || data->pkt_fatal_errors ||
		    data->frame_fatal_errors || data->phy_errors || data->pkt_errors) {
			LOG_WRN("CSI-2 errors: phy-fatal %u, pkt-fatal %u, frame-fatal %u, "
				"phy %u, pkt %u",
				data->phy_fatal_errors, data->pkt_fatal_errors,
				data->frame_fatal_errors, data->phy_errors, data->pkt_errors);
		}

		return ret;
	}

	ret = video_get_format(cfg->source_dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	link_freq = video_get_csi_link_freq(cfg->source_dev,
					    video_bits_per_pixel(fmt.pixelformat),
					    cfg->num_lanes);
	if (link_freq <= 0) {
		LOG_ERR("Failed to retrieve the source link frequency");
		return -EIO;
	}

	memset(data, 0, sizeof(*data));

	/*
	 * The link is brought up before the sensor starts transmitting: the D-PHY
	 * analog configuration, the lane count and the data identifier all have to
	 * be in place before the first high-speed transmission.
	 */
	regs->CSI_CSI2_RESETN = 0;

	/* The D-PHY runs at double data rate, hence two bits per lane per clock */
	ret = csi2host_dphy_power_up(dev, (uint32_t)(link_freq * 2 / 1000000));
	if (ret < 0) {
		return ret;
	}

	regs->CSI_N_LANES = CSI_N_LANES_N_LANES(cfg->num_lanes - 1);

	/* Identify the incoming stream so that errors are reported against it */
	regs->CSI_DATA_IDS_1 = CSI_DATA_IDS_1_DI0_DT(video_mipi_data_type(fmt.pixelformat)) |
			       CSI_DATA_IDS_1_DI0_VC(0);

	/* Report every error class through the interrupt line */
	regs->CSI_INT_MSK_PHY_FATAL = CSI_INT_MSK_PHY_FATAL_Msk;
	regs->CSI_INT_MSK_PKT_FATAL = CSI_INT_MSK_PKT_FATAL_Msk;
	regs->CSI_INT_MSK_FRAME_FATAL = CSI_INT_MSK_FRAME_FATAL_Msk;
	regs->CSI_INT_MSK_PHY = CSI_INT_MSK_PHY_Msk;
	regs->CSI_INT_MSK_PKT = CSI_INT_MSK_PKT_Msk;
	irq_enable(cfg->irq);

	regs->CSI_CSI2_RESETN = CSI_CSI2_RESETN_CSI2_RESETN_Msk;

	ret = video_stream_start(cfg->source_dev, type);
	if (ret < 0) {
		irq_disable(cfg->irq);
		return ret;
	}

	/* The lanes only settle once the sensor drives them */
	csi2host_report_stopstate(dev);

	LOG_DBG("D-PHY receive status 0x%08x (HS clock %s)", regs->CSI_PHY_RX,
		(regs->CSI_PHY_RX & CSI_PHY_RX_PHY_RXCLKACTIVEHS_Msk) ? "active" : "idle");

	/*
	 * Report what the receiver made of the first lines the sensor sent. A link that
	 * never locks shows a large phy-fatal count, one that is not driven at all shows
	 * no counts and a clock lane that never leaves low power.
	 */

	return 0;
}

static int csi2host_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct csi2host_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int csi2host_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct csi2host_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int csi2host_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct csi2host_config *cfg = dev->config;

	return video_enum_frmival(cfg->source_dev, fie);
}

static void csi2host_isr(const struct device *dev)
{
	const struct csi2host_config *cfg = dev->config;
	struct csi2host_data *data = dev->data;
	csi_registers_t *regs = cfg->regs;
	uint32_t status = regs->CSI_INT_ST_MAIN;
	uint32_t detail = 0;

	/*
	 * The per-class status registers are cleared on read, and the main status
	 * stays asserted for as long as one of them is set, so each has to be read
	 * unconditionally rather than only when the log level shows it.
	 */
	if (status & CSI_INT_ST_MAIN_STATUS_INT_PHY_FATAL_Msk) {
		detail = regs->CSI_INT_ST_PHY_FATAL;
		LOG_DBG("PHY fatal error: 0x%08x", detail);
		data->phy_fatal_errors++;
	}

	if (status & CSI_INT_ST_MAIN_STATUS_INT_PKT_FATAL_Msk) {
		detail = regs->CSI_INT_ST_PKT_FATAL;
		LOG_DBG("packet fatal error: 0x%08x", detail);
		data->pkt_fatal_errors++;
	}

	if (status & CSI_INT_ST_MAIN_STATUS_INT_FRAME_FATAL_Msk) {
		detail = regs->CSI_INT_ST_FRAME_FATAL;
		LOG_DBG("frame fatal error: 0x%08x", detail);
		data->frame_fatal_errors++;
	}

	if (status & CSI_INT_ST_MAIN_STATUS_INT_DPHY_Msk) {
		detail = regs->CSI_INT_ST_PHY;
		LOG_DBG("PHY error: 0x%08x", detail);
		data->phy_errors++;
	}

	if (status & CSI_INT_ST_MAIN_STATUS_INT_PKT_Msk) {
		detail = regs->CSI_INT_ST_PKT;
		LOG_DBG("packet error: 0x%08x", detail);
		data->pkt_errors++;
	}

	ARG_UNUSED(detail);
}

static DEVICE_API(video, csi2host_driver_api) = {
	.set_format = csi2host_set_fmt,
	.get_format = csi2host_get_fmt,
	.get_caps = csi2host_get_caps,
	.set_stream = csi2host_set_stream,
	.set_frmival = csi2host_set_frmival,
	.get_frmival = csi2host_get_frmival,
	.enum_frmival = csi2host_enum_frmival,
};

static int csi2host_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct csi2host_config *cfg = dev->config;
	uint32_t rate;
	int ret;

	if (!device_is_ready(cfg->source_dev)) {
		LOG_ERR("Source device %s is not ready", cfg->source_dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	if (cfg->num_lanes < 1 || cfg->num_lanes > 2) {
		LOG_ERR("%u data lanes are not supported", cfg->num_lanes);
		return -EINVAL;
	}

	/*
	 * The peripheral clock feeds the controller logic, the generated clock
	 * the configuration clock input of the D-PHY.
	 */
	for (uint8_t i = 0; i < cfg->num_clocks; i++) {
		if (cfg->clock_cfg[i].clock_type == PMC_TYPE_GCK) {
			ret = clock_control_set_rate(pmc,
						     (clock_control_subsys_t)&cfg->clock_cfg[i],
						     (clock_control_subsys_rate_t)
							     (uintptr_t)cfg->phyclk_rate);
			if (ret < 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to set the D-PHY configuration clock rate (%d)",
					ret);
				return ret;
			}
		}

		ret = clock_control_on(pmc, (clock_control_subsys_t)&cfg->clock_cfg[i]);
		if (ret < 0) {
			LOG_ERR("Failed to enable clock %u (%d)", i, ret);
			return ret;
		}

		if (cfg->clock_cfg[i].clock_type != PMC_TYPE_GCK) {
			continue;
		}

		ret = clock_control_get_rate(pmc, (clock_control_subsys_t)&cfg->clock_cfg[i],
					     &rate);
		if (ret < 0) {
			LOG_WRN("Cannot read the D-PHY configuration clock rate (%d)", ret);
		} else if (!IN_RANGE(rate, CSI2HOST_CFGCLK_MIN_HZ, CSI2HOST_CFGCLK_MAX_HZ)) {
			LOG_WRN("D-PHY configuration clock is %u Hz, outside the %u-%u Hz range",
				rate, CSI2HOST_CFGCLK_MIN_HZ, CSI2HOST_CFGCLK_MAX_HZ);
		} else {
			LOG_INF("D-PHY configuration clock: %u Hz (requested %u Hz)", rate,
				cfg->phyclk_rate);
		}
	}

	/*
	 * The controller and its D-PHY are left untouched here: they are fully
	 * reset and configured when the stream is started. This keeps register
	 * access out of the boot path, where a stalled access would hang the
	 * system before the console is usable.
	 */
	LOG_DBG("ready, %u data lanes, source is %s", cfg->num_lanes, cfg->source_dev->name);

	return 0;
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 1, 0)))

#define CSI2HOST_INIT(n)                                                                           \
	BUILD_ASSERT(DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(n, 1, 0)),                              \
		     "the CSI-2 host requires a sink endpoint describing the sensor link");        \
                                                                                                   \
	static struct csi2host_data csi2host_data_##n;                                             \
                                                                                                   \
	static const struct sam_clk_cfg csi2host_clock_cfg_##n[] = SAM_DT_INST_CLOCKS_PMC_CFG(n);   \
                                                                                                   \
	static const struct csi2host_config csi2host_cfg_##n = {                                   \
		.regs = (csi_registers_t *)DT_INST_REG_ADDR(n),                                    \
		.source_dev = SOURCE_DEV(n),                                                       \
		.clock_cfg = csi2host_clock_cfg_##n,                                               \
		.num_clocks = ARRAY_SIZE(csi2host_clock_cfg_##n),                                  \
		.irq = DT_INST_IRQN(n),                                                            \
		.bitrate_code = DT_INST_PROP_OR(n, microchip_dphy_bitrate_code,                     \
						CSI2HOST_BITRATE_CODE_AUTO),                       \
		.dphy_freq_kbps = DT_INST_PROP_OR(n, microchip_dphy_frequency, 0),                  \
		.phyclk_rate = DT_INST_PROP(n, assigned_clock_rates),                              \
		.num_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(n, 1, 0), data_lanes),             \
	};                                                                                         \
                                                                                                   \
	static int csi2host_init_##n(const struct device *dev)                                     \
	{                                                                                          \
		/* Enabled only while streaming, see csi2host_set_stream() */                        \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), csi2host_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		return csi2host_init(dev);                                                         \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &csi2host_init_##n, NULL, &csi2host_data_##n, &csi2host_cfg_##n,   \
			      POST_KERNEL, CONFIG_VIDEO_MCHP_SAM_CSI2HOST_INIT_PRIORITY,           \
			      &csi2host_driver_api);                                               \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(csi2host_##n, DEVICE_DT_INST_GET(n), SOURCE_DEV(n));

DT_INST_FOREACH_STATUS_OKAY(CSI2HOST_INIT)
