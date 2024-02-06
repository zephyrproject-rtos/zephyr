/* btp_csip.c - Bluetooth CSIP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "btp/btp.h"
#include <zephyr/bluetooth/audio/csip.h>

#include "../bluetooth/audio/csip_internal.h"
#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_csip
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

const struct bt_csip_set_coordinator_set_member *btp_csip_set_members[CONFIG_BT_MAX_CONN];
static const struct bt_csip_set_coordinator_csis_inst *cur_csis_inst;
static struct bt_csip_set_coordinator_svc_inst *csip_inst;

static uint8_t btp_csip_supported_commands(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	struct btp_csip_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_CSIP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_CSIP_DISCOVER);
	tester_set_bit(rp->data, BTP_CSIP_START_ORDERED_ACCESS);
	tester_set_bit(rp->data, BTP_CSIP_SET_COORDINATOR_LOCK);
	tester_set_bit(rp->data, BTP_CSIP_SET_COORDINATOR_RELEASE);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static void btp_send_csip_discovered_ev(struct bt_conn *conn, uint16_t sirk_handle,
					uint16_t size_handle, uint16_t lock_handle,
					uint16_t rank_handle, uint8_t status)
{
	struct btp_csip_discovered_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	ev.status = status;
	ev.sirk_handle = sys_cpu_to_le16(sirk_handle);
	ev.size_handle = sys_cpu_to_le16(size_handle);
	ev.lock_handle = sys_cpu_to_le16(lock_handle);
	ev.rank_handle = sys_cpu_to_le16(rank_handle);

	tester_event(BTP_SERVICE_ID_CSIP, BTP_CSIP_DISCOVERED_EV, &ev, sizeof(ev));
}

static void btp_send_csip_sirk_ev(struct bt_conn *conn, uint8_t *sirk, size_t sirk_size)
{
	struct btp_csip_sirk_ev ev;

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));

	memcpy(ev.sirk, sirk, sirk_size);

	tester_event(BTP_SERVICE_ID_CSIP, BTP_CSIP_SIRK_EV, &ev, sizeof(ev));
}

static void btp_send_csip_lock_ev(int err)
{
	struct btp_csip_lock_ev ev;

	ev.status = err;

	tester_event(BTP_SERVICE_ID_CSIP, BTP_CSIP_LOCK_EV, &ev, sizeof(ev));
}

static void csip_set_coordinator_lock_set_cb(int err)
{
	LOG_DBG("");

	btp_send_csip_lock_ev(err);
}

static void csip_set_coordinator_lock_release_cb(int err)
{
	LOG_DBG("");

	btp_send_csip_lock_ev(err);
}

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member,
			     int err, size_t set_count)
{
	LOG_DBG("");

	uint8_t sirk[BT_CSIP_SET_SIRK_SIZE];
	size_t sirk_size = ARRAY_SIZE(sirk);
	uint8_t conn_index;

	if (err != 0) {
		LOG_DBG("discover failed (%d)", err);
		return;
	}

	if (set_count == 0) {
		LOG_DBG("Device has no sets");
		return;
	}

	conn_index = bt_conn_index(conn);

	LOG_DBG("Found %zu sets on member[%u]", set_count, conn_index);

	cur_csis_inst = &member->insts[0];

	memcpy(sirk, cur_csis_inst->info.set_sirk, sizeof(cur_csis_inst->info.set_sirk));

	btp_send_csip_sirk_ev(conn, sirk, sirk_size);

	for (size_t i = 0U; i < set_count; i++) {
		LOG_DBG("CSIS[%zu]: %p", i, &member->insts[i]);
		LOG_DBG("Rank: %u", member->insts[i].info.rank);
		LOG_DBG("Set Size: %u", member->insts[i].info.set_size);
		LOG_DBG("Lockable: %u", member->insts[i].info.lockable);
	}

	btp_csip_set_members[conn_index] = member;

	csip_inst = bt_csip_set_coordinator_lookup_instance_by_index(conn, conn_index);
	btp_send_csip_discovered_ev(conn, csip_inst->set_sirk_handle, csip_inst->set_size_handle,
				    csip_inst->set_lock_handle, csip_inst->rank_handle, err);
}

static void csip_lock_changed_cb(struct bt_csip_set_coordinator_csis_inst *inst,
				 bool locked)
{
	LOG_DBG("");
}

static void csip_set_coordinator_ordered_access_cb(
	const struct bt_csip_set_coordinator_set_info *set_info, int err,
	bool locked,  struct bt_csip_set_coordinator_set_member *member)
{
	LOG_DBG("");

	if (err) {
		LOG_ERR("Ordered access failed with err %d", err);
	} else if (locked) {
		LOG_DBG("Ordered access procedure locked member %p", member);
	} else {
		LOG_DBG("Ordered access procedure finished");
	}
}

static struct bt_csip_set_coordinator_cb set_coordinator_cbs = {
	.lock_set = csip_set_coordinator_lock_set_cb,
	.release_set = csip_set_coordinator_lock_release_cb,
	.discover = csip_discover_cb,
	.lock_changed = csip_lock_changed_cb,
	.ordered_access = csip_set_coordinator_ordered_access_cb
};

static bool csip_set_coordinator_oap_cb(const struct bt_csip_set_coordinator_set_info *set_info,
					struct bt_csip_set_coordinator_set_member *members[],
					size_t count)
{
	for (size_t i = 0; i < count; i++) {
		LOG_DBG("Ordered access for members[%zu]: %p", i, members[i]);
	}

	return true;
}

static uint8_t btp_csip_discover(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	int err;
	struct bt_conn *conn;
	const struct btp_csip_discover_cmd *cp = cmd;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");

		return BTP_STATUS_FAILED;
	}

	err = bt_csip_set_coordinator_discover(conn);
	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static uint8_t btp_csip_set_coordinator_lock(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_csip_set_coordinator_lock_cmd *cp = cmd;
	int err;

	LOG_DBG("");

	err = bt_csip_set_coordinator_lock(btp_csip_set_members, cp->count, &cur_csis_inst->info);

	if (err) {
		LOG_DBG("Failed to lock set members");
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_csip_set_coordinator_release(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_csip_set_coordinator_release_cmd *cp = cmd;
	int err;

	LOG_DBG("");

	err = bt_csip_set_coordinator_release(btp_csip_set_members, cp->count,
					      &cur_csis_inst->info);

	if (err) {
		LOG_DBG("Failed to release set members");
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_csip_start_ordered_access(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct bt_csip_set_coordinator_set_member *members[ARRAY_SIZE(btp_csip_set_members)];
	unsigned long member_count = 0;
	int err;

	LOG_DBG("");

	if (cur_csis_inst == NULL) {
		LOG_ERR("No CISP instance available");

		return BTP_STATUS_FAILED;
	}

	for (size_t i = 0; i < (size_t)ARRAY_SIZE(btp_csip_set_members); i++) {
		if (btp_csip_set_members[i] == NULL) {
			continue;
		}

		members[member_count++] = btp_csip_set_members[i];
	}

	if (member_count == 0) {
		LOG_ERR("No set members available");

		return BTP_STATUS_FAILED;
	}

	err = bt_csip_set_coordinator_ordered_access(members,
						     member_count,
						     &cur_csis_inst->info,
						     csip_set_coordinator_oap_cb);

	return BTP_STATUS_VAL(err);
}

static const struct btp_handler csip_handlers[] = {
	{
		.opcode = BTP_CSIP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = btp_csip_supported_commands
	},
	{
		.opcode = BTP_CSIP_DISCOVER,
		.expect_len = sizeof(struct btp_csip_discover_cmd),
		.func = btp_csip_discover
	},
	{
		.opcode = BTP_CSIP_START_ORDERED_ACCESS,
		.expect_len = sizeof(struct btp_csip_start_ordered_access_cmd),
		.func = btp_csip_start_ordered_access
	},
	{
		.opcode = BTP_CSIP_SET_COORDINATOR_LOCK,
		.expect_len = sizeof(struct btp_csip_set_coordinator_lock_cmd),
		.func = btp_csip_set_coordinator_lock,
	},
	{
		.opcode = BTP_CSIP_SET_COORDINATOR_RELEASE,
		.expect_len = sizeof(struct btp_csip_set_coordinator_release_cmd),
		.func = btp_csip_set_coordinator_release,
	},
};

uint8_t tester_init_csip(void)
{
	bt_csip_set_coordinator_register_cb(&set_coordinator_cbs);

	tester_register_command_handlers(BTP_SERVICE_ID_CSIP, csip_handlers,
					 ARRAY_SIZE(csip_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_csip(void)
{
	return BTP_STATUS_SUCCESS;
}
