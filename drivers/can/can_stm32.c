/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>

#include "can_stm32.h"

LOG_MODULE_REGISTER(can_stm32, CONFIG_CAN_LOG_LEVEL);

#define CAN_INIT_TIMEOUT  (10 * sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC)

#define DT_DRV_COMPAT st_stm32_can

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
 * Translation tables
 * filter_in_bank[enum can_filter_type] = number of filters in bank for this type
 * reg_demand[enum can_filter_type] = how many registers are used for this type
 */
static const uint8_t filter_in_bank[] = {2, 4, 1, 2};
static const uint8_t reg_demand[] = {2, 1, 4, 2};

/*
 * Mutex to prevent simultaneous access to filter registers shared between CAN1
 * and CAN2.
 */
static struct k_mutex filter_mutex;

static void can_stm32_signal_tx_complete(const struct device *dev, struct can_mailbox *mb)
{
	if (mb->tx_callback) {
		mb->tx_callback(dev, mb->error, mb->callback_arg);
	} else  {
		k_sem_give(&mb->tx_int_sem);
	}
}

static void can_stm32_get_msg_fifo(CAN_FIFOMailBox_TypeDef *mbox,
				    struct zcan_frame *frame)
{
	if (mbox->RIR & CAN_RI0R_IDE) {
		frame->id = mbox->RIR >> CAN_RI0R_EXID_Pos;
		frame->id_type = CAN_EXTENDED_IDENTIFIER;
	} else {
		frame->id =  mbox->RIR >> CAN_RI0R_STID_Pos;
		frame->id_type = CAN_STANDARD_IDENTIFIER;
	}

	frame->rtr = mbox->RIR & CAN_RI0R_RTR ? CAN_REMOTEREQUEST : CAN_DATAFRAME;
	frame->dlc = mbox->RDTR & (CAN_RDT0R_DLC >> CAN_RDT0R_DLC_Pos);
	frame->data_32[0] = mbox->RDLR;
	frame->data_32[1] = mbox->RDHR;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	frame->timestamp = ((mbox->RDTR & CAN_RDT0R_TIME) >> CAN_RDT0R_TIME_Pos);
#endif
}

static inline void can_stm32_rx_isr_handler(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	CAN_FIFOMailBox_TypeDef *mbox;
	int filter_match_index;
	struct zcan_frame frame;
	can_rx_callback_t callback;

	while (can->RF0R & CAN_RF0R_FMP0) {
		mbox = &can->sFIFOMailBox[0];
		filter_match_index = ((mbox->RDTR & CAN_RDT0R_FMI)
					   >> CAN_RDT0R_FMI_Pos);

		if (filter_match_index >= CONFIG_CAN_MAX_FILTER) {
			break;
		}

		LOG_DBG("Message on filter index %d", filter_match_index);
		can_stm32_get_msg_fifo(mbox, &frame);

		callback = data->rx_cb[filter_match_index];

		if (callback) {
			callback(dev, &frame, data->cb_arg[filter_match_index]);
		}

		/* Release message */
		can->RF0R |= CAN_RF0R_RFOM0;
	}

	if (can->RF0R & CAN_RF0R_FOVR0) {
		LOG_ERR("RX FIFO Overflow");
	}
}

static int can_stm32_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	if (state != NULL) {
		if (can->ESR & CAN_ESR_BOFF) {
			*state = CAN_BUS_OFF;
		} else if (can->ESR & CAN_ESR_EPVF) {
			*state = CAN_ERROR_PASSIVE;
		} else if (can->ESR & CAN_ESR_EWGF) {
			*state = CAN_ERROR_WARNING;
		} else {
			*state = CAN_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt =
			((can->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
		err_cnt->rx_err_cnt =
			((can->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
	}

	return 0;
}

static inline void can_stm32_bus_state_change_isr(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *state_change_cb_data = data->state_change_cb_data;

#ifdef CONFIG_CAN_STATS
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	switch (can->ESR & CAN_ESR_LEC) {
	case (CAN_ESR_LEC_0):
		CAN_STATS_STUFF_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_1):
		CAN_STATS_FORM_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_1 | CAN_ESR_LEC_0):
		CAN_STATS_ACK_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2):
		CAN_STATS_BIT1_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2 | CAN_ESR_LEC_0):
		CAN_STATS_BIT0_ERROR_INC(dev);
		break;
	case (CAN_ESR_LEC_2 | CAN_ESR_LEC_1):
		CAN_STATS_CRC_ERROR_INC(dev);
		break;
	default:
		break;
	}

	/* Clear last error code flag */
	can->ESR |= CAN_ESR_LEC;
#endif /* CONFIG_CAN_STATS */

	(void)can_stm32_get_state(dev, &state, &err_cnt);

	if (state != data->state) {
		data->state = state;

		if (cb != NULL) {
			cb(dev, state, err_cnt, state_change_cb_data);
		}
	}
}

static inline void can_stm32_tx_isr_handler(const struct device *dev)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	uint32_t bus_off;

	bus_off = can->ESR & CAN_ESR_BOFF;

	if ((can->TSR & CAN_TSR_RQCP0) | bus_off) {
		data->mb0.error =
				can->TSR & CAN_TSR_TXOK0 ? 0  :
				can->TSR & CAN_TSR_TERR0 ? -EIO :
				can->TSR & CAN_TSR_ALST0 ? -EBUSY :
						 bus_off ? -ENETDOWN :
							   -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP0;
		can_stm32_signal_tx_complete(dev, &data->mb0);
	}

	if ((can->TSR & CAN_TSR_RQCP1) | bus_off) {
		data->mb1.error =
				can->TSR & CAN_TSR_TXOK1 ? 0  :
				can->TSR & CAN_TSR_TERR1 ? -EIO :
				can->TSR & CAN_TSR_ALST1 ? -EBUSY :
				bus_off                  ? -ENETDOWN :
							   -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP1;
		can_stm32_signal_tx_complete(dev, &data->mb1);
	}

	if ((can->TSR & CAN_TSR_RQCP2) | bus_off) {
		data->mb2.error =
				can->TSR & CAN_TSR_TXOK2 ? 0  :
				can->TSR & CAN_TSR_TERR2 ? -EIO :
				can->TSR & CAN_TSR_ALST2 ? -EBUSY :
				bus_off                  ? -ENETDOWN :
							   -EIO;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP2;
		can_stm32_signal_tx_complete(dev, &data->mb2);
	}

	if (can->TSR & CAN_TSR_TME) {
		k_sem_give(&data->tx_int_sem);
	}
}

#ifdef CONFIG_SOC_SERIES_STM32F0X

static void can_stm32_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	can_stm32_tx_isr_handler(dev);
	can_stm32_rx_isr_handler(dev);

	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#else

static void can_stm32_rx_isr(const struct device *dev)
{
	can_stm32_rx_isr_handler(dev);
}

static void can_stm32_tx_isr(const struct device *dev)
{
	can_stm32_tx_isr_handler(dev);
}

static void can_stm32_state_change_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	/* Signal bus-off to waiting tx */
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_tx_isr_handler(dev);
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#endif

static int can_enter_init_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR |= CAN_MCR_INRQ;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_INAK) == 0U) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			can->MCR &= ~CAN_MCR_INRQ;
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_leave_init_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR &= ~CAN_MCR_INRQ;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_INAK) != 0U) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_leave_sleep_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR &= ~CAN_MCR_SLEEP;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_SLAK) != 0) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_stm32_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;

	return 0;
}

static int can_stm32_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = dev->data;
	int ret;

	LOG_DBG("Set mode %d", mode);

	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (cfg->phy != NULL) {
		ret = can_transceiver_enable(cfg->phy);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto done;
		}
	}

	ret = can_enter_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		goto done;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* Loopback mode */
		can->BTR |= CAN_BTR_LBKM;
	} else {
		can->BTR &= ~CAN_BTR_LBKM;
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		/* Silent mode */
		can->BTR |= CAN_BTR_SILM;
	} else {
		can->BTR &= ~CAN_BTR_SILM;
	}

	if ((mode & CAN_MODE_ONE_SHOT) != 0) {
		/* No automatic retransmission */
		can->MCR |= CAN_MCR_NART;
	} else {
		can->MCR &= ~CAN_MCR_NART;
	}

done:
	ret = can_leave_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to leave init mode");

		if (cfg->phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(cfg->phy);
		}
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_stm32_set_timing(const struct device *dev,
				const struct can_timing *timing)
{
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = dev->data;
	int ret = -EIO;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	ret = can_enter_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		goto done;
	}

	can->BTR = (can->BTR & ~(CAN_BTR_BRP_Msk | CAN_BTR_TS1_Msk | CAN_BTR_TS2_Msk)) |
	     (((timing->phase_seg1 - 1) << CAN_BTR_TS1_Pos) & CAN_BTR_TS1_Msk) |
	     (((timing->phase_seg2 - 1) << CAN_BTR_TS2_Pos) & CAN_BTR_TS2_Msk) |
	     (((timing->prescaler  - 1) << CAN_BTR_BRP_Pos) & CAN_BTR_BRP_Msk);

	if (timing->sjw != CAN_SJW_NO_CHANGE) {
		can->BTR = (can->BTR & ~CAN_BTR_SJW_Msk) |
			   (((timing->sjw - 1) << CAN_BTR_SJW_Pos) & CAN_BTR_SJW_Msk);
	}

	ret = can_leave_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to leave init mode");
	} else {
		ret = 0;
	}

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static int can_stm32_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_stm32_config *cfg = dev->config;
	const struct device *clock;
	int ret;

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = clock_control_get_rate(clock,
				     (clock_control_subsys_t *) &cfg->pclken,
				     rate);
	if (ret != 0) {
		LOG_ERR("Failed call clock_control_get_rate: return [%d]", ret);
		return -EIO;
	}

	return 0;
}

static int can_stm32_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_stm32_config *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

static int can_stm32_init(const struct device *dev)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	struct can_timing timing;
	const struct device *clock;
	int ret;

	k_mutex_init(&filter_mutex);
	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, 0, 1);
	k_sem_init(&data->mb0.tx_int_sem, 0, 1);
	k_sem_init(&data->mb1.tx_int_sem, 0, 1);
	k_sem_init(&data->mb2.tx_int_sem, 0, 1);
	data->mb0.tx_callback = NULL;
	data->mb1.tx_callback = NULL;
	data->mb2.tx_callback = NULL;
	data->state_change_cb = NULL;
	data->state_change_cb_data = NULL;

	data->filter_usage = (1ULL << CAN_MAX_NUMBER_OF_FILTERS) - 1ULL;
	(void)memset(data->rx_cb, 0, sizeof(data->rx_cb));
	(void)memset(data->cb_arg, 0, sizeof(data->cb_arg));

	if (cfg->phy != NULL) {
		if (!device_is_ready(cfg->phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = clock_control_on(clock, (clock_control_subsys_t *) &cfg->pclken);
	if (ret != 0) {
		LOG_ERR("HAL_CAN_Init clock control on failed: %d", ret);
		return -EIO;
	}

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl setup failed (%d)", ret);
		return ret;
	}

	ret = can_leave_sleep_mode(can);
	if (ret) {
		LOG_ERR("Failed to exit sleep mode");
		return ret;
	}

	ret = can_enter_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		return ret;
	}

	can->MCR &= ~CAN_MCR_TTCM & ~CAN_MCR_ABOM & ~CAN_MCR_AWUM &
		    ~CAN_MCR_NART & ~CAN_MCR_RFLM & ~CAN_MCR_TXFP;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	can->MCR |= CAN_MCR_TTCM;
#endif
#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	can->MCR |= CAN_MCR_ABOM;
#endif
	timing.sjw = cfg->sjw;
	if (cfg->sample_point && USE_SP_ALGO) {
		ret = can_calc_timing(dev, &timing, cfg->bus_speed,
				      cfg->sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, TS1: %d, TS2: %d",
			timing.prescaler, timing.phase_seg1, timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing.prop_seg = 0;
		timing.phase_seg1 = cfg->prop_ts1;
		timing.phase_seg2 = cfg->ts2;
		ret = can_calc_prescaler(dev, &timing, cfg->bus_speed);
		if (ret) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}

	ret = can_stm32_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}

	ret = can_stm32_set_mode(dev, CAN_MODE_NORMAL);
	if (ret) {
		return ret;
	}

	(void)can_stm32_get_state(dev, &data->state, NULL);

	cfg->config_irq(can);
	can->IER |= CAN_IER_TMEIE;
	LOG_INF("Init of %s done", dev->name);
	return 0;
}

static void can_stm32_set_state_change_callback(const struct device *dev,
						can_state_change_callback_t cb,
						void *user_data)
{
	struct can_stm32_data *data = dev->data;
	const struct can_stm32_config *cfg = dev->config;
	CAN_TypeDef *can = cfg->can;

	data->state_change_cb = cb;
	data->state_change_cb_data = user_data;

	if (cb == NULL) {
		can->IER &= ~(CAN_IER_BOFIE | CAN_IER_EPVIE | CAN_IER_EWGIE);
	} else {
		can->IER |= CAN_IER_BOFIE | CAN_IER_EPVIE | CAN_IER_EWGIE;
	}
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_stm32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	int ret = -EAGAIN;
	int64_t start_time;

	if (!(can->ESR & CAN_ESR_BOFF)) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	ret = can_enter_init_mode(can);
	if (ret) {
		goto done;
	}

	can_leave_init_mode(can);

	start_time = k_uptime_ticks();

	while (can->ESR & CAN_ESR_BOFF) {
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			goto done;
		}
	}

	ret = 0;

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */


static int can_stm32_send(const struct device *dev, const struct zcan_frame *frame,
			  k_timeout_t timeout, can_tx_callback_t callback,
			  void *user_data)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	uint32_t transmit_status_register = can->TSR;
	CAN_TxMailBox_TypeDef *mailbox = NULL;
	struct can_mailbox *mb = NULL;

	LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , frame->dlc, dev->name
		    , frame->id
		    , frame->id_type == CAN_STANDARD_IDENTIFIER ?
		    "standard" : "extended"
		    , frame->rtr == CAN_DATAFRAME ? "no" : "yes");

	__ASSERT(frame->dlc == 0U || frame->data != NULL, "Dataptr is null");

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if (can->ESR & CAN_ESR_BOFF) {
		return -ENETDOWN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSR_TME)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_DBG("Transmit buffer full");
		if (k_sem_take(&data->tx_int_sem, timeout)) {
			return -EAGAIN;
		}

		k_mutex_lock(&data->inst_mutex, K_FOREVER);
		transmit_status_register = can->TSR;
	}

	if (transmit_status_register & CAN_TSR_TME0) {
		LOG_DBG("Using mailbox 0");
		mailbox = &can->sTxMailBox[0];
		mb = &(data->mb0);
	} else if (transmit_status_register & CAN_TSR_TME1) {
		LOG_DBG("Using mailbox 1");
		mailbox = &can->sTxMailBox[1];
		mb = &data->mb1;
	} else if (transmit_status_register & CAN_TSR_TME2) {
		LOG_DBG("Using mailbox 2");
		mailbox = &can->sTxMailBox[2];
		mb = &data->mb2;
	}

	mb->tx_callback = callback;
	mb->callback_arg = user_data;
	k_sem_reset(&mb->tx_int_sem);

	/* mailbox identifier register setup */
	mailbox->TIR &= CAN_TI0R_TXRQ;

	if (frame->id_type == CAN_STANDARD_IDENTIFIER) {
		mailbox->TIR |= (frame->id << CAN_TI0R_STID_Pos);
	} else {
		mailbox->TIR |= (frame->id << CAN_TI0R_EXID_Pos)
				| CAN_TI0R_IDE;
	}

	if (frame->rtr == CAN_REMOTEREQUEST) {
		mailbox->TIR |= CAN_TI1R_RTR;
	}

	mailbox->TDTR = (mailbox->TDTR & ~CAN_TDT1R_DLC) |
			((frame->dlc & 0xF) << CAN_TDT1R_DLC_Pos);

	mailbox->TDLR = frame->data_32[0];
	mailbox->TDHR = frame->data_32[1];

	mailbox->TIR |= CAN_TI0R_TXRQ;
	k_mutex_unlock(&data->inst_mutex);

	if (callback == NULL) {
		k_sem_take(&mb->tx_int_sem, K_FOREVER);
		return mb->error;
	}

	return 0;
}

static int can_stm32_shift_arr(void **arr, int start, int count)
{
	void **start_ptr = arr + start;
	size_t cnt;

	if (start > CONFIG_CAN_MAX_FILTER) {
		return -ENOSPC;
	}

	if (count > 0) {
		void *move_dest;

		/* Check if nothing used will be overwritten */
		for (int i = CONFIG_CAN_MAX_FILTER - count; i <= CONFIG_CAN_MAX_FILTER - 1; i++) {
			if (arr[i] != NULL) {
				return -ENOSPC;
			}
		}

		/* No need to shift. Destination is already outside the arr */
		if ((start + count) >= CONFIG_CAN_MAX_FILTER) {
			return 0;
		}

		cnt = (CONFIG_CAN_MAX_FILTER - start - count) * sizeof(void *);
		move_dest = start_ptr + count;
		memmove(move_dest, start_ptr, cnt);
		(void)memset(start_ptr, 0, count * sizeof(void *));
	} else if (count < 0) {
		count = -count;

		if (start - count < 0) {
			return -ENOSPC;
		}

		cnt = (CONFIG_CAN_MAX_FILTER - start) * sizeof(void *);
		memmove(start_ptr - count, start_ptr, cnt);
		(void)memset(arr + CONFIG_CAN_MAX_FILTER - count, 0,
			     count * sizeof(void *));
	}

	return 0;
}

static enum can_filter_type can_stm32_get_filter_type(int bank_nr, uint32_t mode_reg,
						      uint32_t scale_reg)
{
	uint32_t mode_masked  = (mode_reg  >> bank_nr) & 0x01;
	uint32_t scale_masked = (scale_reg >> bank_nr) & 0x01;
	enum can_filter_type type = (scale_masked << 1) | mode_masked;

	return type;
}

static int can_calc_filter_index(int filter_id, int bank_offset, uint32_t mode_reg,
				 uint32_t scale_reg)
{
	int filter_bank = bank_offset + filter_id / 4;
	int cnt = 0;
	uint32_t mode_masked, scale_masked;
	enum can_filter_type filter_type;

	/* count filters in the banks before this bank */
	for (int i = bank_offset; i < filter_bank; i++) {
		filter_type = can_stm32_get_filter_type(i, mode_reg, scale_reg);
		cnt += filter_in_bank[filter_type];
	}

	/* plus the filters in the same bank */
	mode_masked  = mode_reg & (1U << filter_bank);
	scale_masked = scale_reg & (1U << filter_bank);
	cnt += (!scale_masked && mode_masked) ? filter_id & 0x03 :
					       (filter_id & 0x03) >> 1;
	return cnt;
}

static void can_stm32_set_filter_bank(int filter_id,
				      CAN_FilterRegister_TypeDef *filter_reg,
				      enum can_filter_type filter_type,
				      uint32_t id, uint32_t mask)
{
	switch (filter_type) {
	case CAN_FILTER_STANDARD:
		switch (filter_id & 0x03) {
		case 0:
			filter_reg->FR1 = (filter_reg->FR1 & 0xFFFF0000) | id;
			break;
		case 1:
			filter_reg->FR1 = (filter_reg->FR1 & 0x0000FFFF)
					  | (id << 16);
			break;
		case 2:
			filter_reg->FR2 = (filter_reg->FR2 & 0xFFFF0000) | id;
			break;
		case 3:
			filter_reg->FR2 = (filter_reg->FR2 & 0x0000FFFF)
					   | (id << 16);
			break;
		}

		break;
	case CAN_FILTER_STANDARD_MASKED:
		switch (filter_id & 0x02) {
		case 0:
			filter_reg->FR1 = id | (mask << 16);
			break;
		case 2:
			filter_reg->FR2 = id | (mask << 16);
			break;
		}

		break;
	case CAN_FILTER_EXTENDED:
		switch (filter_id & 0x02) {
		case 0:
			filter_reg->FR1 = id;
			break;
		case 2:
			filter_reg->FR2 = id;
			break;
		}

		break;
	case CAN_FILTER_EXTENDED_MASKED:
		filter_reg->FR1 = id;
		filter_reg->FR2 = mask;
		break;
	}
}

static inline void can_stm32_set_mode_scale(enum can_filter_type filter_type,
					    uint32_t *mode_reg, uint32_t *scale_reg,
					    int  bank_nr)
{
	uint32_t mode_reg_bit  = (filter_type & 0x01) << bank_nr;
	uint32_t scale_reg_bit = (filter_type >>   1) << bank_nr;

	*mode_reg &= ~(1 << bank_nr);
	*mode_reg |= mode_reg_bit;

	*scale_reg &= ~(1 << bank_nr);
	*scale_reg |= scale_reg_bit;
}

static inline uint32_t can_generate_std_mask(const struct zcan_filter *filter)
{
	return  (filter->id_mask  << CAN_FIRX_STD_ID_POS) |
		(filter->rtr_mask << CAN_FIRX_STD_RTR_POS) |
		(1U               << CAN_FIRX_STD_IDE_POS);
}

static inline uint32_t can_generate_ext_mask(const struct zcan_filter *filter)
{
	return  (filter->id_mask  << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr_mask << CAN_FIRX_EXT_RTR_POS) |
		(1U               << CAN_FIRX_EXT_IDE_POS);
}

static inline uint32_t can_generate_std_id(const struct zcan_filter *filter)
{

	return  (filter->id << CAN_FIRX_STD_ID_POS) |
		(filter->rtr    << CAN_FIRX_STD_RTR_POS);

}

static inline uint32_t can_generate_ext_id(const struct zcan_filter *filter)
{
	return  (filter->id  << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr << CAN_FIRX_EXT_RTR_POS) |
		(1U          << CAN_FIRX_EXT_IDE_POS);
}

static inline int can_stm32_set_filter(const struct device *dev, const struct zcan_filter *filter,
				       int *filter_index)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *device_data = dev->data;
	CAN_TypeDef *can = cfg->master_can;
	uint32_t mask = 0U;
	uint32_t id = 0U;
	int filter_id = 0;
	int filter_index_new = -ENOSPC;
	int bank_nr;
	int bank_offset = 0;
	uint32_t bank_bit;
	int register_demand;
	enum can_filter_type filter_type;
	enum can_filter_type bank_mode;

	if (cfg->can != cfg->master_can) {
		/* CAN slave instance: start with offset */
		bank_offset = CAN_NUMBER_OF_FILTER_BANKS;
	}

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		id = can_generate_std_id(filter);
		filter_type = CAN_FILTER_STANDARD;

		if (filter->id_mask != CAN_STD_ID_MASK) {
			mask = can_generate_std_mask(filter);
			filter_type = CAN_FILTER_STANDARD_MASKED;
		}
	} else {
		id = can_generate_ext_id(filter);
		filter_type = CAN_FILTER_EXTENDED;

		if (filter->id_mask != CAN_EXT_ID_MASK) {
			mask = can_generate_ext_mask(filter);
			filter_type = CAN_FILTER_EXTENDED_MASKED;
		}
	}

	register_demand = reg_demand[filter_type];

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->id,
		filter->id_mask);
	LOG_DBG("Filter type: %s ID %s mask (%d)",
		    (filter_type == CAN_FILTER_STANDARD ||
		     filter_type == CAN_FILTER_STANDARD_MASKED) ?
		    "standard" : "extended",
		    (filter_type == CAN_FILTER_STANDARD_MASKED ||
		     filter_type == CAN_FILTER_EXTENDED_MASKED) ?
		    "with" : "without",
		    filter_type);

	do {
		uint64_t usage_shifted = (device_data->filter_usage >> filter_id);
		uint64_t usage_demand_mask = (1ULL << register_demand) - 1;
		bool bank_is_empty;

		bank_nr = bank_offset + filter_id / 4;
		bank_bit = (1U << bank_nr);
		bank_mode = can_stm32_get_filter_type(bank_nr, can->FM1R,
						      can->FS1R);

		bank_is_empty = CAN_BANK_IS_EMPTY(device_data->filter_usage,
						  bank_nr, bank_offset);

		if (!bank_is_empty && bank_mode != filter_type) {
			filter_id = (filter_id / 4 + 1) * 4;
		} else if (usage_shifted & usage_demand_mask) {
			device_data->filter_usage &=
				~(usage_demand_mask << filter_id);
			break;
		} else {
			filter_id += register_demand;
		}

		if (!usage_shifted) {
			LOG_INF("No free filter bank found");
			return -ENOSPC;
		}
	} while (filter_id < CAN_MAX_NUMBER_OF_FILTERS);

	/* set the filter init mode */
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	/* TODO fifo balancing */
	if (filter_type != bank_mode) {
		int shift_width, start_index;
		int res;
		uint32_t mode_reg  = can->FM1R;
		uint32_t scale_reg = can->FS1R;

		can_stm32_set_mode_scale(filter_type, &mode_reg, &scale_reg,
					 bank_nr);

		shift_width = filter_in_bank[filter_type] - filter_in_bank[bank_mode];

		filter_index_new = can_calc_filter_index(filter_id, bank_offset,
							 mode_reg, scale_reg);

		start_index = filter_index_new + filter_in_bank[bank_mode];

		if (shift_width && start_index <= CAN_MAX_NUMBER_OF_FILTERS) {
			res = can_stm32_shift_arr((void **)device_data->rx_cb,
						start_index,
						shift_width);

			res |= can_stm32_shift_arr(device_data->cb_arg,
						   start_index,
						   shift_width);

			if (filter_index_new >= CONFIG_CAN_MAX_FILTER || res) {
				LOG_INF("No space for a new filter!");
				filter_id = -ENOSPC;
				goto done;
			}
		}

		can->FM1R = mode_reg;
		can->FS1R = scale_reg;
	} else {
		filter_index_new = can_calc_filter_index(filter_id, bank_offset,
							 can->FM1R, can->FS1R);
		if (filter_index_new >= CAN_MAX_NUMBER_OF_FILTERS) {
			filter_id = -ENOSPC;
			goto done;
		}
	}

	can_stm32_set_filter_bank(filter_id, &can->sFilterRegister[bank_nr],
				  filter_type, id, mask);
done:
	can->FA1R |= bank_bit;
	can->FMR &= ~(CAN_FMR_FINIT);
	LOG_DBG("Filter set: id %d, index %d, bank %d", filter_id, filter_index_new, bank_nr);
	*filter_index = filter_index_new;
	return filter_id;
}

static int can_stm32_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				   void *cb_arg, const struct zcan_filter *filter)
{
	struct can_stm32_data *data = dev->data;
	int filter_index = 0;
	int filter_id;

	k_mutex_lock(&filter_mutex, K_FOREVER);
	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	filter_id = can_stm32_set_filter(dev, filter, &filter_index);
	if (filter_id != -ENOSPC) {
		data->rx_cb[filter_index] = cb;
		data->cb_arg[filter_index] = cb_arg;
	}

	k_mutex_unlock(&data->inst_mutex);
	k_mutex_unlock(&filter_mutex);

	return filter_id;
}

static void can_stm32_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_stm32_config *cfg = dev->config;
	struct can_stm32_data *data = dev->data;
	CAN_TypeDef *can = cfg->master_can;
	int bank_offset = 0;
	int bank_nr;
	int filter_index;
	uint32_t bank_bit;
	uint32_t mode_reg;
	uint32_t scale_reg;
	enum can_filter_type type;
	uint32_t reset_mask;

	__ASSERT_NO_MSG(filter_id >= 0 && filter_id < CAN_MAX_NUMBER_OF_FILTERS);

	k_mutex_lock(&filter_mutex, K_FOREVER);
	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (cfg->can != cfg->master_can) {
		bank_offset = CAN_NUMBER_OF_FILTER_BANKS;
	}

	bank_nr = bank_offset + filter_id / 4;
	bank_bit = (1U << bank_nr);
	mode_reg  = can->FM1R;
	scale_reg = can->FS1R;

	filter_index = can_calc_filter_index(filter_id, bank_offset, mode_reg, scale_reg);
	type = can_stm32_get_filter_type(bank_nr, mode_reg, scale_reg);

	LOG_DBG("Detach filter number %d (index %d), type %d", filter_id,
		    filter_index,
		    type);

	reset_mask = ((1 << (reg_demand[type])) - 1) << filter_id;
	data->filter_usage |= reset_mask;
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	can_stm32_set_filter_bank(filter_id, &can->sFilterRegister[bank_nr],
				  type, 0, 0xFFFFFFFF);

	if (!CAN_BANK_IS_EMPTY(data->filter_usage, bank_nr, bank_offset)) {
		can->FA1R |= bank_bit;
	} else {
		LOG_DBG("Bank number %d is empty -> deactivate", bank_nr);
	}

	can->FMR &= ~(CAN_FMR_FINIT);
	data->rx_cb[filter_index] = NULL;
	data->cb_arg[filter_index] = NULL;

	k_mutex_unlock(&data->inst_mutex);
	k_mutex_unlock(&filter_mutex);
}

static const struct can_driver_api can_api_funcs = {
	.get_capabilities = can_stm32_get_capabilities,
	.set_mode = can_stm32_set_mode,
	.set_timing = can_stm32_set_timing,
	.send = can_stm32_send,
	.add_rx_filter = can_stm32_add_rx_filter,
	.remove_rx_filter = can_stm32_remove_rx_filter,
	.get_state = can_stm32_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_stm32_recover,
#endif
	.set_state_change_callback = can_stm32_set_state_change_callback,
	.get_core_clock = can_stm32_get_core_clock,
	.get_max_bitrate = can_stm32_get_max_bitrate,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x00,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x07,
		.prop_seg = 0x00,
		.phase_seg1 = 0x0F,
		.phase_seg2 = 0x07,
		.prescaler = 0x400
	}
};

#ifdef CONFIG_SOC_SERIES_STM32F0X
#define CAN_STM32_IRQ_INST(inst)                                     \
static void config_can_##inst##_irq(CAN_TypeDef *can)                \
{                                                                    \
	IRQ_CONNECT(DT_INST_IRQN(inst),                              \
		    DT_INST_IRQ(inst, priority),                     \
		    can_stm32_isr, DEVICE_DT_INST_GET(inst), 0);     \
	irq_enable(DT_INST_IRQN(inst));                              \
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_FMPIE0 | \
		    CAN_IER_FMPIE1 | CAN_IER_BOFIE;                  \
	if (IS_ENABLED(CONFIG_CAN_STATS)) {                          \
		can->IER |= CAN_IER_LECIE;                           \
	}                                                            \
}
#else
#define CAN_STM32_IRQ_INST(inst)                                     \
static void config_can_##inst##_irq(CAN_TypeDef *can)                \
{                                                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, rx0, irq),             \
		    DT_INST_IRQ_BY_NAME(inst, rx0, priority),        \
		    can_stm32_rx_isr, DEVICE_DT_INST_GET(inst), 0);  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, rx0, irq));             \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, tx, irq),              \
		    DT_INST_IRQ_BY_NAME(inst, tx, priority),         \
		    can_stm32_tx_isr, DEVICE_DT_INST_GET(inst), 0);  \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, tx, irq));              \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, sce, irq),             \
		    DT_INST_IRQ_BY_NAME(inst, sce, priority),        \
		    can_stm32_state_change_isr,                      \
		    DEVICE_DT_INST_GET(inst), 0);                    \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, sce, irq));             \
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_FMPIE0 | \
		    CAN_IER_FMPIE1 | CAN_IER_BOFIE;                  \
	if (IS_ENABLED(CONFIG_CAN_STATS)) {                          \
		can->IER |= CAN_IER_LECIE;                           \
	}                                                            \
}
#endif /* CONFIG_SOC_SERIES_STM32F0X */

#define CAN_STM32_CONFIG_INST(inst)                                      \
PINCTRL_DT_INST_DEFINE(inst);                                            \
static const struct can_stm32_config can_stm32_cfg_##inst = {            \
	.can = (CAN_TypeDef *)DT_INST_REG_ADDR(inst),                    \
	.master_can = (CAN_TypeDef *)DT_INST_PROP_OR(inst,               \
		master_can_reg, DT_INST_REG_ADDR(inst)),                 \
	.bus_speed = DT_INST_PROP(inst, bus_speed),                      \
	.sample_point = DT_INST_PROP_OR(inst, sample_point, 0),          \
	.sjw = DT_INST_PROP_OR(inst, sjw, 1),                            \
	.prop_ts1 = DT_INST_PROP_OR(inst, prop_seg, 0) +                 \
		    DT_INST_PROP_OR(inst, phase_seg1, 0),                \
	.ts2 = DT_INST_PROP_OR(inst, phase_seg2, 0),                     \
	.pclken = {                                                      \
		.enr = DT_INST_CLOCKS_CELL(inst, bits),                  \
		.bus = DT_INST_CLOCKS_CELL(inst, bus),                   \
	},                                                               \
	.config_irq = config_can_##inst##_irq,                           \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	                 \
	.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(id, phys)),         \
	.max_bitrate = DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(id, 1000000), \
};

#define CAN_STM32_DATA_INST(inst) \
static struct can_stm32_data can_stm32_dev_data_##inst;

#define CAN_STM32_DEFINE_INST(inst)                                      \
DEVICE_DT_INST_DEFINE(inst, &can_stm32_init, NULL,                       \
		      &can_stm32_dev_data_##inst, &can_stm32_cfg_##inst, \
		      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,             \
		      &can_api_funcs);

#define CAN_STM32_INST(inst)      \
CAN_STM32_IRQ_INST(inst)          \
CAN_STM32_CONFIG_INST(inst)       \
CAN_STM32_DATA_INST(inst)         \
CAN_STM32_DEFINE_INST(inst)

DT_INST_FOREACH_STATUS_OKAY(CAN_STM32_INST)
