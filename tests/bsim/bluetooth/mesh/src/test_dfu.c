/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "settings_test_backend.h"
#include "mesh/dfd_srv_internal.h"
#include "mesh/dfu_slot.h"
#include "mesh/adv.h"
#include "mesh/dfu.h"
#include "mesh/blob.h"
#include "argparse.h"
#include "dfu_blob_common.h"

#define LOG_MODULE_NAME test_dfu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 420 /* seconds */
#define DFU_TIMEOUT 400 /* seconds */
#define DIST_ADDR 0x0001
#define TARGET_ADDR 0x0100
#define IMPOSTER_MODEL_ID 0xe000
#define TEST_BLOB_ID 0xaabbccdd

struct bind_params {
	uint16_t model_id;
	uint16_t addr;
};

static uint8_t dev_key[16] = { 0xdd };

static struct k_sem dfu_dist_ended;
static struct k_sem dfu_started;
static struct k_sem dfu_verifying;
static struct k_sem dfu_verify_failed;
static struct k_sem dfu_applying;
static struct k_sem dfu_ended;

static struct bt_mesh_prov prov;

static enum bt_mesh_dfu_effect dfu_target_effect;
static uint32_t target_fw_ver_curr = 0xDEADBEEF;
static uint32_t target_fw_ver_new;
static struct bt_mesh_dfu_img dfu_imgs[] = { {
	.fwid = &target_fw_ver_curr,
	.fwid_len = sizeof(target_fw_ver_curr),
} };

static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;

static int dfu_targets_cnt;
static bool dfu_fail_confirm;
static bool recover;
static bool expect_fail;
static enum bt_mesh_dfu_phase expected_stop_phase;

static void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &dfu_targets_cnt,
			.type = 'i',
			.name = "{targets}",
			.option = "targets",
			.descript = "Number of targets to upgrade"
		},
		{
			.dest = &dfu_fail_confirm,
			.type = 'b',
			.name = "{0, 1}",
			.option = "fail-confirm",
			.descript = "Request target to fail confirm step"
		},
		{
			.dest = &expected_stop_phase,
			.type = 'i',
			.name = "{none, start, verify, verify-ok, verify-fail, apply}",
			.option = "expected-phase",
			.descript = "Expected DFU Server phase value restored from flash"
		},
		{
			.dest = &recover,
			.type = 'b',
			.name = "{0, 1}",
			.option = "recover",
			.descript = "Recover DFU server phase"
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);
}

static int dummy_blob_chunk_wr(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	return 0;
}

static int dummy_blob_chunk_rd(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	memset(chunk->data, 0, chunk->size);

	return 0;
}

static const struct bt_mesh_blob_io dummy_blob_io = {
	.rd = dummy_blob_chunk_rd,
	.wr = dummy_blob_chunk_wr,
};

static int dist_fw_recv(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	*io = &dummy_blob_io;

	return 0;
}

static void dist_fw_del(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot)
{
}

static int dist_fw_send(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	*io = &dummy_blob_io;

	return 0;
}

static void dist_phase_changed(struct bt_mesh_dfd_srv *srv, enum bt_mesh_dfd_phase phase)
{
	static enum bt_mesh_dfd_phase prev_phase;

	if (phase == BT_MESH_DFD_PHASE_COMPLETED ||
	    phase == BT_MESH_DFD_PHASE_FAILED) {
		if (phase == BT_MESH_DFD_PHASE_FAILED) {
			ASSERT_EQUAL(BT_MESH_DFD_PHASE_APPLYING_UPDATE, prev_phase);
		}

		k_sem_give(&dfu_dist_ended);
	}

	prev_phase = phase;
}

static struct bt_mesh_dfd_srv_cb dfd_srv_cb = {
	.recv = dist_fw_recv,
	.del = dist_fw_del,
	.send = dist_fw_send,
	.phase = dist_phase_changed,
};

struct bt_mesh_dfd_srv dfd_srv = BT_MESH_DFD_SRV_INIT(&dfd_srv_cb);

static struct k_sem dfu_metadata_check_sem;
static bool dfu_metadata_fail = true;

static int target_metadata_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct net_buf_simple *metadata_raw,
			  enum bt_mesh_dfu_effect *effect)
{
	*effect = dfu_target_effect;

	memcpy(&target_fw_ver_new, net_buf_simple_pull_mem(metadata_raw, sizeof(target_fw_ver_new)),
	       sizeof(target_fw_ver_new));

	k_sem_give(&dfu_metadata_check_sem);

	return dfu_metadata_fail ? 0 : -1;
}

static bool expect_dfu_start = true;

static int target_dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct net_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	ASSERT_TRUE(expect_dfu_start);

	*io = &dummy_blob_io;

	if (expected_stop_phase == BT_MESH_DFU_PHASE_APPLYING) {
		return -EALREADY;
	}

	return 0;
}

static struct k_sem dfu_verify_sem;
static bool dfu_verify_fail;
static bool expect_dfu_xfer_end = true;

static void target_dfu_transfer_end(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img,
				    bool success)
{
	ASSERT_TRUE(expect_dfu_xfer_end);
	ASSERT_TRUE(success);

	if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY) {
		k_sem_give(&dfu_verifying);
		return;
	}

	if (dfu_verify_fail) {
		bt_mesh_dfu_srv_rejected(srv);
		if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_FAIL) {
			k_sem_give(&dfu_verify_failed);
			return;
		}
	} else {
		bt_mesh_dfu_srv_verified(srv);
	}

	k_sem_give(&dfu_verify_sem);
}

static int target_dfu_recover(struct bt_mesh_dfu_srv *srv,
		       const struct bt_mesh_dfu_img *img,
		       const struct bt_mesh_blob_io **io)
{
	if (!recover) {
		FAIL("Not supported");
	}

	*io = &dummy_blob_io;

	return 0;
}

static bool expect_dfu_apply = true;

static int target_dfu_apply(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img)
{
	if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_OK) {
		k_sem_give(&dfu_verifying);
	} else if (expected_stop_phase == BT_MESH_DFU_PHASE_APPLYING) {
		k_sem_give(&dfu_applying);
		return 0;
	}

	ASSERT_TRUE(expect_dfu_apply);

	bt_mesh_dfu_srv_applied(srv);

	k_sem_give(&dfu_ended);

	if (dfu_fail_confirm) {
		/* To fail the confirm step, don't change fw version for devices that should boot
		 * up provisioned. Change fw version for devices that should boot up unprovisioned.
		 */
		if (dfu_target_effect == BT_MESH_DFU_EFFECT_UNPROV) {
			target_fw_ver_curr = target_fw_ver_new;
		}
	} else {
		if (dfu_target_effect == BT_MESH_DFU_EFFECT_UNPROV) {
			bt_mesh_reset();
		}

		target_fw_ver_curr = target_fw_ver_new;
	}

	return 0;
}

static const struct bt_mesh_dfu_srv_cb dfu_handlers = {
	.check = target_metadata_check,
	.start = target_dfu_start,
	.end = target_dfu_transfer_end,
	.apply = target_dfu_apply,
	.recover = target_dfu_recover,
};

static struct bt_mesh_dfu_srv dfu_srv = BT_MESH_DFU_SRV_INIT(&dfu_handlers, dfu_imgs,
							     ARRAY_SIZE(dfu_imgs));

static const struct bt_mesh_comp dist_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_DFD_SRV(&dfd_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const struct bt_mesh_comp dist_comp_self_update = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_DFD_SRV(&dfd_srv)),
				     BT_MESH_MODEL_NONE),
			BT_MESH_ELEM(2,
				     MODEL_LIST(BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 2,
};

static const struct bt_mesh_model_op model_dummy_op[] = {
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp target_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						/* Imposter model without custom handlers is used
						 * so device testing persistent storage can be
						 * configured using both `target_comp` and
						 * `srv_caps_broken_comp`. If these compositions
						 * have different model count and order
						 * loading settings will fail.
						 */
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_dummy_op, NULL, NULL, NULL),
						BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static void provision(uint16_t addr)
{
	int err;

	err = bt_mesh_provision(test_net_key, 0, 0, 0, addr, dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}
}

static void common_configure(uint16_t addr)
{
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, test_app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}
}

static void common_app_bind(uint16_t addr, struct bind_params *params, size_t num)
{
	uint8_t status;
	int err;

	for (size_t i = 0; i < num; i++) {
		err = bt_mesh_cfg_cli_mod_app_bind(0, addr, params[i].addr, 0, params[i].model_id,
						   &status);
		if (err || status) {
			FAIL("Model %#4x bind failed (err %d, status %u)", params[i].model_id,
			     err, status);
			return;
		}
	}
}

static void dist_prov_and_conf(uint16_t addr)
{
	provision(addr);
	common_configure(addr);

	struct bind_params bind_params[] = {
		{ BT_MESH_MODEL_ID_BLOB_CLI, addr },
		{ BT_MESH_MODEL_ID_DFU_CLI, addr },
	};

	common_app_bind(addr, &bind_params[0], ARRAY_SIZE(bind_params));
	common_sar_conf(addr);
}

static void dist_self_update_prov_and_conf(uint16_t addr)
{
	provision(addr);
	common_configure(addr);

	struct bind_params bind_params[] = {
		{ BT_MESH_MODEL_ID_BLOB_CLI, addr },
		{ BT_MESH_MODEL_ID_DFU_CLI, addr },
		{ BT_MESH_MODEL_ID_BLOB_SRV, addr + 1 },
		{ BT_MESH_MODEL_ID_DFU_SRV, addr + 1 },
	};

	common_app_bind(addr, &bind_params[0], ARRAY_SIZE(bind_params));
	common_sar_conf(addr);
}

static void target_prov_and_conf(uint16_t addr, struct bind_params *params, size_t len)
{
	settings_test_backend_clear();
	provision(addr);
	common_configure(addr);

	common_app_bind(addr, params, len);
	common_sar_conf(addr);
}

static void target_prov_and_conf_default(void)
{
	uint16_t addr = bt_mesh_test_own_addr_get(TARGET_ADDR);
	struct bind_params bind_params[] = {
		{ BT_MESH_MODEL_ID_BLOB_SRV, addr },
		{ BT_MESH_MODEL_ID_DFU_SRV, addr },
	};

	target_prov_and_conf(addr, bind_params, ARRAY_SIZE(bind_params));
}

static bool slot_add(const struct bt_mesh_dfu_slot **slot)
{
	const struct bt_mesh_dfu_slot *new_slot;
	size_t size = 100;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = { 0xAA, 0xBB, 0xCC, 0xDD };
	size_t fwid_len = 4;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN] = { 0xAA, 0xBB, 0xCC, 0xDD };
	size_t metadata_len = 4;
	const char *uri = "";

	ASSERT_EQUAL(sizeof(target_fw_ver_new), fwid_len);

	new_slot = bt_mesh_dfu_slot_add(size, fwid, fwid_len, metadata, metadata_len, uri,
					strlen(uri));
	if (!new_slot) {
		return false;
	}

	bt_mesh_dfu_slot_valid_set(new_slot, true);

	if (slot) {
		*slot = new_slot;
	}

	return true;
}

static void dist_dfu_start_and_confirm(void)
{
	enum bt_mesh_dfd_status status;
	struct bt_mesh_dfd_start_params start_params = {
		.app_idx = 0,
		.timeout_base = 10,
		.slot_idx = 0,
		.group = 0,
		.xfer_mode = BT_MESH_BLOB_XFER_MODE_PUSH,
		.ttl = 2,
		.apply = true,
	};

	status = bt_mesh_dfd_srv_start(&dfd_srv, &start_params);
	ASSERT_EQUAL(BT_MESH_DFD_SUCCESS, status);

	if (k_sem_take(&dfu_dist_ended, K_SECONDS(DFU_TIMEOUT))) {
		FAIL("DFU timed out");
	}

	enum bt_mesh_dfu_status expected_status;
	enum bt_mesh_dfu_phase expected_phase;

	if (dfu_fail_confirm) {
		ASSERT_EQUAL(BT_MESH_DFD_PHASE_FAILED, dfd_srv.phase);
		expected_status = BT_MESH_DFU_ERR_INTERNAL;
		expected_phase = BT_MESH_DFU_PHASE_APPLY_FAIL;
	} else {
		ASSERT_EQUAL(BT_MESH_DFD_PHASE_COMPLETED, dfd_srv.phase);
		expected_status = BT_MESH_DFU_SUCCESS;
		expected_phase = BT_MESH_DFU_PHASE_APPLY_SUCCESS;
	}

	for (int i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(expected_status, dfd_srv.targets[i].status);

		if (dfd_srv.targets[i].effect == BT_MESH_DFU_EFFECT_UNPROV) {
			/* If device should unprovision itself after the update, the phase won't
			 * change. If phase changes, DFU failed.
			 */
			if  (dfu_fail_confirm) {
				ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLY_FAIL,
					     dfd_srv.targets[i].phase);
			} else {
				ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLYING, dfd_srv.targets[i].phase);
			}
		} else {
			ASSERT_EQUAL(expected_phase, dfd_srv.targets[i].phase);
		}
	}
}

static void test_dist_dfu(void)
{
	enum bt_mesh_dfd_status status;

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);
	dist_prov_and_conf(DIST_ADDR);

	ASSERT_TRUE(slot_add(NULL));

	ASSERT_TRUE(dfu_targets_cnt > 0);

	for (int i = 0; i < dfu_targets_cnt; i++) {
		status = bt_mesh_dfd_srv_receiver_add(&dfd_srv, TARGET_ADDR + 1 + i, 0);
		ASSERT_EQUAL(BT_MESH_DFD_SUCCESS, status);
	}

	dist_dfu_start_and_confirm();

	PASS();
}

static void test_dist_dfu_self_update(void)
{
	enum bt_mesh_dfd_status status;

	ASSERT_TRUE(dfu_targets_cnt > 0);

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp_self_update);
	dist_self_update_prov_and_conf(DIST_ADDR);

	ASSERT_TRUE(slot_add(NULL));

	status = bt_mesh_dfd_srv_receiver_add(&dfd_srv, DIST_ADDR + 1, 0);
	ASSERT_EQUAL(BT_MESH_DFD_SUCCESS, status);
	dfu_target_effect = BT_MESH_DFU_EFFECT_NONE;

	for (int i = 1; i < dfu_targets_cnt; i++) {
		status = bt_mesh_dfd_srv_receiver_add(&dfd_srv, TARGET_ADDR + i, 0);
		ASSERT_EQUAL(BT_MESH_DFD_SUCCESS, status);
	}

	dist_dfu_start_and_confirm();

	/* Check that DFU finished on distributor. */
	if (k_sem_take(&dfu_ended, K_SECONDS(DFU_TIMEOUT))) {
		FAIL("firmware was not applied");
	}

	PASS();
}

static void test_dist_dfu_slot_create(void)
{
	const struct bt_mesh_dfu_slot *slot[3];
	size_t size = 100;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = { 0 };
	size_t fwid_len = 4;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN] = { 0 };
	size_t metadata_len = 4;
	const char *uri = "test";
	int err, i;

	ASSERT_TRUE(CONFIG_BT_MESH_DFU_SLOT_CNT >= 3,
		    "CONFIG_BT_MESH_DFU_SLOT_CNT must be at least 3");

	settings_test_backend_clear();

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);
	dist_prov_and_conf(DIST_ADDR);

	for (i = 0; i < CONFIG_BT_MESH_DFU_SLOT_CNT; i++) {
		fwid[0] = i;
		metadata[0] = i;
		slot[i] = bt_mesh_dfu_slot_add(size, fwid, fwid_len, metadata, metadata_len, uri,
					       strlen(uri));

		ASSERT_FALSE(slot[i] == NULL, "Failed to add slot");
	}

	/* First slot is set as valid */
	err = bt_mesh_dfu_slot_valid_set(slot[0], true);
	if (err) {
		FAIL("Setting slot to valid state failed (err %d)", err);
		return;
	}
	ASSERT_TRUE(bt_mesh_dfu_slot_is_valid(slot[0]));

	/* Second slot is set as invalid */
	err = bt_mesh_dfu_slot_valid_set(slot[1], false);
	if (err) {
		FAIL("Setting slot to invalid state failed (err %d)", err);
		return;
	}
	ASSERT_TRUE(!bt_mesh_dfu_slot_is_valid(slot[1]));

	/* Last slot is deleted */
	err = bt_mesh_dfu_slot_del(slot[CONFIG_BT_MESH_DFU_SLOT_CNT - 1]);
	if (err) {
		FAIL("Slot delete failed (err %d)", err);
		return;
	}

	PASS();
}

enum bt_mesh_dfu_iter check_slot(const struct bt_mesh_dfu_slot *slot, void *data)
{
	size_t size = 100;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = { 0 };
	size_t fwid_len = 4;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN] = { 0 };
	size_t metadata_len = 4;
	const char *uri = "test";
	int idx = bt_mesh_dfu_slot_idx_get(slot);

	ASSERT_TRUE(idx >= 0, "Failed to retrieve slot index");

	ASSERT_EQUAL(size, slot->size);
	ASSERT_TRUE(strcmp(uri, slot->uri) == 0);

	fwid[0] = idx;

	ASSERT_EQUAL(fwid_len, slot->fwid_len);
	ASSERT_TRUE(memcmp(fwid, slot->fwid, fwid_len) == 0);

	metadata[0] = idx;
	ASSERT_EQUAL(metadata_len, slot->metadata_len);
	ASSERT_TRUE(memcmp(metadata, slot->metadata, metadata_len) == 0);

	return BT_MESH_DFU_ITER_CONTINUE;
}

static void test_dist_dfu_slot_create_recover(void)
{
	size_t slot_count;
	const struct bt_mesh_dfu_slot *slot;
	size_t size = 100;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = { 0 };
	size_t fwid_len = 4;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN] = { 0 };
	size_t metadata_len = 4;
	const char *uri = "test";
	int i, idx;

	ASSERT_TRUE(CONFIG_BT_MESH_DFU_SLOT_CNT >= 3,
		    "CONFIG_BT_MESH_DFU_SLOT_CNT must be at least 3");

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);

	slot_count = bt_mesh_dfu_slot_foreach(check_slot, NULL);
	ASSERT_EQUAL(CONFIG_BT_MESH_DFU_SLOT_CNT - 1, slot_count);

	slot = bt_mesh_dfu_slot_at(0);
	ASSERT_EQUAL(true, bt_mesh_dfu_slot_is_valid(slot));

	slot = bt_mesh_dfu_slot_at(1);
	ASSERT_TRUE(slot != NULL);
	ASSERT_EQUAL(false, bt_mesh_dfu_slot_is_valid(slot));

	for (i = 0; i < (CONFIG_BT_MESH_DFU_SLOT_CNT - 1); i++) {
		fwid[0] = i;
		idx = bt_mesh_dfu_slot_get(fwid, fwid_len, &slot);
		ASSERT_TRUE(idx >= 0);
		ASSERT_EQUAL(idx, bt_mesh_dfu_slot_idx_get(slot));

		ASSERT_EQUAL(size, slot->size);
		ASSERT_TRUE(strcmp(uri, slot->uri) == 0);

		metadata[0] = idx;
		ASSERT_EQUAL(metadata_len, slot->metadata_len);
		ASSERT_TRUE(memcmp(metadata, slot->metadata, metadata_len) == 0);
	}

	PASS();
}

static void check_delete_all(void)
{
	int i, idx, err;
	const struct bt_mesh_dfu_slot *slot;
	size_t slot_count;

	ASSERT_TRUE(CONFIG_BT_MESH_DFU_SLOT_CNT >= 3,
		    "CONFIG_BT_MESH_DFU_SLOT_CNT must be at least 3");

	slot_count = bt_mesh_dfu_slot_foreach(NULL, NULL);
	ASSERT_EQUAL(0, slot_count);

	for (i = 0; i < CONFIG_BT_MESH_DFU_SLOT_CNT - 1; i++) {
		slot = bt_mesh_dfu_slot_at(i);
		ASSERT_TRUE(slot == NULL);

		idx = bt_mesh_dfu_slot_idx_get(slot);
		ASSERT_TRUE(idx < 0);

		err = bt_mesh_dfu_slot_valid_set(slot, true);
		ASSERT_EQUAL(err, -ENOENT);

		ASSERT_TRUE(!bt_mesh_dfu_slot_is_valid(slot));
	}
}

static void test_dist_dfu_slot_delete_all(void)
{
	ASSERT_TRUE(CONFIG_BT_MESH_DFU_SLOT_CNT >= 3,
		    "CONFIG_BT_MESH_DFU_SLOT_CNT must be at least 3");

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);

	bt_mesh_dfu_slot_del_all();

	check_delete_all();

	PASS();
}

static void test_dist_dfu_slot_check_delete_all(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);

	check_delete_all();

	PASS();
}

static void target_test_effect(enum bt_mesh_dfu_effect effect)
{
	dfu_target_effect = effect;

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &target_comp);
	target_prov_and_conf_default();

	if (k_sem_take(&dfu_ended, K_SECONDS(DFU_TIMEOUT))) {
		FAIL("Firmware was not applied");
	}
}

static void test_target_dfu_no_change(void)
{
	target_test_effect(BT_MESH_DFU_EFFECT_NONE);

	PASS();
}

static void test_target_dfu_new_comp_no_rpr(void)
{
	target_test_effect(BT_MESH_DFU_EFFECT_COMP_CHANGE_NO_RPR);

	PASS();
}

static void test_target_dfu_new_comp_rpr(void)
{
	target_test_effect(BT_MESH_DFU_EFFECT_COMP_CHANGE);

	PASS();
}

static void test_target_dfu_unprov(void)
{
	target_test_effect(BT_MESH_DFU_EFFECT_UNPROV);

	PASS();
}

static struct {
	struct bt_mesh_blob_cli_inputs inputs;
	struct bt_mesh_blob_target_pull pull[7];
	struct bt_mesh_dfu_target targets[7];
	uint8_t target_count;
	struct bt_mesh_dfu_cli_xfer xfer;
} dfu_cli_xfer;

static void dfu_cli_inputs_prepare(uint16_t group)
{
	dfu_cli_xfer.inputs.ttl = BT_MESH_TTL_DEFAULT;
	dfu_cli_xfer.inputs.group = group;
	dfu_cli_xfer.inputs.app_idx = 0;
	dfu_cli_xfer.inputs.timeout_base = 1;
	sys_slist_init(&dfu_cli_xfer.inputs.targets);

	for (int i = 0; i < dfu_cli_xfer.target_count; ++i) {
		/* Reset target context. */
		uint16_t addr = dfu_cli_xfer.targets[i].blob.addr;

		memset(&dfu_cli_xfer.targets[i], 0, sizeof(struct bt_mesh_dfu_target));
		dfu_cli_xfer.targets[i].blob.addr = addr;
		if (recover) {
			memset(&dfu_cli_xfer.pull[i].missing, 1,
			       DIV_ROUND_UP(CONFIG_BT_MESH_BLOB_CHUNK_COUNT_MAX, 8));
			dfu_cli_xfer.targets[i].blob.pull = &dfu_cli_xfer.pull[i];
		}

		sys_slist_append(&dfu_cli_xfer.inputs.targets, &dfu_cli_xfer.targets[i].blob.n);
	}
}

static struct bt_mesh_blob_target *target_srv_add(uint16_t addr, bool expect_lost)
{
	if (expect_lost) {
		lost_target_add(addr);
	}

	ASSERT_TRUE(dfu_cli_xfer.target_count < ARRAY_SIZE(dfu_cli_xfer.targets));
	struct bt_mesh_blob_target *t = &dfu_cli_xfer.targets[dfu_cli_xfer.target_count].blob;

	t->addr = addr;
	dfu_cli_xfer.target_count++;
	return t;
}

static void dfu_cli_suspended(struct bt_mesh_dfu_cli *cli)
{
	FAIL("Unexpected call");
}

static void dfu_cli_ended(struct bt_mesh_dfu_cli *cli, enum bt_mesh_dfu_status reason)
{
	if ((expected_stop_phase == BT_MESH_DFU_PHASE_IDLE ||
	     expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_OK) &&
	    !expect_fail) {
		ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, reason);
	}

	if (expected_stop_phase == BT_MESH_DFU_PHASE_TRANSFER_ACTIVE) {
		k_sem_give(&dfu_started);
	} else if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY) {
		k_sem_give(&dfu_verifying);
	} else if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_FAIL) {
		k_sem_give(&dfu_verify_failed);
	}

	k_sem_give(&dfu_ended);
}

static struct k_sem dfu_cli_applied_sem;

static void dfu_cli_applied(struct bt_mesh_dfu_cli *cli)
{
	k_sem_give(&dfu_cli_applied_sem);
}

static struct k_sem dfu_cli_confirmed_sem;

static void dfu_cli_confirmed(struct bt_mesh_dfu_cli *cli)
{
	k_sem_give(&dfu_cli_confirmed_sem);
}

static struct k_sem lost_target_sem;

static void dfu_cli_lost_target(struct bt_mesh_dfu_cli *cli, struct bt_mesh_dfu_target *target)
{
	ASSERT_FALSE(target->status == BT_MESH_DFU_SUCCESS);
	ASSERT_TRUE(lost_target_find_and_remove(target->blob.addr));

	if (!lost_targets_rem()) {
		k_sem_give(&lost_target_sem);
	}
}

static struct bt_mesh_dfu_cli_cb dfu_cli_cb = {
	.suspended = dfu_cli_suspended,
	.ended = dfu_cli_ended,
	.applied = dfu_cli_applied,
	.confirmed = dfu_cli_confirmed,
	.lost_target = dfu_cli_lost_target,
};

static struct bt_mesh_dfu_cli dfu_cli = BT_MESH_DFU_CLI_INIT(&dfu_cli_cb);

static const struct bt_mesh_comp cli_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_DFU_CLI(&dfu_cli)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static void cli_common_fail_on_init(void)
{
	const struct bt_mesh_dfu_slot *slot;

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, 300);
	bt_mesh_device_setup(&prov, &cli_comp);
	dist_prov_and_conf(DIST_ADDR);

	ASSERT_TRUE(slot_add(&slot));

	dfu_cli_inputs_prepare(0);
	dfu_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	dfu_cli_xfer.xfer.slot = slot;
	dfu_cli_xfer.xfer.blob_id = TEST_BLOB_ID;
}

static void cli_common_init_recover(void)
{
	const struct bt_mesh_dfu_slot *slot;

	bt_mesh_test_cfg_set(NULL, 300);
	bt_mesh_device_setup(&prov, &cli_comp);

	ASSERT_TRUE(slot_add(&slot));

	dfu_cli_inputs_prepare(0);
	dfu_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	dfu_cli_xfer.xfer.slot = slot;
	dfu_cli_xfer.xfer.blob_id = TEST_BLOB_ID;
}

static void test_cli_fail_on_persistency(void)
{
	int err;

	/** Test that DFU transfer persists as long as at least one target is still active. During
	 * the test multiple servers will become unresponsive at different phases of the transfer:
	 * - Srv 0x0002 will reject firmware by metadata.
	 * - Srv 0x0003 will not respond to BLOB Information Get msg (Retrieve Caps proc).
	 * - Srv 0x0004 will not respond to Firmware Update Get msg after BLOB Transfer.
	 * - Srv 0x0005 will fail firmware verification.
	 * - Srv 0x0006 will not respond to Firmware Update Apply msg.
	 * - Srv 0x0007 is responsive all the way.
	 * - Srv 0x0008 is a non-existing unresponsive node that will not respond to Firmware
	 *   Update Start msg, which is the first message sent by DFU Client.
	 */
	(void)target_srv_add(TARGET_ADDR + 1, true);
	(void)target_srv_add(TARGET_ADDR + 2, true);
	(void)target_srv_add(TARGET_ADDR + 3, true);
	(void)target_srv_add(TARGET_ADDR + 4, true);
	(void)target_srv_add(TARGET_ADDR + 5, true);
	(void)target_srv_add(TARGET_ADDR + 6, false);
	(void)target_srv_add(TARGET_ADDR + 7, true);

	cli_common_fail_on_init();

	err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
				   &dfu_cli_xfer.xfer);
	if (err) {
		FAIL("DFU Client send failed (err: %d)", err);
	}

	if (k_sem_take(&dfu_ended, K_SECONDS(200))) {
		FAIL("Firmware transfer failed");
	}

	/* This is non-existing unresponsive target that didn't reply on Firmware Update Start
	 * message.
	 */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[6].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_UNKNOWN, dfu_cli_xfer.targets[6].phase);
	/* This target rejected metadata. */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_METADATA, dfu_cli_xfer.targets[0].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_IDLE, dfu_cli_xfer.targets[0].phase);
	/* This target shouldn't respond on BLOB Information Get message from Retrieve Caps
	 * procedure.
	 */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[1].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ACTIVE, dfu_cli_xfer.targets[1].phase);
	/* This target shouldn't respond on Firmware Update Get msg. */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[2].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ACTIVE, dfu_cli_xfer.targets[2].phase);
	/* This target failed firmware verification. */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_WRONG_PHASE, dfu_cli_xfer.targets[3].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_FAIL, dfu_cli_xfer.targets[3].phase);
	/* The next two targets should be OK. */
	ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, dfu_cli_xfer.targets[4].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_cli_xfer.targets[4].phase);
	ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, dfu_cli_xfer.targets[5].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_cli_xfer.targets[5].phase);

	err = bt_mesh_dfu_cli_apply(&dfu_cli);
	if (err) {
		FAIL("DFU Client apply failed (err: %d)", err);
	}

	if (k_sem_take(&dfu_cli_applied_sem, K_SECONDS(200))) {
		FAIL("Failed to apply firmware");
	}

	/* This target shouldn't respond on Firmware Update Apply message. */
	ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[4].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_cli_xfer.targets[4].phase);

	err = bt_mesh_dfu_cli_confirm(&dfu_cli);
	if (err) {
		FAIL("DFU Client confirm failed (err: %d)", err);
	}

	if (k_sem_take(&dfu_cli_confirmed_sem, K_SECONDS(200))) {
		FAIL("Failed to confirm firmware");
	}

	/* This target should complete DFU successfully. */
	ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, dfu_cli_xfer.targets[5].status);
	ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLY_SUCCESS, dfu_cli_xfer.targets[5].phase);

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expected lost targets");
	}

	PASS();
}

static void test_cli_all_targets_lost_common(void)
{
	int err, i;

	expect_fail = true;

	for (i = 1; i <= dfu_targets_cnt; i++) {
		(void)target_srv_add(TARGET_ADDR + i, true);
	}

	cli_common_fail_on_init();

	err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
				   &dfu_cli_xfer.xfer);
	if (err) {
		FAIL("DFU Client send failed (err: %d)", err);
	}

	if (k_sem_take(&dfu_ended, K_SECONDS(200))) {
		FAIL("Firmware transfer failed");
	}
}

static void test_cli_all_targets_lost_on_metadata(void)
{
	int i;

	test_cli_all_targets_lost_common();

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_ERR_METADATA, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_IDLE, dfu_cli_xfer.targets[i].phase);
	}

	/* `lost_target` cb must be called on all targets */
	ASSERT_EQUAL(0, lost_targets_rem());

	PASS();
}

static void test_cli_all_targets_lost_on_caps_get(void)
{
	int i;

	test_cli_all_targets_lost_common();

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ACTIVE,
			     dfu_cli_xfer.targets[i].phase);
	}

	/* `lost_target` cb must be called on all targets */
	ASSERT_EQUAL(0, lost_targets_rem());

	PASS();
}

static void test_cli_all_targets_lost_on_update_get(void)
{
	int i;

	test_cli_all_targets_lost_common();

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ACTIVE,
			     dfu_cli_xfer.targets[i].phase);
	}

	/* `lost_target` cb must be called on all targets */
	ASSERT_EQUAL(0, lost_targets_rem());

	PASS();
}

static void test_cli_all_targets_lost_on_verify(void)
{
	int i;

	test_cli_all_targets_lost_common();

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_ERR_WRONG_PHASE, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_FAIL, dfu_cli_xfer.targets[i].phase);
	}

	/* `lost_target` cb must be called on all targets */
	ASSERT_EQUAL(0, lost_targets_rem());

	PASS();
}

static void test_cli_all_targets_lost_on_apply(void)
{
	int err, i;

	test_cli_all_targets_lost_common();

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_cli_xfer.targets[i].phase);
	}

	err = bt_mesh_dfu_cli_apply(&dfu_cli);
	if (err) {
		FAIL("DFU Client apply failed (err: %d)", err);
	}

	if (!k_sem_take(&dfu_cli_applied_sem, K_SECONDS(200))) {
		FAIL("Apply should not be successful on any target");
	}

	for (i = 0; i < dfu_targets_cnt; i++) {
		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[i].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_cli_xfer.targets[i].phase);
	}

	/* `lost_target` cb must be called on all targets */
	ASSERT_EQUAL(0, lost_targets_rem());

	PASS();
}

static void test_cli_stop(void)
{
	int err;

	(void)target_srv_add(TARGET_ADDR + 1, true);

	switch (expected_stop_phase) {
	case BT_MESH_DFU_PHASE_TRANSFER_ACTIVE:
		cli_common_fail_on_init();

		err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
					   &dfu_cli_xfer.xfer);
		if (err) {
			FAIL("DFU Client send failed (err: %d)", err);
		}

		if (k_sem_take(&dfu_started, K_SECONDS(200))) {
			FAIL("Firmware transfer failed");
		}

		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[0].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ACTIVE, dfu_cli_xfer.targets[0].phase);
		break;
	case BT_MESH_DFU_PHASE_VERIFY:
		cli_common_init_recover();

		err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
					   &dfu_cli_xfer.xfer);
		if (err) {
			FAIL("DFU Client resume failed (err: %d)", err);
		}

		if (k_sem_take(&dfu_verifying, K_SECONDS(200))) {
			FAIL("Firmware transfer failed");
		}
		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[0].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY, dfu_cli_xfer.targets[0].phase);

		break;
	case BT_MESH_DFU_PHASE_VERIFY_OK:
		/* Nothing to do here on distributor side, target must verify image */
		break;
	case BT_MESH_DFU_PHASE_VERIFY_FAIL:
		cli_common_fail_on_init();

		err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
					   &dfu_cli_xfer.xfer);
		if (err) {
			FAIL("DFU Client send failed (err: %d)", err);
		}

		if (k_sem_take(&dfu_verify_failed, K_SECONDS(200))) {
			FAIL("Firmware transfer failed");
		}

		ASSERT_EQUAL(BT_MESH_DFU_ERR_WRONG_PHASE, dfu_cli_xfer.targets[0].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_FAIL, dfu_cli_xfer.targets[0].phase);
		break;
	case BT_MESH_DFU_PHASE_APPLYING:
		cli_common_init_recover();

		err = bt_mesh_dfu_cli_send(&dfu_cli, &dfu_cli_xfer.inputs, &dummy_blob_io,
					   &dfu_cli_xfer.xfer);

		if (err) {
			FAIL("DFU Client send failed (err: %d)", err);
		}
		if (k_sem_take(&dfu_ended, K_SECONDS(200))) {
			FAIL("Firmware transfer failed");
		}

		bt_mesh_dfu_cli_apply(&dfu_cli);
		if (k_sem_take(&dfu_cli_applied_sem, K_SECONDS(200))) {
			/* This will time out as target will reboot before applying */
		}
		ASSERT_EQUAL(BT_MESH_DFU_ERR_INTERNAL, dfu_cli_xfer.targets[0].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLYING, dfu_cli_xfer.targets[0].phase);
		break;
	case BT_MESH_DFU_PHASE_APPLY_SUCCESS:
		cli_common_init_recover();

		dfu_cli.xfer.state = 5;
		dfu_cli.xfer.slot = dfu_cli_xfer.xfer.slot;
		dfu_cli.xfer.blob.id = TEST_BLOB_ID;
		dfu_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;

		dfu_cli.blob.inputs = &dfu_cli_xfer.inputs;

		err = bt_mesh_dfu_cli_confirm(&dfu_cli);
		if (err) {
			FAIL("DFU Client confirm failed (err: %d)", err);
		}

		ASSERT_EQUAL(BT_MESH_DFU_SUCCESS, dfu_cli_xfer.targets[0].status);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_IDLE, dfu_cli_xfer.targets[0].phase);

		PASS();
		break;
	default:
		break;
	}
	PASS();
}

static struct k_sem caps_get_sem;

static int mock_handle_caps_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	LOG_WRN("Rejecting BLOB Information Get message");

	k_sem_give(&caps_get_sem);

	return 0;
}

static const struct bt_mesh_model_op model_caps_op1[] = {
	{ BT_MESH_BLOB_OP_INFO_GET, 0, mock_handle_caps_get },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp srv_caps_broken_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_caps_op1, NULL, NULL, NULL),
						BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static int mock_handle_chunks(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	LOG_WRN("Skipping receiving block");

	k_sem_give(&dfu_started);

	return 0;
}

static const struct bt_mesh_model_op model_caps_op2[] = {
	{ BT_MESH_BLOB_OP_CHUNK, 0, mock_handle_chunks },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp broken_target_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_caps_op2, NULL, NULL, NULL),
						BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static struct k_sem update_get_sem;

static int mock_handle_update_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	LOG_WRN("Rejecting Firmware Update Get message");
	k_sem_give(&update_get_sem);

	return 0;
}

static const struct bt_mesh_model_op model_update_get_op1[] = {
	{ BT_MESH_DFU_OP_UPDATE_GET, 0, mock_handle_update_get },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp srv_update_get_broken_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_update_get_op1, NULL, NULL,
								 NULL),
						BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static struct k_sem update_apply_sem;

static int mock_handle_update_apply(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	LOG_WRN("Rejecting Firmware Update Apply message");
	k_sem_give(&update_apply_sem);

	return 0;
}

static const struct bt_mesh_model_op model_update_apply_op1[] = {
	{ BT_MESH_DFU_OP_UPDATE_APPLY, 0, mock_handle_update_apply },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp srv_update_apply_broken_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_update_apply_op1, NULL,
								 NULL, NULL),
						BT_MESH_MODEL_DFU_SRV(&dfu_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static void target_prov_and_conf_with_imposer(void)
{
	uint16_t addr = bt_mesh_test_own_addr_get(TARGET_ADDR);
	struct bind_params bind_params[] = {
		{ BT_MESH_MODEL_ID_BLOB_SRV, addr },
		{ BT_MESH_MODEL_ID_DFU_SRV, addr },
		{ IMPOSTER_MODEL_ID, addr },
	};

	target_prov_and_conf(addr, bind_params, ARRAY_SIZE(bind_params));
}

static void common_fail_on_target_init(const struct bt_mesh_comp *comp)
{
	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, 300);
	bt_mesh_device_setup(&prov, comp);

	dfu_target_effect = BT_MESH_DFU_EFFECT_NONE;
}

static void test_target_fail_on_metadata(void)
{
	dfu_metadata_fail = false;
	expect_dfu_start = false;

	common_fail_on_target_init(&target_comp);
	target_prov_and_conf_default();

	if (k_sem_take(&dfu_metadata_check_sem, K_SECONDS(200))) {
		FAIL("Metadata check CB wasn't called");
	}

	PASS();
}

static void test_target_fail_on_caps_get(void)
{
	expect_dfu_xfer_end = false;

	common_fail_on_target_init(&srv_caps_broken_comp);
	target_prov_and_conf_with_imposer();

	if (k_sem_take(&caps_get_sem, K_SECONDS(200))) {
		FAIL("BLOB Info Get msg handler wasn't called");
	}

	PASS();
}

static void test_target_fail_on_update_get(void)
{
	expect_dfu_apply = false;

	common_fail_on_target_init(&srv_update_get_broken_comp);
	target_prov_and_conf_with_imposer();

	if (k_sem_take(&dfu_verify_sem, K_SECONDS(200))) {
		FAIL("Transfer end CB wasn't triggered");
	}

	if (k_sem_take(&update_get_sem, K_SECONDS(200))) {
		FAIL("Firmware Update Get msg handler wasn't called");
	}

	PASS();
}

static void test_target_fail_on_verify(void)
{
	dfu_verify_fail = true;
	expect_dfu_apply = false;

	common_fail_on_target_init(&target_comp);
	target_prov_and_conf_default();

	if (k_sem_take(&dfu_verify_sem, K_SECONDS(200))) {
		FAIL("Transfer end CB wasn't triggered");
	}

	PASS();
}

static void test_target_fail_on_apply(void)
{
	expect_dfu_apply = false;

	common_fail_on_target_init(&srv_update_apply_broken_comp);
	target_prov_and_conf_with_imposer();

	if (k_sem_take(&update_apply_sem, K_SECONDS(200))) {
		FAIL("Firmware Update Apply msg handler wasn't called");
	}

	PASS();
}

static void test_target_fail_on_nothing(void)
{
	common_fail_on_target_init(&target_comp);
	target_prov_and_conf_default();

	if (k_sem_take(&dfu_ended, K_SECONDS(200))) {
		FAIL("DFU failed");
	}

	PASS();
}

static void test_target_dfu_stop(void)
{
	dfu_target_effect = BT_MESH_DFU_EFFECT_NONE;

	if (!recover) {
		settings_test_backend_clear();
		bt_mesh_test_cfg_set(NULL, WAIT_TIME);

		common_fail_on_target_init(expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_FAIL ?
					   &target_comp : &broken_target_comp);
		target_prov_and_conf_with_imposer();

		if (expected_stop_phase == BT_MESH_DFU_PHASE_VERIFY_FAIL) {
			dfu_verify_fail = true;
			if (k_sem_take(&dfu_verify_failed, K_SECONDS(DFU_TIMEOUT))) {
				FAIL("Phase not reached");
			}
		} else {
			/* Stop at BT_MESH_DFU_PHASE_TRANSFER_ACTIVE */
			if (k_sem_take(&dfu_started, K_SECONDS(DFU_TIMEOUT))) {
				FAIL("Phase not reached");
			}
		}

		ASSERT_EQUAL(expected_stop_phase, dfu_srv.update.phase);
		PASS();
		return;
	}

	bt_mesh_device_setup(&prov, &target_comp);
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);

	switch (expected_stop_phase) {
	case BT_MESH_DFU_PHASE_VERIFY:
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_TRANSFER_ERR, dfu_srv.update.phase);
		if (k_sem_take(&dfu_verifying, K_SECONDS(DFU_TIMEOUT))) {
			FAIL("Phase not reached");
		}
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY, dfu_srv.update.phase);
		break;
	case BT_MESH_DFU_PHASE_VERIFY_OK:
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY, dfu_srv.update.phase);
		bt_mesh_dfu_srv_verified(&dfu_srv);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_OK, dfu_srv.update.phase);
		break;
	case BT_MESH_DFU_PHASE_APPLYING:
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_VERIFY_FAIL, dfu_srv.update.phase);
		if (k_sem_take(&dfu_applying, K_SECONDS(DFU_TIMEOUT))) {
			FAIL("Phase not reached");
		}
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLYING, dfu_srv.update.phase);
		break;
	case BT_MESH_DFU_PHASE_APPLY_SUCCESS:
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_APPLYING, dfu_srv.update.phase);
		bt_mesh_dfu_srv_applied(&dfu_srv);
		ASSERT_EQUAL(BT_MESH_DFU_PHASE_IDLE, dfu_srv.update.phase);
		break;
	default:
		FAIL("Wrong expected phase");
		break;
	}

	ASSERT_EQUAL(0, dfu_srv.update.idx);
	PASS();
}

static void test_pre_init(void)
{
	k_sem_init(&dfu_dist_ended, 0, 1);
	k_sem_init(&dfu_ended, 0, 1);
	k_sem_init(&caps_get_sem, 0, 1);
	k_sem_init(&update_get_sem, 0, 1);
	k_sem_init(&update_apply_sem, 0, 1);
	k_sem_init(&dfu_metadata_check_sem, 0, 1);
	k_sem_init(&dfu_verify_sem, 0, 1);
	k_sem_init(&dfu_cli_applied_sem, 0, 1);
	k_sem_init(&dfu_cli_confirmed_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&dfu_started, 0, 1);
	k_sem_init(&dfu_verifying, 0, 1);
	k_sem_init(&dfu_verify_failed, 0, 1);
	k_sem_init(&dfu_applying, 0, 1);
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "dfu_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_pre_init_f = test_pre_init,              \
		.test_args_f = test_args_parse,                \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_dfu[] = {
	TEST_CASE(dist, dfu, "Distributor performs DFU"),
	TEST_CASE(dist, dfu_self_update, "Distributor performs DFU with self update"),
	TEST_CASE(dist, dfu_slot_create, "Distributor creates image slots"),
	TEST_CASE(dist, dfu_slot_create_recover,
		      "Distributor recovers created image slots from persitent storage"),
	TEST_CASE(dist, dfu_slot_delete_all, "Distributor deletes all image slots"),
	TEST_CASE(dist, dfu_slot_check_delete_all,
		      "Distributor checks if all slots are removed from persistent storage"),
	TEST_CASE(cli, stop, "DFU Client stops at configured point of Firmware Distribution"),
	TEST_CASE(cli, fail_on_persistency, "DFU Client doesn't give up DFU Transfer"),
	TEST_CASE(cli, all_targets_lost_on_metadata,
		  "All targets fail to check metadata and Client ends DFU Transfer"),
	TEST_CASE(cli, all_targets_lost_on_caps_get,
		  "All targets fail to respond to caps get and Client ends DFU Transfer"),
	TEST_CASE(cli, all_targets_lost_on_update_get,
		  "All targets fail to respond to update get and Client ends DFU Transfer"),
	TEST_CASE(cli, all_targets_lost_on_verify,
		  "All targets fail on verify step and Client ends DFU Transfer"),
	TEST_CASE(cli, all_targets_lost_on_apply,
		  "All targets fail on apply step and Client ends DFU Transfer"),

	TEST_CASE(target, dfu_no_change, "Target node, Comp Data stays unchanged"),
	TEST_CASE(target, dfu_new_comp_no_rpr, "Target node, Comp Data changes, no RPR"),
	TEST_CASE(target, dfu_new_comp_rpr, "Target node, Comp Data changes, has RPR"),
	TEST_CASE(target, dfu_unprov, "Target node, Comp Data changes, unprovisioned"),
	TEST_CASE(target, fail_on_metadata, "Server rejects metadata"),
	TEST_CASE(target, fail_on_caps_get, "Server failing on Retrieve Capabilities procedure"),
	TEST_CASE(target, fail_on_update_get, "Server failing on Fw Update Get msg"),
	TEST_CASE(target, fail_on_verify, "Server rejects fw at Refresh step"),
	TEST_CASE(target, fail_on_apply, "Server failing on Fw Update Apply msg"),
	TEST_CASE(target, fail_on_nothing, "Non-failing server"),
	TEST_CASE(target, dfu_stop, "Server stops FD procedure at configured step"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_dfu_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_dfu);
	return tests;
}
