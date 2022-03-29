/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_CONN_MGR_H_
#define ZEPHYR_INCLUDE_NET_CONN_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NET_CONNECTION_MANAGER)

void net_conn_mgr_resend_status(void);

#else

#define net_conn_mgr_resend_status(...)

#endif /* CONFIG_NET_CONNECTION_MANAGER */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CONN_MGR_H_ */
