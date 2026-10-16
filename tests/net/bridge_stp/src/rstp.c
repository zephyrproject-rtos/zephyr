/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The RSTP state machines on a simulated network. Each bridge port is
 * attached to a LAN. A transmitted BPDU is encoded, queued and, when the
 * queue is pumped, decoded and received by every other port on the LAN.
 * Time only passes through sim_ticks().
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "rstp_internal.h"

#define NUM_BRIDGES   3
#define NUM_PORTS     3
#define NUM_LANS      8
#define LAN_MAX_PORTS 3
#define QUEUE_LEN     64
#define MAX_DELIVERED 10000

struct sim_port {
	struct eth_rstp_port rstp;
	int lan;
	uint32_t tx_count;
	uint32_t flush_count;
	struct eth_rstp_bpdu last_tx;
	uint8_t last_tx_raw[ETH_RSTP_BPDU_MAX_LEN];
	size_t last_tx_len;
};

struct sim_bridge {
	struct eth_rstp_bridge rstp;
	struct sim_port port[NUM_PORTS];
	struct eth_rstp_port *ports[NUM_PORTS];
};

struct sim_frame {
	struct sim_port *from;
	uint8_t data[ETH_RSTP_BPDU_MAX_LEN];
	size_t len;
};

static struct sim_bridge bridges[NUM_BRIDGES];
static struct sim_port *lans[NUM_LANS][LAN_MAX_PORTS];
static struct sim_frame queue[QUEUE_LEN];
static size_t queue_head;
static size_t queue_count;
static uint32_t queue_overflows;

#define A (&bridges[0])
#define B (&bridges[1])
#define C (&bridges[2])

static struct sim_port *port_of(struct eth_rstp_port *port)
{
	return CONTAINER_OF(port, struct sim_port, rstp);
}

static void sim_tx_bpdu(struct eth_rstp_port *port, const struct eth_rstp_bpdu *bpdu)
{
	struct sim_port *sp = port_of(port);
	struct sim_frame *frame;

	sp->tx_count++;
	sp->last_tx = *bpdu;
	sp->last_tx_len = eth_rstp_bpdu_encode(bpdu, sp->last_tx_raw, sizeof(sp->last_tx_raw));
	zassert_not_equal(sp->last_tx_len, 0U);

	if (queue_count == QUEUE_LEN) {
		queue_overflows++;
		return;
	}

	frame = &queue[(queue_head + queue_count) % QUEUE_LEN];
	frame->from = sp;
	memcpy(frame->data, sp->last_tx_raw, sp->last_tx_len);
	frame->len = sp->last_tx_len;
	queue_count++;
}

static void sim_set_port_state(struct eth_rstp_port *port, bool learning, bool forwarding)
{
	/* The state machines keep the learning and forwarding variables */
	ARG_UNUSED(port);
	ARG_UNUSED(learning);
	ARG_UNUSED(forwarding);
}

static void sim_flush_fdb(struct eth_rstp_port *port)
{
	port_of(port)->flush_count++;
}

static const struct eth_rstp_ops sim_ops = {
	.tx_bpdu = sim_tx_bpdu,
	.set_port_state = sim_set_port_state,
	.flush_fdb = sim_flush_fdb,
};

static void pump(void)
{
	for (uint32_t delivered = 0U; queue_count > 0U; delivered++) {
		struct sim_frame frame = queue[queue_head];
		struct eth_rstp_bpdu bpdu;

		zassert_true(delivered < MAX_DELIVERED, "BPDUs never settle");

		queue_head = (queue_head + 1U) % QUEUE_LEN;
		queue_count--;

		zassert_ok(eth_rstp_bpdu_decode(frame.data, frame.len, &bpdu));

		ARRAY_FOR_EACH(lans[frame.from->lan], i) {
			struct sim_port *to = lans[frame.from->lan][i];

			if (to != NULL && to != frame.from) {
				eth_rstp_rx(&to->rstp, &bpdu);
			}
		}
	}
}

static void sim_bridge_init(struct sim_bridge *sb, uint8_t id)
{
	const uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, id};

	eth_rstp_bridge_init(&sb->rstp, &sim_ops, sb->ports, NUM_PORTS, mac);

	ARRAY_FOR_EACH(sb->port, i) {
		eth_rstp_port_init(&sb->port[i].rstp, &sb->rstp, (uint16_t)(i + 1U));
		sb->port[i].lan = -1;
		sb->ports[i] = &sb->port[i].rstp;
	}
}

static void attach(struct sim_bridge *sb, size_t port, int lan)
{
	struct sim_port *sp = &sb->port[port];

	ARRAY_FOR_EACH(lans[lan], i) {
		if (lans[lan][i] == NULL) {
			lans[lan][i] = sp;
			sp->lan = lan;
			return;
		}
	}

	zassert_unreachable("LAN %d is full", lan);
}

/* Start all bridges and enable the attached ports. A LAN with more than two
 * ports is a shared LAN, the others are point-to-point links.
 */
static void sim_start(void)
{
	ARRAY_FOR_EACH(bridges, b) {
		eth_rstp_start(&bridges[b].rstp);
	}

	ARRAY_FOR_EACH(lans, lan) {
		size_t n = 0;

		ARRAY_FOR_EACH(lans[lan], i) {
			n += (lans[lan][i] != NULL) ? 1U : 0U;
		}

		ARRAY_FOR_EACH(lans[lan], i) {
			if (lans[lan][i] != NULL) {
				eth_rstp_port_set_p2p(&lans[lan][i]->rstp, n <= 2U);
				eth_rstp_port_set_enabled(&lans[lan][i]->rstp, true);
			}
		}
	}

	pump();
}

static void sim_ticks(uint32_t count)
{
	for (uint32_t t = 0U; t < count; t++) {
		ARRAY_FOR_EACH(bridges, b) {
			eth_rstp_tick(&bridges[b].rstp);
		}

		pump();
	}
}

static void lan_down(int lan)
{
	ARRAY_FOR_EACH(lans[lan], i) {
		if (lans[lan][i] != NULL) {
			eth_rstp_port_set_enabled(&lans[lan][i]->rstp, false);
		}
	}

	pump();
}

/* Receive a BPDU on a port from a bridge that is not simulated */
static void inject(struct sim_port *sp, const struct eth_rstp_bpdu *bpdu)
{
	eth_rstp_rx(&sp->rstp, bpdu);
	pump();
}

static struct eth_rstp_port *P(struct sim_bridge *sb, size_t port)
{
	return &sb->port[port].rstp;
}

static uint64_t bridge_id(struct sim_bridge *sb)
{
	return sb->rstp.bridge_id;
}

static bool is_forwarding(struct sim_bridge *sb, size_t port)
{
	return P(sb, port)->learning && P(sb, port)->forwarding;
}

static bool is_discarding(struct sim_bridge *sb, size_t port)
{
	return !P(sb, port)->learning && !P(sb, port)->forwarding;
}

static uint32_t flushes(struct sim_bridge *sb)
{
	uint32_t count = 0U;

	ARRAY_FOR_EACH(sb->port, i) {
		count += sb->port[i].flush_count;
	}

	return count;
}

static void expect_settled(void)
{
	ARRAY_FOR_EACH(bridges, b) {
		zexpect_equal(bridges[b].rstp.unsettled, 0U, "bridge %zu did not settle", b);
	}

	zexpect_equal(queue_overflows, 0U);
}

/* A designated BPDU from a bridge with the given identifier */
static struct eth_rstp_bpdu designated_bpdu(uint64_t root_id, uint64_t sender_id)
{
	return (struct eth_rstp_bpdu){
		.type = ETH_RSTP_BPDU_RST,
		.flags = ETH_RSTP_BPDU_ROLE_DESIGNATED << ETH_RSTP_FLAG_ROLE_SHIFT,
		.root_id = root_id,
		.root_path_cost = 0U,
		.bridge_id = sender_id,
		.port_id = 0x8001U,
		.times = {.message_age = 0U,
			  .max_age = ETH_RSTP_DEFAULT_MAX_AGE,
			  .hello_time = ETH_RSTP_HELLO_TIME,
			  .forward_delay = ETH_RSTP_DEFAULT_FORWARD_DELAY},
	};
}

/* A bridge identifier better than any simulated bridge */
#define BETTER_ID 0x1000020000000099ULL
/* A bridge identifier worse than any simulated bridge */
#define WORSE_ID  0xf000020000000099ULL

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(bridges, 0, sizeof(bridges));
	memset(lans, 0, sizeof(lans));
	queue_head = 0U;
	queue_count = 0U;
	queue_overflows = 0U;

	ARRAY_FOR_EACH(bridges, b) {
		sim_bridge_init(&bridges[b], (uint8_t)(b + 1U));
	}
}

/* Two bridges on one link: proposal and agreement make both ports forward
 * without waiting for any timer (13.16).
 */
ZTEST(rstp, test_two_bridges)
{
	attach(A, 0, 0);
	attach(B, 0, 0);
	sim_start();

	zexpect_equal(A->rstp.root_port_id, 0U, "A is not the root");
	zexpect_equal(B->rstp.root_prio.root_id, bridge_id(A));
	zexpect_equal(B->rstp.root_prio.root_path_cost, ETH_RSTP_DEFAULT_PATH_COST);
	zexpect_equal(B->rstp.root_port_id, P(B, 0)->port_id);

	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_DESIGNATED);
	zexpect_equal(P(B, 0)->role, ETH_RSTP_ROLE_ROOT);
	zexpect_true(is_forwarding(A, 0));
	zexpect_true(is_forwarding(B, 0));

	/* The last BPDU of the root port carries the agreement */
	zexpect_equal(B->port[0].last_tx.type, ETH_RSTP_BPDU_RST);
	zexpect_equal(B->port[0].last_tx_raw[2], 2U, "Protocol Version Identifier");
	zexpect_equal((B->port[0].last_tx.flags & ETH_RSTP_FLAG_ROLE_MASK) >>
			      ETH_RSTP_FLAG_ROLE_SHIFT,
		      ETH_RSTP_BPDU_ROLE_ROOT);
	zexpect_true((B->port[0].last_tx.flags & ETH_RSTP_FLAG_AGREEMENT) != 0U);

	expect_settled();
}

/* Three bridges in a ring: exactly one port blocks the loop */
ZTEST(rstp, test_triangle)
{
	int discarding = 0;

	attach(A, 0, 0);
	attach(B, 0, 0);
	attach(A, 1, 1);
	attach(C, 0, 1);
	attach(B, 1, 2);
	attach(C, 1, 2);
	sim_start();

	zexpect_equal(B->rstp.root_port_id, P(B, 0)->port_id);
	zexpect_equal(C->rstp.root_port_id, P(C, 0)->port_id);

	/* On the B-C link B has the better bridge identifier */
	zexpect_equal(P(B, 1)->role, ETH_RSTP_ROLE_DESIGNATED);
	zexpect_equal(P(C, 1)->role, ETH_RSTP_ROLE_ALTERNATE);

	ARRAY_FOR_EACH(bridges, b) {
		for (size_t i = 0; i < 2U; i++) {
			if (is_discarding(&bridges[b], i)) {
				discarding++;
			} else {
				zexpect_true(is_forwarding(&bridges[b], i), "bridge %zu port %zu",
					     b, i);
			}
		}
	}

	zexpect_equal(discarding, 1);
	zexpect_true(is_discarding(C, 1));

	/* Nothing changes once the protocol has settled */
	sim_ticks(10U);
	zexpect_equal(P(C, 1)->role, ETH_RSTP_ROLE_ALTERNATE);
	zexpect_true(is_discarding(C, 1));
	zexpect_true(is_forwarding(B, 1));

	expect_settled();
}

/* Two links between the same bridges: the one from the better Designated
 * Port is the Root Port, the other one an Alternate Port.
 */
ZTEST(rstp, test_parallel_links)
{
	attach(A, 0, 0);
	attach(B, 0, 0);
	attach(A, 1, 1);
	attach(B, 1, 1);
	sim_start();

	zexpect_equal(P(B, 0)->role, ETH_RSTP_ROLE_ROOT);
	zexpect_equal(P(B, 1)->role, ETH_RSTP_ROLE_ALTERNATE);
	zexpect_true(is_forwarding(B, 0));
	zexpect_true(is_discarding(B, 1));
	zexpect_true(is_forwarding(A, 1));

	expect_settled();
}

/* Two ports of one bridge on a shared LAN: the worse one is a Backup Port */
ZTEST(rstp, test_backup_port)
{
	attach(A, 0, 0);
	attach(A, 1, 0);
	attach(B, 0, 0);
	sim_start();

	zexpect_false(P(A, 0)->oper_p2p);
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_DESIGNATED);
	zexpect_equal(P(A, 1)->role, ETH_RSTP_ROLE_BACKUP);
	zexpect_equal(P(B, 0)->role, ETH_RSTP_ROLE_ROOT);

	/* No agreements on a shared LAN, the Designated Port waits for the
	 * timers.
	 */
	zexpect_false(is_forwarding(A, 0));

	sim_ticks(2U * ETH_RSTP_DEFAULT_MAX_AGE);
	zexpect_true(is_forwarding(A, 0));
	zexpect_true(is_forwarding(B, 0));
	zexpect_true(is_discarding(A, 1));
	zexpect_equal(P(A, 1)->role, ETH_RSTP_ROLE_BACKUP);

	expect_settled();
}

/* An AdminEdge port forwards at once, and is no longer an Edge Port once it
 * receives a BPDU.
 */
ZTEST(rstp, test_admin_edge)
{
	struct eth_rstp_bpdu bpdu = designated_bpdu(WORSE_ID, WORSE_ID);

	attach(A, 0, 0);
	eth_rstp_port_set_edge(P(A, 0), true, false);
	sim_start();

	zexpect_true(P(A, 0)->oper_edge);
	zexpect_true(is_forwarding(A, 0));

	inject(&A->port[0], &bpdu);
	zexpect_false(P(A, 0)->oper_edge);

	expect_settled();
}

/* With AutoEdge, a port that receives no BPDU becomes an Edge Port after
 * EdgeDelay, MigrateTime on a point-to-point link (13.28.8).
 */
ZTEST(rstp, test_auto_edge)
{
	attach(A, 0, 0);
	sim_start();

	zexpect_false(is_forwarding(A, 0));

	sim_ticks(ETH_RSTP_MIGRATE_TIME - 1U);
	zexpect_false(P(A, 0)->oper_edge);
	zexpect_false(is_forwarding(A, 0));

	sim_ticks(1U);
	zexpect_true(P(A, 0)->oper_edge);
	zexpect_true(is_forwarding(A, 0));

	expect_settled();
}

/* A failed Root Port is replaced by the Alternate Port, which forwards at
 * once, and the topology change flushes the FDBs of the other bridges.
 */
ZTEST(rstp, test_root_port_failure)
{
	uint32_t a_flushes;
	uint32_t b_flushes;

	attach(A, 0, 0);
	attach(B, 0, 0);
	attach(A, 1, 1);
	attach(C, 0, 1);
	attach(B, 1, 2);
	attach(C, 1, 2);
	sim_start();
	sim_ticks(1U);

	zassert_true(is_discarding(C, 1));
	a_flushes = flushes(A);
	b_flushes = flushes(B);

	lan_down(1);

	zexpect_equal(P(C, 1)->role, ETH_RSTP_ROLE_ROOT);
	zexpect_true(is_forwarding(C, 1));
	zexpect_equal(C->rstp.root_prio.root_id, bridge_id(A));
	zexpect_equal(C->rstp.root_prio.root_path_cost, 2U * ETH_RSTP_DEFAULT_PATH_COST);

	zexpect_true(P(C, 1)->tc_while != 0U, "no topology change detected");
	zexpect_true(flushes(B) > b_flushes, "B did not flush");
	zexpect_true(flushes(A) > a_flushes, "A did not flush");

	expect_settled();
}

/* No more than TxHoldCount BPDUs leave a port in one second (13.34) */
ZTEST(rstp, test_tx_hold_count)
{
	attach(A, 0, 0);
	attach(B, 0, 0);
	zassert_ok(eth_rstp_set_tx_hold_count(&A->rstp, 1U));
	zassert_ok(eth_rstp_set_tx_hold_count(&B->rstp, 1U));
	sim_start();

	zexpect_equal(A->port[0].tx_count, 1U);
	zexpect_equal(B->port[0].tx_count, 1U);
	zexpect_false(is_forwarding(A, 0));

	/* One more BPDU per port and second, and the handshake completes */
	sim_ticks(3U);
	zexpect_true(A->port[0].tx_count <= 4U);
	zexpect_true(B->port[0].tx_count <= 4U);
	zexpect_true(is_forwarding(A, 0));
	zexpect_true(is_forwarding(B, 0));

	expect_settled();
}

/* A port that hears an STP bridge sends STP BPDUs after MigrateTime, and RST
 * BPDUs again after mcheck once the STP bridge is gone (13.32).
 */
ZTEST(rstp, test_stp_migration)
{
	struct eth_rstp_bpdu config = designated_bpdu(WORSE_ID, WORSE_ID);

	config.type = ETH_RSTP_BPDU_CONFIG;
	config.flags = 0U;

	attach(A, 0, 0);
	sim_start();

	zexpect_equal(A->port[0].last_tx.type, ETH_RSTP_BPDU_RST);

	/* Received during the first MigrateTime, the BPDU changes nothing */
	inject(&A->port[0], &config);
	zexpect_true(P(A, 0)->send_rstp);

	sim_ticks(ETH_RSTP_MIGRATE_TIME);
	inject(&A->port[0], &config);
	zexpect_false(P(A, 0)->send_rstp);

	sim_ticks(ETH_RSTP_HELLO_TIME);
	zexpect_equal(A->port[0].last_tx.type, ETH_RSTP_BPDU_CONFIG);
	zexpect_equal(A->port[0].last_tx_len, ETH_RSTP_BPDU_CONFIG_LEN);
	zexpect_equal(A->port[0].last_tx_raw[2], 0U, "Protocol Version Identifier");

	eth_rstp_port_mcheck(P(A, 0));
	pump();
	zexpect_true(P(A, 0)->send_rstp);

	sim_ticks(ETH_RSTP_HELLO_TIME);
	zexpect_equal(A->port[0].last_tx.type, ETH_RSTP_BPDU_RST);

	expect_settled();
}

/* With Force Protocol Version 0 there are no rapid transitions: ports wait
 * for the Forward Delay timers (13.7.2 d).
 */
ZTEST(rstp, test_force_version_stp)
{
	attach(A, 0, 0);
	attach(B, 0, 0);
	zassert_ok(eth_rstp_set_force_version(&A->rstp, 0U));
	zassert_ok(eth_rstp_set_force_version(&B->rstp, 0U));
	sim_start();

	zexpect_equal(A->port[0].last_tx.type, ETH_RSTP_BPDU_CONFIG);
	zexpect_equal(P(B, 0)->role, ETH_RSTP_ROLE_ROOT);

	sim_ticks(2U * ETH_RSTP_DEFAULT_FORWARD_DELAY);
	zexpect_false(is_forwarding(A, 0));
	zexpect_false(is_forwarding(B, 0));

	sim_ticks(ETH_RSTP_DEFAULT_MAX_AGE);
	zexpect_true(is_forwarding(A, 0));
	zexpect_true(is_forwarding(B, 0));

	expect_settled();
}

/* A Designated Port that receives an inferior designated BPDU with the
 * Learning flag stops forwarding (13.21, 13.29.16).
 */
ZTEST(rstp, test_dispute)
{
	struct eth_rstp_bpdu bpdu = designated_bpdu(WORSE_ID, WORSE_ID);

	attach(A, 0, 0);
	sim_start();
	sim_ticks(ETH_RSTP_MIGRATE_TIME);
	zassert_true(is_forwarding(A, 0));

	inject(&A->port[0], &bpdu);
	zexpect_true(is_forwarding(A, 0), "inferior BPDU without Learning flag");

	bpdu.flags |= ETH_RSTP_FLAG_LEARNING;
	inject(&A->port[0], &bpdu);
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_DESIGNATED);
	zexpect_true(is_discarding(A, 0));

	expect_settled();
}

/* Information whose Message Age reaches Max Age is aged out at once
 * (13.29.33).
 */
ZTEST(rstp, test_message_age)
{
	struct eth_rstp_bpdu bpdu = designated_bpdu(BETTER_ID, BETTER_ID);

	attach(A, 0, 0);
	sim_start();

	bpdu.times.message_age = ETH_RSTP_DEFAULT_MAX_AGE;
	inject(&A->port[0], &bpdu);
	zexpect_equal(A->rstp.root_port_id, 0U, "aged information selected the root");
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_DESIGNATED);

	bpdu.times.message_age = ETH_RSTP_DEFAULT_MAX_AGE - 1U;
	inject(&A->port[0], &bpdu);
	zexpect_equal(A->rstp.root_port_id, P(A, 0)->port_id);
	zexpect_equal(A->rstp.root_times.message_age, ETH_RSTP_DEFAULT_MAX_AGE);

	expect_settled();
}

/* Received information is aged out after three Hello Times without a BPDU
 * (13.29.33).
 */
ZTEST(rstp, test_info_ages_out)
{
	struct eth_rstp_bpdu bpdu = designated_bpdu(BETTER_ID, BETTER_ID);

	attach(A, 0, 0);
	sim_start();

	inject(&A->port[0], &bpdu);
	zassert_equal(P(A, 0)->role, ETH_RSTP_ROLE_ROOT);

	sim_ticks(3U * ETH_RSTP_HELLO_TIME - 1U);
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_ROOT);

	sim_ticks(1U);
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_DESIGNATED);
	zexpect_equal(A->rstp.root_port_id, 0U);

	expect_settled();
}

/* A better Bridge Priority makes a bridge the root */
ZTEST(rstp, test_bridge_priority)
{
	attach(A, 0, 0);
	attach(B, 0, 0);
	sim_start();
	zassert_equal(P(B, 0)->role, ETH_RSTP_ROLE_ROOT);

	zexpect_equal(eth_rstp_set_bridge_priority(&B->rstp, 100U), -EINVAL);
	zassert_ok(eth_rstp_set_bridge_priority(&B->rstp, 4096U));
	pump();
	sim_ticks(1U);

	zexpect_equal(B->rstp.root_port_id, 0U);
	zexpect_equal(P(A, 0)->role, ETH_RSTP_ROLE_ROOT);
	zexpect_equal(A->rstp.root_prio.root_id, bridge_id(B));
	zexpect_true(is_forwarding(A, 0));
	zexpect_true(is_forwarding(B, 0));

	expect_settled();
}

ZTEST(rstp, test_parameter_ranges)
{
	zexpect_equal(eth_rstp_set_times(&A->rstp, 5U, 15U), -EINVAL);
	zexpect_equal(eth_rstp_set_times(&A->rstp, 20U, 3U), -EINVAL);
	/* 2 * (Forward Delay - 1) >= Max Age */
	zexpect_equal(eth_rstp_set_times(&A->rstp, 20U, 10U), -EINVAL);
	zexpect_ok(eth_rstp_set_times(&A->rstp, 20U, 11U));
	zexpect_equal(eth_rstp_set_tx_hold_count(&A->rstp, 0U), -EINVAL);
	zexpect_equal(eth_rstp_set_tx_hold_count(&A->rstp, 11U), -EINVAL);
	zexpect_equal(eth_rstp_set_force_version(&A->rstp, 1U), -EINVAL);
	zexpect_equal(eth_rstp_set_force_version(&A->rstp, 3U), -EINVAL);
	zexpect_equal(eth_rstp_port_set_priority(P(A, 0), 8U), -EINVAL);
	zexpect_equal(eth_rstp_port_set_path_cost(P(A, 0), 0U), -EINVAL);
	zexpect_equal(eth_rstp_port_set_path_cost(P(A, 0), ETH_RSTP_MAX_PATH_COST + 1U), -EINVAL);
}

ZTEST_SUITE(rstp, NULL, NULL, before, NULL, NULL);
