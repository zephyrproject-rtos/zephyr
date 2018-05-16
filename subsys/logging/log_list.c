/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_list.h"

void log_list_init(struct log_list_t *p_list)
{
	p_list->p_tail = NULL;
	p_list->p_head = NULL;
}

void log_list_add_tail(struct log_list_t *p_list, struct log_msg *p_msg)
{
	if (p_list->p_head == NULL) {
		p_list->p_head = p_msg;
	} else   {
		p_list->p_tail->p_next = p_msg;
	}
	p_list->p_tail = p_msg;
	p_msg->p_next = NULL;
}

struct log_msg *log_list_head_peek(struct log_list_t *p_list)
{
	return p_list->p_head;
}

struct log_msg *log_list_head_get(struct log_list_t *p_list)
{
	struct log_msg *p_msg = p_list->p_head;

	if (p_list->p_head != NULL) {
		p_list->p_head = p_list->p_head->p_next;
	}
	return p_msg;
}
