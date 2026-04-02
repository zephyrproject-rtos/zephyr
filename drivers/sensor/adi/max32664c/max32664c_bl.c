/*
 * I2C firmware loader for the MAX32664C biometric sensor hub.
 *
 * Copyright (c) 2025, Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "max32664c.h"

#define MAX32664C_FW_PAGE_SIZE         8192
#define MAX32664C_FW_UPDATE_CRC_SIZE   16
#define MAX32664C_FW_UPDATE_WRITE_SIZE (MAX32664C_FW_PAGE_SIZE + MAX32664C_FW_UPDATE_CRC_SIZE)
#define MAX32664C_DEFAULT_CMD_DELAY_MS 10
#define MAX32664C_PAGE_WRITE_DELAY_MS  680

static uint8_t max32664c_fw_init_vector[11];
static uint8_t max32664c_fw_auth_vector[16];

LOG_MODULE_REGISTER(max32664_loader, CONFIG_SENSOR_LOG_LEVEL);

/** @brief          Read / write bootloader data from / to the sensor hub.
 *  @param dev      Pointer to device
 *  @param tx_buf   Pointer to transmit buffer
 *  @param tx_len   Length of transmit buffer
 *  @param rx_buf   Pointer to receive buffer
 *                  NOTE: The buffer must be large enough to store the response and the status byte!
 *  @param rx_len   Length of the receive buffer
 *  @return         0 when successful
 */
static int max32664c_bl_i2c_transmit(const struct device *dev, uint8_t *tx_buf, uint8_t tx_len,
				     uint8_t *rx_buf, uint8_t rx_len)
{
	int err;
	const struct max32664c_config *config = dev->config;

	err = i2c_write_dt(&config->i2c, tx_buf, tx_len);
	if (err) {
		LOG_ERR("I2C transmission error %d!", err);
		return err;
	}
	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);
	err = i2c_read_dt(&config->i2c, rx_buf, rx_len);
	if (err) {
		LOG_ERR("I2C transmission error %d!", err);
		return err;
	}
	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);

	/* Check the status byte for a valid transaction */
	LOG_DBG("Status: %u", rx_buf[0]);
	if (rx_buf[0] != 0) {
		return -EINVAL;
	}

	return 0;
}

/** @brief          Read application data from the sensor hub.
 *  @param dev      Pointer to device
 *  @param family   Family byte
 *  @param index    Index byte
 *  @param rx_buf   Pointer to receive buffer
 *                  NOTE: The buffer must be large enough to store the response and the status byte!
 *  @param rx_len   Length of receive buffer
 *  @return         0 when successful
 */
static int max32664c_app_i2c_read(const struct device *dev, uint8_t family, uint8_t index,
				  uint8_t *rx_buf, uint8_t rx_len)
{
	uint8_t tx_buf[] = {family, index};
	const struct max32664c_config *config = dev->config;

	/* Wake the sensor hub before starting an I2C read (see page 17 of the user Guide) */
	gpio_pin_set_dt(&config->mfio_gpio, false);
	k_usleep(300);

	i2c_write_dt(&config->i2c, tx_buf, sizeof(tx_buf));
	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);
	i2c_read_dt(&config->i2c, rx_buf, rx_len);
	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);

	gpio_pin_set_dt(&config->mfio_gpio, true);

	/* Check the status byte for a valid transaction */
	if (rx_buf[0] != 0) {
		return -EINVAL;
	}

	return 0;
}

/** @brief          Write a page of data into the sensor hub.
 *  @param dev      Pointer to device
 *  @param data     Pointer to firmware data
 *  @param offset   Start address in the firmware data
 *  @return         0 when successful
 */
static int max32664c_bl_write_page(const struct device *dev, const uint8_t *data, uint32_t offset)
{
	int err;
	uint8_t rx_buf;
	uint8_t *tx_buf;
	const struct max32664c_config *config = dev->config;

	/* Alloc memory for one page plus two command bytes */
	tx_buf = (uint8_t *)k_malloc(MAX32664C_FW_UPDATE_WRITE_SIZE + 2);
	if (tx_buf == NULL) {
		return -ENOMEM;
	}

	/* Copy the data for one page into the buffer but leave space for the two command bytes */
	memcpy(&tx_buf[2], &data[offset], MAX32664C_FW_UPDATE_WRITE_SIZE);

	/* Set the two command bytes */
	tx_buf[0] = 0x80;
	tx_buf[1] = 0x04;

	if (i2c_write_dt(&config->i2c, tx_buf, MAX32664C_FW_UPDATE_WRITE_SIZE + 2)) {
		err = -EINVAL;
		goto max32664c_bl_write_page_exit;
	};
	k_msleep(MAX32664C_PAGE_WRITE_DELAY_MS);
	err = i2c_read_dt(&config->i2c, &rx_buf, 1);
	if (err) {
		LOG_ERR("I2C read error %d!", err);
		err = -EINVAL;
		goto max32664c_bl_write_page_exit;
	};
	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);

	err = rx_buf;

	LOG_DBG("Write page status: %u", err);

max32664c_bl_write_page_exit:
	k_free(tx_buf);
	return err;
}

/** @brief      Erase the application from the sensor hub.
 *  @param dev  Pointer to device
 *  @return     0 when successful
 */
static int max32664c_bl_erase_app(const struct device *dev)
{
	uint8_t tx_buf[2] = {0x80, 0x03};
	uint8_t rx_buf;
	const struct max32664c_config *config = dev->config;

	if (i2c_write_dt(&config->i2c, tx_buf, sizeof(tx_buf))) {
		return -EINVAL;
	};

	k_msleep(1500);

	if (i2c_read_dt(&config->i2c, &rx_buf, sizeof(rx_buf))) {
		return -EINVAL;
	};

	k_msleep(MAX32664C_DEFAULT_CMD_DELAY_MS);

	/* Check the status byte for a valid transaction */
	if (rx_buf != 0) {
		return -EINVAL;
	}

	return 0;
}

/** @brief          Load the firmware into the hub.
 *                  NOTE: See User Guide, Table 9 for the required steps.
 *  @param dev      Pointer to device
 *  @param firmware Pointer to firmware data
 *  @param size     Firmware size
 *  @return         0 when successful
 */
static int max32664c_bl_load_fw(const struct device *dev, const uint8_t *firmware, uint32_t size)
{
	uint8_t rx_buf;
	uint8_t tx_buf[18] = {0};
	uint32_t page_offset;

	/* Get the number of pages from the firmware file (see User Guide page 53) */
	uint8_t num_pages = firmware[0x44];

	LOG_INF("Loading firmware...");
	LOG_INF("\tSize: %u", size);
	LOG_INF("\tPages: %u", num_pages);

	/* Set the number of pages */
	tx_buf[0] = 0x80;
	tx_buf[1] = 0x02;
	tx_buf[2] = 0x00;
	tx_buf[3] = num_pages;
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 4, &rx_buf, 1)) {
		return -EINVAL;
	}

	if (rx_buf != 0) {
		LOG_ERR("Failed to set number of pages: %d", rx_buf);
		return -EINVAL;
	}

	/* Get the initialization and authentication vectors from the firmware */
	/* (see User Guide page 53) */
	memcpy(max32664c_fw_init_vector, &firmware[0x28], sizeof(max32664c_fw_init_vector));
	memcpy(max32664c_fw_auth_vector, &firmware[0x34], sizeof(max32664c_fw_auth_vector));

	/* Write the initialization vector */
	LOG_INF("\tWriting init vector...");
	tx_buf[0] = 0x80;
	tx_buf[1] = 0x00;
	memcpy(&tx_buf[2], max32664c_fw_init_vector, sizeof(max32664c_fw_init_vector));
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 13, &rx_buf, 1)) {
		return -EINVAL;
	}
	if (rx_buf != 0) {
		LOG_ERR("Failed to set init vector: %d", rx_buf);
		return -EINVAL;
	}

	/* Write the authentication vector */
	LOG_INF("\tWriting auth vector...");
	tx_buf[0] = 0x80;
	tx_buf[1] = 0x01;
	memcpy(&tx_buf[2], max32664c_fw_auth_vector, sizeof(max32664c_fw_auth_vector));
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 18, &rx_buf, 1)) {
		return -EINVAL;
	}
	if (rx_buf != 0) {
		LOG_ERR("Failed to set auth vector: %d", rx_buf);
		return -EINVAL;
	}

	/* Remove the old app from the hub */
	LOG_INF("\tRemove old app...");
	if (max32664c_bl_erase_app(dev)) {
		return -EINVAL;
	}

	/* Write the new firmware */
	LOG_INF("\tWriting new firmware...");
	page_offset = 0x4C;
	for (uint8_t i = 0; i < num_pages; i++) {
		uint8_t status;

		LOG_INF("\t\tPage: %d of %d", (i + 1), num_pages);
		LOG_INF("\t\tOffset: 0x%x", page_offset);
		status = max32664c_bl_write_page(dev, firmware, page_offset);
		LOG_INF("\t\tStatus: %u", status);
		if (status != 0) {
			return -EINVAL;
		}

		page_offset += MAX32664C_FW_UPDATE_WRITE_SIZE;
	}

	LOG_INF("\tSuccessful!");

	return max32664c_bl_leave(dev);
}

int max32664c_bl_enter(const struct device *dev, const uint8_t *firmware, uint32_t size)
{
	uint8_t rx_buf[4] = {0};
	uint8_t tx_buf[3];
	const struct max32664c_config *config = dev->config;

	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->mfio_gpio, GPIO_OUTPUT);

	/* Put the processor into bootloader mode */
	LOG_INF("Entering bootloader mode");
	gpio_pin_set_dt(&config->reset_gpio, false);
	k_msleep(20);

	gpio_pin_set_dt(&config->mfio_gpio, false);
	k_msleep(20);

	gpio_pin_set_dt(&config->reset_gpio, true);
	k_msleep(200);

	/* Set bootloader mode */
	tx_buf[0] = 0x01;
	tx_buf[1] = 0x00;
	tx_buf[2] = 0x08;
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 3, rx_buf, 1)) {
		return -EINVAL;
	}

	/* Read the device mode */
	tx_buf[0] = 0x02;
	tx_buf[1] = 0x00;
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 2, rx_buf, 2)) {
		return -EINVAL;
	}

	LOG_DBG("Mode: %x ", rx_buf[1]);
	if (rx_buf[1] != 8) {
		LOG_ERR("Device not in bootloader mode!");
		return -EINVAL;
	}

	/* Read the bootloader information */
	tx_buf[0] = 0x81;
	tx_buf[1] = 0x00;
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 2, rx_buf, 4)) {
		return -EINVAL;
	}

	LOG_INF("Version: %d.%d.%d", rx_buf[1], rx_buf[2], rx_buf[3]);

	/* Read the bootloader page size */
	tx_buf[0] = 0x81;
	tx_buf[1] = 0x01;
	if (max32664c_bl_i2c_transmit(dev, tx_buf, 2, rx_buf, 3)) {
		return -EINVAL;
	}

	LOG_INF("Page size: %u", (uint16_t)(rx_buf[1] << 8) | rx_buf[2]);

	return max32664c_bl_load_fw(dev, firmware, size);
}

int max32664c_bl_leave(const struct device *dev)
{
	uint8_t hub_ver[3];
	uint8_t rx_buf[4] = {0};
	const struct max32664c_config *config = dev->config;

	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->mfio_gpio, GPIO_OUTPUT);

	LOG_INF("Entering app mode");
	gpio_pin_set_dt(&config->reset_gpio, true);
	gpio_pin_set_dt(&config->mfio_gpio, false);
	k_msleep(2000);

	gpio_pin_set_dt(&config->reset_gpio, false);
	k_msleep(5);

	gpio_pin_set_dt(&config->mfio_gpio, true);
	k_msleep(15);

	gpio_pin_set_dt(&config->reset_gpio, true);
	k_msleep(1700);

	/* Read the device mode */
	if (max32664c_app_i2c_read(dev, 0x02, 0x00, rx_buf, 2)) {
		return -EINVAL;
	}

	LOG_DBG("Mode: %x ", rx_buf[1]);
	if (rx_buf[1] != 0) {
		LOG_ERR("Device not in application mode!");
		return -EINVAL;
	}

	/* Read the MCU type */
	if (max32664c_app_i2c_read(dev, 0xFF, 0x00, rx_buf, 2)) {
		return -EINVAL;
	}

	LOG_INF("MCU type: %u", rx_buf[1]);

	/* Read the firmware version */
	if (max32664c_app_i2c_read(dev, 0xFF, 0x03, rx_buf, 4)) {
		return -EINVAL;
	}

	memcpy(hub_ver, &rx_buf[1], 3);

	LOG_INF("Version: %d.%d.%d", hub_ver[0], hub_ver[1], hub_ver[2]);

	return 0;
}
