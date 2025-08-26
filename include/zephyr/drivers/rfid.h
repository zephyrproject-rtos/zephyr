/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for RFID drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RFID_H_
#define ZEPHYR_INCLUDE_DRIVERS_RFID_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RFID Interface
 * @defgroup rfid_interface RFID Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name RFID protocol definitions
 * @anchor RFID_PROTO_DEFS
 * @{
 */

/** RFID protocol Jewel tag by Broadcom */
#define RFID_PROTO_JEWEL BIT(0)

/** RFID protocol MIFARE ICs by NXP */
#define RFID_PROTO_MIFARE BIT(1)

/** RFID protocol Felica by Sony */
#define RFID_PROTO_FELICA BIT(2)

/** RFID protocol ISO/IEC 14443 A */
#define RFID_PROTO_ISO14443A BIT(3)

/** RFID protocol ISO/IEC 14443 B */
#define RFID_PROTO_ISO14443B BIT(4)

/** RFID protocol ISO/IEC 7816-4 */
#define RFID_PROTO_RFID_DEP BIT(5)

/** RFID protocol ISO/IEC 15693 */
#define RFID_PROTO_ISO15693 BIT(6)

/** @} */

/**
 * @brief Provides a type to hold RFID protocol definitions.
 *
 * @see @ref RFID_PROTO_DEFS.
 */
typedef uint32_t rfid_proto_t;

/**
 * @name RFID mode flags
 * @anchor RFID_MODE_FLAGS
 * @{
 */

/** RFID initiator mode */
#define RFID_MODE_INITIATOR BIT(0)

/** RFID target mode */
#define RFID_MODE_TARGET BIT(1)

/** RFID peer-to-peer mode */
#define RFID_MODE_P2P BIT(2)

/** Helper mask to get Initiator/Target/P2P mode */
#define RFID_MODE_ROLE_MASK (RFID_MODE_INITIATOR | RFID_MODE_TARGET | RFID_MODE_P2P)

/** RFID baudrate in both directions have to be the same */
#define RFID_MODE_TX_RX_SAME_RATE BIT(3)

/** RFID transmit baudrate of 106 Kbps */
#define RFID_MODE_TX_106 BIT(4)

/** RFID transmit baudrate of 212 Kbps */
#define RFID_MODE_TX_212 BIT(5)

/** RFID transmit baudrate of 424 Kbps */
#define RFID_MODE_TX_424 BIT(6)

/** RFID transmit baudrate of 848 Kbps */
#define RFID_MODE_TX_848 BIT(7)

/** Helper mask to get TX modes */
#define RFID_MODE_TX_MASK                                                                          \
	(RFID_MODE_TX_106 | RFID_MODE_TX_212 | RFID_MODE_TX_424 | RFID_MODE_TX_848)

/** RFID receive baudrate of 106 Kbps */
#define RFID_MODE_RX_106 BIT(8)

/** RFID receive baudrate of 212 Kbps */
#define RFID_MODE_RX_212 BIT(9)

/** RFID receive baudrate of 424 Kbps */
#define RFID_MODE_RX_424 BIT(10)

/** RFID receive baudrate of 848 Kbps */
#define RFID_MODE_RX_848 BIT(11)

/** Helper mask to get RX modes */
#define RFID_MODE_RX_MASK                                                                          \
	(RFID_MODE_RX_106 | RFID_MODE_RX_212 | RFID_MODE_RX_424 | RFID_MODE_RX_848)

/** @} */

/**
 * @brief Provides a type to hold RFID mode flags.
 *
 * @see @ref RFID_MODE_FLAGS.
 */
typedef uint32_t rfid_mode_t;

/**
 * @brief Defines the properties for an RFID device driver.
 */
enum rfid_property_type {
	/** Turn RF field on/off (field rf_on) */
	RFID_PROP_RF_FIELD,

	/** Turn HW CRC on/off for transmitting (field hw_tx_crc) */
	RFID_PROP_HW_TX_CRC,

	/** Turn HW CRC on/off for receiving (field hw_rx_crc) */
	RFID_PROP_HW_RX_CRC,

	/** Turn HW parity on/off (field hw_parity) */
	RFID_PROP_HW_PARITY,

	/** Turn MIFARE Classic crypto on/off (field mfc_crypto_on) */
	RFID_PROP_MFC_CRYPTO,

	/** Timeout in microseconds to wait for a response (field timeout_us) */
	RFID_PROP_TIMEOUT,

	/** Time in microseconds to wait before sending data (field tx_guard_us) */
	RFID_PROP_TX_GUARD_TIME,

	/** Time in microseconds to wait before receiving incoming data (field rx_guard_us) */
	RFID_PROP_RX_GUARD_TIME,

	/** Generate a random UID (field random_uid) */
	RFID_PROP_RANDOM_UID,

	/** Response value to ReqA (ATQA) (field sens_res) */
	RFID_PROP_SENS_RES,

	/** Response value to SelectA (field sel_res) */
	RFID_PROP_SEL_RES,
};

/**
 * @brief RFID property structure
 */
struct rfid_property {
	/** RFID property type to access */
	enum rfid_property_type type;

	/** Negative error status set by callee e.g. -ENOTSUP for an unsupported property */
	int status;

	union {
		/* Fields have the format: */
		/* RFID_PROPERTY_FIELD */
		/* type property_field; */

		/* RFID_PROP_RF_FIELD */
		bool rf_on;
		/* RFID_PROP_HW_TX_CRC */
		bool hw_tx_crc;
		/* RFID_PROP_HW_RX_CRC */
		bool hw_rx_crc;
		/* RFID_PROP_HW_PARITY */
		bool hw_parity;
		/* RFID_PROP_MFC_CRYPTO */
		bool mfc_crypto_on;
		/* RFID_PROP_TIMEOUT */
		uint32_t timeout_us;
		/* RFID_PROP_TX_GUARD_TIME */
		uint32_t tx_guard_us;
		/* RFID_PROP_RX_GUARD_TIME */
		uint32_t rx_guard_us;
		/* RFID_PROP_RANDOM_UID */
		bool random_uid;
		/* RFID_PROP_SENS_RES */
		uint8_t sens_res[2];
		/* RFID_PROP_SEL_RES */
		uint8_t sel_res;
	};
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal driver use only, skip these in public documentation.
 */

/**
 * @brief Callback API upon claiming an RFID device.
 * See @a rfid_claim() for argument descriptions
 */
typedef int (*rfid_claim_t)(const struct device *dev);

/**
 * @brief Callback API upon releasing an RFID device.
 * See @a rfid_release() for argument descriptions
 */
typedef int (*rfid_release_t)(const struct device *dev);

/**
 * @brief Callback API upon loading an RFID protocol.
 * See @a rfid_load_protocol() for argument descriptions
 */
typedef int (*rfid_load_protocol_t)(const struct device *dev, rfid_proto_t proto, rfid_mode_t mode);

/**
 * @brief Callback API upon getting RFID properties.
 * See @a rfid_get_property() for argument descriptions
 */
typedef int (*rfid_properties_get_t)(const struct device *dev, struct rfid_property *props,
				     size_t props_len);

/**
 * @brief Callback API upon setting RFID properties.
 * See @a rfid_set_property() for argument descriptions
 */
typedef int (*rfid_properties_set_t)(const struct device *dev, struct rfid_property *props,
				     size_t props_len);

/**
 * @brief Callback API upon sending and receiving data as initiator/reader.
 * See @a rfid_initiator_transceive() for argument descriptions
 */
typedef int (*rfid_initiator_transceive_t)(const struct device *dev, const uint8_t *tx_data,
					   uint16_t tx_len, uint8_t tx_last_bits, uint8_t *rx_data,
					   uint16_t *rx_len);

/**
 * @brief Callback API upon transmitting as target.
 * See @a rfid_target_send() for argument descriptions
 */
typedef int (*rfid_target_transmit_t)(const struct device *dev, const uint8_t *tx_data,
				      uint16_t tx_len, uint8_t tx_last_bits);

/**
 * @brief Callback API upon receiving as target.
 * See @a rfid_target_send() for argument descriptions
 */
typedef int (*rfid_target_receive_t)(const struct device *dev, uint8_t *rx_data, uint16_t *rx_len);

/**
 * @brief Callback API upon listening for RFID data.
 * See @a rfid_listen() for argument descriptions
 */
typedef int (*rfid_listen_t)(const struct device *dev, rfid_proto_t proto, uint8_t *rx_data,
			     uint16_t *rx_len);

/**
 * @brief Callback API upon getting the RFID device driver supported protocols.
 * See @a rfid_supported_protocols() for argument descriptions
 */
typedef rfid_proto_t (*rfid_supported_protocols_t)(const struct device *dev);

/**
 * @brief Callback API upon getting the RFID device driver supported modes.
 * See @a rfid_supported_modes() for argument descriptions
 */
typedef rfid_mode_t (*rfid_supported_modes_t)(const struct device *dev, rfid_proto_t proto);

__subsystem struct rfid_driver_api {
	rfid_claim_t claim;
	rfid_release_t release;
	rfid_load_protocol_t load_protocol;
	rfid_properties_get_t get_properties;
	rfid_properties_set_t set_properties;
	rfid_initiator_transceive_t im_transceive;
	rfid_listen_t listen;
	rfid_target_transmit_t tm_transmit;
	rfid_target_receive_t tm_receive;
	rfid_supported_protocols_t supported_protocols;
	rfid_supported_modes_t supported_modes;
};

/** @endcond */

/**
 * @name RFID controller configuration
 *
 * @{
 */

/**
 * @brief
 *
 * TODO
 */
__syscall rfid_proto_t rfid_claim(const struct device *dev);

static inline rfid_proto_t z_impl_rfid_claim(const struct device *dev)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	return api->claim(dev);
}

/**
 * @brief
 *
 * TODO
 */
__syscall rfid_proto_t rfid_release(const struct device *dev);

static inline rfid_proto_t z_impl_rfid_release(const struct device *dev)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	return api->release(dev);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_load_protocol(const struct device *dev, rfid_proto_t proto, rfid_mode_t mode);

static inline int z_impl_rfid_load_protocol(const struct device *dev, rfid_proto_t proto,
					    rfid_mode_t mode)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->load_protocol == NULL) {
		return -ENOSYS;
	}

	return api->load_protocol(dev, proto, mode);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_get_properties(const struct device *dev, struct rfid_property *props,
				  size_t props_len);

static inline int z_impl_rfid_get_properties(const struct device *dev, struct rfid_property *props,
					     size_t props_len)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->get_properties == NULL) {
		return -ENOSYS;
	}

	return api->get_properties(dev, props, props_len);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_set_properties(const struct device *dev, struct rfid_property *props,
				  size_t props_len);

static inline int z_impl_rfid_set_properties(const struct device *dev, struct rfid_property *props,
					     size_t props_len)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->set_properties == NULL) {
		return -ENOSYS;
	}

	return api->set_properties(dev, props, props_len);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_initiator_transceive(const struct device *dev, const uint8_t *tx_data,
					uint16_t tx_len, uint8_t tx_last_bits, uint8_t *rx_data,
					uint16_t *rx_len);

static inline int z_impl_rfid_initiator_transceive(const struct device *dev, const uint8_t *tx_data,
						   uint16_t tx_len, uint8_t tx_last_bits,
						   uint8_t *rx_data, uint16_t *rx_len)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->im_transceive == NULL) {
		return -ENOSYS;
	}

	return api->im_transceive(dev, tx_data, tx_len, tx_last_bits, rx_data, rx_len);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_target_transmit(const struct device *dev, const uint8_t *tx_data,
				   uint16_t tx_len, uint8_t tx_last_bits);

static inline int z_impl_rfid_target_transmit(const struct device *dev, const uint8_t *tx_data,
					      uint16_t tx_len, uint8_t tx_last_bits)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->tm_transmit == NULL) {
		return -ENOSYS;
	}

	return api->tm_transmit(dev, tx_data, tx_len, tx_last_bits);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_target_receive(const struct device *dev, uint8_t *rx_data, uint16_t *rx_len);

static inline int z_impl_rfid_target_receive(const struct device *dev, uint8_t *rx_data,
					     uint16_t *rx_len)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->tm_receive == NULL) {
		return -ENOSYS;
	}

	return api->tm_receive(dev, rx_data, rx_len);
}

/**
 * @brief
 *
 * TODO
 */
__syscall int rfid_listen(const struct device *dev, rfid_proto_t proto, uint8_t *rx_data,
			  uint16_t *rx_len);

static inline int z_impl_rfid_listen(const struct device *dev, rfid_proto_t proto, uint8_t *rx_data,
				     uint16_t *rx_len)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->listen == NULL) {
		return -ENOSYS;
	}

	return api->listen(dev, proto, rx_data, rx_len);
}

/**
 * @brief
 *
 * TODO
 */
__syscall rfid_proto_t rfid_supported_protocols(const struct device *dev);

static inline rfid_proto_t z_impl_rfid_supported_protocols(const struct device *dev)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->supported_protocols == NULL) {
		return -ENOSYS;
	}

	return api->supported_protocols(dev);
}

/**
 * @brief
 *
 * TODO
 */
__syscall rfid_mode_t rfid_supported_modes(const struct device *dev, rfid_proto_t proto);

static inline rfid_mode_t z_impl_rfid_supported_modes(const struct device *dev, rfid_proto_t proto)
{
	const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;

	if (api->supported_modes == NULL) {
		return -ENOSYS;
	}

	return api->supported_modes(dev, proto);
}

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rfid.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RFID_H_ */
