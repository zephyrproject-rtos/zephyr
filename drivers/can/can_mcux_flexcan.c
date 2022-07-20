/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_flexcan

#include <zephyr/zephyr.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <fsl_flexcan.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

LOG_MODULE_REGISTER(can_mcux_flexcan, CONFIG_CAN_LOG_LEVEL);

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

#if ((defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) || \
	(defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829))
/* the first valid MB should be occupied by ERRATA 5461 or 5829. */
#define RX_START_IDX 1
#else
#define RX_START_IDX 0
#endif

/*
 * RX message buffers (filters) will take up the first N message
 * buffers. The rest are available for TX use.
 */
#define MCUX_FLEXCAN_MAX_RX (CONFIG_CAN_MAX_FILTER + RX_START_IDX)
#define MCUX_FLEXCAN_MAX_TX \
	(FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(0) \
	- MCUX_FLEXCAN_MAX_RX)

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
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	int clk_source;
	uint32_t bitrate;
	uint32_t sample_point;
	uint32_t sjw;
	uint32_t prop_seg;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	void (*irq_config_func)(const struct device *dev);
	const struct device *phy;
	uint32_t max_bitrate;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
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
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
	struct can_timing timing;
};

static int mcux_flexcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcux_flexcan_config *config = dev->config;

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

static int mcux_flexcan_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	ARG_UNUSED(id_type);

	return CONFIG_CAN_MAX_FILTER;
}

static int mcux_flexcan_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct mcux_flexcan_config *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

static int mcux_flexcan_set_timing(const struct device *dev,
				   const struct can_timing *timing)
{
	struct mcux_flexcan_data *data = dev->data;
	const struct mcux_flexcan_config *config = dev->config;
	uint8_t sjw_backup = data->timing.sjw;
	flexcan_timing_config_t timing_tmp;

	if (!timing) {
		return -EINVAL;
	}

	data->timing = *timing;
	if (timing->sjw == CAN_SJW_NO_CHANGE) {
		data->timing.sjw = sjw_backup;
	}

	timing_tmp.preDivider = data->timing.prescaler - 1U;
	timing_tmp.rJumpwidth = data->timing.sjw - 1U;
	timing_tmp.phaseSeg1 = data->timing.phase_seg1 - 1U;
	timing_tmp.phaseSeg2 = data->timing.phase_seg2 - 1U;
	timing_tmp.propSeg = data->timing.prop_seg - 1U;

	FLEXCAN_SetTimingConfig(config->base, &timing_tmp);

	return 0;
}

static int mcux_flexcan_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_3_SAMPLES;

	return 0;
}

static int mcux_flexcan_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct mcux_flexcan_config *config = dev->config;
	uint32_t ctrl1;
	uint32_t mcr;
	int err;

	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_3_SAMPLES)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (config->phy != NULL) {
		err = can_transceiver_enable(config->phy);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	FLEXCAN_EnterFreezeMode(config->base);

	ctrl1 = config->base->CTRL1;
	mcr = config->base->MCR;

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* Enable loopback and self-reception */
		ctrl1 |= CAN_CTRL1_LPB_MASK;
		mcr &= ~(CAN_MCR_SRXDIS_MASK);
	} else {
		/* Disable loopback and self-reception */
		ctrl1 &= ~(CAN_CTRL1_LPB_MASK);
		mcr |= CAN_MCR_SRXDIS_MASK;
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		/* Enable listen-only mode */
		ctrl1 |= CAN_CTRL1_LOM_MASK;
	} else {
		/* Disable listen-only mode */
		ctrl1 &= ~(CAN_CTRL1_LOM_MASK);
	}

	if ((mode & CAN_MODE_3_SAMPLES) != 0) {
		/* Enable triple sampling mode */
		ctrl1 |= CAN_CTRL1_SMP_MASK;
	} else {
		/* Disable triple sampling mode */
		ctrl1 &= ~(CAN_CTRL1_SMP_MASK);
	}

	config->base->CTRL1 = ctrl1;
	config->base->MCR = mcr;

	FLEXCAN_ExitFreezeMode(config->base);

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
		*mask = FLEXCAN_RX_MB_STD_MASK(src->id_mask, src->rtr_mask, 1);
	} else {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
		*mask = FLEXCAN_RX_MB_EXT_MASK(src->id_mask, src->rtr_mask, 1);
	}

	if ((src->rtr & src->rtr_mask) == CAN_DATAFRAME) {
		dest->type = kFLEXCAN_FrameTypeData;
	} else {
		dest->type = kFLEXCAN_FrameTypeRemote;
	}
}

static int mcux_flexcan_get_state(const struct device *dev, enum can_state *state,
				  struct can_bus_err_cnt *err_cnt)
{
	const struct mcux_flexcan_config *config = dev->config;
	uint64_t status_flags;

	if (state != NULL) {
		status_flags = FLEXCAN_GetStatusFlags(config->base);

		if ((status_flags & CAN_ESR1_FLTCONF(2)) != 0U) {
			*state = CAN_BUS_OFF;
		} else if ((status_flags & CAN_ESR1_FLTCONF(1)) != 0U) {
			*state = CAN_ERROR_PASSIVE;
		} else if ((status_flags &
			(kFLEXCAN_TxErrorWarningFlag | kFLEXCAN_RxErrorWarningFlag)) != 0) {
			*state = CAN_ERROR_WARNING;
		} else {
			*state = CAN_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		FLEXCAN_GetBusErrCount(config->base, &err_cnt->tx_err_cnt,
				       &err_cnt->rx_err_cnt);
	}

	return 0;
}

static int mcux_flexcan_send(const struct device *dev,
			     const struct zcan_frame *frame,
			     k_timeout_t timeout,
			     can_tx_callback_t callback, void *user_data)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	flexcan_mb_transfer_t xfer;
	enum can_state state;
	status_t status;
	int alloc;

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	(void)mcux_flexcan_get_state(dev, &state, NULL);
	if (state == CAN_BUS_OFF) {
		LOG_DBG("Transmit failed, bus-off");
		return -ENETDOWN;
	}

	if (k_sem_take(&data->tx_allocs_sem, timeout) != 0) {
		return -EAGAIN;
	}

	for (alloc = 0; alloc < MCUX_FLEXCAN_MAX_TX; alloc++) {
		if (!atomic_test_and_set_bit(data->tx_allocs, alloc)) {
			break;
		}
	}

	mcux_flexcan_copy_zframe_to_frame(frame, &data->tx_cbs[alloc].frame);
	data->tx_cbs[alloc].function = callback;
	data->tx_cbs[alloc].arg = user_data;
	xfer.frame = &data->tx_cbs[alloc].frame;
	xfer.mbIdx = ALLOC_IDX_TO_TXMB_IDX(alloc);
	FLEXCAN_SetTxMbConfig(config->base, xfer.mbIdx, true);
	status = FLEXCAN_TransferSendNonBlocking(config->base, &data->handle,
						 &xfer);
	if (status != kStatus_Success) {
		return -EIO;
	}

	if (callback == NULL) {
		k_sem_take(&data->tx_cbs[alloc].done, K_FOREVER);
		return data->tx_cbs[alloc].status;
	}

	return 0;
}

static int mcux_flexcan_add_rx_filter(const struct device *dev,
				      can_rx_callback_t callback,
				      void *user_data,
				      const struct zcan_filter *filter)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	flexcan_mb_transfer_t xfer;
	status_t status;
	uint32_t mask;
	int alloc = -ENOSPC;
	int i;

	__ASSERT_NO_MSG(callback);

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	/* Find and allocate RX message buffer */
	for (i = RX_START_IDX; i < MCUX_FLEXCAN_MAX_RX; i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			alloc = i;
			break;
		}
	}

	if (alloc == -ENOSPC) {
		return alloc;
	}

	mcux_flexcan_copy_zfilter_to_mbconfig(filter,
					      &data->rx_cbs[alloc].mb_config,
					      &mask);

	data->rx_cbs[alloc].arg = user_data;
	data->rx_cbs[alloc].function = callback;

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
		alloc = -ENOSPC;
	}

	k_mutex_unlock(&data->rx_mutex);

	return alloc;
}

static void mcux_flexcan_set_state_change_callback(const struct device *dev,
						   can_state_change_callback_t callback,
						   void *user_data)
{
	struct mcux_flexcan_data *data = dev->data;

	data->state_change_cb = callback;
	data->state_change_cb_data = user_data;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int mcux_flexcan_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct mcux_flexcan_config *config = dev->config;
	enum can_state state;
	uint64_t start_time;
	int ret = 0;

	(void)mcux_flexcan_get_state(dev, &state, NULL);
	if (state != CAN_BUS_OFF) {
		return 0;
	}

	start_time = k_uptime_ticks();
	config->base->CTRL1 &= ~CAN_CTRL1_BOFFREC_MASK;

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		(void)mcux_flexcan_get_state(dev, &state, NULL);

		while (state == CAN_BUS_OFF) {
			if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
			    k_uptime_ticks() - start_time >= timeout.ticks) {
				ret = -EAGAIN;
			}

			(void)mcux_flexcan_get_state(dev, &state, NULL);
		}
	}

	config->base->CTRL1 |= CAN_CTRL1_BOFFREC_MASK;

	return ret;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void mcux_flexcan_remove_rx_filter(const struct device *dev, int filter_id)
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
						      uint64_t error)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *cb_data = data->state_change_cb_data;
	can_tx_callback_t function;
	void *arg;
	int alloc;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;

	if ((error & kFLEXCAN_Bit0Error) != 0U) {
		CAN_STATS_BIT0_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_Bit1Error) != 0U) {
		CAN_STATS_BIT1_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_AckError) != 0U) {
		CAN_STATS_ACK_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_StuffingError) != 0U) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_FormError) != 0U) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_CrcError) != 0U) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}

	(void)mcux_flexcan_get_state(dev, &state, &err_cnt);
	if (data->state != state) {
		data->state = state;

		if (cb != NULL) {
			cb(dev, state, err_cnt, cb_data);
		}
	}

	if (state == CAN_BUS_OFF) {
		/* Abort any pending TX frames in case of bus-off */
		for (alloc = 0; alloc < MCUX_FLEXCAN_MAX_TX; alloc++) {
			/* Copy callback function and argument before clearing bit */
			function = data->tx_cbs[alloc].function;
			arg = data->tx_cbs[alloc].arg;

			if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
				FLEXCAN_TransferAbortSend(config->base, &data->handle,
							  ALLOC_IDX_TO_TXMB_IDX(alloc));
				if (function != NULL) {
					function(dev, -ENETDOWN, arg);
				} else {
					data->tx_cbs[alloc].status = -ENETDOWN;
					k_sem_give(&data->tx_cbs[alloc].done);
				}

				k_sem_give(&data->tx_allocs_sem);
			}
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
			function(dev, 0, arg);
		} else {
			data->tx_cbs[alloc].status = 0;
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
		function(dev, &frame, arg);

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

static FLEXCAN_CALLBACK(mcux_flexcan_transfer_callback)
{
	struct mcux_flexcan_data *data = (struct mcux_flexcan_data *)userData;
	const struct mcux_flexcan_config *config = data->dev->config;
	/*
	 * The result field can either be a MB index (which is limited to 32 bit
	 * value) or a status flags value, which is 32 bit on some platforms but
	 * 64 on others. To decouple the remaining functions from this, the
	 * result field is always promoted to uint64_t.
	 */
	uint32_t mb = (uint32_t)result;
	uint64_t status_flags = result;

	ARG_UNUSED(base);

	switch (status) {
	case kStatus_FLEXCAN_UnHandled:
		/* Not all fault confinement state changes are handled by the HAL */
		__fallthrough;
	case kStatus_FLEXCAN_ErrorStatus:
		mcux_flexcan_transfer_error_status(data->dev, status_flags);
		break;
	case kStatus_FLEXCAN_TxSwitchToRx:
		FLEXCAN_TransferAbortReceive(config->base, &data->handle, mb);
		__fallthrough;
	case kStatus_FLEXCAN_TxIdle:
		mcux_flexcan_transfer_tx_idle(data->dev, mb);
		break;
	case kStatus_FLEXCAN_RxOverflow:
		__fallthrough;
	case kStatus_FLEXCAN_RxIdle:
		mcux_flexcan_transfer_rx_idle(data->dev, mb);
		break;
	default:
		LOG_WRN("Unhandled status 0x%08x (result = 0x%016llx)",
			status, status_flags);
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
	flexcan_config_t flexcan_config;
	uint32_t clock_freq;
	int err;
	int i;

	if (config->phy != NULL) {
		if (!device_is_ready(config->phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock device not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->rx_mutex);
	k_sem_init(&data->tx_allocs_sem, MCUX_FLEXCAN_MAX_TX,
		   MCUX_FLEXCAN_MAX_TX);

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

#ifdef CONFIG_PINCTRL
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}
#endif

	err = mcux_flexcan_get_core_clock(dev, &clock_freq);
	if (err != 0) {
		return -EIO;
	}

	data->dev = dev;

	FLEXCAN_GetDefaultConfig(&flexcan_config);
	flexcan_config.maxMbNum = FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(0);
	flexcan_config.clkSrc = config->clk_source;
	flexcan_config.baudRate = clock_freq /
	      (1U + data->timing.prop_seg + data->timing.phase_seg1 +
	       data->timing.phase_seg2) / data->timing.prescaler;
	flexcan_config.enableIndividMask = true;
	flexcan_config.enableLoopBack = false;
	flexcan_config.disableSelfReception = true;
	flexcan_config.enableListenOnlyMode = false;

	flexcan_config.timingConfig.rJumpwidth = data->timing.sjw - 1U;
	flexcan_config.timingConfig.propSeg = data->timing.prop_seg - 1U;
	flexcan_config.timingConfig.phaseSeg1 = data->timing.phase_seg1 - 1U;
	flexcan_config.timingConfig.phaseSeg2 = data->timing.phase_seg2 - 1U;

	FLEXCAN_Init(config->base, &flexcan_config, clock_freq);
	FLEXCAN_TransferCreateHandle(config->base, &data->handle,
				     mcux_flexcan_transfer_callback, data);

	config->irq_config_func(dev);

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	config->base->CTRL1 |= CAN_CTRL1_BOFFREC_MASK;
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

	(void)mcux_flexcan_get_state(dev, &data->state, NULL);

	return 0;
}

static const struct can_driver_api mcux_flexcan_driver_api = {
	.get_capabilities = mcux_flexcan_get_capabilities,
	.set_mode = mcux_flexcan_set_mode,
	.set_timing = mcux_flexcan_set_timing,
	.send = mcux_flexcan_send,
	.add_rx_filter = mcux_flexcan_add_rx_filter,
	.remove_rx_filter = mcux_flexcan_remove_rx_filter,
	.get_state = mcux_flexcan_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcux_flexcan_recover,
#endif
	.set_state_change_callback = mcux_flexcan_set_state_change_callback,
	.get_core_clock = mcux_flexcan_get_core_clock,
	.get_max_filters = mcux_flexcan_get_max_filters,
	.get_max_bitrate = mcux_flexcan_get_max_bitrate,
	/*
	 * FlexCAN timing limits are specified in the "FLEXCANx_CTRL1 field
	 * descriptions" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the flexcan_config_t field assignments elsewhere in this
	 * driver).
	 */
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x04,
		.prop_seg = 0x08,
		.phase_seg1 = 0x08,
		.phase_seg2 = 0x08,
		.prescaler = 0x100
	}
};

#define FLEXCAN_IRQ_CODE(id, name) \
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(id, name, irq),		\
		DT_INST_IRQ_BY_NAME(id, name, priority),		\
		mcux_flexcan_isr,					\
		DEVICE_DT_INST_GET(id), 0);				\
		irq_enable(DT_INST_IRQ_BY_NAME(id, name, irq));		\
	} while (false)

#define FLEXCAN_IRQ(id, name) \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(id, name), \
		(FLEXCAN_IRQ_CODE(id, name)), ())

#ifdef CONFIG_PINCTRL
#define FLEXCAN_PINCTRL_DEFINE(id) PINCTRL_DT_INST_DEFINE(id);
#define FLEXCAN_PINCTRL_INIT(id) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),
#else
#define FLEXCAN_PINCTRL_DEFINE(id)
#define FLEXCAN_PINCTRL_INIT(id)
#endif /* CONFIG_PINCTRL */


#define FLEXCAN_DEVICE_INIT_MCUX(id)					\
	FLEXCAN_PINCTRL_DEFINE(id)					\
									\
	static void mcux_flexcan_irq_config_##id(const struct device *dev); \
									\
	static const struct mcux_flexcan_config mcux_flexcan_config_##id = { \
		.base = (CAN_Type *)DT_INST_REG_ADDR(id),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
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
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(id, phys)),\
		.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(id, 1000000), \
		FLEXCAN_PINCTRL_INIT(id)				\
	};								\
									\
	static struct mcux_flexcan_data mcux_flexcan_data_##id;		\
									\
	CAN_DEVICE_DT_INST_DEFINE(id, mcux_flexcan_init,		\
				  NULL, &mcux_flexcan_data_##id,	\
				  &mcux_flexcan_config_##id,		\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,\
				  &mcux_flexcan_driver_api);		\
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
