/** @file
 *  @brief Bluetooth Generic Access Profile defines and Assigned Numbers.
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GAP_H_

#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth Generic Access Profile defines and Assigned Numbers.
 * @defgroup bt_gap_defines Defines and Assigned Numbers
 * @ingroup bt_gap
 * @{
 */

/**
 * @name Company Identifiers (see Bluetooth Assigned Numbers)
 * @{
 */
#define BT_COMP_ID_LF           0x05f1 /**< The Linux Foundation */
/**
 * @}
 */

/**
 * @name Defined GAP timers
 * @{
 */
#define BT_GAP_SCAN_FAST_INTERVAL_MIN           0x0030  /* 30 ms    */
#define BT_GAP_SCAN_FAST_INTERVAL               0x0060  /* 60 ms    */
#define BT_GAP_SCAN_FAST_WINDOW                 0x0030  /* 30 ms    */
#define BT_GAP_SCAN_SLOW_INTERVAL_1             0x0800  /* 1.28 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_1               0x0012  /* 11.25 ms */
#define BT_GAP_SCAN_SLOW_INTERVAL_2             0x1000  /* 2.56 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_2               0x0012  /* 11.25 ms */
#define BT_GAP_ADV_FAST_INT_MIN_1               0x0030  /* 30 ms    */
#define BT_GAP_ADV_FAST_INT_MAX_1               0x0060  /* 60 ms    */
#define BT_GAP_ADV_FAST_INT_MIN_2               0x00a0  /* 100 ms   */
#define BT_GAP_ADV_FAST_INT_MAX_2               0x00f0  /* 150 ms   */
#define BT_GAP_ADV_SLOW_INT_MIN                 0x0640  /* 1 s      */
#define BT_GAP_ADV_SLOW_INT_MAX                 0x0780  /* 1.2 s    */
#define BT_GAP_PER_ADV_FAST_INT_MIN_1           0x0018  /* 30 ms    */
#define BT_GAP_PER_ADV_FAST_INT_MAX_1           0x0030  /* 60 ms    */
#define BT_GAP_PER_ADV_FAST_INT_MIN_2           0x0050  /* 100 ms   */
#define BT_GAP_PER_ADV_FAST_INT_MAX_2           0x0078  /* 150 ms   */
#define BT_GAP_PER_ADV_SLOW_INT_MIN             0x0320  /* 1 s      */
#define BT_GAP_PER_ADV_SLOW_INT_MAX             0x03C0  /* 1.2 s    */
#define BT_GAP_INIT_CONN_INT_MIN                0x0018  /* 30 ms    */
#define BT_GAP_INIT_CONN_INT_MAX                0x0028  /* 50 ms    */
/**
 * @}
 */

/** LE PHY types */
enum bt_gap_le_phy {
	/** Convenience macro for when no PHY is set. */
	BT_GAP_LE_PHY_NONE                    = 0,
	/** LE 1M PHY */
	BT_GAP_LE_PHY_1M                      = BIT(0),
	 /** LE 2M PHY */
	BT_GAP_LE_PHY_2M                      = BIT(1),
	/** LE Coded PHY, coding scheme not specified */
	BT_GAP_LE_PHY_CODED                   = BIT(2),
	/** LE Coded S=8 PHY. Only used for advertising reports
	 * when Kconfig BT_EXT_ADV_CODING_SELECTION is enabled.
	 */
	BT_GAP_LE_PHY_CODED_S8                = BIT(3),
	/** LE Coded S=2 PHY. Only used for advertising reports
	 * when Kconfig BT_EXT_ADV_CODING_SELECTION is enabled.
	 */
	BT_GAP_LE_PHY_CODED_S2                = BIT(4),
};

/** Advertising PDU types */
enum bt_gap_adv_type {
	/** Scannable and connectable advertising. */
	BT_GAP_ADV_TYPE_ADV_IND               = 0x00,
	/** Directed connectable advertising. */
	BT_GAP_ADV_TYPE_ADV_DIRECT_IND        = 0x01,
	/** Non-connectable and scannable advertising. */
	BT_GAP_ADV_TYPE_ADV_SCAN_IND          = 0x02,
	/** Non-connectable and non-scannable advertising. */
	BT_GAP_ADV_TYPE_ADV_NONCONN_IND       = 0x03,
	/** Additional advertising data requested by an active scanner. */
	BT_GAP_ADV_TYPE_SCAN_RSP              = 0x04,
	/** Extended advertising, see advertising properties. */
	BT_GAP_ADV_TYPE_EXT_ADV               = 0x05,
};

/** Advertising PDU properties */
enum bt_gap_adv_prop {
	/** Connectable advertising. */
	BT_GAP_ADV_PROP_CONNECTABLE           = BIT(0),
	/** Scannable advertising. */
	BT_GAP_ADV_PROP_SCANNABLE             = BIT(1),
	/** Directed advertising. */
	BT_GAP_ADV_PROP_DIRECTED              = BIT(2),
	/** Additional advertising data requested by an active scanner. */
	BT_GAP_ADV_PROP_SCAN_RESPONSE         = BIT(3),
	/** Extended advertising. */
	BT_GAP_ADV_PROP_EXT_ADV               = BIT(4),
};

/** Maximum advertising data length. */
#define BT_GAP_ADV_MAX_ADV_DATA_LEN             31
/** Maximum extended advertising data length.
 *
 *  @note The maximum advertising data length that can be sent by an extended
 *        advertiser is defined by the controller.
 */
#define BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN         1650

#define BT_GAP_TX_POWER_INVALID                 0x7f
#define BT_GAP_RSSI_INVALID                     0x7f
#define BT_GAP_SID_INVALID                      0xff
#define BT_GAP_NO_TIMEOUT                       0x0000

/* The maximum allowed high duty cycle directed advertising timeout, 1.28
 * seconds in 10 ms unit.
 */
#define BT_GAP_ADV_HIGH_DUTY_CYCLE_MAX_TIMEOUT  128

/** Default data length */
#define BT_GAP_DATA_LEN_DEFAULT                 0x001b /* 27 bytes */
/** Maximum data length */
#define BT_GAP_DATA_LEN_MAX                     0x00fb /* 251 bytes */

/** Default data time */
#define BT_GAP_DATA_TIME_DEFAULT                0x0148 /* 328 us */
/** Maximum data time */
#define BT_GAP_DATA_TIME_MAX                    0x4290 /* 17040 us */

/** Minimum advertising set number */
#define BT_GAP_SID_MIN                          0x00
/** Maximum advertising set number */
#define BT_GAP_SID_MAX                          0x0F
/** Maximum number of consecutive periodic advertisement events that can be
 *  skipped after a successful receive.
 */
#define BT_GAP_PER_ADV_MAX_SKIP                 0x01F3
/** Minimum Periodic Advertising Timeout (N * 10 ms) */
#define BT_GAP_PER_ADV_MIN_TIMEOUT              0x000A /* 100 ms */
/** Maximum Periodic Advertising Timeout (N * 10 ms) */
#define BT_GAP_PER_ADV_MAX_TIMEOUT              0x4000 /* 163.84 s */
/** Minimum Periodic Advertising Interval (N * 1.25 ms) */
#define BT_GAP_PER_ADV_MIN_INTERVAL             0x0006 /* 7.5 ms */
/** Maximum Periodic Advertising Interval (N * 1.25 ms) */
#define BT_GAP_PER_ADV_MAX_INTERVAL             0xFFFF /* 81.91875 s */

/**
 * @brief Convert periodic advertising interval (N * 0.625 ms) to microseconds
 *
 * Value range of @p _interval is @ref BT_LE_ADV_INTERVAL_MIN to @ref BT_LE_ADV_INTERVAL_MAX
 */
#define BT_GAP_ADV_INTERVAL_TO_US(_interval) ((uint32_t)((_interval) * 625U))

/**
 * @brief Convert periodic advertising interval (N * 0.625 ms) to milliseconds
 *
 * Value range of @p _interval is @ref BT_LE_ADV_INTERVAL_MIN to @ref BT_LE_ADV_INTERVAL_MAX
 *
 * @note When intervals cannot be represented in milliseconds, this will round down.
 * For example BT_GAP_ADV_INTERVAL_TO_MS(0x0021) will become 20 ms instead of 20.625 ms
 */
#define BT_GAP_ADV_INTERVAL_TO_MS(_interval) (BT_GAP_ADV_INTERVAL_TO_US(_interval) / USEC_PER_MSEC)

/**
 * @brief Convert isochronous interval (N * 1.25 ms) to microseconds
 *
 * Value range of @p _interval is @ref BT_HCI_ISO_INTERVAL_MIN to @ref BT_HCI_ISO_INTERVAL_MAX
 */
#define BT_GAP_ISO_INTERVAL_TO_US(_interval) ((uint32_t)((_interval) * 1250U))

/**
 * @brief Convert isochronous interval (N * 1.25 ms) to milliseconds
 *
 * Value range of @p _interval is @ref BT_HCI_ISO_INTERVAL_MIN to @ref BT_HCI_ISO_INTERVAL_MAX
 *
 * @note When intervals cannot be represented in milliseconds, this will round down.
 * For example BT_GAP_ISO_INTERVAL_TO_MS(0x0005) will become 6 ms instead of 6.25 ms
 */
#define BT_GAP_ISO_INTERVAL_TO_MS(_interval) (BT_GAP_ISO_INTERVAL_TO_US(_interval) / USEC_PER_MSEC)

/** @brief Convert periodic advertising interval (N * 1.25 ms) to microseconds *
 *
 * Value range of @p _interval is @ref BT_HCI_LE_PER_ADV_INTERVAL_MIN to @ref
 * BT_HCI_LE_PER_ADV_INTERVAL_MAX
 */
#define BT_GAP_PER_ADV_INTERVAL_TO_US(_interval) ((uint32_t)((_interval) * 1250U))

/**
 * @brief Convert periodic advertising interval (N * 1.25 ms) to milliseconds
 *
 * @note When intervals cannot be represented in milliseconds, this will round down.
 * For example BT_GAP_PER_ADV_INTERVAL_TO_MS(0x0009) will become 11 ms instead of 11.25 ms
 */
#define BT_GAP_PER_ADV_INTERVAL_TO_MS(_interval)                                                   \
	(BT_GAP_PER_ADV_INTERVAL_TO_US(_interval) / USEC_PER_MSEC)

/**
 * @brief Convert microseconds to advertising interval units (0.625 ms)
 *
 * Value range of @p _interval is 20000 to 1024000
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_ADV_INTERVAL(21000) will become 20625 microseconds
 */
#define BT_GAP_US_TO_ADV_INTERVAL(_interval) ((uint16_t)((_interval) / 625U))

/**
 * @brief Convert milliseconds to advertising interval units (0.625 ms)
 *
 * Value range of @p _interval is 20 to 1024
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_ADV_INTERVAL(21) will become 20.625 milliseconds
 */
#define BT_GAP_MS_TO_ADV_INTERVAL(_interval)                                                       \
	(BT_GAP_US_TO_ADV_INTERVAL((_interval) * USEC_PER_MSEC))

/**
 * @brief Convert microseconds to periodic advertising interval units (1.25 ms)
 *
 * Value range of @p _interval is 7500 to 81918750
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_PER_ADV_INTERVAL(11000) will become 10000 microseconds
 */
#define BT_GAP_US_TO_PER_ADV_INTERVAL(_interval) ((uint16_t)((_interval) / 1250U))

/**
 * @brief Convert milliseconds to periodic advertising interval units (1.25 ms)
 *
 * Value range of @p _interval is 7.5 to 81918.75
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_PER_ADV_INTERVAL(11) will become 10 milliseconds
 */
#define BT_GAP_MS_TO_PER_ADV_INTERVAL(_interval)                                                   \
	(BT_GAP_US_TO_PER_ADV_INTERVAL((_interval) * USEC_PER_MSEC))

/**
 * @brief Convert milliseconds to periodic advertising sync timeout units (10 ms)
 *
 * Value range of @p _timeout is 100 to 163840
 *
 * @note If @p _timeout is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT(4005) will become 4000 milliseconds
 */
#define BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT(_timeout) ((uint16_t)((_timeout) / 10U))

/**
 * @brief Convert microseconds to periodic advertising sync timeout units (10 ms)
 *
 * Value range of @p _timeout is 100000 to 163840000
 *
 * @note If @p _timeout is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT(4005000) will become 4000000 microseconds
 */
#define BT_GAP_US_TO_PER_ADV_SYNC_TIMEOUT(_timeout)                                                \
	(BT_GAP_MS_TO_PER_ADV_SYNC_TIMEOUT((_timeout) / USEC_PER_MSEC))

/**
 * @brief Convert microseconds to scan interval units (0.625 ms)
 *
 * Value range of @p _interval is 2500 to 40959375 if @kconfig{CONFIG_BT_EXT_ADV} else
 * 2500 to 10240000
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_SCAN_INTERVAL(21000) will become 20625 microseconds
 */
#define BT_GAP_US_TO_SCAN_INTERVAL(_interval) ((uint16_t)((_interval) / 625U))

/**
 * @brief Convert milliseconds to scan interval units (0.625 ms)
 *
 * Value range of @p _interval is 2.5 to 40959.375 if @kconfig{CONFIG_BT_EXT_ADV} else
 * 2500 to 10240
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_SCAN_INTERVAL(21) will become 20.625 milliseconds
 */
#define BT_GAP_MS_TO_SCAN_INTERVAL(_interval)                                                      \
	(BT_GAP_US_TO_SCAN_INTERVAL((_interval) * USEC_PER_MSEC))

/**
 * @brief Convert microseconds to scan window units (0.625 ms)
 *
 * Value range of @p _window is 2500 to 40959375 if @kconfig{CONFIG_BT_EXT_ADV} else
 * 2500 to 10240000
 *
 * @note If @p _window is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_SCAN_WINDOW(21000) will become 20625 microseconds
 */
#define BT_GAP_US_TO_SCAN_WINDOW(_window) ((uint16_t)((_window) / 625U))

/**
 * @brief Convert milliseconds to scan window units (0.625 ms)
 *
 * Value range of @p _window is 2.5 to 40959.375 if @kconfig{CONFIG_BT_EXT_ADV} else
 * 2500 to 10240
 *
 * @note If @p _window is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_SCAN_WINDOW(21) will become 20.625 milliseconds
 */
#define BT_GAP_MS_TO_SCAN_WINDOW(_window) (BT_GAP_US_TO_SCAN_WINDOW((_window) * USEC_PER_MSEC))

/**
 * @brief Convert microseconds to connection interval units (1.25 ms)
 *
 * Value range of @p _interval is 7500 to 4000000
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_CONN_INTERVAL(21000) will become 20000 microseconds
 */
#define BT_GAP_US_TO_CONN_INTERVAL(_interval) ((uint16_t)((_interval) / 1250U))

/**
 * @brief Convert milliseconds to connection interval units (1.25 ms)
 *
 * Value range of @p _interval is 7.5 to 4000
 *
 * @note If @p _interval is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_CONN_INTERVAL(21) will become 20 milliseconds
 */
#define BT_GAP_MS_TO_CONN_INTERVAL(_interval)                                                      \
	(BT_GAP_US_TO_CONN_INTERVAL((_interval) * USEC_PER_MSEC))

/**
 * @brief Convert milliseconds to connection supervision timeout units (10 ms)
 *
 * Value range of @p _timeout is 100 to 32000
 *
 * @note If @p _timeout is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_CONN_TIMEOUT(4005) will become 4000 milliseconds
 */
#define BT_GAP_MS_TO_CONN_TIMEOUT(_timeout) ((uint16_t)((_timeout) / 10U))

/**
 * @brief Convert microseconds to connection supervision timeout units (10 ms)

 * Value range of @p _timeout is 100000 to 32000000
 *
 * @note If @p _timeout is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_CONN_TIMEOUT(4005000) will become 4000000 microseconds
 */
#define BT_GAP_US_TO_CONN_TIMEOUT(_timeout) (BT_GAP_MS_TO_CONN_TIMEOUT((_timeout) / USEC_PER_MSEC))

/**
 * @brief Convert milliseconds to connection event length units (0.625)
 *
 * Value range of @p _event_len is 0 to 40959375
 *
 * @note If @p _event_len is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_US_TO_CONN_EVENT_LEN(21000) will become 20625 milliseconds
 */
#define BT_GAP_US_TO_CONN_EVENT_LEN(_event_len) ((uint16_t)((_event_len) / 625U))

/**
 * @brief Convert milliseconds to connection event length units (0.625)
 *
 * Value range of @p _event_len is 0 to 40959.375
 *
 * @note If @p _event_len is not a multiple of the unit, it will round down to nearest.
 * For example BT_GAP_MS_TO_CONN_EVENT_LEN(21) will become 20.625 milliseconds
 */
#define BT_GAP_MS_TO_CONN_EVENT_LEN(_event_len)                                                    \
	(BT_GAP_US_TO_CONN_EVENT_LEN((_event_len) * USEC_PER_MSEC))

/** Constant Tone Extension (CTE) types */
enum bt_gap_cte {
	/** Angle of Arrival */
	BT_GAP_CTE_AOA = 0x00,
	/** Angle of Departure with 1 us slots */
	BT_GAP_CTE_AOD_1US = 0x01,
	/** Angle of Departure with 2 us slots */
	BT_GAP_CTE_AOD_2US = 0x02,
	/** No extensions */
	BT_GAP_CTE_NONE = 0xFF,
};

/** @brief Peripheral sleep clock accuracy (SCA) in ppm (parts per million) */
enum bt_gap_sca {
	BT_GAP_SCA_UNKNOWN = 0,   /**< Unknown */
	BT_GAP_SCA_251_500 = 0,   /**< 251 ppm to 500 ppm */
	BT_GAP_SCA_151_250 = 1,   /**< 151 ppm to 250 ppm */
	BT_GAP_SCA_101_150 = 2,   /**< 101 ppm to 150 ppm */
	BT_GAP_SCA_76_100 = 3,    /**< 76 ppm to 100 ppm */
	BT_GAP_SCA_51_75 = 4,     /**< 51 ppm to 75 ppm */
	BT_GAP_SCA_31_50 = 5,     /**< 31 ppm to 50 ppm */
	BT_GAP_SCA_21_30 = 6,     /**< 21 ppm to 30 ppm */
	BT_GAP_SCA_0_20 = 7,      /**< 0 ppm to 20 ppm */
};

/**
 * @brief Encode 40 least significant bits of 64-bit LE Supported Features into array values
 *        in little-endian format.
 *
 * Helper macro to encode 40 least significant bits of 64-bit LE Supported Features value into
 * advertising data. The number of bits that are encoded is a number of LE Supported Features
 * defined by BT 5.3 Core specification.
 *
 * Example of how to encode the `0x000000DFF00DF00D` into advertising data.
 *
 * @code
 * BT_DATA_BYTES(BT_DATA_LE_SUPPORTED_FEATURES, BT_LE_SUPP_FEAT_40_ENCODE(0x000000DFF00DF00D))
 * @endcode
 *
 * @param w64 LE Supported Features value (64-bits)
 *
 * @return The comma separated values for LE Supported Features value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_LE_SUPP_FEAT_40_ENCODE(w64) BT_BYTES_LIST_LE40(w64)

/** @brief Encode 4 least significant bytes of 64-bit LE Supported Features into
 *         4 bytes long array of values in little-endian format.
 *
 *  Helper macro to encode 64-bit LE Supported Features value into advertising
 *  data. The macro encodes 4 least significant bytes into advertising data.
 *  Other 4 bytes are not encoded.
 *
 *  Example of how to encode the `0x000000DFF00DF00D` into advertising data.
 *
 *  @code
 *  BT_DATA_BYTES(BT_DATA_LE_SUPPORTED_FEATURES, BT_LE_SUPP_FEAT_32_ENCODE(0x000000DFF00DF00D))
 *  @endcode
 *
 * @param w64 LE Supported Features value (64-bits)
 *
 * @return The comma separated values for LE Supported Features value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_LE_SUPP_FEAT_32_ENCODE(w64) BT_BYTES_LIST_LE32(w64)

/**
 * @brief Encode 3 least significant bytes of 64-bit LE Supported Features into
 *        3 bytes long array of values in little-endian format.
 *
 * Helper macro to encode 64-bit LE Supported Features value into advertising
 * data. The macro encodes 3 least significant bytes into advertising data.
 * Other 5 bytes are not encoded.
 *
 * Example of how to encode the `0x000000DFF00DF00D` into advertising data.
 *
 * @code
 * BT_DATA_BYTES(BT_DATA_LE_SUPPORTED_FEATURES, BT_LE_SUPP_FEAT_24_ENCODE(0x000000DFF00DF00D))
 * @endcode
 *
 * @param w64 LE Supported Features value (64-bits)
 *
 * @return The comma separated values for LE Supported Features value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_LE_SUPP_FEAT_24_ENCODE(w64) BT_BYTES_LIST_LE24(w64)

/**
 * @brief Encode 2 least significant bytes of 64-bit LE Supported Features into
 *        2 bytes long array of values in little-endian format.
 *
 * Helper macro to encode 64-bit LE Supported Features value into advertising
 * data. The macro encodes 3 least significant bytes into advertising data.
 * Other 6 bytes are not encoded.
 *
 * Example of how to encode the `0x000000DFF00DF00D` into advertising data.
 *
 * @code
 * BT_DATA_BYTES(BT_DATA_LE_SUPPORTED_FEATURES, BT_LE_SUPP_FEAT_16_ENCODE(0x000000DFF00DF00D))
 * @endcode
 *
 * @param w64 LE Supported Features value (64-bits)
 *
 * @return The comma separated values for LE Supported Features value that
 *         may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_LE_SUPP_FEAT_16_ENCODE(w64) BT_BYTES_LIST_LE16(w64)

/**
 * @brief Encode the least significant byte of 64-bit LE Supported Features into
 *        single byte long array.
 *
 * Helper macro to encode 64-bit LE Supported Features value into advertising
 * data. The macro encodes the least significant byte into advertising data.
 * Other 7 bytes are not encoded.
 *
 * Example of how to encode the `0x000000DFF00DF00D` into advertising data.
 *
 * @code
 * BT_DATA_BYTES(BT_DATA_LE_SUPPORTED_FEATURES, BT_LE_SUPP_FEAT_8_ENCODE(0x000000DFF00DF00D))
 * @endcode
 *
 * @param w64 LE Supported Features value (64-bits)
 *
 * @return The value of least significant byte of LE Supported Features value
 *         that may be used directly as an argument for @ref BT_DATA_BYTES.
 */
#define BT_LE_SUPP_FEAT_8_ENCODE(w64) \
	(((w64) >> 0) & 0xFF)

/**
 * @brief Validate whether LE Supported Features value does not use bits that are reserved for
 *        future use.
 *
 * Helper macro to check if @p w64 has zeros as bits 40-63. The macro is compliant with BT 5.3
 * Core Specification where bits 0-40 has assigned values. In case of invalid value, build time
 * error is reported.
 */
#define BT_LE_SUPP_FEAT_VALIDATE(w64) \
	BUILD_ASSERT(!((w64) & (~BIT64_MASK(40))), \
		     "RFU bit in LE Supported Features are not zeros.")

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GAP_H_ */
