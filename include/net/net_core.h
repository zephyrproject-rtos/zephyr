/** @file
 * @brief Network core definitions
 *
 * Definitions for networking support.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CORE_H_
#define ZEPHYR_INCLUDE_NET_NET_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Networking
 * @defgroup networking Networking
 * @{
 * @}
 */

/**
 * @brief Network core library
 * @defgroup net_core Network Core Library
 * @ingroup networking
 * @{
 */

/* Network subsystem logging helpers */

#if defined(NET_LOG_ENABLED)
#if !defined(SYS_LOG_DOMAIN)
#define SYS_LOG_DOMAIN "net"
#endif /* !SYS_LOG_DOMAIN */

#undef SYS_LOG_LEVEL
#ifndef NET_SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_NET_LEVEL
#else
#define SYS_LOG_LEVEL NET_SYS_LOG_LEVEL
#endif /* !NET_SYS_LOG_LEVEL */

#define NET_DBG(fmt, ...) SYS_LOG_DBG("(%p): " fmt, k_current_get(), \
				      ##__VA_ARGS__)
#define NET_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...) SYS_LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_INFO(fmt, ...) SYS_LOG_INF(fmt,  ##__VA_ARGS__)
#define NET_ASSERT(cond) do {				     \
		if (!(cond)) {					     \
			NET_ERR("{assert: '" #cond "' failed}");     \
		} } while (0)
#define NET_ASSERT_INFO(cond, fmt, ...) do {			     \
		if (!(cond)) {					     \
			NET_ERR("{assert: '" #cond "' failed} " fmt, \
				##__VA_ARGS__);			     \
		} } while (0)
#else /* NET_LOG_ENABLED */
#define NET_DBG(...)
#define NET_ERR(...)
#define NET_INFO(...)
#define NET_WARN(...)
#define NET_ASSERT(...)
#define NET_ASSERT_INFO(...)
#endif /* NET_LOG_ENABLED */

#include <kernel.h>

struct net_buf;
struct net_pkt;
struct net_context;
struct net_if;

#include <logging/sys_log.h>
#include <string.h>

/**
 * @brief Net Verdict
 */
enum net_verdict {
	NET_OK,		/** Packet has been taken care of */
	NET_CONTINUE,	/** Packet has not been touched,
			    other part should decide about its fate */
	NET_DROP,	/** Packet must be dropped */
};

/* Called by lower network stack when a network packet has been received */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Send data to network.
 *
 * @details Send data to network. This should not be used normally by
 * applications as it requires that the pktfer and fragments are properly
 * constructed.
 *
 * @param pkt Network packet.
 *
 * @return 0 if ok, <0 if error. If <0 is returned, then the caller needs
 * to unref the pkt in order to avoid memory leak.
 */
int net_send_data(struct net_pkt *pkt);

/*
 * The net_stack_info struct needs to be aligned to 32 byte boundary,
 * otherwise the __net_stack_end will point to wrong location and looping
 * through net_stack section will go wrong.
 * So this alignment is a workaround and should eventually be removed.
 */
struct net_stack_info {
	k_thread_stack_t *stack;
	const char *pretty_name;
	const char *name;
	size_t orig_size;
	size_t size;
	int prio;
	int idx;
} __aligned(32);

#if defined(CONFIG_NET_SHELL)
#define NET_STACK_GET_NAME(pretty, name, sfx) \
	(__net_stack_##pretty##_##name##_##sfx)

#define NET_STACK_INFO_ADDR(_pretty, _name, _orig, _size, _addr, sfx)	\
	static struct net_stack_info					\
	(NET_STACK_GET_NAME(_pretty, _name, sfx)) __used		\
	__attribute__((__section__(".net_stack.data"))) = {		\
		.stack = _addr,						\
		.size = _size,						\
		.orig_size = _orig,					\
		.name = #_name,						\
		.pretty_name = #_pretty,				\
		.prio = -1,						\
		.idx = -1,						\
	}

/* Note that the stack address needs to be fixed at runtime because
 * we cannot do it during static initialization. For name we allocate
 * some space so that the stack index can be printed too.
 */
#define NET_STACK_INFO_ADDR_ARRAY(_pretty, _name, _orig, _size, _addr,	\
				  sfx, _nmemb)				\
	static struct net_stack_info					\
	(NET_STACK_GET_NAME(_pretty, _name, sfx))[_nmemb] __used	\
	__attribute__((__section__(".net_stack.data"))) = {		\
		[0 ... (_nmemb - 1)] = {				\
			.stack = _addr[0],				\
			.size = _size,					\
			.orig_size = _orig,				\
			.name = #_name,					\
			.pretty_name = #_pretty,			\
			.prio = -1,					\
			.idx = -1,					\
		}							\
	}

#define NET_STACK_INFO(_pretty_name, _name, _orig, _size)		\
	NET_STACK_INFO_ADDR(_pretty_name, _name, _orig, _size, _name, 0)

#define NET_STACK_INFO_ARRAY(_pretty_name, _name, _orig, _size, _nmemb)	 \
	NET_STACK_INFO_ADDR_ARRAY(_pretty_name, _name, _orig, _size, _name, \
				  0, _nmemb)

#define NET_STACK_DEFINE(pretty_name, name, orig, size)			\
	K_THREAD_STACK_DEFINE(name, size);				\
	NET_STACK_INFO(pretty_name, name, orig, size)

#define NET_STACK_ARRAY_DEFINE(pretty_name, name, orig, size, nmemb)	\
	K_THREAD_STACK_ARRAY_DEFINE(name, nmemb, size);			\
	NET_STACK_INFO_ARRAY(pretty_name, name, orig, size, nmemb)

#else /* CONFIG_NET_SHELL */

#define NET_STACK_GET_NAME(pretty, name, sfx) (name)

#define NET_STACK_INFO(...)
#define NET_STACK_INFO_ADDR(...)

#define NET_STACK_DEFINE(pretty_name, name, orig, size)			\
	K_THREAD_STACK_DEFINE(name, size)

#define NET_STACK_ARRAY_DEFINE(pretty_name, name, orig, size, nmemb)	\
	K_THREAD_STACK_ARRAY_DEFINE(name, nmemb, size)

#endif /* CONFIG_NET_SHELL */

#define NET_STACK_DEFINE_EMBEDDED(name, size) char name[size]

/** @cond ignore */
#if defined(CONFIG_INIT_STACKS)
#include <misc/stack.h>

static inline void net_analyze_stack_get_values(const char *stack,
						size_t size,
						unsigned *pcnt,
						unsigned *unused)
{
	*unused = stack_unused_space_get(stack, size);

	/* Calculate the real size reserved for the stack */
	*pcnt = ((size - *unused) * 100) / size;
}

static inline void net_analyze_stack(const char *name,
				     const char *stack,
				     size_t size)
{
	unsigned int pcnt, unused;

	net_analyze_stack_get_values(stack, size, &pcnt, &unused);

	NET_INFO("net (%p): %s stack real size %zu "
		 "unused %u usage %zu/%zu (%u %%)",
		 k_current_get(), name,
		 size, unused, size - unused, size, pcnt);
}
#else
#define net_analyze_stack(...)
#define net_analyze_stack_get_values(...)
#endif
/* @endcond */

/* Some helper defines for traffic class support */
#if defined(CONFIG_NET_TC_TX_COUNT) && defined(CONFIG_NET_TC_RX_COUNT)
#define NET_TC_TX_COUNT CONFIG_NET_TC_TX_COUNT
#define NET_TC_RX_COUNT CONFIG_NET_TC_RX_COUNT

#if NET_TC_TX_COUNT > NET_TC_RX_COUNT
#define NET_TC_COUNT NET_TC_TX_COUNT
#else
#define NET_TC_COUNT NET_TC_RX_COUNT
#endif
#else /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */
#define NET_TC_TX_COUNT 1
#define NET_TC_RX_COUNT 1
#define NET_TC_COUNT 1
#endif /* CONFIG_NET_TC_TX_COUNT && CONFIG_NET_TC_RX_COUNT */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CORE_H_ */
