/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_mipi_dsi

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/mipi_dsi.h>
#include "r_mipi_dsi_b.h"
#include "r_display_api.h"

LOG_MODULE_REGISTER(dsi_renesas_rz, CONFIG_MIPI_DSI_LOG_LEVEL);

struct mipi_dsi_renesas_rz_config {
	void (*irq_configure)(void);
	const mipi_dsi_api_t *fsp_api;
};

struct mipi_dsi_renesas_rz_data {
	mipi_dsi_ctrl_t *fsp_ctrl;
	mipi_dsi_cfg_t *fsp_cfg;
	struct k_sem in_transmission;
	atomic_t status;
};

extern void mipi_dsi_seq0(void *irq);

static void renesas_rz_mipi_dsi_isr(const struct device *dev)
{
	struct mipi_dsi_renesas_rz_data *data = dev->data;
	const mipi_dsi_b_extended_cfg_t *ext_cfg = data->fsp_cfg->p_extend;

	mipi_dsi_seq0((void *)ext_cfg->dsi_seq0.irq);
}

void mipi_dsi_callback(mipi_dsi_callback_args_t *p_args)
{
	const struct device *dev = (struct device *)p_args->p_context;
	struct mipi_dsi_renesas_rz_data *data = dev->data;

	if (p_args->event == MIPI_DSI_EVENT_SEQUENCE_0) {
		atomic_set(&data->status, p_args->tx_status);
		k_sem_give(&data->in_transmission);
	}
}

static int mipi_dsi_renesas_rz_attach(const struct device *dev, uint8_t channel,
				      const struct mipi_dsi_device *mdev)
{
	struct mipi_dsi_renesas_rz_data *data = dev->data;
	const struct mipi_dsi_renesas_rz_config *config = dev->config;
	mipi_dsi_cfg_t *cfg = data->fsp_cfg;
	fsp_err_t err;

	if (!(mdev->mode_flags & MIPI_DSI_MODE_VIDEO)) {
		LOG_ERR("DSI host supports video mode only!");
		return -ENOTSUP;
	}

	if (channel == 0 && (mdev->mode_flags & MIPI_DSI_MODE_LPM) == 0) {
		LOG_ERR("This channel support LP mode transfer only");
		return -ENOTSUP;
	}

	cfg->virtual_channel_id = channel;
	cfg->num_lanes = mdev->data_lanes;

	if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB888) {
		cfg->data_type = MIPI_DSI_VIDEO_DATA_24RGB_PIXEL_STREAM;
	} else if (mdev->pixfmt == MIPI_DSI_PIXFMT_RGB565) {
		cfg->data_type = MIPI_DSI_VIDEO_DATA_16RGB_PIXEL_STREAM;
	} else {
		return -ENOTSUP;
	}

	cfg->horizontal_active_lines = mdev->timings.hactive;
	cfg->horizontal_front_porch = mdev->timings.hfp;
	cfg->horizontal_back_porch = mdev->timings.hbp;
	cfg->horizontal_sync_lines = mdev->timings.hsync;

	cfg->vertical_active_lines = mdev->timings.vactive;
	cfg->vertical_front_porch = mdev->timings.vfp;
	cfg->vertical_back_porch = mdev->timings.vbp;
	cfg->vertical_sync_lines = mdev->timings.vsync;

	err = config->fsp_api->open(data->fsp_ctrl, cfg);
	if (err != FSP_SUCCESS) {
		LOG_ERR("Open DSI failed (%d)", err);
		return -EIO;
	}

	err = config->fsp_api->start(data->fsp_ctrl);
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

static ssize_t mipi_dsi_renesas_rz_dcs_write(const struct device *dev, uint8_t channel,
					     struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_renesas_rz_data *data = dev->data;
	const struct mipi_dsi_renesas_rz_config *config = dev->config;
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

	if (config->fsp_api->command(data->fsp_ctrl, &fsp_msg) != FSP_SUCCESS) {
		LOG_ERR("DSI write fail");
		return -EIO;
	}

	k_sem_take(&data->in_transmission, K_FOREVER);

	if ((data->status & MIPI_DSI_SEQUENCE_STATUS_ERROR) != MIPI_DSI_SEQUENCE_STATUS_NONE) {
		return -EIO;
	}

	return (ssize_t)msg->tx_len;
}

static ssize_t mipi_dsi_renesas_rz_generic_write(const struct device *dev, uint8_t channel,
						 struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_renesas_rz_data *data = dev->data;
	const struct mipi_dsi_renesas_rz_config *config = dev->config;
	mipi_dsi_cmd_t fsp_msg = {.channel = channel,
				  .cmd_id = msg->type,
				  .p_tx_buffer = msg->tx_buf,
				  .tx_len = msg->tx_len,
				  .flags = (msg->flags & MIPI_DSI_MSG_USE_LPM) != 0
						   ? MIPI_DSI_CMD_FLAG_LOW_POWER
						   : 0};

	atomic_clear(&data->status);
	k_sem_reset(&data->in_transmission);

	if (config->fsp_api->command(data->fsp_ctrl, &fsp_msg) != FSP_SUCCESS) {
		LOG_ERR("DSI write fail");
		return -EIO;
	}

	k_sem_take(&data->in_transmission, K_FOREVER);

	if ((data->status & MIPI_DSI_SEQUENCE_STATUS_ERROR) != MIPI_DSI_SEQUENCE_STATUS_NONE) {
		return -EIO;
	}

	return (ssize_t)msg->tx_len;
}

static ssize_t mipi_dsi_renesas_rz_transfer(const struct device *dev, uint8_t channel,
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
		return mipi_dsi_renesas_rz_dcs_write(dev, channel, msg);
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
		__fallthrough;
	case MIPI_DSI_GENERIC_LONG_WRITE:
		return mipi_dsi_renesas_rz_generic_write(dev, channel, msg);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(mipi_dsi, mipi_dsi_api) = {
	.attach = mipi_dsi_renesas_rz_attach,
	.transfer = mipi_dsi_renesas_rz_transfer,
};

static int mipi_dsi_renesas_rz_init(const struct device *dev)
{
	const struct mipi_dsi_renesas_rz_config *config = dev->config;
	struct mipi_dsi_renesas_rz_data *data = dev->data;

	k_sem_init(&data->in_transmission, 0, 1);

	config->irq_configure();

	return 0;
}

#define RENESAS_RZ_MIPI_PHYS_SETTING_DEFINE(n)                                                     \
	static const mipi_phy_b_timing_t mipi_phy##n##_timing = {                                  \
		.t_init = DT_PROP(DT_INST_CHILD(n, phys_timing), t_init),                          \
		.t_clk_prep = DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_prep),                  \
		.t_hs_prep = DT_PROP(DT_INST_CHILD(n, phys_timing), t_hs_prep),                    \
		.t_clk_zero = DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_zero),                  \
		.t_clk_pre = DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_pre),                    \
		.t_clk_post = DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_post),                  \
		.t_clk_trail = DT_PROP(DT_INST_CHILD(n, phys_timing), t_clk_trail),                \
		.t_hs_zero = DT_PROP(DT_INST_CHILD(n, phys_timing), t_hs_zero),                    \
		.t_hs_trail = DT_PROP(DT_INST_CHILD(n, phys_timing), t_hs_trail),                  \
		.t_hs_exit = DT_PROP(DT_INST_CHILD(n, phys_timing), t_hs_exit),                    \
		.t_lp_exit = DT_PROP(DT_INST_CHILD(n, phys_timing), t_lp_exit),                    \
	};                                                                                         \
                                                                                                   \
	static const mipi_phy_b_cfg_t mipi_phy##n##_cfg = {                                        \
		.p_timing = &mipi_phy##n##_timing,                                                 \
	};                                                                                         \
                                                                                                   \
	mipi_phy_b_ctrl_t mipi_phy##n##_ctrl;                                                      \
                                                                                                   \
	static const mipi_phy_instance_t mipi_phy##n = {                                           \
		.p_ctrl = &mipi_phy##n##_ctrl,                                                     \
		.p_cfg = &mipi_phy##n##_cfg,                                                       \
		.p_api = &g_mipi_phy,                                                              \
	};

#define RENESAS_RZ_MIPI_DSI_PHYS_GET(n) &mipi_phy##n

#define RENESAS_RZ_MIPI_DSI_TIMING_DEFINE(n)                                                       \
	static const mipi_dsi_timing_t mipi_dsi_##n##_timing = {                                   \
		.clock_stop_time = DT_INST_PROP_BY_IDX(n, timing, 0),                              \
		.clock_beforehand_time = DT_INST_PROP_BY_IDX(n, timing, 1),                        \
		.clock_keep_time = DT_INST_PROP_BY_IDX(n, timing, 2),                              \
		.go_lp_and_back = DT_INST_PROP_BY_IDX(n, timing, 3),                               \
	};

#define RENESAS_RZ_MIPI_DSI_TIMING_GET(n) &mipi_dsi_##n##_timing

#define RENESAS_MIPI_DSI_DEVICE(id)                                                                \
	static void mipi_dsi_rz_configure_func_##id(void)                                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, sq0, irq),                                     \
			    DT_INST_IRQ_BY_NAME(id, sq0, priority), renesas_rz_mipi_dsi_isr,       \
			    DEVICE_DT_INST_GET(id), DT_INST_IRQ_BY_NAME(id, sq0, flags));          \
		irq_enable(DT_INST_IRQ_BY_NAME(id, sq0, irq));                                     \
	}                                                                                          \
                                                                                                   \
	RENESAS_RZ_MIPI_DSI_TIMING_DEFINE(id)                                                      \
	RENESAS_RZ_MIPI_PHYS_SETTING_DEFINE(id)                                                    \
                                                                                                   \
	static const mipi_dsi_b_extended_cfg_t mipi_dsi_##id##_extended_cfg = {                    \
		.dsi_seq0.ipl = DT_INST_IRQ_BY_NAME(id, sq0, priority),                            \
		.dsi_seq0.irq = DT_INST_IRQ_BY_NAME(id, sq0, irq),                                 \
		.dsi_seq1.ipl = DT_INST_IRQ_BY_NAME(id, sq1, priority),                            \
		.dsi_seq1.irq = DT_INST_IRQ_BY_NAME(id, sq1, irq),                                 \
		.dsi_ferr.ipl = DT_INST_IRQ_BY_NAME(id, ferr, priority),                           \
		.dsi_ferr.irq = DT_INST_IRQ_BY_NAME(id, ferr, irq),                                \
		.dsi_ppi.ipl = DT_INST_IRQ_BY_NAME(id, ppi, priority),                             \
		.dsi_ppi.irq = DT_INST_IRQ_BY_NAME(id, ppi, irq),                                  \
		.dsi_rcv.ipl = DT_INST_IRQ_BY_NAME(id, rcv, priority),                             \
		.dsi_rcv.irq = DT_INST_IRQ_BY_NAME(id, rcv, irq),                                  \
		.dsi_vin1.ipl = DT_INST_IRQ_BY_NAME(id, vin1, priority),                           \
		.dsi_vin1.irq = DT_INST_IRQ_BY_NAME(id, vin1, irq),                                \
		.dsi_rxie = R_MIPI_DSI_RXIER_BTAREQEND_Msk | R_MIPI_DSI_RXIER_LRXHTO_Msk |         \
			    R_MIPI_DSI_RXIER_TATO_Msk | R_MIPI_DSI_RXIER_RXRESP_Msk |              \
			    R_MIPI_DSI_RXIER_RXEOTP_Msk | R_MIPI_DSI_RXIER_RXACK_Msk |             \
			    R_MIPI_DSI_RXIER_MLFERR_Msk | R_MIPI_DSI_RXIER_ECCERR_Msk |            \
			    R_MIPI_DSI_RXIER_UEXPKTERR_Msk | R_MIPI_DSI_RXIER_WCERR_Msk |          \
			    R_MIPI_DSI_RXIER_CRCERR_Msk | R_MIPI_DSI_RXIER_IBERR_Msk |             \
			    R_MIPI_DSI_RXIER_RXOVFERR_Msk | R_MIPI_DSI_RXIER_PRESPTOERR_Msk |      \
			    R_MIPI_DSI_RXIER_NORETERR_Msk | R_MIPI_DSI_RXIER_MAXRPSZERR_Msk |      \
			    R_MIPI_DSI_RXIER_ECCERR1B_Msk | R_MIPI_DSI_RXIER_RXAKE_Msk,            \
		.dsi_ferrie = R_MIPI_DSI_FERRIER_HTXTO_Msk | R_MIPI_DSI_FERRIER_LRXHTO_Msk |       \
			      R_MIPI_DSI_FERRIER_TATO_Msk | R_MIPI_DSI_FERRIER_ERRESC_Msk |        \
			      R_MIPI_DSI_FERRIER_ERRSYNESC_Msk | R_MIPI_DSI_FERRIER_ERRCTRL_Msk |  \
			      R_MIPI_DSI_FERRIER_ERRCLP0_Msk | R_MIPI_DSI_FERRIER_ERRCLP1_Msk,     \
		.dsi_plie = 0x0,                                                                   \
		.dsi_vmie = R_MIPI_DSI_VICH1IER_VBUFUDF_Msk | R_MIPI_DSI_VICH1IER_VBUFOVF_Msk,     \
		.dsi_sqch0ie =                                                                     \
			R_MIPI_DSI_SQCH0IER_AACTFIN_Msk | R_MIPI_DSI_SQCH0IER_ADESFIN_Msk |        \
			R_MIPI_DSI_SQCH0IER_TXIBERR_Msk | R_MIPI_DSI_SQCH0IER_RXFATALERR_Msk |     \
			R_MIPI_DSI_SQCH0IER_RXFAIL_Msk | R_MIPI_DSI_SQCH0IER_RXPKTDFAIL_Msk |      \
			R_MIPI_DSI_SQCH0IER_RXCORERR_Msk | R_MIPI_DSI_SQCH0IER_RXAKE_Msk,          \
		.dsi_sqch1ie =                                                                     \
			R_MIPI_DSI_SQCH1IER_AACTFIN_Msk | R_MIPI_DSI_SQCH1IER_ADESFIN_Msk |        \
			R_MIPI_DSI_SQCH1IER_PKTBIGERR_Msk | R_MIPI_DSI_SQCH1IER_TXIBERR_Msk |      \
			R_MIPI_DSI_SQCH1IER_RXFATALERR_Msk | R_MIPI_DSI_SQCH1IER_RXFAIL_Msk |      \
			R_MIPI_DSI_SQCH1IER_RXPKTDFAIL_Msk | R_MIPI_DSI_SQCH1IER_RXCORERR_Msk |    \
			R_MIPI_DSI_SQCH1IER_RXAKE_Msk,                                             \
	};                                                                                         \
                                                                                                   \
	static struct mipi_dsi_renesas_rz_config rz_config_##id = {                                \
		.irq_configure = mipi_dsi_rz_configure_func_##id,                                  \
		.fsp_api = &g_mipi_dsi,                                                            \
	};                                                                                         \
                                                                                                   \
	static mipi_dsi_b_instance_ctrl_t g_mipi_dsi##id##_ctrl;                                   \
                                                                                                   \
	static mipi_dsi_cfg_t g_mipi_dsi##id##_cfg = {                                             \
		.p_mipi_phy_instance = RENESAS_RZ_MIPI_DSI_PHYS_GET(id),                           \
		.p_timing = RENESAS_RZ_MIPI_DSI_TIMING_GET(id),                                    \
		.hsa_no_lp = 0,                                                                    \
		.hbp_no_lp = 0,                                                                    \
		.hfp_no_lp = 0,                                                                    \
		.ulps_wakeup_period = DT_INST_PROP(id, ulps_wakeup_period),                        \
		.continuous_clock = 1,                                                             \
		.hs_tx_timeout = 0,                                                                \
		.lp_rx_timeout = 0,                                                                \
		.turnaround_timeout = 0,                                                           \
		.bta_timeout = 0,                                                                  \
		.lprw_timeout = 0,                                                                 \
		.hsrw_timeout = 0,                                                                 \
		.max_return_packet_size = 1,                                                       \
		.ecc_enable = 1,                                                                   \
		.crc_check_mask = (mipi_dsi_vc_t)0x0,                                              \
		.scramble_enable = 0,                                                              \
		.tearing_detect = 0,                                                               \
		.eotp_enable = 1,                                                                  \
		.sync_pulse = 0,                                                                   \
		.vertical_sync_polarity =                                                          \
			DT_PROP(DT_CHILD(DT_NODELABEL(zephyr_lcdif), display_timings),             \
				vsync_active) != DISPLAY_SIGNAL_POLARITY_HIACTIVE,                 \
		.horizontal_sync_polarity =                                                        \
			DT_PROP(DT_CHILD(DT_NODELABEL(zephyr_lcdif), display_timings),             \
				hsync_active) != DISPLAY_SIGNAL_POLARITY_HIACTIVE,                 \
		.video_mode_delay = DT_INST_PROP(id, video_mode_delay),                            \
		.p_extend = &mipi_dsi_##id##_extended_cfg,                                         \
		.p_callback = mipi_dsi_callback,                                                   \
		.p_context = DEVICE_DT_INST_GET(id),                                               \
	};                                                                                         \
                                                                                                   \
	static struct mipi_dsi_renesas_rz_data rz_data_##id = {                                    \
		.fsp_ctrl = (mipi_dsi_ctrl_t *)&g_mipi_dsi##id##_ctrl,                             \
		.fsp_cfg = &g_mipi_dsi##id##_cfg,                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &mipi_dsi_renesas_rz_init, NULL, &rz_data_##id, &rz_config_##id, \
			      POST_KERNEL, CONFIG_MIPI_DSI_INIT_PRIORITY, &mipi_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_MIPI_DSI_DEVICE)
