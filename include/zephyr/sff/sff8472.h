/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SFF-8472 management interface decoding for SFP/SFP+ modules
 *
 * SFF-8472 defines the two-wire (I2C) management memory map of 1-lane
 * pluggable transceivers (SFP, SFP+, SFP28). The map is split across two
 * I2C device addresses: the serial ID page at A0h (0x50) and the digital
 * diagnostic monitoring (DDM) page at A2h (0x51). This API decodes the raw
 * page contents; reading them over the bus is left to the caller.
 */

#ifndef ZEPHYR_INCLUDE_SFF_SFF8472_H_
#define ZEPHYR_INCLUDE_SFF_SFF8472_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SFF-8472 management decoding
 * @defgroup sff8472 SFF-8472 management decoding
 * @ingroup utilities
 * @{
 */

/** Size of an SFF-8472 management page, in bytes. */
#define SFF8472_PAGE_SIZE 256U

/**
 * @name Serial ID page (A0h) byte offsets
 * @{
 */
#define SFF8472_A0_IDENTIFIER     0U   /**< Type of transceiver (SFF-8024). */
#define SFF8472_A0_EXT_IDENTIFIER 1U   /**< Extended identifier. */
#define SFF8472_A0_CONNECTOR      2U   /**< Connector type (SFF-8024). */
#define SFF8472_A0_BR_NOMINAL     12U  /**< Nominal bit rate, units of 100 MBd. */
#define SFF8472_A0_VENDOR_NAME    20U  /**< Vendor name, 16 ASCII bytes. */
#define SFF8472_A0_VENDOR_OUI     37U  /**< Vendor IEEE OUI, 3 bytes. */
#define SFF8472_A0_VENDOR_PN      40U  /**< Vendor part number, 16 ASCII bytes. */
#define SFF8472_A0_VENDOR_REV     56U  /**< Vendor revision, 4 ASCII bytes. */
#define SFF8472_A0_WAVELENGTH     60U  /**< Laser wavelength, big-endian nm. */
#define SFF8472_A0_CC_BASE        63U  /**< Check code over bytes 0..62. */
#define SFF8472_A0_VENDOR_SN      68U  /**< Vendor serial number, 16 ASCII bytes. */
#define SFF8472_A0_DATE_CODE      84U  /**< Date code, 8 ASCII bytes. */
#define SFF8472_A0_DIAG_MON_TYPE  92U  /**< Diagnostic monitoring type. */
#define SFF8472_A0_CC_EXT         95U  /**< Check code over bytes 64..94. */
/** @} */

/**
 * @name Decoded ASCII field lengths (excluding the NUL terminator)
 * @{
 */
#define SFF8472_VENDOR_NAME_LEN 16U /**< Vendor name field length. */
#define SFF8472_VENDOR_PN_LEN   16U /**< Vendor part number field length. */
#define SFF8472_VENDOR_REV_LEN  4U  /**< Vendor revision field length. */
#define SFF8472_VENDOR_SN_LEN   16U /**< Vendor serial number field length. */
#define SFF8472_DATE_CODE_LEN   8U  /**< Date code field length. */
#define SFF8472_VENDOR_OUI_LEN  3U  /**< Vendor OUI field length. */
/** @} */

/**
 * @name Diagnostic monitoring type bits (A0h byte 92)
 * @{
 */
#define SFF8472_DIAG_MON_DDM_IMPLEMENTED BIT(6) /**< DDM is implemented. */
#define SFF8472_DIAG_MON_INT_CAL         BIT(5) /**< Internally calibrated. */
#define SFF8472_DIAG_MON_EXT_CAL         BIT(4) /**< Externally calibrated. */
/** @} */

/**
 * @name Diagnostics page (A2h) byte offsets
 * @{
 */
#define SFF8472_A2_TEMP          96U  /**< Temperature, big-endian 1/256 deg C. */
#define SFF8472_A2_VCC           98U  /**< Supply voltage, big-endian 100 uV/LSB. */
#define SFF8472_A2_TX_BIAS       100U /**< TX bias current, big-endian 2 uA/LSB. */
#define SFF8472_A2_TX_POWER      102U /**< TX output power, big-endian 0.1 uW/LSB. */
#define SFF8472_A2_RX_POWER      104U /**< RX input power, big-endian 0.1 uW/LSB. */
#define SFF8472_A2_ALARM_FLAGS   112U /**< Alarm flags, 2 bytes. */
#define SFF8472_A2_WARNING_FLAGS 116U /**< Warning flags, 2 bytes. */
/** @} */

/**
 * @name Normalized alarm and warning flags
 *
 * SFF-8472 spreads the alarm (A2h bytes 112..113) and warning (A2h bytes
 * 116..117) status across two bytes each. @ref sff8472_parse_diagnostics
 * collapses each set into one bitmask using the values below.
 * @{
 */
#define SFF8472_FLAG_TEMP_HIGH     BIT(0) /**< Temperature high. */
#define SFF8472_FLAG_TEMP_LOW      BIT(1) /**< Temperature low. */
#define SFF8472_FLAG_VCC_HIGH      BIT(2) /**< Supply voltage high. */
#define SFF8472_FLAG_VCC_LOW       BIT(3) /**< Supply voltage low. */
#define SFF8472_FLAG_TX_BIAS_HIGH  BIT(4) /**< TX bias current high. */
#define SFF8472_FLAG_TX_BIAS_LOW   BIT(5) /**< TX bias current low. */
#define SFF8472_FLAG_TX_POWER_HIGH BIT(6) /**< TX output power high. */
#define SFF8472_FLAG_TX_POWER_LOW  BIT(7) /**< TX output power low. */
#define SFF8472_FLAG_RX_POWER_HIGH BIT(8) /**< RX input power high. */
#define SFF8472_FLAG_RX_POWER_LOW  BIT(9) /**< RX input power low. */
/** @} */

/**
 * @name Transceiver identifier values (SFF-8024 table 4-1, subset)
 * @{
 */
#define SFF8472_ID_UNKNOWN 0x00U /**< Unknown or unspecified. */
#define SFF8472_ID_GBIC    0x01U /**< GBIC. */
#define SFF8472_ID_SFF     0x02U /**< Module soldered to motherboard. */
#define SFF8472_ID_SFP     0x03U /**< SFP/SFP+/SFP28. */
/** @} */

/** Decoded SFF-8472 serial ID (A0h) contents. */
struct sff8472_id {
	/** Transceiver identifier, raw SFF-8024 value (@c SFF8472_ID_*). */
	uint8_t identifier;
	/** Connector type, raw SFF-8024 value. */
	uint8_t connector;
	/** Nominal signalling rate in MBd (0 if unspecified). */
	uint16_t br_nominal_mbd;
	/** Laser wavelength in nm (0 if not applicable). */
	uint16_t wavelength_nm;
	/** Vendor IEEE OUI. */
	uint8_t vendor_oui[SFF8472_VENDOR_OUI_LEN];
	/** Vendor name, trailing spaces trimmed. */
	char vendor_name[SFF8472_VENDOR_NAME_LEN + 1U];
	/** Vendor part number, trailing spaces trimmed. */
	char vendor_pn[SFF8472_VENDOR_PN_LEN + 1U];
	/** Vendor revision, trailing spaces trimmed. */
	char vendor_rev[SFF8472_VENDOR_REV_LEN + 1U];
	/** Vendor serial number, trailing spaces trimmed. */
	char vendor_sn[SFF8472_VENDOR_SN_LEN + 1U];
	/** Date code, trailing spaces trimmed. */
	char date_code[SFF8472_DATE_CODE_LEN + 1U];
	/** True if digital diagnostic monitoring is implemented. */
	bool ddm_supported;
	/** True if diagnostics use internal calibration. */
	bool ddm_internally_calibrated;
	/** True if diagnostics require external calibration constants. */
	bool ddm_externally_calibrated;
};

/** Decoded SFF-8472 real-time diagnostics (A2h) contents. */
struct sff8472_diagnostics {
	/** Module temperature in millidegrees Celsius. */
	int32_t temp_mdegc;
	/** Supply voltage in microvolts. */
	uint32_t vcc_uv;
	/** Laser bias current in microamperes. */
	uint32_t tx_bias_ua;
	/** Coupled TX output power in nanowatts. */
	uint32_t tx_power_nw;
	/** Received optical power in nanowatts. */
	uint32_t rx_power_nw;
	/** Active alarms as a bitmask of @c SFF8472_FLAG_* values. */
	uint16_t alarm_flags;
	/** Active warnings as a bitmask of @c SFF8472_FLAG_* values. */
	uint16_t warning_flags;
};

/**
 * @brief Decode the SFF-8472 serial ID page (A0h).
 *
 * @param page_a0 Buffer holding at least the first 96 bytes of the A0h page.
 * @param id Destination for the decoded identification data.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p page_a0 or @p id is NULL.
 */
int sff8472_parse_id(const uint8_t *page_a0, struct sff8472_id *id);

/**
 * @brief Decode the SFF-8472 real-time diagnostics page (A2h).
 *
 * The decoded values assume internal calibration. For a module that reports
 * @ref sff8472_id.ddm_externally_calibrated, the caller must still apply the
 * A2h calibration constants; this function does not.
 *
 * @param page_a2 Buffer holding at least the first 118 bytes of the A2h page.
 * @param diag Destination for the decoded diagnostics.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p page_a2 or @p diag is NULL.
 */
int sff8472_parse_diagnostics(const uint8_t *page_a2, struct sff8472_diagnostics *diag);

/**
 * @brief Validate the A0h base check code (CC_BASE, byte 63).
 *
 * @param page_a0 Buffer holding at least the first 64 bytes of the A0h page.
 *
 * @retval true If the low byte of the sum of bytes 0..62 matches byte 63.
 * @retval false Otherwise, or if @p page_a0 is NULL.
 */
bool sff8472_base_checksum_valid(const uint8_t *page_a0);

/**
 * @brief Validate the A0h extended check code (CC_EXT, byte 95).
 *
 * @param page_a0 Buffer holding at least the first 96 bytes of the A0h page.
 *
 * @retval true If the low byte of the sum of bytes 64..94 matches byte 95.
 * @retval false Otherwise, or if @p page_a0 is NULL.
 */
bool sff8472_ext_checksum_valid(const uint8_t *page_a0);

/**
 * @brief Return a human-readable name for a transceiver identifier.
 *
 * @param identifier Raw SFF-8024 identifier value (A0h byte 0).
 *
 * @return A static, NUL-terminated string; never NULL.
 */
const char *sff8472_identifier_name(uint8_t identifier);

/**
 * @brief Return a human-readable name for a connector type.
 *
 * @param connector Raw SFF-8024 connector value (A0h byte 2).
 *
 * @return A static, NUL-terminated string; never NULL.
 */
const char *sff8472_connector_name(uint8_t connector);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SFF_SFF8472_H_ */
