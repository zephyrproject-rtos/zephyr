/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_mipi_dsi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include "r_mipi_dsi.h"
#include "r_mipi_phy.h"

LOG_MODULE_REGISTER(dsi_renesas_ra, CONFIG_MIPI_DSI_LOG_LEVEL);

/* MIPI PHY Macros */
#define MIPI_PHY_CLKSTPT (1183)
#define MIPI_PHY_CLKBFHT (11)
#define MIPI_PHY_CLKKPT  (26)
#define MIPI_PHY_GOLPBKT (40)

#define MIPI_PHY_TINIT     (71999)
#define MIPI_PHY_TCLKPREP  (8)
#define MIPI_PHY_THSPREP   (5)
#define MIPI_PHY_TCLKTRAIL (7)
#define MIPI_PHY_TCLKPOST  (19)
#define MIPI_PHY_TCLKPRE   (1)
#define MIPI_PHY_TCLKZERO  (27)
#define MIPI_PHY_THSEXIT   (11)
#define MIPI_PHY_THSTRAIL  (8)
#define MIPI_PHY_THSZERO   (19)
#define MIPI_PHY_TLPEXIT   (7)
#define LP_DIVISOR         (4)
#define PLL_MUL_SETTING    (49)
#define VIDEO_MODE_DELAY   (186)
#define ULPS_WAKEUP_PERIOD (97)
struct mipi_dsi_renesas_ra_config {
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_dsi_subsys;
	void (*irq_configure)(void);
};

struct mipi_dsi_renesas_ra_data {
	mipi_dsi_instance_ctrl_t mipi_dsi_ctrl;
	mipi_dsi_cfg_t mipi_dsi_cfg;
	volatile bool message_sent;
	volatile bool fatal_error;
};

void mipi_dsi_seq0(void);
void mipi_dsi_ferr(void);
void mipi_dsi_callback(mipi_dsi_callback_args_t *p_args);

typedef struct {
	unsigned char size;
	unsigned char buffer[256];
	mipi_dsi_cmd_id_t cmd_id;
	mipi_dsi_cmd_flag_t flags;
} lcd_table_setting_t;

void mipi_dsi_callback(mipi_dsi_callback_args_t *p_args)
{
	const struct device *dev = (struct device *)p_args->p_context;
	struct mipi_dsi_renesas_ra_data *data = dev->data;

	switch (p_args->event) {
	case MIPI_DSI_EVENT_SEQUENCE_0: {
		if (MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED == p_args->tx_status) {
			data->message_sent = true;
		}
		break;
	}
	case MIPI_DSI_EVENT_FATAL: {
		data->fatal_error = true;
		break;
	}
	default: {
		break;
	}
	}
}

static int mipi_dsi_renesas_ra_attach(const struct device *dev, uint8_t channel,
				      const struct mipi_dsi_device *mdev)
{
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	mipi_dsi_cfg_t cfg = data->mipi_dsi_cfg;
	int ret;

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("DSI host supports video mode only!");
		return -ENOTSUP;
	}
	cfg.virtual_channel_id = channel;
	cfg.num_lanes = mdev->data_lanes;
	if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB888) {
		cfg.data_type = MIPI_DSI_VIDEO_DATA_24RGB_PIXEL_STREAM;
	} else if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB565) {
		cfg.data_type = MIPI_DSI_VIDEO_DATA_16RGB_PIXEL_STREAM;
	}
	cfg.horizontal_active_lines = mdev->timings.hactive;
	cfg.horizontal_front_porch = mdev->timings.hfp;
	cfg.horizontal_back_porch = mdev->timings.hbp;
	cfg.horizontal_sync_lines = mdev->timings.hsync;

	cfg.vertical_active_lines = mdev->timings.vactive;
	cfg.vertical_front_porch = mdev->timings.vfp;
	cfg.vertical_back_porch = mdev->timings.vbp;
	cfg.vertical_sync_lines = mdev->timings.vsync;

	ret = R_MIPI_DSI_Open(&data->mipi_dsi_ctrl, &cfg);
	if (ret) {
		LOG_ERR("Open DSI failed (%d)", ret);
		return -EIO;
	}

	ret = R_MIPI_DSI_Start(&data->mipi_dsi_ctrl);
	if (ret) {
		LOG_ERR("Start DSI host failed! (%d)", ret);
		return -EIO;
	}

	return 0;
}

static ssize_t mipi_dsi_renesas_ra_transfer(const struct device *dev, uint8_t channel,
					    struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	ssize_t len;
	int ret;
	uint8_t combined_tx_buffer[msg->tx_len + 1];

	combined_tx_buffer[0] = msg->cmd;
	memcpy(&combined_tx_buffer[1], msg->tx_buf, msg->tx_len);

	mipi_dsi_cmd_t fsp_msg = {
		.channel = channel,
		.cmd_id = msg->type,
		.flags = MIPI_DSI_CMD_FLAG_LOW_POWER,
		.tx_len = msg->tx_len + 1,
		.p_tx_buffer = combined_tx_buffer,
	};
	data->message_sent = false;
	data->fatal_error = false;

	switch (msg->type) {
	case MIPI_DSI_DCS_READ:
		LOG_ERR("DCS Read not yet implemented or used");
		return -ENOTSUP;
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_DCS_LONG_WRITE:
		ret = R_MIPI_DSI_Command(&data->mipi_dsi_ctrl, &fsp_msg);
		if (ret) {
			LOG_ERR("DSI write fail: err: (%d)", ret);
			return -EIO;
		}
		while (!(data->message_sent)) {
			if (data->fatal_error) {
				LOG_ERR("fatal error");
				return -EIO;
			}
		}
		len = msg->tx_len;
		break;
	default:
		LOG_ERR("Unsupported message type (%d)", msg->type);
		return -ENOTSUP;
	}

	return len;
}

static DEVICE_API(mipi_dsi, mipi_dsi_api) = {
	.attach = mipi_dsi_renesas_ra_attach,
	.transfer = mipi_dsi_renesas_ra_transfer,
};

static int mipi_dsi_renesas_ra_init(const struct device *dev)
{
	const struct mipi_dsi_renesas_ra_config *config = dev->config;
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)&config->clock_dsi_subsys);
	if (ret) {
		LOG_ERR("Enable DSI peripheral clock failed! (%d)", ret);
		return ret;
	}

	config->irq_configure();
	data->mipi_dsi_cfg.p_context = dev;

	return 0;
}

#define IRQ_CONFIGURE_FUNC(id)                                                                     \
	static void mipi_dsi_ra_configure_func_##id(void)                                          \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(id, sq0, irq)] =                                  \
			BSP_PRV_IELS_ENUM(EVENT_MIPIDSI_SEQ0);                                     \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, sq0, irq),                                     \
			    DT_INST_IRQ_BY_NAME(id, sq0, priority), mipi_dsi_seq0,                 \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(id, sq0, irq));                                     \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(id, ferr, irq)] =                                 \
			BSP_PRV_IELS_ENUM(EVENT_MIPIDSI_FERR);                                     \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, ferr, irq),                                    \
			    DT_INST_IRQ_BY_NAME(id, ferr, priority), mipi_dsi_ferr,                \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQ_BY_NAME(id, ferr, irq));                                    \
	}

#define IRQ_CONFIGURE_DEFINE(id) .irq_configure = mipi_dsi_ra_configure_func_##id

#define RENESAS_MIPI_DSI_DEVICE(id)                                                                \
	IRQ_CONFIGURE_FUNC(id)                                                                     \
	mipi_phy_ctrl_t mipi_phy_##id##_ctrl;                                                      \
	static const mipi_phy_timing_t mipi_phy_##id##_timing = {                                  \
		.t_init = 0x3FFFF & (uint32_t)MIPI_PHY_TINIT,                                      \
		.t_clk_prep = (uint8_t)MIPI_PHY_TCLKPREP,                                          \
		.t_hs_prep = (uint8_t)MIPI_PHY_THSPREP,                                            \
		.dphytim4_b.t_clk_trail = (uint32_t)MIPI_PHY_TCLKTRAIL,                            \
		.dphytim4_b.t_clk_post = (uint32_t)MIPI_PHY_TCLKPOST,                              \
		.dphytim4_b.t_clk_pre = (uint32_t)MIPI_PHY_TCLKPRE,                                \
		.dphytim4_b.t_clk_zero = (uint32_t)MIPI_PHY_TCLKZERO,                              \
		.dphytim5_b.t_hs_exit = (uint32_t)MIPI_PHY_THSEXIT,                                \
		.dphytim5_b.t_hs_trail = (uint32_t)MIPI_PHY_THSTRAIL,                              \
		.dphytim5_b.t_hs_zero = (uint32_t)MIPI_PHY_THSZERO,                                \
		.t_lp_exit = (uint32_t)MIPI_PHY_TLPEXIT,                                           \
	};                                                                                         \
	static const mipi_phy_cfg_t mipi_phy_##id##_cfg = {                                        \
		.pll_settings = {.div = 0, .mul_int = PLL_MUL_SETTING, .mul_frac = 0},             \
		.lp_divisor = LP_DIVISOR,                                                          \
		.p_timing = &mipi_phy_##id##_timing,                                               \
	};                                                                                         \
	static const mipi_phy_instance_t mipi_phy##id = {                                          \
		.p_ctrl = &mipi_phy_##id##_ctrl,                                                   \
		.p_cfg = &mipi_phy_##id##_cfg,                                                     \
		.p_api = &g_mipi_phy,                                                              \
	};                                                                                         \
	static const mipi_dsi_extended_cfg_t mipi_dsi_##id##_extended_cfg = {                      \
		.dsi_seq0.ipl = DT_INST_IRQ_BY_NAME(id, sq0, priority),                            \
		.dsi_seq0.irq = DT_INST_IRQ_BY_NAME(id, sq0, irq),                                 \
		.dsi_seq1.ipl = DT_INST_IRQ_BY_NAME(id, sq1, priority),                            \
		.dsi_seq1.irq = DT_INST_IRQ_BY_NAME(id, sq1, irq),                                 \
		.dsi_vin1.ipl = DT_INST_IRQ_BY_NAME(id, vm, priority),                             \
		.dsi_vin1.irq = DT_INST_IRQ_BY_NAME(id, vm, irq),                                  \
		.dsi_rcv.ipl = DT_INST_IRQ_BY_NAME(id, rcv, priority),                             \
		.dsi_rcv.irq = DT_INST_IRQ_BY_NAME(id, rcv, irq),                                  \
		.dsi_ferr.ipl = DT_INST_IRQ_BY_NAME(id, ferr, priority),                           \
		.dsi_ferr.irq = DT_INST_IRQ_BY_NAME(id, ferr, irq),                                \
		.dsi_ppi.ipl = DT_INST_IRQ_BY_NAME(id, ppi, priority),                             \
		.dsi_ppi.irq = DT_INST_IRQ_BY_NAME(id, ppi, irq),                                  \
		.dsi_rxie = R_DSILINK_RXIER_BTAREND_Msk | R_DSILINK_RXIER_LRXHTO_Msk |             \
			    R_DSILINK_RXIER_TATO_Msk | R_DSILINK_RXIER_RXRESP_Msk |                \
			    R_DSILINK_RXIER_RXEOTP_Msk | R_DSILINK_RXIER_RXTE_Msk |                \
			    R_DSILINK_RXIER_RXACK_Msk | R_DSILINK_RXIER_EXTEDET_Msk |              \
			    R_DSILINK_RXIER_MLFERR_Msk | R_DSILINK_RXIER_ECCERRM_Msk |             \
			    R_DSILINK_RXIER_UNEXERR_Msk | R_DSILINK_RXIER_WCERR_Msk |              \
			    R_DSILINK_RXIER_CRCERR_Msk | R_DSILINK_RXIER_IBERR_Msk |               \
			    R_DSILINK_RXIER_RXOVFERR_Msk | R_DSILINK_RXIER_PRTOERR_Msk |           \
			    R_DSILINK_RXIER_NORESERR_Msk | R_DSILINK_RXIER_RSIZEERR_Msk |          \
			    R_DSILINK_RXIER_ECCERRS_Msk | R_DSILINK_RXIER_RXAKE_Msk | 0x0,         \
		.dsi_ferrie = R_DSILINK_FERRIER_HTXTO_Msk | R_DSILINK_FERRIER_LRXHTO_Msk |         \
			      R_DSILINK_FERRIER_TATO_Msk | R_DSILINK_FERRIER_ESCENT_Msk |          \
			      R_DSILINK_FERRIER_SYNCESC_Msk | R_DSILINK_FERRIER_CTRL_Msk |         \
			      R_DSILINK_FERRIER_CLP0_Msk | R_DSILINK_FERRIER_CLP1_Msk | 0x0,       \
		.dsi_plie = R_DSILINK_PLIER_DLULPENT_Msk | R_DSILINK_PLIER_DLULPEXT_Msk | 0x0,     \
		.dsi_vmie = R_DSILINK_VMIER_VBUFUDF_Msk | R_DSILINK_VMIER_VBUFOVF_Msk | 0x0,       \
		.dsi_sqch0ie = R_DSILINK_SQCH0IER_AACTFIN_Msk | R_DSILINK_SQCH0IER_ADESFIN_Msk |   \
			       R_DSILINK_SQCH0IER_TXIBERR_Msk | R_DSILINK_SQCH0IER_RXFERR_Msk |    \
			       R_DSILINK_SQCH0IER_RXFAIL_Msk | R_DSILINK_SQCH0IER_RXPFAIL_Msk |    \
			       R_DSILINK_SQCH0IER_RXCORERR_Msk | R_DSILINK_SQCH0IER_RXAKE_Msk |    \
			       0x0,                                                                \
		.dsi_sqch1ie = R_DSILINK_SQCH1IER_AACTFIN_Msk | R_DSILINK_SQCH1IER_ADESFIN_Msk |   \
			       R_DSILINK_SQCH1IER_SIZEERR_Msk | R_DSILINK_SQCH1IER_TXIBERR_Msk |   \
			       R_DSILINK_SQCH1IER_RXFERR_Msk | R_DSILINK_SQCH1IER_RXFAIL_Msk |     \
			       R_DSILINK_SQCH1IER_RXPFAIL_Msk | R_DSILINK_SQCH1IER_RXCORERR_Msk |  \
			       R_DSILINK_SQCH1IER_RXAKE_Msk | 0x0,                                 \
	};                                                                                         \
	static const mipi_dsi_timing_t mipi_dsi_##id##_timing = {                                  \
		.clock_stop_time = MIPI_PHY_CLKSTPT,                                               \
		.clock_beforehand_time = MIPI_PHY_CLKBFHT,                                         \
		.clock_keep_time = MIPI_PHY_CLKKPT,                                                \
		.go_lp_and_back = MIPI_PHY_GOLPBKT,                                                \
	};                                                                                         \
	static const struct mipi_dsi_renesas_ra_config ra_config_##id = {                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		IRQ_CONFIGURE_DEFINE(id),                                                          \
		.clock_dsi_subsys = {.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_IDX(id, 0, mstp),    \
				     .stop_bit = DT_INST_CLOCKS_CELL_BY_IDX(id, 0, stop_bit)}};    \
	static struct mipi_dsi_renesas_ra_data ra_data_##id = {                                    \
		.mipi_dsi_cfg =                                                                    \
			{                                                                          \
				.p_mipi_phy_instance = &mipi_phy##id,                              \
				.p_timing = &mipi_dsi_##id##_timing,                               \
				.sync_pulse = (0),                                                 \
				.data_type = MIPI_DSI_PACKED_PIXEL_STREAM_24,                      \
				.vertical_sync_polarity = 1,                                       \
				.horizontal_sync_polarity = 1,                                     \
				.video_mode_delay = VIDEO_MODE_DELAY,                              \
				.hsa_no_lp = ((0x0) & R_DSILINK_VMSET0R_HSANOLP_Msk),              \
				.hbp_no_lp = ((0x0) & R_DSILINK_VMSET0R_HBPNOLP_Msk),              \
				.hfp_no_lp = ((0x0) & R_DSILINK_VMSET0R_HFPNOLP_Msk),              \
				.num_lanes =                                                       \
					DT_PROP_BY_IDX(DT_NODELABEL(ili9806e), data_lanes, 0),     \
				.ulps_wakeup_period = ULPS_WAKEUP_PERIOD,                          \
				.continuous_clock = (1),                                           \
				.hs_tx_timeout = 0,                                                \
				.lp_rx_timeout = 0,                                                \
				.turnaround_timeout = 0,                                           \
				.bta_timeout = 0,                                                  \
				.lprw_timeout = (0 << R_DSILINK_PRESPTOLPSETR_LPRTO_Pos) | 0,      \
				.hsrw_timeout = (0 << R_DSILINK_PRESPTOHSSETR_HSRTO_Pos) | 0,      \
				.max_return_packet_size = 1,                                       \
				.ecc_enable = (1),                                                 \
				.crc_check_mask = (mipi_dsi_vc_t)(0x0),                            \
				.scramble_enable = (0),                                            \
				.tearing_detect = (0),                                             \
				.eotp_enable = (1),                                                \
				.p_extend = &mipi_dsi_##id##_extended_cfg,                         \
				.p_callback = mipi_dsi_callback,                                   \
				.p_context = NULL,                                                 \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, &mipi_dsi_renesas_ra_init, NULL, &ra_data_##id, &ra_config_##id, \
			      POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY, &mipi_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_MIPI_DSI_DEVICE)
