/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NFC_H_
#define ZEPHYR_INCLUDE_DRIVERS_NFC_H_

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
 * @brief NFC Interface
 * @defgroup nfc_interface NFC Interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name NFC protocol definitions
 * @anchor NFC_PROTO_DEFS
 * @{
 */

/** NFC protocol Jewel tag by Broadcom */
#define NFC_PROTO_JEWEL BIT(0)

/** NFC protocol MIFARE ICs by NXP */
#define NFC_PROTO_MIFARE BIT(1)

/** NFC protocol Felica by Sony */
#define NFC_PROTO_FELICA BIT(2)

/** NFC protocol ISO/IEC 14443 A */
#define NFC_PROTO_ISO14443A BIT(3)

/** NFC protocol ISO/IEC 14443 B */
#define NFC_PROTO_ISO14443B BIT(4)

/** NFC protocol NFC-DEP (ISO/IEC 18092) */
#define NFC_PROTO_NFC_DEP BIT(5)

/** NFC protocol ISO/IEC 15693 */
#define NFC_PROTO_ISO15693 BIT(6)

/** @} */

/**
 * @brief NFC protocol type, a bitmask of @ref NFC_PROTO_DEFS.
 */
typedef uint32_t nfc_proto_t;

/**
 * @name NFC mode flags
 * @anchor NFC_MODE_FLAGS
 * @{
 */

/** NFC initiator mode */
#define NFC_MODE_INITIATOR BIT(0)

/** NFC target mode */
#define NFC_MODE_TARGET BIT(1)

/** NFC peer-to-peer mode */
#define NFC_MODE_P2P BIT(2)

/** Helper mask to get Initiator/Target/P2P mode */
#define NFC_MODE_ROLE_MASK (NFC_MODE_INITIATOR | NFC_MODE_TARGET | NFC_MODE_P2P)

/** NFC baudrate in both directions have to be the same */
#define NFC_MODE_TX_RX_SAME_RATE BIT(3)

/** NFC transmit baudrate of 106 Kbps */
#define NFC_MODE_TX_106 BIT(4)

/** NFC transmit baudrate of 212 Kbps */
#define NFC_MODE_TX_212 BIT(5)

/** NFC transmit baudrate of 424 Kbps */
#define NFC_MODE_TX_424 BIT(6)

/** NFC transmit baudrate of 848 Kbps */
#define NFC_MODE_TX_848 BIT(7)

/** Helper mask to get TX modes */
#define NFC_MODE_TX_MASK (NFC_MODE_TX_106 | NFC_MODE_TX_212 | NFC_MODE_TX_424 | NFC_MODE_TX_848)

/** NFC receive baudrate of 106 Kbps */
#define NFC_MODE_RX_106 BIT(8)

/** NFC receive baudrate of 212 Kbps */
#define NFC_MODE_RX_212 BIT(9)

/** NFC receive baudrate of 424 Kbps */
#define NFC_MODE_RX_424 BIT(10)

/** NFC receive baudrate of 848 Kbps */
#define NFC_MODE_RX_848 BIT(11)

/** Helper mask to get RX modes */
#define NFC_MODE_RX_MASK (NFC_MODE_RX_106 | NFC_MODE_RX_212 | NFC_MODE_RX_424 | NFC_MODE_RX_848)

/** @} */

/**
 * @brief Provides a type to hold NFC mode flags.
 *
 * @see @ref NFC_MODE_FLAGS.
 */
typedef uint32_t nfc_mode_t;

/**
 * @brief Defines the properties for an NFC device driver.
 */
enum nfc_property_type {
	/** Turn RF field on/off (field rf_on) */
	NFC_PROP_RF_FIELD,

	/** Turn HW CRC on/off for transmitting (field hw_tx_crc) */
	NFC_PROP_HW_TX_CRC,

	/** Turn HW CRC on/off for receiving (field hw_rx_crc) */
	NFC_PROP_HW_RX_CRC,

	/** Turn HW parity on/off (field hw_parity) */
	NFC_PROP_HW_PARITY,

	/** Turn MIFARE Classic crypto on/off (field mfc_crypto_on) */
	NFC_PROP_MFC_CRYPTO,

	/**
	 * Microseconds to wait for a response to start, that is the frame delay
	 * time (field timeout_us). A controller adds the time the response
	 * itself occupies on the air.
	 */
	NFC_PROP_TIMEOUT,

	/** Time in microseconds to wait before sending data (field tx_guard_us) */
	NFC_PROP_TX_GUARD_TIME,

	/** Time in microseconds to wait before receiving incoming data (field rx_guard_us) */
	NFC_PROP_RX_GUARD_TIME,

	/** Generate a random UID (field random_uid) */
	NFC_PROP_RANDOM_UID,

	/** Response value to ReqA (ATQA) (field sens_res) */
	NFC_PROP_SENS_RES,

	/** Response value to SelectA (field sel_res) */
	NFC_PROP_SEL_RES,
};

/**
 * @brief NFC property structure
 */
struct nfc_property {
	/** NFC property type to access */
	enum nfc_property_type type;

	/** Negative error status set by callee e.g. -ENOTSUP for an unsupported property */
	int status;

	union {
		/** NFC_PROP_RF_FIELD */
		bool rf_on;
		/** NFC_PROP_HW_TX_CRC */
		bool hw_tx_crc;
		/** NFC_PROP_HW_RX_CRC */
		bool hw_rx_crc;
		/** NFC_PROP_HW_PARITY */
		bool hw_parity;
		/** NFC_PROP_MFC_CRYPTO */
		bool mfc_crypto_on;
		/** NFC_PROP_TIMEOUT */
		uint32_t timeout_us;
		/** NFC_PROP_TX_GUARD_TIME */
		uint32_t tx_guard_us;
		/** NFC_PROP_RX_GUARD_TIME */
		uint32_t rx_guard_us;
		/** NFC_PROP_RANDOM_UID */
		bool random_uid;
		/** NFC_PROP_SENS_RES */
		uint8_t sens_res[2];
		/** NFC_PROP_SEL_RES */
		uint8_t sel_res;
	};
};

/**
 * @brief Maximum length of an NFC target UID (ISO/IEC 14443 triple size).
 */
#define NFC_UID_MAXLEN 10U

/**
 * @brief Maximum length of an ATS a target report can carry.
 *
 * Covers TL, T0, TA(1), TB(1), TC(1) and the historical bytes the ISO-DEP
 * layer keeps. A longer ATS is not reported at all, rather than truncated
 * into something a parser would have to reject.
 */
#define NFC_ATS_MAXLEN 37U

/**
 * @brief RF technology a target was discovered on.
 *
 * Selects which member of @ref nfc_target is valid, so a single discovery
 * result can carry NFC-A/B/F/V without an NFC-A-only assumption.
 */
enum nfc_tech {
	NFC_TECH_A,
	NFC_TECH_B,
	NFC_TECH_F,
	NFC_TECH_V,
};

/** @brief NFC-A activation data (ISO/IEC 14443-3 Type A). */
struct nfc_target_a {
	/** Unique identifier (NFCID1) */
	uint8_t uid[NFC_UID_MAXLEN];
	/** Number of valid bytes in @ref nfc_target_a.uid */
	uint8_t uid_len;
	/** Select acknowledge (SAK) */
	uint8_t sak;
	/** Answer to request (ATQA / SENS_RES) */
	uint8_t atqa[2];
	/**
	 * Answer to select, starting at TL.
	 *
	 * Filled by a controller that activates ISO-DEP itself. Left empty by
	 * one that does not, in which case nfc_iso_dep_connect() obtains it
	 * with RATS.
	 */
	uint8_t ats[NFC_ATS_MAXLEN];
	/** Number of valid bytes in @ref nfc_target_a.ats */
	uint8_t ats_len;
};

/** @brief NFC-B activation data (ISO/IEC 14443-3 Type B). */
struct nfc_target_b {
	/** Pseudo-unique PICC identifier from ATQB */
	uint8_t pupi[4];
	/** Application data from ATQB */
	uint8_t app_data[4];
	/** Protocol info from ATQB */
	uint8_t proto_info[3];
};

/** @brief NFC-F activation data (JIS X 6319-4 / FeliCa). */
struct nfc_target_f {
	/** NFCID2 (IDm) from SENSF_RES */
	uint8_t nfcid2[8];
	/** Manufacture parameters (PMm) from SENSF_RES */
	uint8_t pmm[8];
};

/** @brief NFC-V activation data (ISO/IEC 15693). */
struct nfc_target_v {
	/** 8-byte UID */
	uint8_t uid[8];
	/** Data storage format identifier */
	uint8_t dsfid;
};

/**
 * @brief Information about a discovered NFC target.
 *
 * Filled in by an offloading controller that performs collision resolution and
 * activation in firmware (see @ref nfc_offload_poll_start). @a tech selects the
 * valid union member.
 */
struct nfc_target {
	/** RF technology, selects the valid union member */
	enum nfc_tech tech;
	/** Detected protocol, see @ref NFC_PROTO_DEFS */
	nfc_proto_t proto;
	union {
		struct nfc_target_a a;
		struct nfc_target_b b;
		struct nfc_target_f f;
		struct nfc_target_v v;
	};
};

/**
 * @brief Get the identifier a target answered with.
 *
 * Every technology carries one under a different name, so this resolves the
 * union member for a caller that only wants to tell targets apart.
 *
 * @param target Discovered target.
 * @param len Out: length of the returned identifier.
 *
 * @return Identifier bytes, valid as long as @p target is.
 */
static inline const uint8_t *nfc_target_uid(const struct nfc_target *target, uint8_t *len)
{
	switch (target->tech) {
	case NFC_TECH_A:
		*len = target->a.uid_len;
		return target->a.uid;
	case NFC_TECH_B:
		*len = sizeof(target->b.pupi);
		return target->b.pupi;
	case NFC_TECH_F:
		*len = sizeof(target->f.nfcid2);
		return target->f.nfcid2;
	case NFC_TECH_V:
		*len = sizeof(target->v.uid);
		return target->v.uid;
	default:
		*len = 0U;
		return NULL;
	}
}

/**
 * @brief Technology-specific discovery (polling) parameters for
 * @ref nfc_offload_poll_start. Flat (not a union) because @a protos may request
 * several technologies at once. NULL selects per-technology defaults.
 */
struct nfc_poll_config {
	/** NFC-B: application family identifier for REQB */
	uint8_t afi;
	/** NFC-B: number of REQB anticollision slots */
	uint8_t slot_count;
	/** NFC-F: SENSF_REQ system code (0xFFFF = wildcard) */
	uint16_t system_code;
	/** NFC-F: SENSF_REQ request code */
	uint8_t request_code;
};

/**
 * @brief Callback invoked when an offloading controller discovers a target.
 *
 * @param dev NFC device.
 * @param target Discovered target information.
 * @param user_data Opaque pointer passed to @ref nfc_offload_poll_start.
 */
typedef void (*nfc_target_cb_t)(const struct device *dev, const struct nfc_target *target,
				void *user_data);

/**
 * @brief Events reported while acting as an NFC target (card emulation).
 */
enum nfc_target_event {
	/** A poller selected this device as a target. */
	NFC_TARGET_SELECTED,
	/** A frame was received from the poller (@p data / @p len are valid). */
	NFC_TARGET_FRAME,
	/** The poller released this device or the RF field was lost. */
	NFC_TARGET_DESELECTED,
};

/**
 * @brief Callback invoked on NFC target-mode events.
 *
 * Event-driven target front-ends (e.g. the nRF NFCT peripheral) may invoke
 * this callback from interrupt context. The handler must be ISR-safe and defer
 * any heavy processing; the @p data buffer is only valid for the duration of
 * the call.
 *
 * @param dev NFC device.
 * @param event Event type, see @ref nfc_target_event.
 * @param data Received frame for @ref NFC_TARGET_FRAME, otherwise NULL.
 * @param len Length of @p data.
 * @param user_data Opaque pointer passed to @ref nfc_target_start.
 */
typedef void (*nfc_target_event_cb_t)(const struct device *dev, enum nfc_target_event event,
				      const uint8_t *data, uint16_t len, void *user_data);

/**
 * @def_driverbackendgroup{NFC,nfc_interface}
 * @{
 */

/**
 * @brief Callback API upon claiming an NFC device.
 * See @a nfc_claim() for argument descriptions.
 */
typedef nfc_proto_t (*nfc_claim_t)(const struct device *dev);

/**
 * @brief Callback API upon releasing an NFC device.
 * See @a nfc_release() for argument descriptions.
 */
typedef int (*nfc_release_t)(const struct device *dev);

/**
 * @brief Callback API upon loading an NFC protocol.
 * See @a nfc_load_protocol() for argument descriptions.
 */
typedef int (*nfc_load_protocol_t)(const struct device *dev, nfc_proto_t proto, nfc_mode_t mode);

/**
 * @brief Callback API upon getting NFC properties.
 * See @a nfc_get_properties() for argument descriptions.
 */
typedef int (*nfc_properties_get_t)(const struct device *dev, struct nfc_property *props,
				    size_t props_len);

/**
 * @brief Callback API upon setting NFC properties.
 * See @a nfc_set_properties() for argument descriptions.
 */
typedef int (*nfc_properties_set_t)(const struct device *dev, struct nfc_property *props,
				    size_t props_len);

/**
 * @brief Callback API upon sending and receiving data as initiator/reader.
 * See @a nfc_initiator_transceive() for argument descriptions.
 */
typedef int (*nfc_initiator_transceive_t)(const struct device *dev, const uint8_t *tx_data,
					  uint16_t tx_len, uint8_t tx_last_bits, uint8_t *rx_data,
					  uint16_t *rx_len);

/**
 * @brief Callback API to start asynchronous target (listen) mode.
 * See @a nfc_target_start() for argument descriptions.
 */
typedef int (*nfc_target_start_t)(const struct device *dev, nfc_proto_t protos,
				  nfc_target_event_cb_t cb, void *user_data);

/**
 * @brief Callback API to stop target mode.
 * See @a nfc_target_stop() for argument descriptions.
 */
typedef int (*nfc_target_stop_t)(const struct device *dev);

/**
 * @brief Callback API to send a response frame while in target mode.
 * See @a nfc_target_send() for argument descriptions.
 */
typedef int (*nfc_target_send_t)(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
				 uint8_t tx_last_bits);

/**
 * @brief Callback API upon getting the NFC device driver supported protocols.
 * See @a nfc_supported_protocols() for argument descriptions.
 */
typedef nfc_proto_t (*nfc_supported_protocols_t)(const struct device *dev);

/**
 * @brief Callback API upon getting the NFC device driver supported modes.
 * See @a nfc_supported_modes() for argument descriptions.
 */
typedef nfc_mode_t (*nfc_supported_modes_t)(const struct device *dev, nfc_proto_t proto);

/**
 * @brief Callback API to start offloaded target discovery.
 * See @a nfc_offload_poll_start() for argument descriptions.
 */
typedef int (*nfc_offload_poll_start_t)(const struct device *dev, nfc_proto_t protos,
					const struct nfc_poll_config *cfg, nfc_target_cb_t cb,
					void *user_data);

/**
 * @brief Callback API to stop offloaded target discovery.
 * See @a nfc_offload_poll_stop() for argument descriptions.
 */
typedef int (*nfc_offload_poll_stop_t)(const struct device *dev);

/**
 * @brief Callback API to exchange data with a target activated in firmware.
 * See @a nfc_offload_exchange() for argument descriptions.
 */
typedef int (*nfc_offload_exchange_t)(const struct device *dev, const struct nfc_target *target,
				      const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
				      uint16_t *rx_len, uint32_t timeout_ms);

/**
 * @brief Callback API to let go of a target activated in firmware.
 * See @a nfc_offload_release() for argument descriptions.
 */
typedef int (*nfc_offload_release_t)(const struct device *dev, const struct nfc_target *target);

/**
 * @driver_ops{NFC}
 *
 * A driver implements the set its controller class calls for: the frontend
 * operations, the offload operations, or the target operations. Only the two
 * capability queries are common to all of them.
 */
__subsystem struct nfc_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief nfc_supported_protocols
	 */
	nfc_supported_protocols_t supported_protocols;
	/**
	 * @driver_ops_mandatory @copybrief nfc_supported_modes
	 */
	nfc_supported_modes_t supported_modes;
	/**
	 * @driver_ops_optional @copybrief nfc_claim
	 */
	nfc_claim_t claim;
	/**
	 * @driver_ops_optional @copybrief nfc_release
	 */
	nfc_release_t release;
	/**
	 * @driver_ops_optional @copybrief nfc_load_protocol
	 */
	nfc_load_protocol_t load_protocol;
	/**
	 * @driver_ops_optional @copybrief nfc_get_properties
	 */
	nfc_properties_get_t get_properties;
	/**
	 * @driver_ops_optional @copybrief nfc_set_properties
	 */
	nfc_properties_set_t set_properties;
	/**
	 * @driver_ops_optional @copybrief nfc_initiator_transceive
	 */
	nfc_initiator_transceive_t im_transceive;
	/**
	 * @driver_ops_optional @copybrief nfc_offload_poll_start
	 */
	nfc_offload_poll_start_t offload_poll_start;
	/**
	 * @driver_ops_optional @copybrief nfc_offload_poll_stop
	 */
	nfc_offload_poll_stop_t offload_poll_stop;
	/**
	 * @driver_ops_optional @copybrief nfc_offload_exchange
	 */
	nfc_offload_exchange_t offload_exchange;
	/**
	 * @driver_ops_optional @copybrief nfc_offload_release
	 */
	nfc_offload_release_t offload_release;
	/**
	 * @driver_ops_optional @copybrief nfc_target_start
	 */
	nfc_target_start_t target_start;
	/**
	 * @driver_ops_optional @copybrief nfc_target_stop
	 */
	nfc_target_stop_t target_stop;
	/**
	 * @driver_ops_optional @copybrief nfc_target_send
	 */
	nfc_target_send_t target_send;
};

/** @} */

/**
 * @name NFC controller configuration
 *
 * @{
 */

/**
 * @brief Acquire exclusive access to the NFC device.
 *
 * Locks the device for the calling context until nfc_release(). Frontend drivers
 * use this to serialise a multi-step protocol sequence.
 *
 * @param dev NFC device.
 *
 * @return The protocol the device is claimed for, see @ref NFC_PROTO_DEFS.
 */
__syscall nfc_proto_t nfc_claim(const struct device *dev);

static inline nfc_proto_t z_impl_nfc_claim(const struct device *dev)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->claim == NULL) {
		return 0;
	}

	return api->claim(dev);
}

/**
 * @brief Release exclusive access acquired with nfc_claim().
 *
 * @param dev NFC device.
 *
 * @retval 0 on success.
 * @retval -errno on failure.
 */
__syscall int nfc_release(const struct device *dev);

static inline int z_impl_nfc_release(const struct device *dev)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->release == NULL) {
		return 0;
	}

	return api->release(dev);
}

/**
 * @brief Select the active protocol and mode for subsequent exchanges.
 *
 * This also establishes the technology-dependent framing context. Until the
 * next call, @ref nfc_initiator_transceive carries the layer-3/4 payload while
 * the driver adds the on-air framing (short frame / parity for NFC-A, the LEN
 * prefix for NFC-F, and so on), and the CRC variant applied when
 * @ref NFC_PROP_HW_TX_CRC / @ref NFC_PROP_HW_RX_CRC are enabled follows the
 * selected protocol (CRC_A, CRC_B or the FeliCa CRC).
 *
 * @param dev NFC device.
 * @param proto Protocol to load, see @ref NFC_PROTO_DEFS.
 * @param mode Mode flags, see @ref NFC_MODE_FLAGS.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the protocol or mode is not supported.
 */
__syscall int nfc_load_protocol(const struct device *dev, nfc_proto_t proto, nfc_mode_t mode);

static inline int z_impl_nfc_load_protocol(const struct device *dev, nfc_proto_t proto,
					   nfc_mode_t mode)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->load_protocol == NULL) {
		return -ENOSYS;
	}

	return api->load_protocol(dev, proto, mode);
}

/**
 * @brief Read one or more device properties.
 *
 * Each element's @a type selects the property; the driver fills the matching
 * union member and per-element @a status.
 *
 * @param dev NFC device.
 * @param props Array of properties to read.
 * @param props_len Number of elements in @p props.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if property access is not supported.
 */
__syscall int nfc_get_properties(const struct device *dev, struct nfc_property *props,
				 size_t props_len);

static inline int z_impl_nfc_get_properties(const struct device *dev, struct nfc_property *props,
					    size_t props_len)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->get_properties == NULL) {
		return -ENOSYS;
	}

	return api->get_properties(dev, props, props_len);
}

/**
 * @brief Write one or more device properties.
 *
 * Each element's @a type selects the property to write from its union member;
 * the driver reports per-element @a status (e.g. -ENOTSUP for unsupported ones).
 *
 * @param dev NFC device.
 * @param props Array of properties to write.
 * @param props_len Number of elements in @p props.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if property access is not supported.
 */
__syscall int nfc_set_properties(const struct device *dev, struct nfc_property *props,
				 size_t props_len);

static inline int z_impl_nfc_set_properties(const struct device *dev, struct nfc_property *props,
					    size_t props_len)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->set_properties == NULL) {
		return -ENOSYS;
	}

	return api->set_properties(dev, props, props_len);
}

/**
 * @brief Transmit a frame and receive the response (initiator mode).
 *
 * Sends the layer-3/4 payload and returns the target's response. Framing and
 * the CRC variant follow the loaded protocol (see nfc_load_protocol()); the
 * response wait is bounded by @ref NFC_PROP_TIMEOUT.
 *
 * @param dev NFC device.
 * @param tx_data Bytes to transmit.
 * @param tx_len Length of @p tx_data.
 * @param tx_last_bits Valid bits in the last TX byte (0 = whole byte), for short
 *        frames such as SENS_REQ.
 * @param rx_data Response buffer.
 * @param rx_len In: capacity of @p rx_data. Out: bytes received.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if initiator transceive is not supported.
 */
__syscall int nfc_initiator_transceive(const struct device *dev, const uint8_t *tx_data,
				       uint16_t tx_len, uint8_t tx_last_bits, uint8_t *rx_data,
				       uint16_t *rx_len);

static inline int z_impl_nfc_initiator_transceive(const struct device *dev, const uint8_t *tx_data,
						  uint16_t tx_len, uint8_t tx_last_bits,
						  uint8_t *rx_data, uint16_t *rx_len)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->im_transceive == NULL) {
		return -ENOSYS;
	}

	return api->im_transceive(dev, tx_data, tx_len, tx_last_bits, rx_data, rx_len);
}

/**
 * @brief Start asynchronous target (listen) mode.
 *
 * The device acts as an NFC target; @p cb is invoked from the driver context
 * on each target-mode event. This is not a syscall.
 *
 * @param dev NFC device.
 * @param protos Bitmask of protocols to listen for, see @ref NFC_PROTO_DEFS.
 * @param cb Callback invoked on target-mode events.
 * @param user_data Opaque pointer passed to @p cb.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if target mode is not supported.
 */
static inline int nfc_target_start(const struct device *dev, nfc_proto_t protos,
				   nfc_target_event_cb_t cb, void *user_data)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->target_start == NULL) {
		return -ENOSYS;
	}

	return api->target_start(dev, protos, cb, user_data);
}

/**
 * @brief Stop target mode.
 *
 * @param dev NFC device.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if target mode is not supported.
 */
static inline int nfc_target_stop(const struct device *dev)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->target_stop == NULL) {
		return -ENOSYS;
	}

	return api->target_stop(dev);
}

/**
 * @brief Send a response frame while in target mode.
 *
 * @param dev NFC device.
 * @param tx_data Data to send.
 * @param tx_len Length of @p tx_data.
 * @param tx_last_bits Number of valid bits in the last byte (0 means all 8).
 *
 * @retval 0 on success.
 * @retval -ENOSYS if target mode is not supported.
 */
static inline int nfc_target_send(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
				  uint8_t tx_last_bits)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->target_send == NULL) {
		return -ENOSYS;
	}

	return api->target_send(dev, tx_data, tx_len, tx_last_bits);
}

/**
 * @brief Get the protocols the device supports.
 *
 * @param dev NFC device.
 *
 * @return Bitmask of supported protocols, see @ref NFC_PROTO_DEFS (0 if none).
 */
__syscall nfc_proto_t nfc_supported_protocols(const struct device *dev);

static inline nfc_proto_t z_impl_nfc_supported_protocols(const struct device *dev)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->supported_protocols == NULL) {
		return 0;
	}

	return api->supported_protocols(dev);
}

/**
 * @brief Get the modes the device supports for a protocol.
 *
 * @param dev NFC device.
 * @param proto Protocol to query, see @ref NFC_PROTO_DEFS.
 *
 * @return Bitmask of supported modes, see @ref NFC_MODE_FLAGS (0 if none).
 */
__syscall nfc_mode_t nfc_supported_modes(const struct device *dev, nfc_proto_t proto);

static inline nfc_mode_t z_impl_nfc_supported_modes(const struct device *dev, nfc_proto_t proto)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->supported_modes == NULL) {
		return 0;
	}

	return api->supported_modes(dev, proto);
}

/**
 * @brief Start offloaded target discovery.
 *
 * Controllers that perform collision resolution and activation in firmware
 * report discovered targets through @p cb. This is not a syscall: @p cb is
 * invoked from the driver context (typically an IRQ work item).
 *
 * @param dev NFC device.
 * @param protos Bitmask of protocols to poll for, see @ref NFC_PROTO_DEFS.
 * @param cfg Technology-specific poll parameters, or NULL for defaults.
 * @param cb Callback invoked for each discovered target.
 * @param user_data Opaque pointer passed to @p cb.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if offloaded polling is not supported.
 */
static inline int nfc_offload_poll_start(const struct device *dev, nfc_proto_t protos,
					 const struct nfc_poll_config *cfg, nfc_target_cb_t cb,
					 void *user_data)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->offload_poll_start == NULL) {
		return -ENOSYS;
	}

	return api->offload_poll_start(dev, protos, cfg, cb, user_data);
}

/**
 * @brief Stop offloaded target discovery.
 *
 * Returns once the callback given to @ref nfc_offload_poll_start can no longer
 * run, so the caller may then release whatever it passed as user data.
 *
 * @param dev NFC device.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if offloaded polling is not supported.
 */
static inline int nfc_offload_poll_stop(const struct device *dev)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->offload_poll_stop == NULL) {
		return -ENOSYS;
	}

	return api->offload_poll_stop(dev);
}

/**
 * @brief Exchange data with a target activated by an offloading controller.
 *
 * @param dev NFC device.
 * @param target Target previously reported by @ref nfc_offload_poll_start.
 * @param tx_data Data to send.
 * @param tx_len Length of @p tx_data.
 * @param rx_data Buffer for the response.
 * @param rx_len In: size of @p rx_data. Out: number of bytes received.
 * @param timeout_ms Response timeout in milliseconds.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if offloaded exchange is not supported.
 * @retval -errno on failure.
 */
static inline int nfc_offload_exchange(const struct device *dev, const struct nfc_target *target,
				       const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
				       uint16_t *rx_len, uint32_t timeout_ms)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->offload_exchange == NULL) {
		return -ENOSYS;
	}

	return api->offload_exchange(dev, target, tx_data, tx_len, rx_data, rx_len, timeout_ms);
}

/**
 * @brief Let go of a target activated by an offloading controller.
 *
 * Ends the firmware-side session with @p target, so that discovery reaches the
 * other targets in the field. The controller decides which protocol frames that
 * takes.
 *
 * @param dev NFC device.
 * @param target Target previously reported by @ref nfc_offload_poll_start.
 *
 * @retval 0 on success.
 * @retval -ENOSYS if offloaded release is not supported.
 * @retval -errno on failure.
 */
static inline int nfc_offload_release(const struct device *dev, const struct nfc_target *target)
{
	const struct nfc_driver_api *api = DEVICE_API_GET(nfc, dev);

	if (api->offload_release == NULL) {
		return -ENOSYS;
	}

	return api->offload_release(dev, target);
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

#include <zephyr/syscalls/nfc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_NFC_H_ */
