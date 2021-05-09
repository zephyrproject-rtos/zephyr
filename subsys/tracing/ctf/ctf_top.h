/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_DEBUG_TRACING_CTF_TOP_H
#define SUBSYS_DEBUG_TRACING_CTF_TOP_H

#include <stddef.h>
#include <string.h>
#include <ctf_map.h>
#include <tracing/tracing_format.h>

/* Limit strings to 20 bytes to optimize bandwidth */
#define CTF_MAX_STRING_LEN 20

/*
 * Obtain a field's size at compile-time.
 */
#define CTF_INTERNAL_FIELD_SIZE(x)      + sizeof(x)

/*
 * Append a field to current event-packet.
 */
#define CTF_INTERNAL_FIELD_APPEND(x)			 \
	{						 \
		memcpy(epacket_cursor, &(x), sizeof(x)); \
		epacket_cursor += sizeof(x);		 \
	}

/*
 * Gather fields to a contiguous event-packet, then atomically emit.
 */
#define CTF_GATHER_FIELDS(...)						    \
{									    \
	uint8_t epacket[0 MAP(CTF_INTERNAL_FIELD_SIZE, ##__VA_ARGS__)];	    \
	uint8_t *epacket_cursor = &epacket[0];				    \
									    \
	MAP(CTF_INTERNAL_FIELD_APPEND, ##__VA_ARGS__)			    \
	tracing_format_raw_data(epacket, sizeof(epacket));		    \
}

#ifdef CONFIG_TRACING_CTF_TIMESTAMP
#define CTF_EVENT(...)							    \
	{								    \
		const uint32_t tstamp = k_cyc_to_ns_floor64(		    \
				k_cycle_get_32());			    \
									    \
		CTF_GATHER_FIELDS(tstamp, __VA_ARGS__)			    \
	}
#else
#define CTF_EVENT(...)							    \
	{								    \
		CTF_GATHER_FIELDS(__VA_ARGS__)				    \
	}
#endif

/* Anonymous compound literal with 1 member. Legal since C99.
 * This permits us to take the address of literals, like so:
 *  &CTF_LITERAL(int, 1234)
 *
 * This may be required if a ctf_bottom layer uses memcpy.
 *
 * NOTE string literals already support address-of and sizeof,
 * so string literals should not be wrapped with CTF_LITERAL.
 */
#define CTF_LITERAL(type, value)  ((type) { (type)(value) })

typedef enum {
	CTF_EVENT_THREAD_SWITCHED_OUT   =  0x10,
	CTF_EVENT_THREAD_SWITCHED_IN    =  0x11,
	CTF_EVENT_THREAD_PRIORITY_SET   =  0x12,
	CTF_EVENT_THREAD_CREATE         =  0x13,
	CTF_EVENT_THREAD_ABORT          =  0x14,
	CTF_EVENT_THREAD_SUSPEND        =  0x15,
	CTF_EVENT_THREAD_RESUME         =  0x16,
	CTF_EVENT_THREAD_READY          =  0x17,
	CTF_EVENT_THREAD_PENDING        =  0x18,
	CTF_EVENT_THREAD_INFO           =  0x19,
	CTF_EVENT_THREAD_NAME_SET       =  0x1A,
	CTF_EVENT_ISR_ENTER             =  0x20,
	CTF_EVENT_ISR_EXIT              =  0x21,
	CTF_EVENT_ISR_EXIT_TO_SCHEDULER =  0x22,
	CTF_EVENT_IDLE                  =  0x30,
	CTF_EVENT_ID_START_CALL         =  0x41,
	CTF_EVENT_ID_END_CALL           =  0x42,
	CTF_EVENT_SEMAPHORE_INIT		=  0x43,
	CTF_EVENT_SEMAPHORE_GIVE		=  0x44,
	CTF_EVENT_SEMAPHORE_TAKE		=  0x45,
	CTF_EVENT_MUTEX_INIT			=  0x46,
	CTF_EVENT_MUTEX_LOCK			=  0x47,
	CTF_EVENT_MUTEX_UNLOCK			=  0x48,
} ctf_event_t;


typedef struct {
	char buf[CTF_MAX_STRING_LEN];
} ctf_bounded_string_t;


static inline void ctf_top_thread_switched_out(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SWITCHED_OUT),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_switched_in(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SWITCHED_IN),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_priority_set(
	uint32_t thread_id,
	int8_t prio,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_PRIORITY_SET),
		thread_id,
		name,
		prio
		);
}

static inline void ctf_top_thread_create(
	uint32_t thread_id,
	int8_t prio,
	ctf_bounded_string_t name
	)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_CREATE),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_abort(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_ABORT),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_suspend(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SUSPEND),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_resume(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_RESUME),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_ready(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_READY),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_pend(
	uint32_t thread_id,
	ctf_bounded_string_t name)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_PENDING),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_info(
	uint32_t thread_id,
	ctf_bounded_string_t name,
	uint32_t stack_base,
	uint32_t stack_size
	)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_INFO),
		thread_id,
		name,
		stack_base,
		stack_size
		);
}

static inline void ctf_top_thread_name_set(
	uint32_t thread_id,
	ctf_bounded_string_t name
	)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_NAME_SET),
		thread_id,
		name
		);
}

static inline void ctf_top_isr_enter(void)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_ISR_ENTER)
		);
}

static inline void ctf_top_isr_exit(void)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_ISR_EXIT)
		);
}

static inline void ctf_top_isr_exit_to_scheduler(void)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_ISR_EXIT_TO_SCHEDULER)
		);
}

static inline void ctf_top_idle(void)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_IDLE)
		);
}

static inline void ctf_top_void(uint32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_ID_START_CALL),
		id
		);
}

static inline void ctf_top_end_call(uint32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_ID_END_CALL),
		id
		);
}

static inline void ctf_top_semaphore_init(uint32_t sem_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_INIT),
		sem_id
		);
}

static inline void ctf_top_semaphore_take(uint32_t sem_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_TAKE),
		sem_id
		);
}

static inline void ctf_top_semaphore_give(uint32_t sem_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_GIVE),
		sem_id
		);
}

static inline void ctf_top_mutex_init(uint32_t mutex_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_INIT),
		mutex_id
		);
}

static inline void ctf_top_mutex_lock(uint32_t mutex_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_LOCK),
		mutex_id
		);
}

static inline void ctf_top_mutex_unlock(uint32_t mutex_id)
{
	CTF_EVENT(
		CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_UNLOCK),
		mutex_id
		);
}

#endif /* SUBSYS_DEBUG_TRACING_CTF_TOP_H */
