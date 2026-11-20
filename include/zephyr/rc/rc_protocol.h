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

#include <stdbool.h>
#include <stddef.h>
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

/**
 * @brief Raw word writer for encoders.
 *
 * Adjacent durations of one level are merged, a leading space is dropped and
 * rc_raw_end() drops a trailing space, so the result starts with a pulse and
 * alternates.
 */
struct rc_raw_writer {
	/** Output words */
	uint32_t *words;
	/** Size of @ref words */
	size_t max;
	/** Words written */
	size_t count;
	/** Set when a word did not fit */
	bool overflow;
};

/** @brief Pulse distance timings: data in the space after each fixed pulse. */
struct rc_gen_pd_timings {
	/** Header pulse in us, 0 for none */
	uint32_t header_pulse;
	/** Header space in us */
	uint32_t header_space;
	/** Bit pulse in us */
	uint32_t bit_pulse;
	/** Bit space in us, for a 0 and a 1 */
	uint32_t bit_space[2];
	/** Trailer pulse in us, 0 for none */
	uint32_t trailer_pulse;
	/** Most significant bit first */
	bool msb_first;
};

/** @brief Pulse length timings: data in the pulse after each fixed space. */
struct rc_gen_pl_timings {
	/** Header pulse in us */
	uint32_t header_pulse;
	/** Bit space in us, before each bit pulse */
	uint32_t bit_space;
	/** Bit pulse in us, for a 0 and a 1 */
	uint32_t bit_pulse[2];
	/** Most significant bit first */
	bool msb_first;
};

/**
 * @brief Start writing raw words.
 *
 * @param w Writer.
 * @param words Output buffer.
 * @param max Size of @p words.
 */
static inline void rc_raw_writer_init(struct rc_raw_writer *w, uint32_t *words, size_t max)
{
	*w = (struct rc_raw_writer){.words = words, .max = max};
}

/**
 * @brief Append a pulse or a space.
 *
 * @param w Writer.
 * @param pulse true for a pulse, false for a space.
 * @param us Duration in us, 0 is ignored.
 */
void rc_raw_put(struct rc_raw_writer *w, bool pulse, uint32_t us);

/**
 * @brief Append Manchester coded bits, most significant bit first.
 *
 * @param w Writer.
 * @param data Bits, the low @p nbits are sent.
 * @param nbits Number of bits, 1 to 32.
 * @param half_us Duration of half a bit in us.
 * @param one_pulse_first true if a 1 is a pulse then a space.
 */
void rc_gen_manchester(struct rc_raw_writer *w, uint32_t data, uint8_t nbits, uint32_t half_us,
		       bool one_pulse_first);

/**
 * @brief Append a pulse distance coded frame.
 *
 * @param w Writer.
 * @param t Timings.
 * @param data Bits, the low @p nbits are sent.
 * @param nbits Number of bits, 1 to 32.
 */
void rc_gen_pd(struct rc_raw_writer *w, const struct rc_gen_pd_timings *t, uint32_t data,
	       uint8_t nbits);

/**
 * @brief Append a pulse length coded frame.
 *
 * @param w Writer.
 * @param t Timings.
 * @param data Bits, the low @p nbits are sent.
 * @param nbits Number of bits, 1 to 32.
 */
void rc_gen_pl(struct rc_raw_writer *w, const struct rc_gen_pl_timings *t, uint32_t data,
	       uint8_t nbits);

/**
 * @brief Finish writing.
 *
 * @param w Writer.
 *
 * @return Number of words written, a trailing space removed.
 * @retval -ENOBUFS A word did not fit.
 */
int rc_raw_end(struct rc_raw_writer *w);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RC_RC_PROTOCOL_H_ */
