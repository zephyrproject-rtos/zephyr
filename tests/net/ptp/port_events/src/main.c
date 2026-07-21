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
#include <zephyr/net/ptp.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "clock.h"
#include "msg.h"
#include "port.h"
#include "transport.h"

static struct ptp_default_ds fake_default_ds;
static struct ptp_parent_ds fake_parent_ds;
static struct ptp_current_ds fake_current_ds;
static struct ptp_time_prop_ds fake_time_prop_ds;
static struct ptp_msg msg_pool[4];
static struct ptp_msg last_tx_msg;
static struct ptp_msg scripted_rx_msg;
static struct ptp_msg scripted_timestamp_msg;
static struct ptp_tlv_container fake_tlv_container;
static uint8_t fake_added_tlv[sizeof(struct ptp_tlv_mgmt_err)];
static struct net_if fake_iface;
static const struct device fake_phc;
static uint32_t fake_random_value;
static int random_get_calls;

void fake_z_impl_sys_rand_get(void *dst, size_t len);

static bool alloc_should_fail;
static size_t msg_pool_next;
static int transport_recv_ret;
static int msg_post_recv_ret;
static int transport_send_ret;
static int management_process_ret;
static int msg_alloc_calls;
static int msg_unref_calls;
static int msg_pre_send_calls;
static int transport_recv_calls;
static int transport_send_calls;
static int management_process_calls;
static int timestamp_register_calls;
static int timestamp_unregister_calls;
static int clock_sync_calls;
static int clock_sync_with_delay_calls;
static int clock_delay_calls;
static int clock_pdelay_calls;
static uint64_t last_delay_egress;
static uint64_t last_delay_ingress;
static uint64_t last_sync_ingress;
static uint64_t last_sync_egress;
static ptp_timeinterval last_sync_mean_delay;
static bool last_sync_ingress_ts_valid;
static int64_t last_pdelay_t1;
static int64_t last_pdelay_t2;
static int64_t last_pdelay_t3;
static int64_t last_pdelay_t4;
static ptp_timeinterval last_pdelay_correction_resp;
static ptp_timeinterval last_pdelay_correction_fup;
static struct ptp_port *timestamp_cb_port;
static struct net_if_timestamp_cb *last_timestamp_handle;
static net_if_timestamp_callback_t last_timestamp_cb;

static void set_clk_id(ptp_clk_id *clk_id, uint8_t value)
{
	memset(clk_id->id, value, sizeof(clk_id->id));
}

struct ptp_msg *ptp_msg_alloc(void)
{
	struct ptp_msg *msg;

	msg_alloc_calls++;
	if (alloc_should_fail || msg_pool_next >= ARRAY_SIZE(msg_pool)) {
		return NULL;
	}

	msg = &msg_pool[msg_pool_next++];
	memset(msg, 0, sizeof(*msg));
	sys_slist_init(&msg->tlvs);
	atomic_set(&msg->ref, 1);

	return msg;
}

void ptp_msg_unref(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);

	msg_unref_calls++;
}

void ptp_msg_ref(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);
}

enum ptp_msg_type ptp_msg_type(const struct ptp_msg *msg)
{
	return (enum ptp_msg_type)(msg->header.type_major_sdo_id & 0xF);
}

struct ptp_msg *ptp_msg_from_pkt(struct net_pkt *pkt)
{
	ARG_UNUSED(pkt);

	return &scripted_timestamp_msg;
}

void ptp_msg_pre_send(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);

	msg_pre_send_calls++;
}

int ptp_msg_post_recv(struct ptp_port *port, struct ptp_msg *msg, int cnt)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);
	ARG_UNUSED(cnt);

	return msg_post_recv_ret;
}

int ptp_msg_announce_cmp(const struct ptp_announce_msg *m1, const struct ptp_announce_msg *m2)
{
	size_t offset = offsetof(struct ptp_announce_msg, gm_priority1);
	size_t len = offsetof(struct ptp_announce_msg, time_src) - offset;

	return memcmp((const uint8_t *)m1 + offset, (const uint8_t *)m2 + offset, len);
}

bool ptp_msg_current_parent(const struct ptp_msg *msg)
{
	return ptp_port_id_eq(&fake_parent_ds.port_id, &msg->header.src_port_id);
}

struct ptp_tlv *ptp_msg_add_tlv(struct ptp_msg *msg, int length)
{
	msg->header.msg_length += length;
	memset(&fake_added_tlv, 0, sizeof(fake_added_tlv));

	return (struct ptp_tlv *)fake_added_tlv;
}

int ptp_transport_recv(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	ARG_UNUSED(port);
	ARG_UNUSED(idx);

	transport_recv_calls++;
	if (transport_recv_ret > 0) {
		memcpy(msg, &scripted_rx_msg, sizeof(*msg));
		sys_slist_init(&msg->tlvs);
	}

	return transport_recv_ret;
}

int ptp_transport_send(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	ARG_UNUSED(port);
	ARG_UNUSED(idx);

	memcpy(&last_tx_msg, msg, sizeof(last_tx_msg));
	transport_send_calls++;
	return transport_send_ret;
}

int ptp_transport_open(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return 0;
}

int ptp_transport_close(struct ptp_port *port)
{
	ARG_UNUSED(port);

	return 0;
}

const struct ptp_default_ds *ptp_clock_default_ds(void)
{
	return &fake_default_ds;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &fake_parent_ds;
}

const struct ptp_current_ds *ptp_clock_current_ds(void)
{
	return &fake_current_ds;
}

const struct ptp_time_prop_ds *ptp_clock_time_prop_ds(void)
{
	return &fake_time_prop_ds;
}

void ptp_clock_synchronize(uint64_t ingress, uint64_t egress, bool ingress_ts_valid)
{
	clock_sync_calls++;
	last_sync_ingress = ingress;
	last_sync_egress = egress;
	last_sync_ingress_ts_valid = ingress_ts_valid;
}

void ptp_clock_synchronize_with_delay(uint64_t ingress, uint64_t egress,
				      ptp_timeinterval mean_delay, bool ingress_ts_valid)
{
	clock_sync_with_delay_calls++;
	last_sync_ingress = ingress;
	last_sync_egress = egress;
	last_sync_mean_delay = mean_delay;
	last_sync_ingress_ts_valid = ingress_ts_valid;
}

void ptp_clock_delay(uint64_t egress, uint64_t ingress)
{
	clock_delay_calls++;
	last_delay_egress = egress;
	last_delay_ingress = ingress;
}

int ptp_clock_pdelay(struct ptp_port *port, int64_t t1, int64_t t2, int64_t t3, int64_t t4,
		     ptp_timeinterval correction_resp, ptp_timeinterval correction_fup)
{
	ARG_UNUSED(port);

	clock_pdelay_calls++;
	last_pdelay_t1 = t1;
	last_pdelay_t2 = t2;
	last_pdelay_t3 = t3;
	last_pdelay_t4 = t4;
	last_pdelay_correction_resp = correction_resp;
	last_pdelay_correction_fup = correction_fup;

	return 0;
}

void ptp_clock_pollfd_invalidate(void)
{
	/* Test fakes do not own the clock pollfd set. */
}

void ptp_clock_signal_timeout(void)
{
	/* Timer signaling is driven directly by the tests. */
}

void ptp_clock_state_decision_req(void)
{
	/* Tests call the state decision paths explicitly. */
}

void ptp_clock_port_add(struct ptp_port *port)
{
	ARG_UNUSED(port);
}

struct ptp_port *ptp_clock_port_from_iface(struct net_if *iface)
{
	return iface == &fake_iface ? timestamp_cb_port : NULL;
}

int ptp_clock_management_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	ARG_UNUSED(port);
	ARG_UNUSED(msg);

	management_process_calls++;
	return management_process_ret;
}

enum ptp_port_state ptp_state_machine(enum ptp_port_state state, enum ptp_port_event event,
				      bool tt_diff)
{
	ARG_UNUSED(event);
	ARG_UNUSED(tt_diff);

	return state;
}

enum ptp_port_state ptp_tr_state_machine(enum ptp_port_state state, enum ptp_port_event event,
					 bool tt_diff)
{
	ARG_UNUSED(event);
	ARG_UNUSED(tt_diff);

	return state;
}

static enum ptp_port_state disabled_state_machine(enum ptp_port_state state,
						  enum ptp_port_event event, bool tt_diff)
{
	ARG_UNUSED(state);
	ARG_UNUSED(event);
	ARG_UNUSED(tt_diff);

	return PTP_PS_DISABLED;
}

int ptp_tlv_post_recv(struct ptp_tlv *tlv)
{
	ARG_UNUSED(tlv);

	return 0;
}

void ptp_tlv_pre_send(struct ptp_tlv *tlv)
{
	ARG_UNUSED(tlv);
}

enum ptp_mgmt_op ptp_mgmt_action(struct ptp_msg *msg)
{
	return msg->management.action;
}

struct ptp_tlv_container *ptp_tlv_alloc(void)
{
	memset(&fake_tlv_container, 0, sizeof(fake_tlv_container));

	return &fake_tlv_container;
}

void ptp_tlv_free(struct ptp_tlv_container *tlv_container)
{
	ARG_UNUSED(tlv_container);
}

void fake_net_if_register_timestamp_cb(struct net_if_timestamp_cb *handle, struct net_pkt *pkt,
				       struct net_if *iface, net_if_timestamp_callback_t cb)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(iface);

	last_timestamp_handle = handle;
	last_timestamp_cb = cb;
	timestamp_register_calls++;
}

void fake_net_if_unregister_timestamp_cb(struct net_if_timestamp_cb *handle)
{
	if (handle == last_timestamp_handle) {
		last_timestamp_handle = NULL;
		last_timestamp_cb = NULL;
	}
	timestamp_unregister_calls++;
}

const struct device *fake_net_eth_get_ptp_clock(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return &fake_phc;
}

int fake_ptp_clock_get(const struct device *dev, struct net_ptp_time *tm)
{
	ARG_UNUSED(dev);

	memset(tm, 0, sizeof(*tm));
	return 0;
}

enum net_if_oper_state fake_net_if_oper_state(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return NET_IF_OPER_UP;
}

void fake_z_impl_sys_rand_get(void *dst, size_t len)
{
	zassert_equal(len, sizeof(fake_random_value), "unexpected random request size");
	random_get_calls++;
	memcpy(dst, &fake_random_value, sizeof(fake_random_value));
}

static void timer_dummy_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}

static void init_port(struct ptp_port *port, enum ptp_port_state state)
{
	memset(port, 0, sizeof(*port));
	port->iface = &fake_iface;
	port->socket[PTP_SOCKET_EVENT] = 7;
	port->socket[PTP_SOCKET_GENERAL] = 8;
	port->port_ds.id.port_number = 1;
	set_clk_id(&port->port_ds.id.clk_id, 0xAA);
	port->port_ds.state = state;
	port->port_ds.announce_receipt_timeout = 3;
	port->port_ds.log_announce_interval = 0;
	port->port_ds.log_sync_interval = 0;
	port->port_ds.log_min_delay_req_interval = 0;
	port->port_ds.log_min_pdelay_req_interval = 0;
	port->port_ds.delay_mechanism = PTP_DM_E2E;
	port->neighbor_rate_ratio = 1.0;
	sys_slist_init(&port->foreign_list);
	sys_slist_init(&port->delay_req_list);
	k_timer_init(&port->timers.announce, timer_dummy_cb, NULL);
	k_timer_init(&port->timers.delay, timer_dummy_cb, NULL);
	k_timer_init(&port->timers.sync, timer_dummy_cb, NULL);
	k_timer_init(&port->timers.qualification, timer_dummy_cb, NULL);
	k_timer_init(&port->timers.pdelay, timer_dummy_cb, NULL);
}

static void stop_port_timers(struct ptp_port *port)
{
	k_timer_stop(&port->timers.announce);
	k_timer_stop(&port->timers.delay);
	k_timer_stop(&port->timers.sync);
	k_timer_stop(&port->timers.qualification);
	k_timer_stop(&port->timers.pdelay);
}

static void init_rx_msg(enum ptp_msg_type type, uint8_t sender_id)
{
	memset(&scripted_rx_msg, 0, sizeof(scripted_rx_msg));
	scripted_rx_msg.header.type_major_sdo_id = type;
	scripted_rx_msg.header.version = PTP_VERSION;
	scripted_rx_msg.header.src_port_id.port_number = 2;
	set_clk_id(&scripted_rx_msg.header.src_port_id.clk_id, sender_id);

	switch (type) {
	case PTP_MSG_ANNOUNCE:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_announce_msg);
		scripted_rx_msg.announce.gm_priority1 = 100;
		scripted_rx_msg.announce.gm_priority2 = 100;
		scripted_rx_msg.announce.steps_rm = 1;
		scripted_rx_msg.announce.gm_clk_quality.cls = 128;
		scripted_rx_msg.announce.gm_clk_quality.accuracy = 0x31;
		scripted_rx_msg.announce.gm_clk_quality.offset_scaled_log_variance = 0x200;
		set_clk_id(&scripted_rx_msg.announce.gm_id, (uint8_t)(sender_id + 1));
		break;
	case PTP_MSG_DELAY_REQ:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_delay_req_msg);
		break;
	case PTP_MSG_DELAY_RESP:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_delay_resp_msg);
		break;
	case PTP_MSG_PDELAY_REQ:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_pdelay_req_msg);
		break;
	case PTP_MSG_PDELAY_RESP:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_pdelay_resp_msg);
		break;
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_pdelay_resp_follow_up_msg);
		break;
	case PTP_MSG_FOLLOW_UP:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_follow_up_msg);
		break;
	case PTP_MSG_MANAGEMENT:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_management_msg);
		break;
	case PTP_MSG_SYNC:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_sync_msg);
		break;
	default:
		scripted_rx_msg.header.msg_length = sizeof(struct ptp_msg);
		break;
	}
}

static void init_timestamp_msg(enum ptp_msg_type type, const struct ptp_port *port, uint16_t seq)
{
	memset(&scripted_timestamp_msg, 0, sizeof(scripted_timestamp_msg));
	scripted_timestamp_msg.header.type_major_sdo_id = type;
	scripted_timestamp_msg.header.src_port_id = port->port_ds.id;
	scripted_timestamp_msg.header.src_port_id.port_number =
		net_htons(port->port_ds.id.port_number);
	scripted_timestamp_msg.header.sequence_id = net_htons(seq);
}

static void fire_timestamp_cb(struct ptp_port *port, uint64_t sec, uint32_t nsec)
{
	struct net_pkt pkt = {0};

	zassert_not_null(last_timestamp_cb, "timestamp callback not registered");

	timestamp_cb_port = port;
	pkt.iface = &fake_iface;
	pkt.timestamp.second = sec;
	pkt.timestamp.nanosecond = nsec;
	last_timestamp_cb(&pkt);
}

static void reset_fakes(void)
{
	memset(&fake_default_ds, 0, sizeof(fake_default_ds));
	memset(&fake_parent_ds, 0, sizeof(fake_parent_ds));
	memset(&fake_current_ds, 0, sizeof(fake_current_ds));
	memset(&fake_time_prop_ds, 0, sizeof(fake_time_prop_ds));
	memset(msg_pool, 0, sizeof(msg_pool));
	memset(&last_tx_msg, 0, sizeof(last_tx_msg));
	memset(&scripted_rx_msg, 0, sizeof(scripted_rx_msg));
	memset(&scripted_timestamp_msg, 0, sizeof(scripted_timestamp_msg));
	memset(&fake_tlv_container, 0, sizeof(fake_tlv_container));
	memset(&fake_added_tlv, 0, sizeof(fake_added_tlv));
	fake_default_ds.domain = 1;
	fake_default_ds.max_steps_rm = 255;
	fake_parent_ds.gm_priority1 = 100;
	fake_parent_ds.gm_priority2 = 100;
	fake_parent_ds.gm_clk_quality.cls = 128;
	fake_parent_ds.gm_clk_quality.accuracy = 0x31;
	fake_time_prop_ds.current_utc_offset = 37;
	fake_time_prop_ds.time_src = 0x20;
	alloc_should_fail = false;
	msg_pool_next = 0;
	transport_recv_ret = 64;
	msg_post_recv_ret = 0;
	transport_send_ret = 0;
	management_process_ret = 0;
	msg_alloc_calls = 0;
	msg_unref_calls = 0;
	msg_pre_send_calls = 0;
	transport_recv_calls = 0;
	transport_send_calls = 0;
	management_process_calls = 0;
	timestamp_register_calls = 0;
	timestamp_unregister_calls = 0;
	clock_sync_calls = 0;
	clock_sync_with_delay_calls = 0;
	clock_delay_calls = 0;
	clock_pdelay_calls = 0;
	last_delay_egress = 0;
	last_delay_ingress = 0;
	last_sync_ingress = 0;
	last_sync_egress = 0;
	last_sync_mean_delay = 0;
	last_sync_ingress_ts_valid = false;
	last_pdelay_t1 = 0;
	last_pdelay_t2 = 0;
	last_pdelay_t3 = 0;
	last_pdelay_t4 = 0;
	last_pdelay_correction_resp = 0;
	last_pdelay_correction_fup = 0;
	timestamp_cb_port = NULL;
	last_timestamp_handle = NULL;
	last_timestamp_cb = NULL;
	fake_random_value = 0;
	random_get_calls = 0;
}

static void port_events_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_fakes();
}

ZTEST(ptp_port_events, test_event_gen_invalid_index_is_noop)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);

	zassert_equal(ptp_port_event_gen(&port, -1), PTP_EVT_NONE, "invalid index event");
	zassert_equal(msg_alloc_calls, 0, "message should not be allocated");
}

ZTEST(ptp_port_events, test_event_gen_allocation_failure_reports_fault)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	alloc_should_fail = true;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_FAULT_DETECTED,
		      "allocation failure should report fault");
}

ZTEST(ptp_port_events, test_event_gen_receive_failure_reports_fault)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	transport_recv_ret = -EIO;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_FAULT_DETECTED,
		      "receive failure should report fault");
	zassert_equal(msg_unref_calls, 1, "message should be unreferenced");
}

ZTEST(ptp_port_events, test_event_gen_post_recv_failure_reports_fault)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	init_rx_msg(PTP_MSG_SYNC, 0x10);
	msg_post_recv_ret = -EBADMSG;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_FAULT_DETECTED,
		      "post-receive failure should report fault");
	zassert_equal(msg_unref_calls, 1, "message should be unreferenced");
}

ZTEST(ptp_port_events, test_event_gen_ignores_self_originated_message)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	init_rx_msg(PTP_MSG_SYNC, 0xAA);
	scripted_rx_msg.header.src_port_id.port_number = port.port_ds.id.port_number;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "self-originated message should be ignored");
	zassert_equal(msg_unref_calls, 1, "message should be unreferenced");
}

ZTEST(ptp_port_events, test_event_gen_announce_threshold_requests_state_decision)
{
	struct ptp_port port;
	struct ptp_msg first_announce;

	init_port(&port, PTP_PS_LISTENING);
	init_rx_msg(PTP_MSG_ANNOUNCE, 0x20);
	first_announce = scripted_rx_msg;

	zassert_equal(ptp_port_add_foreign_tt(&port, &first_announce), 0,
		      "first announce should prime threshold");
	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_STATE_DECISION,
		      "second announce should request state decision");

	stop_port_timers(&port);
	ptp_port_free_foreign_tts(&port);
}

ZTEST(ptp_port_events, test_event_gen_management_can_request_state_decision)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	init_rx_msg(PTP_MSG_MANAGEMENT, 0x30);
	scripted_rx_msg.header.msg_length = sizeof(struct ptp_management_msg);
	management_process_ret = 1;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_STATE_DECISION,
		      "management processing should request state decision");
	zassert_equal(management_process_calls, 1, "management processor not called");
}

ZTEST(ptp_port_events, test_event_gen_delay_req_transmitter_sends_response)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	init_rx_msg(PTP_MSG_DELAY_REQ, 0x40);
	scripted_rx_msg.header.sequence_id = 7;
	scripted_rx_msg.header.correction = 4;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 20;
	scripted_rx_msg.rx_timestamp_valid = true;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Delay_Req should not change port event state");
	zassert_equal(transport_send_calls, 1, "Delay_Resp not sent");
	zassert_equal(ptp_msg_type(&last_tx_msg), PTP_MSG_DELAY_RESP, "unexpected response type");
	zassert_equal(last_tx_msg.header.sequence_id, scripted_rx_msg.header.sequence_id,
		      "response sequence mismatch");
	zassert_true(ptp_port_id_eq(&last_tx_msg.delay_resp.req_port_id,
				    &scripted_rx_msg.header.src_port_id),
		     "requesting port ID mismatch");
}

ZTEST(ptp_port_events, test_event_gen_delay_req_without_rx_timestamp_is_dropped)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	init_rx_msg(PTP_MSG_DELAY_REQ, 0x41);
	scripted_rx_msg.header.sequence_id = 8;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 20;
	scripted_rx_msg.rx_timestamp_valid = false;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "dropped Delay_Req should not change port event state");
	zassert_equal(transport_send_calls, 0,
		      "Delay_Req without a valid RX timestamp should not be answered");
}

ZTEST(ptp_port_events, test_event_gen_delay_resp_time_receiver_updates_delay)
{
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	memset(&req, 0, sizeof(req));
	req.header.sequence_id = net_htons(3);
	req.timestamp.host.second = 1;
	req.timestamp.host.nanosecond = 20;
	sys_slist_append(&port.delay_req_list, &req.node);

	init_rx_msg(PTP_MSG_DELAY_RESP, 0x50);
	scripted_rx_msg.header.sequence_id = 3;
	scripted_rx_msg.header.correction = 10 << 16;
	scripted_rx_msg.header.log_msg_interval = -2;
	scripted_rx_msg.delay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.timestamp.protocol.second = 2;
	scripted_rx_msg.timestamp.protocol.nanosecond = 30;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_NONE,
		      "Delay_Resp should not change port event state");
	zassert_equal(clock_delay_calls, 1, "delay calculation not requested");
	zassert_equal(last_delay_egress, NSEC_PER_SEC + 20, "egress timestamp mismatch");
	zassert_equal(last_delay_ingress, 2 * NSEC_PER_SEC + 20,
		      "corrected ingress timestamp mismatch");
	zassert_is_null(sys_slist_peek_head(&port.delay_req_list),
			"matched Delay_Req should be removed");
	zassert_equal(port.port_ds.log_min_delay_req_interval, -2,
		      "delay interval should be updated from response");
}

ZTEST(ptp_port_events, test_timer_pdelay_sends_request_only_for_p2p)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_PDELAY_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.pdelay), PTP_EVT_NONE,
		      "successful Pdelay_Req should not raise event");
	zassert_equal(transport_send_calls, 1, "Pdelay_Req not sent");
	zassert_equal(ptp_msg_type(&last_tx_msg), PTP_MSG_PDELAY_REQ,
		      "unexpected PDelay timer message");
	zassert_equal(timestamp_register_calls, 1, "Pdelay timestamp callback not registered");
	zassert_not_null(port.last_pdelay_req_sent, "Pdelay request should be tracked");
	zassert_false(atomic_test_bit(&port.timeouts, PTP_PORT_TIMER_PDELAY_TO),
		      "Pdelay timeout bit should be cleared");
	stop_port_timers(&port);

	reset_fakes();
	init_port(&port, PTP_PS_TIME_RECEIVER);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_PDELAY_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.pdelay), PTP_EVT_NONE,
		      "E2E port should ignore Pdelay timeout");
	zassert_equal(transport_send_calls, 0, "E2E port should not send Pdelay_Req");
	zassert_is_null(port.last_pdelay_req_sent, "E2E port should not track Pdelay request");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_pdelay_req_generates_two_step_response)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	init_rx_msg(PTP_MSG_PDELAY_REQ, 0x70);
	scripted_rx_msg.header.sequence_id = 6;
	scripted_rx_msg.header.correction = (ptp_timeinterval)3 << 16;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 20;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Pdelay_Req should not change port event state");
	zassert_equal(transport_send_calls, 1, "Pdelay_Resp not sent");
	zassert_equal(ptp_msg_type(&last_tx_msg), PTP_MSG_PDELAY_RESP, "unexpected response type");
	zassert_true(last_tx_msg.header.flags[0] & PTP_MSG_TWO_STEP_FLAG,
		     "Pdelay_Resp should be two-step");
	zassert_equal(last_tx_msg.header.sequence_id, scripted_rx_msg.header.sequence_id,
		      "response sequence mismatch");
	zassert_true(ptp_port_id_eq(&last_tx_msg.pdelay_resp.req_port_id,
				    &scripted_rx_msg.header.src_port_id),
		     "requesting port ID mismatch");
	zassert_equal(last_tx_msg.pdelay_resp.req_receipt_timestamp.seconds_low, 1,
		      "request receipt seconds mismatch");
	zassert_equal(last_tx_msg.pdelay_resp.req_receipt_timestamp.nanoseconds, 20,
		      "request receipt nanoseconds mismatch");

	init_timestamp_msg(PTP_MSG_PDELAY_RESP, &port, scripted_rx_msg.header.sequence_id);
	fire_timestamp_cb(&port, 2, 30);

	zassert_equal(transport_send_calls, 2, "Pdelay_Resp_Follow_Up not sent");
	zassert_equal(ptp_msg_type(&last_tx_msg), PTP_MSG_PDELAY_RESP_FOLLOW_UP,
		      "unexpected follow-up type");
	zassert_equal(last_tx_msg.header.sequence_id, scripted_rx_msg.header.sequence_id,
		      "follow-up sequence mismatch");
	zassert_equal(last_tx_msg.pdelay_resp_follow_up.resp_origin_timestamp.seconds_low, 2,
		      "response origin seconds mismatch");
	zassert_equal(last_tx_msg.pdelay_resp_follow_up.resp_origin_timestamp.nanoseconds, 30,
		      "response origin nanoseconds mismatch");
	zassert_is_null(port.last_pdelay_req_received,
			"responded Pdelay_Req should be cleared after Follow_Up");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_pdelay_resp_follow_up_updates_delay)
{
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	memset(&req, 0, sizeof(req));
	req.timestamp.host.second = 1;
	req.timestamp.host.nanosecond = 100;
	port.last_pdelay_req_sent = &req;
	port.pdelay_req_sequence_id = 9;

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x80);
	scripted_rx_msg.header.sequence_id = 9;
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.header.correction = (ptp_timeinterval)10 << 16;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.protocol.second = 1;
	scripted_rx_msg.timestamp.protocol.nanosecond = 300;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 900;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Pdelay_Resp should not change port event state");
	zassert_equal(clock_pdelay_calls, 0, "Pdelay should wait for Follow_Up");

	init_rx_msg(PTP_MSG_PDELAY_RESP_FOLLOW_UP, 0x80);
	scripted_rx_msg.header.sequence_id = 9;
	scripted_rx_msg.header.correction = (ptp_timeinterval)20 << 16;
	scripted_rx_msg.pdelay_resp_follow_up.req_port_id = port.port_ds.id;
	scripted_rx_msg.timestamp.protocol.second = 1;
	scripted_rx_msg.timestamp.protocol.nanosecond = 500;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_NONE,
		      "Pdelay Follow_Up should not change port event state");
	zassert_equal(clock_pdelay_calls, 1, "Pdelay calculation not requested");
	zassert_equal(last_pdelay_t1, NSEC_PER_SEC + 100, "t1 mismatch");
	zassert_equal(last_pdelay_t2, NSEC_PER_SEC + 300, "t2 mismatch");
	zassert_equal(last_pdelay_t3, NSEC_PER_SEC + 500, "t3 mismatch");
	zassert_equal(last_pdelay_t4, NSEC_PER_SEC + 900, "t4 mismatch");
	zassert_equal(last_pdelay_correction_resp, (ptp_timeinterval)10 << 16,
		      "Pdelay_Resp correction mismatch");
	zassert_equal(last_pdelay_correction_fup, (ptp_timeinterval)20 << 16,
		      "Pdelay Follow_Up correction mismatch");
	zassert_is_null(port.last_pdelay_req_sent,
			"completed Pdelay exchange should clear pending request");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_pdelay_follow_up_before_resp_updates_delay)
{
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	memset(&req, 0, sizeof(req));
	req.timestamp.host.second = 3;
	req.timestamp.host.nanosecond = 100;
	port.last_pdelay_req_sent = &req;
	port.pdelay_req_sequence_id = 12;

	init_rx_msg(PTP_MSG_PDELAY_RESP_FOLLOW_UP, 0x81);
	scripted_rx_msg.header.sequence_id = 12;
	scripted_rx_msg.pdelay_resp_follow_up.req_port_id = port.port_ds.id;
	scripted_rx_msg.timestamp.protocol.second = 3;
	scripted_rx_msg.timestamp.protocol.nanosecond = 700;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_NONE,
		      "early Pdelay Follow_Up should not change port event state");
	zassert_equal(clock_pdelay_calls, 0, "Pdelay should wait for response");

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x81);
	scripted_rx_msg.header.sequence_id = 12;
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.protocol.second = 3;
	scripted_rx_msg.timestamp.protocol.nanosecond = 300;
	scripted_rx_msg.timestamp.host.second = 3;
	scripted_rx_msg.timestamp.host.nanosecond = 900;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Pdelay_Resp should complete out-of-order exchange");
	zassert_equal(clock_pdelay_calls, 1, "Pdelay calculation not requested");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_pdelay_rejects_multiple_responders)
{
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	memset(&req, 0, sizeof(req));
	req.timestamp.host.second = 1;
	req.timestamp.host.nanosecond = 100;
	port.last_pdelay_req_sent = &req;
	port.pdelay_req_sequence_id = 19;

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x80);
	scripted_rx_msg.header.sequence_id = 19;
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 900;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "first Pdelay_Resp should not change port event state");
	zassert_not_null(port.last_pdelay_resp, "first responder should be tracked");

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x81);
	scripted_rx_msg.header.sequence_id = 19;
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 950;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "second Pdelay_Resp should not change port event state");
	zassert_equal(clock_pdelay_calls, 0, "multiple responders should not update delay");
	zassert_is_null(port.last_pdelay_req_sent, "multiple responders should clear request");
	zassert_is_null(port.last_pdelay_resp, "multiple responders should clear response");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_pdelay_rejects_one_step_and_stale_response)
{
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	memset(&req, 0, sizeof(req));
	req.timestamp.host.second = 1;
	req.timestamp.host.nanosecond = 100;
	port.last_pdelay_req_sent = &req;
	port.pdelay_req_sequence_id = 20;

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x90);
	scripted_rx_msg.header.sequence_id = 21;
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "stale Pdelay_Resp should not change port event state");
	zassert_equal(clock_pdelay_calls, 0, "stale response should not update delay");
	zassert_not_null(port.last_pdelay_req_sent, "stale response should not clear request");

	init_rx_msg(PTP_MSG_PDELAY_RESP, 0x90);
	scripted_rx_msg.header.sequence_id = 20;
	scripted_rx_msg.pdelay_resp.req_port_id = port.port_ds.id;
	scripted_rx_msg.rx_timestamp_valid = true;
	scripted_rx_msg.timestamp.host.second = 1;
	scripted_rx_msg.timestamp.host.nanosecond = 900;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "one-step Pdelay_Resp should not change port event state");
	zassert_equal(clock_pdelay_calls, 0, "one-step response should not update delay");
	zassert_is_null(port.last_pdelay_req_sent, "one-step response should clear request");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_sync_follow_up_pair_synchronizes)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	init_rx_msg(PTP_MSG_SYNC, 0x60);
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.header.sequence_id = 11;
	scripted_rx_msg.header.log_msg_interval = -1;
	scripted_rx_msg.timestamp.host.second = 3;
	scripted_rx_msg.timestamp.host.nanosecond = 40;
	scripted_rx_msg.rx_timestamp_valid = true;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Sync should not change port event state");
	zassert_not_null(port.last_sync_fup, "two-step Sync should wait for Follow_Up");
	zassert_equal(clock_sync_calls, 0, "Sync should wait for Follow_Up");

	init_rx_msg(PTP_MSG_FOLLOW_UP, 0x60);
	scripted_rx_msg.header.sequence_id = 11;
	scripted_rx_msg.timestamp.protocol.second = 4;
	scripted_rx_msg.timestamp.protocol.nanosecond = 50;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_NONE,
		      "Follow_Up should not change port event state");
	zassert_is_null(port.last_sync_fup, "matched Sync/Follow_Up should be consumed");
	zassert_equal(clock_sync_calls, 1, "clock synchronization not requested");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_sync_without_rx_timestamp_is_dropped)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	init_rx_msg(PTP_MSG_SYNC, 0x61);
	scripted_rx_msg.header.sequence_id = 12;
	scripted_rx_msg.timestamp.host.second = 5;
	scripted_rx_msg.timestamp.host.nanosecond = 200;
	scripted_rx_msg.timestamp.protocol.second = 5;
	scripted_rx_msg.timestamp.protocol.nanosecond = 100;
	scripted_rx_msg.rx_timestamp_valid = false;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "dropped Sync should not change port event state");
	zassert_equal(clock_sync_calls, 0, "Sync without RX timestamp should not synchronize");
	zassert_equal(clock_sync_with_delay_calls, 0,
		      "Sync without RX timestamp should not synchronize with P2P delay");
	zassert_is_null(port.last_sync_fup, "dropped Sync should not wait for Follow_Up");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_follow_up_first_bad_sync_clears_pair)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	init_rx_msg(PTP_MSG_FOLLOW_UP, 0x62);
	scripted_rx_msg.header.sequence_id = 13;
	scripted_rx_msg.timestamp.protocol.second = 6;
	scripted_rx_msg.timestamp.protocol.nanosecond = 100;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_GENERAL), PTP_EVT_NONE,
		      "Follow_Up should not change port event state");
	zassert_not_null(port.last_sync_fup, "Follow_Up should wait for matching Sync");

	init_rx_msg(PTP_MSG_SYNC, 0x62);
	scripted_rx_msg.header.flags[0] = PTP_MSG_TWO_STEP_FLAG;
	scripted_rx_msg.header.sequence_id = 13;
	scripted_rx_msg.timestamp.host.second = 6;
	scripted_rx_msg.timestamp.host.nanosecond = 200;
	scripted_rx_msg.rx_timestamp_valid = false;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "dropped Sync should not change port event state");
	zassert_is_null(port.last_sync_fup, "bad Sync should clear matching pending Follow_Up");
	zassert_equal(clock_sync_calls, 0, "bad Sync/Follow_Up pair should not synchronize");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_event_gen_sync_uses_mean_link_delay_for_p2p)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	port.port_ds.mean_link_delay = (ptp_timeinterval)50 << 16;
	init_rx_msg(PTP_MSG_SYNC, 0x61);
	scripted_rx_msg.header.sequence_id = 12;
	scripted_rx_msg.timestamp.host.second = 5;
	scripted_rx_msg.timestamp.host.nanosecond = 200;
	scripted_rx_msg.timestamp.protocol.second = 5;
	scripted_rx_msg.timestamp.protocol.nanosecond = 100;
	scripted_rx_msg.rx_timestamp_valid = true;
	fake_parent_ds.port_id = scripted_rx_msg.header.src_port_id;

	zassert_equal(ptp_port_event_gen(&port, PTP_SOCKET_EVENT), PTP_EVT_NONE,
		      "Sync should not change port event state");
	zassert_equal(clock_sync_calls, 0, "P2P sync should not use E2E mean delay path");
	zassert_equal(clock_sync_with_delay_calls, 1,
		      "P2P sync should use explicit mean link delay");
	zassert_equal(last_sync_mean_delay, (ptp_timeinterval)50 << 16, "mean link delay mismatch");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_management_response_fills_port_tlv)
{
	struct ptp_port port;
	struct ptp_msg req;
	struct ptp_tlv_mgmt tlv;

	init_port(&port, PTP_PS_LISTENING);
	port.port_ds.log_sync_interval = -3;
	memset(&req, 0, sizeof(req));
	memset(&tlv, 0, sizeof(tlv));
	req.header.sequence_id = 12;
	req.header.src_port_id.port_number = 99;
	req.management.action = PTP_MGMT_GET;
	req.management.starting_boundary_hops = 5;
	req.management.boundary_hops = 2;
	tlv.id = PTP_MGMT_LOG_SYNC_INTERVAL;

	zassert_equal(ptp_port_management_resp(&port, &req, &tlv), 0,
		      "management response should be sent");
	zassert_equal(transport_send_calls, 1, "management response not sent");
	zassert_equal(ptp_msg_type(&last_tx_msg), PTP_MSG_MANAGEMENT,
		      "unexpected management response type");
	zassert_equal(last_tx_msg.management.action, PTP_MGMT_RESP,
		      "GET should produce a management response");
	zassert_true(last_tx_msg.header.msg_length > sizeof(struct ptp_management_msg),
		     "management TLV should extend response length");
}

ZTEST(ptp_port_events, test_management_set_rejects_delay_mechanism_and_sets_pdelay_interval)
{
	uint8_t storage[sizeof(struct ptp_tlv_mgmt) + 2] = {0};
	struct ptp_tlv_mgmt *tlv = (struct ptp_tlv_mgmt *)storage;
	struct ptp_port port;
	struct ptp_msg req;
	uint32_t max_remaining_ms = 250 + k_ticks_to_ms_ceil32(1);
	uint32_t remaining_ms;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_E2E;
	memset(&req, 0, sizeof(req));
	req.management.action = PTP_MGMT_SET;
	req.management.target_port_id.port_number = port.port_ds.id.port_number;
	req.header.sequence_id = 30;
	req.header.src_port_id.port_number = 99;
	tlv->id = PTP_MGMT_DELAY_MECHANISM;
	tlv->length = sizeof(tlv->id) + sizeof(uint8_t);
	tlv->data[0] = PTP_DM_P2P;

	zassert_ok(ptp_port_management_msg_process(&port, &port, &req, tlv),
		   "delay mechanism SET should send a management error");
	zassert_equal(port.port_ds.delay_mechanism, PTP_DM_E2E,
		      "delay mechanism SET should not update the port");
	zassert_equal(transport_send_calls, 1, "delay mechanism SET error not sent");

	reset_fakes();
	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	port.port_ds.log_min_pdelay_req_interval = 0;
	memset(&req, 0, sizeof(req));
	req.management.action = PTP_MGMT_SET;
	req.management.target_port_id.port_number = port.port_ds.id.port_number;
	req.header.sequence_id = 31;
	req.header.src_port_id.port_number = 99;
	memset(storage, 0, sizeof(storage));
	tlv = (struct ptp_tlv_mgmt *)storage;
	tlv->id = PTP_MGMT_LOG_MIN_PDELAY_REQ_INTERVAL;
	tlv->length = sizeof(tlv->id) + sizeof(uint8_t);
	tlv->data[0] = (uint8_t)-3;
	fake_random_value = BIT(15) - 1;

	zassert_ok(ptp_port_management_msg_process(&port, &port, &req, tlv),
		   "Pdelay interval SET failed");
	zassert_equal(port.port_ds.log_min_pdelay_req_interval, -3, "Pdelay interval not updated");
	zassert_equal(transport_send_calls, 1, "Pdelay interval SET response not sent");
	remaining_ms = k_timer_remaining_get(&port.timers.pdelay);
	zassert_true(remaining_ms >= 200 && remaining_ms <= max_remaining_ms,
		     "negative interval timeout outside expected range: %u ms", remaining_ms);
	zassert_equal(random_get_calls, 1, "valid interval should reschedule Pdelay");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_management_set_rejects_invalid_pdelay_interval)
{
	uint8_t storage[sizeof(struct ptp_tlv_mgmt) + 2] = {0};
	struct ptp_tlv_mgmt *tlv = (struct ptp_tlv_mgmt *)storage;
	struct ptp_tlv_mgmt_err *mgmt_err = (struct ptp_tlv_mgmt_err *)fake_added_tlv;
	struct ptp_port port;
	struct ptp_msg req;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	port.port_ds.log_min_pdelay_req_interval = 0;
	memset(&req, 0, sizeof(req));
	req.management.action = PTP_MGMT_SET;
	req.management.target_port_id.port_number = port.port_ds.id.port_number;
	req.header.sequence_id = 32;
	req.header.src_port_id.port_number = 99;
	tlv->id = PTP_MGMT_LOG_MIN_PDELAY_REQ_INTERVAL;
	tlv->length = sizeof(tlv->id) + sizeof(uint8_t);
	tlv->data[0] = (uint8_t)-63;

	zassert_ok(ptp_port_management_msg_process(&port, &port, &req, tlv),
		   "invalid Pdelay interval should send a management error");
	zassert_equal(port.port_ds.log_min_pdelay_req_interval, 0,
		      "invalid Pdelay interval should preserve the current value");
	zassert_equal(transport_send_calls, 1, "Pdelay interval error response not sent");
	zassert_equal(mgmt_err->err_id, PTP_MGMT_ERR_WRONG_VALUE,
		      "invalid Pdelay interval should report WRONG_VALUE");
	zassert_equal(random_get_calls, 0, "invalid interval should not reschedule Pdelay");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_disable_resets_pdelay_measurement_state)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	port.port_ds.enable = true;
	port.port_ds.delay_mechanism = PTP_DM_P2P;
	port.port_ds.mean_link_delay = (ptp_timeinterval)100 << 16;
	port.neighbor_rate_ratio = 1.0001;
	port.neighbor_rate_ratio_valid = true;
	port.pdelay_prev_resp_origin_ns = 1000;
	port.pdelay_prev_resp_ingress_ns = 2000;
	port.pdelay_prev_rate_sample_valid = true;
	port.state_machine = disabled_state_machine;

	ptp_port_event_handle(&port, PTP_EVT_DESIGNATED_DISABLED, false);

	zassert_equal(port.port_ds.mean_link_delay, 0, "meanLinkDelay should be reset");
	zassert_equal(port.neighbor_rate_ratio, 1.0, "neighbor rate ratio should reset");
	zassert_false(port.neighbor_rate_ratio_valid, "neighbor rate ratio should be invalid");
	zassert_false(port.pdelay_prev_rate_sample_valid, "previous rate sample should reset");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_timer_qualification_timeout)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_PRE_TIME_TRANSMITTER);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_QUALIFICATION_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.qualification),
		      PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES,
		      "qualification timeout event mismatch");
	zassert_false(atomic_test_bit(&port.timeouts, PTP_PORT_TIMER_QUALIFICATION_TO),
		      "qualification timeout bit should be cleared");
}

ZTEST(ptp_port_events, test_timer_listening_announce_timeout)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.announce),
		      PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES, "announce timeout event mismatch");
	zassert_false(atomic_test_bit(&port.timeouts, PTP_PORT_TIMER_ANNOUNCE_TO),
		      "announce timeout bit should be cleared");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_timer_listening_sync_timeout)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_LISTENING);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_SYNC_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.sync),
		      PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES, "sync timeout event mismatch");
	zassert_false(atomic_test_bit(&port.timeouts, PTP_PORT_TIMER_SYNC_TO),
		      "sync timeout bit should be cleared");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_timer_time_transmitter_sync_success_and_failure)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_SYNC_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.sync), PTP_EVT_NONE,
		      "successful sync transmit should not raise event");
	zassert_equal(transport_send_calls, 1, "sync message not sent");
	zassert_equal(timestamp_register_calls, 1, "sync timestamp callback not registered");
	stop_port_timers(&port);

	reset_fakes();
	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	transport_send_ret = -EIO;
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_SYNC_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.sync), PTP_EVT_FAULT_DETECTED,
		      "failed sync transmit should report fault");
	zassert_equal(timestamp_unregister_calls, 1,
		      "failed sync transmit should unregister timestamp callback");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_timer_time_transmitter_announce_success_and_failure)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.announce), PTP_EVT_NONE,
		      "successful announce transmit should not raise event");
	zassert_equal(transport_send_calls, 1, "announce message not sent");
	stop_port_timers(&port);

	reset_fakes();
	init_port(&port, PTP_PS_TIME_TRANSMITTER);
	transport_send_ret = -EIO;
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.announce),
		      PTP_EVT_FAULT_DETECTED, "failed announce transmit should report fault");
	stop_port_timers(&port);
}

ZTEST(ptp_port_events, test_timer_time_receiver_delay_success_and_failure)
{
	struct ptp_port port;

	init_port(&port, PTP_PS_TIME_RECEIVER);
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_DELAY_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.delay), PTP_EVT_NONE,
		      "successful delay request should not raise event");
	zassert_equal(transport_send_calls, 1, "delay request not sent");
	zassert_equal(timestamp_register_calls, 1, "delay timestamp callback not registered");
	zassert_not_null(sys_slist_peek_head(&port.delay_req_list),
			 "successful delay request should be tracked");
	stop_port_timers(&port);

	reset_fakes();
	init_port(&port, PTP_PS_TIME_RECEIVER);
	transport_send_ret = -EIO;
	atomic_set_bit(&port.timeouts, PTP_PORT_TIMER_DELAY_TO);

	zassert_equal(ptp_port_timer_event_gen(&port, &port.timers.delay), PTP_EVT_FAULT_DETECTED,
		      "failed delay request should report fault");
	zassert_equal(msg_unref_calls, 1, "failed delay request should be unreferenced");
	zassert_is_null(sys_slist_peek_head(&port.delay_req_list),
			"failed delay request should not be tracked");
	stop_port_timers(&port);
}

ZTEST_SUITE(ptp_port_events, NULL, NULL, port_events_before, NULL, NULL);
