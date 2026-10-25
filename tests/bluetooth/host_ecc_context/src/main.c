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
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
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

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Parameters zeroed for commands without a dedicated handler, large enough for the return
 * parameters of any command the Host sends during bring-up.
 */
#define GENERIC_RP_SIZE 64U

/* The ECC operations take no simulated time, so this only bounds a failed wait. */
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

/* Only declared: the linker option --wrap=psa_generate_key (see CMakeLists.txt) resolves it to the
 * PSA psa_generate_key() and routes the calls of psa_generate_key() in ecc.c to
 * __wrap_psa_generate_key() below, so ecc.c needs no test hook.
 */
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
	(void)atomic_clear(&keygen.hold);
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

/* The test thread is cooperative, so the other threads only run while it blocks. They take no
 * simulated time on native_sim, so any sleep lets them run until they wait.
 */
static void run_other_threads(void)
{
	(void)k_sleep(K_MSEC(10));
}

/* The workqueue that ecc.c computes on */
static int ecc_workq_submit(struct k_work *work)
{
	if (IS_ENABLED(CONFIG_BT_LONG_WQ)) {
		return bt_long_wq_submit(work);
	}

	return k_work_submit(work);
}

/* Return once the ECC operations queued so far have completed and their results have been
 * delivered
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

/* Return the local public key requested by bt_enable() or by an earlier test once it is published,
 * or NULL if it is not within ECC_TIMEOUT. Polled, as waiting for it with bt_pub_key_gen() would
 * request a key when there is none.
 */
static const uint8_t *wait_for_pub_key(void)
{
	k_timepoint_t end = sys_timepoint_calc(ECC_TIMEOUT);

	while (bt_pub_key_get() == NULL && !sys_timepoint_expired(end)) {
		run_other_threads();
	}

	return bt_pub_key_get();
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
	(void)atomic_clear(&keygen.hold);
	(void)atomic_clear(&keygen.fail);

	k_sem_init(&seen.called, 0, 1);
	seen.calls = 0U;
	seen.key = false;
	seen.on_bt_workq = false;

	zassert_not_null(wait_for_pub_key(), "Local public key not generated");
}

ZTEST_SUITE(host_ecc_context, NULL, setup, before, NULL, NULL);

/* SMP continues pairing from the ECC callbacks, so they have to run on the Bluetooth workqueue,
 * serialized with the PDUs and the disconnection of the same connection.
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
	(void)atomic_set(&keygen.hold, 1);
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

/* A published key stays until bt_disable(): pairings and OOB data may be using it. */
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

/* A public key still waiting for its delivery when bt_disable() comes is not published: the
 * disable completes the callback without a key, and the next enable generates a new one.
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

	/* The key requested by the enable is generated while the other threads run, and its
	 * delivery waits behind the blocker. Nothing is asserted before the releases, which a
	 * failed assertion would skip.
	 */
	disable_err[0] = bt_disable();
	(void)atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	gen_err = bt_pub_key_gen(&cb);
	submit_ret = bt_work_submit(&blocker.work);
	keygen_release();
	run_other_threads();
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

/* A DH Key calculation still pending when bt_disable() comes does not keep the next bt_enable()
 * from generating a new public key.
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

	/* The calculation waits behind the blocker across the disable and the enable. Nothing is
	 * asserted before the release, which a failed assertion would skip.
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
	zassert_false(util_memeq(key, old_key, sizeof(old_key)),
		      "Public key of the previous enable kept");
}

/* A generation queued before bt_disable() can still be running when the next bt_enable() requests
 * a key, which queues it once more. The key published for that request must stay.
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

	/* The generation requested by the enable waits behind the blocker, and the disable ends its
	 * request. Nothing is asserted before the releases, which a failed assertion would skip.
	 */
	disable_err[0] = bt_disable();
	enable_err[0] = bt_enable(NULL);
	disable_err[1] = bt_disable();

	/* The generation queued by the first enable gets to run, and is held if it starts: it can
	 * still be running when the key is requested again.
	 */
	(void)atomic_set(&keygen.hold, 1);
	k_sem_give(&blocker.release);
	run_other_threads();
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

/* A public key callback registered after SMP's, so called before it: the key is published, and
 * SMP has not been told yet.
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

/* -EBUSY while the thread of index i is still running from an earlier start. The IDs are kept
 * across tests on purpose: a thread left running by an earlier test counts too.
 */
static int oob_start(size_t i)
{
	static k_tid_t tids[OOB_THREADS];

	if (tids[i] != NULL && k_thread_join(tids[i], K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	tids[i] = k_thread_create(&oob_threads[i], oob_stacks[i],
				  K_THREAD_STACK_SIZEOF(oob_stacks[i]), oob_thread,
				  UINT_TO_POINTER(i), NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	return 0;
}

/* OOB data asked for after bt_disable() and bt_enable(), before the new key is there, waits for it
 * instead of being made with the previous one.
 */
static ZTEST(host_ecc_context, test_oob_data_waits_for_new_pub_key)
{
	struct bt_le_oob oob;
	int entered_err;
	int disable_err;
	int enable_err;
	int start_err;
	int join_err;
	int oob_err;

	/* Nothing is asserted before the release, which a failed assertion would skip. */
	disable_err = bt_disable();
	(void)atomic_set(&keygen.hold, 1);
	enable_err = bt_enable(NULL);
	entered_err = k_sem_take(&keygen.entered, ECC_TIMEOUT);

	/* The OOB thread asks for the data while the generation is held */
	start_err = oob_start(0);
	run_other_threads();
	keygen_release();

	join_err = k_thread_join(&oob_threads[0], ECC_TIMEOUT);
	settle();

	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_ok(entered_err, "No key generation");
	zassert_ok(start_err, "OOB thread still running");
	zassert_ok(join_err);
	zassert_ok(oob_results[0].err, "OOB data: %d", oob_results[0].err);

	oob_err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	zassert_ok(oob_err);
	zassert_mem_equal(oob_results[0].oob.le_sc_data.c, oob.le_sc_data.c,
			  sizeof(oob.le_sc_data.c), "OOB data made with another public key");
}

/* OOB data asked for after a new key has been published but before SMP has been told is made with
 * that key, and the key stays.
 */
static ZTEST(host_ecc_context, test_oob_data_while_pub_key_delivered)
{
	static struct bt_pub_key_cb cb = { .func = hold_delivery };
	int start_err[OOB_THREADS];
	int join_err[OOB_THREADS];
	struct bt_le_oob oob;
	int entered_err[2];
	int disable_err;
	int enable_err;
	int gen_err;
	int oob_err;

	k_sem_init(&delivery.entered, 0, 1);
	k_sem_init(&delivery.release, 0, 1);

	/* A failed generation leaves SMP without a key */
	disable_err = bt_disable();
	(void)atomic_set(&keygen.fail, 1);
	enable_err = bt_enable(NULL);
	settle();
	zassert_ok(disable_err);
	zassert_ok(enable_err);
	zassert_is_null(bt_pub_key_get(), "Public key generated");

	/* The first caller requests a new key and waits for it, and the delivery of the key is
	 * held after its publication. Nothing is asserted before the releases, which a failed
	 * assertion would skip.
	 */
	(void)atomic_set(&keygen.hold, 1);
	start_err[0] = oob_start(0);
	entered_err[0] = k_sem_take(&keygen.entered, ECC_TIMEOUT);
	gen_err = bt_pub_key_gen(&cb);
	keygen_release();
	entered_err[1] = k_sem_take(&delivery.entered, ECC_TIMEOUT);

	/* The second caller asks while the delivery is held */
	start_err[1] = oob_start(1);
	run_other_threads();
	k_sem_give(&delivery.release);

	for (size_t i = 0; i < OOB_THREADS; i++) {
		join_err[i] = k_thread_join(&oob_threads[i], ECC_TIMEOUT);
	}

	settle();

	zassert_ok(entered_err[0], "No key generation");
	zassert_ok(gen_err);
	zassert_ok(entered_err[1], "Public key not delivered");

	for (size_t i = 0; i < OOB_THREADS; i++) {
		zassert_ok(start_err[i], "OOB thread %zu still running", i);
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
