/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <assert.h>
#include <errno.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/bluetooth/mesh/cfg.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>
#include <app_keys.h>
#include <va.h>
#include <sar_cfg_internal.h>
#include <string.h>
#include "mesh/access.h"

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_mesh
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"
#include "dfu_slot.h"

#define CID_LOCAL 0x05F1
#define COMPANY_ID_LF 0x05F1
#define COMPANY_ID_NORDIC_SEMI 0x05F9

/* Health server data */
#define CUR_FAULTS_MAX 4
#define HEALTH_TEST_ID 0x00

static uint8_t cur_faults[CUR_FAULTS_MAX];
static uint8_t reg_faults[CUR_FAULTS_MAX * 2];

/* Provision node data */
static uint8_t net_key[16];
static uint16_t net_key_idx;
static uint8_t flags;
static uint32_t iv_index;
static uint16_t addr;
static uint8_t dev_key[16];
static uint8_t input_size;
static uint8_t pub_key[64];
static uint8_t priv_key[32];

/* Configured provisioning data */
static uint8_t dev_uuid[16];
static uint8_t static_auth[BTP_MESH_PROV_AUTH_MAX_LEN];
static uint8_t static_auth_size;

/* Vendor Model data */
#define VND_MODEL_ID_1 0x1234
static uint8_t vnd_app_key[16];
static uint16_t vnd_app_key_idx = 0x000f;

/* Model send data */
#define MODEL_BOUNDS_MAX 100

#if defined(CONFIG_BT_MESH_BLOB_SRV) || defined(CONFIG_BT_MESH_BLOB_CLI)
/* BLOB Model data*/
static uint8_t blob_rx_sum;
static bool blob_valid;
static const char *blob_data = "11111111111111111111111111111111";

static int blob_io_open(const struct bt_mesh_blob_io *io,
			const struct bt_mesh_blob_xfer *xfer,
			enum bt_mesh_blob_io_mode mode)
{
	blob_rx_sum = 0;
	blob_valid = true;
	return 0;
}

static int blob_chunk_wr(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	for (int i = 0; i < chunk->size; ++i) {
		blob_rx_sum += chunk->data[i];
		if (chunk->data[i] !=
		    blob_data[(i + chunk->offset) % strlen(blob_data)]) {
			blob_valid = false;
		}
	}

	return 0;
}

static int blob_chunk_rd(const struct bt_mesh_blob_io *io,
			 const struct bt_mesh_blob_xfer *xfer,
			 const struct bt_mesh_blob_block *block,
			 const struct bt_mesh_blob_chunk *chunk)
{
	for (int i = 0; i < chunk->size; ++i) {
		chunk->data[i] =
			blob_data[(i + chunk->offset) % strlen(blob_data)];
	}

	return 0;
}

static const struct bt_mesh_blob_io dummy_blob_io = {
	.open = blob_io_open,
	.rd = blob_chunk_rd,
	.wr = blob_chunk_wr,
};
#endif

#if defined(CONFIG_BT_MESH_DFD_SRV)
static const struct bt_mesh_dfu_slot *dfu_self_update_slot;

static bool is_self_update(struct bt_mesh_dfd_srv *srv)
{
	for (int i = 0; i < ARRAY_SIZE(srv->targets); i++) {
		if (bt_mesh_has_addr(srv->targets[i].blob.addr)) {
			return true;
		}
	}

	return false;
}

/* DFD Model data*/
static int dfd_srv_recv(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	LOG_DBG("Uploading new firmware image to the distributor.");

	*io = &dummy_blob_io;

	return 0;
}

static void dfd_srv_del(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot)
{
	LOG_DBG("Deleting the firmware image from the distributor.");
}

static int dfd_srv_send(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	LOG_DBG("Starting the firmware distribution.");

	*io = &dummy_blob_io;

	dfu_self_update_slot = NULL;

	if (is_self_update(srv)) {
		LOG_DBG("DFD server starts self-update...");
		dfu_self_update_slot = slot;
	}

	return 0;
}

#ifdef CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD
static struct {
	uint8_t uri[CONFIG_BT_MESH_DFU_URI_MAXLEN];
	uint8_t uri_len;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	uint8_t fwid_len;
	const struct bt_mesh_dfu_slot *slot;
	uint8_t progress;
	bool started;
} dfd_srv_oob_ctx;

static void oob_check_handler(struct k_work *work);
static K_WORK_DEFINE(oob_check, oob_check_handler);
static void oob_store_handler(struct k_work *work);
static K_WORK_DEFINE(oob_store, oob_store_handler);

static int dfd_srv_start_oob_upload(struct bt_mesh_dfd_srv *srv,
				    const struct bt_mesh_dfu_slot *slot,
				    const char *uri, uint8_t uri_len,
				    const uint8_t *fwid, uint16_t fwid_len)
{
	LOG_DBG("Start OOB Upload");

	memcpy(dfd_srv_oob_ctx.uri, uri, uri_len);
	dfd_srv_oob_ctx.uri_len = uri_len;
	memcpy(dfd_srv_oob_ctx.fwid, fwid, fwid_len);
	dfd_srv_oob_ctx.fwid_len = fwid_len;
	dfd_srv_oob_ctx.slot = slot;
	dfd_srv_oob_ctx.progress = 0;
	dfd_srv_oob_ctx.started = true;

	k_work_submit(&oob_check);

	return BT_MESH_DFD_SUCCESS;
}

static void dfd_srv_cancel_oob_upload(struct bt_mesh_dfd_srv *srv,
				      const struct bt_mesh_dfu_slot *slot)
{
	LOG_DBG("Cancel OOB Upload");

	dfd_srv_oob_ctx.started = false;
}

static uint8_t dfd_srv_oob_progress_get(struct bt_mesh_dfd_srv *srv,
					const struct bt_mesh_dfu_slot *slot)
{
	uint8_t progress;

	if (dfd_srv_oob_ctx.started) {
		progress = dfd_srv_oob_ctx.progress;

		dfd_srv_oob_ctx.progress = MIN(dfd_srv_oob_ctx.progress + 25, 99);

		if (dfd_srv_oob_ctx.progress == 99) {
			k_work_submit(&oob_store);
		}
	} else {
		progress = 0;
	}

	LOG_DBG("OOB Progress Get (%sstarted: %d %%)", dfd_srv_oob_ctx.started ? "" : "not ",
		progress);
	return progress;
}
#endif /* CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD */

static struct bt_mesh_dfd_srv_cb dfd_srv_cb = {
	.recv = dfd_srv_recv,
	.del = dfd_srv_del,
	.send = dfd_srv_send,
#ifdef CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD
	.start_oob_upload = dfd_srv_start_oob_upload,
	.cancel_oob_upload = dfd_srv_cancel_oob_upload,
	.oob_progress_get = dfd_srv_oob_progress_get,
#endif
};

static struct bt_mesh_dfd_srv dfd_srv = BT_MESH_DFD_SRV_INIT(&dfd_srv_cb);

#ifdef CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD
#define SUPPORTED_SCHEME "http"

static void oob_check_handler(struct k_work *work)
{
	uint8_t scheme[10];
	int i;
	int status;
	int err;

	for (i = 0; i < MIN(dfd_srv_oob_ctx.uri_len, sizeof(scheme)); i++) {
		if (IN_RANGE(dfd_srv_oob_ctx.uri[i], 48, 57) || /* DIGIT */
		    IN_RANGE(dfd_srv_oob_ctx.uri[i], 65, 90) || /* ALPHA UPPER CASE */
		    IN_RANGE(dfd_srv_oob_ctx.uri[i], 97, 122) || /* ALPHA LOWER CASE */
		    dfd_srv_oob_ctx.uri[i] == '.' ||
		    dfd_srv_oob_ctx.uri[i] == '+' ||
		    dfd_srv_oob_ctx.uri[i] == '-') {
			scheme[i] = dfd_srv_oob_ctx.uri[i];
		} else {
			break;
		}
	}

	if (i == dfd_srv_oob_ctx.uri_len || dfd_srv_oob_ctx.uri[i] != ':') {
		status = BT_MESH_DFD_ERR_URI_MALFORMED;
	} else if (i != strlen(SUPPORTED_SCHEME) ||
		   memcmp(scheme, SUPPORTED_SCHEME, strlen(SUPPORTED_SCHEME))) {
		status = BT_MESH_DFD_ERR_URI_NOT_SUPPORTED;
	} else {
		status = BT_MESH_DFD_SUCCESS;
	}

	err = bt_mesh_dfd_srv_oob_check_complete(&dfd_srv, dfd_srv_oob_ctx.slot, status,
						 dfd_srv_oob_ctx.fwid, dfd_srv_oob_ctx.fwid_len);
	LOG_DBG("OOB check completed (err %d)", err);
}

static void oob_store_handler(struct k_work *work)
{
	int err;

	err = bt_mesh_dfd_srv_oob_store_complete(&dfd_srv, dfd_srv_oob_ctx.slot, true,
						 10000, "metadata", 8);
	LOG_DBG("OOB store completed (err %d)", err);
}
#endif /* CONFIG_BT_MESH_DFD_SRV_OOB_UPLOAD */
#endif

#if defined(CONFIG_BT_MESH_BLOB_CLI) && !defined(CONFIG_BT_MESH_DFD_SRV)
static struct {
	struct bt_mesh_blob_cli_inputs inputs;
	struct bt_mesh_blob_target targets[32];
	struct bt_mesh_blob_target_pull pull[32];
	uint8_t target_count;
	struct bt_mesh_blob_xfer xfer;
} blob_cli_xfer;

static void blob_cli_lost_target(struct bt_mesh_blob_cli *cli,
				 struct bt_mesh_blob_target *target,
				 enum bt_mesh_blob_status reason)
{
	LOG_DBG("Mesh Blob: Lost target 0x%04x (reason: %u)", target->addr,
		reason);
	tester_event(BTP_SERVICE_ID_MESH, MESH_EV_BLOB_LOST_TARGET, NULL, 0);
}

static void blob_cli_caps(struct bt_mesh_blob_cli *cli,
			  const struct bt_mesh_blob_cli_caps *caps)
{
	const char *const modes[] = {
		"none",
		"push",
		"pull",
		"all",
	};

	if (!caps) {
		LOG_DBG("None of the targets can be used for BLOB transfer");
		return;
	}

	LOG_DBG("Mesh BLOB: capabilities:");
	LOG_DBG("\tMax BLOB size: %u bytes", caps->max_size);
	LOG_DBG("\tBlock size: %u-%u (%u-%u bytes)", caps->min_block_size_log,
		caps->max_block_size_log, 1 << caps->min_block_size_log,
		1 << caps->max_block_size_log);
	LOG_DBG("\tMax chunks: %u", caps->max_chunks);
	LOG_DBG("\tChunk size: %u", caps->max_chunk_size);
	LOG_DBG("\tMTU size: %u", caps->mtu_size);
	LOG_DBG("\tModes: %s", modes[caps->modes]);
}

static void blob_cli_end(struct bt_mesh_blob_cli *cli,
			 const struct bt_mesh_blob_xfer *xfer, bool success)
{
	if (success) {
		LOG_DBG("Mesh BLOB transfer complete.");
	} else {
		LOG_DBG("Mesh BLOB transfer failed.");
	}
}

static const struct bt_mesh_blob_cli_cb blob_cli_handlers = {
	.lost_target = blob_cli_lost_target,
	.caps = blob_cli_caps,
	.end = blob_cli_end,
};

static struct bt_mesh_blob_cli blob_cli = { .cb = &blob_cli_handlers };
#endif

#if defined(CONFIG_BT_MESH_DFU_SRV)
const char *metadata_data = "1100000000000011";

static uint8_t dfu_fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN] = {
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static struct bt_mesh_dfu_img dfu_imgs[] = { {
	.fwid = &dfu_fwid,
	.fwid_len = sizeof(dfu_fwid),
} };

static int dfu_meta_check(struct bt_mesh_dfu_srv *srv,
			  const struct bt_mesh_dfu_img *img,
			  struct net_buf_simple *metadata,
			  enum bt_mesh_dfu_effect *effect)
{
	char string[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	int i;
	size_t len;

	len = bin2hex(metadata->data, metadata->len, string, sizeof(string));
	string[len] = '\0';

	for (i = 0; i <= len; i++) {
		if (string[i] != metadata_data[i]) {
			LOG_ERR("Wrong Firmware Metadata");
			return -EINVAL;
		}
	}

	return 0;
}

static int dfu_start(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img,
		     struct net_buf_simple *metadata,
		     const struct bt_mesh_blob_io **io)
{
	LOG_DBG("DFU setup");

	*io = &dummy_blob_io;

	return 0;
}

static void dfu_end(struct bt_mesh_dfu_srv *srv,
		    const struct bt_mesh_dfu_img *img, bool success)
{
	if (!success) {
		LOG_ERR("DFU failed");
		return;
	}

	if (!blob_valid) {
		bt_mesh_dfu_srv_rejected(srv);
		return;
	}

	bt_mesh_dfu_srv_verified(srv);
}

static int dfu_apply(struct bt_mesh_dfu_srv *srv,
		     const struct bt_mesh_dfu_img *img)
{
	if (!blob_valid) {
		return -EINVAL;
	}

	LOG_DBG("Applying DFU transfer...");

#if defined(CONFIG_BT_MESH_DFD_SRV)
	if (is_self_update(&dfd_srv) && dfu_self_update_slot != NULL) {
		LOG_DBG("Swapping fwid for self-update");
		/* Simulate self-update by swapping fwid. */
		memcpy(&dfu_fwid[0], dfu_self_update_slot->fwid, dfu_self_update_slot->fwid_len);
		dfu_imgs[0].fwid_len = dfu_self_update_slot->fwid_len;
	}

#endif

	return 0;
}

static const struct bt_mesh_dfu_srv_cb dfu_handlers = {
	.check = dfu_meta_check,
	.start = dfu_start,
	.end = dfu_end,
	.apply = dfu_apply,
};

static struct bt_mesh_dfu_srv dfu_srv =
	BT_MESH_DFU_SRV_INIT(&dfu_handlers, dfu_imgs, ARRAY_SIZE(dfu_imgs));
#endif /* CONFIG_BT_MESH_DFU_SRV */

/* Model Authentication Method */
#define AUTH_METHOD_STATIC 0x01
#define AUTH_METHOD_OUTPUT 0x02
#define AUTH_METHOD_INPUT 0x03

static struct model_data {
	struct bt_mesh_model *model;
	uint16_t addr;
	uint16_t appkey_idx;
} model_bound[MODEL_BOUNDS_MAX];

static struct {
	uint16_t local;
	uint16_t dst;
	uint16_t net_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_mesh_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_MESH_CONFIG_PROVISIONING);
	tester_set_bit(rp->data, BTP_MESH_PROVISION_NODE);
	tester_set_bit(rp->data, BTP_MESH_INIT);
	tester_set_bit(rp->data, BTP_MESH_RESET);
	tester_set_bit(rp->data, BTP_MESH_INPUT_NUMBER);
	tester_set_bit(rp->data, BTP_MESH_INPUT_STRING);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_MESH_IVU_TEST_MODE);
	tester_set_bit(rp->data, BTP_MESH_IVU_TOGGLE_STATE);
	tester_set_bit(rp->data, BTP_MESH_NET_SEND);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_GENERATE_FAULTS);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_CLEAR_FAULTS);
	tester_set_bit(rp->data, BTP_MESH_LPN);
	tester_set_bit(rp->data, BTP_MESH_LPN_POLL);
	tester_set_bit(rp->data, BTP_MESH_MODEL_SEND);

	/* octet 2 */
#if defined(CONFIG_BT_TESTING)
	tester_set_bit(rp->data, BTP_MESH_LPN_SUBSCRIBE);
	tester_set_bit(rp->data, BTP_MESH_LPN_UNSUBSCRIBE);
	tester_set_bit(rp->data, BTP_MESH_RPL_CLEAR);
#endif /* CONFIG_BT_TESTING */
	tester_set_bit(rp->data, BTP_MESH_PROXY_IDENTITY);
	tester_set_bit(rp->data, BTP_MESH_COMP_DATA_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_BEACON_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_BEACON_SET);

	/* octet 3 */
	tester_set_bit(rp->data, BTP_MESH_CFG_DEFAULT_TTL_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_DEFAULT_TTL_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_GATT_PROXY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_GATT_PROXY_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_FRIEND_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_FRIEND_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_RELAY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_RELAY_SET);

	/* octet 4 */
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_PUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_PUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_ADD);

	/* octet 5 */
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_BIND);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_UNBIND);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_VND_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_PUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_PUB_GET);

	/* octet 6 */
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_SUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_SUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NET_TRANS_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NET_TRANS_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_OVW);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_DEL_ALL);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_GET_VND);

	/* octet 7 */
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_OVW);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_UPDATE);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_UPDATE);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_IDT_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_IDT_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_RESET);

	/* octet 8 */
	tester_set_bit(rp->data, BTP_MESH_CFG_LPN_TIMEOUT_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_BIND_VND);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_FAULT_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_FAULT_CLEAR);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_PERIOD_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_PERIOD_SET);

	/* octet 9 */
	tester_set_bit(rp->data, BTP_MESH_HEALTH_ATTENTION_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_ATTENTION_SET);
	tester_set_bit(rp->data, BTP_MESH_PROVISION_ADV);
	tester_set_bit(rp->data, BTP_MESH_CFG_KRP_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_KRP_SET);
	tester_set_bit(rp->data, BTP_MESH_VA_ADD);
	tester_set_bit(rp->data, BTP_MESH_VA_DEL);

	*rsp_len = sizeof(*rp) + 10;

	return BTP_STATUS_SUCCESS;
}

static void get_faults(uint8_t *faults, uint8_t faults_size, uint8_t *dst, uint8_t *count)
{
	uint8_t i, limit = *count;

	for (i = 0U, *count = 0U; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int fault_get_cur(struct bt_mesh_model *model, uint8_t *test_id,
			 uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
	LOG_DBG("");

	*test_id = HEALTH_TEST_ID;
	*company_id = CID_LOCAL;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t company_id,
			 uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	*test_id = HEALTH_TEST_ID;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	LOG_DBG("test_id 0x%02x company_id 0x%04x", test_id, company_id);

	if (company_id != CID_LOCAL || test_id != HEALTH_TEST_ID) {
		return -EINVAL;
	}

	return 0;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
};

static void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x: no faults",
			test_id, cid);
		return;
	}

	LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu: ",
		test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		LOG_DBG("0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				  uint8_t test_id, uint16_t cid, uint8_t *faults,
				  size_t fault_count)
{
	LOG_DBG("Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
static struct bt_mesh_large_comp_data_cli lcd_cli = {
};
#endif

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};


#ifdef CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
static uint8_t health_tests[] = {
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_LF, 6, 0x01, 0x02, 0x03, 0x04, 0x34,
				 0x15),
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_NORDIC_SEMI, 3, 0x01, 0x02, 0x03),
};

static uint8_t zero_metadata[100];

static struct bt_mesh_models_metadata_entry health_srv_meta[] = {
	BT_MESH_HEALTH_TEST_INFO_METADATA(health_tests),
	{
		.len = ARRAY_SIZE(zero_metadata),
		.id = 0xABCD,
		.data = zero_metadata,
	},
	BT_MESH_MODELS_METADATA_END,
};

static uint8_t health_tests_alt[] = {
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_LF, 6, 0x11, 0x22, 0x33, 0x44, 0x55,
				 0x66),
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_NORDIC_SEMI, 3, 0x11, 0x22, 0x33),
};

static struct bt_mesh_models_metadata_entry health_srv_meta_alt[] = {
	BT_MESH_HEALTH_TEST_INFO_METADATA(health_tests_alt),
	{
		.len = ARRAY_SIZE(zero_metadata),
		.id = 0xFEED,
		.data = zero_metadata,
	},
	BT_MESH_MODELS_METADATA_END,
};
#endif

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
#ifdef CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
	.metadata = health_srv_meta,
#endif
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);

static struct bt_mesh_cfg_cli cfg_cli = {
};

#if defined(CONFIG_BT_MESH_SAR_CFG_CLI)
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;
#endif

#if defined(CONFIG_BT_MESH_RPR_CLI)
static void rpr_scan_report(struct bt_mesh_rpr_cli *cli,
			    const struct bt_mesh_rpr_node *srv,
			    struct bt_mesh_rpr_unprov *unprov,
			    struct net_buf_simple *adv_data)
{
	char uuid_hex_str[32 + 1];

	bin2hex(unprov->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	LOG_DBG("Server 0x%04x:\n"
		    "\tuuid:   %s\n"
		    "\tOOB:    0x%04x",
		    srv->addr, uuid_hex_str, unprov->oob);

	while (adv_data && adv_data->len > 2) {
		uint8_t len, type;
		uint8_t data[31];

		len = net_buf_simple_pull_u8(adv_data) - 1;
		type = net_buf_simple_pull_u8(adv_data);
		memcpy(data, net_buf_simple_pull_mem(adv_data, len), len);
		data[len] = '\0';

		if (type == BT_DATA_URI) {
			LOG_DBG("\tURI:    \"\\x%02x%s\"",
				data[0], &data[1]);
		} else if (type == BT_DATA_NAME_COMPLETE) {
			LOG_DBG("\tName:   \"%s\"", data);
		} else {
			char string[64 + 1];

			bin2hex(data, len, string, sizeof(string));
			LOG_DBG("\t0x%02x:  %s", type, string);
		}
	}
}

static struct bt_mesh_rpr_cli rpr_cli = {
	.scan_report = rpr_scan_report,
};
#endif

#if defined(CONFIG_BT_MESH_DFU_SRV)
static uint8_t dfu_srv_apply(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("Applying image on server");
	bt_mesh_dfu_srv_applied(&dfu_srv);
	return BTP_STATUS_SUCCESS;
}
#endif

#ifdef CONFIG_BT_MESH_PRIV_BEACON_CLI
static struct bt_mesh_priv_beacon_cli priv_beacon_cli;

static uint8_t priv_beacon_get(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_beacon_get_cmd *cp = cmd;

	struct bt_mesh_priv_beacon val;
	int err;

	err = bt_mesh_priv_beacon_cli_get(net.net_idx, cp->dst, &val);
	if (err) {
		LOG_ERR("Failed to send Private Beacon Get (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Private Beacon state: %u, %u", val.enabled, val.rand_interval);
	return BTP_STATUS_SUCCESS;
}

static uint8_t priv_beacon_set(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_beacon_set_cmd *cp = cmd;
	struct bt_mesh_priv_beacon val;
	int err;

	val.enabled = cp->enabled;
	val.rand_interval = cp->rand_interval;

	err = bt_mesh_priv_beacon_cli_set(net.net_idx, cp->dst, &val);
	if (err) {
		LOG_ERR("Failed to send Private Beacon Set (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t priv_gatt_proxy_get(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_gatt_proxy_get_cmd *cp = cmd;

	uint8_t state;
	int err;

	err = bt_mesh_priv_beacon_cli_gatt_proxy_get(net.net_idx, cp->dst, &state);
	if (err) {
		LOG_ERR("Failed to send Private GATT Proxy Get (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Private GATT Proxy state: %u", state);
	return BTP_STATUS_SUCCESS;
}

static uint8_t priv_gatt_proxy_set(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_gatt_proxy_set_cmd *cp = cmd;

	uint8_t state;
	int err;

	state = cp->state;

	err = bt_mesh_priv_beacon_cli_gatt_proxy_set(net.net_idx, cp->dst, &state);
	if (err) {
		LOG_ERR("Failed to send Private GATT Proxy Set (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t priv_node_id_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_node_id_get_cmd *cp = cmd;
	struct bt_mesh_priv_node_id val;
	uint16_t key_net_idx;
	int err;

	key_net_idx = cp->key_net_idx;

	err = bt_mesh_priv_beacon_cli_node_id_get(net.net_idx, cp->dst, key_net_idx, &val);
	if (err) {
		LOG_ERR("Failed to send Private Node Identity Get (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Private Node Identity state: %u %u %u", val.net_idx, val.state, val.status);
	return BTP_STATUS_SUCCESS;
}

static uint8_t priv_node_id_set(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_priv_node_id_set_cmd *cp = cmd;
	struct bt_mesh_priv_node_id val;
	int err;

	val.net_idx = cp->net_idx;
	val.state = cp->state;

	err = bt_mesh_priv_beacon_cli_node_id_set(net.net_idx, cp->dst, &val);
	if (err) {
		LOG_ERR("Failed to send Private Node Identity Set (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t proxy_private_identity_enable(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_proxy_private_identity_enable();
	if (err) {
		LOG_ERR("Failed to enable proxy private identity (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
static struct bt_mesh_sol_pdu_rpl_cli srpl_cli;
#endif


#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
static struct bt_mesh_od_priv_proxy_cli od_priv_proxy_cli;

static uint8_t od_priv_proxy_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_od_priv_proxy_get_cmd *cp = cmd;
	uint8_t val_rsp;
	int err;

	LOG_DBG("");

	err = bt_mesh_od_priv_proxy_cli_get(net.net_idx, cp->dst, &val_rsp);
	if (err) {
		LOG_ERR("Failed to get On-Demand Private Proxy state (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("On-Demand Private Proxy state: %u", val_rsp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t od_priv_proxy_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_od_priv_proxy_set_cmd *cp = cmd;
	uint8_t val_rsp;
	int err;

	LOG_DBG("");

	err = bt_mesh_od_priv_proxy_cli_set(net.net_idx, cp->dst, cp->val, &val_rsp);
	if (err) {
		LOG_ERR("Failed to set On-Demand Private Proxy state (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("On-Demand Private Proxy set state: %u", val_rsp);

	return BTP_STATUS_SUCCESS;
}

#endif

#if defined(CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
static uint8_t srpl_clear(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_srpl_clear_cmd *cp = cmd;
	uint16_t app_idx = BT_MESH_KEY_UNUSED;
	uint16_t start_rsp;
	uint8_t len_rsp;
	int err;

	/* Lookup source address */
	for (int i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model->id == BT_MESH_MODEL_ID_SOL_PDU_RPL_CLI) {
			app_idx = model_bound[i].appkey_idx;
			break;
		}
	}

	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT_APP(app_idx, cp->dst);

	if (cp->acked) {
		err = bt_mesh_sol_pdu_rpl_clear(&ctx, cp->range_start, cp->range_len, &start_rsp,
						&len_rsp);
	} else {
		err = bt_mesh_sol_pdu_rpl_clear_unack(&ctx, cp->range_start, cp->range_len);
	}
	if (err) {
		LOG_ERR("Failed to clear SRPL (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_PROXY_SOLICITATION)
static uint8_t proxy_solicit(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_proxy_solicit_cmd *cp = cmd;
	int err;

	err = bt_mesh_proxy_solicit(cp->net_idx);
	if (err) {
		LOG_ERR("Failed to advertise solicitation PDU (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_MESH_PROXY_SOLICITATION */

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL_HEALTH_CLI(&health_cli),
#if defined(CONFIG_BT_MESH_SAR_CFG_SRV)
	BT_MESH_MODEL_SAR_CFG_SRV,
#endif
#if defined(CONFIG_BT_MESH_SAR_CFG_CLI)
	BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
#endif
#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
	BT_MESH_MODEL_LARGE_COMP_DATA_SRV,
#endif
#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
	BT_MESH_MODEL_LARGE_COMP_DATA_CLI(&lcd_cli),
#endif
#if defined(CONFIG_BT_MESH_OP_AGG_SRV)
	BT_MESH_MODEL_OP_AGG_SRV,
#endif
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
	BT_MESH_MODEL_OP_AGG_CLI,
#endif
#if defined(CONFIG_BT_MESH_RPR_CLI)
	BT_MESH_MODEL_RPR_CLI(&rpr_cli),
#endif
#if defined(CONFIG_BT_MESH_RPR_SRV)
	BT_MESH_MODEL_RPR_SRV,
#endif
#if defined(CONFIG_BT_MESH_DFD_SRV)
	BT_MESH_MODEL_DFD_SRV(&dfd_srv),
#endif
#if defined(CONFIG_BT_MESH_DFU_SRV)
	BT_MESH_MODEL_DFU_SRV(&dfu_srv),
#endif
#if defined(CONFIG_BT_MESH_BLOB_CLI) && !defined(CONFIG_BT_MESH_DFD_SRV)
	BT_MESH_MODEL_BLOB_CLI(&blob_cli),
#endif
#if defined(CONFIG_BT_MESH_PRIV_BEACON_SRV)
	BT_MESH_MODEL_PRIV_BEACON_SRV,
#endif
#if defined(CONFIG_BT_MESH_PRIV_BEACON_CLI)
	BT_MESH_MODEL_PRIV_BEACON_CLI(&priv_beacon_cli),
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
	BT_MESH_MODEL_OD_PRIV_PROXY_CLI(&od_priv_proxy_cli),
#endif
#if defined(CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
	BT_MESH_MODEL_SOL_PDU_RPL_CLI(&srpl_cli),
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	BT_MESH_MODEL_OD_PRIV_PROXY_SRV,
#endif

};
struct model_data *lookup_model_bound(uint16_t id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model && model_bound[i].model->id == id) {
			return &model_bound[i];
		}
	}

	return NULL;
}
static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CID_LOCAL, VND_MODEL_ID_1, BT_MESH_MODEL_NO_OPS, NULL,
			  NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	struct btp_mesh_prov_link_open_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_GATT;
		break;
	case BT_MESH_PROV_REMOTE:
		ev.bearer = BTP_MESH_PROV_BEARER_REMOTE;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_LINK_OPEN, &ev, sizeof(ev));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	struct btp_mesh_prov_link_closed_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_GATT;
		break;
	case BT_MESH_PROV_REMOTE:
		ev.bearer = BTP_MESH_PROV_BEARER_REMOTE;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_LINK_CLOSED, &ev, sizeof(ev));
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct btp_mesh_out_number_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%08x", action, number);

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_OUT_NUMBER_ACTION, &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	struct btp_mesh_out_string_action_ev *ev;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	LOG_DBG("str %s", str);

	net_buf_simple_init(buf, 0);

	ev = net_buf_simple_add(buf, sizeof(*ev));
	ev->string_len = strlen(str);

	net_buf_simple_add_mem(buf, str, ev->string_len);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_OUT_STRING_ACTION,
		    buf->data, buf->len);

	return 0;
}

static int input(bt_mesh_input_action_t action, uint8_t size)
{
	struct btp_mesh_in_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%02x", action, size);

	input_size = size;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_IN_ACTION, &ev, sizeof(ev));

	return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	LOG_DBG("net_idx 0x%04x addr 0x%04x", net_idx, addr);

	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROVISIONED, NULL, 0);
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	struct btp_mesh_prov_node_added_ev ev;

	LOG_DBG("net_idx 0x%04x addr 0x%04x num_elem %d", net_idx, addr,
		num_elem);

	ev.net_idx = net_idx;
	ev.addr = addr;
	ev.num_elems = num_elem;
	memcpy(&ev.uuid, uuid, sizeof(ev.uuid));

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_NODE_ADDED, &ev, sizeof(ev));
}

static void prov_reset(void)
{
	LOG_DBG("");

	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV)) {
		bt_mesh_prov_enable(BT_MESH_PROV_REMOTE);
	}
}

static const struct bt_mesh_comp comp = {
	.cid = CID_LOCAL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
	.vid = 1,
};

static const struct bt_mesh_comp comp_alt = {
	.cid = CID_LOCAL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
	.vid = 2,
};

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.static_val = static_auth,
	.static_val_len = sizeof(static_auth),
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.node_added = prov_node_added,
	.reset = prov_reset,
	.uri = "Tester",
};

static uint8_t config_prov(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_config_provisioning_cmd *cp = cmd;
	const struct btp_mesh_config_provisioning_cmd_v2 *cp2 = cmd;
	int err = 0;

	/* TODO consider fix BTP commands to avoid this */
	if (cmd_len != sizeof(*cp) && cmd_len != (sizeof(*cp2))) {
		LOG_DBG("wrong cmd size");
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("");

	static_auth_size = cp->static_auth_size;

	if (static_auth_size > BTP_MESH_PROV_AUTH_MAX_LEN || static_auth_size == 0) {
		LOG_DBG("wrong static auth length");
		return BTP_STATUS_FAILED;
	}

	memcpy(dev_uuid, cp->uuid, sizeof(dev_uuid));
	memcpy(static_auth, cp->static_auth, cp->static_auth_size);

	prov.output_size = cp->out_size;
	prov.output_actions = sys_le16_to_cpu(cp->out_actions);
	prov.input_size = cp->in_size;
	prov.input_actions = sys_le16_to_cpu(cp->in_actions);

	if (cmd_len == sizeof(*cp2)) {
		memcpy(pub_key, cp2->set_pub_key, sizeof(cp2->set_pub_key));
		memcpy(priv_key, cp2->set_priv_key, sizeof(cp2->set_priv_key));
		prov.public_key_be = pub_key;
		prov.private_key_be = priv_key;
	}

	if (cp->auth_method == AUTH_METHOD_OUTPUT) {
		err = bt_mesh_auth_method_set_output(prov.output_actions, prov.output_size);
	} else if (cp->auth_method == AUTH_METHOD_INPUT) {
		err = bt_mesh_auth_method_set_input(prov.input_actions, prov.input_size);
	} else if (cp->auth_method == AUTH_METHOD_STATIC) {
		err = bt_mesh_auth_method_set_static(static_auth, static_auth_size);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t provision_node(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_provision_node_cmd *cp = cmd;
	const struct btp_mesh_provision_node_cmd_v2 *cp2 = cmd;
	int err;

	/* TODO consider fix BTP commands to avoid this */
	if (cmd_len != sizeof(*cp) && cmd_len != (sizeof(*cp2))) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("");

	memcpy(dev_key, cp->dev_key, sizeof(dev_key));
	memcpy(net_key, cp->net_key, sizeof(net_key));

	addr = sys_le16_to_cpu(cp->addr);
	flags = cp->flags;
	iv_index = sys_le32_to_cpu(cp->iv_index);
	net_key_idx = sys_le16_to_cpu(cp->net_key_idx);

	if (cmd_len == sizeof(*cp2)) {
		memcpy(pub_key, cp2->pub_key, sizeof(pub_key));

		err = bt_mesh_prov_remote_pub_key_set(pub_key);
		if (err) {
			LOG_ERR("err %d", err);
			return BTP_STATUS_FAILED;
		}
	}
#if defined(CONFIG_BT_MESH_PROVISIONER)
	err = bt_mesh_cdb_create(net_key);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}
#endif
	err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index, addr,
				dev_key);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t provision_adv(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_provision_adv_cmd *cp = cmd;
	int err;

	LOG_DBG("");

	err = bt_mesh_provision_adv(cp->uuid, sys_le16_to_cpu(cp->net_idx),
				    sys_le16_to_cpu(cp->address),
				    cp->attention_duration);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t init(const void *cmd, uint16_t cmd_len,
		    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_init_cmd *cp = cmd;
	int err;

	if (cp->comp == 0) {
		LOG_WRN("Loading default comp data");
		err = bt_mesh_init(&prov, &comp);
	} else {
		LOG_WRN("Loading alternative comp data");
#ifdef CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
		health_srv.metadata = health_srv_meta_alt;
#endif
		err = bt_mesh_init(&prov, &comp_alt);
	}

	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t start(const void *cmd, uint16_t cmd_len,
		     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	if (addr) {
		err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index,
					addr, dev_key);
		if (err && err != -EALREADY) {
			return BTP_STATUS_FAILED;
		}
	} else {
		err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
		if (err && err != -EALREADY) {
			return BTP_STATUS_FAILED;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV)) {
		err = bt_mesh_prov_enable(BT_MESH_PROV_REMOTE);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t reset(const void *cmd, uint16_t cmd_len,
		     void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("");

	bt_mesh_reset();

	return BTP_STATUS_SUCCESS;
}

static uint8_t input_number(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_input_number_cmd *cp = cmd;
	uint32_t number;
	int err;

	number = sys_le32_to_cpu(cp->number);

	LOG_DBG("number 0x%04x", number);

	err = bt_mesh_input_number(number);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t input_string(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_input_string_cmd *cp = cmd;
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("");

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->string_len)) {
		return BTP_STATUS_FAILED;
	}

	/* for historical reasons this commands must send NULL terminated
	 * string
	 */
	if (cp->string[cp->string_len] != '\0') {
		return BTP_STATUS_FAILED;
	}

	if (strlen(cp->string) < input_size) {
		LOG_ERR("Too short input (%u chars required)", input_size);
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_input_string(cp->string);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ivu_test_mode(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_ivu_test_mode_cmd *cp = cmd;

	LOG_DBG("enable 0x%02x", cp->enable);

	bt_mesh_iv_update_test(cp->enable ? true : false);

	return BTP_STATUS_SUCCESS;
}

static uint8_t ivu_toggle_state(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	bool result;

	LOG_DBG("");

	result = bt_mesh_iv_update();
	if (!result) {
		LOG_ERR("Failed to toggle the IV Update state");
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn(const void *cmd, uint16_t cmd_len,
		   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_set_cmd *cp = cmd;
	int err;

	LOG_DBG("enable 0x%02x", cp->enable);

	err = bt_mesh_lpn_set(cp->enable ? true : false);
	if (err) {
		LOG_ERR("Failed to toggle LPN (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn_poll(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_lpn_poll();
	if (err) {
		LOG_ERR("Failed to send poll msg (err %d)", err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t net_send(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_net_send_cmd *cp = cmd;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	int err;

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->payload_len)) {
		return BTP_STATUS_FAILED;
	}

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = vnd_app_key_idx,
		.addr = sys_le16_to_cpu(cp->dst),
		.send_ttl = cp->ttl,
	};

	if (BT_MESH_ADDR_IS_VIRTUAL(ctx.addr)) {
		ctx.uuid = bt_mesh_va_uuid_get(ctx.addr, NULL, NULL);
	}

	LOG_DBG("ttl 0x%02x dst 0x%04x payload_len %d", ctx.send_ttl,
		ctx.addr, cp->payload_len);

	if (!bt_mesh_app_key_exists(vnd_app_key_idx)) {
		(void)bt_mesh_app_key_add(vnd_app_key_idx, net.net_idx,
					  vnd_app_key);
		vnd_models[0].keys[0] = vnd_app_key_idx;
	}

	net_buf_simple_add_mem(&msg, cp->payload, cp->payload_len);

	err = bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t va_add(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_va_add_cmd *cp = cmd;
	struct btp_mesh_va_add_rp *rp = rsp;
	const struct bt_mesh_va *va;
	int err;

	err = bt_mesh_va_add(cp->label_uuid, &va);
	if (err) {
		LOG_ERR("Failed to add Label UUID (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	rp->addr = sys_cpu_to_le16(va->addr);
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t va_del(const void *cmd, uint16_t cmd_len,
		      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_va_del_cmd *cp = cmd;
	const struct bt_mesh_va *va;
	int err;

	va = bt_mesh_va_find(cp->label_uuid);
	if (!va) {
		LOG_ERR("Failed to find Label UUID");
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_va_del(va->uuid);
	if (err) {
		LOG_ERR("Failed to delete Label UUID (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_generate_faults(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_mesh_health_generate_faults_rp *rp = rsp;
	uint8_t some_faults[] = { 0x01, 0x02, 0x03, 0xff, 0x06 };
	uint8_t cur_faults_count;
	uint8_t reg_faults_count;

	cur_faults_count = MIN(sizeof(cur_faults), sizeof(some_faults));
	memcpy(cur_faults, some_faults, cur_faults_count);
	memcpy(rp->current_faults, cur_faults, cur_faults_count);
	rp->cur_faults_count = cur_faults_count;

	reg_faults_count = MIN(sizeof(reg_faults), sizeof(some_faults));
	memcpy(reg_faults, some_faults, reg_faults_count);
	memcpy(rp->registered_faults + cur_faults_count, reg_faults, reg_faults_count);
	rp->reg_faults_count = reg_faults_count;

	bt_mesh_health_srv_fault_update(&elements[0]);

	*rsp_len = sizeof(*rp) + cur_faults_count + reg_faults_count;

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_clear_faults(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("");

	(void)memset(cur_faults, 0, sizeof(cur_faults));
	(void)memset(reg_faults, 0, sizeof(reg_faults));

	bt_mesh_health_srv_fault_update(&elements[0]);

	return BTP_STATUS_SUCCESS;
}

static uint8_t model_send(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_model_send_cmd *cp = cmd;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	struct bt_mesh_model *model = NULL;
	uint16_t src;
	int err;

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->payload_len)) {
		return BTP_STATUS_FAILED;
	}

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = sys_le16_to_cpu(cp->dst),
		.send_ttl = cp->ttl,
	};

	if (BT_MESH_ADDR_IS_VIRTUAL(ctx.addr)) {
		ctx.uuid = bt_mesh_va_uuid_get(ctx.addr, NULL, NULL);
	}

	src = sys_le16_to_cpu(cp->src);

	/* Lookup source address */
	for (int i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (bt_mesh_model_elem(model_bound[i].model)->addr == src) {
			model = model_bound[i].model;
			ctx.app_idx = model_bound[i].appkey_idx;

			break;
		}
	}

	if (!model) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("src 0x%04x dst 0x%04x model %p payload_len %d", src,
		ctx.addr, model, cp->payload_len);

	net_buf_simple_add_mem(&msg, cp->payload, cp->payload_len);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_TESTING)
static uint8_t lpn_subscribe(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_subscribe_cmd *cp = cmd;
	uint16_t address = sys_le16_to_cpu(cp->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_add(address);
	if (err) {
		LOG_ERR("Failed to subscribe (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn_unsubscribe(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_unsubscribe_cmd *cp = cmd;
	uint16_t address = sys_le16_to_cpu(cp->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_remove(&address, 1);
	if (err) {
		LOG_ERR("Failed to unsubscribe (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpl_clear(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_test_mesh_rpl_clear();
	if (err) {
		LOG_ERR("Failed to clear RPL (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_TESTING */

static uint8_t proxy_identity_enable(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		LOG_ERR("Failed to enable proxy identity (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_MESH_PROXY_CLIENT)
static uint8_t proxy_connect(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_proxy_connect_cmd *cp = cmd;
	int err;

	err = bt_mesh_proxy_connect(cp->net_idx);
	if (err) {
		LOG_ERR("Failed to connect to GATT Proxy (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_SAR_CFG_CLI)
static uint8_t sar_transmitter_get(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_sar_transmitter_get_cmd *cp = cmd;
	struct bt_mesh_sar_tx tx_rsp;
	int err;

	LOG_DBG("");

	bt_mesh_sar_cfg_cli_timeout_set(5000);

	err = bt_mesh_sar_cfg_cli_transmitter_get(
		net_key_idx, sys_le16_to_cpu(cp->dst), &tx_rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t sar_transmitter_set(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_sar_transmitter_set_cmd *cp = cmd;
	struct bt_mesh_sar_tx set, tx_rsp;
	int err;

	LOG_DBG("");

	bt_mesh_sar_cfg_cli_timeout_set(5000);

	set.seg_int_step = cp->tx.seg_int_step;
	set.unicast_retrans_count = cp->tx.unicast_retrans_count;
	set.unicast_retrans_int_inc = cp->tx.unicast_retrans_int_inc;
	set.unicast_retrans_int_step = cp->tx.unicast_retrans_int_step;
	set.unicast_retrans_without_prog_count =
		cp->tx.unicast_retrans_without_prog_count;
	set.multicast_retrans_count = cp->tx.multicast_retrans_count;
	set.multicast_retrans_int = cp->tx.multicast_retrans_int;

	err = bt_mesh_sar_cfg_cli_transmitter_set(net_key_idx,
						  sys_le16_to_cpu(cp->dst),
						  &set, &tx_rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t sar_receiver_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_sar_receiver_get_cmd *cp = cmd;
	struct bt_mesh_sar_rx rx_rsp;
	int err;

	LOG_DBG("");

	err = bt_mesh_sar_cfg_cli_receiver_get(net_key_idx,
					       sys_le16_to_cpu(cp->dst), &rx_rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t sar_receiver_set(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_sar_receiver_set_cmd *cp = cmd;
	struct bt_mesh_sar_rx set, rx_rsp;
	int err;

	LOG_DBG("");

	set.ack_delay_inc = cp->rx.ack_delay_inc;
	set.ack_retrans_count = cp->rx.ack_retrans_count;
	set.discard_timeout = cp->rx.discard_timeout;
	set.seg_thresh = cp->rx.seg_thresh;
	set.rx_seg_int_step = cp->rx.rx_seg_int_step;

	err = bt_mesh_sar_cfg_cli_receiver_set(net_key_idx,
					       sys_le16_to_cpu(cp->dst), &set,
					       &rx_rsp);
	if (err) {
		LOG_ERR("err=%d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
static uint8_t large_comp_data_get(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_large_comp_data_get_cmd *cp = cmd;
	struct btp_mesh_large_comp_data_get_rp *rp = rsp;
	int err;

	NET_BUF_SIMPLE_DEFINE(data, 500);
	net_buf_simple_init(&data, 0);

	struct bt_mesh_large_comp_data_rsp comp = {
		.data = &data,
	};

	err = bt_mesh_large_comp_data_get(sys_le16_to_cpu(cp->net_idx),
				    sys_le16_to_cpu(cp->addr), cp->page,
				    sys_le16_to_cpu(cp->offset), &comp);
	if (err) {
		LOG_ERR("Large Composition Data Get failed (err %d)", err);

		return BTP_STATUS_FAILED;
	}

	memcpy(rp->data, comp.data->data, comp.data->len);
	*rsp_len = comp.data->len;

	return BTP_STATUS_SUCCESS;
}

static uint8_t models_metadata_get(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_models_metadata_get_cmd *cp = cmd;
	struct btp_mesh_models_metadata_get_rp *rp = rsp;
	int err;

	NET_BUF_SIMPLE_DEFINE(data, 500);
	net_buf_simple_init(&data, 0);

	struct bt_mesh_large_comp_data_rsp metadata = {
		.data = &data,
	};

	err = bt_mesh_models_metadata_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->addr), cp->page,
					  sys_le16_to_cpu(cp->offset), &metadata);

	if (err) {
		LOG_ERR("Models Metadata Get failed (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	memcpy(rp->data, metadata.data->data, metadata.data->len);
	*rsp_len = metadata.data->len;

	return BTP_STATUS_SUCCESS;
}
#endif

static uint8_t composition_data_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_comp_data_get_cmd *cp = cmd;
	struct btp_mesh_comp_data_get_rp *rp = rsp;
	uint8_t page;
	struct net_buf_simple *comp = NET_BUF_SIMPLE(128);
	int err;

	LOG_DBG("");

	bt_mesh_cfg_cli_timeout_set(10 * MSEC_PER_SEC);

	net_buf_simple_init(comp, 0);

	err = bt_mesh_cfg_cli_comp_data_get(sys_le16_to_cpu(cp->net_idx),
					    sys_le16_to_cpu(cp->address),
					    cp->page, &page, comp);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	memcpy(rp->data, comp->data, comp->len);
	*rsp_len = comp->len;

	return BTP_STATUS_SUCCESS;
}

static uint8_t change_prepare(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_comp_change_prepare();
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}

#if CONFIG_BT_MESH_LARGE_COMP_DATA_SRV
	err = bt_mesh_models_metadata_change_prepare();
	if (err < 0) {
		return BTP_STATUS_FAILED;
	}
#endif

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_krp_get(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_krp_get_cmd *cp = cmd;
	struct btp_mesh_cfg_krp_get_rp *rp = rsp;
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_get(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address),
				      sys_le16_to_cpu(cp->key_net_idx),
				      &status, &phase);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->phase = phase;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_krp_set(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_krp_set_cmd *cp = cmd;
	struct btp_mesh_cfg_krp_set_rp *rp = rsp;
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_set(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address),
				      sys_le16_to_cpu(cp->key_net_idx),
				      cp->transition, &status, &phase);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->phase = phase;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_beacon_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_beacon_get_cmd *cp = cmd;
	struct btp_mesh_cfg_beacon_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address), &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_beacon_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_beacon_set_cmd *cp = cmd;
	struct btp_mesh_cfg_beacon_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address), cp->val,
					 &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_default_ttl_get(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_default_ttl_get_cmd *cp = cmd;
	struct btp_mesh_cfg_default_ttl_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_ttl_get(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address), &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_default_ttl_set(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_default_ttl_set_cmd *cp = cmd;
	struct btp_mesh_cfg_default_ttl_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_ttl_set(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address), cp->val,
				      &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_gatt_proxy_get(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_gatt_proxy_get_cmd *cp = cmd;
	struct btp_mesh_cfg_gatt_proxy_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_gatt_proxy_get(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_gatt_proxy_set(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_gatt_proxy_set_cmd *cp = cmd;
	struct btp_mesh_cfg_gatt_proxy_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_gatt_proxy_set(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     cp->val, &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_friend_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_friend_get_cmd *cp = cmd;
	struct btp_mesh_cfg_friend_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_friend_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_friend_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_friend_set_cmd *cp = cmd;
	struct btp_mesh_cfg_friend_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_friend_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 cp->val, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_relay_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_relay_get_cmd *cp = cmd;
	struct btp_mesh_cfg_relay_get_rp *rp = rsp;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_relay_get(sys_le16_to_cpu(cp->net_idx),
					sys_le16_to_cpu(cp->address), &status,
					&transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_relay_set(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_relay_set_cmd *cp = cmd;
	struct btp_mesh_cfg_relay_set_rp *rp = rsp;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_relay_set(sys_le16_to_cpu(cp->net_idx),
					sys_le16_to_cpu(cp->address),
					cp->new_relay, cp->new_transmit,
					&status, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_get(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_get_rp *rp = rsp;
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_pub_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_set(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_set_rp *rp = rsp;
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	LOG_DBG("");

	pub.addr = sys_le16_to_cpu(cp->pub_addr);
	pub.uuid = NULL;
	pub.app_idx = sys_le16_to_cpu(cp->app_idx);
	pub.cred_flag = cp->cred_flag;
	pub.ttl = cp->ttl;
	pub.period = cp->period;
	pub.transmit = cp->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_va_set(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_va_set_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_va_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_mod_pub pub;
	int err;

	LOG_DBG("");

	pub.uuid = cp->uuid;
	pub.app_idx = sys_le16_to_cpu(cp->app_idx);
	pub.cred_flag = cp->cred_flag;
	pub.ttl = cp->ttl;
	pub.period = cp->period;
	pub.transmit = cp->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_add(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_add_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->sub_addr),
					  sys_le16_to_cpu(cp->model_id),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_ovw(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_ovw_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_overwrite(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->elem_address),
						sys_le16_to_cpu(cp->sub_addr),
						sys_le16_to_cpu(cp->model_id),
						&status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_del(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_del_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->sub_addr),
					  sys_le16_to_cpu(cp->model_id),
					  &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_del_all(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_del_all_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_del_all_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del_all(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->model_id),
					      &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_get(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_get_rp *rp = rsp;
	uint8_t status;
	int16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &status, &subs, &sub_cn);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_get_vnd(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_get_vnd_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_get_vnd_rp *rp = rsp;
	uint8_t status;
	uint16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get_vnd(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->model_id),
					      sys_le16_to_cpu(cp->cid),
					      &status, &subs, &sub_cn);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_add(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_add_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_add_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_add(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->uuid),
					     sys_le16_to_cpu(cp->model_id),
					     &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_del(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_del_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_del_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_del(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->uuid),
					     sys_le16_to_cpu(cp->model_id),
					     &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_ovw(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_ovw_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_ovw_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_overwrite(sys_le16_to_cpu(cp->net_idx),
						   sys_le16_to_cpu(cp->address),
						   sys_le16_to_cpu(cp->elem_address),
						   sys_le16_to_cpu(cp->uuid),
						   sys_le16_to_cpu(cp->model_id),
						   &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_add(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_add_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  cp->net_key, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_update(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_update_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_update_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_update(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->net_key_idx),
					     cp->net_key,
					     &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_get_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_get_rp *rp = rsp;
	size_t key_cnt = 1;
	uint16_t keys;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  &keys, &key_cnt);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	/* for historical reasons this command has status in response */
	rp->status = 0;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_del(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_del_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_add(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_add_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  sys_le16_to_cpu(cp->app_key_idx),
					  sys_le16_to_cpu(cp->app_key),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_update(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_update_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_update_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_update(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->net_key_idx),
					     sys_le16_to_cpu(cp->app_key_idx),
					     sys_le16_to_cpu(cp->app_key),
					     &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_del(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_del_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  sys_le16_to_cpu(cp->app_key_idx),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_get_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_get_rp *rp = rsp;
	uint8_t status;
	uint16_t keys;
	size_t key_cnt = 1;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  &status, &keys, &key_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t config_model_app_bind(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_bind_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_bind_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	bt_mesh_cfg_cli_timeout_set(5000);

	err = bt_mesh_cfg_cli_mod_app_bind(sys_le16_to_cpu(cp->net_idx),
					   sys_le16_to_cpu(cp->address),
					   sys_le16_to_cpu(cp->elem_address),
					   sys_le16_to_cpu(cp->app_key_idx),
					   sys_le16_to_cpu(cp->mod_id),
					   &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_bind_vnd(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_bind_vnd_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_bind_vnd_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_bind_vnd(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       sys_le16_to_cpu(cp->elem_address),
					       sys_le16_to_cpu(cp->app_key_idx),
					       sys_le16_to_cpu(cp->mod_id),
					       sys_le16_to_cpu(cp->cid),
					       &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_unbind(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_unbind_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_unbind_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_unbind(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->app_key_idx),
					     sys_le16_to_cpu(cp->mod_id),
					     &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t config_model_app_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_get_rp *rp = rsp;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->mod_id),
					  &status, &apps, &app_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_vnd_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_vnd_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_vnd_get_rp *rp = rsp;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get_vnd(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->mod_id),
					      sys_le16_to_cpu(cp->cid),
					      &status, &apps, &app_cnt);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_pub_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_pub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_pub_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	pub.net_idx = sys_le16_to_cpu(cp->net_key_idx);
	pub.dst = sys_le16_to_cpu(cp->destination);
	pub.count = cp->count_log;
	pub.period = cp->period_log;
	pub.ttl = cp->ttl;
	pub.feat = sys_le16_to_cpu(cp->features);

	err = bt_mesh_cfg_cli_hb_pub_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_pub_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_pub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_pub_get_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_pub_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_sub_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_sub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_sub_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	sub.src = sys_le16_to_cpu(cp->source);
	sub.dst = sys_le16_to_cpu(cp->destination);
	sub.period = cp->period_log;

	err = bt_mesh_cfg_cli_hb_sub_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_sub_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_sub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_sub_get_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_sub_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_net_trans_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_net_trans_get_cmd *cp = cmd;
	struct btp_mesh_cfg_net_trans_get_rp *rp = rsp;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_get(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->transmit = transmit;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_net_trans_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_net_trans_set_cmd *cp = cmd;
	struct btp_mesh_cfg_net_trans_set_rp *rp = rsp;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_set(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       cp->transmit, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->transmit = transmit;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_identity_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_idt_set_cmd *cp = cmd;
	struct btp_mesh_cfg_node_idt_set_rp *rp = rsp;
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_set(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->net_key_idx),
						cp->new_identity,
						&status, &identity);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->identity = identity;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_identity_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_idt_get_cmd *cp = cmd;
	struct btp_mesh_cfg_node_idt_get_rp *rp = rsp;
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_get(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->net_key_idx),
						&status, &identity);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->identity = identity;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_reset(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_reset_cmd *cp = cmd;
	struct btp_mesh_cfg_node_reset_rp *rp = rsp;
	bool status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_reset(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_lpn_timeout_get(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_lpn_timeout_cmd *cp = cmd;
	struct btp_mesh_cfg_lpn_timeout_rp *rp = rsp;
	int32_t polltimeout;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_lpn_timeout_get(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->unicast_addr),
					      &polltimeout);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->timeout = sys_cpu_to_le32(polltimeout);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t test_id;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_fault_get(&health_cli, &ctx,
					   sys_le16_to_cpu(cp->cid), &test_id,
					   faults, &fault_count);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_clear(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_clear_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t test_id = 0;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_fault_clear(&health_cli, &ctx,
						     sys_le16_to_cpu(cp->cid),
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						     bt_mesh_op_agg_cli_seq_is_started() ?
						     NULL :
#endif
						     &test_id,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						     bt_mesh_op_agg_cli_seq_is_started() ?
						     NULL :
#endif
						     faults,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						     bt_mesh_op_agg_cli_seq_is_started() ?
						     NULL :
#endif
						     &fault_count);
	} else {
		err = bt_mesh_health_cli_fault_clear_unack(&health_cli, &ctx,
							   sys_le16_to_cpu(cp->cid));
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_fault_clear_rp *rp = rsp;

		rp->test_id = test_id;
		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_test(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_test_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_fault_test(&health_cli, &ctx,
						    sys_le16_to_cpu(cp->cid),
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						    bt_mesh_op_agg_cli_seq_is_started() ?
						    0 :
#endif
						    cp->test_id,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						    bt_mesh_op_agg_cli_seq_is_started() ?
						    NULL :
#endif
						    faults,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						    bt_mesh_op_agg_cli_seq_is_started() ?
						    NULL :
#endif
						    &fault_count);
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
		if (bt_mesh_op_agg_cli_seq_is_started()) {
			fault_count = 0;
		}
#endif
	} else {
		err = bt_mesh_health_cli_fault_test_unack(&health_cli, &ctx,
							  sys_le16_to_cpu(cp->cid),
							  cp->test_id);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_fault_test_rp *rp = rsp;

		rp->test_id = cp->test_id;
		rp->cid = cp->cid;
		(void)memcpy(rp->faults, faults, fault_count);

		*rsp_len = sizeof(*rp) + fault_count;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_period_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_period_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t divisor;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_period_get(&health_cli, &ctx, &divisor);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_period_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_period_set_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t updated_divisor;
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_period_set(&health_cli, &ctx, cp->divisor,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						    bt_mesh_op_agg_cli_seq_is_started() ?
						    NULL :
#endif
						    &updated_divisor);
	} else {
		err = bt_mesh_health_cli_period_set_unack(&health_cli, &ctx, cp->divisor);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_period_set_rp *rp = rsp;

		rp->divisor = updated_divisor;

		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_attention_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_attention_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t attention;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_attention_get(&health_cli, &ctx, &attention);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_attention_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_attention_set_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t updated_attention;
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_attention_set(&health_cli, &ctx, cp->attention,
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
						       bt_mesh_op_agg_cli_seq_is_started() ?
						       NULL :
#endif
						       &updated_attention);
	} else {
		err = bt_mesh_health_cli_attention_set_unack(&health_cli, &ctx, cp->attention);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_attention_set_rp *rp = rsp;

		rp->attention = updated_attention;

		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
static uint8_t opcodes_aggregator_init(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_opcodes_aggregator_init_cmd *cp = cmd;
	int err;

	LOG_DBG("");

	err = bt_mesh_op_agg_cli_seq_start(cp->net_idx, cp->app_idx, cp->dst, cp->elem_addr);
	if (err) {
		LOG_ERR("Failed to init Opcodes Aggregator Context (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t opcodes_aggregator_send(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_op_agg_cli_seq_send();
	if (err) {
		LOG_ERR("Failed to send Opcodes Aggregator message (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_RPR_CLI)
static uint8_t rpr_scan_start(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_scan_start_cmd *cp = cmd;

	struct bt_mesh_rpr_scan_status status;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t uuid[16] = {0};
	int err;

	err = bt_mesh_rpr_scan_start(&rpr_cli, &srv,
				     memcmp(uuid, cp->uuid, 16) ? cp->uuid : NULL,
				     cp->timeout,
				     BT_MESH_RPR_SCAN_MAX_DEVS_ANY, &status);

	if (err) {
		LOG_ERR("Scan start failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_ext_scan_start(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_ext_scan_start_cmd *cp = cmd;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_scan_start_ext(&rpr_cli, &srv, cp->uuid,
					 cp->timeout, cp->ad_types,
					 cp->ad_count);
	if (err) {
		LOG_ERR("Scan start failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_scan_caps_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_scan_caps_get_cmd *cp = cmd;
	struct bt_mesh_rpr_caps caps;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_scan_caps_get(&rpr_cli, &srv, &caps);
	if (err) {
		LOG_ERR("Scan capabilities get failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Remote Provisioning scan capabilities of 0x%04x:",
		net.dst);
	LOG_DBG("\tMax devices:     %u", caps.max_devs);
	LOG_DBG("\tActive scanning: %s",
		    caps.active_scan ? "true" : "false");

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_scan_get(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_scan_get_cmd *cp = cmd;
	struct bt_mesh_rpr_scan_status status;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_scan_get(&rpr_cli, &srv, &status);
	if (err) {
		LOG_ERR("Scan get failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Remote Provisioning scan on 0x%04x:", cp->dst);
	LOG_DBG("\tStatus:         %u", status.status);
	LOG_DBG("\tScan type:      %u", status.scan);
	LOG_DBG("\tMax devices:    %u", status.max_devs);
	LOG_DBG("\tRemaining time: %u", status.timeout);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_scan_stop(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_scan_stop_cmd *cp = cmd;
	struct bt_mesh_rpr_scan_status status;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_scan_stop(&rpr_cli, &srv, &status);
	if (err || status.status) {
		LOG_DBG("Scan stop failed: %d %u", err, status.status);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Remote Provisioning scan on 0x%04x stopped.",
		    net.dst);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_link_get(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_link_get_cmd *cp = cmd;
	struct bt_mesh_rpr_link link;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_link_get(&rpr_cli, &srv, &link);
	if (err) {
		LOG_ERR("Link get failed: %d %u", err, link.status);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Remote Provisioning Link on 0x%04x:", cp->dst);
	LOG_DBG("\tStatus: %u", link.status);
	LOG_DBG("\tState:  %u", link.state);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_link_close(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_link_close_cmd *cp = cmd;
	struct bt_mesh_rpr_link link;
	const struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_rpr_link_close(&rpr_cli, &srv, &link);
	if (err) {
		LOG_ERR("Link close failed: %d %u", err, link.status);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Remote Provisioning Link on 0x%04x:", cp->dst);
	LOG_DBG("\tStatus: %u", link.status);
	LOG_DBG("\tState:  %u", link.state);

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_prov_remote(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_prov_remote_cmd *cp = cmd;
	struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	err = bt_mesh_provision_remote(&rpr_cli, &srv, cp->uuid,
				       cp->net_idx, cp->addr);
	if (err) {
		LOG_ERR("Prov remote start failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpr_reprov_remote(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_rpr_reprov_remote_cmd *cp = cmd;
	struct bt_mesh_rpr_node srv = {
		.addr = cp->dst,
		.net_idx = net.net_idx,
		.ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	if (!BT_MESH_ADDR_IS_UNICAST(cp->addr)) {
		LOG_ERR("Must be a valid unicast address");
		err = -EINVAL;
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_reprovision_remote(&rpr_cli, &srv, cp->addr,
					 cp->comp_change);
	if (err) {
		LOG_ERR("Reprovisioning failed: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_DFD_SRV)
static struct {
	struct bt_mesh_dfu_target targets[32];
	struct bt_mesh_blob_target_pull pull[32];
	size_t target_cnt;
	struct bt_mesh_blob_cli_inputs inputs;
} dfu_tx;

static void dfu_tx_prepare(void)
{
	sys_slist_init(&dfu_tx.inputs.targets);

	for (int i = 0; i < dfu_tx.target_cnt; i++) {
		/* Reset target context. */
		uint16_t addr = dfu_tx.targets[i].blob.addr;

		memset(&dfu_tx.targets[i].blob, 0,
		       sizeof(struct bt_mesh_blob_target));
		memset(&dfu_tx.pull[i], 0,
		       sizeof(struct bt_mesh_blob_target_pull));
		dfu_tx.targets[i].blob.addr = addr;
		dfu_tx.targets[i].blob.pull = &dfu_tx.pull[i];

		sys_slist_append(&dfu_tx.inputs.targets,
				 &dfu_tx.targets[i].blob.n);
	}
}

static void dfu_target(uint8_t img_idx, uint16_t addr)
{
	if (dfu_tx.target_cnt == ARRAY_SIZE(dfu_tx.targets)) {
		LOG_ERR("No room.");
		return;
	}

	for (int i = 0; i < dfu_tx.target_cnt; i++) {
		if (dfu_tx.targets[i].blob.addr == addr) {
			LOG_ERR("Target 0x%04x already exists", addr);
			return;
		}
	}

	dfu_tx.targets[dfu_tx.target_cnt].blob.addr = addr;
	dfu_tx.targets[dfu_tx.target_cnt].img_idx = img_idx;
	sys_slist_append(&dfu_tx.inputs.targets,
			 &dfu_tx.targets[dfu_tx.target_cnt].blob.n);
	dfu_tx.target_cnt++;

	LOG_DBG("Added target 0x%04x", addr);
}
static void dfu_slot_add(size_t size, uint8_t *fwid, size_t fwid_len,
			 uint8_t *metadata, size_t metadata_len)
{
	struct bt_mesh_dfu_slot *slot;
	int err;

	slot = bt_mesh_dfu_slot_reserve();
	err = bt_mesh_dfu_slot_info_set(slot, size, metadata, metadata_len);
	if (err) {
		LOG_ERR("Failed to set slot info: %d", err);
		return;
	}

	err = bt_mesh_dfu_slot_fwid_set(slot, fwid, fwid_len);
	if (err) {
		LOG_ERR("Failed to set slot fwid: %d", err);
		return;
	}

	err = bt_mesh_dfu_slot_commit(slot);
	if (err) {
		LOG_ERR("Failed to commit slot: %d", err);
		return;
	}

	LOG_DBG("Slot added.");
}
static enum bt_mesh_dfu_iter dfu_img_cb(struct bt_mesh_dfu_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					uint8_t idx, uint8_t total,
					const struct bt_mesh_dfu_img *img,
					void *cb_data)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	size_t len;

	idx = 0xff;

	if (img->fwid_len <= sizeof(fwid)) {
		len = bin2hex(img->fwid, img->fwid_len, fwid, sizeof(fwid));
	} else {
		LOG_ERR("FWID is too big");
		return BT_MESH_DFU_ITER_STOP;
	}

	fwid[len] = '\0';

	LOG_DBG("Image %u:", idx);
	LOG_DBG("\tFWID: ");
	if (img->uri) {
		LOG_DBG("\tURI:  ");
	}

	return BT_MESH_DFU_ITER_CONTINUE;
}

static uint8_t dfu_info_get(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_dfu_info_get_cmd *cp = cmd;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t max_count;
	int err = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}
	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	max_count = cp->limit;

	err = bt_mesh_dfu_cli_imgs_get(&dfd_srv.dfu, &ctx, dfu_img_cb, NULL,
				       max_count);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t dfu_update_metadata_check(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_dfu_metadata_check_cmd *cp = cmd;
	struct btp_mmdl_dfu_metadata_check_rp *rp = rsp;
	const struct bt_mesh_dfu_slot *slot;
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct bt_mesh_dfu_metadata_status rsp_data;
	uint8_t img_idx, slot_idx;
	size_t size;
	size_t fwid_len;
	size_t metadata_len;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN];
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;
	img_idx = cp->index;
	slot_idx = cp->slot_idx;
	size = cp->slot_size;
	fwid_len = cp->fwid_len;
	metadata_len = cp->metadata_len;

	if ((metadata_len > 0) &&
		(metadata_len < CONFIG_BT_MESH_DFU_METADATA_MAXLEN)) {
		memcpy(&metadata, cp->data, metadata_len);
	}

	dfu_slot_add(size, fwid, fwid_len, metadata, metadata_len);

	slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!slot) {
		LOG_ERR("No image in slot %u", slot_idx);
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_dfu_cli_metadata_check(&dfd_srv.dfu, &ctx, img_idx, slot,
					     &rsp_data);

	if (err) {
		LOG_ERR("ERR %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->idx = rsp_data.idx;
	rp->status = rsp_data.status;
	rp->effect = rsp_data.effect;

	*rsp_len = sizeof(struct btp_mmdl_dfu_metadata_check_rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t dfu_firmware_update_get(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct bt_mesh_dfu_target_status rsp_data;
	struct btp_mmdl_dfu_firmware_update_rp *rp = rsp;
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_dfu_cli_status_get(&dfd_srv.dfu, &ctx, &rsp_data);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = rsp_data.status;
	*rsp_len = sizeof(struct btp_mmdl_dfu_firmware_update_rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t dfu_firmware_update_cancel(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	struct model_data *model_bound;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	ctx.addr = model_bound->addr;
	ctx.app_idx = model_bound->appkey_idx;

	err = bt_mesh_dfu_cli_cancel(&dfd_srv.dfu, &ctx);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t dfu_firmware_update_start(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_dfu_firmware_update_cmd *cp = cmd;
	struct model_data *model_bound;
	struct bt_mesh_dfu_cli_xfer xfer;
	uint8_t addr_cnt;
	uint16_t addr = BT_MESH_ADDR_UNASSIGNED;
	uint8_t slot_idx;
	size_t size;
	size_t fwid_len;
	size_t metadata_len;
	uint8_t fwid[CONFIG_BT_MESH_DFU_FWID_MAXLEN];
	uint8_t metadata[CONFIG_BT_MESH_DFU_METADATA_MAXLEN];
	int err = 0;
	int i = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	struct bt_mesh_dfu_cli_xfer_blob_params blob = {
		.block_size_log = cp->block_size,
		.chunk_size = cp->chunk_size,
	};

	addr_cnt = cp->addr_cnt;
	slot_idx = cp->slot_idx;
	size = cp->slot_size;
	fwid_len = cp->fwid_len;
	metadata_len = cp->metadata_len;
	xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	xfer.blob_params = &blob;

	if ((metadata_len > 0) &&
		(metadata_len < CONFIG_BT_MESH_DFU_METADATA_MAXLEN)) {
		memcpy(&metadata, cp->data, metadata_len);
	}

	bt_mesh_dfu_slot_del_all();

	dfu_slot_add(size, fwid, fwid_len, metadata, metadata_len);

	xfer.slot = bt_mesh_dfu_slot_at(slot_idx);
	if (!xfer.slot) {
		LOG_ERR("No image in slot %u", slot_idx);
		return BTP_STATUS_FAILED;
	}

	for (i = 0; i < addr_cnt; i++) {
		addr = cp->data[metadata_len + 1 + i * sizeof(uint16_t)] |
			(cp->data[metadata_len + i * sizeof(uint16_t)] << 8);
		dfu_target(slot_idx, addr);
	}

	dfu_tx_prepare();

	if (!dfu_tx.target_cnt) {
		LOG_ERR("No targets.");
		return BTP_STATUS_FAILED;
	}

	if (addr_cnt > 1) {
		dfu_tx.inputs.group = BT_MESH_ADDR_UNASSIGNED;
	} else {
		dfu_tx.inputs.group = addr;
	}

	dfu_tx.inputs.app_idx = model_bound->appkey_idx;
	dfu_tx.inputs.ttl = BT_MESH_TTL_DEFAULT;

	err = bt_mesh_dfu_cli_send(&dfd_srv.dfu, &dfu_tx.inputs, &dummy_blob_io, &xfer);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t dfu_firmware_update_apply(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	struct model_data *model_bound;
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_DFU_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_dfu_cli_apply(&dfd_srv.dfu);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

#if defined(CONFIG_BT_MESH_BLOB_CLI) && !defined(CONFIG_BT_MESH_DFD_SRV)
static void blob_cli_inputs_prepare(uint16_t group, uint16_t app_idx)
{
	int i;

	blob_cli_xfer.inputs.ttl = BT_MESH_TTL_DEFAULT;
	blob_cli_xfer.inputs.group = group;
	blob_cli_xfer.inputs.app_idx = app_idx;
	sys_slist_init(&blob_cli_xfer.inputs.targets);

	for (i = 0; i < blob_cli_xfer.target_count; ++i) {
		/* Reset target context. */
		uint16_t addr = blob_cli_xfer.targets[i].addr;

		memset(&blob_cli_xfer.targets[i], 0,
		       sizeof(struct bt_mesh_blob_target));
		memset(&blob_cli_xfer.pull[i], 0,
		       sizeof(struct bt_mesh_blob_target_pull));
		blob_cli_xfer.targets[i].addr = addr;
		blob_cli_xfer.targets[i].pull = &blob_cli_xfer.pull[i];

		sys_slist_append(&blob_cli_xfer.inputs.targets,
				 &blob_cli_xfer.targets[i].n);
	}
}

static int cmd_blob_target(uint16_t addr)
{
	struct bt_mesh_blob_target *t;

	if (blob_cli_xfer.target_count == ARRAY_SIZE(blob_cli_xfer.targets)) {
		LOG_ERR("No more room");
		return 0;
	}

	t = &blob_cli_xfer.targets[blob_cli_xfer.target_count];

	t->addr = addr;

	LOG_DBG("Added target 0x%04x", t->addr);

	blob_cli_xfer.target_count++;
	return 0;
}

static uint8_t blob_info_get(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_blob_info_get_cmd *cp = cmd;
	struct model_data *model_bound;
	uint16_t addr = BT_MESH_ADDR_UNASSIGNED;
	uint16_t group = BT_MESH_ADDR_UNASSIGNED;
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_BLOB_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	for (int i = 0; i < cp->addr_cnt; i++) {
		addr = cp->addr[1 + i * sizeof(uint16_t)] |
			(cp->addr[i * sizeof(uint16_t)] << 8);
		err = cmd_blob_target(addr);
		if (err) {
			LOG_ERR("err target %d", err);
			return BTP_STATUS_FAILED;
		}
	}

	if (cp->addr_cnt > 1) {
		group = BT_MESH_ADDR_UNASSIGNED;
	} else {
		group = addr;
	}

	if (!blob_cli_xfer.target_count) {
		LOG_ERR("Failed: No targets");
		return BTP_STATUS_FAILED;
	}

	blob_cli_inputs_prepare(group, model_bound->appkey_idx);

	err = bt_mesh_blob_cli_caps_get(&blob_cli, &blob_cli_xfer.inputs);

	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t blob_transfer_start(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_blob_transfer_start_cmd *cp = cmd;
	struct model_data *model_bound;
	int err = 0;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_BLOB_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	if (!blob_cli_xfer.target_count) {
		LOG_ERR("Failed: No targets");
		return BTP_STATUS_FAILED;
	}
	blob_cli_xfer.xfer.id = cp->id;
	blob_cli_xfer.xfer.size = cp->size;
	blob_cli_xfer.xfer.block_size_log = cp->block_size;
	blob_cli_xfer.xfer.chunk_size = cp->chunk_size;

	if (blob_cli.caps.modes) {
		blob_cli_xfer.xfer.mode = blob_cli.caps.modes;
	} else {
		blob_cli_xfer.xfer.mode = BT_MESH_BLOB_XFER_MODE_PUSH;
	}

	if (cp->timeout) {
		blob_cli_xfer.inputs.timeout_base = cp->timeout;
	}

	if (cp->ttl) {
		blob_cli_xfer.inputs.ttl = cp->ttl;
	}

	err = bt_mesh_blob_cli_send(&blob_cli, &blob_cli_xfer.inputs,
				    &blob_cli_xfer.xfer, &dummy_blob_io);

	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t blob_transfer_cancel(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("");

	bt_mesh_blob_cli_cancel(&blob_cli);

	return BTP_STATUS_SUCCESS;
}

static uint8_t blob_transfer_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	struct model_data *model_bound;
	uint16_t group;
	int err;

	LOG_DBG("");

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_BLOB_CLI);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	group = model_bound->addr;

	err = cmd_blob_target(group);
	if (err) {
		LOG_ERR("err target %d", err);
		return BTP_STATUS_FAILED;
	}

	if (!blob_cli_xfer.target_count) {
		LOG_ERR("Failed: No targets");
		return BTP_STATUS_FAILED;
	}

	blob_cli_inputs_prepare(group, model_bound->appkey_idx);

	err = bt_mesh_blob_cli_xfer_progress_get(&blob_cli, &blob_cli_xfer.inputs);

	if (err) {
		LOG_ERR("ERR %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_MESH_BLOB_CLI */

#if defined(CONFIG_BT_MESH_BLOB_SRV)
static uint8_t blob_srv_recv(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mmdl_blob_srv_recv_cmd *cp = cmd;
	struct model_data *model_bound;
	int err;

#if defined(CONFIG_BT_MESH_DFU_SRV)
	struct bt_mesh_blob_srv *srv = &dfu_srv.blob;
#elif defined(CONFIG_BT_MESH_DFD_SRV)
	struct bt_mesh_blob_srv *srv = &dfd_srv.upload.blob;
#endif

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_BLOB_SRV);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	uint16_t timeout_base;
	uint64_t id;
	uint8_t ttl;

	LOG_DBG("");

	id = cp->id;
	timeout_base = cp->timeout;
	ttl = cp->ttl;

	err = bt_mesh_blob_srv_recv(srv, id, &dummy_blob_io, ttl,
				    timeout_base);

	if (err) {
		LOG_ERR("ERR %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t blob_srv_cancel(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	struct model_data *model_bound;
	int err;

#if defined(CONFIG_BT_MESH_DFU_SRV)
	struct bt_mesh_blob_srv *srv = &dfu_srv.blob;
#elif defined(CONFIG_BT_MESH_DFD_SRV)
	struct bt_mesh_blob_srv *srv = &dfd_srv.upload.blob;
#endif

	model_bound = lookup_model_bound(BT_MESH_MODEL_ID_BLOB_SRV);
	if (!model_bound) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("");

	err = bt_mesh_blob_srv_cancel(srv);

	if (err) {
		LOG_ERR("ERR %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_MESH_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_MESH_CONFIG_PROVISIONING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = config_prov,
	},
	{
		.opcode = BTP_MESH_PROVISION_NODE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = provision_node,
	},
	{
		.opcode = BTP_MESH_INIT,
		.expect_len = sizeof(struct btp_mesh_init_cmd),
		.func = init,
	},
	{
		.opcode = BTP_MESH_RESET,
		.expect_len = 0,
		.func = reset,
	},
	{
		.opcode = BTP_MESH_INPUT_NUMBER,
		.expect_len = sizeof(struct btp_mesh_input_number_cmd),
		.func = input_number,
	},
	{
		.opcode = BTP_MESH_INPUT_STRING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = input_string,
	},
	{
		.opcode = BTP_MESH_IVU_TEST_MODE,
		.expect_len = sizeof(struct btp_mesh_ivu_test_mode_cmd),
		.func = ivu_test_mode,
	},
	{
		.opcode = BTP_MESH_IVU_TOGGLE_STATE,
		.expect_len = 0,
		.func = ivu_toggle_state,
	},
	{
		.opcode = BTP_MESH_LPN,
		.expect_len = sizeof(struct btp_mesh_lpn_set_cmd),
		.func = lpn,
	},
	{
		.opcode = BTP_MESH_LPN_POLL,
		.expect_len = 0,
		.func = lpn_poll,
	},
	{
		.opcode = BTP_MESH_NET_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = net_send,
	},
	{
		.opcode = BTP_MESH_HEALTH_GENERATE_FAULTS,
		.expect_len = 0,
		.func = health_generate_faults,
	},
	{
		.opcode = BTP_MESH_HEALTH_CLEAR_FAULTS,
		.expect_len = 0,
		.func = health_clear_faults,
	},
	{
		.opcode = BTP_MESH_MODEL_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = model_send,
	},
	{
		.opcode = BTP_MESH_COMP_DATA_GET,
		.expect_len = sizeof(struct btp_mesh_comp_data_get_cmd),
		.func = composition_data_get,
	},
	{
		.opcode = BTP_MESH_CFG_BEACON_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_beacon_get_cmd),
		.func = config_beacon_get,
	},
	{
		.opcode = BTP_MESH_CFG_BEACON_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_beacon_set_cmd),
		.func = config_beacon_set,
	},
	{
		.opcode = BTP_MESH_CFG_DEFAULT_TTL_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_default_ttl_get_cmd),
		.func = config_default_ttl_get,
	},
	{
		.opcode = BTP_MESH_CFG_DEFAULT_TTL_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_default_ttl_set_cmd),
		.func = config_default_ttl_set,
	},
	{
		.opcode = BTP_MESH_CFG_GATT_PROXY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_gatt_proxy_get_cmd),
		.func = config_gatt_proxy_get,
	},
	{
		.opcode = BTP_MESH_CFG_GATT_PROXY_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_gatt_proxy_set_cmd),
		.func = config_gatt_proxy_set,
	},
	{
		.opcode = BTP_MESH_CFG_FRIEND_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_friend_get_cmd),
		.func = config_friend_get,
	},
	{
		.opcode = BTP_MESH_CFG_FRIEND_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_friend_set_cmd),
		.func = config_friend_set,
	},
	{
		.opcode = BTP_MESH_CFG_RELAY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_relay_get_cmd),
		.func = config_relay_get,
	},
	{
		.opcode = BTP_MESH_CFG_RELAY_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_relay_set_cmd),
		.func = config_relay_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_get_cmd),
		.func = config_mod_pub_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_set_cmd),
		.func = config_mod_pub_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_add_cmd),
		.func = config_mod_sub_add,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_del_cmd),
		.func = config_mod_sub_del,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_OVW,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_ovw_cmd),
		.func = config_mod_sub_ovw,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_DEL_ALL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_del_all_cmd),
		.func = config_mod_sub_del_all,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_get_cmd),
		.func = config_mod_sub_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_GET_VND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_get_vnd_cmd),
		.func = config_mod_sub_get_vnd,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_add_cmd),
		.func = config_mod_sub_va_add,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_del_cmd),
		.func = config_mod_sub_va_del,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_OVW,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_ovw_cmd),
		.func = config_mod_sub_va_ovw,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_add_cmd),
		.func = config_netkey_add,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_get_cmd),
		.func = config_netkey_get,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_del_cmd),
		.func = config_netkey_del,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_UPDATE,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_update_cmd),
		.func = config_netkey_update,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_add_cmd),
		.func = config_appkey_add,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_get_cmd),
		.func = config_appkey_get,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_del_cmd),
		.func = config_appkey_del,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_UPDATE,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_update_cmd),
		.func = config_appkey_update,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_BIND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_bind_cmd),
		.func = config_model_app_bind,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_UNBIND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_unbind_cmd),
		.func = config_model_app_unbind,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_get_cmd),
		.func = config_model_app_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_VND_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_vnd_get_cmd),
		.func = config_model_app_vnd_get,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_PUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_pub_set_cmd),
		.func = config_hb_pub_set,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_PUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_pub_get_cmd),
		.func = config_hb_pub_get,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_SUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_sub_set_cmd),
		.func = config_hb_sub_set,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_SUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_sub_get_cmd),
		.func = config_hb_sub_get,
	},
	{
		.opcode = BTP_MESH_CFG_NET_TRANS_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_net_trans_get_cmd),
		.func = config_net_trans_get,
	},
	{
		.opcode = BTP_MESH_CFG_NET_TRANS_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_net_trans_set_cmd),
		.func = config_net_trans_set,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_IDT_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_idt_set_cmd),
		.func = config_node_identity_set,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_IDT_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_idt_get_cmd),
		.func = config_node_identity_get,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_RESET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_reset_cmd),
		.func = config_node_reset,
	},
	{
		.opcode = BTP_MESH_CFG_LPN_TIMEOUT_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_lpn_timeout_cmd),
		.func = config_lpn_timeout_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_VA_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_va_set_cmd),
		.func = config_mod_pub_va_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_BIND_VND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_bind_vnd_cmd),
		.func = config_model_app_bind_vnd,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_GET,
		.expect_len = sizeof(struct btp_mesh_health_fault_get_cmd),
		.func = health_fault_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_CLEAR,
		.expect_len = sizeof(struct btp_mesh_health_fault_clear_cmd),
		.func = health_fault_clear,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_TEST,
		.expect_len = sizeof(struct btp_mesh_health_fault_test_cmd),
		.func = health_fault_test,
	},
	{
		.opcode = BTP_MESH_HEALTH_PERIOD_GET,
		.expect_len = sizeof(struct btp_mesh_health_period_get_cmd),
		.func = health_period_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_PERIOD_SET,
		.expect_len = sizeof(struct btp_mesh_health_period_set_cmd),
		.func = health_period_set,
	},
	{
		.opcode = BTP_MESH_HEALTH_ATTENTION_GET,
		.expect_len = sizeof(struct btp_mesh_health_attention_get_cmd),
		.func = health_attention_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_ATTENTION_SET,
		.expect_len = sizeof(struct btp_mesh_health_attention_set_cmd),
		.func = health_attention_set,
	},
	{
		.opcode = BTP_MESH_PROVISION_ADV,
		.expect_len = sizeof(struct btp_mesh_provision_adv_cmd),
		.func = provision_adv,
	},
	{
		.opcode = BTP_MESH_CFG_KRP_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_krp_get_cmd),
		.func = config_krp_get,
	},
	{
		.opcode = BTP_MESH_CFG_KRP_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_krp_set_cmd),
		.func = config_krp_set,
	},
	{
		.opcode = BTP_MESH_VA_ADD,
		.expect_len = sizeof(struct btp_mesh_va_add_cmd),
		.func = va_add,
	},
	{
		.opcode = BTP_MESH_VA_DEL,
		.expect_len = sizeof(struct btp_mesh_va_del_cmd),
		.func = va_del,
	},
#if defined(CONFIG_BT_TESTING)
	{
		.opcode = BTP_MESH_LPN_SUBSCRIBE,
		.expect_len = sizeof(struct btp_mesh_lpn_subscribe_cmd),
		.func = lpn_subscribe,
	},
	{
		.opcode = BTP_MESH_LPN_UNSUBSCRIBE,
		.expect_len = sizeof(struct btp_mesh_lpn_unsubscribe_cmd),
		.func = lpn_unsubscribe,
	},
	{
		.opcode = BTP_MESH_RPL_CLEAR,
		.expect_len = 0,
		.func = rpl_clear,
	},
#endif /* CONFIG_BT_TESTING */
	{
		.opcode = BTP_MESH_PROXY_IDENTITY,
		.expect_len = 0,
		.func = proxy_identity_enable,
	},
#if defined(CONFIG_BT_MESH_PROXY_CLIENT)
	{
		.opcode = BTP_MESH_PROXY_CONNECT,
		.expect_len = sizeof(struct btp_proxy_connect_cmd),
		.func = proxy_connect
	},
#endif
#if defined(CONFIG_BT_MESH_SAR_CFG_CLI)
	{
		.opcode = BTP_MESH_SAR_TRANSMITTER_GET,
		.expect_len = sizeof(struct btp_mesh_sar_transmitter_get_cmd),
		.func = sar_transmitter_get
	},
	{
		.opcode = BTP_MESH_SAR_TRANSMITTER_SET,
		.expect_len = sizeof(struct btp_mesh_sar_transmitter_set_cmd),
		.func = sar_transmitter_set
	},
	{
		.opcode = BTP_MESH_SAR_RECEIVER_GET,
		.expect_len = sizeof(struct btp_mesh_sar_receiver_get_cmd),
		.func = sar_receiver_get
	},
	{
		.opcode = BTP_MESH_SAR_RECEIVER_SET,
		.expect_len = sizeof(struct btp_mesh_sar_receiver_set_cmd),
		.func = sar_receiver_set
	},
#endif
#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_CLI)
	{
		.opcode = BTP_MESH_LARGE_COMP_DATA_GET,
		.expect_len = sizeof(struct btp_mesh_large_comp_data_get_cmd),
		.func = large_comp_data_get
	},
	{
		.opcode = BTP_MESH_MODELS_METADATA_GET,
		.expect_len = sizeof(struct btp_mesh_models_metadata_get_cmd),
		.func = models_metadata_get
	},
#endif
#if defined(CONFIG_BT_MESH_OP_AGG_CLI)
	{
		.opcode = BTP_MESH_OPCODES_AGGREGATOR_INIT,
		.expect_len = sizeof(struct btp_mesh_opcodes_aggregator_init_cmd),
		.func = opcodes_aggregator_init
	},
	{
		.opcode = BTP_MESH_OPCODES_AGGREGATOR_SEND,
		.expect_len = 0,
		.func = opcodes_aggregator_send
	},
#endif
	{
		.opcode = BTP_MESH_COMP_CHANGE_PREPARE,
		.expect_len = 0,
		.func = change_prepare
	},
#if defined(CONFIG_BT_MESH_RPR_CLI)
	{
		.opcode = BTP_MESH_RPR_SCAN_START,
		.expect_len = sizeof(struct btp_rpr_scan_start_cmd),
		.func = rpr_scan_start
	},
	{
		.opcode = BTP_MESH_RPR_EXT_SCAN_START,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = rpr_ext_scan_start
	},
	{
		.opcode = BTP_MESH_RPR_SCAN_CAPS_GET,
		.expect_len = sizeof(struct btp_rpr_scan_caps_get_cmd),
		.func = rpr_scan_caps_get
	},
	{
		.opcode = BTP_MESH_RPR_SCAN_GET,
		.expect_len = sizeof(struct btp_rpr_scan_get_cmd),
		.func = rpr_scan_get
	},
	{
		.opcode = BTP_MESH_RPR_SCAN_STOP,
		.expect_len = sizeof(struct btp_rpr_scan_stop_cmd),
		.func = rpr_scan_stop
	},
	{
		.opcode = BTP_MESH_RPR_LINK_GET,
		.expect_len = sizeof(struct btp_rpr_link_get_cmd),
		.func = rpr_link_get
	},
	{
		.opcode = BTP_MESH_RPR_LINK_CLOSE,
		.expect_len = sizeof(struct btp_rpr_link_close_cmd),
		.func = rpr_link_close
	},
	{
		.opcode = BTP_MESH_RPR_PROV_REMOTE,
		.expect_len = sizeof(struct btp_rpr_prov_remote_cmd),
		.func = rpr_prov_remote
	},
	{
		.opcode = BTP_MESH_RPR_REPROV_REMOTE,
		.expect_len = sizeof(struct btp_rpr_reprov_remote_cmd),
		.func = rpr_reprov_remote
	},
#endif
#if defined(CONFIG_BT_MESH_PRIV_BEACON_CLI)
	{
		.opcode = BTP_MESH_PRIV_BEACON_GET,
		.expect_len = sizeof(struct btp_priv_beacon_get_cmd),
		.func = priv_beacon_get
	},
	{
		.opcode = BTP_MESH_PRIV_BEACON_SET,
		.expect_len = sizeof(struct btp_priv_beacon_set_cmd),
		.func = priv_beacon_set
	},
	{
		.opcode = BTP_MESH_PRIV_GATT_PROXY_GET,
		.expect_len = sizeof(struct btp_priv_gatt_proxy_get_cmd),
		.func = priv_gatt_proxy_get
	},
	{
		.opcode = BTP_MESH_PRIV_GATT_PROXY_SET,
		.expect_len = sizeof(struct btp_priv_gatt_proxy_set_cmd),
		.func = priv_gatt_proxy_set
	},
	{
		.opcode = BTP_MESH_PRIV_NODE_ID_GET,
		.expect_len = sizeof(struct btp_priv_node_id_get_cmd),
		.func = priv_node_id_get
	},
	{
		.opcode = BTP_MESH_PRIV_NODE_ID_SET,
		.expect_len = sizeof(struct btp_priv_node_id_set_cmd),
		.func = priv_node_id_set
	},
	{
		.opcode = BTP_MESH_PROXY_PRIVATE_IDENTITY,
		.expect_len = 0,
		.func = proxy_private_identity_enable
	},
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
	{
		.opcode = BTP_MESH_OD_PRIV_PROXY_GET,
		.expect_len = sizeof(struct btp_od_priv_proxy_get_cmd),
		.func = od_priv_proxy_get
	},
	{
		.opcode = BTP_MESH_OD_PRIV_PROXY_SET,
		.expect_len = sizeof(struct btp_od_priv_proxy_set_cmd),
		.func = od_priv_proxy_set
	},
#endif
#if defined(CONFIG_BT_MESH_SOL_PDU_RPL_CLI)
	{
		.opcode = BTP_MESH_SRPL_CLEAR,
		.expect_len = sizeof(struct btp_srpl_clear_cmd),
		.func = srpl_clear
	},
#endif
#if defined(CONFIG_BT_MESH_PROXY_SOLICITATION)
	{
		.opcode = BTP_MESH_PROXY_SOLICIT,
		.expect_len = sizeof(struct btp_proxy_solicit_cmd),
		.func = proxy_solicit
	},
#endif
	{
		.opcode = BTP_MESH_START,
		.expect_len = 0,
		.func = start
	},
};


static const struct btp_handler mdl_handlers[] = {
#if defined(CONFIG_BT_MESH_DFD_SRV)
	{
		.opcode = BTP_MMDL_DFU_INFO_GET,
		.expect_len = sizeof(struct btp_mmdl_dfu_info_get_cmd),
		.func = dfu_info_get,
	},
	{
		.opcode = BTP_MMDL_DFU_UPDATE_METADATA_CHECK,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = dfu_update_metadata_check,
	},
	{
		.opcode = BTP_MMDL_DFU_FIRMWARE_UPDATE_GET,
		.expect_len = 0,
		.func = dfu_firmware_update_get,
	},
	{
		.opcode = BTP_MMDL_DFU_FIRMWARE_UPDATE_CANCEL,
		.expect_len = 0,
		.func = dfu_firmware_update_cancel,
	},
	{
		.opcode = BTP_MMDL_DFU_FIRMWARE_UPDATE_START,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = dfu_firmware_update_start,
	},
	{
		.opcode = BTP_MMDL_DFU_FIRMWARE_UPDATE_APPLY,
		.expect_len = 0,
		.func = dfu_firmware_update_apply,
	},
#endif
#if defined(CONFIG_BT_MESH_BLOB_CLI) && !defined(CONFIG_BT_MESH_DFD_SRV)
	{
		.opcode = BTP_MMDL_BLOB_INFO_GET,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = blob_info_get,
	},
	{
		.opcode = BTP_MMDL_BLOB_TRANSFER_START,
		.expect_len = sizeof(struct btp_mmdl_blob_transfer_start_cmd),
		.func = blob_transfer_start,
	},
	{
		.opcode = BTP_MMDL_BLOB_TRANSFER_CANCEL,
		.expect_len = 0,
		.func = blob_transfer_cancel,
	},
	{
		.opcode = BTP_MMDL_BLOB_TRANSFER_GET,
		.expect_len = 0,
		.func = blob_transfer_get,
	},
#endif
#if defined(CONFIG_BT_MESH_BLOB_SRV)
	{
		.opcode = BTP_MMDL_BLOB_SRV_RECV,
		.expect_len = sizeof(struct btp_mmdl_blob_srv_recv_cmd),
		.func = blob_srv_recv
	},
	{
		.opcode = BTP_MMDL_BLOB_SRV_CANCEL,
		.expect_len = 0,
		.func = blob_srv_cancel
	},
#endif
#if defined(CONFIG_BT_MESH_DFU_SRV)
	{
		.opcode = BTP_MMDL_DFU_SRV_APPLY,
		.expect_len = 0,
		.func = dfu_srv_apply
	},
#endif
};

void net_recv_ev(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst, const void *payload,
		 size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct btp_mesh_net_recv_ev *ev;

	LOG_DBG("ttl 0x%02x ctl 0x%02x src 0x%04x dst 0x%04x payload_len %zu",
		ttl, ctl, src, dst, payload_len);

	if (payload_len > net_buf_simple_tailroom(&buf)) {
		LOG_ERR("Payload size exceeds buffer size");
		return;
	}

	ev = net_buf_simple_add(&buf, sizeof(*ev));
	ev->ttl = ttl;
	ev->ctl = ctl;
	ev->src = sys_cpu_to_le16(src);
	ev->dst = sys_cpu_to_le16(dst);
	ev->payload_len = payload_len;
	net_buf_simple_add_mem(&buf, payload, payload_len);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_NET_RECV, buf.data, buf.len);
}

void model_recv_ev(uint16_t src, uint16_t dst, const void *payload,
		   size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct btp_mesh_model_recv_ev *ev;

	LOG_DBG("src 0x%04x dst 0x%04x payload_len %zu", src, dst, payload_len);

	if (payload_len > net_buf_simple_tailroom(&buf)) {
		LOG_ERR("Payload size exceeds buffer size");
		return;
	}

	ev = net_buf_simple_add(&buf, sizeof(*ev));
	ev->src = sys_cpu_to_le16(src);
	ev->dst = sys_cpu_to_le16(dst);
	ev->payload_len = payload_len;
	net_buf_simple_add_mem(&buf, payload, payload_len);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_MODEL_RECV, buf.data, buf.len);
}

static void model_bound_cb(uint16_t addr, struct bt_mesh_model *model,
			   uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (!model_bound[i].model) {
			model_bound[i].model = model;
			model_bound[i].addr = addr;
			model_bound[i].appkey_idx = key_idx;

			return;
		}
	}

	LOG_ERR("model_bound is full");
}

static void model_unbound_cb(uint16_t addr, struct bt_mesh_model *model,
			     uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model == model) {
			model_bound[i].model = NULL;
			model_bound[i].addr = 0x0000;
			model_bound[i].appkey_idx = BT_MESH_KEY_UNUSED;

			return;
		}
	}

	LOG_INF("model not found");
}

static void invalid_bearer_cb(uint8_t opcode)
{
	struct btp_mesh_invalid_bearer_ev ev = {
		.opcode = opcode,
	};

	LOG_DBG("opcode 0x%02x", opcode);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_INVALID_BEARER, &ev, sizeof(ev));
}

static void incomp_timer_exp_cb(void)
{
	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_INCOMP_TIMER_EXP, NULL, 0);
}

static struct bt_test_cb bt_test_cb = {
	.mesh_net_recv = net_recv_ev,
	.mesh_model_recv = model_recv_ev,
	.mesh_model_bound = model_bound_cb,
	.mesh_model_unbound = model_unbound_cb,
	.mesh_prov_invalid_bearer = invalid_bearer_cb,
	.mesh_trans_incomp_timer_exp = incomp_timer_exp_cb,
};

static void friend_established(uint16_t net_idx, uint16_t lpn_addr,
			       uint8_t recv_delay, uint32_t polltimeout)
{
	struct btp_mesh_frnd_established_ev ev = { net_idx, lpn_addr, recv_delay,
					       polltimeout };

	LOG_DBG("Friendship (as Friend) established with "
			"LPN 0x%04x Receive Delay %u Poll Timeout %u",
			lpn_addr, recv_delay, polltimeout);


	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_FRND_ESTABLISHED, &ev, sizeof(ev));
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
	struct btp_mesh_frnd_terminated_ev ev = { net_idx, lpn_addr };

	LOG_DBG("Friendship (as Friend) lost with LPN "
			"0x%04x", lpn_addr);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_FRND_TERMINATED, &ev, sizeof(ev));
}

BT_MESH_FRIEND_CB_DEFINE(friend_cb) = {
	.established = friend_established,
	.terminated = friend_terminated,
};

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
					uint8_t queue_size, uint8_t recv_win)
{
	struct btp_mesh_lpn_established_ev ev = { net_idx, friend_addr, queue_size,
					      recv_win };

	LOG_DBG("Friendship (as LPN) established with "
			"Friend 0x%04x Queue Size %d Receive Window %d",
			friend_addr, queue_size, recv_win);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_ESTABLISHED, &ev, sizeof(ev));
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	struct btp_mesh_lpn_polled_ev ev = { net_idx, friend_addr };

	LOG_DBG("Friendship (as LPN) lost with Friend "
			"0x%04x", friend_addr);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_TERMINATED, &ev, sizeof(ev));
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
	struct btp_mesh_lpn_polled_ev ev = { net_idx, friend_addr, (uint8_t)retry };

	LOG_DBG("LPN polled 0x%04x %s", friend_addr, retry ? "(retry)" : "");

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_POLLED, &ev, sizeof(ev));
}

BT_MESH_LPN_CB_DEFINE(lpn_cb) = {
	.established = lpn_established,
	.terminated = lpn_terminated,
	.polled = lpn_polled,
};

uint8_t tester_init_mesh(void)
{
	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_cb_register(&bt_test_cb);
	}

	tester_register_command_handlers(BTP_SERVICE_ID_MESH, handlers,
					 ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_mmdl(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_MESH_MDL, mdl_handlers,
					 ARRAY_SIZE(mdl_handlers));
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mmdl(void)
{
	return BTP_STATUS_SUCCESS;
}
