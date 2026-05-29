/*
 * Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_ETS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_ETS_H_

/**
 * @file
 * @brief Elapsed Time Service (ETS)
 * @defgroup bt_ets Elapsed Time Service (ETS)
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Elapsed Time data structure as defined in GATT Specification Supplement Section 3.82.
 */
struct bt_ets_elapsed_time {
	/** Flags field. @see @ref BT_ETS_FLAGS. */
	uint8_t flags;
	/** The actual time value in the resolution as defined by the flags.
	 * The Time Value field contains a counter of the number of time units as determined by the
	 * time resolution of the clock. The starting point of the timeline is 2000-01-01 00:00:00
	 * when reporting a time of day or is implementation-dependent for a tick counter.
	 */
	uint8_t time_value[6];
	/** Time synchronization source type. @see @ref BT_ETS_TIME_SOURCE */
	uint8_t time_sync_src;
	/** Combined Time Zone Daylight Saving Time offset from UTC in 15-minute units (signed). */
	int8_t tz_dst_offset;
} __packed;

/**
 * @brief Elapsed Time Service flags field bits (GATT Specification Supplement Section 3.82)
 * @name Elapsed Time Service Flags
 * @anchor BT_ETS_FLAGS
 *
 * @{
 */

/** Time Value reports a counter (tick counter). If 0, Time Value reports a time of day. */
#define BT_ETS_FLAG_TICK_COUNTER BIT(0)

/** Time is UTC. If 0, time is local time. Has no meaning for tick counter. */
#define BT_ETS_FLAG_UTC BIT(1)

/** Time resolution mask (bits 2-3). */
#define BT_ETS_FLAG_RESOLUTION_MASK (BIT(2) | BIT(3))

/** TZ/DST offset is used. If 0, offset not used. Has no meaning for tick counter. */
#define BT_ETS_FLAG_TZ_DST_USED BIT(4)

/** Time stamp is from current timeline. */
#define BT_ETS_FLAG_CURRENT_TIMELINE BIT(5)

/** Reserved bits (6-7). */
#define BT_ETS_FLAG_RESERVED_MASK (BIT(6) | BIT(7))

/** @} */

/**
 * @brief Time resolution values for bits 2-3 of flags field.
 */
enum bt_ets_time_resolution {
	/** 1 second resolution. */
	BT_ETS_RESOLUTION_1_SEC = 0,
	/** 100 milliseconds resolution. */
	BT_ETS_RESOLUTION_100_MS = 1,
	/** 1 millisecond resolution. */
	BT_ETS_RESOLUTION_1_MS = 2,
	/** 100 microseconds resolution. */
	BT_ETS_RESOLUTION_100_US = 3,
};

/**
 * @brief Time synchronization source values (GATT Specification Supplement Section 3.243)
 * @name Time Source Values
 * @anchor BT_ETS_TIME_SOURCE
 *
 * @{
 */

/** Unknown time source. */
#define BT_ETS_TIME_SOURCE_UNKNOWN          0
/** Network Time Protocol. */
#define BT_ETS_TIME_SOURCE_NTP              1
/** GPS/GNSS. */
#define BT_ETS_TIME_SOURCE_GPS              2
/** Radio Time Signal. */
#define BT_ETS_TIME_SOURCE_RADIO            3
/** Manually set. */
#define BT_ETS_TIME_SOURCE_MANUAL           4
/** Atomic Clock. */
#define BT_ETS_TIME_SOURCE_ATOMIC           5
/** Cellular Network. */
#define BT_ETS_TIME_SOURCE_CELLULAR         6
/** Not synchronized. */
#define BT_ETS_TIME_SOURCE_NOT_SYNCHRONIZED 7

/** @} */

/**
 * @brief Write result codes for write_elapsed_time callback.
 *
 * These codes indicate the result of a time write operation and map to
 * specific ATT error responses as defined in the ETS specification.
 */
enum bt_ets_write_result {
	/** Write accepted successfully. */
	BT_ETS_WRITE_SUCCESS = 0,
	/** Time source quality is too low. Rejects write with ATT error 0x80. */
	BT_ETS_WRITE_TIME_SOURCE_TOO_LOW = 1,
	/** Time value is out of acceptable range. Rejects write with ATT error 0xFF. */
	BT_ETS_WRITE_OUT_OF_RANGE = 2,
	/** Incorrect time format (flags mismatch). Rejects write with ATT error 0x81. */
	BT_ETS_WRITE_INCORRECT_FORMAT = 3,
};

/** Combined supported flags mask based on configuration. */
#define BT_ETS_SUPPORTED_FLAGS_MASK                                                                \
	((IS_ENABLED(CONFIG_BT_ETS_SUPPORT_TICK_COUNTER) ? BT_ETS_FLAG_TICK_COUNTER : 0) |         \
	 (IS_ENABLED(CONFIG_BT_ETS_SUPPORT_UTC) ? BT_ETS_FLAG_UTC : 0) |                           \
	 (IS_ENABLED(CONFIG_BT_ETS_SUPPORT_TZ_DST) ? BT_ETS_FLAG_TZ_DST_USED : 0) |                \
	 (IS_ENABLED(CONFIG_BT_ETS_RESOLUTION_1_SEC)    ? 0                                        \
	  : IS_ENABLED(CONFIG_BT_ETS_RESOLUTION_100_MS) ? BIT(2)                                   \
	  : IS_ENABLED(CONFIG_BT_ETS_RESOLUTION_1_MS)   ? BIT(3)                                   \
	  : IS_ENABLED(CONFIG_BT_ETS_RESOLUTION_100_US) ? (BIT(2) | BIT(3))                        \
							: 0) |                                     \
	 BT_ETS_FLAG_CURRENT_TIMELINE)

/**
 * @brief Clock Status flags (Elapsed Time Service - Section 3.1.1.2)
 * @name Clock Status Flags
 * @anchor BT_ETS_CLOCK_STATUS
 *
 * @{
 */

/** Clock needs to be set. If 0, clock is set. */
#define BT_ETS_CLOCK_STATUS_NEEDS_SET BIT(0)
/** Clock Status RFU mask (bits 1-7). */
#define BT_ETS_CLOCK_STATUS_RFU_MASK  0xFE

/** @} */

/**
 * @name Clock Capabilities flags (Elapsed Time Service - Section 3.1.1.3)
 * @anchor BT_ETS_CLOCK_CAPABILITIES
 *
 * @{
 */

/** Clock autonomously applies DST rules. */
#define BT_ETS_CLOCK_CAP_DST_AUTO BIT(0)
/** Clock autonomously manages TZ changes. */
#define BT_ETS_CLOCK_CAP_TZ_AUTO  BIT(1)
/** Clock Capabilities RFU mask (bits 2-7). */
#define BT_ETS_CLOCK_CAP_RFU_MASK 0xFC

/** @} */

/**
 * @brief Clock capabilities (Elapsed Time Service - Section 3.1.1.3)
 */
#define BT_ETS_CLOCK_CAPABILITIES_VALUE                                                            \
	((IS_ENABLED(CONFIG_BT_ETS_CLOCK_CAP_DST_AUTO) ? BT_ETS_CLOCK_CAP_DST_AUTO : 0) |          \
	 (IS_ENABLED(CONFIG_BT_ETS_CLOCK_CAP_TZ_AUTO) ? BT_ETS_CLOCK_CAP_TZ_AUTO : 0))

/**
 * @brief Elapsed Time Service callback structure.
 */
struct bt_ets_cb {
	/**
	 * @brief Read elapsed time callback (MANDATORY).
	 *
	 * Called when a BLE client reads the Current Elapsed Time characteristic.
	 * Application must fill the elapsed time structure with current time information.
	 *
	 * @param time Elapsed time structure to fill.
	 *
	 * @retval 0 On success.
	 * @retval -errno Negative errno on failure.
	 */
	int (*read_elapsed_time)(struct bt_ets_elapsed_time *time);

	/**
	 * @brief Read clock status (@ref BT_ETS_CLOCK_STATUS) callback (MANDATORY).
	 *
	 * Called when a BLE client reads the Current Elapsed Time characteristic.
	 * Application must return the current clock status flags.
	 *
	 * @param status Pointer to store clock status byte.
	 *
	 * @retval 0 On success.
	 * @retval -errno Negative errno on failure.
	 */
	int (*read_clock_status)(uint8_t *status);

	/**
	 * @brief Write elapsed time callback (OPTIONAL).
	 *
	 * Called when a BLE client writes to the Current Elapsed Time characteristic.
	 * Only applicable if @kconfig_dep{CONFIG_BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE} is enabled.
	 * Application should validate time source quality, range, and other constraints.
	 *
	 * @note Clock Status and Clock Capabilities are excluded from writes per spec.
	 *
	 * @param time Elapsed time structure written by client.
	 *
	 * @return Result code indicating acceptance or specific rejection reason.
	 */
	enum bt_ets_write_result (*write_elapsed_time)(const struct bt_ets_elapsed_time *time);

	/**
	 * @brief Indication subscription changed callback (OPTIONAL).
	 *
	 * Called when a client enables or disables indications for the
	 * Current Elapsed Time characteristic.
	 *
	 * @param enabled True if indications enabled, false if disabled.
	 */
	void (*indication_changed)(bool enabled);
};

/**
 * @brief Initialize Elapsed Time Service.
 *
 * Must be called during initialization. This can be called before or after bt_enable, but before
 * advertising.
 *
 * @param cb Callback structure with mandatory callbacks set.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p cb is NULL or mandatory callbacks are NULL.
 */
int bt_ets_init(const struct bt_ets_cb *cb);

/**
 * @brief Send indication to subscribed clients.
 *
 * Send indication to all subscribed clients with updated elapsed time.
 * This should be called when the time changes other than by natural progression
 * (e.g., manual adjustment, external time sync, DST change, timezone change).
 *
 * @param elapsed_time Elapsed time to indicate.
 * @param clock_status Current clock status flags (@ref BT_ETS_CLOCK_STATUS).
 *
 * @retval 0 On success.
 * @retval -ENOTCONN If no clients are connected or subscribed.
 * @retval -errno Other negative errno on failure.
 */
int bt_ets_indicate(const struct bt_ets_elapsed_time *elapsed_time, uint8_t clock_status);

/**
 * @brief Decode ETS formatted time into milliseconds since Unix epoch.
 *
 * Converts ETS time format to milliseconds with Unix epoch (1970-01-01 00:00:00 UTC).
 * Handles epoch conversion (ETS uses 2000-01-01 vs Unix 1970-01-01), resolution scaling,
 * and timezone conversion based on the flags field in @p et_time.
 *
 * The output @p unix_ms is always in UTC. Timezone handling based on flags:
 * - @kconfig_dep{CONFIG_BT_ETS_SUPPORT_UTC}: Time is already UTC, no conversion needed.
 *   If @ref BT_ETS_FLAG_TZ_DST_USED is set, the offset is informational only.
 * - @kconfig_dep{CONFIG_BT_ETS_SUPPORT_LOCAL_TIME} with @kconfig_dep{CONFIG_BT_ETS_SUPPORT_TZ_DST}:
 *   If @ref BT_ETS_FLAG_TZ_DST_USED is set in @p et_time, converts local time to UTC
 *   by subtracting the offset: UTC = Local time - TZ/DST Offset × 15 minutes
 *
 * @note This function is only available when @kconfig_dep{CONFIG_BT_ETS_HELPER_API} is enabled.
 *       This requires either @kconfig_dep{CONFIG_BT_ETS_SUPPORT_UTC} or
 *       @kconfig_dep{CONFIG_BT_ETS_SUPPORT_TZ_DST} to be enabled (proper UTC conversion needs
 *       either UTC mode or timezone offset information).
 *
 * @param et_time ETS time formatted structure to decode.
 * @param unix_ms Pointer to store milliseconds since Unix epoch (1970-01-01 00:00:00 UTC).
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p et_time or @p unix_ms is NULL.
 * @retval -EOVERFLOW If the converted time value overflows during conversion.
 */
int bt_ets_time_to_unix_ms(const struct bt_ets_elapsed_time *et_time, int64_t *unix_ms);

/**
 * @brief Encode Unix milliseconds into ETS formatted time.
 *
 * Converts a Unix timestamp (1970-01-01 00:00:00 UTC) to ETS time format.
 * Handles epoch conversion (Unix 1970-01-01 to ETS 2000-01-01), resolution scaling,
 * and timezone conversion based on compile-time configuration.
 *
 * The @p unix_ms input is always in UTC. The @p tz_dst_offset parameter behavior:
 * - @kconfig_dep{CONFIG_BT_ETS_SUPPORT_UTC}: Time stays as UTC, no conversion.
 *   If @kconfig_dep{CONFIG_BT_ETS_SUPPORT_TZ_DST} enabled, offset is stored for client use.
 * - @kconfig_dep{CONFIG_BT_ETS_SUPPORT_LOCAL_TIME} with @kconfig_dep{CONFIG_BT_ETS_SUPPORT_TZ_DST}:
 *   Offset is applied to convert UTC to local time and stored in output.
 *   Formula: Local time = UTC time + TZ/DST Offset × 15 minutes
 *   @ref BT_ETS_FLAG_TZ_DST_USED flag is set in output.
 *
 * The output @p et_time flags are set based on compile-time configuration:
 * - Time mode: @kconfig_dep{CONFIG_BT_ETS_SUPPORT_UTC} or
 * @kconfig_dep{CONFIG_BT_ETS_SUPPORT_LOCAL_TIME}
 * - Resolution: CONFIG_BT_ETS_RESOLUTION_* (1_SEC, 100_MS, 1_MS, or 100_US)
 * - @ref BT_ETS_FLAG_CURRENT_TIMELINE is always set
 *
 * @note This function is only available when @kconfig_dep{CONFIG_BT_ETS_HELPER_API} is enabled.
 *       This requires either @kconfig_dep{CONFIG_BT_ETS_SUPPORT_UTC} or
 *       @kconfig_dep{CONFIG_BT_ETS_SUPPORT_TZ_DST} to be enabled.
 *
 * @param et_time Pointer to ETS time structure to fill.
 * @param unix_ms Milliseconds since Unix epoch (1970-01-01 00:00:00 UTC) - always UTC.
 * @param time_src Time synchronization source type. (see @ref BT_ETS_TIME_SOURCE)
 * @param tz_dst_offset TZ/DST offset from UTC in 15-minute units (signed).
 *                      Used for UTC-to-local conversion if in LOCAL_TIME mode.
 *
 * @retval 0 On success.
 * @retval -EINVAL If @p et_time is NULL or time is before ETS epoch (2000-01-01) after conversion.
 * @retval -EOVERFLOW If the time value exceeds 48-bit range after conversion.
 */
int bt_ets_time_from_unix_ms(struct bt_ets_elapsed_time *et_time, int64_t unix_ms, uint8_t time_src,
			     int8_t tz_dst_offset);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_ETS_H_ */
