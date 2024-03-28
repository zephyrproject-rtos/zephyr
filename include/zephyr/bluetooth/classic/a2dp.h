/** @file
 * @brief Advanced Audio Distribution Profile header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_

#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avdtp.h>

#ifdef __cplusplus
extern "C" {
#endif

/** SBC IE length */
#define BT_A2DP_SBC_IE_LENGTH      (4u)
/** MPEG1,2 IE length */
#define BT_A2DP_MPEG_1_2_IE_LENGTH (4u)
/** MPEG2,4 IE length */
#define BT_A2DP_MPEG_2_4_IE_LENGTH (6u)

/** @brief define the audio endpoint
 *  @param _role BT_A2DP_SOURCE or BT_A2DP_SINK.
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 *  @param _config the default config to configure the peer same codec type endpoint.
 */
#define BT_A2DP_ENDPOINT_INIT(_role, _codec, _capability, _config)\
{\
	.codec_id = _codec,\
	.info = {.sep = {.media_type = BT_A2DP_AUDIO, .tsep = _role}, .next = NULL},\
	.config = (struct bt_a2dp_codec_ie *)(_config),\
	.capabilities = (struct bt_a2dp_codec_ie *)(_capability),\
	.control_cbs.configured = NULL,\
	.control_cbs.start_play = NULL,\
	.control_cbs.stop_play = NULL,\
	.control_cbs.sink_streamer_data = NULL\
}

/** @brief define the audio sink endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 */
#define BT_A2DP_SINK_ENDPOINT_INIT(_codec, _capability)\
BT_A2DP_ENDPOINT_INIT(BT_A2DP_SINK, _codec, _capability, NULL)

/** @brief define the audio source endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 *  @param _config the default config to configure the peer same codec type endpoint.
 */
#define BT_A2DP_SOURCE_ENDPOINT_INIT(_codec, _capability, _config)\
BT_A2DP_ENDPOINT_INIT(BT_A2DP_SOURCE, _codec, _capability, _config)

/** @brief define the default SBC sink endpoint that can be used as
 * bt_a2dp_register_endpoint's parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SINK_ENDPOINT is more convenient for
 * user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 */
#define BT_A2DP_SBC_SINK_ENDPOINT(_name)\
static uint8_t bt_a2dp_endpoint_cap_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000 | \
A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO | A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 | \
A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 18U, 35U};\
static struct bt_a2dp_endpoint _name = BT_A2DP_SINK_ENDPOINT_INIT(BT_A2DP_SBC,\
(&bt_a2dp_endpoint_cap_buffer##_name[0]))

/** @brief define the default SBC source endpoint that can be used as bt_a2dp_register_endpoint's
 * parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SOURCE_ENDPOINT is more convenient for
 * user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 *  @param _config_freq the frequency to configure the peer same codec type endpoint.
 */
#define BT_A2DP_SBC_SOURCE_ENDPOINT(_name, _config_freq)\
static uint8_t bt_a2dp_endpoint_cap_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, A2DP_SBC_SAMP_FREQ_44100 | \
A2DP_SBC_SAMP_FREQ_48000 | A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO | \
A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS,\
18U, 35U};\
static uint8_t bt_a2dp_endpoint_preset_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, _config_freq | A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 |\
A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 18U, 35U};\
static struct bt_a2dp_endpoint _name = BT_A2DP_SOURCE_ENDPOINT_INIT(BT_A2DP_SBC,\
&bt_a2dp_endpoint_cap_buffer##_name[0], &bt_a2dp_endpoint_preset_buffer##_name[0])

/** @brief Codec ID */
enum bt_a2dp_codec_id {
	/** Codec SBC */
	BT_A2DP_SBC = 0x00,
	/** Codec MPEG-1 */
	BT_A2DP_MPEG1 = 0x01,
	/** Codec MPEG-2 */
	BT_A2DP_MPEG2 = 0x02,
	/** Codec ATRAC */
	BT_A2DP_ATRAC = 0x04,
	/** Codec Non-A2DP */
	BT_A2DP_VENDOR = 0xff
};

/** @brief codec information elements for the endpoint */
struct bt_a2dp_codec_ie {
	/** Length of capabilities */
	uint8_t len;
	/** codec information element */
	uint8_t codec_ie[0];
};

/** @brief Stream End Point Media Type */
enum MEDIA_TYPE {
	/** Audio Media Type */
	BT_A2DP_AUDIO = 0x00,
	/** Video Media Type */
	BT_A2DP_VIDEO = 0x01,
	/** Multimedia Media Type */
	BT_A2DP_MULTIMEDIA = 0x02
};

/** @brief Stream End Point Role */
enum ROLE_TYPE {
	/** Source Role */
	BT_A2DP_SOURCE = 0,
	/** Sink Role */
	BT_A2DP_SINK = 1
};

/** @brief A2DP structure */
struct bt_a2dp;

struct bt_a2dp_endpoint;

/** @brief The endpoint configuration */
struct bt_a2dp_endpoint_config {
	/** The media configuration content */
	struct bt_a2dp_codec_ie *media_config;
};

/** @brief The configuration result */
struct bt_a2dp_endpoint_configure_result {
	/** 0 - success; other values - fail code */
	int err;
	/** The configuration content */
	struct bt_a2dp_endpoint_config config;
};

/** @brief Helper enum to be used as return value of bt_a2dp_discover_peer_endpoint_cb_t.
 *  The value informs the caller to perform further pending actions or stop them.
 */
enum {
	BT_A2DP_DISCOVER_ENDPOINT_STOP = 0,
	BT_A2DP_DISCOVER_ENDPOINT_CONTINUE,
};

/** @brief Get peer's endpoints callback */
typedef uint8_t (*bt_a2dp_discover_peer_endpoint_cb_t)(struct bt_a2dp *a2dp,
							struct bt_a2dp_endpoint *endpoint, int err);

/** @brief The callback that is controlled by peer */
struct bt_a2dp_control_cb {
	/** @brief a2dp is configured by peer.
	 *
	 *  @param err a2dp configuration result.
	 */
	void (*configured)(struct bt_a2dp_endpoint_configure_result *config);
	/** @brief a2dp is de-configured by peer.
	 *
	 *  @param err a2dp configuration result.
	 */
	void (*deconfigured)(int err);
	/** @brief The result of starting media streamer. */
	void (*start_play)(int err);
	/** @brief the result of stopping media streaming. */
	void (*stop_play)(int err);
	/** @brief the media streaming data, only for sink.
	 *
	 *  @param buf the data buf
	 *  @param seq_num the seqence number
	 *  @param ts the time stamp
	 */
	void (*sink_streamer_data)(struct net_buf *buf, uint16_t seq_num, uint32_t ts);
};

/** @brief The connecting callback */
struct bt_a2dp_connect_cb {
	/** @brief A a2dp connection has been established.
	 *
	 *  This callback notifies the application of a a2dp connection.
	 *  It means the AVDTP L2CAP connection.
	 *  In case the err parameter is non-zero it means that the
	 *  connection establishment failed.
	 *
	 *  @param a2dp a2dp connection object.
	 *  @param err error code.
	 */
	void (*connected)(struct bt_a2dp *a2dp, int err);
	/** @brief A a2dp connection has been disconnected.
	 *
	 *  This callback notifies the application that a a2dp connection
	 *  has been disconnected.
	 *
	 *  @param a2dp a2dp connection object.
	 */
	void (*disconnected)(struct bt_a2dp *a2dp);
};

/** @brief Stream End Point */
struct bt_a2dp_endpoint {
	/** Code ID */
	uint8_t codec_id;
	/** Stream End Point Information */
	struct bt_avdtp_seid_lsep info;
	/** Pointer to codec default config */
	struct bt_a2dp_codec_ie *config;
	/** Capabilities */
	struct bt_a2dp_codec_ie *capabilities;
	/** endpoint control callbacks */
	struct bt_a2dp_control_cb control_cbs;
};

/** @brief A2DP Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish A2DP
 *  connection between devices.
 *  This funciton only establish AVDTP L2CAP Signaling connection.
 *  After connection success, the callback that is registered by
 *  bt_a2dp_register_connect_callback is called.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return pointer to struct bt_a2dp in case of success or NULL in case
 *  of error.
 */
struct bt_a2dp *bt_a2dp_connect(struct bt_conn *conn);

/** @brief disconnect l2cap a2dp
 *
 * This function close AVDTP L2CAP Signaling connection.
 * It closes the AVDTP L2CAP Media connection too if it is established.
 *
 *  @param a2dp The a2dp instance.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_disconnect(struct bt_a2dp *a2dp);

/** @brief Endpoint Registration.
 *
 *  This function is used for registering the stream end points. The user has
 *  to take care of allocating the memory of the endpoint pointer and then pass the
 *  required arguments. Also, only one sep can be registered at a time.
 *  Multiple stream end points can be registered by calling multiple times.
 *  The endpoint registered first has a higher priority than the endpoint registered later.
 *  The priority is used in bt_a2dp_configure.
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param media_type Media type that the Endpoint is.
 *  @param role Role of Endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
				  uint8_t media_type, uint8_t role);

/** @brief register connecting callback.
 *
 *  The cb is called when bt_a2dp_connect is called or it is connected by peer device.
 *
 *  @param cb The callback function.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_register_connect_callback(struct bt_a2dp_connect_cb *cb);

/** @brief configure control callback.
 *
 *  This function will get peer's all endpoints and select one endpoint
 *  based on the priority of registered endpoints,
 *  then configure the endpoint based on the "config" of endpoint.
 *  Note: (1) priority is described in bt_a2dp_register_endpoint;
 *        (2) "config" is the config field of struct bt_a2dp_endpoint that is registered by
 *            bt_a2dp_register_endpoint.
 * This function sends AVDTP_DISCOVER, AVDTP_GET_CAPABILITIES, AVDTP_SET_CONFIGURATION and
 * AVDTP_OPEN commands. It establishes AVDTP L2CAP Media connection too.
 *
 *  @param a2dp The a2dp instance.
 *  @param result_cb The result callback function.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_configure(struct bt_a2dp *a2dp, void (*result_cb)(int err));

/** @brief get peer's endpoints.
 *
 *  bt_a2dp_configure can be called to configure a2dp.
 *  bt_a2dp_discover_peer_endpoints and bt_a2dp_configure_endpoint can be used too.
 *  In bt_a2dp_configure, the endpoint is selected automatically based on the
 *  prioriy. If bt_a2dp_configure fails, it means the default config of endpoint
 *  is not reasonal. bt_a2dp_discover_peer_endpoints and bt_a2dp_configure_endpoint
 *  can be used.
 *  bt_a2dp_discover_peer_endpoints is used to get peer endpoints. the peer endpoint
 *  is returned in the cb. then endpoint can be selected and configufed by
 *  bt_a2dp_configure_endpoint. If user stops to discover more peer endpoins, return
 *  BT_A2DP_DISCOVER_ENDPOINT_STOP in the cb; if user wants to discover more peer endpoints,
 *  return BT_A2DP_DISCOVER_ENDPOINT_CONTINUE in the cb.
 *
 *  @param a2dp The a2dp instance.
 *  @param cb notify the result.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_discover_peer_endpoints(struct bt_a2dp *a2dp, bt_a2dp_discover_peer_endpoint_cb_t cb);

/** @brief configure endpoint.
 *
 *  bt_a2dp_discover_peer_endpoints can be used to find peer's endpoints.
 *  This function to configure the selected endpoint that is found by
 *  bt_a2dp_discover_peer_endpoints.
 * This function sends AVDTP_SET_CONFIGURATION and
 * AVDTP_OPEN commands. It establishes AVDTP L2CAP Media connection too.
 *
 *  @param a2dp The a2dp instance.
 *  @param endpoint The configured endpoint that is registered.
 *  @param peer_endpoint The peer endpoint.
 *  @param config The config to configure the endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_configure_endpoint(struct bt_a2dp *a2dp, struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint *peer_endpoint,
		struct bt_a2dp_endpoint_config *config);

/** @brief revert the configuration, then it can be configured again.
 *
 *  Release the endpoint based on the endpoint's state.
 *  After this, the endpoint can be re-configured again.
 *
 * This function sends AVDTP_CLOSE command and close the AVDTP L2CAP Media connection.
 *
 *  @param endpoint the registered endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_deconfigure(struct bt_a2dp_endpoint *endpoint);

/** @brief start a2dp streamer, it is source only.
 *
 * This function sends the AVDTP_START command.
 *
 *  @param endpoint The endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_start(struct bt_a2dp_endpoint *endpoint);

/** @brief stop a2dp streamer, it is source only.
 *
 * This function sends the AVDTP_SUSPEND command.
 *
 *  @param endpoint The registered stream endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stop(struct bt_a2dp_endpoint *endpoint);

/** @brief re-configure a2dp streamer
 *
 * This function sends the AVDTP_RECONFIGURE command.
 *
 *  @param endpoint the endpoint.
 *  @param config The config to configure the endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_reconfigure(struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint_config *config);

/** @brief get the media l2cap mtu
 *
 *  @param endpoint the endpoint.
 *
 *  @return mtu value
 */
uint32_t bt_a2dp_get_media_mtu(struct bt_a2dp_endpoint *endpoint);

#if ((defined(CONFIG_BT_A2DP_SOURCE)) && (CONFIG_BT_A2DP_SOURCE > 0U))
/** @brief get net buf for sending media
 *
 *  @param pool If it is NULL, use default l2cap pool.
 *
 *  @return net buf in case of success and NULL in case of error.
 */
struct net_buf *bt_a2dp_media_buf_alloc(struct net_buf_pool *pool);

/** @brief send a2dp media data
 *
 *  @param endpoint The endpoint.
 *  @param buf  The data.
 *  @param seq_num The sequence number.
 *  @param ts The time stamp.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_media_send(struct bt_a2dp_endpoint *endpoint,  struct net_buf *buf,
			uint16_t seq_num, uint32_t ts);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_ */
