/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RC_RC_PROTOCOL_H_
#define ZEPHYR_INCLUDE_RC_RC_PROTOCOL_H_

/**
 * @file
 * @ingroup rc_protocol_interface
 * @brief Remote control protocol decoder and encoder interface.
 */

#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Remote control protocol interface
 * @defgroup rc_protocol_interface Remote Control Protocols
 * @ingroup rc_interface
 * @{
 */

/** Maximum number of protocol decoders in one build */
#define RC_PROTOCOL_MAX 32

/** @brief Result of a decoded frame. */
struct rc_decode_result {
	/** Protocol, one of RC_PROTO_* */
	uint16_t proto;
	/** RC_EVENT_FLAG_TOGGLE, or RC_EVENT_FLAG_REPEAT for a repeat code without scancode */
	uint16_t flags;
	/** Scancode in the protocol's Linux layout */
	uint32_t scancode;
};

/** @brief Protocol decoder and encoder. */
struct rc_protocol {
	/** Protocol name */
	const char *name;
	/** First RC_PROTO_* identifier handled */
	uint16_t proto_first;
	/** Last RC_PROTO_* identifier handled */
	uint16_t proto_last;
	/** Size in bytes of the per-device decoder state */
	uint16_t state_size;
	/** Shortest receiver timeout that ends a frame of this protocol, in us, 0 if none */
	uint32_t min_timeout_us;
	/** Carrier of transmitted frames in Hz, 0 for the device's raw carrier */
	uint32_t carrier_hz;
	/** Time between repeated frames of a held key, in ms */
	uint16_t repeat_period_ms;
	/** Start to start time of the frames of one request in ms, 0 for no repeats */
	uint16_t tx_period_ms;
	/**
	 * @brief Reset the decoder state. Optional.
	 *
	 * @param state Decoder state of one device.
	 */
	void (*reset)(void *state);
	/**
	 * @brief Feed one raw word to the decoder. Optional, NULL for encode only.
	 *
	 * @param state Decoder state of one device.
	 * @param word Raw word.
	 * @param res Filled when a frame is complete.
	 *
	 * @retval 1 Frame complete, @p res is valid.
	 * @retval 0 More words needed.
	 */
	int (*decode)(void *state, uint32_t word, struct rc_decode_result *res);
	/**
	 * @brief Encode a frame. Optional, NULL for decode only.
	 *
	 * @param proto Protocol, one of RC_PROTO_* handled.
	 * @param scancode Scancode in the protocol's Linux layout.
	 * @param flags RC_EVENT_FLAG_TOGGLE to set the toggle bit, if any.
	 * @param words Filled with raw words.
	 * @param max Size of @p words.
	 *
	 * @return Number of words on success.
	 * @retval -EINVAL Invalid scancode.
	 * @retval -ENOBUFS The frame needs more than @p max words.
	 */
	int (*encode)(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max);
	/**
	 * @brief Encode a repeat frame, such as the NEC repeat code. Optional.
	 */
	int (*encode_repeat)(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
			     size_t max);
};

/**
 * @brief Define a protocol.
 *
 * @param _name Name of the protocol object.
 */
#define RC_PROTOCOL_DEFINE(_name) static const STRUCT_SECTION_ITERABLE(rc_protocol, _name)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RC_RC_PROTOCOL_H_ */
