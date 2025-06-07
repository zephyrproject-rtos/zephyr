/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT   st_cr95hf
#define LOG_MODULE_NAME rfid

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_RFID_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rfid.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "rfid_cr95hf.h"


#if defined(CONFIG_RFID_USE_UART)
#error "UART Communication is not yet implemented"
#endif

#if defined(CONFIG_RFID_USE_SPI)
struct rfid_cr95hf_spi_config {
	const struct spi_dt_spec spi;       /* SPI device specification */
	const struct gpio_dt_spec *irq_in;  /* GPIO specification for interrupt input */
	const struct gpio_dt_spec *irq_out; /* GPIO specification for interrupt output */
	const struct gpio_dt_spec *cs;      /* GPIO specification for chip select */
};

struct rfid_cr95hf_data {
	rfid_mode_t current_mode;           /* Current operating mode of the RFID reader */
	uint64_t cm_timestamp;              /* Timestamp for the current mode */
	uint8_t tag_detector_msg[17];       /* Message buffer for tag detection */
	uint8_t protocol_msg[13];           /* Message buffer for the protocol */
	uint8_t protocol_msg_len;           /* Length of the protocol message */
	struct gpio_callback irq_callback;  /* GPIO callback structure for interrupts */
	struct spi_buf spi_snd_buffer;      /* Buffer for SPI send operations */
	struct spi_buf spi_rcv_buffer;      /* Buffer for SPI receive operations */
	struct spi_buf_set spi_snd_buffer_arr; /* Set of SPI send buffers */
	struct spi_buf_set spi_rcv_buffer_arr; /* Set of SPI receive buffers */
	uint8_t rcv_buffer[CR95HF_RCV_BUF_SIZE]; /* Buffer for received data */
	uint8_t snd_buffer[CR95HF_SND_BUF_SIZE]; /* Buffer for data to send */
	struct k_sem irq_out_sem;            /* Semaphore to wait for IRQ_OUT to be low */
	gpio_callback_handler_t cb_handler;  /* Handler for the GPIO callback */
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
static inline void rfid_cr95hf_setmode(struct rfid_cr95hf_data *data, rfid_mode_t mode)
{
	/* Update current operating mode */
	data->current_mode = mode;
	/* Record the current uptime */
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

	/* Configure interrupt for active edge */
	gpio_pin_interrupt_configure_dt(irq_out, GPIO_INT_EDGE_TO_ACTIVE);
	/* Wait until the semaphore is given by the interrupt */
	k_sem_take(&data->irq_out_sem, K_FOREVER);
	/* Disable interrupt */
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

	gpio_pin_set_dt(config->cs, 1);  /* Set CS pin low */
	k_sleep(K_MSEC(1));

	if (data->spi_snd_buffer.len > 0 && data->spi_rcv_buffer.len > 0) {
		/* Transceive */
		err = spi_transceive_dt(&config->spi, &data->spi_snd_buffer_arr,
					&data->spi_rcv_buffer_arr);
		if (err) {
			return err;
		}
	} else if (data->spi_snd_buffer.len > 0) {
		/* Write */
		err = spi_write_dt(&config->spi, &data->spi_snd_buffer_arr);
		if (err) {
			return err;
		}
	} else if (data->spi_rcv_buffer.len > 0) {
		/* Read */
		err = spi_transceive_dt(&config->spi, NULL, &data->spi_rcv_buffer_arr);
		if (err) {
			return err;
		}
	} else {
		/* Nothing to do */
	}

	if (release_cs) {
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(config->cs, 0); /* Set CS pin high */
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

	/* Check if SPI is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s is not ready", config->spi.bus->name);
		return -ENODEV;
	}

	/* Check if cs is ready*/
	const struct gpio_dt_spec *cs = config->cs;

	if (device_is_ready(cs->port)) {
		err = gpio_pin_configure_dt(cs, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		err = -ENODEV;
	}

	/* Check if IRQ_IN is ready*/
	const struct gpio_dt_spec *irq_in = config->irq_in;

	if (device_is_ready(irq_in->port)) {
		err = gpio_pin_configure_dt(irq_in, GPIO_OUTPUT_INACTIVE);
		if (err) {
			LOG_ERR("Cannot configure GPIO (err %d)", err);
		}
	} else {
		LOG_ERR("%s: GPIO device not ready", dev->name);
		err = -ENODEV;
	}

	if (config->irq_out) {
		const struct gpio_dt_spec *irq_out = config->irq_out;

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

		/* Connect the callback function to the interrupt */
		gpio_init_callback((struct gpio_callback *)&data->irq_callback, data->cb_handler,
				   BIT(irq_out->pin));

		/* Add callback to the specified GPIO device */
		err = gpio_add_callback_dt(irq_out, (struct gpio_callback *)&data->irq_callback);
		if (err) {
			LOG_ERR("Failed to add GPIO callback (err %d)", err);
			return err;
		}
	}

	rfid_cr95hf_IRQ_IN_pulse(irq_in);
	spi_release_dt(&config->spi);
	k_sleep(K_MSEC(1));

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

		/* Set CS pin high to end the SPI transfer */
		k_sleep(K_MSEC(1));
		spi_release_dt(&config->spi);
		k_sleep(K_MSEC(3));
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
		k_sleep(K_MSEC(1));
		spi_release_dt(&config->spi);
		k_sleep(K_MSEC(3));

		/* Receive Echo*/
		data->snd_buffer[0] = 0x02; /* SPI control Byte: Send*/
		data->spi_snd_buffer.len = 1;
		data->spi_rcv_buffer.len = 2; /* Byte 0: Dummy Read; Byte 1: Echo Value*/

		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to read echo (err %d)", err);
			return err;
		}

		k_sleep(K_MSEC(1));
		spi_release_dt(&config->spi);
		k_sleep(K_MSEC(3));
		LOG_DBG("Echo Response: %02X", data->rcv_buffer[1]);
		tries--;
	} while (data->rcv_buffer[1] != 0x55 && tries > 0);

	if (tries == 0 && data->rcv_buffer[1] != 0x55) {
		/* Initialization failed */
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
	uint8_t *flags;

	data->snd_buffer[0] = 0x03; /* SPI control Byte: Poll */
	data->spi_snd_buffer.len = 1;
	data->spi_rcv_buffer.len = 0;

	/* Send Control Byte for polling */
	err = rfid_cr95hf_transceive(dev, true);
	if (err) {
		LOG_ERR("Failed to send poll command (err %d)", err);
		return err;
	}

	/* Read Flags */
	data->spi_snd_buffer.len = 0;
	data->spi_rcv_buffer.len = 1;
	flags = &data->rcv_buffer[0];
	while (1) {
		*flags = 0;
		err = rfid_cr95hf_transceive(dev, true); /* Read one byte */
		if (err) {
			LOG_ERR("Failed to read data (err %d)", err);
			return err;
		}
		LOG_DBG("Polling: Flags received: %d", *flags);
		/* Check if read flag is requested and set */
		if (ready_read && (*flags & (0x8))) {
			return 0;
		}
		/* Check if send flag is requested and set */
		if (ready_send && (*flags & (0x4))) {
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
	if (config->irq_out == NULL) {
		LOG_DBG("Polling");
		/* Active polling for readiness since irq_out is not configured */
		rfid_cr95hf_polling(dev, 1, 1);
	} else {
		LOG_DBG("Sleeping");
		/* Wait for IRQ_OUT to be set low, indicating readiness */
		rfid_cr95hf_wait_for_irq_out_low(config->irq_out, data);
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

	/* Prepare and send the command to read response data */
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

	response_code = data->rcv_buffer[1]; /* Get the response code */
	data_len = data->rcv_buffer[2]; /* Get the data length */

	LOG_DBG("Response Code: %02X Datalen %02X ", response_code, data_len);

	/* Read data from the CR95HF, limiting to buffer size */
	if (data_len > CR95HF_RCV_BUF_SIZE) {
		data_len = CR95HF_RCV_BUF_SIZE; /* Limit to the buffer size */
	}

	/* Prepare to read the actual data into the response buffer */
	data->spi_snd_buffer.len = 0; /* No command byte needed for reading data */
	data->spi_rcv_buffer.len = data_len; /* Use the actual data length */
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
int rfid_cr95hf_select_mode(const struct device *dev, rfid_mode_t req_mode)
{
	struct rfid_cr95hf_data *data = dev->data;
	const struct rfid_cr95hf_spi_config *config = dev->config;
	const struct gpio_dt_spec *irq_in = config->irq_in;
	rfid_mode_t current_mode = data->current_mode;
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

	/* Switch to the requested mode */
	switch (req_mode) {
	case TAG_DETECTOR:
		memcpy(data->snd_buffer, data->tag_detector_msg, 17);
		data->spi_snd_buffer.len = 17;
		data->spi_rcv_buffer.len = 0;

		/* Send command to switch to TAG_DETECTOR mode */
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			LOG_ERR("Failed to send tag detector command (err %d)", err);
			return err;
		}

		rfid_cr95hf_setmode(data, TAG_DETECTOR);

		/* Block until the CR95HF device has woken up */
		rfid_cr95hf_wait(dev);

		/* Wait for response after waking up */
		err = rfid_cr95hf_response(dev);
		if (err) {
			LOG_ERR("Failed to read response after wakeup (err %d)", err);
			return err;
		}

		/* Transition back to READY mode */
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

	 /* Wait for response after sending SEL_CL1 */
	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after SEL_CL1 (err %d)", err);
		return err;
	}

	 /* Parse the received UID based on the response */
	if (data->rcv_buffer[0] == 0x88)	{
		uid[0] = data->rcv_buffer[1];
		uid[1] = data->rcv_buffer[2];
		uid[2] = data->rcv_buffer[3];
	} else {
		uid[0] = data->rcv_buffer[0];
		uid[1] = data->rcv_buffer[1];
		uid[2] = data->rcv_buffer[2];
		uid[3] = data->rcv_buffer[3];
		*uid_len = 4;  /* Set UID length to 4 bytes */
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

	 /* Wait for response after sending SEL_CL1 complete */
	err = rfid_cr95hf_response(dev);
	if (err) {
		LOG_ERR("Failed to read response after SEL_CL1 complete (err %d)", err);
		return err;
	}

	sak_byte = data->rcv_buffer[0];  /* Store the SAK byte from the response */

	/* Check if more UID data can be read */
	if (sak_byte & 0x04) {
		data->snd_buffer[0] = 0x00;  /* Control Byte: Send */
		data->snd_buffer[1] = 0x04;  /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x03;  /* Data Length */
		data->snd_buffer[3] = 0x95;  /* SEL_CL2 command */
		data->snd_buffer[4] = 0x20;  /* Parameter */
		data->snd_buffer[5] = 0x08;  /* Parameter */
		data->spi_snd_buffer.len = 6;  /* Set length for sending */
		data->spi_rcv_buffer.len = 0;  /* No expected response */
		err = rfid_cr95hf_transceive(dev, true);  /* Send command */
		if (err) {
			LOG_ERR("Failed to send SEL_CL2 (err %d)", err);  /* Log error */
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for the device to process */

		err = rfid_cr95hf_response(dev);  /* Get the response from the device */
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL2 (err %d)", err);
			return err;  /* Return error code */
		}

		/* Check for special response */
		if (data->rcv_buffer[0] == 0x88) {
			 /* Assign UID parts based on response type */
			uid[3] = data->rcv_buffer[1];
			uid[4] = data->rcv_buffer[2];
			uid[5] = data->rcv_buffer[3];
		} else {
			 /* Standard assignment for UID parts */
			uid[3] = data->rcv_buffer[0];
			uid[4] = data->rcv_buffer[1];
			uid[5] = data->rcv_buffer[2];
			uid[6] = data->rcv_buffer[3];
			*uid_len = 7;  /* Update UID length */
		}

		data->snd_buffer[0] = 0x00;  /* Control Byte: Send */
		data->snd_buffer[1] = 0x04;  /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x08;  /* Data Length */
		data->snd_buffer[3] = 0x95;  /* SEL_CL2 complete command */
		data->snd_buffer[4] = 0x70;  /* Parameter */
		data->snd_buffer[5] = data->rcv_buffer[0];  /* Echo from previous response */
		data->snd_buffer[6] = data->rcv_buffer[1];  /* Echo from previous response */
		data->snd_buffer[7] = data->rcv_buffer[2];  /* Echo from previous response */
		data->snd_buffer[8] = data->rcv_buffer[3];  /* Echo from previous response */
		data->snd_buffer[9] = data->rcv_buffer[4];  /* Echo from previous response */
		data->snd_buffer[10] = 0x28;  /* Termination byte */

		data->spi_snd_buffer.len = 11;  /* Set length for sending */
		data->spi_rcv_buffer.len = 0;  /* No expected response */
		err = rfid_cr95hf_transceive(dev, true);  /* Send command */
		if (err) {
			LOG_ERR("Failed to send SEL_CL2 complete (err %d)", err);  /* Log error */
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for the device to process */
		err = rfid_cr95hf_response(dev);  /* Get the response from the device */
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL2 complete (err %d)", err);
			return err;  /* Return error code */
		}

		sak_byte = data->rcv_buffer[0];  /* Update SAK byte for next operation */
	}

	/* Check if more UID data can be read */
	if (sak_byte & 0x04) {
		data->snd_buffer[0] = 0x00; /* Control Byte: Send */
		data->snd_buffer[1] = 0x04; /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x03; /* Data Length */
		data->snd_buffer[3] = 0x97; /* Command for reading next UID part */
		data->snd_buffer[4] = 0x20; /* Parameter */
		data->snd_buffer[5] = 0x08; /* Parameter */
		data->spi_snd_buffer.len = 6;  /* Set length for sending */
		data->spi_rcv_buffer.len = 0;  /* No expected response */
		err = rfid_cr95hf_transceive(dev, true);  /* Send command */
		if (err) {
			LOG_ERR("Failed to send SEL_CL3 (err %d)", err);  /* Log error */
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for the device to process */
		err = rfid_cr95hf_response(dev);  /* Get the response from the device */
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL3 (err %d)", err);
			return err;  /* Return error code */
		}

		 /* Assign new UID parts */
		uid[6] = data->rcv_buffer[0];
		uid[7] = data->rcv_buffer[1];
		uid[8] = data->rcv_buffer[2];
		uid[9] = data->rcv_buffer[3];
		*uid_len = 10;  /* Update UID length */
		data->snd_buffer[0] = 0x00; /* Control Byte: Send */
		data->snd_buffer[1] = 0x04; /* Cmd: SendRecev */
		data->snd_buffer[2] = 0x08; /* Data Length */
		data->snd_buffer[3] = 0x97; /* Complete command */
		data->snd_buffer[4] = 0x70; /* Parameter */
		data->snd_buffer[5] = data->rcv_buffer[0]; /* Echo from previous response */
		data->snd_buffer[6] = data->rcv_buffer[1]; /* Echo from previous response */
		data->snd_buffer[7] = data->rcv_buffer[2]; /* Echo from previous response */
		data->snd_buffer[8] = data->rcv_buffer[3]; /* Echo from previous response */
		data->snd_buffer[9] = data->rcv_buffer[4]; /* Echo from previous response */
		data->snd_buffer[10] = 0x28;  /* Termination byte */

		data->spi_snd_buffer.len = 11;  /* Set length for sending */
		data->spi_rcv_buffer.len = 0;  /* No expected response */
		err = rfid_cr95hf_transceive(dev, true);  /* Send command */
		if (err) {
			LOG_ERR("Failed to send SEL_CL3 complete (err %d)", err);  /* Log error */
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for the device to process */
		err = rfid_cr95hf_response(dev);  /* Get the response from the device */
		if (err) {
			LOG_ERR("Failed to read response after SEL_CL3 complete (err %d)", err);
			return err;  /* Return error code */
		}

		sak_byte = data->snd_buffer[0];  /* Update SAK byte for next operation */
	}

	 /* Log UID based on length */
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
int rfid_cr95hf_protocol_select(const struct device *dev, rfid_protocol_t proto)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;

	switch (proto) {
	case ISO_14443A:
		 /* Prepare the command for switching the protocol */
		memcpy(data->snd_buffer, data->protocol_msg, data->protocol_msg_len);
		data->spi_snd_buffer.len = data->protocol_msg_len; /* Set length of send buffer */
		data->spi_rcv_buffer.len = 0;  /* No expected response for this command */
		 /* Send command to switch to the specified protocol */
		err = rfid_cr95hf_transceive(dev, true);
		if (err) {
			/* Log error if sending fails */
			LOG_ERR("Failed to send protocol select command (err %d)", err);
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for the device to process the command */
		err = rfid_cr95hf_response(dev);
		if (err) {
			/* Log error if reading fails */
			LOG_ERR("Failed to read response after protocol select command (err %d)",
				err);
			return err;  /* Return error code */
		}

		break;  /* Exit switch case after successful command execution */
	default:
		/* Log unsupported protocol error */
		LOG_ERR("The selected protocol is not supported");
		return -EINVAL;  /* Return invalid argument error */
	}
	return 0;  /* Return success */
}

/**
 * @brief Transmits data to the RFID device and receives a response.
 *
 * This function is responsible for sending a buffer of data (`tx`) to the RFID device and receiving
 * a response buffer (`rx`) back. It also manages the lengths of the transmitted and received data.
 */
int rfid_cr95hf_transceive_api(const struct device *dev, const uint8_t *tx, size_t *tx_len,
								uint8_t *rx, size_t *rx_len)
{
	LOG_ERR("Function not implemented");
	return -EPERM;
}

/**
 * @brief Performs calibration of the RFID reader.
 *
 * This function sends a series of commands to the RFID reader to adjust
 * its DAC settings based on the received responses, aiming to optimize
 * detection sensitivity and performance. It iteratively modifies the DAC
 * data reference value based on the responses received from the reader.
 */
int rfid_cr95hf_calibration(const struct device *dev)
{
	struct rfid_cr95hf_data *data = dev->data;
	int err;

	LOG_INF("Don't rely on this function. It is not tested, "
				"because I get always Tag Detected and never Timeout");

	 /* Initialize command buffer with specific parameters for calibration */
	data->snd_buffer[0] = 0x00;
	data->snd_buffer[1] = 0x07;
	data->snd_buffer[2] = 0x0E;
	data->snd_buffer[3] = 0x03;
	data->snd_buffer[4] = 0xA1;
	data->snd_buffer[5] = 0x00;
	data->snd_buffer[6] = 0xF8;
	data->snd_buffer[7] = 0x01;
	data->snd_buffer[8] = 0x18;
	data->snd_buffer[9] = 0x00;
	data->snd_buffer[10] = 0x20;
	data->snd_buffer[11] = 0x60;
	data->snd_buffer[12] = 0x60;
	data->snd_buffer[13] = 0x00;
	data->snd_buffer[14] = 0x00;
	data->snd_buffer[15] = 0x3F;
	data->snd_buffer[16] = 0x01;

	 /* Iterate through calibration steps */
	for (int i = 0; i < 8; i++) {
		data->spi_snd_buffer.len = 17;  /* Set length for sending */
		data->spi_rcv_buffer.len = 0;    /* No expected response for this command */
		/* Log current DAC reference value */
		LOG_DBG("Step %d: search DacDataRef = 0x%02X", i, data->snd_buffer[14]);

		err = rfid_cr95hf_transceive(dev, true);  /* Send command */
		if (err) {
			/* Log error if sending fails */
			LOG_ERR("Failed to send idle command (err %d)", err);
			return err;  /* Return error code */
		}

		rfid_cr95hf_wait(dev);  /* Wait for device to process command */

		err = rfid_cr95hf_response(dev);  /* Retrieve response from device */
		if (err) {
			/* Log error if reading fails */
			LOG_ERR("Failed to read response after idle command (err %d)", err);
			return err;  /* Return error code */
		}

		/* Evaluate response based on step */
		switch (i) {
		case 0:
				/* Check for expected response data */
			if (!(data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2)) {
				LOG_ERR("Unexpected Data Received"); /* Log unexpected data error */
				return -EIO;  /* Return input/output error */
			}
			data->snd_buffer[14] = 0xFC;  /* Set DAC reference for next step */
			break;

		case 1:
				/* Check for expected response data */
			if (!(data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1)) {
				LOG_ERR("Unexpected Data Received"); /* Log unexpected data error */
				return -EIO;  /* Return input/output error */
			}
			/* Adjust DAC reference based on received data */
			data->snd_buffer[14] -= 0x80;
			break;

		case 2:
				/* Adjust DAC reference based on received data */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				data->snd_buffer[14] -= 0x40;  /* Decrease DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				data->snd_buffer[14] += 0x40;  /* Increase DAC reference */
			} else {
				LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
				return -EIO;  /* Return I/O error */
			}
			break;

		case 3:
				/* Adjust DAC reference further based on response */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				data->snd_buffer[14] -= 0x20;  /* Decrease DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				data->snd_buffer[14] += 0x20;  /* Increase DAC reference */
			} else {
				LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
				return -EIO;  /* Return I/O error */
			}
			break;

		case 4:
				/* Further DAC adjustment based on data received */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				data->snd_buffer[14] -= 0x10;  /* Decrease DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				data->snd_buffer[14] += 0x10;  /* Increase DAC reference */
			} else {
				LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
				return -EIO;  /* Return I/O error */
			}
			break;

		case 5:
				/* Final adjustment of DAC based on response data */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				data->snd_buffer[14] -= 0x08;  /* Decrease DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				data->snd_buffer[14] += 0x08;  /* Increase DAC reference */
			} else {
				LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
				return -EIO;  /* Return I/O error */
			}
			break;

		case 6:
				/* Adjust DAC reference based on received data */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				data->snd_buffer[14] -= 0x04;  /* Decrease DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				data->snd_buffer[14] += 0x04;  /* Increase DAC reference */
			} else {
				LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
				return -EIO;  /* Return I/O error */
			}
			break;

		case 7:
				/* Final adjustment and return DAC reference value */
			if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 1) {
				/* Log reference value before returning */
				LOG_DBG("Step %d: search DacDataRef = 0x%02X",
					i, data->snd_buffer[14]-4);
				return data->snd_buffer[14] - 4; /* Return adjusted DAC reference */
			} else if (data->rcv_buffer[0] == 0 &&
				data->rcv_buffer[1] == 1 &&
				data->rcv_buffer[2] == 2) {
				/* Log reference value being returned */
				LOG_DBG("Step %d: search DacDataRef = 0x%02X",
					i, data->snd_buffer[14]);
				return data->snd_buffer[14];  /* Return current DAC reference */
			}

			LOG_ERR("Unexpected Data Received");  /* Log unexpected response */
			return -EIO;  /* Return I/O error */

			break;

		default:
			/* Assert that this case should never be reached */
			__ASSERT_NO_MSG(0);
			break;
		}
	}
	/* Assert that this return should never be reached */
	__ASSERT_NO_MSG(0);
	return -EIO;
}

static DEVICE_API(rfid, rfid_cr95hf_api) = {
	.select_mode = rfid_cr95hf_select_mode, /* Selects operating mode for RFID communication */
	.protocol_select = rfid_cr95hf_protocol_select, /* Selects the protocol for communication */
	.get_uid = rfid_cr95hf_iso14443A_get_uid, /* Retrieves the unique identifier of RFID tag */
	.transceive = rfid_cr95hf_transceive_api, /* Sends and receives data from the RFID device */
	.calibration = rfid_cr95hf_calibration,   /* Handles calibration routines for the device */
};

 /* Define to initialize RFID device using SPI communication */
#define RFID_DEVICE_SPI(n)                                                                     \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n;                               \
	static void rfid_cr95hf_irq_out_callback_##n(                                          \
		const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)    \
	{                                                                                      \
		k_sem_give(&rfid_device_prv_data_##n.irq_out_sem);                             \
	}                                                                                      \
	static const struct gpio_dt_spec cs_gpio_dt_spec_##n =                                 \
		GPIO_DT_SPEC_GET(DT_DRV_INST(n), cs_gpios);                                    \
	static const struct gpio_dt_spec irq_in_gpio_dt_spec_##n =                             \
		GPIO_DT_SPEC_GET(DT_DRV_INST(n), irq_in_gpios);                                \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), irq_out_gpios),                           \
		(static const struct gpio_dt_spec irq_out_gpio_dt_spec_##n =                   \
			GPIO_DT_SPEC_GET(DT_DRV_INST(n), irq_out_gpios);), ())                 \
	static const struct rfid_cr95hf_spi_config rfid_device_prv_config_##n = {              \
		.spi = SPI_DT_SPEC_INST_GET(                                                   \
			n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U) | SPI_TRANSFER_MSB, 0U),      \
		.cs = &cs_gpio_dt_spec_##n,                                                    \
		.irq_in = &irq_in_gpio_dt_spec_##n,                                            \
		.irq_out = COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), irq_out_gpios),        \
			(&irq_out_gpio_dt_spec_##n), (NULL)),                                  \
	};                                                                                     \
	static struct rfid_cr95hf_data rfid_device_prv_data_##n = {                            \
		.current_mode = UNINITIALIZED,                                                 \
		.tag_detector_msg = CR95HF_CREATE_IDLE_ARRAY(                                  \
			CR95HF_WU_SOURCE_TAG_DETECTION | CR95HF_WU_SOURCE_LOW_PULSE_IRQ_IN,    \
			CR95HF_ENTER_CTRL_DETECTION_H, CR95HF_ENTER_CTRL_DETECTION_L,          \
			CR95HF_WU_CTRL_DETECTION_H, CR95HF_WU_CTRL_DETECTION_L,                \
			CR95HF_LEAVE_CTRL_DETECTION_H, CR95HF_LEAVE_CTRL_DETECTION_L,          \
			CR95HF_DEFAULT_WU_PERIOD, CR95HF_DEFAULT_OSC_START,                    \
			CR95HF_DEFAULT_DAC_START, CR95HF_DEFAULT_DAC_DATA_H,                   \
			CR95HF_DEFAULT_DAC_DATA_L, CR95HF_DEFAULT_SWING_COUNT,                 \
			CR95HF_DEFAULT_MAX_SLEEP),                                             \
		.protocol_msg = CR95HF_CREATE_SELECT_14443_A_ARRAY(CR95HF_ISO_14443_106_KBPS,  \
								   CR95HF_ISO_14443_106_KBPS), \
		.protocol_msg_len = 7,                                                         \
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
