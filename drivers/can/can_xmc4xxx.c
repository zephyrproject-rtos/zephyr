/*
 * Copyright (c) 2023 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_can_node

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/bitarray.h>

#include <soc.h>
#include <xmc_can.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_xmc4xxx, CONFIG_CAN_LOG_LEVEL);

#define CAN_XMC4XXX_MULTICAN_NODE DT_INST(0, infineon_xmc4xxx_can)

#define CAN_XMC4XXX_NUM_MESSAGE_OBJECTS DT_PROP(CAN_XMC4XXX_MULTICAN_NODE, message_objects)
#define CAN_XMC4XXX_CLOCK_PRESCALER DT_PROP(CAN_XMC4XXX_MULTICAN_NODE, clock_prescaler)

static CAN_GLOBAL_TypeDef *const can_xmc4xxx_global_reg =
	(CAN_GLOBAL_TypeDef *)DT_REG_ADDR(CAN_XMC4XXX_MULTICAN_NODE);

static bool can_xmc4xxx_global_init;
static uint32_t can_xmc4xxx_clock_frequency;

SYS_BITARRAY_DEFINE_STATIC(mo_usage_bitarray, CAN_XMC4XXX_NUM_MESSAGE_OBJECTS);
static int can_xmc4xxx_num_free_mo = CAN_XMC4XXX_NUM_MESSAGE_OBJECTS;

#define CAN_XMC4XXX_IRQ_MIN 76
#define CAN_XMC4XXX_MAX_DLC 8

#define CAN_XMC4XXX_REG_TO_NODE_IND(reg) (((uint32_t)(reg) - (uint32_t)CAN_NODE0_BASE) / 0x100)

struct can_xmc4xxx_tx_callback {
	can_tx_callback_t function;
	void *user_data;
};

struct can_xmc4xxx_rx_callback {
	can_rx_callback_t function;
	void *user_data;
};

struct can_xmc4xxx_rx_fifo {
	CAN_MO_TypeDef *base;
	CAN_MO_TypeDef *top;
	CAN_MO_TypeDef *tail;
	CAN_MO_TypeDef *head;
};

struct can_xmc4xxx_data {
	struct can_driver_data common;

	enum can_state state;
	struct k_mutex mutex;

	struct k_sem tx_sem;
	struct can_xmc4xxx_tx_callback tx_callbacks[CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE];

	uint32_t filter_usage;
	struct can_xmc4xxx_rx_callback rx_callbacks[CONFIG_CAN_MAX_FILTER];
	struct can_xmc4xxx_rx_fifo rx_fifos[CONFIG_CAN_MAX_FILTER];
#if defined(CONFIG_CAN_ACCEPT_RTR)
	struct can_xmc4xxx_rx_fifo rtr_fifos[CONFIG_CAN_MAX_FILTER];
#endif

	CAN_MO_TypeDef *tx_mo[CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE];
};

struct can_xmc4xxx_config {
	struct can_driver_config common;

	CAN_NODE_TypeDef *can;
	bool clock_div8;

	uint8_t service_request;
	void (*irq_config_func)(void);

	uint8_t input_src;
	const struct pinctrl_dev_config *pcfg;
};

static int can_xmc4xxx_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;

	if (dev_data->common.started) {
		return -EBUSY;
	}

	if ((mode & (CAN_MODE_3_SAMPLES | CAN_MODE_ONE_SHOT |
		     CAN_MODE_LOOPBACK | CAN_MODE_FD)) != 0) {
		return -ENOTSUP;
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		XMC_CAN_NODE_SetAnalyzerMode(dev_cfg->can);
	} else {
		XMC_CAN_NODE_ReSetAnalyzerMode(dev_cfg->can);
	}

	dev_data->common.mode = mode;

	return 0;
}

static int can_xmc4xxx_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	uint32_t reg;

	if (!timing) {
		return -EINVAL;
	}

	if (dev_data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = FIELD_PREP(CAN_NODE_NBTR_DIV8_Msk, dev_cfg->clock_div8);
	reg |= FIELD_PREP(CAN_NODE_NBTR_BRP_Msk, timing->prescaler - 1);
	reg |= FIELD_PREP(CAN_NODE_NBTR_TSEG1_Msk, timing->prop_seg + timing->phase_seg1 - 1);
	reg |= FIELD_PREP(CAN_NODE_NBTR_TSEG2_Msk, timing->phase_seg2 - 1);
	reg |= FIELD_PREP(CAN_NODE_NBTR_SJW_Msk, timing->sjw - 1);

	dev_cfg->can->NBTR = reg;

	k_mutex_unlock(&dev_data->mutex);

	return 0;
}

static int can_xmc4xxx_send(const struct device *dev, const struct can_frame *msg,
			    k_timeout_t timeout, can_tx_callback_t callback, void *callback_arg)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	uint8_t mailbox_idx;
	struct can_xmc4xxx_tx_callback *callbacks = &dev_data->tx_callbacks[0];
	CAN_MO_TypeDef *mo;
	unsigned int key;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s %s %s", can_dlc_to_bytes(msg->dlc),
		msg->id, msg->flags & CAN_FRAME_IDE ? "extended" : "standard",
		msg->flags & CAN_FRAME_RTR ? "RTR" : "",
		msg->flags & CAN_FRAME_FDF ? "FD frame" : "",
		msg->flags & CAN_FRAME_BRS ? "BRS" : "");

	if (msg->dlc > CAN_XMC4XXX_MAX_DLC) {
		return -EINVAL;
	}

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (dev_data->state == CAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	if ((msg->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
		return -ENOTSUP;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (mailbox_idx = 0; mailbox_idx < CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE; mailbox_idx++) {
		if (callbacks[mailbox_idx].function == NULL) {
			break;
		}
	}

	__ASSERT_NO_MSG(mailbox_idx < CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE);

	key = irq_lock();
	/* critical section in case can_xmc4xxx_reset_tx_fifos() called in isr  */
	/* so that callback function and callback_arg are consistent */
	callbacks[mailbox_idx].function = callback;
	callbacks[mailbox_idx].user_data = callback_arg;
	irq_unlock(key);

	mo = dev_data->tx_mo[mailbox_idx];
	mo->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;

	if ((msg->flags & CAN_FRAME_IDE) != 0) {
		/* MOAR - message object arbitration register */
		mo->MOAR = FIELD_PREP(CAN_MO_MOAR_PRI_Msk, 1) |
			   FIELD_PREP(CAN_MO_MOAR_ID_Msk, msg->id) | CAN_MO_MOAR_IDE_Msk;
	} else {
		mo->MOAR = FIELD_PREP(CAN_MO_MOAR_PRI_Msk, 1) |
			   FIELD_PREP(XMC_CAN_MO_MOAR_STDID_Msk, msg->id);
	}

	mo->MOFCR &= ~CAN_MO_MOFCR_DLC_Msk;
	mo->MOFCR |= FIELD_PREP(CAN_MO_MOFCR_DLC_Msk, msg->dlc);

	if ((msg->flags & CAN_FRAME_RTR) != 0) {
		mo->MOCTR = CAN_MO_MOCTR_RESDIR_Msk;
	} else {
		mo->MOCTR = CAN_MO_MOCTR_SETDIR_Msk;
		memcpy((void *)&mo->MODATAL, &msg->data[0], sizeof(uint32_t));
		memcpy((void *)&mo->MODATAH, &msg->data[4], sizeof(uint32_t));
	}

	mo->MOCTR = CAN_MO_MOCTR_SETTXEN0_Msk | CAN_MO_MOCTR_SETTXEN1_Msk |
		    CAN_MO_MOCTR_SETMSGVAL_Msk | CAN_MO_MOCTR_RESRXEN_Msk |
		    CAN_MO_MOCTR_RESRTSEL_Msk;
	mo->MOCTR = CAN_MO_MOCTR_SETTXRQ_Msk;

	k_mutex_unlock(&dev_data->mutex);
	return 0;
}

static CAN_MO_TypeDef *can_xmc4xxx_get_mo(uint8_t *mo_index)
{
	int i;

	for (i = 0; i < CAN_XMC4XXX_NUM_MESSAGE_OBJECTS; i++) {
		int prev_val;

		sys_bitarray_test_and_set_bit(&mo_usage_bitarray, i, &prev_val);
		if (prev_val == 0) {
			*mo_index = i;
			can_xmc4xxx_num_free_mo--;
			return &CAN_MO->MO[i];
		}
	}

	return NULL;
}

static void can_xmc4xxx_deinit_fifo(const struct device *dev, struct can_xmc4xxx_rx_fifo *fifo)
{
	CAN_MO_TypeDef *mo = fifo->base;

	while (mo != NULL) {
		int next_index;
		int index;

		/* invalidate message */
		mo->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;

		next_index = FIELD_GET(CAN_MO_MOSTAT_PNEXT_Msk, mo->MOSTAT);
		index = ((uint32_t)mo - (uint32_t)&CAN_MO->MO[0]) / sizeof(*mo);

		if ((uint32_t)mo == (uint32_t)fifo->top) {
			mo = NULL;
		} else {
			mo = &CAN_MO->MO[next_index];
		}

		/* we need to move the node back to the list of unallocated message objects, */
		/* which is list index = 0. 255 gets rolled over to 0 in the function below */
		XMC_CAN_AllocateMOtoNodeList(can_xmc4xxx_global_reg, 255, index);

		sys_bitarray_clear_bit(&mo_usage_bitarray, index);
		can_xmc4xxx_num_free_mo++;
	}
}

static int can_xmc4xxx_init_fifo(const struct device *dev, const struct can_filter *filter,
				 struct can_xmc4xxx_rx_fifo *fifo, bool is_rtr)
{
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	CAN_MO_TypeDef *mo;
	uint32_t reg;
	uint8_t mo_index = 0, base_index;

	if (can_xmc4xxx_num_free_mo < CONFIG_CAN_XMC4XXX_RX_FIFO_ITEMS) {
		return -ENOMEM;
	}

	mo = can_xmc4xxx_get_mo(&mo_index);
	__ASSERT_NO_MSG(mo != NULL);

	base_index = mo_index;
	fifo->base = mo;
	fifo->tail = mo;

	XMC_CAN_AllocateMOtoNodeList(can_xmc4xxx_global_reg,
				     CAN_XMC4XXX_REG_TO_NODE_IND(dev_cfg->can), mo_index);

	/* setup the base object - this controls the filtering for the fifo */
	mo->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;
	mo->MOAMR &= ~(CAN_MO_MOAMR_AM_Msk | CAN_MO_MOAMR_MIDE_Msk);
	mo->MOAR = 0;

	if ((filter->flags & CAN_FILTER_IDE) != 0) {
		mo->MOAMR |= FIELD_PREP(CAN_MO_MOAMR_AM_Msk, filter->mask) | CAN_MO_MOAMR_MIDE_Msk;
		mo->MOAR |= FIELD_PREP(CAN_MO_MOAR_ID_Msk, filter->id) | CAN_MO_MOAR_IDE_Msk;
	} else {
		mo->MOAMR |= FIELD_PREP(XMC_CAN_MO_MOAR_STDID_Msk, filter->mask);
		mo->MOAR |= FIELD_PREP(XMC_CAN_MO_MOAR_STDID_Msk, filter->id);
	}

	mo->MOFCR = FIELD_PREP(CAN_MO_MOFCR_MMC_Msk, 1) | CAN_MO_MOFCR_RXIE_Msk;
	if (is_rtr) {
		mo->MOFCR |= CAN_MO_MOFCR_RMM_Msk;
		mo->MOCTR = CAN_MO_MOCTR_SETDIR_Msk;
	} else {
		mo->MOCTR = CAN_MO_MOCTR_RESDIR_Msk;
	}

	/* Writing to MOCTR sets or resets message object properties */
	mo->MOCTR = CAN_MO_MOCTR_RESTXEN0_Msk | CAN_MO_MOCTR_RESTXEN1_Msk |
		    CAN_MO_MOCTR_SETMSGVAL_Msk | CAN_MO_MOCTR_SETRXEN_Msk |
		    CAN_MO_MOCTR_RESRTSEL_Msk;

	mo->MOIPR = FIELD_PREP(CAN_MO_MOIPR_RXINP_Msk, dev_cfg->service_request);

	/* setup the remaining message objects in the fifo */
	for (int i = 1; i < CONFIG_CAN_XMC4XXX_RX_FIFO_ITEMS; i++) {
		mo = can_xmc4xxx_get_mo(&mo_index);
		__ASSERT_NO_MSG(mo != NULL);

		XMC_CAN_AllocateMOtoNodeList(can_xmc4xxx_global_reg,
					     CAN_XMC4XXX_REG_TO_NODE_IND(dev_cfg->can), mo_index);

		mo->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;
		mo->MOCTR = CAN_MO_MOCTR_SETMSGVAL_Msk | CAN_MO_MOCTR_RESRXEN_Msk;

		/* all the other message objects in the fifo must point to the base object */
		mo->MOFGPR = FIELD_PREP(CAN_MO_MOFGPR_CUR_Msk, base_index);
	}

	reg = 0;
	reg |= FIELD_PREP(CAN_MO_MOFGPR_CUR_Msk, base_index);
	reg |= FIELD_PREP(CAN_MO_MOFGPR_TOP_Msk, mo_index);
	reg |= FIELD_PREP(CAN_MO_MOFGPR_BOT_Msk, base_index);
	reg |= FIELD_PREP(CAN_MO_MOFGPR_SEL_Msk, base_index);

	fifo->base->MOFGPR = reg;
	fifo->top = mo;

	return 0;
}

static int can_xmc4xxx_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				     void *user_data, const struct can_filter *filter)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	int filter_idx;

	if ((filter->flags & ~CAN_FILTER_IDE) != 0) {
		LOG_ERR("Unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (filter_idx = 0; filter_idx < CONFIG_CAN_MAX_FILTER; filter_idx++) {
		if ((BIT(filter_idx) & dev_data->filter_usage) == 0) {
			break;
		}
	}

	if (filter_idx >= CONFIG_CAN_MAX_FILTER) {
		filter_idx = -ENOSPC;
	} else {
		unsigned int key = irq_lock();
		int ret;

		ret = can_xmc4xxx_init_fifo(dev, filter, &dev_data->rx_fifos[filter_idx], false);
		if (ret < 0) {
			irq_unlock(key);
			k_mutex_unlock(&dev_data->mutex);
			return ret;
		}

#if defined(CONFIG_CAN_ACCEPT_RTR)
		ret = can_xmc4xxx_init_fifo(dev, filter, &dev_data->rtr_fifos[filter_idx], true);
		if (ret < 0) {
			can_xmc4xxx_deinit_fifo(dev, &dev_data->rx_fifos[filter_idx]);
			irq_unlock(key);
			k_mutex_unlock(&dev_data->mutex);
			return ret;
		}
#endif

		dev_data->filter_usage |= BIT(filter_idx);
		dev_data->rx_callbacks[filter_idx].function = callback;
		dev_data->rx_callbacks[filter_idx].user_data = user_data;

		irq_unlock(key);
	}

	k_mutex_unlock(&dev_data->mutex);

	return filter_idx;
}

static void can_xmc4xxx_remove_rx_filter(const struct device *dev, int filter_idx)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	unsigned int key;

	if (filter_idx < 0 || filter_idx >= CONFIG_CAN_MAX_FILTER) {
		LOG_ERR("Filter ID %d out of bounds", filter_idx);
		return;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	if ((dev_data->filter_usage & BIT(filter_idx)) == 0) {
		k_mutex_unlock(&dev_data->mutex);
		return;
	}

	key = irq_lock();
	can_xmc4xxx_deinit_fifo(dev, &dev_data->rx_fifos[filter_idx]);
#if defined(CONFIG_CAN_ACCEPT_RTR)
	can_xmc4xxx_deinit_fifo(dev, &dev_data->rtr_fifos[filter_idx]);
#endif

	dev_data->filter_usage &= ~BIT(filter_idx);
	dev_data->rx_callbacks[filter_idx].function = NULL;
	dev_data->rx_callbacks[filter_idx].user_data = NULL;
	irq_unlock(key);

	k_mutex_unlock(&dev_data->mutex);
}

static void can_xmc4xxx_set_state_change_callback(const struct device *dev,
						  can_state_change_callback_t cb, void *user_data)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	unsigned int key;

	key = irq_lock();
	/* critical section so that state_change_cb and state_change_cb_data are consistent */
	dev_data->common.state_change_cb = cb;
	dev_data->common.state_change_cb_user_data = user_data;
	irq_unlock(key);
}

static void can_xmc4xxx_get_state_from_status(const struct device *dev, enum can_state *state,
					      struct can_bus_err_cnt *err_cnt, uint32_t *status)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	uint8_t tec = XMC_CAN_NODE_GetTransmitErrorCounter(dev_cfg->can);
	uint8_t rec = XMC_CAN_NODE_GetTransmitErrorCounter(dev_cfg->can);

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = tec;
		err_cnt->rx_err_cnt = rec;
	}

	if (state == NULL) {
		return;
	}

	if (!dev_data->common.started) {
		*state = CAN_STATE_STOPPED;
		return;
	}

	if ((*status & XMC_CAN_NODE_STATUS_BUS_OFF) != 0) {
		*state = CAN_STATE_BUS_OFF;
	} else if (tec >= 128 || rec >= 128) {
		*state = CAN_STATE_ERROR_PASSIVE;
	} else if ((*status & XMC_CAN_NODE_STATUS_ERROR_WARNING_STATUS) != 0) {
		*state = CAN_STATE_ERROR_WARNING;
	} else {
		*state = CAN_STATE_ERROR_ACTIVE;
	}
}

static int can_xmc4xxx_get_state(const struct device *dev, enum can_state *state,
				 struct can_bus_err_cnt *err_cnt)
{
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	uint32_t status;

	status = XMC_CAN_NODE_GetStatus(dev_cfg->can);

	can_xmc4xxx_get_state_from_status(dev, state, err_cnt, &status);

	return 0;
}

static int can_xmc4xxx_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct can_xmc4xxx_config *dev_cfg = dev->config;

	*rate = can_xmc4xxx_clock_frequency;
	if (dev_cfg->clock_div8) {
		*rate /= 8;
	}

	return 0;
}

static int can_xmc4xxx_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

static void can_xmc4xxx_reset_tx_fifos(const struct device *dev, int status)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	struct can_xmc4xxx_tx_callback *tx_callbacks = &dev_data->tx_callbacks[0];

	LOG_DBG("All Tx message objects reset");
	for (int i = 0; i < CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE; i++) {
		can_tx_callback_t callback;
		void *user_data;

		callback = tx_callbacks[i].function;
		user_data = tx_callbacks[i].user_data;

		tx_callbacks[i].function = NULL;

		if (callback) {
			dev_data->tx_mo[i]->MOCTR = CAN_MO_MOCTR_RESMSGVAL_Msk;
			callback(dev, status, user_data);
			k_sem_give(&dev_data->tx_sem);
		}
	}
}

static void can_xmc4xxx_tx_handler(const struct device *dev)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	struct can_xmc4xxx_tx_callback *tx_callbacks = &dev_data->tx_callbacks[0];

	for (int i = 0; i < CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE; i++) {
		CAN_MO_TypeDef *mo = dev_data->tx_mo[i];

		if ((mo->MOSTAT & XMC_CAN_MO_STATUS_TX_PENDING) != 0) {
			can_tx_callback_t callback;
			void *user_data;

			mo->MOCTR = XMC_CAN_MO_RESET_STATUS_TX_PENDING;

			callback = tx_callbacks[i].function;
			user_data = tx_callbacks[i].user_data;

			tx_callbacks[i].function = NULL;

			if (callback) {
				callback(dev, 0, user_data);
				k_sem_give(&dev_data->tx_sem);
			}
		}
	}
}

static inline void can_xmc4xxx_increment_fifo_tail(struct can_xmc4xxx_rx_fifo *fifo)
{
	uint8_t next_index;

	if ((uint32_t)fifo->tail == (uint32_t)fifo->top) {
		fifo->tail = fifo->base;
		return;
	}

	next_index = FIELD_GET(CAN_MO_MOSTAT_PNEXT_Msk, fifo->tail->MOSTAT);
	fifo->tail = &CAN_MO->MO[next_index];
}

static inline bool can_xmc4xxx_is_fifo_empty(struct can_xmc4xxx_rx_fifo *fifo)
{
	if (fifo->tail->MOSTAT & XMC_CAN_MO_STATUS_RX_PENDING) {
		return false;
	}

	return true;
}

static inline void can_xmc4xxx_update_fifo_head(struct can_xmc4xxx_rx_fifo *fifo)
{
	uint32_t reg = fifo->base->MOFGPR;
	uint8_t top_index, bot_index, cur_index;
	uint8_t head_index = FIELD_GET(CAN_MO_MOFGPR_CUR_Msk, reg);

	fifo->head = &CAN_MO->MO[head_index];
	top_index = FIELD_GET(CAN_MO_MOFGPR_TOP_Msk, reg);
	bot_index = FIELD_GET(CAN_MO_MOFGPR_BOT_Msk, reg);
	cur_index = FIELD_GET(CAN_MO_MOFGPR_CUR_Msk, reg);

	LOG_DBG("Fifo: top %d, bot %d, cur %d", top_index, bot_index, cur_index);
}

static void can_xmc4xxx_rx_fifo_handler(const struct device *dev, struct can_xmc4xxx_rx_fifo *fifo,
					struct can_xmc4xxx_rx_callback *rx_callback)
{
	bool is_rtr = (fifo->base->MOSTAT & CAN_MO_MOSTAT_DIR_Msk) != 0;

	while (!can_xmc4xxx_is_fifo_empty(fifo)) {
		struct can_frame frame;
		CAN_MO_TypeDef *mo_tail = fifo->tail;

		memset(&frame, 0, sizeof(frame));

		if ((mo_tail->MOAR & CAN_MO_MOAR_IDE_Msk) != 0) {
			frame.flags |= CAN_FRAME_IDE;
			frame.id = FIELD_GET(CAN_MO_MOAR_ID_Msk, mo_tail->MOAR);
		} else {
			frame.id = FIELD_GET(XMC_CAN_MO_MOAR_STDID_Msk, mo_tail->MOAR);
		}

		frame.dlc = FIELD_GET(CAN_MO_MOFCR_DLC_Msk, mo_tail->MOFCR);

		if (!is_rtr) {
			memcpy(&frame.data[0], (void *)&mo_tail->MODATAL, sizeof(uint32_t));
			memcpy(&frame.data[4], (void *)&mo_tail->MODATAH, sizeof(uint32_t));
		} else {
			frame.flags |= CAN_FRAME_RTR;
			memset(&frame.data[0], 0, CAN_MAX_DLEN);
		}

		if (rx_callback->function != NULL) {
			rx_callback->function(dev, &frame, rx_callback->user_data);
		}

		/* reset the rx pending bit on the tail */
		mo_tail->MOCTR = XMC_CAN_MO_RESET_STATUS_RX_PENDING;
		can_xmc4xxx_increment_fifo_tail(fifo);
	}
}

static void can_xmc4xxx_rx_handler(const struct device *dev)
{
	struct can_xmc4xxx_data *dev_data = dev->data;

	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		if ((BIT(i) & dev_data->filter_usage) == 0) {
			continue;
		}

		can_xmc4xxx_update_fifo_head(&dev_data->rx_fifos[i]);
		can_xmc4xxx_rx_fifo_handler(dev, &dev_data->rx_fifos[i],
					    &dev_data->rx_callbacks[i]);
#if defined(CONFIG_CAN_ACCEPT_RTR)
		can_xmc4xxx_update_fifo_head(&dev_data->rtr_fifos[i]);
		can_xmc4xxx_rx_fifo_handler(dev, &dev_data->rtr_fifos[i],
					    &dev_data->rx_callbacks[i]);
#endif
	}
}

static void can_xmc4xxx_state_change_handler(const struct device *dev, uint32_t status)
{
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	struct can_xmc4xxx_data *dev_data = dev->data;
	enum can_state new_state;
	struct can_bus_err_cnt err_cnt;

	can_xmc4xxx_get_state_from_status(dev, &new_state, &err_cnt, &status);
	if (dev_data->state != new_state) {
		if (dev_data->common.state_change_cb) {
			dev_data->common.state_change_cb(
				dev, new_state, err_cnt,
				dev_data->common.state_change_cb_user_data);
		}

		if (dev_data->state != CAN_STATE_STOPPED && new_state == CAN_STATE_BUS_OFF) {
			/* re-enable the node after auto bus-off recovery completes */
			XMC_CAN_NODE_ResetInitBit(dev_cfg->can);
		}

		dev_data->state = new_state;

		if (dev_data->state == CAN_STATE_BUS_OFF) {
			can_xmc4xxx_reset_tx_fifos(dev, -ENETDOWN);
		}
	}
}

static void can_xmc4xxx_isr(const struct device *dev)
{
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	uint32_t status;

	status = XMC_CAN_NODE_GetStatus(dev_cfg->can);
	XMC_CAN_NODE_ClearStatus(dev_cfg->can, status);

	if ((status & XMC_CAN_NODE_STATUS_TX_OK) != 0) {
		can_xmc4xxx_tx_handler(dev);
	}

	if ((status & XMC_CAN_NODE_STATUS_RX_OK) != 0) {
		can_xmc4xxx_rx_handler(dev);
	}

	if ((status & XMC_CAN_NODE_STATUS_ALERT_WARNING) != 0) {
		/* change of bit NSRx.BOFF */
		/* change of bit NSRx.EWRN */
		can_xmc4xxx_state_change_handler(dev, status);
	}
}

static int can_xmc4xxx_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LISTENONLY;

	return 0;
}

static int can_xmc4xxx_start(const struct device *dev)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	int ret = 0;
	unsigned int key;

	if (dev_data->common.started) {
		return -EALREADY;
	}

	key = irq_lock();
	can_xmc4xxx_reset_tx_fifos(dev, -ENETDOWN);
	irq_unlock(key);

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_enable(dev_cfg->common.phy, dev_data->common.mode);
		if (ret < 0) {
			LOG_ERR("Failed to enable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	XMC_CAN_NODE_DisableConfigurationChange(dev_cfg->can);

	dev_data->common.started = true;
	XMC_CAN_NODE_ResetInitBit(dev_cfg->can);

	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

static int can_xmc4xxx_stop(const struct device *dev)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	int ret = 0;
	unsigned int key;

	if (!dev_data->common.started) {
		return -EALREADY;
	}

	key = irq_lock();
	XMC_CAN_NODE_SetInitBit(dev_cfg->can);

	XMC_CAN_NODE_EnableConfigurationChange(dev_cfg->can);

	can_xmc4xxx_reset_tx_fifos(dev, -ENETDOWN);
	dev_data->common.started = false;
	irq_unlock(key);

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_disable(dev_cfg->common.phy);
		if (ret < 0) {
			LOG_ERR("Failed to disable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	return 0;
}

static int can_xmc4xxx_init(const struct device *dev)
{
	struct can_xmc4xxx_data *dev_data = dev->data;
	const struct can_xmc4xxx_config *dev_cfg = dev->config;
	int ret;
	struct can_timing timing = {0};
	CAN_MO_TypeDef *mo;
	uint8_t mo_index = 0;

	k_sem_init(&dev_data->tx_sem, CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE,
		   CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE);
	k_mutex_init(&dev_data->mutex);

	if (!can_xmc4xxx_global_init) {
		uint32_t fdr_step;
		uint32_t clk_module;

		XMC_CAN_Enable(can_xmc4xxx_global_reg);
		XMC_CAN_SetBaudrateClockSource(can_xmc4xxx_global_reg, XMC_CAN_CANCLKSRC_FPERI);

		clk_module = XMC_CAN_GetBaudrateClockFrequency(can_xmc4xxx_global_reg);
		fdr_step = 1024 - CAN_XMC4XXX_CLOCK_PRESCALER;
		can_xmc4xxx_clock_frequency = clk_module / CAN_XMC4XXX_CLOCK_PRESCALER;

		LOG_DBG("Clock frequency %dHz\n", can_xmc4xxx_clock_frequency);

		can_xmc4xxx_global_reg->FDR &= ~(CAN_FDR_DM_Msk | CAN_FDR_STEP_Msk);
		can_xmc4xxx_global_reg->FDR |= FIELD_PREP(CAN_FDR_DM_Msk, XMC_CAN_DM_NORMAL) |
					       FIELD_PREP(CAN_FDR_STEP_Msk, fdr_step);

		can_xmc4xxx_global_init = true;
	}

	XMC_CAN_NODE_EnableConfigurationChange(dev_cfg->can);

	XMC_CAN_NODE_SetReceiveInput(dev_cfg->can, dev_cfg->input_src);

	XMC_CAN_NODE_SetInitBit(dev_cfg->can);

	XMC_CAN_NODE_SetEventNodePointer(dev_cfg->can, XMC_CAN_NODE_POINTER_EVENT_ALERT,
					 dev_cfg->service_request);

	XMC_CAN_NODE_SetEventNodePointer(dev_cfg->can, XMC_CAN_NODE_POINTER_EVENT_LEC,
					 dev_cfg->service_request);

	XMC_CAN_NODE_SetEventNodePointer(dev_cfg->can, XMC_CAN_NODE_POINTER_EVENT_TRANSFER_OK,
					 dev_cfg->service_request);

	XMC_CAN_NODE_SetEventNodePointer(dev_cfg->can, XMC_CAN_NODE_POINTER_EVENT_FRAME_COUNTER,
					 dev_cfg->service_request);

	XMC_CAN_NODE_EnableEvent(dev_cfg->can, XMC_CAN_NODE_EVENT_TX_INT |
					       XMC_CAN_NODE_EVENT_ALERT);

	/* set up tx messages */
	for (int i = 0; i < CONFIG_CAN_XMC4XXX_MAX_TX_QUEUE; i++) {
		mo = can_xmc4xxx_get_mo(&mo_index);
		if (mo == NULL) {
			return -ENOMEM;
		}

		dev_data->tx_mo[i] = mo;

		XMC_CAN_AllocateMOtoNodeList(can_xmc4xxx_global_reg,
					     CAN_XMC4XXX_REG_TO_NODE_IND(dev_cfg->can), mo_index);

		mo->MOIPR = FIELD_PREP(CAN_MO_MOIPR_TXINP_Msk, dev_cfg->service_request);
		mo->MOFCR = FIELD_PREP(CAN_MO_MOFCR_MMC_Msk, 0) | CAN_MO_MOFCR_TXIE_Msk;
	}

#ifdef CONFIG_CAN_XMC4XXX_INTERNAL_BUS_MODE
	/* The name of this function is misleading. It doesn't actually enable */
	/* loopback on a single node, but connects all CAN devices to an internal bus. */
	XMC_CAN_NODE_EnableLoopBack(dev_cfg->can);
#endif

	dev_cfg->irq_config_func();

	dev_data->state = CAN_STATE_STOPPED;

#ifndef CONFIG_CAN_XMC4XXX_INTERNAL_BUS_MODE
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif

	ret = can_calc_timing(dev, &timing, dev_cfg->common.bitrate,
			      dev_cfg->common.sample_point);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Presc: %d, BS1: %d, BS2: %d", timing.prescaler, timing.phase_seg1,
		timing.phase_seg2);
	LOG_DBG("Sample-point err : %d", ret);

	return can_set_timing(dev, &timing);
}

static const struct can_driver_api can_xmc4xxx_api_funcs = {
	.get_capabilities = can_xmc4xxx_get_capabilities,
	.set_mode = can_xmc4xxx_set_mode,
	.set_timing = can_xmc4xxx_set_timing,
	.start = can_xmc4xxx_start,
	.stop = can_xmc4xxx_stop,
	.send = can_xmc4xxx_send,
	.add_rx_filter = can_xmc4xxx_add_rx_filter,
	.remove_rx_filter = can_xmc4xxx_remove_rx_filter,
	.get_state = can_xmc4xxx_get_state,
	.set_state_change_callback = can_xmc4xxx_set_state_change_callback,
	.get_core_clock = can_xmc4xxx_get_core_clock,
	.get_max_filters = can_xmc4xxx_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 3,
		.phase_seg2 = 2,
		.prescaler = 1,
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 0,
		.phase_seg1 = 16,
		.phase_seg2 = 8,
		.prescaler = 64,
	},
};

#define CAN_XMC4XXX_INIT(inst)                                                                     \
	static void can_xmc4xxx_irq_config_##inst(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), can_xmc4xxx_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct can_xmc4xxx_data can_xmc4xxx_data_##inst;                                    \
	static const struct can_xmc4xxx_config can_xmc4xxx_config_##inst = {                       \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
		.can = (CAN_NODE_TypeDef *)DT_INST_REG_ADDR(inst),                                 \
		.clock_div8 = DT_INST_PROP(inst, clock_div8),                                      \
		.irq_config_func = can_xmc4xxx_irq_config_##inst,                                  \
		.service_request = DT_INST_IRQN(inst) - CAN_XMC4XXX_IRQ_MIN,                       \
		.input_src = DT_INST_ENUM_IDX(inst, input_src),                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_xmc4xxx_init, NULL, &can_xmc4xxx_data_##inst,          \
				  &can_xmc4xxx_config_##inst, POST_KERNEL,                         \
				  CONFIG_CAN_INIT_PRIORITY, &can_xmc4xxx_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(CAN_XMC4XXX_INIT)
