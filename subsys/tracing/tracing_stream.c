/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Disable syscall tracing for all calls from this compilation unit to avoid
 * recursing back into the tracing core from the packet path.
 */
#define DISABLE_SYSCALL_TRACING

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <tracing_core.h>
#include <tracing_stream.h>

#define PACKET_SIZE  CONFIG_TRACING_CTF_PACKET_SIZE
#define NUM_PACKETS  CONFIG_TRACING_CTF_PACKETS_PER_CPU
#define NUM_CPUS     CONFIG_MP_MAX_NUM_CPUS
#define PACKET_ROOM  (PACKET_SIZE - TRACING_PACKET_PREFIX_SIZE)

struct tracing_cpu_stream {
	uint8_t packet[NUM_PACKETS][PACKET_SIZE];
	/* Bytes used in each packet, prefix included. */
	uint32_t used[NUM_PACKETS];
	/*
	 * Free-running counters; the slot in use is the counter modulo
	 * NUM_PACKETS. head is written only by the CPU that owns this stream
	 * and tail only by whoever drains, which is what makes this a
	 * single-producer single-consumer ring needing no lock between them.
	 */
	atomic_t head;
	atomic_t tail;
	/* Whether the packet at head has had its prefix written. */
	bool open;
	/* Cumulative since boot: CTF expects a running total, and a reader
	 * takes the difference between consecutive packets.
	 */
	uint32_t discarded;
	uint64_t ts_begin;
	uint64_t ts_end;
};

static struct tracing_cpu_stream streams[NUM_CPUS];

/*
 * Backends generally drive one shared link, so shipping has to be serialized
 * even though producing does not.
 */
static struct k_spinlock stream_out_lock;

#ifdef CONFIG_TRACING_ASYNC
/* Set once a wake-up has been requested, cleared when a drain starts. */
static atomic_t drain_pending;
#endif

static inline uint32_t stream_cpu_id(void)
{
#ifdef CONFIG_SMP
	return (uint32_t)arch_curr_cpu()->id;
#else
	return 0U;
#endif
}

static void packet_open(struct tracing_cpu_stream *s, uint64_t tstamp)
{
	uint32_t slot = (uint32_t)atomic_get(&s->head) % NUM_PACKETS;
	struct tracing_packet_header *hdr = (struct tracing_packet_header *)&s->packet[slot][0];

	hdr->magic = TRACING_CTF_MAGIC;
	/* One stream class; the per-CPU instances are told apart by the cpu_id
	 * in the packet context.
	 */
	hdr->stream_id = 0U;

	s->used[slot] = TRACING_PACKET_PREFIX_SIZE;
	s->ts_begin = tstamp;
	s->ts_end = tstamp;
	s->open = true;
}

static void packet_close(struct tracing_cpu_stream *s, uint32_t cpu)
{
	uint32_t slot = (uint32_t)atomic_get(&s->head) % NUM_PACKETS;
	struct tracing_packet_context *ctx =
		(struct tracing_packet_context *)&s->packet[slot]
						  [sizeof(struct tracing_packet_header)];

	ctx->timestamp_begin = s->ts_begin;
	ctx->timestamp_end = s->ts_end;
	ctx->content_size = s->used[slot] * 8U;
	ctx->packet_size = PACKET_SIZE * 8U;
	ctx->events_discarded = s->discarded;
	ctx->cpu_id = (uint8_t)cpu;

	/*
	 * Every packet goes out at full size so the stream stays a sequence of
	 * equally sized packets; the reader stops at content_size and ignores
	 * the padding.
	 */
	(void)memset(&s->packet[slot][s->used[slot]], 0, PACKET_SIZE - s->used[slot]);
	s->used[slot] = PACKET_SIZE;

	s->open = false;
	(void)atomic_inc(&s->head);
}

void tracing_stream_init(void)
{
	(void)memset(streams, 0, sizeof(streams));
}

uint8_t *tracing_stream_reserve(uint32_t length, uint64_t tstamp)
{
	uint32_t cpu = stream_cpu_id();
	struct tracing_cpu_stream *s = &streams[cpu];
	uint32_t slot;
	uint8_t *ptr;

	if (length > PACKET_ROOM) {
		/* Cannot ever fit, whatever the packet does next. */
		s->discarded++;
		return NULL;
	}

	slot = (uint32_t)atomic_get(&s->head) % NUM_PACKETS;

	if (s->open && ((s->used[slot] + length) > PACKET_SIZE)) {
		packet_close(s, cpu);
	}

	if (!s->open) {
		if ((uint32_t)(atomic_get(&s->head) - atomic_get(&s->tail)) >= NUM_PACKETS) {
			/* Every packet is waiting to be shipped. */
			s->discarded++;
			return NULL;
		}
		packet_open(s, tstamp);
	}

	slot = (uint32_t)atomic_get(&s->head) % NUM_PACKETS;
	ptr = &s->packet[slot][s->used[slot]];
	s->used[slot] += length;
	s->ts_end = tstamp;

	return ptr;
}

bool tracing_stream_has_data(void)
{
	for (uint32_t cpu = 0U; cpu < NUM_CPUS; cpu++) {
		if (atomic_get(&streams[cpu].head) != atomic_get(&streams[cpu].tail)) {
			return true;
		}
	}

	return false;
}

void tracing_stream_drain(void)
{
	k_spinlock_key_t key;

	/*
	 * Deliberately a trylock. A backend can trace on its own account - a
	 * UART backend driving a traced driver, say - which re-enters here on
	 * the same CPU, and a spinlock is not recursive. Backing off instead
	 * leaves the packets ready, for the next drain or the tracing thread
	 * to ship.
	 */
	if (k_spin_trylock(&stream_out_lock, &key) != 0) {
		return;
	}

#ifdef CONFIG_TRACING_ASYNC
	/* Cleared before shipping, so an event produced during the drain arms
	 * another wake-up rather than being left to sit until the next one.
	 */
	(void)atomic_clear(&drain_pending);
#endif

	for (uint32_t cpu = 0U; cpu < NUM_CPUS; cpu++) {
		struct tracing_cpu_stream *s = &streams[cpu];

		while (atomic_get(&s->tail) != atomic_get(&s->head)) {
			uint32_t slot = (uint32_t)atomic_get(&s->tail) % NUM_PACKETS;

			tracing_buffer_handle((uint8_t)cpu, &s->packet[slot][0], PACKET_SIZE);
			(void)atomic_inc(&s->tail);
		}
	}

	k_spin_unlock(&stream_out_lock, key);
}

void tracing_ctf_emit(uint8_t *data, uint32_t length)
{
	uint64_t tstamp;
	uint8_t *dst;

#ifdef CONFIG_TRACING_CTF_TIMESTAMP
	/*
	 * Sampled here, inside the caller's interrupt lock, so that the order
	 * in which this CPU reads the clock is the order in which it appends
	 * to its own stream. Streams from different CPUs are independent and a
	 * reader merges them on the declared trace clock, so no cross-CPU
	 * serialization is needed to keep timestamps sane.
	 */
	tstamp = tracing_timestamp_get();
	(void)memcpy(data, &tstamp, sizeof(tstamp));
#else
	tstamp = 0U;
#endif

	dst = tracing_stream_reserve(length, tstamp);
	if (dst == NULL) {
		tracing_packet_drop_handle();
		return;
	}

	(void)memcpy(dst, data, length);
}

void tracing_ctf_emit_post(void)
{
	if (!tracing_stream_has_data()) {
		return;
	}

#ifdef CONFIG_TRACING_ASYNC
	/*
	 * Edge triggered, not level triggered. Waking the tracing thread means
	 * starting a timer, and starting a timer is itself traced, so a wake on
	 * every event with a packet pending would recurse until the stack ran
	 * out. Arming once and letting the drain clear it bounds the recursion
	 * at one level: the nested event finds the flag already set and stops.
	 */
	if (atomic_cas(&drain_pending, 0, 1)) {
		tracing_trigger_output(true);
	}
#else
	/*
	 * Shipping calls into a backend, which may itself be traced - a UART
	 * backend driving a traced driver, say. That recursion terminates in
	 * tracing_stream_drain(), whose trylock fails on the nested call.
	 */
	tracing_stream_drain();
#endif
}

void tracing_stream_flush(void)
{
	for (uint32_t cpu = 0U; cpu < NUM_CPUS; cpu++) {
		struct tracing_cpu_stream *s = &streams[cpu];
		unsigned int key;

		/*
		 * Closing touches state owned by the CPU that produces into
		 * this stream. Locking interrupts only shuts out that CPU's own
		 * interrupt-context producers; a flush racing a remote CPU
		 * still in the middle of an event is not made safe by this, so
		 * only close a packet that has something in it and accept that
		 * a concurrently produced event lands in the next packet.
		 */
		key = irq_lock();
		if (s->open && (s->used[(uint32_t)atomic_get(&s->head) % NUM_PACKETS] >
				TRACING_PACKET_PREFIX_SIZE)) {
			packet_close(s, cpu);
		}
		irq_unlock(key);
	}

	tracing_stream_drain();
}
