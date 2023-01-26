/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_

/**
 * @brief Common Audio Profile (CAP)
 *
 * @defgroup bt_cap Common Audio Profile (CAP)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the Common Audio Service.
 *
 * This will register and enable the service and make it discoverable by
 * clients. This will also register a Coordinated Set Identification
 * Service instance.
 *
 * This shall only be done as a server, and requires
 * @kconfig{BT_CAP_ACCEPTOR_SET_MEMBER}. If @kconfig{BT_CAP_ACCEPTOR_SET_MEMBER}
 * is not enabled, the Common Audio Service will by statically registered.
 *
 * @param[in]  param     Coordinated Set Identification Service register
 *                       parameters.
 * @param[out] svc_inst  Pointer to the registered Coordinated Set
 *                       Identification Service.
 *
 * @return 0 if success, errno on failure.
 */
int bt_cap_acceptor_register(const struct bt_csip_set_member_register_param *param,
			     struct bt_csip_set_member_svc_inst **svc_inst);

/** Callback structure for CAP procedures */
struct bt_cap_initiator_cb {
#if defined(CONFIG_BT_AUDIO_UNICAST_CLIENT)
	/**
	 * @brief Callback for bt_cap_initiator_unicast_discover().
	 *
	 * @param conn      The connection pointer supplied to
	 *                  bt_cap_initiator_unicast_discover().
	 * @param err       0 if Common Audio Service was found else -ENODATA.
	 * @param csis_inst The Coordinated Set Identification Service if
	 *                  Common Audio Service was found and includes a
	 *                  Coordinated Set Identification Service.
	 *                  NULL on error or if remote device does not include
	 *                  Coordinated Set Identification Service.
	 */
	void (*unicast_discovery_complete)(
		struct bt_conn *conn, int err,
		const struct bt_csip_set_coordinator_csis_inst *csis_inst);

	/**
	 * @brief Callback for bt_cap_initiator_unicast_audio_start().
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_initiator_unicast_audio_start().
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_start_complete)(struct bt_audio_unicast_group *unicast_group,
				       int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_initiator_unicast_audio_update().
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_initiator_unicast_audio_update().
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_update_complete)(struct bt_audio_unicast_group *unicast_group,
					int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_initiator_unicast_audio_stop().
	 *
	 * @param unicast_group  The unicast group pointer supplied to
	 *                       bt_cap_initiator_unicast_audio_stop().
	 * @param err            0 if success, else BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code.
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0.
	 */
	void (*unicast_stop_complete)(struct bt_audio_unicast_group *unicast_group,
				      int err, struct bt_conn *conn);
#endif /* CONFIG_BT_AUDIO_UNICAST_CLIENT */
};

/**
 * @brief Discovers audio support on a remote device.
 *
 * This will discover the Common Audio Service (CAS) on the remote device, to
 * verify if the remote device supports the Common Audio Profile.
 *
 * @param conn Connection to a remote server.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_discover(struct bt_conn *conn);

/** Type of CAP set */
enum bt_cap_set_type {
	/** The set is an ad-hoc set */
	BT_CAP_SET_TYPE_AD_HOC,
	/** The set is a CSIP Coordinated Set */
	BT_CAP_SET_TYPE_CSIP,
};

/** Represents a Common Audio Set member that are either in a Coordinated or ad-hoc set */
union bt_cap_set_member {
	/** Connection pointer if the type is BT_CAP_SET_TYPE_AD_HOC. */
	struct bt_conn *member;

	/** CSIP Coordinated Set struct used if type is BT_CAP_SET_TYPE_CSIP. */
	struct bt_csip_set_coordinator_set_member *csip;
};

struct bt_cap_stream {
	struct bt_audio_stream bap_stream;
	struct bt_audio_stream_ops *ops;
};

/** @brief Register Audio operations for a Common Audio Profile stream.
 *
 *  Register Audio operations for a stream.
 *
 *  @param stream Stream object.
 *  @param ops    Stream operations structure.
 */
void bt_cap_stream_ops_register(struct bt_cap_stream *stream,
				struct bt_audio_stream_ops *ops);

struct bt_cap_unicast_audio_start_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The number of set members in @p members and number of streams in @p streams. */
	size_t count;

	/** Coordinated or ad-hoc set members. */
	union bt_cap_set_member **members;

	/** @brief Streams for the @p members
	 *
	 * stream[i] will be associated with members[i] if not already
	 * initialized, else the stream will be verified against the member.
	 */
	struct bt_cap_stream **streams;

	/**
	 * @brief Codec configuration.
	 *
	 * The @p codec.meta shall include a list of CCIDs
	 * (@ref BT_AUDIO_METADATA_TYPE_CCID_LIST) as well as a non-0
	 * stream context (@ref BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) bitfield.
	 */
	const struct bt_codec *codec;

	/** Quality of Service configuration. */
	const struct bt_codec_qos *qos;
};

/**
 * @brief Register Common Audio Profile callbacks
 *
 * @param cb   The callback structure. Shall remain static.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_register_cb(const struct bt_cap_initiator_cb *cb);

/**
 * @brief Setup and start unicast audio streams for a set of devices.
 *
 * The result of this operation is that the streams in @p param will be
 * initialized and will be usable for streaming audio data.
 * The @p unicast_group value can be used to update and stop the streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param          Parameters to start the audio streams.
 * @param[out] unicast_group  Pointer to the unicast group created.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param,
					 struct bt_audio_unicast_group **unicast_group);

/**
 * @brief Update unicast audio streams for a unicast group.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param unicast_group The group of unicast devices to update.
 * @param meta_count    The number of entries in @p meta.
 * @param meta          The new metadata. The metadata shall contain a list of
 *                      CCIDs as well as a non-0 context bitfield.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_update(struct bt_audio_unicast_group *unicast_group,
					  uint8_t meta_count,
					  const struct bt_codec_data *meta);

/**
 * @brief Stop unicast audio streams for a unicast group.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param unicast_group The group of unicast devices to stop. The audio streams
 *                      in this will be stopped and reset, and the
 *                      @p unicast_group will be invalidated.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_stop(struct bt_audio_unicast_group *unicast_group);

struct bt_cap_broadcast_audio_start_param {

	/** The number of streams in @p streams. */
	size_t count;

	/** Streams for broadcast source. */
	struct bt_cap_stream **streams;

	/**
	 * @brief Codec configuration.
	 *
	 * The @p codec.meta shall include a list of CCIDs as well as a non-0
	 * context bitfield.
	 */
	const struct bt_codec *codec;

	/** Quality of Service configuration. */
	const struct bt_codec_qos *qos;

	/**
	 * @brief Whether or not to encrypt the streams.
	 *
	 * If set to true, then the broadcast code in @p broadcast_code
	 * will be used to encrypt the streams.
	 */
	bool encrypt;

	/**
	 * @brief 16-octet broadcast code.
	 *
	 * Only valid if @p encrypt is true.
	 *
	 * If the value is a string or a the value is less than 16 octets,
	 * the remaining octets shall be 0.
	 *
	 * Example:
	 *   The string "Broadcast Code" shall be
	 *   [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
	 */
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
};

/**
 * @brief Start broadcast audio.
 *
 * Create a new audio broadcast source with one or more audio streams.
 *
 * The broadcast source will be visible for scanners once this has been called,
 * and the device will advertise audio announcements.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param   Parameters to start the audio streams.
 * @param[out] source  Pointer to the broadcast source started.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_start(const struct bt_cap_broadcast_audio_start_param *param,
					   struct bt_audio_broadcast_source **source);

/**
 * @brief Update broadcast audio streams for a broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to update.
 * @param meta_count       The number of entries in @p meta.
 * @param meta             The new metadata. The metadata shall contain a list
 *                         of CCIDs as well as a non-0 context bitfield.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_update(struct bt_audio_broadcast_source *broadcast_source,
					    uint8_t meta_count,
					    const struct bt_codec_data *meta);

/**
 * @brief Stop broadcast audio streams for a broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to stop. The audio streams
 *                         in this will be stopped and reset, and the
 *                         @p broadcast_source will be invalidated.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_stop(struct bt_audio_broadcast_source *broadcast_source);


struct bt_cap_unicast_to_broadcast_param {
	/** The source unicast group with the streams. */
	struct bt_audio_unicast_group *unicast_group;

	/**
	 * @brief Whether or not to encrypt the streams.
	 *
	 * If set to true, then the broadcast code in @p broadcast_code
	 * will be used to encrypt the streams.
	 */
	bool encrypt;

	/**
	 * @brief 16-octet broadcast code.
	 *
	 * Only valid if @p encrypt is true.
	 *
	 * If the value is a string or a the value is less than 16 octets,
	 * the remaining octets shall be 0.
	 *
	 * Example:
	 *   The string "Broadcast Code" shall be
	 *   [42 72 6F 61 64 63 61 73 74 20 43 6F 64 65 00 00]
	 */
	uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];
};

/**
 * @brief Hands over the data streams in a unicast group to a broadcast source.
 *
 * The streams in the unicast group will be stopped and the unicast group
 * will be deleted. This can only be done for source streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR},
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param param         The parameters for the handover.
 * @param source        The resulting broadcast source.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_to_broadcast(const struct bt_cap_unicast_to_broadcast_param *param,
					  struct bt_audio_broadcast_source **source);

struct bt_cap_broadcast_to_unicast_param {
	/**
	 * @brief The source broadcast source with the streams.
	 *
	 * The broadcast source will be stopped and deleted.
	 */
	struct bt_audio_broadcast_source *broadcast_source;

	/** The type of the set. */
	enum bt_cap_set_type type;

	/**
	 * @brief The number of set members in @p members.
	 *
	 * This value shall match the number of streams in the
	 * @p broadcast_source.
	 *
	 */
	size_t count;

	/** Coordinated or ad-hoc set members. */
	union bt_cap_set_member **members;
};

/**
 * @brief Hands over the data streams in a broadcast source to a unicast group.
 *
 * The streams in the broadcast source will be stopped and the broadcast source
 * will be deleted.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR},
 * @kconfig{CONFIG_BT_AUDIO_UNICAST_CLIENT} and
 * @kconfig{CONFIG_BT_AUDIO_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param          The parameters for the handover.
 * @param[out] unicast_group  The resulting broadcast source.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_audio_unicast_group **unicast_group);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_ */
