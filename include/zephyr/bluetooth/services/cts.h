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
#include <zephyr/posix/time.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** @brief Current Time Service callback structure */
struct bt_cts_cb {
	/** @brief Current Time Service notifications changed
	 *
	 * @param enabled True if notifications are enabled, false if disabled
	 */
	void (*notification_changed)(bool enabled);

	/**
	 * @brief The Current Time has been updated by a peer.
	 * It is the responsibility of the application to store the new time.
	 *
	 * @param cts_time [IN] updated time
	 *
	 * @return 0 application has decoded it successfully
	 * @return negative error codes on failure
	 *
	 */
	int (*cts_time_write)(struct bt_cts_time_format *cts_time);

	/**
	 * @brief When current time Read request or notification is triggered, CTS uses
	 * this callback to retrieve current time information from application. Application
	 * must implement it and provide cts formatted current time information
	 *
	 * @note this callback is mandatory
	 *
	 * @param cts_time [IN] updated time
	 *
	 * @return 0 application has encoded it successfully
	 * @return negative error codes on failure
	 */
	int (*fill_current_cts_time)(struct bt_cts_time_format *cts_time);
};

/**
 * @brief This API should be called at application init.
 * it is safe to call this API before or after bt_enable API
 *
 * @param cb pointer to required callback
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_cts_init(const struct bt_cts_cb *cb);

/**
 * @brief Notify all connected clients that have enabled the
 * current time update notification
 *
 * @param reason update reason to be sent to the clients
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_cts_send_notification(enum bt_cts_update_reason reason);

/**
 * @brief Helper API to decode CTS formatted time into milliseconds since epoch
 *
 * @note @kconfig{CONFIG_BT_CTS_HELPER_API} needs to be enabled to use this API.
 *
 * @param ct_time [IN]  cts time formatted time
 * @param unix_ms [OUT] pointer to store parsed millisecond since epoch
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_cts_time_to_unix_ms(const struct bt_cts_time_format *ct_time, int64_t *unix_ms);

/**
 * @brief Helper API to encode milliseconds since epoch to CTS formatted time
 *
 * @note @kconfig{CONFIG_BT_CTS_HELPER_API} needs to be enabled to use this API.
 *
 * @param ct_time [OUT] Pointer to store CTS formatted time
 * @param unix_ms [IN]  milliseconds since epoch to be converted
 *
 * @return 0 on success
 * @return negative error codes on failure
 */
int bt_cts_time_from_unix_ms(struct bt_cts_time_format *ct_time, int64_t unix_ms);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_CTS_H_ */
