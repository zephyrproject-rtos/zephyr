/** @file
 *  @brief Bluetooth LC3 codec handling
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_LC3_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_LC3_H_

/**
 * @brief LC3
 * @defgroup bt_codec_lc3 AUDIO
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief LC3 codec ID
 */
#define BT_CODEC_LC3_ID                  0x06

/**
 * @brief Codec capability type id's
 *
 * Used to build and parse codec capabilities as specified in the PAC specification.
 * Source is assigned numbers for Generic Audio, bluetooth.com.
 *
 * Even though they are in-fixed with LC3 they can be used for other codec types as well.
 */
enum bt_codec_capability_type {

	/**
	 *  @brief LC3 sample frequency capability type
	 */
	BT_CODEC_LC3_FREQ                = 0x01,

	/**
	 *  @brief LC3 frame duration capability type
	 */
	BT_CODEC_LC3_DURATION            = 0x02,

	/**
	 *  @brief LC3 channel count capability type
	 */
	BT_CODEC_LC3_CHAN_COUNT          = 0x03,

	/**
	 *  @brief LC3 frame length capability type
	 */
	BT_CODEC_LC3_FRAME_LEN           = 0x04,

	/**
	 *  @brief Max codec frame count per SDU capability type
	 */
	BT_CODEC_LC3_FRAME_COUNT         = 0x05,
};

/**
 *  @brief LC3 8 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_8KHZ           BIT(0)
/**
 *  @brief LC3 11.025 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_11KHZ          BIT(1)
/**
 *  @brief LC3 16 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_16KHZ          BIT(2)
/**
 *  @brief LC3 22.05 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_22KHZ          BIT(3)
/**
 *  @brief LC3 24 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_24KHZ          BIT(4)
/**
 *  @brief LC3 32 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_32KHZ          BIT(5)
/**
 *  @brief LC3 44.1 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_44KHZ          BIT(6)
/**
 *  @brief LC3 48 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_48KHZ          BIT(7)
/**
 *  @brief LC3 any frequency capability
 */
#define BT_CODEC_LC3_FREQ_ANY            (BT_CODEC_LC3_FREQ_8KHZ | \
					  BT_CODEC_LC3_FREQ_16KHZ | \
					  BT_CODEC_LC3_FREQ_24KHZ | \
					  BT_CODEC_LC3_FREQ_32KHZ | \
					  BT_CODEC_LC3_FREQ_44KHZ | \
					  BT_CODEC_LC3_FREQ_48KHZ)

/**
 *  @brief LC3 7.5 msec frame duration capability
 */
#define BT_CODEC_LC3_DURATION_7_5        BIT(0)
/**
 *  @brief LC3 10 msec frame duration capability
 */
#define BT_CODEC_LC3_DURATION_10         BIT(1)
/**
 *  @brief LC3 any frame duration capability
 */
#define BT_CODEC_LC3_DURATION_ANY        (BT_CODEC_LC3_DURATION_7_5 | \
					  BT_CODEC_LC3_DURATION_10)
/**
 *  @brief LC3 7.5 msec preferred frame duration capability
 */
#define BT_CODEC_LC3_DURATION_PREFER_7_5 BIT(4)
/**
 *  @brief LC3 10 msec preferred frame duration capability
 */
#define BT_CODEC_LC3_DURATION_PREFER_10  BIT(5)

/**
 *  @brief LC3 minimum supported channel counts
 */
#define BT_CODEC_LC3_CHAN_COUNT_MIN 1
/**
 *  @brief LC3 maximum supported channel counts
 */
#define BT_CODEC_LC3_CHAN_COUNT_MAX 8
/**
 *  @brief LC3 channel count support capability
 *
 *  Macro accepts variable number of channel counts.
 *  The allowed channel counts are defined by specification and have to be in range from
 *  @ref BT_CODEC_LC3_CHAN_COUNT_MIN to @ref BT_CODEC_LC3_CHAN_COUNT_MAX inclusive.
 *
 *  Example to support 1 and 3 channels:
 *    BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1, 3)
 */
#define BT_CODEC_LC3_CHAN_COUNT_SUPPORT(...) ((uint8_t)((FOR_EACH(BIT, (|), __VA_ARGS__)) >> 1))

struct bt_codec_lc3_frame_len {
	uint16_t min;
	uint16_t max;
};

/**
 * @brief Codec configuration type IDs
 *
 * Used to build and parse codec configurations as specified in the ASCS and BAP specifications.
 * Source is assigned numbers for Generic Audio, bluetooth.com.
 *
 * Even though they are in-fixed with LC3 they can be used for other codec types as well.
 */
enum bt_codec_config_type {

	/** @brief LC3 Sample Frequency configuration type. */
	BT_CODEC_CONFIG_LC3_FREQ                 = 0x01,

	/** @brief LC3 Frame Duration configuration type. */
	BT_CODEC_CONFIG_LC3_DURATION             = 0x02,

	/** @brief LC3 channel Allocation configuration type. */
	BT_CODEC_CONFIG_LC3_CHAN_ALLOC           = 0x03,

	/** @brief LC3 Frame Length configuration type. */
	BT_CODEC_CONFIG_LC3_FRAME_LEN            = 0x04,

	/** @brief Codec frame blocks, per SDU configuration type. */
	BT_CODEC_CONFIG_LC3_FRAME_BLKS_PER_SDU   = 0x05,
};

/**
 *  @brief 8 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_8KHZ    0x01
/**
 *  @brief 11.025 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_11KHZ   0x02
/**
 *  @brief 16 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_16KHZ   0x03
/**
 *  @brief 22.05 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_22KHZ   0x04
/**
 *  @brief 24 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_24KHZ   0x05
/**
 *  @brief 32 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_32KHZ   0x06
/**
 *  @brief 44.1 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_44KHZ   0x07
/**
 *  @brief 48 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_48KHZ   0x08
/**
 *  @brief 88.2 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_88KHZ   0x09
/**
 *  @brief 96 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_96KHZ   0x0a
/**
 *  @brief 176.4 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_176KHZ   0x0b
/**
 *  @brief 192 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_192KHZ   0x0c
/**
 *  @brief 384 Khz codec Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_384KHZ   0x0d

/**
 *  @brief LC3 7.5 msec Frame Duration configuration
 */
#define BT_CODEC_CONFIG_LC3_DURATION_7_5 0x00
/**
 *  @brief LC3 10 msec Frame Duration configuration
 */
#define BT_CODEC_CONFIG_LC3_DURATION_10  0x01


/**
 *  @brief Helper to declare LC3 codec capability
 *
 *  _max_frames_per_sdu value is optional and will be included only if != 1
 */
/* COND_CODE_1 is used to omit an LTV entry in case the _frames_per_sdu is 1.
 * COND_CODE_1 will evaluate to second argument if the flag parameter(first argument) is 1
 * - removing one layer of paranteses.
 * If the flags argument is != 1 it will evaluate to the third argument which inserts a LTV
 * entry for the max_frames_per_sdu value.
 */
 #define BT_CODEC_LC3_DATA(_freq, _duration, _chan_count, _len_min, _len_max, _max_frames_per_sdu) \
{ \
	BT_CODEC_DATA(BT_CODEC_LC3_FREQ, \
		      ((_freq) & 0xFFU), \
		      (((_freq) >> 8) & 0xFFU)), \
	BT_CODEC_DATA(BT_CODEC_LC3_DURATION, _duration), \
	BT_CODEC_DATA(BT_CODEC_LC3_CHAN_COUNT, _chan_count), \
	BT_CODEC_DATA(BT_CODEC_LC3_FRAME_LEN, \
		      ((_len_min) & 0xFFU), \
		      (((_len_min) >> 8) & 0xFFU), \
		      ((_len_max) & 0xFFU), \
		      (((_len_max) >> 8) & 0xFFU)) \
	COND_CODE_1(_max_frames_per_sdu, (), \
		    (, BT_CODEC_DATA(BT_CODEC_LC3_FRAME_COUNT, \
				     _max_frames_per_sdu))) \
}

/**
 *  @brief Helper to declare LC3 codec metadata
 */
#define BT_CODEC_LC3_META(_prefer_context) \
{ \
	BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PREF_CONTEXT, \
		      ((_prefer_context) & 0xFFU), \
		      (((_prefer_context) >> 8) & 0xFFU)) \
}

/**
 *  @brief Helper to declare LC3 codec
 */
#define BT_CODEC_LC3(_freq, _duration, _chan_count, _len_min, _len_max, \
		     _max_frames_per_sdu, _prefer_context) \
	BT_CODEC(BT_CODEC_LC3_ID, 0x0000, 0x0000, \
		 BT_CODEC_LC3_DATA(_freq, _duration, _chan_count, _len_min, \
				   _len_max, _max_frames_per_sdu), \
		 BT_CODEC_LC3_META(_prefer_context))

/**
 *  @brief Helper to declare LC3 codec data configuration
 *
 *  _frame_blocks_per_sdu value is optional and will be included only if != 1
 */
/* COND_CODE_1 is used to omit an LTV entry in case the _frames_per_sdu is 1.
 * COND_CODE_1 will evaluate to second argument if the flag parameter(first argument) is 1
 * - removing one layer of paranteses.
 * If the flags argument is != 1 it will evaluare to the third argument which inserts a LTV
 * entry for the frames_per_sdu value.
 */
#define BT_CODEC_LC3_CONFIG_DATA(_freq, _duration, _loc, _len, _frame_blocks_per_sdu) \
{ \
	BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FREQ, _freq), \
	BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_DURATION, _duration), \
	BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_CHAN_ALLOC, \
		      ((_loc) & 0xFFU), \
		      (((_loc) >> 8) & 0xFFU), \
		      (((_loc) >> 16) & 0xFFU), \
		      (((_loc) >> 24) & 0xFFU)), \
	BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FRAME_LEN, \
		      ((_len) & 0xFFU), \
		      (((_len) >> 8) & 0xFFU)) \
	COND_CODE_1(_frame_blocks_per_sdu, (), \
		    (, BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FRAME_BLKS_PER_SDU, \
				     _frames_per_sdu))) \
}

/**
 *  @brief Helper to declare LC3 codec metadata configuration
 */
#define BT_CODEC_LC3_CONFIG_META(_stream_context) \
{ \
	BT_CODEC_DATA(BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT, \
		      ((_stream_context) & 0xFFU), \
		      (((_stream_context) >> 8) & 0xFFU)) \
}

/**
 *  @brief Helper to declare LC3 codec configuration.
 *
 *  @param _freq            Sampling frequency (BT_CODEC_CONFIG_LC3_FREQ_*)
 *  @param _duration        Frame duration (BT_CODEC_CONFIG_LC3_DURATION_*)
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _len             Octets per frame (16-bit integer)
 *  @param _frames_per_sdu  Frames per SDU (8-bit integer)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG(_freq, _duration, _loc, _len, _frames_per_sdu, \
			    _stream_context) \
	BT_CODEC(BT_CODEC_LC3_ID, 0x0000, 0x0000, \
		 BT_CODEC_LC3_CONFIG_DATA(_freq, _duration, _loc, _len, _frames_per_sdu), \
		 BT_CODEC_LC3_CONFIG_META(_stream_context))

/**
 *  @brief Helper to declare LC3 8.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_8_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_8KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 26u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 8.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_8_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_8KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 30u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 16.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_16_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_16KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 30u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 16.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_16_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_16KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 40u, \
			    1, _stream_context)

/**
 *  @brief Helper to declare LC3 24.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_24_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_24KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 45u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 24.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_24_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_24KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 60u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 32.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_32_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_32KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 60u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 32.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_32_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_32KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 80u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 441.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_441_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_44KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 98u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 441.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_441_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_44KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 130u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.1 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_1(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 75u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.2 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_2(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 100u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.3 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_3(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 90u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.4 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_4(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 120u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.5 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_5(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, _loc, 117u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 48.6 codec configuration
 *
 *  @param _loc             Audio channel location bitfield (@ref bt_audio_location)
 *  @param _stream_context  Stream context (BT_AUDIO_CONTEXT_*)
 */
#define BT_CODEC_LC3_CONFIG_48_6(_loc, _stream_context) \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, _loc, 155u, \
			    1, _stream_context)
/**
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval
 */
#define BT_CODEC_LC3_QOS_7_5(_framing, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(7500u, _framing, BT_CODEC_QOS_2M, _sdu, _rtn, \
		     _latency, _pd)
/**
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval unframed input
 */
#define BT_CODEC_LC3_QOS_7_5_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_UNFRAMED(7500u, _sdu, _rtn, _latency, _pd)
/**
 *  @brief Helper to declare LC3 codec QoS for 10ms frame internal
 */
#define BT_CODEC_LC3_QOS_10(_framing, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(10000u, _framing, BT_CODEC_QOS_2M, _sdu, _rtn, \
		     _latency, _pd)
/**
 *  @brief Helper to declare LC3 codec QoS for 10ms interval unframed input
 */
#define BT_CODEC_LC3_QOS_10_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_UNFRAMED(10000u, _sdu, _rtn, _latency, _pd)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_LC3_H_ */
