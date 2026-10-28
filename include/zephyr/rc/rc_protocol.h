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
	/** RC_EVENT_FLAG_TOGGLE and RC_EVENT_FLAG_REPEAT bits */
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
