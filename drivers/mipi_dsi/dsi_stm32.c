/*
 * Copyright (c) 2023 bytes at work AG
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * based on dsi_mcux.c
 * Copyright (c) 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_mipi_dsi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dsi_stm32, CONFIG_MIPI_DSI_LOG_LEVEL);

#if defined(CONFIG_STM32_LTDC_ARGB8888)
#define STM32_DSI_INIT_PIXEL_FORMAT	DSI_RGB888
#elif defined(CONFIG_STM32_LTDC_RGB888)
#define STM32_DSI_INIT_PIXEL_FORMAT	DSI_RGB888
#elif defined(CONFIG_STM32_LTDC_RGB565)
#define STM32_DSI_INIT_PIXEL_FORMAT	DSI_RGB565
#else
#error "Invalid LTDC pixel format chosen"
#endif /* CONFIG_STM32_LTDC_ARGB8888 */

#define MAX_TX_ESC_CLK_KHZ 20000
#define MAX_TX_ESC_CLK_DIV 8

struct mipi_dsi_stm32_config {
	const struct device *rcc;
	const struct reset_dt_spec reset;
	struct stm32_pclken dsi_clk;
	struct stm32_pclken ref_clk;
	struct stm32_pclken pix_clk;
	uint32_t data_lanes;
	uint32_t active_errors;
	uint32_t lp_rx_filter_freq;
	int test_pattern;
};

struct mipi_dsi_stm32_data {
	DSI_HandleTypeDef hdsi;
	DSI_HOST_TimeoutTypeDef *host_timeouts;
	DSI_PHY_TimerTypeDef *phy_timings;
	DSI_VidCfgTypeDef vid_cfg;
	DSI_PLLInitTypeDef pll_init;
	uint32_t lane_clk_khz;
	uint32_t pixel_clk_khz;
};

static void mipi_dsi_stm32_log_config(const struct device *dev)
{
	const struct mipi_dsi_stm32_config *config = dev->config;
	struct mipi_dsi_stm32_data *data = dev->data;

	LOG_DBG("DISPLAY: pix %d kHz, lane %d kHz", data->pixel_clk_khz, data->lane_clk_khz);
	LOG_DBG("HAL_DSI_Init setup:");
	LOG_DBG("  AutomaticClockLaneControl 0x%x", data->hdsi.Init.AutomaticClockLaneControl);
	LOG_DBG("  TXEscapeCkdiv %u", data->hdsi.Init.TXEscapeCkdiv);
	LOG_DBG("  NumberOfLanes %u", data->hdsi.Init.NumberOfLanes);
	LOG_DBG("  PLLNDIV %u", data->pll_init.PLLNDIV);
	LOG_DBG("  PLLIDF %u", data->pll_init.PLLIDF);
	LOG_DBG("  PLLODF %u", data->pll_init.PLLODF);

	LOG_DBG("HAL_DSI_ConfigVideoMode setup:");
	LOG_DBG("  VirtualChannelID %u", data->vid_cfg.VirtualChannelID);
	LOG_DBG("  ColorCoding 0x%x", data->vid_cfg.ColorCoding);
	LOG_DBG("  LooselyPacked 0x%x", data->vid_cfg.LooselyPacked);
	LOG_DBG("  Mode 0x%x", data->vid_cfg.Mode);
	LOG_DBG("  PacketSize %u", data->vid_cfg.PacketSize);
	LOG_DBG("  NumberOfChunks %u", data->vid_cfg.NumberOfChunks);
	LOG_DBG("  NullPacketSize %u", data->vid_cfg.NullPacketSize);
	LOG_DBG("  HSPolarity 0x%x", data->vid_cfg.HSPolarity);
	LOG_DBG("  VSPolarity 0x%x", data->vid_cfg.VSPolarity);
	LOG_DBG("  DEPolarity 0x%x", data->vid_cfg.DEPolarity);
	LOG_DBG("  HorizontalSyncActive %u", data->vid_cfg.HorizontalSyncActive);
	LOG_DBG("  HorizontalBackPorch %u", data->vid_cfg.HorizontalBackPorch);
	LOG_DBG("  HorizontalLine %u", data->vid_cfg.HorizontalLine);
	LOG_DBG("  VerticalSyncActive %u", data->vid_cfg.VerticalSyncActive);
	LOG_DBG("  VerticalBackPorch %u", data->vid_cfg.VerticalBackPorch);
	LOG_DBG("  VerticalFrontPorch %u", data->vid_cfg.VerticalFrontPorch);
	LOG_DBG("  VerticalActive %u", data->vid_cfg.VerticalActive);
	LOG_DBG("  LPCommandEnable 0x%x", data->vid_cfg.LPCommandEnable);
	LOG_DBG("  LPLargestPacketSize %u", data->vid_cfg.LPLargestPacketSize);
	LOG_DBG("  LPVACTLargestPacketSize %u", data->vid_cfg.LPVACTLargestPacketSize);
	LOG_DBG("  LPHorizontalFrontPorchEnable 0x%x", data->vid_cfg.LPHorizontalFrontPorchEnable);
	LOG_DBG("  LPHorizontalBackPorchEnable 0x%x", data->vid_cfg.LPHorizontalBackPorchEnable);
	LOG_DBG("  LPVerticalActiveEnable 0x%x", data->vid_cfg.LPVerticalActiveEnable);
	LOG_DBG("  LPVerticalFrontPorchEnable 0x%x", data->vid_cfg.LPVerticalFrontPorchEnable);
	LOG_DBG("  LPVerticalBackPorchEnable 0x%x", data->vid_cfg.LPVerticalBackPorchEnable);
	LOG_DBG("  LPVerticalSyncActiveEnable 0x%x", data->vid_cfg.LPVerticalSyncActiveEnable);
	LOG_DBG("  FrameBTAAcknowledgeEnable 0x%x", data->vid_cfg.FrameBTAAcknowledgeEnable);

	if (config->active_errors) {
		LOG_DBG("HAL_DSI_ConfigErrorMonitor: 0x%x", config->active_errors);
	}

	if (config->lp_rx_filter_freq) {
		LOG_DBG("HAL_DSI_SetLowPowerRXFilter: %d", config->lp_rx_filter_freq);
	}

	if (data->host_timeouts) {
		DSI_HOST_TimeoutTypeDef *ht = data->host_timeouts;

		LOG_DBG("HAL_DSI_ConfigHostTimeouts:");
		LOG_DBG("  TimeoutCkdiv %u", ht->TimeoutCkdiv);
		LOG_DBG("  HighSpeedTransmissionTimeout %u", ht->HighSpeedTransmissionTimeout);
		LOG_DBG("  LowPowerReceptionTimeout %u", ht->LowPowerReceptionTimeout);
		LOG_DBG("  HighSpeedReadTimeout %u", ht->HighSpeedReadTimeout);
		LOG_DBG("  LowPowerReadTimeout %u", ht->LowPowerReadTimeout);
		LOG_DBG("  HighSpeedWriteTimeout %u", ht->HighSpeedWriteTimeout);
		LOG_DBG("  HighSpeedWritePrespMode %u", ht->HighSpeedWritePrespMode);
		LOG_DBG("  LowPowerWriteTimeout %u", ht->LowPowerWriteTimeout);
		LOG_DBG("  BTATimeout %u", ht->BTATimeout);
	}

	if (data->phy_timings) {
		LOG_DBG("HAL_DSI_ConfigPhyTimer:");
		LOG_DBG("  ClockLaneHS2LPTime %u", data->phy_timings->ClockLaneHS2LPTime);
		LOG_DBG("  ClockLaneLP2HSTime %u", data->phy_timings->ClockLaneLP2HSTime);
		LOG_DBG("  DataLaneHS2LPTime %u", data->phy_timings->DataLaneHS2LPTime);
		LOG_DBG("  DataLaneLP2HSTime %u", data->phy_timings->DataLaneLP2HSTime);
		LOG_DBG("  DataLaneMaxReadTime %u", data->phy_timings->DataLaneMaxReadTime);
		LOG_DBG("  StopWaitTime %u", data->phy_timings->StopWaitTime);
	}
}

static int mipi_dsi_stm32_host_init(const struct device *dev)
{
	const struct mipi_dsi_stm32_config *config = dev->config;
	struct mipi_dsi_stm32_data *data = dev->data;
	uint32_t hse_clock;
	int ret;

	switch (config->data_lanes) {
	case 1:
		data->hdsi.Init.NumberOfLanes = DSI_ONE_DATA_LANE;
		break;
	case 2:
		data->hdsi.Init.NumberOfLanes = DSI_TWO_DATA_LANES;
		break;
	default:
		LOG_ERR("Number of DSI lanes (%d) not supported!", config->data_lanes);
		return -ENOTSUP;
	}

	ret = clock_control_get_rate(config->rcc, (clock_control_subsys_t)&config->pix_clk,
				     &data->pixel_clk_khz);
	if (ret) {
		LOG_ERR("Get pixel clock failed! (%d)", ret);
		return ret;
	}

	data->pixel_clk_khz /= 1000;
	ret = clock_control_get_rate(config->rcc, (clock_control_subsys_t)&config->ref_clk,
				     &hse_clock);
	if (ret) {
		LOG_ERR("Get HSE clock failed! (%d)", ret);
		return ret;
	}

	/* LANE_BYTE_CLOCK = CLK_IN / PLLIDF * 2 * PLLNDIV / 2 / PLLODF / 8 */
	data->lane_clk_khz = hse_clock / data->pll_init.PLLIDF * 2 * data->pll_init.PLLNDIV / 2 /
			     (1UL << data->pll_init.PLLODF) / 8 / 1000;

	/* stm32x_hal_dsi: The values 0 and 1 stop the TX_ESC clock generation */
	data->hdsi.Init.TXEscapeCkdiv = 0;
	for (int i = 2; i <= MAX_TX_ESC_CLK_DIV; i++) {
		if ((data->lane_clk_khz / i) <= MAX_TX_ESC_CLK_KHZ) {
			data->hdsi.Init.TXEscapeCkdiv = i;
			break;
		}
	}

	if (data->hdsi.Init.TXEscapeCkdiv < 2) {
		LOG_WRN("DSI TX escape clock disabled.");
	}

	ret = HAL_DSI_Init(&data->hdsi, &data->pll_init);
	if (ret != HAL_OK) {
		LOG_ERR("DSI init failed! (%d)", ret);
		return -ret;
	}

	if (data->host_timeouts) {
		ret = HAL_DSI_ConfigHostTimeouts(&data->hdsi, data->host_timeouts);
		if (ret != HAL_OK) {
			LOG_ERR("Set DSI host timeouts failed! (%d)", ret);
			return -ret;
		}
	}

	if (data->phy_timings) {
		ret = HAL_DSI_ConfigPhyTimer(&data->hdsi, data->phy_timings);
		if (ret != HAL_OK) {
			LOG_ERR("Set DSI PHY timings failed! (%d)", ret);
			return -ret;
		}
	}

	ret = HAL_DSI_ConfigFlowControl(&data->hdsi, DSI_FLOW_CONTROL_BTA);
	if (ret != HAL_OK) {
		LOG_ERR("Setup DSI flow control failed! (%d)", ret);
		return -ret;
	}

	if (config->lp_rx_filter_freq) {
		ret = HAL_DSI_SetLowPowerRXFilter(&data->hdsi, config->lp_rx_filter_freq);
		if (ret != HAL_OK) {
			LOG_ERR("Setup DSI LP RX filter failed! (%d)", ret);
			return -ret;
		}
	}

	ret = HAL_DSI_ConfigErrorMonitor(&data->hdsi, config->active_errors);
	if (ret != HAL_OK) {
		LOG_ERR("Setup DSI error monitor failed! (%d)", ret);
		return -ret;
	}

	return 0;
}


static int mipi_dsi_stm32_attach(const struct device *dev, uint8_t channel,
				 const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_stm32_config *config = dev->config;
	struct mipi_dsi_stm32_data *data = dev->data;
	DSI_VidCfgTypeDef *vcfg = &data->vid_cfg;
	int ret;

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("DSI host supports video mode only!");
		return -ENOTSUP;
	}

	vcfg->VirtualChannelID = channel;
	vcfg->ColorCoding = STM32_DSI_INIT_PIXEL_FORMAT;

	if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) {
		vcfg->Mode = DSI_VID_MODE_BURST;
	} else if (mdev->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE) {
		vcfg->Mode = DSI_VID_MODE_NB_PULSES;
	} else {
		vcfg->Mode = DSI_VID_MODE_NB_EVENTS;
	}

	vcfg->PacketSize = mdev->timings.hactive;
	vcfg->NumberOfChunks = 0;
	vcfg->NullPacketSize = 0xFFFU;

	vcfg->HorizontalSyncActive =
		(mdev->timings.hsync * data->lane_clk_khz) / data->pixel_clk_khz;
	vcfg->HorizontalBackPorch =
		(mdev->timings.hbp * data->lane_clk_khz) / data->pixel_clk_khz;
	vcfg->HorizontalLine =
		((mdev->timings.hactive + mdev->timings.hsync + mdev->timings.hbp +
		  mdev->timings.hfp) * data->lane_clk_khz) / data->pixel_clk_khz;
	vcfg->VerticalSyncActive = mdev->timings.vsync;
	vcfg->VerticalBackPorch = mdev->timings.vbp;
	vcfg->VerticalFrontPorch = mdev->timings.vfp;
	vcfg->VerticalActive = mdev->timings.vactive;

	if (mdev->mode_flags & MIPI_DSI_MODE_LPM) {
		vcfg->LPCommandEnable = DSI_LP_COMMAND_ENABLE;
	} else {
		vcfg->LPCommandEnable = DSI_LP_COMMAND_DISABLE;
	}

	vcfg->LPHorizontalFrontPorchEnable = DSI_LP_HFP_ENABLE;
	vcfg->LPHorizontalBackPorchEnable = DSI_LP_HBP_ENABLE;
	vcfg->LPVerticalActiveEnable = DSI_LP_VACT_ENABLE;
	vcfg->LPVerticalFrontPorchEnable = DSI_LP_VFP_ENABLE;
	vcfg->LPVerticalBackPorchEnable = DSI_LP_VBP_ENABLE;
	vcfg->LPVerticalSyncActiveEnable = DSI_LP_VSYNC_ENABLE;

	ret = HAL_DSI_ConfigVideoMode(&data->hdsi, vcfg);
	if (ret != HAL_OK) {
		LOG_ERR("Setup DSI video mode failed! (%d)", ret);
		return -ret;
	}

	if (IS_ENABLED(CONFIG_MIPI_DSI_LOG_LEVEL_DBG)) {
		mipi_dsi_stm32_log_config(dev);
	}

	ret = HAL_DSI_Start(&data->hdsi);
	if (ret != HAL_OK) {
		LOG_ERR("Start DSI host failed! (%d)", ret);
		return -ret;
	}

	if (config->test_pattern >= 0) {
		ret = HAL_DSI_PatternGeneratorStart(&data->hdsi, 0, config->test_pattern);
		if (ret != HAL_OK) {
			LOG_ERR("Start DSI pattern generator failed! (%d)", ret);
			return -ret;
		}
	}

	return 0;
}

static ssize_t mipi_dsi_stm32_transfer(const struct device *dev, uint8_t channel,
				       struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_stm32_data *data = dev->data;
	uint32_t param1 = 0;
	uint32_t param2 = 0;
	ssize_t len;
	int ret;

	switch (msg->type) {
	case MIPI_DSI_DCS_READ:
		ret = HAL_DSI_Read(&data->hdsi, channel, (uint8_t *)msg->rx_buf, msg->rx_len,
				   msg->type, msg->cmd, (uint8_t *)msg->rx_buf);
		len =  msg->rx_len;
		break;
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		param1 = msg->cmd;
		if (msg->tx_len >= 1U) {
			param2 = ((uint8_t *)msg->tx_buf)[0];
		}

		ret = HAL_DSI_ShortWrite(&data->hdsi, channel, msg->type, param1, param2);
		len = msg->tx_len;
		break;
	case MIPI_DSI_DCS_LONG_WRITE:
		ret = HAL_DSI_LongWrite(&data->hdsi, channel, msg->type, msg->tx_len, msg->cmd,
					(uint8_t *)msg->tx_buf);
		len = msg->tx_len;
		break;
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		param1 = ((uint8_t *)msg->tx_buf)[0];
		if (msg->tx_len == 1U) {
			param2 = ((uint8_t *)msg->tx_buf)[1];
		}

		if (msg->tx_len >= 2U) {
			param2 = *(uint16_t *)&((uint8_t *)msg->tx_buf)[1];
		}

		ret = HAL_DSI_ShortWrite(&data->hdsi, channel, msg->type, param1, param2);
		len = msg->tx_len;
		break;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		ret = HAL_DSI_LongWrite(&data->hdsi, channel, msg->type, msg->tx_len,
					((uint8_t *)msg->tx_buf)[0], &((uint8_t *)msg->tx_buf)[1]);
		len = msg->tx_len;
		break;
	default:
		LOG_ERR("Unsupported message type (%d)", msg->type);
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_MIPI_DSI_LOG_LEVEL_DBG)) {
		char tmp[64];

		if (msg->type == MIPI_DSI_DCS_READ) {
			snprintk(tmp, sizeof(tmp), "RX: ch %3d, reg 0x%02x, len %2d",
				 channel, msg->cmd, msg->rx_len);
			LOG_HEXDUMP_DBG(msg->rx_buf, msg->rx_len, tmp);
		} else {
			snprintk(tmp, sizeof(tmp), "TX: ch %3d, reg 0x%02x, len %2d",
				 channel, msg->cmd, msg->tx_len);
			LOG_HEXDUMP_DBG(msg->tx_buf, msg->tx_len, tmp);
		}
	}

	if (ret != HAL_OK) {
		LOG_ERR("Transfer failed! (%d)", ret);
		return -EIO;
	}

	return len;
}

static struct mipi_dsi_driver_api dsi_stm32_api = {
	.attach = mipi_dsi_stm32_attach,
	.transfer = mipi_dsi_stm32_transfer,
};

static int mipi_dsi_stm32_init(const struct device *dev)
{
	const struct mipi_dsi_stm32_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->rcc)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->rcc, (clock_control_subsys_t)&config->dsi_clk);
	if (ret < 0) {
		LOG_ERR("Enable DSI peripheral clock failed! (%d)", ret);
		return ret;
	}

	(void)reset_line_toggle_dt(&config->reset);

	ret = mipi_dsi_stm32_host_init(dev);
	if (ret) {
		LOG_ERR("Setup DSI host failed! (%d)", ret);
		return ret;
	}

	return 0;
}

#define CHILD_GET_DATA_LANES(child) DT_PROP(child, data_lanes)

#define STM32_MIPI_DSI_DEVICE(inst)								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, host_timeouts),					\
		(static DSI_HOST_TimeoutTypeDef host_timeouts_##inst = {			\
			.TimeoutCkdiv = DT_INST_PROP_BY_IDX(inst, host_timeouts, 0),		\
			.HighSpeedTransmissionTimeout =						\
				DT_INST_PROP_BY_IDX(inst, host_timeouts, 1),			\
			.LowPowerReceptionTimeout =						\
				DT_INST_PROP_BY_IDX(inst, host_timeouts, 2),			\
			.HighSpeedReadTimeout = DT_INST_PROP_BY_IDX(inst, host_timeouts, 3),	\
			.LowPowerReadTimeout = DT_INST_PROP_BY_IDX(inst, host_timeouts, 4),	\
			.HighSpeedWriteTimeout = DT_INST_PROP_BY_IDX(inst, host_timeouts, 5),	\
			.HighSpeedWritePrespMode = DT_INST_PROP_BY_IDX(inst, host_timeouts, 6),	\
			.LowPowerWriteTimeout = DT_INST_PROP_BY_IDX(inst, host_timeouts, 7),	\
			.BTATimeout = DT_INST_PROP_BY_IDX(inst, host_timeouts, 8)		\
		}), ());									\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, phy_timings),					\
		(static DSI_PHY_TimerTypeDef phy_timings_##inst = {				\
			.ClockLaneHS2LPTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 0),	\
			.ClockLaneLP2HSTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 1),	\
			.DataLaneHS2LPTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 2),		\
			.DataLaneLP2HSTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 3),		\
			.DataLaneMaxReadTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 4),	\
			.StopWaitTime = DT_INST_PROP_BY_IDX(inst, phy_timings, 5)		\
		}), ());									\
	/* Only child data-lanes property at index 0 is taken into account */			\
	static const uint32_t data_lanes_##inst[] = {						\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(inst, DT_PROP_BY_IDX, (,),		\
							    data_lanes, 0)			\
	};											\
	static const struct mipi_dsi_stm32_config stm32_dsi_config_##inst = {			\
		.rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),					\
		.reset = RESET_DT_SPEC_INST_GET(inst),						\
		.dsi_clk = {									\
			.enr = DT_INST_CLOCKS_CELL_BY_NAME(inst, dsiclk, bits),			\
			.bus = DT_INST_CLOCKS_CELL_BY_NAME(inst, dsiclk, bus),			\
		},										\
		.ref_clk = {									\
			.enr = DT_INST_CLOCKS_CELL_BY_NAME(inst, refclk, bits),			\
			.bus = DT_INST_CLOCKS_CELL_BY_NAME(inst, refclk, bus),			\
		},										\
		.pix_clk = {									\
			.enr = DT_INST_CLOCKS_CELL_BY_NAME(inst, pixelclk, bits),		\
			.bus = DT_INST_CLOCKS_CELL_BY_NAME(inst, pixelclk, bus),		\
		},										\
		/* Use only one (the first) display configuration for DSI HOST configuration */	\
		.data_lanes = data_lanes_##inst[0],						\
		.active_errors = DT_INST_PROP_OR(inst, active_errors, HAL_DSI_ERROR_NONE),	\
		.lp_rx_filter_freq = DT_INST_PROP_OR(inst, lp_rx_filter, 0),			\
		.test_pattern = DT_INST_PROP_OR(inst, test_pattern, -1),			\
	};											\
	static struct mipi_dsi_stm32_data stm32_dsi_data_##inst = {				\
		.hdsi = {									\
			.Instance = (DSI_TypeDef *)DT_INST_REG_ADDR(inst),			\
			.Init = {								\
				.AutomaticClockLaneControl =					\
					DT_INST_PROP(inst, non_continuous) ?			\
						DSI_AUTO_CLK_LANE_CTRL_ENABLE :			\
						DSI_AUTO_CLK_LANE_CTRL_DISABLE,			\
			},									\
		},										\
		.host_timeouts = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, host_timeouts),	\
					     (&host_timeouts_##inst), (NULL)),			\
		.phy_timings = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, phy_timings),		\
					   (&phy_timings_##inst), (NULL)),			\
		.vid_cfg = {									\
			.HSPolarity = DT_INST_PROP(inst, hs_active_high) ?			\
				      DSI_HSYNC_ACTIVE_HIGH : DSI_HSYNC_ACTIVE_LOW,		\
			.VSPolarity = DT_INST_PROP(inst, vs_active_high) ?			\
				      DSI_VSYNC_ACTIVE_HIGH : DSI_VSYNC_ACTIVE_LOW,		\
			.DEPolarity = DT_INST_PROP(inst, de_active_high) ?			\
				      DSI_DATA_ENABLE_ACTIVE_HIGH : DSI_DATA_ENABLE_ACTIVE_LOW, \
			.LooselyPacked = DT_INST_PROP(inst, loosely_packed) ? \
				      DSI_LOOSELY_PACKED_ENABLE : DSI_LOOSELY_PACKED_DISABLE,	\
			.LPLargestPacketSize =  DT_INST_PROP_OR(inst, largest_packet_size, 4), \
			.LPVACTLargestPacketSize = DT_INST_PROP_OR(inst, largest_packet_size, 4), \
			.FrameBTAAcknowledgeEnable = DT_INST_PROP(inst, bta_ack_disable) ?	\
					  DSI_FBTAA_DISABLE : DSI_FBTAA_ENABLE,	\
		},										\
		.pll_init = {									\
			.PLLNDIV = DT_INST_PROP(inst, pll_ndiv),				\
			.PLLIDF = DT_INST_PROP(inst, pll_idf),					\
			.PLLODF = DT_INST_PROP(inst, pll_odf),					\
		},										\
	};											\
	DEVICE_DT_INST_DEFINE(inst, &mipi_dsi_stm32_init, NULL,					\
			      &stm32_dsi_data_##inst, &stm32_dsi_config_##inst,			\
			      POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY, &dsi_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_MIPI_DSI_DEVICE)
