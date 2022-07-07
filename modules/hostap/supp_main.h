/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SUPP_MAIN_H_
#define __SUPP_MAIN_H_

struct wpa_supplicant *z_wpas_get_handle_by_ifname(const char *ifname);

struct wpa_supplicant_event_msg {
	/* Dummy messages to unblock select */
	bool ignore_msg;
	void *ctx;
	unsigned int event;
	void *data;
};
int send_wpa_supplicant_event(const struct wpa_supplicant_event_msg *msg);
#endif /* __SUPP_MAIN_H_ */
