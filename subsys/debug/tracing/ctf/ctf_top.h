/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_DEBUG_TRACING_CTF_TOP_H
#define SUBSYS_DEBUG_TRACING_CTF_TOP_H

#include <stddef.h>
#include <string.h>
#include <ctf_bottom.h>

/* Limit strings to 20 bytes to optimize bandwidth */
#define CTF_MAX_STRING_LEN 20

/* Optionally enter into a critical region, decided by bottom layer */
#define CTF_CRITICAL_REGION(x)	     \
	{			     \
		CTF_BOTTOM_LOCK();   \
		x;		     \
		CTF_BOTTOM_UNLOCK(); \
	}


#ifdef CTF_BOTTOM_TIMESTAMPED_EXTERNALLY
/* Emit CTF event using the bottom-level IO mechanics */
#define CTF_EVENT(...)						    \
	{							    \
		CTF_CRITICAL_REGION(CTF_BOTTOM_FIELDS(__VA_ARGS__)) \
	}
#endif /* CTF_BOTTOM_TIMESTAMPED_EXTERNALLY */

#ifdef CTF_BOTTOM_TIMESTAMPED_INTERNALLY
/* Emit CTF event using the bottom-level IO mechanics. Prefix by sample time */
#define CTF_EVENT(...)							    \
	{								    \
		const u32_t tstamp = k_cycle_get_32();			    \
		CTF_CRITICAL_REGION(CTF_BOTTOM_FIELDS(tstamp, __VA_ARGS__)) \
	}
#endif /* CTF_BOTTOM_TIMESTAMPED_INTERNALLY */


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
	CTF_EVENT_ID_END_CALL           =  0x42
} ctf_event_t;


typedef struct {
	char buf[CTF_MAX_STRING_LEN];
} ctf_bounded_string_t;


static inline void ctf_top_thread_switched_out(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_SWITCHED_OUT),
		thread_id
		);
}

static inline void ctf_top_thread_switched_in(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_SWITCHED_IN),
		thread_id
		);
}

static inline void ctf_top_thread_priority_set(u32_t thread_id, s8_t prio)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_PRIORITY_SET),
		thread_id,
		prio
		);
}

static inline void ctf_top_thread_create(
	u32_t thread_id,
	s8_t prio,
	ctf_bounded_string_t name
	)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_CREATE),
		thread_id,
		name
		);
}

static inline void ctf_top_thread_abort(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_ABORT),
		thread_id
		);
}

static inline void ctf_top_thread_suspend(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_SUSPEND),
		thread_id
		);
}

static inline void ctf_top_thread_resume(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_RESUME),
		thread_id
		);
}

static inline void ctf_top_thread_ready(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_READY),
		thread_id
		);
}

static inline void ctf_top_thread_pend(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_PENDING),
		thread_id
		);
}

static inline void ctf_top_thread_info(
	u32_t thread_id,
	u32_t stack_base,
	u32_t stack_size
	)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_INFO),
		thread_id,
		stack_base,
		stack_size
		);
}

static inline void ctf_top_thread_name_set(
	u32_t thread_id,
	ctf_bounded_string_t name
	)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_NAME_SET),
		thread_id,
		name
		);
}

static inline void ctf_top_isr_enter(void)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ISR_ENTER)
		);
}

static inline void ctf_top_isr_exit(void)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ISR_EXIT)
		);
}

static inline void ctf_top_isr_exit_to_scheduler(void)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ISR_EXIT_TO_SCHEDULER)
		);
}

static inline void ctf_top_idle(void)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_IDLE)
		);
}

static inline void ctf_top_void(u32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ID_START_CALL),
		id
		);
}

static inline void ctf_top_end_call(u32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ID_END_CALL),
		id
		);
}

#endif /* SUBSYS_DEBUG_TRACING_CTF_TOP_H */
