/**
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SOCKETS_H_
#define ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SOCKETS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern const struct socket_offload simplelink_ops;
extern void simplelink_sockets_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_SOCKETS_H_ */
