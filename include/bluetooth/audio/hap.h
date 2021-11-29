/*
 * Copyright (c) 2021 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAP_H_

/**
 * @brief Hearing Access Profile (HAP)
 *
 * @defgroup bt_hap Hearing Access Profile (HAP)
 *
 * @ingroup bluetooth
 * @{
 *
 * The Hearing Access Profile is used to identify a hearing aid and optionally
 * to control hearing aid presets.
 *
 * [Experimental] Users should note that the APIs can change as a part of
 * ongoing development.
 */

#include <zephyr/types.h>
#include <bluetooth/audio/has.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque Hearing Access Profile object. */
struct bt_hap;

struct bt_hap_cb {
	/**
	 * @brief A profile has been connected.
	 *
	 * This callback notifies the application of a new profile connection.
	 * In case the err parameter is non-zero it means that the profile
	 * connection failed.
	 *
	 * @param hap Profile instance object if any.
	 * @param ha_type Hearing Aid Type.
	 * @param error Zero for success, non-zero otherwise.
	 */
	void (*connected)(struct bt_hap *hap, uint8_t ha_type, int error);

	/**
	 * @brief A profile has been disconnected.
	 *
	 * This callback notifies the application that a profile
	 * has been disconnected.
	 *
	 * @param hap Profile instance object.
	 * @param reason Reason for the disconnection.
	 */
	void (*disconnected)(struct bt_hap *hap, int reason);

	/**
	 * @brief A profile instance has been deleted.
	 *
	 * This callback notifies the application that a profile
	 * instance has been removed. This may be called when the bond has
	 * been removed or @ref bt_hap_delete has been called.
	 *
	 * @param hap Profile instance object.
	 */
	void (*deleted)(struct bt_hap *hap);

	/**
	 * @brief An active preset has been changed.
	 *
	 * This callback notifies the application when the active preset or
	 * it's name has been changed.
	 * If @kconfig{BT_HAP_HARC_PRESET_CACHE_SIZE} is 0 the preset name will
	 * not be set, but if @kconfig{BT_HAP_HARC_AUTO_READ_ACTIVE_PRESET}
	 * option is enabled, the callback will be called again on preset read
	 * complete.
	 *
	 * @param hap Profile instance object.
	 * @param preset Preset record object.
	 */
	void (*active_preset_changed)(struct bt_hap *hap,
				      const struct bt_has_preset *preset);

	/**
	 * @brief A cached preset list has been updated.
	 *
	 * This callback notifies the application when the list of preset
	 * records changed i.e. record has been added/removed, changed name or
	 * become available.
	 * The user can call @ref bt_hap_preset_foreach to obtain the current
	 * presets.
	 *
	 * @param hap Profile instance object.
	 */
	void (*preset_cache_updated)(struct bt_hap *hap);
};

/**
 * @def BT_HAP_CB_DEFINE
 *
 * @brief Register a callback structure for Hearing Access Profile events.
 *
 * @param _name Name of callback structure.
 */
#define BT_HAP_CB_DEFINE(_name)							\
	static const STRUCT_SECTION_ITERABLE(bt_hap_cb,				\
					     _CONCAT(bt_hap_cb_, _name))

/**
 * @brief Create profile instance to remote Hearing Access Service
 *
 * This will start a GATT discovery to setup handles and subscriptions.
 * This shall be called once before any other actions can be executed for the
 * peer device, and the @ref bt_hap_cb connected callback will notify
 * when it is possible to start remote operations.
 * The profile instance is removed when the bond is removed or when @ref
 * bt_hap_delete is called.
 * 
 * @param conn The connection to initialize the profile for.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_new(struct bt_conn *conn);

/**
 * @brief Delete profile instance
 *
 * This will remove stored profile instance of remote Hearing Aid Service.
 *
 * @param hap Profile instance object.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_delete(struct bt_hap *hap);

/**
 * @brief Iterate through all existing profile instances.
 *
 * Iterate profile instances of remote Hearing Access Services.
 *
 * @param func Function to call for each profile instance.
 * @param user_data Data to pass to the callback function.
 */
void bt_hap_foreach(bool (*func)(const struct bt_hap *hap, void *user_data),
		    void *user_data);

/**
 * @brief Get profile instance to local Hearing Access Service.
 *
 * Get the profile instance connection to local Hearing Access Service.
 *
 * @return Profile instance object or NULL if there is no service.
 */
struct bt_hap *bt_hap_local_get(void);

/**
 * @brief Get the Bluetooth connection object of the profile instance.
 *
 * Get the Bluetooth connection object of a HAP instance.
 *
 * @param hap Profile instance object.
 *
 * @return Bluetooth connection object or NULL.
 */
struct bt_conn *bt_hap_conn_get(const struct bt_hap *hap);

/** 
 * @typedef bt_hap_complete_func_t
 * @brief Operation complete callback.
 *
 * @param hap Profile instance object.
 * @param err Error code.
 * @param user_data User data passed to callback.
 */
typedef void (*bt_hap_complete_func_t) (struct bt_hap *hap, int err,
					void *user_data);

/**
 * @brief Write Preset Name.
 *
 * Write the Name of a writable preset record identified by the Index.
 *
 * @param hap Profile instance object.
 * @param index Index of preset record that name to be written.
 * @param data Name to be written.
 * @param length Name length.
 * @param func Operation complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_preset_write_name(struct bt_hap *hap, uint8_t index,
			     const void *data, uint8_t length,
			     bt_hap_complete_func_t func, void *user_data);

/**
 * @brief Set Active Preset.
 *
 * Set the preset record identified by the Index field as the active preset.
 *
 * @param hap Profile instance object.
 * @param index Preset record index to be set to active.
 * @param func Operation complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_preset_set_active(struct bt_hap *hap, uint8_t index,
		      	     bt_hap_complete_func_t func, void *user_data);

/**
 * @brief Get Active Preset.
 *
 * @param hap Profile instance object.
 *
 * @return Preset record object.
 */
const struct bt_has_preset *bt_hap_preset_get_active(struct bt_hap *hap);

/**
 * @brief Set Next Preset.
 *
 * Set the next preset record in the server list of preset records as the
 * active preset.
 *
 * @param hap Profile instance object.
 * @param func Operation complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_preset_set_active_next(struct bt_hap *hap,
				  bt_hap_complete_func_t func, void *user_data);

/**
 * @brief Set Previous Preset.
 *
 * Set the previous preset record in the server’s list of preset records as
 * the active preset.
 *
 * @param hap Profile instance object.
 * @param func Operation complete callback.
 * @param user_data User data to be passed back to callback.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_preset_set_active_prev(struct bt_hap *hap,
				  bt_hap_complete_func_t func, void *user_data);

/**
 * @brief Iterate through all cached preset record instances.
 *
 * @param hap Profile instance object.
 * @param func Function to call for each preset record object.
 * @param user_data Data to pass to the callback function.
 */
void bt_hap_preset_foreach(struct bt_hap *hap,
			   bool (*func)(const struct bt_hap *hap,
					const struct bt_has_preset *preset,
					void *user_data),
			   void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HAP_H_ */
