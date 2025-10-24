/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/renesas_rx_cgc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/interrupt_controller/intc_renesas_rx_icum_grp_irq.h>

#include "r_can_api.h"
#include "r_canfdg.h"

LOG_MODULE_REGISTER(can_renesas_rx_canfdg, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rx_canfdg

#define CAN_RENESAS_RX_TIMING_MAX                                                                  \
	{                                                                                          \
		.sjw = 128,                                                                        \
		.prop_seg = 1,                                                                     \
		.phase_seg1 = 255,                                                                 \
		.phase_seg2 = 128,                                                                 \
		.prescaler = 1024,                                                                 \
	}

#define CAN_RENESAS_RX_TIMING_MIN                                                                  \
	{                                                                                          \
		.sjw = 1,                                                                          \
		.prop_seg = 1,                                                                     \
		.phase_seg1 = 2,                                                                   \
		.phase_seg2 = 2,                                                                   \
		.prescaler = 1,                                                                    \
	}

#ifdef CONFIG_CAN_FD_MODE
#define CAN_RENESAS_RX_TIMING_DATA_MAX                                                             \
	{                                                                                          \
		.sjw = 16,                                                                         \
		.prop_seg = 1,                                                                     \
		.phase_seg1 = 31,                                                                  \
		.phase_seg2 = 16,                                                                  \
		.prescaler = 256,                                                                  \
	}
#define CAN_RENESAS_RX_TIMING_DATA_MIN                                                             \
	{                                                                                          \
		.sjw = 1,                                                                          \
		.prop_seg = 1,                                                                     \
		.phase_seg1 = 2,                                                                   \
		.phase_seg2 = 2,                                                                   \
		.prescaler = 1,                                                                    \
	}
#endif /* CONFIG_CAN_FD_MODE */

#define CANFDG_GROUP_IRQ DT_IRQ(DT_NODELABEL(group_irq_al2), irq)

/* This frame ID will be reserved. Any filter using this ID may cause undefined behavior. */
#define CAN_RENESAS_RX_RESERVED_ID (CAN_EXT_ID_MASK)

/*
 * Common FIFO configuration: refer to '45.2.27 Common FIFO n Configuration Register (CFCRn)' -
 * RX74M MCU group HWM
 */
#define CANFDG_CFG_COMMONFIFO                                                                       \
	{CANFDG_CFG_COMMONFIFO0, CANFDG_CFG_COMMONFIFO1, CANFDG_CFG_COMMONFIFO2,                      \
	 CANFDG_CFG_COMMONFIFO3, CANFDG_CFG_COMMONFIFO4, CANFDG_CFG_COMMONFIFO5}

#define CANFDG_CFG_RX_FIFO0                                                                         \
	((1U << R_CANFDG_RFCR_RFE_Pos) |  /* RX FIFO Enable */                                      \
	 (1U << R_CANFDG_RFCR_RFIE_Pos) | /* RX FIFO Interrupt Enable */                            \
	 (7U << R_CANFDG_RFCR_PLS_Pos) |  /* RX FIFO Payload Data Size: 64 */                       \
	 (3U << R_CANFDG_RFCR_FDS_Pos) |  /* RX FIFO Depth: 16 messages */                          \
	 (1U << R_CANFDG_RFCR_RFIM_Pos))  /* Interrupt generated at the end of                      \
					  *  every received message storage                        \
					  */

#define CANFDG_CFG_RX_FIFO1 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO1 Disable */
#define CANFDG_CFG_RX_FIFO2 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO2 Disable */
#define CANFDG_CFG_RX_FIFO3 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO3 Disable */
#define CANFDG_CFG_RX_FIFO4 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO4 Disable */
#define CANFDG_CFG_RX_FIFO5 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO5 Disable */
#define CANFDG_CFG_RX_FIFO6 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO6 Disable */
#define CANFDG_CFG_RX_FIFO7 (0U << R_CANFDG_RFCR_RFE_Pos) /* RX FIFO7 Disable */

/*
 * RX FIFO configuration: refer to '45.2.24 Receive FIFO n Configuration Register (RFCRn)' -
 * RX74M MCU group HWM
 */
#define CANFDG_CFG_RXFIFO                                                                           \
	{CANFDG_CFG_RX_FIFO0, CANFDG_CFG_RX_FIFO1, CANFDG_CFG_RX_FIFO2, CANFDG_CFG_RX_FIFO3,           \
	 CANFDG_CFG_RX_FIFO4, CANFDG_CFG_RX_FIFO5, CANFDG_CFG_RX_FIFO6, CANFDG_CFG_RX_FIFO7}

/*
 * Global Configuration: refer to '45.2.10 Global Configuration Register (GCFG)' - RX74M MCU
 * group HWM
 */
#define CANFDG_CFG_GLOBAL                                                                           \
	((0U << R_CANFDG_GCFG_TPRI_Pos) | /* Transmission Priority: ID priority */                  \
	 (0U << R_CANFDG_GCFG_DCE_Pos) |  /* DLC check disabled */                                  \
	 (0U << R_CANFDG_GCFG_DLLCS_Pos)) /* DLL Clock Select: CANFDCLK */

/*
 * TX Message Buffer Interrupt Enable Configuration: refer to '45.2.52 Transmit Message Buffer
 * Interrupt Enable Register m (TMIERm)' - RX74M MCU group HWM
 */
#define CANFDG_CFG_TXMB_TXI_ENABLE (BIT(0)) /* Enable TXMB0 interrupt */

/*
 * Number and size of RX Message Buffers: refer to '45.2.22 Receive Message Buffer Configuration
 * Register (RMCR)' - RX74M MCU group HWM
 */
#define CANFDG_CFG_RXMB (0U << R_CANFDG_RMCR_NMB_Pos) /* Number of RX Message Buffers: 0 */

/*
 * Channel Error IRQ configurations: refer to '45.2.2 Channel Control Register (CHCR)' - RX74M MCU
 * group HWM
 */
#define CANFDG_CFG_ERR_IRQ                                                                          \
	(BIT(R_CANFDG_CFDC_CHCR_EWIE_Pos) |  /* Error Warning Interrupt Enable */                   \
	 BIT(R_CANFDG_CFDC_CHCR_EPIE_Pos) |  /* Error Passive Interrupt Enable */                   \
	 BIT(R_CANFDG_CFDC_CHCR_BOEIE_Pos) | /* Bus-Off Entry Interrupt Enable */                   \
	 BIT(R_CANFDG_CFDC_CHCR_BORIE_Pos) | /* Bus-Off Recovery Interrupt Enable */                \
	 BIT(R_CANFDG_CFDC_CHCR_OLIE_Pos))   /* Overload Interrupt Enable */

/*
 * Global Error IRQ configurations: refer to '45.2.11 Global Control Register (GCR)' - RX74M MCU
 * group HWM
 */
#define CANFDG_CFG_GLERR_IRQ                                                                        \
	((3UL << R_CANFDG_GCR_MDC_Pos) |   /* Global Mode Control: Keep current value */            \
	 (0UL << R_CANFDG_GCR_DEIE_Pos) |  /* DLC check interrupt disabled */                       \
	 (0UL << R_CANFDG_GCR_MLIE_Pos) |  /* Message lost error interrupt disabled */              \
	 (0UL << R_CANFDG_GCR_THLIE_Pos) | /* TX history list entry lost interrupt disabled */      \
	 (0UL << R_CANFDG_GCR_POIE_Pos))   /* CANFD message payload overflow flag interrupt         \
					   * disabled                                              \
					   */
/* Keycode to enable/disable accessing to AFL entry */
#define AFIGER_KEY_CODE (0xC4UL)

/* Default Dataphase bitrate configuration in case classic mode is enabled */
static const can_bit_timing_cfg_t classic_can_data_timing_default = {
	.baud_rate_prescaler = 1,
	.time_segment_1 = 3,
	.time_segment_2 = 2,
	.synchronization_jump_width = 1,
};

struct can_renesas_rx_global_cfg {
	const struct device *op_clk;
	const struct device *ram_clk;
	const struct clock_control_rx_subsys_cfg op_subsys;
	const struct clock_control_rx_subsys_cfg ram_subsys;
	const unsigned int dll_min_freq;
	const unsigned int dll_max_freq;
};

struct can_renesas_rx_global_data {
	canfdg_global_cfg_t fsp_canfdg_global_cfg;
	const struct device *can_global_intc;
	const unsigned int rfri_num;
	const unsigned int glei_num;
};

struct can_renesas_rx_cfg {
	struct can_driver_config common;
	const struct device *global_dev;
	const struct pinctrl_dev_config *pcfg;
	const struct device *dll_clk;
	const struct clock_control_rx_subsys_cfg dll_subsys;
	const uint32_t rx_filter_num;
};

struct can_renesas_rx_filter {
	bool set;
	struct can_filter filter;
	can_rx_callback_t rx_cb;
	void *rx_usr_data;
};

struct can_renesas_rx_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	struct k_sem tx_sem;
	can_tx_callback_t tx_cb;
	void *tx_usr_data;
	struct can_renesas_rx_filter *rx_filter;

	/* Renesas RX FSP data */
	can_instance_t fsp_can;
	canfdg_instance_ctrl_t fsp_canfdg_ctrl;
	can_cfg_t fsp_can_cfg;
	canfdg_extended_cfg_t fsp_canfdg_extend;
	can_bit_timing_cfg_t bit_timing;
	can_bit_timing_cfg_t data_timing;

	const struct device *can_intc;
	unsigned int chti_num;
	unsigned int chei_num;
	unsigned int chri_num;
};

extern void canfdg_error_isr(void);
extern void canfdg_rx_fifo_isr(void);
extern void canfdg_common_fifo_rx_isr(void);
extern void canfdg_channel_tx_isr(void);

/**************************************************************************************************
 * Subsys APIs implementation
 *************************************************************************************************/
/**
 * Get the supported modes of the CAN controller
 */
static int can_renesas_rx_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);
	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK;

#ifdef CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	*cap |= CAN_MODE_MANUAL_RECOVERY;
#endif
	return 0;
}

/**
 * Start the CAN controller
 */
static int can_renesas_rx_start(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;
	const struct device *transceiver_dev = can_get_transceiver(dev);

	if (!device_is_ready(dev)) {
		return -EIO;
	}

	if (data->common.started) {
		return -EALREADY;
	}

	if (transceiver_dev && can_transceiver_enable(transceiver_dev, data->common.mode)) {
		LOG_DBG("Transceiver: can transceiver enable failed");
		return -EIO;
	}

	int ret = 0;

	data->fsp_canfdg_extend.p_data_timing =
		(data->common.mode & CAN_MODE_FD)
			? (can_bit_timing_cfg_t *)&data->data_timing
			: (can_bit_timing_cfg_t *)&classic_can_data_timing_default;

	if (data->fsp_canfdg_ctrl.open != 0) {
		if (FSP_SUCCESS != can_api->close(data->fsp_can.p_ctrl)) {
			LOG_DBG("CAN close failed");
			ret = -EIO;
			goto end;
		}
	}

	if (FSP_SUCCESS != can_api->open(data->fsp_can.p_ctrl, data->fsp_can.p_cfg)) {
		LOG_DBG("CAN open failed");
		ret = -EIO;
		goto end;
	}

	if ((data->common.mode & CAN_MODE_LOOPBACK) != 0) {
		if (FSP_SUCCESS != can_api->modeTransition(data->fsp_can.p_ctrl,
							   CAN_OPERATION_MODE_NORMAL,
							   CAN_TEST_MODE_LOOPBACK_INTERNAL)) {
			LOG_DBG("CAN mode change failed");
			ret = -EIO;
			goto end;
		}
	}

	data->common.started = true;

end:
	return ret;
}

/**
 * Stop the CAN controller
 */
static int can_renesas_rx_stop(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;
	const struct device *transceiver_dev = can_get_transceiver(dev);
	fsp_err_t fsp_err;
	int ret = 0;

	if (!data->common.started) {
		return -EALREADY;
	}

	fsp_err = can_api->modeTransition(data->fsp_can.p_ctrl, CAN_OPERATION_MODE_HALT,
					  CAN_TEST_MODE_DISABLED);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Can stop failed");
		ret = -EIO;
		goto end;
	}

	if (transceiver_dev && can_transceiver_disable(transceiver_dev)) {
		LOG_DBG("Transceiver: can transceiver disable failed");
		ret = -EIO;
		goto end;
	}

	if (data->tx_cb != NULL) {
		data->tx_cb = NULL;
		k_sem_give(&data->tx_sem);
	}

	data->common.started = false;

end:
	return ret;
}

/**
 * Set the CAN controller to the given operation mode
 */
static int can_renesas_rx_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_renesas_rx_data *data = dev->data;
	int ret = 0;

	if (data->common.started) {
		/* CAN controller is not in stopped state */
		return -EBUSY;
	}

	can_mode_t caps = 0;

	ret = can_renesas_rx_get_capabilities(dev, &caps);
	if (ret != 0) {
		goto end;
	}

	if ((mode & ~caps) != 0) {
		ret = -ENOTSUP;
		goto end;
	}

	data->common.mode = mode;
end:
	return ret;
}

static inline void can_renesas_rx_call_tx_cb(const struct device *dev, int err)
{
	struct can_renesas_rx_data *data = dev->data;
	can_tx_callback_t cb = data->tx_cb;

	if (cb != NULL) {
		data->tx_cb = NULL;
		cb(dev, err, data->tx_usr_data);
		k_sem_give(&data->tx_sem);
	}
}

static inline void can_renesas_rx_call_rx_cb(const struct device *dev, can_callback_args_t *p_args)
{
	struct can_renesas_rx_data *data = dev->data;
	const struct can_renesas_rx_cfg *cfg = dev->config;
	struct can_renesas_rx_filter *rx_filter = data->rx_filter;

	struct can_frame frame = {
		.dlc = can_bytes_to_dlc(p_args->frame.data_length_code),
		.id = p_args->frame.id,
		.flags = (((p_args->frame.id_mode == CAN_ID_MODE_EXTENDED) ? CAN_FRAME_IDE : 0UL) |
			  ((p_args->frame.type == CAN_FRAME_TYPE_REMOTE) ? CAN_FRAME_RTR : 0UL) |
			  ((p_args->frame.options & CANFDG_FRAME_OPTION_FD) != 0 ? CAN_FRAME_FDF
										: 0UL) |
			  ((p_args->frame.options & CANFDG_FRAME_OPTION_ERROR) != 0 ? CAN_FRAME_ESI
										   : 0UL) |
			  ((p_args->frame.options & CANFDG_FRAME_OPTION_BRS) != 0 ? CAN_FRAME_BRS
										 : 0UL)),
	};

	memcpy(frame.data, p_args->frame.data, p_args->frame.data_length_code);

	for (uint32_t i = 0; i < cfg->rx_filter_num; i++) {
		can_rx_callback_t cb = rx_filter->rx_cb;
		void *usr_data = rx_filter->rx_usr_data;

		if (cb == NULL) {
			rx_filter++;
			continue; /* this filter has not set yet */
		}

		if (!can_frame_matches_filter(&frame, &rx_filter->filter)) {
			rx_filter++;
			continue; /* filter did not match */
		}

		cb(dev, &frame, usr_data);
		break;
	}
}

static void can_renesas_rx_call_state_change_cb(const struct device *dev, enum can_state state)
{
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;
	can_info_t can_info = {};
	fsp_err_t fsp_err;

	fsp_err = can_api->infoGet(data->fsp_can.p_ctrl, &can_info);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Can get err info failed");
		return;
	}
	struct can_bus_err_cnt err_cnt = {
		.rx_err_cnt = can_info.error_count_receive,
		.tx_err_cnt = can_info.error_count_transmit,
	};

	can_fire_state_change_callbacks(dev, state, err_cnt);
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int recover_bus(const struct device *dev, k_timeout_t timeout)
{
	struct can_renesas_rx_data *data = dev->data;
	canfdg_instance_ctrl_t *p_ctrl = &data->fsp_canfdg_ctrl;
	R_CANFDG_Type *p_reg = p_ctrl->p_reg;
	uint32_t chcr = p_reg->CFDC->CHCR;
	int ret = 0;

	if (p_reg->CFDC->CHESR_b.BOEDF == 0) {
		/* Channel bus-off entry not detected */
		goto end;
	}

	p_reg->CFDC->CHCR_b.BOM =
		0x00;                 /* Switch to Normal Bus-Off mode (comply with ISO 11898-1) */
	p_reg->CFDC->CHCR_b.RTBO = 1; /* Force channel state to return from bus-off */

	int64_t start_ticks = k_uptime_ticks();

	while (p_reg->CFDC->CHESR_b.BORDF == 0) {
		if ((k_uptime_ticks() - start_ticks) > timeout.ticks) {
			ret = -EAGAIN;
			goto end;
		}
	}

end:
	p_reg->CFDC->CHCR = chcr; /* Restore channel configuration */
	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

/**************************************************************************************************
 * Internal function definition
 *************************************************************************************************/
static inline int set_hw_timing_configuration(const struct device *dev,
					      can_bit_timing_cfg_t *f_timing,
					      const struct can_timing *z_timing)
{
	struct can_renesas_rx_data *data = dev->data;

	if (f_timing == NULL || z_timing == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	*f_timing = (can_bit_timing_cfg_t){
		.baud_rate_prescaler = z_timing->prescaler,
		.time_segment_1 = z_timing->prop_seg + z_timing->phase_seg1,
		.time_segment_2 = z_timing->phase_seg2,
		.synchronization_jump_width = z_timing->sjw,
	};

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

/**
 * Configure the bus timing of a CAN controller.
 */
static int can_renesas_rx_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_renesas_rx_data *data = dev->data;

	if (data->common.started) {
		/* Device is not in stopped state */
		return -EBUSY;
	}

	return set_hw_timing_configuration(dev, &data->bit_timing, timing);
}

/**
 * Queue a CAN frame for transmission on the CAN bus with optional timeout and completion callback
 * function.
 */
static int can_renesas_rx_send(const struct device *dev, const struct can_frame *frame,
			       k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;

	if (!data->common.started) {
		/* CAN controller is in stopped state */
		return -ENETDOWN;
	}

#ifdef CONFIG_CAN_FD_MODE
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR | CAN_FRAME_FDF | CAN_FRAME_BRS)) !=
	    0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if ((data->common.mode & CAN_MODE_FD) == 0U &&
	    ((frame->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0U)) {
		LOG_ERR("CAN FD format not supported in non-FD mode");
		return -ENOTSUP;
	}
#else  /* CONFIG_CAN_FD_MODE */
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0U) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}
#endif /* !CONFIG_CAN_FD_MODE */

	if ((frame->flags & CAN_FRAME_FDF) != 0U) {
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("DLC of %d for CAN FD format frame", frame->dlc);
			return -EINVAL;
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("DLC of %d for non-FD format frame", frame->dlc);
			return -EINVAL;
		}
	}

	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	data->tx_cb = callback;
	data->tx_usr_data = user_data;

	/* Store user callback to be called when transmission error occur */

	can_frame_t fsp_frame = {
		.id = frame->id,
		.id_mode = ((frame->flags & CAN_FRAME_IDE) != 0) ? CAN_ID_MODE_EXTENDED
								 : CAN_ID_MODE_STANDARD,
		.type = ((frame->flags & CAN_FRAME_RTR) != 0) ? CAN_FRAME_TYPE_REMOTE
							      : CAN_FRAME_TYPE_DATA,
		.data_length_code = can_dlc_to_bytes(frame->dlc),
		.options =
			((((frame->flags & CAN_FRAME_FDF) != 0) ? CANFDG_FRAME_OPTION_FD : 0UL) |
			 (((frame->flags & CAN_FRAME_BRS) != 0) ? CANFDG_FRAME_OPTION_BRS : 0UL) |
			 (((frame->flags & CAN_FRAME_ESI) != 0) ? CANFDG_FRAME_OPTION_ERROR : 0UL)),
	};

	memcpy(fsp_frame.data, frame->data, fsp_frame.data_length_code);

	if (FSP_SUCCESS != can_api->write(data->fsp_can.p_ctrl, CANFDG_TX_MB_0, &fsp_frame)) {
		data->tx_cb = NULL;
		data->tx_usr_data = NULL;
		LOG_DBG("Can send failed");
		k_sem_give(&data->tx_sem);
		k_mutex_unlock(&data->inst_mutex);
		return -EIO;
	}

	k_mutex_unlock(&data->inst_mutex);
	return 0;
}

static inline int get_free_filter_id(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	const struct can_renesas_rx_cfg *cfg = dev->config;

	for (uint32_t filter_id = 0; filter_id < cfg->rx_filter_num; filter_id++) {
		if (!data->rx_filter[filter_id].set) {
			return filter_id;
		}
	}

	return -ENOSPC;
}

static inline void set_afl_rule(const struct device *dev, const struct can_filter *filter,
				uint32_t afl_offset)
{
	struct can_renesas_rx_data *data = dev->data;
	canfdg_afl_entry_t *afl = (canfdg_afl_entry_t *)&data->fsp_canfdg_extend.p_afl[afl_offset];

	*afl = (canfdg_afl_entry_t){
		.id_u = {.id = {.id = filter->id,
#ifndef CONFIG_CAN_ACCEPT_RTR
		       .frame_type = CAN_FRAME_TYPE_DATA,
#endif
		       .id_mode = ((filter->flags & CAN_FILTER_IDE) != 0) ? CAN_ID_MODE_EXTENDED
									  : CAN_ID_MODE_STANDARD}},
		.mask_u = {.mask = {
				.mask_id = filter->mask,
#ifdef CONFIG_CAN_ACCEPT_RTR
				/* Accept all types of frames */
				.mask_frame_type = 0,
#else
				/* Only accept frames with the configured frametype */
				.mask_frame_type = 1,
#endif
				.mask_id_mode = ((filter->flags & CAN_FILTER_IDE) != 0)
							? CAN_ID_MODE_EXTENDED
							: CAN_ID_MODE_STANDARD,
		}},
		.destination_u = {.destination = {
				.fifo_select_flags = CANFDG_RX_FIFO_0,
		}},
	};

	if (data->common.started) {
		canfdg_instance_ctrl_t *p_ctrl = &data->fsp_canfdg_ctrl;
		R_CANFDG_Type *reg = p_ctrl->p_reg;
		uint32_t channel = data->fsp_can_cfg.channel;

		/* Update AFL rules while CAN module is running */
		reg->AFIGSR_b.IGES = afl_offset & 0x7F;
		reg->AFIGSR_b.IGCS = channel & 0x1FF;

		/* Ignore entry enabled*/
		reg->AFIGER = ((AFIGER_KEY_CODE << R_CANFDG_AFIGER_KEY_Pos) |
			       BIT(R_CANFDG_AFIGER_IGEE_Pos));

		/* Set AFL page number and enable AFL write */
		reg->AFCR = (afl_offset >> 4) | R_CANFDG_AFCR_AFLWE_Msk;

		/* Write AFL configuration */
		reg->AFL[afl_offset & 0xF] = *(R_CANFDG_AFL_Type *)afl;

		/*Set Information Label 0 to the channel being configured*/
		reg->AFL[afl_offset & 0xF].PTR0_b.IFL0 = channel & 1U;

		/* Lock AFL entry access */
		reg->AFCR = 0;
		reg->AFIGER = ((AFIGER_KEY_CODE << R_CANFDG_AFIGER_KEY_Pos));
	}
}

static int can_renesas_rx_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
					void *user_data, const struct can_filter *filter)
{
	struct can_renesas_rx_data *data = dev->data;
	int filter_id = -ENOSPC;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	filter_id = get_free_filter_id(dev);

	if (filter_id == -ENOSPC) {
		k_mutex_unlock(&data->inst_mutex);
		return filter_id;
	}

	set_afl_rule(dev, filter, filter_id);

	memcpy(&data->rx_filter[filter_id].filter, filter, sizeof(struct can_filter));
	data->rx_filter[filter_id].rx_cb = callback;
	data->rx_filter[filter_id].rx_usr_data = user_data;
	data->rx_filter[filter_id].set = true;

	k_mutex_unlock(&data->inst_mutex);

	return filter_id;
}

static inline void remove_afl_rule(const struct device *dev, uint32_t afl_offset)
{
	struct can_renesas_rx_data *data = dev->data;
	canfdg_afl_entry_t *afl = (canfdg_afl_entry_t *)&data->fsp_canfdg_extend.p_afl[afl_offset];

	/* Set the AFL ID to reserved ID */
	*afl = (canfdg_afl_entry_t){
		.id_u = {.id = {
				.id = CAN_RENESAS_RX_RESERVED_ID,
				.id_mode = CAN_ID_MODE_EXTENDED,
		}},
		.mask_u = {.mask = {
				.mask_id = CAN_RENESAS_RX_RESERVED_ID,
				.mask_id_mode = CAN_ID_MODE_EXTENDED,
		}},
	};

	if (data->common.started) {
		canfdg_instance_ctrl_t *p_ctrl = &data->fsp_canfdg_ctrl;
		R_CANFDG_Type *reg = p_ctrl->p_reg;
		uint32_t channel = data->fsp_can_cfg.channel;

		/* Update AFL rules while CAN module is running */
		reg->AFIGSR_b.IGES = afl_offset & 0x7F;
		reg->AFIGSR_b.IGCS = channel & 0x1FF;
		/* Ignore entry enabled*/
		reg->AFIGER = ((AFIGER_KEY_CODE << R_CANFDG_AFIGER_KEY_Pos) |
			       BIT(R_CANFDG_AFIGER_IGEE_Pos));

		/* Set AFL page number and enable AFL write */
		reg->AFCR = (afl_offset >> 4) | R_CANFDG_AFCR_AFLWE_Msk;

		/* Write AFL configuration */
		reg->AFL[afl_offset & 0xF] = *(R_CANFDG_AFL_Type *)afl;

		/*Set Information Label 0 to the channel being configured*/
		reg->AFL[afl_offset & 0xF].PTR0_b.IFL0 = channel & 1U;

		/* Lock AFL entry access */
		reg->AFCR = 0;
		reg->AFIGER = ((AFIGER_KEY_CODE << R_CANFDG_AFIGER_KEY_Pos));
	}
}

static void can_renesas_rx_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_renesas_rx_data *data = dev->data;
	const struct can_renesas_rx_cfg *cfg = dev->config;

	if ((filter_id < 0) || (filter_id >= cfg->rx_filter_num)) {
		LOG_ERR("Invalid filter ID %d", filter_id);
		return;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	remove_afl_rule(dev, filter_id);

	data->rx_filter[filter_id].rx_cb = NULL;
	data->rx_filter[filter_id].rx_usr_data = NULL;
	data->rx_filter[filter_id].set = false;

	k_mutex_unlock(&data->inst_mutex);
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
/**
 * Recover the CAN controller from bus-off state to error-active state.
 */
static int can_renesas_rx_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_renesas_rx_data *data = dev->data;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (!(data->common.mode & CAN_MODE_MANUAL_RECOVERY)) {
		return -ENOTSUP;
	}

	return recover_bus(dev, timeout);
}
#endif

/**
 * Get current CAN controller state
 */
static int can_renesas_rx_get_state(const struct device *dev, enum can_state *state,
				    struct can_bus_err_cnt *err_cnt)
{
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;
	can_info_t fsp_info = {0};
	fsp_err_t fsp_err;

	fsp_err = can_api->infoGet(data->fsp_can.p_ctrl, &fsp_info);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Can get state failed");
		return -EIO;
	}

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			if (fsp_info.error_code & R_CANFDG_CFDC_CHESR_BOEDF_Msk) {
				*state = CAN_STATE_BUS_OFF;
			} else if (fsp_info.error_code & R_CANFDG_CFDC_CHESR_EPDF_Msk) {
				*state = CAN_STATE_ERROR_PASSIVE;
			} else if (fsp_info.error_code & R_CANFDG_CFDC_CHESR_EWDF_Msk) {
				*state = CAN_STATE_ERROR_WARNING;
			} else if (fsp_info.error_code & R_CANFDG_CFDC_CHESR_BEDF_Msk) {
				*state = CAN_STATE_ERROR_ACTIVE;
			}
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = fsp_info.error_count_transmit;
		err_cnt->rx_err_cnt = fsp_info.error_count_receive;
	}
	return 0;
}

static int can_renesas_rx_state_change_callbacks_enabled(const struct device *dev, bool enabled)
{
	struct can_renesas_rx_data *data = dev->data;
	canfdg_instance_ctrl_t *p_ctrl = data->fsp_can.p_ctrl;
	int key = irq_lock();

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	if (enabled) {
		/* Enable state change interrupt */
		p_ctrl->p_reg->CFDC->CHCR |= (uint32_t)CANFDG_CFG_ERR_IRQ;
	} else {
		/* Disable state change interrupt */
		p_ctrl->p_reg->CFDC->CHCR &= (uint32_t)~CANFDG_CFG_ERR_IRQ;

		/* Clear error flags */
		p_ctrl->p_reg->CFDC->CHESR &=
			~(BIT(R_CANFDG_CFDC_CHESR_BOEDF_Pos) | BIT(R_CANFDG_CFDC_CHESR_EWDF_Pos) |
			  BIT(R_CANFDG_CFDC_CHESR_EPDF_Pos) | BIT(R_CANFDG_CFDC_CHESR_BEDF_Pos));
	}

	k_mutex_unlock(&data->inst_mutex);

	irq_unlock(key);

	return 0;
}

/**
 * Get the CAN core clock rate.
 */
static int can_renesas_rx_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_renesas_rx_cfg *cfg = dev->config;
	clock_control_subsys_t dll_subsys = (clock_control_subsys_t)&cfg->dll_subsys;

	return clock_control_get_rate(cfg->dll_clk, dll_subsys, rate) ? -EIO : 0;
}

/**
 * Get maximum number of RX filters.
 */
static int can_renesas_rx_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);
	const struct can_renesas_rx_cfg *cfg = dev->config;

	return cfg->rx_filter_num;
}

#ifdef CONFIG_CAN_FD_MODE
/**
 * Configure the bus timing for the data phase of a CAN FD controller.
 */
static int can_renesas_rx_set_timing_data(const struct device *dev,
					  const struct can_timing *timing_data)
{
	struct can_renesas_rx_data *data = dev->data;

	if (data->common.started) {
		/* CAN controller is not in stopped state. */
		return -EBUSY;
	}

	return set_hw_timing_configuration(dev, &data->data_timing, timing_data);
}
#endif /* CONFIG_CAN_FD_MODE */

void can_renesas_rx_fsp_cb(can_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;

	switch (p_args->event) {
	case CAN_EVENT_RX_COMPLETE: {
		can_renesas_rx_call_rx_cb(dev, p_args);
		break;
	}

	case CAN_EVENT_TX_COMPLETE: {
		can_renesas_rx_call_tx_cb(dev, 0);
		break;
	}

	case CAN_EVENT_ERR_CHANNEL: {
		if (p_args->error & R_CANFDG_CFDC_CHESR_BEDF_Msk) {
			can_renesas_rx_call_state_change_cb(dev, CAN_STATE_ERROR_ACTIVE);
		}
		if (p_args->error & R_CANFDG_CFDC_CHESR_EWDF_Msk) {
			can_renesas_rx_call_state_change_cb(dev, CAN_STATE_ERROR_WARNING);
		}
		if (p_args->error & R_CANFDG_CFDC_CHESR_EPDF_Msk) {
			can_renesas_rx_call_state_change_cb(dev, CAN_STATE_ERROR_PASSIVE);
		}
		if (p_args->error & R_CANFDG_CFDC_CHESR_BOEDF_Msk) {
			can_renesas_rx_call_state_change_cb(dev, CAN_STATE_BUS_OFF);
		}
		break;
	}

	default:
		break;
	}
}

static int can_renesas_rx_apply_default_config(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	const struct can_renesas_rx_cfg *cfg = dev->config;
	struct can_renesas_rx_global_data *global_data = cfg->global_dev->data;
	int ret;

	struct can_timing timing = {0};

	/* Calculate bit_timing parameter */
	can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);
	ret = can_renesas_rx_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}
#ifdef CONFIG_CAN_FD_MODE
	can_calc_timing_data(dev, &timing, cfg->common.bitrate_data, cfg->common.sample_point_data);
	ret = can_renesas_rx_set_timing_data(dev, &timing);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_CAN_FD_MODE */

	data->fsp_canfdg_extend.p_global_cfg = &global_data->fsp_canfdg_global_cfg;
	for (uint32_t filter_id = 0; filter_id < cfg->rx_filter_num; filter_id++) {
		remove_afl_rule(dev, filter_id);
	}

	return 0;
}

static int can_renesas_module_clock_init(const struct device *dev)
{
	const struct can_renesas_rx_cfg *cfg = dev->config;
	int ret;

	ret = clock_control_on(cfg->dll_clk, (clock_control_subsys_t)&cfg->dll_subsys);
	if (ret) {
		return -EIO;
	}

	const struct can_renesas_rx_global_cfg *global_cfg = cfg->global_dev->config;
	uint32_t op_rate;
	uint32_t ram_rate;
	uint32_t dll_rate;

	ret = clock_control_get_rate(global_cfg->op_clk,
				     (clock_control_subsys_t)&global_cfg->op_subsys, &op_rate);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(global_cfg->ram_clk,
				     (clock_control_subsys_t)&global_cfg->ram_subsys, &ram_rate);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(cfg->dll_clk, (clock_control_subsys_t)&cfg->dll_subsys,
				     &dll_rate);
	if (ret < 0) {
		return ret;
	}

	if (dll_rate < global_cfg->dll_min_freq || dll_rate > global_cfg->dll_max_freq) {
		LOG_ERR("%s frequency is out of supported range: %d < %s freq < %d",
			cfg->dll_clk->name, global_cfg->dll_min_freq, cfg->dll_clk->name,
			global_cfg->dll_max_freq);
		return -ENOTSUP;
	}

	/* Clock constraint: refer to '34.1.2 Clock restriction' - RA8M1 MCU group HWM */
	/*
	 * Operation clock rate must be at least 40Mhz in case CANFD mode.
	 * Otherwise, it must be at least 32MHz.
	 */
	if (IS_ENABLED(CONFIG_CAN_FD_MODE) ? op_rate < MHZ(40) : op_rate < MHZ(32)) {
		LOG_ERR("%s frequency should be at least %d", global_cfg->op_clk->name,
			IS_ENABLED(CONFIG_CAN_FD_MODE) ? MHZ(40) : MHZ(32));
		return -ENOTSUP;
	}

	/*
	 * (RAM clock rate / 2) >= DLL rate
	 * (CANFD operation clock rate) >= DLL rate
	 */
	if ((ram_rate / 2) < dll_rate || op_rate < dll_rate) {
		LOG_ERR("%s frequency should be less than half of %s and %s frequency should "
			"be less than %s",
			global_cfg->ram_clk->name, cfg->dll_clk->name, global_cfg->op_clk->name,
			cfg->dll_clk->name);
		return -ENOTSUP;
	}

	return 0;
}

static int can_renesas_rx_chei_enable(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	int ret;

	ret = rx_icum_grp_irq_callback_set(data->can_intc, data->chei_num, canfdg_error_isr,
					   (void *)dev);
	if (ret) {
		return ret;
	}

	ret = rx_icum_grp_irq_enable(data->can_intc, data->chei_num);
	return ret;
}

static int can_renesas_rx_chri_enable(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	int ret;

	ret = rx_icum_grp_irq_callback_set(data->can_intc, data->chri_num,
					   canfdg_common_fifo_rx_isr, (void *)dev);
	if (ret) {
		return ret;
	}

	ret = rx_icum_grp_irq_enable(data->can_intc, data->chri_num);
	return ret;
}

static int can_renesas_rx_chti_enable(const struct device *dev)
{
	struct can_renesas_rx_data *data = dev->data;
	int ret;

	ret = rx_icum_grp_irq_callback_set(data->can_intc, data->chti_num, canfdg_channel_tx_isr,
					   (void *)dev);
	if (ret) {
		return ret;
	}

	ret = rx_icum_grp_irq_enable(data->can_intc, data->chti_num);
	return ret;
}

static int can_renesas_rx_glei_enable(const struct device *dev)
{
	struct can_renesas_rx_global_data *data = dev->data;
	int ret;

	ret = rx_icum_grp_irq_callback_set(data->can_global_intc, data->glei_num, canfdg_error_isr,
					   (void *)dev);
	if (ret) {
		return ret;
	}

	ret = rx_icum_grp_irq_enable(data->can_global_intc, data->glei_num);
	return ret;
}

static int can_renesas_rx_rfri_enable(const struct device *dev)
{
	struct can_renesas_rx_global_data *data = dev->data;
	int ret;

	ret = rx_icum_grp_irq_callback_set(data->can_global_intc, data->rfri_num, canfdg_rx_fifo_isr,
					   (void *)dev);
	if (ret) {
		return ret;
	}

	ret = rx_icum_grp_irq_enable(data->can_global_intc, data->rfri_num);
	return ret;
}

static int can_renesas_rx_init(const struct device *dev)
{
	const struct can_renesas_rx_cfg *cfg = dev->config;
	struct can_renesas_rx_data *data = dev->data;
	const can_api_t *can_api = data->fsp_can.p_api;

	int ret = 0;
	fsp_err_t err;

	if (!device_is_ready(cfg->global_dev)) {
		return -ENODEV;
	}

	sys_slist_init(&data->common.state_change_callbacks);
	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_sem, 1, 1);
	data->common.started = false;

	ret = can_renesas_module_clock_init(dev);
	if (ret) {
		LOG_DBG("Clock initialize failed");
		return ret;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_DBG("Pin function initial failed: %d", ret);
		return ret;
	}

	/* Apply config and setting for CAN controller HW */
	ret = can_renesas_rx_apply_default_config(dev);
	if (ret) {
		LOG_DBG("Apply default configure failed: %d", ret);
		return ret;
	}
	err = can_api->open(data->fsp_can.p_ctrl, data->fsp_can.p_cfg);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Can bus initialize failed");
		return -EIO;
	}
	/* Put CAN controller into stopped state */
	err = can_api->modeTransition(data->fsp_can.p_ctrl, CAN_OPERATION_MODE_HALT,
				      CAN_TEST_MODE_DISABLED);
	if (err != FSP_SUCCESS) {
		LOG_DBG("Can bus initialize failed");
		goto end;
	}

	ret = can_renesas_rx_chei_enable(dev);
	if (ret) {
		LOG_DBG("Enable chei intc failed");
		goto end;
	}

	ret = can_renesas_rx_chti_enable(dev);
	if (ret) {
		LOG_DBG("Enable chti intc failed");
		goto end;
	}

	ret = can_renesas_rx_chri_enable(dev);
	if (ret) {
		LOG_DBG("Enable chri intc failed");
		goto end;
	}

	return 0;

end:
	can_api->close(data->fsp_can.p_ctrl);
	LOG_DBG("Close CAN due to init failure");
	return -EIO;
}

static int can_renesas_rx_global_init(const struct device *dev)
{
	const struct can_renesas_rx_global_cfg *cfg = dev->config;
	int ret;

	ret = clock_control_on(cfg->op_clk, (clock_control_subsys_t)&cfg->op_subsys);
	if (ret < 0) {
		LOG_DBG("Clock initialize failed");
		return ret;
	}

	ret = clock_control_on(cfg->ram_clk, (clock_control_subsys_t)&cfg->ram_subsys);
	if (ret < 0) {
		LOG_DBG("Clock initialize failed");
		return ret;
	}

	ret = can_renesas_rx_rfri_enable(dev);
	if (ret) {
		LOG_DBG("Enable rfri intc failed");
		return ret;
	}

	ret = can_renesas_rx_glei_enable(dev);
	if (ret) {
		LOG_DBG("Enable glei intc failed");
		return ret;
	}

	return 0;
}

static DEVICE_API(can, can_renesas_rx_driver_api) = {
	.get_capabilities = can_renesas_rx_get_capabilities,
	.start = can_renesas_rx_start,
	.stop = can_renesas_rx_stop,
	.set_mode = can_renesas_rx_set_mode,
	.set_timing = can_renesas_rx_set_timing,
	.send = can_renesas_rx_send,
	.add_rx_filter = can_renesas_rx_add_rx_filter,
	.remove_rx_filter = can_renesas_rx_remove_rx_filter,
#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
	.recover = can_renesas_rx_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_state = can_renesas_rx_get_state,
	.state_change_callbacks_enabled = can_renesas_rx_state_change_callbacks_enabled,
	.get_core_clock = can_renesas_rx_get_core_clock,
	.get_max_filters = can_renesas_rx_get_max_filters,
	.timing_min = CAN_RENESAS_RX_TIMING_MIN,
	.timing_max = CAN_RENESAS_RX_TIMING_MAX,
#if defined(CONFIG_CAN_FD_MODE)
	.set_timing_data = can_renesas_rx_set_timing_data,
	.timing_data_min = CAN_RENESAS_RX_TIMING_DATA_MIN,
	.timing_data_max = CAN_RENESAS_RX_TIMING_DATA_MAX,
#endif /* CONFIG_CAN_FD_MODE */
};

#define CAN_RENESAS_RX_GLOBAL_INIT(id)                                                             \
	static struct can_renesas_rx_global_data can_renesas_rx_global_data##id = {                \
		.fsp_canfdg_global_cfg =                                                            \
			{                                                                          \
				.global_interrupts = CANFDG_CFG_GLERR_IRQ,                          \
				.global_config = CANFDG_CFG_GLOBAL,                                 \
				.rx_fifo_config = CANFDG_CFG_RXFIFO,                                \
				.rx_mb_config = CANFDG_CFG_RXMB,                                    \
				.global_err_ipl = DT_IRQ_BY_NAME(id, glei, priority),              \
				.rx_fifo_ipl = DT_IRQ_BY_NAME(id, rfri, priority),                 \
				.common_fifo_config = CANFDG_CFG_COMMONFIFO,                        \
			},                                                                         \
		.rfri_num = DT_IRQ_BY_NAME(id, rfri, irq),                                         \
		.glei_num = DT_IRQ_BY_NAME(id, glei, irq),                                         \
		.can_global_intc = DEVICE_DT_GET(DT_IRQ_INTC_BY_NAME(id, rfri)),                   \
	};                                                                                         \
                                                                                                   \
	static const struct can_renesas_rx_global_cfg can_renesas_rx_global_cfg##id = {            \
		.op_clk = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(id, opclk)),                        \
		.ram_clk = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(id, ramclk)),                      \
		.op_subsys =                                                                       \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL_BY_NAME(id, opclk, mstp),                   \
				.stop_bit = DT_CLOCKS_CELL_BY_NAME(id, opclk, stop_bit),           \
			},                                                                         \
		.ram_subsys =                                                                      \
			{                                                                          \
				.mstp = DT_CLOCKS_CELL_BY_NAME(id, ramclk, mstp),                  \
				.stop_bit = DT_CLOCKS_CELL_BY_NAME(id, ramclk, stop_bit),          \
			},                                                                         \
		.dll_min_freq = DT_PROP_OR(id, dll_min_freq, 0),                                   \
		.dll_max_freq = DT_PROP_OR(id, dll_max_freq, 80000000),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(id, can_renesas_rx_global_init, NULL, &can_renesas_rx_global_data##id,    \
			 &can_renesas_rx_global_cfg##id, PRE_KERNEL_2, CONFIG_CAN_INIT_PRIORITY,   \
			 NULL)

DT_FOREACH_STATUS_OKAY(renesas_rx_canfdg_global, CAN_RENESAS_RX_GLOBAL_INIT)

#define CAN_RENESAS_RX_INIT(index)                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static canfdg_afl_entry_t canfdg_afl##index[DT_INST_PROP(index, rx_max_filters)] = {0};      \
	struct can_renesas_rx_filter                                                               \
		can_renesas_rx_rx_filter##index[DT_INST_PROP(index, rx_max_filters)];              \
	static const struct can_renesas_rx_cfg can_renesas_rx_cfg##index = {                       \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(index, 0, 5000000),                        \
		.global_dev = DEVICE_DT_GET(DT_INST_PARENT(index)),                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.dll_clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(index, dllclk)),              \
		.dll_subsys =                                                                      \
			{                                                                          \
				.mstp = DT_INST_CLOCKS_CELL_BY_NAME(index, dllclk, mstp),          \
				.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, dllclk, stop_bit),  \
			},                                                                         \
		.rx_filter_num = DT_INST_PROP(index, rx_max_filters),                              \
	};                                                                                         \
	static struct can_renesas_rx_data can_renesas_rx_data##index = {                           \
		.chei_num = DT_INST_IRQ_BY_NAME(index, chei, irq),                                 \
		.chri_num = DT_INST_IRQ_BY_NAME(index, chri, irq),                                 \
		.chti_num = DT_INST_IRQ_BY_NAME(index, chti, irq),                                 \
		.can_intc = DEVICE_DT_GET(DT_IRQ_INTC_BY_NAME(DT_DRV_INST(index), chei)),          \
		.fsp_can =                                                                         \
			{                                                                          \
				.p_ctrl = &can_renesas_rx_data##index.fsp_canfdg_ctrl,              \
				.p_cfg = &can_renesas_rx_data##index.fsp_can_cfg,                  \
				.p_api = &g_canfd_on_canfdg,                                        \
			},                                                                         \
		.fsp_can_cfg =                                                                     \
			{                                                                          \
				.channel = DT_INST_PROP(index, channel),                           \
				.p_bit_timing = &can_renesas_rx_data##index.bit_timing,            \
				.p_callback = can_renesas_rx_fsp_cb,                               \
				.p_context = (void *)DEVICE_DT_INST_GET(index),                    \
				.p_extend = &can_renesas_rx_data##index.fsp_canfdg_extend,          \
				.ipl = 1,                                                          \
				.error_irq =                                                       \
					(IRQn_Type)(DT_INST_IRQ_BY_NAME(index, chei, irq) << 8 |   \
						    CANFDG_GROUP_IRQ),                              \
				.rx_irq = (IRQn_Type)(DT_INST_IRQ_BY_NAME(index, chri, irq) << 8 | \
						      CANFDG_GROUP_IRQ),                            \
				.tx_irq = (IRQn_Type)(DT_INST_IRQ_BY_NAME(index, chti, irq) << 8 | \
						      CANFDG_GROUP_IRQ),                            \
			},                                                                         \
		.fsp_canfdg_extend =                                                                \
			{                                                                          \
				.p_afl = canfdg_afl##index,                                         \
				.txmb_txi_enable = CANFDG_CFG_TXMB_TXI_ENABLE,                      \
				.error_interrupts = 0U,                                            \
				.p_data_timing = &can_renesas_rx_data##index.data_timing,          \
			},                                                                         \
		.rx_filter = can_renesas_rx_rx_filter##index,                                      \
	};                                                                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(index, can_renesas_rx_init, NULL, &can_renesas_rx_data##index,   \
				  &can_renesas_rx_cfg##index, POST_KERNEL,                         \
				  CONFIG_CAN_INIT_PRIORITY, &can_renesas_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_RENESAS_RX_INIT)
