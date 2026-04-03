/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_OS_GR_STUB_
#define H_OS_GR_STUB_

#include <inttypes.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

void os_stub_init(const char *echo_str);
void os_reset_response(void);
void os_echo_verify(struct net_buf *nb);

#ifdef __cplusplus
}
#endif

#endif /* H_OS_GR_STUB_ */
