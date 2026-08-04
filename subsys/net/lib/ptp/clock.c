/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_clock, CONFIG_PTP_LOG_LEVEL);

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/zvfs/eventfd.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/slist.h>
#include <zephyr/precision_timing/precision_clock_ptp.h>
#include <zephyr/precision_timing/precision_timing.h>

#include "btca.h"
#include "clock.h"
#include "ddt.h"
#include "msg.h"
#include "port.h"
#include "tlv.h"
#include "transport.h"

#define MIN_NSEC_TO_TIMEINTERVAL (0xFFFF800000000000ULL)
#define MAX_NSEC_TO_TIMEINTERVAL (0x00007FFFFFFFFFFFULL)
#define INGRESS_TS_PHC_DELTA_GUARD_NS (5ULL * NSEC_PER_SEC)

/*
 * Servo acquisition policy:
 * - offsets above the step threshold are corrected by setting the clock;
 * - three consecutive samples within 10 ms mark the frequency servo as locked;
 * - while locked, offsets above 100 ms are rejected, and two consecutive
 *   outliers reset the servo. The next sample starts acquisition again.
 *
 * Lock is based on samples rather than elapsed time, so acquisition time follows
 * the configured Sync interval. These thresholds protect the PI controller from
 * bad timestamps; they are not clock-accuracy guarantees.
 */
#define SYNC_SERVO_STEP_THRESHOLD_NS (1LL * NSEC_PER_SEC)
#define SYNC_SERVO_LOCK_OFFSET_NS    (10LL * NSEC_PER_MSEC)
#define SYNC_SERVO_OUTLIER_NS        (100LL * NSEC_PER_MSEC)
#define SYNC_SERVO_LOCK_SAMPLES      3U
#define SYNC_SERVO_OUTLIER_SAMPLES   2U
#define SYNC_SERVO_MIN_RATE_PPB      (-999999999)
#define SYNC_SERVO_MAX_RATE_PPB      INT32_MAX
#if defined(CONFIG_PTP_ANNOUNCE_RECV_TIMEOUT)
#define SYNC_SERVO_SOURCE_TIMEOUT_FACTOR CONFIG_PTP_ANNOUNCE_RECV_TIMEOUT
#else
#define SYNC_SERVO_SOURCE_TIMEOUT_FACTOR 3
#endif
#if defined(CONFIG_PTP_SYNC_LOG_INTERVAL)
#define SYNC_SERVO_LOG_INTERVAL CONFIG_PTP_SYNC_LOG_INTERVAL
#else
#define SYNC_SERVO_LOG_INTERVAL 0
#endif
/* Bound the Sync interval decoded from a received message header. IEEE 1588-2019
 * allows the full int8_t range, but 2^24 s and 2^-24 s already exceed anything a
 * sane deployment uses, and the bound keeps the shift below in range.
 */
#define SYNC_SERVO_LOG_INTERVAL_LIMIT 24
/* The source timeout is polled from the event loop, which also runs on every
 * received message. Rate limit the poll to a fraction of the timeout so that a
 * fast Sync rate does not turn every message into an extra PHC read.
 */
#define SYNC_SOURCE_CHECK_DIVIDER     4
#define SYNC_SOURCE_CHECK_MIN_NS      (10LL * NSEC_PER_MSEC)
#define SYNC_SOURCE_CHECK_MAX_NS      (1LL * NSEC_PER_SEC)
/* Keep the last frequency correction while the selected transmitter is absent.
 * A real transmitter identity change resets the discipline explicitly.
 */
#define SYNC_SERVO_HOLDOVER_NS        0

/**
 * @brief PTP Clock structure.
 */
struct ptp_clock {
	const struct device	    *phc;
	struct ptp_default_ds	    default_ds;
	struct ptp_current_ds	    current_ds;
	struct ptp_parent_ds	    parent_ds;
	struct ptp_time_prop_ds	    time_prop_ds;
	struct ptp_dataset	    dataset;
	struct ptp_foreign_tt_clock *best;
	sys_slist_t		    ports_list;
	struct zsock_pollfd	    pollfd[1 + 2 * CONFIG_PTP_NUM_PORTS];
	bool			    pollfd_valid;
	bool			    state_decision_event;
	uint8_t			    time_src;
	struct precision_clock_ptp_adapter precision_phc;
	struct precision_pi_discipline sync_discipline;
	struct precision_time_mapping sync_mapping;
	struct precision_deadline sync_timeout_check;
	bool sync_discipline_ready;
	struct {
		struct ptp_port_id sender;
		ptp_clk_id grandmaster;
		bool reset_pending;
		bool valid;
	} sync_source;
	struct {
		uint64_t	    t1;
		uint64_t	    t2;
		uint64_t	    t3;
		uint64_t	    t4;
	} timestamp;			/* latest timestamps in nanoseconds */
};

__maybe_unused static struct ptp_clock ptp_clk = { 0 };
char str_clock_id[] = "FF:FF:FF:FF:FF:FF:FF:FF";

static int clock_generate_id(ptp_clk_id *clock_id, struct net_if *iface)
{
	struct net_linkaddr *addr = net_if_get_link_addr(iface);

	if (addr) {
		clock_id->id[0] = addr->addr[0];
		clock_id->id[1] = addr->addr[1];
		clock_id->id[2] = addr->addr[2];
		clock_id->id[3] = 0xFF;
		clock_id->id[4] = 0xFE;
		clock_id->id[5] = addr->addr[3];
		clock_id->id[6] = addr->addr[4];
		clock_id->id[7] = addr->addr[5];
		return 0;
	}
	return -1;
}

static const char *clock_id_str(ptp_clk_id *clock_id)
{
	uint8_t *cid = clock_id->id;

	snprintk(str_clock_id, sizeof(str_clock_id), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
		 cid[0],
		 cid[1],
		 cid[2],
		 cid[3],
		 cid[4],
		 cid[5],
		 cid[6],
		 cid[7]);

	return str_clock_id;
}

static ptp_timeinterval clock_ns_to_timeinterval(int64_t val)
{
	if (val < (int64_t)MIN_NSEC_TO_TIMEINTERVAL) {
		val = MIN_NSEC_TO_TIMEINTERVAL;
	} else if (val > (int64_t)MAX_NSEC_TO_TIMEINTERVAL) {
		val = MAX_NSEC_TO_TIMEINTERVAL;
	}

	return (uint64_t)val << 16;
}

static bool clock_sync_source_matches(struct ptp_foreign_tt_clock *best)
{
	return best != NULL && ptp_clk.sync_source.valid &&
	       ptp_port_id_eq(&ptp_clk.sync_source.sender, &best->dataset.sender) &&
	       ptp_clock_id_eq(&ptp_clk.sync_source.grandmaster, &best->dataset.clk_id);
}

static void clock_sync_source_update(struct ptp_foreign_tt_clock *best)
{
	if (best == NULL) {
		return;
	}

	ptp_clk.sync_source.sender = best->dataset.sender;
	ptp_clk.sync_source.grandmaster = best->dataset.clk_id;
	ptp_clk.sync_source.valid = true;
}

static struct precision_time_domain clock_ptp_domain(void)
{
	return (struct precision_time_domain){
		.type = PRECISION_TIME_DOMAIN_PTP,
		.id = ptp_clk.default_ds.domain,
	};
}

static struct precision_time_domain clock_phc_domain(void)
{
	return (struct precision_time_domain){
		.type = PRECISION_TIME_DOMAIN_PHC,
		.id = 0,
	};
}

static int clock_precision_phc(const struct precision_clock **phc)
{
	int ret;

	ret = precision_clock_ptp_init(&ptp_clk.precision_phc, ptp_clk.phc, clock_phc_domain());
	if (ret < 0) {
		return ret;
	}

	*phc = precision_clock_ptp_get(&ptp_clk.precision_phc);

	return 0;
}

static void clock_servo_update_rate_caps(struct precision_pi_config *config)
{
	const struct precision_clock *phc;
	struct precision_clock_caps caps;

	if (ptp_clk.phc == NULL) {
		return;
	}

	if ((clock_precision_phc(&phc) == 0) && (precision_clock_get_caps(phc, &caps) == 0)) {
		config->min_rate_ppb = MAX(config->min_rate_ppb, caps.min_rate_ppb);
		config->max_rate_ppb = MIN(config->max_rate_ppb, caps.max_rate_ppb);
	}
}

static precision_time_t clock_source_timeout_ns(int8_t log_sync_interval)
{
	precision_time_t interval_ns;
	int8_t log_interval = CLAMP(log_sync_interval, -SYNC_SERVO_LOG_INTERVAL_LIMIT,
				    SYNC_SERVO_LOG_INTERVAL_LIMIT);

	if (log_interval < 0) {
		interval_ns = (precision_time_t)NSEC_PER_SEC >> (-log_interval);
	} else {
		interval_ns = (precision_time_t)NSEC_PER_SEC << log_interval;
	}

	return (precision_time_t)SYNC_SERVO_SOURCE_TIMEOUT_FACTOR * interval_ns;
}

static void clock_servo_init(void)
{
	struct precision_pi_config config = {
		.source_domain = clock_ptp_domain(),
		.local_domain = clock_phc_domain(),
		.step_threshold_ns = SYNC_SERVO_STEP_THRESHOLD_NS,
		.lock_threshold_ns = SYNC_SERVO_LOCK_OFFSET_NS,
		.outlier_threshold_ns = SYNC_SERVO_OUTLIER_NS,
		.source_timeout_ns = clock_source_timeout_ns(SYNC_SERVO_LOG_INTERVAL),
		.holdover_ns = SYNC_SERVO_HOLDOVER_NS,
		.lock_sample_count = SYNC_SERVO_LOCK_SAMPLES,
		.outlier_sample_count = SYNC_SERVO_OUTLIER_SAMPLES,
		.min_rate_ppb = SYNC_SERVO_MIN_RATE_PPB,
		.max_rate_ppb = SYNC_SERVO_MAX_RATE_PPB,
		.kp_num = CONFIG_PRECISION_TIMING_PI_KP,
		.ki_num = CONFIG_PRECISION_TIMING_PI_KI,
		.gain_den = PRECISION_PI_GAIN_DEN,
	};
	int ret;

	clock_servo_update_rate_caps(&config);

	/* The configuration is build time constant, so a rejected configuration
	 * cannot be fixed by trying again. Record the attempt and leave a rejected
	 * discipline faulted instead of retrying on every worker wake.
	 */
	ret = precision_pi_init(&ptp_clk.sync_discipline, &config);
	ptp_clk.sync_discipline_ready = true;
	if (ret < 0) {
		LOG_ERR("Failed to initialize PTP sync discipline (err %d)", ret);
		precision_pi_fault(&ptp_clk.sync_discipline);
		return;
	}

	precision_time_mapping_init(&ptp_clk.sync_mapping, config.source_domain,
				    config.local_domain);
	precision_deadline_cancel(&ptp_clk.sync_timeout_check);
}

static void clock_servo_ensure_init(void)
{
	if (!ptp_clk.sync_discipline_ready) {
		clock_servo_init();
	}
}

/* Not every PHC supports rate adjustment. Such a clock can still be stepped, so
 * treat a missing rate adjustment as a no-op instead of faulting the servo.
 */
static int clock_adjust_rate(const struct precision_clock *phc, int32_t rate_ppb)
{
	int ret = precision_clock_adjust_rate(phc, rate_ppb);

	return ret == -ENOTSUP ? 0 : ret;
}

/* A read-only PHC can still be used to exercise the PTP protocol state machine.
 * Preserve that behavior while keeping genuine set failures sticky.
 */
static int clock_set_time(const struct precision_clock *phc,
			  const struct precision_time_point *target)
{
	int ret = precision_clock_set(phc, target);

	return ret == -ENOTSUP ? 0 : ret;
}

static precision_time_t clock_source_check_period_ns(void)
{
	struct precision_pi_config config;

	if (precision_pi_get_config(&ptp_clk.sync_discipline, &config) < 0 ||
	    config.source_timeout_ns <= 0) {
		return SYNC_SOURCE_CHECK_MAX_NS;
	}

	return CLAMP(config.source_timeout_ns / SYNC_SOURCE_CHECK_DIVIDER, SYNC_SOURCE_CHECK_MIN_NS,
		     SYNC_SOURCE_CHECK_MAX_NS);
}

static void clock_sync_data_reset(void);
static int clock_servo_reset(void);
static void clock_servo_fault(void);
static void clock_servo_holdover_apply(void);

static int clock_forward_msg(struct ptp_port *ingress,
			     struct ptp_port *port,
			     struct ptp_msg *msg,
			     bool *network_byte_order)
{
	if (ingress == port) {
		return 0;
	}

	if (*network_byte_order == false) {
		ptp_msg_pre_send(msg);
		*network_byte_order = true;
	}

	return ptp_transport_send(port, msg, PTP_SOCKET_GENERAL);
}

static void clock_forward_management_msg(struct ptp_port *port, struct ptp_msg *msg)
{
	int length;
	struct ptp_port *iter;
	bool net_byte_ord = false;
	enum ptp_port_state state = ptp_port_state(port);

	if (ptp_clock_type() != PTP_CLOCK_TYPE_BOUNDARY) {
		/* Clocks other than Boundary Clock shouldn't retransmit messages. */
		return;
	}

	if (msg->header.flags[0] & PTP_MSG_UNICAST_FLAG) {
		return;
	}

	if (msg->management.boundary_hops &&
	    (state == PTP_PS_GRAND_MASTER ||
	     state == PTP_PS_TIME_TRANSMITTER ||
	     state == PTP_PS_PRE_TIME_TRANSMITTER ||
	     state == PTP_PS_TIME_RECEIVER ||
	     state == PTP_PS_UNCALIBRATED)) {
		length = msg->header.msg_length;
		msg->management.boundary_hops--;

		SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, iter, node) {
			if (clock_forward_msg(port, iter, msg, &net_byte_ord)) {
				LOG_ERR("Failed to forward message to %d Port",
					iter->port_ds.id.port_number);
			}
		}

		if (net_byte_ord) {
			ptp_msg_post_recv(port, msg, length);
			msg->management.boundary_hops++;
		}
	}
}

static int clock_management_set(struct ptp_port *port,
				struct ptp_msg *req,
				struct ptp_tlv_mgmt *tlv)
{
	bool send_resp = false;

	switch (tlv->id) {
	case PTP_MGMT_PRIORITY1:
		ptp_clk.default_ds.priority1 = *tlv->data;
		send_resp = true;
		break;
	case PTP_MGMT_PRIORITY2:
		ptp_clk.default_ds.priority2 = *tlv->data;
		send_resp = true;
		break;
	default:
		break;
	}

	return send_resp ? ptp_port_management_resp(port, req, tlv) : 0;
}

static void clock_update_grandmaster(void)
{
	memset(&ptp_clk.current_ds, 0, sizeof(struct ptp_current_ds));

	memcpy(&ptp_clk.parent_ds.port_id.clk_id,
	       &ptp_clk.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	memcpy(&ptp_clk.parent_ds.gm_id,
	       &ptp_clk.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	ptp_clk.parent_ds.port_id.port_number = 0;
	ptp_clk.parent_ds.gm_clk_quality = ptp_clk.default_ds.clk_quality;
	ptp_clk.parent_ds.gm_priority1 = ptp_clk.default_ds.priority1;
	ptp_clk.parent_ds.gm_priority2 = ptp_clk.default_ds.priority2;

	ptp_clk.time_prop_ds.current_utc_offset = 37; /* IEEE 1588-2019 9.4 */
	ptp_clk.time_prop_ds.time_src = ptp_clk.time_src;
	ptp_clk.time_prop_ds.flags = 0;
}

static void clock_update_time_receiver(void)
{
	struct ptp_msg *best_msg = (struct ptp_msg *)k_fifo_peek_tail(&ptp_clk.best->messages);

	ptp_clk.current_ds.steps_rm = 1 + ptp_clk.best->dataset.steps_rm;

	memcpy(&ptp_clk.parent_ds.gm_id,
	       &best_msg->announce.gm_id,
	       sizeof(best_msg->announce.gm_id));
	memcpy(&ptp_clk.parent_ds.port_id,
	       &ptp_clk.best->dataset.sender,
	       sizeof(ptp_clk.best->dataset.sender));
	ptp_clk.parent_ds.gm_clk_quality = best_msg->announce.gm_clk_quality;
	ptp_clk.parent_ds.gm_priority1 = best_msg->announce.gm_priority1;
	ptp_clk.parent_ds.gm_priority2 = best_msg->announce.gm_priority2;

	ptp_clk.time_prop_ds.current_utc_offset = best_msg->announce.current_utc_offset;
	ptp_clk.time_prop_ds.flags = best_msg->header.flags[1];
}

static void clock_check_pollfd(void)
{
	struct ptp_port *port;
	struct zsock_pollfd *fd = &ptp_clk.pollfd[1];

	if (ptp_clk.pollfd_valid) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		for (int i = 0; i < PTP_SOCKET_CNT; i++) {
			fd->fd = port->socket[i];
			fd->events = ZSOCK_POLLIN | ZSOCK_POLLPRI;
			fd++;
		}
	}

	ptp_clk.pollfd_valid = true;
}

static void clock_notify_worker(void)
{
	if (ptp_clk.pollfd[0].events != ZSOCK_POLLIN) {
		return;
	}

	zvfs_eventfd_write(ptp_clk.pollfd[0].fd, 1);
}

const struct ptp_clock *ptp_clock_init(void)
{
	struct ptp_default_ds *dds = &ptp_clk.default_ds;
	struct ptp_parent_ds *pds  = &ptp_clk.parent_ds;
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	ptp_clk.time_src = (enum ptp_time_src)PTP_TIME_SRC_INTERNAL_OSC;

	/* Initialize Default Dataset. */
	int ret = clock_generate_id(&dds->clk_id, iface);

	if (ret) {
		LOG_ERR("Couldn't assign Clock Identity.");
		return NULL;
	}

	dds->type = (enum ptp_clock_type)CONFIG_PTP_CLOCK_TYPE;
	dds->n_ports = 0;
	dds->time_receiver_only = IS_ENABLED(CONFIG_PTP_TIME_RECEIVER_ONLY) ? true : false;

	dds->clk_quality.cls = dds->time_receiver_only ? 255 : 248;
	dds->clk_quality.accuracy = CONFIG_PTP_CLOCK_ACCURACY;
	/* 0xFFFF means that value has not been computed - IEEE 1588-2019 7.6.3.3 */
	dds->clk_quality.offset_scaled_log_variance = 0xFFFF;

	dds->max_steps_rm = 255;

	dds->priority1 = CONFIG_PTP_PRIORITY1;
	dds->priority2 = CONFIG_PTP_PRIORITY2;

	/* Initialize Parent Dataset. */
	clock_update_grandmaster();
	pds->obsreved_parent_offset_scaled_log_variance = 0xFFFF;
	pds->obsreved_parent_clk_phase_change_rate = 0x7FFFFFFF;
	/* Parent statistics haven't been measured - IEEE 1588-2019 7.6.4.2 */
	pds->stats = false;

	ptp_clk.phc = net_eth_get_ptp_clock(iface);
	if (!ptp_clk.phc) {
		LOG_ERR("Couldn't get PTP HW Clock for the interface.");
		return NULL;
	}
	ptp_clk.sync_source.valid = false;
	ptp_clk.sync_source.reset_pending = false;
	clock_servo_init();

	ret = zvfs_eventfd(0, ZVFS_EFD_NONBLOCK);
	if (ret < 0) {
		LOG_ERR("Failed to create event fd (err %d)", -errno);
		return NULL;
	}
	ptp_clk.pollfd[0].fd = ret;
	ptp_clk.pollfd[0].events = ZSOCK_POLLIN;

	sys_slist_init(&ptp_clk.ports_list);
	LOG_DBG("PTP Clock %s initialized", clock_id_str(&dds->clk_id));
	return &ptp_clk;
}

struct zsock_pollfd *ptp_clock_poll_sockets(void)
{
	int ret;

	clock_check_pollfd();
	ret = zsock_poll(ptp_clk.pollfd, PTP_SOCKET_CNT * ptp_clk.default_ds.n_ports + 1, -1);
	if (ret > 0 && ptp_clk.pollfd[0].revents) {
		zvfs_eventfd_t value;

		zvfs_eventfd_read(ptp_clk.pollfd[0].fd, &value);
	}

	return &ptp_clk.pollfd[1];
}

void ptp_clock_handle_state_decision_evt(void)
{
	struct ptp_foreign_tt_clock *sync_best = NULL;
	struct ptp_foreign_tt_clock *best = NULL, *foreign;
	struct ptp_port *port;
	bool decision_requested = ptp_clk.state_decision_event;
	bool reset_needed = false;

	if (!decision_requested && !ptp_clk.sync_source.reset_pending) {
		return;
	}

	if (sys_slist_is_empty(&ptp_clk.ports_list)) {
		ptp_clk.best = NULL;
		ptp_clk.sync_source.valid = false;
		ptp_clk.sync_source.reset_pending = false;
		ptp_clk.state_decision_event = false;
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		foreign = ptp_port_best_foreign(port);
		if (!foreign) {
			continue;
		}
		if (!best || ptp_btca_ds_cmp(&foreign->dataset, &best->dataset) > 0) {
			best = foreign;
		}
	}

	ptp_clk.best = best;

	/* Decide the state of every Port once. The decision has no side effects,
	 * so it can be taken before the servo is reset for a source change and
	 * reused when the resulting events are dispatched below.
	 */
	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		port->state_decision = ptp_btca_state_decision(port);
		if (port->state_decision == PTP_PS_TIME_RECEIVER) {
			sync_best = best;
		}
	}

	if (sync_best != NULL) {
		reset_needed = ptp_clk.sync_source.valid && !clock_sync_source_matches(sync_best);
	}
	reset_needed = reset_needed || ptp_clk.sync_source.reset_pending;

	if (reset_needed) {
		clock_sync_data_reset();
		if (clock_servo_reset() == 0) {
			ptp_clk.sync_source.reset_pending = false;
			clock_sync_source_update(sync_best);
		} else {
			/* Retry when normal PTP traffic next wakes the worker. */
			ptp_clk.sync_source.reset_pending = true;
		}
	} else {
		clock_sync_source_update(sync_best);
	}

	if (!decision_requested) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		enum ptp_port_event event;

		switch (port->state_decision) {
		case PTP_PS_LISTENING:
			event = PTP_EVT_NONE;
			break;
		case PTP_PS_GRAND_MASTER:
			clock_update_grandmaster();
			clock_servo_holdover_apply();
			event = PTP_EVT_RS_GRAND_MASTER;
			break;
		case PTP_PS_TIME_TRANSMITTER:
			clock_servo_holdover_apply();
			event = PTP_EVT_RS_TIME_TRANSMITTER;
			break;
		case PTP_PS_TIME_RECEIVER:
			clock_update_time_receiver();
			event = PTP_EVT_RS_TIME_RECEIVER;
			break;
		case PTP_PS_PASSIVE:
			event = PTP_EVT_RS_PASSIVE;
			break;
		default:
			event = PTP_EVT_FAULT_DETECTED;
			break;
		}

		ptp_port_event_handle(port, event, false);
	}

	ptp_clk.state_decision_event = false;
}

int ptp_clock_management_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	static const ptp_clk_id all_ones = {
		.id = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
	};
	int ret;
	bool state_decision_required = false;
	enum ptp_mgmt_op action = ptp_mgmt_action(msg);
	struct ptp_port_id *target_port = &msg->management.target_port_id;
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	struct ptp_tlv_mgmt *mgmt = (struct ptp_tlv_mgmt *)msg->management.suffix;
	struct ptp_port *iter;

	if (!ptp_clock_id_eq(&dds->clk_id, &target_port->clk_id) &&
	    !ptp_clock_id_eq(&target_port->clk_id, &all_ones)) {
		return state_decision_required;
	}

	if (sys_slist_len(&msg->tlvs) != 1) {
		/* IEEE 1588-2019 15.3.2 - PTP mgmt msg transports single mgmt TLV */
		return state_decision_required;
	}

	clock_forward_management_msg(port, msg);

	switch (action) {
	case PTP_MGMT_SET:
		ret = clock_management_set(port, msg, mgmt);
		if (ret < 0) {
			return state_decision_required;
		}
		state_decision_required = ret ? true : false;
		break;
	case PTP_MGMT_GET:
		__fallthrough;
	case PTP_MGMT_CMD:
		break;
	default:
		return state_decision_required;
	}

	switch (mgmt->id) {
	case PTP_MGMT_CLOCK_DESCRIPTION:
		__fallthrough;
	case PTP_MGMT_USER_DESCRIPTION:
		__fallthrough;
	case PTP_MGMT_SAVE_IN_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_RESET_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_INITIALIZE:
		__fallthrough;
	case PTP_MGMT_FAULT_LOG:
		__fallthrough;
	case PTP_MGMT_FAULT_LOG_RESET:
		__fallthrough;
	case PTP_MGMT_DOMAIN:
		__fallthrough;
	case PTP_MGMT_TIME_RECEIVER_ONLY:
		__fallthrough;
	case PTP_MGMT_ANNOUNCE_RECEIPT_TIMEOUT:
		__fallthrough;
	case PTP_MGMT_VERSION_NUMBER:
		__fallthrough;
	case PTP_MGMT_ENABLE_PORT:
		__fallthrough;
	case PTP_MGMT_DISABLE_PORT:
		__fallthrough;
	case PTP_MGMT_TIME:
		__fallthrough;
	case PTP_MGMT_CLOCK_ACCURACY:
		__fallthrough;
	case PTP_MGMT_UTC_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_TRACEBILITY_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_TIMESCALE_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_UNICAST_NEGOTIATION_ENABLE:
		__fallthrough;
	case PTP_MGMT_PATH_TRACE_LIST:
		__fallthrough;
	case PTP_MGMT_PATH_TRACE_ENABLE:
		__fallthrough;
	case PTP_MGMT_GRANDMASTER_CLUSTER_TABLE:
		__fallthrough;
	case PTP_MGMT_UNICAST_TIME_TRANSMITTER_TABLE:
		__fallthrough;
	case PTP_MGMT_UNICAST_TIME_TRANSMITTER_MAX_TABLE_SIZE:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_TABLE:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_TABLE_ENABLED:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_MAX_TABLE_SIZE:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_TRANSMITTER:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_ENABLE:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_NAME:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_MAX_KEY:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_EXTERNAL_PORT_CONFIGURATION_ENABLED:
		__fallthrough;
	case PTP_MGMT_TIME_TRANSMITTER_ONLY:
		__fallthrough;
	case PTP_MGMT_HOLDOVER_UPGRADE_ENABLE:
		__fallthrough;
	case PTP_MGMT_EXT_PORT_CONFIG_PORT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_TRANSPARENT_CLOCK_PORT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_PRIMARY_DOMAIN:
		ptp_port_management_error(port, msg, PTP_MGMT_ERR_NOT_SUPPORTED);
		break;
	default:
		if (target_port->port_number == port->port_ds.id.port_number) {
			ptp_port_management_msg_process(port, port, msg, mgmt);
		} else if (target_port->port_number == UINT16_MAX) {
			SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, iter, node) {
				if (ptp_port_management_msg_process(iter, port, msg, mgmt)) {
					break;
				}
			}
		}
		break;
	}

	return state_decision_required;
}

static void clock_sync_data_reset(void)
{
	memset(&ptp_clk.timestamp, 0, sizeof(ptp_clk.timestamp));
	ptp_clk.current_ds.offset_from_tt = 0;
	ptp_clk.current_ds.mean_delay = 0;
}

static void clock_servo_fault(void)
{
	clock_servo_ensure_init();
	precision_pi_fault(&ptp_clk.sync_discipline);
	precision_time_mapping_invalidate(&ptp_clk.sync_mapping);
	precision_deadline_cancel(&ptp_clk.sync_timeout_check);
	clock_sync_data_reset();
}

static int clock_servo_reset(void)
{
	const struct precision_clock *phc;
	int ret;

	clock_servo_ensure_init();
	precision_pi_reset(&ptp_clk.sync_discipline);
	precision_time_mapping_invalidate(&ptp_clk.sync_mapping);
	precision_deadline_cancel(&ptp_clk.sync_timeout_check);

	if (ptp_clk.phc == NULL) {
		return 0;
	}

	ret = clock_precision_phc(&phc);
	if (ret < 0) {
		return ret;
	}

	ret = clock_adjust_rate(phc, 0);
	if (ret < 0) {
		LOG_WRN("Failed to reset PHC rate to nominal (err %d)", ret);
		clock_servo_fault();
	}

	return ret;
}

static void clock_servo_holdover_apply(void)
{
	struct precision_pi_status status;
	const struct precision_clock *phc;
	int ret;

	clock_servo_ensure_init();

	if (ptp_clk.phc == NULL || precision_pi_get_status(&ptp_clk.sync_discipline, &status) < 0 ||
	    !status.has_observation) {
		return;
	}

	ret = clock_precision_phc(&phc);
	if (ret < 0) {
		clock_servo_fault();
		return;
	}

	ret = clock_adjust_rate(phc, status.frequency_correction_ppb);
	if (ret < 0) {
		LOG_WRN("Failed to keep PHC holdover rate (ppb=%d err %d)",
			status.frequency_correction_ppb, ret);
		clock_servo_fault();
	}
}

void ptp_clock_check_source_timeout(void)
{
	struct precision_discipline_result discipline;
	struct precision_pi_status status;
	struct precision_time_point current;
	const struct precision_clock *phc;
	enum precision_sync_state old_state;
	int ret;

	clock_servo_ensure_init();

	if (ptp_clk.phc == NULL || precision_pi_get_status(&ptp_clk.sync_discipline, &status) < 0) {
		return;
	}

	if (status.state == PRECISION_SYNC_FAULT || !status.has_observation) {
		precision_deadline_cancel(&ptp_clk.sync_timeout_check);
		return;
	}

	if (!ptp_clk.sync_timeout_check.scheduled) {
		precision_deadline_schedule(&ptp_clk.sync_timeout_check,
					    clock_source_check_period_ns());
		return;
	}

	/* This runs on every event loop wakeup, so only sample the PHC once per
	 * check period instead of once per received message.
	 */
	if (!precision_deadline_due(&ptp_clk.sync_timeout_check)) {
		return;
	}

	precision_deadline_schedule(&ptp_clk.sync_timeout_check, clock_source_check_period_ns());

	ret = clock_precision_phc(&phc);
	if (ret < 0) {
		clock_servo_fault();
		return;
	}

	ret = precision_clock_read(phc, &current);
	if (ret < 0) {
		LOG_WRN("Failed to read PHC time for source timeout (err %d)", ret);
		clock_servo_fault();
		return;
	}

	old_state = status.state;
	ret = precision_pi_check_source_timeout(&ptp_clk.sync_discipline, current.time,
						&discipline);
	if (ret != -ESTALE) {
		return;
	}

	if (discipline.action == PRECISION_DISCIPLINE_RESET) {
		LOG_WRN("PTP sync holdover expired; resetting servo");
		clock_sync_data_reset();
		(void)clock_servo_reset();
		return;
	}

	if (discipline.state == PRECISION_SYNC_HOLDOVER) {
		if (old_state != PRECISION_SYNC_HOLDOVER) {
			LOG_WRN("PTP sync source timed out; entering holdover");
		}
		clock_servo_holdover_apply();
	}
}

void ptp_clock_sync_interval_update(int8_t log_sync_interval)
{
	precision_time_t timeout_ns = clock_source_timeout_ns(log_sync_interval);
	struct precision_pi_config config;

	clock_servo_ensure_init();

	if (precision_pi_get_config(&ptp_clk.sync_discipline, &config) < 0 ||
	    config.source_timeout_ns == timeout_ns) {
		return;
	}

	(void)precision_pi_set_source_timeout(&ptp_clk.sync_discipline, timeout_ns,
					      config.holdover_ns);
	precision_deadline_cancel(&ptp_clk.sync_timeout_check);
}

static uint64_t clock_abs_delta_u64(uint64_t a, uint64_t b)
{
	return a >= b ? a - b : b - a;
}

static int clock_u64_ns_to_precision(uint64_t ns, precision_time_t *precision_ns)
{
	if (precision_ns == NULL) {
		return -EINVAL;
	}

	if (ns > (uint64_t)PRECISION_TIME_MAX) {
		return -ERANGE;
	}

	*precision_ns = (precision_time_t)ns;

	return 0;
}

static void clock_update_neighbor_rate_ratio(struct ptp_port *port, int64_t resp_origin_ns,
					     int64_t resp_ingress_ns)
{
	if (!port->pdelay_prev_rate_sample_valid) {
		port->neighbor_rate_ratio = 1.0;
		port->neighbor_rate_ratio_valid = false;
		port->pdelay_prev_resp_origin_ns = resp_origin_ns;
		port->pdelay_prev_resp_ingress_ns = resp_ingress_ns;
		port->pdelay_prev_rate_sample_valid = true;
		return;
	}

	int64_t peer_delta = resp_origin_ns - port->pdelay_prev_resp_origin_ns;
	int64_t local_delta = resp_ingress_ns - port->pdelay_prev_resp_ingress_ns;

	port->pdelay_prev_resp_origin_ns = resp_origin_ns;
	port->pdelay_prev_resp_ingress_ns = resp_ingress_ns;

	if (peer_delta <= 0 || local_delta <= 0) {
		port->neighbor_rate_ratio = 1.0;
		port->neighbor_rate_ratio_valid = false;
		return;
	}

	port->neighbor_rate_ratio = (double)peer_delta / (double)local_delta;
	port->neighbor_rate_ratio_valid = true;
}

static void clock_synchronize_with_delay(uint64_t ingress, uint64_t egress,
					 ptp_timeinterval mean_delay, bool ingress_ts_valid)
{
	struct precision_time_observation observation;
	struct precision_discipline_result discipline;
	struct precision_pi_status status;
	struct precision_time_point current;
	struct precision_time_point target;
	const struct precision_clock *phc;
	precision_time_t source_time;
	precision_time_t ingress_time;
	precision_time_t egress_time;
	precision_time_t phc_now_ns;
	precision_time_t offset;
	int ret;
	int64_t delay = mean_delay >> 16;
	uint64_t ingress_phc_delta;
	uint64_t target_sec;
	uint32_t target_nsec;

	clock_servo_ensure_init();
	if (precision_pi_get_status(&ptp_clk.sync_discipline, &status) < 0 ||
	    status.state == PRECISION_SYNC_FAULT) {
		return;
	}

	ret = clock_precision_phc(&phc);
	if (ret < 0) {
		clock_servo_fault();
		return;
	}

	ret = precision_clock_read(phc, &current);
	if (ret < 0) {
		LOG_WRN("Failed to read PHC time (err %d)", ret);
		clock_servo_fault();
		return;
	}

	ret = clock_u64_ns_to_precision(ingress, &ingress_time);
	if (ret < 0) {
		LOG_WRN("Ingress timestamp out of precision range (%" PRIu64 "ns)", ingress);
		return;
	}

	ret = clock_u64_ns_to_precision(egress, &egress_time);
	if (ret < 0) {
		LOG_WRN("Egress timestamp out of precision range (%" PRIu64 "ns)", egress);
		return;
	}

	phc_now_ns = current.time;
	ingress_phc_delta = clock_abs_delta_u64((uint64_t)ingress_time, (uint64_t)phc_now_ns);

	if (!ingress_ts_valid || ingress_phc_delta > INGRESS_TS_PHC_DELTA_GUARD_NS) {
		LOG_WRN("Ingress timestamp fallback (%s): ingress=%" PRIu64 ".%09u phc_now=%" PRIu64
			".%09u |ingress-phc|=%" PRIu64 "ns",
			ingress_ts_valid ? "out-of-range" : "missing", ingress / NSEC_PER_SEC,
			(uint32_t)(ingress % NSEC_PER_SEC), (uint64_t)(phc_now_ns / NSEC_PER_SEC),
			(uint32_t)(phc_now_ns % NSEC_PER_SEC), ingress_phc_delta);

		ingress = (uint64_t)phc_now_ns;
		ingress_time = phc_now_ns;
		ingress_phc_delta = 0;
	}

	ptp_clk.timestamp.t1 = egress;
	ptp_clk.timestamp.t2 = ingress;

	if (mean_delay == 0) {
		return;
	}

	ret = precision_time_add(egress_time, delay, &source_time);
	if (ret < 0) {
		LOG_WRN("Failed to build sync observation (err %d)", ret);
		return;
	}

	observation = (struct precision_time_observation){
		.source = {.time = source_time, .domain = clock_ptp_domain()},
		.local = {.time = ingress_time, .domain = clock_phc_domain()},
		.flags = PRECISION_OBSERVATION_SOURCE_VALID | PRECISION_OBSERVATION_LOCAL_VALID,
	};

	ret = precision_time_sub(observation.local.time, observation.source.time, &offset);
	if (ret < 0) {
		LOG_WRN("Failed to calculate sync offset (err %d)", ret);
		return;
	}

	ret = precision_pi_process(&ptp_clk.sync_discipline, &observation, &discipline);
	if (ret < 0) {
		LOG_WRN("Rejected sync observation: offset=%" PRId64 "ns err %d", offset, ret);
		return;
	}

	if (discipline.action == PRECISION_DISCIPLINE_STEP) {
		LOG_WRN("Clock offset exceeds 1 second (t1=%" PRIu64 ".%09u t2=%" PRIu64
			".%09u delay=%lldns offset=%lldns phc_now=%" PRIu64
			".%09u |t2-phc|=%" PRIu64 "ns)",
			ptp_clk.timestamp.t1 / NSEC_PER_SEC,
			(uint32_t)(ptp_clk.timestamp.t1 % NSEC_PER_SEC),
			ptp_clk.timestamp.t2 / NSEC_PER_SEC,
			(uint32_t)(ptp_clk.timestamp.t2 % NSEC_PER_SEC), delay, offset,
			(uint64_t)(phc_now_ns / NSEC_PER_SEC),
			(uint32_t)(phc_now_ns % NSEC_PER_SEC),
			clock_abs_delta_u64(ptp_clk.timestamp.t2, (uint64_t)phc_now_ns));

		ret = precision_clock_read(phc, &current);
		if (ret < 0) {
			LOG_WRN("Failed to read PHC time for clock step (err %d)", ret);
			clock_servo_fault();
			return;
		}

		target.domain = current.domain;
		ret = precision_time_add(current.time, discipline.phase_correction_ns,
					 &target.time);
		if (ret < 0) {
			LOG_WRN("Failed to calculate PHC step target (err %d)", ret);
			clock_servo_fault();
			return;
		}

		ret = clock_set_time(phc, &target);
		if (ret < 0) {
			LOG_WRN("Failed to set PHC time for clock step (err %d)", ret);
			clock_servo_fault();
			return;
		}

		/* A hard step invalidates the timestamps used by the E2E delay path.
		 * The servo reset also drops any accumulated frequency correction from
		 * the previous time base.
		 */
		clock_sync_data_reset();
		ret = clock_servo_reset();
		if (ret < 0) {
			return;
		}

		ret = precision_time_to_u64_sec_nsec(target.time, &target_sec, &target_nsec);
		if (ret == 0) {
			LOG_WRN("Set clock time: %" PRIu64 ".%09u", target_sec, target_nsec);
		}
		return;
	}

	LOG_DBG("Offset %lldns", offset);
	ptp_clk.current_ds.offset_from_tt = clock_ns_to_timeinterval(offset);

	if (discipline.action == PRECISION_DISCIPLINE_IGNORE) {
		if (precision_pi_get_status(&ptp_clk.sync_discipline, &status) == 0 &&
		    status.outlier_samples > 0) {
			LOG_WRN("Rejecting sync outlier after servo lock: offset=%lldns (%u/%u)",
				offset, (unsigned int)status.outlier_samples,
				SYNC_SERVO_OUTLIER_SAMPLES);
		}
		return;
	}

	if (discipline.action == PRECISION_DISCIPLINE_RESET) {
		LOG_WRN("Rejecting sync outlier after servo lock: offset=%lldns (%u/%u)", offset,
			SYNC_SERVO_OUTLIER_SAMPLES, SYNC_SERVO_OUTLIER_SAMPLES);
		ret = clock_servo_reset();
		if (ret < 0) {
			return;
		}
		return;
	}

	ret = precision_time_mapping_update(&ptp_clk.sync_mapping, &observation);
	if (ret < 0) {
		LOG_DBG("Failed to update PTP sync mapping (err %d)", ret);
	}

	ret = clock_adjust_rate(phc, discipline.rate_ppb);
	if (ret < 0) {
		LOG_WRN("Failed to adjust PHC rate for offset %lldns (ppb=%d err %d), "
			"faulting servo",
			offset, discipline.rate_ppb, ret);
		clock_servo_fault();
		return;
	}
}

void ptp_clock_synchronize(uint64_t ingress, uint64_t egress, bool ingress_ts_valid)
{
	clock_synchronize_with_delay(ingress, egress, ptp_clk.current_ds.mean_delay,
				     ingress_ts_valid);
}

void ptp_clock_synchronize_with_delay(uint64_t ingress, uint64_t egress,
				      ptp_timeinterval mean_delay, bool ingress_ts_valid)
{
	ptp_clk.current_ds.mean_delay = mean_delay;
	clock_synchronize_with_delay(ingress, egress, mean_delay, ingress_ts_valid);
}

void ptp_clock_delay(uint64_t egress, uint64_t ingress)
{
	int64_t delay;

	if (ptp_clk.timestamp.t1 == 0 || ptp_clk.timestamp.t2 == 0) {
		return;
	}

	ptp_clk.timestamp.t3 = egress;
	ptp_clk.timestamp.t4 = ingress;

	delay = ((int64_t)(ptp_clk.timestamp.t2 - ptp_clk.timestamp.t3) +
		 (int64_t)(ptp_clk.timestamp.t4 - ptp_clk.timestamp.t1)) /
		2LL;

	if (llabs(delay) > (int64_t)NSEC_PER_SEC) {
		LOG_WRN("Ignoring unrealistic delay sample: %lldns", delay);
		return;
	}

	LOG_DBG("Delay %lldns", delay);
	ptp_clk.current_ds.mean_delay = clock_ns_to_timeinterval(delay);
}

int ptp_clock_pdelay(struct ptp_port *port, int64_t t1, int64_t t2, int64_t t3, int64_t t4,
		     ptp_timeinterval correction_resp, ptp_timeinterval correction_fup)
{
	int64_t correction_resp_ns;
	int64_t correction_fup_ns;
	int64_t delay_asymmetry_ns;
	int64_t turnaround;
	int64_t delay;

	if (port == NULL || t1 <= 0 || t2 <= 0 || t3 <= 0 || t4 <= 0) {
		return -EINVAL;
	}

	correction_resp_ns = correction_resp >> 16;
	correction_fup_ns = correction_fup >> 16;
	delay_asymmetry_ns = port->port_ds.delay_asymmetry >> 16;
	turnaround = t3 - t2;
	delay = ((t4 - t1) - turnaround - correction_resp_ns - correction_fup_ns -
		 delay_asymmetry_ns) /
		2LL;

	if (delay < 0 || delay > CONFIG_PTP_PEER_DELAY_MAX_NS) {
		LOG_WRN("Ignoring unrealistic peer delay sample: %lldns", delay);
		return -ERANGE;
	}

	LOG_DBG("Peer delay %lldns", delay);
	port->port_ds.mean_link_delay = clock_ns_to_timeinterval(delay);

	clock_update_neighbor_rate_ratio(port, t3 + correction_resp_ns + correction_fup_ns, t4);

	return 0;
}

sys_slist_t *ptp_clock_ports_list(void)
{
	return &ptp_clk.ports_list;
}

enum ptp_clock_type ptp_clock_type(void)
{
	return (enum ptp_clock_type)ptp_clk.default_ds.type;
}

const struct ptp_default_ds *ptp_clock_default_ds(void)
{
	return &ptp_clk.default_ds;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &ptp_clk.parent_ds;
}

const struct ptp_current_ds *ptp_clock_current_ds(void)
{
	return &ptp_clk.current_ds;
}

const struct ptp_time_prop_ds *ptp_clock_time_prop_ds(void)
{
	return &ptp_clk.time_prop_ds;
}

const struct ptp_dataset *ptp_clock_ds(void)
{
	struct ptp_dataset *ds = &ptp_clk.dataset;

	ds->priority1		 = ptp_clk.default_ds.priority1;
	ds->clk_quality		 = ptp_clk.default_ds.clk_quality;
	ds->priority2		 = ptp_clk.default_ds.priority2;
	ds->steps_rm		 = 0;
	ds->sender.port_number	 = 0;
	ds->receiver.port_number = 0;
	memcpy(&ds->clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->sender.clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->receiver.clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	return ds;
}

const struct ptp_dataset *ptp_clock_best_foreign_ds(void)
{
	return ptp_clk.best ? &ptp_clk.best->dataset : NULL;
}

struct ptp_port *ptp_clock_port_from_iface(struct net_if *iface)
{
	struct ptp_port *port;

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		if (port->iface == iface) {
			return port;
		}
	}

	return NULL;
}

void ptp_clock_pollfd_invalidate(void)
{
	bool was_valid = ptp_clk.pollfd_valid;

	ptp_clk.pollfd_valid = false;

	if (was_valid) {
		clock_notify_worker();
	}
}

void ptp_clock_signal_timeout(void)
{
	zvfs_eventfd_write(ptp_clk.pollfd[0].fd, 1);
}

void ptp_clock_state_decision_req(void)
{
	if (ptp_clk.state_decision_event) {
		return;
	}

	ptp_clk.state_decision_event = true;
	clock_notify_worker();
}

void ptp_clock_port_add(struct ptp_port *port)
{
	ptp_clk.default_ds.n_ports++;
	sys_slist_append(&ptp_clk.ports_list, &port->node);
}

const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void)
{
	return ptp_clk.best;
}

bool ptp_clock_id_eq(const ptp_clk_id *c1, const ptp_clk_id *c2)
{
	return memcmp(c1, c2, sizeof(ptp_clk_id)) == 0;
}
