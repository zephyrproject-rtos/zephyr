/** @file
 * @brief Advance Audio Distribution Profile header.
 */

/*
 * Copyright (c) 2021 NXP
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_

#include <bluetooth/avdtp.h>

#ifdef __cplusplus
extern "C" {
#endif

/** SBC IE length */
#define BT_A2DP_SBC_IE_LENGTH      (4u)
/** MPEG1,2 IE length */
#define BT_A2DP_MPEG_1_2_IE_LENGTH (4u)
/** MPEG2,4 IE length */
#define BT_A2DP_MPEG_2_4_IE_LENGTH (6u)

#define BT_A2DP_SOURCE_SBC_CODEC_BUFFER_SIZE (2264U + CONFIG_BT_A2DP_SBC_ENCODER_PCM_BUFFER_SIZE)
#define BT_A2DP_SOURCE_SBC_CODEC_BUFFER_NOCACHED_SIZE (0U)
#define BT_A2DP_SINK_SBC_CODEC_BUFFER_SIZE (2600U)
#define BT_A2DP_SINK_SBC_CODEC_BUFFER_NOCACHED_SIZE (CONFIG_BT_A2DP_SBC_DECODER_PCM_BUFFER_SIZE)

#if ((defined(CONFIG_BT_A2DP_CP_SERVICE)) && (CONFIG_BT_A2DP_CP_SERVICE > 0U))
#define BT_A2DP_EP_CONTENT_PROTECTION_INIT \
.cp_ie = NULL,.cp_config = NULL,.cp_ie_count = 0,
#else
#define BT_A2DP_EP_CONTENT_PROTECTION_INIT
#endif

#if ((defined(CONFIG_BT_A2DP_RECOVERY_SERVICE)) && (CONFIG_BT_A2DP_RECOVERY_SERVICE > 0U))
#define BT_A2DP_EP_RECOVERY_SERVICE_INIT \
.recovery_ie = NULL,.recovery_config = NULL,
#else
#define BT_A2DP_EP_RECOVERY_SERVICE_INIT
#endif

#if ((defined(CONFIG_BT_A2DP_REPORTING_SERVICE)) && (CONFIG_BT_A2DP_REPORTING_SERVICE > 0U))
#define BT_A2DP_EP_REPORTING_SERVICE_INIT \
.reporting_service_enable = 0,
#else
#define BT_A2DP_EP_REPORTING_SERVICE_INIT
#endif

#if ((defined(CONFIG_BT_A2DP_DR_SERVICE)) && (CONFIG_BT_A2DP_DR_SERVICE > 0U))
#define BT_A2DP_EP_DELAY_REPORTING_INIT \
.delay_reporting_service_enable = 0, .control_cbs.sink_delay_report_cb = NULL,
#else
#define BT_A2DP_EP_DELAY_REPORTING_INIT
#endif

#if ((defined(CONFIG_BT_A2DP_HC_SERVICE)) && (CONFIG_BT_A2DP_HC_SERVICE > 0U))
#define BT_A2DP_EP_HEADER_COMPRESSION_INIT \
.header_compression_cap = 0,.header_compression_config = 0,
#else
#define BT_A2DP_EP_HEADER_COMPRESSION_INIT
#endif

#if ((defined(CONFIG_BT_A2DP_MULTIPLEXING_SERVICE)) && (CONFIG_BT_A2DP_MULTIPLEXING_SERVICE > 0U))
#define BT_A2DP_EP_MULTIPLEXING_INIT \
.header_compression_cap = 0,.header_compression_config = 0,
#else
#define BT_A2DP_EP_MULTIPLEXING_INIT
#endif

/** @brief define the audio endpoint
 *  @param _role BT_A2DP_SOURCE or BT_A2DP_SINK.
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 *  @param config the default config to configure the peer same codec type endpoint.
 *  @param _codec_buffer the codec function used buffer.
 *  @param _codec_buffer_nocahced the codec function used nocached buffer.
 */
#define BT_A2DP_ENDPOINT_INIT(_role, _codec, _capability, _config,\
	_codec_buffer, _codec_buffer_nocahced)\
{\
	.codec_id = _codec,\
	.info = {.sep = {.media_type = BT_A2DP_AUDIO, .tsep = _role}, .next = NULL},\
	.config = (struct bt_a2dp_codec_ie *)(_config),\
	.capabilities = (struct bt_a2dp_codec_ie *)(_capability),\
	.control_cbs.configured = NULL,\
	.control_cbs.start_play = NULL,\
	.control_cbs.stop_play = NULL,\
	.control_cbs.sink_streamer_data = NULL,\
	.codec_buffer = (uint8_t *)(_codec_buffer),\
	.codec_buffer_nocached = (uint8_t *)(_codec_buffer_nocahced),\
	BT_A2DP_EP_CONTENT_PROTECTION_INIT\
	BT_A2DP_EP_RECOVERY_SERVICE_INIT\
	BT_A2DP_EP_REPORTING_SERVICE_INIT\
	BT_A2DP_EP_DELAY_REPORTING_INIT\
	BT_A2DP_EP_HEADER_COMPRESSION_INIT\
	BT_A2DP_EP_MULTIPLEXING_INIT\
}

/** @brief define the audio sink endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 *  @param _codec_buffer the codec function used buffer.
 *  @param _codec_buffer_nocahced the codec function used nocached buffer.
 */
#define BT_A2DP_SINK_ENDPOINT_INIT(_codec, _capability,\
		_codec_buffer, _codec_buffer_nocahced)\
BT_A2DP_ENDPOINT_INIT(BT_A2DP_SINK, _codec,\
		_capability, NULL, _codec_buffer, _codec_buffer_nocahced)

/** @brief define the audio source endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 *  @param _config the default config to configure the peer same codec type endpoint.
 *  @param _codec_buffer the codec function used buffer.
 *  @param _codec_buffer_nocahced the codec function used nocached buffer.
 */
#define BT_A2DP_SOURCE_ENDPOINT_INIT(_codec, _capability,\
		_config, _codec_buffer, _codec_buffer_nocahced)\
BT_A2DP_ENDPOINT_INIT(BT_A2DP_SOURCE, _codec, _capability,\
		_config, _codec_buffer, _codec_buffer_nocahced)

/** @brief define the default SBC sink endpoint that can be used as
 * bt_a2dp_register_endpoint's parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SINK_ENDPOINT is more convenient for
 * user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 */
#define BT_A2DP_SBC_SINK_ENDPOINT(_name)\
static uint32_t \
bt_a2dp_endpoint_codec_buffer##_name\
[(BT_A2DP_SINK_SBC_CODEC_BUFFER_SIZE + 3) / 4];\
static __nocache uint32_t bt_a2dp_endpoint_codec_buffer_nocached##_name\
[(BT_A2DP_SINK_SBC_CODEC_BUFFER_NOCACHED_SIZE + 3) / 4];\
static uint8_t bt_a2dp_endpoint_cap_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000 | A2DP_SBC_CH_MODE_MONO | \
A2DP_SBC_CH_MODE_STREO | A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 | \
A2DP_SBC_ALLOC_MTHD_LOUDNESS, 18U, 35U};\
static struct bt_a2dp_endpoint _name = BT_A2DP_SINK_ENDPOINT_INIT(BT_A2DP_SBC,\
(&bt_a2dp_endpoint_cap_buffer##_name[0]),\
&bt_a2dp_endpoint_codec_buffer##_name[0], \
&bt_a2dp_endpoint_codec_buffer_nocached##_name[0])

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
uint32_t bt_a2dp_endpoint_codec_buffer##_name[(BT_A2DP_SOURCE_SBC_CODEC_BUFFER_SIZE + 3) / 4];\
static uint8_t bt_a2dp_endpoint_cap_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, A2DP_SBC_SAMP_FREQ_44100 | \
A2DP_SBC_SAMP_FREQ_48000 | A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO | \
A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS,\
18U, 35U};\
static uint8_t bt_a2dp_endpoint_preset_buffer##_name[BT_A2DP_SBC_IE_LENGTH + 1] =\
{BT_A2DP_SBC_IE_LENGTH, _config_freq | A2DP_SBC_CH_MODE_JOINT, A2DP_SBC_BLK_LEN_16 |\
A2DP_SBC_SUBBAND_8 | A2DP_SBC_ALLOC_MTHD_LOUDNESS, 18U, 35U};\
static struct bt_a2dp_endpoint _name = BT_A2DP_SOURCE_ENDPOINT_INIT(BT_A2DP_SBC,\
&bt_a2dp_endpoint_cap_buffer##_name[0], &bt_a2dp_endpoint_preset_buffer##_name[0],\
&bt_a2dp_endpoint_codec_buffer##_name[0], NULL)

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

#if ((defined(CONFIG_BT_A2DP_CP_SERVICE)) && (CONFIG_BT_A2DP_CP_SERVICE > 0U))
	/** Pointer to content protection config */
	struct bt_a2dp_codec_ie *cp_config;
#endif
#if ((defined(CONFIG_BT_A2DP_RECOVERY_SERVICE)) && (CONFIG_BT_A2DP_RECOVERY_SERVICE > 0U))
	/** Pointer to recovery service default config */
	struct bt_a2dp_codec_ie *recovery_config; 
#endif
#if ((defined(CONFIG_BT_A2DP_REPORTING_SERVICE)) && (CONFIG_BT_A2DP_REPORTING_SERVICE > 0U))
	/** reporting service configure flag */
	uint8_t reporting_service_config;
#endif
#if ((defined(CONFIG_BT_A2DP_DR_SERVICE)) && (CONFIG_BT_A2DP_DR_SERVICE > 0U))
	/** delay reporting service configure flag */
	uint8_t delay_reporting_service_config;
#endif
#if ((defined(CONFIG_BT_A2DP_HC_SERVICE)) && (CONFIG_BT_A2DP_HC_SERVICE > 0U))
	/** header compression config,
	 *  0x00 means the endpoint doesn't configure defaultly.
	 */
	uint8_t header_compression_config;
#endif
#if ((defined(CONFIG_BT_A2DP_MULTIPLEXING_SERVICE)) && (CONFIG_BT_A2DP_MULTIPLEXING_SERVICE > 0U))
	/** multiplexing service configure flag */
	uint8_t multiplexing_service_config;
#endif
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
	 *  @param data the data buffer pointer.
	 *  @param length the data length.
	 */
	void (*sink_streamer_data)(uint8_t *data, uint32_t length);
#if ((defined(CONFIG_BT_A2DP_DR_SERVICE)) && (CONFIG_BT_A2DP_DR_SERVICE > 0U))
	union
	{
		/** @brief the delay report callback
		 *
		 *  it is called after received delay report data.
		 *
		 *  @param err error code.
		 *  @param delay the delay value.
		 */
		void (*source_delay_report_cb)(int err, int16_t delay);
		/** @brief the delay report callback
		 *
		 *  it is called after sending delay report data.
		 *
		 *  @param err error code.
		 *  @param delay the delay value.
		 */
		void (*sink_delay_report_cb)(int err);
	};
#endif
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
	/** reserved codec related buffer (can be cacaheable ram) */
	uint8_t *codec_buffer;
	/** reserved codec related buffer (nocached) */
	uint8_t *codec_buffer_nocached;
#if ((defined(CONFIG_BT_A2DP_CP_SERVICE)) && (CONFIG_BT_A2DP_CP_SERVICE > 0U))
	/** Pointer to content protection default config */
	struct bt_a2dp_codec_ie *cp_config;
	/** content protection ie */
	struct bt_a2dp_codec_ie *cp_ie;
	/** content protection ie count */
	uint8_t cp_ie_count;
#endif
#if ((defined(CONFIG_BT_A2DP_RECOVERY_SERVICE)) && (CONFIG_BT_A2DP_RECOVERY_SERVICE > 0U))
	/** recovery service capabilities ie */
	struct bt_a2dp_codec_ie *recovery_ie;
	/** Pointer to recovery service default config */
	struct bt_a2dp_codec_ie *recovery_config;
#endif
#if ((defined(CONFIG_BT_A2DP_REPORTING_SERVICE)) && (CONFIG_BT_A2DP_REPORTING_SERVICE > 0U))
	/** reporting service enable flag */
	uint8_t reporting_service_enable;
#endif
#if ((defined(CONFIG_BT_A2DP_DR_SERVICE)) && (CONFIG_BT_A2DP_DR_SERVICE > 0U))
	/** delay reporting service enable flag */
	uint8_t delay_reporting_service_enable;
#endif
#if ((defined(CONFIG_BT_A2DP_HC_SERVICE)) && (CONFIG_BT_A2DP_HC_SERVICE > 0U))
	/** header compression cap,
	 *  0x00 means the endpoint doesn't support header compression
	 */
	uint8_t header_compression_cap;
	/** header compression config,
	 *  0x00 means the endpoint doesn't configure defaultly.
	 */
	uint8_t header_compression_config;
#endif
#if ((defined(CONFIG_BT_A2DP_MULTIPLEXING_SERVICE)) && (CONFIG_BT_A2DP_MULTIPLEXING_SERVICE > 0U))
	/** multiplexing service enable */
	uint8_t multiplexing_service_enable;
#endif
};

/** @brief A2DP Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish A2DP
 *  connection between devices.
 *  This funciton only establish AVDTP L2CAP connection.
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
 *  If the bt_a2dp_configure is failed or user want to change configured endpoint,
 *  user can call bt_a2dp_discover_peer_endpoints and this function to configure
 *  the selected endpoint.
 *
 *  @param a2dp The a2dp instance.
 *  @param endpoint The configured endpoint that is registered.
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
 *  @param endpoint the registered endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_deconfigure(struct bt_a2dp_endpoint *endpoint);

/** @brief start a2dp streamer, it is source only.
 *
 *  @param endpoint The endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_start(struct bt_a2dp_endpoint *endpoint);

/** @brief stop a2dp streamer, it is source only.
 *
 *  @param endpoint The registered endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stop(struct bt_a2dp_endpoint *endpoint);

/** @brief re-configure a2dp streamer
 *
 * This function send the AVDTP_RECONFIGURE command
 *
 *  @param a2dp The a2dp instance.
 *  @param endpoint the endpoint.
 *  @param config The config to configure the endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_reconfigure(struct bt_a2dp_endpoint *endpoint,
		struct bt_a2dp_endpoint_config *config);

#if ((defined(CONFIG_BT_A2DP_SOURCE)) && (CONFIG_BT_A2DP_SOURCE > 0U))
/** @brief send a2dp streamer data
 *
 *  @param endpoint The endpoint.
 *  @param data The streamer data.
 *  @param datalen The streamer data length.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_src_media_write(struct bt_a2dp_endpoint *endpoint,
							uint8_t *data, uint16_t datalen);
#endif

#if ((defined(CONFIG_BT_A2DP_SINK)) && (CONFIG_BT_A2DP_SINK > 0U))
/** @brief notify the streamer data is consumed.
 *
 *  The streamer data is received in sink_streamer_data callback
 *
 *  @param endpoint The endpoint.
 *  @param data The streamer data.
 *  @param datalen The streamer data length.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_snk_media_sync(struct bt_a2dp_endpoint *endpoint,
							uint8_t *data, uint16_t datalen);
#endif

#if ((defined(CONFIG_BT_A2DP_CP_SERVICE)) && (CONFIG_BT_A2DP_CP_SERVICE > 0U))
/** @brief set the content protection header.
 *
 *  @param endpoint The endpoint.
 *  @param header The header data.
 *  @param header_len The header data length.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_set_cp_header(struct bt_a2dp_endpoint *endpoint, uint8_t *header, uint16_t header_len);
#endif

#if ((defined(CONFIG_BT_A2DP_DR_SERVICE)) && (CONFIG_BT_A2DP_DR_SERVICE > 0U))
/** @brief set the initial delay report data.
 *
 *  @param endpoint The endpoint.
 *  @param delay The delay value.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_set_initial_delay_report(struct bt_a2dp_endpoint *endpoint, int16_t delay);

/** @brief send delay report data.
 *
 *  @param endpoint The endpoint.
 *  @param delay The delay value.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_send_delay_report(struct bt_a2dp_endpoint *endpoint, int16_t delay);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_ */
