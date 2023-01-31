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
#include "argparse.h"

#define LOG_MODULE_NAME test_dfu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 360 /* seconds */
#define DFU_TIMEOUT 300 /* seconds */
#define DIST_ADDR 0x0001
#define MODEL_LIST(...) ((struct bt_mesh_model[]){ __VA_ARGS__ })

struct bind_params {
	uint16_t model_id;
	uint16_t addr;
};

static uint8_t dev_key[16] = { 0xdd };

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

static int dfu_targets_cnt;
static bool dfu_fail_confirm;

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

		k_sem_give(&dfu_ended);
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

static int target_metadata_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct net_buf_simple *metadata_raw,
			  enum bt_mesh_dfu_effect *effect)
{
	*effect = dfu_target_effect;

	memcpy(&target_fw_ver_new, net_buf_simple_pull_mem(metadata_raw, sizeof(target_fw_ver_new)),
	       sizeof(target_fw_ver_new));

	return 0;
}

static int target_dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct net_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	*io = &dummy_blob_io;

	return 0;
}

static void target_dfu_transfer_end(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img,
				    bool success)
{
	ASSERT_TRUE(success);

	bt_mesh_dfu_srv_verified(srv);
}

static int target_dfu_recover(struct bt_mesh_dfu_srv *srv,
		       const struct bt_mesh_dfu_img *img,
		       const struct bt_mesh_blob_io **io)
{
	FAIL("Not supported");

	return 0;
}

static int target_dfu_apply(struct bt_mesh_dfu_srv *srv, const struct bt_mesh_dfu_img *img)
{
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
						BT_MESH_MODEL_DFD_SRV(&dfd_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const struct bt_mesh_comp target_comp = {
	.elem =
		(struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
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
}

static void target_prov_and_conf(uint16_t addr)
{
	settings_test_backend_clear();
	provision(addr);
	common_configure(addr);

	struct bind_params bind_params[] = {
		{ BT_MESH_MODEL_ID_BLOB_SRV, addr },
		{ BT_MESH_MODEL_ID_DFU_SRV, addr },
	};

	common_app_bind(addr, &bind_params[0], ARRAY_SIZE(bind_params));
}

static bool slot_add(void)
{
	const struct bt_mesh_dfu_slot *slot;
	size_t size = 100;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = { 0xAA, 0xBB, 0xCC, 0xDD };
	size_t fwid_len = 4;
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN] = { 0xAA, 0xBB, 0xCC, 0xDD };
	size_t metadata_len = 4;
	const char *uri = "";

	ASSERT_EQUAL(sizeof(target_fw_ver_new), fwid_len);

	slot = bt_mesh_dfu_slot_add(size, fwid, fwid_len, metadata, metadata_len, uri, strlen(uri));
	if (!slot) {
		return false;
	}

	bt_mesh_dfu_slot_valid_set(slot, true);

	return true;
}

static void test_dist_dfu(void)
{
	enum bt_mesh_dfd_status status;

	ASSERT_TRUE(dfu_targets_cnt > 0);

	k_sem_init(&dfu_ended, 0, 1);

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &dist_comp);
	dist_prov_and_conf(DIST_ADDR);

	ASSERT_TRUE(slot_add());

	for (int i = 0; i < dfu_targets_cnt; i++) {
		status = bt_mesh_dfd_srv_receiver_add(&dfd_srv, DIST_ADDR + 1 + i, 0);
		ASSERT_EQUAL(BT_MESH_DFD_SUCCESS, status);
	}

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

	if (k_sem_take(&dfu_ended, K_SECONDS(DFU_TIMEOUT))) {
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

	PASS();
}

static void target_test_effect(enum bt_mesh_dfu_effect effect)
{
	k_sem_init(&dfu_ended, 0, 1);

	dfu_target_effect = effect;

	settings_test_backend_clear();
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &target_comp);
	target_prov_and_conf(bt_mesh_test_own_addr_get(DIST_ADDR));

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

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "dfu_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_args_f = test_args_parse,                \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_dfu[] = {
	TEST_CASE(dist, dfu, "Distributor performs DFU"),

	TEST_CASE(target, dfu_no_change, "Target node, Comp Data stays unchanged"),
	TEST_CASE(target, dfu_new_comp_no_rpr, "Target node, Comp Data changes, no RPR"),
	TEST_CASE(target, dfu_new_comp_rpr, "Target node, Comp Data changes, has RPR"),
	TEST_CASE(target, dfu_unprov, "Target node, Comp Data changes, unprovisioned"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_dfu_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_dfu);
	return tests;
}
