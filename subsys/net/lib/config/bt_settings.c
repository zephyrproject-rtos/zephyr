/* IEEE 802.15.4 settings code */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_mgmt.h>
#include <net/bt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#if defined(CONFIG_NET_CONFIG_BT_NODE)
#define ADV_STR "on"

static struct bt_gatt_attr attrs[] = {
	/* IP Support Service Declaration */
	BT_GATT_PRIMARY_SERVICE(BT_UUID_IPSS),
};

static struct bt_gatt_service ipss_svc = BT_GATT_SERVICE(attrs);
#endif

int _net_config_bt_setup(void)
{
	struct net_if *iface;
	struct device *dev;
	int err;

	err = bt_enable(NULL);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	dev = device_get_binding("net_bt");
	if (!dev) {
		return -ENODEV;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_CONFIG_BT_NODE)
	bt_gatt_service_register(&ipss_svc);

	if (net_mgmt(NET_REQUEST_BT_ADVERTISE, iface, ADV_STR,
		     sizeof(ADV_STR))) {
		return -EINVAL;
	}
#endif

	return 0;
}
