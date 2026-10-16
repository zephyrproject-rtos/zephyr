/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Rapid Spanning Tree Protocol state machines for the CIST, as specified in
 * IEEE Std 802.1Q-2014 Clause 13 for a bridge that runs RSTP or, with Force
 * Protocol Version 0, STP. MSTP, SPB, Layer 2 Gateway Ports, isolate
 * (13.23) and restrictedRole/restrictedTcn are not supported, so
 * rcvdInternal, isolate, restrictedRole and restrictedTcn are always FALSE
 * and enableBPDUrx and enableBPDUtx are always TRUE.
 *
 * The machines run to completion: every call that changes an input runs
 * them until no machine changes state. Port Transmit runs once the others
 * have settled, so that a received BPDU is processed entirely before new
 * BPDUs are encoded (13.34, NOTE 1).
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include "rstp_internal.h"

#define BRIDGE_ADDRESS_MASK 0x0000ffffffffffffULL
#define PORT_NUMBER_MASK    0x0fffU

/* Upper bound of state machine passes in one run. A run settles in a few
 * dozen passes, so reaching it means a defect.
 */
#define MAX_PASSES 256U

enum prx_state {
	PRX_DISCARD,
	PRX_RECEIVE,
};

enum ppm_state {
	PPM_CHECKING_RSTP,
	PPM_SELECTING_STP,
	PPM_SENSING,
};

enum bdm_state {
	BDM_EDGE,
	BDM_NOT_EDGE,
};

enum ptx_state {
	PTX_TRANSMIT_INIT,
	PTX_TRANSMIT_PERIODIC,
	PTX_TRANSMIT_CONFIG,
	PTX_TRANSMIT_TCN,
	PTX_TRANSMIT_RSTP,
	PTX_IDLE,
};

enum pim_state {
	PIM_DISABLED,
	PIM_AGED,
	PIM_UPDATE,
	PIM_CURRENT,
	PIM_RECEIVE,
	PIM_SUPERIOR_DESIGNATED,
	PIM_REPEATED_DESIGNATED,
	PIM_INFERIOR_DESIGNATED,
	PIM_NOT_DESIGNATED,
	PIM_OTHER,
};

enum prs_state {
	PRS_INIT_TREE,
	PRS_ROLE_SELECTION,
};

enum prt_state {
	PRT_INIT_PORT,
	PRT_DISABLE_PORT,
	PRT_DISABLED_PORT,
	PRT_ROOT_PROPOSED,
	PRT_ROOT_AGREED,
	PRT_ROOT_SYNCED,
	PRT_REROOT,
	PRT_ROOT_FORWARD,
	PRT_ROOT_LEARN,
	PRT_REROOTED,
	PRT_ROOT_DISCARD,
	PRT_ROOT_PORT,
	PRT_DESIGNATED_PROPOSE,
	PRT_DESIGNATED_AGREE,
	PRT_DESIGNATED_SYNCED,
	PRT_DESIGNATED_RETIRED,
	PRT_DESIGNATED_FORWARD,
	PRT_DESIGNATED_LEARN,
	PRT_DESIGNATED_DISCARD,
	PRT_DESIGNATED_PORT,
	PRT_ALTERNATE_PROPOSED,
	PRT_ALTERNATE_AGREED,
	PRT_BLOCK_PORT,
	PRT_BACKUP_PORT,
	PRT_ALTERNATE_PORT,
};

enum pst_state {
	PST_DISCARDING,
	PST_LEARNING,
	PST_FORWARDING,
};

enum tcm_state {
	TCM_INACTIVE,
	TCM_LEARNING,
	TCM_DETECTED,
	TCM_ACTIVE,
	TCM_NOTIFIED_TCN,
	TCM_NOTIFIED_TC,
	TCM_PROPAGATING,
	TCM_ACKNOWLEDGED,
};

static void run(struct eth_rstp_bridge *br);

/* 13.28 conditions and parameters */

static bool rstp_version(const struct eth_rstp_bridge *br)
{
	return br->force_version >= 2U;
}

static uint16_t hello_time(const struct eth_rstp_port *p)
{
	return p->port_times.hello_time;
}

static uint16_t fwd_delay(const struct eth_rstp_port *p)
{
	return p->designated_times.forward_delay;
}

static uint16_t max_age(const struct eth_rstp_port *p)
{
	return p->designated_times.max_age;
}

static uint16_t forward_delay(const struct eth_rstp_port *p)
{
	return p->send_rstp ? hello_time(p) : fwd_delay(p);
}

static uint16_t edge_delay(const struct eth_rstp_port *p)
{
	return p->oper_p2p ? ETH_RSTP_MIGRATE_TIME : max_age(p);
}

static bool all_synced(const struct eth_rstp_port *p)
{
	const struct eth_rstp_bridge *br = p->bridge;

	for (size_t i = 0; i < br->num_ports; i++) {
		const struct eth_rstp_port *q = br->ports[i];

		if (q == NULL) {
			continue;
		}

		if (!q->selected || q->role != q->selected_role || q->updt_info) {
			return false;
		}
	}

	if (p->role != ETH_RSTP_ROLE_ROOT && p->role != ETH_RSTP_ROLE_ALTERNATE &&
	    p->role != ETH_RSTP_ROLE_DESIGNATED) {
		return false;
	}

	for (size_t i = 0; i < br->num_ports; i++) {
		const struct eth_rstp_port *q = br->ports[i];

		if (q == NULL || q->synced) {
			continue;
		}

		if (p->role == ETH_RSTP_ROLE_DESIGNATED) {
			if (q != p) {
				return false;
			}
		} else if (q->role != ETH_RSTP_ROLE_ROOT) {
			return false;
		}
	}

	return true;
}

static bool re_rooted(const struct eth_rstp_port *p)
{
	const struct eth_rstp_bridge *br = p->bridge;

	for (size_t i = 0; i < br->num_ports; i++) {
		const struct eth_rstp_port *q = br->ports[i];

		if (q != NULL && q != p && q->rr_while != 0U) {
			return false;
		}
	}

	return true;
}

/* Priority vector comparisons (13.10). A negative result means that @p a is
 * better than @p b. The Receiving Port Identifier is compared only when
 * selecting the root priority vector.
 */
static int prio_cmp(const struct eth_rstp_prio *a, const struct eth_rstp_prio *b)
{
	if (a->root_id != b->root_id) {
		return a->root_id < b->root_id ? -1 : 1;
	}

	if (a->root_path_cost != b->root_path_cost) {
		return a->root_path_cost < b->root_path_cost ? -1 : 1;
	}

	if (a->designated_bridge_id != b->designated_bridge_id) {
		return a->designated_bridge_id < b->designated_bridge_id ? -1 : 1;
	}

	if (a->designated_port_id != b->designated_port_id) {
		return a->designated_port_id < b->designated_port_id ? -1 : 1;
	}

	return 0;
}

static bool prio_better(const struct eth_rstp_prio *a, const struct eth_rstp_prio *b)
{
	return prio_cmp(a, b) < 0;
}

static bool prio_same(const struct eth_rstp_prio *a, const struct eth_rstp_prio *b)
{
	return prio_cmp(a, b) == 0;
}

/* A message is superior if it is better, or if it comes from the same
 * Designated Port as the port priority vector and differs from it (13.10,
 * 13.15).
 */
static bool prio_superior(const struct eth_rstp_prio *msg, const struct eth_rstp_prio *port)
{
	bool same_sender = (msg->designated_bridge_id & BRIDGE_ADDRESS_MASK) ==
				   (port->designated_bridge_id & BRIDGE_ADDRESS_MASK) &&
			   (msg->designated_port_id & PORT_NUMBER_MASK) ==
				   (port->designated_port_id & PORT_NUMBER_MASK);

	return prio_better(msg, port) || (same_sender && !prio_same(msg, port));
}

/* Hello Time is not compared: recordTimes() keeps the fixed value of
 * Table 13-5 rather than the received one (13.29.20).
 */
static bool times_same(const struct eth_rstp_times *a, const struct eth_rstp_times *b)
{
	return a->message_age == b->message_age && a->max_age == b->max_age &&
	       a->forward_delay == b->forward_delay;
}

static uint8_t rcvd_role(const struct eth_rstp_port *p)
{
	if (p->rcvd.type == ETH_RSTP_BPDU_CONFIG) {
		/* A Configuration BPDU conveys a Designated Port Role (13.29.12) */
		return ETH_RSTP_BPDU_ROLE_DESIGNATED;
	}

	return (p->rcvd.flags & ETH_RSTP_FLAG_ROLE_MASK) >> ETH_RSTP_FLAG_ROLE_SHIFT;
}

static bool rcvd_flag(const struct eth_rstp_port *p, uint8_t flag)
{
	return p->rcvd.type != ETH_RSTP_BPDU_TCN && (p->rcvd.flags & flag) != 0U;
}

/* 13.29 procedures */

static bool betterorsame_info(const struct eth_rstp_port *p, enum eth_rstp_info_is new_info_is)
{
	if (new_info_is == ETH_RSTP_INFO_RECEIVED && p->info_is == ETH_RSTP_INFO_RECEIVED) {
		return !prio_better(&p->port_prio, &p->msg_prio);
	}

	if (new_info_is == ETH_RSTP_INFO_MINE && p->info_is == ETH_RSTP_INFO_MINE) {
		return !prio_better(&p->port_prio, &p->designated_prio);
	}

	return false;
}

static void set_sync_tree(struct eth_rstp_bridge *br)
{
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			br->ports[i]->sync = true;
		}
	}
}

static void set_re_root_tree(struct eth_rstp_bridge *br)
{
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			br->ports[i]->re_root = true;
		}
	}
}

static void set_tc_prop_tree(struct eth_rstp_port *p)
{
	struct eth_rstp_bridge *br = p->bridge;

	for (size_t i = 0; i < br->num_ports; i++) {
		struct eth_rstp_port *q = br->ports[i];

		if (q != NULL && q != p) {
			q->tc_prop = true;
		}
	}
}

static void new_tc_while(struct eth_rstp_port *p)
{
	if (p->tc_while != 0U) {
		return;
	}

	if (p->send_rstp) {
		p->tc_while = hello_time(p) + 1U;
		p->new_info = true;
	} else {
		p->tc_while = p->bridge->root_times.max_age + p->bridge->root_times.forward_delay;
	}
}

static enum eth_rstp_rcvd_info rcv_info(const struct eth_rstp_port *p)
{
	uint8_t role;

	if (p->rcvd.type == ETH_RSTP_BPDU_TCN) {
		return ETH_RSTP_RCVD_OTHER;
	}

	role = rcvd_role(p);

	if (role == ETH_RSTP_BPDU_ROLE_DESIGNATED) {
		if (prio_superior(&p->msg_prio, &p->port_prio) ||
		    (prio_same(&p->msg_prio, &p->port_prio) &&
		     !times_same(&p->msg_times, &p->port_times))) {
			return ETH_RSTP_RCVD_SUPERIOR_DESIGNATED;
		}

		if (prio_same(&p->msg_prio, &p->port_prio) &&
		    p->info_is == ETH_RSTP_INFO_RECEIVED) {
			return ETH_RSTP_RCVD_REPEATED_DESIGNATED;
		}

		return ETH_RSTP_RCVD_INFERIOR_DESIGNATED;
	}

	if ((role == ETH_RSTP_BPDU_ROLE_ROOT || role == ETH_RSTP_BPDU_ROLE_ALT_BACKUP) &&
	    !prio_better(&p->msg_prio, &p->port_prio)) {
		return ETH_RSTP_RCVD_INFERIOR_ROOT_ALTERNATE;
	}

	return ETH_RSTP_RCVD_OTHER;
}

static void rcv_msgs(struct eth_rstp_port *p)
{
	const struct eth_rstp_bpdu *b = &p->rcvd;

	if (b->type == ETH_RSTP_BPDU_TCN) {
		p->rcvd_tcn = true;
	} else {
		p->msg_prio.root_id = b->root_id;
		p->msg_prio.root_path_cost = b->root_path_cost;
		p->msg_prio.designated_bridge_id = b->bridge_id;
		p->msg_prio.designated_port_id = b->port_id;
		p->msg_prio.rcv_port_id = p->port_id;
		p->msg_times = b->times;
	}

	p->rcvd_msg = true;
}

static void record_agreement(struct eth_rstp_port *p)
{
	if (rstp_version(p->bridge) && p->oper_p2p && rcvd_flag(p, ETH_RSTP_FLAG_AGREEMENT)) {
		p->agreed = true;
		p->proposing = false;
	} else {
		p->agreed = false;
	}
}

static void record_dispute(struct eth_rstp_port *p)
{
	if (rcvd_flag(p, ETH_RSTP_FLAG_LEARNING)) {
		p->disputed = true;
		p->agreed = false;
	}
}

static void record_proposal(struct eth_rstp_port *p)
{
	if (rcvd_role(p) == ETH_RSTP_BPDU_ROLE_DESIGNATED && rcvd_flag(p, ETH_RSTP_FLAG_PROPOSAL)) {
		p->proposed = true;
	}
}

static void record_times(struct eth_rstp_port *p)
{
	p->port_times = p->msg_times;
	p->port_times.hello_time = ETH_RSTP_HELLO_TIME;
}

static void set_tc_flags(struct eth_rstp_port *p)
{
	if (rcvd_flag(p, ETH_RSTP_FLAG_TC_ACK)) {
		p->rcvd_tc_ack = true;
	}

	if (rcvd_flag(p, ETH_RSTP_FLAG_TC)) {
		p->rcvd_tc = true;
	}
}

static void updt_rcvd_info_while(struct eth_rstp_port *p)
{
	if (p->port_times.message_age + 1U <= p->port_times.max_age) {
		p->rcvd_info_while = 3U * hello_time(p);
	} else {
		p->rcvd_info_while = 0U;
	}
}

static uint8_t bpdu_role(enum eth_rstp_role role)
{
	switch (role) {
	case ETH_RSTP_ROLE_ROOT:
		return ETH_RSTP_BPDU_ROLE_ROOT;
	case ETH_RSTP_ROLE_DESIGNATED:
		return ETH_RSTP_BPDU_ROLE_DESIGNATED;
	case ETH_RSTP_ROLE_ALTERNATE:
	case ETH_RSTP_ROLE_BACKUP:
		return ETH_RSTP_BPDU_ROLE_ALT_BACKUP;
	default:
		return ETH_RSTP_BPDU_ROLE_UNKNOWN;
	}
}

static void tx_message(struct eth_rstp_port *p, enum eth_rstp_bpdu_type type, uint8_t flags)
{
	struct eth_rstp_bpdu bpdu = {
		.type = type,
		.flags = flags,
		.root_id = p->designated_prio.root_id,
		.root_path_cost = p->designated_prio.root_path_cost,
		.bridge_id = p->designated_prio.designated_bridge_id,
		.port_id = p->designated_prio.designated_port_id,
		.times = p->designated_times,
	};

	bpdu.times.hello_time = hello_time(p);
	p->bridge->ops->tx_bpdu(p, &bpdu);
}

static void tx_config(struct eth_rstp_port *p)
{
	uint8_t flags = 0U;

	if (p->tc_while != 0U) {
		flags |= ETH_RSTP_FLAG_TC;
	}

	if (p->tc_ack) {
		flags |= ETH_RSTP_FLAG_TC_ACK;
	}

	tx_message(p, ETH_RSTP_BPDU_CONFIG, flags);
}

static void tx_rstp(struct eth_rstp_port *p)
{
	uint8_t flags = (uint8_t)(bpdu_role(p->role) << ETH_RSTP_FLAG_ROLE_SHIFT);

	if (p->agree) {
		flags |= ETH_RSTP_FLAG_AGREEMENT;
	}

	if (p->proposing) {
		flags |= ETH_RSTP_FLAG_PROPOSAL;
	}

	if (p->tc_while != 0U) {
		flags |= ETH_RSTP_FLAG_TC;
	}

	if (p->learning) {
		flags |= ETH_RSTP_FLAG_LEARNING;
	}

	if (p->forwarding) {
		flags |= ETH_RSTP_FLAG_FORWARDING;
	}

	tx_message(p, ETH_RSTP_BPDU_RST, flags);
}

static void tx_tcn(struct eth_rstp_port *p)
{
	struct eth_rstp_bpdu bpdu = {
		.type = ETH_RSTP_BPDU_TCN,
	};

	p->bridge->ops->tx_bpdu(p, &bpdu);
}

static void bridge_prio(const struct eth_rstp_bridge *br, struct eth_rstp_prio *prio)
{
	*prio = (struct eth_rstp_prio){
		.root_id = br->bridge_id,
		.designated_bridge_id = br->bridge_id,
	};
}

static bool same_bridge_address(uint64_t a, uint64_t b)
{
	return (a & BRIDGE_ADDRESS_MASK) == (b & BRIDGE_ADDRESS_MASK);
}

/* 13.29.34 */
static void updt_roles_tree(struct eth_rstp_bridge *br)
{
	struct eth_rstp_port *root_port = NULL;
	struct eth_rstp_prio root;

	bridge_prio(br, &root);

	for (size_t i = 0; i < br->num_ports; i++) {
		struct eth_rstp_port *q = br->ports[i];
		struct eth_rstp_prio path;

		if (q == NULL || q->info_is != ETH_RSTP_INFO_RECEIVED ||
		    same_bridge_address(q->port_prio.designated_bridge_id, br->bridge_id)) {
			continue;
		}

		path = q->port_prio;
		path.root_path_cost = (path.root_path_cost > UINT32_MAX - q->port_path_cost)
					      ? UINT32_MAX
					      : path.root_path_cost + q->port_path_cost;
		path.rcv_port_id = q->port_id;

		if (prio_better(&path, &root) ||
		    (prio_same(&path, &root) && path.rcv_port_id < root.rcv_port_id)) {
			root = path;
			root_port = q;
		}
	}

	br->root_prio = root;

	if (root_port == NULL) {
		br->root_port_id = 0U;
		br->root_times = br->bridge_times;
	} else {
		br->root_port_id = root_port->port_id;
		br->root_times = root_port->port_times;
		br->root_times.message_age += 1U;
	}

	for (size_t i = 0; i < br->num_ports; i++) {
		struct eth_rstp_port *q = br->ports[i];

		if (q == NULL) {
			continue;
		}

		q->designated_prio = (struct eth_rstp_prio){
			.root_id = root.root_id,
			.root_path_cost = root.root_path_cost,
			.designated_bridge_id = br->bridge_id,
			.designated_port_id = q->port_id,
			.rcv_port_id = q->port_id,
		};
		q->designated_times = br->root_times;

		switch (q->info_is) {
		case ETH_RSTP_INFO_DISABLED:
			q->selected_role = ETH_RSTP_ROLE_DISABLED;
			break;
		case ETH_RSTP_INFO_AGED:
			q->updt_info = true;
			q->selected_role = ETH_RSTP_ROLE_DESIGNATED;
			break;
		case ETH_RSTP_INFO_MINE:
			q->selected_role = ETH_RSTP_ROLE_DESIGNATED;
			if (!prio_same(&q->port_prio, &q->designated_prio) ||
			    !times_same(&q->port_times, &br->root_times)) {
				q->updt_info = true;
			}
			break;
		case ETH_RSTP_INFO_RECEIVED:
			if (q == root_port) {
				q->selected_role = ETH_RSTP_ROLE_ROOT;
				q->updt_info = false;
			} else if (!prio_better(&q->designated_prio, &q->port_prio)) {
				if (same_bridge_address(q->port_prio.designated_bridge_id,
							br->bridge_id) &&
				    (q->port_prio.designated_port_id & PORT_NUMBER_MASK) !=
					    (q->port_id & PORT_NUMBER_MASK)) {
					q->selected_role = ETH_RSTP_ROLE_BACKUP;
				} else {
					q->selected_role = ETH_RSTP_ROLE_ALTERNATE;
				}
				q->updt_info = false;
			} else {
				q->selected_role = ETH_RSTP_ROLE_DESIGNATED;
				q->updt_info = true;
			}
			break;
		}
	}
}

static void dec(uint16_t *timer)
{
	if (*timer != 0U) {
		(*timer)--;
	}
}

/* Port Timers, Figure 13-15 */
static bool pti_run(struct eth_rstp_port *p)
{
	if (!p->tick) {
		return false;
	}

	p->tick = false;
	dec(&p->hello_when);
	dec(&p->tc_while);
	dec(&p->fd_while);
	dec(&p->rcvd_info_while);
	dec(&p->rr_while);
	dec(&p->rb_while);
	dec(&p->mdelay_while);
	dec(&p->edge_delay_while);
	dec(&p->tx_count);

	return true;
}

/* Port Receive, Figure 13-16 */
static void prx_enter(struct eth_rstp_port *p, enum prx_state state)
{
	p->prx_state = state;

	switch (state) {
	case PRX_DISCARD:
		p->rcvd_bpdu = false;
		p->rcvd_rstp = false;
		p->rcvd_stp = false;
		p->rcvd_msg = false;
		p->edge_delay_while = ETH_RSTP_MIGRATE_TIME;
		break;
	case PRX_RECEIVE:
		/* updtBPDUVersion() */
		if (p->rcvd.type == ETH_RSTP_BPDU_RST) {
			p->rcvd_rstp = true;
		} else {
			p->rcvd_stp = true;
		}
		rcv_msgs(p);
		p->oper_edge = false;
		p->rcvd_bpdu = false;
		p->edge_delay_while = ETH_RSTP_MIGRATE_TIME;
		break;
	}
}

static bool prx_run(struct eth_rstp_port *p)
{
	if ((p->rcvd_bpdu || p->edge_delay_while != ETH_RSTP_MIGRATE_TIME) && !p->port_enabled) {
		prx_enter(p, PRX_DISCARD);
		return true;
	}

	if (p->rcvd_bpdu && p->port_enabled && (p->prx_state == PRX_DISCARD || !p->rcvd_msg)) {
		prx_enter(p, PRX_RECEIVE);
		return true;
	}

	return false;
}

/* Port Protocol Migration, Figure 13-17 */
static void ppm_enter(struct eth_rstp_port *p, enum ppm_state state)
{
	p->ppm_state = state;

	switch (state) {
	case PPM_CHECKING_RSTP:
		p->mcheck = false;
		p->send_rstp = rstp_version(p->bridge);
		p->mdelay_while = ETH_RSTP_MIGRATE_TIME;
		break;
	case PPM_SELECTING_STP:
		p->send_rstp = false;
		p->mdelay_while = ETH_RSTP_MIGRATE_TIME;
		break;
	case PPM_SENSING:
		p->rcvd_rstp = false;
		p->rcvd_stp = false;
		break;
	}
}

static bool ppm_run(struct eth_rstp_port *p)
{
	enum ppm_state next;

	switch (p->ppm_state) {
	case PPM_CHECKING_RSTP:
		if (p->mdelay_while != ETH_RSTP_MIGRATE_TIME && !p->port_enabled) {
			next = PPM_CHECKING_RSTP;
		} else if (p->mdelay_while == 0U) {
			next = PPM_SENSING;
		} else {
			return false;
		}
		break;
	case PPM_SELECTING_STP:
		if (p->mdelay_while == 0U || !p->port_enabled || p->mcheck) {
			next = PPM_SENSING;
		} else {
			return false;
		}
		break;
	case PPM_SENSING:
		if (!p->port_enabled || p->mcheck ||
		    (rstp_version(p->bridge) && !p->send_rstp && p->rcvd_rstp)) {
			next = PPM_CHECKING_RSTP;
		} else if (p->send_rstp && p->rcvd_stp) {
			next = PPM_SELECTING_STP;
		} else {
			return false;
		}
		break;
	default:
		return false;
	}

	ppm_enter(p, next);
	return true;
}

/* Bridge Detection, Figure 13-18 */
static void bdm_enter(struct eth_rstp_port *p, enum bdm_state state)
{
	p->bdm_state = state;
	p->oper_edge = (state == BDM_EDGE);
}

static bool bdm_run(struct eth_rstp_port *p)
{
	switch (p->bdm_state) {
	case BDM_NOT_EDGE:
		if ((!p->port_enabled && p->admin_edge) ||
		    (p->edge_delay_while == 0U && p->auto_edge && p->send_rstp && p->proposing)) {
			bdm_enter(p, BDM_EDGE);
			return true;
		}
		break;
	case BDM_EDGE:
		if (((!p->port_enabled || !p->auto_edge) && !p->admin_edge) || !p->oper_edge) {
			bdm_enter(p, BDM_NOT_EDGE);
			return true;
		}
		break;
	}

	return false;
}

/* Port Transmit, Figure 13-19 */
static void ptx_enter(struct eth_rstp_port *p, enum ptx_state state)
{
	p->ptx_state = state;

	switch (state) {
	case PTX_TRANSMIT_INIT:
		p->new_info = true;
		p->tx_count = 0U;
		break;
	case PTX_TRANSMIT_PERIODIC:
		p->new_info = p->new_info || p->role == ETH_RSTP_ROLE_DESIGNATED ||
			      (p->role == ETH_RSTP_ROLE_ROOT && p->tc_while != 0U);
		break;
	case PTX_TRANSMIT_CONFIG:
		p->new_info = false;
		tx_config(p);
		p->tx_count++;
		p->tc_ack = false;
		break;
	case PTX_TRANSMIT_TCN:
		p->new_info = false;
		tx_tcn(p);
		p->tx_count++;
		break;
	case PTX_TRANSMIT_RSTP:
		p->new_info = false;
		tx_rstp(p);
		p->tx_count++;
		p->tc_ack = false;
		break;
	case PTX_IDLE:
		p->hello_when = hello_time(p);
		break;
	}
}

static bool ptx_run(struct eth_rstp_port *p)
{
	bool can_tx;

	if (!p->port_enabled) {
		if (p->ptx_state == PTX_TRANSMIT_INIT) {
			return false;
		}

		ptx_enter(p, PTX_TRANSMIT_INIT);
		return true;
	}

	if (p->ptx_state != PTX_IDLE) {
		/* UCT. TRANSMIT_INIT is left only once the port is enabled. */
		ptx_enter(p, PTX_IDLE);
		return true;
	}

	/* allTransmitReady */
	if (!p->selected || p->updt_info) {
		return false;
	}

	can_tx = p->new_info && p->tx_count < p->bridge->tx_hold_count && p->hello_when != 0U;

	if (p->hello_when == 0U) {
		ptx_enter(p, PTX_TRANSMIT_PERIODIC);
	} else if (can_tx && p->send_rstp) {
		ptx_enter(p, PTX_TRANSMIT_RSTP);
	} else if (can_tx && p->role == ETH_RSTP_ROLE_ROOT) {
		ptx_enter(p, PTX_TRANSMIT_TCN);
	} else if (can_tx && p->role == ETH_RSTP_ROLE_DESIGNATED) {
		ptx_enter(p, PTX_TRANSMIT_CONFIG);
	} else {
		return false;
	}

	return true;
}

/* Port Information, Figure 13-20 */
static void pim_enter(struct eth_rstp_port *p, enum pim_state state)
{
	p->pim_state = state;

	switch (state) {
	case PIM_DISABLED:
		p->rcvd_msg = false;
		p->proposing = false;
		p->proposed = false;
		p->agree = false;
		p->agreed = false;
		p->rcvd_info_while = 0U;
		p->info_is = ETH_RSTP_INFO_DISABLED;
		p->reselect = true;
		p->selected = false;
		break;
	case PIM_AGED:
		p->info_is = ETH_RSTP_INFO_AGED;
		p->reselect = true;
		p->selected = false;
		break;
	case PIM_UPDATE:
		p->proposing = false;
		p->proposed = false;
		p->agreed = p->agreed && betterorsame_info(p, ETH_RSTP_INFO_MINE);
		p->synced = p->synced && p->agreed;
		p->port_prio = p->designated_prio;
		p->port_times = p->designated_times;
		p->updt_info = false;
		p->info_is = ETH_RSTP_INFO_MINE;
		p->new_info = true;
		break;
	case PIM_CURRENT:
		break;
	case PIM_RECEIVE:
		p->rcvd_info = rcv_info(p);
		break;
	case PIM_SUPERIOR_DESIGNATED:
		p->agreed = false;
		p->proposing = false;
		record_proposal(p);
		set_tc_flags(p);
		p->agree = p->agree && betterorsame_info(p, ETH_RSTP_INFO_RECEIVED);
		record_agreement(p);
		p->synced = p->synced && p->agreed;
		p->port_prio = p->msg_prio;
		record_times(p);
		updt_rcvd_info_while(p);
		p->info_is = ETH_RSTP_INFO_RECEIVED;
		p->reselect = true;
		p->selected = false;
		p->rcvd_msg = false;
		break;
	case PIM_REPEATED_DESIGNATED:
		record_proposal(p);
		set_tc_flags(p);
		record_agreement(p);
		updt_rcvd_info_while(p);
		p->rcvd_msg = false;
		break;
	case PIM_INFERIOR_DESIGNATED:
		record_dispute(p);
		p->rcvd_msg = false;
		break;
	case PIM_NOT_DESIGNATED:
		record_agreement(p);
		set_tc_flags(p);
		p->rcvd_msg = false;
		break;
	case PIM_OTHER:
		p->rcvd_msg = false;
		break;
	}
}

static bool pim_run(struct eth_rstp_port *p)
{
	enum pim_state next;

	if (!p->port_enabled && p->info_is != ETH_RSTP_INFO_DISABLED) {
		pim_enter(p, PIM_DISABLED);
		return true;
	}

	switch (p->pim_state) {
	case PIM_DISABLED:
		if (p->rcvd_msg) {
			next = PIM_DISABLED;
		} else if (p->port_enabled) {
			next = PIM_AGED;
		} else {
			return false;
		}
		break;
	case PIM_AGED:
		if (p->selected && p->updt_info) {
			next = PIM_UPDATE;
		} else {
			return false;
		}
		break;
	case PIM_UPDATE:
	case PIM_SUPERIOR_DESIGNATED:
	case PIM_REPEATED_DESIGNATED:
	case PIM_INFERIOR_DESIGNATED:
	case PIM_NOT_DESIGNATED:
	case PIM_OTHER:
		next = PIM_CURRENT;
		break;
	case PIM_CURRENT:
		if (p->selected && p->updt_info) {
			next = PIM_UPDATE;
		} else if (p->info_is == ETH_RSTP_INFO_RECEIVED && p->rcvd_info_while == 0U &&
			   !p->updt_info && !p->rcvd_msg) {
			next = PIM_AGED;
		} else if (p->rcvd_msg && !p->updt_info) {
			next = PIM_RECEIVE;
		} else {
			return false;
		}
		break;
	case PIM_RECEIVE:
		switch (p->rcvd_info) {
		case ETH_RSTP_RCVD_SUPERIOR_DESIGNATED:
			next = PIM_SUPERIOR_DESIGNATED;
			break;
		case ETH_RSTP_RCVD_REPEATED_DESIGNATED:
			next = PIM_REPEATED_DESIGNATED;
			break;
		case ETH_RSTP_RCVD_INFERIOR_DESIGNATED:
			next = PIM_INFERIOR_DESIGNATED;
			break;
		case ETH_RSTP_RCVD_INFERIOR_ROOT_ALTERNATE:
			next = PIM_NOT_DESIGNATED;
			break;
		default:
			next = PIM_OTHER;
			break;
		}
		break;
	default:
		return false;
	}

	pim_enter(p, next);
	return true;
}

/* Port Role Selection, Figure 13-21 */
static void prs_enter(struct eth_rstp_bridge *br, enum prs_state state)
{
	br->prs_state = state;

	switch (state) {
	case PRS_INIT_TREE:
		/* updtRolesDisabledTree() */
		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				br->ports[i]->selected_role = ETH_RSTP_ROLE_DISABLED;
			}
		}
		break;
	case PRS_ROLE_SELECTION:
		/* clearReselectTree() */
		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				br->ports[i]->reselect = false;
			}
		}

		updt_roles_tree(br);

		/* setSelectedTree(): reselect was just cleared on every port */
		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				br->ports[i]->selected = true;
			}
		}
		break;
	}
}

static bool prs_run(struct eth_rstp_bridge *br)
{
	if (br->prs_state == PRS_INIT_TREE) {
		prs_enter(br, PRS_ROLE_SELECTION);
		return true;
	}

	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL && br->ports[i]->reselect) {
			prs_enter(br, PRS_ROLE_SELECTION);
			return true;
		}
	}

	return false;
}

/* Port Role Transitions, Figures 13-22 and 13-24 to 13-26 */
static void prt_enter(struct eth_rstp_port *p, enum prt_state state)
{
	p->prt_state = state;

	switch (state) {
	case PRT_INIT_PORT:
		p->role = ETH_RSTP_ROLE_DISABLED;
		p->learn = false;
		p->forward = false;
		p->synced = false;
		p->sync = true;
		p->re_root = true;
		p->rr_while = fwd_delay(p);
		p->fd_while = max_age(p);
		p->rb_while = 0U;
		break;
	case PRT_DISABLE_PORT:
		p->role = ETH_RSTP_ROLE_DISABLED;
		p->learn = false;
		p->forward = false;
		break;
	case PRT_DISABLED_PORT:
		p->fd_while = max_age(p);
		p->synced = true;
		p->rr_while = 0U;
		p->sync = false;
		p->re_root = false;
		break;
	case PRT_ROOT_PROPOSED:
		set_sync_tree(p->bridge);
		p->proposed = false;
		break;
	case PRT_ROOT_AGREED:
		p->proposed = false;
		p->sync = false;
		p->agree = true;
		p->new_info = true;
		break;
	case PRT_ROOT_SYNCED:
		p->synced = true;
		p->sync = false;
		break;
	case PRT_REROOT:
		set_re_root_tree(p->bridge);
		break;
	case PRT_ROOT_FORWARD:
		p->fd_while = 0U;
		p->forward = true;
		break;
	case PRT_ROOT_LEARN:
		p->fd_while = forward_delay(p);
		p->learn = true;
		break;
	case PRT_REROOTED:
		p->re_root = false;
		break;
	case PRT_ROOT_DISCARD:
		if (p->disputed) {
			p->rb_while = 3U * hello_time(p);
		}
		p->learn = false;
		p->forward = false;
		p->disputed = false;
		p->fd_while = fwd_delay(p);
		break;
	case PRT_ROOT_PORT:
		p->role = ETH_RSTP_ROLE_ROOT;
		p->rr_while = fwd_delay(p);
		break;
	case PRT_DESIGNATED_PROPOSE:
		p->proposing = true;
		p->edge_delay_while = edge_delay(p);
		p->new_info = true;
		break;
	case PRT_DESIGNATED_AGREE:
		p->proposed = false;
		p->sync = false;
		p->agree = true;
		p->new_info = true;
		break;
	case PRT_DESIGNATED_SYNCED:
		p->rr_while = 0U;
		p->synced = true;
		p->sync = false;
		break;
	case PRT_DESIGNATED_RETIRED:
		p->re_root = false;
		break;
	case PRT_DESIGNATED_FORWARD:
		p->forward = true;
		p->fd_while = 0U;
		p->agreed = p->send_rstp;
		break;
	case PRT_DESIGNATED_LEARN:
		p->learn = true;
		p->fd_while = forward_delay(p);
		break;
	case PRT_DESIGNATED_DISCARD:
		p->learn = false;
		p->forward = false;
		p->disputed = false;
		p->fd_while = forward_delay(p);
		break;
	case PRT_DESIGNATED_PORT:
		p->role = ETH_RSTP_ROLE_DESIGNATED;
		break;
	case PRT_ALTERNATE_PROPOSED:
		set_sync_tree(p->bridge);
		p->proposed = false;
		break;
	case PRT_ALTERNATE_AGREED:
		p->proposed = false;
		p->agree = true;
		p->new_info = true;
		break;
	case PRT_BLOCK_PORT:
		p->role = p->selected_role;
		p->learn = false;
		p->forward = false;
		break;
	case PRT_BACKUP_PORT:
		p->rb_while = 2U * hello_time(p);
		break;
	case PRT_ALTERNATE_PORT:
		p->fd_while = forward_delay(p);
		p->synced = true;
		p->rr_while = 0U;
		p->sync = false;
		p->re_root = false;
		break;
	}
}

static bool prt_root_port(struct eth_rstp_port *p, enum prt_state *next)
{
	bool can_learn =
		p->fd_while == 0U || (re_rooted(p) && p->rb_while == 0U && rstp_version(p->bridge));

	if (p->proposed && !p->agree) {
		*next = PRT_ROOT_PROPOSED;
	} else if ((all_synced(p) && !p->agree) || (p->proposed && p->agree)) {
		*next = PRT_ROOT_AGREED;
	} else if ((p->agreed && !p->synced) || (p->sync && p->synced)) {
		*next = PRT_ROOT_SYNCED;
	} else if (!p->forward && p->rb_while == 0U && !p->re_root) {
		*next = PRT_REROOT;
	} else if (p->disputed) {
		*next = PRT_ROOT_DISCARD;
	} else if (p->re_root && p->forward) {
		*next = PRT_REROOTED;
	} else if (can_learn && !p->learn) {
		*next = PRT_ROOT_LEARN;
	} else if (can_learn && p->learn && !p->forward) {
		*next = PRT_ROOT_FORWARD;
	} else if (p->rr_while != fwd_delay(p)) {
		*next = PRT_ROOT_PORT;
	} else {
		return false;
	}

	return true;
}

static bool prt_designated_port(struct eth_rstp_port *p, enum prt_state *next)
{
	bool can_learn = (p->fd_while == 0U || p->agreed || p->oper_edge) &&
			 (p->rr_while == 0U || !p->re_root) && !p->sync;

	if (!p->forward && !p->agreed && !p->proposing && !p->oper_edge) {
		*next = PRT_DESIGNATED_PROPOSE;
	} else if (all_synced(p) && (p->proposed || !p->agree)) {
		*next = PRT_DESIGNATED_AGREE;
	} else if ((!p->learning && !p->forwarding && !p->synced) || (p->agreed && !p->synced) ||
		   (p->oper_edge && !p->synced) || (p->sync && p->synced)) {
		*next = PRT_DESIGNATED_SYNCED;
	} else if (p->re_root && p->rr_while == 0U) {
		*next = PRT_DESIGNATED_RETIRED;
	} else if (((p->sync && !p->synced) || (p->re_root && p->rr_while != 0U) || p->disputed) &&
		   !p->oper_edge && (p->learn || p->forward)) {
		*next = PRT_DESIGNATED_DISCARD;
	} else if (can_learn && !p->learn) {
		*next = PRT_DESIGNATED_LEARN;
	} else if (can_learn && p->learn && !p->forward) {
		*next = PRT_DESIGNATED_FORWARD;
	} else {
		return false;
	}

	return true;
}

static bool prt_alternate_port(struct eth_rstp_port *p, enum prt_state *next)
{
	if (p->proposed && !p->agree) {
		*next = PRT_ALTERNATE_PROPOSED;
	} else if ((all_synced(p) && !p->agree) || (p->proposed && p->agree)) {
		*next = PRT_ALTERNATE_AGREED;
	} else if (p->rb_while != 2U * hello_time(p) && p->role == ETH_RSTP_ROLE_BACKUP) {
		*next = PRT_BACKUP_PORT;
	} else if (p->fd_while != forward_delay(p) || p->sync || p->re_root || !p->synced) {
		*next = PRT_ALTERNATE_PORT;
	} else {
		return false;
	}

	return true;
}

static bool prt_run(struct eth_rstp_port *p)
{
	/* All transitions except UCT are qualified by "&& selected && !updtInfo" */
	bool qualified = p->selected && !p->updt_info;
	enum prt_state next;

	if (qualified && p->selected_role != p->role) {
		switch (p->selected_role) {
		case ETH_RSTP_ROLE_DISABLED:
			next = PRT_DISABLE_PORT;
			break;
		case ETH_RSTP_ROLE_ROOT:
			next = PRT_ROOT_PORT;
			break;
		case ETH_RSTP_ROLE_DESIGNATED:
			next = PRT_DESIGNATED_PORT;
			break;
		default:
			next = PRT_BLOCK_PORT;
			break;
		}

		prt_enter(p, next);
		return true;
	}

	switch (p->prt_state) {
	case PRT_INIT_PORT:
		next = PRT_DISABLE_PORT;
		break;
	case PRT_DISABLE_PORT:
		if (!qualified || p->learning || p->forwarding) {
			return false;
		}
		next = PRT_DISABLED_PORT;
		break;
	case PRT_DISABLED_PORT:
		if (!qualified ||
		    !(p->fd_while != max_age(p) || p->sync || p->re_root || !p->synced)) {
			return false;
		}
		next = PRT_DISABLED_PORT;
		break;
	case PRT_ROOT_PROPOSED:
	case PRT_ROOT_AGREED:
	case PRT_ROOT_SYNCED:
	case PRT_REROOT:
	case PRT_ROOT_FORWARD:
	case PRT_ROOT_LEARN:
	case PRT_REROOTED:
	case PRT_ROOT_DISCARD:
		next = PRT_ROOT_PORT;
		break;
	case PRT_ROOT_PORT:
		if (!qualified || !prt_root_port(p, &next)) {
			return false;
		}
		break;
	case PRT_DESIGNATED_PROPOSE:
	case PRT_DESIGNATED_AGREE:
	case PRT_DESIGNATED_SYNCED:
	case PRT_DESIGNATED_RETIRED:
	case PRT_DESIGNATED_FORWARD:
	case PRT_DESIGNATED_LEARN:
	case PRT_DESIGNATED_DISCARD:
		next = PRT_DESIGNATED_PORT;
		break;
	case PRT_DESIGNATED_PORT:
		if (!qualified || !prt_designated_port(p, &next)) {
			return false;
		}
		break;
	case PRT_ALTERNATE_PROPOSED:
	case PRT_ALTERNATE_AGREED:
	case PRT_BACKUP_PORT:
		next = PRT_ALTERNATE_PORT;
		break;
	case PRT_BLOCK_PORT:
		if (!qualified || p->learning || p->forwarding) {
			return false;
		}
		next = PRT_ALTERNATE_PORT;
		break;
	case PRT_ALTERNATE_PORT:
		if (!qualified || !prt_alternate_port(p, &next)) {
			return false;
		}
		break;
	default:
		return false;
	}

	prt_enter(p, next);
	return true;
}

/* Port State Transition, Figure 13-27 */
static void pst_enter(struct eth_rstp_port *p, enum pst_state state)
{
	p->pst_state = state;
	p->learning = (state != PST_DISCARDING);
	p->forwarding = (state == PST_FORWARDING);
	p->bridge->ops->set_port_state(p, p->learning, p->forwarding);
}

static bool pst_run(struct eth_rstp_port *p)
{
	switch (p->pst_state) {
	case PST_DISCARDING:
		if (p->learn) {
			pst_enter(p, PST_LEARNING);
			return true;
		}
		break;
	case PST_LEARNING:
		if (p->forward) {
			pst_enter(p, PST_FORWARDING);
			return true;
		}

		if (!p->learn) {
			pst_enter(p, PST_DISCARDING);
			return true;
		}
		break;
	case PST_FORWARDING:
		if (!p->forward) {
			pst_enter(p, PST_DISCARDING);
			return true;
		}
		break;
	}

	return false;
}

/* Topology Change, Figure 13-28 */
static void tcm_enter(struct eth_rstp_port *p, enum tcm_state state)
{
	p->tcm_state = state;

	switch (state) {
	case TCM_INACTIVE:
		p->fdb_flush = true;
		p->tc_while = 0U;
		p->tc_ack = false;
		break;
	case TCM_LEARNING:
		p->rcvd_tc = false;
		p->rcvd_tcn = false;
		p->rcvd_tc_ack = false;
		p->tc_prop = false;
		break;
	case TCM_DETECTED:
		new_tc_while(p);
		set_tc_prop_tree(p);
		p->new_info = true;
		break;
	case TCM_ACTIVE:
		break;
	case TCM_NOTIFIED_TCN:
		new_tc_while(p);
		break;
	case TCM_NOTIFIED_TC:
		p->rcvd_tcn = false;
		p->rcvd_tc = false;
		if (p->role == ETH_RSTP_ROLE_DESIGNATED) {
			p->tc_ack = true;
		}
		set_tc_prop_tree(p);
		break;
	case TCM_PROPAGATING:
		new_tc_while(p);
		p->fdb_flush = true;
		p->tc_prop = false;
		break;
	case TCM_ACKNOWLEDGED:
		p->tc_while = 0U;
		p->rcvd_tc_ack = false;
		break;
	}
}

static bool tcm_run(struct eth_rstp_port *p)
{
	bool root_or_designated =
		p->role == ETH_RSTP_ROLE_ROOT || p->role == ETH_RSTP_ROLE_DESIGNATED;
	bool rcvd_any = p->rcvd_tc || p->rcvd_tcn || p->rcvd_tc_ack || p->tc_prop;
	enum tcm_state next;

	switch (p->tcm_state) {
	case TCM_INACTIVE:
		if (!p->learn || p->fdb_flush) {
			return false;
		}
		next = TCM_LEARNING;
		break;
	case TCM_LEARNING:
		if (root_or_designated && p->forward && !p->oper_edge) {
			next = TCM_DETECTED;
		} else if (rcvd_any) {
			next = TCM_LEARNING;
		} else if (!root_or_designated && !(p->learn || p->learning)) {
			next = TCM_INACTIVE;
		} else {
			return false;
		}
		break;
	case TCM_DETECTED:
	case TCM_NOTIFIED_TC:
	case TCM_PROPAGATING:
	case TCM_ACKNOWLEDGED:
		next = TCM_ACTIVE;
		break;
	case TCM_NOTIFIED_TCN:
		next = TCM_NOTIFIED_TC;
		break;
	case TCM_ACTIVE:
		if (!root_or_designated || p->oper_edge) {
			next = TCM_LEARNING;
		} else if (p->rcvd_tcn) {
			next = TCM_NOTIFIED_TCN;
		} else if (p->rcvd_tc) {
			next = TCM_NOTIFIED_TC;
		} else if (p->tc_prop && !p->oper_edge) {
			next = TCM_PROPAGATING;
		} else if (p->rcvd_tc_ack) {
			next = TCM_ACKNOWLEDGED;
		} else {
			return false;
		}
		break;
	default:
		return false;
	}

	tcm_enter(p, next);
	return true;
}

/* The FDB side of fdbFlush (13.27.28). With STP compatibility the standard
 * ages the entries out quickly instead, flushing them at once is the
 * stricter choice and lets the stations be learned again.
 */
static bool fdb_run(struct eth_rstp_port *p)
{
	if (!p->fdb_flush) {
		return false;
	}

	if (!p->oper_edge) {
		p->bridge->ops->flush_fdb(p);
	}

	p->fdb_flush = false;
	return true;
}

static bool run_port_input(struct eth_rstp_port *p)
{
	bool changed = false;

	changed = pti_run(p) || changed;
	changed = prx_run(p) || changed;
	changed = ppm_run(p) || changed;
	changed = bdm_run(p) || changed;
	changed = pim_run(p) || changed;

	return changed;
}

static bool run_port_output(struct eth_rstp_port *p)
{
	bool changed = false;

	changed = prt_run(p) || changed;
	changed = pst_run(p) || changed;
	changed = tcm_run(p) || changed;
	changed = fdb_run(p) || changed;

	return changed;
}

static void run(struct eth_rstp_bridge *br)
{
	if (!br->started) {
		return;
	}

	for (uint32_t pass = 0U; pass < MAX_PASSES; pass++) {
		bool changed = false;

		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				changed = run_port_input(br->ports[i]) || changed;
			}
		}

		changed = prs_run(br) || changed;

		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				changed = run_port_output(br->ports[i]) || changed;
			}
		}

		if (changed) {
			continue;
		}

		for (size_t i = 0; i < br->num_ports; i++) {
			if (br->ports[i] != NULL) {
				changed = ptx_run(br->ports[i]) || changed;
			}
		}

		if (!changed) {
			return;
		}
	}

	br->unsettled++;
}

static void port_begin(struct eth_rstp_port *p)
{
	const size_t start = offsetof(struct eth_rstp_port, edge_delay_while);

	memset((uint8_t *)p + start, 0, sizeof(*p) - start);

	p->designated_times = p->bridge->bridge_times;
	p->port_times = p->bridge->bridge_times;

	prx_enter(p, PRX_DISCARD);
	ppm_enter(p, PPM_CHECKING_RSTP);
	bdm_enter(p, p->admin_edge ? BDM_EDGE : BDM_NOT_EDGE);
	ptx_enter(p, PTX_TRANSMIT_INIT);
	pim_enter(p, PIM_DISABLED);
	prt_enter(p, PRT_INIT_PORT);
	pst_enter(p, PST_DISCARDING);
	tcm_enter(p, TCM_INACTIVE);
}

void eth_rstp_bridge_init(struct eth_rstp_bridge *br, const struct eth_rstp_ops *ops,
			  struct eth_rstp_port **ports, size_t num_ports, const uint8_t mac[6])
{
	uint64_t address = 0U;

	memset(br, 0, sizeof(*br));

	for (size_t i = 0; i < 6U; i++) {
		address = (address << 8) | mac[i];
	}

	br->ops = ops;
	br->ports = ports;
	br->num_ports = num_ports;
	br->bridge_id = ((uint64_t)ETH_RSTP_DEFAULT_BRIDGE_PRIORITY << 48) | address;
	br->bridge_times = (struct eth_rstp_times){
		.max_age = ETH_RSTP_DEFAULT_MAX_AGE,
		.hello_time = ETH_RSTP_HELLO_TIME,
		.forward_delay = ETH_RSTP_DEFAULT_FORWARD_DELAY,
	};
	br->force_version = 2U;
	br->tx_hold_count = ETH_RSTP_DEFAULT_TX_HOLD_COUNT;
	br->root_times = br->bridge_times;
}

void eth_rstp_port_init(struct eth_rstp_port *port, struct eth_rstp_bridge *br,
			uint16_t port_number)
{
	memset(port, 0, sizeof(*port));

	port->bridge = br;
	port->port_id = (uint16_t)((ETH_RSTP_DEFAULT_PORT_PRIORITY << 8) |
				   (port_number & PORT_NUMBER_MASK));
	port->port_path_cost = ETH_RSTP_DEFAULT_PATH_COST;
	/* Recommended defaults (13.27.1, 13.27.18) */
	port->admin_edge = false;
	port->auto_edge = true;
	port->designated_times = br->bridge_times;
	port->port_times = br->bridge_times;
}

void eth_rstp_start(struct eth_rstp_bridge *br)
{
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			port_begin(br->ports[i]);
		}
	}

	prs_enter(br, PRS_INIT_TREE);
	br->started = true;
	run(br);
}

void eth_rstp_port_begin(struct eth_rstp_port *port)
{
	port_begin(port);
	run(port->bridge);
}

void eth_rstp_tick(struct eth_rstp_bridge *br)
{
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			br->ports[i]->tick = true;
		}
	}

	run(br);
}

void eth_rstp_rx(struct eth_rstp_port *port, const struct eth_rstp_bpdu *bpdu)
{
	if (!port->port_enabled) {
		return;
	}

	port->rcvd = *bpdu;
	port->rcvd_bpdu = true;
	run(port->bridge);
}

/* A management change of a priority vector component or of the bridge
 * times makes the Port Role Selection recompute (13.26).
 */
static void reselect_all(struct eth_rstp_bridge *br)
{
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			br->ports[i]->reselect = true;
			br->ports[i]->selected = false;
		}
	}

	run(br);
}

void eth_rstp_port_set_enabled(struct eth_rstp_port *port, bool enabled)
{
	port->port_enabled = enabled;
	run(port->bridge);
}

void eth_rstp_port_set_p2p(struct eth_rstp_port *port, bool p2p)
{
	port->oper_p2p = p2p;
	run(port->bridge);
}

void eth_rstp_port_set_edge(struct eth_rstp_port *port, bool admin_edge, bool auto_edge)
{
	port->admin_edge = admin_edge;
	port->auto_edge = auto_edge;
	run(port->bridge);
}

int eth_rstp_port_set_path_cost(struct eth_rstp_port *port, uint32_t cost)
{
	if (cost == 0U || cost > ETH_RSTP_MAX_PATH_COST) {
		return -EINVAL;
	}

	port->port_path_cost = cost;
	reselect_all(port->bridge);

	return 0;
}

int eth_rstp_port_set_priority(struct eth_rstp_port *port, uint8_t priority)
{
	if ((priority & 0x0fU) != 0U) {
		return -EINVAL;
	}

	port->port_id = (uint16_t)(((uint16_t)priority << 8) | (port->port_id & PORT_NUMBER_MASK));
	reselect_all(port->bridge);

	return 0;
}

void eth_rstp_port_mcheck(struct eth_rstp_port *port)
{
	/* Setting mcheck has no effect if stpVersion is TRUE (13.27.38) */
	if (!rstp_version(port->bridge)) {
		return;
	}

	port->mcheck = true;
	run(port->bridge);
}

int eth_rstp_set_bridge_priority(struct eth_rstp_bridge *br, uint16_t priority)
{
	if ((priority & 0x0fffU) != 0U) {
		return -EINVAL;
	}

	br->bridge_id = ((uint64_t)priority << 48) | (br->bridge_id & BRIDGE_ADDRESS_MASK);
	reselect_all(br);

	return 0;
}

int eth_rstp_set_times(struct eth_rstp_bridge *br, uint16_t max_age, uint16_t forward_delay)
{
	/* Table 13-5 and the relationships of 13.25 */
	if (max_age < 6U || max_age > 40U || forward_delay < 4U || forward_delay > 30U ||
	    2U * (forward_delay - 1U) < max_age || max_age < 2U * (ETH_RSTP_HELLO_TIME + 1U)) {
		return -EINVAL;
	}

	br->bridge_times.max_age = max_age;
	br->bridge_times.forward_delay = forward_delay;
	reselect_all(br);

	return 0;
}

int eth_rstp_set_tx_hold_count(struct eth_rstp_bridge *br, uint8_t count)
{
	if (count < 1U || count > 10U) {
		return -EINVAL;
	}

	br->tx_hold_count = count;

	/* 13.26.12 */
	for (size_t i = 0; i < br->num_ports; i++) {
		if (br->ports[i] != NULL) {
			br->ports[i]->tx_count = 0U;
		}
	}

	run(br);

	return 0;
}

int eth_rstp_set_force_version(struct eth_rstp_bridge *br, uint8_t version)
{
	if (version != 0U && version != 2U) {
		return -EINVAL;
	}

	br->force_version = version;

	/* BEGIN is asserted for all state machines (13.26) */
	if (br->started) {
		eth_rstp_start(br);
	}

	return 0;
}
