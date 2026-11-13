/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/keys.h"
#include "host/l2cap_internal.h"
#include "host/smp.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  27U
#define HANDLE          0x0001U

/* More than the HCI command pool holds, which has buffers of its own on top of
 * CONFIG_BT_BUF_CMD_TX_COUNT, such as those for Host Number Of Completed
 * Packets.
 */
#define HELD_CMDS_MAX 32U

/* How long to wait for the security starts and for the disconnection to
 * complete. They run with no simulated time passing, so this only bounds a
 * failed wait.
 */
#define RACE_TIMEOUT K_SECONDS(1)

static const bt_addr_le_t peer_addr = {
	.type = BT_ADDR_LE_PUBLIC,
	.a = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
};

static struct bt_conn *conn;

/* Count of SMP Pairing Request PDUs the Host has sent. */
static atomic_t pairing_reqs;

/* Count of LE Start Encryption commands the Host has sent. */
static atomic_t start_enc_cmds;

/* The controller reports its ACL buffers free again as they are used. */
static atomic_t unreported_cnt;

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void generic_success(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	rp = cmd_complete(evt, GENERIC_RP_SIZE, opcode);
	(void)memset(rp, 0, GENERIC_RP_SIZE);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported, BR/EDR not supported */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->bdaddr = (bt_addr_t){ .val = { 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6 } };
}

static void le_read_local_features(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE Encryption */
	rp->features[0] = BIT(0);
}

static void le_read_buffer_size(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
	rp->le_max_num = CONFIG_BT_L2CAP_TX_BUF_COUNT;
}

static void le_read_supp_states(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *hdr;
	uint16_t opcode;

	hdr = net_buf_pull_mem(cmd, sizeof(*hdr));
	opcode = sys_le16_to_cpu(hdr->opcode);

	switch (opcode) {
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		/* Has no response */
		return;
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		read_local_features(&evt, opcode);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		read_supported_commands(&evt, opcode);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		read_bd_addr(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		le_read_local_features(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		le_read_buffer_size(&evt, opcode);
		break;
	case BT_HCI_OP_LE_READ_SUPP_STATES:
		le_read_supp_states(&evt, opcode);
		break;
	case BT_HCI_OP_LE_START_ENCRYPTION:
		/* The Encryption Change that would follow is never sent */
		(void)atomic_inc(&start_enc_cmds);
		generic_success(&evt, opcode);
		break;
	default:
		generic_success(&evt, opcode);
		break;
	}

	(void)bt_hci_recv(dev, evt);
}

static void report_completed(const struct device *dev)
{
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	struct net_buf *evt;
	atomic_val_t count;

	count = atomic_clear(&unreported_cnt);
	if (count == 0) {
		return;
	}

	evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*ev) + sizeof(*hc));
	ev = net_buf_add(evt, sizeof(*ev));
	ev->num_handles = 1U;
	hc = net_buf_add(evt, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(HANDLE);
	hc->count = sys_cpu_to_le16((uint16_t)count);

	(void)bt_hci_recv(dev, evt);
}

static void acl_handle(const struct device *dev, struct net_buf *acl)
{
	struct bt_hci_acl_hdr *acl_hdr;
	struct bt_l2cap_hdr *l2cap_hdr;

	acl_hdr = net_buf_pull_mem(acl, sizeof(*acl_hdr));
	ARG_UNUSED(acl_hdr);

	if (acl->len >= sizeof(*l2cap_hdr)) {
		l2cap_hdr = net_buf_pull_mem(acl, sizeof(*l2cap_hdr));

		if (sys_le16_to_cpu(l2cap_hdr->cid) == BT_L2CAP_CID_SMP && acl->len >= 1U &&
		    acl->data[0] == BT_SMP_CMD_PAIRING_REQ) {
			(void)atomic_inc(&pairing_reqs);
		}
	}

	(void)atomic_inc(&unreported_cnt);
	report_completed(dev);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else if (type == BT_HCI_H4_ACL) {
		acl_handle(dev, buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static K_SEM_DEFINE(disconnected_sem, 0, 1);
static K_SEM_DEFINE(recycled_sem, 0, 1);

static void disconnected(struct bt_conn *disc_conn, uint8_t reason)
{
	ARG_UNUSED(disc_conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.disconnected = disconnected,
	.recycled = recycled,
};

/* With a display the I/O capability is DisplayOnly, so security level 3 can
 * be reached; level 4 also needs Secure Connections.
 */
static void passkey_display(struct bt_conn *auth_conn, unsigned int passkey)
{
	ARG_UNUSED(auth_conn);
	ARG_UNUSED(passkey);
}

static void auth_cancel(struct bt_conn *auth_conn)
{
	ARG_UNUSED(auth_conn);
}

static const struct bt_conn_auth_cb auth_cb = {
	.passkey_display = passkey_display,
	.cancel = auth_cancel,
};

/* The buffers held to keep a pool empty while the starts run: the L2CAP TX
 * PDUs, to block in the allocation of the Pairing Request, and the HCI
 * commands, to block in the start of encryption.
 */
static struct net_buf *held_pdus[CONFIG_BT_L2CAP_TX_BUF_COUNT];
static size_t held_pdu_count;
static struct net_buf *held_cmds[HELD_CMDS_MAX];
static size_t held_cmd_count;

/* A bond with the peer, for the starts that encrypt with its LTK */
static struct bt_keys *bond;

struct start {
	bt_security_t sec;
	int rc;
	struct k_sem done;
};

static struct start start1;
static struct start start2;

static K_THREAD_STACK_DEFINE(start1_stack, 2048);
static K_THREAD_STACK_DEFINE(start2_stack, 2048);
static K_THREAD_STACK_DEFINE(releaser_stack, 2048);
static struct k_thread start1_thread;
static struct k_thread start2_thread;
static struct k_thread releaser_thread;
static bool threads_created;

static void start_fn(void *p1, void *p2, void *p3)
{
	struct start *start = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	start->rc = bt_conn_set_security(conn, start->sec);
	k_sem_give(&start->done);
}

/* A peer's Security Request, which is allowed before pairing and of no concern
 * to a start that is under way. It asks for Secure Connections, without which
 * a Host that only pairs that way refuses it.
 */
static void peer_security_request(void)
{
	struct bt_hci_acl_hdr *acl;
	struct bt_l2cap_hdr *l2cap;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);

	acl = net_buf_add(buf, sizeof(*acl));
	acl->handle = sys_cpu_to_le16(bt_acl_handle_pack(HANDLE, BT_ACL_START));
	acl->len = sys_cpu_to_le16(sizeof(*l2cap) + 2U);

	l2cap = net_buf_add(buf, sizeof(*l2cap));
	l2cap->len = sys_cpu_to_le16(2U);
	l2cap->cid = sys_cpu_to_le16(BT_L2CAP_CID_SMP);

	(void)net_buf_add_u8(buf, BT_SMP_CMD_SECURITY_REQUEST);
	(void)net_buf_add_u8(buf, BT_SMP_AUTH_SC);

	(void)bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

static void release_held(void)
{
	for (size_t i = 0; i < held_pdu_count; i++) {
		net_buf_unref(held_pdus[i]);
		held_pdus[i] = NULL;
	}

	held_pdu_count = 0;

	for (size_t i = 0; i < held_cmd_count; i++) {
		net_buf_unref(held_cmds[i]);
		held_cmds[i] = NULL;
	}

	held_cmd_count = 0;
}

/* Runs only once both starts are blocked (or done), being the lowest priority.
 * Releasing the buffers lets the blocked start go on.
 */
static void releaser_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	peer_security_request();
	release_held();
}

/* Empty the L2CAP TX PDU pool, so that a Pairing Request blocks in
 * smp_create_pdu() until the releaser frees the buffers.
 */
static void hold_pdus(void)
{
	struct net_buf *buf;

	while (held_pdu_count < ARRAY_SIZE(held_pdus)) {
		buf = bt_l2cap_create_pdu(NULL, 0);
		zassert_not_null(buf, "Could not drain the TX pool");
		held_pdus[held_pdu_count] = buf;
		held_pdu_count++;
	}
}

/* Empty the HCI command pool, so that the start of encryption blocks in
 * bt_hci_cmd_alloc() until the releaser frees the buffers.
 */
static void hold_cmds(void)
{
	struct net_buf *buf;

	while (held_cmd_count < ARRAY_SIZE(held_cmds)) {
		buf = bt_hci_cmd_alloc(K_NO_WAIT);
		if (buf == NULL) {
			break;
		}

		held_cmds[held_cmd_count] = buf;
		held_cmd_count++;
	}

	zassert_true(held_cmd_count > 0U && held_cmd_count < ARRAY_SIZE(held_cmds),
		     "Could not drain the command pool (%zu held)", held_cmd_count);
}

static void add_bond(bool authenticated)
{
	bond = bt_keys_get_addr(BT_ID_DEFAULT, &peer_addr);
	zassert_not_null(bond);

	bond->keys = BT_KEYS_LTK_P256;
	bond->flags = authenticated ? BT_KEYS_AUTHENTICATED : 0U;
	bond->enc_size = BT_SMP_MAX_ENC_KEY_SIZE;
}

/* The first start runs until it blocks, then the second one, then the
 * releaser.
 */
static void race(bt_security_t sec1, bt_security_t sec2)
{
	start1.sec = sec1;
	start2.sec = sec2;

	threads_created = true;
	(void)k_thread_create(&start1_thread, start1_stack, K_THREAD_STACK_SIZEOF(start1_stack),
			      start_fn, &start1, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	(void)k_thread_create(&start2_thread, start2_stack, K_THREAD_STACK_SIZEOF(start2_stack),
			      start_fn, &start2, NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	(void)k_thread_create(&releaser_thread, releaser_stack,
			      K_THREAD_STACK_SIZEOF(releaser_stack), releaser_fn, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	zassert_ok(k_sem_take(&start1.done, RACE_TIMEOUT), "The first start did not finish");
	zassert_ok(k_sem_take(&start2.done, RACE_TIMEOUT), "The second start did not finish");
	zassert_ok(k_thread_join(&releaser_thread, RACE_TIMEOUT), "The releaser did not finish");
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL));
	zassert_ok(bt_conn_auth_cb_register(&auth_cb));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)atomic_clear(&pairing_reqs);
	(void)atomic_clear(&start_enc_cmds);
	k_sem_init(&start1.done, 0, 1);
	k_sem_init(&start2.done, 0, 1);
	k_sem_reset(&disconnected_sem);
	k_sem_reset(&recycled_sem);

	/* As on Connection Complete, in the central role (the default of a
	 * connection created with bt_conn_add_le()).
	 */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &peer_addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_connected(conn);
}

static void peer_disconnect(void)
{
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(HANDLE);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;

	(void)bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* Whatever a failed case left behind: the threads, the buffers, the
 * connection with its SMP context, the bond.
 */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (threads_created) {
		k_thread_abort(&start1_thread);
		k_thread_abort(&start2_thread);
		k_thread_abort(&releaser_thread);
		threads_created = false;
	}

	release_held();

	if (conn != NULL) {
		peer_disconnect();
		zassert_ok(k_sem_take(&disconnected_sem, RACE_TIMEOUT), "No disconnection");
		bt_conn_unref(conn);
		conn = NULL;
		zassert_ok(k_sem_take(&recycled_sem, RACE_TIMEOUT), "Connection not recycled");
	}

	if (bond != NULL) {
		bt_keys_clear(bond);
		bond = NULL;
	}
}

ZTEST_SUITE(host_smp_pairing_race, NULL, setup, before, after, NULL);

/* Two application threads start pairing on the same connection at the same
 * time. smp_send_pairing_req() allocates the Pairing Request with a call that
 * blocks while the TX pool is empty, so a second start meanwhile must be
 * refused before it initializes the context and sends its own request. The
 * Host must send only one.
 */
ZTEST(host_smp_pairing_race, test_concurrent_pairing_start)
{
	hold_pdus();

	race(BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR);

	zassert_equal(atomic_get(&pairing_reqs), 1,
		      "Host sent %ld Pairing Requests for concurrent security starts",
		      (long)atomic_get(&pairing_reqs));
	zassert_ok(start1.rc, "The first start gave %d", start1.rc);
	zassert_equal(start2.rc, -EBUSY, "The second start gave %d", start2.rc);

	/* Neither the refused start nor the peer has touched the first one */
	zassert_equal(conn->required_sec_level, BT_SECURITY_L2, "Required level %d",
		      conn->required_sec_level);
	zassert_true(atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR), "Force pair was lost");
}

/* With a bond whose LTK is good enough, a second start that does not force
 * pairing encrypts instead of pairing. It must not do so while a forced
 * pairing start waits for its buffer.
 */
ZTEST(host_smp_pairing_race, test_encryption_during_pairing_start)
{
	add_bond(true);
	hold_pdus();

	race(BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR, BT_SECURITY_L3);

	zassert_ok(start1.rc, "The first start gave %d", start1.rc);
	zassert_equal(start2.rc, -EBUSY, "The second start gave %d", start2.rc);
	zassert_equal(atomic_get(&start_enc_cmds), 0,
		      "Host started encryption %ld times during pairing",
		      (long)atomic_get(&start_enc_cmds));
	zassert_equal(atomic_get(&pairing_reqs), 1, "Host sent %ld Pairing Requests",
		      (long)atomic_get(&pairing_reqs));

	zassert_equal(conn->required_sec_level, BT_SECURITY_L2, "Required level %d",
		      conn->required_sec_level);
	zassert_true(atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR), "Force pair was lost");
}

/* A second start asks for a level that cannot be reached: without Secure
 * Connections, level 4. It must be refused as a second start, not fail its
 * own checks, which lowers the required level of the first one.
 */
ZTEST(host_smp_pairing_race, test_unreachable_level_during_pairing_start)
{
	hold_pdus();

	race(BT_SECURITY_L3 | BT_SECURITY_FORCE_PAIR, BT_SECURITY_L4 | BT_SECURITY_FORCE_PAIR);

	zassert_ok(start1.rc, "The first start gave %d", start1.rc);
	zassert_equal(start2.rc, -EBUSY, "The second start gave %d", start2.rc);
	zassert_equal(atomic_get(&pairing_reqs), 1, "Host sent %ld Pairing Requests",
		      (long)atomic_get(&pairing_reqs));

	zassert_equal(conn->required_sec_level, BT_SECURITY_L3, "Required level %d",
		      conn->required_sec_level);
	zassert_true(atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR), "Force pair was lost");
}

/* The other way round: the first start encrypts with the bond and blocks in
 * the command, the second forces pairing. Neither it nor the peer's Security
 * Request may start anything while the first one is under way.
 */
ZTEST(host_smp_pairing_race, test_pairing_during_encryption_start)
{
	add_bond(false);
	hold_cmds();

	race(BT_SECURITY_L2, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR);

	zassert_ok(start1.rc, "The first start gave %d", start1.rc);
	zassert_equal(start2.rc, -EBUSY, "The second start gave %d", start2.rc);
	zassert_equal(atomic_get(&pairing_reqs), 0,
		      "Host sent %ld Pairing Requests during the start of encryption",
		      (long)atomic_get(&pairing_reqs));
	zassert_equal(atomic_get(&start_enc_cmds), 1, "Host started encryption %ld times",
		      (long)atomic_get(&start_enc_cmds));

	zassert_equal(conn->required_sec_level, BT_SECURITY_L2, "Required level %d",
		      conn->required_sec_level);
	zassert_false(atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR),
		      "Force pair of the refused start was kept");
}
