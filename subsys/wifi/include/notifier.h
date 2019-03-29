/*
 * @file
 * @brief Notifier chain header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_NOTIFIER_H_
#define _WIFI_NOTIFIER_H_

#include <net/wifi_api.h>

#include "os_adapter.h"

struct wifimgr_notifier {
	wifimgr_snode_t node;
	wifi_notifier_fn_t notifier_call;
};

struct wifimgr_notifier_chain {
	wifimgr_slist_t list;
	sem_t exclsem;		/* exclusive access to the struct */
};

int wifimgr_register_notifier(struct wifimgr_notifier_chain *chain,
			      wifi_notifier_fn_t notifier_call);
int wifimgr_unregister_notifier(struct wifimgr_notifier_chain *chain,
				wifi_notifier_fn_t notifier_call);

#endif

