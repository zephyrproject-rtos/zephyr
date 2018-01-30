/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_H
#define __WIFI_H

typedef int (*wifi_cb_t)(void *);

struct wifi_conn_params {
	char *ap_name;
	char *password;
	int security;
};

int wifi_init(struct wifi_conn_params *conn_params, wifi_cb_t cb, void *arg);

#endif /* __WIFI_H */
