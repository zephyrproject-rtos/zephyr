/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>
#include <zephyr/sys/base64.h>

static void find_vpn_iface(struct net_if *iface, void *user_data)
{
	struct net_if **found = user_data;

	if (*found == NULL && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_virtual_get_iface_capabilities(iface) == VIRTUAL_INTERFACE_VPN) {
		*found = iface;
	}
}

static void set_private_key(void)
{
	static const char *private_key = "lmAIbJR8PQOpgJxmfOydBiDbexTMEKsjglZ1sj3kIjs=";
	struct virtual_interface_req_params params = { 0 };
	struct net_if *vpn_iface = NULL;
	uint8_t key[32];
	size_t olen;
	int ret;

	net_if_foreach(find_vpn_iface, &vpn_iface);
	zassert_not_null(vpn_iface, "No VPN interface");

	ret = base64_decode(key, sizeof(key), &olen, private_key, strlen(private_key));
	zassert_true(ret == 0 && olen == sizeof(key), "Cannot decode the private key");

	params.private_key.data = key;
	params.private_key.len = sizeof(key);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, vpn_iface, &params,
		       sizeof(params));
	(void)memset(key, 0, sizeof(key));
	zassert_equal(ret, 0, "Cannot set the private key (%d)", ret);
}

/* Adding the first peer takes its virtual interface up, and the virtual L2
 * takes the wg_ctrl control interface below it up as well. wg_ctrl is a dummy
 * L2 interface without a link address, which used to trip an assertion in
 * notify_iface_up().
 */
ZTEST(wireguard_iface, test_peer_add_takes_ctrl_iface_up)
{
	struct wireguard_peer_config config = {
		.public_key = "fF2zTE6NVbBEZkwDgaPDkd9Vx7nL6xj/6g9W3nwyyHc=",
	};
	struct net_sockaddr_in *endpoint = (struct net_sockaddr_in *)&config.endpoint_ip;
	struct net_if *peer_iface = NULL;
	struct net_if *ctrl_iface;
	int peer_id;
	int ret;

	ctrl_iface = net_if_get_by_index(net_if_get_by_name("wg_ctrl"));
	zassert_not_null(ctrl_iface, "No wg_ctrl interface");

	set_private_key();

	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_port = net_htons(51820);
	endpoint->sin_addr.s_addr = net_htonl(0xC0000201U);

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);
	zassert_not_null(peer_iface, "No peer interface");

	zassert_true(net_if_is_up(peer_iface), "Peer interface is not up");
	zassert_true(net_if_is_up(ctrl_iface), "wg_ctrl is not up");

	ret = wireguard_peer_remove(peer_id);
	zassert_equal(ret, 0, "Peer remove failed (%d)", ret);
}

ZTEST_SUITE(wireguard_iface, NULL, NULL, NULL, NULL, NULL);
