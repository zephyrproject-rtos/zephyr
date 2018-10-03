/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_LIST_H_
#define LOG_LIST_H_

#include <logging/log_msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief List instance structure. */
struct log_list_t {
	struct log_msg *head;
	struct log_msg *tail;
};

/** @brief Initialize log list instance.
 *
 * @param list List instance.
 */
void log_list_init(struct log_list_t *list);

/** @brief Add item to the tail of the list.
 *
 * @param list List instance.
 * @param msg  Message.
 *
 */
void log_list_add_tail(struct log_list_t *list, struct log_msg *msg);

/** @brief Remove item from the head of the list.
 *
 * @param list List instance.
 *
 * @return Message.
 */
struct log_msg *log_list_head_get(struct log_list_t *list);

/** @brief Peek item from the head of the list.
 *
 * @param list List instance.
 *
 * @return Message.
 */
struct log_msg *log_list_head_peek(struct log_list_t *list);

#ifdef __cplusplus
}
#endif

#endif /* LOG_LIST_H_ */
