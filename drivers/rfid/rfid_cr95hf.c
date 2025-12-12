/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT   st_cr95hf
#define LOG_MODULE_NAME rfid_cr95hf

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_RFID_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rfid.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "rfid_cr95hf.h"

struct rfid_cr95hf_spi_config {
	const struct spi_dt_spec spi;
	const struct gpio_dt_spec irq_in;
	const struct gpio_dt_spec irq_out;
};

struct rfid_cr95hf_data {
	struct gpio_callback irq_callback;
	struct spi_buf spi_snd_buffer;
	struct spi_buf spi_rcv_buffer;
	struct spi_buf_set spi_snd_buffer_arr;
	struct spi_buf_set spi_rcv_buffer_arr;
	uint8_t rcv_buffer[CR95HF_RCV_BUF_SIZE];
	uint8_t snd_buffer[CR95HF_SND_BUF_SIZE];
	struct k_sem irq_out_sem;
	gpio_callback_handler_t cb_handler;
	struct k_mutex data_mutex;
	bool hw_tx_crc;
	uint32_t timeout_us;
};

static inline size_t count_bits(uint32_t val)
{
	/* Implements Brian Kernighan’s Algorithm to count bits. */
	size_t cnt = 0U;

	while (val != 0U) {
		val = val & (val - 1U);
		cnt++;
	}

	return cnt;
}

/**
 * @brief Waits for the IRQ_OUT pin to go low.
 *
 * This function checks the state of the IRQ_OUT pin and waits for it to become
 * low by taking a semaphore. It configures the GPIO interrupt for the pin
 * to be triggered on the active edge and then disables the interrupt again after
 * the wait.
 *
 * @param irq_out Pointer to the GPIO specification for the IRQ_OUT pin.
 * @param data Pointer to the RFID data structure containing the semaphore.
 */
static int rfid_cr95hf_wait_for_irq_out_low(const struct gpio_dt_spec *irq_out,
					    struct rfid_cr95hf_data *data, k_timeout_t timeout)
{
	int ret = 0;

	if (gpio_pin_get_dt(irq_out)) {
		return 0; /* Already low, no need to wait */
	}

	gpio_pin_interrupt_configure_dt(irq_out, GPIO_INT_EDGE_TO_ACTIVE);
	ret = k_sem_take(&data->irq_out_sem, timeout);
	gpio_pin_interrupt_configure_dt(irq_out, GPIO_INT_DISABLE);

	return ret;
}

static inline void rfid_cr95hf_IRQ_IN_pulse(const struct gpio_dt_spec *irq_in)
{
	gpio_pin_set_dt(irq_in, 0);
	k_sleep(K_USEC(100)); /* t0 */
	gpio_pin_set_dt(irq_in, 1);
	k_sleep(K_USEC(10)); /* t1 */
	gpio_pin_set_dt(irq_in, 0);
	k_sleep(K_MSEC(11)); /* t3 */
}

/*
 * @brief Transceive data using SPI communication.
 *
 * This function handles sending and receiving data over SPI.
 * It can either write data, read data, or do nothing based on
 * the lengths of the send and receive buffers.
 * It also manages the chip select (CS) pin.
 */
static int rfid_cr95hf_transceive(const struct device *dev, bool release_cs)
{
	const struct rfid_cr95hf_spi_config *config = dev->config;
	struct rfid_cr95hf_data *data = dev->data;
	int err;

	LOG_DBG("snd_buffer[1]: %02X len: %02X", data->snd_buffer[1], data->spi_snd_buffer.len);

	if (data->spi_snd_buffer.len > 0 && data->spi_rcv_buffer.len > 0) {
		err = spi_transceive_dt(&config->spi, &data->spi_snd_buffer_arr,
					&data->spi_rcv_buffer_arr);
		if (err) {
			return err;
		}
	} else if (data->spi_snd_buffer.len > 0) {
		err = spi_write_dt(&config->spi, &data->spi_snd_buffer_arr);
		if (err) {
			return err;
		}
	} else if (data->spi_rcv_buffer.len > 0) {
		err = spi_transceive_dt(&config->spi, NULL, &data->spi_rcv_buffer_arr);
		if (err) {
			return err;
		}
	} else {
		/* Nothing to do */
	}

	if (release_cs) {
		spi_release_dt(&config->spi);
	}

	return 0;
}

/**
 * @brief Initialize the SPI configuration for the RFID CR95HF.
 *
 * This function checks if the SPI bus is ready and prepares the
 * RFID CR95HF device for communication.
 */
static int rfid_cr95hf_init_spi(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;
	const struct rfid_cr95hf_spi_config *config = dev->config;
	int err = 0;

	LOG_DBG("Initializing RFID CR95HF");

	k_mutex_init(&data->data_mutex);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s is not ready", config->spi.bus->name);
		return -ENODEV;
	}

	const struct gpio_dt_spec *irq_in = &config->irq_in;

	if (gpio_is_ready_dt(irq_in)) {
		err = gpio_pin_configure_dt(irq_in, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
			return err;
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		return -ENODEV;
	}

	if (config->irq_out.port) {
		const struct gpio_dt_spec *irq_out = &config->irq_out;

		if (!gpio_is_ready_dt(irq_out)) {
			LOG_ERR("%s: GPIO device not ready", dev->name);
			return -EBUSY;
		}

		err = gpio_pin_configure_dt(irq_out, GPIO_INPUT | GPIO_ACTIVE_LOW);
		if (err) {
			LOG_ERR("Failed to configure IRQ_OUT GPIO (err %d)", err);
			return err;
		}

		k_sem_init(&data->irq_out_sem, 0, 1);

		gpio_init_callback(&data->irq_callback, data->cb_handler, BIT(irq_out->pin));

		err = gpio_add_callback_dt(irq_out, &data->irq_callback);
		if (err) {
			LOG_ERR("Failed to add GPIO callback (err %d)", err);
			return err;
		}
	}

	rfid_cr95hf_IRQ_IN_pulse(irq_in);

	uint8_t tries = 5;
	uint8_t echo_resp = 0;

	do {
		/* Send Reset Command*/
		data->snd_buffer[0] = 0x01; /* SPI control Byte: Reset*/
		data->spi_snd_buffer.len = 1;
		data->spi_rcv_buffer.len = 0;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send reset command (err %d)", err);
			return err;
		}

		rfid_cr95hf_IRQ_IN_pulse(irq_in);

		/* Send Echo */
		data->snd_buffer[0] = 0x00; /* SPI control Byte: Send*/
		data->snd_buffer[1] = 0x55; /* CMD: Echo */
		data->spi_snd_buffer.len = 2;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send reset command (err %d)", err);
			return err;
		}

		/* Receive Echo*/
		data->snd_buffer[0] = 0x02; /* SPI control Byte: Send*/
		data->spi_snd_buffer.len = 1;
		data->spi_rcv_buffer.len = 2; /* Byte 0: Dummy Read; Byte 1: Echo Value*/

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to read echo (err %d)", err);
			return err;
		}

		echo_resp = data->rcv_buffer[1];

		LOG_DBG("Echo Response: %02X", echo_resp);
		tries--;
	} while (echo_resp != 0x55 && tries > 0);

	if (tries == 0 && echo_resp != 0x55) {
		LOG_ERR("Initialization Failed");
		return -EIO;
	}

	return 0;
}

static int rfid_cr95hf_reset(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;
	const struct rfid_cr95hf_spi_config *config = dev->config;
	int err = 0;
	const struct gpio_dt_spec *irq_in = &config->irq_in;

	rfid_cr95hf_IRQ_IN_pulse(irq_in);

	uint8_t tries = 5;
	uint8_t echo_resp = 0;

	do {
		/* Send Reset Command*/
		data->snd_buffer[0] = 0x01; /* SPI control Byte: Reset*/
		data->spi_snd_buffer.len = 1;
		data->spi_rcv_buffer.len = 0;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send reset command (err %d)", err);
			return err;
		}

		rfid_cr95hf_IRQ_IN_pulse(irq_in);

		/* Send Echo */
		data->snd_buffer[0] = 0x00; /* SPI control Byte: Send*/
		data->snd_buffer[1] = 0x55; /* CMD: Echo */
		data->spi_snd_buffer.len = 2;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send reset command (err %d)", err);
			return err;
		}

		/* Receive Echo*/
		data->snd_buffer[0] = 0x02; /* SPI control Byte: Send*/
		data->spi_snd_buffer.len = 1;
		data->spi_rcv_buffer.len = 2; /* Byte 0: Dummy Read; Byte 1: Echo Value*/

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to read echo (err %d)", err);
			return err;
		}

		echo_resp = data->rcv_buffer[1];

		LOG_DBG("Echo Response: %02X", echo_resp);
		tries--;
	} while (echo_resp != 0x55 && tries > 0);

	if (tries == 0 && echo_resp != 0x55) {
		LOG_ERR("Initialization Failed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Polls the CR95HF device for readiness to read or send data.
 *
 * This function sends a polling command to the device and checks
 * its status flags. It waits until the device is ready either for
 * reading or sending, based on the provided flags. If an error
 * occurs, it logs the error and returns the error code.
 *
 * @param dev The device structure representing the CR95HF device.
 * @param ready_read Flag indicating if the function should check
 * for readiness to read data.
 * @param ready_send Flag indicating if the function should check
 * for readiness to send data.
 * @return 0 on success or a negative error code on failure.
 */
static int rfid_cr95hf_polling(const struct device *dev, bool ready_read, bool ready_send,
			       k_timeout_t timeout)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;
	uint8_t received;
	int64_t start_time = k_uptime_get();
	int64_t timeout_ms;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = INT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timeout_ms = 0;
	} else {
		timeout_ms = k_ticks_to_ms_floor64(timeout.ticks);
	}

	data->snd_buffer[0] = 0x03; /* SPI control Byte: Poll */
	data->spi_snd_buffer.len = 1;
	data->spi_rcv_buffer.len = 0;

	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send poll command (err %d)", err);
		return err;
	}

	/* Read Flags */
	data->spi_snd_buffer.len = 0;
	data->spi_rcv_buffer.len = 1;

	while (1) {
		data->rcv_buffer[0] = 0;
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to read data (err %d)", err);
			return err;
		}

		received = data->rcv_buffer[0];

		LOG_DBG("Polling: Flags received: %02X", received);

		/* Check if read flag is requested and set */
		if (ready_read && (received & (CR95HF_READY_TO_READ))) {
			return 0;
		}

		/* Check if send flag is requested and set */
		if (ready_send && (received & (CR95HF_READY_TO_WRITE))) {
			return 0;
		}

		int64_t elapsed = k_uptime_get() - start_time;

		if (elapsed >= timeout_ms) {
			return -EAGAIN;
		}
	}
}

/**
 * @brief Blocks until the CR95HF device is ready for communication.
 *
 * If the IRQ_OUT pin is not configured, the function will actively poll the
 * device for readiness. If IRQ_OUT is configured, it will wait until the
 * IRQ_OUT pin is asserted low, indicating the device is ready.
 *
 * @param dev The device structure representing the CR95HF device.
 */
static int rfid_cr95hf_wait(const struct device *dev, k_timeout_t timeout)
{
	const struct rfid_cr95hf_spi_config *config = dev->config;
	struct rfid_cr95hf_data *data = dev->data;
	int ret = 0;

	k_sleep(K_MSEC(1));

	/* Function blocks until wakeup of CR95HF*/
	if (config->irq_out.port == NULL) {
		LOG_DBG("Polling");
		/* Active polling for readiness since irq_out is not configured */
		ret = rfid_cr95hf_polling(dev, 1, 1, timeout);
	} else {
		LOG_DBG("Sleeping");
		/* Wait for IRQ_OUT to be set low, indicating readiness */
		ret = rfid_cr95hf_wait_for_irq_out_low(&config->irq_out, data, timeout);
	}

	return ret;
}

/**
 * @brief Reads the response from the CR95HF device.
 *
 * This function sends a command to the CR95HF to read data and retrieves
 * the response code and the length of the data. If the length is greater
 * than the buffer size, it is limited to the buffer size. It then reads
 * the actual data based on the received length.
 *
 * @param dev The device structure representing the CR95HF device.
 * @return 0 on success or a negative error code on failure.
 */
static int rfid_cr95hf_response(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;
	size_t data_len;
	int err;

	data->snd_buffer[0] = 0x02; /* Set the control byte for reading */
	data->spi_snd_buffer.len = 1;
	/* Set Length to 3
	 * Byte 0: Don't Care -> received while sending
	 * Byte 1: Response Code;
	 * Byte 2: Len
	 */
	data->spi_rcv_buffer.len = 3;

	err = rfid_cr95hf_transceive(dev, false);
	if (err) {
		LOG_ERR("Failed to send read command (err %d)", err);
		return err;
	}

	data_len = data->rcv_buffer[2];

	LOG_DBG("Response Code: %02X Datalen %02X ", data->rcv_buffer[1], data_len);

	/* Read data from the CR95HF, limiting to buffer size */
	if (data_len > CR95HF_RCV_BUF_SIZE) {
		data_len = CR95HF_RCV_BUF_SIZE;
	}

	data->spi_snd_buffer.len = 0;
	data->spi_rcv_buffer.len = data_len;

	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to read data (err %d)", err);
		return err;
	}

	return 0;
}

/**
 * @brief Sets the device to tag detector mode
 *
 * The device is in sleep mode until a tag is detected.
 * If timeout is set to K_FOREVER, this function blocks until device wakes up.
 * Wake up is detected by IRQ_OUT or if not configured by polling
 */
static int rfid_cr95hf_tag_detector(const struct device *dev, k_timeout_t timeout)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;
	uint8_t tag_detector_msg[17] = CR95HF_WFE_TAG_DETECTOR_ARRAY;

	memcpy(data->snd_buffer, tag_detector_msg, 17);
	data->spi_snd_buffer.len = 17;
	data->spi_rcv_buffer.len = 0;

	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send tag detector command (err %d)", err);
		return err;
	}

	/* Block until the CR95HF device has woken up */
	err = rfid_cr95hf_wait(dev, timeout);
	if (err) {
		return err;
	}

	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after wakeup (err %d)", err);
		return err;
	}

	if (data->spi_rcv_buffer.len == 0) {
		return -EAGAIN;
	}
	return 0;
}

static int rfid_cr95hf_claim(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;

	return k_mutex_lock(&data->data_mutex, K_FOREVER);
}

static int rfid_cr95hf_release(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;

	return k_mutex_unlock(&data->data_mutex);
}

static int rfid_cr95hf_get_property(const struct device *dev, struct rfid_property *prop)
{
	return -ENOTSUP;
}

static int rfid_cr95hf_get_properties(const struct device *dev, struct rfid_property *props,
				      size_t props_len)
{
	__ASSERT_NO_MSG(props != NULL);

	for (size_t index = 0U; index < props_len; ++index) {
		props[index].status = rfid_cr95hf_get_property(dev, &props[index]);
	}

	return 0;
}

static int rfid_cr95hf_set_property(const struct device *dev, struct rfid_property *prop)
{
	struct rfid_cr95hf_data *data = dev->data;

	switch (prop->type) {
	case RFID_PROP_HW_TX_CRC:
		data->hw_tx_crc = prop->hw_tx_crc;
		return 0;

	case RFID_PROP_TIMEOUT:
		data->timeout_us = prop->timeout_us;
		return 0;

	case RFID_PROP_SLEEP:
		k_timeout_t tout;

		if (prop->timeout_us == UINT32_MAX) {
			tout = K_FOREVER;
		} else {
			tout = K_USEC(prop->timeout_us);
		}

		return rfid_cr95hf_tag_detector(dev, tout);

	case RFID_PROP_RESET:
		return rfid_cr95hf_reset(dev);

	default:
		break;
	}

	return -ENOTSUP;
}

static int rfid_cr95hf_set_properties(const struct device *dev, struct rfid_property *props,
				      size_t props_len)
{
	__ASSERT_NO_MSG(props != NULL);

	for (size_t index = 0; index < props_len; ++index) {
		props[index].status = rfid_cr95hf_set_property(dev, &props[index]);
	}

	return 0;
}

static int rfid_cr95hf_load_protocol(const struct device *dev, rfid_proto_t proto, rfid_mode_t mode)
{
	struct rfid_cr95hf_data *data = dev->data;
	uint8_t rf_tx = CR95HF_ISO_14443_106_KBPS;
	uint8_t rf_rx = CR95HF_ISO_14443_106_KBPS;
	int err;

	/* Only initiator mode is supported */
	if ((mode & RFID_MODE_ROLE_MASK) != RFID_MODE_INITIATOR) {
		return -EINVAL;
	}

	/* Only one data rate could be set*/
	if (count_bits(mode & RFID_MODE_TX_MASK) > 1U ||
	    count_bits(mode & RFID_MODE_RX_MASK) > 1U) {
		return -EINVAL;
	}

	if (proto == RFID_PROTO_ISO14443A) {
		if ((mode & RFID_MODE_TX_MASK) != 0U) {
			switch (mode & RFID_MODE_TX_MASK) {
			case RFID_MODE_TX_106:
				rf_tx = CR95HF_ISO_14443_106_KBPS;
				break;
			case RFID_MODE_TX_212:
				rf_tx = CR95HF_ISO_14443_212_KBPS;
				break;
			case RFID_MODE_TX_424:
				rf_tx = CR95HF_ISO_14443_424_KBPS;
				break;
			default:
				return -EINVAL;
			}
		}

		if ((mode & RFID_MODE_RX_MASK) != 0U) {
			switch (mode & RFID_MODE_RX_MASK) {
			case RFID_MODE_RX_106:
				rf_rx = CR95HF_ISO_14443_106_KBPS;
				break;
			case RFID_MODE_RX_212:
				rf_rx = CR95HF_ISO_14443_212_KBPS;
				break;
			case RFID_MODE_RX_424:
				rf_rx = CR95HF_ISO_14443_424_KBPS;
				break;
			default:
				return -EINVAL;
			}
		}

		/* Prepare the command for switching the protocol */
		uint8_t protocol_msg[7] = {
			0, 2, 4, 2, ((rf_tx & 0x03) << 6) | ((rf_rx & 0x03) << 4), 1, 0x80};

		memcpy(data->snd_buffer, protocol_msg, 7);
		data->spi_snd_buffer.len = 7;
		data->spi_rcv_buffer.len = 0;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send protocol select command (err %d)", err);
			return err;
		}

		err = rfid_cr95hf_wait(dev, K_MSEC(1));
		if (err) {
			return err;
		}

		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after protocol select command (err %d)",
				err);
			return err;
		}

	} else {
		return -EINVAL;
	}

	return 0;
}

static int rfid_cr95hf_im_transceive(const struct device *dev, const uint8_t *tx_data,
				     uint16_t tx_len, uint8_t tx_last_bits, uint8_t *rx_data,
				     uint16_t *rx_len)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;

	data->snd_buffer[0] = 0x00; /* Control Byte: Send */
	data->snd_buffer[1] = 0x04; /* cmd: SendRecev */
	data->snd_buffer[2] = tx_len + 1;
	data->snd_buffer[tx_len + 3] = tx_last_bits | (data->hw_tx_crc ? 0x20 : 0x00);
	memcpy(data->snd_buffer + 3, tx_data, tx_len);
	data->spi_snd_buffer.len = tx_len + 4;
	data->spi_rcv_buffer.len = 0;

	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send data (err %d)", err);
		return err;
	}

	err = rfid_cr95hf_wait(dev, K_USEC(data->timeout_us));
	if (err) {
		return err;
	}

	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response (err %d)", err);
		return err;
	}

	if (data->spi_rcv_buffer.len > 3) {
		data->spi_rcv_buffer.len -= 3;
	}
	*rx_len = data->spi_rcv_buffer.len;
	memcpy(rx_data, data->rcv_buffer, data->spi_rcv_buffer.len);

	return 0;
}

static rfid_proto_t rfid_cr95hf_supported_protocols(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RFID_PROTO_ISO14443A;
}

static rfid_mode_t rfid_cr95hf_supported_modes(const struct device *dev, rfid_proto_t proto)
{
	ARG_UNUSED(dev);

	if (proto == RFID_PROTO_ISO14443A) {
		return RFID_MODE_INITIATOR | RFID_MODE_RX_106 | RFID_MODE_TX_106 |
		       RFID_MODE_RX_212 | RFID_MODE_TX_212 | RFID_MODE_RX_424 | RFID_MODE_TX_424;
	}

	return 0;
}

static DEVICE_API(rfid, rfid_cr95hf_api) = {
	.claim = rfid_cr95hf_claim,
	.release = rfid_cr95hf_release,
	.load_protocol = rfid_cr95hf_load_protocol,
	.get_properties = rfid_cr95hf_get_properties,
	.set_properties = rfid_cr95hf_set_properties,
	.im_transceive = rfid_cr95hf_im_transceive,
	.tm_transmit = NULL,
	.tm_receive = NULL,
	.listen = NULL,
	.supported_protocols = rfid_cr95hf_supported_protocols,
	.supported_modes = rfid_cr95hf_supported_modes,
};

/* Define to initialize RFID device using SPI communication */
#define RFID_DEVICE_SPI(n)                                                                         \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n;                                   \
	static void rfid_cr95hf_irq_out_callback_##n(                                              \
		const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)        \
	{                                                                                          \
		k_sem_give(&rfid_device_prv_data_##n.irq_out_sem);                                 \
	}                                                                                          \
	static const struct rfid_cr95hf_spi_config rfid_device_prv_config_##n = {                  \
		.spi = SPI_DT_SPEC_INST_GET(n,                                                     \
					    SPI_OP_MODE_MASTER | SPI_WORD_SET(8U) |                \
						    SPI_TRANSFER_MSB | SPI_HOLD_ON_CS |            \
						    SPI_LOCK_ON),                                  \
		.irq_in = GPIO_DT_SPEC_INST_GET(n, irq_in_gpios),                                  \
		.irq_out = GPIO_DT_SPEC_INST_GET_OR(n, irq_out_gpios, {0}),                        \
	};                                                                                         \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n = {                                \
		.spi_snd_buffer.buf = rfid_device_prv_data_##n.snd_buffer,                         \
		.spi_rcv_buffer.buf = rfid_device_prv_data_##n.rcv_buffer,                         \
		.spi_snd_buffer_arr.buffers = &rfid_device_prv_data_##n.spi_snd_buffer,            \
		.spi_rcv_buffer_arr.buffers = &rfid_device_prv_data_##n.spi_rcv_buffer,            \
		.spi_snd_buffer_arr.count = 1,                                                     \
		.spi_rcv_buffer_arr.count = 1,                                                     \
		.cb_handler = rfid_cr95hf_irq_out_callback_##n,                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, rfid_cr95hf_init_spi, NULL, &rfid_device_prv_data_##n,            \
			      &rfid_device_prv_config_##n, POST_KERNEL, CONFIG_RFID_INIT_PRIORITY, \
			      &rfid_cr95hf_api);

#define RFID_DEVICE_UART(n) "To be implemented"

#define CR95HF_DEFINE(n)                                                                           \
	COND_CODE_1(DT_INST_ON_BUS(n, spi),	\
		    (RFID_DEVICE_SPI(n)),	\
		    (RFID_DEVICE_UART(n)))

DT_INST_FOREACH_STATUS_OKAY(CR95HF_DEFINE);
