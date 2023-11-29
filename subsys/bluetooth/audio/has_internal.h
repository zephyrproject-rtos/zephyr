/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/has.h>

/* Control Point opcodes */
#define BT_HAS_OP_READ_PRESET_REQ        0x01
#define BT_HAS_OP_READ_PRESET_RSP        0x02
#define BT_HAS_OP_PRESET_CHANGED         0x03
#define BT_HAS_OP_WRITE_PRESET_NAME      0x04
#define BT_HAS_OP_SET_ACTIVE_PRESET      0x05
#define BT_HAS_OP_SET_NEXT_PRESET        0x06
#define BT_HAS_OP_SET_PREV_PRESET        0x07
#define BT_HAS_OP_SET_ACTIVE_PRESET_SYNC 0x08
#define BT_HAS_OP_SET_NEXT_PRESET_SYNC   0x09
#define BT_HAS_OP_SET_PREV_PRESET_SYNC   0x0a

/* Application error codes */
#define BT_HAS_ERR_INVALID_OPCODE         0x80
#define BT_HAS_ERR_WRITE_NAME_NOT_ALLOWED 0x81
#define BT_HAS_ERR_PRESET_SYNC_NOT_SUPP   0x82
#define BT_HAS_ERR_OPERATION_NOT_POSSIBLE 0x83
#define BT_HAS_ERR_INVALID_PARAM_LEN      0x84

#define BT_HAS_FEAT_HEARING_AID_TYPE_MASK (BT_HAS_FEAT_HEARING_AID_TYPE_LSB | \
					   BT_HAS_FEAT_HEARING_AID_TYPE_MSB)

/* Preset Changed Change ID values */
#define BT_HAS_CHANGE_ID_GENERIC_UPDATE     0x00
#define BT_HAS_CHANGE_ID_PRESET_DELETED     0x01
#define BT_HAS_CHANGE_ID_PRESET_AVAILABLE   0x02
#define BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE 0x03

#define BT_HAS_IS_LAST 0x01

struct bt_has {
	/** Hearing Aid Features value */
	enum bt_has_features features;

	/** Active preset index value */
	uint8_t active_index;

	/* Whether the service has been registered or not */
	bool registered;
};

struct bt_has_cp_hdr {
	uint8_t opcode;
	uint8_t data[0];
} __packed;

struct bt_has_cp_read_presets_req {
	uint8_t start_index;
	uint8_t num_presets;
} __packed;

struct bt_has_cp_read_preset_rsp {
	uint8_t is_last;
	uint8_t index;
	uint8_t properties;
	uint8_t name[0];
} __packed;

struct bt_has_cp_preset_changed {
	uint8_t change_id;
	uint8_t is_last;
	uint8_t additional_params[0];
} __packed;

struct bt_has_cp_generic_update {
	uint8_t prev_index;
	uint8_t index;
	uint8_t properties;
	uint8_t name[0];
} __packed;

struct bt_has_cp_write_preset_name {
	uint8_t index;
	uint8_t name[0];
} __packed;

struct bt_has_cp_set_active_preset {
	uint8_t index;
} __packed;

static inline const char *bt_has_op_str(uint8_t op)
{
	switch (op) {
	case BT_HAS_OP_READ_PRESET_REQ:
		return "Read preset request";
	case BT_HAS_OP_READ_PRESET_RSP:
		return "Read preset response";
	case BT_HAS_OP_PRESET_CHANGED:
		return "Preset changed";
	case BT_HAS_OP_WRITE_PRESET_NAME:
		return "Write preset name";
	case BT_HAS_OP_SET_ACTIVE_PRESET:
		return "Set active preset";
	case BT_HAS_OP_SET_NEXT_PRESET:
		return "Set next preset";
	case BT_HAS_OP_SET_PREV_PRESET:
		return "Set previous preset";
	case BT_HAS_OP_SET_ACTIVE_PRESET_SYNC:
		return "Set active preset (sync)";
	case BT_HAS_OP_SET_NEXT_PRESET_SYNC:
		return "Set next preset (sync)";
	case BT_HAS_OP_SET_PREV_PRESET_SYNC:
		return "Set previous preset (sync)";
	default:
		return "Unknown";
	}
}

static inline const char *bt_has_change_id_str(uint8_t change_id)
{
	switch (change_id) {
	case BT_HAS_CHANGE_ID_GENERIC_UPDATE:
		return "Generic update";
	case BT_HAS_CHANGE_ID_PRESET_DELETED:
		return "Preset deleted";
	case BT_HAS_CHANGE_ID_PRESET_AVAILABLE:
		return "Preset available";
	case BT_HAS_CHANGE_ID_PRESET_UNAVAILABLE:
		return "Preset unavailable";
	default:
		return "Unknown changeId";
	}
}

static inline enum bt_has_hearing_aid_type bt_has_features_get_type(enum bt_has_features features)
{
	return features & BT_HAS_FEAT_HEARING_AID_TYPE_MASK;
}

static inline bool bt_has_features_check_preset_sync_supp(enum bt_has_features features)
{
	return (features & BT_HAS_FEAT_PRESET_SYNC_SUPP) > 0;
}

static inline bool bt_has_features_check_preset_list_independent(enum bt_has_features features)
{
	return (features & BT_HAS_FEAT_INDEPENDENT_PRESETS) > 0;
}

static inline bool bt_has_features_check_preset_list_dynamic(enum bt_has_features features)
{
	return (features & BT_HAS_FEAT_DYNAMIC_PRESETS) > 0;
}

static inline bool bt_has_features_check_preset_write_supported(enum bt_has_features features)
{
	return (features & BT_HAS_FEAT_DYNAMIC_PRESETS) > 0;
}

/** @brief Opaque Hearing Access Service Client instance. */
struct bt_has_client;

/** @brief Hearing Access Service Client callback structure. */
struct bt_has_client_cb {
	/**
	 * @brief Service has been connected.
	 *
	 * This callback notifies the application of a new service connection.
	 * In case the err parameter is non-zero it means that the connection establishment failed.
	 *
	 * @param client Client instance.
	 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
	 */
	void (*connected)(struct bt_has_client *client, int err);

	/**
	 * @brief Service has been disconnected.
	 *
	 * This callback notifies the application that the service has been disconnected.
	 *
	 * @param client Client instance.
	 * @param unbound True if service binding has been removed.
	 */
	void (*disconnected)(struct bt_has_client *client);

	/**
	 * @brief Client instance unbound.
	 *
	 * This callback notifies the application that the client instance has been free'd.
	 *
	 * @param client Client instance.
	 * @param err 0 in case of success or negative errno or ATT Error code in case of error.
	 */
	void (*unbound)(struct bt_has_client *client, int err);

	/**
	 * @brief Callback function for Hearing Access Service active preset changes.
	 *
	 * Optional callback called when the active preset is changed by the remote server when the
	 * preset switch procedure is complete. The callback must be set to receive active preset
	 * changes and enable support for switching presets. If the callback is not set, the Active
	 * Index and Control Point characteristics will not be discovered by
	 * @ref bt_has_client_discover.
	 *
	 * @param client Client instance.
	 * @param index Active preset index.
	 */
	void (*preset_switch)(struct bt_has_client *client, uint8_t index);

	/**
	 * @brief Callback function for presets read operation.
	 *
	 * The callback is called when the preset read response is sent by the remote server.
	 * The record object as well as its members are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param client Client instance.
	 * @param record Preset record or NULL on errors.
	 * @param is_last True if Read Presets operation can be considered concluded.
	 */
	void (*preset_read_rsp)(struct bt_has_client *client,
				const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset update notifications.
	 *
	 * The callback is called when the preset record update is notified by the remote server.
	 * The record object as well as its objects are temporary and must be copied to in order
	 * to cache its information.
	 *
	 * @param client Client instance.
	 * @param index_prev Index of the previous preset in the list.
	 * @param record Preset record.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_update)(struct bt_has_client *client, uint8_t index_prev,
			      const struct bt_has_preset_record *record, bool is_last);

	/**
	 * @brief Callback function for preset deletion notifications.
	 *
	 * The callback is called when the preset has been deleted by the remote server.
	 *
	 * @param client Client instance.
	 * @param index Preset index.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_deleted)(struct bt_has_client *client, uint8_t index, bool is_last);

	/**
	 * @brief Callback function for preset availability notifications.
	 *
	 * The callback is called when the preset availability change is notified by the remote
	 * server.
	 *
	 * @param client Client instance.
	 * @param index Preset index.
	 * @param available True if available, false otherwise.
	 * @param is_last True if preset list update operation can be considered concluded.
	 */
	void (*preset_availability)(struct bt_has_client *client, uint8_t index, bool available,
				    bool is_last);

	/**
	 * @brief Callback function for features notifications.
	 *
	 * The callback is called when the Hearing Aid Features change is notified by the remote
	 * server.
	 *
	 * @param client Client instance.
	 * @param features Updated features value.
	 */
	void (*features)(struct bt_has_client *client, enum bt_has_features features);

	/**
	 * @brief Callback function for command operations.
	 *
	 * The callback is called when command status is received from the remote server.
	 *
	 * @param client Client instance.
	 * @param err 0 on success, ATT error otherwise.
	 */
	void (*cmd_status)(struct bt_has_client *client, uint8_t err);
};

/**
 * @brief Initialize Hearing Access Service Client.
 *
 * Register Hearing Access Service Client callbacks.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_init(const struct bt_has_client_cb *cb);

/**
 * @brief Bind service.
 *
 * Initiate service binding to remote device implementing HAS Server role.
 *
 * @param conn Bluetooth connection object.
 * @param[out] client Valid client instance on success.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_bind(struct bt_conn *conn, struct bt_has_client **client);

/**
 * @brief Unbind service.
 *
 * Unbind an service service or cancel pending service binding.
 * The
 *
 * @param client Client instance.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_unbind(struct bt_has_client *client);

/**
 * @brief Read Preset Records command.
 *
 * Client method to read up to @p max_count presets starting from given @p index.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 * The preset records are returned in the @ref bt_has_client_cb.preset_read_rsp callback
 * (called once for each preset).
 *
 * @param client Client instance.
 * @param index The index to start with.
 * @param max_count Maximum number of presets to read.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_presets_read(struct bt_has_client *client, uint8_t index, uint8_t max_count);

/**
 * @brief Set Active Preset command.
 *
 * Client procedure to set preset identified by @p index as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param index Preset index to activate.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_set(struct bt_has_client *client, uint8_t index, bool sync);

/**
 * @brief Activate Next Preset command.
 *
 * Client procedure to set next available preset as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_next(struct bt_has_client *client, bool sync);

/**
 * @brief Activate Previous Preset command.
 *
 * Client procedure to set previous available preset as active.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param sync Request active preset synchronization in set.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_prev(struct bt_has_client *client, bool sync);

/**
 * @brief Write Preset Name command.
 *
 * Client procedure to change the preset name.
 * The status is returned in the @ref bt_has_client_cb.cmd_status callback.
 *
 * @param client Client instance.
 * @param index Preset index to change the name of.
 * @param name Preset name to write.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_has_client_cmd_preset_write(struct bt_has_client *client, uint8_t index, const char *name);

/** Hearing Aid device Info Structure */
struct bt_has_client_info {
	/** Local identity */
	uint8_t id;
	/** Peer device address */
	const bt_addr_le_t *addr;
	/** Hearing Aid Type. */
	enum bt_has_hearing_aid_type type;
	/** Hearing Aid Features. */
	enum bt_has_features features;
	/** Hearing Aid Capabilities. */
	enum bt_has_capabilities caps;
	/** Active Preset Index.
	 *
	 *  Only applicable if @ref BT_HAS_PRESET_SUPPORT is set in @ref caps.
	 */
	uint8_t active_index;
};

/**
 * @brief Get information for the remote Hearing Aid device.
 *
 * @param client Client instance.
 * @param[out] info Info object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_has_client_info_get(const struct bt_has_client *client, struct bt_has_client_info *info);

/**
 * @brief Get the connection pointer of a HAS Client instance
 *
 * Get the Bluetooth connection pointer of a HAS Client instance.
 *
 * @param client Client instance.
 * @param[out] conn Connection pointer.
 *
 * @return 0 if success, errno on failure.
 */
int bt_has_client_conn_get(const struct bt_has_client *client, struct bt_conn **conn);
