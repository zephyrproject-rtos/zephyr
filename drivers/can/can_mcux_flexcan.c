/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_flexcan

#include <zephyr.h>
#include <sys/atomic.h>
#include <drivers/can.h>
#include <drivers/clock_control.h>
#include <device.h>
#include <sys/byteorder.h>
#include <fsl_flexcan.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(can_mcux_flexcan);

#define SP_IS_SET(inst) DT_INST_NODE_HAS_PROP(inst, sample_point) ||

/* Macro to exclude the sample point algorithm from compilation if not used
 * Without the macro, the algorithm would always waste ROM
 */
#define USE_SP_ALGO (DT_INST_FOREACH_STATUS_OKAY(SP_IS_SET) 0)

#define SP_AND_TIMING_NOT_SET(inst) \
	(!DT_INST_NODE_HAS_PROP(inst, sample_point) && \
	!(DT_INST_NODE_HAS_PROP(inst, prop_seg) && \
	DT_INST_NODE_HAS_PROP(inst, phase_seg1) && \
	DT_INST_NODE_HAS_PROP(inst, phase_seg2))) ||

#if DT_INST_FOREACH_STATUS_OKAY(SP_AND_TIMING_NOT_SET) 0
#error You must either set a sampling-point or timings (phase-seg* and prop-seg)
#endif

/*
 * RX message buffers (filters) will take up the first N message
 * buffers. The rest are available for TX use.
 */
#define MCUX_FLEXCAN_MAX_RX CONFIG_CAN_MAX_FILTER
#define MCUX_FLEXCAN_MAX_TX \
	(FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(0) \
	- MCUX_FLEXCAN_MAX_RX)

#define MCUX_N_TX_ALLOC_ELEM (1 + (MCUX_FLEXCAN_MAX_TX - 1) / ATOMIC_BITS)

/*
 * Convert from RX message buffer index to allocated filter ID and
 * vice versa.
 */
#define RX_MBIDX_TO_ALLOC_IDX(x) (x)
#define ALLOC_IDX_TO_RXMB_IDX(x) (x)

/*
 * Convert from TX message buffer index to allocated TX ID and vice
 * versa.
 */
#define TX_MBIDX_TO_ALLOC_IDX(x) (x - MCUX_FLEXCAN_MAX_RX)
#define ALLOC_IDX_TO_TXMB_IDX(x) (x + MCUX_FLEXCAN_MAX_RX)

/* Convert from back from FLEXCAN IDs to Zephyr CAN IDs. */
#define FLEXCAN_ID_TO_ZCAN_ID_STD(id) \
	((uint32_t)((((uint32_t)(id)) & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT))
#define FLEXCAN_ID_TO_ZCAN_ID_EXT(id) \
	((uint32_t)((((uint32_t)(id)) & (CAN_ID_STD_MASK | CAN_ID_EXT_MASK)) \
	>> CAN_ID_EXT_SHIFT))

struct mcux_flexcan_config {
	CAN_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	int clk_source;
	uint32_t bitrate;
	uint32_t sample_point;
	uint32_t sjw;
	uint32_t prop_seg;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_flexcan_rx_callback {
	flexcan_rx_mb_config_t mb_config;
	flexcan_frame_t frame;
	can_rx_callback_t function;
	void *arg;
};

struct mcux_flexcan_tx_callback {
	struct k_sem done;
	int status;
	flexcan_frame_t frame;
	can_tx_callback_t function;
	void *arg;
};

struct mcux_flexcan_data {
	const struct device *dev;
	flexcan_handle_t handle;

	ATOMIC_DEFINE(rx_allocs, MCUX_FLEXCAN_MAX_RX);
	struct k_mutex rx_mutex;
	struct mcux_flexcan_rx_callback rx_cbs[MCUX_FLEXCAN_MAX_RX];

	ATOMIC_DEFINE(tx_allocs, MCUX_FLEXCAN_MAX_TX);
	struct k_sem tx_allocs_sem;
	struct mcux_flexcan_tx_callback tx_cbs[MCUX_FLEXCAN_MAX_TX];
	enum can_state state;
	can_state_change_isr_t state_change_isr;
	struct can_timing timing;
};

static int mcux_flexcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcux_flexcan_config *config = dev->config;
	const struct device *clock_dev;

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		return -EIO;
	}

	return clock_control_get_rate(clock_dev, config->clock_subsys, rate);
}

static int mcux_flexcan_set_timing(const struct device *dev,
				   const struct can_timing *timing,
				   const struct can_timing *timing_data)
{
	ARG_UNUSED(timing_data);
	struct mcux_flexcan_data *data = dev->data;
	const struct mcux_flexcan_config *config = dev->config;
	flexcan_timing_config_t timing_tmp;

	if (!timing) {
		return -EINVAL;
	}

	data->timing = *timing;

	timing_tmp.preDivider = data->timing.prescaler - 1U;
	timing_tmp.rJumpwidth = data->timing.sjw - 1U;
	timing_tmp.phaseSeg1 = data->timing.phase_seg1 - 1U;
	timing_tmp.phaseSeg2 = data->timing.phase_seg2 - 1U;
	timing_tmp.propSeg = data->timing.prop_seg - 1U;

	FLEXCAN_SetTimingConfig(config->base, &timing_tmp);

	return 0;
}

static int mcux_flexcan_set_mode(const struct device *dev, enum can_mode mode)
{
	struct mcux_flexcan_data *data = dev->data;
	const struct mcux_flexcan_config *config = dev->config;
	flexcan_config_t flexcan_config;
	uint32_t clock_freq;
	int ret;

	ret = mcux_flexcan_get_core_clock(dev, &clock_freq);
	if (ret != 0) {
		return -EIO;
	}

	FLEXCAN_GetDefaultConfig(&flexcan_config);
	flexcan_config.clkSrc = config->clk_source;
	flexcan_config.baudRate = clock_freq /
	      (1U + data->timing.prop_seg + data->timing.phase_seg1 +
	       data->timing.phase_seg2) / data->timing.prescaler;
	flexcan_config.enableIndividMask = true;

	flexcan_config.timingConfig.rJumpwidth = data->timing.sjw - 1U;
	flexcan_config.timingConfig.propSeg = data->timing.prop_seg - 1U;
	flexcan_config.timingConfig.phaseSeg1 = data->timing.phase_seg1 - 1U;
	flexcan_config.timingConfig.phaseSeg2 = data->timing.phase_seg2 - 1U;

	if (mode == CAN_LOOPBACK_MODE || mode == CAN_SILENT_LOOPBACK_MODE) {
		flexcan_config.enableLoopBack = true;
	} else {
		/* Disable self-reception unless loopback is requested */
		flexcan_config.disableSelfReception = true;
	}

	if (mode == CAN_SILENT_MODE || mode == CAN_SILENT_LOOPBACK_MODE) {
		flexcan_config.enableListenOnlyMode = true;
	}

	FLEXCAN_Init(config->base, &flexcan_config, clock_freq);

	return 0;
}

static void mcux_flexcan_copy_zframe_to_frame(const struct zcan_frame *src,
					      flexcan_frame_t *dest)
{
	if (src->id_type == CAN_STANDARD_IDENTIFIER) {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
	} else {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
	}

	if (src->rtr == CAN_DATAFRAME) {
		dest->type = kFLEXCAN_FrameTypeData;
	} else {
		dest->type = kFLEXCAN_FrameTypeRemote;
	}

	dest->length = src->dlc;
	dest->dataWord0 = sys_cpu_to_be32(src->data_32[0]);
	dest->dataWord1 = sys_cpu_to_be32(src->data_32[1]);
}

static void mcux_flexcan_copy_frame_to_zframe(const flexcan_frame_t *src,
					      struct zcan_frame *dest)
{
	if (src->format == kFLEXCAN_FrameFormatStandard) {
		dest->id_type = CAN_STANDARD_IDENTIFIER;
		dest->id = FLEXCAN_ID_TO_ZCAN_ID_STD(src->id);
	} else {
		dest->id_type = CAN_EXTENDED_IDENTIFIER;
		dest->id = FLEXCAN_ID_TO_ZCAN_ID_EXT(src->id);
	}

	if (src->type == kFLEXCAN_FrameTypeData) {
		dest->rtr = CAN_DATAFRAME;
	} else {
		dest->rtr = CAN_REMOTEREQUEST;
	}

	dest->dlc = src->length;
	dest->data_32[0] = sys_be32_to_cpu(src->dataWord0);
	dest->data_32[1] = sys_be32_to_cpu(src->dataWord1);
#ifdef CONFIG_CAN_RX_TIMESTAMP
	dest->timestamp = src->timestamp;
#endif /* CAN_RX_TIMESTAMP */
}

static void mcux_flexcan_copy_zfilter_to_mbconfig(const struct zcan_filter *src,
						  flexcan_rx_mb_config_t *dest,
						  uint32_t *mask)
{
	if (src->id_type == CAN_STANDARD_IDENTIFIER) {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
		*mask = FLEXCAN_RX_MB_STD_MASK(src->id_mask,
					       src->rtr & src->rtr_mask, 1);
	} else {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
		*mask = FLEXCAN_RX_MB_EXT_MASK(src->id_mask,
					       src->rtr & src->rtr_mask, 1);
	}

	if ((src->rtr & src->rtr_mask) == CAN_DATAFRAME) {
		dest->type = kFLEXCAN_FrameTypeData;
	} else {
		dest->type = kFLEXCAN_FrameTypeRemote;
	}
}

/* mcux_get_tx_alloc is a linear on array, and binary on atomic_val_t search
 * for the highest bit set in data->tx_allocs. 0 is returned in case of an empty
 * tx_alloc, the next free bit otherwise.
 * The reason to always use a higher buffer number than the current in use is
 * that a FIFO manner is kept. The Controller would otherwise send the frame
 * that is in the lowest buffer number first.
 */
static int mcux_get_tx_alloc(struct mcux_flexcan_data *data)
{
	atomic_val_t *allocs = data->tx_allocs;
	atomic_val_t pivot = ATOMIC_BITS / 2;
	atomic_val_t alloc, mask;
	int i;

	for (i = MCUX_N_TX_ALLOC_ELEM - 1; i >= 0; i--) {
		alloc = allocs[i];
		if (alloc) {
			for (atomic_val_t bits = ATOMIC_BITS / 2U;
			    bits; bits >>= 1) {
				mask = GENMASK(pivot + bits - 1, pivot);
				if (alloc & mask) {
					pivot += bits / 2U;
				} else {
					pivot -= bits / 2U;
				}
			}

			if (!(alloc & mask)) {
				pivot--;
			}

			break;
		}
	}

	alloc = alloc ? (pivot + 1 + i * ATOMIC_BITS) : 0;
	return alloc >= MCUX_FLEXCAN_MAX_TX ? -1 : alloc;
}

static int mcux_flexcan_send(const struct device *dev,
			     const struct zcan_frame *msg,
			     k_timeout_t timeout,
			     can_tx_callback_t callback_isr, void *callback_arg)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	flexcan_mb_transfer_t xfer;
	status_t status;
	int alloc;

	if (msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", msg->dlc, CAN_MAX_DLC);
		return CAN_TX_EINVAL;
	}

	while (true) {
		alloc = mcux_get_tx_alloc(data);
		if (alloc >= 0) {
			if (atomic_test_and_set_bit(data->tx_allocs, alloc)) {
				continue;
			}

			break;
		}

		if (k_sem_take(&data->tx_allocs_sem, timeout) != 0) {
			return CAN_TIMEOUT;
		}
	}

	mcux_flexcan_copy_zframe_to_frame(msg, &data->tx_cbs[alloc].frame);
	data->tx_cbs[alloc].function = callback_isr;
	data->tx_cbs[alloc].arg = callback_arg;
	xfer.frame = &data->tx_cbs[alloc].frame;
	xfer.mbIdx = ALLOC_IDX_TO_TXMB_IDX(alloc);
	FLEXCAN_SetTxMbConfig(config->base, xfer.mbIdx, true);
	status = FLEXCAN_TransferSendNonBlocking(config->base, &data->handle,
						 &xfer);
	if (status != kStatus_Success) {
		return CAN_TX_ERR;
	}

	if (callback_isr == NULL) {
		k_sem_take(&data->tx_cbs[alloc].done, K_FOREVER);
		return data->tx_cbs[alloc].status;
	}

	return CAN_TX_OK;
}

static int mcux_flexcan_attach_isr(const struct device *dev,
				   can_rx_callback_t isr,
				   void *callback_arg,
				   const struct zcan_filter *filter)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	flexcan_mb_transfer_t xfer;
	status_t status;
	uint32_t mask;
	int alloc = CAN_NO_FREE_FILTER;
	int i;

	__ASSERT_NO_MSG(isr);

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	/* Find and allocate RX message buffer */
	for (i = 0; i < MCUX_FLEXCAN_MAX_RX; i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			alloc = i;
			break;
		}
	}

	if (alloc == CAN_NO_FREE_FILTER) {
		return alloc;
	}

	mcux_flexcan_copy_zfilter_to_mbconfig(filter,
					      &data->rx_cbs[alloc].mb_config,
					      &mask);

	data->rx_cbs[alloc].arg = callback_arg;
	data->rx_cbs[alloc].function = isr;

	FLEXCAN_SetRxIndividualMask(config->base, ALLOC_IDX_TO_RXMB_IDX(alloc),
				    mask);
	FLEXCAN_SetRxMbConfig(config->base, ALLOC_IDX_TO_RXMB_IDX(alloc),
			      &data->rx_cbs[alloc].mb_config, true);

	xfer.frame = &data->rx_cbs[alloc].frame;
	xfer.mbIdx = ALLOC_IDX_TO_RXMB_IDX(alloc);
	status = FLEXCAN_TransferReceiveNonBlocking(config->base, &data->handle,
						    &xfer);
	if (status != kStatus_Success) {
		LOG_ERR("Failed to start rx for filter id %d (err = %d)",
			alloc, status);
		alloc = CAN_NO_FREE_FILTER;
	}

	k_mutex_unlock(&data->rx_mutex);

	return alloc;
}

static void mcux_flexcan_register_state_change_isr(const struct device *dev,
						   can_state_change_isr_t isr)
{
	struct mcux_flexcan_data *data = dev->data;

	data->state_change_isr = isr;
}

static enum can_state mcux_flexcan_get_state(const struct device *dev,
					     struct can_bus_err_cnt *err_cnt)
{
	const struct mcux_flexcan_config *config = dev->config;
	uint32_t status_flags;

	if (err_cnt) {
		FLEXCAN_GetBusErrCount(config->base, &err_cnt->tx_err_cnt,
				       &err_cnt->rx_err_cnt);
	}

	status_flags = (FLEXCAN_GetStatusFlags(config->base) &
			CAN_ESR1_FLTCONF_MASK) << CAN_ESR1_FLTCONF_SHIFT;

	if (status_flags & 0x02) {
		return CAN_BUS_OFF;
	}

	if (status_flags & 0x01) {
		return CAN_ERROR_PASSIVE;
	}

	return CAN_ERROR_ACTIVE;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int mcux_flexcan_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct mcux_flexcan_config *config = dev->config;
	int ret = 0;
	uint64_t start_time;

	if (mcux_flexcan_get_state(dev, NULL) != CAN_BUS_OFF) {
		return 0;
	}

	start_time = k_uptime_ticks();
	config->base->CTRL1 &= ~CAN_CTRL1_BOFFREC_MASK;

	if (timeout != K_NO_WAIT) {
		while (mcux_flexcan_get_state(dev, NULL) == CAN_BUS_OFF) {
			if (timeout != K_FOREVER &&
			    k_uptime_ticks() - start_time >= timeout.ticks) {
				ret = CAN_TIMEOUT;
			}
		}
	}

	config->base->CTRL1 |= CAN_CTRL1_BOFFREC_MASK;

	return ret;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void mcux_flexcan_detach(const struct device *dev, int filter_id)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;

	if (filter_id >= MCUX_FLEXCAN_MAX_RX) {
		LOG_ERR("Detach: Filter id >= MAX_RX (%d >= %d)", filter_id,
			MCUX_FLEXCAN_MAX_RX);
		return;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	if (atomic_test_and_clear_bit(data->rx_allocs, filter_id)) {
		FLEXCAN_TransferAbortReceive(config->base, &data->handle,
					     ALLOC_IDX_TO_RXMB_IDX(filter_id));
		FLEXCAN_SetRxMbConfig(config->base,
				      ALLOC_IDX_TO_RXMB_IDX(filter_id), NULL,
				      false);
		data->rx_cbs[filter_id].function = NULL;
		data->rx_cbs[filter_id].arg = NULL;
	} else {
		LOG_WRN("Filter ID %d already detached", filter_id);
	}

	k_mutex_unlock(&data->rx_mutex);
}

static inline void mcux_flexcan_transfer_error_status(const struct device *dev,
						      uint32_t error)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	can_tx_callback_t function;
	int status = CAN_TX_OK;
	void *arg;
	int alloc;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;

	if (error & CAN_ESR1_FLTCONF(2)) {
		LOG_DBG("Tx bus off (error 0x%08x)", error);
		status = CAN_TX_BUS_OFF;
	} else if ((error & kFLEXCAN_Bit0Error) ||
		   (error & kFLEXCAN_Bit1Error)) {
		LOG_DBG("TX arbitration lost (error 0x%08x)", error);
		status = CAN_TX_ARB_LOST;
	} else if (error & kFLEXCAN_AckError) {
		LOG_DBG("TX no ACK received (error 0x%08x)", error);
		status = CAN_TX_ERR;
	} else if (error & kFLEXCAN_StuffingError) {
		LOG_DBG("RX stuffing error (error 0x%08x)", error);
	} else if (error & kFLEXCAN_FormError) {
		LOG_DBG("RX form error (error 0x%08x)", error);
	} else if (error & kFLEXCAN_CrcError) {
		LOG_DBG("RX CRC error (error 0x%08x)", error);
	} else {
		LOG_DBG("Unhandled error (error 0x%08x)", error);
	}

	state = mcux_flexcan_get_state(dev, &err_cnt);
	if (data->state != state) {
		data->state = state;
		if (data->state_change_isr) {
			data->state_change_isr(state, err_cnt);
		}
	}

	if (status == CAN_TX_OK) {
		/*
		 * Error/status is not TX related. No further action
		 * required.
		 */
		return;
	}

	/*
	 * Since the FlexCAN module ESR1 register accumulates errors
	 * and warnings across multiple transmitted frames (until the
	 * CPU reads the register) it is not possible to find out
	 * which transfer caused the error/warning.
	 *
	 * We therefore propagate the error/warning to all currently
	 * active transmitters.
	 */
	for (alloc = 0; alloc < MCUX_FLEXCAN_MAX_TX; alloc++) {
		/* Copy callback function and argument before clearing bit */
		function = data->tx_cbs[alloc].function;
		arg = data->tx_cbs[alloc].arg;

		if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
			FLEXCAN_TransferAbortSend(config->base, &data->handle,
						  ALLOC_IDX_TO_TXMB_IDX(alloc));
			if (function != NULL) {
				function(status, arg);
			} else {
				data->tx_cbs[alloc].status = status;
				k_sem_give(&data->tx_cbs[alloc].done);
			}

			k_sem_give(&data->tx_allocs_sem);
		}
	}
}

static inline void mcux_flexcan_transfer_tx_idle(const struct device *dev,
						 uint32_t mb)
{
	struct mcux_flexcan_data *data = dev->data;
	can_tx_callback_t function;
	void *arg;
	int alloc;

	alloc = TX_MBIDX_TO_ALLOC_IDX(mb);

	/* Copy callback function and argument before clearing bit */
	function = data->tx_cbs[alloc].function;
	arg = data->tx_cbs[alloc].arg;

	if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
		if (function != NULL) {
			function(CAN_TX_OK, arg);
		} else {
			data->tx_cbs[alloc].status = CAN_TX_OK;
			k_sem_give(&data->tx_cbs[alloc].done);
		}
		k_sem_give(&data->tx_allocs_sem);
	}
}

static inline void mcux_flexcan_transfer_rx_idle(const struct device *dev,
						 uint32_t mb)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	can_rx_callback_t function;
	flexcan_mb_transfer_t xfer;
	struct zcan_frame frame;
	status_t status;
	void *arg;
	int alloc;

	alloc = RX_MBIDX_TO_ALLOC_IDX(mb);
	function = data->rx_cbs[alloc].function;
	arg = data->rx_cbs[alloc].arg;

	if (atomic_test_bit(data->rx_allocs, alloc)) {
		mcux_flexcan_copy_frame_to_zframe(&data->rx_cbs[alloc].frame,
						  &frame);
		function(&frame, arg);

		/* Setup RX message buffer to receive next message */
		FLEXCAN_SetRxMbConfig(config->base, mb,
				      &data->rx_cbs[alloc].mb_config, true);
		xfer.frame = &data->rx_cbs[alloc].frame;
		xfer.mbIdx = mb;
		status = FLEXCAN_TransferReceiveNonBlocking(config->base,
							    &data->handle,
							    &xfer);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to restart rx for filter id %d "
				"(err = %d)", alloc, status);
		}
	}
}

static void mcux_flexcan_transfer_callback(CAN_Type *base,
					   flexcan_handle_t *handle,
					   status_t status, uint32_t result,
					   void *userData)
{
	struct mcux_flexcan_data *data = (struct mcux_flexcan_data *)userData;

	switch (status) {
	case kStatus_FLEXCAN_UnHandled:
		__fallthrough;
	case kStatus_FLEXCAN_ErrorStatus:
		mcux_flexcan_transfer_error_status(data->dev, result);
		break;
	case kStatus_FLEXCAN_TxSwitchToRx:
		__fallthrough;
	case kStatus_FLEXCAN_TxIdle:
		mcux_flexcan_transfer_tx_idle(data->dev, result);
		break;
	case kStatus_FLEXCAN_RxOverflow:
		__fallthrough;
	case kStatus_FLEXCAN_RxIdle:
		mcux_flexcan_transfer_rx_idle(data->dev, result);
		break;
	default:
		LOG_WRN("Unhandled error/status (status 0x%08x, "
			 "result = 0x%08x", status, result);
	}
}

static void mcux_flexcan_isr(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;

	FLEXCAN_TransferHandleIRQ(config->base, &data->handle);
}

static int mcux_flexcan_init(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	int err;
	int i;

	k_mutex_init(&data->rx_mutex);
	k_sem_init(&data->tx_allocs_sem, 0, 1);

	for (i = 0; i < ARRAY_SIZE(data->tx_cbs); i++) {
		k_sem_init(&data->tx_cbs[i].done, 0, 1);
	}

	data->timing.sjw = config->sjw;
	if (config->sample_point && USE_SP_ALGO) {
		err = can_calc_timing(dev, &data->timing, config->bitrate,
				      config->sample_point);
		if (err == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, Seg1S1: %d, Seg2: %d",
			data->timing.prescaler, data->timing.phase_seg1,
			data->timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", err);
	} else {
		data->timing.prop_seg = config->prop_seg;
		data->timing.phase_seg1 = config->phase_seg1;
		data->timing.phase_seg2 = config->phase_seg2;
		err = can_calc_prescaler(dev, &data->timing, config->bitrate);
		if (err) {
			LOG_WRN("Bitrate error: %d", err);
		}
	}

	err = mcux_flexcan_set_mode(dev, CAN_NORMAL_MODE);
	if (err) {
		return err;
	}

	data->dev = dev;

	FLEXCAN_TransferCreateHandle(config->base, &data->handle,
				     mcux_flexcan_transfer_callback, data);

	config->irq_config_func(dev);

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	config->base->CTRL1 |= CAN_CTRL1_BOFFREC_MASK;
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	data->state = mcux_flexcan_get_state(dev, NULL);

	return 0;
}

static const struct can_driver_api mcux_flexcan_driver_api = {
	.set_mode = mcux_flexcan_set_mode,
	.set_timing = mcux_flexcan_set_timing,
	.send = mcux_flexcan_send,
	.attach_isr = mcux_flexcan_attach_isr,
	.detach = mcux_flexcan_detach,
	.get_state = mcux_flexcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcux_flexcan_recover,
#endif
	.register_state_change_isr = mcux_flexcan_register_state_change_isr,
	.get_core_clock = mcux_flexcan_get_core_clock,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x03,
		.prop_seg = 0x07,
		.phase_seg1 = 0x07,
		.phase_seg2 = 0x07,
		.prescaler = 0xFF
	}
};

#define FLEXCAN_IRQ_CODE(id, name) \
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, name, irq),		\
		DT_INST_IRQ_BY_NAME(id, name, priority),		\
		mcux_flexcan_isr,					\
		DEVICE_DT_INST_GET(id), id);				\
		irq_enable(DT_INST_IRQ_BY_NAME(id, name, irq));		\
	} while (0)

#define FLEXCAN_IRQ(id, name) \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(id, name), \
		(FLEXCAN_IRQ_CODE(id, name)), ())

#define FLEXCAN_DEVICE_INIT_MCUX(id)					\
	static void mcux_flexcan_irq_config_##id(const struct device *dev); \
									\
	static const struct mcux_flexcan_config mcux_flexcan_config_##id = { \
		.base = (CAN_Type *)DT_INST_REG_ADDR(id),		\
		.clock_name = DT_INST_CLOCKS_LABEL(id),			\
		.clock_subsys = (clock_control_subsys_t)		\
			DT_INST_CLOCKS_CELL(id, name),			\
		.clk_source = DT_INST_PROP(id, clk_source),		\
		.bitrate = DT_INST_PROP(id, bus_speed),			\
		.sjw = DT_INST_PROP(id, sjw),				\
		.prop_seg = DT_INST_PROP_OR(id, prop_seg, 0),		\
		.phase_seg1 = DT_INST_PROP_OR(id, phase_seg1, 0),	\
		.phase_seg2 = DT_INST_PROP_OR(id, phase_seg2, 0),	\
		.sample_point = DT_INST_PROP_OR(id, sample_point, 0),	\
		.irq_config_func = mcux_flexcan_irq_config_##id,	\
	};								\
									\
	static struct mcux_flexcan_data mcux_flexcan_data_##id;		\
									\
	DEVICE_DT_INST_DEFINE(id, &mcux_flexcan_init,			\
			device_pm_control_nop, &mcux_flexcan_data_##id,	\
			&mcux_flexcan_config_##id, POST_KERNEL,		\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&mcux_flexcan_driver_api);			\
									\
	static void mcux_flexcan_irq_config_##id(const struct device *dev) \
	{								\
		FLEXCAN_IRQ(id, rx_warning);				\
		FLEXCAN_IRQ(id, tx_warning);				\
		FLEXCAN_IRQ(id, bus_off);				\
		FLEXCAN_IRQ(id, warning);				\
		FLEXCAN_IRQ(id, error);					\
		FLEXCAN_IRQ(id, wake_up);				\
		FLEXCAN_IRQ(id, mb_0_15);				\
		FLEXCAN_IRQ(id, common);				\
	}

DT_INST_FOREACH_STATUS_OKAY(FLEXCAN_DEVICE_INIT_MCUX)

#if defined(CONFIG_NET_SOCKETS_CAN)
#include "socket_can_generic.h"						\
#define FLEXCAN_DEVICE_SOCKET_CAN(id)					\
	static int socket_can_init_##id(const struct device *dev)	\
	{								\
		struct device *can_dev = DEVICE_DT_INST_GET(id);	\
		struct socket_can_context *socket_context = dev->data;	\
		LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)", \
			dev, dev->name, can_dev, can_dev->name);	\
		socket_context->can_dev = can_dev;			\
		socket_context->msgq = &socket_can_msgq;		\
		socket_context->rx_tid =				\
		k_thread_create(&socket_context->rx_thread_data,	\
				rx_thread_stack,			\
				K_KERNEL_STACK_SIZEOF(rx_thread_stack),	\
				rx_thread, socket_context, NULL, NULL,	\
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);	\
		return 0;						\
	}								\
									\
	NET_DEVICE_INIT(socket_can_flexcan_##id, SOCKET_CAN_NAME_##id,	\
		socket_can_init_##id, device_pm_control_nop,		\
		&socket_can_context_##id, NULL,				\
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &socket_can_api,	\
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2),	\
		CAN_MTU);						\

DT_INST_FOREACH_STATUS_OKAY(FLEXCAN_DEVICE_SOCKET_CAN)
#endif
