/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TRACE_CORE_H
#define ZEPHYR_INCLUDE_TRACE_CORE_H

#include <syscall.h>
#include <tracing_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tracing enabled or not.
 *
 * @return True if tracing enabled; False if tracing disabled.
 */
__syscall bool is_tracing_enabled(void);

#include <syscalls/tracing_core.h>

/**
 * @brief Add tracing packet to tracing list.
 *
 * @param packet Tracing packet.
 */
void tracing_list_add_packet(struct tracing_packet *packet);

/**
 * @brief Try to free one tracing list packet.
 *
 * Try to handle one packet already added to tracing list to
 * get free space in tracing packet pool. And it will also
 * tell caller if more packet is in tracing list after this
 * free.
 *
 * @return True if tracing list not empty; False if empty.
 */
bool tracing_packet_try_free(void);

/**
 * @brief Check if we are in tracing thread context.
 *
 * @return True if in tracing thread context; False if not.
 */
bool is_tracing_thread(void);

#ifdef __cplusplus
}
#endif

#endif
