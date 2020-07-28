/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <sys/util.h>
#include <string.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <stdbool.h>
#include <drivers/can.h>

#include "can_stm32.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

#define CAN_INIT_TIMEOUT  (10U * sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC)

/*
 * CAN1 and CAN2 share filters
 * To use CAN2 the filters in CAN1 are needed, and for this to work
 * CAN1 needs to be clocked.
 * To make sure this is the case only allow CAN2 when CAN1 is also
 * enabled.
 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can2), st_stm32_can, okay)
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can1), st_stm32_can, disabled)
#error "CAN2 needs CAN1 for filtering"
#endif
#endif

/*
 * Global mutex for accessing the shared filter data.
 */
K_MUTEX_DEFINE(can_stm32_filter_mutex);

static inline void can_stm32_lock_filters(CAN_TypeDef *can)
{
	k_mutex_lock(&can_stm32_filter_mutex, K_FOREVER);

	/* Initialisation mode for the filter, this blocks CAN reception */
	can->FMR |= CAN_FMR_FINIT;
}

static inline void can_stm32_unlock_filters(CAN_TypeDef *can)
{
	/* Leave the initialisation mode for the filter */
	can->FMR &= ~CAN_FMR_FINIT;

	k_mutex_unlock(&can_stm32_filter_mutex);
}

static void can_stm32_signal_tx_complete(struct can_mailbox *mb)
{
	if (mb->tx_callback) {
		mb->tx_callback(mb->error_flags, mb->callback_arg);
	} else  {
		k_sem_give(&mb->tx_int_sem);
	}
}

static void can_stm32_get_msg_fifo(CAN_FIFOMailBox_TypeDef *mbox,
				    struct zcan_frame *msg)
{
	if (mbox->RIR & CAN_RI0R_IDE) {
		msg->ext_id = mbox->RIR >> CAN_RI0R_EXID_Pos;
		msg->id_type = CAN_EXTENDED_IDENTIFIER;
	} else {
		msg->std_id =  mbox->RIR >> CAN_RI0R_STID_Pos;
		msg->id_type = CAN_STANDARD_IDENTIFIER;
	}

	msg->rtr = mbox->RIR & CAN_RI0R_RTR ? CAN_REMOTEREQUEST : CAN_DATAFRAME;
	msg->dlc = mbox->RDTR & (CAN_RDT0R_DLC >> CAN_RDT0R_DLC_Pos);
	msg->data_32[0] = mbox->RDLR;
	msg->data_32[1] = mbox->RDHR;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	msg->timestamp = ((mbox->RDTR & CAN_RDT0R_TIME) >> CAN_RDT0R_TIME_Pos);
#endif
}

static inline uint32_t can_stm32_get_rfxr(CAN_TypeDef *can, uint32_t fifo_nr)
{
	return fifo_nr == 0U ? can->RF0R : can->RF1R;
}

static inline void can_stm32_set_rfxr(CAN_TypeDef *can, uint32_t fifo_nr,
				      uint32_t value)
{
	if (fifo_nr == 0U) {
		can->RF0R = value;
	} else {
		can->RF1R = value;
	}
}

static inline
void can_stm32_rx_isr_handler(const struct device *dev, uint32_t fifo_nr)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_filter_data *filter_data = cfg->shared_filter_data;
	const uint32_t port_nr = cfg->port_nr;
	uint32_t rfxr;

	while ((rfxr = can_stm32_get_rfxr(can, fifo_nr)) & CAN_RF0R_FMP0) {
		uint32_t match_index;
		CAN_FIFOMailBox_TypeDef *mbox;

		if (rfxr & CAN_RF0R_FOVR0) {
			LOG_ERR("RX FIFO %d Overflow", fifo_nr);
		}

		mbox = &can->sFIFOMailBox[fifo_nr];
		match_index = (mbox->RDTR & CAN_RDT0R_FMI) >> CAN_RDT0R_FMI_Pos;

		match_index += filter_data->map_offset[port_nr][fifo_nr];

		if (match_index >= CAN_STM32_FILTER_COUNT) {
			LOG_ERR("CAN%d FIFO%d illegal filter_match_index %d\n",
							port_nr, fifo_nr,
							match_index);
		} else {
			uint32_t filter_nr;
			struct zcan_frame msg;
			struct can_stm32_filter *filter;

			filter_nr = filter_data->filter_map[match_index];

			__ASSERT_NO_MSG(filter_nr < CONFIG_CAN_MAX_FILTER);

			can_stm32_get_msg_fifo(mbox, &msg);

			filter = &(data->filters[filter_nr]);

			if (filter->rx_cb) {
				filter->rx_cb(&msg, filter->cb_arg);
			}
		}

		/* Release message */
		rfxr |= CAN_RF0R_RFOM0;

		can_stm32_set_rfxr(can, fifo_nr, rfxr);
	}
}

static inline void can_stm32_bus_state_change_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;

	struct can_bus_err_cnt err_cnt;
	enum can_state state;

	if (!(can->ESR & CAN_ESR_EPVF) && !(can->ESR & CAN_ESR_BOFF)) {
		return;
	}

	err_cnt.tx_err_cnt = ((can->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
	err_cnt.rx_err_cnt = ((can->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos);

	if (can->ESR & CAN_ESR_BOFF) {
		state = CAN_BUS_OFF;
	} else if (can->ESR & CAN_ESR_EPVF) {
		state = CAN_ERROR_PASSIVE;
	} else {
		state = CAN_ERROR_ACTIVE;
	}

	if (data->state_change_isr) {
		data->state_change_isr(state, err_cnt);
	}
}

static inline
void can_stm32_tx_isr_handler(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;

	uint32_t bus_off;

	bus_off = can->ESR & CAN_ESR_BOFF;

	if ((can->TSR & CAN_TSR_RQCP0) | bus_off) {
		data->mb0.error_flags =
				can->TSR & CAN_TSR_TXOK0 ? CAN_TX_OK  :
				can->TSR & CAN_TSR_TERR0 ? CAN_TX_ERR :
				can->TSR & CAN_TSR_ALST0 ? CAN_TX_ARB_LOST :
						 bus_off ? CAN_TX_BUS_OFF :
							   CAN_TX_UNKNOWN;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP0;
		can_stm32_signal_tx_complete(&data->mb0);
	}

	if ((can->TSR & CAN_TSR_RQCP1) | bus_off) {
		data->mb1.error_flags =
				can->TSR & CAN_TSR_TXOK1 ? CAN_TX_OK  :
				can->TSR & CAN_TSR_TERR1 ? CAN_TX_ERR :
				can->TSR & CAN_TSR_ALST1 ? CAN_TX_ARB_LOST :
				bus_off                  ? CAN_TX_BUS_OFF :
							   CAN_TX_UNKNOWN;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP1;
		can_stm32_signal_tx_complete(&data->mb1);
	}

	if ((can->TSR & CAN_TSR_RQCP2) | bus_off) {
		data->mb2.error_flags =
				can->TSR & CAN_TSR_TXOK2 ? CAN_TX_OK  :
				can->TSR & CAN_TSR_TERR2 ? CAN_TX_ERR :
				can->TSR & CAN_TSR_ALST2 ? CAN_TX_ARB_LOST :
				bus_off                  ? CAN_TX_BUS_OFF :
							   CAN_TX_UNKNOWN;
		/* clear the request. */
		can->TSR |= CAN_TSR_RQCP2;
		can_stm32_signal_tx_complete(&data->mb2);
	}

	if (can->TSR & CAN_TSR_TME) {
		k_sem_give(&data->tx_int_sem);
	}
}

#ifdef CONFIG_SOC_SERIES_STM32F0X

static void can_stm32_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;

	can_stm32_tx_isr_handler(dev);
	can_stm32_rx_isr_handler(dev, 0U);
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#else /* ! CONFIG_SOC_SERIES_STM32F0X */

static void can_stm32_rx0_isr(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	can_stm32_rx_isr_handler(dev, 0U);
}

static void can_stm32_rx1_isr(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	can_stm32_rx_isr_handler(dev, 1U);
}

static void can_stm32_tx_isr(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);

	can_stm32_tx_isr_handler(dev);
}

static void can_stm32_state_change_isr(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;

	/*Signal bus-off to waiting tx*/
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_tx_isr_handler(dev);
		can_stm32_bus_state_change_isr(dev);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#endif /* CONFIG_SOC_SERIES_STM32F0X */

static int can_enter_init_mode(CAN_TypeDef *can)
{
	uint32_t start_time;

	can->MCR |= CAN_MCR_INRQ;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_INAK) == 0U) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			can->MCR &= ~CAN_MCR_INRQ;
			return CAN_TIMEOUT;
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
			return CAN_TIMEOUT;
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
			return CAN_TIMEOUT;
		}
	}

	return 0;
}

int can_stm32_runtime_configure(const struct device *dev, enum can_mode mode,
				uint32_t bitrate)
{
	CAN_HandleTypeDef hcan;
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = DEV_DATA(dev);
	const struct device *clock;
	uint32_t clock_rate;
	uint32_t prescaler;
	uint32_t reg_mode;
	uint32_t ts1;
	uint32_t ts2;
	uint32_t sjw;
	int ret;

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clock);
	hcan.Instance = can;
	ret = clock_control_get_rate(clock, (clock_control_subsys_t *) &cfg->pclken,
				     &clock_rate);
	if (ret != 0) {
		LOG_ERR("Failed call clock_control_get_rate: return [%d]", ret);
		return -EIO;
	}

	if (!bitrate) {
		bitrate = cfg->bus_speed;
	}

	prescaler = clock_rate / (BIT_SEG_LENGTH(cfg) * bitrate);
	if (prescaler == 0U || prescaler > 1024) {
		LOG_ERR("HAL_CAN_Init failed: prescaler > max (%d > 1024)",
					prescaler);
		return -EINVAL;
	}

	if (clock_rate % (BIT_SEG_LENGTH(cfg) * bitrate)) {
		LOG_ERR("Prescaler is not a natural number! "
			    "prescaler = clock_rate / ((PROP_SEG1 + SEG2 + 1)"
			    " * bus_speed); "
			    "prescaler = %d / ((%d + %d + 1) * %d)",
			    clock_rate,
			    cfg->prop_ts1,
			    cfg->ts2,
			    bitrate);
	}

	__ASSERT(cfg->sjw >= 1,      "SJW minimum is 1");
	__ASSERT(cfg->sjw <= 4,      "SJW maximum is 4");
	__ASSERT(cfg->prop_ts1 >= 1, "PROP_TS1 minimum is 1");
	__ASSERT(cfg->prop_ts1 <= 16, "PROP_TS1 maximum is 16");
	__ASSERT(cfg->ts2 >= 1,      "TS2 minimum is 1");
	__ASSERT(cfg->ts2 <= 8,      "TS2 maximum is 8");

	ts1 = ((cfg->prop_ts1 - 1) & 0x0F) << CAN_BTR_TS1_Pos;
	ts2 = ((cfg->ts2      - 1) & 0x07) << CAN_BTR_TS2_Pos;
	sjw = ((cfg->sjw      - 1) & 0x07) << CAN_BTR_SJW_Pos;

	reg_mode =  (mode == CAN_NORMAL_MODE)   ? 0U   :
		    (mode == CAN_LOOPBACK_MODE) ? CAN_BTR_LBKM :
		    (mode == CAN_SILENT_MODE)   ? CAN_BTR_SILM :
						CAN_BTR_LBKM | CAN_BTR_SILM;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	ret = can_enter_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to enter init mode");
		goto done;
	}

	can->BTR = reg_mode | sjw | ts1 | ts2 | (prescaler - 1U);

	ret = can_leave_init_mode(can);
	if (ret) {
		LOG_ERR("Failed to leave init mode");
		goto done;
	}

	LOG_DBG("Runtime configure of %s done", dev->name);
	ret = 0;
done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static int can_stm32_reset_rx_filters(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *master_can = cfg->master_can;


	master_can->FMR &= ~CAN_FMR_CAN2SB;

	master_can->FA1R  = 0x00000000; /* disable all filter banks */
	master_can->FM1R  = 0x00000000; /* clear id/mask mode */
	master_can->FFA1R = 0x00000000; /* clear filter FIFO assignment */
	master_can->FS1R  = 0x00000000; /* clear scale mode */

	for (int i = 0; i < CAN_STM32_FILTER_BANK_COUNT; i++) {
		master_can->sFilterRegister[i].FR1 = 0U;
		master_can->sFilterRegister[i].FR2 = 0U;
	}

	return 0;
}

static int can_stm32_setup_shared_filter(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	int need_reset = 1;
	uint32_t port_nr;

	can_stm32_lock_filters(cfg->master_can);

	__ASSERT_NO_MSG(cfg->shared_filter_data->can_dev[cfg->port_nr] == NULL);

	for (port_nr = 0; port_nr < CAN_STM32_SHARED_PORT_COUNT; ++port_nr) {
		if (cfg->shared_filter_data->can_dev[cfg->port_nr] != NULL) {
			need_reset = 0;
			break;
		}
	}

	if (need_reset) {
		can_stm32_reset_rx_filters(dev);
	}

	cfg->shared_filter_data->can_dev[cfg->port_nr] = dev;

	can_stm32_unlock_filters(cfg->master_can);

	return 0;
}

static int can_stm32_init(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(can2), okay)
	CAN_TypeDef *master_can = cfg->master_can;
#endif
	const struct device *clock;
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, 0U, 1U);
	k_sem_init(&data->mb0.tx_int_sem, 0U, 1U);
	k_sem_init(&data->mb1.tx_int_sem, 0U, 1U);
	k_sem_init(&data->mb2.tx_int_sem, 0U, 1U);
	data->mb0.tx_callback = NULL;
	data->mb1.tx_callback = NULL;
	data->mb2.tx_callback = NULL;
	data->state_change_isr = NULL;

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clock);

	ret = clock_control_on(clock, (clock_control_subsys_t *) &cfg->pclken);
	if (ret != 0) {
		LOG_ERR("HAL_CAN_Init clock control on failed: %d", ret);
		return -EIO;
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

	can_stm32_setup_shared_filter(dev);

	/* Set TX priority to chronological order */
	can->MCR |= CAN_MCR_TXFP;
	can->MCR &= ~CAN_MCR_TTCM & ~CAN_MCR_TTCM & ~CAN_MCR_ABOM &
		    ~CAN_MCR_AWUM & ~CAN_MCR_NART & ~CAN_MCR_RFLM;
#ifdef CONFIG_CAN_RX_TIMESTAMP
	can->MCR |= CAN_MCR_TTCM;
#endif
#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	can->MCR |= CAN_MCR_ABOM;
#endif

	ret = can_stm32_runtime_configure(dev, CAN_NORMAL_MODE, 0U);
	if (ret) {
		return ret;
	}

	/* Leave sleep mode after reset*/
	can->MCR &= ~CAN_MCR_SLEEP;

	cfg->config_irq(can);
	can->IER |= CAN_IER_TMEIE;

	return 0;
}

static void can_stm32_register_state_change_isr(const struct device *dev,
						can_state_change_isr_t isr)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;

	data->state_change_isr = isr;

	if (isr == NULL) {
		can->IER &= ~CAN_IER_EPVIE;
	} else {
		can->IER |= CAN_IER_EPVIE;
	}
}

static enum can_state can_stm32_get_state(const struct device *dev,
					  struct can_bus_err_cnt *err_cnt)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;

	if (err_cnt) {
		err_cnt->tx_err_cnt =
			((can->ESR & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
		err_cnt->rx_err_cnt =
			((can->ESR & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
	}

	if (can->ESR & CAN_ESR_BOFF) {
		return CAN_BUS_OFF;
	}

	if (can->ESR & CAN_ESR_EPVF) {
		return CAN_ERROR_PASSIVE;
	}

	return CAN_ERROR_ACTIVE;

}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_stm32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	int ret = CAN_TIMEOUT;
	int64_t start_time;

	if (!(can->ESR & CAN_ESR_BOFF)) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return CAN_TIMEOUT;
	}

	ret = can_enter_init_mode(can);
	if (ret) {
		goto done;
	}

	can_leave_init_mode(can);

	start_time = k_uptime_ticks();

	while (can->ESR & CAN_ESR_BOFF) {
		if (timeout != K_FOREVER &&
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


int can_stm32_send(const struct device *dev, const struct zcan_frame *msg,
		   k_timeout_t timeout, can_tx_callback_t callback,
		   void *callback_arg)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	uint32_t transmit_status_register = can->TSR;
	CAN_TxMailBox_TypeDef *mailbox = NULL;
	struct can_mailbox *mb = NULL;

	LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , msg->dlc, dev->name
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
				      msg->std_id :  msg->ext_id
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
		    "standard" : "extended"
		    , msg->rtr == CAN_DATAFRAME ? "no" : "yes");

	__ASSERT(msg->dlc == 0U || msg->data != NULL, "Dataptr is null");

	if (msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", msg->dlc, CAN_MAX_DLC);
		return CAN_TX_EINVAL;
	}

	if (can->ESR & CAN_ESR_BOFF) {
		return CAN_TX_BUS_OFF;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSR_TME)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_DBG("Transmit buffer full");
		if (k_sem_take(&data->tx_int_sem, timeout)) {
			return CAN_TIMEOUT;
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
	mb->callback_arg = callback_arg;
	k_sem_reset(&mb->tx_int_sem);

	/* mailbix identifier register setup */
	mailbox->TIR &= CAN_TI0R_TXRQ;

	if (msg->id_type == CAN_STANDARD_IDENTIFIER) {
		mailbox->TIR |= (msg->std_id << CAN_TI0R_STID_Pos);
	} else {
		mailbox->TIR |= (msg->ext_id << CAN_TI0R_EXID_Pos)
				| CAN_TI0R_IDE;
	}

	if (msg->rtr == CAN_REMOTEREQUEST) {
		mailbox->TIR |= CAN_TI1R_RTR;
	}

	mailbox->TDTR = (mailbox->TDTR & ~CAN_TDT1R_DLC) |
			((msg->dlc & 0xF) << CAN_TDT1R_DLC_Pos);

	mailbox->TDLR = msg->data_32[0];
	mailbox->TDHR = msg->data_32[1];

	mailbox->TIR |= CAN_TI0R_TXRQ;
	k_mutex_unlock(&data->inst_mutex);

	if (callback == NULL) {
		k_sem_take(&mb->tx_int_sem, K_FOREVER);
		return mb->error_flags;
	}

	return 0;
}

static inline uint32_t can_generate_std_mask(const struct zcan_filter *filter)
{
	return  (filter->std_id_mask << CAN_FIRX_STD_ID_POS) |
		(filter->rtr_mask    << CAN_FIRX_STD_RTR_POS) |
		(1U                  << CAN_FIRX_STD_IDE_POS);
}

static inline uint32_t can_generate_ext_mask(const struct zcan_filter *filter)
{
	return  (filter->ext_id_mask << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr_mask    << CAN_FIRX_EXT_RTR_POS) |
		(1U                  << CAN_FIRX_EXT_IDE_POS);
}

static inline uint32_t can_generate_std_id(const struct zcan_filter *filter)
{

	return  (filter->std_id << CAN_FIRX_STD_ID_POS) |
		(filter->rtr    << CAN_FIRX_STD_RTR_POS);

}

static inline uint32_t can_generate_ext_id(const struct zcan_filter *filter)
{
	return  (filter->ext_id << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr    << CAN_FIRX_EXT_RTR_POS) |
		(1U             << CAN_FIRX_EXT_IDE_POS);
}

static inline int can_stm32_get_filter_std(CAN_TypeDef *can, uint32_t bank_nr,
					   uint32_t *bank_offset,
					   uint32_t *std_id)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	uint32_t fr1 = can->sFilterRegister[bank_nr].FR1;
	uint32_t fr2 = can->sFilterRegister[bank_nr].FR2;

	switch (*bank_offset) {
	case 0:
		*std_id = fr1 & 0xFFFF;
		break;

	case 1:
		*std_id = (fr1 >> 16U) & 0xFFFF;
		break;

	case 2:
		*std_id = fr2 & 0xFFFF;
		break;

	case 3:
		*std_id = (fr2 >> 16U) & 0xFFFF;
		break;

	default:
		return -1;
	}

	*bank_offset += 1;

	return 0;
}

static inline int can_stm32_get_filter_std_mask(CAN_TypeDef *can,
						uint32_t bank_nr,
						uint32_t *bank_offset,
						uint32_t *std_id,
						uint32_t *std_mask)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	uint32_t fr1 = can->sFilterRegister[bank_nr].FR1;
	uint32_t fr2 = can->sFilterRegister[bank_nr].FR2;

	switch (*bank_offset) {
	case 0:
		*std_id = fr1 & 0xFFFF;
		*std_mask = (fr1 >> 16U) & 0xFFFF;
		break;

	case 2:
		*std_id = fr2 & 0xFFFF;
		*std_mask = (fr2 >> 16U) & 0xFFFF;
		break;

	default:
		return -1;
	}

	*bank_offset += 2;

	return 0;
}

static inline int can_stm32_get_filter_ext(CAN_TypeDef *can, uint32_t bank_nr,
					   uint32_t *bank_offset,
					   uint32_t *ext_id)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	switch (*bank_offset) {
	case 0:
		*ext_id = can->sFilterRegister[bank_nr].FR1;
		break;

	case 2:
		*ext_id = can->sFilterRegister[bank_nr].FR2;
		break;

	default:
		return -1;
	}

	*bank_offset += 2;

	return 0;
}

static inline int can_stm32_get_filter_ext_mask(CAN_TypeDef *can,
						uint32_t bank_nr,
						uint32_t *bank_offset,
						uint32_t *ext_id,
						uint32_t *ext_mask)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	*ext_id = can->sFilterRegister[bank_nr].FR1;
	*ext_mask = can->sFilterRegister[bank_nr].FR2;

	*bank_offset += 4;

	return 0;
}

static inline int can_stm32_get_filter(CAN_TypeDef *can, uint32_t bank_nr,
				       uint32_t *bank_offset,
				       enum can_filter_type filter_type,
				       uint32_t *id, uint32_t *mask)
{
	switch (filter_type) {
	case CAN_FILTER_EXTENDED_MASKED:
		return can_stm32_get_filter_ext_mask(can, bank_nr,
						     bank_offset, id, mask);

	case CAN_FILTER_EXTENDED:
		return can_stm32_get_filter_ext(can, bank_nr, bank_offset, id);

	case CAN_FILTER_STANDARD_MASKED:
		return can_stm32_get_filter_std_mask(can, bank_nr,
						     bank_offset, id, mask);

	case CAN_FILTER_STANDARD:
		return can_stm32_get_filter_std(can, bank_nr, bank_offset, id);

	default:
		return -1;
	}

	return 0;
}

static inline int can_stm32_set_filter_std(CAN_TypeDef *can, uint32_t bank_nr,
					   uint32_t *bank_offset,
					   uint32_t std_id)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	uint32_t fr1 = can->sFilterRegister[bank_nr].FR1;
	uint32_t fr2 = can->sFilterRegister[bank_nr].FR2;

	switch (*bank_offset) {
	case 0U:
		can->sFilterRegister[bank_nr].FR1 =
				(fr1 & 0xFFFF0000) | (std_id & 0xFFFF);
		break;

	case 1U:
		can->sFilterRegister[bank_nr].FR1 =
				(fr1 & 0x0000FFFF) | ((std_id & 0xFFFF) << 16U);
		break;

	case 2U:
		can->sFilterRegister[bank_nr].FR2 =
				(fr2 & 0xFFFF0000) | (std_id & 0xFFFF);
		break;

	case 3U:
		can->sFilterRegister[bank_nr].FR2 =
				(fr2 & 0x0000FFFF) | ((std_id & 0xFFFF) << 16U);
		break;

	default:
		return -1;
	}

	*bank_offset += 1U;

	return 0;
}

static inline int can_stm32_set_filter_std_mask(CAN_TypeDef *can,
						uint32_t bank_nr,
						uint32_t *bank_offset,
						uint32_t std_id,
						uint32_t std_mask)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	switch (*bank_offset) {
	case 0U:
		can->sFilterRegister[bank_nr].FR1 =
			((std_mask & 0xFFFF) << 16U) | (std_id & 0xFFFF);
		break;

	case 2U:
		can->sFilterRegister[bank_nr].FR2 =
			((std_mask & 0xFFFF) << 16U) | (std_id & 0xFFFF);
		break;

	default:
		return -1;

	}

	*bank_offset += 2U;

	return 0;
}

static inline int can_stm32_set_filter_ext(CAN_TypeDef *can, uint32_t bank_nr,
					   uint32_t *bank_offset,
					   uint32_t ext_id)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	switch (*bank_offset) {
	case 0U:
		can->sFilterRegister[bank_nr].FR1 = ext_id;
		break;

	case 2U:
		can->sFilterRegister[bank_nr].FR2 = ext_id;
		break;

	default:
		return -1;
	}

	*bank_offset += 2U;

	return 0;
}

static inline int can_stm32_set_filter_ext_mask(CAN_TypeDef *can,
						uint32_t bank_nr,
						uint32_t *bank_offset,
						uint32_t ext_id,
						uint32_t ext_mask)
{
	if (bank_nr >= CAN_STM32_FILTER_BANK_COUNT) {
		return -1;
	}

	can->sFilterRegister[bank_nr].FR1 = ext_id;
	can->sFilterRegister[bank_nr].FR2 = ext_mask;

	*bank_offset += 4U;

	return 0;
}


static inline int can_stm32_set_filter(CAN_TypeDef *can, uint32_t bank_nr,
				       uint32_t *bank_offset,
				       enum can_filter_type filter_type,
				       uint32_t id, uint32_t mask)
{
	switch (filter_type) {
	case CAN_FILTER_EXTENDED_MASKED:
		return can_stm32_set_filter_ext_mask(can, bank_nr,
						     bank_offset, id, mask);

	case CAN_FILTER_EXTENDED:
		return can_stm32_set_filter_ext(can, bank_nr, bank_offset, id);

	case CAN_FILTER_STANDARD_MASKED:
		return can_stm32_set_filter_std_mask(can, bank_nr,
						     bank_offset, id, mask);

	case CAN_FILTER_STANDARD:
		return can_stm32_set_filter_std(can, bank_nr, bank_offset, id);

	default:
		return -1;
	}

	return 0;
}

static int can_stm32_update_filter_banks_entry_fill(const struct device *dev,
					enum can_filter_type filter_type,
					uint32_t *bank_nr,
					uint32_t *bank_offset,
					uint32_t *filter_map_offset)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_filter_data *filter_data = cfg->shared_filter_data;
	CAN_TypeDef *master_can = cfg->master_can;

	uint32_t prev_id = 0;
	uint32_t prev_mask = 0;
	uint32_t read_bank_offset = 0;

	if (*bank_offset == 0) {
		return 0;
	}

	read_bank_offset = *bank_offset - 1;
	can_stm32_get_filter(master_can, *bank_nr, &read_bank_offset,
			     filter_type, &prev_id, &prev_mask);

	while (*bank_offset < 4) {

		can_stm32_set_filter(master_can, *bank_nr, bank_offset,
				     filter_type, prev_id, prev_mask);

		if (*filter_map_offset >= CAN_STM32_FILTER_COUNT) {
			return -1;
		}

		filter_data->filter_map[*filter_map_offset] =
				filter_data->filter_map[*filter_map_offset - 1];
		*filter_map_offset += 1;
	}

	*bank_offset = 0;
	*bank_nr += 1;

	return 0;
}

static int can_stm32_update_filter_banks_entry(const struct device *dev,
					       uint32_t filter_nr,
					       uint32_t *bank_nr,
					       uint32_t *bank_offset,
					       uint32_t *filter_map_offset)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_filter_data *filter_data = cfg->shared_filter_data;
	CAN_TypeDef *master_can = cfg->master_can;
	struct can_stm32_filter *filter;

	filter = &(data->filters[filter_nr]);

	if (*bank_offset == 0U) {
		uint32_t bank_bitpos = (1U << (*bank_nr));

		if (filter->fifo_nr == 0U) {
			master_can->FFA1R &= ~bank_bitpos;
		} else {
			master_can->FFA1R |= bank_bitpos;
		}

		switch (filter->filter_type) {
		default:
			break;

		case CAN_FILTER_STANDARD:
			master_can->FS1R &= ~bank_bitpos; /* 16bit scale mode */
			master_can->FM1R |= bank_bitpos; /* id mode */
			break;

		case CAN_FILTER_STANDARD_MASKED:
			master_can->FS1R &= ~bank_bitpos; /* 16bit scale mode */
			master_can->FM1R &= ~bank_bitpos; /* mask mode */
			break;

		case CAN_FILTER_EXTENDED:
			master_can->FS1R |= bank_bitpos; /* 32bit scale mode */
			master_can->FM1R |= bank_bitpos; /* id mode */
			break;

		case CAN_FILTER_EXTENDED_MASKED:
			master_can->FS1R |= bank_bitpos;  /* 32bit scale mode */
			master_can->FM1R &= ~bank_bitpos; /* mask mode */
			break;
		}

		/* Filter bank activation */
		master_can->FA1R |= bank_bitpos;
	}

	can_stm32_set_filter(master_can, *bank_nr, bank_offset,
			     filter->filter_type, filter->filter_id,
			     filter->filter_mask);

	if (*filter_map_offset >= CAN_STM32_FILTER_COUNT) {
		return -1;
	}

	filter_data->filter_map[*filter_map_offset] = filter_nr;
	*filter_map_offset += 1U;

	if (*bank_offset == 4U) {
		*bank_offset = 0U;
		*bank_nr += 1U;
	}

	return 0;
}

static int can_stm32_update_filter_banks_type(const struct device *dev,
					      uint32_t fifo_nr,
					      enum can_filter_type filter_type,
					      uint32_t *bank_nr,
					      uint32_t *filter_map_offset)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	struct can_stm32_filter *filter;
	int res;
	uint32_t filter_nr;
	uint32_t bank_offset;


	bank_offset = 0U;
	for (filter_nr = 0U; filter_nr < CONFIG_CAN_MAX_FILTER; ++filter_nr) {
		filter = &(data->filters[filter_nr]);

		if ((filter->rx_cb == NULL) ||
		    (filter->fifo_nr != fifo_nr) ||
		    (filter->filter_type != filter_type)) {
			continue;
		}

		res = can_stm32_update_filter_banks_entry(dev, filter_nr,
							  bank_nr, &bank_offset,
							  filter_map_offset);
		if (res < 0) {
			return res;
		}

	}

	res = can_stm32_update_filter_banks_entry_fill(dev, filter_type,
						       bank_nr, &bank_offset,
						       filter_map_offset);
	if (res < 0) {
		return res;
	}

	return 0;
}

static int can_stm32_update_filter_banks_fifo(const struct device *dev,
					      uint32_t fifo_nr,
					      uint32_t *bank_nr,
					      uint32_t *filter_map_offset)
{
	static const enum can_filter_type
	sort_order[CAN_STM32_FILTER_TYPE_COUNT] = {
		CAN_FILTER_EXTENDED,
		CAN_FILTER_STANDARD,
		CAN_FILTER_EXTENDED_MASKED,
		CAN_FILTER_STANDARD_MASKED,
	};

	uint32_t n;

	for (n = 0U; n < CAN_STM32_FILTER_TYPE_COUNT; ++n) {
		int res = can_stm32_update_filter_banks_type(dev, fifo_nr,
							 sort_order[n],
							 bank_nr,
							 filter_map_offset);
		if (res < 0) {
			return res;
		}
	}

	return 0;
}

/*
 * Update the filters for the selected port
 */
static int can_stm32_update_filter_banks_port(const struct device *dev,
					      uint32_t *bank_nr,
					      uint32_t *filter_map_offset)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	const uint32_t port_nr = cfg->port_nr;
	struct can_stm32_filter_data *filter_data = cfg->shared_filter_data;

	for (uint32_t fifo_nr = 0U; fifo_nr < CAN_STM32_FIFO_COUNT; fifo_nr++) {
		int res;

		filter_data->map_offset[port_nr][fifo_nr] = *filter_map_offset;

		res = can_stm32_update_filter_banks_fifo(dev, fifo_nr, bank_nr,
							 filter_map_offset);
		if (res < 0) {
			return res;
		}
	}

	return 0;
}

/*
 * Update all filter banks for both ports that share the filter banks.
 */
static int can_stm32_update_filter_banks(const struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_filter_data *filter = cfg->shared_filter_data;
	CAN_TypeDef *master_can = cfg->master_can;

	int res;
	uint32_t port_nr;
	uint32_t bank_nr = 0U;
	uint32_t filter_map_offset = 0U;

	/* Initialisation mode for the filter */
	master_can->FMR |= CAN_FMR_FINIT;

	res = can_stm32_reset_rx_filters(dev);
	if (res < 0) {
		return res;
	}

	for (port_nr = 0U; port_nr < CAN_STM32_SHARED_PORT_COUNT; port_nr++) {
		const struct device *d = filter->can_dev[port_nr];

		if (d != NULL) {
			res = can_stm32_update_filter_banks_port(d, &bank_nr,
							&filter_map_offset);
			if (res < 0) {
				return res;
			}
		} else {
			uint32_t fifo_nr = 0;

			for ( ; fifo_nr < CAN_STM32_FIFO_COUNT; fifo_nr++) {
				filter->map_offset[port_nr][fifo_nr] =
							filter_map_offset;
			}
		}

		if (port_nr == 0) {
			/* set start offset for port 2 filters */
			master_can->FMR |= ((bank_nr & 0x1F) << 8);
		}
	}

	/* Leave the initialisation mode for the filter */
	master_can->FMR &= ~CAN_FMR_FINIT;

	return 0;
}

static inline void can_stm32_convert_filter(struct can_stm32_filter *dst,
					    const struct zcan_filter *src)
{
	if (src->id_type == CAN_STANDARD_IDENTIFIER) {
		dst->filter_id = can_generate_std_id(src);

		if (src->std_id_mask != CAN_STD_ID_MASK) {
			dst->filter_mask = can_generate_std_mask(src);
			dst->filter_type = CAN_FILTER_STANDARD_MASKED;
		} else {
			dst->filter_mask = 0;
			dst->filter_type = CAN_FILTER_STANDARD;
		}
	} else {
		dst->filter_id = can_generate_ext_id(src);

		if (src->ext_id_mask != CAN_EXT_ID_MASK) {
			dst->filter_mask = can_generate_ext_mask(src);
			dst->filter_type = CAN_FILTER_EXTENDED_MASKED;
		} else {
			dst->filter_mask = 0;
			dst->filter_type = CAN_FILTER_EXTENDED;
		}
	}
}

static inline int can_stm32_add_filter(const struct device *dev,
				       can_rx_callback_t cb, void *cb_arg,
				       const struct zcan_filter *filter)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	int filter_nr;

	for (filter_nr = 0; filter_nr < CONFIG_CAN_MAX_FILTER; filter_nr++) {
		struct can_stm32_filter *f = &data->filters[filter_nr];

		if (f->rx_cb == NULL) {
			f->rx_cb = cb;
			f->cb_arg = cb_arg;
			f->fifo_nr = filter_nr % 2;

			can_stm32_convert_filter(f, filter);

			return filter_nr;
		}
	}

	return -1;
}

static inline int can_stm32_remove_filter(const struct device *dev, int filter_nr)
{
	struct can_stm32_data *data = DEV_DATA(dev);

	if (filter_nr < 0 || filter_nr >= CONFIG_CAN_MAX_FILTER) {
		return -1;
	}

	data->filters[filter_nr].rx_cb = NULL;

	return 0;
}

int can_stm32_attach_isr(const struct device *dev, can_rx_callback_t isr,
			 void *cb_arg,
			 const struct zcan_filter *filter)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	int filter_nr;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	can_stm32_lock_filters(cfg->master_can);

	filter_nr = can_stm32_add_filter(dev, isr, cb_arg, filter);

	if (filter_nr >= 0) {

		if (can_stm32_update_filter_banks(dev) < 0) {
			can_stm32_remove_filter(dev, filter_nr);
			can_stm32_update_filter_banks(dev);
			filter_nr = -1;
		}
	}

	can_stm32_unlock_filters(cfg->master_can);

	k_mutex_unlock(&data->inst_mutex);

	return filter_nr;
}

void can_stm32_detach(const struct device *dev, int filter_nr)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	const struct can_stm32_config *cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(filter_nr >= 0 && filter_nr < CONFIG_CAN_MAX_FILTER);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	can_stm32_lock_filters(cfg->master_can);

	can_stm32_remove_filter(dev, filter_nr);
	can_stm32_update_filter_banks(dev);

	can_stm32_unlock_filters(cfg->master_can);

	k_mutex_unlock(&data->inst_mutex);
}

static const struct can_driver_api can_api_funcs = {
	.configure = can_stm32_runtime_configure,
	.send = can_stm32_send,
	.attach_isr = can_stm32_attach_isr,
	.detach = can_stm32_detach,
	.get_state = can_stm32_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_stm32_recover,
#endif
	.register_state_change_isr = can_stm32_register_state_change_isr
};

static struct can_stm32_filter_data can_stm32_shared_filter_data_1_2 = {};

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can1), st_stm32_can, okay)

static void config_can_1_irq(CAN_TypeDef *can);

static const struct can_stm32_config can_stm32_cfg_1 = {
	.can = (CAN_TypeDef *)DT_REG_ADDR(DT_NODELABEL(can1)),
	.master_can = (CAN_TypeDef *)DT_REG_ADDR(DT_NODELABEL(can1)),
	.shared_filter_data = &can_stm32_shared_filter_data_1_2,
	.bus_speed = DT_PROP(DT_NODELABEL(can1), bus_speed),
	.port_nr = 0,
	.sjw = DT_PROP(DT_NODELABEL(can1), sjw),
	.prop_ts1 = DT_PROP(DT_NODELABEL(can1), prop_seg) +
					DT_PROP(DT_NODELABEL(can1), phase_seg1),
	.ts2 = DT_PROP(DT_NODELABEL(can1), phase_seg2),
	.pclken = {
		.enr = DT_CLOCKS_CELL(DT_NODELABEL(can1), bits),
		.bus = DT_CLOCKS_CELL(DT_NODELABEL(can1), bus),
	},
	.config_irq = config_can_1_irq
};

static struct can_stm32_data can_stm32_dev_data_1;

DEVICE_AND_API_INIT(can_stm32_1, DT_LABEL(DT_NODELABEL(can1)), &can_stm32_init,
		    &can_stm32_dev_data_1, &can_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_1_irq(CAN_TypeDef *can)
{
	LOG_DBG("Enable CAN1 IRQ");
#ifdef CONFIG_SOC_SERIES_STM32F0X
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(can1)),
		    DT_IRQ(DT_NODELABEL(can1), priority),
		    can_stm32_isr, DEVICE_GET(can_stm32_1), 0U);
	irq_enable(DT_IRQN(DT_NODELABEL(can1)));
#else
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx0, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx0, priority),
		    can_stm32_rx0_isr, DEVICE_GET(can_stm32_1), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx0, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx1, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx1, priority),
		    can_stm32_rx1_isr, DEVICE_GET(can_stm32_1), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), rx1, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), tx, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), tx, priority),
		    can_stm32_tx_isr, DEVICE_GET(can_stm32_1), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), tx, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can1), sce, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can1), sce, priority),
		    can_stm32_state_change_isr, DEVICE_GET(can_stm32_1), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can1), sce, irq));
#endif
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_BOFIE |
		    CAN_IER_FMPIE0 | CAN_IER_FMPIE1;
}

#if defined(CONFIG_NET_SOCKETS_CAN)

#include "socket_can_generic.h"

static int socket_can_init_1(const struct device *dev)
{
	const struct device *can_dev = DEVICE_GET(can_stm32_1);
	struct socket_can_context *socket_context = dev->data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->name, can_dev, can_dev->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	socket_context->rx_tid =
		k_thread_create(&socket_context->rx_thread_data,
				rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(rx_thread_stack),
				rx_thread, socket_context, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

NET_DEVICE_INIT(socket_can_stm32_1, SOCKET_CAN_NAME_1, socket_can_init_1,
		device_pm_control_nop, &socket_can_context_1, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&socket_can_api,
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(can1), okay) */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(can2), st_stm32_can, okay)

static void config_can_2_irq(CAN_TypeDef *can);

static const struct can_stm32_config can_stm32_cfg_2 = {
	.can = (CAN_TypeDef *)DT_REG_ADDR(DT_NODELABEL(can2)),
	.master_can = (CAN_TypeDef *)DT_PROP(DT_NODELABEL(can2),
								master_can_reg),
	.shared_filter_data = &can_stm32_shared_filter_data_1_2,
	.bus_speed = DT_PROP(DT_NODELABEL(can2), bus_speed),
	.port_nr = 1,
	.sjw = DT_PROP(DT_NODELABEL(can2), sjw),
	.prop_ts1 = DT_PROP(DT_NODELABEL(can2), prop_seg) +
					DT_PROP(DT_NODELABEL(can2), phase_seg1),
	.ts2 = DT_PROP(DT_NODELABEL(can2), phase_seg2),
	.pclken = {
		.enr = DT_CLOCKS_CELL(DT_NODELABEL(can2), bits),
		.bus = DT_CLOCKS_CELL(DT_NODELABEL(can2), bus),
	},
	.config_irq = config_can_2_irq
};

static struct can_stm32_data can_stm32_dev_data_2;

DEVICE_AND_API_INIT(can_stm32_2, DT_LABEL(DT_NODELABEL(can2)), &can_stm32_init,
		    &can_stm32_dev_data_2, &can_stm32_cfg_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_2_irq(CAN_TypeDef *can)
{
	LOG_DBG("Enable CAN2 IRQ");
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx0, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx0, priority),
		    can_stm32_rx0_isr, DEVICE_GET(can_stm32_2), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx0, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx1, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx1, priority),
		    can_stm32_rx1_isr, DEVICE_GET(can_stm32_2), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can2), rx1, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can2), tx, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can2), tx, priority),
		    can_stm32_tx_isr, DEVICE_GET(can_stm32_2), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can2), tx, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_NODELABEL(can2), sce, irq),
		    DT_IRQ_BY_NAME(DT_NODELABEL(can2), sce, priority),
		    can_stm32_state_change_isr, DEVICE_GET(can_stm32_2), 0U);
	irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(can2), sce, irq));
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_BOFIE |
		    CAN_IER_FMPIE0 | CAN_IER_FMPIE1;
}

#if defined(CONFIG_NET_SOCKETS_CAN)

#include "socket_can_generic.h"

static int socket_can_init_2(const struct device *dev)
{
	const struct device *can_dev = DEVICE_GET(can_stm32_2);
	struct socket_can_context *socket_context = dev->data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->name, can_dev, can_dev->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	socket_context->rx_tid =
		k_thread_create(&socket_context->rx_thread_data,
				rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(rx_thread_stack),
				rx_thread, socket_context, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

NET_DEVICE_INIT(socket_can_stm32_2, SOCKET_CAN_NAME_2, socket_can_init_2,
		device_pm_control_nop, &socket_can_context_2, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&socket_can_api,
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(can2), okay) */
