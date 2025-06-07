/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_RFID_H_
#define ZEPHYR_DRIVERS_RFID_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

typedef enum {
	UNINITIALIZED,
	POWER_UP,
	READY,
	HIBERNATE,
	SLEEP,
	TAG_DETECTOR,
	READER,
	INVALID
} rfid_mode_t;

typedef enum {
	FIELD_OFF,
	ISO_15693,
	ISO_14443A,
	ISO_14443B,
	ISO_18092,
} rfid_protocol_t;

/**
 * @brief RFID Driver API
 *
 * This file defines the API for RFID drivers.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef rfid_api_select_mode()
 * @brief API for selecting the mode of the RFID device
 *
 * @param dev RFID device
 * @param req_mode Requested Mode
 * @return 0 on success, negative on error
 */
typedef int (*rfid_api_select_mode)(const struct device *dev, rfid_mode_t req_mode);

/**
 * @typedef rfid_api_protocol_select()
 * @brief API for selecting the communication protocol of the CR95HF RFID device
 *
 * @param dev RFID device
 * @param proto Communication protocol to be selected
 * @return 0 on success, negative on error
 */
typedef int (*rfid_api_protocol_select)(const struct device *dev, rfid_protocol_t proto);

/**
 * @typedef rfid_api_get_uid()
 * @brief API to read the UID
 *
 * @param dev RFID device
 * @param uid Buffer for the UID to be written
 * @param uid_len Length of the UID
 * @return 0 on success, negative on error
 */
typedef int (*rfid_api_get_uid)(const struct device *dev, uint8_t *uid, size_t *uid_len);

/**
 * @typedef rfid_api_transceive()
 * @brief API to transceive data betwenn TAG and Host
 *
 * @param dev RFID device
 * @param tx Buffer for data to be send
 * @param tx_len Length of the tx buffer
 * @param rx Buffer for data to be send
 * @param rx_len Length of the tx buffer
 * @return 0 on success, negative on error
 */
typedef int (*rfid_api_transceive)(const struct device *dev, const uint8_t *tx, size_t *tx_len,
					uint8_t *rx, size_t *rx_len);

/**
 * @typedef rfid_api_calibration()
 * @brief API for calibrating the CR95HF RFID device. Returns DacDataRef.
 *        Set DacDataH to DacDataRef+8 and DacDataL to DacDataRef-8
 *
 * @param dev RFID device
 * @return DacDataRef on success, negative on error
 */
typedef int (*rfid_api_calibration)(const struct device *dev);

/**
 * @brief RFID driver API
 */
struct rfid_driver_api {
	rfid_api_select_mode select_mode;
	rfid_api_protocol_select protocol_select;
	rfid_api_get_uid get_uid;
	rfid_api_transceive transceive;
	rfid_api_calibration calibration;
};

static inline int rfid_select_mode(const struct device *dev, rfid_mode_t req_mode)
{
	const struct rfid_driver_api *api = dev->api;

	return api->select_mode(dev, req_mode);
}

static inline int rfid_protocol_select(const struct device *dev, rfid_protocol_t proto)
{
	const struct rfid_driver_api *api = dev->api;

	return api->protocol_select(dev, proto);
}

static inline int rfid_get_uid(const struct device *dev, uint8_t *uid, size_t *uid_len)
{
	const struct rfid_driver_api *api = dev->api;

	return api->get_uid(dev, uid, uid_len);
}

static inline int rfid_transceive(const struct device *dev, const uint8_t *tx, size_t *tx_len,
									uint8_t *rx, size_t *rx_len)
{
	const struct rfid_driver_api *api = dev->api;

	return api->transceive(dev, tx, tx_len, rx, rx_len);
}

static inline int rfid_calibration(const struct device *dev)
{
	const struct rfid_driver_api *api = dev->api;

	return api->calibration(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_RFID_H_ */
