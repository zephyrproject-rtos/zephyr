/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief CYW43xxx HCI extension driver.
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cyw43xxx_driver);

#include <stdint.h>

BUILD_ASSERT(DT_PROP(DT_CHOSEN(zephyr_bt_uart), hw_flow_control) == 1,
		"hw_flow_control must be enabled for HCI H4 UART");

#define DT_DRV_COMPAT infineon_cyw43xxx_bt_hci

/* BT settling time after power on */
#define BT_POWER_ON_SETTLING_TIME_MS      (500u)

/* Stabilization delay after FW loading */
#define BT_STABILIZATION_DELAY_MS         (250u)

/* HCI Command packet from Host to Controller */
#define HCI_COMMAND_PACKET                (0x01)

/* Length of UPDATE BAUD RATE command */
#define HCI_VSC_UPDATE_BAUD_RATE_LENGTH   (6u)

/* Default BAUDRATE */
#define HCI_UART_DEFAULT_BAUDRATE         (115200)

/* Externs for CY43xxx controller FW */
extern const uint8_t brcm_patchram_buf[];
extern const int brcm_patch_ram_length;

enum {
	BT_HCI_VND_OP_DOWNLOAD_MINIDRIVER       = 0xFC2E,
	BT_HCI_VND_OP_WRITE_RAM                 = 0xFC4C,
	BT_HCI_VND_OP_LAUNCH_RAM                = 0xFC4E,
	BT_HCI_VND_OP_UPDATE_BAUDRATE           = 0xFC18,
};

/*  bt_h4_vnd_setup function.
 * This function executes vendor-specific commands sequence to
 * initialize BT Controller before BT Host executes Reset sequence.
 * bt_h4_vnd_setup function must be implemented in vendor-specific HCI
 * extansion module if CONFIG_BT_HCI_SETUP is enabled.
 */
int bt_h4_vnd_setup(const struct device *dev);

static int bt_hci_uart_set_baudrate(const struct device *bt_uart_dev, uint32_t baudrate)
{
	struct uart_config uart_cfg;
	int err;

	/* Get current UART configuration */
	err = uart_config_get(bt_uart_dev, &uart_cfg);
	if (err) {
		return err;
	}

	if (uart_cfg.baudrate != baudrate) {
		/* Re-configure UART */
		uart_cfg.baudrate = baudrate;
		err = uart_configure(bt_uart_dev, &uart_cfg);
		if (err) {
			return err;
		}

		/* Revert Interrupt options */
		uart_irq_rx_enable(bt_uart_dev);
	}
	return 0;
}

static int bt_update_controller_baudrate(const struct device *bt_uart_dev, uint32_t baudrate)
{
	/*
	 *  NOTE from datasheet for update baudrate:
	 *  - To speed up application downloading, the MCU host commands the CYWxxx device
	 *    to communicate at a new, higher rate by issuing the following Vendor Specific
	 *    UPDATE_BAUDRATE command:
	 *      01 18 FC 06 00 00 xx xx xx xx
	 *    In the above command, the xx xx xx xx bytes specify the 32-bit little-endian
	 *    value of the new rate in bits per second. For example,
	 *    115200 is represented as 00 C2 01 00.
	 *  The following response to the UPDATE_BAUDRATE command is expected within 100 ms:
	 *  04 0E 04 01 18 FC 00
	 *  - The host switches to the new baud rate after receiving the response at the old
	 *  baud rate.
	 */
	struct net_buf *buf;
	int err;
	uint8_t hci_data[HCI_VSC_UPDATE_BAUD_RATE_LENGTH];

	/* Baudrate is loaded LittleEndian */
	hci_data[0] = 0;
	hci_data[1] = 0;
	hci_data[2] = (uint8_t)(baudrate & 0xFFUL);
	hci_data[3] = (uint8_t)((baudrate >> 8) & 0xFFUL);
	hci_data[4] = (uint8_t)((baudrate >> 16) & 0xFFUL);
	hci_data[5] = (uint8_t)((baudrate >> 24) & 0xFFUL);

	/* Allocate buffer for update uart baudrate command.
	 * It will be BT_HCI_OP_RESET with extra parameters.
	 */
	buf = bt_hci_cmd_create(BT_HCI_VND_OP_UPDATE_BAUDRATE,
				HCI_VSC_UPDATE_BAUD_RATE_LENGTH);
	if (buf == NULL) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	/* Add data part of packet */
	net_buf_add_mem(buf, &hci_data, HCI_VSC_UPDATE_BAUD_RATE_LENGTH);

	/* Send update uart baudrate command. */
	err = bt_hci_cmd_send_sync(BT_HCI_VND_OP_UPDATE_BAUDRATE, buf, NULL);
	if (err) {
		return err;
	}

	/* Re-configure Uart baudrate */
	err = bt_hci_uart_set_baudrate(bt_uart_dev, baudrate);
	if (err) {
		return err;
	}

	return 0;
}

static int bt_firmware_download(const uint8_t *firmware_image, uint32_t size)
{
	uint8_t *data = (uint8_t *)firmware_image;
	volatile uint32_t remaining_length = size;
	struct net_buf *buf;
	int err;

	LOG_DBG("Executing Fw downloading for CYW43xx device");

	/* Send hci_download_minidriver command */
	err = bt_hci_cmd_send_sync(BT_HCI_VND_OP_DOWNLOAD_MINIDRIVER, NULL, NULL);
	if (err) {
		return err;
	}

	/* The firmware image (.hcd format) contains a collection of hci_write_ram
	 * command + a block of the image, followed by a hci_write_ram image at the end.
	 * Parse and send each individual command and wait for the response. This is to
	 * ensure the integrity of the firmware image sent to the bluetooth chip.
	 */
	while (remaining_length) {
		size_t data_length = data[2]; /* data length from firmware image block */
		uint16_t op_code = *(uint16_t *)data;

		/* Allocate buffer for hci_write_ram/hci_launch_ram command. */
		buf = bt_hci_cmd_create(op_code, data_length);
		if (buf == NULL) {
			LOG_ERR("Unable to allocate command buffer");
			return err;
		}

		/* Add data part of packet */
		net_buf_add_mem(buf, &data[3], data_length);

		/* Send hci_write_ram command. */
		err = bt_hci_cmd_send_sync(op_code, buf, NULL);
		if (err) {
			return err;
		}

		switch (op_code) {
		case BT_HCI_VND_OP_WRITE_RAM:
			/* Update remaining length and data pointer:
			 * content of data length + 2 bytes of opcode and 1 byte of data length.
			 */
			data += data_length + 3;
			remaining_length -= data_length + 3;
			break;

		case BT_HCI_VND_OP_LAUNCH_RAM:
			remaining_length = 0;
			break;

		default:
			return -ENOMEM;
		}
	}

	LOG_DBG("Fw downloading complete");
	return 0;
}

int bt_h4_vnd_setup(const struct device *dev)
{
	int err;
	uint32_t default_uart_speed = DT_PROP(DT_INST_BUS(0), current_speed);
	uint32_t hci_operation_speed = DT_INST_PROP_OR(0, hci_operation_speed, default_uart_speed);
	uint32_t fw_download_speed = DT_INST_PROP_OR(0, fw_download_speed, default_uart_speed);

	/* Check BT Uart instance */
	if (!device_is_ready(dev)) {
		return -EINVAL;
	}

#if DT_INST_NODE_HAS_PROP(0, bt_reg_on_gpios)
	struct gpio_dt_spec bt_reg_on = GPIO_DT_SPEC_GET(DT_DRV_INST(0), bt_reg_on_gpios);

	/* Check BT REG_ON gpio instance */
	if (!gpio_is_ready_dt(&bt_reg_on)) {
		LOG_ERR("Error: failed to configure bt_reg_on %s pin %d",
			bt_reg_on.port->name, bt_reg_on.pin);
		return -EIO;
	}

	/* Configure bt_reg_on as output  */
	err = gpio_pin_configure_dt(&bt_reg_on, GPIO_OUTPUT);
	if (err) {
		LOG_ERR("Error %d: failed to configure bt_reg_on %s pin %d",
			err, bt_reg_on.port->name, bt_reg_on.pin);
		return err;
	}
	err = gpio_pin_set_dt(&bt_reg_on, 1);
	if (err) {
		return err;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, bt_reg_on_gpios) */

	/* BT settling time after power on */
	(void)k_msleep(BT_POWER_ON_SETTLING_TIME_MS);

	/* Send HCI_RESET */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
	if (err) {
		return err;
	}

	/* Re-configure baudrate for BT Controller */
	if (fw_download_speed != default_uart_speed) {
		err = bt_update_controller_baudrate(dev, fw_download_speed);
		if (err) {
			return err;
		}
	}

	/* BT firmware download */
	err = bt_firmware_download(brcm_patchram_buf, (uint32_t) brcm_patch_ram_length);
	if (err) {
		return err;
	}

	/* Stabilization delay */
	(void)k_msleep(BT_STABILIZATION_DELAY_MS);

	/* When FW launched, HCI UART baudrate should be configured to default */
	if (fw_download_speed != default_uart_speed) {
		err = bt_hci_uart_set_baudrate(dev, default_uart_speed);
		if (err) {
			return err;
		}
	}

	/* Send HCI_RESET */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
	if (err) {
		return err;
	}

	/* Set host controller functionality to user defined baudrate
	 * after fw downloading.
	 */
	if (hci_operation_speed != default_uart_speed) {
		err = bt_update_controller_baudrate(dev, hci_operation_speed);
		if (err) {
			return err;
		}
	}

	return 0;
}
