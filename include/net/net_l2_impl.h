#pragma once

// Header file with symbols that are needed by those that are below
// the L2 abstraction layer; Drivers, etc. (Or is it those that are
// above? Anyways, those that are one the side of net_l2.h that need
// access to the context)

// Nedeed to use NET_DEVICE_INIT()

// Should not be included by those above the L2 abstraction layer.

// TODO: I need help from someone that understands the Zephyr
// networking stack to make sure that this makes sense and that the
// correct technical jargon is used.

#ifdef CONFIG_NET_L2_IEEE802154
#include <net/ieee802154.h>
#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context
#endif /* CONFIG_NET_L2_IEEE802154 */
