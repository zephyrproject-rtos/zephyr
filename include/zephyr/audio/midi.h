/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_AUDIO_MIDI_H_
#define ZEPHYR_INCLUDE_AUDIO_MIDI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Universal MIDI Packet definitions
 * @defgroup midi_ump MIDI2 Universal MIDI Packet definitions
 * @ingroup audio_interface
 * @since 4.1
 * @version 0.1.0
 * @{
 */

/**
 * @defgroup ump112 Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol
 * @ingroup midi_ump
 * @{
 * @details Definitions based on the following document
 * <a href="https://midi.org/universal-midi-packet-ump-and-midi-2-0-protocol-specification">
 * Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol
 * With MIDI 1.0 Protocol in UMP Format - Document version 1.1.2
 * (MIDI Association Document: M2-104-UM)
 * </a>
 * @}
 */

/**
 * @brief      Universal MIDI Packet container
 */
struct midi_ump {
	uint32_t data[4]; /**< Raw content, in the CPU native endianness */
};

/**
 * @defgroup midi_ump_mt Message types
 * @ingroup midi_ump
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 * @{
 */

/** Utility Messages */
#define UMP_MT_UTILITY             0x00
/** System Real Time and System Common Messages (except System Exclusive) */
#define UMP_MT_SYS_RT_COMMON       0x01
/** MIDI 1.0 Channel Voice Messages */
#define UMP_MT_MIDI1_CHANNEL_VOICE 0x02
/** 64 bits Data Messages (including System Exclusive) */
#define UMP_MT_DATA_64             0x03
/** MIDI 2.0 Channel Voice Messages */
#define UMP_MT_MIDI2_CHANNEL_VOICE 0x04
/** 128 bits Data Messages */
#define UMP_MT_DATA_128            0x05
/** Flex Data Messages */
#define UMP_MT_FLEX_DATA           0x0d
/**
 * UMP Stream Message
 * @see midi_ump_stream
 */
#define UMP_MT_UMP_STREAM          0x0f
/** @} */

/**
 * @brief      Message Type field of a Universal MIDI Packet
 * @param[in]  ump    Universal MIDI Packet
 * @see midi_ump_mt
 */
#define UMP_MT(ump) \
	((ump).data[0] >> 28)

/**
 * There are 16 UMP message types, each of which can be 1 to 4 uint32 long.
 * Hence this packed representation of 16x2b array as an uint32 lookup table
 */
#define UMP_NUM_WORDS_LOOKUP_TABLE \
	((0U << 0) | (0U << 2) | (0U << 4) | (1U << 6) | \
	(1U << 8) | (3U << 10) | (0U << 12) | (0U << 14) | \
	(1U << 16) | (1U << 18) | (1U << 20) | (2U << 22) | \
	(2U << 24) | (3U << 26) | (3U << 28) | (3U << 30))

/**
 * @brief      Size of a Universal MIDI Packet, in 32bit words
 * @param[in]  ump    Universal MIDI Packet
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 */
#define UMP_NUM_WORDS(ump) \
	(1 + ((UMP_NUM_WORDS_LOOKUP_TABLE >> (2 * UMP_MT(ump))) & 3))

/**
 * @brief      MIDI group field of a Universal MIDI Packet
 * @param[in]  ump    Universal MIDI Packet
 */
#define UMP_GROUP(ump) \
	(((ump).data[0] >> 24) & 0xF)

/**
 * @brief      Status byte of a MIDI channel voice or system message
 * @param[in]  ump    Universal MIDI Packet (containing a MIDI1 event)
 * @see midi_ump_sys
 */
#define UMP_MIDI_STATUS(ump) \
	(((ump).data[0] >> 16) & 0xFF)
/**
 * @brief      Command of a MIDI channel voice message
 * @param[in]  ump    Universal MIDI Packet (containing a MIDI event)
 * @see midi_ump_cmd
 */
#define UMP_MIDI_COMMAND(ump) \
	(UMP_MIDI_STATUS(ump) >> 4)
/**
 * @brief      Channel of a MIDI channel voice message
 * @param[in]  ump    Universal MIDI Packet (containing a MIDI event)
 */
#define UMP_MIDI_CHANNEL(ump) \
	(UMP_MIDI_STATUS(ump) & 0xF)
/**
 * @brief      First parameter of a MIDI1 channel voice or system message
 * @param[in]  ump     Universal MIDI Packet (containing a MIDI1 message)
 */
#define UMP_MIDI1_P1(ump) \
	(((ump).data[0] >> 8) & 0x7F)
/**
 * @brief      Second parameter of a MIDI1 channel voice or system message
 * @param[in]  ump     Universal MIDI Packet (containing a MIDI1 message)
 */
#define UMP_MIDI1_P2(ump) \
	((ump).data[0] & 0x7F)

/**
 * @brief      Initialize a UMP with a MIDI1 channel voice message
 * @remark     For messages that take a single parameter, p2 is ignored by the receiver.
 * @param      group    The UMP group
 * @param      command  The MIDI1 command
 * @param      channel  The MIDI1 channel number
 * @param      p1       The 1st MIDI1 parameter
 * @param      p2       The 2nd MIDI1 parameter
 */
#define UMP_MIDI1_CHANNEL_VOICE(group, command, channel, p1, p2) \
	(struct midi_ump) {.data = {                             \
		(UMP_MT_MIDI1_CHANNEL_VOICE << 28)               \
		| (((group) & 0x0f) << 24)                       \
		| (((command) & 0x0f) << 20)                     \
		| (((channel) & 0x0f) << 16)                     \
		| (((p1) & 0x7f) << 8)                           \
		| ((p2) & 0x7f)                                  \
	}}

/**
 * @defgroup midi_ump_cmd MIDI commands
 * @ingroup midi_ump
 * @see ump112: 7.3 MIDI 1.0 Channel Voice Messages
 * @remark When UMP_MT(x)=UMP_MT_MIDI1_CHANNEL_VOICE or UMP_MT_MIDI2_CHANNEL_VOICE,
 *         then UMP_MIDI_COMMAND(x) may be one of:
 * @{
 */
#define UMP_MIDI_NOTE_OFF        0x8 /**< Note Off (p1=note number, p2=velocity) */
#define UMP_MIDI_NOTE_ON         0x9 /**< Note On (p1=note number, p2=velocity) */
#define UMP_MIDI_AFTERTOUCH      0xa /**< Polyphonic aftertouch (p1=note number, p2=data) */
#define UMP_MIDI_CONTROL_CHANGE  0xb /**< Control Change (p1=index, p2=data) */
#define UMP_MIDI_PROGRAM_CHANGE  0xc /**< Control Change (p1=program) */
#define UMP_MIDI_CHAN_AFTERTOUCH 0xd /**< Channel aftertouch (p1=data) */
#define UMP_MIDI_PITCH_BEND      0xe /**< Pitch bend (p1=lsb, p2=msb) */
/** @} */

/**
 * @brief      Initialize a UMP with a System Real Time and System Common Message
 * @remark     For messages that take only one (or no) parameter, p2 (and p1)
 *             are ignored by the receiver.
 * @param      group    The UMP group
 * @param      status   The status byte
 * @param      p1       The 1st parameter
 * @param      p2       The 2nd parameter
 */
#define UMP_SYS_RT_COMMON(group, status, p1, p2) \
	(struct midi_ump) {.data = {             \
		(UMP_MT_SYS_RT_COMMON << 28)     \
		| (((group) & 0x0f) << 24)       \
		| ((status) << 16)               \
		| (((p1) & 0x7f) << 8)           \
		| ((p2) & 0x7f)                  \
	}}

/**
 * @defgroup midi_ump_sys System common and System Real Time message status
 * @ingroup midi_ump
 * @see ump112: 7.6 System Common and System Real Time Messages
 *
 * @remark When UMP_MT(x)=UMP_MT_SYS_RT_COMMON,
 *         then UMP_MIDI_STATUS(x) may be one of:
 * @{
 */
#define UMP_SYS_MIDI_TIME_CODE 0xf1 /**< MIDI Time Code (no param) */
#define UMP_SYS_SONG_POSITION  0xf2 /**< Song Position Pointer (p1=lsb, p2=msb) */
#define UMP_SYS_SONG_SELECT    0xf3 /**< Song Select (p1=song number) */
#define UMP_SYS_TUNE_REQUEST   0xf6 /**< Tune Request (no param) */
#define UMP_SYS_TIMING_CLOCK   0xf8 /**< Timing Clock (no param) */
#define UMP_SYS_START          0xfa /**< Start (no param) */
#define UMP_SYS_CONTINUE       0xfb /**< Continue (no param) */
#define UMP_SYS_STOP           0xfc /**< Stop (no param) */
#define UMP_SYS_ACTIVE_SENSING 0xfe /**< Active sensing (no param) */
#define UMP_SYS_RESET          0xff /**< Reset (no param) */
/** @} */


/**
 * @defgroup midi_ump_stream UMP Stream specific fields
 * @ingroup midi_ump
 * @see ump112: 7.1 UMP Stream Messages
 *
 * @{
 */

/**
 * @brief      Format of a UMP Stream message
 * @param[in]  ump     Universal MIDI Packet (containing a UMP Stream message)
 * @see midi_ump_stream_format
 */
#define UMP_STREAM_FORMAT(ump) \
	(((ump).data[0] >> 26) & 0x03)

/**
 * @defgroup midi_ump_stream_format UMP Stream format
 * @ingroup midi_ump_stream
 * @see ump112: 7.1 UMP Stream Messages: Format
 * @remark When UMP_MT(x)=UMP_MT_UMP_STREAM,
 *         then UMP_STREAM_FORMAT(x) may be one of:
 * @{
 */

/** Complete message in one UMP */
#define UMP_STREAM_FORMAT_COMPLETE 0x00
/** Start of a message which spans two or more UMPs */
#define UMP_STREAM_FORMAT_START    0x01
/** Continuing a message which spans three or more UMPs.
 *  There might be multiple Continue UMPs in a single message
 */
#define UMP_STREAM_FORMAT_CONTINUE 0x02
/** End of message which spans two or more UMPs */
#define UMP_STREAM_FORMAT_END      0x03

/** @} */

/**
 * @brief      Status field of a UMP Stream message
 * @param[in]  ump     Universal MIDI Packet (containing a UMP Stream message)
 * @see midi_ump_stream_status
 */
#define UMP_STREAM_STATUS(ump) \
	(((ump).data[0] >> 16) & 0x3FF)

/**
 * @defgroup midi_ump_stream_status UMP Stream status
 * @ingroup midi_ump_stream
 * @see ump112: 7.1 UMP Stream Messages
 * @remark When UMP_MT(x)=UMP_MT_UMP_STREAM,
 *         then UMP_STREAM_STATUS(x) may be one of:
 * @{
 */

/** Endpoint Discovery Message */
#define UMP_STREAM_STATUS_EP_DISCOVERY 0x00
/** Endpoint Info Notification Message */
#define UMP_STREAM_STATUS_EP_INFO      0x01
/** Device Identity Notification Message */
#define UMP_STREAM_STATUS_DEVICE_IDENT 0x02
/** Endpoint Name Notification */
#define UMP_STREAM_STATUS_EP_NAME      0x03
/** Product Instance Id Notification Message */
#define UMP_STREAM_STATUS_PROD_ID      0x04
/** Stream Configuration Request Message */
#define UMP_STREAM_STATUS_CONF_REQ     0x05
/** Stream Configuration Notification Message */
#define UMP_STREAM_STATUS_CONF_NOTIF   0x06
/** Function Block Discovery Message */
#define UMP_STREAM_STATUS_FB_DISCOVERY 0x10
/** Function Block Info Notification */
#define UMP_STREAM_STATUS_FB_INFO      0x11
/** Function Block Name Notification */
#define UMP_STREAM_STATUS_FB_NAME      0x12
/** @} */

/**
 * @brief      Filter bitmap of an Endpoint Discovery message
 * @param[in]  ump     Universal MIDI Packet (containing an Endpoint Discovery message)
 * @see ump112: 7.1.1 Endpoint Discovery Message
 * @see midi_ump_ep_disc
 */
#define UMP_STREAM_EP_DISCOVERY_FILTER(ump) \
	((ump).data[1] & 0xFF)

/**
 * @defgroup midi_ump_ep_disc UMP Stream endpoint discovery message filter bits
 * @ingroup midi_ump_stream
 * @see ump112: 7.1.1 Fig. 12: Endpoint Discovery Message Filter Bitmap Field
 * @remark When UMP_MT(x)=UMP_MT_UMP_STREAM and
 *         UMP_STREAM_STATUS(x)=UMP_STREAM_STATUS_EP_DISCOVERY,
 *         then UMP_STREAM_EP_DISCOVERY_FILTER(x) may be an ORed combination of:
 * @{
 */

/** Requesting an Endpoint Info Notification */
#define UMP_EP_DISC_FILTER_EP_INFO    BIT(0)
/** Requesting a Device Identity Notification */
#define UMP_EP_DISC_FILTER_DEVICE_ID  BIT(1)
/** Requesting an Endpoint Name Notification */
#define UMP_EP_DISC_FILTER_EP_NAME    BIT(2)
/** Requesting a Product Instance Id Notification */
#define UMP_EP_DISC_FILTER_PRODUCT_ID BIT(3)
/** Requesting a Stream Configuration Notification */
#define UMP_EP_DISC_FILTER_STREAM_CFG BIT(4)
/** @} */

/**
 * @brief      Filter bitmap of a Function Block Discovery message
 * @param[in]  ump     Universal MIDI Packet (containing a Function Block Discovery message)
 * @see ump112: 7.1.7 Function Block Discovery Message
 * @see midi_ump_fb_disc
 */
#define UMP_STREAM_FB_DISCOVERY_FILTER(ump) \
	((ump).data[0] & 0xFF)

/**
 * @brief      Block number requested in a Function Block Discovery message
 * @param[in]  ump     Universal MIDI Packet (containing a Function Block Discovery message)
 * @see ump112: 7.1.7 Function Block Discovery Message
 */
#define UMP_STREAM_FB_DISCOVERY_NUM(ump) \
	(((ump).data[0] >> 8) & 0xFF)

/**
 * @defgroup midi_ump_fb_disc UMP Stream Function Block discovery message filter bits
 * @ingroup midi_ump_stream
 * @see ump112: 7.1.7 Fig. 21: Function Block Discovery Filter Bitmap Field Format
 * @remark When UMP_MT(x)=UMP_MT_UMP_STREAM and
 *         UMP_STREAM_STATUS(x)=UMP_STREAM_STATUS_FB_DISCOVERY,
 *         then UMP_STREAM_FB_DISCOVERY_FILTER(x) may be an ORed combination of:
 * @{
 */
/** Requesting a Function Block Info Notification */
#define UMP_FB_DISC_FILTER_INFO BIT(0)
/** Requesting a Function Block Name Notification */
#define UMP_FB_DISC_FILTER_NAME BIT(1)
/** @} */

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
