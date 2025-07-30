/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/lin.h>
#include <zephyr/drivers/lin/transceiver.h>
#include <zephyr/logging/log.h>

#include "lin_renesas_ra_priv.h"

LOG_MODULE_REGISTER(lin_renesas_ra, CONFIG_LIN_LOG_LEVEL);

/* Call the user-defined callback function for a LIN event. */
static inline void lin_renesas_ra_call_usr_callback(const struct device *dev,
						    struct lin_event *event)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_event_callback_t callback = data->common.callback;
	void *user_data = data->common.callback_data;

	if (callback) {
		callback(dev, event, user_data);
	}
}

/* Release the LIN bus and abort any ongoing transmission, then return the previous device state. */
static atomic_val_t lin_renesas_ra_abort_transmission(const struct device *dev,
						      bool ongoing_transfer)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret;

	if (ongoing_transfer) {
		fsp_err = fsp_lin_instance->p_api->communicationAbort(fsp_lin_instance->p_ctrl);
		__ASSERT_NO_MSG(fsp_err == FSP_SUCCESS);
	}

	if (k_work_delayable_is_pending(&data->timeout_work)) {
		ret = k_work_cancel_delayable(&data->timeout_work);
		__ASSERT_NO_MSG(ret == 0 || FIELD_GET(ret, K_WORK_CANCELING_BIT));
	}

	k_sem_give(&data->transmission_sem);

	return atomic_clear(&data->device_state);
}

/* Transmission timeout handler */
void lin_renesas_ra_timeout_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct lin_renesas_ra_data *data =
		CONTAINER_OF(dwork, struct lin_renesas_ra_data, timeout_work);
	struct device *dev = data->fsp_lin_instance.p_cfg->p_context;
	atomic_val_t prev_state = lin_renesas_ra_abort_transmission(dev, true);
	struct lin_event event;

	if (prev_state == LIN_RENESAS_RA_STATE_IDLE) {
		/* No ongoing transmission, spurious timeout */
		return;
	}

	event.type = (prev_state == LIN_RENESAS_RA_STATE_TX_ON_GOING) ? LIN_EVT_TX_DATA
								      : LIN_EVT_RX_DATA;
	event.data.pid = lin_compute_pid(data->last_transfer_params.id);
	event.status = -EAGAIN;

	lin_renesas_ra_call_usr_callback(data->fsp_lin_instance.p_cfg->p_context, &event);
}

int lin_renesas_ra_start(const struct device *dev)
{
	struct lin_renesas_ra_data *data = dev->data;
	const struct device *lin_transceiver_dev = lin_get_transceiver(dev);
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret = 0;

	if (data->common.started) {
		LOG_DBG("LIN device is already running");
		return -EALREADY;
	}

	if (lin_transceiver_dev != NULL) {
		ret = lin_transceiver_enable(lin_transceiver_dev, 0);
		if (ret < 0) {
			LOG_DBG("Failed to enable transceiver: %d", ret);
			return ret;
		}
	}

	fsp_err = fsp_lin_instance->p_api->open(fsp_lin_instance->p_ctrl, fsp_lin_instance->p_cfg);
	if (fsp_err == FSP_SUCCESS) {
		data->common.started = true;
	} else if (fsp_err == FSP_ERR_INVALID_MODE) {
		ret = -EINVAL;
	} else {
		ret = -EIO;
	}

	if (ret < 0 && lin_transceiver_dev != NULL) {
		(void)lin_transceiver_disable(lin_transceiver_dev);
	}

	return ret;
}

int lin_renesas_ra_stop(const struct device *dev)
{
	struct lin_renesas_ra_data *data = dev->data;
	const struct device *lin_transceiver_dev = lin_get_transceiver(dev);
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret = 0;

	if (!data->common.started) {
		return -EALREADY;
	}

	lin_renesas_ra_abort_transmission(dev, true);

	fsp_err = fsp_lin_instance->p_api->close(fsp_lin_instance->p_ctrl);
	if (fsp_err != FSP_SUCCESS) {
		LOG_DBG("Failed to close LIN instance: %d", fsp_err);
		return -EIO;
	}

	if (lin_transceiver_dev != NULL) {
		ret = lin_transceiver_disable(lin_transceiver_dev);
		if (ret < 0) {
			LOG_DBG("Failed to disable transceiver: %d", ret);
			return ret;
		}
	}

	data->common.started = false;

	return 0;
}

int lin_renesas_ra_get_config(const struct device *dev, struct lin_config *cfg)
{
	struct lin_renesas_ra_data *data = dev->data;

	if (cfg == NULL) {
		return -EINVAL;
	}

	memcpy(cfg, &data->common.config, sizeof(struct lin_config));

	return 0;
}

static int lin_msg_parameter_validate(const struct lin_msg *const msg)
{
	if (msg == NULL) {
		return -EINVAL;
	}

	if ((msg->id & (uint8_t)~LIN_ID_MASK) != 0) {
		return -EINVAL;
	}

	if ((msg->checksum_type != LIN_CHECKSUM_CLASSIC) &&
	    (msg->checksum_type != LIN_CHECKSUM_ENHANCED)) {
		return -EINVAL;
	}

	if (msg->data_len > 0 && msg->data == NULL) {
		return -EINVAL;
	}

	if (msg->data_len > LIN_MAX_DLEN) {
		return -EINVAL;
	}

	return 0;
}

static void lin_renesas_ra_msg_prepare(struct lin_msg *const msg,
				       lin_transfer_params_t *const transfer_params)
{
	if (msg->checksum_type == LIN_CHECKSUM_CLASSIC) {
		transfer_params->checksum_type = LIN_CHECKSUM_TYPE_CLASSIC;
	} else {
		transfer_params->checksum_type = LIN_CHECKSUM_TYPE_ENHANCED;
	}

	transfer_params->id = msg->id;
	transfer_params->p_data = msg->data;
	transfer_params->num_bytes = msg->data_len;
}

int lin_renesas_ra_send(const struct device *dev, const struct lin_msg *msg, k_timeout_t timeout)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret;

	ret = lin_msg_parameter_validate(msg);
	if (ret < 0) {
		return ret;
	}

	if (data->common.config.mode == LIN_MODE_RESPONDER) {
		return -EPERM;
	}

	ret = k_sem_take(&data->transmission_sem, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	lin_renesas_ra_msg_prepare((struct lin_msg *const)msg, &data->last_transfer_params);

	fsp_err = fsp_lin_instance->p_api->write(fsp_lin_instance->p_ctrl,
						 &data->last_transfer_params);
	if (fsp_err != FSP_SUCCESS) {
		lin_renesas_ra_abort_transmission(dev, false);
		return -EIO;
	}

	atomic_set(&data->device_state, LIN_RENESAS_RA_STATE_TX_ON_GOING);
	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		ret = k_work_reschedule(&data->timeout_work, timeout);
		__ASSERT_NO_MSG(ret >= 0);
	}

	return 0;
}

int lin_renesas_ra_receive(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	lin_transfer_params_t transfer_params;
	fsp_err_t fsp_err;
	int ret;

	ret = lin_msg_parameter_validate(msg);
	if (ret < 0) {
		return ret;
	}

	if (data->common.config.mode == LIN_MODE_RESPONDER) {
		return -EPERM;
	}

	ret = k_sem_take(&data->transmission_sem, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	lin_renesas_ra_msg_prepare((struct lin_msg *const)msg, &data->last_transfer_params);

	/* Prepare transfer parameters for header sent only */
	transfer_params.id = data->last_transfer_params.id;
	transfer_params.checksum_type = data->last_transfer_params.checksum_type;
	transfer_params.p_data = NULL;
	transfer_params.num_bytes = 0;

	fsp_err = fsp_lin_instance->p_api->write(fsp_lin_instance->p_ctrl, &transfer_params);
	if (fsp_err != FSP_SUCCESS) {
		lin_renesas_ra_abort_transmission(dev, false);
		return -EIO;
	}

	atomic_set(&data->device_state, LIN_RENESAS_RA_STATE_RX_ON_GOING);
	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		ret = k_work_reschedule(&data->timeout_work, timeout);
		__ASSERT_NO_MSG(ret >= 0);
	}

	return 0;
}

int lin_renesas_ra_response(const struct device *dev, const struct lin_msg *msg,
			    k_timeout_t timeout)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret;

	ret = lin_msg_parameter_validate(msg);
	if (ret < 0) {
		return ret;
	}

	if (data->common.config.mode == LIN_MODE_COMMANDER) {
		return -EPERM;
	}

	if (!atomic_cas(&data->device_state, LIN_RENESAS_RA_HEADER_RECEIVED,
			LIN_RENESAS_RA_STATE_TX_ON_GOING)) {
		return -EFAULT;
	}

	ret = k_sem_take(&data->transmission_sem, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	lin_renesas_ra_msg_prepare((struct lin_msg *const)msg, &data->last_transfer_params);

	fsp_err = fsp_lin_instance->p_api->write(fsp_lin_instance->p_ctrl,
						 &data->last_transfer_params);
	if (fsp_err != FSP_SUCCESS) {
		lin_renesas_ra_abort_transmission(dev, false);
		return -EIO;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		ret = k_work_reschedule(&data->timeout_work, timeout);
		__ASSERT_NO_MSG(ret >= 0);
	}

	return 0;
}

int lin_renesas_ra_read(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout)
{
	struct lin_renesas_ra_data *data = dev->data;
	lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
	fsp_err_t fsp_err;
	int ret;

	ret = lin_msg_parameter_validate(msg);
	if (ret < 0) {
		return ret;
	}

	if (data->common.config.mode == LIN_MODE_COMMANDER) {
		return -EPERM;
	}

	if (!atomic_cas(&data->device_state, LIN_RENESAS_RA_HEADER_RECEIVED,
			LIN_RENESAS_RA_STATE_RX_ON_GOING)) {
		return -EFAULT;
	}

	ret = k_sem_take(&data->transmission_sem, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	lin_renesas_ra_msg_prepare((struct lin_msg *const)msg, &data->last_transfer_params);

	fsp_err = fsp_lin_instance->p_api->read(fsp_lin_instance->p_ctrl,
						&data->last_transfer_params);
	if (fsp_err != FSP_SUCCESS) {
		lin_renesas_ra_abort_transmission(dev, false);
		return -EIO;
	}

	if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) && !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		ret = k_work_reschedule(&data->timeout_work, timeout);
		__ASSERT_NO_MSG(ret >= 0);
	}

	return 0;
}

int lin_renesas_ra_set_callback(const struct device *dev, lin_event_callback_t callback,
				void *user_data)
{
	struct lin_renesas_ra_data *data = dev->data;
	int key = irq_lock();

	data->common.callback = callback;
	data->common.callback_data = user_data;

	irq_unlock(key);

	return 0;
}

void lin_renesas_ra_callback_adapter(lin_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct lin_renesas_ra_data *data = dev->data;
	struct lin_event event;
	bool release_bus = false;

	switch (p_args->event) {
	case LIN_EVENT_RX_HEADER_COMPLETE:
		atomic_set(&data->device_state, LIN_RENESAS_RA_HEADER_RECEIVED);
		event.type = LIN_EVT_RX_HEADER;
		event.header.pid = p_args->pid;
		event.status = 0;
		break;
	case LIN_EVENT_TX_HEADER_COMPLETE: {
		lin_instance_t *fsp_lin_instance = &data->fsp_lin_instance;
		atomic_val_t current_state = atomic_get(&data->device_state);
		fsp_err_t fsp_err;

		event.type = LIN_EVT_TX_HEADER;
		event.header.pid = p_args->pid;
		event.status = 0;

		if (current_state == LIN_RENESAS_RA_STATE_RX_ON_GOING) {
			fsp_err = fsp_lin_instance->p_api->read(fsp_lin_instance->p_ctrl,
								&data->last_transfer_params);
			if (fsp_err != FSP_SUCCESS) {
				event.type = LIN_EVT_RX_DATA;
				event.status = -EIO;
				event.data.pid = p_args->pid;
				event.data.bytes_received = 0;
				release_bus = true;
			}
		} else if (current_state == LIN_RENESAS_RA_STATE_TX_ON_GOING) {
			if (data->last_transfer_params.num_bytes == 0) {
				/* Header only transmission, release the bus */
				release_bus = true;
			}
		} else {
			/* Spurious event, return immediately */
			return;
		}

		break;
	}
	case LIN_EVENT_RX_DATA_COMPLETE:
		event.type = LIN_EVT_RX_DATA;
		event.data.pid = p_args->pid;
		event.data.bytes_received = p_args->bytes_received;
		event.data.checksum = p_args->checksum;
		event.status = 0;
		release_bus = true;
		break;
	case LIN_EVENT_TX_DATA_COMPLETE:
		event.type = LIN_EVT_TX_DATA;
		event.data.pid = p_args->pid;
		event.status = 0;
		release_bus = true;
		break;
	case LIN_EVENT_ERR_INVALID_CHECKSUM:
		event.type = LIN_EVT_ERR;
		event.error_flags = LIN_ERR_INVALID_CHECKSUM;
		release_bus = true;
		break;
	case LIN_EVENT_ERR_BUS_COLLISION_DETECTED:
		event.type = LIN_EVT_ERR;
		event.error_flags = LIN_ERR_BUS_COLLISION;
		release_bus = true;
		break;
	case LIN_EVENT_ERR_COUNTER_OVERFLOW:
		event.type = LIN_EVT_ERR;
		event.error_flags = LIN_ERR_COUNTER_OVERFLOW;
		release_bus = true;
		break;
	case LIN_EVENT_ERR_PARITY:
		event.type = LIN_EVT_ERR;
		event.error_flags = LIN_ERR_PARITY;
		release_bus = true;
		break;
	case LIN_EVENT_ERR_FRAMING:
		event.type = LIN_EVT_ERR;
		event.error_flags = LIN_ERR_FRAMING;
		release_bus = true;
		break;
	default:
		return;
	}

	if (release_bus) {
		lin_renesas_ra_abort_transmission(dev, false);
	}

	lin_renesas_ra_call_usr_callback(dev, &event);
}
