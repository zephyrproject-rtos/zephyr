/*
 * Copyright (c) 2025 Titouan Christophe
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_MIDI2_UMP_STREAM_RESPONDER_H_
#define ZEPHYR_LIB_MIDI2_UMP_STREAM_RESPONDER_H_

/**
 * @brief Respond to UMP Stream message Endpoint or Function Block discovery
 * @defgroup ump_stream_responder UMP Stream Responder
 * @ingroup midi_ump
 * @since 4.3
 * @version 0.1.0
 * @see ump112 7.1: UMP Stream Messages
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/audio/midi.h>

/**
 * @brief      UMP Function Block specification
 * @see ump112: 6: Function Blocks
 */
struct ump_block_dt_spec {
	/** Name of this function block, or NULL if unnamed */
	const char *name;
	/** Number of the first UMP group in this block */
	uint8_t first_group;
	/** Number of (contiguous) UMP groups spanned by this block */
	uint8_t groups_spanned;
	/** True if this function block is an input */
	bool is_input;
	/** True if this function block is an output */
	bool is_output;
	/** True if this function block carries MIDI1 data only */
	bool is_midi1;
	/** True if this function block is physically wired to a (MIDI1)
	 *  serial interface, where data is transmitted at the standard
	 *  baud rate of 31250 b/s
	 */
	bool is_31250bps;
};

/**
 * @brief      UMP endpoint specification
 */
struct ump_endpoint_dt_spec {
	/** Name of this endpoint, or NULL if unnamed */
	const char *name;
	/** Number of function blocks in this endpoint */
	size_t n_blocks;
	/** Function blocks in this endpoint */
	struct ump_block_dt_spec blocks[];
};

/**
 * @brief      A function to send a UMP
 */
typedef void (*ump_send_func)(const void *, const struct midi_ump);

/**
 * @brief      Initialize a configuration for the UMP Stream responder
 * @param      _dev      The device to send reply packets
 * @param      _send     The send function
 * @param      _ep_spec  The UMP endpoint specification
 */
#define UMP_STREAM_RESPONDER(_dev, _send, _ep_spec)				\
	{									\
		.dev = _dev,							\
		.send = (ump_send_func) _send,	\
		.ep_spec = _ep_spec,						\
	}

/**
 * @brief      Configuration for the UMP Stream responder
 */
struct ump_stream_responder_cfg {
	/** The device to send reply packets */
	const void *dev;
	/** The function to call to send a reply packet */
	ump_send_func send;
	/** The UMP endpoint specification */
	const struct ump_endpoint_dt_spec *ep_spec;
};

/**
 * @brief      Get a Universal MIDI Packet endpoint function block from its
 *             device-tree representation
 * @param      _node  The device tree node representing the midi2 block
 */
#define UMP_BLOCK_DT_SPEC_GET(_node)					   \
{									   \
	.name = DT_PROP_OR(_node, label, NULL),				   \
	.first_group = DT_REG_ADDR(_node),				   \
	.groups_spanned = DT_REG_SIZE(_node),				   \
	.is_input = !DT_ENUM_HAS_VALUE(_node, terminal_type, output_only), \
	.is_output = !DT_ENUM_HAS_VALUE(_node, terminal_type, input_only), \
	.is_midi1 = !DT_ENUM_HAS_VALUE(_node, protocol, midi2),		   \
	.is_31250bps = DT_PROP(_node, serial_31250bps),			   \
}

#define UMP_BLOCK_SEP_IF_OKAY(_node)			\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(_node),	\
		    (UMP_BLOCK_DT_SPEC_GET(_node),),	\
		    ())

/**
 * @brief      Get a Universal MIDI Packet endpoint description from
 *             the device-tree representation of a midi2 device
 * @param      _node  The device tree node representing a midi2 device
 */
#define UMP_ENDPOINT_DT_SPEC_GET(_node)						\
{										\
	.name = DT_PROP_OR(_node, label, NULL),					\
	.n_blocks = DT_FOREACH_CHILD_SEP(_node, DT_NODE_HAS_STATUS_OKAY, (+)),	\
	.blocks = {DT_FOREACH_CHILD(_node, UMP_BLOCK_SEP_IF_OKAY)},		\
}

/**
 * @brief      Respond to an UMP Stream message
 * @param[in]  cfg   The responder configuration
 * @param[in]  pkt   The message to respond to
 * @return     The number of UMP packets sent as reply,
 *             or -errno in case of error
 */
int ump_stream_respond(const struct ump_stream_responder_cfg *cfg,
		       const struct midi_ump pkt);

/**
 * @return     The UMP Product Instance ID of this device, based on the device
 *             hwinfo if available, otherwise an empty string
 */
const char *ump_product_instance_id(void);

/** @} */

#endif
