/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>

#include <pmstats.h>
#include <soc/soc_caps.h>

#define PM_TEST_DATA "ICMP PM dummy data"

#define SLEEP_MS        200U
#define SLEEP_CYCLES    2U
#define LINK_TIMEOUT_MS 10000U
#define PING_TIMEOUT_MS 2000U

#if defined(CONFIG_ESP32_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP) && defined(SOC_PMU_SUPPORTED)
#define EXPECT_TOP_DOWN true
#else
#define EXPECT_TOP_DOWN false
#endif

#define TC_BLANK() TC_PRINT("%s\n", " ")

static K_SEM_DEFINE(echo_reply, 0, 1);

static struct net_if *iface;
static const struct device *phy_dev;

static enum net_verdict icmp_reply(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
				   struct net_icmp_ip_hdr *hdr, struct net_icmp_hdr *icmp_hdr,
				   void *user_data)
{
	k_sem_give(&echo_reply);

	return NET_OK;
}

static bool ping_gateway(uint32_t timeout_ms)
{
	struct net_icmp_ping_params params = {
		.identifier = 4321,
		.sequence = 1,
		.data = PM_TEST_DATA,
		.data_size = sizeof(PM_TEST_DATA),
	};
	struct net_sockaddr_in dst4 = {.sin_family = NET_AF_INET};
	struct net_in_addr gw = net_if_ipv4_get_gw(iface);
	struct net_icmp_ctx ctx;
	bool replied = false;

	if (gw.s_addr == 0U) {
		return false;
	}

	memcpy(&dst4.sin_addr, &gw, sizeof(gw));

	if (net_icmp_init_ctx(&ctx, NET_AF_INET, NET_ICMPV4_ECHO_REPLY, 0, icmp_reply) != 0) {
		return false;
	}

	k_sem_reset(&echo_reply);

	if (net_icmp_send_echo_request(&ctx, iface, (struct net_sockaddr *)&dst4, &params,
				       NULL) == 0) {
		replied = k_sem_take(&echo_reply, K_MSEC(timeout_ms)) == 0;
	}

	net_icmp_cleanup_ctx(&ctx);

	return replied;
}

/* Retry while DHCP and ARP settle after the link comes back. */
static bool ping_gateway_until(uint32_t timeout_ms)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(timeout_ms));

	do {
		if (ping_gateway(PING_TIMEOUT_MS)) {
			return true;
		}
		k_msleep(100);
	} while (!sys_timepoint_expired(deadline));

	return false;
}

static bool wait_carrier(bool up, uint32_t timeout_ms)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(timeout_ms));

	while (net_if_is_carrier_ok(iface) != up) {
		if (sys_timepoint_expired(deadline)) {
			return false;
		}
		k_msleep(50);
	}

	return true;
}

static void phy_set_power_down(bool down)
{
	uint32_t bmcr;

	zassert_equal(phy_read(phy_dev, MII_BMCR, &bmcr), 0, "PHY BMCR read failed");

	if (down) {
		bmcr |= MII_BMCR_POWER_DOWN;
	} else {
		bmcr &= ~MII_BMCR_POWER_DOWN;
		bmcr |= MII_BMCR_AUTONEG_RESTART;
	}

	zassert_equal(phy_write(phy_dev, MII_BMCR, bmcr), 0, "PHY BMCR write failed");
}

/*
 * Goal/Pass-if TC_PRINT can still be draining on the console after this thread
 * calls k_sleep, which would wake the first sleep early. Let it finish first.
 */
static void settle_console(void)
{
	k_msleep(50);
}

static void print_window(uint32_t n, const struct esp32_sleep_window *w)
{
	TC_PRINT("  #%u  fragments: %u  slept: %u  skipped: %u  err: 0x%x  TOP: %s\n", n,
		 w->fragments, w->slept, w->skipped, (unsigned int)w->err,
		 w->top_down ? "OFF" : "ON");
}

static void check_no_sleep(const char *msg)
{
	settle_console();

	uint32_t seq0 = esp32_sleep_stats_get(NULL);

	k_msleep(SLEEP_MS);

	zassert_equal(esp32_sleep_stats_get(NULL), seq0, "%s", msg);
}

static void check_sleep_with_link_down(void)
{
	TC_BLANK();
	TC_PRINT("  --- sleeps with the link down ---\n");

	settle_console();

	for (uint32_t i = 0; i < SLEEP_CYCLES; i++) {
		struct esp32_sleep_window w;
		uint32_t seq0 = esp32_sleep_stats_get(NULL);

		k_msleep(SLEEP_MS);

		uint32_t seq1 = esp32_sleep_stats_get(&w);

		print_window(i + 1, &w);
		zassert_true(seq1 != seq0, "no light sleep window with the link down");
		zassert_true(w.slept > 0, "window reported but nothing actually slept");
		zassert_equal(w.err, 0, "HAL sleep error 0x%x", (unsigned int)w.err);
		if (EXPECT_TOP_DOWN) {
			zassert_true(w.top_down, "TOP stayed ON with the link down");
		}
	}
	TC_BLANK();
}

ZTEST(ethernet_pm, test_link_up_blocks_sleep)
{
	TC_BLANK();
	TC_PRINT("Goal: Verify that light sleep is blocked while the Ethernet link is up.\n");
	TC_BLANK();
	TC_PRINT("  Pass if:\n");
	TC_PRINT("    - no light sleep window is reported across a %u ms sleep\n", SLEEP_MS);
	TC_PRINT("    - the gateway answers a ping\n");

	zassert_true(wait_carrier(true, LINK_TIMEOUT_MS), "link did not come up");
	check_no_sleep("light sleep entered with the link up");
	zassert_true(ping_gateway_until(LINK_TIMEOUT_MS), "gateway ping failed");
	TC_BLANK();
}

ZTEST(ethernet_pm, test_link_down_sleep_and_recover)
{
	TC_BLANK();
	TC_PRINT("Goal: Verify light sleep while the PHY holds the Ethernet link down,\n");
	TC_PRINT("      and that Ethernet works again once the link is back.\n");
	TC_BLANK();
	TC_PRINT("  Pass if:\n");
	TC_PRINT("    - light sleep is executed while the link is down\n");
	if (EXPECT_TOP_DOWN) {
		TC_PRINT("    - the TOP domain is powered down\n");
	}
	TC_PRINT("    - the gateway answers a ping after the link is back\n");
	TC_PRINT("    - light sleep is blocked again once the link is back\n");
	TC_BLANK();

	zassert_true(wait_carrier(true, LINK_TIMEOUT_MS), "link did not come up");

	phy_set_power_down(true);
	zassert_true(wait_carrier(false, LINK_TIMEOUT_MS), "link did not go down");

	check_sleep_with_link_down();

	phy_set_power_down(false);
	zassert_true(wait_carrier(true, LINK_TIMEOUT_MS), "link did not come back");

	zassert_true(ping_gateway_until(CONFIG_DHCP_ASSIGN_TIMEOUT * MSEC_PER_SEC),
		     "gateway ping failed after the link came back");
	check_no_sleep("light sleep entered after the link came back");
	TC_BLANK();
}

static void *ethernet_pm_setup(void)
{
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	zassert_not_null(iface, "no Ethernet interface");

	phy_dev = net_eth_get_phy(iface);
	zassert_not_null(phy_dev, "no PHY attached to the Ethernet interface");

	return NULL;
}

ZTEST_SUITE(ethernet_pm, NULL, ethernet_pm_setup, NULL, NULL, NULL);
