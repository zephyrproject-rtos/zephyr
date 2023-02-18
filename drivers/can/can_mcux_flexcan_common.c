/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2022 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can_mcux_flexcan_common.h"

LOG_MODULE_REGISTER(can_mcux_flexcan, CONFIG_CAN_LOG_LEVEL);

int mcux_flexcan_common_verify_frame_filter_flags(const bool is_fd_compatible, const uint8_t flags)
{
	int status = 0;
	uint32_t supported_filters = CAN_FILTER_IDE | CAN_FILTER_DATA | CAN_FILTER_RTR;

	if (is_fd_compatible) {
		supported_filters |= CAN_FILTER_FDF;
	}

	if ((flags & ~(supported_filters)) != 0U) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", flags);
		status = -ENOTSUP;
	}

	return status;
}

int mcux_flexcan_common_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

int mcux_flexcan_common_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct mcux_flexcan_generic_config *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

int mcux_flexcan_common_set_timing(struct can_timing *flexcan_timing,
				   const struct can_timing *reference_timing, bool is_started)
{
	uint8_t sjw_backup = flexcan_timing->sjw;

	if (reference_timing == NULL) {
		return -EINVAL;
	}

	if (is_started) {
		return -EBUSY;
	}

	*flexcan_timing = *reference_timing;
	if (reference_timing->sjw == CAN_SJW_NO_CHANGE) {
		flexcan_timing->sjw = sjw_backup;
	}

	return 0;
}

void mcux_flexcan_config_ctrl1(can_mode_t mode, CAN_Type *can_base)
{
	uint32_t ctrl1 = can_base->CTRL1;

	if ((mode & CAN_MODE_LOOPBACK) != 0U) {
		/* Enable loopback and self-reception */
		ctrl1 |= CAN_CTRL1_LPB_MASK;
	} else {
		/* Disable loopback and self-reception */
		ctrl1 &= ~(CAN_CTRL1_LPB_MASK);
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0U) {
		/* Enable listen-only mode */
		ctrl1 |= CAN_CTRL1_LOM_MASK;
	} else {
		/* Disable listen-only mode */
		ctrl1 &= ~(CAN_CTRL1_LOM_MASK);
	}

	if ((mode & CAN_MODE_3_SAMPLES) != 0U) {
		/* Enable triple sampling mode */
		ctrl1 |= CAN_CTRL1_SMP_MASK;
	} else {
		/* Disable triple sampling mode */
		ctrl1 &= ~(CAN_CTRL1_SMP_MASK);
	}

	can_base->CTRL1 = ctrl1;
}

void mcux_flexcan_config_mcr(can_mode_t mode, CAN_Type *can_base)
{
	uint32_t mcr = can_base->MCR;

	if ((mode & CAN_MODE_LOOPBACK) != 0U) {
		/* Enable loopback and self-reception */
		mcr &= ~(CAN_MCR_SRXDIS_MASK);
	} else {
		/* Disable loopback and self-reception */
		mcr |= CAN_MCR_SRXDIS_MASK;
	}
	can_base->MCR = mcr;
}

int mcux_flexcan_common_verify_can_frame_flags(const uint8_t dlc, const uint8_t flags,
					       const uint32_t frame_id, const bool is_fd_frame)
{
	uint8_t max_dlc;
	uint8_t supported_flags;

	if (is_fd_frame) {
		max_dlc = CANFD_MAX_DLC;
		supported_flags = CAN_FRAME_IDE | CAN_FRAME_RTR | CAN_FRAME_FDF | CAN_FRAME_BRS;
	} else {
		max_dlc = CAN_MAX_DLC;
		supported_flags = CAN_FRAME_IDE | CAN_FRAME_RTR;
	}

	if ((flags & ~(supported_flags)) != 0U) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", flags);
		return -ENOTSUP;
	}

	if (((flags & CAN_FRAME_FDF) == 0U) && (dlc > max_dlc)) {
		LOG_ERR("Non FD frame with incorrect dlc %d", dlc);
		return -EINVAL;
	}

	if (((flags & CAN_FRAME_IDE) != 0U) && (frame_id <= 0x7FFU)) {
		LOG_ERR("Standard frame id used with frame tagged Extended Frame id: 0x%x",
			frame_id);
		return -EINVAL;
	}

	if (((flags & CAN_FRAME_IDE) == 0U) && (frame_id >= 0x7FFU)) {
		LOG_ERR("Extended Frame id used with frame tagged Standard Frame id: 0x%x",
			frame_id);
		return -EINVAL;
	}

	return 0;
}

void mcux_flexcan_common_can_filter_to_mbconfig(const struct can_filter *src,
						flexcan_rx_mb_config_t *dest, uint32_t *mask)
{
	static const uint32_t ide_mask = 1U;
	uint32_t rtr_mask = (src->flags & (CAN_FILTER_DATA | CAN_FILTER_RTR)) !=
					    (CAN_FILTER_DATA | CAN_FILTER_RTR)
				    ? 1U
				    : 0U;

	if ((src->flags & CAN_FILTER_IDE) != 0U) {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
		*mask = FLEXCAN_RX_MB_EXT_MASK(src->mask, rtr_mask, ide_mask);
	} else {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
		*mask = FLEXCAN_RX_MB_STD_MASK(src->mask, rtr_mask, ide_mask);
	}

	if ((src->flags & CAN_FILTER_RTR) != 0U) {
		dest->type = kFLEXCAN_FrameTypeRemote;
	} else {
		dest->type = kFLEXCAN_FrameTypeData;
	}
}

int mcux_flexcan_common_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcux_flexcan_generic_config *config = dev->config;

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

void mcux_flexcan_common_get_state(const struct mcux_flexcan_generic_config *config,
				   bool is_started, enum can_state *state,
				   struct can_bus_err_cnt *const err_cnt)
{
	uint64_t status_flags;

	if (state != NULL) {
		if (!is_started) {
			*state = CAN_STATE_STOPPED;
		} else {
			status_flags = FLEXCAN_GetStatusFlags(config->base);

			if ((status_flags & CAN_ESR1_FLTCONF(2)) != 0U) {
				*state = CAN_STATE_BUS_OFF;
			} else if ((status_flags & CAN_ESR1_FLTCONF(1)) != 0U) {
				*state = CAN_STATE_ERROR_PASSIVE;
			} else if ((status_flags & (kFLEXCAN_TxErrorWarningFlag |
						    kFLEXCAN_RxErrorWarningFlag)) != 0U) {
				*state = CAN_STATE_ERROR_WARNING;
			} else {
				*state = CAN_STATE_ERROR_ACTIVE;
			}
		}
	}

	if (err_cnt != NULL) {
		FLEXCAN_GetBusErrCount(config->base, &err_cnt->tx_err_cnt, &err_cnt->rx_err_cnt);
	}
}

int mcux_flexcan_common_set_can_mode(const struct mcux_flexcan_generic_config *config,
				     can_mode_t mode, bool is_started, bool is_can_fd_configured)
{
	if (is_started) {
		return -EBUSY;
	}

	if (((is_can_fd_configured) &&
	     ((mode & ~(CAN_MODE_FD | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY |
			CAN_MODE_3_SAMPLES)) != 0U)) ||
	    ((!is_can_fd_configured) &&
	     (mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_3_SAMPLES)) != 0U)) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	mcux_flexcan_config_ctrl1(mode, config->base);
	mcux_flexcan_config_mcr(mode, config->base);

	return 0;
}

int mcux_flexcan_common_check_can_start(const struct mcux_flexcan_generic_config *const config,
					bool is_started)
{
	int err;

	if (is_started) {
		return -EALREADY;
	}

	if (config->phy != NULL) {
		err = can_transceiver_enable(config->phy);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	/* Clear error counters */
	config->base->ECR &= ~(CAN_ECR_TXERRCNT_MASK | CAN_ECR_RXERRCNT_MASK);

	return 0;
}

void mcux_flexcan_common_extract_timing_from_can_timing(flexcan_timing_config_t *flexcan_Timing,
							struct can_timing *timing)
{
	flexcan_Timing->preDivider = timing->prescaler - 1U;
	flexcan_Timing->rJumpwidth = timing->sjw - 1U;
	flexcan_Timing->phaseSeg1 = timing->phase_seg1 - 1U;
	flexcan_Timing->phaseSeg2 = timing->phase_seg2 - 1U;
	flexcan_Timing->propSeg = timing->prop_seg - 1U;
}

void increment_error_counters(const struct device *dev, const uint64_t error)
{
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

	/* Handle FlexCAN FD errors */
#ifdef CONFIG_CAN_FD_MODE
	if ((error & kFLEXCAN_FDBit0Error) != 0U) {
		CAN_STATS_BIT0_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_FDBit1Error) != 0U) {
		CAN_STATS_BIT1_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_FDStuffingError) != 0U) {
		CAN_STATS_STUFF_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_FDFormError) != 0U) {
		CAN_STATS_FORM_ERROR_INC(dev);
	}

	if ((error & kFLEXCAN_FDCrcError) != 0U) {
		CAN_STATS_CRC_ERROR_INC(dev);
	}
#endif /* CONFIG_CAN_FD_MODE */
}

int mcux_flexcan_common_init_check_ready(const struct device *can_transceiver,
					 const struct device *can_clock)
{
	if (can_transceiver != NULL) {
		if (!device_is_ready(can_transceiver)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(can_clock)) {
		LOG_ERR("clock device not ready");
		return -ENODEV;
	}

	return 0;
}

int mcux_flexcan_common_calc_timing(const struct device *can_dev, struct can_timing *timing,
				    uint32_t bitrate, uint32_t sample_point)
{
	int err;

	err = can_calc_timing(can_dev, timing, bitrate, sample_point);
	if (err == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}
	LOG_DBG("Presc: %d, Seg1S1: %d, Seg2: %d", timing->prescaler, timing->phase_seg1,
		timing->phase_seg2);
	LOG_DBG("Sample-point err : %d", err);
	return 0;
}

void mcux_flexcan_common_config_calc_bitrate(const struct device *dev,
					     const struct mcux_flexcan_generic_config *const config,
					     struct can_timing *const timing)
{
	int err;

	timing->prop_seg = config->prop_seg;
	timing->phase_seg1 = config->phase_seg1;
	timing->phase_seg2 = config->phase_seg2;
	err = can_calc_prescaler(dev, timing, config->bitrate);

	if (err != 0) {
		LOG_WRN("Bitrate error: %d", err);
	}
}

void mcux_flexcan_common_init_config(flexcan_config_t *const flexcan_config,
				     const struct can_timing *const timing,
				     const uint32_t clock_freq, const int clock_source,
				     const uint8_t max_mb)
{
	FLEXCAN_GetDefaultConfig(flexcan_config);
	flexcan_config->maxMbNum = max_mb;
	flexcan_config->clkSrc = clock_source;
	flexcan_config->baudRate =
		clock_freq / (1U + timing->prop_seg + timing->phase_seg1 + timing->phase_seg2) /
		timing->prescaler;
	flexcan_config->enableIndividMask = true;
	flexcan_config->enableLoopBack = false;
	flexcan_config->disableSelfReception = true;
	flexcan_config->enableListenOnlyMode = true;

	flexcan_config->timingConfig.rJumpwidth = timing->sjw - 1U;
	flexcan_config->timingConfig.propSeg = timing->prop_seg - 1U;
	flexcan_config->timingConfig.phaseSeg1 = timing->phase_seg1 - 1U;
	flexcan_config->timingConfig.phaseSeg2 = timing->phase_seg2 - 1U;
}

void mcux_flexcan_from_can_frame(const struct can_frame *src, flexcan_frame_t *dest)
{
	(void)memset(dest, 0, sizeof(*dest));

	if ((src->flags & CAN_FRAME_IDE) != 0U) {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
	} else {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
	}

	if ((src->flags & CAN_FRAME_RTR) != 0U) {
		dest->type = kFLEXCAN_FrameTypeRemote;
	} else {
		dest->type = kFLEXCAN_FrameTypeData;
	}

	dest->length = src->dlc;
	dest->dataWord0 = sys_cpu_to_be32(src->data_32[0]);
	dest->dataWord1 = sys_cpu_to_be32(src->data_32[1]);
}

void mcux_flexcan_to_can_frame(const flexcan_frame_t *src, struct can_frame *dest)
{
	(void)memset(dest, 0, sizeof(*dest));

	if (src->format == kFLEXCAN_FrameFormatStandard) {
		dest->id = FLEXCAN_ID_TO_CAN_ID_STD(src->id);
	} else {
		dest->flags |= CAN_FRAME_IDE;
		dest->id = FLEXCAN_ID_TO_CAN_ID_EXT(src->id);
	}

	if (src->type == kFLEXCAN_FrameTypeRemote) {
		dest->flags |= CAN_FRAME_RTR;
	}

	dest->dlc = src->length;
	dest->data_32[0] = sys_be32_to_cpu(src->dataWord0);
	dest->data_32[1] = sys_be32_to_cpu(src->dataWord1);
#ifdef CONFIG_CAN_RX_TIMESTAMP
	dest->timestamp = src->timestamp;
#endif /* CONFIG_CAN_RX_TIMESTAMP */
}
