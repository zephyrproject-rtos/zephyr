/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAPD_MAIN_H_
#define __HAPD_MAIN_H_

#include "common.h"

struct hostapd_iface *zephyr_get_hapd_handle_by_ifname(const char *ifname);
void zephyr_hostapd_init(struct hapd_interfaces *interfaces);
void zephyr_hostapd_msg(void *ctx, const char *txt, size_t len);
#endif /* __HAPD_MAIN_H_ */
