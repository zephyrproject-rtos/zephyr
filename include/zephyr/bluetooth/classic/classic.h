/** @file
 *  @brief Bluetooth subsystem classic core APIs.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Access Profile (GAP)
 * @defgroup bt_gap Generic Access Profile (GAP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/**
 * @private
 * @brief BR/EDR discovery private structure
 */
struct bt_br_discovery_priv {
	/** Clock offset */
	uint16_t clock_offset;
	/** Page scan repetition mode */
	uint8_t pscan_rep_mode;
	/** Resolving remote name*/
	bool resolving;
};

/** @brief BR/EDR discovery result structure */
struct bt_br_discovery_result {
	/** Private data */
	struct bt_br_discovery_priv _priv;

	/** Remote device address */
	bt_addr_t addr;

	/** RSSI from inquiry */
	int8_t rssi;

	/** Class of Device */
	uint8_t cod[3];

	/** Extended Inquiry Response */
	uint8_t eir[240];
};

/** BR/EDR discovery parameters */
struct bt_br_discovery_param {
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	uint8_t length;

	/** True if limited discovery procedure is to be used. */
	bool limited;
};

/**
 * @brief Start BR/EDR discovery
 *
 * Start BR/EDR discovery (inquiry) and provide results through the specified
 * callback. The discovery results will be notified through callbacks
 * registered by @ref bt_br_discovery_cb_register.
 * If more inquiry results were received during session than
 * fits in provided result storage, only ones with highest RSSI will be
 * reported.
 *
 * @param param Discovery parameters.
 * @param results Storage for discovery results.
 * @param count Number of results in storage. Valid range: 1-255.
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t count);

/**
 * @brief Stop BR/EDR discovery.
 *
 * Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 * results won't be reported
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_br_discovery_stop(void);

struct bt_br_discovery_cb {

	/**
	 * @brief An inquiry response received callback.
	 *
	 * @param result Storage used for discovery results
	 */
	void (*recv)(const struct bt_br_discovery_result *result);

	/** @brief The inquiry has stopped after discovery timeout.
	 *
	 * @param results Storage used for discovery results
	 * @param count Number of valid discovery results.
	 */
	void (*timeout)(const struct bt_br_discovery_result *results,
				  size_t count);

	sys_snode_t node;
};

/**
 * @brief Register discovery packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * inquiry activity.
 *
 * This callback will be called for all inquiry activity, regardless of what
 * API was used to start the discovery.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_register(struct bt_br_discovery_cb *cb);

/**
 * @brief Unregister discovery packet callbacks.
 *
 * Remove the callback structure from the list of discovery callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_unregister(struct bt_br_discovery_cb *cb);

struct bt_br_oob {
	/** BR/EDR address. */
	bt_addr_t addr;
};

/**
 * @brief Get BR/EDR local Out Of Band information
 *
 * This function allows to get local controller information that are useful
 * for Out Of Band pairing or connection creation process.
 *
 * @param oob Out Of Band information
 */
int bt_br_oob_get_local(struct bt_br_oob *oob);

/**
 * @brief Enable/disable set controller in discoverable state.
 *
 * Allows make local controller to listen on INQUIRY SCAN channel and responds
 * to devices making general inquiry. To enable this state it's mandatory
 * to first be in connectable state.
 *
 * @param enable Value allowing/disallowing controller to become discoverable.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_discoverable(bool enable);

/**
 * @brief Enable/disable set controller in connectable state.
 *
 * Allows make local controller to be connectable. It means the controller
 * start listen to devices requests on PAGE SCAN channel. If disabled also
 * resets discoverability if was set.
 *
 * @param enable Value allowing/disallowing controller to be connectable.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_connectable(bool enable);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_ */
