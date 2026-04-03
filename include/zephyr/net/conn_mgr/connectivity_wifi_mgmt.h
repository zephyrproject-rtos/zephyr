/*
 * Copyright (c) 2024 CSIRO
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Connectivity implementation for drivers exposing the wifi_mgmt API
 */

#ifndef ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_WIFI_MGMT_H_
#define ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_WIFI_MGMT_H_

#include <zephyr/net/conn_mgr_connectivity_impl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Context type for generic WIFI_MGMT connectivity backend.
 */
#define CONNECTIVITY_WIFI_MGMT_CTX_TYPE void *

/**
 * @brief Associate the generic WIFI_MGMT implementation with a network device
 *
 * @param dev_id Network device id.
 */
#define CONNECTIVITY_WIFI_MGMT_BIND(dev_id)				  \
	IF_ENABLED(CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT,  \
		   (CONN_MGR_CONN_DECLARE_PUBLIC(CONNECTIVITY_WIFI_MGMT); \
		    CONN_MGR_BIND_CONN(dev_id, CONNECTIVITY_WIFI_MGMT)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONN_MGR_CONNECTIVITY_WIFI_MGMT_H_ */
