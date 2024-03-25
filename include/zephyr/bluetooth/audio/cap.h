/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
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

#include <stdint.h>

#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Abstract Audio Broadcast Source structure. */
struct bt_cap_broadcast_source;

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
#if defined(CONFIG_BT_BAP_UNICAST_CLIENT)
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
	 * @param err            0 if success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_initiator_unicast_audio_cancel().
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_initiator_unicast_audio_cancel()
	 */
	void (*unicast_start_complete)(int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_initiator_unicast_audio_update().
	 *
	 * @param err            0 if success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_initiator_unicast_audio_cancel().
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_initiator_unicast_audio_cancel()
	 */
	void (*unicast_update_complete)(int err, struct bt_conn *conn);

	/**
	 * @brief Callback for bt_cap_initiator_unicast_audio_stop().
	 *
	 * @param err            0 if success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_initiator_unicast_audio_cancel().
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_initiator_unicast_audio_cancel()
	 */
	void (*unicast_stop_complete)(int err, struct bt_conn *conn);
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */
};

/**
 * @brief Discovers audio support on a remote device.
 *
 * This will discover the Common Audio Service (CAS) on the remote device, to
 * verify if the remote device supports the Common Audio Profile.
 *
 * @param conn Connection to a remote server.
 *
 * @retval 0 Success
 * @retval -EINVAL @p conn is NULL
 * @retval -ENOTCONN @p conn is not connected
 * @retval -ENOMEM Could not allocated memory for the request
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
	struct bt_csip_set_coordinator_csis_inst *csip;
};

struct bt_cap_stream {
	struct bt_bap_stream bap_stream;
	struct bt_bap_stream_ops *ops;
};

/** @brief Register Audio operations for a Common Audio Profile stream.
 *
 *  Register Audio operations for a stream.
 *
 *  @param stream Stream object.
 *  @param ops    Stream operations structure.
 */
void bt_cap_stream_ops_register(struct bt_cap_stream *stream, struct bt_bap_stream_ops *ops);

/**
 * @brief Send data to Common Audio Profile stream without timestamp
 *
 * See bt_bap_stream_send() for more information
 *
 * @note Support for sending must be supported, determined by @kconfig{CONFIG_BT_AUDIO_TX}.
 *
 * @param stream   Stream object.
 * @param buf      Buffer containing data to be sent.
 * @param seq_num  Packet Sequence number. This value shall be incremented for each call to this
 *                 function and at least once per SDU interval for a specific channel.
 *
 * @retval -EINVAL if stream object is NULL
 * @retval Any return value from bt_bap_stream_send()
 */
int bt_cap_stream_send(struct bt_cap_stream *stream, struct net_buf *buf, uint16_t seq_num);

/**
 * @brief Send data to Common Audio Profile stream with timestamp
 *
 * See bt_bap_stream_send() for more information
 *
 * @note Support for sending must be supported, determined by @kconfig{CONFIG_BT_AUDIO_TX}.
 *
 * @param stream   Stream object.
 * @param buf      Buffer containing data to be sent.
 * @param seq_num  Packet Sequence number. This value shall be incremented for each call to this
 *                 function and at least once per SDU interval for a specific channel.
 * @param ts       Timestamp of the SDU in microseconds (us). This value can be used to transmit
 *                 multiple SDUs in the same SDU interval in a CIG or BIG.
 *
 * @retval -EINVAL if stream object is NULL
 * @retval Any return value from bt_bap_stream_send()
 */
int bt_cap_stream_send_ts(struct bt_cap_stream *stream, struct net_buf *buf, uint16_t seq_num,
			  uint32_t ts);

/**
 * @brief Get ISO transmission timing info for a Common Audio Profile stream
 *
 * See bt_bap_stream_get_tx_sync() for more information
 *
 * @note Support for sending must be supported, determined by @kconfig{CONFIG_BT_AUDIO_TX}.
 *
 * @param[in]  stream Stream object.
 * @param[out] info   Transmit info object.
 *
 * @retval -EINVAL if stream object is NULL
 * @retval Any return value from bt_bap_stream_get_tx_sync()
 */
int bt_cap_stream_get_tx_sync(struct bt_cap_stream *stream, struct bt_iso_tx_info *info);

/** Stream specific parameters for the bt_cap_initiator_unicast_audio_start() function */
struct bt_cap_unicast_audio_start_stream_param {
	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member member;

	/** @brief Stream for the @p member */
	struct bt_cap_stream *stream;

	/** Endpoint reference for the @p stream */
	struct bt_bap_ep *ep;

	/**
	 * @brief Codec configuration.
	 *
	 * The @p codec_cfg.meta shall include a list of CCIDs
	 * (@ref BT_AUDIO_METADATA_TYPE_CCID_LIST) as well as a non-0
	 * stream context (@ref BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) bitfield.
	 *
	 * This value is assigned to the @p stream, and shall remain valid while the stream is
	 * non-idle.
	 */
	struct bt_audio_codec_cfg *codec_cfg;
};

/** Parameters for the bt_cap_initiator_unicast_audio_start() function */
struct bt_cap_unicast_audio_start_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The number of parameters in @p stream_params */
	size_t count;

	/** Array of stream parameters */
	struct bt_cap_unicast_audio_start_stream_param *stream_params;
};

/** Stream specific parameters for the bt_cap_initiator_unicast_audio_update() function */
struct bt_cap_unicast_audio_update_stream_param {
	/** Stream to update */
	struct bt_cap_stream *stream;

	/** The length of @p meta. */
	size_t meta_len;

	/** @brief The new metadata.
	 *
	 * The metadata shall a list of CCIDs as
	 * well as a non-0 context bitfield.
	 */
	uint8_t *meta;
};

/** Parameters for the bt_cap_initiator_unicast_audio_update() function */
struct bt_cap_unicast_audio_update_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The number of parameters in @p stream_params */
	size_t count;

	/** Array of stream parameters */
	struct bt_cap_unicast_audio_update_stream_param *stream_params;
};

/** Parameters for the bt_cap_initiator_unicast_audio_stop() function */
struct bt_cap_unicast_audio_stop_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The number of streams in @p streams */
	size_t count;

	/** Array of streams to stop */
	struct bt_cap_stream **streams;
};

/**
 * @brief Register Common Audio Profile Initiator callbacks
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
 * @kconfig{CONFIG_BT_BAP_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param param Parameters to start the audio streams.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_start(const struct bt_cap_unicast_audio_start_param *param);

/**
 * @brief Update unicast audio streams.
 *
 * This will update the metadata of one or more streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param param Update parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_update(const struct bt_cap_unicast_audio_update_param *param);

/**
 * @brief Stop unicast audio streams.
 *
 * This will stop one or more streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_UNICAST_CLIENT} must be enabled for this function
 * to be enabled.
 *
 * @param param Stop parameters.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_audio_stop(const struct bt_cap_unicast_audio_stop_param *param);

/** @brief Cancel any current Common Audio Profile procedure
 *
 * This will stop the current procedure from continuing and making it possible to run a new
 * Common Audio Profile procedure.
 *
 * It is recommended to do this if any existing procedure takes longer time than expected, which
 * could indicate a missing response from the Common Audio Profile Acceptor.
 *
 * This does not send any requests to any Common Audio Profile Acceptors involved with the current
 * procedure, and thus notifications from the Common Audio Profile Acceptors may arrive after this
 * has been called. It is thus recommended to either only use this if a procedure has stalled, or
 * wait a short while before starting any new Common Audio Profile procedure after this has been
 * called to avoid getting notifications from the cancelled procedure. The wait time depends on
 * the connection interval, the number of devices in the previous procedure and the behavior of the
 * Common Audio Profile Acceptors.
 *
 * The respective callbacks of the procedure will be called as part of this with the connection
 * pointer set to 0 and the err value set to -ECANCELED.
 *
 * @retval 0 on success
 * @retval -EALREADY if no procedure is active
 */
int bt_cap_initiator_unicast_audio_cancel(void);

struct bt_cap_initiator_broadcast_stream_param {
	/** Audio stream */
	struct bt_cap_stream *stream;

	/** The length of the %p data array.
	 *
	 * The BIS specific data may be omitted and this set to 0.
	 */
	size_t data_len;

	/** BIS Codec Specific Configuration */
	uint8_t *data;
};

struct bt_cap_initiator_broadcast_subgroup_param {
	/** The number of parameters in @p stream_params */
	size_t stream_count;

	/** Array of stream parameters */
	struct bt_cap_initiator_broadcast_stream_param *stream_params;

	/** Subgroup Codec configuration. */
	struct bt_audio_codec_cfg *codec_cfg;
};

struct bt_cap_initiator_broadcast_create_param {
	/** The number of parameters in @p subgroup_params */
	size_t subgroup_count;

	/** Array of stream parameters */
	struct bt_cap_initiator_broadcast_subgroup_param *subgroup_params;

	/** Quality of Service configuration. */
	struct bt_audio_codec_qos *qos;

	/** @brief Broadcast Source packing mode.
	 *
	 *  @ref BT_ISO_PACKING_SEQUENTIAL or @ref BT_ISO_PACKING_INTERLEAVED.
	 *
	 *  @note This is a recommendation to the controller, which the
	 *  controller may ignore.
	 */
	uint8_t packing;

	/** Whether or not to encrypt the streams. */
	bool encryption;

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
	uint8_t broadcast_code[BT_AUDIO_BROADCAST_CODE_SIZE];

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	/** @brief Immediate Repetition Count
	 *
	 *  The number of times the scheduled payloads are transmitted in a
	 *  given event.
	 *
	 *  Value range from @ref BT_ISO_MIN_IRC to @ref BT_ISO_MAX_IRC.
	 */
	uint8_t irc;

	/** @brief Pre-transmission offset
	 *
	 *  Offset used for pre-transmissions.
	 *
	 *  Value range from @ref BT_ISO_MIN_PTO to @ref BT_ISO_MAX_PTO.
	 */
	uint8_t pto;

	/** @brief ISO interval
	 *
	 *  Time between consecutive BIS anchor points.
	 *
	 *  Value range from @ref BT_ISO_ISO_INTERVAL_MIN to
	 *  @ref BT_ISO_ISO_INTERVAL_MAX.
	 */
	uint16_t iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

/**
 * @brief Create a Common Audio Profile broadcast source.
 *
 * Create a new audio broadcast source with one or more audio streams.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param             Parameters to start the audio streams.
 * @param[out] broadcast_source  Pointer to the broadcast source created.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_create(
	const struct bt_cap_initiator_broadcast_create_param *param,
	struct bt_cap_broadcast_source **broadcast_source);

/**
 * @brief Start Common Audio Profile broadcast source.
 *
 * The broadcast source will be visible for scanners once this has been called,
 * and the device will advertise audio announcements.
 *
 * This will allow the streams in the broadcast source to send audio by calling
 * bt_bap_stream_send().
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source  Pointer to the broadcast source.
 * @param adv               Pointer to an extended advertising set with
 *                          periodic advertising configured.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_start(struct bt_cap_broadcast_source *broadcast_source,
					   struct bt_le_ext_adv *adv);
/**
 * @brief Update broadcast audio streams for a Common Audio Profile broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to update.
 * @param meta             The new metadata. The metadata shall contain a list
 *                         of CCIDs as well as a non-0 context bitfield.
 * @param meta_len         The length of @p meta.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_update(struct bt_cap_broadcast_source *broadcast_source,
					    const uint8_t meta[], size_t meta_len);

/**
 * @brief Stop broadcast audio streams for a Common Audio Profile broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to stop. The audio streams
 *                         in this will be stopped and reset.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_stop(struct bt_cap_broadcast_source *broadcast_source);

/**
 * @brief Delete Common Audio Profile broadcast source
 *
 * This can only be done after the broadcast source has been stopped by calling
 * bt_cap_initiator_broadcast_audio_stop() and after the
 * bt_bap_stream_ops.stopped() callback has been called for all streams in the
 * broadcast source.
 *
 * @note @kconfig{CONFIG_BT_CAP_INITIATOR} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param broadcast_source The broadcast source to delete.
 *                         The @p broadcast_source will be invalidated.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_audio_delete(struct bt_cap_broadcast_source *broadcast_source);

/**
 * @brief Get the broadcast ID of a Common Audio Profile broadcast source
 *
 * This will return the 3-octet broadcast ID that should be advertised in the
 * extended advertising data with @ref BT_UUID_BROADCAST_AUDIO_VAL as
 * @ref BT_DATA_SVC_DATA16.
 *
 * See table 3.14 in the Basic Audio Profile v1.0.1 for the structure.
 *
 * @param[in]  broadcast_source  Pointer to the broadcast source.
 * @param[out] broadcast_id      Pointer to the 3-octet broadcast ID.
 *
 * @return int		0 if on success, errno on error.
 */
int bt_cap_initiator_broadcast_get_id(const struct bt_cap_broadcast_source *broadcast_source,
				      uint32_t *const broadcast_id);

/**
 * @brief Get the Broadcast Audio Stream Endpoint of a Common Audio Profile broadcast source
 *
 * This will encode the BASE of a broadcast source into a buffer, that can be
 * used for advertisement. The encoded BASE will thus be encoded as
 * little-endian. The BASE shall be put into the periodic advertising data
 * (see bt_le_per_adv_set_data()).
 *
 * See table 3.15 in the Basic Audio Profile v1.0.1 for the structure.
 *
 * @param broadcast_source  Pointer to the broadcast source.
 * @param base_buf          Pointer to a buffer where the BASE will be inserted.
 *
 * @return int		0 if on success, errno on error.
 */
int bt_cap_initiator_broadcast_get_base(struct bt_cap_broadcast_source *broadcast_source,
					struct net_buf_simple *base_buf);

struct bt_cap_unicast_to_broadcast_param {
	/** The source unicast group with the streams. */
	struct bt_bap_unicast_group *unicast_group;

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
 * @kconfig{CONFIG_BT_BAP_UNICAST_CLIENT} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param param         The parameters for the handover.
 * @param source        The resulting broadcast source.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_unicast_to_broadcast(const struct bt_cap_unicast_to_broadcast_param *param,
					  struct bt_cap_broadcast_source **source);

struct bt_cap_broadcast_to_unicast_param {
	/**
	 * @brief The source broadcast source with the streams.
	 *
	 * The broadcast source will be stopped and deleted.
	 */
	struct bt_cap_broadcast_source *broadcast_source;

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
 * @kconfig{CONFIG_BT_BAP_UNICAST_CLIENT} and
 * @kconfig{CONFIG_BT_BAP_BROADCAST_SOURCE} must be enabled for this function
 * to be enabled.
 *
 * @param[in]  param          The parameters for the handover.
 * @param[out] unicast_group  The resulting broadcast source.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_initiator_broadcast_to_unicast(const struct bt_cap_broadcast_to_unicast_param *param,
					  struct bt_bap_unicast_group **unicast_group);

/** Callback structure for CAP procedures */
struct bt_cap_commander_cb {
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
	void (*discovery_complete)(struct bt_conn *conn, int err,
				   const struct bt_csip_set_coordinator_csis_inst *csis_inst);

#if defined(CONFIG_BT_VCP_VOL_CTLR)
	/**
	 * @brief Callback for bt_cap_commander_change_volume().
	 *
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_commander_cancel()
	 * @param err            0 on success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_commander_cancel().
	 */
	void (*volume_changed)(struct bt_conn *conn, int err);

	/**
	 * @brief Callback for bt_cap_commander_change_volume_mute_state().
	 *
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_commander_cancel()
	 * @param err            0 on success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_commander_cancel().
	 */
	void (*volume_mute_changed)(struct bt_conn *conn, int err);

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	/**
	 * @brief Callback for bt_cap_commander_change_volume_offset().
	 *
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_commander_cancel()
	 * @param err            0 on success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_commander_cancel().
	 */
	void (*volume_offset_changed)(struct bt_conn *conn, int err);
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	/**
	 * @brief Callback for bt_cap_commander_change_microphone_gain_setting().
	 *
	 * @param conn           Pointer to the connection where the error
	 *                       occurred. NULL if @p err is 0 or if cancelled by
	 *                       bt_cap_commander_cancel()
	 * @param err            0 on success, BT_GATT_ERR() with a
	 *                       specific ATT (BT_ATT_ERR_*) error code or -ECANCELED if cancelled
	 *                       by bt_cap_commander_cancel().
	 */
	void (*microphone_gain_changed)(struct bt_conn *conn, int err);
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
};

/**
 * @brief Register Common Audio Profile Commander callbacks
 *
 * @param cb   The callback structure. Shall remain static.
 *
 * @retval 0 Success
 * @retval -EINVAL @p cb is NULL
 * @retval -EALREADY Callbacks are already registered
 */
int bt_cap_commander_register_cb(const struct bt_cap_commander_cb *cb);

/**
 * @brief Unregister Common Audio Profile Commander callbacks
 *
 * @param cb   The callback structure that was previously registered.
 *
 * @retval 0 Success
 * @retval -EINVAL @p cb is NULL or @p cb was not registered
 */
int bt_cap_commander_unregister_cb(const struct bt_cap_commander_cb *cb);

/**
 * @brief Discovers audio support on a remote device.
 *
 * This will discover the Common Audio Service (CAS) on the remote device, to
 * verify if the remote device supports the Common Audio Profile.
 *
 * @note @kconfig{CONFIG_BT_CAP_COMMANDER} must be enabled for this function. If
 * @kconfig{CONFIG_BT_CAP_INITIATOR} is also enabled, it does not matter if
 * bt_cap_commander_discover() or bt_cap_initiator_unicast_discover() is used.
 *
 * @param conn Connection to a remote server.
 *
 * @retval 0 Success
 * @retval -EINVAL @p conn is NULL
 * @retval -ENOTCONN @p conn is not connected
 * @retval -ENOMEM Could not allocated memory for the request
 * @retval -EBUSY Already doing discovery for @p conn
 */
int bt_cap_commander_discover(struct bt_conn *conn);

/** @brief Cancel any current Common Audio Profile commander procedure
 *
 * This will stop the current procedure from continuing and making it possible to run a new
 * Common Audio Profile procedure.
 *
 * It is recommended to do this if any existing procedure takes longer time than expected, which
 * could indicate a missing response from the Common Audio Profile Acceptor.
 *
 * This does not send any requests to any Common Audio Profile Acceptors involved with the
 * current procedure, and thus notifications from the Common Audio Profile Acceptors may
 * arrive after this has been called. It is thus recommended to either only use this if a
 * procedure has stalled, or wait a short while before starting any new Common Audio Profile
 * procedure after this has been called to avoid getting notifications from the cancelled
 * procedure. The wait time depends on the connection interval, the number of devices in the
 * previous procedure and the behavior of the Common Audio Profile Acceptors.
 *
 * The respective callbacks of the procedure will be called as part of this with the connection
 * pointer set to NULL and the err value set to -ECANCELED.
 *
 * @retval 0 on success
 * @retval -EALREADY if no procedure is active
 */
int bt_cap_commander_cancel(void);

struct bt_cap_commander_broadcast_reception_start_member_param {
	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member member;

	/** Address of the advertiser. */
	bt_addr_le_t addr;

	/** SID of the advertising set. */
	uint8_t adv_sid;

	/**
	 * @brief Periodic advertising interval in milliseconds.
	 *
	 * BT_BAP_PA_INTERVAL_UNKNOWN if unknown.
	 */
	uint16_t pa_interval;

	/** 24-bit broadcast ID */
	uint32_t broadcast_id;

	/**
	 * @brief Pointer to array of subgroups
	 *
	 * At least one bit in one of the subgroups bis_sync parameters shall be set.
	 */
	struct bt_bap_bass_subgroup *subgroups;

	/** Number of subgroups */
	size_t num_subgroups;
};

/** Parameters for starting broadcast reception  */
struct bt_cap_commander_broadcast_reception_start_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The set of devices for this procedure */
	struct bt_cap_commander_broadcast_reception_start_member_param *param;

	/** The number of parameters in @p param */
	size_t count;
};

/**
 * @brief Starts the reception of broadcast audio on one or more remote Common Audio Profile
 * Acceptors
 *
 * @param param The parameters to start the broadcast audio
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_broadcast_reception_start(
	const struct bt_cap_commander_broadcast_reception_start_param *param);

/** Parameters for stopping broadcast reception  */
struct bt_cap_commander_broadcast_reception_stop_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member *members;

	/** The number of members in @p members */
	size_t count;
};

/**
 * @brief Stops the reception of broadcast audio on one or more remote Common Audio Profile
 * Acceptors
 *
 * @param param The parameters to stop the broadcast audio
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_broadcast_reception_stop(
	const struct bt_cap_commander_broadcast_reception_stop_param *param);

/** Parameters for changing absolute volume  */
struct bt_cap_commander_change_volume_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member *members;

	/** The number of members in @p members */
	size_t count;

	/** The absolute volume to set */
	uint8_t volume;
};

/**
 * @brief Change the volume on one or more Common Audio Profile Acceptors
 *
 * @param param The parameters for the volume change
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_change_volume(const struct bt_cap_commander_change_volume_param *param);

struct bt_cap_commander_change_volume_offset_member_param {
	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member member;

	/**
	 * @brief  The offset to set
	 *
	 * Value shall be between @ref BT_VOCS_MIN_OFFSET and @ref BT_VOCS_MAX_OFFSET
	 */
	int16_t offset;
};

/** Parameters for changing volume offset */
struct bt_cap_commander_change_volume_offset_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The set of devices for this procedure */
	struct bt_cap_commander_change_volume_offset_member_param *param;

	/** The number of parameters in @p param */
	size_t count;
};

/**
 * @brief Change the volume offset on one or more Common Audio Profile Acceptors
 *
 * @param param The parameters for the volume offset change
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_change_volume_offset(
	const struct bt_cap_commander_change_volume_offset_param *param);

/** Parameters for changing volume mute state */
struct bt_cap_commander_change_volume_mute_state_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member *members;

	/** The number of members in @p members */
	size_t count;

	/**
	 * @brief The volume mute state to set
	 *
	 * true to mute, and false to unmute
	 */
	bool mute;
};

/**
 * @brief Change the volume mute state on one or more Common Audio Profile Acceptors
 *
 * @param param The parameters for the volume mute state change
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_change_volume_mute_state(
	const struct bt_cap_commander_change_volume_mute_state_param *param);

/** Parameters for changing microphone mute state */
struct bt_cap_commander_change_microphone_mute_state_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member *members;

	/** The number of members in @p members */
	size_t count;

	/**
	 * @brief The microphone mute state to set
	 *
	 * true to mute, and false to unmute
	 */
	bool mute;
};

/**
 * @brief Change the microphone mute state on one or more Common Audio Profile Acceptors
 *
 * @param param The parameters for the microphone mute state change
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_change_microphone_mute_state(
	const struct bt_cap_commander_change_microphone_mute_state_param *param);

struct bt_cap_commander_change_microphone_gain_setting_member_param {
	/** Coordinated or ad-hoc set member. */
	union bt_cap_set_member member;

	/** @brief The microphone gain setting to set */
	int8_t gain;
};

/** Parameters for changing microphone mute state */
struct bt_cap_commander_change_microphone_gain_setting_param {
	/** The type of the set. */
	enum bt_cap_set_type type;

	/** The set of devices for this procedure */
	struct bt_cap_commander_change_microphone_gain_setting_member_param *param;

	/** The number of parameters in @p param */
	size_t count;
};

/**
 * @brief Change the microphone gain setting on one or more Common Audio Profile Acceptors
 *
 * @param param The parameters for the microphone gain setting change
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_cap_commander_change_microphone_gain_setting(
	const struct bt_cap_commander_change_microphone_gain_setting_param *param);
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAP_H_ */
