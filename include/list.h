/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LIST_H_
#define ZEPHYR_INCLUDE_LIST_H_

#include <stdbool.h>
#include <sys/util.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct list_head;
struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

static inline void list_init(struct list_head *head) {
	head->prev = head;
	head->next = head;
}

static inline bool list_is_empty(struct list_head *head) {
	return head->prev == head && head->next == head;
}

static inline void list_del(struct list_head *iter) {

	struct list_head *prev;
	struct list_head *next;

	if (NULL == iter) {
		return;
	}

	prev = iter->prev;
	next = iter->next;

	if (NULL == prev || NULL == next) {
		return;
	}

	prev->next = next;
	next->prev = prev;

	iter->next = iter;
	iter->prev = iter;
}

static inline void list_add(struct list_head *head, struct list_head *iter) {
	struct list_head *tail;

	if (NULL == head || NULL == iter) {
		return;
	}

	if (list_is_empty(head)) {

		head->prev = iter;
		head->next = iter;
		iter->prev = head;
		iter->next = head;

		return;
	}

	tail = head->prev;
	tail->next = iter;

	head->prev = iter;

	iter->prev = tail;
	iter->next = head;
}

#define list_foreach(head, iter) \
	for(iter = (head)->next; iter != head; iter = (iter)->next)

#define list_foreach_safe(head, iter, iter_next) \
	for(iter = (head)->next, iter_next = iter->next; iter != head; iter = iter_next, iter_next = iter->next)

#define list_entry(iter, container, member) CONTAINER_OF(iter, container, member)

#define LIST_DECLARE(name) struct list_head name
#define LIST_INIT(head) { .prev = &(head), .next = &(head) }

#if defined(__cplusplus)
}
#endif

#endif /* ZEPHYR_INCLUDE_LIST_H_ */
