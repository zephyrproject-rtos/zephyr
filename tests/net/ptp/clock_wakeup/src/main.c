/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>
#include <zephyr/timing/precision_ptp_clock.h>
#include <zephyr/ztest.h>
#include <zephyr/zvfs/eventfd.h>

#include "btca.h"
#include "clock.h"
#include "msg.h"
#include "port.h"
#include "transport.h"

static struct net_if fake_iface;
static uint8_t fake_mac[NET_ETH_ADDR_LEN] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static struct net_linkaddr fake_linkaddr;
static const struct device fake_phc;

static int fake_eventfd_create_calls;
static int fake_eventfd_write_calls;
static int fake_eventfd_last_fd;
static zvfs_eventfd_t fake_eventfd_last_value;
static int fake_ptp_clock_get_calls;
static int fake_ptp_clock_set_calls;
static int fake_ptp_clock_rate_adjust_calls;
static int fake_ptp_clock_get_ret;
static int fake_ptp_clock_get_second_ret;
static int fake_ptp_clock_set_ret;
static int fake_ptp_clock_rate_adjust_ret;
static int fake_precision_clock_get_caps_ret;
static double fake_ptp_clock_last_rate_ratio;
static struct net_ptp_time fake_ptp_clock_time;
static struct net_ptp_time fake_ptp_clock_last_set_time;
static struct precision_clock_caps fake_precision_clock_caps;
static int port_management_error_calls;
static int port_management_process_calls;
static uint16_t last_port_management_tlv_id;
static int fake_port_event_calls;
static struct ptp_foreign_tt_clock *fake_best_foreign;
static enum ptp_port_state fake_state_decision;

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
	int ret;

	ARG_UNUSED(dev);

	fake_ptp_clock_get_calls++;
	ret = fake_ptp_clock_get_calls == 2 ? fake_ptp_clock_get_second_ret
					    : fake_ptp_clock_get_ret;
	if (ret == 0 && tm != NULL) {
		*tm = fake_ptp_clock_time;
	}

	return ret;
}

static int fake_ptp_clock_set(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	fake_ptp_clock_set_calls++;
	if (tm) {
		fake_ptp_clock_last_set_time = *tm;
	}

	return fake_ptp_clock_set_ret;
}

static int fake_ptp_clock_rate_adjust(const struct device *dev, double ratio)
{
	ARG_UNUSED(dev);

	fake_ptp_clock_rate_adjust_calls++;
	fake_ptp_clock_last_rate_ratio = ratio;

	return fake_ptp_clock_rate_adjust_ret;
}

static int fake_precision_ptp_clock_init(struct precision_ptp_clock_adapter *adapter,
					 const struct device *ptp_clock,
					 struct precision_time_domain domain)
{
	adapter->ptp_clock = ptp_clock;
	adapter->clock.domain = domain;

	return 0;
}

static const struct precision_clock *
fake_precision_ptp_clock_get(const struct precision_ptp_clock_adapter *adapter)
{
	return &adapter->clock;
}

static int fake_precision_clock_read(const struct precision_clock *precision_clk,
				     struct precision_time_point *tp)
{
	struct net_ptp_time ptp_time = {0};
	int ret;

	ret = fake_ptp_clock_get(&fake_phc, &ptp_time);
	if (ret < 0) {
		return ret;
	}

	ret = precision_time_from_u64_sec_nsec(ptp_time.second, ptp_time.nanosecond, &tp->time);
	if (ret < 0) {
		return ret;
	}

	tp->domain = precision_clk->domain;

	return 0;
}

static int fake_precision_clock_set(const struct precision_clock *precision_clk,
				    const struct precision_time_point *tp)
{
	struct net_ptp_time ptp_time;
	int ret;

	ARG_UNUSED(precision_clk);

	ret = precision_time_to_u64_sec_nsec(tp->time, &ptp_time.second, &ptp_time.nanosecond);
	if (ret < 0) {
		return ret;
	}

	return fake_ptp_clock_set(&fake_phc, &ptp_time);
}

static int fake_precision_clock_adjust_rate(const struct precision_clock *precision_clk,
					    int32_t rate_ppb)
{
	ARG_UNUSED(precision_clk);

	return fake_ptp_clock_rate_adjust(&fake_phc, 1.0 + ((double)rate_ppb / 1000000000.0));
}

static int fake_precision_clock_get_caps(const struct precision_clock *precision_clk,
					 struct precision_clock_caps *caps)
{
	ARG_UNUSED(precision_clk);

	if (fake_precision_clock_get_caps_ret == 0) {
		*caps = fake_precision_clock_caps;
	}

	return fake_precision_clock_get_caps_ret;
}

bool ptp_port_id_eq(const struct ptp_port_id *p1, const struct ptp_port_id *p2)
{
	return memcmp(&p1->clk_id, &p2->clk_id, sizeof(p1->clk_id)) == 0 &&
	       p1->port_number == p2->port_number;
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

	return fake_state_decision;
}

struct ptp_foreign_tt_clock *ptp_port_best_foreign(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return fake_best_foreign;
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

	fake_port_event_calls++;
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

enum ptp_mgmt_op ptp_mgmt_action(struct ptp_msg *msg)
{
	return msg->management.action;
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

	last_port_management_tlv_id = tlv->id;
	return 0;
}

int ptp_port_management_error(struct ptp_port *port, struct ptp_msg *msg, enum ptp_mgmt_err err)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);
	ARG_UNUSED(err);

	port_management_error_calls++;
	return 0;
}

int ptp_port_management_msg_process(struct ptp_port *port, struct ptp_port *sender,
				    struct ptp_msg *msg, struct ptp_tlv_mgmt *tlv)
{
	ARG_UNUSED(port);
	ARG_UNUSED(sender);
	ARG_UNUSED(msg);

	port_management_process_calls++;
	last_port_management_tlv_id = tlv->id;
	return 0;
}

#define zvfs_eventfd             fake_zvfs_eventfd
#define zvfs_eventfd_write       fake_zvfs_eventfd_write
#define zvfs_eventfd_read        fake_zvfs_eventfd_read
#define net_if_get_first_by_type fake_net_if_get_first_by_type
#define net_if_get_link_addr     fake_net_if_get_link_addr
#define net_eth_get_ptp_clock    fake_net_eth_get_ptp_clock
#define ptp_clock_get            fake_ptp_clock_get
#define ptp_clock_set            fake_ptp_clock_set
#define ptp_clock_rate_adjust    fake_ptp_clock_rate_adjust
#define precision_ptp_clock_init    fake_precision_ptp_clock_init
#define precision_ptp_clock_get     fake_precision_ptp_clock_get
#define precision_clock_read        fake_precision_clock_read
#define precision_clock_set         fake_precision_clock_set
#define precision_clock_adjust_rate fake_precision_clock_adjust_rate
#define precision_clock_get_caps    fake_precision_clock_get_caps
#include "../../../../../subsys/net/lib/ptp/clock.c"
#undef precision_clock_get_caps
#undef precision_clock_adjust_rate
#undef precision_clock_set
#undef precision_clock_read
#undef precision_ptp_clock_get
#undef precision_ptp_clock_init
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
	fake_ptp_clock_get_calls = 0;
	fake_ptp_clock_set_calls = 0;
	fake_ptp_clock_rate_adjust_calls = 0;
	fake_ptp_clock_get_ret = 0;
	fake_ptp_clock_get_second_ret = 0;
	fake_ptp_clock_set_ret = 0;
	fake_ptp_clock_rate_adjust_ret = 0;
	fake_precision_clock_get_caps_ret = -ENOTSUP;
	fake_ptp_clock_last_rate_ratio = 0.0;
	memset(&fake_ptp_clock_time, 0, sizeof(fake_ptp_clock_time));
	memset(&fake_ptp_clock_last_set_time, 0, sizeof(fake_ptp_clock_last_set_time));
	memset(&fake_precision_clock_caps, 0, sizeof(fake_precision_clock_caps));
	port_management_error_calls = 0;
	port_management_process_calls = 0;
	last_port_management_tlv_id = 0;
	fake_port_event_calls = 0;
	fake_best_foreign = NULL;
	fake_state_decision = PTP_PS_LISTENING;
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

ZTEST(ptp_clock_wakeup, test_u64_timestamp_conversion_distinguishes_invalid_output)
{
	precision_time_t converted;

	zassert_equal(clock_u64_ns_to_precision(0, NULL), -EINVAL);
	zassert_equal(clock_u64_ns_to_precision((uint64_t)PRECISION_TIME_MAX + 1U, &converted),
		      -ERANGE);
	zassert_ok(clock_u64_ns_to_precision(PRECISION_TIME_MAX, &converted));
	zassert_equal(converted, PRECISION_TIME_MAX);
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

ZTEST(ptp_clock_wakeup, test_state_decision_retries_source_change_after_servo_reset_failure)
{
	struct ptp_foreign_tt_clock foreign = {0};
	struct ptp_port port = {0};
	struct ptp_msg announce = {0};
	struct ptp_port_id old_sender = {0};
	ptp_clk_id old_grandmaster = {0};

	old_sender.clk_id.id[0] = 0x10;
	old_sender.port_number = 1;
	old_grandmaster.id[0] = 0x20;
	foreign.dataset.sender.clk_id.id[0] = 0x30;
	foreign.dataset.sender.port_number = 2;
	foreign.dataset.clk_id.id[0] = 0x40;
	k_fifo_init(&foreign.messages);
	k_fifo_put(&foreign.messages, &announce);

	sys_slist_init(&ptp_clk.ports_list);
	sys_slist_append(&ptp_clk.ports_list, &port.node);
	ptp_clk.phc = &fake_phc;
	ptp_clk.sync_source.sender = old_sender;
	ptp_clk.sync_source.grandmaster = old_grandmaster;
	ptp_clk.sync_source.valid = true;
	fake_best_foreign = &foreign;
	fake_state_decision = PTP_PS_TIME_RECEIVER;
	fake_ptp_clock_rate_adjust_ret = -ETIMEDOUT;

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1,
		      "source change should attempt to restore nominal rate");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_FAULT,
		      "failed nominal-rate write should fault the discipline");
	zassert_true(ptp_port_id_eq(&ptp_clk.sync_source.sender, &old_sender),
		     "failed reset should preserve the previous source identity");
	zassert_true(ptp_clock_id_eq(&ptp_clk.sync_source.grandmaster, &old_grandmaster),
		     "failed reset should preserve the previous grandmaster identity");
	zassert_true(ptp_clk.sync_source.reset_pending,
		     "failed reset should keep the source transition pending");
	zassert_false(ptp_clk.state_decision_event,
		      "failed reset should not create another state-decision wakeup");
	zassert_equal(fake_port_event_calls, 1,
		      "source-change decision should dispatch one port event");

	fake_ptp_clock_rate_adjust_ret = 0;
	ptp_clock_handle_state_decision_evt();

	zassert_equal(fake_ptp_clock_rate_adjust_calls, 2,
		      "pending source change should retry the nominal-rate write");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_UNSYNCED,
		      "successful retry should clear the discipline fault");
	zassert_false(ptp_clk.sync_source.reset_pending,
		      "successful retry should clear the pending transition");
	zassert_true(clock_sync_source_matches(&foreign),
		     "successful retry should commit the new source identity");
	zassert_equal(fake_port_event_calls, 1,
		      "retry-only handler call should not repeat port state events");
}

ZTEST(ptp_clock_wakeup, test_pending_source_reset_is_dropped_without_ports)
{
	ptp_clk.sync_source.valid = true;
	ptp_clk.sync_source.reset_pending = true;
	sys_slist_init(&ptp_clk.ports_list);

	ptp_clock_handle_state_decision_evt();

	zassert_false(ptp_clk.sync_source.valid, "empty port list should clear the source");
	zassert_false(ptp_clk.sync_source.reset_pending,
		      "empty port list should clear the pending source reset");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 0,
		      "empty port list should not touch the PHC");
}

ZTEST(ptp_clock_wakeup, test_delay_ignores_missing_sync_timestamps)
{
	ptp_clock_delay(1000, 2000);

	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "delay should not update without sync");
	zassert_equal(ptp_clk.timestamp.t3, 0, "egress timestamp should not be stored");
	zassert_equal(ptp_clk.timestamp.t4, 0, "ingress timestamp should not be stored");
}

ZTEST(ptp_clock_wakeup, test_delay_updates_valid_mean_delay)
{
	ptp_clk.timestamp.t1 = 1000;
	ptp_clk.timestamp.t2 = 2000;

	ptp_clock_delay(1500, 2600);

	zassert_equal(ptp_clk.timestamp.t3, 1500, "delay egress timestamp mismatch");
	zassert_equal(ptp_clk.timestamp.t4, 2600, "delay ingress timestamp mismatch");
	zassert_equal(ptp_clk.current_ds.mean_delay, (ptp_timeinterval)1050 << 16,
		      "mean delay mismatch");
}

ZTEST(ptp_clock_wakeup, test_delay_rejects_unrealistic_sample)
{
	ptp_clk.timestamp.t1 = 100;
	ptp_clk.timestamp.t2 = 200;

	ptp_clock_delay(0, 3ULL * NSEC_PER_SEC);

	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "unrealistic delay should be ignored");
}

ZTEST(ptp_clock_wakeup, test_pdelay_updates_mean_link_delay_with_corrections)
{
	struct ptp_port port = {0};

	port.port_ds.delay_asymmetry = (ptp_timeinterval)20 << 16;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)42 << 16;

	zassert_ok(ptp_clock_pdelay(&port, 1000, 1500, 1800, 2600, (ptp_timeinterval)30 << 16,
				    (ptp_timeinterval)10 << 16),
		   "valid Pdelay sample rejected");
	zassert_equal(port.port_ds.mean_link_delay, (ptp_timeinterval)620 << 16,
		      "meanLinkDelay mismatch");
	zassert_equal(ptp_clk.current_ds.mean_delay, (ptp_timeinterval)42 << 16,
		      "Pdelay should not update currentDS mean delay");
	zassert_equal(port.neighbor_rate_ratio, 1.0, "first sample should use nominal ratio");
	zassert_false(port.neighbor_rate_ratio_valid, "first sample should not have rate ratio");
}

ZTEST(ptp_clock_wakeup, test_management_routes_pdelay_get_tlvs_to_port)
{
	struct ptp_tlv_mgmt *tlv;
	struct ptp_msg msg = {0};
	struct ptp_port port = {0};
	sys_snode_t tlv_node = {0};

	port.port_ds.id.port_number = 1;
	msg.management.action = PTP_MGMT_GET;
	msg.management.target_port_id.port_number = port.port_ds.id.port_number;
	sys_slist_init(&msg.tlvs);
	sys_slist_append(&msg.tlvs, &tlv_node);
	tlv = (struct ptp_tlv_mgmt *)msg.management.suffix;

	tlv->id = PTP_MGMT_DELAY_MECHANISM;
	(void)ptp_clock_management_msg_process(&port, &msg);
	zassert_equal(port_management_process_calls, 1, "delay mechanism GET not routed");
	zassert_equal(port_management_error_calls, 0, "delay mechanism GET reported an error");
	zassert_equal(last_port_management_tlv_id, PTP_MGMT_DELAY_MECHANISM,
		      "unexpected routed TLV");

	tlv->id = PTP_MGMT_LOG_MIN_PDELAY_REQ_INTERVAL;
	(void)ptp_clock_management_msg_process(&port, &msg);
	zassert_equal(port_management_process_calls, 2, "Pdelay interval GET not routed");
	zassert_equal(port_management_error_calls, 0, "Pdelay interval GET reported an error");
	zassert_equal(last_port_management_tlv_id, PTP_MGMT_LOG_MIN_PDELAY_REQ_INTERVAL,
		      "unexpected routed TLV");
}

ZTEST(ptp_clock_wakeup, test_pdelay_rejects_negative_and_incomplete_samples)
{
	struct ptp_port port = {0};

	port.port_ds.mean_link_delay = (ptp_timeinterval)123 << 16;

	zassert_equal(ptp_clock_pdelay(&port, 1000, 1000, 3000, 1500, 0, 0), -ERANGE,
		      "negative Pdelay sample should be rejected");
	zassert_equal(port.port_ds.mean_link_delay, (ptp_timeinterval)123 << 16,
		      "rejected sample should not update meanLinkDelay");

	zassert_equal(ptp_clock_pdelay(&port, 0, 1000, 2000, 3000, 0, 0), -EINVAL,
		      "incomplete Pdelay timestamps should be rejected");
	zassert_equal(port.port_ds.mean_link_delay, (ptp_timeinterval)123 << 16,
		      "incomplete sample should not update meanLinkDelay");
}

ZTEST(ptp_clock_wakeup, test_pdelay_accepts_configured_maximum_only)
{
	struct ptp_port port = {0};
	int64_t t4_at_limit = 2000 + 2LL * CONFIG_PTP_PEER_DELAY_MAX_NS;

	zassert_ok(ptp_clock_pdelay(&port, 1000, 1000, 2000, t4_at_limit, 0, 0),
		   "Pdelay sample at configured limit should be accepted");
	zassert_equal(port.port_ds.mean_link_delay,
		      (ptp_timeinterval)CONFIG_PTP_PEER_DELAY_MAX_NS << 16,
		      "meanLinkDelay at configured limit mismatch");

	zassert_equal(
		ptp_clock_pdelay(&port, 1000, 1000, 2000, t4_at_limit + 2, 0, 0), -ERANGE,
		"Pdelay sample above configured limit should be rejected");
	zassert_equal(port.port_ds.mean_link_delay,
		      (ptp_timeinterval)CONFIG_PTP_PEER_DELAY_MAX_NS << 16,
		      "rejected sample should preserve meanLinkDelay");
}

ZTEST(ptp_clock_wakeup, test_pdelay_updates_neighbor_rate_ratio_after_second_sample)
{
	struct ptp_port port = {0};

	zassert_ok(ptp_clock_pdelay(&port, 1000, 1100, 1200, 1600,
				    (ptp_timeinterval)100 << 16, 0),
		   "first Pdelay sample rejected");
	zassert_false(port.neighbor_rate_ratio_valid, "first sample should not have ratio");

	zassert_ok(ptp_clock_pdelay(&port, 2000, 2200, 3300, 3600,
				    (ptp_timeinterval)300 << 16, 0),
		   "second Pdelay sample rejected");
	zassert_true(port.neighbor_rate_ratio_valid, "second sample should update ratio");
	zassert_within(port.neighbor_rate_ratio, 1.15, 0.000001,
		       "neighbor rate ratio should include response corrections");
}

ZTEST(ptp_clock_wakeup, test_synchronize_uses_phc_time_when_ingress_timestamp_missing)
{
	uint64_t phc_now = 5ULL * NSEC_PER_SEC + 250;

	ptp_clk.phc = &fake_phc;
	fake_ptp_clock_time.second = 5;
	fake_ptp_clock_time.nanosecond = 250;

	ptp_clock_synchronize(1111, 900, false);

	zassert_equal(fake_ptp_clock_get_calls, 1, "PHC time should be sampled");
	zassert_equal(ptp_clk.timestamp.t1, 900, "egress timestamp mismatch");
	zassert_equal(ptp_clk.timestamp.t2, phc_now, "missing ingress should fall back to PHC");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 0, "servo should not run without delay");
}

ZTEST(ptp_clock_wakeup, test_synchronize_uses_phc_time_when_ingress_timestamp_out_of_range)
{
	uint64_t phc_now = 7ULL * NSEC_PER_SEC + 500;

	ptp_clk.phc = &fake_phc;
	fake_ptp_clock_time.second = 7;
	fake_ptp_clock_time.nanosecond = 500;

	ptp_clock_synchronize(100, 800, true);

	zassert_equal(ptp_clk.timestamp.t1, 800, "egress timestamp mismatch");
	zassert_equal(ptp_clk.timestamp.t2, phc_now, "out-of-range ingress should fall back");
}

ZTEST(ptp_clock_wakeup, test_synchronize_stops_when_phc_read_fails)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	ptp_clk.timestamp.t1 = 1234;
	ptp_clk.timestamp.t2 = 5678;
	fake_ptp_clock_get_ret = -EIO;

	ptp_clock_synchronize(10000, 9800, true);

	zassert_equal(fake_ptp_clock_get_calls, 1, "PHC time should be sampled once");
	zassert_equal(ptp_clk.timestamp.t1, 0, "clock fault should clear sync timestamps");
	zassert_equal(ptp_clk.timestamp.t2, 0, "clock fault should clear sync timestamps");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "clock fault should clear mean delay");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_FAULT,
		      "clock read failure should fault the discipline");
	zassert_equal(fake_ptp_clock_set_calls, 0, "failed PHC read should not set the clock");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 0,
		      "failed PHC read should not adjust the clock");
}

ZTEST(ptp_clock_wakeup, test_pi_servo_uses_configured_gains)
{
	clock_servo_init();

	zassert_equal(ptp_clk.sync_discipline.config.kp_num, CONFIG_PTP_SERVO_KP,
		      "proportional gain should come from Kconfig");
	zassert_equal(ptp_clk.sync_discipline.config.ki_num, CONFIG_PTP_SERVO_KI,
		      "integral gain should come from Kconfig");
}

ZTEST(ptp_clock_wakeup, test_pi_servo_intersects_protocol_and_clock_rate_limits)
{
	ptp_clk.phc = &fake_phc;
	fake_precision_clock_get_caps_ret = 0;
	fake_precision_clock_caps.min_rate_ppb = INT32_MIN;
	fake_precision_clock_caps.max_rate_ppb = INT32_MAX;

	clock_servo_init();
	zassert_equal(ptp_clk.sync_discipline.config.min_rate_ppb, SYNC_SERVO_MIN_RATE_PPB);
	zassert_equal(ptp_clk.sync_discipline.config.max_rate_ppb, SYNC_SERVO_MAX_RATE_PPB);

	fake_precision_clock_caps.min_rate_ppb = -100;
	fake_precision_clock_caps.max_rate_ppb = 200;
	clock_servo_init();
	zassert_equal(ptp_clk.sync_discipline.config.min_rate_ppb, -100);
	zassert_equal(ptp_clk.sync_discipline.config.max_rate_ppb, 200);
}

ZTEST(ptp_clock_wakeup, test_synchronize_applies_pi_rate_adjustment)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 0;
	fake_ptp_clock_time.nanosecond = 10000;

	ptp_clock_synchronize(10000, 9800, true);

	zassert_equal(ptp_clk.timestamp.t1, 9800, "egress timestamp mismatch");
	zassert_equal(ptp_clk.timestamp.t2, 10000, "ingress timestamp mismatch");
	zassert_equal(ptp_clk.current_ds.offset_from_tt, (ptp_timeinterval)100 << 16,
		      "offset mismatch");
	zassert_equal(fake_ptp_clock_set_calls, 0, "small offset should not hard-step");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1, "rate adjust should be applied");
	zassert_true(fake_ptp_clock_last_rate_ratio < 1.0, "positive offset should slow clock");
}

ZTEST(ptp_clock_wakeup, test_source_timeout_check_arms_and_enters_holdover)
{
	struct precision_pi_config config;
	struct precision_pi_status status;
	precision_time_t timeout_time;
	uint64_t timeout_second;
	uint32_t timeout_nanosecond;
	int get_calls;
	int64_t expiry_ms;

	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 1;

	ptp_clock_check_source_timeout();
	zassert_false(ptp_clk.sync_timeout_check.scheduled,
		      "timeout check should not arm before an observation");
	zassert_equal(fake_ptp_clock_get_calls, 0,
		      "timeout check should not read the PHC before an observation");

	ptp_clock_synchronize(NSEC_PER_SEC, NSEC_PER_SEC - 100, true);
	zassert_ok(precision_pi_get_status(&ptp_clk.sync_discipline, &status));
	zassert_true(status.has_observation, "synchronization should accept an observation");
	get_calls = fake_ptp_clock_get_calls;

	ptp_clock_check_source_timeout();
	zassert_true(ptp_clk.sync_timeout_check.scheduled,
		     "first accepted observation should arm the timeout check");
	zassert_equal(fake_ptp_clock_get_calls, get_calls,
		      "arming the timeout check should not read the PHC");
	expiry_ms = ptp_clk.sync_timeout_check.expiry_ms;

	ptp_clock_check_source_timeout();
	zassert_equal(fake_ptp_clock_get_calls, get_calls,
		      "an early timeout check should not read the PHC");
	zassert_equal(ptp_clk.sync_timeout_check.expiry_ms, expiry_ms,
		      "an early timeout check should not move the deadline");

	ptp_clock_sync_interval_update(-1);
	zassert_false(ptp_clk.sync_timeout_check.scheduled,
		      "a changed Sync interval should cancel the old deadline");
	ptp_clock_check_source_timeout();
	zassert_true(ptp_clk.sync_timeout_check.scheduled,
		     "timeout check should rearm after a Sync interval change");
	zassert_equal(fake_ptp_clock_get_calls, get_calls,
		      "rearming the timeout check should not read the PHC");

	zassert_ok(precision_pi_get_config(&ptp_clk.sync_discipline, &config));
	zassert_ok(precision_time_add(status.last_update_ns, config.source_timeout_ns + 1,
				      &timeout_time));
	zassert_ok(
		precision_time_to_u64_sec_nsec(timeout_time, &timeout_second, &timeout_nanosecond));
	fake_ptp_clock_time.second = timeout_second;
	fake_ptp_clock_time.nanosecond = timeout_nanosecond;
	ptp_clk.sync_timeout_check.expiry_ms = k_uptime_get();
	ptp_clock_check_source_timeout();

	zassert_equal(fake_ptp_clock_get_calls, get_calls + 1,
		      "an expired timeout check should read the PHC once");
	zassert_ok(precision_pi_get_status(&ptp_clk.sync_discipline, &status));
	zassert_equal(status.state, PRECISION_SYNC_HOLDOVER,
		      "an expired source timeout should enter holdover");
	zassert_true(ptp_clk.sync_timeout_check.scheduled,
		     "an expired timeout check should schedule the next check");

	zassert_ok(clock_servo_reset());
	zassert_false(ptp_clk.sync_timeout_check.scheduled,
		      "resetting the discipline should cancel the timeout check");
}

ZTEST(ptp_clock_wakeup, test_synchronize_tolerates_unsupported_rate_adjust)
{
	struct precision_pi_status status;

	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.nanosecond = 10000;
	fake_ptp_clock_rate_adjust_ret = -ENOTSUP;

	ptp_clock_synchronize(10000, 9800, true);

	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1,
		      "unsupported rate adjustment should be attempted once");
	zassert_ok(precision_pi_get_status(&ptp_clk.sync_discipline, &status));
	zassert_equal(status.state, PRECISION_SYNC_ACQUIRING,
		      "unsupported rate adjustment should not fault the discipline");
	zassert_equal(ptp_clk.current_ds.mean_delay, (ptp_timeinterval)100 << 16,
		      "unsupported rate adjustment should preserve synchronization data");

	ptp_clock_synchronize(10100, 9900, true);
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 2,
		      "the discipline should continue processing later observations");
}

ZTEST(ptp_clock_wakeup, test_synchronize_faults_servo_after_rate_adjust_failure)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 0;
	fake_ptp_clock_time.nanosecond = 10000;
	fake_ptp_clock_rate_adjust_ret = -EIO;
	clock_servo_init();
	precision_deadline_schedule(&ptp_clk.sync_timeout_check, NSEC_PER_SEC);

	ptp_clock_synchronize(10000, 9800, true);

	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1,
		      "failed adjustment should not issue another clock operation");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_FAULT,
		      "failed adjustment should fault the discipline");
	zassert_equal(ptp_clk.sync_discipline.drift_ppb, 0, "servo drift should be cleared");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "clock fault should clear mean delay");
	zassert_false(ptp_clk.sync_timeout_check.scheduled,
		      "faulting the discipline should cancel the timeout check");

	fake_ptp_clock_rate_adjust_ret = 0;
	ptp_clock_synchronize(11000, 10800, true);
	zassert_equal(fake_ptp_clock_get_calls, 1, "faulted discipline should block new control");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1,
		      "faulted discipline should block new adjustments");

	zassert_ok(clock_servo_reset());
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_UNSYNCED,
		      "explicit reset should clear the fault");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 2,
		      "explicit reset should restore nominal rate");
	zassert_equal(fake_ptp_clock_last_rate_ratio, 1.0,
		      "explicit reset should restore nominal rate");
}

ZTEST(ptp_clock_wakeup, test_synchronize_resets_after_consecutive_locked_outliers)
{
	const uint64_t ingress = NSEC_PER_SEC;
	const int64_t delay = 100;
	const int64_t locked_offset = 5LL * NSEC_PER_MSEC;
	const int64_t reacquire_offset = 200LL * NSEC_PER_MSEC;

	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)delay << 16;
	fake_ptp_clock_time.second = 1;

	for (int i = 0; i < SYNC_SERVO_LOCK_SAMPLES; i++) {
		uint64_t sample_ingress = ingress + i + 1;

		ptp_clock_synchronize(sample_ingress, sample_ingress - delay - locked_offset, true);
	}

	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_LOCKED,
		      "servo should lock after stable samples");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, SYNC_SERVO_LOCK_SAMPLES,
		      "stable samples should drive the PI controller");

	ptp_clock_synchronize(ingress + 10, ingress + 10 - delay - reacquire_offset, true);

	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_LOCKED,
		      "one outlier should preserve servo lock");
	zassert_equal(ptp_clk.sync_discipline.outlier_samples, 1, "outlier should be counted");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, SYNC_SERVO_LOCK_SAMPLES,
		      "isolated outlier should not change the clock rate");

	ptp_clock_synchronize(ingress + 20, ingress + 20 - delay - reacquire_offset, true);

	zassert_not_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_LOCKED,
			  "consecutive outliers should reset the servo");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, SYNC_SERVO_LOCK_SAMPLES + 1,
		      "second outlier should only restore nominal rate");
	zassert_equal(fake_ptp_clock_last_rate_ratio, 1.0,
		      "servo reset should restore nominal rate");

	ptp_clock_synchronize(ingress + 30, ingress + 30 - delay - reacquire_offset, true);

	zassert_equal(fake_ptp_clock_rate_adjust_calls, SYNC_SERVO_LOCK_SAMPLES + 2,
		      "persistent offset should restart PI acquisition");
	zassert_not_equal(fake_ptp_clock_last_rate_ratio, 1.0,
			  "reacquisition should apply a frequency correction");
}

ZTEST(ptp_clock_wakeup, test_synchronize_good_sample_clears_locked_outlier_count)
{
	const uint64_t ingress = NSEC_PER_SEC;
	const int64_t delay = 100;
	const int64_t locked_offset = 5LL * NSEC_PER_MSEC;
	const int64_t outlier_offset = 200LL * NSEC_PER_MSEC;

	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)delay << 16;
	fake_ptp_clock_time.second = 1;

	for (int i = 0; i < SYNC_SERVO_LOCK_SAMPLES; i++) {
		uint64_t sample_ingress = ingress + i + 1;

		ptp_clock_synchronize(sample_ingress, sample_ingress - delay - locked_offset, true);
	}

	ptp_clock_synchronize(ingress + 10, ingress + 10 - delay - outlier_offset, true);
	zassert_equal(ptp_clk.sync_discipline.outlier_samples, 1, "outlier should be counted");

	ptp_clock_synchronize(ingress + 20, ingress + 20 - delay - locked_offset, true);

	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_LOCKED,
		      "good sample should preserve servo lock");
	zassert_equal(ptp_clk.sync_discipline.outlier_samples, 0,
		      "good sample should clear the outlier count");
}

ZTEST(ptp_clock_wakeup, test_synchronize_hard_steps_large_offset_and_resets_delay)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	ptp_clk.timestamp.t3 = 1234;
	ptp_clk.timestamp.t4 = 5678;
	fake_ptp_clock_time.second = 10;
	fake_ptp_clock_time.nanosecond = 500;

	ptp_clock_synchronize(10ULL * NSEC_PER_SEC + 500, 8ULL * NSEC_PER_SEC, true);

	zassert_equal(fake_ptp_clock_get_calls, 2, "hard step should resample PHC time");
	zassert_equal(fake_ptp_clock_set_calls, 1, "large offset should hard-step PHC");
	zassert_equal(fake_ptp_clock_last_set_time.second, 8, "hard-step seconds mismatch");
	zassert_equal(fake_ptp_clock_last_set_time.nanosecond, 100,
		      "hard-step nanoseconds mismatch");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "hard step should reset mean delay");
	zassert_equal(ptp_clk.timestamp.t1, 0, "hard step should clear timestamps");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1, "hard step should reset servo rate");
	zassert_equal(fake_ptp_clock_last_rate_ratio, 1.0, "servo reset should use nominal rate");
}

ZTEST(ptp_clock_wakeup, test_synchronize_stops_when_hard_step_phc_read_fails)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 10;
	fake_ptp_clock_time.nanosecond = 500;
	fake_ptp_clock_get_second_ret = -EIO;

	ptp_clock_synchronize(10ULL * NSEC_PER_SEC + 500, 8ULL * NSEC_PER_SEC, true);

	zassert_equal(fake_ptp_clock_get_calls, 2, "hard step should resample PHC time");
	zassert_equal(fake_ptp_clock_set_calls, 0, "failed PHC read should prevent hard step");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 0,
		      "failed hard step should not issue another clock operation");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_FAULT,
		      "failed hard-step read should fault the discipline");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0, "failed hard step should clear mean delay");
}

ZTEST(ptp_clock_wakeup, test_synchronize_faults_servo_after_hard_step_set_fails)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 10;
	fake_ptp_clock_time.nanosecond = 500;
	fake_ptp_clock_set_ret = -EIO;

	ptp_clock_synchronize(10ULL * NSEC_PER_SEC + 500, 8ULL * NSEC_PER_SEC, true);

	zassert_equal(fake_ptp_clock_get_calls, 2, "hard step should resample PHC time");
	zassert_equal(fake_ptp_clock_set_calls, 1, "hard step should attempt to set PHC");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 0,
		      "failed hard step set should not issue another clock operation");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_FAULT,
		      "failed hard-step set should fault the discipline");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0,
		      "failed hard step set should clear mean delay");
}

ZTEST(ptp_clock_wakeup, test_synchronize_tolerates_unsupported_hard_step)
{
	ptp_clk.phc = &fake_phc;
	ptp_clk.current_ds.mean_delay = (ptp_timeinterval)100 << 16;
	fake_ptp_clock_time.second = 10;
	fake_ptp_clock_time.nanosecond = 500;
	fake_ptp_clock_set_ret = -ENOTSUP;

	ptp_clock_synchronize(10ULL * NSEC_PER_SEC + 500, 8ULL * NSEC_PER_SEC, true);

	zassert_equal(fake_ptp_clock_set_calls, 1, "hard step should be attempted once");
	zassert_equal(fake_ptp_clock_rate_adjust_calls, 1,
		      "unsupported hard step should still reset the servo rate");
	zassert_equal(ptp_clk.sync_discipline.state, PRECISION_SYNC_UNSYNCED,
		      "unsupported hard step should not fault the discipline");
	zassert_equal(ptp_clk.current_ds.mean_delay, 0,
		      "unsupported hard step should reset stale delay data");
}

ZTEST_SUITE(ptp_clock_wakeup, NULL, NULL, clock_wakeup_before, NULL, NULL);
