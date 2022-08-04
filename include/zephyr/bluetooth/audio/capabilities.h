/* @file
 * @brief Internal APIs for Audio Capabilities handling
 *
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_

#include <zephyr/bluetooth/audio/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get list of capabilities by type */
sys_slist_t *bt_audio_capability_get(enum bt_audio_dir dir);


/** @brief Audio Capability type */
enum bt_audio_capability_framing {
	BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED     = 0x00,
	BT_AUDIO_CAPABILITY_UNFRAMED_NOT_SUPPORTED = 0x01,
};

/** @def BT_AUDIO_CAPABILITY_PREF
 *  @brief Helper to declare elements of @ref bt_audio_capability_pref
 *
 *  @param _framing Framing Support
 *  @param _phy Preferred Target PHY
 *  @param _rtn Preferred Retransmission number
 *  @param _latency Preferred Maximum Transport Latency (msec)
 *  @param _pd_min Minimum Presentation Delay (usec)
 *  @param _pd_max Maximum Presentation Delay (usec)
 *  @param _pref_pd_min Preferred Minimum Presentation Delay (usec)
 *  @param _pref_pd_max Preferred Maximum Presentation Delay (usec)
 */
#define BT_AUDIO_CAPABILITY_PREF(_framing, _phy, _rtn, _latency, _pd_min, \
				 _pd_max, _pref_pd_min, _pref_pd_max) \
	{ \
		.framing = _framing, \
		.phy = _phy, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd_min = _pd_min, \
		.pd_max = _pd_max, \
		.pref_pd_min = _pref_pd_min, \
		.pref_pd_max = _pref_pd_max, \
	}

/** @brief Audio Capability Preference structure. */
struct bt_audio_capability_pref {
	/** @brief Framing support value
	 *
	 *  Unlike the other fields, this is not a preference but whether
	 *  the capability supports unframed ISOAL PDUs.
	 *
	 *  Possible values: BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED and
	 *  BT_AUDIO_CAPABILITY_UNFRAMED_NOT_SUPPORTED.
	 */
	uint8_t framing;

	/** Preferred PHY */
	uint8_t phy;

	/** Preferred Retransmission Number */
	uint8_t rtn;

	/** Preferred Transport Latency in ms */
	uint16_t latency;

	/** @brief Minimum Presentation Delay in us
	 *
	 *  Unlike the other fields, this is not a preference but a minimum
	 *  requirement.
	 */
	uint32_t pd_min;

	/** @brief Maximum Presentation Delay in us
	 *
	 *  Unlike the other fields, this is not a preference but a maximum
	 *  requirement.
	 */
	uint32_t pd_max;

	/** @brief Preferred minimum Presentation Delay in us*/
	uint32_t pref_pd_min;

	/** @brief Preferred maximum Presentation Delay	in us */
	uint32_t pref_pd_max;
};

struct bt_audio_capability; /* Handle circular dependency */

/** @brief Capability operations structure.
 *
 *  These are only used for unicast streams and broadcast sink streams.
 */
struct bt_audio_capability_ops {
	/** @brief Capability config callback
	 *
	 *  Config callback is called whenever a new Audio Stream needs to be
	 *  allocated.
	 *
	 *  @param conn Connection object
	 *  @param ep Remote Audio Endpoint being configured
	 *  @param dir Direction of the endpoint.
	 *  @param cap Local Audio Capability being configured
	 *  @param codec Codec configuration
	 *
	 *  @return Allocated Audio Stream object or NULL in case of error.
	 */
	struct bt_audio_stream *(*config)(struct bt_conn *conn,
					  struct bt_audio_ep *ep,
					  enum bt_audio_dir dir,
					  struct bt_audio_capability *cap,
					  struct bt_codec *codec);

	/** @brief Capability reconfig callback
	 *
	 *  Reconfig callback is called whenever an Audio Stream needs to be
	 *  reconfigured with different codec configuration.
	 *
	 *  @param stream Stream object being reconfigured.
	 *  @param cap Local Audio Capability being reconfigured
	 *  @param codec Codec configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig)(struct bt_audio_stream *stream,
			struct bt_audio_capability *cap,
			struct bt_codec *codec);

	/** @brief Capability QoS callback
	 *
	 *  QoS callback is called whenever an Audio Stream Quality of
	 *  Service needs to be configured.
	 *
	 *  @param stream Stream object being reconfigured.
	 *  @param QoS Quality of Service configuration
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*qos)(struct bt_audio_stream *stream, struct bt_codec_qos *qos);

	/** @brief Capability Enable callback
	 *
	 *  Enable callback is called whenever an Audio Stream is about to be
	 *  enabled.
	 *
	 *  @param stream Stream object being enabled.
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*enable)(struct bt_audio_stream *stream,
		      struct bt_codec_data *meta,
		      size_t meta_count);

	/** @brief Capability Start callback
	 *
	 *  Start callback is called whenever an Audio Stream is about to
	 *  start streaming.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*start)(struct bt_audio_stream *stream);

	/** @brief Capability Metadata callback
	 *
	 *  Metadata callback is called whenever an Audio Stream needs to
	 *  update its metadata.
	 *
	 *  @param stream Stream object.
	 *  @param meta_count Number of metadata entries
	 *  @param meta Metadata entries
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*metadata)(struct bt_audio_stream *stream,
			struct bt_codec_data *meta,
			size_t meta_count);

	/** @brief Capability Disable callback
	 *
	 *  Disable callback is called whenever an Audio Stream is about to be
	 *  disabled.
	 *
	 *  @param stream Stream object being disabled.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*disable)(struct bt_audio_stream *stream);

	/** @brief Capability Stop callback
	 *
	 *  Stop callback is called whenever an Audio Stream is about to
	 *  stop streaming.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*stop)(struct bt_audio_stream *stream);

	/** @brief Capability release callback
	 *
	 *  Release callback is called whenever a new Audio Stream needs to be
	 *  deallocated.
	 *
	 *  @param stream Stream object.
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*release)(struct bt_audio_stream *stream);
};

/** @brief Audio Capability structure.
 *
 *  Audio Capability represents a Local Codec including its preferrable
 *  Quality of service.
 *
 */
struct bt_audio_capability {
	/** Capability direction */
	enum bt_audio_dir dir;
	/** Capability codec reference */
	struct bt_codec *codec;
#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)
	/** Capability preferences */
	struct bt_audio_capability_pref pref;
	/** Capability operations reference */
	struct bt_audio_capability_ops *ops;
#endif /* CONFIG_BT_AUDIO_UNICAST_SERVER */

	/* Internally used list node */
	sys_snode_t _node;
};

/** @brief Register Audio Capability.
 *
 *  Register Audio Local Capability.
 *
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_register(struct bt_audio_capability *cap);

/** @brief Unregister Audio Capability.
 *
 *  Unregister Audio Local Capability.
 *
 *  @param cap Capability structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_audio_capability_unregister(struct bt_audio_capability *cap);

/** @brief Set the location for an endpoint type
 *
 * @param dir      Direction of the endpoints to change location for.
 * @param location The location to be set.
 *
 */
int bt_audio_capability_set_location(enum bt_audio_dir dir,
				     enum bt_audio_location location);

/** @brief Set the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to change available contexts for.
 * @param contexts The contexts to be set.
 */
int bt_audio_capability_set_available_contexts(enum bt_audio_dir dir,
					       enum bt_audio_context contexts);

/** @brief Get the available contexts for an endpoint type
 *
 * @param dir      Direction of the endpoints to get contexts for.
 *
 * @return Bitmask of available contexts.
 */
enum bt_audio_context bt_audio_capability_get_available_contexts(enum bt_audio_dir dir);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_ */
