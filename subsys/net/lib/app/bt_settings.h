/* IEEE 802.15.4 settings header */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_L2_BT) && defined(CONFIG_NET_APP_SETTINGS)
int _net_app_bt_setup(void);
#else
#define _net_app_bt_setup(...) 0
#endif
