/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_mipi_dsi

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/mipi_dsi.h>
#include "r_mipi_dsi.h"

LOG_MODULE_REGISTER(dsi_renesas_ra, CONFIG_MIPI_DSI_LOG_LEVEL);

struct mipi_dsi_renesas_ra_config {
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_dsi_subsys;
	void (*irq_configure)(void);
};

struct mipi_dsi_renesas_ra_data {
	mipi_dsi_instance_ctrl_t mipi_dsi_ctrl;
	mipi_dsi_cfg_t mipi_dsi_cfg;
	struct k_sem in_transmission;
	atomic_t status;
};

void mipi_dsi_seq0_isr(void);

void mipi_dsi_callback(mipi_dsi_callback_args_t *p_args)
{
	const struct device *dev = (struct device *)p_args->p_context;
	struct mipi_dsi_renesas_ra_data *data = dev->data;

	if (p_args->event == MIPI_DSI_EVENT_SEQUENCE_0) {
		atomic_set(&data->status, p_args->tx_status);
		k_sem_give(&data->in_transmission);
	}
}

static int mipi_dsi_renesas_ra_attach(const struct device *dev, uint8_t channel,
				      const struct mipi_dsi_device *mdev)
{
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	mipi_dsi_cfg_t cfg = data->mipi_dsi_cfg;
	fsp_err_t err;

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("DSI host supports video mode only!");
		return -ENOTSUP;
	}

	if (channel == 0 && (mdev->mode_flags & MIPI_DSI_MODE_LPM) == 0) {
		LOG_ERR("This channel support LP mode transfer only");
		return -ENOTSUP;
	}

	cfg.virtual_channel_id = channel;
	cfg.num_lanes = mdev->data_lanes;

	if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB888) {
		cfg.data_type = MIPI_DSI_VIDEO_DATA_24RGB_PIXEL_STREAM;
	} else if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB565) {
		cfg.data_type = MIPI_DSI_VIDEO_DATA_16RGB_PIXEL_STREAM;
	} else {
		return -ENOTSUP;
	}

	cfg.horizontal_active_lines = mdev->timings.hactive;
	cfg.horizontal_front_porch = mdev->timings.hfp;
	cfg.horizontal_back_porch = mdev->timings.hbp;
	cfg.horizontal_sync_lines = mdev->timings.hsync;

	cfg.vertical_active_lines = mdev->timings.vactive;
	cfg.vertical_front_porch = mdev->timings.vfp;
	cfg.vertical_back_porch = mdev->timings.vbp;
	cfg.vertical_sync_lines = mdev->timings.vsync;

	err = R_MIPI_DSI_Open(&data->mipi_dsi_ctrl, &cfg);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Open DSI failed (%d)", err);
		return -EIO;
	}

	err = R_MIPI_DSI_Start(&data->mipi_dsi_ctrl);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Start DSI host failed! (%d)", err);
		return -EIO;
	}

	return 0;
}

#define MIPI_DSI_SEQUENCE_STATUS_ERROR                                                             \
	(MIPI_DSI_SEQUENCE_STATUS_DESCRIPTOR_ABORT | MIPI_DSI_SEQUENCE_STATUS_SIZE_ERROR |         \
	 MIPI_DSI_SEQUENCE_STATUS_TX_INTERNAL_BUS_ERROR |                                          \
	 MIPI_DSI_SEQUENCE_STATUS_RX_FATAL_ERROR | MIPI_DSI_SEQUENCE_STATUS_RX_FAIL |              \
	 MIPI_DSI_SEQUENCE_STATUS_RX_PACKET_DATA_FAIL |                                            \
	 MIPI_DSI_SEQUENCE_STATUS_RX_CORRECTABLE_ERROR |                                           \
	 MIPI_DSI_SEQUENCE_STATUS_RX_ACK_AND_ERROR)

static ssize_t mipi_dsi_renesas_ra_dcs_write(const struct device *dev, uint8_t channel,
					     struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	uint8_t payload[msg->tx_len + 1];
	mipi_dsi_cmd_t fsp_msg = {.channel = channel,
				  .cmd_id = msg->type,
				  .p_tx_buffer = payload,
				  .tx_len = msg->tx_len + 1,
				  .flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) != 0
						   ? MIPI_DSI_CMD_FLAG_LOW_POWER
						   : 0};

	payload[0] = msg->cmd;
	memcpy(&payload[1], msg->tx_buf, msg->tx_len);

	atomic_clear(&data->status);
	k_sem_reset(&data->in_transmission);

	if (R_MIPI_DSI_Command(&data->mipi_dsi_ctrl, &fsp_msg) != FSP_SUCCESS) {
		LOG_ERR("DSI write fail");
		return -EIO;
	}

	k_sem_take(&data->in_transmission, K_FOREVER);

	if ((data->status & MIPI_DSI_SEQUENCE_STATUS_ERROR) != MIPI_DSI_SEQUENCE_STATUS_NONE) {
		return -EIO;
	}

	return (ssize_t)msg->tx_len;
}

static ssize_t mipi_dsi_renesas_ra_generic_write(const struct device *dev, uint8_t channel,
						 struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_renesas_ra_data *data = dev->data;
	mipi_dsi_cmd_t fsp_msg = {.channel = channel,
				  .cmd_id = msg->type,
				  .p_tx_buffer = msg->tx_buf,
				  .tx_len = msg->tx_len,
				  .flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) != 0
						   ? MIPI_DSI_CMD_FLAG_LOW_POWER
						   : 0};

	atomic_clear(&data->status);
	k_sem_reset(&data->in_transmission);

	if (R_MIPI_DSI_Command(&data->mipi_dsi_ctrl, &fsp_msg) != FSP_SUCCESS) {
		LOG_ERR("DSI write fail");
		return -EIO;
	}

	k_sem_take(&data->in_transmission, K_FOREVER);

	if ((data->status & MIPI_DSI_SEQUENCE_STATUS_ERROR) != MIPI_DSI_SEQUENCE_STATUS_NONE) {
		return -EIO;
	}

	return (ssize_t)msg->tx_len;
}

static ssize_t mipi_dsi_renesas_ra_transfer(const struct device *dev, uint8_t channel,
					    struct mipi_dsi_msg *msg)
{
	if (channel == 0 && (msg->flags & MIPI_DSI_MSG_USE_LPM) == 0) {
		LOG_ERR("This channel support LP mode transfer only");
		return -ENOTSUP;
	}

	switch (msg->type) {
	case MIPI_DSI_DCS_SHORT_WRITE:
		__fallthrough;
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
		__fallthrough;
	case MIPI_DSI_DCS_LONG_WRITE:
		return mipi_dsi_renesas_ra_dcs_write(dev, channel, msg);
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		return mipi_dsi_renesas_ra_generic_write(dev, channel, msg);
	default:
		return -ENOTSUP;
	}

	return 0;
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
	if (ret != 0) {
		LOG_ERR("Enable DSI peripheral clock failed! (%d)", ret);
		return ret;
	}

	k_sem_init(&data->in_transmission, 0, 1);

	config->irq_configure();

	return 0;
}

#define RENESAS_RA_MIPI_PHYS_SETTING_DEFINE(n)                                                     \
	static const mipi_phy_timing_t mipi_phy_##n##_timing = {                                   \
		.t_init = CLAMP(DT_PROP(DT_INST_CHILD(n, phys_timing), t_init), 0, 0x7FFF),        \
		.t_clk_prep = CLAMP(DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_prep), 0, 0xFF),  \
		.t_hs_prep = CLAMP(DT_PROP(DT_INST_CHILD(n, phys_timing), t_hs_prep), 0, 0xFF),    \
		.t_lp_exit = CLAMP(DT_PROP(DT_INST_CHILD(n, phys_timing), t_lp_exit), 0, 0xFF),    \
		.dphytim4_b =                                                                      \
			{                                                                          \
				.t_clk_zero = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),        \
							     dphytim4, 0),                         \
				.t_clk_pre = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),         \
							    dphytim4, 1),                          \
				.t_clk_post = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),        \
							     dphytim4, 2),                         \
				.t_clk_trail = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),       \
							      dphytim4, 3),                        \
			},                                                                         \
		.dphytim5_b =                                                                      \
			{                                                                          \
				.t_hs_zero = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),         \
							    dphytim5, 0),                          \
				.t_hs_trail = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),        \
							     dphytim5, 1),                         \
				.t_hs_exit = DT_PROP_BY_IDX(DT_INST_CHILD(n, phys_timing),         \
							    dphytim5, 2),                          \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const mipi_phy_cfg_t mipi_phy_##n##_cfg = {                                         \
		.pll_settings =                                                                    \
			{                                                                          \
				.div = DT_INST_PROP(n, pll_div) - 1,                               \
				.mul_frac = DT_INST_ENUM_IDX(n, pll_mul_frac),                     \
				.mul_int = CLAMP(DT_INST_PROP(n, pll_mul_int), 20, 180) - 1,       \
			},                                                                         \
		.lp_divisor = CLAMP(DT_INST_PROP(n, lp_divisor), 1, 32) - 1,                       \
		.p_timing = &mipi_phy_##n##_timing,                                                \
	};                                                                                         \
                                                                                                   \
	mipi_phy_ctrl_t mipi_phy_##n##_ctrl;                                                       \
                                                                                                   \
	static const mipi_phy_instance_t mipi_phy##n = {                                           \
		.p_ctrl = &mipi_phy_##n##_ctrl,                                                    \
		.p_cfg = &mipi_phy_##n##_cfg,                                                      \
		.p_api = &g_mipi_phy,                                                              \
	};

#define RENESAS_RA_MIPI_DSI_PHYS_GET(n) &mipi_phy##n

#define RENESAS_RA_MIPI_DSI_TIMING_DEFINE(n)                                                       \
	static const mipi_dsi_timing_t mipi_dsi_##n##_timing = {                                   \
		.clock_stop_time = DT_INST_PROP_BY_IDX(n, timing, 0),                              \
		.clock_beforehand_time = DT_INST_PROP_BY_IDX(n, timing, 1),                        \
		.clock_keep_time = DT_INST_PROP_BY_IDX(n, timing, 2),                              \
		.go_lp_and_back = DT_INST_PROP_BY_IDX(n, timing, 3),                               \
	};

#define RENESAS_RA_MIPI_DSI_TIMING_GET(n) &mipi_dsi_##n##_timing

#define RENESAS_MIPI_DSI_DEVICE(id)                                                                \
	static void mipi_dsi_ra_configure_func_##id(void)                                          \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(id, sq0, irq)] =                                  \
			BSP_PRV_IELS_ENUM(EVENT_MIPIDSI_SEQ0);                                     \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, sq0, irq),                                     \
			    DT_INST_IRQ_BY_NAME(id, sq0, priority), mipi_dsi_seq0_isr, NULL, 0);   \
		irq_enable(DT_INST_IRQ_BY_NAME(id, sq0, irq));                                     \
	}                                                                                          \
                                                                                                   \
	RENESAS_RA_MIPI_DSI_TIMING_DEFINE(id)                                                      \
	RENESAS_RA_MIPI_PHYS_SETTING_DEFINE(id)                                                    \
                                                                                                   \
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
			    R_DSILINK_RXIER_ECCERRS_Msk | R_DSILINK_RXIER_RXAKE_Msk,               \
		.dsi_ferrie = R_DSILINK_FERRIER_HTXTO_Msk | R_DSILINK_FERRIER_LRXHTO_Msk |         \
			      R_DSILINK_FERRIER_TATO_Msk | R_DSILINK_FERRIER_ESCENT_Msk |          \
			      R_DSILINK_FERRIER_SYNCESC_Msk | R_DSILINK_FERRIER_CTRL_Msk |         \
			      R_DSILINK_FERRIER_CLP0_Msk | R_DSILINK_FERRIER_CLP1_Msk,             \
		.dsi_plie = R_DSILINK_PLIER_DLULPENT_Msk | R_DSILINK_PLIER_DLULPEXT_Msk,           \
		.dsi_vmie = R_DSILINK_VMIER_VBUFUDF_Msk | R_DSILINK_VMIER_VBUFOVF_Msk,             \
		.dsi_sqch0ie = R_DSILINK_SQCH0IER_AACTFIN_Msk | R_DSILINK_SQCH0IER_ADESFIN_Msk |   \
			       R_DSILINK_SQCH0IER_TXIBERR_Msk | R_DSILINK_SQCH0IER_RXFERR_Msk |    \
			       R_DSILINK_SQCH0IER_RXFAIL_Msk | R_DSILINK_SQCH0IER_RXPFAIL_Msk |    \
			       R_DSILINK_SQCH0IER_RXCORERR_Msk | R_DSILINK_SQCH0IER_RXAKE_Msk,     \
		.dsi_sqch1ie = R_DSILINK_SQCH1IER_AACTFIN_Msk | R_DSILINK_SQCH1IER_ADESFIN_Msk |   \
			       R_DSILINK_SQCH1IER_SIZEERR_Msk | R_DSILINK_SQCH1IER_TXIBERR_Msk |   \
			       R_DSILINK_SQCH1IER_RXFERR_Msk | R_DSILINK_SQCH1IER_RXFAIL_Msk |     \
			       R_DSILINK_SQCH1IER_RXPFAIL_Msk | R_DSILINK_SQCH1IER_RXCORERR_Msk |  \
			       R_DSILINK_SQCH1IER_RXAKE_Msk,                                       \
	};                                                                                         \
                                                                                                   \
	static const struct mipi_dsi_renesas_ra_config ra_config_##id = {                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),                               \
		.clock_dsi_subsys =                                                                \
			{                                                                          \
				.mstp = DT_INST_CLOCKS_CELL(id, mstp),                             \
				.stop_bit = DT_INST_CLOCKS_CELL(id, stop_bit),                     \
			},                                                                         \
		.irq_configure = mipi_dsi_ra_configure_func_##id,                                  \
	};                                                                                         \
                                                                                                   \
	static struct mipi_dsi_renesas_ra_data ra_data_##id = {                                    \
		.mipi_dsi_cfg =                                                                    \
			{                                                                          \
				.p_mipi_phy_instance = RENESAS_RA_MIPI_DSI_PHYS_GET(id),           \
				.p_timing = RENESAS_RA_MIPI_DSI_TIMING_GET(id),                    \
				.sync_pulse = (0),                                                 \
				.vertical_sync_polarity = 1,                                       \
				.horizontal_sync_polarity = 1,                                     \
				.video_mode_delay = DT_INST_PROP(id, video_mode_delay),            \
				.hsa_no_lp = R_DSILINK_VMSET0R_HSANOLP_Msk,                        \
				.hbp_no_lp = R_DSILINK_VMSET0R_HBPNOLP_Msk,                        \
				.hfp_no_lp = R_DSILINK_VMSET0R_HFPNOLP_Msk,                        \
				.ulps_wakeup_period = DT_INST_PROP(id, ulps_wakeup_period),        \
				.continuous_clock = (1),                                           \
				.hs_tx_timeout = 0,                                                \
				.lp_rx_timeout = 0,                                                \
				.turnaround_timeout = 0,                                           \
				.bta_timeout = 0,                                                  \
				.lprw_timeout = 0,                                                 \
				.hsrw_timeout = 0,                                                 \
				.max_return_packet_size = 1,                                       \
				.ecc_enable = (1),                                                 \
				.crc_check_mask = (mipi_dsi_vc_t)(0x0),                            \
				.scramble_enable = (0),                                            \
				.tearing_detect = (0),                                             \
				.eotp_enable = (1),                                                \
				.p_extend = &mipi_dsi_##id##_extended_cfg,                         \
				.p_callback = mipi_dsi_callback,                                   \
				.p_context = DEVICE_DT_INST_GET(id),                               \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &mipi_dsi_renesas_ra_init, NULL, &ra_data_##id, &ra_config_##id, \
			      POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY, &mipi_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_MIPI_DSI_DEVICE)
