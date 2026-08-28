/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_STREAM_H
#define _TRACE_STREAM_H

#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-CPU CTF packet streams.
 *
 * Each CPU owns a private ring of fixed-size CTF packets and one CTF stream.
 * Only the owning CPU appends to its packets, so an event can be reserved with
 * nothing but a local interrupt lock: producers on different CPUs never
 * contend, and every stream is monotonic in time because a single CPU samples
 * the clock and appends in that order. A reader merges the streams by the
 * declared trace clock.
 *
 * Packets are fixed size because CTF records the amount of data a packet holds
 * in its own header. Filling a packet to a boundary and padding the tail lets
 * that count be written when the packet is closed, with no need to go back and
 * patch data already handed to a backend.
 */

/** CTF packet header, on the wire. Must match trace.packet.header in the TSDL. */
struct tracing_packet_header {
	uint32_t magic;
	uint32_t stream_id;
} __packed;

/** CTF packet context, on the wire. Must match stream.packet.context in the TSDL. */
struct tracing_packet_context {
	uint64_t timestamp_begin;
	uint64_t timestamp_end;
	/* CTF counts both of these in bits, not bytes. */
	uint32_t content_size;
	uint32_t packet_size;
	uint32_t events_discarded;
	uint8_t cpu_id;
} __packed;

#define TRACING_CTF_MAGIC 0xC1FC1FC1U

#define TRACING_PACKET_PREFIX_SIZE                                                                 \
	(sizeof(struct tracing_packet_header) + sizeof(struct tracing_packet_context))

/**
 * @brief Initialize the per-CPU packet streams.
 */
void tracing_stream_init(void);

/**
 * @brief Reserve room for one event in the current CPU's packet.
 *
 * Must be called with interrupts locked on the calling CPU, and the returned
 * pointer used before they are unlocked: the reservation belongs to whichever
 * CPU is executing, so migrating mid-event would append to another CPU's
 * stream.
 *
 * Closes the current packet and opens the next one when @a length no longer
 * fits. If no free packet is left the event is accounted as discarded (and
 * reported in the next packet's context) and NULL is returned.
 *
 * @param length Number of bytes needed for the event.
 * @param tstamp Timestamp of the event, on the declared trace clock.
 *
 * @return Address to write @a length bytes of event to, or NULL if the event
 *         has to be dropped.
 */
uint8_t *tracing_stream_reserve(uint32_t length, uint64_t tstamp);

/**
 * @brief Ship every packet that is ready to leave, to all backends.
 *
 * Safe to call from any context; serializes internally, since backends
 * generally drive one shared link.
 */
void tracing_stream_drain(void);

/**
 * @brief Close the calling CPU's packet so its events can be shipped.
 *
 * Meant for the moment a CPU goes idle: what it has gathered so far would
 * otherwise sit in an unfinished packet until enough further events arrived to
 * fill it, which on a CPU that has just gone quiet may be a long time or never.
 */
void tracing_stream_flush_cpu(void);

/**
 * @brief Close every partially filled packet and ship everything.
 *
 * Used to get a complete trace out at a point where the application knows it
 * wants one, at the cost of a short packet per CPU.
 */
void tracing_stream_flush(void);

/**
 * @brief Whether any packet is ready to be shipped.
 *
 * @return true if a call to tracing_stream_drain() would do anything.
 */
bool tracing_stream_has_data(void);

/**
 * @brief Append one gathered CTF event to the current CPU's stream.
 *
 * Must be called with interrupts locked; @a data must begin with the
 * sizeof(uint64_t) timestamp slot the caller reserved, which this fills in
 * with the clock reading that also orders the event within the stream.
 *
 * @param data   Gathered event, timestamp slot first.
 * @param length Event length in bytes.
 */
void tracing_ctf_emit(uint8_t *data, uint32_t length);

/**
 * @brief Follow-up work for an emitted event, run with interrupts unlocked.
 *
 * Kept out of the interrupt-locked region so that neither shipping a packet to
 * a backend nor poking the tracing thread happens with interrupts off.
 */
void tracing_ctf_emit_post(void);

#ifdef __cplusplus
}
#endif

#endif /* _TRACE_STREAM_H */
