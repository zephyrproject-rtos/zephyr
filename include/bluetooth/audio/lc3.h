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

#ifdef __cplusplus
extern "C" {
#endif

/** @def BT_CODEC_LC3_ID
 *  @brief LC3 codec ID
 */
#define BT_CODEC_LC3_ID                  0x06

/* TODO: Remove base once LTV types are defined */
#define BT_CODEC_LC3_CAP_BASE            0x01

/** @def BT_CODEC_LC3_FREQ
 *  @brief LC3 sample frequency capability type
 */
#define BT_CODEC_LC3_FREQ                (BT_CODEC_LC3_CAP_BASE)
/** @def BT_CODEC_LC3_FREQ_8KHZ
 *  @brief LC3 8 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_8KHZ           BIT(0)
/** @def BT_CODEC_LC3_FREQ_11KHZ
 *  @brief LC3 11.025 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_11KHZ          BIT(1)
/** @def BT_CODEC_LC3_FREQ_16KHZ
 *  @brief LC3 16 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_16KHZ          BIT(2)
/** @def BT_CODEC_LC3_FREQ_22KHZ
 *  @brief LC3 22.05 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_22KHZ          BIT(3)
/** @def BT_CODEC_LC3_FREQ_24KHZ
 *  @brief LC3 24 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_24KHZ          BIT(4)
/** @def BT_CODEC_LC3_FREQ_32KHZ
 *  @brief LC3 32 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_32KHZ          BIT(5)
/** @def BT_CODEC_LC3_FREQ_44KHZ
 *  @brief LC3 44.1 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_44KHZ          BIT(6)
/** @def BT_CODEC_LC3_FREQ_48KHZ
 *  @brief LC3 48 Khz frequency capability
 */
#define BT_CODEC_LC3_FREQ_48KHZ          BIT(7)
/** @def BT_CODEC_LC3_FREQ_ANY
 *  @brief LC3 any frequency capability
 */
#define BT_CODEC_LC3_FREQ_ANY            (BT_CODEC_LC3_FREQ_8KHZ | \
					  BT_CODEC_LC3_FREQ_16KHZ | \
					  BT_CODEC_LC3_FREQ_24KHZ | \
					  BT_CODEC_LC3_FREQ_32KHZ | \
					  BT_CODEC_LC3_FREQ_44KHZ | \
					  BT_CODEC_LC3_FREQ_48KHZ)

/** @def BT_CODEC_LC3_DURATION
 *  @brief LC3 frame duration capability type
 */
#define BT_CODEC_LC3_DURATION            (BT_CODEC_LC3_CAP_BASE + 1)
/** @def BT_CODEC_LC3_DURATION_7_5
 *  @brief LC3 7.5 msec frame duration capability
 */
#define BT_CODEC_LC3_DURATION_7_5        BIT(0)
/** @def BT_CODEC_LC3_DURATION_10
 *  @brief LC3 10 msec frame duration capability
 */
#define BT_CODEC_LC3_DURATION_10         BIT(1)
/** @def BT_CODEC_LC3_DURATION_ANY
 *  @brief LC3 any frame duration capability
 */
#define BT_CODEC_LC3_DURATION_ANY        (BT_CODEC_LC3_DURATION_7_5 | \
					  BT_CODEC_LC3_DURATION_10)
/** @def BT_CODEC_LC3_DURATION_PREFER_7_5
 *  @brief LC3 7.5 msec preferred frame duration capability
 */
#define BT_CODEC_LC3_DURATION_PREFER_7_5 BIT(4)
/** @def BT_CODEC_LC3_DURATION_PREFER_10
 *  @brief LC3 10 msec preferred frame duration capability
 */
#define BT_CODEC_LC3_DURATION_PREFER_10  BIT(5)


/** @def BT_CODEC_LC3_CHAN_COUNT
 *  @brief LC3 channel count capability type
 */
#define BT_CODEC_LC3_CHAN_COUNT          (BT_CODEC_LC3_CAP_BASE + 2)
/** @def BT_CODEC_LC3_CHAN_COUNT_SUPPORT
 *  @brief LC3 channel count support capability
 */
#define BT_CODEC_LC3_CHAN_COUNT_SUPPORT  BIT(0)

/** @def BT_CODEC_LC3_FRAME_LEN
 *  @brief LC3 frame length capability type
 */
#define BT_CODEC_LC3_FRAME_LEN           (BT_CODEC_LC3_CAP_BASE + 3)

/** @def BT_CODEC_LC3_FRAME_COUNT
 *  @brief LC3 frame count capability type
 */
#define BT_CODEC_LC3_FRAME_COUNT         (BT_CODEC_LC3_CAP_BASE + 4)

/* TODO: Remove base once LTV types are defined */
#define BT_CODEC_LC3_CONFIG_BASE         0x01

struct bt_codec_lc3_frame_len {
	uint16_t min;
	uint16_t max;
};

/** @def BT_CODEC_CONFIG_LC3_FREQ
 *  @brief LC3 Sample Frequency configuration type
 */
#define BT_CODEC_CONFIG_LC3_FREQ         (BT_CODEC_LC3_CONFIG_BASE)
/** @def BT_CODEC_CONFIG_LC3_FREQ_8KHZ
 *  @brief LC3 8 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_8KHZ    0x01
/** @def BT_CODEC_CONFIG_LC3_FREQ_11KHZ
 *  @brief LC3 11.025 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_11KHZ   0x02
/** @def BT_CODEC_CONFIG_LC3_FREQ_16KHZ
 *  @brief LC3 16 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_16KHZ   0x03
/** @def BT_CODEC_CONFIG_LC3_FREQ_22KHZ
 *  @brief LC3 22.05 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_22KHZ   0x04
/** @def BT_CODEC_CONFIG_LC3_FREQ_24KHZ
 *  @brief LC3 24 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_24KHZ   0x05
/** @def BT_CODEC_CONFIG_LC3_FREQ_32KHZ
 *  @brief LC3 32 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_32KHZ   0x06
/** @def BT_CODEC_CONFIG_LC3_FREQ_44KHZ
 *  @brief LC3 44.1 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_44KHZ   0x07
/** @def BT_CODEC_CONFIG_LC3_FREQ_48KHZ
 *  @brief LC3 48 Khz Sample Frequency configuration
 */
#define BT_CODEC_CONFIG_LC3_FREQ_48KHZ   0x08

/** @def BT_CODEC_CONFIG_LC3_DURATION
 *  @brief LC3 Frame Duration configuration type
 */
#define BT_CODEC_CONFIG_LC3_DURATION     (BT_CODEC_LC3_CONFIG_BASE + 1)
/** @def BT_CODEC_CONFIG_LC3_DURATION_7_5
 *  @brief LC3 7.5 msec Frame Duration configuration
 */
#define BT_CODEC_CONFIG_LC3_DURATION_7_5 0x00
/** @def BT_CODEC_CONFIG_LC3_DURATION_10
 *  @brief LC3 10 msec Frame Duration configuration
 */
#define BT_CODEC_CONFIG_LC3_DURATION_10  0x01

/** @def BT_CODEC_CONFIG_LC3_CHAN_ALLOC
 *  @brief LC3 channel Allocation configuration type
 */
#define BT_CODEC_CONFIG_LC3_CHAN_ALLOC   (BT_CODEC_LC3_CONFIG_BASE + 2)

/** @def BT_CODEC_CONFIG_LC3_FRAME_LEN
 *  @brief LC3 Frame Length configuration type
 */
#define BT_CODEC_CONFIG_LC3_FRAME_LEN    (BT_CODEC_LC3_CONFIG_BASE + 3)

/** @def BT_CODEC_LC3_DATA
 *  @brief Helper to declare LC3 codec capability
 */
#define BT_CODEC_LC3_DATA(_freq, _duration, _chan_count, _len_min, _len_max) \
{ \
	 BT_CODEC_DATA(BT_CODEC_LC3_FREQ, (_freq) & 0xffu, (_freq) >> 8), \
	 BT_CODEC_DATA(BT_CODEC_LC3_DURATION, _duration), \
	 BT_CODEC_DATA(BT_CODEC_LC3_CHAN_COUNT, _chan_count), \
	 BT_CODEC_DATA(BT_CODEC_LC3_FRAME_LEN, (_len_min) & 0xffu, (_len_min) >> 8, \
		       (_len_max) & 0xffu, (_len_max) >> 8) \
}

/** @def BT_CODEC_LC3_META
 *  @brief Helper to declare LC3 codec metadata
 */
#define BT_CODEC_LC3_META(_prefer_context, _context) \
{ \
	 BT_CODEC_DATA(BT_CODEC_META_PREFER_CONTEXT, (_prefer_context) & 0xffu, \
		       (_prefer_context) >> 8), \
	 BT_CODEC_DATA(BT_CODEC_META_CONTEXT, (_context) & 0xffu, (_context) >> 8) \
}

/** @def BT_CODEC_LC3
 *  @brief Helper to declare LC3 codec
 */
#define BT_CODEC_LC3(_freq, _duration, _chan_count, _len_min, _len_max, \
		     _frames, _prefer_context, _context) \
	BT_CODEC(BT_CODEC_LC3_ID, 0x0000, 0x0000, \
		 BT_CODEC_LC3_DATA(_freq, _duration, _chan_count, _len_min, \
				   _len_max), \
		 BT_CODEC_LC3_META(_prefer_context, _context))

/** @def BT_CODEC_LC3_CONFIG_DATA
 *  @brief Helper to declare LC3 codec data configuration
 */
#define BT_CODEC_LC3_CONFIG_DATA(_freq, _duration, _loc, _len) \
{ \
	 BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FREQ, _freq), \
	 BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_DURATION, _duration), \
	 BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_CHAN_ALLOC, (_loc) & 0xffu, ((_loc) >> 8) & 0xffu, \
		       ((_loc) >> 16) & 0xffu, (_loc) >> 24), \
	 BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FRAME_LEN, (_len) & 0xffu, (_len) >> 8) \
}

/** @def BT_CODEC_LC3_CONFIG_DATA
 *  @brief Helper to declare LC3 codec metadata configuration
 */
#define BT_CODEC_LC3_CONFIG_META(_context) \
{ \
	 BT_CODEC_DATA(BT_CODEC_META_CONTEXT, _context, _context >> 8), \
}

/** @def BT_CODEC_LC3_CONFIG_N
 *  @brief Helper to declare LC3 codec configuration for multiple channels.
 */
#define BT_CODEC_LC3_CONFIG_N(_freq, _duration, _loc, _len, _context) \
	BT_CODEC(BT_CODEC_LC3_ID, 0x0000, 0x0000, \
		 BT_CODEC_LC3_CONFIG_DATA(_freq, _duration, _loc, _len), \
		 BT_CODEC_LC3_CONFIG_META(_context))

/** @def BT_CODEC_LC3_CONFIG
 *  @brief Helper to declare LC3 codec configuration
 */
#define BT_CODEC_LC3_CONFIG(_freq, _duration, _len, _context) \
	BT_CODEC_LC3_CONFIG_N(_freq, _duration, 0x000000000, _len, _context)

/** @def BT_CODEC_LC3_CONFIG_8_1
 *  @brief Helper to declare LC3 8.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_8_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_8KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 26u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_8_2
 *  @brief Helper to declare LC3 8.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_8_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_8KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 30u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_16_1
 *  @brief Helper to declare LC3 16.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_16_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_16KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 30u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_16_2
 *  @brief Helper to declare LC3 16.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_16_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_16KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 40u, \
			    BT_CODEC_META_CONTEXT_VOICE)

/** @def BT_CODEC_LC3_CONFIG_24_1
 *  @brief Helper to declare LC3 24.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_24_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_24KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 45u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_24_2
 *  @brief Helper to declare LC3 24.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_24_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_24KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 60u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_32_1
 *  @brief Helper to declare LC3 32.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_32_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_32KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 60u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_32_2
 *  @brief Helper to declare LC3 32.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_32_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_32KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 80u, \
			    BT_CODEC_META_CONTEXT_VOICE)
/** @def BT_CODEC_LC3_CONFIG_441_1
 *  @brief Helper to declare LC3 441.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_441_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_44KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 98u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_441_2
 *  @brief Helper to declare LC3 441.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_441_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_44KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 130u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_1
 *  @brief Helper to declare LC3 48.1 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_1 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 75u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_2
 *  @brief Helper to declare LC3 48.2 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_2 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 100u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_3
 *  @brief Helper to declare LC3 48.3 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_3 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_LC3_DURATION_7_5, 90u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_4
 *  @brief Helper to declare LC3 48.4 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_4 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 120u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_5
 *  @brief Helper to declare LC3 48.5 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_5 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_7_5, 117u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_CONFIG_48_6
 *  @brief Helper to declare LC3 48.6 codec configuration
 */
#define BT_CODEC_LC3_CONFIG_48_6 \
	BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ, \
			    BT_CODEC_CONFIG_LC3_DURATION_10, 155u, \
			    BT_CODEC_META_CONTEXT_MEDIA)
/** @def BT_CODEC_LC3_QOS_7_5
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval
 */
#define BT_CODEC_LC3_QOS_7_5(_dir, _framing, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(_dir, 7500u, _framing, BT_CODEC_QOS_2M, _sdu, _rtn, \
		     _latency, _pd)
/** @def BT_CODEC_LC3_QOS_7_5_IN_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval unframed input
 */
#define BT_CODEC_LC3_QOS_7_5_IN_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_IN_UNFRAMED(7500u, _sdu, _rtn, _latency, _pd)
/** @def BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval unframed output
 */
#define BT_CODEC_LC3_QOS_7_5_OUT_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_OUT_UNFRAMED(7500u, _sdu, _rtn, _latency, _pd)
/** @def BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 7.5ms interval unframed in/out
 */
#define BT_CODEC_LC3_QOS_7_5_INOUT_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_INOUT_UNFRAMED(7500u, _sdu, _rtn, _latency, _pd)
/** @def BT_CODEC_LC3_QOS_10
 *  @brief Helper to declare LC3 codec QoS for 10ms frame internal
 */
#define BT_CODEC_LC3_QOS_10(_dir, _framing, _sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS(_dir, 10000u, _framing, BT_CODEC_QOS_2M, _sdu, _rtn, \
		     _latency, _pd)
/** @def BT_CODEC_LC3_QOS_10_IN_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 10ms interval unframed input
 */
#define BT_CODEC_LC3_QOS_10_IN_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_IN_UNFRAMED(10000u, _sdu, _rtn, _latency, _pd)
/** @def BT_CODEC_LC3_QOS_10_OUT_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 10ms interval unframed output
 */
#define BT_CODEC_LC3_QOS_10_OUT_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_OUT_UNFRAMED(10000u, _sdu, _rtn, _latency, _pd)
/** @def BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED
 *  @brief Helper to declare LC3 codec QoS for 10ms interval unframed in/out
 */
#define BT_CODEC_LC3_QOS_10_INOUT_UNFRAMED(_sdu, _rtn, _latency, _pd) \
	BT_CODEC_QOS_INOUT_UNFRAMED(10000u, _sdu, _rtn, _latency, _pd)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_LC3_H_ */
