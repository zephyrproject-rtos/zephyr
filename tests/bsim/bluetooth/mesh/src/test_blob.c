/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "dfu_blob_common.h"
#include "friendship_common.h"
#include "mesh/adv.h"
#include "mesh/blob.h"
#include "argparse.h"

#define LOG_MODULE_NAME test_blob

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define BLOB_GROUP_ADDR 0xc000
#define BLOB_CLI_ADDR 0x0001
#define SYNC_CHAN 0
#define CLI_DEV 0
#define SRV1_DEV 1
#define IMPOSTER_MODEL_ID 0xe000

static bool is_pull_mode;
static enum {
	BLOCK_GET_FAIL = 0,
	XFER_GET_FAIL = 1
} msg_fail_type;
static enum bt_mesh_blob_xfer_phase expected_stop_phase;

static void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &is_pull_mode,
			.type = 'b',
			.name = "{0, 1}",
			.option = "use-pull-mode",
			.descript = "Set transfer type to pull mode"
		},
		{
			.dest = &msg_fail_type,
			.type = 'i',
			.name = "{0, 1}",
			.option = "msg-fail-type",
			.descript = "Message type to fail on"
		},
		{
			.dest = &expected_stop_phase,
			.type = 'i',
			.name = "{inactive, start, wait-block, wait-chunk, complete, suspended}",
			.option = "expected-phase",
			.descript = "Expected DFU Server phase value restored from flash"
		}
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);
}

static int blob_io_open(const struct bt_mesh_blob_io *io,
			const struct bt_mesh_blob_xfer *xfer,
			enum bt_mesh_blob_io_mode mode)
{
	return 0;
}

static struct k_sem first_block_wr_sem;
static uint16_t partial_block;
ATOMIC_DEFINE(block_bitfield, 8);

static struct k_sem blob_srv_end_sem;

static int blob_chunk_wr(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	partial_block += chunk->size;
	ASSERT_TRUE_MSG(partial_block <= block->size, "Received block is too large\n");


	if (partial_block == block->size) {
		partial_block = 0;
		ASSERT_FALSE_MSG(atomic_test_and_set_bit(block_bitfield, block->number),
				 "Received duplicate block\n");
	}

	if (atomic_test_bit(block_bitfield, 0)) {
		k_sem_give(&first_block_wr_sem);
	}

	if (expected_stop_phase == BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_CHUNK) {
		bt_mesh_scan_disable();
		k_sem_give(&blob_srv_end_sem);
	}
	return 0;
}

static int blob_chunk_rd(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	memset(chunk->data, 0, chunk->size);

	return 0;
}

static void blob_block_end(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block)
{
	if (expected_stop_phase == BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_BLOCK ||
	    expected_stop_phase == BT_MESH_BLOB_XFER_PHASE_SUSPENDED) {
		bt_mesh_scan_disable();
		k_sem_give(&blob_srv_end_sem);
	}
}

static const struct bt_mesh_blob_io blob_io = {
	.open = blob_io_open,
	.rd = blob_chunk_rd,
	.wr = blob_chunk_wr,
	.block_end = blob_block_end,
};

static uint8_t dev_key[16] = { 0xdd };
static uint8_t app_key[16] = { 0xaa };
static uint8_t net_key[16] = { 0xcc };
static struct bt_mesh_prov prov;

static struct {
	struct bt_mesh_blob_cli_inputs inputs;
	struct bt_mesh_blob_target targets[5];
	struct bt_mesh_blob_target_pull pull[5];
	uint8_t target_count;
	struct bt_mesh_blob_xfer xfer;
} blob_cli_xfer;

static struct k_sem blob_caps_sem;

static void blob_cli_caps(struct bt_mesh_blob_cli *b, const struct bt_mesh_blob_cli_caps *caps)
{
	k_sem_give(&blob_caps_sem);
	if (caps) {
		ASSERT_EQUAL(caps->mtu_size, BT_MESH_RX_SDU_MAX - BT_MESH_MIC_SHORT);
		ASSERT_EQUAL(caps->modes, BT_MESH_BLOB_XFER_MODE_ALL);
		ASSERT_EQUAL(caps->max_size, CONFIG_BT_MESH_BLOB_SIZE_MAX);
		ASSERT_EQUAL(caps->min_block_size_log, BLOB_BLOCK_SIZE_LOG_MIN);
		ASSERT_EQUAL(caps->max_block_size_log, BLOB_BLOCK_SIZE_LOG_MAX);
		ASSERT_EQUAL(caps->max_chunk_size, BLOB_CHUNK_SIZE_MAX(BT_MESH_RX_SDU_MAX));
		ASSERT_EQUAL(caps->max_chunks, CONFIG_BT_MESH_BLOB_CHUNK_COUNT_MAX);
	}
}

static struct k_sem lost_target_sem;

static void blob_cli_lost_target(struct bt_mesh_blob_cli *b, struct bt_mesh_blob_target *blobt,
				 enum bt_mesh_blob_status reason)
{
	ASSERT_FALSE(reason == BT_MESH_BLOB_SUCCESS);
	ASSERT_TRUE(lost_target_find_and_remove(blobt->addr));

	if (!lost_targets_rem()) {
		k_sem_give(&lost_target_sem);
	}
}

static struct k_sem blob_cli_suspend_sem;

static void blob_cli_suspended(struct bt_mesh_blob_cli *b)
{
	k_sem_give(&blob_cli_suspend_sem);
}

static struct k_sem blob_cli_end_sem;
static bool cli_end_success;

static void blob_cli_end(struct bt_mesh_blob_cli *b, const struct bt_mesh_blob_xfer *xfer,
			 bool success)
{
	cli_end_success = success;
	k_sem_give(&blob_cli_end_sem);
}

static struct k_sem blob_srv_suspend_sem;

static void blob_srv_suspended(struct bt_mesh_blob_srv *b)
{
	k_sem_give(&blob_srv_suspend_sem);
}

static void blob_srv_end(struct bt_mesh_blob_srv *b, uint64_t id, bool success)
{
	k_sem_give(&blob_srv_end_sem);
}

static int blob_srv_recover(struct bt_mesh_blob_srv *b, struct bt_mesh_blob_xfer *xfer,
			    const struct bt_mesh_blob_io **io)
{
	*io = &blob_io;
	return 0;
}

static int blob_srv_start(struct bt_mesh_blob_srv *srv, struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_blob_xfer *xfer)
{
	return 0;
}

static void blob_srv_resume(struct bt_mesh_blob_srv *srv)
{
}

static const struct bt_mesh_blob_srv_cb blob_srv_cb = {
	.suspended = blob_srv_suspended,
	.end = blob_srv_end,
	.recover = blob_srv_recover,
	.start = blob_srv_start,
	.resume = blob_srv_resume,
};
static const struct bt_mesh_blob_cli_cb blob_cli_handlers = {
	.caps = blob_cli_caps,
	.lost_target = blob_cli_lost_target,
	.suspended = blob_cli_suspended,
	.end = blob_cli_end,
};
static struct bt_mesh_blob_srv blob_srv = { .cb = &blob_srv_cb };
static struct bt_mesh_blob_cli blob_cli = { .cb = &blob_cli_handlers };
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;

static const struct bt_mesh_comp srv_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_BLOB_SRV(&blob_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static const struct bt_mesh_comp cli_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_BLOB_CLI(&blob_cli)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static struct k_sem info_get_sem;

static int mock_handle_info_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	k_sem_give(&info_get_sem);
	return 0;
}

static const struct bt_mesh_model_op model_op1[] = {
	{ BT_MESH_BLOB_OP_INFO_GET, 0, mock_handle_info_get },
	BT_MESH_MODEL_OP_END
};

static const struct bt_mesh_comp none_rsp_srv_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_BLOB_SRV,
								 model_op1, NULL, NULL, NULL)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static void provision(uint16_t addr)
{
	int err;

	err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}
}

static void common_configure(uint16_t addr)
{
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}
}

static void blob_srv_prov_and_conf(uint16_t addr)
{
	uint8_t status;
	int err;

	provision(addr);
	common_configure(addr);

	err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, BT_MESH_MODEL_ID_BLOB_SRV, &status);
	if (err || status) {
		FAIL("Model %#4x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_BLOB_SRV, err,
		     status);
		return;
	}

	err = bt_mesh_cfg_cli_mod_sub_add(0, addr, addr, BLOB_GROUP_ADDR, BT_MESH_MODEL_ID_BLOB_SRV,
					  &status);
	if (err || status) {
		FAIL("Model %#4x sub add failed (err %d, status %u)", BT_MESH_MODEL_ID_BLOB_SRV,
		     err, status);
		return;
	}

	common_sar_conf(addr);
}

static void blob_cli_prov_and_conf(uint16_t addr)
{
	uint8_t status;
	int err;

	provision(addr);
	common_configure(addr);

	err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, BT_MESH_MODEL_ID_BLOB_CLI, &status);
	if (err || status) {
		FAIL("Model %#4x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_BLOB_CLI, err,
		     status);
		return;
	}

	common_sar_conf(addr);
}

static void blob_cli_inputs_prepare(uint16_t group)
{
	blob_cli_xfer.inputs.ttl = BT_MESH_TTL_DEFAULT;
	blob_cli_xfer.inputs.group = group;
	blob_cli_xfer.inputs.app_idx = 0;
	sys_slist_init(&blob_cli_xfer.inputs.targets);

	for (int i = 0; i < blob_cli_xfer.target_count; ++i) {
		/* Reset target context. */
		uint16_t addr = blob_cli_xfer.targets[i].addr;

		memset(&blob_cli_xfer.targets[i], 0, sizeof(struct bt_mesh_blob_target));
		blob_cli_xfer.targets[i].addr = addr;
		blob_cli_xfer.targets[i].pull = &blob_cli_xfer.pull[i];

		sys_slist_append(&blob_cli_xfer.inputs.targets, &blob_cli_xfer.targets[i].n);
	}
}

static struct bt_mesh_blob_target *target_srv_add(uint16_t addr, bool expect_lost)
{
	if (expect_lost) {
		lost_target_add(addr);
	}

	ASSERT_TRUE(blob_cli_xfer.target_count < ARRAY_SIZE(blob_cli_xfer.targets));
	struct bt_mesh_blob_target *t = &blob_cli_xfer.targets[blob_cli_xfer.target_count];

	t->addr = addr;
	blob_cli_xfer.target_count++;
	return t;
}

static void cli_caps_common_procedure(bool lost_targets)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);

	err = bt_mesh_blob_cli_caps_get(&blob_cli, &blob_cli_xfer.inputs);
	if (err) {
		FAIL("Boundary check start failed (err: %d)", err);
	}

	if (lost_targets) {
		if (k_sem_take(&lost_target_sem, K_SECONDS(60))) {
			FAIL("Lost targets CB did not trigger for all expected lost targets");
		}
	}

	if (k_sem_take(&blob_caps_sem, K_SECONDS(60))) {
		FAIL("Caps CB did not trigger at the end of caps procedure");
	}
}

static void test_cli_caps_all_rsp(void)
{
	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, false);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, false);

	cli_caps_common_procedure(false);

	ASSERT_TRUE(srv1->acked);
	ASSERT_FALSE(srv1->timedout);
	ASSERT_TRUE(srv2->acked);
	ASSERT_FALSE(srv2->timedout);

	PASS();
}

static void test_cli_caps_partial_rsp(void)
{
	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, false);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, true);

	cli_caps_common_procedure(true);

	ASSERT_TRUE(srv1->acked);
	ASSERT_FALSE(srv1->timedout);
	ASSERT_FALSE(srv2->acked);
	ASSERT_TRUE(srv2->timedout);

	PASS();
}

static void test_cli_caps_no_rsp(void)
{
	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, true);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, true);

	cli_caps_common_procedure(true);

	ASSERT_FALSE(srv1->acked);
	ASSERT_TRUE(srv1->timedout);
	ASSERT_FALSE(srv2->acked);
	ASSERT_TRUE(srv2->timedout);

	PASS();
}

static void test_cli_caps_cancelled(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 300);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, false);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, true);

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);

	/* Start first caps procedure */
	err = bt_mesh_blob_cli_caps_get(&blob_cli, &blob_cli_xfer.inputs);
	if (err) {
		FAIL("Boundary check start failed (err: %d)", err);
	}

	/* Let first caps procedure run for a little while */
	k_sleep(K_SECONDS(15));

	/* Cancel first caps procedure */
	bt_mesh_blob_cli_cancel(&blob_cli);
	ASSERT_EQUAL(blob_cli.state, BT_MESH_BLOB_CLI_STATE_NONE);

	/* Wait and assure that caps procedure is canceled */
	if (!k_sem_take(&blob_caps_sem, K_SECONDS(60))) {
		FAIL("Caps CB triggered unexpectedly");
	}

	/* Expect that the responsive srv responded, while the */
	/* unresponsive srv has not yet timed out due to cancel call */
	ASSERT_TRUE(srv1->acked);
	ASSERT_FALSE(srv1->timedout);
	ASSERT_FALSE(srv2->acked);
	ASSERT_FALSE(srv2->timedout);

	/* Start second caps procedure and verify that it completes as expected*/
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	err = bt_mesh_blob_cli_caps_get(&blob_cli, &blob_cli_xfer.inputs);
	if (err) {
		FAIL("Boundary check start failed (err: %d)", err);
	}

	if (k_sem_take(&blob_caps_sem, K_SECONDS(60))) {
		FAIL("Caps CB did not trigger at the end of second caps procedure");
	}

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expeted lost targets");
	}

	ASSERT_TRUE(srv1->acked);
	ASSERT_FALSE(srv1->timedout);
	ASSERT_FALSE(srv2->acked);
	ASSERT_TRUE(srv2->timedout);

	PASS();
}

static void test_srv_caps_standard(void)
{
	bt_mesh_test_cfg_set(NULL, 140);
	bt_mesh_device_setup(&prov, &srv_comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	PASS();
}

static void test_srv_caps_no_rsp(void)
{
	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &none_rsp_srv_comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	k_sem_init(&info_get_sem, 0, 1);

	/* Checks that the client performs correct amount of retransmit attempts */
	for (size_t j = 0; j < CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES; j++) {
		int err = k_sem_take(&info_get_sem, K_SECONDS(15));

		if (err) {
			FAIL("Failed to receive expected number of info get messages from cli"
			"(expected: %d, got %d)", CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES, j);
		}
	}

	PASS();
}

static struct k_sem blob_broad_send_sem;
static bool broadcast_tx_complete_auto;

static void broadcast_send(struct bt_mesh_blob_cli *b, uint16_t dst)
{
	ASSERT_EQUAL(BLOB_GROUP_ADDR, dst);
	k_sem_give(&blob_broad_send_sem);
	if (broadcast_tx_complete_auto) {
		/* Mocks completion of transmission to trigger retransmit timer */
		blob_cli_broadcast_tx_complete(&blob_cli);
	}
}

static struct k_sem blob_broad_next_sem;

static void broadcast_next(struct bt_mesh_blob_cli *b)
{
	k_sem_give(&blob_broad_next_sem);
}

static void test_cli_broadcast_basic(void)
{
	bt_mesh_test_cfg_set(NULL, 300);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, false);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, false);

	struct blob_cli_broadcast_ctx tx = {
		.send = broadcast_send,
		.next = broadcast_next,
		.acked = true,
		.optional = false,
	};

	broadcast_tx_complete_auto = false;
	k_sem_init(&blob_broad_send_sem, 0, 1);
	k_sem_init(&blob_broad_next_sem, 0, 1);

	blob_cli.inputs = &blob_cli_xfer.inputs;
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	/* Call broadcast and expect send CB to trigger */
	blob_cli_broadcast(&blob_cli, &tx);
	if (k_sem_take(&blob_broad_send_sem, K_SECONDS(15))) {
		FAIL("Broadcast did not trigger send CB");
	}

	ASSERT_FALSE(srv1->acked);
	ASSERT_FALSE(srv2->acked);

	/* Run tx complete with two missing responses */
	blob_cli_broadcast_tx_complete(&blob_cli);
	if (k_sem_take(&blob_broad_send_sem, K_SECONDS(15))) {
		FAIL("Tx_complete did not trigger send CB after timeout");
	}

	ASSERT_FALSE(srv1->acked);
	ASSERT_FALSE(srv2->acked);

	/* Mock rsp from first target server */
	/* Run tx complete with one missing response */
	blob_cli_broadcast_rsp(&blob_cli, srv1);
	blob_cli_broadcast_tx_complete(&blob_cli);
	if (k_sem_take(&blob_broad_send_sem, K_SECONDS(15))) {
		FAIL("Tx_complete did not trigger send CB after timeout");
	}

	ASSERT_TRUE(srv1->acked);
	ASSERT_FALSE(srv2->acked);

	/* Mock rsp from second target server */
	/* Run tx complete with response from all targets */
	blob_cli_broadcast_tx_complete(&blob_cli);
	blob_cli_broadcast_rsp(&blob_cli, srv2);
	if (k_sem_take(&blob_broad_next_sem, K_SECONDS(15))) {
		FAIL("Tx_complete did not trigger next CB after timeout");
	}

	ASSERT_TRUE(srv1->acked);
	ASSERT_TRUE(srv2->acked);

	/* Verify that a single broadcast call triggers a single send CB */
	k_sem_init(&blob_broad_send_sem, 0, 2);
	(void)target_srv_add(BLOB_CLI_ADDR + 3, false);

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	blob_cli_broadcast(&blob_cli, &tx);
	k_sleep(K_SECONDS(80));

	ASSERT_EQUAL(k_sem_count_get(&blob_broad_send_sem), 1);

	PASS();
}

static void test_cli_broadcast_trans(void)
{
	bt_mesh_test_cfg_set(NULL, 150);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, true);

	struct blob_cli_broadcast_ctx tx = {
		.send = broadcast_send,
		.next = broadcast_next,
		.acked = true,
		.optional = false
	};

	broadcast_tx_complete_auto = true;
	k_sem_init(&blob_broad_send_sem, 0, 1);
	k_sem_init(&blob_broad_next_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);

	blob_cli.inputs = &blob_cli_xfer.inputs;

	/* Run acked broadcast */
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	blob_cli_broadcast(&blob_cli, &tx);

	/* Checks that the client performs correct amount of retransmit attempts */
	for (size_t j = 0; j < CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES; j++) {
		if (k_sem_take(&blob_broad_send_sem, K_SECONDS(15))) {
			FAIL("Wrong number of attempted transmissions from blob cli"
			     "(expected: %d, got %d)",
			     CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES, j);
		}
	}

	if (k_sem_take(&blob_broad_next_sem, K_SECONDS(15))) {
		FAIL("Broadcast did not trigger next CB after retransmisson ran out of attempts");
	}

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expected lost targets");
	}

	ASSERT_TRUE(srv1->timedout);

	/* Re-run with unacked broadcast */
	tx.acked = false;
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	/* Call broadcast and expect send CB to trigger once*/
	blob_cli_broadcast(&blob_cli, &tx);
	if (k_sem_take(&blob_broad_send_sem, K_NO_WAIT)) {
		FAIL("Broadcast did not trigger send CB");
	}

	if (k_sem_take(&blob_broad_next_sem, K_SECONDS(1))) {
		FAIL("Broadcast did not trigger next CB");
	}

	/* Lost target CB should not trigger for unacked broadcast */
	if (!k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB triggered unexpectedly");
	}

	ASSERT_FALSE(srv1->timedout);

	/* Re-run with optional flag */
	tx.acked = true;
	tx.optional = true;
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);

	blob_cli_broadcast(&blob_cli, &tx);

	for (size_t j = 0; j < CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES; j++) {
		if (k_sem_take(&blob_broad_send_sem, K_SECONDS(15))) {
			FAIL("Wrong number of attempted transmissions from blob cli"
			     "(expected: %d, got %d)",
			     CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES, j);
		}
	}

	if (k_sem_take(&blob_broad_next_sem, K_SECONDS(15))) {
		FAIL("Broadcast did not trigger next CB");
	}

	/* Lost target CB should not trigger for optional broadcast */
	if (!k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB triggered unexpectedly");
	}

	ASSERT_FALSE(srv1->timedout);

	PASS();
}

static uint16_t dst_addr_last;
static struct k_sem blob_broad_send_uni_sem;

static void broadcast_uni_send(struct bt_mesh_blob_cli *b, uint16_t dst)
{
	dst_addr_last = dst;
	k_sem_give(&blob_broad_send_uni_sem);
	if (broadcast_tx_complete_auto) {
		/* Mocks completion of transmission to trigger retransmit timer */
		blob_cli_broadcast_tx_complete(&blob_cli);
	}
}

static void test_cli_broadcast_unicast_seq(void)
{
	bt_mesh_test_cfg_set(NULL, 60);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	struct bt_mesh_blob_target *srv1 = target_srv_add(BLOB_CLI_ADDR + 1, false);
	struct bt_mesh_blob_target *srv2 = target_srv_add(BLOB_CLI_ADDR + 2, false);

	struct blob_cli_broadcast_ctx tx = {
		.send = broadcast_uni_send,
		.next = broadcast_next,
		.acked = true,
		.optional = false,
	};

	k_sem_init(&blob_broad_send_uni_sem, 0, 1);
	k_sem_init(&blob_broad_next_sem, 0, 1);

	blob_cli.inputs = &blob_cli_xfer.inputs;
	broadcast_tx_complete_auto = false;

	/** Two responsive targets. Checks that:
	 * - Send CB alternates between targets
	 * - Don't retransmit to responded targets
	 * - Next CB is called as soon as all have responded
	 * (Test assumes at least 5 transmission attempts)
	 */
	BUILD_ASSERT(CONFIG_BT_MESH_BLOB_CLI_BLOCK_RETRIES >= 5);

	blob_cli_inputs_prepare(BT_MESH_ADDR_UNASSIGNED);
	blob_cli_broadcast(&blob_cli, &tx);

	for (size_t i = 0; i < 2; i++) {
		if (k_sem_take(&blob_broad_send_uni_sem, K_SECONDS(10))) {
			FAIL("Broadcast did not trigger send CB");
		}

		ASSERT_EQUAL(BLOB_CLI_ADDR + 1, dst_addr_last);
		blob_cli_broadcast_tx_complete(&blob_cli);
		if (k_sem_take(&blob_broad_send_uni_sem, K_SECONDS(10))) {
			FAIL("Tx complete did not trigger send CB");
		}

		ASSERT_EQUAL(BLOB_CLI_ADDR + 2, dst_addr_last);
		blob_cli_broadcast_tx_complete(&blob_cli);
	}

	blob_cli_broadcast_rsp(&blob_cli, srv1);
	for (size_t i = 0; i < 2; i++) {
		if (k_sem_take(&blob_broad_send_uni_sem, K_SECONDS(10))) {
			FAIL("Tx complete did not trigger send CB");
		}

		ASSERT_EQUAL(BLOB_CLI_ADDR + 2, dst_addr_last);
		blob_cli_broadcast_tx_complete(&blob_cli);
	}

	blob_cli_broadcast_rsp(&blob_cli, srv2);
	if (!k_sem_take(&blob_broad_send_uni_sem, K_SECONDS(10))) {
		FAIL("Unexpected send CB");
	}

	if (k_sem_take(&blob_broad_next_sem, K_NO_WAIT)) {
		FAIL("Broadcast did not trigger next CB");
	}

	PASS();
}

static void test_cli_broadcast_unicast(void)
{
	bt_mesh_test_cfg_set(NULL, 120);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 2, true);

	struct blob_cli_broadcast_ctx tx = {
		.send = broadcast_uni_send,
		.next = broadcast_next,
		.acked = true,
		.optional = false,
	};

	k_sem_init(&blob_broad_send_uni_sem, 0, 1);
	k_sem_init(&blob_broad_next_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);

	blob_cli.inputs = &blob_cli_xfer.inputs;
	broadcast_tx_complete_auto = true;

	/** 1. Two non-responsive targets. Checks that:
	 * - Next CB is called after all retransmit attempts expires
	 * - All lost targets is registered
	 */
	blob_cli_inputs_prepare(BT_MESH_ADDR_UNASSIGNED);
	blob_cli_broadcast(&blob_cli, &tx);

	if (k_sem_take(&blob_broad_next_sem, K_SECONDS(60))) {
		FAIL("Broadcast did not trigger next CB");
	}

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expected lost targets");
	}

	/** 2. Two non-responsive targets re-run. Checks that:
	 * - Already lost targets does not attempt new transmission
	 * (Next CB called immediately)
	 */
	blob_cli_broadcast(&blob_cli, &tx);
	if (k_sem_take(&blob_broad_next_sem, K_NO_WAIT)) {
		FAIL("Broadcast did not trigger immediate next CB");
	}

	/** 3. Two non-responsive targets (Abort after first attempt). Checks that:
	 * - First transmission calls send CB
	 * - After abort is called, neither send or next CB is called
	 */
	k_sem_init(&blob_broad_send_uni_sem, 0, 1);
	blob_cli_inputs_prepare(BT_MESH_ADDR_UNASSIGNED);
	blob_cli_broadcast(&blob_cli, &tx);
	if (k_sem_take(&blob_broad_send_uni_sem, K_NO_WAIT)) {
		FAIL("Broadcast did not trigger send CB");
	}

	blob_cli_broadcast_abort(&blob_cli);
	if (!k_sem_take(&blob_broad_send_uni_sem, K_SECONDS(60))) {
		FAIL("Unexpected send CB");
	}

	if (!k_sem_take(&blob_broad_next_sem, K_NO_WAIT)) {
		FAIL("Unexpected next CB");
	}

	PASS();
}

static void test_cli_trans_complete(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 400);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	(void)target_srv_add(BLOB_CLI_ADDR + 1, false);
	(void)target_srv_add(BLOB_CLI_ADDR + 2, false);
	(void)target_srv_add(BLOB_CLI_ADDR + 3, false);
	(void)target_srv_add(BLOB_CLI_ADDR + 4, false);

	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&blob_cli_end_sem, 0, 1);
	k_sem_init(&blob_cli_suspend_sem, 0, 1);

	LOG_INF("Running transfer in %s", is_pull_mode ? "Pull mode" : "Push mode");

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	blob_cli_xfer.xfer.mode =
		is_pull_mode ? BT_MESH_BLOB_XFER_MODE_PULL : BT_MESH_BLOB_XFER_MODE_PUSH;
	blob_cli_xfer.xfer.size = CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 4;
	blob_cli_xfer.xfer.id = 1;
	blob_cli_xfer.xfer.block_size_log = 9;
	blob_cli_xfer.xfer.chunk_size = 377;
	blob_cli_xfer.inputs.timeout_base = 10;

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_end_sem, K_SECONDS(380))) {
		FAIL("End CB did not trigger as expected for the cli");
	}

	ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_NONE);

	PASS();
}

static void test_srv_trans_complete(void)
{
	bt_mesh_test_cfg_set(NULL, 400);
	bt_mesh_device_setup(&prov, &srv_comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	k_sem_init(&first_block_wr_sem, 0, 1);
	k_sem_init(&blob_srv_end_sem, 0, 1);
	k_sem_init(&blob_srv_suspend_sem, 0, 1);

	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 10);

	if (k_sem_take(&blob_srv_end_sem, K_SECONDS(380))) {
		FAIL("End CB did not trigger as expected for the srv");
	}

	ASSERT_TRUE(blob_srv.phase == BT_MESH_BLOB_XFER_PHASE_COMPLETE);

	/* Check that all blocks is received */
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 0));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 1));

	/* Check that a third block was not received */
	ASSERT_FALSE(atomic_test_bit(block_bitfield, 2));

	PASS();
}

static void test_cli_trans_resume(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 800);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);

	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&blob_cli_end_sem, 0, 1);
	k_sem_init(&blob_cli_suspend_sem, 0, 1);

	LOG_INF("Running transfer in %s", is_pull_mode ? "Pull mode" : "Push mode");

	/** Test resumption of suspended BLOB transfer (Push).
	 * Client initiates transfer with two blocks. After
	 * first block completes the server will be suspended.
	 * At this point the client will attempt to resume the
	 * transfer.
	 */
	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	blob_cli_xfer.xfer.mode =
		is_pull_mode ? BT_MESH_BLOB_XFER_MODE_PULL : BT_MESH_BLOB_XFER_MODE_PUSH;
	blob_cli_xfer.xfer.size = CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 4;
	blob_cli_xfer.xfer.id = 1;
	blob_cli_xfer.xfer.block_size_log = 9;
	blob_cli_xfer.xfer.chunk_size = 377;
	blob_cli_xfer.inputs.timeout_base = 10;

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_suspend_sem, K_SECONDS(500))) {
		FAIL("Suspend CB did not trigger as expected for the cli");
	}

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for the target srv");
	}

	ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_SUSPENDED);

	/* Initiate resumption of BLOB transfer */
	err = bt_mesh_blob_cli_resume(&blob_cli);
	if (err) {
		FAIL("BLOB resume failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_end_sem, K_SECONDS(780))) {
		FAIL("End CB did not trigger as expected for the cli");
	}

	ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_NONE);

	PASS();
}

static void test_srv_trans_resume(void)
{
	bt_mesh_test_cfg_set(NULL, 800);
	bt_mesh_device_setup(&prov, &srv_comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	k_sem_init(&first_block_wr_sem, 0, 1);
	k_sem_init(&blob_srv_end_sem, 0, 1);
	k_sem_init(&blob_srv_suspend_sem, 0, 1);

	/** Wait for a first blob block to be received, then simulate radio
	 * disruption to cause suspension of the blob srv. Re-enable the radio
	 * as soon as the server is suspended and wait to receive the second
	 * block.
	 */
	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 10);

	/* Let server receive a couple of chunks from second block before disruption */
	for (int i = 0; i < 2; i++) {
		if (k_sem_take(&first_block_wr_sem, K_SECONDS(180))) {
			FAIL("Server did not receive the first BLOB block");
		}
	}

	bt_mesh_scan_disable();
	partial_block = 0;
	if (k_sem_take(&blob_srv_suspend_sem, K_SECONDS(140))) {
		FAIL("Suspend CB did not trigger as expected for the srv");
	}

	ASSERT_TRUE(blob_srv.phase == BT_MESH_BLOB_XFER_PHASE_SUSPENDED);

	/* Wait for BLOB client to suspend. Measured experimentally. */
	k_sleep(K_SECONDS(140));

	bt_mesh_scan_enable();

	if (k_sem_take(&blob_srv_end_sem, K_SECONDS(780))) {
		FAIL("End CB did not trigger as expected for the srv");
	}

	ASSERT_TRUE(blob_srv.phase == BT_MESH_BLOB_XFER_PHASE_COMPLETE);

	/* Check that all blocks is received */
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 0));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 1));

	/* Check that a third block was not received */
	ASSERT_FALSE(atomic_test_bit(block_bitfield, 2));

	PASS();
}

static void cli_pull_mode_setup(void)
{
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&blob_cli_end_sem, 0, 1);
	k_sem_init(&blob_cli_suspend_sem, 0, 1);

	blob_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PULL;
	blob_cli_xfer.xfer.size = CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 3;
	blob_cli_xfer.xfer.id = 1;
	blob_cli_xfer.xfer.block_size_log = 8;
	blob_cli_xfer.xfer.chunk_size = 36;
	blob_cli_xfer.inputs.timeout_base = 10;
}

static void test_cli_trans_persistency_pull(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 240);

	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 2, false);

	cli_pull_mode_setup();

	blob_cli_inputs_prepare(0);

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_end_sem, K_SECONDS(230))) {
		FAIL("End CB did not trigger as expected for the cli");
	}

	ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_NONE);

	PASS();
}

static void srv_pull_mode_setup(void)
{
	bt_mesh_device_setup(&prov, &srv_comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	k_sem_init(&first_block_wr_sem, 0, 1);
	k_sem_init(&blob_srv_end_sem, 0, 1);
	k_sem_init(&blob_srv_suspend_sem, 0, 1);
}

static void test_srv_trans_persistency_pull(void)
{
	bt_mesh_test_cfg_set(NULL, 240);

	srv_pull_mode_setup();

	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 10);

	/* Target with address 0x0002 (the first one) will disappear after receiving the first
	 * block. Target with address 0x0003 (the second one) will stay alive.
	 */
	if (bt_mesh_test_own_addr_get(BLOB_CLI_ADDR) == 0x0002) {
		/* Let the first target receive a couple of chunks from second block before
		 * disruption.
		 */
		for (int i = 0; i < 3; i++) {
			if (k_sem_take(&first_block_wr_sem, K_SECONDS(100))) {
				FAIL("Server did not receive the first BLOB block");
			}
		}

		bt_mesh_scan_disable();
		bt_mesh_blob_srv_cancel(&blob_srv);
		PASS();
		return;
	}

	if (k_sem_take(&blob_srv_end_sem, K_SECONDS(230))) {
		FAIL("End CB did not trigger as expected for the srv");
	}

	ASSERT_TRUE(blob_srv.phase == BT_MESH_BLOB_XFER_PHASE_COMPLETE);

	/* Check that all blocks is received */
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 0));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 1));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 2));

	/* Check that a third block was not received */
	ASSERT_FALSE(atomic_test_bit(block_bitfield, 3));

	PASS();
}

/* Makes device unresponsive after I/O is opened */
static int fail_on_io_open(const struct bt_mesh_blob_io *io,
			const struct bt_mesh_blob_xfer *xfer,
			enum bt_mesh_blob_io_mode mode)
{
	bt_mesh_scan_disable();
	return 0;
}

/* Makes device unresponsive after receiving block start msg */
static int fail_on_block_start(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
		     const struct bt_mesh_blob_block *block)
{
	bt_mesh_scan_disable();
	return 0;
}

static void cli_common_fail_on_init(void)
{
	bt_mesh_test_cfg_set(NULL, 800);
	bt_mesh_device_setup(&prov, &cli_comp);
	blob_cli_prov_and_conf(BLOB_CLI_ADDR);

	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&blob_cli_end_sem, 0, 1);
	k_sem_init(&blob_cli_suspend_sem, 0, 1);

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	blob_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	blob_cli_xfer.xfer.size = CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 2;
	blob_cli_xfer.xfer.id = 1;
	blob_cli_xfer.xfer.block_size_log = 9;
	blob_cli_xfer.xfer.chunk_size = 377;
	blob_cli_xfer.inputs.timeout_base = 10;
}

static void test_cli_fail_on_persistency(void)
{
	int err;

	/** Test that Push mode BLOB transfer persists as long as at
	 * least one target is still active. During the test multiple
	 * servers will become unresponsive at different phases of the
	 * transfer:
	 * - Srv 0x0002 will not respond to Block start msg.
	 * - Srv 0x0003 will not respond to Block get msg.
	 * - Srv 0x0004 will not respond to Xfer get msg.
	 * - Srv 0x0005 is responsive all the way
	 * - Srv 0x0006 is a non-existing unresponsive node
	 */
	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 2, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 3, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 4, false);
	(void)target_srv_add(BLOB_CLI_ADDR + 5, true);

	cli_common_fail_on_init();

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_end_sem, K_SECONDS(750))) {
		FAIL("End CB did not trigger as expected for the cli");
	}

	ASSERT_TRUE(cli_end_success);

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expected lost targets");
	}

	PASS();
}

static void common_fail_on_srv_init(const struct bt_mesh_comp *comp)
{
	bt_mesh_test_cfg_set(NULL, 800);
	bt_mesh_device_setup(&prov, comp);
	blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));

	k_sem_init(&first_block_wr_sem, 0, 1);
	k_sem_init(&blob_srv_end_sem, 0, 1);
	k_sem_init(&blob_srv_suspend_sem, 0, 1);
}

static void test_srv_fail_on_block_start(void)
{
	common_fail_on_srv_init(&srv_comp);

	static const struct bt_mesh_blob_io io = {
		.open = fail_on_io_open,
		.rd = blob_chunk_rd,
		.wr = blob_chunk_wr,
	};

	bt_mesh_blob_srv_recv(&blob_srv, 1, &io, 0, 1);

	PASS();
}

static void test_srv_fail_on_block_get(void)
{
	common_fail_on_srv_init(&srv_comp);

	static const struct bt_mesh_blob_io io = {
		.open = blob_io_open,
		.rd = blob_chunk_rd,
		.wr = blob_chunk_wr,
		.block_start = fail_on_block_start,
	};

	bt_mesh_blob_srv_recv(&blob_srv, 1, &io, 0, 1);

	PASS();
}

static int dummy_xfer_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	return 0;
}

static const struct bt_mesh_model_op model_op2[] = {
	{ BT_MESH_BLOB_OP_XFER_GET, 0, dummy_xfer_get },
	BT_MESH_MODEL_OP_END
};

/** Composition data for BLOB server where we intercept the
 * BT_MESH_BLOB_OP_XFER_GET message handler through an imposter
 * model. This is done to emulate a BLOB server that becomes
 * unresponsive at the later stage of a BLOB transfer.
 */
static const struct bt_mesh_comp srv_broken_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_SAR_CFG_SRV,
						BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
						BT_MESH_MODEL_CB(IMPOSTER_MODEL_ID,
								 model_op2, NULL, NULL, NULL),
						BT_MESH_MODEL_BLOB_SRV(&blob_srv)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static void test_srv_fail_on_xfer_get(void)
{
	common_fail_on_srv_init(&srv_broken_comp);

	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 5);

	PASS();
}

static void test_srv_fail_on_nothing(void)
{
	common_fail_on_srv_init(&srv_comp);

	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 5);

	PASS();
}

static void test_cli_fail_on_no_rsp(void)
{
	int err;

	/** Test fail conditions upon non-responsive servers
	 * during push transfer. Depending on the set
	 * test message type it tests the following:
	 *
	 * msg_fail_type = BLOCK_GET_FAIL - BLOB transfer suspends
	 * when targets does not respond to Block get.
	 * msg_fail_type = XFER_GET_FAIL - BLOB transfer stops
	 * when targets does not respond to Xfer get message.
	 */

	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);
	(void)target_srv_add(BLOB_CLI_ADDR + 2, true);

	cli_common_fail_on_init();

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	switch (msg_fail_type) {
	case BLOCK_GET_FAIL:
		if (k_sem_take(&blob_cli_suspend_sem, K_SECONDS(750))) {
			FAIL("Suspend CB did not trigger as expected for the cli");
		}

		break;

	case XFER_GET_FAIL:
		if (k_sem_take(&blob_cli_end_sem, K_SECONDS(750))) {
			FAIL("End CB did not trigger as expected for the cli");
		}

		ASSERT_FALSE(cli_end_success);
		break;

	default:
		FAIL("Did not recognize the message type of the test");
	}

	if (k_sem_take(&lost_target_sem, K_NO_WAIT)) {
		FAIL("Lost targets CB did not trigger for all expected lost targets");
	}

	PASS();
}

#if CONFIG_BT_SETTINGS
static void cli_stop_setup(void)
{
	bt_mesh_device_setup(&prov, &cli_comp);

	(void)target_srv_add(BLOB_CLI_ADDR + 1, true);

	blob_cli_inputs_prepare(BLOB_GROUP_ADDR);
	blob_cli_xfer.xfer.mode =
		is_pull_mode ? BT_MESH_BLOB_XFER_MODE_PULL : BT_MESH_BLOB_XFER_MODE_PUSH;
	blob_cli_xfer.xfer.size = CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 4;
	blob_cli_xfer.xfer.id = 1;
	blob_cli_xfer.xfer.block_size_log = 9;
	blob_cli_xfer.xfer.chunk_size = 377;
	blob_cli_xfer.inputs.timeout_base = 10;
}

static void cli_restore_suspended(void)
{
	blob_cli.state = BT_MESH_BLOB_CLI_STATE_SUSPENDED;
	blob_cli.inputs = &blob_cli_xfer.inputs;
	blob_cli.xfer = &blob_cli_xfer.xfer;
	blob_cli_xfer.xfer.id = 1;
	blob_cli.io = &blob_io;

	bt_mesh_blob_cli_resume(&blob_cli);
}

static void test_cli_stop(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 500);
	k_sem_init(&blob_caps_sem, 0, 1);
	k_sem_init(&lost_target_sem, 0, 1);
	k_sem_init(&blob_cli_end_sem, 0, 1);
	k_sem_init(&blob_cli_suspend_sem, 0, 1);

	switch (expected_stop_phase) {
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_START:
		/* Nothing to do on client side in this step,
		 * just self-provision for future steps
		 */
		bt_mesh_device_setup(&prov, &cli_comp);
		blob_cli_prov_and_conf(BLOB_CLI_ADDR);
		break;
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_BLOCK:
		/* Target will be unresponsive once first block completes */
		cli_stop_setup();

		err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
					    &blob_cli_xfer.xfer, &blob_io);
		if (err) {
			FAIL("BLOB send failed (err: %d)", err);
		}

		if (k_sem_take(&blob_cli_suspend_sem, K_SECONDS(750))) {
			FAIL("Suspend targets CB did not trigger for all expected lost targets");
		}

		break;
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_CHUNK:
		cli_stop_setup();

		cli_restore_suspended();

		/* This will time out but gives time for server to process all messages */
		k_sem_take(&blob_cli_end_sem, K_SECONDS(380));

		break;
	case BT_MESH_BLOB_XFER_PHASE_COMPLETE:
		cli_stop_setup();

		cli_restore_suspended();

		if (k_sem_take(&blob_cli_end_sem, K_SECONDS(380))) {
			FAIL("End CB did not trigger as expected for the cli");
		}

		ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_NONE);

		break;
	case BT_MESH_BLOB_XFER_PHASE_SUSPENDED:
		/* Server will become unresponsive after receiving first chunk */
		cli_stop_setup();

		blob_cli_prov_and_conf(BLOB_CLI_ADDR);

		err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
					    &blob_cli_xfer.xfer, &blob_io);
		if (err) {
			FAIL("BLOB send failed (err: %d)", err);
		}

		if (k_sem_take(&blob_cli_suspend_sem, K_SECONDS(750))) {
			FAIL("Lost targets CB did not trigger for all expected lost targets");
		}

		break;
	default:
		/* There is no use case to stop in Inactive phase */
		FAIL();
	}

	PASS();
}

static void srv_check_reboot_and_continue(void)
{
	ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_SUSPENDED, blob_srv.phase);
	ASSERT_EQUAL(0, blob_srv.state.ttl);
	ASSERT_EQUAL(BLOB_CLI_ADDR, blob_srv.state.cli);
	ASSERT_EQUAL(1, blob_srv.state.timeout_base);
	ASSERT_EQUAL(BT_MESH_RX_SDU_MAX - BT_MESH_MIC_SHORT, blob_srv.state.mtu_size);
	ASSERT_EQUAL(CONFIG_BT_MESH_BLOB_BLOCK_SIZE_MIN * 4, blob_srv.state.xfer.size);
	ASSERT_EQUAL(9, blob_srv.state.xfer.block_size_log);
	ASSERT_EQUAL(1, blob_srv.state.xfer.id);
	ASSERT_TRUE(blob_srv.state.xfer.mode != BT_MESH_BLOB_XFER_MODE_NONE);
	/* First block should be already received, second one pending */
	ASSERT_FALSE(atomic_test_bit(blob_srv.state.blocks, 0));
	ASSERT_TRUE(atomic_test_bit(blob_srv.state.blocks, 1));

	k_sem_take(&blob_srv_end_sem, K_SECONDS(500));
}

static void test_srv_stop(void)
{
	bt_mesh_test_cfg_set(NULL, 500);
	k_sem_init(&blob_srv_end_sem, 0, 1);
	k_sem_init(&first_block_wr_sem, 0, 1);
	k_sem_init(&blob_srv_suspend_sem, 0, 1);

	bt_mesh_device_setup(&prov, &srv_comp);

	switch (expected_stop_phase) {
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_START:
		blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));
		bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 1);

		ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_START, blob_srv.phase);
		break;
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_BLOCK:
		ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_START, blob_srv.phase);
		ASSERT_OK(blob_srv.state.xfer.mode != BT_MESH_BLOB_XFER_MODE_NONE);
		ASSERT_EQUAL(0, blob_srv.state.ttl);

		k_sem_take(&blob_srv_end_sem, K_SECONDS(500));

		ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_BLOCK, blob_srv.phase);
		break;
	case BT_MESH_BLOB_XFER_PHASE_WAITING_FOR_CHUNK:
		__fallthrough;
	case BT_MESH_BLOB_XFER_PHASE_COMPLETE:
		srv_check_reboot_and_continue();

		ASSERT_EQUAL(expected_stop_phase, blob_srv.phase);
		break;
	case BT_MESH_BLOB_XFER_PHASE_SUSPENDED:
		/* This state is expected to be reached from freshly started procedure */
		ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_INACTIVE, blob_srv.phase);
		ASSERT_EQUAL(BT_MESH_BLOB_XFER_MODE_NONE, blob_srv.state.xfer.mode);
		ASSERT_EQUAL(BT_MESH_TTL_DEFAULT, blob_srv.state.ttl);

		blob_srv_prov_and_conf(bt_mesh_test_own_addr_get(BLOB_CLI_ADDR));
		bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 1);
		k_sem_take(&blob_srv_suspend_sem, K_SECONDS(140));

		ASSERT_EQUAL(BT_MESH_BLOB_XFER_PHASE_SUSPENDED, blob_srv.phase);
		break;
	default:
		/* There is no use case to stop in Inactive phase */
		FAIL();
	}

	PASS();
}
#endif /* CONFIG_BT_SETTINGS */

static void test_cli_friend_pull(void)
{
	int err;

	bt_mesh_test_cfg_set(NULL, 500);

	bt_mesh_test_friendship_init(1);

	cli_pull_mode_setup();

	bt_mesh_friend_set(BT_MESH_FEATURE_ENABLED);

	for (int i = 1; i <= CONFIG_BT_MESH_FRIEND_LPN_COUNT; i++) {

		ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_FRIEND_ESTABLISHED,
							       K_SECONDS(5)),
			      "Friendship not established");
		(void)target_srv_add(BLOB_CLI_ADDR + i, false);
	}

	blob_cli_inputs_prepare(0);

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &blob_io);
	if (err) {
		FAIL("BLOB send failed (err: %d)", err);
	}

	if (k_sem_take(&blob_cli_end_sem, K_SECONDS(750))) {
		FAIL("End CB did not trigger as expected for the cli");
	}

	ASSERT_TRUE(blob_cli.state == BT_MESH_BLOB_CLI_STATE_NONE);

	PASS();
}

static void test_srv_lpn_pull(void)
{
	bt_mesh_test_cfg_set(NULL, 500);

	bt_mesh_test_friendship_init(1);

	srv_pull_mode_setup();

	/* This test is used to establish friendship with single lpn as well as
	 * with many lpn devices. If legacy advertiser is used friendship with
	 * many lpn devices is established normally due to bad precision of advertiser.
	 * If extended advertiser is used simultaneous lpn running causes the situation
	 * when Friend Request from several devices collide in emulated radio channel.
	 * This shift of start moment helps to avoid Friend Request collisions.
	 */
	k_sleep(K_MSEC(10 * get_device_nbr()));

	bt_mesh_lpn_set(true);

	ASSERT_OK_MSG(bt_mesh_test_friendship_evt_wait(BT_MESH_TEST_LPN_ESTABLISHED, K_SECONDS(5)),
		      "LPN not established");

	bt_mesh_blob_srv_recv(&blob_srv, 1, &blob_io, 0, 10);

	if (k_sem_take(&blob_srv_end_sem, K_SECONDS(750))) {
		FAIL("End CB did not trigger as expected for the srv");
	}

	ASSERT_TRUE(blob_srv.phase == BT_MESH_BLOB_XFER_PHASE_COMPLETE);

	/* Check that all blocks is received */
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 0));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 1));
	ASSERT_TRUE(atomic_test_bit(block_bitfield, 2));

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "blob_" #role "_" #name,            \
		.test_descr = description,                     \
		.test_args_f = test_args_parse,                \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_blob[] = {
	TEST_CASE(cli, caps_all_rsp, "Caps procedure: All responsive targets"),
	TEST_CASE(cli, caps_partial_rsp, "Caps procedure: Mixed response from targets"),
	TEST_CASE(cli, caps_no_rsp, "Caps procedure: No response from targets"),
	TEST_CASE(cli, caps_cancelled, "Caps procedure: Cancel caps"),
	TEST_CASE(cli, broadcast_basic, "Test basic broadcast API and CBs "),
	TEST_CASE(cli, broadcast_trans, "Test all broadcast transmission types"),
	TEST_CASE(cli, broadcast_unicast_seq, "Test broadcast with unicast addr (Sequential)"),
	TEST_CASE(cli, broadcast_unicast, "Test broadcast with unicast addr"),
	TEST_CASE(cli, trans_complete, "Transfer completes successfully on client (Default: Push)"),
	TEST_CASE(cli, trans_resume, "Resume BLOB transfer after srv suspension (Default: Push)"),
	TEST_CASE(cli, fail_on_persistency, "BLOB Client doesn't give up BLOB Transfer"),
	TEST_CASE(cli, trans_persistency_pull, "Test transfer persistency in Pull mode"),
	TEST_CASE(cli, fail_on_no_rsp, "BLOB Client end transfer if no targets rsp to Xfer Get"),
	TEST_CASE(cli, friend_pull, "BLOB Client on friend node completes transfer in pull mode"),

	TEST_CASE(srv, caps_standard, "Standard responsive blob server"),
	TEST_CASE(srv, caps_no_rsp, "Non-responsive blob server"),
	TEST_CASE(srv, trans_complete, "Transfer completes successfully on server"),
	TEST_CASE(srv, trans_resume, "Self suspending server after first received block"),
	TEST_CASE(srv, trans_persistency_pull, "Test transfer persistency in Pull mode"),
	TEST_CASE(srv, fail_on_block_start, "Server failing right before first block start msg"),
	TEST_CASE(srv, fail_on_block_get, "Server failing right before first block get msg"),
	TEST_CASE(srv, fail_on_xfer_get, "Server failing right before first xfer get msg"),
	TEST_CASE(srv, fail_on_nothing, "Non-failing server"),
	TEST_CASE(srv, lpn_pull, "BLOB Server on LPN completes transfer in pull mode"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_blob_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_blob);
	return tests;
}

#if CONFIG_BT_SETTINGS
static const struct bst_test_instance test_blob_pst[] = {
	TEST_CASE(cli, stop,
		  "Client expecting server to stop after reaching configured phase and continuing"),
	TEST_CASE(srv, stop, "Server stopping after reaching configured xfer phase"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_blob_pst_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_blob_pst);
	return tests;
}
#endif
