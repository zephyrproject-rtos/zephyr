/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/dt-bindings/clock/ra_clock.h>
#include "r_can_api.h"
#include "r_canfd.h"

LOG_MODULE_REGISTER(can_renesas_ra, CONFIG_CAN_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_canfd

#define CAN_RENESAS_RA_TIMING_MAX                                                                  \
	{                                                                                          \
		.sjw = 128, .prop_seg = 1, .phase_seg1 = 255, .phase_seg2 = 128,                   \
		.prescaler = 1024,                                                                 \
	}

#define CAN_RENESAS_RA_TIMING_MIN                                                                  \
	{                                                                                          \
		.sjw = 1, .prop_seg = 1, .phase_seg1 = 2, .phase_seg2 = 2, .prescaler = 1,         \
	}

#if CONFIG_CAN_FD_MODE
#define CAN_RENESAS_RA_TIMING_DATA_MAX                                                             \
	{                                                                                          \
		.sjw = 16, .prop_seg = 1, .phase_seg1 = 31, .phase_seg2 = 16, .prescaler = 128     \
	}
#define CAN_RENESAS_RA_TIMING_DATA_MIN                                                             \
	{                                                                                          \
		.sjw = 1, .prop_seg = 1, .phase_seg1 = 2, .phase_seg2 = 2, .prescaler = 1          \
	}
#endif

static const can_bit_timing_cfg_t classic_can_data_timing_default = {
	.baud_rate_prescaler = 1,
	.time_segment_1 = 3,
	.time_segment_2 = 2,
	.synchronization_jump_width = 1,
};

struct can_renesas_ra_custom_cfg {
	const struct device *global_dev;
	const struct pinctrl_dev_config *pcfg;
	const struct device *dll_clk;
	const struct clock_control_ra_subsys_cfg dll_subsys;
	const uint32_t rx_filter_num;
};

struct can_renesas_ra_global_cfg {
	const struct device *op_clk;
	const struct device *ram_clk;
	const struct clock_control_ra_subsys_cfg op_subsys;
	const struct clock_control_ra_subsys_cfg ram_subsys;
};

struct can_renesas_ra_filter {
	/* RX Acceptance Filter*/
	bool set;
	struct can_filter filter;

	/* RX user callback */
	can_rx_callback_t rx_cb;
	void *rx_usr_data;
};

struct can_renesas_ra_custom_data {
	/* CANFD device instance pointer */
	struct device *dev;

	/* Renesas RA FSP data */
	can_instance_t fsp_can;

	/* TX user callback */
	can_tx_callback_t tx_cb;
	void *tx_usr_data;
	struct k_work_delayable tx_timeout_work;

	/* RX callback */
	struct can_renesas_ra_filter *rx_filter;

	/* Bitrate timing configuration */
	can_bit_timing_cfg_t data_timing;
};

struct can_renesas_ra_cfg {
	struct can_driver_config common;
	struct can_renesas_ra_custom_cfg custom;
};

struct can_renesas_ra_data {
	struct can_driver_data common;
	struct can_renesas_ra_custom_data custom;
};

/* Configuration macro */
#define CANFD_CFG_COMMONFIFO0                                                                      \
	{                                                                                          \
		(((0) << R_CANFD_CFDCFCC_CFE_Pos) | ((0) << R_CANFD_CFDCFCC_CFRXIE_Pos) |          \
		 ((0) << R_CANFD_CFDCFCC_CFTXIE_Pos) | ((0) << R_CANFD_CFDCFCC_CFPLS_Pos) |        \
		 ((0) << R_CANFD_CFDCFCC_CFM_Pos) | ((0) << R_CANFD_CFDCFCC_CFITSS_Pos) |          \
		 ((0) << R_CANFD_CFDCFCC_CFITR_Pos) |                                              \
		 ((R_CANFD_CFDRFCC_RFIE_Msk) << R_CANFD_CFDCFCC_CFIM_Pos) |                        \
		 ((3U) << R_CANFD_CFDCFCC_CFIGCV_Pos) | (0 << R_CANFD_CFDCFCC_CFTML_Pos) |         \
		 ((3) << R_CANFD_CFDCFCC_CFDC_Pos) | (0 << R_CANFD_CFDCFCC_CFITT_Pos))             \
	}

#define CANFD_CFG_RXFIFO                                                                           \
	{                                                                                          \
		[0] = ((3U) << R_CANFD_CFDRFCC_RFIGCV_Pos) | ((3) << R_CANFD_CFDRFCC_RFDC_Pos) |   \
		      ((7) << R_CANFD_CFDRFCC_RFPLS_Pos) |                                         \
		      ((R_CANFD_CFDRFCC_RFIE_Msk | R_CANFD_CFDRFCC_RFIM_Msk)) | ((1)),             \
		[1] = ((3U) << R_CANFD_CFDRFCC_RFIGCV_Pos) | ((2) << R_CANFD_CFDRFCC_RFDC_Pos) |   \
		      ((7) << R_CANFD_CFDRFCC_RFPLS_Pos) |                                         \
		      ((R_CANFD_CFDRFCC_RFIE_Msk | R_CANFD_CFDRFCC_RFIM_Msk)) | ((0))              \
	}

#define CANFD_CFG_GLOBAL                                                                           \
	((R_CANFD_CFDGCFG_TPRI_Msk) | (R_CANFD_CFDGCFG_DCE_Msk) |                                  \
	 (BSP_CFG_CANFDCLK_SOURCE == BSP_CLOCKS_SOURCE_CLOCK_MAIN_OSC ? R_CANFD_CFDGCFG_DCS_Msk    \
								      : 0U) |                      \
	 (0))

#define CANFD_CFG_TXMB_TXI_ENABLE ((1ULL << 0) | (1ULL << 1) | (1ULL << 2) | (1ULL << 3) | 0ULL)

#define CANFD_CFG_RXMB (0 | ((7) << R_CANFD_CFDRMNB_RMPLS_Pos))

#define CANFD_CFG_ERR_IRQ                                                                          \
	(R_CANFD_CFDC_CTR_EWIE_Msk | R_CANFD_CFDC_CTR_EPIE_Msk | R_CANFD_CFDC_CTR_BOEIE_Msk |      \
	 R_CANFD_CFDC_CTR_BORIE_Msk | R_CANFD_CFDC_CTR_OLIE_Msk | 0U)

#define CANFD_CFG_GLERR_IRQ (0x3)

/* Declare HAL driver implementations */
static int can_renesas_ra_get_capabilities(const struct device *dev, can_mode_t *cap);
static int can_renesas_ra_start(const struct device *dev);
static int can_renesas_ra_stop(const struct device *dev);
static int can_renesas_ra_set_mode(const struct device *dev, can_mode_t mode);
static int can_renesas_ra_set_timing(const struct device *dev, const struct can_timing *timing);
static int can_renesas_ra_send(const struct device *dev, const struct can_frame *frame,
			       k_timeout_t timeout, can_tx_callback_t callback, void *user_data);
static int can_renesas_ra_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
					void *user_data, const struct can_filter *filter);
static void can_renesas_ra_remove_rx_filter(const struct device *dev, int filter_id);
#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
static int can_renesas_ra_recover(const struct device *dev, k_timeout_t timeout);
#endif
static int can_renesas_ra_get_state(const struct device *dev, enum can_state *state,
				    struct can_bus_err_cnt *err_cnt);
static void can_renesas_ra_get_state_change_callback(const struct device *dev,
						     can_state_change_callback_t callback,
						     void *user_data);
static int can_renesas_ra_get_core_clock(const struct device *dev, uint32_t *rate);
static int can_renesas_ra_get_max_filters(const struct device *dev, bool ide);
#if defined(CONFIG_CAN_FD_MODE)
static int can_renesas_ra_set_timing_data(const struct device *dev,
					  const struct can_timing *timing_data);
#endif

/* Re-declare FSP interrupt handler */
extern void canfd_error_isr(void);
extern void canfd_rx_fifo_isr(void);
extern void canfd_common_fifo_rx_isr(void);
extern void canfd_channel_tx_isr(void);

/* Unitily macros */
#define FSP_TO_ZEPHYR_FRAME(f_frame, z_frame)                                                      \
	{                                                                                          \
		struct can_frame *p_zephyr = z_frame;                                              \
		can_frame_t *p_fsp = f_frame;                                                      \
		p_zephyr->dlc = can_bytes_to_dlc(p_fsp->data_length_code);                         \
		p_zephyr->id = p_fsp->id;                                                          \
		p_zephyr->flags =                                                                  \
			(p_fsp->id_mode == CAN_ID_MODE_EXTENDED ? CAN_FRAME_IDE : 0) |             \
			(p_fsp->type == CAN_FRAME_TYPE_REMOTE ? CAN_FRAME_RTR : 0) |               \
			(p_fsp->options & CANFD_FRAME_OPTION_FD ? CAN_FRAME_FDF : 0) |             \
			(p_fsp->options & CANFD_FRAME_OPTION_ERROR ? CAN_FRAME_ESI : 0) |          \
			(p_fsp->options & CANFD_FRAME_OPTION_BRS ? CAN_FRAME_BRS : 0);             \
		memcpy(&p_zephyr->data, &p_fsp->data, can_dlc_to_bytes(p_fsp->data_length_code));  \
	}

#define ZEPHYR_TO_FSP_FRAME(f_frame, z_frame)                                                      \
	{                                                                                          \
		f_frame.id = z_frame->id;                                                          \
		f_frame.id_mode = (z_frame->flags & CAN_FRAME_IDE) ? CAN_ID_MODE_EXTENDED          \
								   : CAN_ID_MODE_STANDARD;         \
		f_frame.type = (z_frame->flags & CAN_FRAME_RTR) ? CAN_FRAME_TYPE_REMOTE            \
								: CAN_FRAME_TYPE_DATA;             \
		f_frame.data_length_code = can_dlc_to_bytes(z_frame->dlc);                         \
		f_frame.options =                                                                  \
			((z_frame->flags & CAN_FRAME_FDF) ? CANFD_FRAME_OPTION_FD : 0) |           \
			((z_frame->flags & CAN_FRAME_BRS) ? CANFD_FRAME_OPTION_BRS : 0) |          \
			((z_frame->flags & CAN_FRAME_ESI) ? CANFD_FRAME_OPTION_ERROR : 0);         \
		memcpy(&f_frame.data, &z_frame->data, can_dlc_to_bytes(z_frame->dlc));             \
	}

#define ZEPHYR_TO_FSP_TIMING(z_timing)                                                             \
	(can_bit_timing_cfg_t)                                                                     \
	{                                                                                          \
		.baud_rate_prescaler = ((struct can_timing *)(z_timing))->prescaler,               \
		.time_segment_1 = ((struct can_timing *)(z_timing))->prop_seg +                    \
				  ((struct can_timing *)(z_timing))->phase_seg1,                   \
		.time_segment_2 = ((struct can_timing *)(z_timing))->phase_seg2,                   \
		.synchronization_jump_width = ((struct can_timing *)(z_timing))->sjw,              \
	}

#define ZEPHYR_TO_FSP_RX_FILTER(f_filter, z_filter, filter_id, f_type)                             \
	{                                                                                          \
		canfd_afl_entry_t *p_afl = (canfd_afl_entry_t *)f_filter;                          \
		struct can_filter *p_filter = (struct can_filter *)z_filter;                       \
		p_afl->id.id = p_filter->id;                                                       \
		p_afl->id.frame_type = f_type;                                                     \
		p_afl->id.id_mode = (p_filter->flags & CAN_FILTER_IDE) ? CAN_ID_MODE_EXTENDED      \
								       : CAN_ID_MODE_STANDARD;     \
		p_afl->mask.mask_id = p_filter->mask;                                              \
		p_afl->mask.mask_frame_type = f_type;                                              \
		p_afl->mask.mask_id_mode = (p_filter->flags & CAN_FILTER_IDE)                      \
						   ? CAN_ID_MODE_EXTENDED                          \
						   : CAN_ID_MODE_STANDARD;                         \
		p_afl->destination.rx_buffer = (canfd_rx_mb_t)(CANFD_RX_MB_0 + filter_id);         \
		p_afl->destination.fifo_select_flags = CANFD_RX_FIFO_0;                            \
	}

#define CAN_RENESAS_RA_AFL_REMOVE(p_afl)                                                           \
	*(canfd_afl_entry_t *)p_afl = (canfd_afl_entry_t)                                          \
	{                                                                                          \
		.id =                                                                              \
			{                                                                          \
				.id = CAN_EXT_ID_MASK,                                             \
				.id_mode = CAN_ID_MODE_EXTENDED,                                   \
			},                                                                         \
		.mask = {                                                                          \
			.mask_id = CAN_EXT_ID_MASK,                                                \
			.mask_id_mode = CAN_ID_MODE_EXTENDED,                                      \
		},                                                                                 \
	}

#define CAN_RENESAS_RA_FILTER_ID_TO_AFL_OFFSET(id, frame_type)                                     \
	(2 * id + ((frame_type == CAN_FRAME_TYPE_DATA) ? 0 : 1))

#define CAN_RENESAS_RA_AFL_LOCK(reg)                                                               \
	{                                                                                          \
		reg->CFDGAFLIGNCTR = 0xC400;                                                       \
		reg->CFDGAFLECTR_b.AFLDAE = 0;                                                     \
	}

#define CAN_RENESAS_RA_AFL_UNLOCK(reg)                                                             \
	{                                                                                          \
		reg->CFDGAFLECTR_b.AFLDAE = 1;                                                     \
		reg->CFDGAFLIGNCTR = 0xC401;                                                       \
	}

/* Internal function declarations */
static inline void can_renesas_ra_call_tx_cb(const struct device *dev);
static inline void can_renesas_ra_call_rx_cb(const struct device *dev, struct can_frame *rx_frame);
static inline void can_renesas_ra_call_state_change_cb(const struct device *dev,
						       enum can_state state);
static void can_renesas_ra_tx_timeout_handler(struct k_work *work);
static inline int can_renesas_ra_apply_default_config(const struct device *dev);

/* Subsys APIs */
static const struct can_driver_api can_renesas_ra_driver_api = {
	.get_capabilities = can_renesas_ra_get_capabilities,
	.start = can_renesas_ra_start,
	.stop = can_renesas_ra_stop,
	.set_mode = can_renesas_ra_set_mode,
	.set_timing = can_renesas_ra_set_timing,
	.send = can_renesas_ra_send,
	.add_rx_filter = can_renesas_ra_add_rx_filter,
	.remove_rx_filter = can_renesas_ra_remove_rx_filter,
#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
	.recover = can_renesas_ra_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.get_state = can_renesas_ra_get_state,
	.set_state_change_callback = can_renesas_ra_get_state_change_callback,
	.get_core_clock = can_renesas_ra_get_core_clock,
	.get_max_filters = can_renesas_ra_get_max_filters,
	.timing_min = CAN_RENESAS_RA_TIMING_MIN,
	.timing_max = CAN_RENESAS_RA_TIMING_MAX,
#if defined(CONFIG_CAN_FD_MODE)
	.set_timing_data = can_renesas_ra_set_timing_data,
	.timing_data_min = CAN_RENESAS_RA_TIMING_DATA_MIN,
	.timing_data_max = CAN_RENESAS_RA_TIMING_DATA_MAX,
#endif /* CONFIG_CAN_FD_MODE */
};

static const can_api_t *can_api = &g_canfd_on_canfd;

/**************************************************************************************************
 * HW configuration routines
 **************************************************************************************************/

static inline int get_free_filter_id(const struct device *dev)
{
	struct can_renesas_ra_data *data = dev->data;
	const struct can_renesas_ra_cfg *cfg = dev->config;

	for (uint32_t filter_id = 0; filter_id < cfg->custom.rx_filter_num; filter_id++) {
		if (!data->custom.rx_filter[filter_id].set) {
			return filter_id;
		}
	}

	return -ENOSPC;
}

static inline bool r_canfd_bit_timing_parameter_check(can_bit_timing_cfg_t *const p_bit_timing,
						      bool is_data_phase)
{
	/* Check that prescaler is in range */
	if (!((p_bit_timing->baud_rate_prescaler <= 1024U) &&
	      (p_bit_timing->baud_rate_prescaler >= 1U))) {
		return false;
	}

#if CONFIG_CAN_FD_MODE
	if (is_data_phase) {
		/* Check Time Segment 1 is greater than or equal to Time Segment 2 */
		if (!((uint32_t)p_bit_timing->time_segment_1 >=
		      (uint32_t)p_bit_timing->time_segment_2)) {
			return false;
		}
	} else
#else

	/* Data phase is only available for FD mode. */
	ARG_UNUSED(is_data_phase);
#endif
	{
		/* Check Time Segment 1 is greater than Time Segment 2 */
		if (!((uint32_t)p_bit_timing->time_segment_1 >
		      (uint32_t)p_bit_timing->time_segment_2)) {
			return false;
		}
	}

	/* Check Time Segment 2 is greater than or equal to the synchronization jump width */
	if (!((uint32_t)p_bit_timing->time_segment_2 >=
	      (uint32_t)p_bit_timing->synchronization_jump_width)) {
		return false;
	}

	return true;
}

static inline void set_afl_rule(const struct device *dev, const struct can_filter *filter,
				uint32_t filter_id, can_frame_type_t frametype)
{
	struct can_renesas_ra_data *data = dev->data;
	canfd_instance_ctrl_t *p_ctrl = (canfd_instance_ctrl_t *)data->custom.fsp_can.p_ctrl;
	can_cfg_t *p_cfg = (can_cfg_t *)data->custom.fsp_can.p_cfg;
	canfd_extended_cfg_t *p_extend = (canfd_extended_cfg_t *)p_cfg->p_extend;
	R_CANFD_Type *reg = p_ctrl->p_reg;
	uint32_t filter_offset = CAN_RENESAS_RA_FILTER_ID_TO_AFL_OFFSET(filter_id, frametype);
	volatile R_CANFD_CFDGAFL_Type *cfdgafl = &reg->CFDGAFL[filter_offset];

	/* Convert the Zephyr filter to HW configuration */
	ZEPHYR_TO_FSP_RX_FILTER(&p_extend->p_afl[filter_offset], filter, filter_id, frametype);

	/* Ignore AFL entry which will be changed */
	reg->CFDGAFLIGNENT_b.IRN = filter_offset;

	/* Write AFL configuration */
	CAN_RENESAS_RA_AFL_UNLOCK(reg);
	*(cfdgafl) = *((R_CANFD_CFDGAFL_Type *)&p_extend->p_afl[filter_offset]);
	CAN_RENESAS_RA_AFL_LOCK(reg);
}

static inline void remove_afl_rule(const struct device *dev, uint32_t filter_id,
				   can_frame_type_t frametype)
{
	struct can_renesas_ra_data *data = dev->data;
	canfd_instance_ctrl_t *p_ctrl = (canfd_instance_ctrl_t *)data->custom.fsp_can.p_ctrl;
	can_cfg_t *p_cfg = (can_cfg_t *)data->custom.fsp_can.p_cfg;
	canfd_extended_cfg_t *p_extend = (canfd_extended_cfg_t *)p_cfg->p_extend;
	R_CANFD_Type *reg = p_ctrl->p_reg;
	uint32_t filter_offset = CAN_RENESAS_RA_FILTER_ID_TO_AFL_OFFSET(filter_id, frametype);

	/* Ignore AFL entry which will be changed */
	reg->CFDGAFLIGNENT_b.IRN = filter_offset;

	/* Unlock AFL */
	CAN_RENESAS_RA_AFL_UNLOCK(reg);
	CAN_RENESAS_RA_AFL_REMOVE(&reg->CFDGAFL[filter_offset]);
	CAN_RENESAS_RA_AFL_LOCK(reg);

	/* Synchronize updates with instance data */
	CAN_RENESAS_RA_AFL_REMOVE(&p_extend->p_afl[filter_offset]);
}

static inline int recover_bus(const struct device *dev)
{
	struct can_renesas_ra_data *data = dev->data;
	canfd_instance_ctrl_t *p_ctrl = data->custom.fsp_can.p_ctrl;
	R_CANFD_Type *reg = p_ctrl->p_reg;

	if (reg->CFDC->ERFL_b.BOEF == 0) {
		return 0;
	}

	/* Change state from bus-off to error active */
	uint32_t cfdcnctr = reg->CFDC->CTR;

	reg->CFDC->CTR_b.BOM = 0x00;
	reg->CFDC->CTR_b.RTBO = 1;

	uint8_t timeout = UINT8_MAX;

	while ((reg->CFDC->ERFL_b.BORF != 1) && likely(--timeout))
		;

	/* Restore value of CFDCnCTR */
	reg->CFDC->CTR = cfdcnctr;

	return timeout > 0 ? 0 : -EIO;
}

/**************************************************************************************************
 * Subsys APIs implementation
 *************************************************************************************************/

/**
 * Get the supported modes of the CAN controller
 */
static int can_renesas_ra_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);
	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK;

#if CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif

#if CONFIG_CAN_MANUAL_RECOVERY_MODE
	*cap |= CAN_MODE_MANUAL_RECOVERY;
#endif

	return 0;
}

/**
 * Start the CAN controller
 */
static int can_renesas_ra_start(const struct device *dev)
{
	struct can_renesas_ra_data *data = dev->data;
	const struct device *transceiver_dev = can_get_transceiver(dev);

	if (!device_is_ready(dev)) {
		return -EIO;
	}

	if (data->common.started) {
		return -EALREADY;
	}

	if (transceiver_dev && can_transceiver_enable(transceiver_dev, data->common.mode)) {
		LOG_DBG("drivers: can: transceiver: can transceiver enable failed");
		return -EIO;
	}

	fsp_err_t fsp_err;
	canfd_extended_cfg_t *p_extend =
		(canfd_extended_cfg_t *)data->custom.fsp_can.p_cfg->p_extend;

	fsp_err = can_api->close(data->custom.fsp_can.p_ctrl);
	p_extend->p_data_timing =
		(data->common.mode & CAN_MODE_FD)
			? (can_bit_timing_cfg_t *)&data->custom.data_timing
			: (can_bit_timing_cfg_t *)&classic_can_data_timing_default;
	fsp_err |= can_api->open(data->custom.fsp_can.p_ctrl, data->custom.fsp_can.p_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: can: can reset failed");
		return -EIO;
	}

	if (data->common.mode & CAN_MODE_LOOPBACK) {
		fsp_err = can_api->modeTransition(data->custom.fsp_can.p_ctrl,
						  CAN_OPERATION_MODE_NORMAL,
						  CAN_TEST_MODE_LOOPBACK_INTERNAL);
		if (fsp_err != FSP_SUCCESS) {
			LOG_DBG("drivers: can: can mode change failed");
			return -EIO;
		}
	}

	data->common.started = true;

	return 0;
}

/**
 * Stop the CAN controller
 */
static int can_renesas_ra_stop(const struct device *dev)
{
	struct can_renesas_ra_data *data = dev->data;
	const struct device *transceiver_dev = can_get_transceiver(dev);
	fsp_err_t fsp_err;

	if (!data->common.started) {
		return -EALREADY;
	}

	fsp_err = can_api->modeTransition(data->custom.fsp_can.p_ctrl, CAN_OPERATION_MODE_HALT,
					  CAN_TEST_MODE_DISABLED);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: can: can stop failed");
		return -EIO;
	}

	if (transceiver_dev && can_transceiver_disable(transceiver_dev)) {
		LOG_DBG("drivers: can: transceiver: can transceiver disable failed");
		return -EIO;
	}

	data->common.started = false;
	can_renesas_ra_call_state_change_cb(dev, CAN_STATE_STOPPED);

	return 0;
}

/**
 * Set the CAN controller to the given operation mode
 */
static int can_renesas_ra_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_renesas_ra_data *data = dev->data;

	if (data->common.started) {
		/* CAN controller should be in stopped state */
		return -EBUSY;
	}
	data->common.mode = mode;

	return 0;
}

/**
 * Configure the bus timing of a CAN controller.
 */
static int can_renesas_ra_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_renesas_ra_data *data = dev->data;

	if (data->common.started) {
		/* Device is not in stopped state */
		return -EBUSY;
	}

	can_bit_timing_cfg_t bit_timing = ZEPHYR_TO_FSP_TIMING(timing);

	if (!r_canfd_bit_timing_parameter_check(&bit_timing, false)) {
		return -ENOTSUP;
	}
	*(data->custom.fsp_can.p_cfg->p_bit_timing) = bit_timing;

	return 0;
}

/**
 * Queue a CAN frame for transmission on the CAN bus with optional timeout and completion callback
 * function.
 */
static int can_renesas_ra_send(const struct device *dev, const struct can_frame *frame,
			       k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_renesas_ra_data *data = dev->data;

	if (!data->common.started) {
		/* CAN controller is in stopped state */
		callback(dev, -ENETDOWN, user_data);
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_FD) ? frame->dlc > CANFD_MAX_DLC
					      : frame->dlc > CAN_MAX_DLC) {
		callback(dev, -EINVAL, user_data);
		return -EINVAL;
	}

	if (!(data->common.mode & CAN_MODE_FD) && (frame->flags & CAN_FRAME_FDF)) {
		/* Device is not in CAN_MODE_FD */
		callback(dev, -ENOTSUP, user_data);
		return -ENOTSUP;
	}

	/* Store user callback to be called when transmission error occur */
	data->custom.tx_cb = callback;
	data->custom.tx_usr_data = user_data;

	fsp_err_t fsp_err;
	can_frame_t fsp_frame = {0};

	ZEPHYR_TO_FSP_FRAME(fsp_frame, frame);
	for (uint32_t i = 0; i < 4; i++) {
		fsp_err = can_api->write(data->custom.fsp_can.p_ctrl, (canfd_tx_buffer_t)i,
					 &fsp_frame);
		if (fsp_err == FSP_ERR_CAN_TRANSMIT_NOT_READY) {
			continue;
		} else {
			break;
		}
	}
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: can: can send failed");
		return -EIO;
	}
	if (timeout.ticks != 0 && timeout.ticks != K_TICKS_FOREVER) {
		k_work_reschedule(&data->custom.tx_timeout_work, timeout);
	}

	return 0;
}

/**
 * Add a callback function for a given CAN filter
 */
static int can_renesas_ra_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
					void *user_data, const struct can_filter *filter)
{
	struct can_renesas_ra_data *data = dev->data;
	int filter_id = get_free_filter_id(dev);

	if (filter_id == -ENOSPC) {
		return filter_id;
	}

	set_afl_rule(dev, filter, filter_id, CAN_FRAME_TYPE_DATA);
	set_afl_rule(dev, filter, filter_id, CAN_FRAME_TYPE_REMOTE);

	memcpy(&data->custom.rx_filter[filter_id].filter, filter, sizeof(struct can_filter));
	data->custom.rx_filter[filter_id].rx_cb = callback;
	data->custom.rx_filter[filter_id].rx_usr_data = user_data;
	data->custom.rx_filter[filter_id].set = true;

	return filter_id;
}

/**
 * Remove a CAN RX filter
 */
static void can_renesas_ra_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_renesas_ra_data *data = dev->data;
	const struct can_renesas_ra_cfg *cfg = dev->config;

	if (filter_id > cfg->custom.rx_filter_num) {
		return;
	}

	remove_afl_rule(dev, filter_id, CAN_FRAME_TYPE_DATA);
	remove_afl_rule(dev, filter_id, CAN_FRAME_TYPE_REMOTE);

	data->custom.rx_filter[filter_id].rx_cb = NULL;
	data->custom.rx_filter[filter_id].rx_usr_data = NULL;
	data->custom.rx_filter[filter_id].set = false;
}

#if CONFIG_CAN_MANUAL_RECOVERY_MODE
/**
 * Recover the CAN controller from bus-off state to error-active state.
 */
static int can_renesas_ra_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_renesas_ra_data *data = dev->data;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if (!(data->common.mode & CAN_MODE_MANUAL_RECOVERY)) {
		return -ENOTSUP;
	}

	return recover_bus(dev);
}
#endif

/**
 * Get current CAN controller state
 */
static int can_renesas_ra_get_state(const struct device *dev, enum can_state *state,
				    struct can_bus_err_cnt *err_cnt)
{
	struct can_renesas_ra_data *data = dev->data;

	if (!data->common.started) {
		*state = CAN_STATE_STOPPED;
	} else {
		fsp_err_t fsp_err;
		can_info_t fsp_info = {0};

		fsp_err = can_api->infoGet(data->custom.fsp_can.p_ctrl, &fsp_info);
		if (fsp_err != FSP_SUCCESS) {
			LOG_DBG("drivers: can: can get state failed");
			return -EIO;
		}
		err_cnt->tx_err_cnt = fsp_info.error_count_transmit;
		err_cnt->rx_err_cnt = fsp_info.error_count_receive;

		if (fsp_info.error_code & R_CANFD_CFDC_ERFL_BOEF_Msk) {
			*state = CAN_STATE_BUS_OFF;
		} else if (fsp_info.error_code & R_CANFD_CFDC_ERFL_EPF_Msk) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if (fsp_info.error_code & R_CANFD_CFDC_ERFL_EWF_Msk) {
			*state = CAN_STATE_ERROR_WARNING;
		} else if (fsp_info.error_code & R_CANFD_CFDC_ERFL_BEF_Msk) {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	return 0;
}

/**
 * Set a callback for CAN controller state change events
 */
static void can_renesas_ra_get_state_change_callback(const struct device *dev,
						     can_state_change_callback_t callback,
						     void *user_data)
{
	struct can_renesas_ra_data *data = dev->data;

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

/**
 * Get the CAN core clock rate.
 */
static int can_renesas_ra_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_renesas_ra_cfg *cfg = dev->config;
	clock_control_subsys_t dll_subsys = (clock_control_subsys_t)&cfg->custom.dll_subsys;

	return clock_control_get_rate(cfg->custom.dll_clk, dll_subsys, rate) ? -EIO : 0;
}

/**
 * Get maximum number of RX filters.
 */
static int can_renesas_ra_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);
	const struct can_renesas_ra_cfg *cfg = dev->config;

	return cfg->custom.rx_filter_num;
}

#if CONFIG_CAN_FD_MODE
/**
 * Configure the bus timing for the data phase of a CAN FD controller.
 */
static int can_renesas_ra_set_timing_data(const struct device *dev,
					  const struct can_timing *timing_data)
{
	struct can_renesas_ra_data *data = dev->data;

	if (data->common.started) {
		/* CAN controller is not in stopped state. */
		return -EBUSY;
	}

	can_bit_timing_cfg_t data_timing = ZEPHYR_TO_FSP_TIMING(timing_data);

	if (!r_canfd_bit_timing_parameter_check(&data_timing, true)) {
		return -ENOTSUP;
	}
	data->custom.data_timing = data_timing;

	return 0;
}
#endif /* CONFIG_CAN_FD_MODE */

/**************************************************************************************************
 * Internal function definition
 *************************************************************************************************/
void can_renesas_ra_fsp_cb(can_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;

	switch (p_args->event) {
	case CAN_EVENT_RX_COMPLETE: {
		struct can_frame z_frame = {0};

		FSP_TO_ZEPHYR_FRAME(&p_args->frame, &z_frame);
		can_renesas_ra_call_rx_cb(dev, &z_frame);
		break;
	}

	case CAN_EVENT_TX_COMPLETE: {
		can_renesas_ra_call_tx_cb(dev);
		break;
	}

	case CAN_EVENT_ERR_CHANNEL: {
		if (p_args->error & R_CANFD_CFDC_ERFL_BEF_Msk) {
			can_renesas_ra_call_state_change_cb(dev, CAN_STATE_ERROR_ACTIVE);
		}
		if (p_args->error & R_CANFD_CFDC_ERFL_EWF_Msk) {
			can_renesas_ra_call_state_change_cb(dev, CAN_STATE_ERROR_WARNING);
		}
		if (p_args->error & R_CANFD_CFDC_ERFL_EPF_Msk) {
			can_renesas_ra_call_state_change_cb(dev, CAN_STATE_ERROR_PASSIVE);
		}
		if (p_args->error & R_CANFD_CFDC_ERFL_BOEF_Msk) {
			can_renesas_ra_call_state_change_cb(dev, CAN_STATE_BUS_OFF);
		}
		break;
	}

	default:
		break;
	}
}

static inline void can_renesas_ra_call_tx_cb(const struct device *dev)
{
	struct can_renesas_ra_data *data = dev->data;

	data->custom.tx_cb(dev, 0, data->custom.tx_usr_data);
	k_work_cancel_delayable(&data->custom.tx_timeout_work);
}

static inline void can_renesas_ra_call_rx_cb(const struct device *dev, struct can_frame *rx_frame)
{
	struct can_renesas_ra_data *data = dev->data;
	const struct can_renesas_ra_cfg *cfg = dev->config;
	struct can_renesas_ra_filter *rx_filter = data->custom.rx_filter;

	for (uint32_t i = 0; i < cfg->custom.rx_filter_num; i++) {
		if (!rx_filter->rx_cb) {
			rx_filter++;
			continue; /* this filter has not set yet */
		}

		if (!can_frame_matches_filter(rx_frame, &rx_filter->filter)) {
			rx_filter++;
			continue; /* filter did not match */
		}

		rx_filter->rx_cb(dev, rx_frame, rx_filter->rx_usr_data);
		break;
	}
}

static inline void can_renesas_ra_call_state_change_cb(const struct device *dev,
						       enum can_state state)
{
	struct can_renesas_ra_data *data = dev->data;
	can_info_t can_info = {};
	fsp_err_t fsp_err;

	fsp_err = can_api->infoGet(data->custom.fsp_can.p_ctrl, &can_info);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("drivers: can: can get err info failed");
		return;
	}
	struct can_bus_err_cnt err_cnt = {
		.rx_err_cnt = can_info.error_count_receive,
		.tx_err_cnt = can_info.error_count_transmit,
	};

	if (data->common.state_change_cb == NULL) {
		LOG_DBG("drivers: can: state change callback is not set");
		return;
	}
	data->common.state_change_cb(dev, state, err_cnt, data->common.state_change_cb_user_data);
}

static void can_renesas_ra_tx_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct can_renesas_ra_custom_data *cus_data =
		CONTAINER_OF(dwork, struct can_renesas_ra_custom_data, tx_timeout_work);

	cus_data->tx_cb(cus_data->dev, -EAGAIN, cus_data->tx_usr_data);
}

static inline int can_renesas_ra_apply_default_config(const struct device *dev)
{
	struct can_renesas_ra_data *data = (struct can_renesas_ra_data *)dev->data;
	struct can_renesas_ra_cfg *cfg = (struct can_renesas_ra_cfg *)dev->config;
	int ret;

	can_cfg_t *p_cfg = (can_cfg_t *)data->custom.fsp_can.p_cfg;
	struct can_timing timing = {0};

	/* Calculate bit_timing parameter */
	can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);
	ret = can_renesas_ra_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}
#if CONFIG_CAN_FD_MODE
	can_calc_timing_data(dev, &timing, cfg->common.bitrate_data, cfg->common.sample_point_data);
	ret = can_renesas_ra_set_timing_data(dev, &timing);
	if (ret) {
		return ret;
	}
#endif

	for (uint32_t filter_id = 0; filter_id < cfg->custom.rx_filter_num; filter_id++) {
		remove_afl_rule(dev, filter_id, CAN_FRAME_TYPE_DATA);
		remove_afl_rule(dev, filter_id, CAN_FRAME_TYPE_REMOTE);
	}

	p_cfg->p_context = dev;
	p_cfg->p_callback = can_renesas_ra_fsp_cb;

	return 0;
}

static inline int can_renesas_module_clock_init(const struct device *dev)
{
	const struct can_renesas_ra_cfg *cfg = dev->config;
	const struct can_renesas_ra_global_cfg *global_cfg = cfg->custom.global_dev->config;
	int ret;

	ret = clock_control_on(cfg->custom.dll_clk,
			       (clock_control_subsys_t)&cfg->custom.dll_subsys);
	if (ret) {
		return -EIO;
	}
	uint32_t op_rate;
	uint32_t ram_rate;
	uint32_t dll_rate;

	ret = clock_control_get_rate(global_cfg->op_clk,
				     (clock_control_subsys_t)&global_cfg->op_subsys, &op_rate);
	ret |= clock_control_get_rate(global_cfg->ram_clk,
				      (clock_control_subsys_t)&global_cfg->ram_subsys, &ram_rate);
	ret |= clock_control_get_rate(cfg->custom.dll_clk,
				      (clock_control_subsys_t)&cfg->custom.dll_subsys, &dll_rate);
	if (ret) {
		return -EIO;
	}
	if (IS_ENABLED(CONFIG_CAN_FD_MODE) ? op_rate < 40000000 : op_rate < 32000000) {
		return -ENOTSUP;
	}
	if ((ram_rate / 2) < dll_rate || op_rate < dll_rate) {
		return -ENOTSUP;
	}

	return 0;
}

static int can_renesas_ra_init(const struct device *dev)
{
	struct can_renesas_ra_cfg *cfg = (struct can_renesas_ra_cfg *)dev->config;
	struct can_renesas_ra_data *data = (struct can_renesas_ra_data *)dev->data;
	int ret = 0;
	fsp_err_t err;

	data->common.started = false;

	ret = can_renesas_module_clock_init(dev);
	if (ret) {
		LOG_DBG("driver: can: clock initialize failed");
		return ret;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->custom.pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_DBG("driver: can: pin function initial failed: %d", ret);
		return ret;
	}

	/* Apply config and setting for CAN controller HW */
	ret = can_renesas_ra_apply_default_config(dev);
	if (ret) {
		LOG_DBG("driver: can: init: apply default configure failed: %d", ret);
		return ret;
	}
	err = can_api->open(data->custom.fsp_can.p_ctrl, data->custom.fsp_can.p_cfg);
	if (err != FSP_SUCCESS) {
		LOG_DBG("drivers: can: can bus initialize failed");
		return -EIO;
	}
	/* Put CAN controller into stopped state */
	err = can_api->modeTransition(data->custom.fsp_can.p_ctrl, CAN_OPERATION_MODE_HALT,
				      CAN_TEST_MODE_DISABLED);
	if (err != FSP_SUCCESS) {
		can_api->close(data->custom.fsp_can.p_ctrl);
		LOG_DBG("drivers: can: can bus initialize failed");
		return -EIO;
	}

	/* Store device instance, used by k_work of TX timeout event */
	data->custom.dev = (struct device *)dev;

	/* Initial kernel delayable work for CAN transmit */
	k_work_init_delayable(&data->custom.tx_timeout_work, can_renesas_ra_tx_timeout_handler);

	return 0;
}

#define CAN_RENESAS_RA_GLOBAL_IRQ_INIT()                                                           \
	R_ICU->IELSR_b[VECTOR_NUMBER_CAN_GLERR].IELS = ELC_EVENT_CAN_GLERR;                        \
	R_ICU->IELSR_b[VECTOR_NUMBER_CAN_RXF].IELS = ELC_EVENT_CAN_RXF;                            \
	IRQ_CONNECT(VECTOR_NUMBER_CAN_GLERR,                                                       \
		    DT_IRQ_BY_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), glerr,  \
				   priority),                                                      \
		    canfd_error_isr, NULL, 0);                                                     \
	IRQ_CONNECT(VECTOR_NUMBER_CAN_RXF,                                                         \
		    DT_IRQ_BY_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), rxf,    \
				   priority),                                                      \
		    canfd_rx_fifo_isr, NULL, 0);                                                   \
	irq_enable(VECTOR_NUMBER_CAN_RXF);                                                         \
	irq_enable(VECTOR_NUMBER_CAN_GLERR);

static canfd_global_cfg_t g_canfd_global_cfg = {
	.global_interrupts = CANFD_CFG_GLERR_IRQ,
	.global_config = CANFD_CFG_GLOBAL,
	.rx_mb_config = CANFD_CFG_RXMB,
	.global_err_ipl = DT_IRQ_BY_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global),
					 glerr, priority),
	.rx_fifo_ipl = DT_IRQ_BY_NAME(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), rxf,
				      priority),
	.rx_fifo_config = CANFD_CFG_RXFIFO,
	.common_fifo_config = CANFD_CFG_COMMONFIFO0,
};

static const struct can_renesas_ra_global_cfg g_can_renesas_ra_global_cfg = {
	.op_clk = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(
		DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), opclk)),
	.ram_clk = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(
		DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), ramclk)),
	.op_subsys = {.mstp = (uint32_t *)DT_CLOCKS_CELL_BY_NAME(
			      DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), opclk, mstp),
		      .stop_bit = DT_CLOCKS_CELL_BY_NAME(
			      DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), opclk,
			      stop_bit)},
	.ram_subsys = {.mstp = (uint32_t *)DT_CLOCKS_CELL_BY_NAME(
			       DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), ramclk,
			       mstp),
		       .stop_bit = DT_CLOCKS_CELL_BY_NAME(
			       DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), ramclk,
			       stop_bit)},
};

static int can_renesas_ra_global_init(const struct device *dev)
{
	int ret;
	const struct can_renesas_ra_global_cfg *cfg = dev->config;

	ret = clock_control_on(cfg->op_clk, (clock_control_subsys_t)&cfg->op_subsys);
	ret |= clock_control_on(cfg->ram_clk, (clock_control_subsys_t)&cfg->ram_subsys);
	if (ret) {
		LOG_DBG("driver: can: init: clock initialize failed");
		return ret;
	}
	CAN_RENESAS_RA_GLOBAL_IRQ_INIT();

	return 0;
}

DEVICE_DT_DEFINE(DT_COMPAT_GET_ANY_STATUS_OKAY(renesas_ra_canfd_global), can_renesas_ra_global_init,
		 NULL, NULL, &g_can_renesas_ra_global_cfg, PRE_KERNEL_2, CONFIG_CAN_INIT_PRIORITY,
		 NULL)

#define _ELC_EVENT_CAN_COMFRX(channel) ELC_EVENT_CAN##channel##_COMFRX
#define _ELC_EVENT_CAN_TX(channel)     ELC_EVENT_CAN##channel##_TX
#define _ELC_EVENT_CAN_CHERR(channel)  ELC_EVENT_CAN##channel##_CHERR

#define ELC_EVENT_CAN_COMFRX(channel) _ELC_EVENT_CAN_COMFRX(channel)
#define ELC_EVENT_CAN_TX(channel)     _ELC_EVENT_CAN_TX(channel)
#define ELC_EVENT_CAN_CHERR(channel)  _ELC_EVENT_CAN_CHERR(channel)

#define CAN_RENESAS_RA_CHANNEL_IRQ_INIT(index)                                                     \
	{                                                                                          \
		R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(index, rx, irq)].IELS =                         \
			ELC_EVENT_CAN_COMFRX(DT_INST_PROP(index, channel));                        \
		R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(index, tx, irq)].IELS =                         \
			ELC_EVENT_CAN_TX(DT_INST_PROP(index, channel));                            \
		R_ICU->IELSR_b[DT_INST_IRQ_BY_NAME(index, err, irq)].IELS =                        \
			ELC_EVENT_CAN_CHERR(DT_INST_PROP(index, channel));                         \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, rx, irq),                                   \
			    DT_INST_IRQ_BY_NAME(index, rx, priority), canfd_common_fifo_rx_isr,    \
			    NULL, 0);                                                              \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tx, irq),                                   \
			    DT_INST_IRQ_BY_NAME(index, tx, priority), canfd_channel_tx_isr, NULL,  \
			    0);                                                                    \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, err, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, err, priority), canfd_error_isr, NULL, 0);  \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, rx, irq));                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, tx, irq));                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(index, err, irq));                                  \
	}

#define CAN_RENESAS_RA_INIT(index)                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static canfd_afl_entry_t canfd_afl##index[DT_INST_PROP(index, rx_max_filters) * 2] = {0};  \
	struct can_renesas_ra_filter                                                               \
		can_renesas_ra_rx_filter##index[DT_INST_PROP(index, rx_max_filters)];              \
	static can_bit_timing_cfg_t g_canfd_bit_timing##index;                                     \
	static const struct can_renesas_ra_cfg can_renesas_ra_cfg##index = {                       \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(index, 500, 5000000),                      \
		.custom =                                                                          \
			{                                                                          \
				.global_dev = DEVICE_DT_GET(DT_INST_PARENT(index)),                \
				.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                     \
				.dll_clk =                                                         \
					DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(index, dllclk)), \
				.dll_subsys =                                                      \
					{                                                          \
						.mstp = (uint32_t *)DT_INST_CLOCKS_CELL_BY_NAME(   \
							index, dllclk, mstp),                      \
						.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(           \
							index, dllclk, stop_bit),                  \
					},                                                         \
				.rx_filter_num = DT_INST_PROP(index, rx_max_filters),              \
			},                                                                         \
	};                                                                                         \
	static canfd_instance_ctrl_t fsp_canfd_ctrl##index;                                        \
	static canfd_extended_cfg_t fsp_canfd_extend####index = {                                  \
		.p_afl = canfd_afl##index,                                                         \
		.txmb_txi_enable = CANFD_CFG_TXMB_TXI_ENABLE,                                      \
		.error_interrupts = CANFD_CFG_ERR_IRQ,                                             \
		.p_global_cfg = &g_canfd_global_cfg,                                               \
	};                                                                                         \
	static can_cfg_t fsp_canfd_cfg####index = {                                                \
		.channel = DT_INST_PROP(index, channel),                                           \
		.ipl = DT_INST_IRQ_BY_NAME(index, err, priority),                                  \
		.error_irq = DT_INST_IRQ_BY_NAME(index, err, irq),                                 \
		.rx_irq = DT_INST_IRQ_BY_NAME(index, rx, irq),                                     \
		.tx_irq = DT_INST_IRQ_BY_NAME(index, tx, irq),                                     \
		.p_extend = &fsp_canfd_extend##index,                                              \
		.p_bit_timing = &g_canfd_bit_timing##index,                                        \
	};                                                                                         \
	static struct can_renesas_ra_data can_renesas_ra_data##index = {                           \
		.custom =                                                                          \
			{                                                                          \
				.fsp_can =                                                         \
					{                                                          \
						.p_ctrl = &fsp_canfd_ctrl##index,                  \
						.p_cfg = &fsp_canfd_cfg##index,                    \
						.p_api = &g_canfd_on_canfd,                        \
					},                                                         \
				.rx_filter = can_renesas_ra_rx_filter##index,                      \
			},                                                                         \
	};                                                                                         \
	static int can_renesas_ra_init##index(const struct device *dev)                            \
	{                                                                                          \
		const struct device *global_canfd = DEVICE_DT_GET(DT_INST_PARENT(index));          \
		if (!device_is_ready(global_canfd)) {                                              \
			return -EIO;                                                               \
		}                                                                                  \
		CAN_RENESAS_RA_CHANNEL_IRQ_INIT(index)                                             \
		return can_renesas_ra_init(dev);                                                   \
	}                                                                                          \
	CAN_DEVICE_DT_INST_DEFINE(index, can_renesas_ra_init##index, NULL,                         \
				  &can_renesas_ra_data##index, &can_renesas_ra_cfg##index,         \
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,                           \
				  &can_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_RENESAS_RA_INIT)
