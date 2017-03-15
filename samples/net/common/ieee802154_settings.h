/* IEEE 802.15.4 Samples settings header */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_L2_IEEE802154) && defined(CONFIG_NET_APP_SETTINGS)

int ieee802154_sample_setup(void);

#else
#define ieee802154_sample_setup(...) 0
#endif
