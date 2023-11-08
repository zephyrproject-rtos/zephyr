/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_

/**
 * @brief Hearing Access Profile (HAP)
 *
 * @defgroup bt_hap Hearing Access Profile (HAP)
 *
 * @ingroup bluetooth
 * @{
 *
 * The Hearing Access Profile is used to identify and control a Hearing Aid devices.
 *
 * [Experimental] Users should note that the APIs can change as a part of
 * ongoing development.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/has.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hearing Aid Controller Maximum Set Size */
#define BT_HAP_HARC_SET_SIZE_MAX 2

/** @brief Opaque Hearing Aid Remote Controller instance. */
struct bt_hap_harc;

/** Hearing Aid Remote Controller info structure */
struct bt_hap_harc_info {
	/** Hearing Aid Type. */
	enum bt_has_hearing_aid_type type;
	/** Hearing Aid Features. */
	enum bt_has_features features;
	/** Hearing Aid Capabilities. */
	enum bt_has_capabilities caps;
	struct {
		/* Binaural pair controller instance */
		struct bt_hap_harc *pair;
		/* Coordinated set service instance used to match the devices */
		const struct bt_csip_set_coordinator_csis_inst *csis;
		/* Coordinated set member instance */
		const struct bt_csip_set_coordinator_set_member *member;
	} binaural;
};

/** @brief Hearing Aid Remote Controller callback structure. */
struct bt_hap_harc_cb {
	/**
	 * @brief Profile has been unbound.
	 *
	 * This callback notifies the application that the profile has been unbound.
	 *
	 * @param conn Controller instance pointer.
	 */
	void (*unbound)(struct bt_hap_harc *harc);

	/**
	 * @brief Controller has been connected.
	 *
	 * This callback notifies the application of a new connection to remote Hearing Aid device.
	 * In case the err parameter is non-zero it means that the connection establishment failed.
	 *
	 * @param conn Controller instance pointer.
	 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
	 */
	void (*connected)(struct bt_hap_harc *harc, int err);

	/**
	 * @brief Controller has been disconnected.
	 *
	 * This callback notifies the application that the controller has been disconnected.
	 *
	 * @param conn Controller instance pointer.
	 */
	void (*disconnected)(struct bt_hap_harc *harc);

	/**
	 * @brief Controller info has been updated.
	 *
	 * This callback notifies the application that the controller info has been changed,
	 * e.g. the Hearing Aid Features has been updated.
	 *
	 * @param conn Controller instance pointer.
	 * @param info Info object.
	 */
	void (*info)(struct bt_hap_harc *harc, const struct bt_hap_harc_info *info);

	sys_snode_t _node;
};

/**
 * @brief Register Hearing Aid Remote Controller callbacks.
 *
 * @param cb The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_hap_harc_cb_register(struct bt_hap_harc_cb *cb);

/**
 * @brief Unregister Hearing Aid Remote Controller callbacks.
 *
 * @param cb The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_hap_harc_cb_unregister(struct bt_hap_harc_cb *cb);

/**
 * @brief Bind profile.
 *
 * Bind profile to remote device implementing HAP Hearing Aid role.
 *
 * @param conn Bluetooth connection object.
 * @param[out] harc Valid controller object on success.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_hap_harc_bind(struct bt_conn *conn, struct bt_hap_harc **harc);

/**
 * @brief Unbind profile.
 *
 * Unbind profile or cancel pending profile binding.
 *
 * @param harc Controller instance pointer.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_hap_harc_unbind(struct bt_hap_harc *harc);

/** @brief Hearing Aid Remote Controller Preset callback structure. */
struct bt_hap_harc_preset_cb {
	/**
	 * @brief Callback function for Hearing Access Service active preset changes.
	 *
	 * Optional callback called when the active preset is changed by the remote server when the
	 * preset switch procedure is complete. The callback must be set to receive active preset
	 * changes and enable support for switching presets.
	 *
	 * @param harc Controller instance pointer.
	 * @param index Active preset index.
	 */
	void (*active)(struct bt_hap_harc *harc, uint8_t index);

	/**
	 * @brief Callback function for preset store.
	 *
	 * The callback is called when the Preset Record Update ot Preset Read Response is notified
	 * by the remote server.
	 * The record object as well as its objects are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param harc Controller instance pointer.
	 * @param record Preset record to store.
	 */
	void (*store)(struct bt_hap_harc *harc, const struct bt_has_preset_record *record);

	/**
	 * @brief Callback function for preset deletion.
	 *
	 * The callback provides a range of presets that have been removed on the remote server.
	 * The application is obliged to remove the cached entries within the index range.
	 *
	 * @param conn Bluetooth connection object.
	 * @param start_index Preset start index that has been deleted.
	 * @param end_index Preset end index that has been deleted.
	 */
	void (*remove)(struct bt_hap_harc *harc, uint8_t start_index, uint8_t end_index);

	/**
	 * @brief Callback function for preset availability notifications.
	 *
	 * The callback is called when the preset availability change is notified by the remote
	 * server.
	 *
	 * @param harc Controller instance pointer.
	 * @param index Preset index.
	 * @param available True if available, false otherwise.
	 */
	void (*available)(struct bt_hap_harc *harc, uint8_t index, bool available);

	/**
	 * @brief Preset list update complete.
	 *
	 * Function called after all the preset list changes have been submitted.
	 * User might apply the updated list to the application.
	 *
	 * @param harc Controller instance pointer.
	 */
	void (*commit)(struct bt_hap_harc *harc);

	/**
	 * @brief Preset get callback function.
	 *
	 * The callback is called to retrieve the stored preset record.
	 *
	 * @param harc Controller instance pointer.
	 * @param index Preset index.
	 * @param[out] record Preset record object to fill in.
	 *
	 * @return 0 if success, errno on failure.
	 */
	int (*get)(struct bt_hap_harc *harc, uint8_t index, struct bt_has_preset_record *record);
};

/**
 * @brief Registers Hearing Aid Remote Controller Preset List callbacks.
 *
 * @param cb The callback structure.
 *
 * @return 0 if success, errno on failure.
 */
int bt_hap_harc_preset_cb_register(const struct bt_hap_harc_preset_cb *cb);

/**
 * @brief Get information for the profile connection.
 *
 * @param harc      Controller instance pointer.
 * @param[out] info Info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_hap_harc_info_get(const struct bt_hap_harc *harc, struct bt_hap_harc_info *info);

/**
 * @brief Get the connection pointer of a Hearing Aid Remote Controller instance
 *
 * Get the Bluetooth connection pointer of a Hearing Aid Remote Controller instance.
 *
 * @param harc Controller instance pointer.
 * @param[out] conn Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_hap_harc_conn_get(const struct bt_hap_harc *harc, struct bt_conn **conn);

/**
 * @brief Procedure complete result callback.
 *
 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
 * @param params Parameters passed in by the user.
 */
typedef void (*bt_hap_harc_complete_func_t)(int err, void *params);

/**
 * @brief Procedure status callback.
 *
 * @param harc Controller instance pointer.
 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
 * @param params Procedure parameters.
 */
typedef void (*bt_hap_harc_status_func_t)(struct bt_hap_harc *harc, int err, void *params);

/** @brief Read Preset parameters */
struct bt_hap_harc_preset_read_params {
	/** Complete result callback */
	bt_hap_harc_complete_func_t complete;
	/** The index to start with. */
	uint8_t start_index;
	/** Maximum number of presets to read. */
	uint8_t max_count;
};

/**
 * @brief Read Preset Records.
 *
 * This procedure is used to read up to @p max_count presets starting from given @p index.
 * The Response comes in callback @ref bt_hap_harc_preset_cb.store callback
 * (called once for each preset).
 * @p params must remain valid until start of @p params->complete callback.
 *
 * @param harc Controller instance to perform procedure on.
 * @param params Read parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_read(struct bt_hap_harc *harc,
			    struct bt_hap_harc_preset_read_params *params);

/** @brief Set Preset parameters */
struct bt_hap_harc_preset_set_params {
	/** Complete result callback. */
	bt_hap_harc_complete_func_t complete;
	/** Set preset callback. */
	bt_hap_harc_status_func_t status;
	/** Reserved for internal use */
	void *reserved;
};

/**
 * @brief Get Active Preset index.
 *
 * This function is used to get the currently active preset index.
 *
 * @param harc Controller instance.
 * @param[out] index Active index.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_get_active(struct bt_hap_harc *harc, uint8_t *index);

/**
 * @brief Set Active Preset.
 *
 * This procedure is used to set preset identified by @p index as active.
 * The status comes in @p params->status callback (called once per each instance).
 * @p params must remain valid until start of @p params->complete callback.
 *
 * @param harcs Array of controller instances to perform procedure on.
 * @param count Number of controllers in @p harcs.
 * @param index Preset index to activate.
 * @param params Set parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_set(struct bt_hap_harc **harcs, uint8_t count, uint8_t index,
			   struct bt_hap_harc_preset_set_params *params);

/**
 * @brief Activate Next Preset.
 *
 * This procedure is used to set next available preset as active.
 * The status comes in @p params->status callback (called once per each instance).
 * @p params must remain valid until start of @p params->complete callback.
 *
 * @param harcs Array of controller instances to perform procedure on.
 * @param count Number of controllers in @p harcs.
 * @param params Set parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_set_next(struct bt_hap_harc **harcs, uint8_t count,
				struct bt_hap_harc_preset_set_params *params);

/**
 * @brief Activate Previous Preset.
 *
 * This procedure is used to set previous available preset as active.
 * The status comes in @p params->status callback (called once per each instance).
 * @p params must remain valid until start of @p params->complete callback.
 *
 * @param harcs Array of controller instances to perform procedure on.
 * @param count Number of controllers in @p harcs.
 * @param params Set parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_set_prev(struct bt_hap_harc **harcs, uint8_t count,
				struct bt_hap_harc_preset_set_params *params);

/** @brief Write Preset parameters */
struct bt_hap_harc_preset_write_params {
	/** Complete result callback */
	bt_hap_harc_complete_func_t complete;
	/** Write preset callback. */
	bt_hap_harc_status_func_t status;
	/** Preset index to change the name of. */
	uint8_t index;
	/** Preset name to write. */
	const char *name;
};

/**
 * @brief Write Preset Name.
 *
 * This procedure is used to change the preset name.
 * The status comes in @p params->status callback (called once per each instance).
 * @p params must remain valid until start of @p params->complete callback.
 *
 * @param harcs Array of controller instances to perform procedure on.
 * @param count Number of controllers in @p harcs.
 * @param params Write parameters.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_hap_harc_preset_write(struct bt_hap_harc **harcs, uint8_t count,
			     struct bt_hap_harc_preset_write_params *params);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_HAP_H_ */
