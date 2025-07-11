/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>

#include "lbm_common.h"

/* LoRa interrupts from the RAL library */
#define RAL_IRQ_LORA                                                                               \
	RAL_IRQ_TX_DONE | RAL_IRQ_RX_DONE | RAL_IRQ_RX_HDR_ERROR | RAL_IRQ_RX_CRC_ERROR |          \
		RAL_IRQ_CAD_DONE | RAL_IRQ_CAD_OK

LOG_MODULE_REGISTER(lbm_driver, CONFIG_LORA_LOG_LEVEL);

/**
 * @brief Attempt to acquire the modem for operations
 *
 * @param dev modem to acquire
 *
 * @retval true if modem was acquired
 * @retval false otherwise
 */
static inline bool modem_acquire(const struct device *dev)
{
	struct lbm_lora_data_common *data = dev->data;

	return atomic_cas(&data->modem_state, STATE_FREE, STATE_BUSY);
}

/**
 * @brief Safely release the modem from any context
 *
 * This function can be called from any context and guarantees that the
 * release operations will only be run once.
 *
 * @param dev modem to release
 *
 * @retval true if modem was released by this function
 * @retval false otherwise
 */
static bool modem_release(const struct device *dev)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;

	/* Move to cleanup state so both acquire and release will fail */
	if (!atomic_cas(&data->modem_state, STATE_BUSY, STATE_CLEANUP)) {
		return false;
	}

	/* Configure modem for sleep */
	lbm_driver_antenna_configure(dev, MODE_SLEEP);
	data->modem_mode = MODE_SLEEP;

	/* Put radio back into sleep mode */
	(void)ral_set_sleep(&config->ralf.ral, true);

	/* Completely release modem */
	data->operation_done = NULL;
	atomic_set(&data->modem_state, STATE_FREE);
	return true;
}

int lbm_lora_config(const struct device *dev, struct lora_modem_config *lora_config)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ralf_params_lora_t params = {
		.mod_params = {
			.sf = lora_config->datarate,
			.cr = lora_config->coding_rate,
			.ldro = 0,
		},
		.pkt_params = {
			.preamble_len_in_symb = lora_config->preamble_len,
			.header_type = RAL_LORA_PKT_EXPLICIT,
			.pld_len_in_bytes = UINT8_MAX,
			.crc_is_on = true,
			.invert_iq_is_on = lora_config->iq_inverted,
		},
		.rf_freq_in_hz = lora_config->frequency,
		.output_pwr_in_dbm = lora_config->tx_power,
		.sync_word = lora_config->public_network ? LBM_LORA_SYNC_WORD_PUBLIC
							 : LBM_LORA_SYNC_WORD_PRIVATE,
	};
	ral_status_t status;
	int ret;

	/* Ensure available, decremented after configuration */
	if (!modem_acquire(dev)) {
		return -EBUSY;
	}

	switch (lora_config->bandwidth) {
	case BW_125_KHZ:
		params.mod_params.bw = RAL_LORA_BW_125_KHZ;
		break;
	case BW_250_KHZ:
		params.mod_params.bw = RAL_LORA_BW_250_KHZ;
		break;
	case BW_500_KHZ:
		params.mod_params.bw = RAL_LORA_BW_500_KHZ;
		break;
	default:
		ret = -EINVAL;
		goto release;
	}

	/* Store TX parameters for use in the TX functions */
	data->mod_params = params.mod_params;
	data->pkt_params = params.pkt_params;

	status = ralf_setup_lora(&config->ralf, &params);
	ret = status == RAL_STATUS_OK ? 0 : -EIO;

release:
	modem_release(dev);
	return ret;
}

int lbm_lora_send_async(const struct device *dev, uint8_t *msg, uint32_t msg_len,
			struct k_poll_signal *async)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_status_t status;
	int ret = 0;

	/* Ensure available, freed by TX done callback */
	if (!modem_acquire(dev)) {
		return -EBUSY;
	}

	/* Configure modem for TX */
	lbm_driver_antenna_configure(dev, MODE_TX);
	data->modem_mode = MODE_TX;

	/* Validate that we have a TX configuration */
	if (data->mod_params.sf == 0) {
		ret = -EINVAL;
		goto release;
	}

	/* Store signal */
	data->operation_done = async;

	/* Update packet params to override the internal packet length variable.
	 * This has a huge overhead since it performs many register writes, but is the only
	 * generic way to update the variable. Why this isn't just done in ral_set_pkt_payload
	 * is anyones guess.
	 */
	data->pkt_params.pld_len_in_bytes = msg_len;
	status = ral_set_lora_pkt_params(&config->ralf.ral, &data->pkt_params);
	if (status != RAL_STATUS_OK) {
		ret = -EINVAL;
		goto release;
	}

	/* Set the packet payload */
	status = ral_set_pkt_payload(&config->ralf.ral, msg, msg_len);
	if (status != RAL_STATUS_OK) {
		ret = -EINVAL;
		goto release;
	}

	/* Start the transmission */
	status = ral_set_tx(&config->ralf.ral);
	if (status != RAL_STATUS_OK) {
		ret = -EINVAL;
		goto release;
	}
	return 0;

release:
	modem_release(dev);
	return ret;
}

int lbm_lora_send(const struct device *dev, uint8_t *msg, uint32_t msg_len)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	struct k_poll_signal done = K_POLL_SIGNAL_INITIALIZER(done);
	struct k_poll_event evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &done);
	uint32_t air_time;
	int ret;

	/* Trigger the asynchronous send */
	ret = lbm_lora_send_async(dev, msg, msg_len, &done);
	if (ret < 0) {
		return ret;
	}

	/* Calculate expected airtime of the packet */
	air_time = ral_get_lora_time_on_air_in_ms(&config->ralf.ral, &data->pkt_params,
						  &data->mod_params);
	LOG_DBG("Expected airtime: %d ms", air_time);

	/* Wait for the packet to finish transmitting.
	 * Setting up the transaction takes some minimal time, take it into
	 * account to ensure extremely short packets don't incorrectly timeout.
	 * Use twice the tx duration to ensure that we are actually detecting
	 * a failed transmission, and not some minor timing variation between
	 * modem and driver.
	 */
	ret = k_poll(&evt, 1, K_MSEC(10 + (2 * air_time)));
	if (ret < 0) {
		if (modem_release(dev)) {
			LOG_ERR("Packet transmission failed!");
		} else {
			/* TX done interrupt is currently running */
			k_poll(&evt, 1, K_FOREVER);
		}
	}
	return ret;
}

int lbm_lora_recv(const struct device *dev, uint8_t *msg, uint8_t msg_len, k_timeout_t timeout,
		  int16_t *rssi, int8_t *snr)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	struct k_poll_signal done = K_POLL_SIGNAL_INITIALIZER(done);
	struct k_poll_event evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &done);
	ral_status_t status;
	int ret;

	/* Ensure available, decremented by op_done_work_handler or on timeout */
	if (!modem_acquire(dev)) {
		return -EBUSY;
	}

	/* Store signal */
	data->operation_done = &done;
	data->rx_state.sync.msg = msg;
	data->rx_state.sync.msg_len = msg_len;

	/* Configure modem for RX */
	lbm_driver_antenna_configure(dev, MODE_RX);
	data->modem_mode = MODE_RX;

	/* Start the reception in continuous mode.
	 * Receive timeouts are handled by the k_poll timeout.
	 * In theory we should be able to use the one-shot mode here and transition
	 * back to IDLE slightly faster, but the SX127x driver does not appear to
	 * receive packets reliably in the single-shot mode.
	 */
	status = ral_set_rx(&config->ralf.ral, RAL_RX_TIMEOUT_CONTINUOUS_MODE);
	if (status != RAL_STATUS_OK) {
		ret = -EINVAL;
		goto release;
	}

	/* Wait for the packet to be received */
	ret = k_poll(&evt, 1, timeout);
	if (ret < 0) {
		if (modem_release(dev)) {
			LOG_INF("Receive timeout");
			return -EAGAIN;
		}
		/* Releasing the modem failed, which means that
		 * the RX callback is currently running. Wait until
		 * the RX callback finishes and we get our packet.
		 */
		k_poll(&evt, 1, K_FOREVER);

		/* We did receive a packet, continue processing */
	}

	if (done.result != 0) {
		LOG_ERR("Receive error");
		ret = done.result;
		goto release;
	}

	/* Retrieve cached RSSI and SNR */
	*rssi = data->rx_state.sync.rssi_dbm;
	*snr = data->rx_state.sync.snr_db;
	ret = data->rx_state.sync.msg_len;

release:
	modem_release(dev);
	return ret;
}

int lbm_lora_recv_async(const struct device *dev, lora_recv_cb cb, void *user_data)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_status_t status;

	/* Cancel ongoing reception */
	if (cb == NULL) {
		if (!modem_release(dev)) {
			/* Not receiving or already being stopped */
			return -EINVAL;
		}
		return 0;
	}

	/* Ensure available */
	if (!modem_acquire(dev)) {
		return -EBUSY;
	}

	/* Configure modem for asynchronous RX */
	lbm_driver_antenna_configure(dev, MODE_RX_ASYNC);
	data->modem_mode = MODE_RX_ASYNC;

	/* Store user state */
	data->rx_state.async.rx_cb = cb;
	data->rx_state.async.user_data = user_data;

	/* Start the reception in continuous mode */
	status = ral_set_rx(&config->ralf.ral, RAL_RX_TIMEOUT_CONTINUOUS_MODE);
	if (status != RAL_STATUS_OK) {
		modem_release(dev);
		return -EIO;
	}
	return 0;
}

int lbm_lora_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power,
		     uint16_t duration)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_status_t status;
	int ret = 0;

	/* Ensure available, freed by op_done_work */
	if (!modem_acquire(dev)) {
		return -EBUSY;
	}

	/* Configure modem for CW */
	lbm_driver_antenna_configure(dev, MODE_CW);
	data->modem_mode = MODE_CW;

	/* Invalidate stored config */
	data->mod_params.sf = 0;

	/* Configure continuous wave */
	status = ral_set_pkt_type(&config->ralf.ral, RAL_PKT_TYPE_LORA);
	if (status != RAL_STATUS_OK) {
		return status;
	}
	status = ral_set_rf_freq(&config->ralf.ral, frequency);
	if (status != RAL_STATUS_OK) {
		return status;
	}
	status = ral_set_tx_cfg(&config->ralf.ral, tx_power, frequency);
	if (status != RAL_STATUS_OK) {
		return status;
	}

	/* Start the continuous wave transmission */
	status = ral_set_tx_cw(&config->ralf.ral);
	if (status != RAL_STATUS_OK) {
		ret = -EIO;
		goto release;
	}

	/* Schedule the end of the transmission */
	k_work_reschedule(&data->op_done_work, K_MSEC(duration));
	return 0;
release:
	modem_release(dev);
	return ret;
}

static int op_done_sync_rx(const struct device *dev)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_lora_rx_pkt_status_t pkt_status;
	ral_status_t status;
	int ret;

	/* Retrieve packet information before putting modem into sleep mode */
	status = ral_get_pkt_payload(&config->ralf.ral, data->rx_state.sync.msg_len,
				     data->rx_state.sync.msg, &data->rx_state.sync.msg_len);
	if (status == RAL_STATUS_OK) {
		LOG_HEXDUMP_DBG(data->rx_state.sync.msg, data->rx_state.sync.msg_len, "RX");
		ret = 0;
	} else {
		LOG_ERR("Failed to retrieve packet payload");
		ret = -EIO;
	}

	status = ral_get_lora_rx_pkt_status(&config->ralf.ral, &pkt_status);
	if (status == RAL_STATUS_OK) {
		data->rx_state.sync.rssi_dbm = pkt_status.signal_rssi_pkt_in_dbm;
		data->rx_state.sync.snr_db = pkt_status.snr_pkt_in_db;
	} else {
		LOG_WRN("Failed to query packet signal stats");
		data->rx_state.sync.rssi_dbm = INT16_MIN;
		data->rx_state.sync.snr_db = INT8_MIN;
	}

	return ret;
}

static void op_done_async_rx(const struct device *dev)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_lora_rx_pkt_status_t pkt_status;
	uint8_t rx_buffer[CONFIG_LORA_BASICS_MODEM_ASYNC_RX_MAX_PAYLOAD];
	ral_status_t status;
	uint16_t size;
	int16_t rssi;

	/* Retrieve the packet payload */
	status = ral_get_pkt_payload(&config->ralf.ral, sizeof(rx_buffer), rx_buffer, &size);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("Failed to retrieve packet payload");
		return;
	}
	LOG_HEXDUMP_DBG(rx_buffer, size, "RX");

	/* Retrieve packet parameters */
	status = ral_get_lora_rx_pkt_status(&config->ralf.ral, &pkt_status);
	if (status != RAL_STATUS_OK) {
		LOG_WRN("Failed to query packet signal stats");
	}

	rssi = IS_ENABLED(CONFIG_LORA_BASICS_MODEM_RSSI_REPORT_TYPE_PACKET)
		       ? pkt_status.rssi_pkt_in_dbm
		       : pkt_status.signal_rssi_pkt_in_dbm;

	/* Run the user callback */
	data->rx_state.async.rx_cb(dev, rx_buffer, size, rssi, pkt_status.snr_pkt_in_db,
				   data->rx_state.async.user_data);
}

static void op_done_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct lbm_lora_data_common *data =
		CONTAINER_OF(dwork, struct lbm_lora_data_common, op_done_work);
	const struct device *dev = data->dev;
	const struct lbm_lora_config_common *config = dev->config;
	struct k_poll_signal *sig_done;
	ral_irq_t irq_state;
	ral_status_t status;
	bool release = false;
	bool error_irq;
	int ret = 0;

	LOG_DBG("%d", data->modem_mode);

	switch (data->modem_mode) {
	case MODE_SLEEP:
		LOG_WRN("Unexpected modem mode (%d)", data->modem_mode);
		return;
	case MODE_TX:
	case MODE_CW:
		status = ral_handle_tx_done(&config->ralf.ral);
		if (status != RAL_STATUS_OK) {
			LOG_WRN("RAL handle TX done failed (%d)", status);
		}
		release = true;
		break;
	case MODE_RX:
		status = ral_handle_rx_done(&config->ralf.ral);
		if (status != RAL_STATUS_OK) {
			LOG_WRN("RAL handle RX done failed (%d)", status);
		}
		ret = op_done_sync_rx(dev);
		release = true;
		break;
	case MODE_RX_ASYNC:
		status = ral_handle_rx_done(&config->ralf.ral);
		if (status != RAL_STATUS_OK) {
			LOG_WRN("RAL handle RX done failed (%d)", status);
		}
		op_done_async_rx(dev);
		/* Don't release the modem here, RX continues */
		break;
	case MODE_CAD:
		LOG_DBG("CAD complete (TBC)");
		break;
	}

	/* Get and reset the current IRQ state */
	(void)ral_get_irq_status(&config->ralf.ral, &irq_state);
	(void)ral_clear_irq_status(&config->ralf.ral, RAL_IRQ_ALL);
	error_irq = irq_state & (RAL_IRQ_RX_TIMEOUT | RAL_IRQ_RX_HDR_ERROR | RAL_IRQ_RX_CRC_ERROR);

	/* Release the modem before running the user callback so that the notified thread can
	 * immediately start another operation before the work item terminates. This requires
	 * preserving the operation_done pointer, since modem_release clears it.
	 */
	sig_done = data->operation_done;

	/* Modem should return to idle */
	if (release) {
		/* Return to sleep mode */
		modem_release(dev);
	}

	/* Notify user that operation has completed */
	if (sig_done) {
		k_poll_signal_raise(sig_done, error_irq ? -EAGAIN : ret);
	}
}

int lbm_lora_common_init(const struct device *dev)
{
	const struct lbm_lora_config_common *config = dev->config;
	struct lbm_lora_data_common *data = dev->data;
	ral_status_t status;

	data->dev = dev;
	k_work_init_delayable(&data->op_done_work, op_done_work_handler);
	atomic_clear(&data->modem_state);

	/* Initialise the radio abstraction layer */
	status = ral_init(&config->ralf.ral);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("RAL init failure (%d)", status);
		return -EIO;
	}

	/* Enable all relevant interrupts */
	status = ral_set_dio_irq_params(&config->ralf.ral, RAL_IRQ_LORA);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("RAL DIO init failure (%d)", status);
		return -EIO;
	}

	/* Idle in sleep mode */
	status = ral_set_sleep(&config->ralf.ral, true);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("Sleep failure (%d)", status);
		return -EIO;
	}
	return 0;
}

DEVICE_API(lora, lbm_lora_api) = {
	.config = lbm_lora_config,
	.send = lbm_lora_send,
	.send_async = lbm_lora_send_async,
	.recv = lbm_lora_recv,
	.recv_async = lbm_lora_recv_async,
	.test_cw = lbm_lora_test_cw,
};
