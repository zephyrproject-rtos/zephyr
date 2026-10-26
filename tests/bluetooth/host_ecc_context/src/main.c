/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include <psa/crypto.h>

#include "common/long_wq.h"
#include "host/ecc.h"
#include "host/hci_core.h"
#include "host/smp.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Parameters zeroed for commands without a dedicated handler, large enough for
 * the return parameters of any command the Host sends during bring-up.
 */
#define GENERIC_RP_SIZE 64U

/* The ECC operations take no simulated time, so this only bounds a failed
 * wait.
 */
#define ECC_TIMEOUT K_SECONDS(1)

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

static void le_read_buffer_size(struct net_buf **evt, uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(27U);
	rp->le_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
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

/* The SMP PDUs the host sent, by code, the last public key among them, and
 * the ACL packets it has not been told about yet
 */
static struct {
	struct k_sem sent;
	atomic_t codes;
	uint8_t pkey[BT_PUB_KEY_LEN];
	uint8_t pdu[80];
	size_t len;
	atomic_t unreported;
	uint16_t handle;
} smp_tx;

static void completed_handler(struct k_work *work)
{
	struct bt_hci_evt_num_completed_packets *ev;
	struct bt_hci_handle_count *hc;
	struct net_buf *evt;
	atomic_val_t count;

	ARG_UNUSED(work);

	count = atomic_clear(&smp_tx.unreported);
	if (count == 0) {
		return;
	}

	evt = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*ev) + sizeof(*hc));
	ev = net_buf_add(evt, sizeof(*ev));
	ev->num_handles = 1U;
	hc = net_buf_add(evt, sizeof(*hc));
	hc->handle = sys_cpu_to_le16(smp_tx.handle);
	hc->count = sys_cpu_to_le16((uint16_t)count);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), evt);
}

static K_WORK_DEFINE(completed_work, completed_handler);

static void acl_handle(struct net_buf *acl)
{
	struct bt_hci_acl_hdr *hdr;
	uint16_t handle;

	hdr = net_buf_pull_mem(acl, sizeof(*hdr));
	handle = sys_le16_to_cpu(hdr->handle);

	/* Reassemble the L2CAP PDU, which the 27-octet buffers fragment */
	if (bt_acl_flags_pb(bt_acl_flags(handle)) == BT_ACL_START_NO_FLUSH) {
		smp_tx.len = 0U;
	}

	if (smp_tx.len + acl->len <= sizeof(smp_tx.pdu)) {
		(void)memcpy(&smp_tx.pdu[smp_tx.len], acl->data, acl->len);
		smp_tx.len += acl->len;
	}

	if (smp_tx.len > 4U && smp_tx.len == 4U + sys_get_le16(&smp_tx.pdu[0]) &&
	    sys_get_le16(&smp_tx.pdu[2]) == BT_L2CAP_CID_SMP) {
		uint8_t code = smp_tx.pdu[4];

		if (code == BT_SMP_CMD_PUBLIC_KEY && smp_tx.len == 5U + BT_PUB_KEY_LEN) {
			(void)memcpy(smp_tx.pkey, &smp_tx.pdu[5], BT_PUB_KEY_LEN);
		}

		atomic_set_bit(&smp_tx.codes, code);
		k_sem_give(&smp_tx.sent);
	}

	smp_tx.handle = bt_acl_handle(handle);
	(void)atomic_inc(&smp_tx.unreported);
	(void)k_work_submit(&completed_work);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
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
		acl_handle(buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

/* The key generations of ecc.c, held in progress or failed on request */
static struct {
	struct k_sem entered;
	struct k_sem release;
	atomic_t hold;
	atomic_t fail;
} keygen;

psa_status_t __real_psa_generate_key(const psa_key_attributes_t *attributes,
				     mbedtls_svc_key_id_t *key);

psa_status_t __wrap_psa_generate_key(const psa_key_attributes_t *attributes,
				     mbedtls_svc_key_id_t *key)
{
	if (atomic_cas(&keygen.fail, 1, 0)) {
		return PSA_ERROR_INSUFFICIENT_MEMORY;
	}

	if (atomic_get(&keygen.hold) != 0) {
		k_sem_give(&keygen.entered);
		(void)k_sem_take(&keygen.release, K_FOREVER);
	}

	return __real_psa_generate_key(attributes, key);
}

static void keygen_release(void)
{
	atomic_clear(&keygen.hold);
	k_sem_give(&keygen.release);
}

/* What the ECC callback under test was given, and where it ran */
static struct {
	struct k_sem called;
	unsigned int calls;
	bool key;
	bool on_bt_workq;
	uint8_t pub_key[BT_PUB_KEY_LEN];
} seen;

static void record_call(const uint8_t *key)
{
	seen.calls++;
	seen.key = (key != NULL);
	seen.on_bt_workq = bt_is_work_thread();
	k_sem_give(&seen.called);
}

static void record_pub_key(const uint8_t key[BT_PUB_KEY_LEN])
{
	if (key != NULL) {
		(void)memcpy(seen.pub_key, key, BT_PUB_KEY_LEN);
	}

	record_call(key);
}

static void noop_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

/* Return once the work items queued so far on the Bluetooth workqueue have run */
static void drain_bt_workq(void)
{
	struct k_work_sync sync;
	struct k_work work;

	k_work_init(&work, noop_handler);
	zassert_equal(bt_work_submit(&work), 1, "Work item not queued");
	(void)k_work_flush(&work, &sync);
}

/* The workqueue that ecc.c computes on */
static int ecc_workq_submit(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_BT_LONG_WQ)) {
		return bt_long_wq_submit(work);
	}

	return k_work_submit(work);
}

/* Return once the ECC operations queued so far have completed and their
 * results have been delivered
 */
static void settle(void)
{
	struct k_work_sync sync;
	struct k_work work;

	k_work_init(&work, noop_handler);
	zassert_equal(ecc_workq_submit(&work), 1, "Work item not queued");
	(void)k_work_flush(&work, &sync);

	drain_bt_workq();
}

/* The local public key requested by bt_enable() or by an earlier test */
static const uint8_t *wait_for_pub_key(void)
{
	const uint8_t *key = bt_pub_key_get();

	for (int i = 0; i < 100 && key == NULL; i++) {
		k_sleep(K_MSEC(10));
		key = bt_pub_key_get();
	}

	return key;
}

static void *setup(void)
{
	zassert_ok(bt_enable(NULL));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_init(&keygen.entered, 0, 1);
	k_sem_init(&keygen.release, 0, 1);
	atomic_clear(&keygen.hold);
	atomic_clear(&keygen.fail);

	k_sem_init(&seen.called, 0, 1);
	seen.calls = 0U;
	seen.key = false;
	seen.on_bt_workq = false;

	zassert_not_null(wait_for_pub_key(), "Local public key not generated");
}

ZTEST_SUITE(host_ecc_context, NULL, setup, before, NULL, NULL);

/* SMP continues pairing from the ECC callbacks, so they have to run on the
 * Bluetooth workqueue, serialized with the PDUs and the disconnection of the
 * same connection.
 */
static ZTEST(host_ecc_context, test_dh_key_delivered_on_bt_workq)
{
	zassert_ok(bt_dh_key_gen(bt_pub_key_get(), record_call));
	zassert_ok(k_sem_take(&seen.called, ECC_TIMEOUT), "DH key callback not called");

	zassert_true(seen.key, "No DH key");
	zassert_true(seen.on_bt_workq, "DH key not delivered on the Bluetooth workqueue");
}

static ZTEST(host_ecc_context, test_pub_key_delivered_on_bt_workq)
{
	static struct bt_pub_key_cb cb = { .func = record_call };
	int disable_err;
	int enable_err;
	int gen_err;

	/* Registered while the key requested by the enable is generated */
	disable_err = bt_disable();
	atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	gen_err = bt_pub_key_gen(&cb);
	keygen_release();

	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_ok(gen_err);
	zassert_ok(k_sem_take(&seen.called, ECC_TIMEOUT), "Public key callback not called");

	zassert_true(seen.key, "No public key");
	zassert_true(seen.on_bt_workq, "Public key not delivered on the Bluetooth workqueue");
}

/* A published key stays until bt_disable(): pairings and OOB data may be
 * using it.
 */
static ZTEST(host_ecc_context, test_pub_key_gen_keeps_key)
{
	static struct bt_pub_key_cb cb = { .func = record_call };
	uint8_t key[BT_PUB_KEY_LEN];
	int gen_err;

	(void)memcpy(key, bt_pub_key_get(), sizeof(key));

	gen_err = bt_pub_key_gen(&cb);
	settle();

	zassert_equal(gen_err, -EALREADY, "bt_pub_key_gen() returned %d", gen_err);
	zassert_equal(seen.calls, 0U, "Public key callback called %u times", seen.calls);
	zassert_mem_equal(bt_pub_key_get(), key, sizeof(key), "Published public key replaced");
}

struct workq_blocker {
	struct k_work work;
	struct k_sem release;
};

static void workq_blocker_handler(struct k_work *work)
{
	struct workq_blocker *blocker = CONTAINER_OF(work, struct workq_blocker, work);

	(void)k_sem_take(&blocker->release, K_FOREVER);
}

/* A public key still waiting for its delivery when bt_disable() comes is not
 * published: the disable completes the callback without a key, and the next
 * enable generates a new one.
 */
static ZTEST(host_ecc_context, test_pub_key_delivery_after_disable)
{
	static struct bt_pub_key_cb cb = { .func = record_call };
	static struct workq_blocker blocker;
	unsigned int calls_while_held;
	int disable_err[2];
	int submit_ret;
	int enable_err;
	int gen_err;

	k_work_init(&blocker.work, workq_blocker_handler);
	k_sem_init(&blocker.release, 0, 1);

	/* The key requested by the enable is generated during the sleep and
	 * its delivery waits behind the blocker. Nothing is asserted before
	 * the releases, which a failed assertion would skip.
	 */
	disable_err[0] = bt_disable();
	atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	gen_err = bt_pub_key_gen(&cb);
	submit_ret = bt_work_submit(&blocker.work);
	keygen_release();
	k_sleep(K_MSEC(100));
	calls_while_held = seen.calls;
	disable_err[1] = bt_disable();

	k_sem_give(&blocker.release);
	drain_bt_workq();

	zassert_ok(disable_err[0]);
	zassert_ok(enable_err);
	zassert_ok(gen_err);
	zassert_equal(submit_ret, 1, "Work item not queued");
	zassert_equal(calls_while_held, 0U,
		      "Public key delivered while the Bluetooth workqueue was held");
	zassert_ok(disable_err[1]);
	zassert_equal(seen.calls, 1U, "Public key callback called %u times", seen.calls);
	zassert_false(seen.key, "Public key callback given a key after the disable");
	zassert_is_null(bt_pub_key_get(), "Public key published after the disable");

	zassert_ok(bt_enable(NULL));
	zassert_not_null(wait_for_pub_key(), "No public key after the enable");
}

/* A DH Key calculation still pending when bt_disable() comes does not keep the
 * next bt_enable() from generating a new public key.
 */
static ZTEST(host_ecc_context, test_pub_key_after_disable_during_dh_key)
{
	static struct workq_blocker blocker;
	uint8_t old_key[BT_PUB_KEY_LEN];
	const uint8_t *key;
	int disable_err;
	int enable_err;
	int dh_err;

	(void)memcpy(old_key, bt_pub_key_get(), sizeof(old_key));

	k_work_init(&blocker.work, workq_blocker_handler);
	k_sem_init(&blocker.release, 0, 1);
	zassert_equal(ecc_workq_submit(&blocker.work), 1, "Work item not queued");

	/* The calculation waits behind the blocker across the disable and the
	 * enable. Nothing is asserted before the release, which a failed
	 * assertion would skip.
	 */
	dh_err = bt_dh_key_gen(old_key, record_call);
	disable_err = bt_disable();
	enable_err = bt_enable(NULL);

	k_sem_give(&blocker.release);
	settle();
	key = wait_for_pub_key();

	zassert_ok(dh_err);
	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_not_null(key, "No public key after the enable");
	zassert_true(memcmp(key, old_key, sizeof(old_key)) != 0,
		     "Public key of the previous enable kept");
}

/* A generation queued before bt_disable() can still be running when the next
 * bt_enable() requests a key, which queues it once more. The key published
 * for that request must stay.
 */
static ZTEST(host_ecc_context, test_pub_key_kept_after_queued_again)
{
	static struct bt_pub_key_cb cb = { .func = record_pub_key };
	static struct workq_blocker blocker;
	int disable_err[2];
	int enable_err[2];
	int gen_err;

	k_work_init(&blocker.work, workq_blocker_handler);
	k_sem_init(&blocker.release, 0, 1);
	zassert_equal(ecc_workq_submit(&blocker.work), 1, "Work item not queued");

	/* The generation requested by the enable waits behind the blocker,
	 * and the disable ends its request. Nothing is asserted before the
	 * releases, which a failed assertion would skip.
	 */
	disable_err[0] = bt_disable();
	enable_err[0] = bt_enable(NULL);
	disable_err[1] = bt_disable();

	/* A generation that starts is held, and the key is requested again
	 * meanwhile.
	 */
	atomic_set(&keygen.hold, 1);
	k_sem_give(&blocker.release);
	k_sleep(K_MSEC(10));
	enable_err[1] = bt_enable(NULL);
	gen_err = bt_pub_key_gen(&cb);

	keygen_release();
	settle();

	zassert_ok(disable_err[0]);
	zassert_ok(enable_err[0]);
	zassert_ok(disable_err[1]);
	zassert_ok(enable_err[1]);
	zassert_ok(gen_err);
	zassert_equal(seen.calls, 1U, "Public key callback called %u times", seen.calls);
	zassert_true(seen.key, "No public key");
	zassert_not_null(bt_pub_key_get(), "No public key published");
	zassert_mem_equal(bt_pub_key_get(), seen.pub_key, BT_PUB_KEY_LEN,
			  "Published public key replaced");
}

/* A public key callback registered after SMP's, so called before it: the key
 * is published, and SMP has not been told yet.
 */
static struct {
	struct k_sem entered;
	struct k_sem release;
	uint8_t key[BT_PUB_KEY_LEN];
} delivery;

static void hold_delivery(const uint8_t key[BT_PUB_KEY_LEN])
{
	if (key != NULL) {
		(void)memcpy(delivery.key, key, BT_PUB_KEY_LEN);
	}

	k_sem_give(&delivery.entered);
	(void)k_sem_take(&delivery.release, K_FOREVER);
}

#define OOB_THREADS    2
#define OOB_STACK_SIZE 4096

static K_THREAD_STACK_ARRAY_DEFINE(oob_stacks, OOB_THREADS, OOB_STACK_SIZE);
static struct k_thread oob_threads[OOB_THREADS];

static struct {
	struct bt_le_oob oob;
	int err;
} oob_results[OOB_THREADS];

static void oob_thread(void *p1, void *p2, void *p3)
{
	size_t i = POINTER_TO_UINT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	oob_results[i].err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_results[i].oob);
}

static void oob_start(size_t i)
{
	(void)k_thread_create(&oob_threads[i], oob_stacks[i], K_THREAD_STACK_SIZEOF(oob_stacks[i]),
			      oob_thread, UINT_TO_POINTER(i), NULL, NULL, K_PRIO_PREEMPT(1), 0,
			      K_NO_WAIT);
}

/* OOB data asked for after bt_disable() and bt_enable(), before the new key
 * is there, waits for it instead of being made with the previous one.
 */
static ZTEST(host_ecc_context, test_oob_data_waits_for_new_pub_key)
{
	struct bt_le_oob oob;
	int entered_err;
	int disable_err;
	int enable_err;
	int join_err;
	int oob_err;

	/* Nothing is asserted before the release, which a failed assertion
	 * would skip.
	 */
	disable_err = bt_disable();
	atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	entered_err = k_sem_take(&keygen.entered, ECC_TIMEOUT);
	oob_start(0);
	k_sleep(K_MSEC(10));
	keygen_release();

	join_err = k_thread_join(&oob_threads[0], ECC_TIMEOUT);
	settle();

	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_ok(entered_err, "No key generation");
	zassert_ok(join_err);
	zassert_ok(oob_results[0].err, "OOB data: %d", oob_results[0].err);

	oob_err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	zassert_ok(oob_err);
	zassert_mem_equal(oob_results[0].oob.le_sc_data.c, oob.le_sc_data.c,
			  sizeof(oob.le_sc_data.c), "OOB data made with another public key");
}

/* OOB data asked for after a new key has been published but before SMP has
 * been told is made with that key, and the key stays.
 */
static ZTEST(host_ecc_context, test_oob_data_while_pub_key_delivered)
{
	static struct bt_pub_key_cb cb = { .func = hold_delivery };
	struct bt_le_oob oob;
	int entered_err[2];
	int join_err[OOB_THREADS];
	int disable_err;
	int enable_err;
	int gen_err;
	int oob_err;

	k_sem_init(&delivery.entered, 0, 1);
	k_sem_init(&delivery.release, 0, 1);

	/* A failed generation leaves SMP without a key */
	disable_err = bt_disable();
	atomic_set(&keygen.fail, 1);
	enable_err = bt_enable(NULL);
	settle();
	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_is_null(bt_pub_key_get(), "Public key generated");

	/* The first caller requests a new key and waits for it. Its delivery
	 * is held after the publication, while the second caller asks.
	 * Nothing is asserted before the releases, which a failed assertion
	 * would skip.
	 */
	atomic_set(&keygen.hold, 1);
	oob_start(0);
	entered_err[0] = k_sem_take(&keygen.entered, ECC_TIMEOUT);
	gen_err = bt_pub_key_gen(&cb);
	keygen_release();
	entered_err[1] = k_sem_take(&delivery.entered, ECC_TIMEOUT);

	oob_start(1);
	k_sleep(K_MSEC(10));
	k_sem_give(&delivery.release);

	for (size_t i = 0; i < OOB_THREADS; i++) {
		join_err[i] = k_thread_join(&oob_threads[i], ECC_TIMEOUT);
	}

	settle();

	zassert_ok(entered_err[0], "No key generation");
	zassert_ok(gen_err);
	zassert_ok(entered_err[1], "Public key not delivered");

	for (size_t i = 0; i < OOB_THREADS; i++) {
		zassert_ok(join_err[i]);
		zassert_ok(oob_results[i].err, "OOB data %zu: %d", i, oob_results[i].err);
	}

	zassert_not_null(bt_pub_key_get(), "No public key published");
	zassert_mem_equal(bt_pub_key_get(), delivery.key, BT_PUB_KEY_LEN,
			  "Published public key replaced");

	oob_err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	zassert_ok(oob_err);

	for (size_t i = 0; i < OOB_THREADS; i++) {
		zassert_mem_equal(oob_results[i].oob.le_sc_data.c, oob.le_sc_data.c,
				  sizeof(oob.le_sc_data.c),
				  "OOB data %zu made with another public key", i);
	}
}

/* bt_pub_key_get() as called by the host: when armed and there is no key, the
 * key is generated and delivered before the caller gets the NULL it read.
 */
static atomic_t delivery_after_get;

const uint8_t *__real_bt_pub_key_get(void);

const uint8_t *__wrap_bt_pub_key_get(void)
{
	const uint8_t *key = __real_bt_pub_key_get();

	if (key == NULL && atomic_cas(&delivery_after_get, 1, 0)) {
		keygen_release();
		k_sleep(K_MSEC(10));
	}

	return key;
}

#define CONN_HANDLE 0x0001

static struct bt_conn *test_conn;
static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (err == 0U) {
		test_conn = bt_conn_ref(conn);
	}

	k_sem_give(&connected_sem);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(test_conn_cb) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static const bt_addr_le_t peer_addr = {
	.type = BT_ADDR_LE_PUBLIC,
	.a = { .val = { 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6 } },
};

static void inject_conn_complete(uint8_t role)
{
	struct net_buf *buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	struct bt_hci_evt_le_meta_event *mev;
	struct bt_hci_evt_le_conn_complete *cc;

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*mev) + sizeof(*cc));
	mev = net_buf_add(buf, sizeof(*mev));
	cc = net_buf_add(buf, sizeof(*cc));

	mev->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	cc->status = BT_HCI_ERR_SUCCESS;
	cc->handle = sys_cpu_to_le16(CONN_HANDLE);
	cc->role = role;
	bt_addr_le_copy(&cc->peer_addr, &peer_addr);
	cc->interval = sys_cpu_to_le16(24U);
	cc->latency = 0U;
	cc->supv_timeout = sys_cpu_to_le16(400U);
	cc->clock_accuracy = 0U;

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

static void inject_disconn_complete(void)
{
	struct net_buf *buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	struct bt_hci_evt_disconn_complete *dc;

	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*dc));
	dc = net_buf_add(buf, sizeof(*dc));
	dc->status = BT_HCI_ERR_SUCCESS;
	dc->handle = sys_cpu_to_le16(CONN_HANDLE);
	dc->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* An SMP PDU from the central */
static void inject_smp(uint8_t code, const void *data, uint16_t len)
{
	struct net_buf *buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	struct bt_hci_acl_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(CONN_HANDLE, BT_ACL_START));
	hdr->len = sys_cpu_to_le16(4U + 1U + len);
	net_buf_add_le16(buf, 1U + len);
	net_buf_add_le16(buf, BT_L2CAP_CID_SMP);
	net_buf_add_u8(buf, code);
	(void)net_buf_add_mem(buf, data, len);

	bt_hci_recv(DEVICE_DT_GET(DT_DRV_INST(0)), buf);
}

/* Waits for the host to send an SMP PDU with the given code */
static bool smp_sent(uint8_t code)
{
	for (int i = 0; i < 10 && !atomic_test_bit(&smp_tx.codes, code); i++) {
		(void)k_sem_take(&smp_tx.sent, K_MSEC(10));
	}

	return atomic_test_bit(&smp_tx.codes, code);
}

/* The Core Specification debug public key, X and Y little-endian, as the
 * central's key
 */
static const uint8_t central_pkey[BT_PUB_KEY_LEN] = {
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
	0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
	0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20,
	0x8b, 0xd2, 0x89, 0x15, 0xd0, 0x8e, 0x1c, 0x74,
	0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63,
	0x6d, 0xeb, 0x2a, 0x65, 0x49, 0x9c, 0x80, 0xdc,
};

/* LE Secure Connections Just Works, no bonding */
static const struct bt_smp_pairing pairing_req = {
	.io_capability = BT_SMP_IO_NO_INPUT_OUTPUT,
	.oob_flag = BT_SMP_OOB_NOT_PRESENT,
	.auth_req = BT_SMP_AUTH_SC,
	.max_key_size = BT_SMP_MAX_ENC_KEY_SIZE,
};

struct pairing_errs {
	int disable;
	int enable;
	int adv;
	int sec;
};

static void pairing_reset(void)
{
	k_sem_init(&smp_tx.sent, 0, K_SEM_MAX_LIMIT);
	atomic_clear(&smp_tx.codes);
	(void)memset(smp_tx.pkey, 0, sizeof(smp_tx.pkey));
	smp_tx.len = 0U;
	test_conn = NULL;
}

/* Gets connected as peripheral and starts the security procedure, delivering
 * the key while it starts if asked
 */
static void pairing_connect(struct pairing_errs *errs, bool deliver_during_start)
{
	errs->adv = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0, NULL, 0);
	inject_conn_complete(BT_HCI_ROLE_PERIPHERAL);
	(void)k_sem_take(&connected_sem, ECC_TIMEOUT);

	atomic_set(&delivery_after_get, deliver_during_start ? 1 : 0);
	errs->sec =
		(test_conn != NULL) ? bt_conn_set_security(test_conn, BT_SECURITY_L2) : -ENOTCONN;
	atomic_clear(&delivery_after_get);
}

/* Re-enables with the key generation held, then connects */
static void pairing_start(struct pairing_errs *errs, bool deliver_during_start)
{
	pairing_reset();

	errs->disable = bt_disable();
	atomic_set(&keygen.hold, 1);
	errs->enable = bt_enable(NULL);
	pairing_connect(errs, deliver_during_start);
}

static void pairing_end(void)
{
	settle();

	if (test_conn != NULL) {
		(void)bt_conn_disconnect(test_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		inject_disconn_complete();
		(void)k_sem_take(&disconnected_sem, ECC_TIMEOUT);
		bt_conn_unref(test_conn);
		test_conn = NULL;
	}
}

static void pairing_assert_started(const struct pairing_errs *errs)
{
	zassert_ok(errs->disable);
	zassert_ok(errs->enable);
	zassert_ok(errs->adv);
	zassert_ok(errs->sec);
	zassert_true(atomic_test_bit(&smp_tx.codes, BT_SMP_CMD_SECURITY_REQUEST),
		     "No Security Request sent");
}

/* A pairing started while the key of an enable is generated sends the key once
 * it is there, also when the key arrives while the pairing is being started.
 * Nothing is asserted before the end of the pairing, which a failed assertion
 * would skip.
 */
static ZTEST(host_ecc_context, test_pairing_started_during_pub_key_delivery)
{
	struct pairing_errs errs;
	const uint8_t *key;
	bool rsp_sent;
	bool pkey_sent;

	pairing_start(&errs, true);
	keygen_release();
	settle();

	inject_smp(BT_SMP_CMD_PAIRING_REQ, &pairing_req, sizeof(pairing_req));
	rsp_sent = smp_sent(BT_SMP_CMD_PAIRING_RSP);
	inject_smp(BT_SMP_CMD_PUBLIC_KEY, central_pkey, sizeof(central_pkey));
	pkey_sent = smp_sent(BT_SMP_CMD_PUBLIC_KEY);

	pairing_end();
	key = bt_pub_key_get();

	pairing_assert_started(&errs);
	zassert_true(rsp_sent, "No Pairing Response sent");
	zassert_true(pkey_sent, "Public key not sent");
	zassert_not_null(key, "No public key published");
	zassert_mem_equal(smp_tx.pkey, key, BT_PUB_KEY_LEN, "Another public key sent");
}

/* A pairing that reaches the public key exchange before the key of a re-enable
 * is there waits for that key instead of sending the previous one.
 */
static ZTEST(host_ecc_context, test_pairing_waits_for_pub_key_of_enable)
{
	struct pairing_errs errs;
	const uint8_t *key;
	bool pkey_sent;

	/* With the generation on the system workqueue, holding it also holds
	 * the link, and the key is there before the public key exchange.
	 */
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_LONG_WQ);

	pairing_start(&errs, false);

	inject_smp(BT_SMP_CMD_PAIRING_REQ, &pairing_req, sizeof(pairing_req));
	inject_smp(BT_SMP_CMD_PUBLIC_KEY, central_pkey, sizeof(central_pkey));
	k_sleep(K_MSEC(10));

	keygen_release();
	settle();
	pkey_sent = smp_sent(BT_SMP_CMD_PUBLIC_KEY);

	pairing_end();
	key = bt_pub_key_get();

	pairing_assert_started(&errs);
	zassert_true(atomic_test_bit(&smp_tx.codes, BT_SMP_CMD_PAIRING_RSP),
		     "No Pairing Response sent");
	zassert_true(pkey_sent, "Public key not sent");
	zassert_not_null(key, "No public key published");
	zassert_mem_equal(smp_tx.pkey, key, BT_PUB_KEY_LEN, "Another public key sent");
	zassert_false(atomic_test_bit(&smp_tx.codes, BT_SMP_CMD_PAIRING_FAIL), "Pairing failed");
}

/* A key generated after bt_disable(), by a request that crossed it, is kept by
 * the next bt_enable() with the no-reset quirk, without a callback to SMP. A
 * pairing uses that key instead of waiting for another one.
 */
static ZTEST(host_ecc_context, test_pairing_with_pub_key_from_before_enable)
{
	static struct bt_pub_key_cb cb = { .func = record_pub_key };
	struct pairing_errs errs;
	const uint8_t *key;
	bool pkey_sent;
	int gen_err;

	pairing_reset();

	errs.disable = bt_disable();
	gen_err = bt_pub_key_gen(&cb);
	settle();
	errs.enable = bt_enable(NULL);
	pairing_connect(&errs, false);

	inject_smp(BT_SMP_CMD_PAIRING_REQ, &pairing_req, sizeof(pairing_req));
	inject_smp(BT_SMP_CMD_PUBLIC_KEY, central_pkey, sizeof(central_pkey));
	pkey_sent = smp_sent(BT_SMP_CMD_PUBLIC_KEY);

	pairing_end();
	key = bt_pub_key_get();

	zassert_ok(gen_err);
	zassert_true(seen.key, "No key generated after the disable");
	pairing_assert_started(&errs);
	zassert_true(pkey_sent, "Public key not sent");
	zassert_not_null(key, "No public key published");
	zassert_mem_equal(smp_tx.pkey, key, BT_PUB_KEY_LEN, "Another public key sent");
}

/* z_impl_atomic_or() as called by the host: when armed for a thread, the key
 * is generated and delivered just before the next atomic OR of that thread.
 */
static struct {
	atomic_t armed;
	k_tid_t thread;
} delivery_before_or;

#if defined(CONFIG_TEST_ATOMIC_OPERATIONS_C)
atomic_val_t __real_z_impl_atomic_or(atomic_t *target, atomic_val_t value);

atomic_val_t __wrap_z_impl_atomic_or(atomic_t *target, atomic_val_t value)
{
	if (delivery_before_or.thread == k_current_get() &&
	    atomic_cas(&delivery_before_or.armed, 1, 0)) {
		keygen_release();
		k_sleep(K_MSEC(10));
	}

	return __real_z_impl_atomic_or(target, value);
}
#endif /* CONFIG_TEST_ATOMIC_OPERATIONS_C */

static K_SEM_DEFINE(pairing_confirm_sem, 0, 1);

static void pairing_confirm_cb(struct bt_conn *conn)
{
	ARG_UNUSED(conn);

	k_sem_give(&pairing_confirm_sem);
}

static void auth_cancel_cb(struct bt_conn *conn)
{
	ARG_UNUSED(conn);
}

/* As central, a Just Works pairing that the peripheral requested and that the
 * application confirms while the key of an enable is generated sends the key
 * once it is there, also when the key arrives during the confirmation. The
 * confirmation exists only with legacy pairing enabled.
 */
static ZTEST(host_ecc_context, test_pairing_confirm_during_pub_key_delivery)
{
	static const struct bt_conn_auth_cb auth_cb = {
		.pairing_confirm = pairing_confirm_cb,
		.cancel = auth_cancel_cb,
	};
	static const struct bt_smp_security_request sec_req = {
		.auth_req = BT_SMP_AUTH_SC,
	};
	struct bt_conn *conn = NULL;
	const uint8_t *key;
	int confirm_requested;
	int confirm_err;
	int disable_err;
	int enable_err;
	int auth_err;
	int conn_err;
	bool pkey_sent;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_SMP_SC_PAIR_ONLY);
	Z_TEST_SKIP_IFNDEF(CONFIG_TEST_ATOMIC_OPERATIONS_C);

	pairing_reset();
	k_sem_reset(&pairing_confirm_sem);

	/* Nothing is asserted before the end of the pairing, which a failed
	 * assertion would skip.
	 */
	disable_err = bt_disable();
	atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	auth_err = bt_conn_auth_cb_register(&auth_cb);
	conn_err = bt_conn_le_create(&peer_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				     &conn);
	if (conn_err == 0) {
		inject_conn_complete(BT_HCI_ROLE_CENTRAL);
		(void)k_sem_take(&connected_sem, ECC_TIMEOUT);
		bt_conn_unref(conn);
	}

	inject_smp(BT_SMP_CMD_SECURITY_REQUEST, &sec_req, sizeof(sec_req));
	inject_smp(BT_SMP_CMD_PAIRING_RSP, &pairing_req, sizeof(pairing_req));
	confirm_requested = k_sem_take(&pairing_confirm_sem, ECC_TIMEOUT);

	delivery_before_or.thread = k_current_get();
	atomic_set(&delivery_before_or.armed, 1);
	confirm_err = (test_conn != NULL) ? bt_conn_auth_pairing_confirm(test_conn) : -ENOTCONN;
	atomic_clear(&delivery_before_or.armed);
	keygen_release();
	settle();
	pkey_sent = smp_sent(BT_SMP_CMD_PUBLIC_KEY);

	pairing_end();
	(void)bt_conn_auth_cb_register(NULL);
	key = bt_pub_key_get();

	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_ok(auth_err);
	zassert_ok(conn_err);
	zassert_ok(confirm_requested, "Pairing confirmation not requested");
	zassert_ok(confirm_err);
	zassert_true(pkey_sent, "Public key not sent");
	zassert_not_null(key, "No public key published");
	zassert_mem_equal(smp_tx.pkey, key, BT_PUB_KEY_LEN, "Another public key sent");
}
