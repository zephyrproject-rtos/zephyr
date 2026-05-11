/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>
#include <zephyr/zvfs/eventfd.h>

#include "btca.h"
#include "clock.h"
#include "msg.h"
#include "port.h"
#include "transport.h"

static struct net_if fake_iface;
static uint8_t fake_mac[NET_ETH_ADDR_LEN] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
static struct net_linkaddr fake_linkaddr;
static const struct device fake_phc;

static int fake_eventfd_create_calls;
static int fake_eventfd_write_calls;
static int fake_eventfd_last_fd;
static zvfs_eventfd_t fake_eventfd_last_value;

static int fake_zvfs_eventfd(unsigned int initval, int flags)
{
	ARG_UNUSED(initval);
	ARG_UNUSED(flags);

	fake_eventfd_create_calls++;
	return 17;
}

static int fake_zvfs_eventfd_write(int fd, zvfs_eventfd_t value)
{
	fake_eventfd_write_calls++;
	fake_eventfd_last_fd = fd;
	fake_eventfd_last_value = value;

	return 0;
}

static int fake_zvfs_eventfd_read(int fd, zvfs_eventfd_t *value)
{
	ARG_UNUSED(fd);

	if (value) {
		*value = 0;
	}

	return 0;
}

static struct net_if *fake_net_if_get_first_by_type(const struct net_l2 *l2)
{
	ARG_UNUSED(l2);

	return &fake_iface;
}

static struct net_linkaddr *fake_net_if_get_link_addr(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return &fake_linkaddr;
}

static const struct device *fake_net_eth_get_ptp_clock(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return &fake_phc;
}

static int fake_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	memset(tm, 0, sizeof(*tm));
	return 0;
}

static int fake_ptp_clock_set(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tm);

	return 0;
}

static int fake_ptp_clock_rate_adjust(const struct device *dev, double ratio)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ratio);

	return 0;
}

int ptp_btca_ds_cmp(const struct ptp_dataset *a, const struct ptp_dataset *b)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);

	return 0;
}

enum ptp_port_state ptp_btca_state_decision(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return PTP_PS_LISTENING;
}

struct ptp_foreign_tt_clock *ptp_port_best_foreign(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return NULL;
}

enum ptp_port_state ptp_port_state(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return PTP_PS_LISTENING;
}

void ptp_port_event_handle(struct ptp_port *port, enum ptp_port_event event, bool tt_diff)
{
	ARG_UNUSED(port);
	ARG_UNUSED(event);
	ARG_UNUSED(tt_diff);
}

void ptp_msg_pre_send(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);
}

int ptp_msg_post_recv(struct ptp_port *port, struct ptp_msg *msg, int cnt)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);
	ARG_UNUSED(cnt);

	return 0;
}

int ptp_transport_send(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);
	ARG_UNUSED(idx);

	return 0;
}

int ptp_port_management_resp(struct ptp_port *port, struct ptp_msg *req, struct ptp_tlv_mgmt *tlv)
{
	ARG_UNUSED(port);
	ARG_UNUSED(req);
	ARG_UNUSED(tlv);

	return 0;
}

int ptp_port_management_error(struct ptp_port *port, struct ptp_msg *msg, enum ptp_mgmt_err err)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);
	ARG_UNUSED(err);

	return 0;
}

int ptp_port_management_msg_process(struct ptp_port *port, struct ptp_port *sender,
				    struct ptp_msg *msg, struct ptp_tlv_mgmt *tlv)
{
	ARG_UNUSED(port);
	ARG_UNUSED(sender);
	ARG_UNUSED(msg);
	ARG_UNUSED(tlv);

	return 0;
}

#define zvfs_eventfd fake_zvfs_eventfd
#define zvfs_eventfd_write fake_zvfs_eventfd_write
#define zvfs_eventfd_read fake_zvfs_eventfd_read
#define net_if_get_first_by_type fake_net_if_get_first_by_type
#define net_if_get_link_addr fake_net_if_get_link_addr
#define net_eth_get_ptp_clock fake_net_eth_get_ptp_clock
#define ptp_clock_get fake_ptp_clock_get
#define ptp_clock_set fake_ptp_clock_set
#define ptp_clock_rate_adjust fake_ptp_clock_rate_adjust
#include "../../../../../subsys/net/lib/ptp/clock.c"
#undef ptp_clock_rate_adjust
#undef ptp_clock_set
#undef ptp_clock_get
#undef net_eth_get_ptp_clock
#undef net_if_get_link_addr
#undef net_if_get_first_by_type
#undef zvfs_eventfd_read
#undef zvfs_eventfd_write
#undef zvfs_eventfd

static void reset_clock_state(void)
{
	memset(&ptp_clk, 0, sizeof(ptp_clk));
	memset(&fake_linkaddr, 0, sizeof(fake_linkaddr));
	fake_linkaddr.type = NET_LINK_ETHERNET;
	fake_linkaddr.len = NET_ETH_ADDR_LEN;
	memcpy(fake_linkaddr.addr, fake_mac, sizeof(fake_mac));
	fake_eventfd_create_calls = 0;
	fake_eventfd_write_calls = 0;
	fake_eventfd_last_fd = -1;
	fake_eventfd_last_value = 0;
}

static void clock_wakeup_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_clock_state();
}

ZTEST(ptp_clock_wakeup, test_pollfd_invalidate_is_quiet_before_init)
{
	ptp_clock_pollfd_invalidate();
	zassert_equal(fake_eventfd_write_calls, 0, "unexpected wakeup before init");
}

ZTEST(ptp_clock_wakeup, test_pollfd_invalidate_wakes_worker_after_init)
{
	zassert_not_null(ptp_clock_init(), "clock init failed");
	zassert_equal(fake_eventfd_create_calls, 1, "eventfd should be created once");

	ptp_clk.pollfd_valid = true;
	fake_eventfd_write_calls = 0;

	ptp_clock_pollfd_invalidate();

	zassert_false(ptp_clk.pollfd_valid, "pollfd cache should be invalidated");
	zassert_equal(fake_eventfd_write_calls, 1, "worker was not woken");
	zassert_equal(fake_eventfd_last_fd, 17, "unexpected eventfd target");
	zassert_equal(fake_eventfd_last_value, 1, "unexpected wakeup value");
}

ZTEST(ptp_clock_wakeup, test_state_decision_request_wakes_only_once_per_pending_event)
{
	zassert_not_null(ptp_clock_init(), "clock init failed");

	ptp_clk.state_decision_event = false;
	fake_eventfd_write_calls = 0;

	ptp_clock_state_decision_req();
	zassert_true(ptp_clk.state_decision_event, "state decision flag not set");
	zassert_equal(fake_eventfd_write_calls, 1, "initial state decision wakeup missing");

	ptp_clock_state_decision_req();
	zassert_equal(fake_eventfd_write_calls, 1, "duplicate wakeup for pending decision");
}

ZTEST_SUITE(ptp_clock_wakeup, NULL, NULL, clock_wakeup_before, NULL, NULL);
