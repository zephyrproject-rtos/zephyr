/*
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_CTS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_CTS_H_

/**
 * @brief Current Time Service (CTS)
 * @defgroup bt_cts Current Time Service (CTS)
 * @ingroup bluetooth
 * @{
 *
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ATT error code for ignored values during CTS write operations
 */
#define BT_CTS_ATT_ERR_VALUES_IGNORED 0x80U

/**
 * @brief Maximum value for the 1/256-second fractions field in CTS time format
 */
#define BT_CTS_FRACTION_256_MAX_VALUE 255U

/**
 * @brief Default timezone offset value indicating the timezone is unknown (15-minute increments)
 */
#define BT_CTS_TIMEZONE_DEFAULT_VALUE -128

/**
 * @brief Timezone offset increment in minutes
 */
#define BT_CTS_TIMEZONE_INCREMENT_MIN 15U

/**
 * @brief DST offset increment in minutes
 */
#define BT_CTS_DST_INCREMENT_MIN 15U

/**
 * @brief Minimum valid timezone offset in 15-minute increments (-48 means UTC-12:00)
 */
#define BT_CTS_TIMEZONE_MIN -48

/**
 * @brief Maximum valid timezone offset in 15-minute increments (56 means UTC+14:00)
 */
#define BT_CTS_TIMEZONE_MAX 56

/**
 * @brief Daylight Saving Time (DST) offsets as defined in the specification
 */
enum bt_cts_dst_offset {
	/** Standard Time */
	BT_CTS_DST_OFFSET_STANDARD_TIME = 0,
	/** Half an hour daylight saving time */
	BT_CTS_DST_OFFSET_HALF_HOUR_DAYLIGHT_TIME = 2,
	/** One hour daylight saving time */
	BT_CTS_DST_OFFSET_DAYLIGHT_TIME = 4,
	/** Two hour daylight saving time */
	BT_CTS_DST_OFFSET_DOUBLE_DAYLIGHT_TIME = 8,
	/** Offset unknown */
	BT_CTS_DST_OFFSET_UNKNOWN = 255,
};

/**
 * @brief CTS time update reason bits as defined in the specification
 */
enum bt_cts_update_reason {
	/* Unknown reason of update no bit is set */
	BT_CTS_UPDATE_REASON_UNKNOWN = 0,
	/* When time is changed manually e.g. through UI */
	BT_CTS_UPDATE_REASON_MANUAL = BIT(0),
	/* If time is changed through external reference */
	BT_CTS_UPDATE_REASON_EXTERNAL_REF = BIT(1),
	/* time changed due to timezone adjust */
	BT_CTS_UPDATE_REASON_TIME_ZONE_CHANGE = BIT(2),
	/* time changed due to dst offset change */
	BT_CTS_UPDATE_REASON_DAYLIGHT_SAVING = BIT(3),
};

/**
 * @brief Current Time service data format, Please refer to
 * specifications for more details
 */
struct bt_cts_time_format {
	uint16_t year;
	uint8_t mon;
	uint8_t mday;
	uint8_t hours;
	uint8_t min;
	uint8_t sec;
	uint8_t wday;
	uint8_t fractions256;
	uint8_t reason;
} __packed;

/**
 * @brief CTS Local Time Information
 */
struct bt_cts_local_time {
	/**
	 * @brief Represents the offset from UTC in number in 15-minute increments.
	 *
	 * Valid range from -48 (@ref BT_CTS_TIMEZONE_MIN) to +56 (@ref BT_CTS_TIMEZONE_MAX).
	 * A value of -128 (@ref BT_CTS_TIMEZONE_DEFAULT_VALUE) means that the timezone offset
	 * is not known. All other values are Reserved for Future Use.
	 * The offset defined in this characteristic is constant regardless of whether
	 * daylight savings is in effect.
	 */
	int8_t timezone_offset;
	/**
	 * @brief Represents the daylight saving time offset in 15-minute increments.
	 */
	enum bt_cts_dst_offset dst_offset;
};

/** @brief Current Time Service callback structure */
struct bt_cts_cb {
	/**
	 * @brief Current Time Service notifications changed
	 *
	 * @param enabled True if notifications are enabled, false if disabled
	 */
	void (*notification_changed)(bool enabled);

	/**
	 * @brief The Current Time has been requested to update by a peer.
	 *
	 * It is the responsibility of the application to store the new time.
	 *
	 * @param[in] cts_time Requested time to write
	 *
	 * @return 0 if written successfully, negative error code on failure
	 */
	int (*cts_time_write)(struct bt_cts_time_format *cts_time);

	/**
	 * @brief Callback to get current time.
	 *
	 * When current time Read request or notification is triggered, CTS uses
	 * this callback to retrieve current time information from application.
	 * Application must implement it and provide cts formatted current time
	 * information
	 *
	 * @param[out] cts_time Current time for the application to be filled
	 *
	 * @return 0 if filled successfully, negative error code on failure
	 */
	int (*fill_current_cts_time)(struct bt_cts_time_format *cts_time);

	/**
	 * @brief The Local Time Information has been requested to update by a peer.
	 *
	 * It is the responsibility of the application to store the new Local time.
	 *
	 * @param[in] local_time Requested local time to write
	 *
	 * @return 0 if written successfully, negative error code on failure
	 *
	 */
	int (*cts_local_time_write)(const struct bt_cts_local_time *cts_local_time);

	/**
	 * @brief Request to get current local time
	 *
	 * CTS uses this callback to retrieve local time information from application.
	 * Application must implement it and provide cts local time information
	 *
	 * @param[out] local_time Current local time struct to be filled
	 *
	 * @return 0 if filled successfully, negative error code on failure
	 */
	int (*fill_current_cts_local_time)(struct bt_cts_local_time *local_time);
};

/**
 * @brief This API should be called at application init.
 *
 * It is safe to call this API before or after bt_enable API
 *
 * @param cb pointer to required callback
 *
 * @return 0 on success, negative error code on failure
 */
int bt_cts_init(const struct bt_cts_cb *cb);

/**
 * @brief Notify all connected clients with CTS enabled
 *
 * @param reason update reason to be sent to the clients
 *
 * @return 0 on success, negative error code on failure
 */
int bt_cts_send_notification(enum bt_cts_update_reason reason);

/**
 * @brief Convert CTS formatted time to milliseconds since epoch
 *
 * @param[in] ct_time CTS formatted time
 * @param[out] unix_ms pointer to store parsed millisecond since epoch
 *
 * @return 0 on success, negative error code on failure
 */
int bt_cts_time_to_unix_ms(const struct bt_cts_time_format *ct_time, int64_t *unix_ms);

/**
 * @brief Convert milliseconds since epoch to CTS formatted time
 *
 * @param[out] ct_time Pointer to store CTS formatted time
 * @param[in] unix_ms  milliseconds since epoch to be converted
 *
 * @return 0 on success, negative error code on failure
 */
int bt_cts_time_from_unix_ms(struct bt_cts_time_format *ct_time, int64_t unix_ms);

/**
 * @brief Convert CTS local time to milliseconds
 *
 * @param[in] local_time Pointer to local time information containing timezone and DST offset
 * @param[out] relative_ms Pointer to variable containing the result value in milliseconds
 *
 * @return 0 on success, negative error code on failure
 */
int bt_cts_local_time_to_ms(const struct bt_cts_local_time *local_time, int32_t *relative_ms);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_CTS_H_ */
