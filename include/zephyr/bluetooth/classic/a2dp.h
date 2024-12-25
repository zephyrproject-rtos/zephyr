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

#define BT_A2DP_STREAM_BUF_RESERVE (12U + BT_L2CAP_BUF_SIZE(0))

/** SBC IE length */
#define BT_A2DP_SBC_IE_LENGTH      (4U)
/** MPEG1,2 IE length */
#define BT_A2DP_MPEG_1_2_IE_LENGTH (4U)
/** MPEG2,4 IE length */
#define BT_A2DP_MPEG_2_4_IE_LENGTH (6U)
/** The max IE (Codec Info Element) length */
#define BT_A2DP_MAX_IE_LENGTH      (8U)

/** @brief define the audio endpoint
 *  @param _role BT_AVDTP_SOURCE or BT_AVDTP_SINK.
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 */
#define BT_A2DP_EP_INIT(_role, _codec, _capability)                                                \
	{                                                                                          \
		.codec_type = _codec,                                                              \
		.sep = {.sep_info = {.media_type = BT_AVDTP_AUDIO, .tsep = _role}},                \
		.codec_cap = _capability, .stream = NULL,                                          \
	}

/** @brief define the audio sink endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 */
#define BT_A2DP_SINK_EP_INIT(_codec, _capability)                                                  \
	BT_A2DP_EP_INIT(BT_AVDTP_SINK, _codec, _capability)

/** @brief define the audio source endpoint
 *  @param _codec value of enum bt_a2dp_codec_id.
 *  @param _capability the codec capability.
 */
#define BT_A2DP_SOURCE_EP_INIT(_codec, _capability)                                                \
	BT_A2DP_EP_INIT(BT_AVDTP_SOURCE, _codec, _capability)

/** @brief define the SBC sink endpoint that can be used as
 * bt_a2dp_register_endpoint's parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SINK_EP_DEFAULT
 * is more convenient for user to register SBC endpoint.
 *
 *  @param _name unique structure name postfix.
 *  @param _freq sbc codec frequency.
 *               for example: A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000
 *  @param _ch_mode sbc codec channel mode.
 *               for example: A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO
 *  @param _blk_len sbc codec block length.
 *               for example: A2DP_SBC_BLK_LEN_16
 *  @param _subband sbc codec subband.
 *               for example: A2DP_SBC_SUBBAND_8
 *  @param _alloc_mthd sbc codec allocate method.
 *               for example: A2DP_SBC_ALLOC_MTHD_LOUDNESS
 *  @param _min_bitpool sbc codec min bit pool. for example: 18
 *  @param _max_bitpool sbc codec max bit pool. for example: 35
 *  @
 */
#define BT_A2DP_SBC_SINK_EP(_name, _freq, _ch_mode, _blk_len, _subband, _alloc_mthd, _min_bitpool, \
			    _max_bitpool)                                                          \
	static struct bt_a2dp_codec_ie bt_a2dp_ep_cap_ie##_name = {                                \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {_freq | _ch_mode, _blk_len | _subband | _alloc_mthd, _min_bitpool,    \
			     _max_bitpool}};                                                       \
	static struct bt_a2dp_ep _name =                                                           \
		BT_A2DP_SINK_EP_INIT(BT_A2DP_SBC, (&bt_a2dp_ep_cap_ie##_name))

/** @brief define the SBC source endpoint that can be used as bt_a2dp_register_endpoint's
 * parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SOURCE_EP_DEFAULT
 * is more convenient for user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 *  @param _freq sbc codec frequency.
 *               for example: A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000
 *  @param _ch_mode sbc codec channel mode.
 *               for example: A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO
 *  @param _blk_len sbc codec block length.
 *               for example: A2DP_SBC_BLK_LEN_16
 *  @param _subband sbc codec subband.
 *               for example: A2DP_SBC_SUBBAND_8
 *  @param _alloc_mthd sbc codec allocate method.
 *               for example: A2DP_SBC_ALLOC_MTHD_LOUDNESS
 *  @param _min_bitpool sbc codec min bit pool. for example: 18
 *  @param _max_bitpool sbc codec max bit pool. for example: 35
 */
#define BT_A2DP_SBC_SOURCE_EP(_name, _freq, _ch_mode, _blk_len, _subband, _alloc_mthd,             \
			      _min_bitpool, _max_bitpool)                                          \
	static struct bt_a2dp_codec_ie bt_a2dp_ep_cap_ie##_name = {                                \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {_freq | _ch_mode, _blk_len | _subband | _alloc_mthd, _min_bitpool,    \
			     _max_bitpool}};                                                       \
	static struct bt_a2dp_ep _name =                                                           \
		BT_A2DP_SOURCE_EP_INIT(BT_A2DP_SBC, &bt_a2dp_ep_cap_ie##_name)

/** @brief define the default SBC sink endpoint that can be used as
 * bt_a2dp_register_endpoint's parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SINK_EP_DEFAULT
 * is more convenient for user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 */
#define BT_A2DP_SBC_SINK_EP_DEFAULT(_name)                                                         \
	static struct bt_a2dp_codec_ie bt_a2dp_ep_cap_ie##_name = {                                \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000 |                 \
				     A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO |              \
				     A2DP_SBC_CH_MODE_JOINT,                                       \
			     A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 |                            \
				     A2DP_SBC_ALLOC_MTHD_LOUDNESS,                                 \
			     18U, 35U}};                                                           \
	static struct bt_a2dp_ep _name =                                                           \
		BT_A2DP_SINK_EP_INIT(BT_A2DP_SBC, &bt_a2dp_ep_cap_ie##_name)

/** @brief define the default SBC source endpoint that can be used as bt_a2dp_register_endpoint's
 * parameter.
 *
 * SBC is mandatory as a2dp specification, BT_A2DP_SBC_SOURCE_EP_DEFAULT
 * is more convenient for user to register SBC endpoint.
 *
 *  @param _name the endpoint variable name.
 */
#define BT_A2DP_SBC_SOURCE_EP_DEFAULT(_name)                                                       \
	static struct bt_a2dp_codec_ie bt_a2dp_ep_cap_ie##_name = {                                \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {A2DP_SBC_SAMP_FREQ_44100 | A2DP_SBC_SAMP_FREQ_48000 |                 \
				     A2DP_SBC_CH_MODE_MONO | A2DP_SBC_CH_MODE_STREO |              \
				     A2DP_SBC_CH_MODE_JOINT,                                       \
			     A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 |                            \
				     A2DP_SBC_ALLOC_MTHD_LOUDNESS,                                 \
			     18U, 35U},                                                            \
	};                                                                                         \
	static struct bt_a2dp_ep _name =                                                           \
		BT_A2DP_SOURCE_EP_INIT(BT_A2DP_SBC, &bt_a2dp_ep_cap_ie##_name)

/** @brief define the SBC default configuration.
 *
 *  @param _name unique structure name postfix.
 *  @param _freq_cfg sbc codec frequency.
 *               for example: A2DP_SBC_SAMP_FREQ_44100
 *  @param _ch_mode_cfg sbc codec channel mode.
 *               for example: A2DP_SBC_CH_MODE_JOINT
 *  @param _blk_len_cfg sbc codec block length.
 *               for example: A2DP_SBC_BLK_LEN_16
 *  @param _subband_cfg sbc codec subband.
 *               for example: A2DP_SBC_SUBBAND_8
 *  @param _alloc_mthd_cfg sbc codec allocate method.
 *               for example: A2DP_SBC_ALLOC_MTHD_LOUDNESS
 *  @param _min_bitpool_cfg sbc codec min bit pool. for example: 18
 *  @param _max_bitpool_cfg sbc codec max bit pool. for example: 35
 */
#define BT_A2DP_SBC_EP_CFG(_name, _freq_cfg, _ch_mode_cfg, _blk_len_cfg, _subband_cfg,             \
			   _alloc_mthd_cfg, _min_bitpool_cfg, _max_bitpool_cfg)                    \
	static struct bt_a2dp_codec_ie bt_a2dp_codec_ie##_name = {                                 \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {_freq_cfg | _ch_mode_cfg,                                             \
			     _blk_len_cfg | _subband_cfg | _alloc_mthd_cfg, _min_bitpool_cfg,      \
			     _max_bitpool_cfg},                                                    \
	};                                                                                         \
	struct bt_a2dp_codec_cfg _name = {                                                         \
		.codec_config = &bt_a2dp_codec_ie##_name,                                          \
	}

/** @brief define the SBC default configuration.
 *
 *  @param _name unique structure name postfix.
 *  @param _freq_cfg the frequency to configure the remote same codec type endpoint.
 */
#define BT_A2DP_SBC_EP_CFG_DEFAULT(_name, _freq_cfg)                                               \
	static struct bt_a2dp_codec_ie bt_a2dp_codec_ie##_name = {                                 \
		.len = BT_A2DP_SBC_IE_LENGTH,                                                      \
		.codec_ie = {_freq_cfg | A2DP_SBC_CH_MODE_JOINT,                                   \
			     A2DP_SBC_BLK_LEN_16 | A2DP_SBC_SUBBAND_8 |                            \
				     A2DP_SBC_ALLOC_MTHD_LOUDNESS,                                 \
			     18U, 35U},                                                            \
	};                                                                                         \
	struct bt_a2dp_codec_cfg _name = {                                                         \
		.codec_config = &bt_a2dp_codec_ie##_name,                                          \
	}

/**
 * @brief A2DP error code
 */
enum bt_a2dp_err_code {
	/** Media Codec Type is not valid */
	BT_A2DP_INVALID_CODEC_TYPE = 0xC1,
	/** Media Codec Type is not supported */
	BT_A2DP_NOT_SUPPORTED_CODEC_TYPE = 0xC2,
	/** Sampling Frequency is not valid or multiple values have been selected */
	BT_A2DP_INVALID_SAMPLING_FREQUENCY = 0xC3,
	/** Sampling Frequency is not supported */
	BT_A2DP_NOT_SUPPORTED_SAMPLING_FREQUENCY = 0xC4,
	/** Channel Mode is not valid or multiple values have been selected */
	BT_A2DP_INVALID_CHANNEL_MODE = 0xC5,
	/** Channel Mode is not supported */
	BT_A2DP_NOT_SUPPORTED_CHANNEL_MODE = 0xC6,
	/** None or multiple values have been selected for Number of Subbands */
	BT_A2DP_INVALID_SUBBANDS = 0xC7,
	/** Number of Subbands is not supported */
	BT_A2DP_NOT_SUPPORTED_SUBBANDS = 0xC8,
	/** None or multiple values have been selected for Allocation Method */
	BT_A2DP_INVALID_ALLOCATION_METHOD = 0xC9,
	/** Allocation Method is not supported */
	BT_A2DP_NOT_SUPPORTED_ALLOCATION_METHOD = 0xCA,
	/** Minimum Bitpool Value is not valid */
	BT_A2DP_INVALID_MINIMUM_BITPOOL_VALUE = 0xCB,
	/** Minimum Bitpool Value is not supported */
	BT_A2DP_NOT_SUPPORTED_MINIMUM_BITPOOL_VALUE = 0xCC,
	/** Maximum Bitpool Value is not valid */
	BT_A2DP_INVALID_MAXIMUM_BITPOOL_VALUE = 0xCD,
	/** Maximum Bitpool Value is not supported */
	BT_A2DP_NOT_SUPPORTED_MAXIMUM_BITPOOL_VALUE = 0xCE,
	/** None or multiple values have been selected for Layer */
	BT_A2DP_INVALID_LAYER = 0xCF,
	/** Layer is not supported */
	BT_A2DP_NOT_SUPPORTED_LAYER = 0xD0,
	/** CRC is not supported */
	BT_A2DP_NOT_SUPPORTED_CRC = 0xD1,
	/** MPF-2 is not supported */
	BT_A2DP_NOT_SUPPORTED_MPF = 0xD2,
	/** VBR is not supported */
	BT_A2DP_NOT_SUPPORTED_VBR = 0xD3,
	/** None or multiple values have been selected for Bit Rate */
	BT_A2DP_INVALID_BIT_RATE = 0xD4,
	/** Bit Rate is not supported */
	BT_A2DP_NOT_SUPPORTED_BIT_RATE = 0xD5,
	/** Either 1) Object type is not valid or
	 * 2) None or multiple values have been selected for Object Type
	 */
	BT_A2DP_INVALID_OBJECT_TYPE = 0xD6,
	/** Object Type is not supported */
	BT_A2DP_NOT_SUPPORTED_OBJECT_TYPE = 0xD7,
	/** Either 1) Channels is not valid or
	 * 2) None or multiple values have been selected for Channels
	 */
	BT_A2DP_INVALID_CHANNELS = 0xD8,
	/** Channels is not supported */
	BT_A2DP_NOT_SUPPORTED_CHANNELS = 0xD9,
	/** Version is not valid */
	BT_A2DP_INVALID_VERSION = 0xDA,
	/** Version is not supported */
	BT_A2DP_NOT_SUPPORTED_VERSION = 0xDB,
	/** Maximum SUL is not acceptable for the Decoder in the SNK */
	BT_A2DP_NOT_SUPPORTED_MAXIMUM_SUL = 0xDC,
	/** None or multiple values have been selected for Block Length */
	BT_A2DP_INVALID_BLOCK_LENGTH = 0xDD,
	/** The requested CP Type is not supported */
	BT_A2DP_INVALID_CP_TYPE = 0xE0,
	/** The format of Content Protection Service Capability/Content
	 * Protection Scheme Dependent Data is not correct
	 */
	BT_A2DP_INVALID_CP_FORMAT = 0xE1,
	/** The codec parameter is invalid.
	 * Used if a more specific error code does not exist for the codec in use
	 */
	BT_A2DP_INVALID_CODEC_PARAMETER = 0xE2,
	/** The codec parameter is not supported.
	 * Used if a more specific error code does not exist for the codec in use
	 */
	BT_A2DP_NOT_SUPPORTED_CODEC_PARAMETER = 0xE3,
	/** Combination of Object Type and DRC is invalid */
	BT_A2DP_INVALID_DRC = 0xE4,
	/** DRC is not supported */
	BT_A2DP_NOT_SUPPORTED_DRC = 0xE5,
};

/** @brief Codec Type */
enum bt_a2dp_codec_type {
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

/** @brief A2DP structure */
struct bt_a2dp;

/* Internal to pass build */
struct bt_a2dp_stream;

/** @brief codec information elements for the endpoint */
struct bt_a2dp_codec_ie {
	/** Length of codec_cap */
	uint8_t len;
	/** codec information element */
	uint8_t codec_ie[BT_A2DP_MAX_IE_LENGTH];
};

/** @brief The endpoint configuration */
struct bt_a2dp_codec_cfg {
	/** The media codec configuration content */
	struct bt_a2dp_codec_ie *codec_config;
};

/** @brief Stream End Point */
struct bt_a2dp_ep {
	/** Code Type @ref bt_a2dp_codec_type */
	uint8_t codec_type;
	/** Capabilities */
	struct bt_a2dp_codec_ie *codec_cap;
	/** AVDTP Stream End Point Identifier */
	struct bt_avdtp_sep sep;
	/* Internally used stream object pointer */
	struct bt_a2dp_stream *stream;
};

struct bt_a2dp_ep_info {
	/** Code Type @ref bt_a2dp_codec_type */
	uint8_t codec_type;
	/** Codec capabilities, if SBC, use function of a2dp_codec_sbc.h to parse it */
	struct bt_a2dp_codec_ie codec_cap;
	/** Stream End Point Information */
	struct bt_avdtp_sep_info *sep_info;
};

/** @brief Helper enum to be used as return value of bt_a2dp_discover_ep_cb.
 *  The value informs the caller to perform further pending actions or stop them.
 */
enum {
	BT_A2DP_DISCOVER_EP_STOP = 0,
	BT_A2DP_DISCOVER_EP_CONTINUE,
};

/** @typedef bt_a2dp_discover_ep_cb
 *
 *  @brief Called when a stream endpoint is discovered.
 *
 *  A function of this type is given by the user to the bt_a2dp_discover_param
 *  object. It'll be called on each valid stream endpoint discovery completion.
 *  When no more endpoint then NULL is passed to the user. Otherwise user can get
 *  valid endpoint information from parameter info, user can set parameter ep to
 *  get the endpoint after the callback is return.
 *  The returned function value allows the user to control retrieving follow-up
 *  endpoints if any. If the user doesn't want to read more endpoints since
 *  current found endpoints fulfill its requirements then should return
 *  BT_A2DP_DISCOVER_EP_STOP. Otherwise returned value means
 *  more subcall iterations are allowable.
 *
 *  @param a2dp a2dp connection object identifying a2dp connection to queried remote.
 *  @param info Object pointing to the information of the callbacked endpoint.
 *  @param ep If the user want to use this found endpoint, user can set value to it
 *  to get the endpoint that can be used further in other A2DP APIs. It is NULL if info
 *  is NULL (no more endpoint is found).
 *
 *  @return BT_A2DP_DISCOVER_EP_STOP in case of no more need to continue discovery
 *  for next endpoint. By returning BT_A2DP_DISCOVER_EP_STOP user allows this
 *  discovery continuation.
 */
typedef uint8_t (*bt_a2dp_discover_ep_cb)(struct bt_a2dp *a2dp, struct bt_a2dp_ep_info *info,
					  struct bt_a2dp_ep **ep);

struct bt_a2dp_discover_param {
	/** discover callback */
	bt_a2dp_discover_ep_cb cb;
	/** The discovered endpoint info that is callbacked by cb */
	struct bt_a2dp_ep_info info;
	/** The max count of remote endpoints that can be got,
	 *  it save endpoint info internally.
	 */
	struct bt_avdtp_sep_info *seps_info;
	/** The max count of seps (stream endpoint) that can be got in this call route */
	uint8_t sep_count;
};

/** @brief The connecting callback */
struct bt_a2dp_cb {
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
	/**
	 * @brief Endpoint config request callback
	 *
	 * The callback is called whenever an endpoint is requested to be
	 * configured.
	 *
	 *  @param a2dp a2dp connection object.
	 *  @param[in]  ep        Local Audio Endpoint being configured.
	 *  @param[in]  codec_cfg Codec configuration.
	 *  @param[out] stream    Pointer to stream that will be configured for the endpoint.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*config_req)(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
			  struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
			  uint8_t *rsp_err_code);
	/**
	 * @brief Endpoint config request callback
	 *
	 * The callback is called whenever an endpoint is requested to be
	 * reconfigured.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*reconfig_req)(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
			    uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_config() and bt_a2dp_stream_reconfig()
	 *
	 *  Called when the codec configure operation is completed.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*config_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
	/**
	 * @brief Stream establishment request callback
	 *
	 * The callback is called whenever an stream is requested to be
	 * established (open cmd and create the stream l2cap channel).
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*establish_req)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_establish()
	 *
	 *  Called when the establishment operation is completed.
	 *  (open cmd and create the stream l2cap channel).
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*establish_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
	/**
	 * @brief Stream release request callback
	 *
	 * The callback is called whenever an stream is requested to be
	 * released (release cmd and release the l2cap channel)
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*release_req)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_release()
	 *
	 *  Called when the release operation is completed.
	 *  (release cmd and release the l2cap channel)
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*release_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
	/**
	 * @brief Stream start request callback
	 *
	 * The callback is called whenever an stream is requested to be
	 * started.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*start_req)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_start()
	 *
	 *  Called when the start operation is completed.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*start_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
	/**
	 * @brief Stream suspend request callback
	 *
	 * The callback is called whenever an stream is requested to be
	 * suspended.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*suspend_req)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_suspend()
	 *
	 *  Called when the suspend operation is completed.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*suspend_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
	/**
	 * @brief Stream abort request callback
	 *
	 * The callback is called whenever an stream is requested to be
	 * aborted.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[out] rsp_err_code  give the error code if response error.
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 *
	 * @return 0 in case of success or negative value in case of error.
	 */
	int (*abort_req)(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
	/** @brief Callback function for bt_a2dp_stream_abort()
	 *
	 *  Called when the abort operation is completed.
	 *
	 *  @param[in] stream    Pointer to stream object.
	 *  @param[in] rsp_err_code the remote responded error code
	 *                          bt_a2dp_err_code or bt_avdtp_err_code
	 */
	void (*abort_rsp)(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);
};

/** @brief A2DP Connect.
 *
 *  This function is to be called after the conn parameter is obtained by
 *  performing a GAP procedure. The API is to be used to establish A2DP
 *  connection between devices.
 *  This function only establish AVDTP L2CAP Signaling connection.
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
 *  @param ep Pointer to bt_a2dp_ep structure.
 *  @param media_type Media type that the Endpoint is, #bt_avdtp_media_type.
 *  @param sep_type Stream endpoint type, #bt_avdtp_sep_type.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_register_ep(struct bt_a2dp_ep *ep, uint8_t media_type, uint8_t sep_type);

/** @brief register callback.
 *
 *  The cb is called when bt_a2dp_connect is called or it is operated by remote device.
 *
 *  @param cb The callback function.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_register_cb(struct bt_a2dp_cb *cb);

/** @brief Discover remote endpoints.
 *
 *  @param a2dp The a2dp instance.
 *  @param param the discover used param.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_discover(struct bt_a2dp *a2dp, struct bt_a2dp_discover_param *param);

/** @brief A2DP Stream */
struct bt_a2dp_stream {
	/** local endpoint */
	struct bt_a2dp_ep *local_ep;
	/** remote endpoint */
	struct bt_a2dp_ep *remote_ep;
	/** remote endpoint's Stream End Point ID */
	uint8_t remote_ep_id;
	/** Audio stream operations */
	struct bt_a2dp_stream_ops *ops;
	/** the a2dp connection */
	struct bt_a2dp *a2dp;
	/** the stream current configuration */
	struct bt_a2dp_codec_ie codec_config;
};

/** @brief The stream endpoint related operations */
struct bt_a2dp_stream_ops {
	/**
	 * @brief Stream configured callback
	 *
	 * The callback is called whenever an Audio Stream has been configured or reconfigured.
	 *
	 * @param stream Stream object that has been configured.
	 */
	void (*configured)(struct bt_a2dp_stream *stream);
	/**
	 * @brief Stream establishment callback
	 *
	 * The callback is called whenever an Audio Stream has been established.
	 *
	 * @param stream Stream object that has been established.
	 */
	void (*established)(struct bt_a2dp_stream *stream);
	/**
	 * @brief Stream release callback
	 *
	 * The callback is called whenever an Audio Stream has been released.
	 * After released, the stream becomes invalid.
	 *
	 * @param stream Stream object that has been released.
	 */
	void (*released)(struct bt_a2dp_stream *stream);
	/**
	 * @brief Stream start callback
	 *
	 * The callback is called whenever an Audio Stream has been started.
	 *
	 * @param stream Stream object that has been started.
	 */
	void (*started)(struct bt_a2dp_stream *stream);
	/**
	 * @brief Stream suspend callback
	 *
	 * The callback is called whenever an Audio Stream has been suspended.
	 *
	 * @param stream Stream object that has been suspended.
	 */
	void (*suspended)(struct bt_a2dp_stream *stream);
	/**
	 * @brief Stream abort callback
	 *
	 * The callback is called whenever an Audio Stream has been aborted.
	 * After aborted, the stream becomes invalid.
	 *
	 * @param stream Stream object that has been aborted.
	 */
	void (*aborted)(struct bt_a2dp_stream *stream);
#if defined(CONFIG_BT_A2DP_SINK)
	/** @brief the media streaming data, only for sink
	 *
	 *  @param buf the data buf
	 *  @param seq_num the sequence number
	 *  @param ts the time stamp
	 */
	void (*recv)(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
		     uint32_t ts);
#endif
#if defined(CONFIG_BT_A2DP_SOURCE)
	/**
	 * @brief Stream audio HCI sent callback
	 *
	 * This callback will be called once the controller marks the SDU
	 * as completed. When the controller does so is implementation
	 * dependent. It could be after the SDU is enqueued for transmission,
	 * or after it is sent on air or flushed.
	 *
	 * This callback is only used if the ISO data path is HCI.
	 *
	 * @param stream Stream object.
	 */
	void (*sent)(struct bt_a2dp_stream *stream);
#endif
};

/**
 * @brief Register Audio callbacks for a stream.
 *
 * Register Audio callbacks for a stream.
 *
 * @param stream Stream object.
 * @param ops    Stream operations structure.
 */
void bt_a2dp_stream_cb_register(struct bt_a2dp_stream *stream, struct bt_a2dp_stream_ops *ops);

/** @brief configure endpoint.
 *
 *  bt_a2dp_discover can be used to find remote's endpoints.
 *  This function to configure the selected endpoint that is found by
 *  bt_a2dp_discover.
 *  This function sends AVDTP_SET_CONFIGURATION.
 *
 *  @param a2dp The a2dp instance.
 *  @param stream Stream object.
 *  @param local_ep The configured endpoint that is registered.
 *  @param remote_ep The remote endpoint.
 *  @param config The config to configure the endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_config(struct bt_a2dp *a2dp, struct bt_a2dp_stream *stream,
			  struct bt_a2dp_ep *local_ep, struct bt_a2dp_ep *remote_ep,
			  struct bt_a2dp_codec_cfg *config);

/** @brief establish a2dp streamer.
 *
 * This function sends the AVDTP_OPEN command and create the l2cap channel.
 *
 *  @param stream The stream object.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_establish(struct bt_a2dp_stream *stream);

/** @brief release a2dp streamer.
 *
 * This function sends the AVDTP_CLOSE command and release the l2cap channel.
 * After release, the stream becomes invalid.
 *
 *  @param stream The stream object.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_release(struct bt_a2dp_stream *stream);

/** @brief start a2dp streamer.
 *
 * This function sends the AVDTP_START command.
 *
 *  @param stream The stream object.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_start(struct bt_a2dp_stream *stream);

/** @brief suspend a2dp streamer.
 *
 * This function sends the AVDTP_SUSPEND command.
 *
 *  @param stream The stream object.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_suspend(struct bt_a2dp_stream *stream);

/** @brief re-configure a2dp streamer
 *
 * This function sends the AVDTP_RECONFIGURE command.
 *
 *  @param stream The stream object.
 *  @param config The config to configure the stream.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_reconfig(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *config);

/** @brief abort a2dp streamer.
 *
 * This function sends the AVDTP_ABORT command.
 * After abort, the stream becomes invalid.
 *
 *  @param stream The stream object.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_abort(struct bt_a2dp_stream *stream);

/** @brief get the stream l2cap mtu
 *
 *  @param stream The stream object.
 *
 *  @return mtu value
 */
uint32_t bt_a2dp_get_mtu(struct bt_a2dp_stream *stream);

#if defined(CONFIG_BT_A2DP_SOURCE)
/** @brief send a2dp media data
 *
 * Only A2DP source side can call this function.
 *
 *  @param stream The stream object.
 *  @param buf  The data.
 *  @param seq_num The sequence number.
 *  @param ts The time stamp.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int bt_a2dp_stream_send(struct bt_a2dp_stream *stream, struct net_buf *buf, uint16_t seq_num,
			uint32_t ts);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_A2DP_H_ */
