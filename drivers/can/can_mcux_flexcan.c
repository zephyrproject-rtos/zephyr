/*
 * Copyright (c) 2019-2026 Vestas Wind Systems A/S
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Base driver compatible */
#define DT_DRV_COMPAT nxp_flexcan

/* CAN FD extension compatible */
#define FLEXCAN_FD_DRV_COMPAT nxp_flexcan_fd

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <fsl_flexcan.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(can_mcux_flexcan, CONFIG_CAN_LOG_LEVEL);

#if ((defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) || \
	(defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829))
/* the first valid MB should be occupied by ERRATA 5461 or 5829. */
#define RX_START_IDX 1
#else
#define RX_START_IDX 0
#endif

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
#define TX_MBIDX_TO_ALLOC_IDX(x) (x - ((const struct mcux_flexcan_config *)dev->config)->rx_mb)
#define ALLOC_IDX_TO_TXMB_IDX(x) (x + ((const struct mcux_flexcan_config *)dev->config)->rx_mb)

/* Convert from back from FLEXCAN IDs to Zephyr CAN IDs. */
#define FLEXCAN_ID_TO_CAN_ID_STD(id) \
	((uint32_t)((((uint32_t)(id)) & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT))
#define FLEXCAN_ID_TO_CAN_ID_EXT(id) \
	((uint32_t)((((uint32_t)(id)) & (CAN_ID_STD_MASK | CAN_ID_EXT_MASK)) \
	>> CAN_ID_EXT_SHIFT))

#define DEV_CFG(_dev) ((const struct mcux_flexcan_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_flexcan_data *)(_dev)->data)

struct mcux_flexcan_config {
	const struct can_driver_config common;

	DEVICE_MMIO_NAMED_ROM(flexcan_mmio);

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t clk_source;
	uint32_t number_of_mb;
	uint32_t rx_mb;
	uint32_t tx_mb;
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	bool flexcan_fd;
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
	void (*irq_config_func)(const struct device *dev);
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
	const struct pinctrl_dev_config *pincfg;
};

struct mcux_flexcan_rx_callback {
	flexcan_rx_mb_config_t mb_config;
	union {
		flexcan_frame_t classic;
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		flexcan_fd_frame_t fd;
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
	} frame;
	can_rx_callback_t function;
	void *arg;
};

struct mcux_flexcan_tx_callback {
	can_tx_callback_t function;
	void *arg;
};

struct mcux_flexcan_data {
	struct can_driver_data common;

	DEVICE_MMIO_NAMED_RAM(flexcan_mmio);

	const struct device *dev;
	flexcan_handle_t handle;

	enum can_state state;
	struct can_timing timing;

	atomic_t *rx_allocs;
	atomic_t *tx_allocs;
	struct mcux_flexcan_rx_callback *rx_cbs;
	struct mcux_flexcan_tx_callback *tx_cbs;

	struct k_mutex rx_mutex;
	struct k_mutex tx_mutex;
	struct k_sem tx_allocs_sem;

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	struct can_timing timing_data;
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
};

static inline CAN_Type *get_base(const struct device *dev)
{
	return (CAN_Type *)DEVICE_MMIO_NAMED_GET(dev, flexcan_mmio);
}

static int mcux_flexcan_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcux_flexcan_config *config = dev->config;

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

static int mcux_flexcan_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MCUX_FLEXCAN_MAX_FILTERS;
}

static int mcux_flexcan_set_timing(const struct device *dev,
				   const struct can_timing *timing)
{
	struct mcux_flexcan_data *data = dev->data;

	if (!timing) {
		return -EINVAL;
	}

	if (data->common.started) {
		return -EBUSY;
	}

	data->timing = *timing;

	return 0;
}

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
static int mcux_flexcan_set_timing_data(const struct device *dev,
					const struct can_timing *timing_data)
{
	struct mcux_flexcan_data *data = dev->data;

	if (!timing_data) {
		return -EINVAL;
	}

	if (data->common.started) {
		return -EBUSY;
	}

	data->timing_data = *timing_data;

	return 0;
}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

static int mcux_flexcan_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	__maybe_unused const struct mcux_flexcan_config *config = dev->config;

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_3_SAMPLES;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	if (UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD), config->flexcan_fd)) {
		*cap |= CAN_MODE_FD;
	}

	return 0;
}

static status_t mcux_flexcan_mb_start(const struct device *dev, int alloc)
{
	__maybe_unused const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	flexcan_mb_transfer_t xfer;
	status_t status;

	__ASSERT_NO_MSG(alloc >= 0 && alloc < config->rx_mb);

	xfer.mbIdx = ALLOC_IDX_TO_RXMB_IDX(alloc);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if ((data->common.mode & CAN_MODE_FD) != 0U) {
		xfer.framefd = &data->rx_cbs[alloc].frame.fd;
		FLEXCAN_SetFDRxMbConfig(base, ALLOC_IDX_TO_RXMB_IDX(alloc),
					&data->rx_cbs[alloc].mb_config, true);
		status = FLEXCAN_TransferFDReceiveNonBlocking(base, &data->handle, &xfer);
	} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		xfer.frame = &data->rx_cbs[alloc].frame.classic;
		FLEXCAN_SetRxMbConfig(base, ALLOC_IDX_TO_RXMB_IDX(alloc),
				      &data->rx_cbs[alloc].mb_config, true);
		status = FLEXCAN_TransferReceiveNonBlocking(base, &data->handle, &xfer);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	return status;
}

static void mcux_flexcan_mb_stop(const struct device *dev, int alloc)
{
	__maybe_unused const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);

	__ASSERT_NO_MSG(alloc >= 0 && alloc < config->rx_mb);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if ((data->common.mode & CAN_MODE_FD) != 0U) {
		FLEXCAN_TransferFDAbortReceive(base, &data->handle,
					       ALLOC_IDX_TO_RXMB_IDX(alloc));
		FLEXCAN_SetFDRxMbConfig(base, ALLOC_IDX_TO_RXMB_IDX(alloc),
					NULL, false);
		} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
			FLEXCAN_TransferAbortReceive(base, &data->handle,
						     ALLOC_IDX_TO_RXMB_IDX(alloc));
			FLEXCAN_SetRxMbConfig(base, ALLOC_IDX_TO_RXMB_IDX(alloc),
					      NULL, false);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
}

static int mcux_flexcan_start(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	flexcan_timing_config_t timing;
	int err;

	if (data->common.started) {
		return -EALREADY;
	}

	if (config->common.phy != NULL) {
		err = can_transceiver_enable(config->common.phy, data->common.mode);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	/* Reset statistics and clear error counters */
	CAN_STATS_RESET(dev);
	base->ECR &= ~(CAN_ECR_TXERRCNT_MASK | CAN_ECR_RXERRCNT_MASK);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	status_t status;
	int alloc;

	if (config->flexcan_fd) {
		/* Re-add all RX filters using current mode */
		k_mutex_lock(&data->rx_mutex, K_FOREVER);

		for (alloc = RX_START_IDX; alloc < config->rx_mb; alloc++) {
			if (atomic_test_bit(data->rx_allocs, alloc)) {
				status = mcux_flexcan_mb_start(dev, alloc);
				if (status != kStatus_Success) {
					LOG_ERR("Failed to re-add rx filter id %d (err = %d)",
						alloc, status);
					k_mutex_unlock(&data->rx_mutex);
					return -EIO;
				}
			}
		}

		k_mutex_unlock(&data->rx_mutex);
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	/* Delay this until start since setting the timing automatically exits freeze mode */
	timing.preDivider = data->timing.prescaler - 1U;
	timing.rJumpwidth = data->timing.sjw - 1U;
	timing.phaseSeg1 = data->timing.phase_seg1 - 1U;
	timing.phaseSeg2 = data->timing.phase_seg2 - 1U;
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG) && \
	     FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG)
	if (UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD), config->flexcan_fd)) {
		/* No propagation segment configuration, so prop_seg must be 0 */
		timing.propSeg = data->timing.prop_seg;
	} else {
		/* Use standard configuration for classic CAN mode */
		timing.propSeg = data->timing.prop_seg - 1U;
	}
#else
	timing.propSeg = data->timing.prop_seg - 1U;
#endif
	FLEXCAN_SetTimingConfig(base, &timing);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if (config->flexcan_fd) {
		timing.fpreDivider = data->timing_data.prescaler - 1U;
		timing.frJumpwidth = data->timing_data.sjw - 1U;
		timing.fphaseSeg1 = data->timing_data.phase_seg1 - 1U;
		timing.fphaseSeg2 = data->timing_data.phase_seg2 - 1U;
		timing.fpropSeg = data->timing_data.prop_seg;
		FLEXCAN_SetFDTimingConfig(base, &timing);

		FLEXCAN_EnterFreezeMode(base);
		base->FDCTRL &= ~(CAN_FDCTRL_TDCOFF_MASK);
		base->FDCTRL |= FIELD_PREP(CAN_FDCTRL_TDCOFF_MASK,
						   CAN_CALC_TDCO((&data->timing_data), 1U, 31U));
		FLEXCAN_ExitFreezeMode(base);
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	data->common.started = true;

	return 0;
}

static int mcux_flexcan_stop(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	can_tx_callback_t function;
	void *arg;
	int alloc;
	int err;

	if (!data->common.started) {
		return -EALREADY;
	}

	data->common.started = false;

	/* Abort any pending TX frames before entering freeze mode */
	for (alloc = 0; alloc < config->tx_mb; alloc++) {
		function = data->tx_cbs[alloc].function;
		arg = data->tx_cbs[alloc].arg;

		if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
			if ((data->common.mode & CAN_MODE_FD) != 0U) {
				FLEXCAN_TransferFDAbortSend(base, &data->handle,
							    ALLOC_IDX_TO_TXMB_IDX(alloc));
			} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
				FLEXCAN_TransferAbortSend(base, &data->handle,
							  ALLOC_IDX_TO_TXMB_IDX(alloc));
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
			}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

			function(dev, -ENETDOWN, arg);
			k_sem_give(&data->tx_allocs_sem);
		}
	}

	FLEXCAN_EnterFreezeMode(base);

	if (UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD), config->flexcan_fd)) {
		/*
		 * Remove all RX filters and re-add them in start() since the mode may change
		 * between stop()/start().
		 */
		k_mutex_lock(&data->rx_mutex, K_FOREVER);

		for (alloc = RX_START_IDX; alloc < config->rx_mb; alloc++) {
			if (atomic_test_bit(data->rx_allocs, alloc)) {
				mcux_flexcan_mb_stop(dev, alloc);
			}
		}

		k_mutex_unlock(&data->rx_mutex);
	}

	if (config->common.phy != NULL) {
		err = can_transceiver_disable(config->common.phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
			return err;
		}
	}

	return 0;
}

static int mcux_flexcan_set_mode(const struct device *dev, can_mode_t mode)
{
	can_mode_t supported = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_3_SAMPLES;
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	const struct mcux_flexcan_config *config = dev->config;
#endif
	CAN_Type *base = get_base(dev);
	struct mcux_flexcan_data *data = dev->data;
	uint32_t ctrl1;
	uint32_t mcr;

	if (data->common.started) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		supported |= CAN_MODE_MANUAL_RECOVERY;
	}

	if (UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD), config->flexcan_fd)) {
		supported |= CAN_MODE_FD;
	}

	if ((mode & ~(supported)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if ((mode & CAN_MODE_FD) != 0 && (mode & CAN_MODE_3_SAMPLES) != 0) {
		LOG_ERR("triple samling is not supported in CAN FD mode");
		return -ENOTSUP;
	}

	ctrl1 = base->CTRL1;
	mcr = base->MCR;

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

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		if ((mode & CAN_MODE_MANUAL_RECOVERY) != 0) {
			/* Disable auto-recovery from bus-off */
			ctrl1 |= CAN_CTRL1_BOFFREC_MASK;
		} else {
			/* Enable auto-recovery from bus-off */
			ctrl1 &= ~(CAN_CTRL1_BOFFREC_MASK);
		}
	}

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if (config->flexcan_fd) {
		if ((mode & CAN_MODE_FD) != 0) {
			/* Enable CAN FD mode */
			mcr |= CAN_MCR_FDEN_MASK;

			/* Transceiver Delay Compensation must be disabled in loopback mode */
			if ((mode & CAN_MODE_LOOPBACK) != 0) {
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG) && \
	 FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG)
				base->ETDC &= ~(CAN_ETDC_ETDCEN_MASK);
#else
				base->FDCTRL &= ~(CAN_FDCTRL_TDCEN_MASK);
#endif
			} else {
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG) && \
	 FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG)
				base->ETDC |= CAN_ETDC_ETDCEN_MASK;
#else
				base->FDCTRL |= CAN_FDCTRL_TDCEN_MASK;
#endif
			}
		} else {
			/* Disable CAN FD mode */
			mcr &= ~(CAN_MCR_FDEN_MASK);
		}
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	base->CTRL1 = ctrl1;
	base->MCR = mcr;

	data->common.mode = mode;

	return 0;
}

static void mcux_flexcan_from_can_frame(const struct can_frame *src,
					flexcan_frame_t *dest)
{
	memset(dest, 0, sizeof(*dest));

	if ((src->flags & CAN_FRAME_IDE) != 0) {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
	} else {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
	}

	if ((src->flags & CAN_FRAME_RTR) != 0) {
		dest->type = kFLEXCAN_FrameTypeRemote;
	} else {
		dest->type = kFLEXCAN_FrameTypeData;
		dest->dataWord0 = sys_cpu_to_be32(src->data_32[0]);
		dest->dataWord1 = sys_cpu_to_be32(src->data_32[1]);
	}

	dest->length = src->dlc;
}

static void mcux_flexcan_to_can_frame(const flexcan_frame_t *src,
				      struct can_frame *dest)
{
	memset(dest, 0, sizeof(*dest));

	if (src->format == kFLEXCAN_FrameFormatStandard) {
		dest->id = FLEXCAN_ID_TO_CAN_ID_STD(src->id);
	} else {
		dest->flags |= CAN_FRAME_IDE;
		dest->id = FLEXCAN_ID_TO_CAN_ID_EXT(src->id);
	}

	if (src->type == kFLEXCAN_FrameTypeRemote) {
		dest->flags |= CAN_FRAME_RTR;
	} else {
		dest->data_32[0] = sys_be32_to_cpu(src->dataWord0);
		dest->data_32[1] = sys_be32_to_cpu(src->dataWord1);
	}

	dest->dlc = src->length;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	dest->timestamp = src->timestamp;
#endif /* CAN_RX_TIMESTAMP */
}

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
static void mcux_flexcan_fd_from_can_frame(const struct can_frame *src,
					   flexcan_fd_frame_t *dest)
{
	int i;

	memset(dest, 0, sizeof(*dest));

	if ((src->flags & CAN_FRAME_IDE) != 0) {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
	} else {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
	}

	if ((src->flags & CAN_FRAME_RTR) != 0) {
		dest->type = kFLEXCAN_FrameTypeRemote;
	} else {
		dest->type = kFLEXCAN_FrameTypeData;

		for (i = 0; i < ARRAY_SIZE(dest->dataWord); i++) {
			dest->dataWord[i] = sys_cpu_to_be32(src->data_32[i]);
		}
	}

	if ((src->flags & CAN_FRAME_FDF) != 0) {
		dest->edl = 1;
	}

	if ((src->flags & CAN_FRAME_BRS) != 0) {
		dest->brs = 1;
	}

	dest->length = src->dlc;
}

static void mcux_flexcan_fd_to_can_frame(const flexcan_fd_frame_t *src,
					 struct can_frame *dest)
{
	int i;

	memset(dest, 0, sizeof(*dest));

	if (src->format == kFLEXCAN_FrameFormatStandard) {
		dest->id = FLEXCAN_ID_TO_CAN_ID_STD(src->id);
	} else {
		dest->flags |= CAN_FRAME_IDE;
		dest->id = FLEXCAN_ID_TO_CAN_ID_EXT(src->id);
	}

	if (src->type == kFLEXCAN_FrameTypeRemote) {
		dest->flags |= CAN_FRAME_RTR;
	} else {
		for (i = 0; i < ARRAY_SIZE(dest->data_32); i++) {
			dest->data_32[i] = sys_be32_to_cpu(src->dataWord[i]);
		}
	}

	if (src->edl != 0) {
		dest->flags |= CAN_FRAME_FDF;
	}

	if (src->brs != 0) {
		dest->flags |= CAN_FRAME_BRS;
	}

	if (src->esi != 0) {
		dest->flags |= CAN_FRAME_ESI;
	}

	dest->dlc = src->length;

#ifdef CONFIG_CAN_RX_TIMESTAMP
	dest->timestamp = src->timestamp;
#endif /* CAN_RX_TIMESTAMP */
}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

static void mcux_flexcan_can_filter_to_mbconfig(const struct can_filter *src,
						flexcan_rx_mb_config_t *dest,
						uint32_t *mask)
{
	static const uint32_t ide_mask = 1U;
	static const uint32_t rtr_mask = !IS_ENABLED(CONFIG_CAN_ACCEPT_RTR);

	if ((src->flags & CAN_FILTER_IDE) != 0) {
		dest->format = kFLEXCAN_FrameFormatExtend;
		dest->id = FLEXCAN_ID_EXT(src->id);
		*mask = FLEXCAN_RX_MB_EXT_MASK(src->mask, rtr_mask, ide_mask);
	} else {
		dest->format = kFLEXCAN_FrameFormatStandard;
		dest->id = FLEXCAN_ID_STD(src->id);
		*mask = FLEXCAN_RX_MB_STD_MASK(src->mask, rtr_mask, ide_mask);
	}

	dest->type = kFLEXCAN_FrameTypeData;
}

static int mcux_flexcan_get_state(const struct device *dev, enum can_state *state,
				  struct can_bus_err_cnt *err_cnt)
{
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	uint64_t status_flags;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			status_flags = FLEXCAN_GetStatusFlags(base);

			if ((status_flags & CAN_ESR1_FLTCONF(2)) != 0U) {
				*state = CAN_STATE_BUS_OFF;
			} else if ((status_flags & CAN_ESR1_FLTCONF(1)) != 0U) {
				*state = CAN_STATE_ERROR_PASSIVE;
			} else if ((status_flags &
				(kFLEXCAN_TxErrorWarningFlag | kFLEXCAN_RxErrorWarningFlag)) != 0) {
				*state = CAN_STATE_ERROR_WARNING;
			} else {
				*state = CAN_STATE_ERROR_ACTIVE;
			}
		}
	}

	if (err_cnt != NULL) {
		FLEXCAN_GetBusErrCount(base, &err_cnt->tx_err_cnt,
				       &err_cnt->rx_err_cnt);
	}

	return 0;
}

static int mcux_flexcan_send(const struct device *dev,
			     const struct can_frame *frame,
			     k_timeout_t timeout,
			     can_tx_callback_t callback, void *user_data)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	flexcan_mb_transfer_t xfer;
	enum can_state state;
	status_t status = kStatus_Fail;
	uint8_t max_dlc = CAN_MAX_DLC;
	int alloc;

	if (UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD),
		     ((data->common.mode & CAN_MODE_FD) != 0U))) {
		if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR |
				      CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
			LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}

		if ((frame->flags & CAN_FRAME_FDF) != 0) {
			max_dlc = CANFD_MAX_DLC;
		}
	} else {
		if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
			LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
			return -ENOTSUP;
		}
	}

	if (frame->dlc > max_dlc) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, max_dlc);
		return -EINVAL;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	(void)mcux_flexcan_get_state(dev, &state, NULL);
	if (state == CAN_STATE_BUS_OFF) {
		LOG_DBG("Transmit failed, bus-off");
		return -ENETUNREACH;
	}

	if (k_sem_take(&data->tx_allocs_sem, timeout) != 0) {
		return -EAGAIN;
	}

	for (alloc = 0; alloc < config->tx_mb; alloc++) {
		if (!atomic_test_and_set_bit(data->tx_allocs, alloc)) {
			break;
		}
	}

	data->tx_cbs[alloc].function = callback;
	data->tx_cbs[alloc].arg = user_data;
	xfer.mbIdx = ALLOC_IDX_TO_TXMB_IDX(alloc);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if ((data->common.mode & CAN_MODE_FD) != 0U) {
		FLEXCAN_SetFDTxMbConfig(base, xfer.mbIdx, true);
	} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		FLEXCAN_SetTxMbConfig(base, xfer.mbIdx, true);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	k_mutex_lock(&data->tx_mutex, K_FOREVER);
	config->irq_disable_func();

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if ((data->common.mode & CAN_MODE_FD) != 0U) {
		flexcan_fd_frame_t flexcan_frame;

		mcux_flexcan_fd_from_can_frame(frame, &flexcan_frame);
		xfer.framefd = &flexcan_frame;
		status = FLEXCAN_TransferFDSendNonBlocking(base, &data->handle, &xfer);
	} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		flexcan_frame_t flexcan_frame;

		mcux_flexcan_from_can_frame(frame, &flexcan_frame);
		xfer.frame = &flexcan_frame;
		status = FLEXCAN_TransferSendNonBlocking(base, &data->handle, &xfer);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	config->irq_enable_func();
	k_mutex_unlock(&data->tx_mutex);
	if (status != kStatus_Success) {
		return -EIO;
	}

	return 0;
}

static int mcux_flexcan_add_rx_filter(const struct device *dev,
				      can_rx_callback_t callback,
				      void *user_data,
				      const struct can_filter *filter)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	status_t status;
	uint32_t mask;
	int alloc = -ENOSPC;
	int i;

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	/* Find and allocate RX message buffer */
	for (i = RX_START_IDX; i < config->rx_mb; i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			alloc = i;
			break;
		}
	}

	if (alloc == -ENOSPC) {
		goto unlock;
	}

	mcux_flexcan_can_filter_to_mbconfig(filter, &data->rx_cbs[alloc].mb_config,
					    &mask);

	data->rx_cbs[alloc].arg = user_data;
	data->rx_cbs[alloc].function = callback;

	/* The indidual RX mask registers can only be written in freeze mode */
	FLEXCAN_EnterFreezeMode(base);
	base->RXIMR[ALLOC_IDX_TO_RXMB_IDX(alloc)] = mask;

	if (data->common.started) {
		FLEXCAN_ExitFreezeMode(base);
	}

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	/* Defer starting FlexCAN FD MBs unless started */
	if (!config->flexcan_fd || data->common.started) {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		status = mcux_flexcan_mb_start(dev, alloc);
		if (status != kStatus_Success) {
			LOG_ERR("Failed to start rx for filter id %d (err = %d)",
				alloc, status);
			alloc = -ENOSPC;
		}
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

unlock:
	k_mutex_unlock(&data->rx_mutex);

	return alloc;
}

static void mcux_flexcan_set_state_change_callback(const struct device *dev,
						   can_state_change_callback_t callback,
						   void *user_data)
{
	struct mcux_flexcan_data *data = dev->data;

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int mcux_flexcan_recover(const struct device *dev, k_timeout_t timeout)
{
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	enum can_state state;
	uint64_t start_time;
	int ret = 0;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	(void)mcux_flexcan_get_state(dev, &state, NULL);
	if (state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	start_time = k_uptime_ticks();
	base->CTRL1 &= ~CAN_CTRL1_BOFFREC_MASK;

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		(void)mcux_flexcan_get_state(dev, &state, NULL);

		while (state == CAN_STATE_BUS_OFF) {
			if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
			    k_uptime_ticks() - start_time >= timeout.ticks) {
				ret = -EAGAIN;
			}

			(void)mcux_flexcan_get_state(dev, &state, NULL);
		}
	}

	base->CTRL1 |= CAN_CTRL1_BOFFREC_MASK;

	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void mcux_flexcan_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;

	if (filter_id < 0 || filter_id >= config->rx_mb) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	if (atomic_test_and_clear_bit(data->rx_allocs, filter_id)) {
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		/* Stop FlexCAN FD MBs unless already in stopped mode */
		if (!config->flexcan_fd || data->common.started) {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
			mcux_flexcan_mb_stop(dev, filter_id);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

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
	CAN_Type *base = get_base(dev);
	const can_state_change_callback_t cb = data->common.state_change_cb;
	void *cb_data = data->common.state_change_cb_user_data;
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

	if (state == CAN_STATE_BUS_OFF) {
		/* Abort any pending TX frames in case of bus-off */
		for (alloc = 0; alloc < config->tx_mb; alloc++) {
			/* Copy callback function and argument before clearing bit */
			function = data->tx_cbs[alloc].function;
			arg = data->tx_cbs[alloc].arg;

			if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
				if ((data->common.mode & CAN_MODE_FD) != 0U) {
					FLEXCAN_TransferFDAbortSend(base, &data->handle,
								    ALLOC_IDX_TO_TXMB_IDX(alloc));
				} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
					FLEXCAN_TransferAbortSend(base, &data->handle,
								  ALLOC_IDX_TO_TXMB_IDX(alloc));
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
				}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

				function(dev, -ENETUNREACH, arg);
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
		function(dev, 0, arg);
		k_sem_give(&data->tx_allocs_sem);
	}
}

static inline void mcux_flexcan_transfer_rx_idle(const struct device *dev,
						 uint32_t mb)
{
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);
	can_rx_callback_t function;
	flexcan_mb_transfer_t xfer;
	struct can_frame frame;
	status_t status = kStatus_Fail;
	void *arg;
	int alloc;

	alloc = RX_MBIDX_TO_ALLOC_IDX(mb);
	function = data->rx_cbs[alloc].function;
	arg = data->rx_cbs[alloc].arg;

	if (atomic_test_bit(data->rx_allocs, alloc)) {
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		if ((data->common.mode & CAN_MODE_FD) != 0U) {
			mcux_flexcan_fd_to_can_frame(&data->rx_cbs[alloc].frame.fd, &frame);
		} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
			mcux_flexcan_to_can_frame(&data->rx_cbs[alloc].frame.classic, &frame);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		function(dev, &frame, arg);

		/* Setup RX message buffer to receive next message */
		xfer.mbIdx = mb;
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		if ((data->common.mode & CAN_MODE_FD) != 0U) {
			xfer.framefd = &data->rx_cbs[alloc].frame.fd;
			status = FLEXCAN_TransferFDReceiveNonBlocking(base,
								      &data->handle,
								      &xfer);
		} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
			xfer.frame = &data->rx_cbs[alloc].frame.classic;
			status = FLEXCAN_TransferReceiveNonBlocking(base,
								    &data->handle,
								    &xfer);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

		if (status != kStatus_Success) {
			LOG_ERR("Failed to restart rx for filter id %d "
				"(err = %d)", alloc, status);
		}
	}
}

static FLEXCAN_CALLBACK(mcux_flexcan_transfer_callback)
{
	struct mcux_flexcan_data *data = (struct mcux_flexcan_data *)userData;
	CAN_Type *dev_base = get_base(data->dev);
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
	case kStatus_FLEXCAN_ErrorStatus:
		mcux_flexcan_transfer_error_status(data->dev, status_flags);
		break;
	case kStatus_FLEXCAN_TxSwitchToRx:
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		if ((data->common.mode & CAN_MODE_FD) != 0U) {
			FLEXCAN_TransferFDAbortReceive(dev_base, &data->handle, mb);
		} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
			FLEXCAN_TransferAbortReceive(dev_base, &data->handle, mb);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
		}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		__fallthrough;
	case kStatus_FLEXCAN_TxIdle:
		mcux_flexcan_transfer_tx_idle(data->dev, mb);
		break;
	case kStatus_FLEXCAN_RxOverflow:
		CAN_STATS_RX_OVERRUN_INC(data->dev);
		__fallthrough;
	case kStatus_Fail:
		/* If reading an RX MB failed mark it as idle to be reprocessed. */
		__fallthrough;
	case kStatus_FLEXCAN_RxIdle:
		mcux_flexcan_transfer_rx_idle(data->dev, mb);
		break;
	case kStatus_FLEXCAN_UnHandled:
		/*
		 * Unhandled status during Message Buffer processing.
		 * If result field is 0xFF, it means no message buffer interrupt occurred.
		 */
		__fallthrough;
	default:
		LOG_WRN("Unhandled status 0x%08x (result = 0x%016llx)",
			status, status_flags);
	}
}

static void mcux_flexcan_isr(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base = get_base(dev);

	FLEXCAN_BusoffErrorHandleIRQ(base, &data->handle);
	FLEXCAN_MbHandleIRQ(base, &data->handle, 0U, config->number_of_mb);
}

static int mcux_flexcan_init(const struct device *dev)
{
	const struct mcux_flexcan_config *config = dev->config;
	struct mcux_flexcan_data *data = dev->data;
	CAN_Type *base;
	flexcan_config_t flexcan_config;
	uint32_t clock_freq;
	int err;

	if (config->common.phy != NULL) {
		if (!device_is_ready(config->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock device not ready");
		return -ENODEV;
	}

	err = clock_control_configure(config->clock_dev, config->clock_subsys, NULL);
	if (err) {
		/* Check if error is due to lack of support */
		if (err != -ENOSYS) {
			/* Real error occurred */
			LOG_ERR("Failed to configure clock: %d", err);
			return err;
		}
	}

#if FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL
	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Failed to enable clock: %d", err);
		return err;
	}
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

	DEVICE_MMIO_NAMED_MAP(dev, flexcan_mmio, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	LOG_DBG("Message Buffers: %d, RX MB: %d, TX MB: %d",
		 config->number_of_mb, config->rx_mb, config->tx_mb);

	k_mutex_init(&data->rx_mutex);
	k_mutex_init(&data->tx_mutex);
	k_sem_init(&data->tx_allocs_sem, config->tx_mb, config->tx_mb);

	err = can_calc_timing(dev, &data->timing, config->common.bitrate,
			      config->common.sample_point);
	if (err == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}
	LOG_DBG("Presc: %d, Seg1S1: %d, Seg2: %d",
		 data->timing.prescaler, data->timing.phase_seg1,
		 data->timing.phase_seg2);
	LOG_DBG("Sample-point err : %d", err);

	/* Validate initial timing parameters */
	err = can_set_timing(dev, &data->timing);
	if (err != 0) {
		LOG_ERR("failed to set timing (err %d)", err);
		return -ENODEV;
	}

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if (config->flexcan_fd) {
		err = can_calc_timing_data(dev, &data->timing_data,
					   config->common.bitrate_data,
					   config->common.sample_point_data);
		if (err == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, Seg1S1: %d, Seg2: %d",
			data->timing_data.prescaler, data->timing_data.phase_seg1,
			data->timing_data.phase_seg2);
		LOG_DBG("Sample-point err : %d", err);

		/* Validate initial data phase timing parameters */
		err = can_set_timing_data(dev, &data->timing_data);
		if (err != 0) {
			LOG_ERR("failed to set data phase timing (err %d)", err);
			return -ENODEV;
		}
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	err = mcux_flexcan_get_core_clock(dev, &clock_freq);
	if (err != 0) {
		return -EIO;
	}

	data->dev = dev;

	FLEXCAN_GetDefaultConfig(&flexcan_config);
	flexcan_config.maxMbNum = config->number_of_mb;
	flexcan_config.clkSrc = config->clk_source;
	flexcan_config.baudRate = clock_freq /
	      (1U + data->timing.prop_seg + data->timing.phase_seg1 +
	       data->timing.phase_seg2) / data->timing.prescaler;

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if (config->flexcan_fd) {
		flexcan_config.baudRateFD = clock_freq /
			(1U + data->timing_data.prop_seg + data->timing_data.phase_seg1 +
			 data->timing_data.phase_seg2) / data->timing_data.prescaler;
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	flexcan_config.enableIndividMask = true;
	flexcan_config.enableLoopBack = false;
	flexcan_config.disableSelfReception = true;
	/* Initialize in listen-only mode since FLEXCAN_{FD}Init() exits freeze mode */
	flexcan_config.enableListenOnlyMode = true;

	flexcan_config.timingConfig.rJumpwidth = data->timing.sjw - 1U;
	flexcan_config.timingConfig.propSeg = data->timing.prop_seg - 1U;
	flexcan_config.timingConfig.phaseSeg1 = data->timing.phase_seg1 - 1U;
	flexcan_config.timingConfig.phaseSeg2 = data->timing.phase_seg2 - 1U;

	base = get_base(dev);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	if (config->flexcan_fd) {
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG) && \
	     FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG)
		/* No propagation segment configuration, so prop_seg must be 0 */
		flexcan_config.timingConfig.propSeg = data->timing.prop_seg;
#endif
		flexcan_config.timingConfig.frJumpwidth = data->timing_data.sjw - 1U;
		flexcan_config.timingConfig.fpropSeg = data->timing_data.prop_seg;
		flexcan_config.timingConfig.fphaseSeg1 = data->timing_data.phase_seg1 - 1U;
		flexcan_config.timingConfig.fphaseSeg2 = data->timing_data.phase_seg2 - 1U;

		FLEXCAN_FDInit(base, &flexcan_config, clock_freq, kFLEXCAN_64BperMB, true);
	} else {
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */
		FLEXCAN_Init(base, &flexcan_config, clock_freq);
#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
	}
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

	FLEXCAN_TransferCreateHandle(base, &data->handle,
				     mcux_flexcan_transfer_callback, data);

	/* Manually enter freeze mode, set normal mode, and clear error counters */
	FLEXCAN_EnterFreezeMode(base);
	(void)mcux_flexcan_set_mode(dev, CAN_MODE_NORMAL);
	base->ECR &= ~(CAN_ECR_TXERRCNT_MASK | CAN_ECR_RXERRCNT_MASK);

	config->irq_config_func(dev);

	/* Enable auto-recovery from bus-off */
	base->CTRL1 &= ~(CAN_CTRL1_BOFFREC_MASK);

	(void)mcux_flexcan_get_state(dev, &data->state, NULL);

	return 0;
}

static DEVICE_API(can, mcux_flexcan_driver_api) __maybe_unused = {
	.get_capabilities = mcux_flexcan_get_capabilities,
	.start = mcux_flexcan_start,
	.stop = mcux_flexcan_stop,
	.set_mode = mcux_flexcan_set_mode,
	.set_timing = mcux_flexcan_set_timing,
	.send = mcux_flexcan_send,
	.add_rx_filter = mcux_flexcan_add_rx_filter,
	.remove_rx_filter = mcux_flexcan_remove_rx_filter,
	.get_state = mcux_flexcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = mcux_flexcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.set_state_change_callback = mcux_flexcan_set_state_change_callback,
	.get_core_clock = mcux_flexcan_get_core_clock,
	.get_max_filters = mcux_flexcan_get_max_filters,
	/*
	 * FlexCAN timing limits are specified in the "FLEXCANx_CTRL1 field
	 * descriptions" table in the SoC reference manual.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the flexcan_timing_config_t field assignments elsewhere
	 * in this driver).
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

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
static DEVICE_API(can, mcux_flexcan_fd_driver_api) = {
	.get_capabilities = mcux_flexcan_get_capabilities,
	.start = mcux_flexcan_start,
	.stop = mcux_flexcan_stop,
	.set_mode = mcux_flexcan_set_mode,
	.set_timing = mcux_flexcan_set_timing,
	.set_timing_data = mcux_flexcan_set_timing_data,
	.send = mcux_flexcan_send,
	.add_rx_filter = mcux_flexcan_add_rx_filter,
	.remove_rx_filter = mcux_flexcan_remove_rx_filter,
	.get_state = mcux_flexcan_get_state,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = mcux_flexcan_recover,
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */
	.set_state_change_callback = mcux_flexcan_set_state_change_callback,
	.get_core_clock = mcux_flexcan_get_core_clock,
	.get_max_filters = mcux_flexcan_get_max_filters,
	/*
	 * FlexCAN FD timing limits are specified in the "CAN Bit Timing
	 * Register (CBT)" and "CAN FD Bit Timing Register" field description
	 * tables in the SoC reference manual.
	 * Some devices have enhanced bit timing registers (EPRS ENCBT EDCBT)
	 * with different limits and do not have propagation segment configuration.
	 *
	 * Note that the values here are the "physical" timing limits, whereas
	 * the register field limits are physical values minus 1 (which is
	 * handled by the flexcan_timing_config_t field assignments elsewhere
	 * in this driver). The exception to this are the prop_seg values for
	 * the data phase, which correspond to the allowed register values.
	 */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG) && \
	     FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG)
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0,
		.phase_seg1 = 0x02,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x80,
		.prop_seg = 0,
		.phase_seg1 = 0x100,
		.phase_seg2 = 0x80,
		.prescaler = 0x400
	},
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0,
		.phase_seg1 = 0x02,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x10,
		.prop_seg = 0,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x10,
		.prescaler = 0x400
	},
#else
	.timing_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x20,
		.prop_seg = 0x40,
		.phase_seg1 = 0x20,
		.phase_seg2 = 0x20,
		.prescaler = 0x400
	},
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x08,
		.prop_seg = 0x1f,
		.phase_seg1 = 0x08,
		.phase_seg2 = 0x08,
		.prescaler = 0x400
	},
#endif /* FSL_FEATURE_FLEXCAN_HAS_ENHANCED_BIT_TIMING_REG */
};
#endif /* CONFIG_CAN_MCUX_FLEXCAN_FD */

#define FLEXCAN_IRQ_BY_IDX(node_id, prop, idx, cell) \
	DT_IRQ_BY_NAME(node_id, \
		DT_STRING_TOKEN_BY_IDX(node_id, prop, idx), cell)

#define FLEXCAN_IRQ_ENABLE_CODE(node_id, prop, idx) \
	irq_enable(FLEXCAN_IRQ_BY_IDX(node_id, prop, idx, irq));

#define FLEXCAN_IRQ_DISABLE_CODE(node_id, prop, idx) \
	irq_disable(FLEXCAN_IRQ_BY_IDX(node_id, prop, idx, irq));

#define FLEXCAN_IRQ_CONFIG_CODE(node_id, prop, idx) \
	do {								\
		IRQ_CONNECT(FLEXCAN_IRQ_BY_IDX(node_id, prop, idx, irq), \
		FLEXCAN_IRQ_BY_IDX(node_id, prop, idx, priority), \
		mcux_flexcan_isr, \
		DEVICE_DT_GET(node_id), 0); \
		FLEXCAN_IRQ_ENABLE_CODE(node_id, prop, idx); \
	} while (false);

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
#define FLEXCAN_MAX_BITRATE(id)							\
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(id, FLEXCAN_FD_DRV_COMPAT),		\
		    (8000000), (1000000))
#else /* CONFIG_CAN_MCUX_FLEXCAN_FD */
#define FLEXCAN_MAX_BITRATE(id) 1000000
#endif /* !CONFIG_CAN_MCUX_FLEXCAN_FD */

#ifdef CONFIG_CAN_MCUX_FLEXCAN_FD
#define FLEXCAN_DRIVER_API(id)							\
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(id, FLEXCAN_FD_DRV_COMPAT),		\
		    (mcux_flexcan_fd_driver_api),				\
		    (mcux_flexcan_driver_api))
#else /* CONFIG_CAN_MCUX_FLEXCAN_FD */
#define FLEXCAN_DRIVER_API(id) mcux_flexcan_driver_api
#endif /* !CONFIG_CAN_MCUX_FLEXCAN_FD */

#define FLEXCAN_INST_NUMBER_OF_MB(id)						\
	COND_CODE_1(UTIL_AND(IS_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD),		\
			DT_INST_NODE_HAS_COMPAT(id, FLEXCAN_FD_DRV_COMPAT)),	\
		(DT_INST_PROP(id, number_of_mb_fd)),				\
		(DT_INST_PROP(id, number_of_mb)))

/*
 * RX message buffers (filters) will take up the first N message
 * buffers. The rest are available for TX use.
 */
#define FLEXCAN_INST_RX_MB(id) (CONFIG_CAN_MCUX_FLEXCAN_MAX_FILTERS + RX_START_IDX)
#define FLEXCAN_INST_TX_MB(id) (FLEXCAN_INST_NUMBER_OF_MB(id) - FLEXCAN_INST_RX_MB(id))

#define FLEXCAN_CLK_SOURCE(id) DT_INST_PROP(id, clk_source)
#define FLEXCAN_CLK_SOURCE_NAME(id) CONCAT(clksrc, FLEXCAN_CLK_SOURCE(id))

#define FLEXCAN_INST_CLOCKS_FROM_CLK_SOURCE(id)							  \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, FLEXCAN_CLK_SOURCE_NAME(id))), \
	.clock_subsys = (clock_control_subsys_t)						  \
		DT_INST_CLOCKS_CELL_BY_NAME(id, FLEXCAN_CLK_SOURCE_NAME(id), name),		  \
	.clk_source = FLEXCAN_CLK_SOURCE(id)

#define FLEXCAN_INST_CLOCKS_NO_CLK_SOURCE(id)					\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),			\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(id, name),	\
	.clk_source = 0U

#define FLEXCAN_INST_CLOCKS(id)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(id, clk_source),			\
		(FLEXCAN_INST_CLOCKS_FROM_CLK_SOURCE(id)),			\
		(FLEXCAN_INST_CLOCKS_NO_CLK_SOURCE(id)))

#define FLEXCAN_CHECK_CLK_SOURCE(id)								\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(id, clk_source),					\
		(BUILD_ASSERT(DT_INST_CLOCKS_HAS_NAME(id, FLEXCAN_CLK_SOURCE_NAME(id)),		\
			"FlexCAN instance " STRINGIFY(id) " clk-source without named clock")))

#define FLEXCAN_CHECK_MAX_FILTER(id)						\
	BUILD_ASSERT(CONFIG_CAN_MCUX_FLEXCAN_MAX_FILTERS > 0,			\
		"Maximum number of RX filters should greater than 0");		\
	BUILD_ASSERT(FLEXCAN_INST_NUMBER_OF_MB(id) > FLEXCAN_INST_RX_MB(id),	\
		     "FlexCAN instance " STRINGIFY(id) " number-of-mb ("	\
		     STRINGIFY(FLEXCAN_INST_NUMBER_OF_MB(id))			\
		     ") is too small for required RX filters ("			\
		     STRINGIFY(FLEXCAN_INST_RX_MB(id)) ")")

#define FLEXCAN_DEVICE_INIT_MCUX(id)					\
	PINCTRL_DT_INST_DEFINE(id);					\
	FLEXCAN_CHECK_CLK_SOURCE(id);					\
	FLEXCAN_CHECK_MAX_FILTER(id);					\
									\
	static void mcux_flexcan_irq_config_##id(const struct device *dev); \
	static void mcux_flexcan_irq_enable_##id(void); \
	static void mcux_flexcan_irq_disable_##id(void); \
									\
	static struct mcux_flexcan_rx_callback flexcan_rx_cbs_##id	\
			[FLEXCAN_INST_RX_MB(id)] = {0};			\
									\
	static struct mcux_flexcan_tx_callback flexcan_tx_cbs_##id	\
			[FLEXCAN_INST_TX_MB(id)] = {0};			\
									\
	static ATOMIC_DEFINE(flexcan_rx_allocs_##id, FLEXCAN_INST_RX_MB(id));	\
									\
	static ATOMIC_DEFINE(flexcan_tx_allocs_##id, FLEXCAN_INST_TX_MB(id));	\
									\
	static const struct mcux_flexcan_config mcux_flexcan_config_##id = { \
		DEVICE_MMIO_NAMED_ROM_INIT(flexcan_mmio, DT_DRV_INST(id)),	\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(id, 0, FLEXCAN_MAX_BITRATE(id)), \
		FLEXCAN_INST_CLOCKS(id),				\
		.number_of_mb = FLEXCAN_INST_NUMBER_OF_MB(id),		\
		.rx_mb = FLEXCAN_INST_RX_MB(id),			\
		.tx_mb = FLEXCAN_INST_TX_MB(id),			\
		IF_ENABLED(CONFIG_CAN_MCUX_FLEXCAN_FD, (		\
			.flexcan_fd = DT_INST_NODE_HAS_COMPAT(id, FLEXCAN_FD_DRV_COMPAT), \
		))							\
		.irq_config_func = mcux_flexcan_irq_config_##id,	\
		.irq_enable_func = mcux_flexcan_irq_enable_##id,	\
		.irq_disable_func = mcux_flexcan_irq_disable_##id,	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),		\
	};								\
									\
	static struct mcux_flexcan_data mcux_flexcan_data_##id = {	\
		.rx_cbs = flexcan_rx_cbs_##id,				\
		.tx_cbs = flexcan_tx_cbs_##id,				\
		.rx_allocs = flexcan_rx_allocs_##id,			\
		.tx_allocs = flexcan_tx_allocs_##id,			\
	};								\
									\
	CAN_DEVICE_DT_INST_DEFINE(id, mcux_flexcan_init,		\
				  NULL, &mcux_flexcan_data_##id,	\
				  &mcux_flexcan_config_##id,		\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,\
				  &FLEXCAN_DRIVER_API(id));		\
									\
	static void mcux_flexcan_irq_config_##id(const struct device *dev) \
	{								\
		DT_INST_FOREACH_PROP_ELEM(id, interrupt_names, FLEXCAN_IRQ_CONFIG_CODE); \
	} \
									\
	static void mcux_flexcan_irq_enable_##id(void) \
	{								\
		DT_INST_FOREACH_PROP_ELEM(id, interrupt_names, FLEXCAN_IRQ_ENABLE_CODE); \
	} \
									\
	static void mcux_flexcan_irq_disable_##id(void) \
	{								\
		DT_INST_FOREACH_PROP_ELEM(id, interrupt_names, FLEXCAN_IRQ_DISABLE_CODE); \
	}

DT_INST_FOREACH_STATUS_OKAY(FLEXCAN_DEVICE_INIT_MCUX)
