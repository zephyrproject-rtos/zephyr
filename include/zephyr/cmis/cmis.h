/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMIS management interface decoding for pluggable optical modules
 *
 * The Common Management Interface Specification (CMIS) defines the management
 * memory map of high-speed pluggable transceivers such as QSFP-DD, OSFP,
 * QSFP112, SFP-DD and DSFP. Unlike SFF-8472, CMIS uses a single I2C address
 * (0x50) and a paged memory model: a 128-byte Lower Memory that is always
 * accessible, and 128-byte upper pages selected through a page-select
 * register. This API decodes Page 00h, which holds the module-level monitors
 * (Lower Memory) and the static administrative information (Upper Page 00h);
 * reading the pages over the bus is left to the caller.
 */

#ifndef ZEPHYR_INCLUDE_CMIS_CMIS_H_
#define ZEPHYR_INCLUDE_CMIS_CMIS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CMIS management decoding
 * @defgroup cmis CMIS management decoding
 * @ingroup utilities
 * @{
 */

/** Size of a CMIS management page, in bytes. */
#define CMIS_PAGE_SIZE 128U

/** Size of the flat Page 00h image: Lower Memory plus Upper Page 00h. */
#define CMIS_PAGE_00H_SIZE 256U

/**
 * @name Lower Memory byte offsets (always accessible)
 * @{
 */
#define CMIS_LOWER_IDENTIFIER   0U   /**< Module type (SFF-8024). */
#define CMIS_LOWER_REVISION     1U   /**< CMIS revision, packed major.minor nibbles. */
#define CMIS_LOWER_MEMORY_MODEL 2U   /**< Management characteristics, incl. flat_mem. */
#define CMIS_LOWER_MODULE_STATE 3U   /**< Module global status, state in bits 3..1. */
#define CMIS_LOWER_MODULE_FLAGS 9U   /**< Temperature and supply-voltage monitor flags. */
#define CMIS_LOWER_TEMPERATURE  14U  /**< Module temperature, big-endian 1/256 deg C. */
#define CMIS_LOWER_VCC          16U  /**< Module supply voltage, big-endian 100 uV/LSB. */
#define CMIS_LOWER_MEDIA_TYPE   85U  /**< Module media type encoding. */
/** @} */

/**
 * @name Upper Page 00h byte offsets (administrative information)
 * @{
 */
#define CMIS_PAGE00_VENDOR_NAME 129U /**< Vendor name, 16 ASCII bytes. */
#define CMIS_PAGE00_VENDOR_OUI  145U /**< Vendor IEEE OUI, 3 bytes. */
#define CMIS_PAGE00_VENDOR_PN   148U /**< Vendor part number, 16 ASCII bytes. */
#define CMIS_PAGE00_VENDOR_REV  164U /**< Vendor revision, 2 ASCII bytes. */
#define CMIS_PAGE00_VENDOR_SN   166U /**< Vendor serial number, 16 ASCII bytes. */
#define CMIS_PAGE00_DATE_CODE   182U /**< Date code, 8 ASCII bytes. */
#define CMIS_PAGE00_POWER_CLASS 200U /**< Module power class in bits 7..5. */
#define CMIS_PAGE00_MAX_POWER   201U /**< Maximum power consumption, 0.25 W/LSB. */
/** @} */

/**
 * @name Decoded ASCII field lengths (excluding the NUL terminator)
 * @{
 */
#define CMIS_VENDOR_NAME_LEN 16U /**< Vendor name field length. */
#define CMIS_VENDOR_PN_LEN   16U /**< Vendor part number field length. */
#define CMIS_VENDOR_REV_LEN  2U  /**< Vendor revision field length. */
#define CMIS_VENDOR_SN_LEN   16U /**< Vendor serial number field length. */
#define CMIS_DATE_CODE_LEN   8U  /**< Date code field length. */
#define CMIS_VENDOR_OUI_LEN  3U  /**< Vendor OUI field length. */
/** @} */

/** Management-characteristics bit: only Page 00h is present (Lower Memory byte 2). */
#define CMIS_MEMORY_MODEL_FLAT_MEM BIT(7)

/**
 * @name Normalized temperature and supply-voltage monitor flags
 *
 * CMIS packs the module temperature and supply-voltage alarm and warning
 * status into Lower Memory byte 9. @ref cmis_parse_monitors collapses that
 * byte into one bitmask using the values below.
 * @{
 */
#define CMIS_FLAG_TEMP_HIGH_ALARM   BIT(0) /**< Temperature high alarm. */
#define CMIS_FLAG_TEMP_LOW_ALARM    BIT(1) /**< Temperature low alarm. */
#define CMIS_FLAG_TEMP_HIGH_WARNING BIT(2) /**< Temperature high warning. */
#define CMIS_FLAG_TEMP_LOW_WARNING  BIT(3) /**< Temperature low warning. */
#define CMIS_FLAG_VCC_HIGH_ALARM    BIT(4) /**< Supply voltage high alarm. */
#define CMIS_FLAG_VCC_LOW_ALARM     BIT(5) /**< Supply voltage low alarm. */
#define CMIS_FLAG_VCC_HIGH_WARNING  BIT(6) /**< Supply voltage high warning. */
#define CMIS_FLAG_VCC_LOW_WARNING   BIT(7) /**< Supply voltage low warning. */
/** @} */

/**
 * @name Module identifier values (SFF-8024 table 4-1, CMIS-capable subset)
 * @{
 */
#define CMIS_ID_QSFP_DD 0x18U /**< QSFP-DD double density 8X. */
#define CMIS_ID_OSFP    0x19U /**< OSFP 8X. */
#define CMIS_ID_SFP_DD  0x1AU /**< SFP-DD double density 2X. */
#define CMIS_ID_DSFP    0x1BU /**< DSFP dual small form factor. */
#define CMIS_ID_QSFP    0x1EU /**< QSFP+ or later with CMIS. */
/** @} */

/**
 * @name Module state values (Lower Memory byte 3, bits 3..1)
 * @{
 */
#define CMIS_MODULE_STATE_LOW_PWR    0x01U /**< ModuleLowPwr. */
#define CMIS_MODULE_STATE_PWR_UP     0x02U /**< ModulePwrUp. */
#define CMIS_MODULE_STATE_READY      0x03U /**< ModuleReady. */
#define CMIS_MODULE_STATE_PWR_DOWN   0x04U /**< ModulePwrDn. */
#define CMIS_MODULE_STATE_FAULT      0x05U /**< ModuleFault. */
/** @} */

/**
 * @name Module media type values (Lower Memory byte 85)
 * @{
 */
#define CMIS_MEDIA_UNDEFINED      0x00U /**< Undefined. */
#define CMIS_MEDIA_OPTICAL_MMF    0x01U /**< Multimode fiber. */
#define CMIS_MEDIA_OPTICAL_SMF    0x02U /**< Single-mode fiber. */
#define CMIS_MEDIA_PASSIVE_COPPER 0x03U /**< Passive copper cable. */
#define CMIS_MEDIA_ACTIVE_CABLE   0x04U /**< Active cable assembly. */
#define CMIS_MEDIA_BASE_T         0x05U /**< BASE-T. */
/** @} */

/** Decoded CMIS Page 00h identification contents. */
struct cmis_id {
	/** Module identifier, raw SFF-8024 value (@c CMIS_ID_*). */
	uint8_t identifier;
	/** CMIS revision, whole part (e.g. 5 for CMIS 5.0). */
	uint8_t cmis_major;
	/** CMIS revision, fractional part (e.g. 0 for CMIS 5.0). */
	uint8_t cmis_minor;
	/** True if the module implements only Page 00h (flat memory). */
	bool flat_memory;
	/** Module state, raw value (@c CMIS_MODULE_STATE_*). */
	uint8_t module_state;
	/** Module media type, raw value (@c CMIS_MEDIA_*). */
	uint8_t media_type;
	/** Module power class, 0 through 7. */
	uint8_t power_class;
	/** Maximum module power consumption in milliwatts. */
	uint16_t max_power_mw;
	/** Vendor IEEE OUI. */
	uint8_t vendor_oui[CMIS_VENDOR_OUI_LEN];
	/** Vendor name, trailing spaces trimmed. */
	char vendor_name[CMIS_VENDOR_NAME_LEN + 1U];
	/** Vendor part number, trailing spaces trimmed. */
	char vendor_pn[CMIS_VENDOR_PN_LEN + 1U];
	/** Vendor revision, trailing spaces trimmed. */
	char vendor_rev[CMIS_VENDOR_REV_LEN + 1U];
	/** Vendor serial number, trailing spaces trimmed. */
	char vendor_sn[CMIS_VENDOR_SN_LEN + 1U];
	/** Date code, trailing spaces trimmed. */
	char date_code[CMIS_DATE_CODE_LEN + 1U];
};

/** Decoded CMIS module-level monitors (Lower Memory). */
struct cmis_monitors {
	/** Module temperature in millidegrees Celsius. */
	int32_t temp_mdegc;
	/** Supply voltage in microvolts. */
	uint32_t vcc_uv;
	/** Active monitor flags as a bitmask of @c CMIS_FLAG_* values. */
	uint8_t flags;
};

/**
 * @brief Decode the CMIS Page 00h identification contents.
 *
 * @param page00 Buffer holding the flat Page 00h image: the 128-byte Lower
 *               Memory followed by the 128-byte Upper Page 00h, at least
 *               @ref CMIS_PAGE_00H_SIZE bytes.
 * @param id Destination for the decoded identification data.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p page00 or @p id is NULL.
 */
int cmis_parse_id(const uint8_t *page00, struct cmis_id *id);

/**
 * @brief Decode the CMIS module-level monitors (Lower Memory).
 *
 * @param lower Buffer holding at least the first 18 bytes of the Lower Memory,
 *              which is the start of the Page 00h image.
 * @param mon Destination for the decoded monitors.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p lower or @p mon is NULL.
 */
int cmis_parse_monitors(const uint8_t *lower, struct cmis_monitors *mon);

/**
 * @brief Return a human-readable name for a module identifier.
 *
 * @param identifier Raw SFF-8024 identifier value (Lower Memory byte 0).
 *
 * @return A static, NUL-terminated string; never NULL.
 */
const char *cmis_identifier_name(uint8_t identifier);

/**
 * @brief Return a human-readable name for a module state.
 *
 * @param state Raw module state value (@c CMIS_MODULE_STATE_*).
 *
 * @return A static, NUL-terminated string; never NULL.
 */
const char *cmis_module_state_name(uint8_t state);

/**
 * @brief Return a human-readable name for a module media type.
 *
 * @param media_type Raw media type value (@c CMIS_MEDIA_*).
 *
 * @return A static, NUL-terminated string; never NULL.
 */
const char *cmis_media_type_name(uint8_t media_type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CMIS_CMIS_H_ */
