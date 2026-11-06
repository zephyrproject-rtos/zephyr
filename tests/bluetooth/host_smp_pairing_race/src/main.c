/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
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
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U
#define LE_ACL_MAX_LEN  27U
#define HANDLE          0x0001U

#define BT_SMP_CMD_PAIRING_REQ      0x01U
#define BT_SMP_CMD_SECURITY_REQUEST 0x0BU

/* How long to wait for the pairing starts to complete. They send with no
 * simulated time passing, so this only bounds a failed wait.
 */
#define RACE_TIMEOUT K_SECONDS(1)

static struct bt_conn *conn;

/* Count of SMP Pairing Request PDUs the Host has sent. */
static atomic_t pairing_reqs;

/* The controller reports its ACL buffers free again as they are used. */
static atomic_t unreported;

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
	default:
		generic_success(&evt, opcode);
		break;
	}

	bt_hci_recv(dev, evt);
}

static void report_completed(const struct device *dev)
{
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	struct net_buf *evt;
	atomic_val_t count;

	count = atomic_clear(&unreported);
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

	bt_hci_recv(dev, evt);
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
			atomic_inc(&pairing_reqs);
		}
	}

	(void)atomic_inc(&unreported);
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

/* The L2CAP TX PDU buffers held to keep the pool empty during the race. */
static struct net_buf *held[CONFIG_BT_L2CAP_TX_BUF_COUNT];
static size_t held_count;

static K_SEM_DEFINE(worker1_done, 0, 1);
static K_SEM_DEFINE(worker2_done, 0, 1);

static int worker1_rc;
static int worker2_rc;

static K_THREAD_STACK_DEFINE(worker1_stack, 2048);
static K_THREAD_STACK_DEFINE(worker2_stack, 2048);
static K_THREAD_STACK_DEFINE(releaser_stack, 2048);
static struct k_thread worker1_thread;
static struct k_thread worker2_thread;
static struct k_thread releaser_thread;

static void worker1_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	worker1_rc = bt_conn_set_security(conn, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR);
	k_sem_give(&worker1_done);
}

static void worker2_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	worker2_rc = bt_conn_set_security(conn, BT_SECURITY_L2 | BT_SECURITY_FORCE_PAIR);
	k_sem_give(&worker2_done);
}

/* A peer's Security Request, which is allowed before pairing and of no concern
 * to a start that is waiting for its buffer.
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

	net_buf_add_u8(buf, BT_SMP_CMD_SECURITY_REQUEST);
	net_buf_add_u8(buf, 0U);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* Runs only once both workers are blocked (or done), being the lowest
 * priority. Releasing the buffers lets their pairing requests be built.
 */
static void releaser_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	peer_security_request();

	for (size_t i = 0; i < held_count; i++) {
		net_buf_unref(held[i]);
		held[i] = NULL;
	}
}

static void *setup(void)
{
	static const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
	};

	zassert_ok(bt_enable(NULL));

	/* As on Connection Complete, in the central role (the default of a
	 * connection created with bt_conn_add_le()).
	 */
	conn = bt_conn_add_le(BT_ID_DEFAULT, &addr);
	zassert_not_null(conn);
	conn->handle = HANDLE;
	bt_conn_set_state(conn, BT_CONN_CONNECTED);
	bt_l2cap_connected(conn);

	return NULL;
}

ZTEST_SUITE(host_smp_pairing_race, NULL, setup, NULL, NULL, NULL);

/* Two application threads start pairing on the same connection at the same
 * time. smp_send_pairing_req() tests SMP_FLAG_PAIRING and only sets it after a
 * blocking PDU allocation, so with the TX pool empty both threads pass the test
 * before either sets the flag, and two Pairing Requests go out. The Host must
 * send only one.
 */
static ZTEST(host_smp_pairing_race, test_concurrent_pairing_start)
{
	struct net_buf *buf;

	atomic_clear(&pairing_reqs);

	/* Empty the L2CAP TX PDU pool so the pairing requests block in
	 * smp_create_pdu() until the releaser frees the buffers.
	 */
	held_count = 0;
	while (held_count < ARRAY_SIZE(held)) {
		buf = bt_l2cap_create_pdu(NULL, 0);
		zassert_not_null(buf, "Could not drain the TX pool");
		held[held_count++] = buf;
	}

	k_thread_create(&worker1_thread, worker1_stack, K_THREAD_STACK_SIZEOF(worker1_stack),
			worker1_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	k_thread_create(&worker2_thread, worker2_stack, K_THREAD_STACK_SIZEOF(worker2_stack),
			worker2_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);
	k_thread_create(&releaser_thread, releaser_stack, K_THREAD_STACK_SIZEOF(releaser_stack),
			releaser_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);

	zassert_ok(k_sem_take(&worker1_done, RACE_TIMEOUT), "First pairing start did not finish");
	zassert_ok(k_sem_take(&worker2_done, RACE_TIMEOUT), "Second pairing start did not finish");

	zassert_equal(atomic_get(&pairing_reqs), 1,
		      "Host sent %ld Pairing Requests for concurrent security starts",
		      (long)atomic_get(&pairing_reqs));
	zassert_ok(worker1_rc, "The first start gave %d", worker1_rc);
	zassert_equal(worker2_rc, -EBUSY, "The second start gave %d", worker2_rc);

	/* Neither the refused start nor the peer has touched the first one */
	zassert_equal(conn->required_sec_level, BT_SECURITY_L2, "Required level %d",
		      conn->required_sec_level);
	zassert_true(atomic_test_bit(conn->flags, BT_CONN_FORCE_PAIR), "Force pair was lost");
}
