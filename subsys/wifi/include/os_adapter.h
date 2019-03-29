/*
 * @file
 * @brief OS adapter
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_OS_ADAPTER_H_
#define _WIFIMGR_OS_ADAPTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <posix/posix_types.h>
#include <posix/pthread.h>
#include <posix/semaphore.h>
#include <posix/mqueue.h>

#include <misc/slist.h>

#define wifimgr_err(...)	LOG_ERR(__VA_ARGS__)
#define wifimgr_warn(...)	LOG_WRN(__VA_ARGS__)
#define wifimgr_info(...)	printk(__VA_ARGS__)
#define wifimgr_dbg(...)	LOG_DBG(__VA_ARGS__)

#define wifimgr_hexdump(...)	LOG_HEXDUMP_DBG(__VA_ARGS__, NULL)

/*
 * Copied from include/linux/...
 */

#define container_of(ptr, type, member) \
	CONTAINER_OF(ptr, type, member)

#define WIFIMGR_WORKQUEUE_STACK_SIZE	1024
typedef struct k_work_q wifimgr_workqueue;
typedef struct k_work wifimgr_work;

#define wifimgr_init_work(...)		k_work_init(__VA_ARGS__)
#define wifimgr_queue_work(...)	\
	k_work_submit_to_queue(__VA_ARGS__)
#define wifimgr_create_workqueue(work_q, work_q_stack)		\
	k_work_q_start(work_q, work_q_stack,			\
		       K_THREAD_STACK_SIZEOF(work_q_stack),	\
		       CONFIG_SYSTEM_WORKQUEUE_PRIORITY - 1)


typedef sys_snode_t wifimgr_snode_t;
typedef sys_slist_t wifimgr_slist_t;

#define wifimgr_list_init(list)			sys_slist_init(list)
#define wifimgr_list_peek_head(list)		sys_slist_peek_head(list)
#define wifimgr_list_peek_next(node)		sys_slist_peek_next(node)
#define wifimgr_list_prepend(list, node)	sys_slist_prepend(list, node)
#define wifimgr_list_append(list, node)		sys_slist_append(list, node)
#define wifimgr_list_merge(list_a, list_b) \
	sys_slist_merge_slist(list_a, list_b)
#define wifimgr_list_remove_first(list)		sys_slist_get(list)
#define wifimgr_list_remove(list, node) \
	sys_slist_find_and_remove(list, node)

static inline void wifimgr_list_free(wifimgr_slist_t *list)
{
	wifimgr_snode_t *node;

	do {
		node = wifimgr_list_remove_first(list);
		if (node)
			free(node);
	} while (node);
}

#define wifimgr_list_for_each_entry(pos, head, type, member)		       \
	for (pos = container_of(wifimgr_list_peek_head(head), type, member);   \
	     pos;							       \
	     pos =							       \
	     container_of(wifimgr_list_peek_next(&pos->member), type, member))

/**
 *
 * @brief Check whether the memory area is zeroed
 *
 * @return 0 if <m> == 0, else non-zero
 */
static inline int memiszero(const void *m, size_t n)
{
	const char *c = m;

	while ((--n > 0) && !(*c)) {
		c++;
	}

	return *c;
}

#endif
