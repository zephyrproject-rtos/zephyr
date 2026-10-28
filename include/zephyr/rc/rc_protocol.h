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

/** @brief Protocol decoder and encoder. */
struct rc_protocol {
	/** Protocol name */
	const char *name;
	/** First RC_PROTO_* identifier handled */
	uint16_t proto_first;
	/** Last RC_PROTO_* identifier handled */
	uint16_t proto_last;
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
