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


#if DT_ON_BUS(DT_NODELABEL(cr95hf), spi)
struct rfid_cr95hf_spi_config {
	const struct spi_dt_spec spi;
	const struct gpio_dt_spec irq_in;
	const struct gpio_dt_spec irq_out;
	const struct gpio_dt_spec cs;
};

struct rfid_cr95hf_data {
	enum rfid_mode current_mode;
	uint64_t cm_timestamp;
	struct gpio_callback irq_callback;
	struct spi_buf spi_snd_buffer;
	struct spi_buf spi_rcv_buffer;
	struct spi_buf_set spi_snd_buffer_arr;
	struct spi_buf_set spi_rcv_buffer_arr;
	uint8_t rcv_buffer[CR95HF_RCV_BUF_SIZE];
	uint8_t snd_buffer[CR95HF_SND_BUF_SIZE];
	struct k_sem irq_out_sem;
	gpio_callback_handler_t cb_handler;
};
#endif

/**
 * @brief Set the current operating mode of the RFID reader.
 *
 * This function updates the mode and records the current timestamp.
 *
 * @param data Pointer to the RFID data structure.
 * @param mode The new operating mode to set.
 */
static inline void rfid_cr95hf_setmode(struct rfid_cr95hf_data *data, enum rfid_mode mode)
{
	data->current_mode = mode;
	data->cm_timestamp = k_uptime_get();
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
static void rfid_cr95hf_wait_for_irq_out_low(const struct gpio_dt_spec *irq_out,
					     struct rfid_cr95hf_data *data)
{
	if (gpio_pin_get_dt(irq_out)) {
		return;  /* Already low, no need to wait */
	}

	gpio_pin_interrupt_configure_dt(irq_out, GPIO_INT_EDGE_TO_ACTIVE);
	k_sem_take(&data->irq_out_sem, K_FOREVER);
	gpio_pin_interrupt_configure_dt(irq_out, GPIO_INT_DISABLE);
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

	gpio_pin_set_dt(&config->cs, 1);  /* Set CS pin low */
	k_sleep(K_MSEC(1));

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
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&config->cs, 0); /* Set CS pin high */
	}
	k_sleep(K_MSEC(1));

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

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s is not ready", config->spi.bus->name);
		return -ENODEV;
	}

	const struct gpio_dt_spec *cs = &config->cs;

	if (gpio_is_ready_dt(cs)) {
		err = gpio_pin_configure_dt(cs, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
			return err;
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", dev->name);
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

		gpio_init_callback(&data->irq_callback, data->cb_handler,
				   BIT(irq_out->pin));

		err = gpio_add_callback_dt(irq_out, &data->irq_callback);
		if (err) {
			LOG_ERR("Failed to add GPIO callback (err %d)", err);
			return err;
		}
	}

	rfid_cr95hf_IRQ_IN_pulse(irq_in);

	uint8_t tries = 5;

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

		LOG_DBG("Echo Response: %02X", data->rcv_buffer[1]);
		tries--;
	} while (data->rcv_buffer[1] != 0x55 && tries > 0);

	if (tries == 0 && data->rcv_buffer[1] != 0x55) {
		rfid_cr95hf_setmode(data, UNINITIALIZED);
		LOG_ERR("Initialization Failed");
		return -EIO;
	}

	rfid_cr95hf_setmode(data, POWER_UP);
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
static int rfid_cr95hf_polling(const struct device *dev, bool ready_read, bool ready_send)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;

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
		LOG_DBG("Polling: Flags received: %02X", data->rcv_buffer[0]);

		/* Check if read flag is requested and set */
		if (ready_read && (data->rcv_buffer[0] & (CR95HF_READY_TO_READ))) {
			return 0;
		}

		/* Check if send flag is requested and set */
		if (ready_send && (data->rcv_buffer[0] & (CR95HF_READY_TO_WRITE))) {
			return 0;
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
static void rfid_cr95hf_wait(const struct device *dev)
{
	const struct rfid_cr95hf_spi_config *config = dev->config;
	struct rfid_cr95hf_data *data = dev->data;

	/* Function blocks until wakeup of CR95HF*/
	if (config->irq_out.port == NULL) {
		LOG_DBG("Polling");
		/* Active polling for readiness since irq_out is not configured */
		rfid_cr95hf_polling(dev, 1, 1);
	} else {
		LOG_DBG("Sleeping");
		/* Wait for IRQ_OUT to be set low, indicating readiness */
		rfid_cr95hf_wait_for_irq_out_low(&config->irq_out, data);
	}
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
	uint8_t response_code;
	int err;

	data->snd_buffer[0] = 0x02;  /* Set the control byte for reading */
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

	response_code = data->rcv_buffer[1];
	data_len = data->rcv_buffer[2];

	LOG_DBG("Response Code: %02X Datalen %02X ", response_code, data_len);

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
 * @brief Switches the CR95HF device to the requested mode.
 *
 * This function sets the CR95HF device to a specific operational mode,
 * while handling transitions from other modes as necessary. It includes
 * a waiting mechanism to ensure the device has stabilized before changing
 * modes. Supported modes include TAG_DETECTOR.
 *
 * @param dev The device structure representing the CR95HF device.
 * @param req_mode The requested mode to switch to.
 * @return 0 on success or a negative error code on failure.
 */
int rfid_cr95hf_select_mode(const struct device *dev, enum rfid_mode req_mode)
{
	struct rfid_cr95hf_data *data = dev->data;
	const struct rfid_cr95hf_spi_config *config = dev->config;
	const struct gpio_dt_spec *irq_in = &config->irq_in;
	enum rfid_mode current_mode = data->current_mode;
	int err;

	if (req_mode == current_mode) {
		LOG_DBG("Nothing to do: Requested Mode = current Mode");
		return 0;
	}

	if (req_mode >= INVALID) {
		LOG_ERR("Invalid Mode requested");
		return -EINVAL;
	}

	/* Wait if the last state switch is lower than 10ms */
	if (data->cm_timestamp < 10) {
		k_sleep(K_MSEC(10 - data->cm_timestamp));
	}

	/* Transition to READY STATE if not already in that state */
	if (current_mode != READY) {
		if (current_mode == READER) {
			/* Todo: Send serial command to switch to READY */
			LOG_ERR("Leaving READER state is not implemented, yet");
		} else {
			/* Set IRQ_IN to 0 for at least 10us (t1) */
			gpio_pin_set_dt(irq_in, 1);
			k_sleep(K_USEC(10));
			gpio_pin_set_dt(irq_in, 0);
			rfid_cr95hf_setmode(data, READY);
		}
	}

	/* Wait 10ms (t3) to stabilize before switching modes */
	k_sleep(K_MSEC(10));

	switch (req_mode) {
	case TAG_DETECTOR:
		uint8_t tag_detector_msg[17] = CR95HF_WFE_TAG_DETECTOR_ARRAY;

		memcpy(data->snd_buffer, tag_detector_msg, 17);
		data->spi_snd_buffer.len = 17;
		data->spi_rcv_buffer.len = 0;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send tag detector command (err %d)", err);
			return err;
		}

		rfid_cr95hf_setmode(data, TAG_DETECTOR);

		/* Block until the CR95HF device has woken up */
		rfid_cr95hf_wait(dev);

		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after wakeup (err %d)", err);
			return err;
		}

		rfid_cr95hf_setmode(data, READY);

		break;
	default:
		LOG_ERR("Requested Mode not implemented");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Retrieves the UID of an ISO 14443A card using the anticollision protocol.
 *
 * This function executes the necessary commands to communicate with an ISO 14443A card
 * and read its Unique Identifier (UID). It handles the anticollision procedure to obtain
 * the UID in cases where the card has multiple parts.
 *
 * @param dev The device structure representing the CR95HF device.
 * @param uid Pointer to a buffer that will store the retrieved UID.
 * @param uid_len Pointer to a variable that indicates the length of the UID buffer.
 * @return 0 on success or a negative error code on failure.
 */
int rfid_cr95hf_iso14443A_get_uid(const struct device *dev, uint8_t *uid, size_t *uid_len)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;
	uint8_t sak_byte;

	 /* Check if the UID buffer is of sufficient size */
	if (*uid_len < 10 || uid == NULL) {
		LOG_ERR("UID Buffer too short. Please provide at least 10 Unsigned Bytes");
		return -EINVAL;
	}

	/* Send REQA command to initiate communication with the card */
	data->snd_buffer[0] = 0x00; /* Control Byte: Send */
	data->snd_buffer[1] = 0x04; /* cmd: SendRecev */
	data->snd_buffer[2] = 0x02; /* Data Length */
	data->snd_buffer[3] = 0x26; /*  */
	data->snd_buffer[4] = 0x07; /*  */
	data->spi_snd_buffer.len = 5;
	data->spi_rcv_buffer.len = 0;
	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send REQA (err %d)", err);
		return err;
	}

	rfid_cr95hf_wait(dev);

	 /* Wait for response (ATQA) after sending REQA */
	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after REQA (err %d)", err);
		return err;
	}

	/* Send SEL_CL1 command to read UID */
	data->snd_buffer[0] = 0x00; /* Control Byte: Send */
	data->snd_buffer[1] = 0x04; /* cmd: SendRecev */
	data->snd_buffer[2] = 0x03; /* Data Length */
	data->snd_buffer[3] = 0x93; /*  */
	data->snd_buffer[4] = 0x20; /*  */
	data->snd_buffer[5] = 0x08; /*  */
	data->spi_snd_buffer.len = 6;
	data->spi_rcv_buffer.len = 0;
	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send SEL_CL1 (err %d)", err);
		return err;
	}

	rfid_cr95hf_wait(dev);

	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after SEL_CL1 (err %d)", err);
		return err;
	}

	if (data->rcv_buffer[0] == 0x88)	{
		uid[0] = data->rcv_buffer[1];
		uid[1] = data->rcv_buffer[2];
		uid[2] = data->rcv_buffer[3];
	} else {
		uid[0] = data->rcv_buffer[0];
		uid[1] = data->rcv_buffer[1];
		uid[2] = data->rcv_buffer[2];
		uid[3] = data->rcv_buffer[3];
		*uid_len = 4;
	}

	/* Send SEL_CL1 complete command */
	data->snd_buffer[0] = 0x00; /* Control Byte: Send */
	data->snd_buffer[1] = 0x04; /* cmd: SendRecev */
	data->snd_buffer[2] = 0x08; /* Data Length */
	data->snd_buffer[3] = 0x93; /*  */
	data->snd_buffer[4] = 0x70; /*  */
	data->snd_buffer[5] = data->rcv_buffer[0]; /*  */
	data->snd_buffer[6] = data->rcv_buffer[1]; /*  */
	data->snd_buffer[7] = data->rcv_buffer[2]; /*  */
	data->snd_buffer[8] = data->rcv_buffer[3]; /*  */
	data->snd_buffer[9] = data->rcv_buffer[4]; /*  */
	data->snd_buffer[10] = 0x28;

	data->spi_snd_buffer.len = 11;
	data->spi_rcv_buffer.len = 0;
	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send SEL_CL1 complete (err %d)", err);
		return err;
	}

	rfid_cr95hf_wait(dev);

	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after SEL_CL1 complete (err %d)", err);
		return err;
	}

	sak_byte = data->rcv_buffer[0];

	/* Check if more UID data can be read */
	if (sak_byte & 0x04) {
		data->snd_buffer[0] = 0x00;  /* Control Byte: Send */
		data->snd_buffer[1] = 0x04;  /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x03;  /* Data Length */
		data->snd_buffer[3] = 0x95;  /* SEL_CL2 command */
		data->snd_buffer[4] = 0x20;
		data->snd_buffer[5] = 0x08;
		data->spi_snd_buffer.len = 6;
		data->spi_rcv_buffer.len = 0;
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send SEL_CL2 (err %d)", err);
			return err;
		}

		rfid_cr95hf_wait(dev);

		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL2 (err %d)", err);
			return err;
		}

		if (data->rcv_buffer[0] == 0x88) {
			uid[3] = data->rcv_buffer[1];
			uid[4] = data->rcv_buffer[2];
			uid[5] = data->rcv_buffer[3];
		} else {
			uid[3] = data->rcv_buffer[0];
			uid[4] = data->rcv_buffer[1];
			uid[5] = data->rcv_buffer[2];
			uid[6] = data->rcv_buffer[3];
			*uid_len = 7;
		}

		data->snd_buffer[0] = 0x00;  /* Control Byte: Send */
		data->snd_buffer[1] = 0x04;  /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x08;  /* Data Length */
		data->snd_buffer[3] = 0x95;  /* SEL_CL2 complete command */
		data->snd_buffer[4] = 0x70;
		data->snd_buffer[5] = data->rcv_buffer[0];
		data->snd_buffer[6] = data->rcv_buffer[1];
		data->snd_buffer[7] = data->rcv_buffer[2];
		data->snd_buffer[8] = data->rcv_buffer[3];
		data->snd_buffer[9] = data->rcv_buffer[4];
		data->snd_buffer[10] = 0x28;  /* Termination byte */

		data->spi_snd_buffer.len = 11;
		data->spi_rcv_buffer.len = 0;
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send SEL_CL2 complete (err %d)", err);
			return err;
		}

		rfid_cr95hf_wait(dev);
		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL2 complete (err %d)", err);
			return err;
		}

		sak_byte = data->rcv_buffer[0];
	}

	if (sak_byte & 0x04) {
		data->snd_buffer[0] = 0x00; /* Control Byte: Send */
		data->snd_buffer[1] = 0x04; /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x03; /* Data Length */
		data->snd_buffer[3] = 0x97; /* Command for reading next UID part */
		data->snd_buffer[4] = 0x20;
		data->snd_buffer[5] = 0x08;
		data->spi_snd_buffer.len = 6;
		data->spi_rcv_buffer.len = 0;
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send SEL_CL3 (err %d)", err);
			return err;
		}

		rfid_cr95hf_wait(dev);
		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL3 (err %d)", err);
			return err;
		}

		uid[6] = data->rcv_buffer[0];
		uid[7] = data->rcv_buffer[1];
		uid[8] = data->rcv_buffer[2];
		uid[9] = data->rcv_buffer[3];
		*uid_len = 10;
		data->snd_buffer[0] = 0x00; /* Control Byte: Send */
		data->snd_buffer[1] = 0x04; /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x08; /* Data Length */
		data->snd_buffer[3] = 0x97; /* Complete command */
		data->snd_buffer[4] = 0x70;
		data->snd_buffer[5] = data->rcv_buffer[0];
		data->snd_buffer[6] = data->rcv_buffer[1];
		data->snd_buffer[7] = data->rcv_buffer[2];
		data->snd_buffer[8] = data->rcv_buffer[3];
		data->snd_buffer[9] = data->rcv_buffer[4];
		data->snd_buffer[10] = 0x28;  /* Termination byte */

		data->spi_snd_buffer.len = 11;
		data->spi_rcv_buffer.len = 0;
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send SEL_CL3 complete (err %d)", err);
			return err;
		}

		rfid_cr95hf_wait(dev);
		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL3 complete (err %d)", err);
			return err;
		}

		sak_byte = data->snd_buffer[0];
	}

	if (*uid_len == 4) {
		/* Log 4-byte UID */
		LOG_DBG("UID: %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3]);
	} else if (*uid_len == 7) {
		/* Log 7-byte UID */
		LOG_DBG("UID: %02X %02X %02X %02X %02X %02X %02X", uid[0], uid[1], uid[2], uid[3],
			uid[4], uid[5], uid[6]);
	} else {
		/* Log 10-byte UID */
		LOG_DBG("UID: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", uid[0], uid[1],
			uid[2], uid[3], uid[4], uid[5], uid[6], uid[7], uid[8], uid[9]);
	}

	return 0;  /* Return success */
}

/**
 * @brief Selects the RFID protocol for communication.
 *
 * This function configures the RFID reader to use the specified protocol,
 * such as ISO_14443A. It sends the necessary command to the device and waits
 * for a response to acknowledge the protocol switch.
 */
int rfid_cr95hf_protocol_select(const struct device *dev, enum rfid_protocol proto)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;

	switch (proto) {
	case ISO_14443A:
		/* Prepare the command for switching the protocol */
		uint8_t protocol_msg[7] = CR95HF_SELECT_14443_A_ARRAY;

		memcpy(data->snd_buffer, protocol_msg, 7);
		data->spi_snd_buffer.len = 7;
		data->spi_rcv_buffer.len = 0;

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send protocol select command (err %d)", err);
			return err;
		}

		rfid_cr95hf_wait(dev);
		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after protocol select command (err %d)",
				err);
			return err;
		}

		break;
	default:
		LOG_ERR("The selected protocol is not supported");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Transmits data to the RFID device and receives a response.
 *
 * This function is responsible for sending a buffer of data to the RFID device and receiving
 * a response buffer back. It also manages the lengths of the transmitted and received data.
 */
int rfid_cr95hf_transceive_api(const struct device *dev, struct transceive_data data)
{
	LOG_ERR("Function not implemented");
	return -EPERM;
}


static DEVICE_API(rfid, rfid_cr95hf_api) = {
	.select_mode = rfid_cr95hf_select_mode,
	.protocol_select = rfid_cr95hf_protocol_select,
	.get_uid = rfid_cr95hf_iso14443A_get_uid,
	.transceive = NULL,
};

 /* Define to initialize RFID device using SPI communication */
#define RFID_DEVICE_SPI(n)                                                                     \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n;                               \
	static void rfid_cr95hf_irq_out_callback_##n(                                          \
		const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)    \
	{                                                                                      \
		k_sem_give(&rfid_device_prv_data_##n.irq_out_sem);                             \
	}                                                                                      \
	static const struct rfid_cr95hf_spi_config rfid_device_prv_config_##n = {              \
		.spi = SPI_DT_SPEC_INST_GET(                                                   \
			n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U) | SPI_TRANSFER_MSB, 0U),      \
		.cs = GPIO_DT_SPEC_GET(DT_DRV_INST(n), cs_gpios),                              \
		.irq_in = GPIO_DT_SPEC_GET(DT_DRV_INST(n), irq_in_gpios),                      \
		.irq_out = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), irq_out_gpios),        \
			(GPIO_DT_SPEC_GET(DT_DRV_INST(n), irq_out_gpios)), ({0})),             \
	};                                                                                     \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n = {                            \
		.current_mode = UNINITIALIZED,                                                 \
		.spi_snd_buffer.buf = rfid_device_prv_data_##n.snd_buffer,                     \
		.spi_rcv_buffer.buf = rfid_device_prv_data_##n.rcv_buffer,                     \
		.spi_snd_buffer_arr.buffers = &rfid_device_prv_data_##n.spi_snd_buffer,        \
		.spi_rcv_buffer_arr.buffers = &rfid_device_prv_data_##n.spi_rcv_buffer,        \
		.spi_snd_buffer_arr.count = 1,                                                 \
		.spi_rcv_buffer_arr.count = 1,                                                 \
		.cb_handler = rfid_cr95hf_irq_out_callback_##n,                                \
	};                                                                                     \
	DEVICE_DT_INST_DEFINE(n, rfid_cr95hf_init_spi, NULL, &rfid_device_prv_data_##n,        \
			      &rfid_device_prv_config_##n, POST_KERNEL, CONFIG_RFID_INIT_PRIORITY, \
			      &rfid_cr95hf_api);

#define RFID_DEVICE_UART(n) "To be implemented"

#define CR95HF_DEFINE(n)                        \
	COND_CODE_1(DT_INST_ON_BUS(n, spi),	\
		    (RFID_DEVICE_SPI(n)),	\
		    (RFID_DEVICE_UART(n)))

DT_INST_FOREACH_STATUS_OKAY(CR95HF_DEFINE);
