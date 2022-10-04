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

#ifdef __cplusplus
extern "C" {
#endif

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

/** @def BT_IAS_CB_DEFINE
 *
 *  @brief Register a callback structure for immediate alert events.
 *
 *  @param _name Name of callback structure.
 */
#define BT_IAS_CB_DEFINE(_name)                                                                    \
	static const STRUCT_SECTION_ITERABLE(bt_ias_cb, _CONCAT(bt_ias_cb_, _name))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_IAS_H_ */
