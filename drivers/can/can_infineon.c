/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief CAN driver for Infineon CAT1 MCU family.
 *
 */

#define DT_DRV_COMPAT infineon_can

#include <string.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include <infineon_kconfig.h>
#include <cy_canfd.h>
#include <cy_gpio.h>
#include <cy_syslib.h>
#include <cy_sysint.h>
#include <cy_device_headers.h>

#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_ifx_cat1, CONFIG_CAN_LOG_LEVEL);

#define STD_FILTER_OFFSET 0
#define EXT_FILTER_OFFSET CONFIG_CAN_INFINEON_MAX_FILTER
#define TOTAL_FILTERS     (2 * CONFIG_CAN_INFINEON_MAX_FILTER)

#define CANFD_BLOCK 0U

static int configure(const struct device *dev);

struct ifx_cat1_can_tx_callback {
	can_tx_callback_t function;
	void *user_data;
};

struct ifx_cat1_can_rx_callback {
	can_rx_callback_t function;
	void *user_data;
};

struct ifx_cat1_can_data {
	struct can_driver_data common;
	uint8_t can_ch_idx;

	struct k_sem operation_sem;
	struct k_sem tx_sem;
	struct k_mutex mutex;

	enum can_state state;
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	uint8_t clock_peri_group;
#endif
	/* CANFD Configuration struct */
	cy_stc_canfd_context_t ctx;
	cy_stc_canfd_config_t canfd_config;
	cy_stc_canfd_bitrate_t nominal_config;
	cy_stc_canfd_bitrate_t fast_config;
	cy_en_canfd_fifo_config_t fifo0_config;
	cy_en_canfd_fifo_config_t fifo1_config;
	cy_stc_canfd_transceiver_delay_compensation_t tdc_config;
	/* Config structures for standard filters */
	cy_stc_id_filter_t std_id_filters[CONFIG_CAN_INFINEON_MAX_FILTER];
	cy_stc_canfd_sid_filter_config_t sid_filters_config;
	/* Config structures for extended filters */
	cy_stc_extid_filter_t ext_id_filters[CONFIG_CAN_INFINEON_MAX_FILTER];
	cy_stc_canfd_extid_filter_config_t extid_filters_config;
	/* Config structures for global filters */
	cy_stc_canfd_global_filter_config_t global_filters_config;
	/* Config structures for TX and RX callback */
	struct ifx_cat1_can_tx_callback tx_callback_data[CONFIG_CAN_INFINEON_MAX_TX_QUEUE];
	struct ifx_cat1_can_rx_callback rx_callback_data[TOTAL_FILTERS];
	/* End CANFD configuration structures */

	uint32_t std_filter_count;
	uint32_t ext_filter_count;
	bool std_filters_ready;
	bool ext_filters_ready;
	/* Temporary variables for filter setup */
	cy_stc_id_filter_t temp_std_filters;
	cy_stc_canfd_f0_t temp_f0;
	cy_stc_canfd_f1_t temp_f1;
	cy_stc_extid_filter_t temp_ext_filter;
};

struct ifx_cat1_can_config {
	struct can_driver_config cfg_common;
	/* Clock configuration */
	cy_en_divider_types_t divider_type;
	uint32_t divider_sel;
	uint32_t divider_val;

	const struct pinctrl_dev_config *pcfg;
	uint8_t irq_priority;
	uint32_t irq_num;
	CANFD_CH_Type *ch_addr;
	CANFD_Type *base;
	void (*irq_config_func)(const struct device *dev);
	cy_canfd_tx_msg_func_ptr_t tx_canfd_callback;
	cy_canfd_rx_msg_func_ptr_t rx_canfd_callback;
	cy_canfd_error_func_ptr_t error_canfd_callback;
};

static void can_isr_handler(const struct device *dev)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;

	/* Handle CANFD interrupt */
	Cy_CANFD_IrqHandler(config->base, data->can_ch_idx, &data->ctx);
}

static int start(const struct device *dev)
{
	struct ifx_cat1_can_data *data = dev->data;

	if (data->common.started) {
		return -EALREADY;
	}

	if (data->state == CAN_STATE_BUS_OFF) {
		return -ENETDOWN;
	}

	configure(dev);

	data->common.started = true;

	k_sem_give(&data->operation_sem);

	return 0;
}

static int stop(const struct device *dev)
{
	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;

	if (!data->common.started) {
		return -EALREADY;
	}

	if (k_sem_take(&data->operation_sem, K_FOREVER) < 0) {
		return -EIO;
	}

	if (CY_CANFD_SUCCESS != Cy_CANFD_DeInit(config->base, data->can_ch_idx, &data->ctx)) {
		LOG_ERR("Cy_CANFD_DeInit failed");
		return -EIO;
	}

	data->common.started = false;
	data->state = CAN_STATE_STOPPED;
	k_sem_give(&data->operation_sem);
	return 0;
}

static int set_mode(const struct device *dev, can_mode_t mode)
{
	struct ifx_cat1_can_data *data = dev->data;

	if (data->common.started) {
		return -EBUSY;
	}

	if ((mode & (CAN_MODE_3_SAMPLES | CAN_MODE_ONE_SHOT)) != 0) {
		return -ENOTSUP;
	}

	/* Set mode functionality moved to can_configure because
	 * because test mode can be enabled after CAN initialization
	 * and FD is enabled during initialization. Zephyr expects to
	 * configure the mode when CAN stopped.
	 */

	data->common.mode = mode;

	return 0;
}

static int set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int ret;

	/* TODO: To pass the test, this check
	 * had to be added, but the bitrate can
	 * be changed during CAN operation.
	 */
	if (data->common.started) {
		return -EBUSY;
	}

	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret < 0) {
		return -EIO;
	}

	data->nominal_config.prescaler = timing->prescaler - 1;
	data->nominal_config.timeSegment1 = timing->prop_seg + timing->phase_seg1 - 1;
	data->nominal_config.timeSegment2 = timing->phase_seg2 - 1;
	data->nominal_config.syncJumpWidth = timing->sjw - 1;

	Cy_CANFD_ConfigChangesEnable(config->base, data->can_ch_idx);
	Cy_CANFD_SetBitrate(config->base, data->can_ch_idx, &data->nominal_config);
	Cy_CANFD_ConfigChangesDisable(config->base, data->can_ch_idx);

	k_sem_give(&data->operation_sem);
	return 0;
}

#if defined(CONFIG_CAN_FD_MODE)
static int set_data_timing(const struct device *dev, const struct can_timing *timing)
{
	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int ret;

	/* TODO: To pass the test, this check
	 * had to be added, but the bitrate can
	 * be changed during CAN operation.
	 */
	if (data->common.started) {
		return -EBUSY;
	}

	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret < 0) {
		return -EIO;
	}

	data->fast_config.prescaler = timing->prescaler - 1;
	data->fast_config.timeSegment1 = timing->prop_seg + timing->phase_seg1 - 1;
	data->fast_config.timeSegment2 = timing->phase_seg2 - 1;
	data->fast_config.syncJumpWidth = timing->sjw - 1;

	Cy_CANFD_ConfigChangesEnable(config->base, data->can_ch_idx);
	Cy_CANFD_SetFastBitrate(config->base, data->can_ch_idx, &data->fast_config);
	Cy_CANFD_ConfigChangesDisable(config->base, data->can_ch_idx);

	k_sem_give(&data->operation_sem);

	return 0;
}
#endif

void tx_cb_wrapper(const struct device *dev)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	struct ifx_cat1_can_tx_callback *tx_callbacks = &data->tx_callback_data[0];

	for (uint8_t tx_id = 0; tx_id < CONFIG_CAN_INFINEON_MAX_TX_QUEUE - 1; tx_id++) {
		if (CY_CANFD_TX_BUFFER_TRANSMIT_OCCURRED ==
		    Cy_CANFD_GetTxBufferStatus(config->base, data->can_ch_idx, tx_id)) {
			can_tx_callback_t callback;
			void *user_data;

			callback = data->tx_callback_data[tx_id].function;
			user_data = data->tx_callback_data[tx_id].user_data;

			tx_callbacks[tx_id].function = NULL;
			tx_callbacks[tx_id].user_data = NULL;
			if (callback) {
				callback(dev, 0, user_data);
				k_sem_give(&data->tx_sem);
			}
		}
	}
}

void rx_cb_wrapper(const struct device *dev, bool rxFIFOMsg, uint8_t msgBufOrRxFIFONum,
		   cy_stc_canfd_rx_buffer_t *basemsg)
{
	struct ifx_cat1_can_data *const data = dev->data;
	struct can_frame frame;
	uint32_t filter_id;

	memset(&frame, 0, sizeof(frame));

	if (basemsg == NULL) {
		LOG_ERR("basemsg is NULL");
		return;
	}

	filter_id = basemsg->r1_f->fidx;

	/* Configure the CAN frame flags */
	frame.flags |= basemsg->r0_f->xtd == CY_CANFD_XTD_EXTENDED_ID ? CAN_FRAME_IDE : 0;
	frame.flags |= basemsg->r0_f->rtr == CY_CANFD_RTR_REMOTE_FRAME ? CAN_FRAME_RTR : 0;
	frame.flags |= basemsg->r0_f->esi == CY_CANFD_ESI_ERROR_PASSIVE ? CAN_FRAME_ESI : 0;
	frame.flags |= basemsg->r1_f->fdf == CY_CANFD_FDF_CAN_FD_FRAME ? CAN_FRAME_FDF : 0;
	frame.flags |= basemsg->r1_f->brs == 1 ? CAN_FRAME_BRS : 0;

	frame.dlc = basemsg->r1_f->dlc;
	frame.id = basemsg->r0_f->id;

	for (int i = 0; i < CAN_MAX_DLEN / sizeof(uint32_t); i++) {
		frame.data_32[i] = basemsg->data_area_f[i];
	}

	/* If filter is extended ID, apply offset */
	if (basemsg->r0_f->xtd == CY_CANFD_XTD_EXTENDED_ID) {
		filter_id += EXT_FILTER_OFFSET;
	}

	if (data->rx_callback_data[filter_id].function != NULL) {
		data->rx_callback_data[filter_id].function(
			dev, &frame, data->rx_callback_data[filter_id].user_data);
	}
}

void error_cb_wrapper(const struct device *dev, uint32_t errorMask)
{
	struct ifx_cat1_can_data *const data = dev->data;
	enum can_state new_state;
	struct can_bus_err_cnt err_cnt = {0};

	if (errorMask & CY_CANFD_BUS_OFF_STATUS) {
		new_state = CAN_STATE_BUS_OFF;
	} else if (errorMask & CY_CANFD_ERROR_PASSIVE) {
		new_state = CAN_STATE_ERROR_PASSIVE;
	} else if (errorMask & CY_CANFD_WARNING_STATUS) {
		new_state = CAN_STATE_ERROR_WARNING;
	} else {
		new_state = CAN_STATE_ERROR_ACTIVE;
	}

	if (new_state != data->state) {
		data->state = new_state;
		if (data->common.state_change_cb) {
			/* No functionality to get error counters */
			data->common.state_change_cb(dev, new_state, err_cnt,
						     data->common.state_change_cb_user_data);
		}
	}
}

static int tx_buffer_config(const struct device *dev, const struct can_frame *frame)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int tx_id;

	if (frame->flags & CAN_FRAME_RTR && frame->dlc != 0) {
		return -ENOSPC;
	}

	/* Configure the CANFD T0 register */
	cy_stc_canfd_t0_t CANFD_T0RegisterBuffer_0 = {
		.id = frame->id,
		.rtr = frame->flags & CAN_FRAME_RTR ? CY_CANFD_RTR_REMOTE_FRAME
						    : CY_CANFD_RTR_DATA_FRAME,
		.xtd = frame->flags & CAN_FRAME_IDE ? CY_CANFD_XTD_EXTENDED_ID
						    : CY_CANFD_XTD_STANDARD_ID,
		.esi = false,
	};

	/* Configure the CANFD T1 register */
	cy_stc_canfd_t1_t CANFD_T1RegisterBuffer_0 = {
		.dlc = frame->dlc,
		.brs = frame->flags & CAN_FRAME_BRS ? true : false,
		.fdf = frame->flags & CAN_FRAME_FDF ? CY_CANFD_FDF_CAN_FD_FRAME
						    : CY_CANFD_FDF_STANDARD_FRAME,
		.efc = true,
		.mm = 0U,
	};

	/* Configure the CANFD TX buffer */
	cy_stc_canfd_tx_buffer_t CANFD_txBuffer_0 = {
		.t0_f = &CANFD_T0RegisterBuffer_0,
		.t1_f = &CANFD_T1RegisterBuffer_0,
		.data_area_f = (uint32_t *)frame->data,
	};

	/* Find an available TX buffer */
	for (tx_id = 0; tx_id < CONFIG_CAN_INFINEON_MAX_TX_QUEUE; tx_id++) {
		if (data->tx_callback_data[tx_id].function == NULL) {
			break;
		}
	}

	if (tx_id >= CONFIG_CAN_INFINEON_MAX_TX_QUEUE) {
		return -ENOSPC;
	}

	if (CY_CANFD_SUCCESS != Cy_CANFD_TxBufferConfig(config->base, data->can_ch_idx,
							&CANFD_txBuffer_0, tx_id, &data->ctx)) {
		return -EIO;
	}

	Cy_CANFD_SetInterruptMask(config->base, data->can_ch_idx, CY_CANFD_TRANSMISSION_COMPLETE);

	return tx_id;
}

static int send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		can_tx_callback_t callback, void *user_data)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int tx_id;
	unsigned int key;

	if (!data->common.started) {
		LOG_ERR("CAN not started");
		return -ENETDOWN;
	}

	if (!(data->common.mode & CAN_MODE_FD) && frame->flags & CAN_FRAME_FDF) {
		LOG_ERR("FD frame not supported in current mode");
		return -ENOTSUP;
	}

	if (!(data->common.mode & CAN_MODE_FD) && (frame->dlc > CAN_MAX_DLC)) {
		LOG_ERR("DLC too large for classic CAN: %d", frame->dlc);
		return -EINVAL;
	}
	if ((data->common.mode & CAN_MODE_FD) && (frame->dlc > CANFD_MAX_DLC)) {
		LOG_ERR("DLC too large for CAN FD: %d", frame->dlc);
		return -EINVAL;
	}

	if (frame->flags & CAN_FRAME_ESI) {
		LOG_ERR("ESI bit must not be set by software");
		return -ENOTSUP;
	}

	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		LOG_WRN("TX semaphore timeout");
		return -EAGAIN;
	}

	tx_id = tx_buffer_config(dev, frame);
	if (tx_id < 0) {
		LOG_ERR("tx_buffer_config failed");
		k_sem_give(&data->tx_sem);
		return tx_id;
	}

	key = irq_lock();

	data->tx_callback_data[tx_id].function = callback;
	data->tx_callback_data[tx_id].user_data = user_data;

	irq_unlock(key);

	if (CY_CANFD_SUCCESS != Cy_CANFD_TransmitTxBuffer(config->base, data->can_ch_idx, tx_id)) {
		LOG_ERR("Cy_CANFD_TransmitTxBuffer failed");
		k_sem_give(&data->tx_sem);
		return -EIO;
	}

	return 0;
}

static int add_std_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			  const struct can_filter *filter)
{
	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	cy_stc_id_filter_t std_filter;
	int filter_id = -ENOSPC;
	unsigned int key;

	if (filter->id > CAN_STD_ID_MASK || filter->mask > CAN_STD_ID_MASK) {
		LOG_ERR("invalid filter with standard (11-bit) CAN ID 0x%x, CAN ID mask 0x%x",
			filter->id, filter->mask);
		return -EINVAL;
	}

	if (data->std_filter_count >= CONFIG_CAN_INFINEON_MAX_FILTER) {
		return -ENOSPC;
	}

	/* Configure standard filter
	 * Zephyr uses mask-based filtering
	 */
	std_filter.sfid1 = filter->id;
	std_filter.sfid2 = filter->mask;
	std_filter.sfec =
		filter->id & 0x01 ? CY_CANFD_SFEC_STORE_RX_FIFO_0 : CY_CANFD_SFEC_STORE_RX_FIFO_1;
	std_filter.sft = CY_CANFD_SFT_CLASSIC_FILTER;

	for (int i = 0; i < CONFIG_CAN_INFINEON_MAX_FILTER; i++) {
		if (data->rx_callback_data[i].function == NULL) {
			filter_id = i;
			break;
		}
	}

	if (filter_id >= CONFIG_CAN_INFINEON_MAX_FILTER || filter_id == -ENOSPC) {
		return -ENOSPC;
	}

	key = irq_lock();

	data->std_id_filters[filter_id] = std_filter;
	data->rx_callback_data[filter_id].function = callback;
	data->rx_callback_data[filter_id].user_data = user_data;

	irq_unlock(key);

	Cy_CANFD_SidFilterSetup(config->base, data->can_ch_idx, &std_filter, filter_id, &data->ctx);

	uint32_t status = (CY_CANFD_RX_BUFFER_NEW_MESSAGE | /* Message stored to Rx Buffer */
			   CY_CANFD_RX_FIFO_1_NEW_MESSAGE | /* Rx FIFO 1 New Message */
			   CY_CANFD_RX_FIFO_0_NEW_MESSAGE); /* Rx FIFO 0 New Message */

	Cy_CANFD_SetInterruptMask(config->base, data->can_ch_idx, status);

	return filter_id;
}

static int add_ext_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			  const struct can_filter *filter)
{

	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int filter_id = -ENOSPC;
	unsigned int key;

	if (filter->id > CAN_EXT_ID_MASK || filter->mask > CAN_EXT_ID_MASK) {
		LOG_ERR("invalid filter with extended (29-bit) CAN ID 0x%x, CAN ID mask 0x%x",
			filter->id, filter->mask);
		return -EINVAL;
	}

	if (data->ext_filter_count - CONFIG_CAN_INFINEON_MAX_FILTER >=
	    CONFIG_CAN_INFINEON_MAX_FILTER) {
		return -ENOSPC;
	}

	/* Configure Filter F0 */
	data->temp_f0.efid1 = filter->id;
	data->temp_f0.efec =
		filter->id & 0x01 ? CY_CANFD_EFEC_STORE_RX_FIFO_0 : CY_CANFD_EFEC_STORE_RX_FIFO_1;

	/* Configure Filter F1 */
	data->temp_f1.efid2 = filter->mask;
	data->temp_f1.eft = CY_CANFD_EFT_CLASSIC_FILTER;

	/* extIdFilter */
	data->temp_ext_filter.f0_f = &data->temp_f0;
	data->temp_ext_filter.f1_f = &data->temp_f1;

	/* For extended filters uses offset because standard filters
	 * and extended filters are different arrays that share one
	 * rx_callback_data array, so extended filters start after
	 * standard filters. Also returned filter ID must be global
	 * for both standard and extended filters.
	 */
	for (int i = EXT_FILTER_OFFSET; i < TOTAL_FILTERS; i++) {
		if (data->rx_callback_data[i].function == NULL) {
			filter_id = i;
			break;
		}
	}

	if (filter_id >= TOTAL_FILTERS) {
		return -ENOSPC;
	}

	if (filter_id == -ENOSPC) {
		return -ENOSPC;
	}

	key = irq_lock();

	data->ext_id_filters[filter_id - EXT_FILTER_OFFSET] = data->temp_ext_filter;
	data->rx_callback_data[filter_id].function = callback;
	data->rx_callback_data[filter_id].user_data = user_data;

	irq_unlock(key);

	Cy_CANFD_XidFilterSetup(config->base, data->can_ch_idx, &data->temp_ext_filter,
				filter_id - EXT_FILTER_OFFSET, &data->ctx);

	uint32_t status = (CY_CANFD_RX_BUFFER_NEW_MESSAGE | /* Message stored to Rx Buffer */
			   CY_CANFD_RX_FIFO_1_NEW_MESSAGE | /* Rx FIFO 1 New Message */
			   CY_CANFD_RX_FIFO_0_NEW_MESSAGE); /* Rx FIFO 0 New Message */

	Cy_CANFD_SetInterruptMask(config->base, data->can_ch_idx, status);

	return filter_id;
}

static int add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			 const struct can_filter *filter)
{

	struct ifx_cat1_can_data *data = dev->data;
	int filter_idx;
	int ret;

	if (callback == NULL) {
		return -EINVAL;
	}

	ret = k_sem_take(&data->operation_sem, K_FOREVER);
	if (ret < 0) {
		return -EIO;
	}

	if ((filter->flags & ~CAN_FILTER_IDE) != 0) {
		LOG_ERR("Unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	if ((filter->flags & CAN_FILTER_IDE) != 0U) {
		filter_idx = add_ext_filter(dev, callback, user_data, filter);
	} else {
		filter_idx = add_std_filter(dev, callback, user_data, filter);
	}
	k_sem_give(&data->operation_sem);

	return filter_idx;
}

static void remove_rx_filter(const struct device *dev, int filter_id)
{
	struct ifx_cat1_can_data *data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;

	if (filter_id < 0) {
		return;
	}

	/* Disable the filter by setting its filter element to DISABLE */
	if (filter_id >= EXT_FILTER_OFFSET &&
	    filter_id < EXT_FILTER_OFFSET + CONFIG_CAN_INFINEON_MAX_FILTER) {
		/* Configure Filter F0 */
		data->temp_f0.efid1 = 0;
		data->temp_f0.efec = CY_CANFD_EFEC_DISABLE;

		/* Configure Filter F1 */
		data->temp_f1.efid2 = 0;
		data->temp_f1.eft = CY_CANFD_EFT_RANGE_EFID1_EFID2;

		/* extIdFilter */
		data->temp_ext_filter.f0_f = &data->temp_f0;
		data->temp_ext_filter.f1_f = &data->temp_f1;

		Cy_CANFD_XidFilterSetup(config->base, data->can_ch_idx, &data->temp_ext_filter,
					filter_id, &data->ctx);

		data->rx_callback_data[filter_id].function = NULL;
		data->rx_callback_data[filter_id].user_data = NULL;

	} else if (filter_id >= STD_FILTER_OFFSET &&
		   filter_id < STD_FILTER_OFFSET + CONFIG_CAN_INFINEON_MAX_FILTER) {
		/* Standard filter */
		data->temp_std_filters.sfid1 = 0U;
		data->temp_std_filters.sfid2 = 0U;
		data->temp_std_filters.sfec = CY_CANFD_SFEC_DISABLE;
		data->temp_std_filters.sft = CY_CANFD_SFT_RANGE_SFID1_SFID2;

		Cy_CANFD_SidFilterSetup(config->base, data->can_ch_idx, &data->temp_std_filters,
					filter_id, &data->ctx);

		data->rx_callback_data[filter_id].function = NULL;
		data->rx_callback_data[filter_id].user_data = NULL;
	}
}

static int get_state(const struct device *dev, enum can_state *state,
		     struct can_bus_err_cnt *err_cnt)
{
	const struct ifx_cat1_can_config *const config = dev->config;
	struct ifx_cat1_can_data *data = dev->data;
	uint32_t status = Cy_CANFD_GetLastError(config->base, data->can_ch_idx);

	/* TODO: Get error counters pdl doesn't have support for this */
	if (err_cnt != NULL) {

		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}
	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if ((status & CY_CANFD_PSR_BO) != 0) {
			*state = CAN_STATE_BUS_OFF;
		} else if ((status & CY_CANFD_PSR_EP) != 0) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if ((status & CY_CANFD_PSR_EW) != 0) {
			*state = CAN_STATE_ERROR_WARNING;
		} else if ((status & CY_CANFD_PSR_EW) == 0 && (status & CY_CANFD_PSR_EP) == 0) {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	return 0;
}

static void set_state_change_callback(const struct device *dev,
				      can_state_change_callback_t callback, void *user_data)
{
	struct ifx_cat1_can_data *data = dev->data;

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

static int get_core_clock(const struct device *dev, uint32_t *rate)
{
	struct ifx_cat1_can_data *data = dev->data;
	en_clk_dst_t clk_connection;

	clk_connection = ifx_cat1_can_get_clock_index(CANFD_BLOCK, data->can_ch_idx);
	*rate = ifx_cat1_utils_peri_pclk_get_frequency(clk_connection, &data->clock);

	return 0;
}

static int get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_INFINEON_MAX_FILTER;
}

static int get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

#if defined(CONFIG_CAN_FD_MODE)
	*cap |= CAN_MODE_FD;
#endif
	return 0;
}

static int configure(const struct device *dev)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	cy_rslt_t result;

	/* 1. TDC configuration */
	data->tdc_config.tdcEnabled = false;
	data->tdc_config.tdcOffset = 0U;
	data->tdc_config.tdcFilterWindow = 0U;

	/* 2. FIFO configuration */
	data->fifo0_config.mode = CY_CANFD_FIFO_MODE_BLOCKING;
	data->fifo0_config.watermark = 0U;
	data->fifo0_config.numberOfFIFOElements = CONFIG_CAN_INFINEON_NUMBER_FIFO0_ELEMENTS;
	data->fifo0_config.topPointerLogicEnabled = false;

	/* 3. FIFO1 configuration */
	data->fifo1_config.mode = CY_CANFD_FIFO_MODE_BLOCKING;
	data->fifo1_config.watermark = 0U;
	data->fifo1_config.numberOfFIFOElements = CONFIG_CAN_INFINEON_NUMBER_FIFO1_ELEMENTS;
	data->fifo1_config.topPointerLogicEnabled = false;

	/* 4. SID configuration
	 * If filters are not yet configured, set all to disabled
	 */
	if (data->std_filters_ready == false) {
		for (uint8_t i = 0; i < CONFIG_CAN_INFINEON_MAX_FILTER; i++) {
			data->std_id_filters[i].sfid1 = 0U;
			data->std_id_filters[i].sfid2 = 0U;
			data->std_id_filters[i].sfec = CY_CANFD_SFEC_DISABLE;
			data->std_id_filters[i].sft = CY_CANFD_SFT_RANGE_SFID1_SFID2;
		}
		data->std_filters_ready = true;
	}
	data->sid_filters_config.numberOfSIDFilters = CONFIG_CAN_INFINEON_MAX_FILTER;
	data->sid_filters_config.sidFilter = data->std_id_filters;

	/* 5. EXTID filter configuration
	 * If filters are not yet configured, set all to disabled
	 */
	if (data->ext_filters_ready == false) {
		data->temp_f0.efid1 = 0U;
		data->temp_f0.efec = CY_CANFD_EFEC_DISABLE;
		data->temp_f1.efid2 = 0U;
		data->temp_f1.eft = CY_CANFD_EFT_RANGE_EFID1_EFID2;

		for (uint8_t i = 0; i < CONFIG_CAN_INFINEON_MAX_FILTER; i++) {
			data->ext_id_filters[i].f0_f = &data->temp_f0;
			data->ext_id_filters[i].f1_f = &data->temp_f1;
		}
		data->ext_filters_ready = true;
	}
	data->extid_filters_config.numberOfEXTIDFilters = CONFIG_CAN_INFINEON_MAX_FILTER;
	data->extid_filters_config.extidFilter = data->ext_id_filters;
	data->extid_filters_config.extIDANDMask = 0x1FFFFFFFU;

	/* 6. Global filter configuration */
	data->global_filters_config.nonMatchingFramesStandard = CY_CANFD_REJECT_NON_MATCHING;
	data->global_filters_config.nonMatchingFramesExtended = CY_CANFD_REJECT_NON_MATCHING;
	data->global_filters_config.rejectRemoteFramesStandard = true;
	data->global_filters_config.rejectRemoteFramesExtended = true;

	/* 7. CANFD configuration */
	data->canfd_config.canFDMode = data->common.mode & CAN_MODE_FD ? true : false;
	data->canfd_config.bitrate = &data->nominal_config;
	data->canfd_config.fastBitrate =
		data->common.mode & CAN_MODE_FD ? &data->fast_config : NULL;
	data->canfd_config.tdcConfig = data->common.mode & CAN_MODE_FD ? &data->tdc_config : NULL;
	data->canfd_config.sidFilterConfig = &data->sid_filters_config;
	data->canfd_config.extidFilterConfig = &data->extid_filters_config;
	data->canfd_config.globalFilterConfig = &data->global_filters_config;
	if (data->common.mode & CAN_MODE_FD) {
		data->canfd_config.rxBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_64;
		data->canfd_config.rxFIFO1DataSize = CY_CANFD_BUFFER_DATA_SIZE_64;
		data->canfd_config.rxFIFO0DataSize = CY_CANFD_BUFFER_DATA_SIZE_64;
		data->canfd_config.txBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_64;
	} else {
		data->canfd_config.rxBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_8;
		data->canfd_config.rxFIFO1DataSize = CY_CANFD_BUFFER_DATA_SIZE_8;
		data->canfd_config.rxFIFO0DataSize = CY_CANFD_BUFFER_DATA_SIZE_8;
		data->canfd_config.txBufferDataSize = CY_CANFD_BUFFER_DATA_SIZE_8;
	}
	data->canfd_config.rxFIFO0Config = &data->fifo0_config;
	data->canfd_config.rxFIFO1Config = &data->fifo1_config;

	data->canfd_config.noOfRxBuffers = CONFIG_CAN_INFINEON_RX_FIFO_ITEMS;
	data->canfd_config.noOfTxBuffers = CONFIG_CAN_INFINEON_MAX_TX_QUEUE;

	data->canfd_config.messageRAMaddress = CY_CAN0MRAM_BASE;
	data->canfd_config.messageRAMsize = CONFIG_CAN_INFINEON_MRAM_SIZE;

	/* 8. CAN Context */
	memset(&data->ctx, 0, sizeof(cy_stc_canfd_context_t));

	/* 9. Initialize CANFD */
	result = Cy_CANFD_Init(config->base, data->can_ch_idx, &data->canfd_config, &data->ctx);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("Cy_CANFD_Init failed with error code: %d", result);
		return -EIO;
	}

	/* 10. Enable test mode */
	Cy_CANFD_ConfigChangesEnable(config->base, data->can_ch_idx);

	if (data->common.mode & CAN_MODE_LOOPBACK) {
		Cy_CANFD_TestModeConfig(config->base, data->can_ch_idx,
					CY_CANFD_TEST_MODE_EXTERNAL_LOOP_BACK);
	} else if (data->common.mode & CAN_MODE_LISTENONLY) {
		Cy_CANFD_TestModeConfig(config->base, data->can_ch_idx,
					CY_CANFD_TEST_MODE_BUS_MONITORING);
	} else {
		Cy_CANFD_TestModeConfig(config->base, data->can_ch_idx, CY_CANFD_TEST_MODE_DISABLE);
	}

	Cy_CANFD_ConfigChangesDisable(config->base, data->can_ch_idx);

	/* 11. Enable error interrupts */
	uint32_t status =
		(CY_CANFD_BUS_OFF_STATUS | CY_CANFD_ERROR_PASSIVE | CY_CANFD_WARNING_STATUS);

	Cy_CANFD_SetInterruptMask(config->base, data->can_ch_idx, status);
	/* connect IRQ handler (will be enabled in configure after PDL init) */
	config->irq_config_func(dev);

	return 0;
}

CANFD_CH_Type *const _IFX_CANFD_CH_ADDRESSES[] = {
#ifdef CANFD0_CH0
	CANFD0_CH0,
#endif
#ifdef CANFD0_CH1
	CANFD0_CH1,
#endif
};

/* Get CAN channel number from CAN ch_addr */
static int get_ch_num(const struct device *dev)
{
	const struct ifx_cat1_can_config *const config = dev->config;
	CANFD_CH_Type *can_base = config->ch_addr;

	for (uint8_t ch = 0; ch < ARRAY_SIZE(_IFX_CANFD_CH_ADDRESSES); ch++) {
		if (can_base == _IFX_CANFD_CH_ADDRESSES[ch]) {
			return ch;
		}
	}

	return -1;
}

static int init(const struct device *dev)
{
	struct ifx_cat1_can_data *const data = dev->data;
	const struct ifx_cat1_can_config *const config = dev->config;
	int ret;
	struct can_timing timing = {0};

	if (config == NULL || config->ch_addr == NULL) {
		return -ENODEV;
	}
	ret = k_sem_init(&data->operation_sem, 1, 1);
	if (ret < 0) {
		return ret;
	}
	k_sem_init(&data->tx_sem, CONFIG_CAN_INFINEON_MAX_TX_QUEUE,
		   CONFIG_CAN_INFINEON_MAX_TX_QUEUE);

	k_mutex_init(&data->mutex);

	data->hw_resource.type = IFX_RSC_CAN;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = get_ch_num(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get CAN channel number");
		return -ENODEV;
	}

	data->can_ch_idx = ret;

	ret = can_calc_timing(dev, &timing, config->cfg_common.bitrate,
			      config->cfg_common.sample_point);
	if (ret < 0) {
		return ret;
	}

	/* Initialize filter count */
	data->std_filter_count = 0;
	data->ext_filter_count = EXT_FILTER_OFFSET;
	/* Initialize CANFD state */
	data->state = CAN_STATE_STOPPED;
	data->common.started = false;
	data->common.mode = CAN_MODE_NORMAL;

	return set_timing(dev, &timing);
}

/* Driver API structure */
static DEVICE_API(can, ifx_cat1_can_driver_api) = {
	.send = send,
	.add_rx_filter = add_rx_filter,
	.remove_rx_filter = remove_rx_filter,
	.set_mode = set_mode,
	.set_timing = set_timing,
	.get_state = get_state,
	.set_state_change_callback = set_state_change_callback,
	.get_core_clock = get_core_clock,
	.get_max_filters = get_max_filters,
	.get_capabilities = get_capabilities,
	.start = start,
	.stop = stop,
	.timing_min = {
			.sjw = 1,
			.prop_seg = 0,
			.phase_seg1 = 2,
			.phase_seg2 = 2,
			.prescaler = 1,
		},
	.timing_max = {
			.sjw = 4,
			.prop_seg = 0,
			.phase_seg1 = 32,
			.phase_seg2 = 8,
			.prescaler = 64,
		},
#ifdef CONFIG_CAN_FD_MODE
	.timing_data_min = {
			.sjw = 1,
			.prop_seg = 0,
			.phase_seg1 = 2,
			.phase_seg2 = 1,
			.prescaler = 1,
		},
	.timing_data_max = {
			.sjw = 16,
			.prop_seg = 0,
			.phase_seg1 = 32,
			.phase_seg2 = 16,
			.prescaler = 32,
		},
	.set_timing_data = set_data_timing,
#endif /* CONFIG_CAN_FD_MODE */
};

#if (CONFIG_SOC_FAMILY_INFINEON_CAT1C)
#define IRQ_INFO(n)                                                                                \
	.irq_num = DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_NUM),                         \
	.irq_priority = DT_INST_PROP_BY_IDX(n, system_interrupts, SYS_INT_PRI)
#else
#define IRQ_INFO(n) .irq_num = DT_INST_IRQN(n), .irq_priority = DT_INST_IRQ(n, priority)
#endif

#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PERI_INFO(n) .clock_peri_group = DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),
#else
#define PERI_INFO(n)
#endif

#define CAN_PERI_CLOCK_INIT(n)                                                                     \
	.clock =                                                                                   \
		{                                                                                  \
			.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                                 \
				DT_PROP_BY_IDX(DT_INST_PHANDLE(n, clocks), peri_group, 1),         \
				DT_INST_PROP_BY_PHANDLE(n, clocks, div_type)),                     \
			.channel = DT_INST_PROP_BY_PHANDLE(n, clocks, channel),                    \
	},                                                                                         \
	PERI_INFO(n)

#define CAN_CAT1_INIT_FUNC(n)                                                                      \
	static void ifx_cat1_can_irq_config_func_##n(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), can_isr_handler,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

/* c */
#define INFINEON_CAT1_CAN_INIT(n)                                                                  \
                                                                                                   \
	void tx_handle_events_func_##n(void)                                                       \
	{                                                                                          \
		tx_cb_wrapper(DEVICE_DT_INST_GET(n));                                              \
	}                                                                                          \
                                                                                                   \
	void rx_handle_events_func_##n(bool rxFIFOMsg, uint8_t msgBufOrRxFIFONum,                  \
				       cy_stc_canfd_rx_buffer_t *basemsg)                          \
	{                                                                                          \
		rx_cb_wrapper(DEVICE_DT_INST_GET(n), rxFIFOMsg, msgBufOrRxFIFONum, basemsg);       \
	}                                                                                          \
	void error_handle_events_func_##n(uint32_t errorMask)                                      \
	{                                                                                          \
		error_cb_wrapper(DEVICE_DT_INST_GET(n), errorMask);                                \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	CAN_CAT1_INIT_FUNC(n)                                                                      \
                                                                                                   \
	static struct ifx_cat1_can_data ifx_cat1_can##n##_data = {                                 \
		.canfd_config.txCallback = tx_handle_events_func_##n,                              \
		.canfd_config.rxCallback = rx_handle_events_func_##n,                              \
		.canfd_config.errorCallback = error_handle_events_func_##n,                        \
		CAN_PERI_CLOCK_INIT(n)};                                                           \
                                                                                                   \
	static const struct ifx_cat1_can_config ifx_cat1_can##n##_cfg = {                          \
		.cfg_common = CAN_DT_DRIVER_CONFIG_INST_GET(n, 0, 1000000),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.ch_addr = (CANFD_CH_Type *)DT_INST_REG_ADDR(n),                                   \
		.base = (CANFD_Type *)CANFD0_BASE,                                                 \
		.irq_config_func = ifx_cat1_can_irq_config_func_##n,                               \
		.tx_canfd_callback = tx_handle_events_func_##n,                                    \
		IRQ_INFO(n)};                                                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, init, NULL, &ifx_cat1_can##n##_data, &ifx_cat1_can##n##_cfg,      \
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, &ifx_cat1_can_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_CAN_INIT)
