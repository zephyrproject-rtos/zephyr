/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <clock_control/stm32_clock_control.h>
#include <clock_control.h>
#include <misc/util.h>
#include <string.h>
#include <kernel.h>
#include <board.h>
#include <errno.h>
#include <stdbool.h>
#include "stm32_can.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CAN_LEVEL
#include <logging/sys_log.h>

static void can_stm32_signal_tx_complete(struct can_mailbox *mb)
{
	if (mb->tx_callback) {
		mb->tx_callback(mb->error_flags);
	} else  {
		k_sem_give(&mb->tx_int_sem);
	}
}

static inline void can_stm32_get_msg_fifo(CAN_FIFOMailBox_TypeDef *mbox,
				    struct can_msg *msg)
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
}

static inline
void can_stm32_rx_isr_handler(CAN_TypeDef *can, struct can_stm32_data *data)
{
	CAN_FIFOMailBox_TypeDef *mbox;
	int filter_match_index;
	struct can_msg msg;

	while (can->RF0R & CAN_RF0R_FMP0) {
		mbox = &can->sFIFOMailBox[0];
		filter_match_index = ((mbox->RDTR & CAN_RDT0R_FMI)
					   >> CAN_RDT0R_FMI_Pos);

		if (filter_match_index >= CONFIG_CAN_MAX_FILTER) {
			break;
		}

		SYS_LOG_DBG("Message on filter index %d", filter_match_index);
		can_stm32_get_msg_fifo(mbox, &msg);

		if (data->rx_response[filter_match_index]) {
			if (data->response_type & (1ULL << filter_match_index)) {
				struct k_msgq *msg_q =
					data->rx_response[filter_match_index];

				k_msgq_put(msg_q, &msg, K_NO_WAIT);
			} else {
				can_rx_callback_t callback =
					data->rx_response[filter_match_index];
				callback(&msg);
			}
		}

		/* Release message */
		can->RF0R |= CAN_RF0R_RFOM0;
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

#endif

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
	ARG_UNUSED(hcan);
}

int can_stm32_runtime_configure(struct device *dev, enum can_mode mode,
				u32_t bitrate)
{
	CAN_HandleTypeDef hcan;
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	CAN_TypeDef *can = cfg->can;
	struct device *clock;
	u32_t clock_rate;
	u32_t prescaler;
	u32_t hal_mode;
	int hal_ret;
	u32_t bs1;
	u32_t bs2;
	u32_t swj;

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clock);
	hcan.Instance = can;

	clock_control_get_rate(clock, (clock_control_subsys_t *) &cfg->pclken,
			       &clock_rate);
	if (!bitrate) {
		bitrate = cfg->bus_speed;
	}

	prescaler = clock_rate / (BIT_SEG_LENGTH(cfg) * bitrate);
	if (prescaler == 0 || prescaler > 1024) {
		SYS_LOG_ERR("HAL_CAN_Init failed: prescaler > max (%d > 1024)",
					prescaler);
		return -EINVAL;
	}

	if (clock_rate % (BIT_SEG_LENGTH(cfg) * bitrate)) {
		SYS_LOG_ERR("Prescaler is not a natural number! "
			    "prescaler = clock_rate / ((PROP_SEG1 + SEG2 + 1)"
			    " * bus_speed); "
			    "prescaler = %d / ((%d + %d + 1) * %d)",
			    clock_rate,
			    cfg->prop_bs1,
			    cfg->bs2,
			    bitrate);
	}

	__ASSERT(cfg->swj <= 0x03,      "SWJ maximum is 3");
	__ASSERT(cfg->prop_bs1 <= 0x0F, "PROP_BS1 maximum is 15");
	__ASSERT(cfg->bs2 <= 0x07,      "BS2 maximum is 7");

	bs1 = ((cfg->prop_bs1 & 0x0F) - 1) << CAN_BTR_TS1_Pos;
	bs2 = ((cfg->bs2      & 0x07) - 1) << CAN_BTR_TS2_Pos;
	swj = ((cfg->swj      & 0x07) - 1) << CAN_BTR_SJW_Pos;

	hal_mode =  mode == CAN_NORMAL_MODE   ? CAN_MODE_NORMAL   :
		    mode == CAN_LOOPBACK_MODE ? CAN_MODE_LOOPBACK :
		    mode == CAN_SILENT_MODE   ? CAN_MODE_SILENT   :
						CAN_MODE_SILENT_LOOPBACK;

	hcan.Init.TTCM = DISABLE;
	hcan.Init.ABOM = DISABLE;
	hcan.Init.AWUM = DISABLE;
	hcan.Init.NART = DISABLE;
	hcan.Init.RFLM = DISABLE;
	hcan.Init.TXFP = DISABLE;
	hcan.Init.Mode = hal_mode;
	hcan.Init.SJW  = swj;
	hcan.Init.BS1  = bs1;
	hcan.Init.BS2  = bs2;
	hcan.Init.Prescaler = prescaler;

	hcan.State = HAL_CAN_STATE_RESET;

	hal_ret = HAL_CAN_Init(&hcan);
	if (hal_ret != HAL_OK) {
		SYS_LOG_ERR("HAL_CAN_Init failed: %d", hal_ret);
		return -EIO;
	}

	SYS_LOG_DBG("Runtime configure of %s done", dev->config->name);
	return 0;
}

static int can_stm32_init(struct device *dev)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	struct device *clock;
	int ret;

	k_mutex_init(&data->tx_mutex);
	k_mutex_init(&data->set_filter_mutex);
	k_sem_init(&data->tx_int_sem, 0, 1);
	k_sem_init(&data->mb0.tx_int_sem, 0, 1);
	k_sem_init(&data->mb1.tx_int_sem, 0, 1);
	k_sem_init(&data->mb2.tx_int_sem, 0, 1);
	data->mb0.tx_callback = NULL;
	data->mb1.tx_callback = NULL;
	data->mb2.tx_callback = NULL;

	data->filter_usage = (1ULL << CAN_MAX_NUMBER_OF_FILTES) - 1ULL;
	(void)memset(data->rx_response, 0,
		     sizeof(void *) * CONFIG_CAN_MAX_FILTER);
	data->response_type = 0;

	clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clock);

	ret = clock_control_on(clock, (clock_control_subsys_t *) &cfg->pclken);
	if (ret != 0) {
		SYS_LOG_ERR("HAL_CAN_Init clock control on failed: %d", ret);
		return -EIO;
	}

	ret = can_stm32_runtime_configure(dev, CAN_NORMAL_MODE, 0);
	if (ret) {
		return ret;
	}

	cfg->config_irq(can);
	can->IER |= CAN_IT_TME;
	SYS_LOG_INF("Init of %s done", dev->config->name);
	return 0;
}

int can_stm32_send(struct device *dev, struct can_msg *msg, s32_t timeout,
		   can_tx_callback_t callback)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	u32_t transmit_status_register = can->TSR;
	CAN_TxMailBox_TypeDef *mailbox = NULL;
	struct k_mutex *tx_mutex = &data->tx_mutex;
	struct can_mailbox *mb = NULL;

	SYS_LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , msg->dlc, dev->config->name
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
				      msg->std_id :  msg->ext_id
		    , msg->id_type == CAN_STANDARD_IDENTIFIER ?
		    "standard" : "extended"
		    , msg->rtr == CAN_DATAFRAME ? "no" : "yes");

	__ASSERT(msg->dlc == 0 || msg->data != NULL, "Dataptr is null");
	__ASSERT(msg->dlc <= CAN_MAX_DLC, "DLC > 8");

	if (can->ESR & CAN_ESR_BOFF) {
		return CAN_TX_BUS_OFF;
	}

	k_mutex_lock(tx_mutex, K_FOREVER);
	while (!(transmit_status_register & CAN_TSR_TME)) {
		k_mutex_unlock(tx_mutex);
		SYS_LOG_DBG("Transmit buffer full. Wait with timeout (%dms)",
			    timeout);
		if (k_sem_take(&data->tx_int_sem, timeout)) {
			return CAN_TIMEOUT;
		}

		k_mutex_lock(tx_mutex, K_FOREVER);
		transmit_status_register = can->TSR;
	}

	if (transmit_status_register & CAN_TSR_TME0) {
		SYS_LOG_DBG("Using mailbox 0");
		mailbox = &can->sTxMailBox[CAN_TXMAILBOX_0];
		mb = &(data->mb0);
	} else if (transmit_status_register & CAN_TSR_TME1) {
		SYS_LOG_DBG("Using mailbox 1");
		mailbox = &can->sTxMailBox[CAN_TXMAILBOX_1];
		mb = &data->mb1;
	} else if (transmit_status_register & CAN_TSR_TME2) {
		SYS_LOG_DBG("Using mailbox 2");
		mailbox = &can->sTxMailBox[CAN_TXMAILBOX_2];
		mb = &data->mb2;
	}

	mb->tx_callback = callback;
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
	k_mutex_unlock(tx_mutex);

	if (callback == NULL) {
		k_sem_take(&mb->tx_int_sem, K_FOREVER);
		return mb->error_flags;
	}

	return 0;
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

		if ((start + count) >= CONFIG_CAN_MAX_FILTER ||
		    arr[CONFIG_CAN_MAX_FILTER - 1 - count] != NULL) {
			return CAN_NO_FREE_FILTER;
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

static inline void can_stm32_shift_bits(u64_t *bits, int start, int count)
{
	u64_t mask_right = (UINT64_MAX >> start);

	if (count > 0) {
		*bits = (*bits & ~mask_right) | ((*bits & mask_right) << count);
	} else if (count < 0) {
		u64_t mask_left;

		count = -count;
		mask_left  = ~(UINT64_MAX >> (start - count));
		*bits = (*bits & mask_left) | ((*bits & mask_right) >> count);
	}
}

static int can_calc_filter_index(int filter_nr, u32_t mode_reg, u32_t scale_reg)
{
	int filter_bank = filter_nr / 4;
	int cnt = 0;
	u32_t mode_masked;
	u32_t scale_masked;

	/*count filters in the banks before */
	for (int i = 0; i < filter_bank; i++) {
		mode_masked  = mode_reg & (1U << i);
		scale_masked = scale_reg & (1U << i);
		cnt += !scale_masked &&  mode_masked  ? 4 :
			scale_masked && !mode_masked  ? 1 :
							2;
	}

	/* plus the filters in the same bank */
	mode_masked  = mode_reg & (1U << filter_bank);
	scale_masked = scale_reg & (1U << filter_bank);
	cnt += (!scale_masked && mode_masked) ? filter_nr & 0x03 :
					       (filter_nr & 0x03) >> 1;
	return cnt;
}

enum can_filter_type can_stm32_get_filter_type(u32_t bank_bit, u32_t mode_reg,
					       u32_t scale_reg)
{
	u32_t mode_masked  = mode_reg  & bank_bit;
	u32_t scale_masked = scale_reg & bank_bit;
	enum can_filter_type type =
		!scale_masked &&  mode_masked ? CAN_FILTER_STANDARD :
		!scale_masked && !mode_masked ? CAN_FILTER_STANDARD_MASKED :
		scale_masked &&  mode_masked ? CAN_FILTER_EXTENDED :
		CAN_FILTER_EXTENDED_MASKED;

	return type;
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

static inline
int can_stm32_calc_shift_width(enum can_filter_type new_filter_type,
			       enum can_filter_type old_filter_type)
{
	switch (new_filter_type) {
	case CAN_FILTER_STANDARD:
		return old_filter_type == CAN_FILTER_EXTENDED_MASKED ? 3 : 1;
	case CAN_FILTER_STANDARD_MASKED:
		return old_filter_type == CAN_FILTER_STANDARD        ? -2 :
		       old_filter_type == CAN_FILTER_EXTENDED_MASKED ?  1 :
									0;
	case CAN_FILTER_EXTENDED:
		return old_filter_type == CAN_FILTER_STANDARD        ? -2 :
		       old_filter_type == CAN_FILTER_EXTENDED_MASKED ?  1 :
									0;
	case CAN_FILTER_EXTENDED_MASKED:
		return old_filter_type == CAN_FILTER_STANDARD ? -3 : -1;
	}

	return 0;
}

static inline void can_stm32_set_mode_scale(enum can_filter_type filter_type,
					    u32_t *mode_reg, u32_t *scale_reg,
					    u32_t bank_bit)
{
	switch (filter_type) {
	case CAN_FILTER_STANDARD:
		*mode_reg  |=  bank_bit;
		*scale_reg &= ~bank_bit;
		break;
	case CAN_FILTER_STANDARD_MASKED:
		*mode_reg  &= ~bank_bit;
		*scale_reg &= ~bank_bit;
		break;
	case CAN_FILTER_EXTENDED:
		*mode_reg  |= bank_bit;
		*scale_reg |= bank_bit;
		break;
	case CAN_FILTER_EXTENDED_MASKED:
		*mode_reg  &= ~bank_bit;
		*scale_reg |= bank_bit;
		break;
	}
}

static inline int can_stm32_set_filter(const struct can_filter *filter,
				       struct can_stm32_data *device_data,
				       CAN_TypeDef *can,
				       int *filter_index)
{
	u32_t mask = 0;
	u32_t id = 0;
	int filter_nr = 0;
	int filter_index_tmp = CAN_NO_FREE_FILTER;
	int bank_nr;
	u32_t bank_bit;
	int register_demand;
	enum can_filter_type filter_type;
	enum can_filter_type bank_mode;

	if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
		id = (filter->std_id << CAN_FIRX_STD_ID_POS)
		     | (filter->rtr << CAN_FIRX_STD_RTR_POS);

		if (filter->std_id_mask == CAN_STD_ID_MASK && filter->rtr_mask) {
			filter_type = CAN_FILTER_STANDARD;
			register_demand = 1;
		} else {
			filter_type = CAN_FILTER_STANDARD_MASKED;
			register_demand = 2;
			mask = (filter->std_id_mask << CAN_FIRX_STD_ID_POS)
			       | (filter->rtr_mask << CAN_FIRX_STD_RTR_POS)
			       | (1U << CAN_FIRX_STD_IDE_POS);
		}

	} else {
		id = (filter->ext_id << CAN_FIRX_EXT_EXT_ID_POS)
		     | (filter->rtr << CAN_FIRX_EXT_RTR_POS)
		     | (1U << CAN_FIRX_EXT_IDE_POS);

		if (filter->ext_id_mask == CAN_EXT_ID_MASK && filter->rtr_mask) {
			filter_type = CAN_FILTER_EXTENDED;
			register_demand = 2;
		} else {
			filter_type = CAN_FILTER_EXTENDED_MASKED;
			register_demand = 4;
			mask = (filter->ext_id_mask << CAN_FIRX_EXT_EXT_ID_POS)
			       | (filter->rtr_mask << CAN_FIRX_EXT_RTR_POS)
			       | (1U << CAN_FIRX_EXT_IDE_POS);
		}
	}

	SYS_LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->ext_id,
		    filter->ext_id_mask);
	SYS_LOG_DBG("Filter type: %s ID %s mask (%d)",
		    (filter_type == CAN_FILTER_STANDARD ||
		     filter_type == CAN_FILTER_STANDARD_MASKED) ?
		    "standard" : "extended",
		    (filter_type == CAN_FILTER_STANDARD_MASKED ||
		     filter_type == CAN_FILTER_EXTENDED_MASKED) ?
		    "with" : "without",
		    filter_type);

	do {
		u64_t usage_shifted = (device_data->filter_usage >> filter_nr);
		u8_t usage_demand_mask = (1U << register_demand) - 1;
		bool bank_is_empty;

		bank_nr = filter_nr / 4;
		bank_bit = (1U << bank_nr);
		bank_mode = can_stm32_get_filter_type(bank_bit, can->FM1R,
						      can->FS1R);
		bank_is_empty = CAN_BANK_IS_EMPTY(device_data->filter_usage,
						  bank_nr);

		if ((usage_shifted & usage_demand_mask) == usage_demand_mask) {
			if (bank_mode == filter_type || bank_is_empty) {
				device_data->filter_usage &=
				       ~((u64_t)usage_demand_mask << filter_nr);
				break;
			} else {
				/* Filter Bank has unsuitable configuration */
				filter_nr = (bank_nr + 1) * 4;
			}
		} else {
			filter_nr += register_demand;
		}

		if (!usage_shifted) {
			SYS_LOG_INF("No free filter bank found");
			return CAN_NO_FREE_FILTER;
		}
	} while (filter_nr < CAN_MAX_NUMBER_OF_FILTES);

	/* set the filter init mode */
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	/* TODO fifo balancing */
	if (filter_type != bank_mode) {
		int shift_width;
		int res;
		u32_t mode_reg  = can->FM1R;
		u32_t scale_reg = can->FS1R;

		can_stm32_set_mode_scale(filter_type, &mode_reg, &scale_reg,
					 bank_bit);

		shift_width = can_stm32_calc_shift_width(filter_type,
							 bank_mode);

		filter_index_tmp = can_calc_filter_index(filter_nr, mode_reg,
							 scale_reg);

		res = can_stm32_shift_arr(device_data->rx_response,
					  filter_index_tmp + 1, shift_width);

		if (filter_index_tmp >= CAN_MAX_NUMBER_OF_FILTES || res) {
			SYS_LOG_INF("No space for a new filter!");
			filter_nr = CAN_NO_FREE_FILTER;
			goto done;
		}

		can_stm32_shift_bits(&device_data->response_type,
				     filter_index_tmp + 1, shift_width);
		can->FM1R = mode_reg;
		can->FS1R = scale_reg;
	} else {
		filter_index_tmp = can_calc_filter_index(filter_nr, can->FM1R,
							 can->FS1R);
		if (filter_index_tmp >= CAN_MAX_NUMBER_OF_FILTES) {
			filter_nr = CAN_NO_FREE_FILTER;
			goto done;
		}
	}

	can_stm32_set_filter_bank(filter_nr, &can->sFilterRegister[bank_nr],
				  filter_type, id, mask);
done:
	can->FA1R |= bank_bit;
	can->FMR &= ~(CAN_FMR_FINIT);
	SYS_LOG_DBG("Filter set! Filter number: %d (index %d)",
		    filter_nr, filter_index_tmp);
	*filter_index = filter_index_tmp;
	return filter_nr;
}


static inline int can_stm32_attach(struct device *dev, void *response_ptr,
				   const struct can_filter *filter,
				   int *filter_index)
{
	const struct can_stm32_config *cfg = DEV_CFG(dev);
	struct can_stm32_data *data = DEV_DATA(dev);
	CAN_TypeDef *can = cfg->can;
	int filter_index_tmp = 0;
	int filter_nr;

	filter_nr = can_stm32_set_filter(filter, data, can, &filter_index_tmp);
	if (filter_nr != CAN_NO_FREE_FILTER) {
		data->rx_response[filter_index_tmp] = response_ptr;
	}

	*filter_index = filter_index_tmp;
	return filter_nr;
}

int can_stm32_attach_msgq(struct device *dev, struct k_msgq *msgq,
			  const struct can_filter *filter)
{
	int filter_nr;
	int filter_index;
	struct can_stm32_data *data = DEV_DATA(dev);

	k_mutex_lock(&data->set_filter_mutex, K_FOREVER);
	filter_nr = can_stm32_attach(dev, msgq, filter, &filter_index);
	data->response_type |= (1ULL << filter_index);
	k_mutex_unlock(&data->set_filter_mutex);
	return filter_nr;
}

int can_stm32_attach_isr(struct device *dev, can_rx_callback_t isr,
			 const struct can_filter *filter)
{
	struct can_stm32_data *data = DEV_DATA(dev);
	int filter_nr;
	int filter_index;

	k_mutex_lock(&data->set_filter_mutex, K_FOREVER);
	filter_nr = can_stm32_attach(dev, isr, filter, &filter_index);
	data->response_type &= ~(1ULL << filter_index);
	k_mutex_unlock(&data->set_filter_mutex);
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

	__ASSERT_NO_MSG(filter_nr >= 0 && filter_nr < CAN_MAX_NUMBER_OF_FILTES);

	k_mutex_lock(&data->set_filter_mutex, K_FOREVER);

	bank_nr = filter_nr / 4;
	bank_bit = (1U << bank_nr);
	mode_reg  = can->FM1R;
	scale_reg = can->FS1R;

	filter_index = can_calc_filter_index(filter_nr, mode_reg, scale_reg);
	type = can_stm32_get_filter_type(bank_bit, mode_reg, scale_reg);

	SYS_LOG_DBG("Detatch filter number %d (index %d), type %d", filter_nr,
		    filter_index,
		    type);

	reset_mask = (type == CAN_FILTER_STANDARD)         ? 0x01 :
		     (type == CAN_FILTER_EXTENDED_MASKED)  ? 0x0F :
							     0x03;
	reset_mask = reset_mask << filter_nr;

	data->filter_usage |= reset_mask;
	can->FMR |= CAN_FMR_FINIT;
	can->FA1R &= ~bank_bit;

	can_stm32_set_filter_bank(filter_nr, &can->sFilterRegister[bank_nr],
				  type, 0, 0xFFFFFFFF);

	if (!CAN_BANK_IS_EMPTY(data->filter_usage, bank_nr)) {
		can->FA1R |= bank_bit;
	} else {
		SYS_LOG_DBG("Bank number %d is empty -> deakivate", bank_nr);
	}

	can->FMR &= ~(CAN_FMR_FINIT);
	data->rx_response[filter_index] = NULL;

	k_mutex_unlock(&data->set_filter_mutex);
}

static const struct can_driver_api can_api_funcs = {
	.configure = can_stm32_runtime_configure,
	.send = can_stm32_send,
	.attach_msgq = can_stm32_attach_msgq,
	.attach_isr = can_stm32_attach_isr,
	.detach = can_stm32_detach
};

#ifdef CONFIG_CAN_1

static void config_can_1_irq(CAN_TypeDef *can);

static const struct can_stm32_config can_stm32_cfg_1 = {
	.can = (CAN_TypeDef *)CONFIG_CAN_1_BASE_ADDRESS,
	.bus_speed = CONFIG_CAN_1_BUS_SPEED,
	.swj = CONFIG_CAN_1_SJW,
	.prop_bs1 = CONFIG_CAN_1_PROP_SEG_PHASE_SEG1,
	.bs2 = CONFIG_CAN_1_PHASE_SEG2,
	.pclken = {
		.enr = CONFIG_CAN_1_CLOCK_BITS,
		.bus = CONFIG_CAN_1_CLOCK_BUS,
	},
	.config_irq = config_can_1_irq
};

static struct can_stm32_data can_stm32_dev_data_1;

DEVICE_AND_API_INIT(can_stm32_1, CONFIG_CAN_1_NAME, &can_stm32_init,
		    &can_stm32_dev_data_1, &can_stm32_cfg_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

static void config_can_1_irq(CAN_TypeDef *can)
{
	SYS_LOG_DBG("Enable CAN1 IRQ");
#ifdef CONFIG_SOC_SERIES_STM32F0X
	IRQ_CONNECT(CONFIG_CAN_1_IRQ, CONFIG_CAN_1_IRQ_PRIORITY, can_stm32_isr,
		    DEVICE_GET(can_stm32_1), 0);
	irq_enable(CONFIG_CAN_1_IRQ);
#else
	IRQ_CONNECT(CONFIG_CAN_1_IRQ_RX0, CONFIG_CAN_1_IRQ_PRIORITY,
		    can_stm32_rx_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(CONFIG_CAN_1_IRQ_RX0);

	IRQ_CONNECT(CONFIG_CAN_1_IRQ_TX, CONFIG_CAN_1_IRQ_PRIORITY,
		    can_stm32_tx_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(CONFIG_CAN_1_IRQ_TX);

	IRQ_CONNECT(CONFIG_CAN_1_IRQ_SCE, CONFIG_CAN_1_IRQ_PRIORITY,
		    can_stm32_tx_isr, DEVICE_GET(can_stm32_1), 0);
	irq_enable(CONFIG_CAN_1_IRQ_SCE);
#endif
	can->IER |= CAN_IT_TME | CAN_IT_ERR | CAN_IT_FMP0 | CAN_IT_FMP1;
}

#endif /*CONFIG_CAN_1*/
