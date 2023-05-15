/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_IAS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_IAS_H_

/**
 * @brief Immediate Alert Service (IAS)
 * @defgroup bt_ias Immediate Alert Service (IAS)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

enum bt_ias_alert_lvl {
	/** No alerting should be done on device */
	BT_IAS_ALERT_LVL_NO_ALERT,

	/** Device shall alert */
	BT_IAS_ALERT_LVL_MILD_ALERT,

	/** Device should alert in strongest possible way */
	BT_IAS_ALERT_LVL_HIGH_ALERT,
};

/** @brief Immediate Alert Service callback structure. */
struct bt_ias_cb {
	/**
	 * @brief Callback function to stop alert.
	 *
	 * This callback is called when peer commands to disable alert.
	 */
	void (*no_alert)(void);

	/**
	 * @brief Callback function for alert level value.
	 *
	 * This callback is called when peer commands to alert.
	 */
	void (*mild_alert)(void);

	/**
	 * @brief Callback function for alert level value.
	 *
	 * This callback is called when peer commands to alert in the strongest possible way.
	 */
	void (*high_alert)(void);
};

/** @brief Method for stopping alert locally
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_ias_local_alert_stop(void);

/**
 *  @brief Register a callback structure for immediate alert events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_IAS_CB_DEFINE(_name)                                                                    \
	static const STRUCT_SECTION_ITERABLE(bt_ias_cb, _CONCAT(bt_ias_cb_, _name))

struct bt_ias_client_cb {
	/** @brief Callback function for bt_ias_discover.
	 *
	 *  This callback is called when discovery procedure is complete.
	 *
	 *  @param conn Bluetooth connection object.
	 *  @param err 0 on success, ATT error or negative errno otherwise
	 */
	void (*discover)(struct bt_conn *conn, int err);
};

/** @brief Set alert level
 *
 *  @param conn Bluetooth connection object
 *  @param bt_ias_alert_lvl Level of alert to write
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_ias_client_alert_write(struct bt_conn *conn, enum bt_ias_alert_lvl);

/** @brief Discover Immediate Alert Service
 *
 *  @param conn Bluetooth connection object
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_ias_discover(struct bt_conn *conn);

/** @brief Register Immediate Alert Client callbacks
 *
 *  @param cb The callback structure
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_ias_client_cb_register(const struct bt_ias_client_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_IAS_H_ */
