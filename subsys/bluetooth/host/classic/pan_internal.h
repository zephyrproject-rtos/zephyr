/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PAN_INTERNAL_H_
#define PAN_INTERNAL_H_

#include <zephyr/bluetooth/classic/pan.h>
#include "bnep_internal.h"

struct bt_pan {
	struct bt_bnep bnep;
	enum bt_pan_role role;
	const struct bt_pan_cb *cb;
#if defined(CONFIG_BT_PAN_NET)
	struct net_if *iface;
#endif
};

#endif /* PAN_INTERNAL_H_ */
