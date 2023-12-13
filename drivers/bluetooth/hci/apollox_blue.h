/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header file of Ambiq Apollox Blue SoC extended driver
 * for SPI based HCI.
 */
#ifndef ZEPHYR_DRIVERS_BLUETOOTH_HCI_APOLLOX_BLUE_H_
#define ZEPHYR_DRIVERS_BLUETOOTH_HCI_APOLLOX_BLUE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef bt_spi_transceive_fun
 * @brief SPI transceive function for Bluetooth packet.
 *
 * @param tx Pointer of transmission packet.
 * @param tx_len Length of transmission packet.
 * @param rx Pointer of reception packet.
 * @param rx_len Length of reception packet.
 *
 * @return 0 on success or negative error number on failure.
 */
typedef int (*bt_spi_transceive_fun)(void *tx, uint32_t tx_len, void *rx, uint32_t rx_len);

/**
 * @typedef spi_transmit_fun
 * @brief Define the SPI transmission function.
 *
 * @param data Pointer of transmission packet.
 * @param len Length of transmission packet.
 *
 * @return 0 on success or negative error number on failure.
 */
typedef int (*spi_transmit_fun)(uint8_t *data, uint16_t len);

/**
 * @brief Initialize the required devices for HCI driver.
 *
 * The devices mainly include the required gpio (e.g. reset-gpios,
 * irq-gpios).
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_apollo_dev_init(void);

/**
 * @brief Send the packets to BLE controller from host via SPI.
 *
 * @param data Pointer of transmission packet.
 * @param len  Length of transmission packet.
 * @param transceive SPI transceive function for Bluetooth packet.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_apollo_spi_send(uint8_t *data, uint16_t len, bt_spi_transceive_fun transceive);

/**
 * @brief Receive the packets sent from BLE controller to host via SPI.
 *
 * @param data Pointer of reception packet.
 * @param len  Pointer of reception packet length.
 * @param transceive  SPI transceive function for Bluetooth packet.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_apollo_spi_rcv(uint8_t *data, uint16_t *len, bt_spi_transceive_fun transceive);

/**
 * @brief Initialize the BLE controller.
 *
 * This step may do the necessary handshaking with the controller before
 * @param transmit SPI transmit function
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_apollo_controller_init(spi_transmit_fun transmit);

/**
 * @brief Vendor specific setup before general HCI command sequence for
 * Bluetooth application.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_apollo_vnd_setup(void);

/**
 * @brief Check if vendor specific receiving handling is ongoing.
 *
 * @param data Pointer of received packet.
 *
 * @return true indicates if vendor specific receiving handling is ongoing.
 */
bool bt_apollo_vnd_rcv_ongoing(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_BLUETOOTH_HCI_APOLLOX_BLUE_H_ */
