/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup nfc_interface
 * @brief Main header file for NFC driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NFC_H_
#define ZEPHYR_INCLUDE_DRIVERS_NFC_H_

/**
 * @brief Interface for Near Field Communication (NFC) devices.
 * @defgroup nfc_interface NFC
 * @since 4.6
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Max length for NFC ID */
#define NFC_ID_MAX_LEN 10

/** Max length for ats */
#define NFC_ATS_MAX_LEN 20

/**
 * @brief NFC target (tag) structure.
 */
struct nfc_target {
	/** Unique identifier of target */
	uint8_t nfc_id[NFC_ID_MAX_LEN];

	/** Unique identifier length */
	uint8_t nfc_id_len;

	/** Answer to Request command */
	uint16_t atq;

	/** Answer to Select command */
	uint8_t ats[NFC_ATS_MAX_LEN];

	/** Length in bytes of Answer to select */
	uint8_t ats_len;
};

/**
 * @brief NFC communication structure between NFC reader and target.
 */
struct nfc_comm {
	/** Buffer for data transmitted from NFC reader to target */
	struct net_buf_simple *tx_buf;

	/** Buffer for data received from target */
	struct net_buf_simple *rx_buf;

	/** Number of bits of last byte that will be transmitted */
	uint8_t valid_bits;
};

/**
 * @brief Callback API for enable.
 *
 * @see nfc_enable() for argument descriptions.
 */
typedef int (*nfc_api_enable)(const struct device *dev, bool enable);

/**
 * @brief Callback API for transceive.
 *
 * @see nfc_transceive() for argument descriptions.
 */
typedef int (*nfc_api_transceive)(const struct device *dev, struct nfc_comm *comm);

/**
 * @brief NFC driver API.
 */
__subsystem struct nfc_driver_api {
	nfc_api_enable enable;
	nfc_api_transceive transceive;
};

/**
 * @brief Turn on/off RF field of NFC reader device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable Boolean value on/off.
 *
 * @return 0 on success, negative on error.
 */
__syscall int nfc_enable(const struct device *dev, bool enable);

static inline int z_impl_nfc_enable(const struct device *dev, bool enable)
{
	const struct nfc_driver_api *api = (const struct nfc_driver_api *)dev->api;

	if (api->enable == NULL) {
		return -ENOSYS;
	}

	return api->enable(dev, enable);
}

/**
 * @brief Send/receive message between NFC reader device and target.
 *
 * @note This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param comm Pointer to comm structure containing tx and rx buffers.
 *
 * @return Number of bytes received on success, negative on error.
 * @retval -ENOBUFS Received message overflows rx_buf.
 * @retval -ETIMEDOUT No target device response.
 * @retval -EBADMSG Invalid message received.
 */
__syscall int nfc_transceive(const struct device *dev, struct nfc_comm *comm);

static inline int z_impl_nfc_transceive(const struct device *dev, struct nfc_comm *comm)
{
	const struct nfc_driver_api *api = (const struct nfc_driver_api *)dev->api;

	if (api->transceive == NULL) {
		return -ENOSYS;
	}

	return api->transceive(dev, comm);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/nfc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_NFC_H_ */
