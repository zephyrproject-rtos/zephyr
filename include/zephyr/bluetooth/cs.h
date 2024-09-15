/** @file
 *  @brief Bluetooth Channel Sounding handling
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CS_H_

/**
 * @brief Channel Sounding (CS)
 * @defgroup bt_cs Channel Sounding (CS)
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

enum bt_cs_sync_antenna_selection_opt {
	/** Use antenna identifier 1 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_ONE = BT_HCI_OP_LE_CS_ANTENNA_SEL_ONE,
	/** Use antenna identifier 2 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_TWO = BT_HCI_OP_LE_CS_ANTENNA_SEL_TWO,
	/** Use antenna identifier 3 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_THREE = BT_HCI_OP_LE_CS_ANTENNA_SEL_THREE,
	/** Use antenna identifier 4 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_FOUR = BT_HCI_OP_LE_CS_ANTENNA_SEL_FOUR,
	/** Use antennas in repetitive order from 1 to 4 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_REPETITIVE = BT_HCI_OP_LE_CS_ANTENNA_SEL_REP,
	/** No recommendation for local controller antenna selection. */
	BT_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION = BT_HCI_OP_LE_CS_ANTENNA_SEL_NONE,
};

/** Default CS settings in the local Controller */
struct bt_cs_set_default_settings_param {
	/** Enable CS initiator role. */
	bool enable_initiator_role;
	/** Enable CS reflector role. */
	bool enable_reflector_role;
	/** Antenna identifier to be used for CS_SYNC packets by the local controller.
	 */
	enum bt_cs_sync_antenna_selection_opt cs_sync_antenna_selection;
	/** Maximum output power (Effective Isotropic Radiated Power) to be used
	 *  for all CS transmissions.
	 *
	 *  Value range is @ref BT_HCI_OP_LE_CS_MIN_MAX_TX_POWER to
	 *  @ref BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER.
	 */
	int8_t max_tx_power;
};


/** @brief Read Remote Supported Capabilities
 *
 * This command is used to query the CS capabilities that are supported
 * by the remote controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_cs_read_remote_supported_capabilities(struct bt_conn *conn);

/** @brief Set Channel Sounding default settings.
 *
 * This command is used to set default Channel Sounding settings for this
 * connection.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 * @param params Channel sounding default settings parameters.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_cs_set_default_settings(struct bt_conn *conn,
			       const struct bt_cs_set_default_settings_param *params);

/** @brief Read Remote FAE Table
 *
 * This command is used to read the per-channel mode-0 Frequency Actuation Error
 * table of the remote Controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_cs_read_remote_fae_table(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CS_H_ */
