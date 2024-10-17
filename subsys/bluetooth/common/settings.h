/*
 * Copyright (c) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_COMMON_SETTINGS_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_COMMON_SETTINGS_H_

#include <zephyr/settings/settings.h>

/* Max settings key length (with all components) */
#define BT_SETTINGS_KEY_MAX 36

#define BT_SETTINGS_PRIO_BT	0
#define BT_SETTINGS_PRIO_AFT_BT 1

/* Base64-encoded string buffer size of in_size bytes */
#define BT_SETTINGS_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

#define _BT_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)				 \
	SETTINGS_STATIC_HANDLER_DEFINE(_prio##_##_hname, _subtree, NULL, _set, _commit, NULL)

#define _BT_SUBSYS_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)			 \
	_BT_SETTINGS_DEFINE(_prio, bt##_##_hname, "bt/" _subtree, _set, _commit)

#define BT_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)				 \
	_BT_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)

#define BT_SUBSYS_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)			 \
	_BT_SUBSYS_SETTINGS_DEFINE(_prio, _hname, _subtree, _set, _commit)

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_COMMON_SETTINGS_H_ */
