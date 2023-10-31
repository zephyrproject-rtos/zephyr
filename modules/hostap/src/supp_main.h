/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

struct wpa_global *zephyr_get_default_supplicant_context(void);
struct wpa_supplicant *zephyr_get_handle_by_ifname(const char *ifname);
struct wpa_supplicant_event_msg {
	bool global;
	void *ctx;
	unsigned int event;
	void *data;
};
int zephyr_wifi_send_event(const struct wpa_supplicant_event_msg *msg);
#endif /* __SUPP_MAIN_H_ */
