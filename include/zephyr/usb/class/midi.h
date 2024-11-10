/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_MIDI_H_
#define ZEPHYR_INCLUDE_USB_CLASS_MIDI_H_

/**
 * @brief Universal MIDI Packet definitions
 * @defgroup usb_midi_ump USB MIDI 2.0 Universal MIDI Packet definitions
 * @ingroup usb
 * @since 4.1
 * @version 0.1.0
 * @see ump112: "Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol"
 *              Document version 1.1.2
 * @{
 *
 * @defgroup usb_midi_ump_mt Message types
 * @ingroup usb_midi_ump
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 * @{
 */

#define UMP_MT_UTILITY             0x00 /**< Utility Messages */
/** System Real Time and System Common Messages (except System Exclusive) */
#define UMP_MT_SYS_RT_COMMON       0x01
#define UMP_MT_MIDI1_CHANNEL_VOICE 0x02 /**< MIDI 1.0 Channel Voice Messages */
#define UMP_MT_DATA                0x03 /**< Data Messages (including System Exclusive) */
#define UMP_MT_MIDI2_CHANNEL_VOICE 0x04 /**< MIDI 2.0 Channel Voice Messages */
#define UMP_MT_FLEX_DATA           0x0d /**< Flex Data Messages */
#define UMP_MT_UMP_STREAM          0x0f /**< UMP Stream Message */
/** @} */

/**
 * @brief      Message Type field of a Universal MIDI Packet
 * @param[in]  ump    The universal MIDI Packet
 */
#define UMP_MT(ump)    ((ump)[0] >> 28)

/**
 * @brief      Size of a Universal MIDI Packet, in 32bit words
 * @param[in]  ump    The universal MIDI Packet
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 */
#define UMP_WORDS(ump) (1 + ((0xfe950d40 >> (2 * UMP_MT(ump))) & 3))

/**
 * @brief      MIDI group field of a Universal MIDI Packet
 * @param[in]  ump    The universal MIDI Packet
 */
#define UMP_GROUP(ump) (((ump)[0] >> 24) & 0x0f)

/**
 * @brief      Status byte of a MIDI channel voice or system message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI_STATUS(ump)  (((ump)[0] >> 16) & 0xff)
/**
 * @brief      Command of a MIDI channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI event)
 * @see usb_midi_cmd
 */
#define UMP_MIDI_COMMAND(ump) (UMP_MIDI_STATUS(ump) >> 4)
/**
 * @brief      Channel of a MIDI channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI event)
 */
#define UMP_MIDI_CHANNEL(ump) (UMP_MIDI_STATUS(ump) & 0x0f)
/**
 * @brief      First parameter of a MIDI1 channel voice or system message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_P1(ump)      (((ump)[0] >> 8) & 0x7f)
/**
 * @brief      Second parameter of a MIDI1 channel voice or system message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_P2(ump)      ((ump)[0] & 0x7f)

/**
 * @brief      Initialize a MIDI1 Universal Midi Packet
 * @param      group    The UMP group
 * @param      command  The MIDI1 command
 * @param      channel  The MIDI1 channel number
 * @param      p1       The 1st MIDI1 parameter
 * @param      p2       The 2nd MIDI1 parameter
 */
#define UMP_MIDI1(group, command, channel, p1, p2) \
	(const uint32_t[4]) {                      \
		(UMP_MT_MIDI1_CHANNEL_VOICE << 28) \
		| ((group & 0x0f) << 24)           \
		| ((command & 0x0f) << 20)         \
		| ((channel & 0x0f) << 16)         \
		| ((p1 & 0x7f) << 8)               \
		| (p2 & 0x7f)                      \
	}

/**
 * @defgroup usb_midi_ump_cmd MIDI commands
 * @ingroup usb_midi_ump
 * @see ump112: 7.3 MIDI 1.0 Channel Voice Messages
 *
 * When UMP_MT(x)=UMP_MT_MIDI1_CHANNEL_VOICE or UMP_MT_MIDI2_CHANNEL_VOICE, then
 * UMP_MIDI_COMMAND(x) may be one of:
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
 * @defgroup usb_midi_ump_sys System common and System Real Time message status
 * @ingroup usb_midi_ump
 * @see ump112: 7.6 System Common and System Real Time Messages
 *
 * When UMP_MT(x)=UMP_MT_SYS_RT_COMMON, UMP_MIDI_STATUS(x) may be one of:
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

/** @} */

#endif
