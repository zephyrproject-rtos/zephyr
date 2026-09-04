/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Stub implementations for the external symbols referenced by adv.c that are
 * not exercised by the PAwR response report reassembly tests. Only the HCI
 * command path used by bt_le_ext_adv_create() needs meaningful behaviour; the
 * rest are no-op stubs so that adv.c links in the native unit test build.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/fff.h>

#include <host/hci_core.h>
#include <host/conn_internal.h>

/* Required by the shared host_mocks assert helper (uses the fff framework). */
DEFINE_FFF_GLOBALS;

/* The single controller state instance referenced by adv.c. */
struct bt_dev bt_dev = {0};

const bt_addr_le_t *bt_lookup_id_addr(uint8_t id, const bt_addr_le_t *addr)
{
	ARG_UNUSED(id);

	return addr;
}

bool bt_addr_le_is_resolved(const bt_addr_le_t *addr)
{
	ARG_UNUSED(addr);

	return false;
}

void bt_addr_le_copy_resolved(bt_addr_le_t *dst, const bt_addr_le_t *src)
{
	*dst = *src;
}

/*
 * Backing storage for the net_buf returned by bt_hci_cmd_alloc(). A single
 * static buffer is sufficient because the command is fully consumed by
 * bt_hci_cmd_send_sync() before the next allocation.
 */
static uint8_t cmd_buf_storage[128];
static struct net_buf test_cmd_buf;

/* Backing storage for the response returned by bt_hci_cmd_send_sync(). */
static struct bt_hci_rp_le_set_ext_adv_param cmd_rsp_payload;
static uint8_t rsp_buf_storage[sizeof(cmd_rsp_payload)];
static struct net_buf test_rsp_buf;

struct net_buf *bt_hci_cmd_alloc(k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	net_buf_simple_init_with_data(&test_cmd_buf.b, cmd_buf_storage,
				      sizeof(cmd_buf_storage));
	net_buf_simple_reset(&test_cmd_buf.b);

	return &test_cmd_buf;
}

int bt_hci_cmd_send_sync(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	ARG_UNUSED(opcode);
	ARG_UNUSED(buf);

	if (rsp != NULL) {
		cmd_rsp_payload.status = 0;
		cmd_rsp_payload.tx_power = 0;

		net_buf_simple_init_with_data(&test_rsp_buf.b, rsp_buf_storage,
					      sizeof(rsp_buf_storage));
		net_buf_simple_reset(&test_rsp_buf.b);
		net_buf_simple_add_mem(&test_rsp_buf.b, &cmd_rsp_payload,
				       sizeof(cmd_rsp_payload));

		*rsp = &test_rsp_buf;
	}

	return 0;
}

void bt_hci_cmd_state_set_init(struct net_buf *buf, struct bt_hci_cmd_state_set *state,
			       atomic_t *target, int bit, bool val)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(state);
	ARG_UNUSED(target);
	ARG_UNUSED(bit);
	ARG_UNUSED(val);
}

void net_buf_unref(struct net_buf *buf)
{
	ARG_UNUSED(buf);
}

int bt_get_df_cte_type(uint8_t hci_cte_type)
{
	return hci_cte_type;
}

int bt_id_set_adv_own_addr(struct bt_le_ext_adv *adv, uint32_t options, bool dir_adv,
			   uint8_t *own_addr_type)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(options);
	ARG_UNUSED(dir_adv);

	*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;

	return 0;
}

int bt_id_set_adv_random_addr(struct bt_le_ext_adv *adv, const bt_addr_t *rpa)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(rpa);

	return 0;
}

int bt_id_set_adv_private_addr(struct bt_le_ext_adv *adv)
{
	ARG_UNUSED(adv);

	return 0;
}

void bt_id_save_adv_addr(struct bt_le_ext_adv *adv, uint8_t own_addr_type)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(own_addr_type);
}

int bt_id_set_private_addr(uint8_t id)
{
	ARG_UNUSED(id);

	return 0;
}

bool bt_id_adv_random_addr_check(const struct bt_le_adv_param *param)
{
	ARG_UNUSED(param);

	return true;
}

void bt_id_adv_limited_stopped(struct bt_le_ext_adv *adv)
{
	ARG_UNUSED(adv);
}

void bt_id_pending_keys_update(void)
{
}

int bt_le_scan_set_enable(uint8_t enable)
{
	ARG_UNUSED(enable);

	return 0;
}

void bt_hci_le_enh_conn_complete(struct bt_hci_evt_le_enh_conn_complete *evt,
				 const struct bt_le_ext_adv *ext_adv)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(ext_adv);
}

struct bt_conn *bt_conn_add_le(uint8_t id, const bt_addr_le_t *peer)
{
	ARG_UNUSED(id);
	ARG_UNUSED(peer);

	return NULL;
}

bool bt_conn_exists_le(uint8_t id, const bt_addr_le_t *peer)
{
	ARG_UNUSED(id);
	ARG_UNUSED(peer);

	return false;
}

struct bt_conn *bt_conn_lookup_handle(uint16_t handle, enum bt_conn_type type)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(type);

	return NULL;
}

struct bt_conn *bt_conn_lookup_state_le(uint8_t id, const bt_addr_le_t *peer,
					const bt_conn_state_t state)
{
	ARG_UNUSED(id);
	ARG_UNUSED(peer);
	ARG_UNUSED(state);

	return NULL;
}

void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(state);
}

void bt_conn_unref(struct bt_conn *conn)
{
	ARG_UNUSED(conn);
}

int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	ARG_UNUSED(dwork);
	ARG_UNUSED(delay);

	return 0;
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	ARG_UNUSED(dwork);

	return 0;
}

int bt_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	ARG_UNUSED(dwork);
	ARG_UNUSED(delay);

	return 0;
}

int bt_work_cancel_delayable(struct k_work_delayable *dwork)
{
	ARG_UNUSED(dwork);

	return 0;
}


void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)
{
	ARG_UNUSED(dwork);
	ARG_UNUSED(handler);
}
