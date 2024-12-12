/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_MIDI_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_MIDI_H_

/**
 * @brief USB-MIDI 2.0 class device API
 * @defgroup usb_midi USB MIDI 2.0 Class device API
 * @ingroup usb
 * @since 4.1
 * @version 0.1.0
 * @see ump112: "Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol"
 *              Document version 1.1.2
 * @{
 */

#include <zephyr/device.h>

/**
 * @defgroup usb_midi_mt Universal MIDI Packet message types
 * @ingroup usb_midi
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 * @{
 */

#define MT_UTILITY             0x00
#define MT_SYS_RT_COMMON       0x01
#define MT_MIDI1_CHANNEL_VOICE 0x02
#define MT_DATA                0x03
#define MT_MIDI2_CHANNEL_VOICE 0x04
#define MT_FLEX_DATA           0x0d
#define MT_UMP_STREAM          0x0f

/**
 * @}
 *
 * @defgroup usb_midi_accessors Universal MIDI Packet accessors
 * @ingroup usb_midi
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 * @{
 *
 * @brief      Size of a Universal MIDI Packet, in 32bit words
 * @param[in]  ump    The universal MIDI Packet
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 */
#define UMP_WORDS(ump) (1 + ((0xfe950d40 >> (2 * UMP_MT(ump))) & 3))

/**
 * @brief      Message Type field of a Universal MIDI Packet
 * @param[in]  ump    The universal MIDI Packet
 */
#define UMP_MT(ump)    ((ump)[0] >> 28)
/**
 * @brief      MIDI group field of a Universal MIDI Packet
 * @param[in]  ump    The universal MIDI Packet
 */
#define UMP_GROUP(ump) (((ump)[0] >> 24) & 0x0f)

/**
 * @brief      Command of a MIDI1 channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_COMMAND(ump) (((ump)[0] >> 20) & 0x0f)
/**
 * @brief      Channel of a MIDI1 channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_CHANNEL(ump) (((ump)[0] >> 16) & 0x0f)
/**
 * @brief      First parameter of a MIDI1 channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_P1(ump)      (((ump)[0] >> 8) & 0x7f)
/**
 * @brief      Second parameter of a MIDI1 channel voice message
 * @param[in]  ump    A universal MIDI Packet (containing a MIDI1 event)
 */
#define UMP_MIDI1_P2(ump)      ((ump)[0] & 0x7f)

/**
 * @}
 *
 * @brief      Initialize a MIDI1 Universal Midi Packet
 * @param      group    The UMP group
 * @param      command  The MIDI1 command nibble
 * @param      channel  The MIDI1 channel number
 * @param      p1       The 1st MIDI1 parameter
 * @param      p2       The 2nd MIDI1 parameter
 */
#define UMP_MIDI1(group, command, channel, p1, p2)                                                 \
	(const uint32_t[])                                                                         \
	{                                                                                          \
		(MT_MIDI1_CHANNEL_VOICE << 28) | ((group & 0x0f) << 24) |                          \
			((command & 0x0f) << 20) | ((channel & 0x0f) << 16) | ((p1 & 0x7f) << 8) | \
			(p2 & 0x7f)                                                                \
	}

/**
 * @defgroup usb_midi_cmd MIDI commands
 * @ingroup usb_midi
 * @see ump112: 7.3 MIDI 1.0 Channel Voice Messages
 * @{
 */
#define MIDI_NOTE_OFF        0x8
#define MIDI_NOTE_ON         0x9
#define MIDI_AFTERTOUCH      0xa
#define MIDI_CONTROL_CHANGE  0xb
#define MIDI_PROGRAM_CHANGE  0xc
#define MIDI_CHAN_AFTERTOUCH 0xd
#define MIDI_PITCH_BEND      0xe

/**
 * @}
 *
 * @brief      Send a Universal MIDI Packet to the host
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  ump   The Universal MIDI packet to send
 * @return     0 on success
 *             -EIO if MIDI2.0 is not enabled by the host
 *             -EAGAIN if there isn't room in the transmission buffer
 *
 */
int usb_midi_send(const struct device *dev, const uint32_t *ump);

/**
 * @brief Callback type for incoming Universal MIDI Packets from host
 * @param[in]  dev   The USB-MIDI interface receiving the packet
 * @param[in]  ump   The received packet
 */
typedef void (*usb_midi_callback)(const struct device *dev, const uint32_t *ump);

/**
 * @brief      Callback for incoming Universal MIDI Packets from host
 * Set the function to call when a Universal MIDI Packet is received from the
 * host on that interface
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  cb    The function to call
 */
void usb_midi_set_callback(const struct device *dev, usb_midi_callback cb);

/**
 * @}
 */

#endif
