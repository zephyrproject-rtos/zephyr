/*
 * Copyright (c) 2023 Florian Grandel, Zephyr Project.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC information element (IE) related types and helpers
 *
 * This is not to be included by the application. This file contains only those
 * parts of the types required for IE support that need to be visible to IEEE
 * 802.15.4 drivers and L2 at the same time, i.e. everything related to header
 * IE representation, parsing and generation.
 *
 * All specification references in this file refer to IEEE 802.15.4-2020.
 *
 * @note All structs and attributes in this file that directly represent parts
 * of IEEE 802.15.4 frames are in LITTLE ENDIAN, see section 4, especially
 * section 4.3.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_IE_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_IE_H_

#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>

/**
 * @addtogroup ieee802154_driver
 * @{
 *
 * @name IEEE 802.15.4, section 7.4.2: MAC header information elements
 * @{
 */

/**
 * @brief Information Element Types.
 *
 * @details See sections 7.4.2.1 and 7.4.3.1.
 */
enum ieee802154_ie_type {
	IEEE802154_IE_TYPE_HEADER = 0x0, /**< Header type */
	IEEE802154_IE_TYPE_PAYLOAD,      /**< Payload type */
};

/**
 * @brief Header Information Element IDs.
 *
 * @details See section 7.4.2.1, table 7-7, partial list, only IEs actually used
 * are implemented.
 */
enum ieee802154_header_ie_element_id {
	IEEE802154_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE = 0x00,   /**< Vendor specific IE */
	IEEE802154_HEADER_IE_ELEMENT_ID_CSL_IE = 0x1a,	             /**< CSL IE */
	IEEE802154_HEADER_IE_ELEMENT_ID_RIT_IE = 0x1b,               /**< RIT IE */
	IEEE802154_HEADER_IE_ELEMENT_ID_RENDEZVOUS_TIME_IE = 0x1d,   /**< Rendezvous time IE */
	IEEE802154_HEADER_IE_ELEMENT_ID_TIME_CORRECTION_IE = 0x1e,   /**< Time correction IE */
	IEEE802154_HEADER_IE_ELEMENT_ID_HEADER_TERMINATION_1 = 0x7e, /**< Header termination 1 */
	IEEE802154_HEADER_IE_ELEMENT_ID_HEADER_TERMINATION_2 = 0x7f, /**< Header termination 2 */
	/* partial list, add additional ids as needed */
};

/** @cond INTERNAL_HIDDEN */
#define IEEE802154_VENDOR_SPECIFIC_IE_OUI_LEN 3
/** INTERNAL_HIDDEN @endcond */

/** @brief Vendor Specific Header IE, see section 7.4.2.3. */
struct ieee802154_header_ie_vendor_specific {
	/** Vendor OUI */
	uint8_t vendor_oui[IEEE802154_VENDOR_SPECIFIC_IE_OUI_LEN];
	/** Vendor specific information */
	uint8_t *vendor_specific_info;
} __packed;

/** @brief Full CSL IE, see section 7.4.2.3. */
struct ieee802154_header_ie_csl_full {
	uint16_t csl_phase;           /**< CSL phase */
	uint16_t csl_period;          /**< CSL period */
	uint16_t csl_rendezvous_time; /**< Rendezvous time */
} __packed;

/** @brief Reduced CSL IE, see section 7.4.2.3. */
struct ieee802154_header_ie_csl_reduced {
	uint16_t csl_phase;  /**< CSL phase */
	uint16_t csl_period; /**< CSL period */
} __packed;

/** @brief Generic CSL IE, see section 7.4.2.3. */
struct ieee802154_header_ie_csl {
	union {
		/** CSL full information */
		struct ieee802154_header_ie_csl_full full;
		/** CSL reduced information */
		struct ieee802154_header_ie_csl_reduced reduced;
	};
} __packed;

/** @brief RIT IE, see section 7.4.2.4. */
struct ieee802154_header_ie_rit {
	uint8_t time_to_first_listen;    /**< Time to First Listen */
	uint8_t number_of_repeat_listen; /**< Number of Repeat Listen */
	uint16_t repeat_listen_interval; /**< Repeat listen interval */
} __packed;

/**
 * @brief Full Rendezvous Time IE, see section 7.4.2.6
 * (macCslInterval is nonzero).
 */
struct ieee802154_header_ie_rendezvous_time_full {
	uint16_t rendezvous_time; /**< Rendezvous time */
	uint16_t wakeup_interval; /**< Wakeup interval */
} __packed;

/**
 * @brief Reduced Rendezvous Time IE, see section 7.4.2.6
 * (macCslInterval is zero).
 */
struct ieee802154_header_ie_rendezvous_time_reduced {
	uint16_t rendezvous_time; /**< Rendezvous time */
} __packed;

/** @brief Rendezvous Time IE, see section 7.4.2.6. */
struct ieee802154_header_ie_rendezvous_time {
	union {
		/** Rendezvous time full information */
		struct ieee802154_header_ie_rendezvous_time_full full;
		/** Rendezvous time reduced information */
		struct ieee802154_header_ie_rendezvous_time_reduced reduced;
	};
} __packed;

/** @brief Time Correction IE, see section 7.4.2.7. */
struct ieee802154_header_ie_time_correction {
	uint16_t time_sync_info;  /**< Time synchronization information */
} __packed;

/** @cond INTERNAL_HIDDEN */

/* @brief Generic Header IE, see section 7.4.2.1. */
struct ieee802154_header_ie {
#if CONFIG_LITTLE_ENDIAN
	uint16_t length : 7;
	uint16_t element_id_low : 1; /* see enum ieee802154_header_ie_element_id */
	uint16_t element_id_high : 7;
	uint16_t type : 1; /* always 0 */
#else
	uint16_t element_id_low : 1; /* see enum ieee802154_header_ie_element_id */
	uint16_t length : 7;
	uint16_t type : 1; /* always 0 */
	uint16_t element_id_high : 7;
#endif
	union {
		struct ieee802154_header_ie_vendor_specific vendor_specific;
		struct ieee802154_header_ie_csl csl;
		struct ieee802154_header_ie_rit rit;
		struct ieee802154_header_ie_rendezvous_time rendezvous_time;
		struct ieee802154_header_ie_time_correction time_correction;
		/* add additional supported header IEs here */
	} content;
} __packed;

/** INTERNAL_HIDDEN @endcond */

/** @brief The header IE's header length (2 bytes). */
#define IEEE802154_HEADER_IE_HEADER_LENGTH sizeof(uint16_t)


/** @cond INTERNAL_HIDDEN */
#define IEEE802154_DEFINE_HEADER_IE(_element_id, _length, _content, _content_type)                 \
	(struct ieee802154_header_ie) {                                                            \
		.length = (_length),                                                               \
		.element_id_high = (_element_id) >> 1U, .element_id_low = (_element_id) & 0x01,    \
		.type = IEEE802154_IE_TYPE_HEADER,                                                 \
		.content._content_type = _content,                                                 \
	}

#define IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC_CONTENT_LEN(_vendor_specific_info_len)         \
	(IEEE802154_VENDOR_SPECIFIC_IE_OUI_LEN + (_vendor_specific_info_len))

#define IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC_CONTENT(_vendor_oui, _vendor_specific_info)    \
	(struct ieee802154_header_ie_vendor_specific) {                                            \
		.vendor_oui = _vendor_oui, .vendor_specific_info = (_vendor_specific_info),        \
	}

#define IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED_CONTENT(_csl_phase, _csl_period)                   \
	(struct ieee802154_header_ie_csl_reduced) {                                                \
		.csl_phase = sys_cpu_to_le16(_csl_phase),                                          \
		.csl_period = sys_cpu_to_le16(_csl_period),                                        \
	}

#define IEEE802154_DEFINE_HEADER_IE_CSL_FULL_CONTENT(_csl_phase, _csl_period,                      \
						     _csl_rendezvous_time)                         \
	(struct ieee802154_header_ie_csl_full) {                                                   \
		.csl_phase = sys_cpu_to_le16(_csl_phase),                                          \
		.csl_period = sys_cpu_to_le16(_csl_period),                                        \
		.csl_rendezvous_time = sys_cpu_to_le16(_csl_rendezvous_time),                      \
	}

#define IEEE802154_HEADER_IE_TIME_CORRECTION_NACK          0x8000
#define IEEE802154_HEADER_IE_TIME_CORRECTION_MASK          0x0fff
#define IEEE802154_HEADER_IE_TIME_CORRECTION_SIGN_BIT_MASK 0x0800

#define IEEE802154_DEFINE_HEADER_IE_TIME_CORRECTION_CONTENT(_ack, _time_correction_us)             \
	(struct ieee802154_header_ie_time_correction) {                                            \
		.time_sync_info = sys_cpu_to_le16(                                                 \
			(!(_ack) * IEEE802154_HEADER_IE_TIME_CORRECTION_NACK) |                    \
			((_time_correction_us) & IEEE802154_HEADER_IE_TIME_CORRECTION_MASK)),      \
	}
/** INTERNAL_HIDDEN @endcond */

/**
 * @brief Define a vendor specific header IE, see section 7.4.2.3.
 *
 * @details Example usage (all parameters in little endian):
 *
 * @code{.c}
 *   uint8_t vendor_specific_info[] = {...some vendor specific IE content...};
 *   struct ieee802154_header_ie header_ie = IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC(
 *       {0x9b, 0xb8, 0xea}, vendor_specific_info, sizeof(vendor_specific_info));
 * @endcode
 *
 * @param _vendor_oui an initializer for a 3 byte vendor oui array in little
 * endian
 * @param _vendor_specific_info pointer to a variable length uint8_t array with
 * the vendor specific IE content
 * @param _vendor_specific_info_len the length of the vendor specific IE content
 * (in bytes)
 */
#define IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC(_vendor_oui, _vendor_specific_info,            \
						    _vendor_specific_info_len)                     \
	IEEE802154_DEFINE_HEADER_IE(IEEE802154_HEADER_IE_ELEMENT_ID_VENDOR_SPECIFIC_IE,            \
				    IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC_CONTENT_LEN(       \
					    _vendor_specific_info_len),                            \
				    IEEE802154_DEFINE_HEADER_IE_VENDOR_SPECIFIC_CONTENT(           \
					    _vendor_oui, _vendor_specific_info),                   \
				    vendor_specific)

/**
 * @brief Define a reduced CSL IE, see section 7.4.2.3.
 *
 * @details Example usage (all parameters in CPU byte order):
 *
 * @code{.c}
 *   uint16_t csl_phase = ...;
 *   uint16_t csl_period = ...;
 *   struct ieee802154_header_ie header_ie =
 *       IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED(csl_phase, csl_period);
 * @endcode
 *
 * @param _csl_phase CSL phase in CPU byte order
 * @param _csl_period CSL period in CPU byte order
 */
#define IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED(_csl_phase, _csl_period)                           \
	IEEE802154_DEFINE_HEADER_IE(                                                               \
		IEEE802154_HEADER_IE_ELEMENT_ID_CSL_IE,                                            \
		sizeof(struct ieee802154_header_ie_csl_reduced),                                   \
		IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED_CONTENT(_csl_phase, _csl_period),          \
		csl.reduced)

/**
 * @brief Define a full CSL IE, see section 7.4.2.3.
 *
 * @details Example usage (all parameters in CPU byte order):
 *
 * @code{.c}
 *   uint16_t csl_phase = ...;
 *   uint16_t csl_period = ...;
 *   uint16_t csl_rendezvous_time = ...;
 *   struct ieee802154_header_ie header_ie =
 *       IEEE802154_DEFINE_HEADER_IE_CSL_REDUCED(csl_phase, csl_period, csl_rendezvous_time);
 * @endcode
 *
 * @param _csl_phase CSL phase in CPU byte order
 * @param _csl_period CSL period in CPU byte order
 * @param _csl_rendezvous_time CSL rendezvous time in CPU byte order
 */
#define IEEE802154_DEFINE_HEADER_IE_CSL_FULL(_csl_phase, _csl_period, _csl_rendezvous_time)        \
	IEEE802154_DEFINE_HEADER_IE(IEEE802154_HEADER_IE_ELEMENT_ID_CSL_IE,                        \
				    sizeof(struct ieee802154_header_ie_csl_full),                  \
				    IEEE802154_DEFINE_HEADER_IE_CSL_FULL_CONTENT(                  \
					    _csl_phase, _csl_period, _csl_rendezvous_time),        \
				    csl.full)

/**
 * @brief Define a Time Correction IE, see section 7.4.2.7.
 *
 * @details Example usage (parameter in CPU byte order):
 *
 * @code{.c}
 *   uint16_t time_sync_info = ...;
 *   struct ieee802154_header_ie header_ie =
 *       IEEE802154_DEFINE_HEADER_IE_TIME_CORRECTION(true, time_sync_info);
 * @endcode
 *
 * @param _ack whether or not the enhanced ACK frame that receives this IE is an
 * ACK (true) or NACK (false)
 * @param _time_correction_us the positive or negative deviation from expected
 * RX time in microseconds
 */
#define IEEE802154_DEFINE_HEADER_IE_TIME_CORRECTION(_ack, _time_correction_us)                     \
	IEEE802154_DEFINE_HEADER_IE(                                                               \
		IEEE802154_HEADER_IE_ELEMENT_ID_TIME_CORRECTION_IE,                                \
		sizeof(struct ieee802154_header_ie_time_correction),                               \
		IEEE802154_DEFINE_HEADER_IE_TIME_CORRECTION_CONTENT(_ack, _time_correction_us),    \
		time_correction)

/**
 * @brief Retrieve the time correction value in microseconds from a Time Correction IE,
 * see section 7.4.2.7.
 *
 * @param[in] ie pointer to the Time Correction IE structure
 *
 * @return The time correction value in microseconds.
 */
static inline int16_t
ieee802154_header_ie_get_time_correction_us(struct ieee802154_header_ie_time_correction *ie)
{
	if (ie->time_sync_info & IEEE802154_HEADER_IE_TIME_CORRECTION_SIGN_BIT_MASK) {
		/* Negative integer */
		return (int16_t)ie->time_sync_info | ~IEEE802154_HEADER_IE_TIME_CORRECTION_MASK;
	}

	/* Positive integer */
	return (int16_t)ie->time_sync_info & IEEE802154_HEADER_IE_TIME_CORRECTION_MASK;
}

/**
 * @brief Set the element ID of a header IE.
 *
 * @param[in] ie pointer to a header IE
 * @param[in] element_id IE element id in CPU byte order
 */
static inline void ieee802154_header_ie_set_element_id(struct ieee802154_header_ie *ie,
						       uint8_t element_id)
{
	ie->element_id_high = element_id >> 1U;
	ie->element_id_low = element_id & 0x01;
}

/**
 * @brief Get the element ID of a header IE.
 *
 * @param[in] ie pointer to a header IE
 *
 * @return header IE element id in CPU byte order
 */
static inline uint8_t ieee802154_header_ie_get_element_id(struct ieee802154_header_ie *ie)
{
	return (ie->element_id_high << 1U) | ie->element_id_low;
}

/** @brief The length in bytes of a "Time Correction" header IE. */
#define IEEE802154_TIME_CORRECTION_HEADER_IE_LEN                                                   \
	(IEEE802154_HEADER_IE_HEADER_LENGTH + sizeof(struct ieee802154_header_ie_time_correction))

/** @brief The length in bytes of a "Header Termination 1" header IE. */
#define IEEE802154_HEADER_TERMINATION_1_HEADER_IE_LEN IEEE802154_HEADER_IE_HEADER_LENGTH

/**
 * @}
 *
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_IE_H_ */
