/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/stm32_clock_control.h>
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

#define CAN_INIT_TIMEOUT  (10 * sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC)
#define CAN_BOFF_RECOVER_TIMEOUT  (10 * sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC)

/*
 * Translation tables
 * filter_in_bank[enum can_filter_type] = number of filters in bank for this type
 * reg_demand[enum can_filter_type] = how many registers are used for this type
 */
static const u8_t filter_in_bank[] = {2, 4, 1, 2};
static const u8_t reg_demand[] = {2, 1, 4, 2};

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

static inline
void can_stm32_rx_isr_handler(CAN_TypeDef *can, struct can_stm32_data *data)
{
	CAN_FIFOMailBox_TypeDef *mbox;
	int filter_match_index;
	struct zcan_frame msg;
	can_rx_callback_t callback;

	while (can->RF0R & CAN_RF0R_FMP0) {
		mbox = &can->sFIFOMailBox[0];
		filter_match_index = ((mbox->RDTR & CAN_RDT0R_FMI)
					   >> CAN_RDT0R_FMI_Pos);

		if (filter_match_index >= CONFIG_CAN_MAX_FILTER) {
			break;
		}

		LOG_DBG("Message on filter index %d", filter_match_index);
		can_stm32_get_msg_fifo(mbox, &msg);

		callback = data->rx_cb[filter_match_index];

		if (callback) {
			callback(&msg, data->cb_arg[filter_match_index]);
		}

		/* Release message */
		can->RF0R |= CAN_RF0R_RFOM0;
	}

	if (can->RF0R & CAN_RF0R_FOVR0) {
		LOG_ERR("RX FIFO Overflow");
	}
}

static inline void can_stm32_bus_state_change_isr(CAN_TypeDef *can,
						  struct can_stm32_data *data)
{
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
void can_stm32_tx_isr_handler(CAN_TypeDef *can, struct can_stm32_data *data)
{
	u32_t bus_off;

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

static void can_stm32_isr(void *arg)
{
	struct device *dev;
	struct can_stm32_data *data;
	const struct can_stm32_config *cfg;
	CAN_TypeDef *can;

	dev = (struct device *)arg;
	data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);
	can = cfg->can;

	can_stm32_tx_isr_handler(can, data);
	can_stm32_rx_isr_handler(can, data);
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_bus_state_change_isr(can, data);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#else

static void can_stm32_rx_isr(void *arg)
{
	struct device *dev;
	struct can_stm32_data *data;
	const struct can_stm32_config *cfg;
	CAN_TypeDef *can;

	dev = (struct device *)arg;
	data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);
	can = cfg->can;

	can_stm32_rx_isr_handler(can, data);
}

static void can_stm32_tx_isr(void *arg)
{
	struct device *dev;
	struct can_stm32_data *data;
	const struct can_stm32_config *cfg;
	CAN_TypeDef *can;

	dev = (struct device *)arg;
	data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);
	can = cfg->can;

	can_stm32_tx_isr_handler(can, data);
}

static void can_stm32_state_change_isr(void *arg)
{
	struct device *dev;
	struct can_stm32_data *data;
	const struct can_stm32_config *cfg;
	CAN_TypeDef *can;

	dev = (struct device *)arg;
	data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);
	can = cfg->can;


	/*Signal bus-off to waiting tx*/
	if (can->MSR & CAN_MSR_ERRI) {
		can_stm32_tx_isr_handler(can, data);
		can_stm32_bus_state_change_isr(can, data);
		can->MSR |= CAN_MSR_ERRI;
	}
}

#endif

static int can_enter_init_mode(CAN_TypeDef *can)
{
	u32_t start_time;

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
	u32_t start_time;

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
	u32_t start_time;

	can->MCR &= ~CAN_MCR_SLEEP;
	start_time = k_cycle_get_32();

	while ((can->MSR & CAN_MSR_SLAK) != 0) {
		if (k_cycle_get_32() - start_time > CAN_INIT_TIMEOUT) {
			return CAN_TIMEOUT;
		}
	}

	return 0;
}

int can_stm32_runtime_configure(struct device *dev, enum can_mode mode,
				u32_t bitrate)
{
	CAN_HandleTypeDef hcan;
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;
	struct can_stm32_data *data = DEV_DATA(dev);
	struct device *clock;
	u32_t clock_rate;
	u32_t prescaler;
	u32_t reg_mode;
	u32_t ts1;
	u32_t ts2;
	u32_t sjw;
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

	__ASSERT(cfg->sjw <= 0x03,      "SJW maximum is 3");
	__ASSERT(cfg->prop_ts1 <= 0x0F, "PROP_BS1 maximum is 15");
	__ASSERT(cfg->ts2 <= 0x07,      "BS2 maximum is 7");

	ts1 = ((cfg->prop_ts1 & 0x0F) - 1) << CAN_BTR_TS1_Pos;
	ts2 = ((cfg->ts2      & 0x07) - 1) << CAN_BTR_TS2_Pos;
	sjw = ((cfg->sjw      & 0x07) - 1) << CAN_BTR_SJW_Pos;

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

	LOG_DBG("Runtime configure of %s done", dev->config->name);
	ret = 0;
done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}

static int can_stm32_init(struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	struct device *clock;
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, 0, 1);
	k_sem_init(&data->mb0.tx_int_sem, 0, 1);
	k_sem_init(&data->mb1.tx_int_sem, 0, 1);
	k_sem_init(&data->mb2.tx_int_sem, 0, 1);
	data->mb0.tx_callback = NULL;
	data->mb1.tx_callback = NULL;
	data->mb2.tx_callback = NULL;
	data->state_change_isr = NULL;

	data->filter_usage = (1ULL << CAN_MAX_NUMBER_OF_FILTERS) - 1ULL;
	(void)memset(data->rx_cb, 0, sizeof(data->rx_cb));
	(void)memset(data->cb_arg, 0, sizeof(data->cb_arg));

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

	ret = can_stm32_runtime_configure(dev, CAN_NORMAL_MODE, 0);
	if (ret) {
		return ret;
	}

	/* Leave sleep mode after reset*/
	can->MCR &= ~CAN_MCR_SLEEP;

	cfg->config_irq(can);
	can->IER |= CAN_IER_TMEIE;
	LOG_INF("Init of %s done", dev->config->name);
	return 0;
}

static void can_stm32_register_state_change_isr(struct device *dev,
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

static enum can_state can_stm32_get_state(struct device *dev,
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
int can_stm32_recover(struct device *dev, s32_t timeout)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	int ret = CAN_TIMEOUT;
	u32_t start_time;

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

	start_time = k_cycle_get_32();

	while (can->ESR & CAN_ESR_BOFF) {
		if (k_cycle_get_32() - start_time >= CAN_BOFF_RECOVER_TIMEOUT) {
			goto done;
		}
	}

	ret = 0;

done:
	k_mutex_unlock(&data->inst_mutex);
	return ret;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */


int can_stm32_send(struct device *dev, const struct zcan_frame *msg,
		   s32_t timeout, can_tx_callback_t callback, void *callback_arg)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	u32_t transmit_status_register = can->TSR;
	CAN_TxMailBox_TypeDef *mailbox = NULL;
	struct can_mailbox *mb = NULL;

	LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , msg->dlc, dev->config->name
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
				      msg->std_id :  msg->ext_id
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
		    "standard" : "extended"
		    , msg->rtr == CAN_DATAFRAME ? "no" : "yes");

	__ASSERT(msg->dlc == 0U || msg->data != NULL, "Dataptr is null");
	__ASSERT(msg->dlc <= CAN_MAX_DLC, "DLC > 8");

	if (can->ESR & CAN_ESR_BOFF) {
		return CAN_TX_BUS_OFF;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSR_TME)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_DBG("Transmit buffer full. Wait with timeout (%dms)",
			    timeout);
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

static inline int can_stm32_check_free(void **arr, int start, int end)
{
	int i;

	for (i = start; i <= end; i++) {
		if (arr[i] != NULL) {
			return 0;
		}
	}
	return 1;
}

static int can_stm32_shift_arr(void **arr, int start, int count)
{
	void **start_ptr = arr + start;
	size_t cnt;

	if (start > CONFIG_CAN_MAX_FILTER) {
		return CAN_NO_FREE_FILTER;
	}

	if (count > 0) {
		void *move_dest;

		/* Check if nothing used will be overwritten */
		if (!can_stm32_check_free(arr, CONFIG_CAN_MAX_FILTER - count,
					       CONFIG_CAN_MAX_FILTER - 1)) {
			return CAN_NO_FREE_FILTER;
		}

		/* No need to shift. Destination is already outside the arr*/
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
			return CAN_NO_FREE_FILTER;
		}

		cnt = (CONFIG_CAN_MAX_FILTER - start) * sizeof(void *);
		memmove(start_ptr - count, start_ptr, cnt);
		(void)memset(arr + CONFIG_CAN_MAX_FILTER - count, 0,
			     count * sizeof(void *));
	}

	return 0;
}

enum can_filter_type can_stm32_get_filter_type(int bank_nr, u32_t mode_reg,
					       u32_t scale_reg)
{
	u32_t mode_masked  = (mode_reg  >> bank_nr) & 0x01;
	u32_t scale_masked = (scale_reg >> bank_nr) & 0x01;
	enum can_filter_type type = (scale_masked << 1) | mode_masked;

	return type;
}

static int can_calc_filter_index(int filter_nr, u32_t mode_reg, u32_t scale_reg)
{
	int filter_bank = filter_nr / 4;
	int cnt = 0;
	u32_t mode_masked, scale_masked;
	enum can_filter_type filter_type;
	/*count filters in the banks before */
	for (int i = 0; i < filter_bank; i++) {
		filter_type = can_stm32_get_filter_type(i, mode_reg, scale_reg);
		cnt += filter_in_bank[filter_type];
	}

	/* plus the filters in the same bank */
	mode_masked  = mode_reg & (1U << filter_bank);
	scale_masked = scale_reg & (1U << filter_bank);
	cnt += (!scale_masked && mode_masked) ? filter_nr & 0x03 :
					       (filter_nr & 0x03) >> 1;
	return cnt;
}

static void can_stm32_set_filter_bank(int filter_nr,
				      CAN_FilterRegister_TypeDef *filter_reg,
				      enum can_filter_type filter_type,
				      u32_t id, u32_t mask)
{
	switch (filter_type) {
	case CAN_FILTER_STANDARD:
		switch (filter_nr & 0x03) {
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
		switch (filter_nr & 0x02) {
		case 0:
			filter_reg->FR1 = id | (mask << 16);
			break;
		case 2:
			filter_reg->FR2 = id | (mask << 16);
			break;
		}

		break;
	case CAN_FILTER_EXTENDED:
		switch (filter_nr & 0x02) {
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
					    u32_t *mode_reg, u32_t *scale_reg,
					    int  bank_nr)
{
	u32_t mode_reg_bit  = (filter_type & 0x01) << bank_nr;
	u32_t scale_reg_bit = (filter_type >>   1) << bank_nr;

	*mode_reg &= ~(1 << bank_nr);
	*mode_reg |= mode_reg_bit;

	*scale_reg &= ~(1 << bank_nr);
	*scale_reg |= scale_reg_bit;
}

static inline u32_t can_generate_std_mask(const struct zcan_filter *filter)
{
	return  (filter->std_id_mask << CAN_FIRX_STD_ID_POS) |
		(filter->rtr_mask    << CAN_FIRX_STD_RTR_POS) |
		(1U                  << CAN_FIRX_STD_IDE_POS);
}

static inline u32_t can_generate_ext_mask(const struct zcan_filter *filter)
{
	return  (filter->ext_id_mask << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr_mask    << CAN_FIRX_EXT_RTR_POS) |
		(1U                  << CAN_FIRX_EXT_IDE_POS);
}

static inline u32_t can_generate_std_id(const struct zcan_filter *filter)
{

	return  (filter->std_id << CAN_FIRX_STD_ID_POS) |
		(filter->rtr    << CAN_FIRX_STD_RTR_POS);

}

static inline u32_t can_generate_ext_id(const struct zcan_filter *filter)
{
	return  (filter->ext_id << CAN_FIRX_EXT_EXT_ID_POS) |
		(filter->rtr    << CAN_FIRX_EXT_RTR_POS) |
		(1U             << CAN_FIRX_EXT_IDE_POS);
}

static inline int can_stm32_set_filter(const struct zcan_filter *filter,
				       struct can_stm32_data *device_data,
				       CAN_TypeDef *can,
				       int *filter_index)
{
	u32_t mask = 0U;
	u32_t id = 0U;
	int filter_nr = 0;
	int filter_index_new = CAN_NO_FREE_FILTER;
	int bank_nr;
	u32_t bank_bit;
	int register_demand;
	enum can_filter_type filter_type;
	enum can_filter_type bank_mode;

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		id = can_generate_std_id(filter);
		filter_type = CAN_FILTER_STANDARD;

		if (filter->std_id_mask != CAN_STD_ID_MASK) {
			mask = can_generate_std_mask(filter);
			filter_type = CAN_FILTER_STANDARD_MASKED;
		}
	} else {
		id = can_generate_ext_id(filter);
		filter_type = CAN_FILTER_EXTENDED;

		if (filter->ext_id_mask != CAN_EXT_ID_MASK) {
			mask = can_generate_ext_mask(filter);
			filter_type = CAN_FILTER_EXTENDED_MASKED;
		}
	}

	register_demand = reg_demand[filter_type];

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->ext_id,
		    filter->ext_id_mask);
	LOG_DBG("Filter type: %s ID %s mask (%d)",
		    (filter_type == CAN_FILTER_STANDARD ||
		     filter_type == CAN_FILTER_STANDARD_MASKED) ?
		    "standard" : "extended",
		    (filter_type == CAN_FILTER_STANDARD_MASKED ||
		     filter_type == CAN_FILTER_EXTENDED_MASKED) ?
		    "with" : "without",
		    filter_type);

	do {
		u64_t usage_shifted = (device_data->filter_usage >> filter_nr);
		u64_t usage_demand_mask = (1ULL << register_demand) - 1;
		bool bank_is_empty;

		bank_nr = filter_nr / 4;
		bank_bit = (1U << bank_nr);
		bank_mode = can_stm32_get_filter_type(bank_nr, can->FM1R,
						      can->FS1R);

		bank_is_empty = CAN_BANK_IS_EMPTY(device_data->filter_usage,
						  bank_nr);

		if (!bank_is_empty && bank_mode != filter_type) {
			filter_nr = (bank_nr + 1) * 4;
		} else if (usage_shifted & usage_demand_mask) {
			device_data->filter_usage &=
				~(usage_demand_mask << filter_nr);
			break;
		} else {
			filter_nr += register_demand;
		}

		if (!usage_shifted) {
			LOG_INF("No free filter bank found");
			return CAN_NO_FREE_FILTER;
		}
	} while (filter_nr < CAN_MAX_NUMBER_OF_FILTERS);

	/* set the filter init mode */
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	/* TODO fifo balancing */
	if (filter_type != bank_mode) {
		int shift_width, start_index;
		int res;
		u32_t mode_reg  = can->FM1R;
		u32_t scale_reg = can->FS1R;

		can_stm32_set_mode_scale(filter_type, &mode_reg, &scale_reg,
					 bank_nr);

		shift_width = filter_in_bank[filter_type] - filter_in_bank[bank_mode];

		filter_index_new = can_calc_filter_index(filter_nr, mode_reg,
							 scale_reg);

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
				filter_nr = CAN_NO_FREE_FILTER;
				goto done;
			}
		}

		can->FM1R = mode_reg;
		can->FS1R = scale_reg;
	} else {
		filter_index_new = can_calc_filter_index(filter_nr, can->FM1R,
							 can->FS1R);
		if (filter_index_new >= CAN_MAX_NUMBER_OF_FILTERS) {
			filter_nr = CAN_NO_FREE_FILTER;
			goto done;
		}
	}

	can_stm32_set_filter_bank(filter_nr, &can->sFilterRegister[bank_nr],
				  filter_type, id, mask);
done:
	can->FA1R |= bank_bit;
	can->FMR &= ~(CAN_FMR_FINIT);
	LOG_DBG("Filter set! Filter number: %d (index %d)",
		    filter_nr, filter_index_new);
	*filter_index = filter_index_new;
	return filter_nr;
}

static inline int can_stm32_attach(struct device *dev, can_rx_callback_t cb,
				   void *cb_arg,
				   const struct zcan_filter *filter)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	int filter_index = 0;
	int filter_nr;

	filter_nr = can_stm32_set_filter(filter, data, can, &filter_index);
	if (filter_nr != CAN_NO_FREE_FILTER) {
		data->rx_cb[filter_index] = cb;
		data->cb_arg[filter_index] = cb_arg;
	}

	return filter_nr;
}

int can_stm32_attach_isr(struct device *dev, can_rx_callback_t isr,
			 void *cb_arg,
			 const struct zcan_filter *filter)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	int filter_nr;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	filter_nr = can_stm32_attach(dev, isr, cb_arg, filter);
	k_mutex_unlock(&data->inst_mutex);
	return filter_nr;
}

void can_stm32_detach(struct device *dev, int filter_nr)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	int bank_nr;
	int filter_index;
	u32_t bank_bit;
	u32_t mode_reg;
	u32_t scale_reg;
	enum can_filter_type type;
	u32_t reset_mask;

	__ASSERT_NO_MSG(filter_nr >= 0 && filter_nr < CAN_MAX_NUMBER_OF_FILTERS);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	bank_nr = filter_nr / 4;
	bank_bit = (1U << bank_nr);
	mode_reg  = can->FM1R;
	scale_reg = can->FS1R;

	filter_index = can_calc_filter_index(filter_nr, mode_reg, scale_reg);
	type = can_stm32_get_filter_type(bank_nr, mode_reg, scale_reg);

	LOG_DBG("Detatch filter number %d (index %d), type %d", filter_nr,
		    filter_index,
		    type);

	reset_mask = ((1 << (reg_demand[type])) - 1) << filter_nr;
	data->filter_usage |= reset_mask;
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	can_stm32_set_filter_bank(filter_nr, &can->sFilterRegister[bank_nr],
				  type, 0, 0xFFFFFFFF);

	if (!CAN_BANK_IS_EMPTY(data->filter_usage, bank_nr)) {
		can->FA1R |= bank_bit;
	} else {
		LOG_DBG("Bank number %d is empty -> deakivate", bank_nr);
	}

	can->FMR &= ~(CAN_FMR_FINIT);
	data->rx_cb[filter_index] = NULL;
	data->cb_arg[filter_index] = NULL;

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

#ifdef CONFIG_CAN_1

static void config_can_1_irq(CAN_TypeDef *can);

static const struct can_stm32_config can_stm32_cfg_1 = {
	.can = (CAN_TypeDef *)DT_CAN_1_BASE_ADDRESS,
	.bus_speed = DT_CAN_1_BUS_SPEED,
	.sjw = DT_CAN_1_SJW,
	.prop_ts1 = DT_CAN_1_PROP_SEG + DT_CAN_1_PHASE_SEG1,
	.ts2 = DT_CAN_1_PHASE_SEG2,
	.pclken = {
		.enr = DT_CAN_1_CLOCK_BITS,
		.bus = DT_CAN_1_CLOCK_BUS,
	},
	.config_irq = config_can_1_irq
};

static struct can_stm32_data can_stm32_dev_data_1;

DEVICE_AND_API_INIT(can_stm32_1, DT_CAN_1_NAME, &can_stm32_init,
		    &can_stm32_dev_data_1, &can_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_1_irq(CAN_TypeDef *can)
{
	LOG_DBG("Enable CAN1 IRQ");
#ifdef CONFIG_SOC_SERIES_STM32F0X
	IRQ_CONNECT(DT_CAN_1_IRQ, DT_CAN_1_IRQ_PRIORITY, can_stm32_isr,
		    DEVICE_GET(can_stm32_1), 0);
	irq_enable(DT_CAN_1_IRQ);
#else
	IRQ_CONNECT(DT_CAN_1_IRQ_RX0, DT_CAN_1_IRQ_PRIORITY,
		    can_stm32_rx_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(DT_CAN_1_IRQ_RX0);

	IRQ_CONNECT(DT_CAN_1_IRQ_TX, DT_CAN_1_IRQ_PRIORITY,
		    can_stm32_tx_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(DT_CAN_1_IRQ_TX);

	IRQ_CONNECT(DT_CAN_1_IRQ_SCE, DT_CAN_1_IRQ_PRIORITY,
		    can_stm32_state_change_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(DT_CAN_1_IRQ_SCE);
#endif
	can->IER |= CAN_IER_TMEIE | CAN_IER_ERRIE | CAN_IER_FMPIE0 |
		    CAN_IER_FMPIE1 | CAN_IER_BOFIE;
}

#if defined(CONFIG_NET_SOCKETS_CAN)

#include "socket_can_generic.h"

static int socket_can_init_1(struct device *dev)
{
	struct device *can_dev = DEVICE_GET(can_stm32_1);
	struct socket_can_context *socket_context = dev->driver_data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->config->name, can_dev, can_dev->config->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	socket_context->rx_tid =
		k_thread_create(&socket_context->rx_thread_data,
				rx_thread_stack,
				K_THREAD_STACK_SIZEOF(rx_thread_stack),
				rx_thread, socket_context, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

NET_DEVICE_INIT(socket_can_stm32_1, SOCKET_CAN_NAME_1, socket_can_init_1,
		&socket_can_context_1, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&socket_can_api,
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */

#endif /*CONFIG_CAN_1*/
