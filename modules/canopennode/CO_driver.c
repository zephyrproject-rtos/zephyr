/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>

#include <canopennode.h>

#define LOG_LEVEL CONFIG_CANOPEN_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(canopen_driver);

K_KERNEL_STACK_DEFINE(canopen_tx_workq_stack,
		      CONFIG_CANOPENNODE_TX_WORKQUEUE_STACK_SIZE);

struct k_work_q canopen_tx_workq;

struct canopen_tx_work_container {
	struct k_work work;
	CO_CANmodule_t *CANmodule;
};

struct canopen_tx_work_container canopen_tx_queue;

K_MUTEX_DEFINE(canopen_send_mutex);
K_MUTEX_DEFINE(canopen_emcy_mutex);
K_MUTEX_DEFINE(canopen_co_mutex);

inline void canopen_send_lock(void)
{
	k_mutex_lock(&canopen_send_mutex, K_FOREVER);
}

inline void canopen_send_unlock(void)
{
	k_mutex_unlock(&canopen_send_mutex);
}

inline void canopen_emcy_lock(void)
{
	k_mutex_lock(&canopen_emcy_mutex, K_FOREVER);
}

inline void canopen_emcy_unlock(void)
{
	k_mutex_unlock(&canopen_emcy_mutex);
}

inline void canopen_od_lock(void)
{
	k_mutex_lock(&canopen_co_mutex, K_FOREVER);
}

inline void canopen_od_unlock(void)
{
	k_mutex_unlock(&canopen_co_mutex);
}

static void canopen_detach_all_rx_filters(CO_CANmodule_t *CANmodule)
{
	uint16_t i;

	if (!CANmodule || !CANmodule->rx_array || !CANmodule->configured) {
		return;
	}

	for (i = 0U; i < CANmodule->rx_size; i++) {
		if (CANmodule->rx_array[i].filter_id != -ENOSPC) {
			can_remove_rx_filter(CANmodule->dev,
					     CANmodule->rx_array[i].filter_id);
			CANmodule->rx_array[i].filter_id = -ENOSPC;
		}
	}
}

static void canopen_rx_callback(const struct device *dev, struct can_frame *frame, void *arg)
{
	CO_CANrx_t *buffer = (CO_CANrx_t *)arg;
	CO_CANrxMsg_t rxMsg;

	ARG_UNUSED(dev);

	if (!buffer || !buffer->pFunct) {
		LOG_ERR("failed to process CAN rx callback");
		return;
	}

	rxMsg.ident = frame->id;
	rxMsg.DLC = frame->dlc;
	memcpy(rxMsg.data, frame->data, frame->dlc);
	buffer->pFunct(buffer->object, &rxMsg);
}

static void canopen_tx_callback(const struct device *dev, int error, void *arg)
{
	CO_CANmodule_t *CANmodule = arg;

	ARG_UNUSED(dev);

	if (!CANmodule) {
		LOG_ERR("failed to process CAN tx callback");
		return;
	}

	if (error == 0) {
		CANmodule->first_tx_msg = false;
	}

	k_work_submit_to_queue(&canopen_tx_workq, &canopen_tx_queue.work);
}

static void canopen_tx_retry(struct k_work *item)
{
	struct canopen_tx_work_container *container =
		CONTAINER_OF(item, struct canopen_tx_work_container, work);
	CO_CANmodule_t *CANmodule = container->CANmodule;
	struct can_frame frame;
	CO_CANtx_t *buffer;
	int err;
	uint16_t i;

	CO_LOCK_CAN_SEND();

	for (i = 0; i < CANmodule->tx_size; i++) {
		buffer = &CANmodule->tx_array[i];
		if (buffer->bufferFull) {
			frame.id_type = CAN_STANDARD_IDENTIFIER;
			frame.id = buffer->ident;
			frame.dlc = buffer->DLC;
			frame.rtr = (buffer->rtr ? 1 : 0);
			memcpy(frame.data, buffer->data, buffer->DLC);

			err = can_send(CANmodule->dev, &frame, K_NO_WAIT,
				       canopen_tx_callback, CANmodule);
			if (err == -EAGAIN) {
				break;
			} else if (err != 0) {
				LOG_ERR("failed to send CAN frame (err %d)",
					err);
				CO_errorReport(CANmodule->em,
					       CO_EM_GENERIC_SOFTWARE_ERROR,
					       CO_EMC_COMMUNICATION, 0);

			}

			buffer->bufferFull = false;
		}
	}

	CO_UNLOCK_CAN_SEND();
}

void CO_CANsetConfigurationMode(void *CANdriverState)
{
	struct canopen_context *ctx = (struct canopen_context *)CANdriverState;
	int err;

	err = can_stop(ctx->dev);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("failed to stop CAN interface (err %d)", err);
	}
}

void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule)
{
	int err;

	err = can_start(CANmodule->dev);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("failed to start CAN interface (err %d)", err);
		return;
	}

	CANmodule->CANnormal = true;
}

CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule,
				   void *CANdriverState,
				   CO_CANrx_t rxArray[], uint16_t rxSize,
				   CO_CANtx_t txArray[], uint16_t txSize,
				   uint16_t CANbitRate)
{
	struct canopen_context *ctx = (struct canopen_context *)CANdriverState;
	uint16_t i;
	int err;
	int max_filters;

	LOG_DBG("rxSize = %d, txSize = %d", rxSize, txSize);

	if (!CANmodule || !rxArray || !txArray || !CANdriverState) {
		LOG_ERR("failed to initialize CAN module");
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	max_filters = can_get_max_filters(ctx->dev, CAN_STANDARD_IDENTIFIER);
	if (max_filters != -ENOSYS) {
		if (max_filters < 0) {
			LOG_ERR("unable to determine number of CAN RX filters");
			return CO_ERROR_SYSCALL;
		}

		if (rxSize > max_filters) {
			LOG_ERR("insufficient number of concurrent CAN RX filters"
				" (needs %d, %d available)", rxSize, max_filters);
			return CO_ERROR_OUT_OF_MEMORY;
		} else if (rxSize < max_filters) {
			LOG_DBG("excessive number of concurrent CAN RX filters enabled"
				" (needs %d, %d available)", rxSize, max_filters);
		}
	}

	canopen_detach_all_rx_filters(CANmodule);
	canopen_tx_queue.CANmodule = CANmodule;

	CANmodule->dev = ctx->dev;
	CANmodule->rx_array = rxArray;
	CANmodule->rx_size = rxSize;
	CANmodule->tx_array = txArray;
	CANmodule->tx_size = txSize;
	CANmodule->CANnormal = false;
	CANmodule->first_tx_msg = true;
	CANmodule->errors = 0;
	CANmodule->em = NULL;

	for (i = 0U; i < rxSize; i++) {
		rxArray[i].ident = 0U;
		rxArray[i].pFunct = NULL;
		rxArray[i].filter_id = -ENOSPC;
	}

	for (i = 0U; i < txSize; i++) {
		txArray[i].bufferFull = false;
	}

	err = can_set_bitrate(CANmodule->dev, KHZ(CANbitRate));
	if (err) {
		LOG_ERR("failed to configure CAN bitrate (err %d)", err);
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	err = can_set_mode(CANmodule->dev, CAN_MODE_NORMAL);
	if (err) {
		LOG_ERR("failed to configure CAN interface (err %d)", err);
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	CANmodule->configured = true;

	return CO_ERROR_NO;
}

void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
	int err;

	if (!CANmodule || !CANmodule->dev) {
		return;
	}

	canopen_detach_all_rx_filters(CANmodule);

	err = can_stop(CANmodule->dev);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("failed to disable CAN interface (err %d)", err);
	}
}

uint16_t CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg)
{
	return rxMsg->ident;
}

CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
				uint16_t ident, uint16_t mask, bool_t rtr,
				void *object,
				CO_CANrxBufferCallback_t pFunct)
{
	struct can_filter filter;
	CO_CANrx_t *buffer;

	if (CANmodule == NULL) {
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	if (!pFunct || (index >= CANmodule->rx_size)) {
		LOG_ERR("failed to initialize CAN rx buffer, illegal argument");
		CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
			       CO_EMC_SOFTWARE_INTERNAL, 0);
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	buffer = &CANmodule->rx_array[index];
	buffer->object = object;
	buffer->pFunct = pFunct;

	filter.id_type = CAN_STANDARD_IDENTIFIER;
	filter.id = ident;
	filter.id_mask = mask;
	filter.rtr = (rtr ? 1 : 0);
	filter.rtr_mask = 1;

	if (buffer->filter_id != -ENOSPC) {
		can_remove_rx_filter(CANmodule->dev, buffer->filter_id);
	}

	buffer->filter_id = can_add_rx_filter(CANmodule->dev,
					      canopen_rx_callback,
					      buffer, &filter);
	if (buffer->filter_id == -ENOSPC) {
		LOG_ERR("failed to add CAN rx callback, no free filter");
		CO_errorReport(CANmodule->em, CO_EM_MEMORY_ALLOCATION_ERROR,
			       CO_EMC_SOFTWARE_INTERNAL, 0);
		return CO_ERROR_OUT_OF_MEMORY;
	}

	return CO_ERROR_NO;
}

CO_CANtx_t *CO_CANtxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index,
			       uint16_t ident, bool_t rtr, uint8_t noOfBytes,
			       bool_t syncFlag)
{
	CO_CANtx_t *buffer;

	if (CANmodule == NULL) {
		return NULL;
	}

	if (index >= CANmodule->tx_size) {
		LOG_ERR("failed to initialize CAN rx buffer, illegal argument");
		CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
			       CO_EMC_SOFTWARE_INTERNAL, 0);
		return NULL;
	}

	buffer = &CANmodule->tx_array[index];
	buffer->ident = ident;
	buffer->rtr = rtr;
	buffer->DLC = noOfBytes;
	buffer->bufferFull = false;
	buffer->syncFlag = syncFlag;

	return buffer;
}

CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
	CO_ReturnError_t ret = CO_ERROR_NO;
	struct can_frame frame;
	int err;

	if (!CANmodule || !CANmodule->dev || !buffer) {
		return CO_ERROR_ILLEGAL_ARGUMENT;
	}

	CO_LOCK_CAN_SEND();

	if (buffer->bufferFull) {
		if (!CANmodule->first_tx_msg) {
			CO_errorReport(CANmodule->em, CO_EM_CAN_TX_OVERFLOW,
				       CO_EMC_CAN_OVERRUN, buffer->ident);
		}
		buffer->bufferFull = false;
		ret = CO_ERROR_TX_OVERFLOW;
	}

	frame.id_type = CAN_STANDARD_IDENTIFIER;
	frame.id = buffer->ident;
	frame.dlc = buffer->DLC;
	frame.rtr = (buffer->rtr ? 1 : 0);
	memcpy(frame.data, buffer->data, buffer->DLC);

	err = can_send(CANmodule->dev, &frame, K_NO_WAIT, canopen_tx_callback,
		       CANmodule);
	if (err == -EAGAIN) {
		buffer->bufferFull = true;
	} else if (err != 0) {
		LOG_ERR("failed to send CAN frame (err %d)", err);
		CO_errorReport(CANmodule->em, CO_EM_GENERIC_SOFTWARE_ERROR,
			       CO_EMC_COMMUNICATION, 0);
		ret = CO_ERROR_TX_UNCONFIGURED;
	}

	CO_UNLOCK_CAN_SEND();

	return ret;
}

void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule)
{
	bool_t tpdoDeleted = false;
	CO_CANtx_t *buffer;
	uint16_t i;

	if (!CANmodule) {
		return;
	}

	CO_LOCK_CAN_SEND();

	for (i = 0; i < CANmodule->tx_size; i++) {
		buffer = &CANmodule->tx_array[i];
		if (buffer->bufferFull && buffer->syncFlag) {
			buffer->bufferFull = false;
			tpdoDeleted = true;
		}
	}

	CO_UNLOCK_CAN_SEND();

	if (tpdoDeleted) {
		CO_errorReport(CANmodule->em, CO_EM_TPDO_OUTSIDE_WINDOW,
			       CO_EMC_COMMUNICATION, 0);
	}
}

void CO_CANverifyErrors(CO_CANmodule_t *CANmodule)
{
	CO_EM_t *em = (CO_EM_t *)CANmodule->em;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	uint8_t rx_overflows;
	uint32_t errors;
	int err;

	/*
	 * TODO: Zephyr lacks an API for reading the rx mailbox
	 * overflow counter.
	 */
	rx_overflows  = 0;

	err = can_get_state(CANmodule->dev, &state, &err_cnt);
	if (err != 0) {
		LOG_ERR("failed to get CAN controller state (err %d)", err);
		return;
	}

	errors = ((uint32_t)err_cnt.tx_err_cnt << 16) |
		 ((uint32_t)err_cnt.rx_err_cnt << 8) |
		 rx_overflows;

	if (errors != CANmodule->errors) {
		CANmodule->errors = errors;

		if (state == CAN_STATE_BUS_OFF) {
			/* Bus off */
			CO_errorReport(em, CO_EM_CAN_TX_BUS_OFF,
				       CO_EMC_BUS_OFF_RECOVERED, errors);
		} else {
			/* Bus not off */
			CO_errorReset(em, CO_EM_CAN_TX_BUS_OFF, errors);

			if ((err_cnt.rx_err_cnt >= 96U) ||
			    (err_cnt.tx_err_cnt >= 96U)) {
				/* Bus warning */
				CO_errorReport(em, CO_EM_CAN_BUS_WARNING,
					       CO_EMC_NO_ERROR, errors);
			} else {
				/* Bus not warning */
				CO_errorReset(em, CO_EM_CAN_BUS_WARNING,
					      errors);
			}

			if (err_cnt.rx_err_cnt >= 128U) {
				/* Bus rx passive */
				CO_errorReport(em, CO_EM_CAN_RX_BUS_PASSIVE,
					       CO_EMC_CAN_PASSIVE, errors);
			} else {
				/* Bus not rx passive */
				CO_errorReset(em, CO_EM_CAN_RX_BUS_PASSIVE,
					      errors);
			}

			if (err_cnt.tx_err_cnt >= 128U &&
			    !CANmodule->first_tx_msg) {
				/* Bus tx passive */
				CO_errorReport(em, CO_EM_CAN_TX_BUS_PASSIVE,
					       CO_EMC_CAN_PASSIVE, errors);
			} else if (CO_isError(em, CO_EM_CAN_TX_BUS_PASSIVE)) {
				/* Bus not tx passive */
				CO_errorReset(em, CO_EM_CAN_TX_BUS_PASSIVE,
					      errors);
				CO_errorReset(em, CO_EM_CAN_TX_OVERFLOW,
					      errors);
			}
		}

		/* This code can be activated if we can read the overflows*/
		if (false && rx_overflows != 0U) {
			CO_errorReport(em, CO_EM_CAN_RXB_OVERFLOW,
				       CO_EMC_CAN_OVERRUN, errors);
		}
	}
}

static int canopen_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_queue_start(&canopen_tx_workq, canopen_tx_workq_stack,
			   K_KERNEL_STACK_SIZEOF(canopen_tx_workq_stack),
			   CONFIG_CANOPENNODE_TX_WORKQUEUE_PRIORITY, NULL);

	k_thread_name_set(&canopen_tx_workq.thread, "canopen_tx_workq");

	k_work_init(&canopen_tx_queue.work, canopen_tx_retry);

	return 0;
}

SYS_INIT(canopen_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
