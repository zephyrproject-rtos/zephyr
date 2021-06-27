/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <logging/log.h>
#include <sys/atomic_builtin.h>
#include <zephyr.h>

/* LoRaMac-node specific includes */
#include <radio.h>

#include "sx12xx_common.h"

#define STATE_FREE      0
#define STATE_BUSY      1
#define STATE_CLEANUP   2

LOG_MODULE_REGISTER(sx12xx_common, CONFIG_LORA_LOG_LEVEL);

static struct sx12xx_data {
	struct k_poll_signal *operation_done;
	RadioEvents_t events;
	atomic_t modem_usage;
	uint8_t *rx_buf;
	uint8_t rx_len;
	int8_t snr;
	int16_t rssi;
} dev_data;

int __sx12xx_configure_pin(const struct device **dev, const char *controller,
			   gpio_pin_t pin, gpio_flags_t flags)
{
	int err;

	*dev = device_get_binding(controller);
	if (!(*dev)) {
		LOG_ERR("Cannot get pointer to %s device", controller);
		return -EIO;
	}

	err = gpio_pin_configure(*dev, pin, flags);
	if (err) {
		LOG_ERR("Cannot configure gpio %s %d: %d", controller, pin,
			err);
		return err;
	}

	return 0;
}

/**
 * @brief Attempt to acquire the modem for operations
 *
 * @param data common sx12xx data struct
 *
 * @retval true if modem was acquired
 * @retval false otherwise
 */
static inline bool modem_acquire(struct sx12xx_data *data)
{
	return atomic_cas(&data->modem_usage, STATE_FREE, STATE_BUSY);
}

/**
 * @brief Safely release the modem from any context
 *
 * This function can be called from any context and guarantees that the
 * release operations will only be run once.
 *
 * @param data common sx12xx data struct
 *
 * @retval true if modem was released by this function
 * @retval false otherwise
 */
static bool modem_release(struct sx12xx_data *data)
{
	/* Increment atomic so both acquire and release will fail */
	if (!atomic_cas(&data->modem_usage, STATE_BUSY, STATE_CLEANUP)) {
		return false;
	}
	/* Put radio back into sleep mode */
	Radio.Sleep();
	/* Completely release modem */
	data->operation_done = NULL;
	atomic_clear(&data->modem_usage);
	return true;
}

static void sx12xx_ev_rx_done(uint8_t *payload, uint16_t size, int16_t rssi,
			      int8_t snr)
{
	dev_data.rx_buf = payload;
	dev_data.rx_len = size;
	dev_data.rssi = rssi;
	dev_data.snr = snr;

	modem_release(&dev_data);

	if (dev_data.operation_done) {
		k_poll_signal_raise(dev_data.operation_done, 0);
	}
}

static void sx12xx_ev_tx_done(void)
{
	modem_release(&dev_data);
}

int sx12xx_lora_send(const struct device *dev, uint8_t *data,
		     uint32_t data_len)
{
	/* Ensure available, decremented by sx12xx_ev_tx_done */
	if (!modem_acquire(&dev_data)) {
		return -EBUSY;
	}

	Radio.SetMaxPayloadLength(MODEM_LORA, data_len);

	Radio.Send(data, data_len);

	return 0;
}

int sx12xx_lora_recv(const struct device *dev, uint8_t *data, uint8_t size,
		     k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	struct k_poll_signal done = K_POLL_SIGNAL_INITIALIZER(done);
	struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(
		K_POLL_TYPE_SIGNAL,
		K_POLL_MODE_NOTIFY_ONLY,
		&done);
	int ret;

	/* Ensure available, decremented by sx12xx_ev_rx_done or on timeout */
	if (!modem_acquire(&dev_data)) {
		return -EBUSY;
	}

	/* Store operation signal */
	dev_data.operation_done = &done;

	Radio.SetMaxPayloadLength(MODEM_LORA, 255);
	Radio.Rx(0);

	ret = k_poll(&evt, 1, timeout);
	if (ret < 0) {
		LOG_INF("Receive timeout");
		modem_release(&dev_data);
		return ret;
	}

	/* Only copy the bytes that can fit the buffer, drop the rest */
	if (dev_data.rx_len > size) {
		dev_data.rx_len = size;
	}

	/*
	 * FIXME: We are copying the global buffer here, so it might get
	 * overwritten inbetween when a new packet comes in. Use some
	 * wise method to fix this!
	 */
	memcpy(data, dev_data.rx_buf, dev_data.rx_len);

	if (rssi != NULL) {
		*rssi = dev_data.rssi;
	}

	if (snr != NULL) {
		*snr = dev_data.snr;
	}

	return dev_data.rx_len;
}

int sx12xx_lora_config(const struct device *dev,
		       struct lora_modem_config *config)
{
	/* Ensure available, decremented after configuration */
	if (!modem_acquire(&dev_data)) {
		return -EBUSY;
	}

	Radio.SetChannel(config->frequency);

	if (config->tx) {
		Radio.SetTxConfig(MODEM_LORA, config->tx_power, 0,
				  config->bandwidth, config->datarate,
				  config->coding_rate, config->preamble_len,
				  false, true, 0, 0, false, 4000);
	} else {
		/* TODO: Get symbol timeout value from config parameters */
		Radio.SetRxConfig(MODEM_LORA, config->bandwidth,
				  config->datarate, config->coding_rate,
				  0, config->preamble_len, 10, false, 0,
				  false, 0, 0, false, true);
	}

	modem_release(&dev_data);
	return 0;
}

int sx12xx_lora_test_cw(const struct device *dev, uint32_t frequency,
			int8_t tx_power,
			uint16_t duration)
{
	/* Ensure available, freed in sx12xx_ev_tx_done */
	if (!modem_acquire(&dev_data)) {
		return -EBUSY;
	}

	Radio.SetTxContinuousWave(frequency, tx_power, duration);
	return 0;
}

int sx12xx_init(const struct device *dev)
{
	atomic_set(&dev_data.modem_usage, 0);

	dev_data.events.TxDone = sx12xx_ev_tx_done;
	dev_data.events.RxDone = sx12xx_ev_rx_done;
	Radio.Init(&dev_data.events);

	/*
	 * Automatically place the radio into sleep mode upon boot.
	 * The required `lora_config` call before transmission or reception
	 * will bring the radio out of sleep mode before it is used. The radio
	 * is automatically placed back into sleep mode upon TX or RX
	 * completion.
	 */
	Radio.Sleep();

	return 0;
}
