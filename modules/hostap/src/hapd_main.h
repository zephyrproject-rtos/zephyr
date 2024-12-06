/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAPD_MAIN_H_
#define __HAPD_MAIN_H_

#include "common.h"
#include "wpa_debug_zephyr.h"

struct hostapd_iface *zephyr_get_hapd_handle_by_ifname(const char *ifname);
void wpa_supplicant_msg_send(void *ctx, int level, enum wpa_msg_type type, const char *txt,
			     size_t len);
void hostapd_msg_send(void *ctx, int level, enum wpa_msg_type type, const char *buf, size_t len);
const char *zephyr_hostap_msg_ifname_cb(void *ctx);
void zephyr_hostap_ctrl_iface_msg_cb(void *ctx, int level, enum wpa_msg_type type,
				     const char *txt, size_t len);
void zephyr_hostapd_init(struct hapd_interfaces *interfaces);
#endif /* __HAPD_MAIN_H_ */
