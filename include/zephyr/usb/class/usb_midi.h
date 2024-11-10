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
 * @brief      Size of a Universal MIDI Packet, in 32bit words
 * @param[in]  mt    The packet Message Type
 * @see ump112: 2.1.4 Message Type (MT) Allocation
 */
static inline size_t ump_words(uint8_t mt)
{
	return ((uint8_t[16]) {1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4})[mt & 0x0f];
}


/**
 * @brief      MIDI1 channel voice message in Universal MIDI Packet format
 * @see ump112: 7.3 MIDI 1.0 Channel Voice Messages
 */
struct ump_midi1 {
#ifdef CONFIG_LITTLE_ENDIAN
	/** @brief The 2nd MIDI1 parameter */
	uint8_t p2;
	/** @brief The 1st MIDI1 parameter */
	uint8_t p1;
	/** @brief The MIDI1 channel number */
	unsigned channel: 4;
	/** @brief The MIDI1 status nibble (command) */
	unsigned status: 4;
	/** @brief The UMP group to which this message belongs */
	unsigned group: 4;
	/** @brief The UMP message type (MT_MIDI1_CHANNEL_VOICE) */
	unsigned mt: 4;
#else
	/** @brief The UMP message type (MT_MIDI1_CHANNEL_VOICE) */
	unsigned mt: 4;
	/** @brief The UMP group to which this message belongs */
	unsigned group: 4;
	/** @brief The MIDI1 status nibble (command) */
	unsigned status: 4;
	/** @brief The MIDI1 channel number */
	unsigned channel: 4;
	/** @brief The 1st MIDI1 parameter */
	uint8_t p1;
	/** @brief The 2nd MIDI1 parameter */
	uint8_t p2;
#endif
} __packed;

/**
 * @brief      The Universal MIDI Packet
 * @see ump112: 2. Universal MIDI Packet (UMP) Format
 */
union ump {
	uint32_t words[4];
	struct {
#ifdef CONFIG_LITTLE_ENDIAN
		unsigned _reserved: 24;
		/** @brief The UMP group to which this message belongs */
		unsigned group: 4;
		/** @brief The UMP message type */
		unsigned mt: 4;
#else
		/** @brief The UMP message type */
		unsigned mt: 4;
		/** @brief The UMP group to which this message belongs */
		unsigned group: 4;
#endif
	} __packed;
	struct ump_midi1 midi1;
} __packed;

/**
 * @brief      Initialize a MIDI1 Universal Midi Packet
 * @param      _group    The UMP group
 * @param      _status   The MIDI1 status nibble (command)
 * @param      _channel  The MIDI1 channel number
 * @param      _p1       The 1st MIDI1 parameter
 * @param      _p2       The 2nd MIDI1 parameter
 */
#define UMP_MIDI1(_group, _status, _channel, _p1, _p2)                                             \
	((union ump){.midi1 = {.mt = MT_MIDI1_CHANNEL_VOICE,                                       \
			       .group = _group,                                                    \
			       .status = _status,                                                  \
			       .channel = _channel,                                                \
			       .p1 = _p1,                                                          \
			       .p2 = _p2}})

/**
 * @defgroup usb_midi_status MIDI status nibble (command)
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
 * @param[in]  pkt   The Universal MIDI packet to send
 * @return     0 on success
 *             -EIO if MIDI2.0 is not enabled by the host
 *             -EAGAIN if there isn't room in the transmission buffer
 *
 */
int usb_midi_send(const struct device *dev, const union ump *pkt);

/**
 * @brief Callback type for incoming Universal MIDI Packets from host
 * @param[in]  dev   The USB-MIDI interface receiving the packet
 * @param[in]  pkt   The received packet
 */
typedef void (*usb_midi_callback)(const struct device *dev, const union ump *pkt);

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
