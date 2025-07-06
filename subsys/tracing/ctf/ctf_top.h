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
#include <zephyr/tracing/tracing_format.h>
#include <zephyr/net/net_ip.h>

/* Limit strings to 20 bytes to optimize bandwidth */
#define CTF_MAX_STRING_LEN 20

/* Increase the string length to be able to store IPv4/6 address */
#if defined(CONFIG_NET_IPV6)
#define CTF_NET_MAX_STRING_LEN INET6_ADDRSTRLEN
#else
#define CTF_NET_MAX_STRING_LEN CTF_MAX_STRING_LEN
#endif

/*
 * Obtain a field's size at compile-time.
 */
#define CTF_INTERNAL_FIELD_SIZE(x) + sizeof(x)

/*
 * Append a field to current event-packet.
 */
#define CTF_INTERNAL_FIELD_APPEND(x)                                           \
	{                                                                      \
		memcpy(epacket_cursor, &(x), sizeof(x));                       \
		epacket_cursor += sizeof(x);                                   \
	}

/*
 * Gather fields to a contiguous event-packet, then atomically emit.
 */
#define CTF_GATHER_FIELDS(...)                                                  \
	{                                                                       \
		uint8_t epacket[0 MAP(CTF_INTERNAL_FIELD_SIZE, ##__VA_ARGS__)]; \
		uint8_t *epacket_cursor = &epacket[0];                          \
										\
		MAP(CTF_INTERNAL_FIELD_APPEND, ##__VA_ARGS__)                   \
		tracing_format_raw_data(epacket, sizeof(epacket));              \
	}

#ifdef CONFIG_TRACING_CTF_TIMESTAMP
#define CTF_EVENT(...)                                                         \
	{                                                                      \
		const uint32_t tstamp = k_cyc_to_ns_floor64(k_cycle_get_32()); \
									       \
		CTF_GATHER_FIELDS(tstamp, __VA_ARGS__)                         \
	}
#else
#define CTF_EVENT(...)                                                         \
	{                                                                      \
		CTF_GATHER_FIELDS(__VA_ARGS__)                                 \
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
#define CTF_LITERAL(type, value) ((type) { (type)(value) })

typedef enum {
	CTF_EVENT_THREAD_SWITCHED_OUT = 0x10,
	CTF_EVENT_THREAD_SWITCHED_IN = 0x11,
	CTF_EVENT_THREAD_PRIORITY_SET = 0x12,
	CTF_EVENT_THREAD_CREATE = 0x13,
	CTF_EVENT_THREAD_ABORT = 0x14,
	CTF_EVENT_THREAD_SUSPEND = 0x15,
	CTF_EVENT_THREAD_RESUME = 0x16,
	CTF_EVENT_THREAD_READY = 0x17,
	CTF_EVENT_THREAD_PENDING = 0x18,
	CTF_EVENT_THREAD_INFO = 0x19,
	CTF_EVENT_THREAD_NAME_SET = 0x1A,
	CTF_EVENT_ISR_ENTER = 0x1B,
	CTF_EVENT_ISR_EXIT = 0x1C,
	CTF_EVENT_ISR_EXIT_TO_SCHEDULER = 0x1D,
	CTF_EVENT_IDLE = 0x1E,
	CTF_EVENT_ID_START_CALL = 0x1F,
	CTF_EVENT_ID_END_CALL = 0x20,
	CTF_EVENT_SEMAPHORE_INIT = 0x21,
	CTF_EVENT_SEMAPHORE_GIVE_ENTER = 0x22,
	CTF_EVENT_SEMAPHORE_GIVE_EXIT = 0x23,
	CTF_EVENT_SEMAPHORE_TAKE_ENTER = 0x24,
	CTF_EVENT_SEMAPHORE_TAKE_BLOCKING = 0x25,
	CTF_EVENT_SEMAPHORE_TAKE_EXIT = 0x26,
	CTF_EVENT_SEMAPHORE_RESET = 0x27,
	CTF_EVENT_MUTEX_INIT = 0x28,
	CTF_EVENT_MUTEX_LOCK_ENTER = 0x29,
	CTF_EVENT_MUTEX_LOCK_BLOCKING = 0x2A,
	CTF_EVENT_MUTEX_LOCK_EXIT = 0x2B,
	CTF_EVENT_MUTEX_UNLOCK_ENTER = 0x2C,
	CTF_EVENT_MUTEX_UNLOCK_EXIT = 0x2D,
	CTF_EVENT_TIMER_INIT = 0x2E,
	CTF_EVENT_TIMER_START = 0x2F,
	CTF_EVENT_TIMER_STOP = 0x30,
	CTF_EVENT_TIMER_STATUS_SYNC_ENTER = 0x31,
	CTF_EVENT_TIMER_STATUS_SYNC_BLOCKING = 0x32,
	CTF_EVENT_TIMER_STATUS_SYNC_EXIT = 0x33,
	CTF_EVENT_THREAD_USER_MODE_ENTER = 0x34,
	CTF_EVENT_THREAD_WAKEUP = 0x35,
	CTF_EVENT_SOCKET_INIT = 0x36,
	CTF_EVENT_SOCKET_CLOSE_ENTER = 0x37,
	CTF_EVENT_SOCKET_CLOSE_EXIT = 0x38,
	CTF_EVENT_SOCKET_SHUTDOWN_ENTER = 0x39,
	CTF_EVENT_SOCKET_SHUTDOWN_EXIT = 0x3A,
	CTF_EVENT_SOCKET_BIND_ENTER = 0x3B,
	CTF_EVENT_SOCKET_BIND_EXIT = 0x3C,
	CTF_EVENT_SOCKET_CONNECT_ENTER = 0x3D,
	CTF_EVENT_SOCKET_CONNECT_EXIT = 0x3E,
	CTF_EVENT_SOCKET_LISTEN_ENTER = 0x3F,
	CTF_EVENT_SOCKET_LISTEN_EXIT = 0x40,
	CTF_EVENT_SOCKET_ACCEPT_ENTER = 0x41,
	CTF_EVENT_SOCKET_ACCEPT_EXIT = 0x42,
	CTF_EVENT_SOCKET_SENDTO_ENTER = 0x43,
	CTF_EVENT_SOCKET_SENDTO_EXIT = 0x44,
	CTF_EVENT_SOCKET_SENDMSG_ENTER = 0x45,
	CTF_EVENT_SOCKET_SENDMSG_EXIT = 0x46,
	CTF_EVENT_SOCKET_RECVFROM_ENTER = 0x47,
	CTF_EVENT_SOCKET_RECVFROM_EXIT = 0x48,
	CTF_EVENT_SOCKET_RECVMSG_ENTER = 0x49,
	CTF_EVENT_SOCKET_RECVMSG_EXIT = 0x4A,
	CTF_EVENT_SOCKET_FCNTL_ENTER = 0x4B,
	CTF_EVENT_SOCKET_FCNTL_EXIT = 0x4C,
	CTF_EVENT_SOCKET_IOCTL_ENTER = 0x4D,
	CTF_EVENT_SOCKET_IOCTL_EXIT = 0x4E,
	CTF_EVENT_SOCKET_POLL_ENTER = 0x4F,
	CTF_EVENT_SOCKET_POLL_VALUE = 0x50,
	CTF_EVENT_SOCKET_POLL_EXIT = 0x51,
	CTF_EVENT_SOCKET_GETSOCKOPT_ENTER = 0x52,
	CTF_EVENT_SOCKET_GETSOCKOPT_EXIT = 0x53,
	CTF_EVENT_SOCKET_SETSOCKOPT_ENTER = 0x54,
	CTF_EVENT_SOCKET_SETSOCKOPT_EXIT = 0x55,
	CTF_EVENT_SOCKET_GETPEERNAME_ENTER = 0x56,
	CTF_EVENT_SOCKET_GETPEERNAME_EXIT = 0x57,
	CTF_EVENT_SOCKET_GETSOCKNAME_ENTER = 0x58,
	CTF_EVENT_SOCKET_GETSOCKNAME_EXIT = 0x59,
	CTF_EVENT_SOCKET_SOCKETPAIR_ENTER = 0x5A,
	CTF_EVENT_SOCKET_SOCKETPAIR_EXIT = 0x5B,
	CTF_EVENT_NET_RECV_DATA_ENTER = 0x5C,
	CTF_EVENT_NET_RECV_DATA_EXIT = 0x5D,
	CTF_EVENT_NET_SEND_DATA_ENTER = 0x5E,
	CTF_EVENT_NET_SEND_DATA_EXIT = 0x5F,
	CTF_EVENT_NET_RX_TIME = 0x60,
	CTF_EVENT_NET_TX_TIME = 0x61,
	CTF_EVENT_NAMED_EVENT = 0x62,
	CTF_EVENT_GPIO_PIN_CONFIGURE_INTERRUPT_ENTER = 0x63,
	CTF_EVENT_GPIO_PIN_CONFIGURE_INTERRUPT_EXIT = 0x64,
	CTF_EVENT_GPIO_PIN_CONFIGURE_ENTER = 0x65,
	CTF_EVENT_GPIO_PIN_CONFIGURE_EXIT = 0x66,
	CTF_EVENT_GPIO_PORT_GET_DIRECTION_ENTER = 0x67,
	CTF_EVENT_GPIO_PORT_GET_DIRECTION_EXIT = 0x68,
	CTF_EVENT_GPIO_PIN_GET_CONFIG_ENTER = 0x69,
	CTF_EVENT_GPIO_PIN_GET_CONFIG_EXIT = 0x6A,
	CTF_EVENT_GPIO_PORT_GET_RAW_ENTER = 0x6B,
	CTF_EVENT_GPIO_PORT_GET_RAW_EXIT = 0x6C,
	CTF_EVENT_GPIO_PORT_SET_MASKED_RAW_ENTER = 0x6D,
	CTF_EVENT_GPIO_PORT_SET_MASKED_RAW_EXIT = 0x6E,
	CTF_EVENT_GPIO_PORT_SET_BITS_RAW_ENTER = 0x6F,
	CTF_EVENT_GPIO_PORT_SET_BITS_RAW_EXIT = 0x70,
	CTF_EVENT_GPIO_PORT_CLEAR_BITS_RAW_ENTER = 0x71,
	CTF_EVENT_GPIO_PORT_CLEAR_BITS_RAW_EXIT = 0x72,
	CTF_EVENT_GPIO_PORT_TOGGLE_BITS_ENTER = 0x73,
	CTF_EVENT_GPIO_PORT_TOGGLE_BITS_EXIT = 0x74,
	CTF_EVENT_GPIO_INIT_CALLBACK_ENTER = 0x75,
	CTF_EVENT_GPIO_INIT_CALLBACK_EXIT = 0x76,
	CTF_EVENT_GPIO_ADD_CALLBACK_ENTER = 0x77,
	CTF_EVENT_GPIO_ADD_CALLBACK_EXIT = 0x78,
	CTF_EVENT_GPIO_REMOVE_CALLBACK_ENTER = 0x79,
	CTF_EVENT_GPIO_REMOVE_CALLBACK_EXIT = 0x7A,
	CTF_EVENT_GPIO_GET_PENDING_INT_ENTER = 0x7B,
	CTF_EVENT_GPIO_GET_PENDING_INT_EXIT = 0x7C,
	CTF_EVENT_GPIO_FIRE_CALLBACKS_ENTER = 0x7D,
	CTF_EVENT_GPIO_FIRE_CALLBACK = 0x7E,
} ctf_event_t;

typedef struct {
	char buf[CTF_MAX_STRING_LEN];
} ctf_bounded_string_t;

static inline void ctf_top_thread_switched_out(uint32_t thread_id,
					       ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SWITCHED_OUT),
		  thread_id, name);
}

static inline void ctf_top_thread_switched_in(uint32_t thread_id,
					      ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SWITCHED_IN), thread_id,
		  name);
}

static inline void ctf_top_thread_priority_set(uint32_t thread_id, int8_t prio,
					       ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_PRIORITY_SET),
		  thread_id, name, prio);
}

static inline void ctf_top_thread_create(uint32_t thread_id, int8_t prio,
					 ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_CREATE), thread_id,
		  name);
}

static inline void ctf_top_thread_abort(uint32_t thread_id,
					ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_ABORT), thread_id,
		  name);
}

static inline void ctf_top_thread_suspend(uint32_t thread_id,
					  ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_SUSPEND), thread_id,
		  name);
}

static inline void ctf_top_thread_resume(uint32_t thread_id,
					 ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_RESUME), thread_id,
		  name);
}

static inline void ctf_top_thread_ready(uint32_t thread_id,
					ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_READY), thread_id,
		  name);
}

static inline void ctf_top_thread_pend(uint32_t thread_id,
				       ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_PENDING), thread_id,
		  name);
}

static inline void ctf_top_thread_info(uint32_t thread_id,
				       ctf_bounded_string_t name,
				       uint32_t stack_base, uint32_t stack_size)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_INFO), thread_id, name,
		  stack_base, stack_size);
}

static inline void ctf_top_thread_name_set(uint32_t thread_id,
					   ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_NAME_SET), thread_id,
		  name);
}


static inline void ctf_top_thread_user_mode_enter(uint32_t thread_id, ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_USER_MODE_ENTER),
		  thread_id, name);
}

static inline void ctf_top_thread_wakeup(uint32_t thread_id, ctf_bounded_string_t name)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_THREAD_WAKEUP),
		  thread_id, name);
}

static inline void ctf_top_isr_enter(void)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_ISR_ENTER));
}

static inline void ctf_top_isr_exit(void)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_ISR_EXIT));
}

static inline void ctf_top_isr_exit_to_scheduler(void)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_ISR_EXIT_TO_SCHEDULER));
}

static inline void ctf_top_idle(void)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_IDLE));
}

static inline void ctf_top_void(uint32_t id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_ID_START_CALL), id);
}

static inline void ctf_top_end_call(uint32_t id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_ID_END_CALL), id);
}

/* Semaphore */
static inline void ctf_top_semaphore_init(uint32_t sem_id,
					  int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_INIT), sem_id, ret);
}

static inline void ctf_top_semaphore_reset(uint32_t sem_id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_RESET), sem_id);
}

static inline void ctf_top_semaphore_take_enter(uint32_t sem_id,
						uint32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_TAKE_ENTER), sem_id,
		  timeout);
}

static inline void ctf_top_semaphore_take_blocking(uint32_t sem_id,
						   uint32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_TAKE_BLOCKING),
		  sem_id, timeout);
}

static inline void ctf_top_semaphore_take_exit(uint32_t sem_id,
					       uint32_t timeout, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_TAKE_EXIT), sem_id,
		  timeout, ret);
}

static inline void ctf_top_semaphore_give_enter(uint32_t sem_id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_GIVE_ENTER), sem_id);
}

static inline void ctf_top_semaphore_give_exit(uint32_t sem_id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SEMAPHORE_GIVE_EXIT), sem_id);
}

/* Mutex */
static inline void ctf_top_mutex_init(uint32_t mutex_id, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_INIT), mutex_id, ret);
}

static inline void ctf_top_mutex_lock_enter(uint32_t mutex_id, uint32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_LOCK_ENTER), mutex_id,
		  timeout);
}

static inline void ctf_top_mutex_lock_blocking(uint32_t mutex_id,
					       uint32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_LOCK_BLOCKING), mutex_id,
		  timeout);
}

static inline void ctf_top_mutex_lock_exit(uint32_t mutex_id, uint32_t timeout,
					   int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_LOCK_EXIT), mutex_id,
		  timeout, ret);
}

static inline void ctf_top_mutex_unlock_enter(uint32_t mutex_id)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_UNLOCK_ENTER), mutex_id);
}

static inline void ctf_top_mutex_unlock_exit(uint32_t mutex_id, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_MUTEX_UNLOCK_EXIT), mutex_id);
}

/* Timer */
static inline void ctf_top_timer_init(uint32_t timer)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_INIT), timer);
}

static inline void ctf_top_timer_start(uint32_t timer, uint32_t duration, uint32_t period)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_START), timer, duration, period);
}

static inline void ctf_top_timer_stop(uint32_t timer)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_STOP), timer);
}

static inline void ctf_top_timer_status_sync_enter(uint32_t timer)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_STATUS_SYNC_ENTER), timer);
}

static inline void ctf_top_timer_status_sync_blocking(uint32_t timer, uint32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_STATUS_SYNC_BLOCKING), timer, timeout);
}

static inline void ctf_top_timer_status_sync_exit(uint32_t timer, uint32_t result)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_TIMER_STATUS_SYNC_EXIT), timer, result);
}

/* Network socket */
typedef struct {
	char buf[CTF_NET_MAX_STRING_LEN];
} ctf_net_bounded_string_t;

static inline void ctf_top_socket_init(int32_t sock, uint32_t family,
				       uint32_t type, uint32_t proto)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_INIT), sock, family, type, proto);
}

static inline void ctf_top_socket_close_enter(int32_t sock)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_CLOSE_ENTER), sock);
}

static inline void ctf_top_socket_close_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_CLOSE_EXIT), sock, ret);
}

static inline void ctf_top_socket_shutdown_enter(int32_t sock, int32_t how)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SHUTDOWN_ENTER), sock, how);
}

static inline void ctf_top_socket_shutdown_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SHUTDOWN_EXIT), sock, ret);
}

static inline void ctf_top_socket_bind_enter(int32_t sock, ctf_net_bounded_string_t addr,
					     uint32_t addrlen, uint16_t port)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_BIND_ENTER), sock, addr, addrlen, port);
}

static inline void ctf_top_socket_bind_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_BIND_EXIT), sock, ret);
}

static inline void ctf_top_socket_connect_enter(int32_t sock,
						ctf_net_bounded_string_t addr,
						uint32_t addrlen)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_CONNECT_ENTER), sock, addr, addrlen);
}

static inline void ctf_top_socket_connect_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_CONNECT_EXIT), sock, ret);
}

static inline void ctf_top_socket_listen_enter(int32_t sock, uint32_t backlog)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_LISTEN_ENTER), sock, backlog);
}

static inline void ctf_top_socket_listen_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_LISTEN_EXIT), sock, ret);
}

static inline void ctf_top_socket_accept_enter(int32_t sock)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_ACCEPT_ENTER), sock);
}

static inline void ctf_top_socket_accept_exit(int32_t sock, ctf_net_bounded_string_t addr,
					      uint32_t addrlen, uint16_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_ACCEPT_EXIT), sock, addr, addrlen,
		  port, ret);
}

static inline void ctf_top_socket_sendto_enter(int32_t sock, uint32_t len, uint32_t flags,
					       ctf_net_bounded_string_t addr, uint32_t addrlen)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SENDTO_ENTER), sock, len, flags,
		  addr, addrlen);
}

static inline void ctf_top_socket_sendto_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SENDTO_EXIT), sock, ret);
}

static inline void ctf_top_socket_sendmsg_enter(int32_t sock, uint32_t flags, uint32_t msg,
						ctf_net_bounded_string_t addr, uint32_t len)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SENDMSG_ENTER), sock, flags, msg,
		  addr, len);
}

static inline void ctf_top_socket_sendmsg_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SENDMSG_EXIT), sock, ret);
}

static inline void ctf_top_socket_recvfrom_enter(int32_t sock, uint32_t max_len, uint32_t flags,
						 uint32_t addr, uint32_t addrlen)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_RECVFROM_ENTER), sock, max_len, flags,
		  addr, addrlen);
}

static inline void ctf_top_socket_recvfrom_exit(int32_t sock, ctf_net_bounded_string_t addr,
						uint32_t addrlen, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_RECVFROM_EXIT), sock, addr, addrlen, ret);
}

static inline void ctf_top_socket_recvmsg_enter(int32_t sock, uint32_t msg, uint32_t max_len,
						uint32_t flags)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_RECVMSG_ENTER), sock, msg, max_len, flags);
}

static inline void ctf_top_socket_recvmsg_exit(int32_t sock, uint32_t len,
					       ctf_net_bounded_string_t addr, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_RECVMSG_EXIT), sock, len, addr, ret);
}

static inline void ctf_top_socket_fcntl_enter(int32_t sock, uint32_t cmd, uint32_t flags)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_FCNTL_ENTER), sock, cmd, flags);
}

static inline void ctf_top_socket_fcntl_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_FCNTL_EXIT), sock, ret);
}

static inline void ctf_top_socket_ioctl_enter(int32_t sock, uint32_t req)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_IOCTL_ENTER), sock, req);
}

static inline void ctf_top_socket_ioctl_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_IOCTL_EXIT), sock, ret);
}

static inline void ctf_top_socket_poll_enter(uint32_t fds, uint32_t nfds, int32_t timeout)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_POLL_ENTER), fds, nfds, timeout);
}

static inline void ctf_top_socket_poll_value(int32_t fd, uint16_t flag)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_POLL_VALUE), fd, flag);
}

static inline void ctf_top_socket_poll_exit(uint32_t fds, int nfds, int ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_POLL_EXIT), fds, nfds, ret);
}

static inline void ctf_top_socket_getsockopt_enter(int32_t sock, uint32_t level, uint32_t optname)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETSOCKOPT_ENTER), sock, level, optname);
}

static inline void ctf_top_socket_getsockopt_exit(int32_t sock, uint32_t level, uint32_t optname,
						  uint32_t optval, uint32_t optlen, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETSOCKOPT_EXIT),
		  sock, level, optname, optval, optlen, ret);
}

static inline void ctf_top_socket_setsockopt_enter(int32_t sock, uint32_t level, uint32_t optname,
						   uint32_t optval, uint32_t optlen)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SETSOCKOPT_ENTER), sock, level,
		  optname, optval, optlen);
}

static inline void ctf_top_socket_setsockopt_exit(int32_t sock, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SETSOCKOPT_EXIT), sock, ret);
}

static inline void ctf_top_socket_getpeername_enter(int32_t sock)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETPEERNAME_ENTER), sock);
}

static inline void ctf_top_socket_getpeername_exit(int32_t sock, ctf_net_bounded_string_t addr,
						   uint32_t addrlen, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETPEERNAME_EXIT),
		  sock, addr, addrlen, ret);
}

static inline void ctf_top_socket_getsockname_enter(int32_t sock)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETSOCKNAME_ENTER), sock);
}

static inline void ctf_top_socket_getsockname_exit(int32_t sock, ctf_net_bounded_string_t addr,
						   uint32_t addrlen, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_GETSOCKNAME_EXIT),
		  sock, addr, addrlen, ret);
}

static inline void ctf_top_socket_socketpair_enter(uint32_t family, uint32_t type,
						   uint32_t proto, uint32_t sv)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SOCKETPAIR_ENTER), family, type,
		  proto, sv);
}

static inline void ctf_top_socket_socketpair_exit(int32_t sock_A, int32_t sock_B, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_SOCKET_SOCKETPAIR_EXIT), sock_A, sock_B, ret);
}

static inline void ctf_top_net_recv_data_enter(int32_t if_index, uint32_t iface, uint32_t pkt,
					       uint32_t len)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_RECV_DATA_ENTER),
		  if_index, iface, pkt, len);
}

static inline void ctf_top_net_recv_data_exit(int32_t if_index, uint32_t iface, uint32_t pkt,
					      int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_RECV_DATA_EXIT),
		  if_index, iface, pkt, ret);
}

static inline void ctf_top_net_send_data_enter(int32_t if_index, uint32_t iface, uint32_t pkt,
					       uint32_t len)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_SEND_DATA_ENTER),
		  if_index, iface, pkt, len);
}

static inline void ctf_top_net_send_data_exit(int32_t if_index, uint32_t iface, uint32_t pkt,
					      int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_SEND_DATA_EXIT),
		  if_index, iface, pkt, ret);
}

static inline void ctf_top_net_rx_time(int32_t if_index, uint32_t iface, uint32_t pkt,
				       uint32_t priority, uint32_t tc, uint32_t duration)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_RX_TIME),
		  if_index, iface, pkt, priority, tc, duration);
}

static inline void ctf_top_net_tx_time(int32_t if_index, uint32_t iface, uint32_t pkt,
				       uint32_t priority, uint32_t tc, uint32_t duration)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NET_TX_TIME),
		  if_index, iface, pkt, priority, tc, duration);
}

static inline void ctf_named_event(ctf_bounded_string_t name, uint32_t arg0,
				   uint32_t arg1)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_NAMED_EVENT), name,
		  arg0, arg1);
}

/* GPIO */
static inline void ctf_top_gpio_pin_interrupt_configure_enter(uint32_t port, uint32_t pin,
							      uint32_t flags)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_CONFIGURE_INTERRUPT_ENTER), port, pin,
		  flags);
}

static inline void ctf_top_gpio_pin_interrupt_configure_exit(uint32_t port, uint32_t pin,
							     int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_CONFIGURE_INTERRUPT_EXIT), port, pin,
		  ret);
}

static inline void ctf_top_gpio_pin_configure_enter(uint32_t port, uint32_t pin, uint32_t flags)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_CONFIGURE_ENTER), port, pin, flags);
}

static inline void ctf_top_gpio_pin_configure_exit(uint32_t port, uint32_t pin, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_CONFIGURE_EXIT), port, pin, ret);
}

static inline void ctf_top_gpio_port_get_direction_enter(uint32_t port, uint32_t map,
							 uint32_t inputs, uint32_t outputs)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_GET_DIRECTION_ENTER), port, map, inputs,
		  outputs);
}

static inline void ctf_top_gpio_port_get_direction_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_GET_DIRECTION_EXIT), port, ret);
}

static inline void ctf_top_gpio_pin_get_config_enter(uint32_t port, uint32_t pin, uint32_t flags)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_GET_CONFIG_ENTER), port, pin, flags);
}

static inline void ctf_top_gpio_pin_get_config_exit(uint32_t port, uint32_t pin, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PIN_GET_CONFIG_EXIT), port, pin, ret);
}

static inline void ctf_top_gpio_port_get_raw_enter(uint32_t port, uint32_t value)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_GET_RAW_ENTER), port, value);
}

static inline void ctf_top_gpio_port_get_raw_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_GET_RAW_EXIT), port, ret);
}

static inline void ctf_top_gpio_port_set_masked_raw_enter(uint32_t port, uint32_t mask,
							  uint32_t value)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_SET_MASKED_RAW_ENTER), port, mask,
		  value);
}

static inline void ctf_top_gpio_port_set_masked_raw_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_SET_MASKED_RAW_EXIT), port, ret);
}

static inline void ctf_top_gpio_port_set_bits_raw_enter(uint32_t port, uint32_t pins)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_SET_BITS_RAW_ENTER), port, pins);
}

static inline void ctf_top_gpio_port_set_bits_raw_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_SET_BITS_RAW_EXIT), port, ret);
}

static inline void ctf_top_gpio_port_clear_bits_raw_enter(uint32_t port, uint32_t pins)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_CLEAR_BITS_RAW_ENTER), port, pins);
}

static inline void ctf_top_gpio_port_clear_bits_raw_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_CLEAR_BITS_RAW_EXIT), port, ret);
}

static inline void ctf_top_gpio_port_toggle_bits_enter(uint32_t port, uint32_t pins)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_TOGGLE_BITS_ENTER), port, pins);
}

static inline void ctf_top_gpio_port_toggle_bits_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_PORT_TOGGLE_BITS_EXIT), port, ret);
}

static inline void ctf_top_gpio_init_callback_enter(uint32_t callback, uint32_t handler,
						    uint32_t pin_mask)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_INIT_CALLBACK_ENTER), callback, handler,
		  pin_mask);
}

static inline void ctf_top_gpio_init_callback_exit(uint32_t callback)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_INIT_CALLBACK_EXIT), callback);
}

static inline void ctf_top_gpio_add_callback_enter(uint32_t port, uint32_t callback)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_ADD_CALLBACK_ENTER), port, callback);
}

static inline void ctf_top_gpio_add_callback_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_ADD_CALLBACK_EXIT), port, ret);
}

static inline void ctf_top_gpio_remove_callback_enter(uint32_t port, uint32_t callback)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_REMOVE_CALLBACK_ENTER), port, callback);
}

static inline void ctf_top_gpio_remove_callback_exit(uint32_t port, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_REMOVE_CALLBACK_EXIT), port, ret);
}

static inline void ctf_top_gpio_get_pending_int_enter(uint32_t dev)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_GET_PENDING_INT_ENTER), dev);
}

static inline void ctf_top_gpio_get_pending_int_exit(uint32_t dev, int32_t ret)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_GET_PENDING_INT_EXIT), dev, ret);
}

static inline void ctf_top_gpio_fire_callbacks_enter(uint32_t list, uint32_t port, uint32_t pins)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_FIRE_CALLBACKS_ENTER), list, port, pins);
}

static inline void ctf_top_gpio_fire_callback(uint32_t port, uint32_t cb)
{
	CTF_EVENT(CTF_LITERAL(uint8_t, CTF_EVENT_GPIO_FIRE_CALLBACK), port, cb);
}

#endif /* SUBSYS_DEBUG_TRACING_CTF_TOP_H */
