/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/linker/sections.h>
#include <zephyr/pm/device.h>
#include <ztest.h>
#include <zephyr/random/rand32.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

struct fake_dev_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_if *iface;
};

static int fake_dev_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	struct fake_dev_context *ctx = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = net_if_suspend(ctx->iface);
		if (ret == -EBUSY) {
			goto out;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = net_if_resume(ctx->iface);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

out:

	return ret;
}


static int fake_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static uint8_t *fake_dev_get_mac(struct fake_dev_context *ctx)
{
	if (ctx->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		ctx->mac_addr[0] = 0x00;
		ctx->mac_addr[1] = 0x00;
		ctx->mac_addr[2] = 0x5E;
		ctx->mac_addr[3] = 0x00;
		ctx->mac_addr[4] = 0x53;
		ctx->mac_addr[5] = sys_rand32_get();
	}

	return ctx->mac_addr;
}

static void fake_dev_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct fake_dev_context *ctx = dev->data;
	uint8_t *mac = fake_dev_get_mac(ctx);

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);

	ctx->iface = iface;
}

int fake_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

struct fake_dev_context fake_dev_context_data;

static struct dummy_api fake_dev_if_api = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER    DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

PM_DEVICE_DEFINE(fake_dev, fake_dev_pm_action);

NET_DEVICE_INIT(fake_dev, "fake_dev",
		fake_dev_init, PM_DEVICE_GET(fake_dev),
		&fake_dev_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_dev_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

void test_setup(void)
{
	struct net_if *iface;
	struct in_addr in4addr_my = { { { 192, 168, 0, 2 } } };
	struct net_if_addr *ifaddr;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	net_if_up(iface);

	ifaddr = net_if_ipv4_addr_add(iface, &in4addr_my, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Could not add iface address");
}

void test_pm(void)
{
	struct net_if *iface =
		net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	const struct device *dev = net_if_get_device(iface);
	char data[] = "some data";
	struct sockaddr_in addr4;
	int sock;
	int ret;

	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(12345);
	inet_pton(AF_INET, "192.168.0.1", &addr4.sin_addr);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Could not open socket");

	zassert_false(net_if_is_suspended(iface), "net iface is not suspended");

	/* Let's send some data, it should go through */
	ret = sendto(sock, data, ARRAY_SIZE(data), 0,
		     (struct sockaddr *)&addr4, sizeof(struct sockaddr_in));
	zassert_true(ret > 0, "Could not send data");

	/* Let's make sure net stack's thread gets ran, or setting PM state
	 * might return -EBUSY instead
	 */
	k_yield();

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_true(ret == 0, "Could not set state");

	zassert_true(net_if_is_suspended(iface), "net iface is not suspended");

	/* Let's try to suspend it again, it should fail relevantly */
	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
	zassert_true(ret == -EALREADY, "Could change state");

	zassert_true(net_if_is_suspended(iface), "net iface is not suspended");

	/* Let's send some data, it should fail relevantly */
	ret = sendto(sock, data, ARRAY_SIZE(data), 0,
		     (struct sockaddr *)&addr4, sizeof(struct sockaddr_in));
	zassert_true(ret < 0, "Could send data");

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	zassert_true(ret == 0, "Could not set state");

	zassert_false(net_if_is_suspended(iface), "net iface is suspended");

	ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	zassert_true(ret == -EALREADY, "Could change state");

	/* Let's send some data, it should go through */
	ret = sendto(sock, data, ARRAY_SIZE(data), 0,
		     (struct sockaddr *)&addr4, sizeof(struct sockaddr_in));
	zassert_true(ret > 0, "Could not send data");

	close(sock);
}

void test_main(void)
{
	ztest_test_suite(test_net_pm,
			 ztest_unit_test(test_setup),
			 ztest_unit_test(test_pm));
	ztest_run_test_suite(test_net_pm);
}
