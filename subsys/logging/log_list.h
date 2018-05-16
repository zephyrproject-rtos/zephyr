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
	struct log_msg *p_head;
	struct log_msg *p_tail;
};

/** @brief Initialize log list instance.
 *
 * @param p_list List instance.
 */
void log_list_init(struct log_list_t *p_list);

/** @brief Add item to the tail of the list.
 *
 * @param p_list List instance.
 * @param p_msg  Message.
 *
 */
void log_list_add_tail(struct log_list_t *p_list, struct log_msg *p_msg);

/** @brief Remove item from the head of the list.
 *
 * @param p_list List instance.
 *
 * @return Message.
 */
struct log_msg *log_list_head_get(struct log_list_t *p_list);

/** @brief Peek item from the head of the list.
 *
 * @param p_list List instance.
 *
 * @return Message.
 */
struct log_msg *log_list_head_peek(struct log_list_t *p_list);

#ifdef __cplusplus
}
#endif

#endif /* LOG_LIST_H_ */
