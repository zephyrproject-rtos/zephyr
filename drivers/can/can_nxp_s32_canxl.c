/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <CanEXCEL_Ip_HwAccess.h>
#include <CanEXCEL_Ip_Irq.h>

#define DT_DRV_COMPAT nxp_s32_canxl

/*
 * Convert from RX message buffer index to allocated filter ID and
 * vice versa.
 */
#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
#define RX_MBIDX_TO_ALLOC_IDX(x)	(x)
#define ALLOC_IDX_TO_RXMB_IDX(x)	(x)
#else
#define RX_MBIDX_TO_ALLOC_IDX(x)	(x - CONFIG_CAN_NXP_S32_MAX_TX)
#define ALLOC_IDX_TO_RXMB_IDX(x)	(x + CONFIG_CAN_NXP_S32_MAX_TX)
#endif


/*
 * Convert from TX message buffer index to allocated TX ID and vice
 * versa.
 */
#define TX_MBIDX_TO_ALLOC_IDX(x) (x)
#define ALLOC_IDX_TO_TXMB_IDX(x) (x)

#define CAN_NXP_S32_TIMEOUT_MS  1
#define CAN_NXP_S32_MAX_BITRATE	8000000
#define CAN_NXP_S32_DATA_LENGTH 64

#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
/* RX FIFO depth is fixed to the maximum value */
#define CAN_NXP_S32_RX_FIFO_DEPTH 32
/* RX FIFO water mark equal 1 that allows the interrupt is generated after 1 message received */
#define CAN_NXP_S32_RX_FIFO_WATERMARK 1
#endif

#if defined(CONFIG_CAN_FD_MODE) && defined(CONFIG_CAN_NXP_S32_RX_FIFO)
#define CAN_NXP_S32_FD_MODE 1
#endif

LOG_MODULE_REGISTER(nxp_s32_canxl, CONFIG_CAN_LOG_LEVEL);

struct can_nxp_s32_config {
	const struct can_driver_config common;
	CANXL_SIC_Type *base_sic;
	CANXL_GRP_CONTROL_Type *base_grp_ctrl;
	CANXL_DSC_CONTROL_Type *base_dsc_ctrl;
#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
	CANXL_RXFIFO_Type * base_rx_fifo;
	CANXL_RXFIFO_CONTROL_Type *base_rx_fifo_ctrl;
#endif
	uint8 instance;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pin_cfg;
	Canexcel_Ip_ConfigType *can_cfg;
	void (*irq_config_func)(void);
};

struct can_nxp_s32_tx_callback {
	Canexcel_Ip_DataInfoType tx_info;
	can_tx_callback_t function;
	void *arg;
};

struct can_nxp_s32_rx_callback {
	struct can_filter filter;
#ifndef CONFIG_CAN_NXP_S32_RX_FIFO
	Canexcel_Ip_DataInfoType rx_info;
#endif
	can_rx_callback_t function;
	void *arg;
};

struct can_nxp_s32_data {
	struct can_driver_data common;
	Canexcel_Ip_StateType *can_state;

	ATOMIC_DEFINE(rx_allocs, CONFIG_CAN_NXP_S32_MAX_RX);
	struct k_mutex rx_mutex;
	struct can_nxp_s32_rx_callback rx_cbs[CONFIG_CAN_NXP_S32_MAX_RX];
#ifndef CONFIG_CAN_NXP_S32_RX_FIFO
	Canexcel_RxFdMsg *rx_msg;
#endif

	ATOMIC_DEFINE(tx_allocs, CONFIG_CAN_NXP_S32_MAX_TX);
	struct k_sem tx_allocs_sem;
	struct k_mutex tx_mutex;
	struct can_nxp_s32_tx_callback tx_cbs[CONFIG_CAN_NXP_S32_MAX_TX];
	Canexcel_TxFdMsgType *tx_msg;

#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
	Canexcel_Ip_RxFifoFilterID_ADDR * rx_fifo_filter;
	Canexcel_RxFdMsg *rx_fifo;
#endif

	struct can_timing timing;
#ifdef CAN_NXP_S32_FD_MODE
	struct can_timing timing_data;
#endif
	enum can_state state;
};

static int can_nxp_s32_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

#ifdef CAN_NXP_S32_FD_MODE
	*cap |= CAN_MODE_FD;
#endif

	return 0;
}

#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
static void can_nxp_s32_config_rx_fifo_filter(const struct device *dev, int filter_id)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;

	/* Lock the RxFIFO by System by reading register */
	(void)config->base_rx_fifo_ctrl->RXFSYSLOCK;

	CanXL_ConfigIDFilter(config->base_rx_fifo,
				&data->rx_fifo_filter[filter_id], filter_id);

	if ((config->base_rx_fifo_ctrl->RXFCSTA & CANXL_RXFIFO_CONTROL_RXFCSTA_SYSLOCK_MASK)
						== CANXL_RXFIFO_CONTROL_RXFCSTA_SYSLOCK_MASK) {
		/* Clear the sys lock to enable transfers */
		config->base_rx_fifo_ctrl->RXFSYSLOCK =
						CANXL_RXFIFO_CONTROL_RXFSYSLOCK_SYSLOCK_MASK;
	}
}

/* Get the RxFiFO filter matched with the received RxFIFO message queue */
static inline int can_nxp_s32_get_rx_fifo_filter(struct can_nxp_s32_data *data)
{
	int alloc = -ENOSPC;
	uint32_t mask;

	for (int filter_id = 0; filter_id < CONFIG_CAN_NXP_S32_MAX_RX; filter_id++) {
		mask = data->rx_fifo_filter[filter_id].idAddrFilterL;

		if (mask == 0) {
			continue;
		}

		if ((data->rx_fifo[0].Header.Id & mask) ==
			(data->rx_fifo_filter[filter_id].idAddrFilterH & mask)) {
			alloc = filter_id;
			break;
		}
	}

	return alloc;
}
#endif

static int can_nxp_s32_start(const struct device *dev)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
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

	data->common.started = true;

	return 0;
}

static int can_nxp_s32_abort_msg(const struct can_nxp_s32_config *config, int mb_idx)
{
	uint32_t time_start = 0;
	int ret = 0;

	Canexcel_Ip_EnterFreezeMode(config->instance);

	CanXL_ClearMsgBuffIntCmd(config->base_grp_ctrl, mb_idx);
	CanXL_ClearMsgDescIntStatusFlag(config->base_grp_ctrl, mb_idx);

	time_start = k_uptime_get();
	/* Set system lock Status */
	(void)config->base_dsc_ctrl->DSCMBCTRLAR[mb_idx].SYSLOCK.DCSYSLOCK;
	while (CanXL_GetDescControlStatus(config->base_dsc_ctrl, mb_idx)
			== CANEXCEL_DESCNTSTATUS_LOCKED_HW) {
		if (k_uptime_get() - time_start >= CAN_NXP_S32_TIMEOUT_MS) {
			ret = CANEXCEL_STATUS_TIMEOUT;
			break;
		}
	}

	/* Inactive descriptor */
	config->base_dsc_ctrl->DSCMBCTRLAR[mb_idx].ACT.DCACT = 0;

	Canexcel_Ip_ExitFreezeMode(config->instance);

	return ret;
}

static int can_nxp_s32_stop(const struct device *dev)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	can_tx_callback_t function;
	void *arg;
	int alloc;
	int err;

	if (!data->common.started) {
		return -EALREADY;
	}

	data->common.started = false;

	/* Abort any pending TX frames before entering freeze mode */
	for (alloc = 0; alloc < CONFIG_CAN_NXP_S32_MAX_TX; alloc++) {
		function = data->tx_cbs[alloc].function;
		arg = data->tx_cbs[alloc].arg;

		if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
			if (can_nxp_s32_abort_msg(config,
					ALLOC_IDX_TO_TXMB_IDX(alloc))) {
				LOG_ERR("Can't abort message !");
			};

			function(dev, -ENETDOWN, arg);
			k_sem_give(&data->tx_allocs_sem);
		}
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


static int can_nxp_s32_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	Canexcel_Ip_ModesType can_nxp_s32_mode = CAN_MODE_NORMAL;
	bool canfd = false;
	bool brs = false;

	if (data->common.started) {
		return -EBUSY;
	}
#ifdef CAN_NXP_S32_FD_MODE
	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_FD)) != 0) {
#else
	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) != 0) {
#endif
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if ((mode & (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY))
				== (CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) {
		LOG_ERR("unsupported mode loopback and "
			"mode listen-only at the same time: 0x%08x", mode);
		return -ENOTSUP;
	}

	canfd = !!(mode & CAN_MODE_FD);
	brs = canfd;

	if (mode & CAN_MODE_LISTENONLY) {
		can_nxp_s32_mode = CANEXCEL_LISTEN_ONLY_MODE;
	} else if (mode & CAN_MODE_LOOPBACK) {
		can_nxp_s32_mode = CANEXCEL_LOOPBACK_MODE;
	}

	Canexcel_Ip_EnterFreezeMode(config->instance);

	CanXL_SetFDEnabled(config->base_sic, canfd, brs);

	CanXL_SetOperationMode(config->base_sic, can_nxp_s32_mode);

	Canexcel_Ip_ExitFreezeMode(config->instance);

	data->common.mode = mode;

	return 0;
}

static int can_nxp_s32_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_nxp_s32_config *config = dev->config;

	__ASSERT_NO_MSG(rate != NULL);

	return clock_control_get_rate(config->clock_dev, config->clock_subsys, rate);
}

static int can_nxp_s32_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_NXP_S32_MAX_RX;
}

static int can_nxp_s32_get_state(const struct device *dev, enum can_state *state,
						struct can_bus_err_cnt *err_cnt)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	uint32_t sys_status = config->base_sic->SYSS;

	if (state) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			if (sys_status & CANXL_SIC_SYSS_CBOFF_MASK) {
				*state = CAN_STATE_BUS_OFF;
			} else if (sys_status & CANXL_SIC_SYSS_CPASERR_MASK) {
				*state = CAN_STATE_ERROR_PASSIVE;
			} else if (sys_status & (CANXL_SIC_SYSS_CRXWRN_MASK
						| CANXL_SIC_SYSS_CTXWRN_MASK)) {
				*state = CAN_STATE_ERROR_WARNING;
			} else {
				*state = CAN_STATE_ERROR_ACTIVE;
			}
		}
	}

	if (err_cnt) {
		/* NXP S32 CANXL HAL is not supported error counter */
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return 0;
}

static void can_nxp_s32_set_state_change_callback(const struct device *dev,
							can_state_change_callback_t callback,
							void *user_data)
{
	struct can_nxp_s32_data *data = dev->data;

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_nxp_s32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	enum can_state state;
	uint64_t start_time;
	int ret = 0;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	can_nxp_s32_get_state(dev, &state, NULL);
	if (state != CAN_STATE_BUS_OFF) {
		return 0;
	}

	start_time = k_uptime_ticks();
	config->base_sic->BCFG1 &= (~CANXL_SIC_BCFG1_ABRDIS_MASK);

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		can_nxp_s32_get_state(dev, &state, NULL);

		while (state == CAN_STATE_BUS_OFF) {
			if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
				k_uptime_ticks() - start_time >= timeout.ticks) {
				ret = -EAGAIN;
			}

			can_nxp_s32_get_state(dev, &state, NULL);
		}
	}

	config->base_sic->BCFG1 |= CANXL_SIC_BCFG1_ABRDIS_MASK;

	return ret;
}
#endif

static void can_nxp_s32_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	int mb_indx = ALLOC_IDX_TO_RXMB_IDX(filter_id);

	if (filter_id < 0 || filter_id >= CONFIG_CAN_NXP_S32_MAX_RX) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	if (atomic_test_and_clear_bit(data->rx_allocs, filter_id)) {
#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
		data->rx_fifo_filter[mb_indx].idAddrFilterL = 0;
		data->rx_fifo_filter[mb_indx].idAddrFilterH = 0;

		Canexcel_Ip_EnterFreezeMode(config->instance);

		can_nxp_s32_config_rx_fifo_filter(dev, mb_indx);

		Canexcel_Ip_ExitFreezeMode(config->instance);
#else
		if (can_nxp_s32_abort_msg(config, mb_indx)) {
			LOG_ERR("Can't abort message !");
		};
#endif

		data->rx_cbs[filter_id].function = NULL;
		data->rx_cbs[filter_id].arg = NULL;
		data->rx_cbs[filter_id].filter = (struct can_filter){0};
	} else {
		LOG_WRN("Filter ID %d already detached", filter_id);
	}

	k_mutex_unlock(&data->rx_mutex);
}

static int can_nxp_s32_add_rx_filter(const struct device *dev,
				can_rx_callback_t callback,
				void *user_data,
				const struct can_filter *filter)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	int alloc = -ENOSPC;
	int mb_indx;
	uint32_t mask;

	__ASSERT_NO_MSG(callback != NULL);

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->rx_mutex, K_FOREVER);

	/* Find and allocate RX message buffer */
	for (int i = 0; i < CONFIG_CAN_NXP_S32_MAX_RX; i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			alloc = i;
			break;
		}
	}

	if (alloc == -ENOSPC) {
		LOG_ERR("No free filter bank found");
		goto unlock;
	}

	data->rx_cbs[alloc].function = callback;
	data->rx_cbs[alloc].arg = user_data;
	data->rx_cbs[alloc].filter = *filter;

	/* Set Rx Mb individual mask for */
	mb_indx = ALLOC_IDX_TO_RXMB_IDX(alloc);
	if (!!(filter->flags & CAN_FILTER_IDE)) {
		mask = filter->mask & CANXL_IP_ID_EXT_MASK;
	} else {
		mask = (filter->mask << CANXL_IP_ID_STD_SHIFT) & CANXL_IP_ID_STD_MASK;
	}

#ifndef CONFIG_CAN_ACCEPT_RTR
	mask |= CANXL_MSG_DESCRIPTORS_MDFLT1FD_RTRMSK_MASK;
#endif /* !CONFIG_CAN_ACCEPT_RTR */

	Canexcel_Ip_EnterFreezeMode(config->instance);

#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
	uint32_t filter_id;

	if (!!(filter->flags & CAN_FILTER_IDE)) {
		filter_id = filter->id & CANXL_IP_ID_EXT_MASK;
	} else {
		filter_id = (filter->id << CANXL_IP_ID_STD_SHIFT) & CANXL_IP_ID_STD_MASK;
	}

	data->rx_fifo_filter[mb_indx].filterType = CANEXCEL_IP_RX_FIFO_MASK_FILTER;
	data->rx_fifo_filter[mb_indx].idAddrFilterL = mask;
	data->rx_fifo_filter[mb_indx].idAddrFilterH = filter_id;

	can_nxp_s32_config_rx_fifo_filter(dev, mb_indx);
#else
	data->rx_cbs[alloc].rx_info = (Canexcel_Ip_DataInfoType) {
		.frame = CANEXCEL_CLASIC_FRAME,
		.idType = !!(filter->flags & CAN_FILTER_IDE) ?
				CANEXCEL_MSG_ID_EXT : CANEXCEL_MSG_ID_STD,
		.dataLength = CAN_NXP_S32_DATA_LENGTH,
	};

	Canexcel_Ip_SetRxIndividualMask(config->instance, mb_indx,
						data->rx_cbs[alloc].rx_info.frame, mask);

	Canexcel_Ip_ConfigRx(config->instance, mb_indx, filter->id,
					&data->rx_cbs[alloc].rx_info);

	Canexcel_Ip_ReceiveFD(config->instance, mb_indx, &data->rx_msg[alloc], FALSE);
#endif

	Canexcel_Ip_ExitFreezeMode(config->instance);

unlock:
	k_mutex_unlock(&data->rx_mutex);

	return alloc;
}

static int can_nxp_s32_send(const struct device *dev,
				const struct can_frame *frame,
				k_timeout_t timeout,
				can_tx_callback_t callback, void *user_data)
{
	const struct can_nxp_s32_config *config = dev->config;
	uint8_t data_length = can_dlc_to_bytes(frame->dlc);
	struct can_nxp_s32_data *data = dev->data;
	Canexcel_Ip_StatusType status;
	enum can_state state;
	int alloc, mb_indx;

	__ASSERT_NO_MSG(callback != NULL);

#ifdef CAN_NXP_S32_FD_MODE
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if ((frame->flags & CAN_FRAME_FDF) != 0 &&
			(config->base_sic->BCFG2 & CANXL_SIC_BCFG2_FDEN_MASK) == 0) {
		LOG_ERR("CAN FD format not supported in non-FD mode");
		return -ENOTSUP;
	}

	if ((frame->flags & CAN_FRAME_BRS) != 0 &&
			~(config->base_sic->BCFG1 & CANXL_SIC_BCFG1_FDRSDIS_MASK) == 0) {
		LOG_ERR("CAN FD BRS not supported in non-FD mode");
		return -ENOTSUP;
	}
#else
	if ((frame->flags & ~CAN_FRAME_IDE) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}
#endif

	if (data_length > sizeof(frame->data)) {
		LOG_ERR("data length (%d) > max frame data length (%d)",
			data_length, sizeof(frame->data));
		return -EINVAL;
	}

	if ((frame->flags & CAN_FRAME_FDF) == 0) {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("DLC of %d for non-FD format frame", frame->dlc);
			return -EINVAL;
		}
#ifdef CAN_NXP_S32_FD_MODE
	} else {
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("DLC of %d for CAN FD format frame", frame->dlc);
			return -EINVAL;
		}
#endif
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	can_nxp_s32_get_state(dev, &state, NULL);
	if (state == CAN_STATE_BUS_OFF) {
		LOG_ERR("Transmit failed, bus-off");
		return -ENETUNREACH;
	}

	if (k_sem_take(&data->tx_allocs_sem, timeout) != 0) {
		return -EAGAIN;
	}

	for (alloc = 0; alloc < CONFIG_CAN_NXP_S32_MAX_TX; alloc++) {
		if (!atomic_test_and_set_bit(data->tx_allocs, alloc)) {
			break;
		}
	}

	data->tx_cbs[alloc].function = callback;
	data->tx_cbs[alloc].arg = user_data;
	mb_indx = ALLOC_IDX_TO_TXMB_IDX(alloc);
	data->tx_cbs[alloc].tx_info = (Canexcel_Ip_DataInfoType) {
		.frame = !!(frame->flags & CAN_FRAME_FDF) ?
				CANEXCEL_FD_FRAME : CANEXCEL_CLASIC_FRAME,
		.enable_brs = !!(frame->flags & CAN_FRAME_BRS) ? TRUE : FALSE,
		.idType = !!(frame->flags & CAN_FRAME_IDE) ?
				CANEXCEL_MSG_ID_EXT : CANEXCEL_MSG_ID_STD,
		.priority = 0,
		.fd_padding = 0,
		.dataLength = data_length,
		.is_polling = FALSE
	};

	LOG_DBG("%s: Sending %d bytes Tx Mb %d, "
		"Tx Id: 0x%x, "
		"Id type: %s %s %s %s",
		dev->name, data_length,
		mb_indx, frame->id,
		!!(frame->flags & CAN_FRAME_IDE) ?
				"extended" : "standard",
		!!(frame->flags & CAN_FRAME_RTR) ? "RTR" : "",
		!!(frame->flags & CAN_FRAME_FDF) ? "FD frame" : "",
		!!(frame->flags & CAN_FRAME_BRS) ? "BRS" : "");

	k_mutex_lock(&data->tx_mutex, K_FOREVER);
	/* Send MB Interrupt */
	status = Canexcel_Ip_SendFDMsg(config->instance, mb_indx, &data->tx_cbs[alloc].tx_info,
				frame->id, (uint8_t *)&frame->data, &data->tx_msg[alloc]);
	k_mutex_unlock(&data->tx_mutex);

	if (status != CANEXCEL_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void nxp_s32_zcan_timing_to_canxl_timing(const struct can_timing *timing,
							Canexcel_Ip_TimeSegmentType *canxl_timing)
{
	LOG_DBG("propSeg: %d, phase_seg1: %d, phase_seg2: %d, prescaler: %d, sjw: %d",
		timing->prop_seg, timing->phase_seg1, timing->phase_seg2,
		timing->prescaler, timing->sjw);

	canxl_timing->propSeg = timing->prop_seg - 1U;
	canxl_timing->phaseSeg1 = timing->phase_seg1 - 1U;
	canxl_timing->phaseSeg2 = timing->phase_seg2 - 1U;
	canxl_timing->preDivider = timing->prescaler - 1U;
	canxl_timing->rJumpwidth = timing->sjw - 1U;
}

static int can_nxp_s32_set_timing(const struct device *dev,
			const struct can_timing *timing)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	Canexcel_Ip_TimeSegmentType can_time_segment = {0};

	if (data->common.started) {
		return -EBUSY;
	}

	nxp_s32_zcan_timing_to_canxl_timing(timing, &can_time_segment);

	/* Set timing for CAN instance*/
	CanXL_SetBaudRate(config->base_sic, &can_time_segment);

	return 0;
}

#ifdef CAN_NXP_S32_FD_MODE
static int can_nxp_s32_set_timing_data(const struct device *dev,
			const struct can_timing *timing_data)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	Canexcel_Ip_TimeSegmentType can_fd_time_segment = {0};

	if (data->common.started) {
		return -EBUSY;
	}

	nxp_s32_zcan_timing_to_canxl_timing(timing_data, &can_fd_time_segment);

	/* Set timing for CAN FD instance*/
	CanXL_SetFDBaudRate(config->base_sic, &can_fd_time_segment);

	return 0;
}
#endif

static void can_nxp_s32_err_callback(const struct device *dev,
					Canexcel_Ip_EventType eventType,
					uint32 u32SysStatus,
					const Canexcel_Ip_StateType *canexcelState)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;
	void *cb_data = data->common.state_change_cb_user_data;
	can_tx_callback_t function;
	int alloc;
	void *arg;

	switch (eventType) {
	case CANEXCEL_EVENT_TX_WARNING:
		LOG_DBG("Tx Warning (error 0x%x)", u32SysStatus);
		break;
	case CANEXCEL_EVENT_RX_WARNING:
		LOG_DBG("Rx Warning (error 0x%x)", u32SysStatus);
		break;
	case CANEXCEL_EVENT_BUSOFF:
		LOG_DBG("Bus Off (error 0x%x)", u32SysStatus);
		break;
	case CANEXCEL_EVENT_ERROR:
		LOG_DBG("Error Format Frames (error 0x%x)", u32SysStatus);
		break;
	case CANEXCEL_EVENT_ERROR_FD:
		LOG_DBG("Error Data Phase (error 0x%x)", u32SysStatus);
		break;
	case CANEXCEL_EVENT_PASSIVE:
		LOG_DBG("Error Passive (error 0x%x)", u32SysStatus);
		break;
	default:
		break;
	}

	can_nxp_s32_get_state(dev, &state, &err_cnt);
	if (data->state != state) {
		data->state = state;
		if (data->common.state_change_cb) {
			data->common.state_change_cb(dev, state, err_cnt, cb_data);
		}
	}

	if (state == CAN_STATE_BUS_OFF) {
		/* Abort any pending TX frames in case of bus-off */
		for (alloc = 0; alloc < CONFIG_CAN_NXP_S32_MAX_TX; alloc++) {
			/* Copy callback function and argument before clearing bit */
			function = data->tx_cbs[alloc].function;
			arg = data->tx_cbs[alloc].arg;

			if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
				if (can_nxp_s32_abort_msg(config,
						ALLOC_IDX_TO_TXMB_IDX(alloc))) {
					LOG_ERR("Can't abort message !");
				};

				function(dev, -ENETUNREACH, arg);
				k_sem_give(&data->tx_allocs_sem);
			}
		}
	}
}

static void nxp_s32_msg_data_to_zcan_frame(Canexcel_RxFdMsg msg_data,
							struct can_frame *frame)
{
	memset(frame, 0, sizeof(*frame));

	if (!!(msg_data.Header.Id & CANXL_TX_HEADER_IDE_MASK)) {
		frame->flags |= CAN_FRAME_IDE;
	}

	if (!!(frame->flags & CAN_FRAME_IDE)) {
		frame->id = (msg_data.Header.Id & CANXL_IP_ID_EXT_MASK);
	} else {
		frame->id = ((msg_data.Header.Id & CANXL_IP_ID_STD_MASK)
						>> CANXL_IP_ID_STD_SHIFT);
	}

	frame->dlc = (msg_data.Header.Control & CANXL_TX_HEADER_DLC_MASK)
						>> CANXL_TX_HEADER_DLC_SHIFT;

	if (!!(msg_data.Header.Control & CANXL_TX_HEADER_FDF_MASK)) {
		frame->flags |= CAN_FRAME_FDF;
	}

	if (!!(msg_data.Header.Control & CANXL_TX_HEADER_BRS_MASK)) {
		frame->flags |= CAN_FRAME_BRS;
	}

	if (!!(msg_data.Header.Id & CANXL_TX_HEADER_RTR_MASK)) {
		frame->flags |= CAN_FRAME_RTR;
	} else {
		memcpy(frame->data, msg_data.data, can_dlc_to_bytes(frame->dlc));
	}

#ifdef CONFIG_CAN_RX_TIMESTAMP
	frame->timestamp = msg_data.timeStampL;
#endif /* CAN_RX_TIMESTAMP */
}

static void can_nxp_s32_ctrl_callback(const struct device *dev,
					Canexcel_Ip_EventType eventType, uint32 buffidx,
					const Canexcel_Ip_StateType *canexcelState)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	struct can_frame frame = {0};
	can_tx_callback_t tx_func;
	can_rx_callback_t rx_func;
	int alloc;

	if (eventType == CANEXCEL_EVENT_TX_COMPLETE) {
		alloc = TX_MBIDX_TO_ALLOC_IDX(buffidx);
		tx_func = data->tx_cbs[alloc].function;
		LOG_DBG("%s: Sent Tx Mb %d", dev->name, buffidx);
		if (atomic_test_and_clear_bit(data->tx_allocs, alloc)) {
			tx_func(dev, 0, data->tx_cbs[alloc].arg);
			k_sem_give(&data->tx_allocs_sem);
		}
#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
	} else if (eventType == CANEXCEL_EVENT_RXFIFO_COMPLETE) {
		alloc = can_nxp_s32_get_rx_fifo_filter(data);

		if (alloc != -ENOSPC) {
			rx_func = data->rx_cbs[alloc].function;
			if (atomic_test_bit(data->rx_allocs, alloc)) {
				nxp_s32_msg_data_to_zcan_frame(data->rx_fifo[0], &frame);

				LOG_DBG("%s: Received %d bytes Rx FiFo %d, "
					"Rx Id: 0x%x, "
					"Id type: %s %s %s %s",
					dev->name, can_dlc_to_bytes(frame.dlc),
					alloc, frame.id,
					!!(frame.flags & CAN_FRAME_IDE) ?
							"extended" : "standard",
					!!(frame.flags & CAN_FRAME_RTR) ? "RTR" : "",
					!!(frame.flags & CAN_FRAME_FDF) ? "FD frame" : "",
					!!(frame.flags & CAN_FRAME_BRS) ? "BRS" : "");

				rx_func(dev, &frame, data->rx_cbs[alloc].arg);
			}
		}

		/* Pop 1 (= RXFSYSPOP + 1) received RxFIFO message queue */
		config->base_rx_fifo_ctrl->RXFSYSPOP = 0;
#else
	} else if (eventType == CANEXCEL_EVENT_RX_COMPLETE) {
		alloc = RX_MBIDX_TO_ALLOC_IDX(buffidx);
		rx_func = data->rx_cbs[alloc].function;
		if (atomic_test_bit(data->rx_allocs, alloc)) {
			nxp_s32_msg_data_to_zcan_frame(data->rx_msg[alloc], &frame);

			LOG_DBG("%s: Received %d bytes Rx Mb %d, "
				"Rx Id: 0x%x, "
				"Id type: %s %s %s %s",
				dev->name, can_dlc_to_bytes(frame.dlc),
				buffidx, frame.id,
				!!(frame.flags & CAN_FRAME_IDE) ?
						"extended" : "standard",
				!!(frame.flags & CAN_FRAME_RTR) ? "RTR" : "",
				!!(frame.flags & CAN_FRAME_FDF) ? "FD frame" : "",
				!!(frame.flags & CAN_FRAME_BRS) ? "BRS" : "");

			rx_func(dev, &frame, data->rx_cbs[alloc].arg);

			if (Canexcel_Ip_ReceiveFD(config->instance, buffidx,
				&data->rx_msg[alloc], FALSE) != CANEXCEL_STATUS_SUCCESS) {
				LOG_ERR("MB %d is not ready for receiving next message", buffidx);
			}
		}
#endif
	}
}

static int can_nxp_s32_init(const struct device *dev)
{
	const struct can_nxp_s32_config *config = dev->config;
	struct can_nxp_s32_data *data = dev->data;
	int err;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	Canexcel_Ip_TimeStampConf_Type time_stamp = {
		.ts64bit = FALSE, /* Time stamp size is 32 bits */
		.capture = CANEXCEL_TIMESTAMPCAPTURE_END,
		.src = CANTBS_TIMESURCE_BUS1
	};
#endif

	if (config->common.phy != NULL) {
		if (!device_is_ready(config->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Failed to enable clock");
		return err;
	}

	k_mutex_init(&data->rx_mutex);
	k_mutex_init(&data->tx_mutex);
	k_sem_init(&data->tx_allocs_sem, CONFIG_CAN_NXP_S32_MAX_TX, CONFIG_CAN_NXP_S32_MAX_TX);

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	/* Enable CANXL HW */
	IP_MC_RGM->PRST_0[0].PRST_0 &=
		~(MC_RGM_PRST_0_PERIPH_16_RST_MASK | MC_RGM_PRST_0_PERIPH_24_RST_MASK);

	err = can_calc_timing(dev, &data->timing, config->common.bus_speed,
			      config->common.sample_point);
	if (err == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}

	if (err > 0) {
		LOG_WRN("Sample-point error : %d", err);
	}

	LOG_DBG("Setting CAN bitrate %d:", config->common.bus_speed);
	nxp_s32_zcan_timing_to_canxl_timing(&data->timing, &config->can_cfg->bitrate);

#ifdef CAN_NXP_S32_FD_MODE
	err = can_calc_timing_data(dev, &data->timing_data, config->common.bus_speed_data,
				   config->common.sample_point_data);
	if (err == -EINVAL) {
		LOG_ERR("Can't find timing data for given param");
		return -EIO;
	}

	if (err > 0) {
		LOG_WRN("Sample-point-data err : %d", err);
	}

	LOG_DBG("Setting CAN FD bitrate %d:", config->common.bus_speed_data);
	nxp_s32_zcan_timing_to_canxl_timing(&data->timing_data, &config->can_cfg->Fd_bitrate);
#endif

	/* Initialize CAN structure */
	Canexcel_Ip_Init(config->instance, config->can_cfg, data->can_state);

	/* Configure time stamp */
#ifdef CONFIG_CAN_RX_TIMESTAMP
	Canexcel_Ip_ConfigTimeStamp(config->instance, &time_stamp);
#endif

	/* Enable Interrupt */
	Canexcel_Ip_EnableInterrupts(config->instance);

	/* Enable Error Interrupt */
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_RX_WARNING, TRUE);
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_TX_WARNING, TRUE);
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_ERR, TRUE);
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_BUSOFF, TRUE);
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_PASIVE_ERR, TRUE);
#ifdef CONFIG_CAN_NXP_S32_RX_FIFO
	CanXL_SetErrIntCmd(config->base_sic, CANXL_INT_RXFIFO_OVER, TRUE);

	/* Configure number of ID acceptance filters*/
	config->base_rx_fifo->AFCFG =
				CANXL_RXFIFO_AFCFG_ACPTID(CONFIG_CAN_NXP_S32_MAX_RX - 1);
#endif

	config->irq_config_func();

	can_nxp_s32_get_state(dev, &data->state, NULL);

	return 0;
}

static void can_nxp_s32_isr_rx_tx(const struct device *dev)
{
	const struct can_nxp_s32_config *config = dev->config;

	Canexcel_Ip_RxTxIRQHandler(config->instance);
}

static void can_nxp_s32_isr_error(const struct device *dev)
{
	const struct can_nxp_s32_config *config = dev->config;

	Canexcel_Ip_ErrIRQHandler(config->instance);
}

static const struct can_driver_api can_nxp_s32_driver_api = {
	.get_capabilities = can_nxp_s32_get_capabilities,
	.start = can_nxp_s32_start,
	.stop = can_nxp_s32_stop,
	.set_mode = can_nxp_s32_set_mode,
	.set_timing = can_nxp_s32_set_timing,
	.send = can_nxp_s32_send,
	.add_rx_filter = can_nxp_s32_add_rx_filter,
	.remove_rx_filter = can_nxp_s32_remove_rx_filter,
	.get_state = can_nxp_s32_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_nxp_s32_recover,
#endif
	.set_state_change_callback = can_nxp_s32_set_state_change_callback,
	.get_core_clock = can_nxp_s32_get_core_clock,
	.get_max_filters = can_nxp_s32_get_max_filters,
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
	},
#ifdef CAN_NXP_S32_FD_MODE
	.set_timing_data = can_nxp_s32_set_timing_data,
	.timing_data_min = {
		.sjw = 0x01,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x04,
		.prop_seg = 0x08,
		.phase_seg1 = 0x08,
		.phase_seg2 = 0x08,
		.prescaler = 0x100
	}
#endif
};

#define _CAN_NXP_S32_IRQ_CONFIG(node_id, prop, idx)					\
	do {										\
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),				\
				DT_IRQ_BY_IDX(node_id, idx, priority),			\
				UTIL_CAT(can_nxp_s32_isr_,				\
					DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)),	\
				DEVICE_DT_GET(node_id),					\
				DT_IRQ_BY_IDX(node_id, idx, flags));			\
		irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));				\
	} while (false);

#define CAN_NXP_S32_IRQ_CONFIG(n)							\
	static void can_irq_config_##n(void)						\
	{										\
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names, _CAN_NXP_S32_IRQ_CONFIG);	\
	}

#define CAN_NXP_S32_ERR_CALLBACK(n)							\
	void nxp_s32_can_##n##_err_callback(uint8 instance, Canexcel_Ip_EventType eventType,\
		uint32 u32SysStatus, const Canexcel_Ip_StateType *canexcelState)	\
	{										\
		const struct device *dev = DEVICE_DT_INST_GET(n);			\
		can_nxp_s32_err_callback(dev, eventType, u32SysStatus, canexcelState);	\
	}

#define CAN_NXP_S32_CTRL_CALLBACK(n)							\
	void nxp_s32_can_##n##_ctrl_callback(uint8 instance, Canexcel_Ip_EventType eventType,\
			uint32 buffIdx, const Canexcel_Ip_StateType *canexcelState)	\
	{										\
		const struct device *dev = DEVICE_DT_INST_GET(n);			\
		can_nxp_s32_ctrl_callback(dev, eventType, buffIdx, canexcelState);	\
	}

#if defined(CAN_NXP_S32_FD_MODE)
#define CAN_NXP_S32_BRS		1
#else
#define CAN_NXP_S32_BRS		0
#endif

#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
#define CAN_NXP_S32_CTRL_OPTIONS CANXL_IP_BUSOFF_RECOVERY_U32
#else
#define CAN_NXP_S32_CTRL_OPTIONS 0
#endif

#define CAN_NXP_S32_HW_INSTANCE_CHECK(i, n) \
	((DT_INST_REG_ADDR(n) == IP_CANXL_##i##__SIC_BASE) ? i : 0)

#define CAN_NXP_S32_HW_INSTANCE(n) \
	LISTIFY(__DEBRACKET CANXL_SIC_INSTANCE_COUNT, CAN_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define CAN_NXP_S32_INIT_DEVICE(n)							\
	CAN_NXP_S32_CTRL_CALLBACK(n)							\
	CAN_NXP_S32_ERR_CALLBACK(n)							\
	CAN_NXP_S32_IRQ_CONFIG(n)							\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	__nocache Canexcel_Ip_StateType can_nxp_s32_state##n;				\
	__nocache Canexcel_TxFdMsgType tx_msg##n[CONFIG_CAN_NXP_S32_MAX_TX];		\
	IF_DISABLED(CONFIG_CAN_NXP_S32_RX_FIFO,						\
		(__nocache Canexcel_RxFdMsg rx_msg_##n[CONFIG_CAN_NXP_S32_MAX_RX];))	\
	IF_ENABLED(CONFIG_CAN_NXP_S32_RX_FIFO,						\
		(__nocache Canexcel_RxFdMsg rx_fifo_##n[CAN_NXP_S32_RX_FIFO_DEPTH];	\
		static Canexcel_Ip_RxFifoFilterID_ADDR					\
			rx_fifo_filter##n[CONFIG_CAN_NXP_S32_MAX_RX];))			\
	Canexcel_Ip_ConfigType can_nxp_s32_default_config##n = {			\
		.rx_mbdesc = (uint8)IS_ENABLED(CONFIG_CAN_NXP_S32_RX_FIFO) ?		\
							0 : CONFIG_CAN_NXP_S32_MAX_RX,	\
		.tx_mbdesc = (uint8)CONFIG_CAN_NXP_S32_MAX_TX,				\
		.CanxlMode = CANEXCEL_LISTEN_ONLY_MODE,					\
		.fd_enable = (boolean)IS_ENABLED(CAN_NXP_S32_FD_MODE),			\
		.bitRateSwitch = (boolean)CAN_NXP_S32_BRS,				\
		.ctrlOptions = (uint32)CAN_NXP_S32_CTRL_OPTIONS,			\
		.Callback = nxp_s32_can_##n##_ctrl_callback,				\
		.ErrorCallback = nxp_s32_can_##n##_err_callback,			\
		IF_ENABLED(CONFIG_CAN_NXP_S32_RX_FIFO,					\
			(.is_rx_fifo_needed = (boolean)TRUE,				\
			.pRxFifoConfig = {						\
				.Rx_Fifo_Depth = CAN_NXP_S32_RX_FIFO_DEPTH,		\
				.Rx_Fifo_Watermark = CAN_NXP_S32_RX_FIFO_WATERMARK,	\
				.Rx_Fifo_Msg_Size = CAN_NXP_S32_DATA_LENGTH,		\
				.Rx_Fifo_KeepLast = (boolean)FALSE,			\
				.isPolling = (boolean)FALSE,				\
				.MsgBuffersPtr = (uint32 *)rx_fifo_##n,			\
			},))								\
	};										\
	static struct can_nxp_s32_data can_nxp_s32_data_##n = {				\
		.can_state = (Canexcel_Ip_StateType *)&can_nxp_s32_state##n,		\
		.tx_msg = tx_msg##n,							\
		IF_DISABLED(CONFIG_CAN_NXP_S32_RX_FIFO,					\
			(.rx_msg = rx_msg_##n,))					\
		IF_ENABLED(CONFIG_CAN_NXP_S32_RX_FIFO,					\
			(.rx_fifo = rx_fifo_##n,					\
			.rx_fifo_filter =						\
				(Canexcel_Ip_RxFifoFilterID_ADDR *)&rx_fifo_filter##n,))\
	};										\
	static struct can_nxp_s32_config can_nxp_s32_config_##n = {			\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(n, CAN_NXP_S32_MAX_BITRATE),	\
		.base_sic = (CANXL_SIC_Type *)DT_INST_REG_ADDR_BY_NAME(n, sic),		\
		.base_grp_ctrl = (CANXL_GRP_CONTROL_Type *)				\
				DT_INST_REG_ADDR_BY_NAME(n, grp_ctrl),			\
		.base_dsc_ctrl = (CANXL_DSC_CONTROL_Type *)				\
				DT_INST_REG_ADDR_BY_NAME(n, dsc_ctrl),			\
		IF_ENABLED(CONFIG_CAN_NXP_S32_RX_FIFO,					\
			(.base_rx_fifo = (CANXL_RXFIFO_Type *)				\
				DT_INST_REG_ADDR_BY_NAME(n, rx_fifo),			\
			.base_rx_fifo_ctrl = (CANXL_RXFIFO_CONTROL_Type *)		\
				DT_INST_REG_ADDR_BY_NAME(n, rx_fifo_ctrl),))		\
		.instance = CAN_NXP_S32_HW_INSTANCE(n),					\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)				\
				DT_INST_CLOCKS_CELL(n, name),				\
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.can_cfg = (Canexcel_Ip_ConfigType *)&can_nxp_s32_default_config##n,	\
		.irq_config_func = can_irq_config_##n					\
	};										\
	static int can_nxp_s32_##n##_init(const struct device *dev)			\
	{										\
		return can_nxp_s32_init(dev);						\
	}										\
	CAN_DEVICE_DT_INST_DEFINE(n,							\
				  can_nxp_s32_##n##_init,				\
				  NULL,							\
				  &can_nxp_s32_data_##n,				\
				  &can_nxp_s32_config_##n,				\
				  POST_KERNEL,						\
				  CONFIG_CAN_INIT_PRIORITY,				\
				  &can_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_NXP_S32_INIT_DEVICE)
