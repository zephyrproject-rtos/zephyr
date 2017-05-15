/** @file
 * @brief Common routines needed in various network sample applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_SAMPLE_APP_H
#define __NET_SAMPLE_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* What kind of functionality is needed by the application. */
#define NET_SAMPLE_NEED_ROUTER 0x00000001
#define NET_SAMPLE_NEED_IPV6   0x00000002
#define NET_SAMPLE_NEED_IPV4   0x00000004

int net_sample_app_init(const char *app_info, u32_t flags, s32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* __NET_SAMPLE_APP_H */
