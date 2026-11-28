/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ppp.h>
#include <zephyr/ztest.h>

static struct net_if *ppp_iface;
static struct net_if *dummy_iface;
static uint32_t transmitted;
static const uint8_t terminate_request[] = {0xc0, 0x21, 0x05, 0x01, 0x00, 0x04};

static void iface_init(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

static int send_packet(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t payload[sizeof(terminate_request)];

	ARG_UNUSED(dev);
	zassert_true(net_pkt_is_ppp(pkt));
	zassert_equal(net_pkt_get_len(pkt), sizeof(payload));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, payload, sizeof(payload)));
	zassert_mem_equal(payload, terminate_request, sizeof(payload));
	transmitted++;
	return 0;
}

static const struct ppp_api ppp_api = {
	.iface_api.init = iface_init,
	.send = send_packet,
};

static const struct dummy_api dummy_api = {
	.iface_api.init = iface_init,
	.send = send_packet,
};

NET_DEVICE_INIT(test_ppp, "test_ppp", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ppp_api,
		PPP_L2, NET_L2_GET_CTX_TYPE(PPP_L2), 1500);
NET_DEVICE_INIT(test_dummy, "test_dummy", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 1500);

static void *suite_setup(void)
{
	ppp_iface = net_if_lookup_by_dev(DEVICE_GET(test_ppp));
	dummy_iface = net_if_lookup_by_dev(DEVICE_GET(test_dummy));
	zassert_not_null(ppp_iface);
	zassert_not_null(dummy_iface);
	return NULL;
}

static void before(void *fixture)
{
	struct net_if *ifaces[] = {ppp_iface, dummy_iface};

	ARG_UNUSED(fixture);
	transmitted = 0;
	/* Set the link state directly to isolate TX admission from PPP negotiation. */
	for (size_t i = 0; i < ARRAY_SIZE(ifaces); i++) {
		net_if_flag_set(ifaces[i], NET_IF_UP);
		net_if_flag_set(ifaces[i], NET_IF_LOWER_UP);
		net_if_flag_set(ifaces[i], NET_IF_DORMANT);
		net_if_flag_clear(ifaces[i], NET_IF_RUNNING);
		net_if_flag_clear(ifaces[i], NET_IF_SUSPENDED);
		ifaces[i]->if_dev->oper_state = NET_IF_OPER_DORMANT;
	}
}

static void check_send(struct net_if *iface, bool control, int expected)
{
	struct net_pkt *pkt = net_pkt_alloc_with_buffer(iface, sizeof(terminate_request),
						      control ? NET_AF_UNSPEC : NET_AF_INET,
						      0, K_NO_WAIT);
	int ret;

	zassert_not_null(pkt);
	zassert_ok(net_pkt_write(pkt, terminate_request, sizeof(terminate_request)));
	net_pkt_set_ppp(pkt, control);
	ret = net_try_send_data(pkt, K_NO_WAIT);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
	zassert_equal(ret, expected);
	zassert_equal(transmitted, expected == 0 ? 1 : 0);
}

ZTEST(ppp_dormancy, test_control_can_terminate_dormant_link)
{
	check_send(ppp_iface, true, 0);
}

ZTEST(ppp_dormancy, test_ip_cannot_use_dormant_link)
{
	check_send(ppp_iface, false, -ENETDOWN);
}

ZTEST(ppp_dormancy, test_control_requires_admin_up)
{
	net_if_flag_clear(ppp_iface, NET_IF_UP);
	check_send(ppp_iface, true, -ENETDOWN);
}

ZTEST(ppp_dormancy, test_control_requires_carrier)
{
	net_if_flag_clear(ppp_iface, NET_IF_LOWER_UP);
	check_send(ppp_iface, true, -ENETDOWN);
}

ZTEST(ppp_dormancy, test_control_flag_does_not_bypass_other_l2)
{
	check_send(dummy_iface, true, -ENETDOWN);
}

ZTEST(ppp_dormancy, test_control_on_operational_link)
{
	net_if_flag_clear(ppp_iface, NET_IF_DORMANT);
	net_if_flag_set(ppp_iface, NET_IF_RUNNING);
	ppp_iface->if_dev->oper_state = NET_IF_OPER_UP;
	check_send(ppp_iface, true, 0);
}

ZTEST(ppp_dormancy, test_control_does_not_bypass_suspension)
{
	net_if_flag_set(ppp_iface, NET_IF_SUSPENDED);
	check_send(ppp_iface, true, -EIO);
}

ZTEST_SUITE(ppp_dormancy, NULL, suite_setup, before, NULL, NULL);
