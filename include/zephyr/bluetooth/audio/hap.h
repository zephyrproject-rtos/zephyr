/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_

/**
 * @brief Hearing Access Service (HAS)
 *
 * @defgroup bt_has Hearing Access Service (HAS)
 *
 * @ingroup bluetooth
 * @{
 *
 * The Hearing Access Service is used to identify a hearing aid and optionally
 * to control hearing aid presets.
 *
 * [Experimental] Users should note that the APIs can change as a part of
 * ongoing development.
 */

#include <stdint.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <sys/types.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hearing Access Service Client callback structure. */
struct bt_hap_harc_cb {
	/**
	 * @brief Callback function for bt_hap_harc_discover.
	 *
	 * This callback is called when discovery procedure is complete.
	 *
	 * @param conn Bluetooth connection object.
	 * @param err 0 on success, ATT error code or negative errno otherwise.
	 */
	void (*discover)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback function for active preset changes.
	 *
	 * The mandatory callback to receive active preset changes.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param err 0 on success, ATT error or negative errno otherwise.
	 * @param index Active preset index.
	 */
	void (*active_preset)(struct bt_conn *conn, uint8_t index);

	/**
	 * @brief Callback function for  active preset changes.
	 *
	 * The mandatory callback to receive active preset changes.
	 *
	 * @param has Pointer to the Hearing Access Service object.
	 * @param err 0 on success, ATT error or negative errno otherwise.
	 * @param index Active preset index.
	 */

	/* Volume Controller callbacks */
	struct bt_vcp_vol_ctlr_cb *vol_ctrl_cb;
};

/** @brief Registers the callbacks used by the Hearing Access Service client.
 *
 *  @param cb The callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_init(struct bt_hap_harc_cb *cb);

/**
 * @brief Bind Hearing Access Service on a remote device.
 *
 * Client method to find a Hearing Access Service on a remote device identified by @p conn.
 * The @ref bt_hap_harc_cb.discover_cb callback will be called when the discovery procedure
 * is complete to provide user a @ref bt_hap_ha_set object.
 *
 * @param conn Bluetooth connection object.
 *
 * @return 0 if success, errno on failure.
 */
int bt_hap_harc_discover(struct bt_conn *conn);

/**
 * @brief Operation complete result callback.
 *
 * @param conn Connection object.
 * @param err 0 on success, ATT error or negative errno otherwise.
 * @param user_data Data passed in by the user.
 */
typedef void (*bt_hap_harc_complete_func_t)(struct bt_conn *conn, int err, void *user_data);

/**
 * @brief Activate Next Preset.
 *
 * Client procedure to select next available preset as active.
 * The status is returned in the @ref bt_has_client_cb.preset_switch callback.
 *
 * @param has Pointer to the Hearing Access Service object.
 * @param func Transmission complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_next(struct bt_conn *conn, bt_hap_harc_complete_func_t func,
			    void *user_data);

/**
 * @brief Activate Previous Preset.
 *
 * Client procedure to select previous available preset as active.
 * The status is returned in the @ref bt_has_client_cb.preset_switch callback.
 *
 * @param has Pointer to the Hearing Access Service object.
 * @param func Transmission complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_prev(struct bt_conn *conn, bt_hap_harc_complete_func_t func,
			    void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_ */
