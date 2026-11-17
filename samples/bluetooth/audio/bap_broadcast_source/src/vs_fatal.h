/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VS_FATAL_H_
#define VS_FATAL_H_

struct net_buf_simple;

/**
 * @brief Decode a Controller fatal error vendor-specific event.
 *
 * @param buf Vendor-specific event, starting with the subevent code.
 *
 * @retval true The event is a Controller fatal error event and was printed
 * @retval false The event is not a Controller fatal error event
 */
bool vs_fatal_vnd_evt(struct net_buf_simple *buf);

#endif /* VS_FATAL_H_ */
