/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_list.h"

void log_list_init(struct log_list_t *list)
{
	list->tail = NULL;
	list->head = NULL;
}

void log_list_add_tail(struct log_list_t *list, struct log_msg *msg)
{
	if (list->head == NULL) {
		list->head = msg;
	} else {
		list->tail->next = msg;
	}

	list->tail = msg;
	msg->next = NULL;
}

struct log_msg *log_list_head_peek(struct log_list_t *list)
{
	return list->head;
}

struct log_msg *log_list_head_get(struct log_list_t *list)
{
	struct log_msg *msg = list->head;

	if (list->head != NULL) {
		list->head = list->head->next;
	}

	return msg;
}
