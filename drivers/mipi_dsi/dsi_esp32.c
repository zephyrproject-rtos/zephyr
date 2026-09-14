/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp_mipi_dsi

#include <zephyr/device.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/logging/log.h>

#include <hal/mipi_dsi_hal.h>
#include <hal/mipi_dsi_ll.h>
#include <hal/mipi_dsi_host_ll.h>
#include <hal/mipi_dsi_brg_ll.h>
#include <hal/mipi_dsi_phy_ll.h>
#include <hal/mipi_dsi_types.h>
#include <hal/clk_gate_ll.h>
#include <soc/clk_tree_defs.h>
#include <soc/reg_base.h>

#include "../display/display_esp32_dsi.h"

LOG_MODULE_REGISTER(dsi_esp32, CONFIG_MIPI_DSI_LOG_LEVEL);

#define MIPI_DSI_DEFAULT_TIMEOUT_CLK_FREQ_MHZ 10
#define MIPI_DSI_DEFAULT_ESCAPE_CLK_FREQ_MHZ  18
#if defined(CONFIG_SOC_ESP32P4_REV_MIN_FULL) && CONFIG_SOC_ESP32P4_REV_MIN_FULL < 300
#define MIPI_DSI_PHY_PLLREF_CLK_SRC           MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT_LEGACY
#define MIPI_DSI_PHY_PLL_REF_CLK_FREQ_HZ      20000000
#else
#define MIPI_DSI_PHY_PLLREF_CLK_SRC           MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT
#define MIPI_DSI_PHY_PLL_REF_CLK_FREQ_HZ      40000000
#endif

/* The PHY PLL only has settings for this range, and a rate outside it is
 * programmed with the wrong one rather than being rejected.
 */
#define MIPI_DSI_LANE_BIT_RATE_MIN_MBPS 80
#define MIPI_DSI_LANE_BIT_RATE_MAX_MBPS 1500

/* Both are polled in 100 us steps. */
#define MIPI_DSI_PLL_LOCK_TIMEOUT_US  10000
#define MIPI_DSI_LANE_STOP_TIMEOUT_US 10000
#define MIPI_DSI_POLL_INTERVAL_US     100

struct mipi_dsi_esp32_config {
	uintptr_t host_reg;
	uintptr_t bridge_reg;
	uint8_t bus_id;
	uint8_t num_data_lanes;
	uint32_t lane_bit_rate_mbps;
	uint32_t dpi_clock_freq_hz;
	const struct device *display;
};

struct mipi_dsi_esp32_data {
	const struct device *dev;
	mipi_dsi_hal_context_t hal;
};

/* The panel is configured over commands before video starts, so the command
 * speed is set to LP first and revisited once a panel attaches and reports
 * whether it wants low power or high speed transfers.
 */
static void mipi_dsi_esp32_set_command_speed(mipi_dsi_hal_context_t *hal, bool low_power)
{
	mipi_dsi_ll_trans_speed_mode_t speed =
		low_power ? MIPI_DSI_LL_TRANS_SPEED_LP : MIPI_DSI_LL_TRANS_SPEED_HS;

	for (uint8_t vcid = 0; vcid < 3; vcid++) {
		mipi_dsi_host_ll_set_gen_short_wr_speed_mode(hal->host, vcid, speed);
		mipi_dsi_host_ll_set_gen_short_rd_speed_mode(hal->host, vcid, speed);
	}

	mipi_dsi_host_ll_set_gen_long_wr_speed_mode(hal->host, speed);
	mipi_dsi_host_ll_set_dcs_short_wr_speed_mode(hal->host, 0, speed);
	mipi_dsi_host_ll_set_dcs_short_wr_speed_mode(hal->host, 1, speed);
	mipi_dsi_host_ll_set_dcs_long_wr_speed_mode(hal->host, speed);
	mipi_dsi_host_ll_set_dcs_short_rd_speed_mode(hal->host, 0, speed);
	mipi_dsi_host_ll_set_mrps_speed_mode(hal->host, speed);
}

static int mipi_dsi_esp32_attach(const struct device *dev, uint8_t channel,
				 const struct mipi_dsi_device *mdev)
{
	struct mipi_dsi_esp32_data *data = dev->data;
	const struct mipi_dsi_esp32_config *config = dev->config;
	mipi_dsi_hal_context_t *hal = &data->hal;
	lcd_color_format_t color_fmt;
	uint32_t bits_per_pixel;

	switch (mdev->pixfmt) {
	case MIPI_DSI_PIXFMT_RGB888:
		color_fmt = LCD_COLOR_FMT_RGB888;
		bits_per_pixel = 24;
		break;
	case MIPI_DSI_PIXFMT_RGB565:
		color_fmt = LCD_COLOR_FMT_RGB565;
		bits_per_pixel = 16;
		break;
	default:
		LOG_ERR("Unsupported pixel format %u", mdev->pixfmt);
		return -ENOTSUP;
	}

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("Only video mode is supported");
		return -ENOTSUP;
	}

	/* The D-PHY was brought up from the host node's lane count before any
	 * panel existed, so a panel asking for a different one would be driven
	 * over lanes that were never configured for it.
	 */
	if (mdev->data_lanes != config->num_data_lanes) {
		LOG_ERR("Panel requests %u lanes, the host is configured for %u", mdev->data_lanes,
			config->num_data_lanes);
		return -EINVAL;
	}

	if (!device_is_ready(config->display)) {
		LOG_ERR("DSI display controller not ready");
		return -ENODEV;
	}

	bool low_power_cmds = (mdev->mode_flags & MIPI_DSI_MODE_LPM) != 0;

	mipi_dsi_esp32_set_command_speed(hal, low_power_cmds);

	mipi_dsi_host_ll_dpi_set_vcid(hal->host, channel);
	mipi_dsi_host_ll_dpi_set_color_coding(hal->host, color_fmt, 0);
	mipi_dsi_host_ll_dpi_set_timing_polarity(hal->host, false, false, false, false, false);

	mipi_dsi_host_ll_dpi_set_pattern_type(hal->host, MIPI_DSI_PATTERN_NONE);

	mipi_dsi_host_ll_dpi_enable_lp_horizontal_timing(hal->host, true, true);
	mipi_dsi_host_ll_dpi_enable_lp_vertical_timing(hal->host, true, true, true, true);
	mipi_dsi_host_ll_dpi_enable_lp_command(hal->host, low_power_cmds);
	mipi_dsi_host_ll_dpi_enable_frame_ack(hal->host, true);

	mipi_dsi_host_ll_dpi_set_video_burst_type(hal->host,
						  MIPI_DSI_LL_VIDEO_BURST_WITH_SYNC_PULSES);

	mipi_dsi_host_ll_dpi_set_video_packet_pixel_num(hal->host, mdev->timings.hactive);
	mipi_dsi_host_ll_dpi_set_trunks_num(hal->host, 0);
	mipi_dsi_host_ll_dpi_set_null_packet_size(hal->host, 0);

	float dpi_clk_src_freq_mhz = 240.0f;
	uint32_t dpi_div = mipi_dsi_hal_host_dpi_calculate_divider(
		hal, dpi_clk_src_freq_mhz, (float)config->dpi_clock_freq_hz / 1000000.0f);

	mipi_dsi_ll_set_dpi_clock_source(0, MIPI_DSI_DPI_CLK_SRC_PLL_F240M);
	mipi_dsi_ll_set_dpi_clock_div(0, dpi_div);
	mipi_dsi_ll_enable_dpi_clock(0, true);

	mipi_dsi_hal_host_dpi_set_horizontal_timing(hal, mdev->timings.hsync, mdev->timings.hbp,
						    mdev->timings.hactive, mdev->timings.hfp);
	mipi_dsi_hal_host_dpi_set_vertical_timing(hal, mdev->timings.vsync, mdev->timings.vbp,
						  mdev->timings.vactive, mdev->timings.vfp);

	mipi_dsi_brg_ll_force_enable_reg_clock(hal->bridge, true);

	mipi_dsi_brg_ll_set_num_pixel_bits(
		hal->bridge, mdev->timings.hactive * mdev->timings.vactive * bits_per_pixel);
	mipi_dsi_brg_ll_set_underrun_discard_count(hal->bridge, mdev->timings.hactive);
	mipi_dsi_brg_ll_set_input_color_format(hal->bridge, color_fmt);
	mipi_dsi_brg_ll_set_output_color_format(hal->bridge, color_fmt, 0);

	mipi_dsi_brg_ll_set_flow_controller(hal->bridge, MIPI_DSI_LL_FLOW_CONTROLLER_DMA);
	mipi_dsi_brg_ll_set_multi_block_number(hal->bridge, 1);
	mipi_dsi_brg_ll_set_burst_len(hal->bridge, 256);
	mipi_dsi_brg_ll_set_empty_threshold(hal->bridge, 1024 - 256);

	mipi_dsi_brg_ll_enable(hal->bridge, true);
	mipi_dsi_brg_ll_update_dpi_config(hal->bridge);

	int err = display_esp32_dsi_start(config->display, bits_per_pixel);

	if (err != 0) {
		mipi_dsi_brg_ll_enable(hal->bridge, false);
		mipi_dsi_brg_ll_force_enable_reg_clock(hal->bridge, false);
		mipi_dsi_ll_enable_dpi_clock(0, false);
		return err;
	}

	mipi_dsi_host_ll_enable_video_mode(hal->host, true);

	mipi_dsi_host_ll_set_clock_lane_state(hal->host, MIPI_DSI_LL_CLOCK_LANE_STATE_AUTO);

	mipi_dsi_brg_ll_enable_dpi_output(hal->bridge, true);
	mipi_dsi_brg_ll_update_dpi_config(hal->bridge);

	LOG_INF("MIPI DSI attached: %ux%u, %u bpp, channel %u", mdev->timings.hactive,
		mdev->timings.vactive, bits_per_pixel, channel);

	return 0;
}

static ssize_t mipi_dsi_esp32_transfer(const struct device *dev, uint8_t channel,
				       struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_esp32_data *data = dev->data;
	mipi_dsi_hal_context_t *hal = &data->hal;

	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
		mipi_dsi_hal_host_gen_write_dcs_command(hal, channel, msg->cmd, 1, NULL, 0);
		return 0;

	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		mipi_dsi_hal_host_gen_write_dcs_command(hal, channel, msg->cmd, 1, msg->tx_buf,
							msg->tx_len);
		return msg->tx_len;

	case MIPI_DSI_DCS_LONG_WRITE:
		mipi_dsi_hal_host_gen_write_dcs_command(hal, channel, msg->cmd, 1, msg->tx_buf,
							msg->tx_len);
		return msg->tx_len;

	case MIPI_DSI_DCS_READ:
		if (msg->rx_buf == NULL || msg->rx_len == 0) {
			return -EINVAL;
		}
		mipi_dsi_hal_host_gen_read_dcs_command(hal, channel, msg->cmd, 1, msg->rx_buf,
						       msg->rx_len);
		return msg->rx_len;

	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		mipi_dsi_hal_host_gen_write_short_packet(hal, channel,
							 MIPI_DSI_DT_GENERIC_SHORT_WRITE_0, 0);
		return msg->tx_len;

	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		if (msg->tx_buf == NULL || msg->tx_len < 1) {
			return -EINVAL;
		}
		mipi_dsi_hal_host_gen_write_short_packet(hal, channel,
							 MIPI_DSI_DT_GENERIC_SHORT_WRITE_1,
							 ((const uint8_t *)msg->tx_buf)[0]);
		return msg->tx_len;

	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		if (msg->tx_buf == NULL || msg->tx_len < 2) {
			return -EINVAL;
		}
		mipi_dsi_hal_host_gen_write_short_packet(
			hal, channel, MIPI_DSI_DT_GENERIC_SHORT_WRITE_2,
			((const uint8_t *)msg->tx_buf)[0] |
				(((const uint8_t *)msg->tx_buf)[1] << 8));
		return msg->tx_len;

	case MIPI_DSI_GENERIC_LONG_WRITE:
		mipi_dsi_hal_host_gen_write_long_packet(
			hal, channel, MIPI_DSI_DT_GENERIC_LONG_WRITE, msg->tx_buf, msg->tx_len);
		return msg->tx_len;

	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
		if (msg->rx_buf == NULL || msg->rx_len == 0) {
			return -EINVAL;
		}
		mipi_dsi_hal_host_gen_read_short_packet(hal, channel,
							MIPI_DSI_DT_GENERIC_READ_REQUEST_0, 0,
							msg->rx_buf, msg->rx_len);
		return msg->rx_len;

	/* The parameter bytes would have to reach the wire for the panel to
	 * answer about the right register, so a read that silently drops them
	 * is refused rather than answered from the wrong one.
	 */
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
		LOG_ERR("Parameterized generic reads are not implemented");
		return -ENOTSUP;

	default:
		LOG_ERR("Unsupported message type %u", msg->type);
		return -ENOTSUP;
	}
}

static int mipi_dsi_esp32_detach(const struct device *dev, uint8_t channel,
				 const struct mipi_dsi_device *mdev)
{
	struct mipi_dsi_esp32_data *data = dev->data;
	const struct mipi_dsi_esp32_config *config = dev->config;
	mipi_dsi_hal_context_t *hal = &data->hal;

	mipi_dsi_brg_ll_enable_dpi_output(hal->bridge, false);
	mipi_dsi_brg_ll_update_dpi_config(hal->bridge);

	display_esp32_dsi_stop(config->display);

	mipi_dsi_host_ll_enable_video_mode(hal->host, false);
	mipi_dsi_brg_ll_enable(hal->bridge, false);
	mipi_dsi_ll_enable_dpi_clock(0, false);

	return 0;
}

static int mipi_dsi_esp32_init(const struct device *dev)
{
	const struct mipi_dsi_esp32_config *config = dev->config;
	struct mipi_dsi_esp32_data *data = dev->data;
	mipi_dsi_hal_context_t *hal = &data->hal;

	data->dev = dev;

	clk_gate_ll_ref_20m_clk_en(true);
	clk_gate_ll_ref_240m_clk_en(true);

	mipi_dsi_ll_enable_bus_clock(0, true);
	mipi_dsi_ll_reset_register(0);

	mipi_dsi_ll_set_phy_config_clock_source(0, MIPI_DSI_PHY_CFG_CLK_SRC_DEFAULT);
	mipi_dsi_ll_enable_phy_config_clock(0, true);

	mipi_dsi_ll_set_phy_pllref_clock_source(0, MIPI_DSI_PHY_PLLREF_CLK_SRC);
	mipi_dsi_ll_set_phy_pll_ref_clock_div(0, 1);
	mipi_dsi_ll_enable_phy_pllref_clock(0, true);

	mipi_dsi_hal_config_t hal_config = {
		.bus_id = config->bus_id,
		.lane_bit_rate_mbps = config->lane_bit_rate_mbps,
		.num_data_lanes = config->num_data_lanes,
	};
	mipi_dsi_hal_init(hal, &hal_config);

	/* The HAL addresses the peripheral by bus index. Check that it drives
	 * the registers the devicetree describes.
	 */
	if ((uintptr_t)hal->host != config->host_reg ||
	    (uintptr_t)hal->bridge != config->bridge_reg) {
		LOG_ERR("Devicetree registers do not match the MIPI DSI peripheral");
		return -ENODEV;
	}

	mipi_dsi_hal_configure_phy_pll(hal, MIPI_DSI_PHY_PLL_REF_CLK_FREQ_HZ,
				       (float)config->lane_bit_rate_mbps);

	if (!WAIT_FOR(mipi_dsi_phy_ll_is_pll_locked(hal->host), MIPI_DSI_PLL_LOCK_TIMEOUT_US,
		      k_busy_wait(MIPI_DSI_POLL_INTERVAL_US))) {
		LOG_ERR("PHY PLL lock timeout (phy_status=0x%08x)", hal->host->phy_status.val);
		return -ETIMEDOUT;
	}

	if (!WAIT_FOR(mipi_dsi_phy_ll_are_lanes_stopped(hal->host, config->num_data_lanes),
		      MIPI_DSI_LANE_STOP_TIMEOUT_US, k_busy_wait(MIPI_DSI_POLL_INTERVAL_US))) {
		LOG_ERR("Lanes stop state timeout");
		return -ETIMEDOUT;
	}

	mipi_dsi_host_ll_enable_video_mode(hal->host, false);
	mipi_dsi_host_ll_set_clock_lane_state(hal->host, MIPI_DSI_LL_CLOCK_LANE_STATE_LP);

	mipi_dsi_host_ll_enable_cmd_ack(hal->host, true);
	mipi_dsi_esp32_set_command_speed(hal, true);

	mipi_dsi_phy_ll_set_switch_time(hal->host, 50, 104, 46, 128);

	mipi_dsi_host_ll_enable_rx_crc(hal->host, true);
	mipi_dsi_host_ll_enable_rx_ecc(hal->host, true);
	mipi_dsi_host_ll_enable_tx_eotp(hal->host, true, false);

	uint32_t byte_clk_mhz = config->lane_bit_rate_mbps / 8;

	mipi_dsi_host_ll_set_timeout_clock_division(
		hal->host, DIV_ROUND_CLOSEST(byte_clk_mhz, MIPI_DSI_DEFAULT_TIMEOUT_CLK_FREQ_MHZ));
	mipi_dsi_host_ll_set_escape_clock_division(
		hal->host, DIV_ROUND_CLOSEST(byte_clk_mhz, MIPI_DSI_DEFAULT_ESCAPE_CLK_FREQ_MHZ));

	mipi_dsi_host_ll_set_timeout_count(hal->host, 0, 0, 0, 0, 0, 0, 0);
	mipi_dsi_phy_ll_set_max_read_time(hal->host, 6000);
	mipi_dsi_phy_ll_set_stop_wait_time(hal->host, 0x3F);

	mipi_dsi_brg_ll_enable_ref_clock(hal->bridge, true);

	LOG_INF("MIPI DSI initialized: %u lanes, %u Mbps", config->num_data_lanes,
		config->lane_bit_rate_mbps);

	return 0;
}

static DEVICE_API(mipi_dsi, mipi_dsi_esp32_api) = {
	.attach = mipi_dsi_esp32_attach,
	.transfer = mipi_dsi_esp32_transfer,
	.detach = mipi_dsi_esp32_detach,
};

/* A panel that asks for a different lane count is refused at attach time, so
 * the mismatch is caught here instead, where it is a devicetree constant. A
 * panel keeps the count in the first entry of the property, while the host
 * node carries it directly.
 */
#define MIPI_DSI_ESP32_ASSERT_LANES(node)                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(node, data_lanes),                                             \
		   (BUILD_ASSERT(DT_PROP_BY_IDX(node, data_lanes, 0) ==                            \
					 DT_PROP(DT_PARENT(node), data_lanes),                     \
				 "panel data-lanes must match the DSI host data-lanes");))

#define MIPI_DSI_ESP32_DEVICE(inst)                                                                \
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), MIPI_DSI_ESP32_ASSERT_LANES)               \
	BUILD_ASSERT(DT_INST_PROP(inst, phy_clock) / 1000000 >= MIPI_DSI_LANE_BIT_RATE_MIN_MBPS && \
			     DT_INST_PROP(inst, phy_clock) / 1000000 <=                            \
				     MIPI_DSI_LANE_BIT_RATE_MAX_MBPS,                              \
		     "phy-clock is outside the range the D-PHY PLL can lock to");                  \
	static struct mipi_dsi_esp32_data mipi_dsi_esp32_data_##inst;                              \
	static const struct mipi_dsi_esp32_config mipi_dsi_esp32_config_##inst = {                 \
		.host_reg = DT_INST_REG_ADDR_BY_NAME(inst, host),                                  \
		.bridge_reg = DT_INST_REG_ADDR_BY_NAME(inst, bridge),                              \
		.bus_id = inst,                                                                    \
		.num_data_lanes = DT_INST_PROP(inst, data_lanes),                                  \
		.lane_bit_rate_mbps = DT_INST_PROP(inst, phy_clock) / 1000000,                     \
		.dpi_clock_freq_hz = DT_INST_PROP(inst, dpi_clock_frequency),                      \
		.display = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mipi_dsi_esp32_init, NULL, &mipi_dsi_esp32_data_##inst,        \
			      &mipi_dsi_esp32_config_##inst, POST_KERNEL,                          \
			      CONFIG_MIPI_DSI_INIT_PRIORITY, &mipi_dsi_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DSI_ESP32_DEVICE)
