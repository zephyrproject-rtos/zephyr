/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CTF_MIDDLE_H
#define _CTF_MIDDLE_H

#include <stddef.h>
#include <string.h>
#include <ctf_bottom.h>


/* Optionally enter into a critical region, decided by bottom layer */
#define CTF_CRITICAL_REGION(x) 		\
	{				\
		CTF_BOTTOM_LOCK;	\
		x;			\
		CTF_BOTTOM_UNLOCK;	\
	}


#ifdef CTF_BOTTOM_TIMESTAMPED_EXTERNALLY
/* Emit CTF event using the bottom-level IO mechanics */
#define CTF_EVENT(...) 							\
	{								\
		CTF_CRITICAL_REGION( CTF_BOTTOM_FIELDS(__VA_ARGS__) )	\
	}
#endif /* CTF_BOTTOM_TIMESTAMPED_EXTERNALLY */

#ifdef CTF_BOTTOM_TIMESTAMPED_INTERNALLY
/* Emit CTF event using the bottom-level IO mechanics. Prefix by sample time */
#define CTF_EVENT(...)							   \
	{								   \
		const u32_t tstamp = k_cycle_get_32();			   \
		CTF_CRITICAL_REGION(CTF_BOTTOM_FIELDS(tstamp,__VA_ARGS__)) \
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
#define CTF_LITERAL(type,value)  ((type){(type)(value)})


typedef enum
{
	CTF_EVENT_ON_IDLE		=  0x00,
	CTF_EVENT_ISR_ENTER		=  0x10,
	CTF_EVENT_ISR_EXIT		=  0x11,
	CTF_EVENT_THREAD_CREATE		=  0x20,
	CTF_EVENT_THREAD_STACK_INFO	=  0x21,
	CTF_EVENT_THREAD_SWITCH_IN	=  0x22,
	CTF_EVENT_THREAD_SWITCH_OUT	=  0x23,
	CTF_EVENT_THREAD_READY		=  0x24,
	CTF_EVENT_THREAD_PENDING	=  0x25,
	CTF_EVENT_ID_END		=  0x30,
	CTF_EVENT_ID_START_VOID		=  0x31
} ctf_event_t;


typedef struct {
	char buf[20];
} ctf_bounded_string_t;


static inline void ctf_middle_isr_enter()
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ISR_ENTER)
	);
}

static inline void ctf_middle_isr_exit()
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ISR_EXIT)
	);
}

static inline void ctf_middle_on_idle(void)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ON_IDLE)
	);
}

static inline void ctf_middle_thread_switch_in()
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_SWITCH_IN)
	);
}

static inline void ctf_middle_thread_switch_out()
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_SWITCH_OUT)
	);
}

static inline void ctf_middle_thread_ready(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_READY),
		thread_id
	);
}

static inline void ctf_middle_thread_pending(u32_t thread_id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_PENDING),
		thread_id
	);
}

static inline void ctf_middle_thread_create(
	u32_t thread_id,
	s8_t  prio,
	ctf_bounded_string_t name
) {
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_CREATE),
		thread_id,
		name
	);
}

static inline void ctf_middle_thread_stack_info(
	u32_t thread_id,
	u32_t stack_base,
	u32_t stack_size
) {
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_THREAD_STACK_INFO),
		thread_id,
		stack_base,
		stack_size
	);
}

static inline void ctf_middle_start_void(u32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ID_START_VOID),
		id
	);
}

static inline void ctf_middle_end(u32_t id)
{
	CTF_EVENT(
		CTF_LITERAL(u8_t, CTF_EVENT_ID_END),
		id
	);
}

#endif /* _CTF_MIDDLE_H */
