/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <version.h>
#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/drivers/bluetooth/hci_driver.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "../host/hci_ecc.h"

#include "util/util.h"
#include "util/memq.h"
#include "util/mem.h"
#include "util/dbuf.h"

#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "ll_sw/pdu_df.h"
#include "lll/pdu_vendor.h"
#include "ll_sw/pdu.h"

#include "ll_sw/lll.h"
#include "lll/lll_adv_types.h"
#include "ll_sw/lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "ll_sw/lll_scan.h"
#include "lll/lll_df_types.h"
#include "ll_sw/lll_sync.h"
#include "ll_sw/lll_sync_iso.h"
#include "ll_sw/lll_conn.h"
#include "ll_sw/lll_conn_iso.h"
#include "ll_sw/lll_iso_tx.h"

#include "ll_sw/isoal.h"

#include "ll_sw/ull_tx_queue.h"

#include "ll_sw/ull_adv_types.h"
#include "ll_sw/ull_scan_types.h"
#include "ll_sw/ull_sync_types.h"
#include "ll_sw/ull_conn_types.h"
#include "ll_sw/ull_iso_types.h"
#include "ll_sw/ull_conn_iso_types.h"
#include "ll_sw/ull_conn_iso_internal.h"
#include "ll_sw/ull_df_types.h"
#include "ll_sw/ull_internal.h"

#include "ll_sw/ull_adv_internal.h"
#include "ll_sw/ull_sync_internal.h"
#include "ll_sw/ull_conn_internal.h"
#include "ll_sw/ull_sync_iso_internal.h"
#include "ll_sw/ull_df_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#include "hci_internal.h"
#include "hci_vendor.h"

#if defined(CONFIG_BT_HCI_MESH_EXT)
#include "ll_sw/ll_mesh.h"
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_DTM_HCI)
#include "ll_sw/ll_test.h"
#endif /* CONFIG_BT_CTLR_DTM_HCI */

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "hci_user_ext.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#include "common/bt_str.h"
#include "hal/debug.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_hci);

#define STR_NULL_TERMINATOR 0x00

/* opcode of the HCI command currently being processed. The opcode is stored
 * by hci_cmd_handle() and then used during the creation of cmd complete and
 * cmd status events to avoid passing it up the call chain.
 */
static uint16_t _opcode;

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
/* NOTE: Duplicate filter uses two LS bits value of standard advertising modes:
 *       0 - Non-Connectable Non-Scannable advertising report
 *       1 - Connectable Non-Scannable advertising report
 *       2 - Non-Connectable Scannable advertisig report
 *       3 - Connectable Scannable advertising report
 *
 *       FIXME: Duplicate filtering of Connectable Directed low and high duty
 *              cycle. If advertiser changes between Connectable Non-Scannable,
 *              Connectable Directed low, and high duty cycle without changing
 *              SID and DID, then such reports will be filtered out by the
 *              implementation. Needs enhancement to current implementation.
 *
 *       Define a custom duplicate filter mode for periodic advertising:
 *       4 - Periodic Advertising report
 */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
#define DUP_EXT_ADV_MODE_MAX      5
#define DUP_EXT_ADV_MODE_PERIODIC BIT(2)
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
#define DUP_EXT_ADV_MODE_MAX      4
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */

#define DUP_EXT_ADV_MODE_COUNT    4

/* Duplicate filter entries, one per Bluetooth address */
static struct dup_entry {
	bt_addr_le_t addr;

	/* Mask to accumulate advertising PDU type as bitmask */
	uint8_t      mask;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	struct dup_ext_adv_mode {
		uint16_t set_count:5;
		uint16_t set_curr:5;
		struct dup_ext_adv_set {
			uint8_t data_cmplt:1;
			struct pdu_adv_adi adi;
		} set[CONFIG_BT_CTLR_DUP_FILTER_ADV_SET_MAX];
	} adv_mode[DUP_EXT_ADV_MODE_MAX];
#endif
} dup_filter[CONFIG_BT_CTLR_DUP_FILTER_LEN];

/* Duplicate filtering is disabled if count value is set to negative integer */
#define DUP_FILTER_DISABLED (-1)

/* Duplicate filtering array entry count, filtering disabled if negative */
static int32_t dup_count;
/* Duplicate filtering current free entry, overwrites entries after rollover */
static uint32_t dup_curr;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
/* Helper function to reset non-periodic advertising entries in filter table */
static void dup_ext_adv_reset(void);
/* Flag for advertising reports be filtered for duplicates. */
static bool dup_scan;
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
/* Set constant true so that (dup_count >= 0) decides if advertising duplicate
 * filter is enabled when Periodic Advertising ADI support is disabled.
 */
static const bool dup_scan = true;
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

#if defined(CONFIG_BT_HCI_MESH_EXT)
struct scan_filter {
	uint8_t count;
	uint8_t lengths[CONFIG_BT_CTLR_MESH_SF_PATTERNS];
	uint8_t patterns[CONFIG_BT_CTLR_MESH_SF_PATTERNS]
		     [BT_HCI_MESH_PATTERN_LEN_MAX];
};

static struct scan_filter scan_filters[CONFIG_BT_CTLR_MESH_SCAN_FILTERS];
static uint8_t sf_curr;
#endif

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
int32_t    hci_hbuf_total;
uint32_t    hci_hbuf_sent;
uint32_t    hci_hbuf_acked;
uint16_t    hci_hbuf_pend[CONFIG_BT_MAX_CONN];
atomic_t hci_state_mask;
static struct k_poll_signal *hbuf_signal;
#endif

#if defined(CONFIG_BT_CONN)
static uint32_t conn_count;
#endif

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static uint32_t cis_pending_count;
#endif

/* In HCI event PHY indices start at 1 compare to 0 indexed in aux_ptr field in
 * the Common Extended Payload Format in the PDUs.
 */
#define HCI_AUX_PHY_TO_HCI_PHY(aux_phy) ((aux_phy) + 1)

#define DEFAULT_EVENT_MASK           0x1fffffffffff
#define DEFAULT_EVENT_MASK_PAGE_2    0x0
#define DEFAULT_LE_EVENT_MASK 0x1f

static uint64_t event_mask = DEFAULT_EVENT_MASK;
static uint64_t event_mask_page_2 = DEFAULT_EVENT_MASK_PAGE_2;
static uint64_t le_event_mask = DEFAULT_LE_EVENT_MASK;
#if defined(CONFIG_BT_HCI_VS_EVT)
static uint64_t vs_events_mask = DEFAULT_VS_EVT_MASK;
#endif /* CONFIG_BT_HCI_VS_EVT */

static struct net_buf *cmd_complete_status(uint8_t status);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define BUF_GET_TIMEOUT K_SECONDS(10)

#if defined(CONFIG_BT_HCI_RAW)
static uint8_t ll_adv_cmds;

__weak int ll_adv_cmds_set(uint8_t adv_cmds)
{
	if (!ll_adv_cmds) {
		ll_adv_cmds = adv_cmds;
	}

	if (ll_adv_cmds != adv_cmds) {
		return -EINVAL;
	}

	return 0;
}

__weak int ll_adv_cmds_is_ext(void)
{
	return ll_adv_cmds == LL_ADV_CMDS_EXT;
}

#else /* !CONFIG_BT_HCI_RAW */
__weak int ll_adv_cmds_is_ext(void)
{
	return 1;
}
#endif /* !CONFIG_BT_HCI_RAW */

static int adv_cmds_legacy_check(struct net_buf **cc_evt)
{
	int err;

#if defined(CONFIG_BT_HCI_RAW)
	err = ll_adv_cmds_set(LL_ADV_CMDS_LEGACY);
	if (err && cc_evt) {
		*cc_evt = cmd_complete_status(BT_HCI_ERR_CMD_DISALLOWED);
	}
#else
	if (cc_evt) {
		*cc_evt = cmd_complete_status(BT_HCI_ERR_CMD_DISALLOWED);
	}

	err = -EINVAL;
#endif /* CONFIG_BT_HCI_RAW */

	return err;
}

static int adv_cmds_ext_check(struct net_buf **cc_evt)
{
	int err;

#if defined(CONFIG_BT_HCI_RAW)
	err = ll_adv_cmds_set(LL_ADV_CMDS_EXT);
	if (err && cc_evt) {
		*cc_evt = cmd_complete_status(BT_HCI_ERR_CMD_DISALLOWED);
	}
#else
	err = 0;
#endif /* CONFIG_BT_HCI_RAW */

	return err;
}
#else
static inline int adv_cmds_legacy_check(struct net_buf **cc_evt)
{
	return 0;
}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CONN)
static void le_conn_complete(struct pdu_data *pdu_data, uint16_t handle,
			     struct net_buf *buf);
#endif /* CONFIG_BT_CONN */

static void hci_evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

void *hci_cmd_complete(struct net_buf **buf, uint8_t plen)
{
	*buf = bt_hci_cmd_complete_create(_opcode, plen);

	return net_buf_add(*buf, plen);
}

static struct net_buf *cmd_status(uint8_t status)
{
	return bt_hci_cmd_status_create(_opcode, status);
}

static struct net_buf *cmd_complete_status(uint8_t status)
{
	struct net_buf *buf;
	struct bt_hci_evt_cc_status *ccst;

	buf = bt_hci_cmd_complete_create(_opcode, sizeof(*ccst));
	ccst = net_buf_add(buf, sizeof(*ccst));
	ccst->status = status;

	return buf;
}

static void *meta_evt(struct net_buf *buf, uint8_t subevt, uint8_t melen)
{
	struct bt_hci_evt_le_meta_event *me;

	hci_evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*me) + melen);
	me = net_buf_add(buf, sizeof(*me));
	me->subevent = subevt;

	return net_buf_add(buf, melen);
}

#if defined(CONFIG_BT_HCI_VS_EVT)
static void *vs_event(struct net_buf *buf, uint8_t subevt, uint8_t evt_len)
{
	struct bt_hci_evt_vs *evt;

	hci_evt_create(buf, BT_HCI_EVT_VENDOR, sizeof(*evt) + evt_len);
	evt = net_buf_add(buf, sizeof(*evt));
	evt->subevent = subevt;

	return net_buf_add(buf, evt_len);
}
#endif /* CONFIG_BT_HCI_VS_EVT */

#if defined(CONFIG_BT_HCI_MESH_EXT)
static void *mesh_evt(struct net_buf *buf, uint8_t subevt, uint8_t melen)
{
	struct bt_hci_evt_mesh *me;

	hci_evt_create(buf, BT_HCI_EVT_VENDOR, sizeof(*me) + melen);
	me = net_buf_add(buf, sizeof(*me));
	me->prefix = BT_HCI_MESH_EVT_PREFIX;
	me->subevent = subevt;

	return net_buf_add(buf, melen);
}
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CONN)
static void disconnect(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_disconnect *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_terminate_ind_send(handle, cmd->reason);

	*evt = cmd_status(status);
}

static void read_remote_ver_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_remote_version_info *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_version_ind_send(handle);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CONN */

static int link_control_cmd_handle(uint16_t  ocf, struct net_buf *cmd,
				   struct net_buf **evt)
{
	switch (ocf) {
#if defined(CONFIG_BT_CONN)
	case BT_OCF(BT_HCI_OP_DISCONNECT):
		disconnect(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_READ_REMOTE_VERSION_INFO):
		read_remote_ver_info(cmd, evt);
		break;
#endif /* CONFIG_BT_CONN */
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_event_mask(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask *cmd = (void *)buf->data;

	event_mask = sys_get_le64(cmd->events);

	*evt = cmd_complete_status(0x00);
}

static void set_event_mask_page_2(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask_page_2 *cmd = (void *)buf->data;

	event_mask_page_2 = sys_get_le64(cmd->events_page_2);

	*evt = cmd_complete_status(0x00);
}

static void reset(struct net_buf *buf, struct net_buf **evt)
{
#if defined(CONFIG_BT_HCI_MESH_EXT)
	int i;

	for (i = 0; i < ARRAY_SIZE(scan_filters); i++) {
		scan_filters[i].count = 0U;
	}
	sf_curr = 0xFF;
#endif

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	dup_count = DUP_FILTER_DISABLED;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
	dup_scan = false;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	/* reset event masks */
	event_mask = DEFAULT_EVENT_MASK;
	event_mask_page_2 = DEFAULT_EVENT_MASK_PAGE_2;
	le_event_mask = DEFAULT_LE_EVENT_MASK;

	if (buf) {
		ll_reset();
		*evt = cmd_complete_status(0x00);
	}

#if defined(CONFIG_BT_CONN)
	conn_count = 0U;
#endif

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	cis_pending_count = 0U;
#endif

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hci_hbuf_total = 0;
	hci_hbuf_sent = 0U;
	hci_hbuf_acked = 0U;
	(void)memset(hci_hbuf_pend, 0, sizeof(hci_hbuf_pend));
	if (buf) {
		atomic_set_bit(&hci_state_mask, HCI_STATE_BIT_RESET);
		k_poll_signal_raise(hbuf_signal, 0x0);
	}
#endif

	hci_recv_fifo_reset();
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
static void set_ctl_to_host_flow(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_ctl_to_host_flow *cmd = (void *)buf->data;
	uint8_t flow_enable = cmd->flow_enable;
	struct bt_hci_evt_cc_status *ccst;

	ccst = hci_cmd_complete(evt, sizeof(*ccst));

	/* require host buffer size before enabling flow control, and
	 * disallow if any connections are up
	 */
	if (!hci_hbuf_total || conn_count) {
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	} else {
		ccst->status = 0x00;
	}

	switch (flow_enable) {
	case BT_HCI_CTL_TO_HOST_FLOW_DISABLE:
		if (hci_hbuf_total < 0) {
			/* already disabled */
			return;
		}
		break;
	case BT_HCI_CTL_TO_HOST_FLOW_ENABLE:
		if (hci_hbuf_total > 0) {
			/* already enabled */
			return;
		}
		break;
	default:
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	hci_hbuf_sent = 0U;
	hci_hbuf_acked = 0U;
	(void)memset(hci_hbuf_pend, 0, sizeof(hci_hbuf_pend));
	hci_hbuf_total = -hci_hbuf_total;
}

static void host_buffer_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_host_buffer_size *cmd = (void *)buf->data;
	uint16_t acl_pkts = sys_le16_to_cpu(cmd->acl_pkts);
	uint16_t acl_mtu = sys_le16_to_cpu(cmd->acl_mtu);
	struct bt_hci_evt_cc_status *ccst;

	ccst = hci_cmd_complete(evt, sizeof(*ccst));

	if (hci_hbuf_total) {
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	}
	/* fragmentation from controller to host not supported, require
	 * ACL MTU to be at least the LL MTU
	 */
	if (acl_mtu < LL_LENGTH_OCTETS_RX_MAX) {
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	LOG_DBG("FC: host buf size: %d", acl_pkts);
	hci_hbuf_total = -acl_pkts;
}

static void host_num_completed_packets(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_host_num_completed_packets *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint32_t count = 0U;

	/* special case, no event returned except for error conditions */
	if (hci_hbuf_total <= 0) {
		ccst = hci_cmd_complete(evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;
		return;
	} else if (!conn_count) {
		ccst = hci_cmd_complete(evt, sizeof(*ccst));
		ccst->status = BT_HCI_ERR_INVALID_PARAM;
		return;
	}

	/* leave *evt == NULL so no event is generated */
	for (uint8_t i = 0; i < cmd->num_handles; i++) {
		uint16_t h = sys_le16_to_cpu(cmd->h[i].handle);
		uint16_t c = sys_le16_to_cpu(cmd->h[i].count);

		if ((h >= ARRAY_SIZE(hci_hbuf_pend)) ||
		    (c > hci_hbuf_pend[h])) {
			ccst = hci_cmd_complete(evt, sizeof(*ccst));
			ccst->status = BT_HCI_ERR_INVALID_PARAM;
			return;
		}

		hci_hbuf_pend[h] -= c;
		count += c;
	}

	LOG_DBG("FC: acked: %d", count);
	hci_hbuf_acked += count;
	k_poll_signal_raise(hbuf_signal, 0x0);
}
#endif

#if defined(CONFIG_BT_CTLR_LE_PING)
static void read_auth_payload_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_auth_payload_timeout *cmd = (void *)buf->data;
	struct bt_hci_rp_read_auth_payload_timeout *rp;
	uint16_t auth_payload_timeout;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_apto_get(handle, &auth_payload_timeout);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
	rp->auth_payload_timeout = sys_cpu_to_le16(auth_payload_timeout);
}

static void write_auth_payload_timeout(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_write_auth_payload_timeout *cmd = (void *)buf->data;
	struct bt_hci_rp_write_auth_payload_timeout *rp;
	uint16_t auth_payload_timeout;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	auth_payload_timeout = sys_le16_to_cpu(cmd->auth_payload_timeout);

	status = ll_apto_set(handle, auth_payload_timeout);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO)
static void configure_data_path(struct net_buf *buf,
				struct net_buf **evt)
{
	struct bt_hci_cp_configure_data_path *cmd = (void *)buf->data;
	struct bt_hci_rp_configure_data_path *rp;

	uint8_t *vs_config;
	uint8_t status;

	vs_config = &cmd->vs_config[0];

	if (IS_ENABLED(CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH)) {
		status = ll_configure_data_path(cmd->data_path_dir,
						cmd->data_path_id,
						cmd->vs_config_len,
						vs_config);
	} else {
		status = BT_HCI_ERR_INVALID_PARAM;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
}
#endif /* CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
static void read_conn_accept_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_conn_accept_timeout *rp;
	uint16_t timeout;

	ARG_UNUSED(buf);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = ll_conn_iso_accept_timeout_get(&timeout);
	rp->conn_accept_timeout = sys_cpu_to_le16(timeout);
}

static void write_conn_accept_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_write_conn_accept_timeout *cmd = (void *)buf->data;
	struct bt_hci_rp_write_conn_accept_timeout *rp;
	uint16_t timeout;

	timeout = sys_le16_to_cpu(cmd->conn_accept_timeout);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = ll_conn_iso_accept_timeout_set(timeout);
}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CONN)
static void read_tx_power_level(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_tx_power_level *cmd = (void *)buf->data;
	struct bt_hci_rp_read_tx_power_level *rp;
	uint16_t handle;
	uint8_t status;
	uint8_t type;

	handle = sys_le16_to_cpu(cmd->handle);
	type = cmd->type;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	status = ll_tx_pwr_lvl_get(BT_HCI_VS_LL_HANDLE_TYPE_CONN,
				   handle, type, &rp->tx_power_level);

	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CONN */

static int ctrl_bb_cmd_handle(uint16_t  ocf, struct net_buf *cmd,
			      struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK):
		set_event_mask(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_RESET):
		reset(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_SET_EVENT_MASK_PAGE_2):
		set_event_mask_page_2(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case BT_OCF(BT_HCI_OP_READ_CONN_ACCEPT_TIMEOUT):
		read_conn_accept_timeout(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_WRITE_CONN_ACCEPT_TIMEOUT):
		write_conn_accept_timeout(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CONN)
	case BT_OCF(BT_HCI_OP_READ_TX_POWER_LEVEL):
		read_tx_power_level(cmd, evt);
		break;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	case BT_OCF(BT_HCI_OP_SET_CTL_TO_HOST_FLOW):
		set_ctl_to_host_flow(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_HOST_BUFFER_SIZE):
		host_buffer_size(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS):
		host_num_completed_packets(cmd, evt);
		break;
#endif

#if defined(CONFIG_BT_CTLR_LE_PING)
	case BT_OCF(BT_HCI_OP_READ_AUTH_PAYLOAD_TIMEOUT):
		read_auth_payload_timeout(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_WRITE_AUTH_PAYLOAD_TIMEOUT):
		write_auth_payload_timeout(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO)
	case BT_OCF(BT_HCI_OP_CONFIGURE_DATA_PATH):
		configure_data_path(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO */

	default:
		return -EINVAL;
	}

	return 0;
}

static void read_local_version_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_local_version_info *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hci_version = LL_VERSION_NUMBER;
	rp->hci_revision = sys_cpu_to_le16(0);
	rp->lmp_version = LL_VERSION_NUMBER;
	rp->manufacturer = sys_cpu_to_le16(ll_settings_company_id());
	rp->lmp_subversion = sys_cpu_to_le16(ll_settings_subversion_number());
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->commands[0], 0, sizeof(rp->commands));

#if defined(CONFIG_BT_REMOTE_VERSION)
	/* Read Remote Version Info. */
	rp->commands[2] |= BIT(7);
#endif
	/* Set Event Mask, and Reset. */
	rp->commands[5] |= BIT(6) | BIT(7);

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	/* Read/Write Connection Accept Timeout */
	rp->commands[7] |= BIT(2) | BIT(3);
#endif /* CONFIG_BT_CTLR_CONN_ISO */

	/* Read TX Power Level. */
	rp->commands[10] |= BIT(2);

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* Set FC, Host Buffer Size and Host Num Completed */
	rp->commands[10] |= BIT(5) | BIT(6) | BIT(7);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

	/* Read Local Version Info, Read Local Supported Features. */
	rp->commands[14] |= BIT(3) | BIT(5);
	/* Read BD ADDR. */
	rp->commands[15] |= BIT(1);

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	/* Read RSSI. */
	rp->commands[15] |= BIT(5);
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	/* Set Event Mask Page 2 */
	rp->commands[22] |= BIT(2);
	/* LE Set Event Mask, LE Read Buffer Size, LE Read Local Supp Feats,
	 * Set Random Addr
	 */
	rp->commands[25] |= BIT(0) | BIT(1) | BIT(2) | BIT(4);

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
	/* LE Read FAL Size, LE Clear FAL */
	rp->commands[26] |= BIT(6) | BIT(7);
	/* LE Add Dev to FAL, LE Remove Dev from FAL */
	rp->commands[27] |= BIT(0) | BIT(1);
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

	/* LE Encrypt, LE Rand */
	rp->commands[27] |= BIT(6) | BIT(7);
	/* LE Read Supported States */
	rp->commands[28] |= BIT(3);

#if defined(CONFIG_BT_BROADCASTER)
	/* LE Set Adv Params, LE Read Adv Channel TX Power, LE Set Adv Data */
	rp->commands[25] |= BIT(5) | BIT(6) | BIT(7);
	/* LE Set Scan Response Data, LE Set Adv Enable */
	rp->commands[26] |= BIT(0) | BIT(1);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* LE Set Adv Set Random Addr, LE Set Ext Adv Params, LE Set Ext Adv
	 * Data, LE Set Ext Adv Scan Rsp Data, LE Set Ext Adv Enable, LE Read
	 * Max Adv Data Len, LE Read Num Supp Adv Sets
	 */
	rp->commands[36] |= BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) |
			    BIT(6) | BIT(7);
	/* LE Remove Adv Set, LE Clear Adv Sets */
	rp->commands[37] |= BIT(0) | BIT(1);
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	/* LE Set PA Params, LE Set PA Data, LE Set PA Enable */
	rp->commands[37] |= BIT(2) | BIT(3) | BIT(4);
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	/* LE Create BIG, LE Create BIG Test, LE Terminate BIG */
	rp->commands[42] |= BIT(5) | BIT(6) | BIT(7);
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	/* LE Set Scan Params, LE Set Scan Enable */
	rp->commands[26] |= BIT(2) | BIT(3);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* LE Set Extended Scan Params, LE Set Extended Scan Enable */
	rp->commands[37] |= BIT(5) | BIT(6);
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	/* LE PA Create Sync, LE PA Create Sync Cancel, LE PA Terminate Sync */
	rp->commands[38] |= BIT(0) | BIT(1) | BIT(2);
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
	/* LE PA Add Device to Periodic Advertiser List,
	 * LE PA Remove Device from Periodic Advertiser List,
	 * LE Clear Periodic Advertiser List,
	 * LE Read Periodic Adveritiser List Size
	 */
	rp->commands[38] |= BIT(3) | BIT(4) | BIT(5) | BIT(6);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */
	/* LE Set PA Receive Enable */
	rp->commands[40] |= BIT(5);
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	/* LE BIG Create Sync, LE BIG Terminate Sync */
	rp->commands[43] |= BIT(0) | BIT(1);
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	/* LE Create Connection, LE Create Connection Cancel */
	rp->commands[26] |= BIT(4) | BIT(5);
	/* Set Host Channel Classification */
	rp->commands[27] |= BIT(3);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	/* LE Extended Create Connection */
	rp->commands[37] |= BIT(7);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* LE Start Encryption */
	rp->commands[28] |= BIT(0);
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	/* LE Set CIG Parameters */
	rp->commands[41] |= BIT(7);
	/* LE Set CIG Parameters Test, LE Create CIS, LE Remove CIS */
	rp->commands[42] |= BIT(0) | BIT(1) | BIT(2);
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
	/* LE LTK Request Reply, LE LTK Request Negative Reply */
	rp->commands[28] |= BIT(1) | BIT(2);
#endif /* CONFIG_BT_CTLR_LE_ENC */
#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	/* LE Accept CIS Request, LE Reject CIS Request */
	rp->commands[42] |= BIT(3) | BIT(4);
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */
#endif /* CONFIG_BT_PERIPHERAL */

	/* Disconnect. */
	rp->commands[0] |= BIT(5);
	/* LE Connection Update, LE Read Channel Map, LE Read Remote Features */
	rp->commands[27] |= BIT(2) | BIT(4) | BIT(5);

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	/* LE Remote Conn Param Req and Neg Reply */
	rp->commands[33] |= BIT(4) | BIT(5);
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_LE_PING)
	/* Read and Write authenticated payload timeout */
	rp->commands[32] |= BIT(4) | BIT(5);
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	/* LE Set Data Length, and LE Read Suggested Data Length. */
	rp->commands[33] |= BIT(6) | BIT(7);
	/* LE Write Suggested Data Length. */
	rp->commands[34] |= BIT(0);
	/* LE Read Maximum Data Length. */
	rp->commands[35] |= BIT(3);
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	/* LE Read PHY Command. */
	rp->commands[35] |= BIT(4);
	/* LE Set Default PHY Command. */
	rp->commands[35] |= BIT(5);
	/* LE Set PHY Command. */
	rp->commands[35] |= BIT(6);
#endif /* CONFIG_BT_CTLR_PHY */
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	/* LE Request Peer SCA */
	rp->commands[43] |= BIT(2);
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_DTM_HCI)
	/* LE RX Test, LE TX Test, LE Test End */
	rp->commands[28] |= BIT(4) | BIT(5) | BIT(6);
	/* LE Enhanced RX Test. */
	rp->commands[35] |= BIT(7);
	/* LE Enhanced TX Test. */
	rp->commands[36] |= BIT(0);
#if defined(CONFIG_BT_CTLR_DTM_HCI_RX_V3)
	rp->commands[39] |= BIT(3);
#endif /* CONFIG_BT_CTLR_DTM_HCI_RX_V3 */

#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V3)
	rp->commands[39] |= BIT(4);
#endif

#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V4)
	rp->commands[45] |= BIT(0);
#endif
#endif /* CONFIG_BT_CTLR_DTM_HCI */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* LE resolving list commands, LE Read Peer RPA */
	rp->commands[34] |= BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(7);
	/* LE Read Local RPA, LE Set AR Enable, Set RPA Timeout */
	rp->commands[35] |= BIT(0) | BIT(1) | BIT(2);
	/* LE Set Privacy Mode */
	rp->commands[39] |= BIT(2);
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_DF)
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	/* LE Set Connectionless CTE Transmit Parameters,
	 * LE Set Connectionless CTE Transmit Enable
	 */
	rp->commands[39] |= BIT(5) | BIT(6);
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	/* LE Set Connectionless IQ Sampling Enable */
	rp->commands[39] |= BIT(7);
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	/* LE Read Antenna Information */
	rp->commands[40] |= BIT(4);
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
	/* LE Set Connection CTE Transmit Parameters */
	rp->commands[40] |= BIT(1);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	/* LE Set Connection CTE Receive Parameters */
	rp->commands[40] |= BIT(0);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	/* LE Connection CTE Request Enable */
	rp->commands[40] |= BIT(2);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	/* LE Connection CTE Response Enable */
	rp->commands[40] |= BIT(3);
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

#endif /* CONFIG_BT_CTLR_DF */

#if defined(CONFIG_BT_HCI_RAW) && defined(CONFIG_BT_TINYCRYPT_ECC)
	bt_hci_ecc_supported_commands(rp->commands);
#endif /* CONFIG_BT_HCI_RAW && CONFIG_BT_TINYCRYPT_ECC */

	/* LE Read TX Power. */
	rp->commands[38] |= BIT(7);

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	/* LE Read Buffer Size v2, LE Read ISO TX Sync */
	rp->commands[41] |= BIT(5) | BIT(6);
	/* LE ISO Transmit Test */
	rp->commands[43] |= BIT(5);
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	/* LE ISO Receive Test, LE ISO Read Test Counters */
	rp->commands[43] |= BIT(6) | BIT(7);

#if defined(CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY)
	/* LE Read ISO Link Quality */
	rp->commands[44] |= BIT(2);
#endif /* CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY */
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ISO)
	/* LE Setup ISO Data Path, LE Remove ISO Data Path */
	rp->commands[43] |= BIT(3) | BIT(4);
	/* LE ISO Test End */
	rp->commands[44] |= BIT(0);
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_SET_HOST_FEATURE)
	/* LE Set Host Feature */
	rp->commands[44] |= BIT(1);
#endif /* CONFIG_BT_CTLR_SET_HOST_FEATURE */

#if defined(CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO)
	/* Read Supported Codecs [v2], Codec Capabilities, Controller Delay, Configure Data Path */
	rp->commands[45] |= BIT(2) | BIT(3) | BIT(4) | BIT(5);
#endif /* CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO */
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->features[0], 0x00, sizeof(rp->features));
	/* BR/EDR not supported and LE supported */
	rp->features[4] = (1 << 5) | (1 << 6);
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_bd_addr *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	(void)ll_addr_read(0, &rp->bdaddr.val[0]);
}

#if defined(CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO)
uint8_t __weak hci_vendor_read_std_codecs(
	const struct bt_hci_std_codec_info_v2 **codecs)
{
	ARG_UNUSED(codecs);

	/* return number of supported codecs */
	return 0;
}

uint8_t __weak hci_vendor_read_vs_codecs(
	const struct bt_hci_vs_codec_info_v2 **codecs)
{
	ARG_UNUSED(codecs);

	/* return number of supported codecs */
	return 0;
}

/* NOTE: Not implementing the [v1] version.
 * Refer to BT Spec v5.3 Vol 4, Part E 7.4.8 Read Local Supported Codecs command
 * The [v1] version of this command shall only return codecs supported on the
 * BR/EDR physical transport, while the [v2] version shall return codecs
 * supported on all physical transports.
 */
static void read_codecs_v2(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_read_codecs_v2 *rp;
	const struct bt_hci_std_codec_info_v2 *std_codec_info;
	const struct bt_hci_vs_codec_info_v2 *vs_codec_info;
	struct bt_hci_std_codecs_v2 *std_codecs;
	struct bt_hci_vs_codecs_v2 *vs_codecs;
	size_t std_codecs_bytes;
	size_t vs_codecs_bytes;
	uint8_t num_std_codecs;
	uint8_t num_vs_codecs;
	uint8_t i;

	/* read standard codec information */
	num_std_codecs = hci_vendor_read_std_codecs(&std_codec_info);
	std_codecs_bytes = sizeof(struct bt_hci_std_codecs_v2) +
		num_std_codecs * sizeof(struct bt_hci_std_codec_info_v2);
	/* read vendor-specific codec information */
	num_vs_codecs = hci_vendor_read_vs_codecs(&vs_codec_info);
	vs_codecs_bytes = sizeof(struct bt_hci_vs_codecs_v2) +
		num_vs_codecs *	sizeof(struct bt_hci_vs_codec_info_v2);

	/* allocate response packet */
	rp = hci_cmd_complete(evt, sizeof(*rp) +
			      std_codecs_bytes +
			      vs_codecs_bytes);
	rp->status = 0x00;

	/* copy standard codec information */
	std_codecs = (struct bt_hci_std_codecs_v2 *)&rp->codecs[0];
	std_codecs->num_codecs = num_std_codecs;
	for (i = 0; i < num_std_codecs; i++) {
		struct bt_hci_std_codec_info_v2 *codec;

		codec = &std_codecs->codec_info[i];
		codec->codec_id = std_codec_info[i].codec_id;
		codec->transports = std_codec_info[i].transports;
	}

	/* copy vendor specific codec information  */
	vs_codecs = (struct bt_hci_vs_codecs_v2 *)&rp->codecs[std_codecs_bytes];
	vs_codecs->num_codecs = num_vs_codecs;
	for (i = 0; i < num_std_codecs; i++) {
		struct bt_hci_vs_codec_info_v2 *codec;

		codec = &vs_codecs->codec_info[i];
		codec->company_id =
			sys_cpu_to_le16(vs_codec_info[i].company_id);
		codec->codec_id = sys_cpu_to_le16(vs_codec_info[i].codec_id);
		codec->transports = vs_codec_info[i].transports;
	}
}

uint8_t __weak hci_vendor_read_codec_capabilities(uint8_t coding_format,
						  uint16_t company_id,
						  uint16_t vs_codec_id,
						  uint8_t transport,
						  uint8_t direction,
						  uint8_t *num_capabilities,
						  size_t *capabilities_bytes,
						  const uint8_t **capabilities)
{
	ARG_UNUSED(coding_format);
	ARG_UNUSED(company_id);
	ARG_UNUSED(vs_codec_id);
	ARG_UNUSED(transport);
	ARG_UNUSED(direction);
	ARG_UNUSED(capabilities);

	*num_capabilities = 0;
	*capabilities_bytes = 0;

	/* return status */
	return 0x00;
}

static void read_codec_capabilities(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_codec_capabilities *cmd = (void *)buf->data;
	struct bt_hci_rp_read_codec_capabilities *rp;
	const uint8_t *capabilities;
	size_t capabilities_bytes;
	uint8_t num_capabilities;
	uint16_t vs_codec_id;
	uint16_t company_id;
	uint8_t status;

	company_id = sys_le16_to_cpu(cmd->codec_id.company_id);
	vs_codec_id = sys_le16_to_cpu(cmd->codec_id.vs_codec_id);

	/* read codec capabilities */
	status = hci_vendor_read_codec_capabilities(cmd->codec_id.coding_format,
						    company_id,
						    vs_codec_id,
						    cmd->transport,
						    cmd->direction,
						    &num_capabilities,
						    &capabilities_bytes,
						    &capabilities);

	/* allocate response packet */
	rp = hci_cmd_complete(evt, sizeof(*rp) + capabilities_bytes);
	rp->status = status;

	/* copy codec capabilities information */
	rp->num_capabilities = num_capabilities;
	memcpy(&rp->capabilities, capabilities, capabilities_bytes);
}

uint8_t __weak hci_vendor_read_ctlr_delay(uint8_t coding_format,
					  uint16_t company_id,
					  uint16_t vs_codec_id,
					  uint8_t transport,
					  uint8_t direction,
					  uint8_t codec_config_len,
					  const uint8_t *codec_config,
					  uint32_t *min_delay,
					  uint32_t *max_delay)
{
	ARG_UNUSED(coding_format);
	ARG_UNUSED(company_id);
	ARG_UNUSED(vs_codec_id);
	ARG_UNUSED(transport);
	ARG_UNUSED(direction);
	ARG_UNUSED(codec_config_len);
	ARG_UNUSED(codec_config);

	*min_delay = 0;
	*max_delay = 0x3D0900; /* 4 seconds, maximum value allowed by spec */

	/* return status */
	return 0x00;
}

static void read_ctlr_delay(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_ctlr_delay *cmd = (void *)buf->data;
	struct bt_hci_rp_read_ctlr_delay *rp;
	uint16_t vs_codec_id;
	uint16_t company_id;
	uint32_t min_delay;
	uint32_t max_delay;
	uint8_t status;

	company_id = sys_le16_to_cpu(cmd->codec_id.company_id);
	vs_codec_id = sys_le16_to_cpu(cmd->codec_id.vs_codec_id);

	status = hci_vendor_read_ctlr_delay(cmd->codec_id.coding_format,
					    company_id,
					    vs_codec_id,
					    cmd->transport,
					    cmd->direction,
					    cmd->codec_config_len,
					    cmd->codec_config,
					    &min_delay,
					    &max_delay);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	sys_put_le24(min_delay, rp->min_ctlr_delay);
	sys_put_le24(max_delay, rp->max_ctlr_delay);
}
#endif /* CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO */

static int info_cmd_handle(uint16_t  ocf, struct net_buf *cmd,
			   struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_READ_LOCAL_VERSION_INFO):
		read_local_version_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_SUPPORTED_COMMANDS):
		read_supported_commands(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_LOCAL_FEATURES):
		read_local_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_BD_ADDR):
		read_bd_addr(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO)
	case BT_OCF(BT_HCI_OP_READ_CODECS_V2):
		read_codecs_v2(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_CODEC_CAPABILITIES):
		read_codec_capabilities(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_READ_CTLR_DELAY):
		read_ctlr_delay(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_HCI_CODEC_AND_DELAY_INFO */

	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
static void read_rssi(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_read_rssi *cmd = (void *)buf->data;
	struct bt_hci_rp_read_rssi *rp;
	uint16_t handle;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = ll_rssi_get(handle, &rp->rssi);

	rp->handle = sys_cpu_to_le16(handle);
	/* The Link Layer currently returns RSSI as an absolute value */
	rp->rssi = (!rp->status) ? -rp->rssi : 127;
}
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

static int status_cmd_handle(uint16_t  ocf, struct net_buf *cmd,
			     struct net_buf **evt)
{
	switch (ocf) {
#if defined(CONFIG_BT_CTLR_CONN_RSSI)
	case BT_OCF(BT_HCI_OP_READ_RSSI):
		read_rssi(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

	default:
		return -EINVAL;
	}

	return 0;
}

static void le_set_event_mask(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_set_event_mask *cmd = (void *)buf->data;

	le_event_mask = sys_get_le64(cmd->events);

	*evt = cmd_complete_status(0x00);
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->le_max_len = sys_cpu_to_le16(LL_LENGTH_OCTETS_TX_MAX);
	rp->le_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static void le_read_buffer_size_v2(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_buffer_size_v2 *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->acl_max_len = sys_cpu_to_le16(LL_LENGTH_OCTETS_TX_MAX);
	rp->acl_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
	rp->iso_max_len = sys_cpu_to_le16(CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE);
	rp->iso_max_num = CONFIG_BT_CTLR_ISO_TX_BUFFERS;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	(void)memset(&rp->features[0], 0x00, sizeof(rp->features));
	sys_put_le64(ll_feat_get(), rp->features);
}

static void le_set_random_address(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_random_address *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_addr_set(1, &cmd->bdaddr.val[0]);

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
static void le_read_fal_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_fal_size *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	rp->fal_size = ll_fal_size_get();
}

static void le_clear_fal(struct net_buf *buf, struct net_buf **evt)
{
	uint8_t status;

	status = ll_fal_clear();

	*evt = cmd_complete_status(status);
}

static void le_add_dev_to_fal(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_add_dev_to_fal *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_fal_add(&cmd->addr);

	*evt = cmd_complete_status(status);
}

static void le_rem_dev_from_fal(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rem_dev_from_fal *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_fal_remove(&cmd->addr);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

static void le_encrypt(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_encrypt *cmd = (void *)buf->data;
	struct bt_hci_rp_le_encrypt *rp;
	uint8_t enc_data[16];

	ecb_encrypt(cmd->key, cmd->plaintext, enc_data, NULL);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	memcpy(rp->enc_data, enc_data, 16);
}

static void le_rand(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_rand *rp;
	uint8_t count = sizeof(rp->rand);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	lll_csrand_get(rp->rand, count);
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_supp_states *rp;
	uint64_t states = 0U;

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

#define ST_ADV (BIT64(0)  | BIT64(1)  | BIT64(8)  | BIT64(9)  | BIT64(12) | \
		BIT64(13) | BIT64(16) | BIT64(17) | BIT64(18) | BIT64(19) | \
		BIT64(20) | BIT64(21))

#define ST_SCA (BIT64(4)  | BIT64(5)  | BIT64(8)  | BIT64(9)  | BIT64(10) | \
		BIT64(11) | BIT64(12) | BIT64(13) | BIT64(14) | BIT64(15) | \
		BIT64(22) | BIT64(23) | BIT64(24) | BIT64(25) | BIT64(26) | \
		BIT64(27) | BIT64(30) | BIT64(31))

#define ST_PER (BIT64(2)  | BIT64(3)  | BIT64(7)  | BIT64(10) | BIT64(11) | \
		BIT64(14) | BIT64(15) | BIT64(20) | BIT64(21) | BIT64(26) | \
		BIT64(27) | BIT64(29) | BIT64(30) | BIT64(31) | BIT64(32) | \
		BIT64(33) | BIT64(34) | BIT64(35) | BIT64(36) | BIT64(37) | \
		BIT64(38) | BIT64(39) | BIT64(40) | BIT64(41))

#define ST_CEN (BIT64(6)  | BIT64(16) | BIT64(17) | BIT64(18) | BIT64(19) | \
		BIT64(22) | BIT64(23) | BIT64(24) | BIT64(25) | BIT64(28) | \
		BIT64(32) | BIT64(33) | BIT64(34) | BIT64(35) | BIT64(36) | \
		BIT64(37) | BIT64(41))

#if defined(CONFIG_BT_BROADCASTER)
	states |= ST_ADV;
#else
	states &= ~ST_ADV;
#endif
#if defined(CONFIG_BT_OBSERVER)
	states |= ST_SCA;
#else
	states &= ~ST_SCA;
#endif
#if defined(CONFIG_BT_PERIPHERAL)
	states |= ST_PER;
#else
	states &= ~ST_PER;
#endif
#if defined(CONFIG_BT_CENTRAL)
	states |= ST_CEN;
#else
	states &= ~ST_CEN;
#endif
	/* All states and combinations supported except:
	 * Initiating State + Passive Scanning
	 * Initiating State + Active Scanning
	 */
	states &= ~(BIT64(22) | BIT64(23));
	LOG_DBG("states: 0x%08x%08x", (uint32_t)(states >> 32), (uint32_t)(states & 0xffffffff));
	sys_put_le64(states, rp->le_states);
}

#if defined(CONFIG_BT_BROADCASTER)
static void le_set_adv_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_param *cmd = (void *)buf->data;
	uint16_t min_interval;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

	min_interval = sys_le16_to_cpu(cmd->min_interval);

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    (cmd->type != BT_HCI_ADV_DIRECT_IND)) {
		uint16_t max_interval = sys_le16_to_cpu(cmd->max_interval);

		if ((min_interval > max_interval) ||
		    (min_interval < 0x0020) ||
		    (max_interval > 0x4000)) {
			*evt = cmd_complete_status(BT_HCI_ERR_INVALID_PARAM);
			return;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_adv_params_set(0, 0, min_interval, cmd->type,
				   cmd->own_addr_type, cmd->direct_addr.type,
				   &cmd->direct_addr.a.val[0], cmd->channel_map,
				   cmd->filter_policy, 0, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_adv_params_set(min_interval, cmd->type,
				   cmd->own_addr_type, cmd->direct_addr.type,
				   &cmd->direct_addr.a.val[0], cmd->channel_map,
				   cmd->filter_policy);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	*evt = cmd_complete_status(status);
}

static void le_read_adv_chan_tx_power(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_chan_tx_power *rp;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;

	rp->tx_power_level = 0;
}

static void le_set_adv_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_data *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_adv_data_set(0, cmd->len, &cmd->data[0]);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_adv_data_set(cmd->len, &cmd->data[0]);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	*evt = cmd_complete_status(status);
}

static void le_set_scan_rsp_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_rsp_data *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_adv_scan_rsp_set(0, cmd->len, &cmd->data[0]);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_adv_scan_rsp_set(cmd->len, &cmd->data[0]);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	*evt = cmd_complete_status(status);
}

static void le_set_adv_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_enable *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
#if defined(CONFIG_BT_HCI_MESH_EXT)
	status = ll_adv_enable(0, cmd->enable, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_HCI_MESH_EXT */
	status = ll_adv_enable(0, cmd->enable, 0, 0);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
	status = ll_adv_enable(cmd->enable);
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_ADV_ISO)
static void le_create_big(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_create_big *cmd = (void *)buf->data;
	uint32_t sdu_interval;
	uint16_t max_latency;
	uint8_t big_handle;
	uint8_t adv_handle;
	uint16_t max_sdu;
	uint8_t status;

	status = ll_adv_iso_by_hci_handle_new(cmd->big_handle, &big_handle);
	if (status) {
		*evt = cmd_status(status);
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->adv_handle, &adv_handle);
	if (status) {
		*evt = cmd_status(status);
		return;
	}

	sdu_interval = sys_get_le24(cmd->sdu_interval);
	max_sdu = sys_le16_to_cpu(cmd->max_sdu);
	max_latency = sys_le16_to_cpu(cmd->max_latency);

	status = ll_big_create(big_handle, adv_handle, cmd->num_bis,
			       sdu_interval, max_sdu, max_latency, cmd->rtn,
			       cmd->phy, cmd->packing, cmd->framing,
			       cmd->encryption, cmd->bcode);

	*evt = cmd_status(status);
}

static void le_create_big_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_create_big_test *cmd = (void *)buf->data;
	uint32_t sdu_interval;
	uint16_t iso_interval;
	uint16_t max_sdu;
	uint16_t max_pdu;
	uint8_t status;

	sdu_interval = sys_get_le24(cmd->sdu_interval);
	iso_interval = sys_le16_to_cpu(cmd->iso_interval);
	max_sdu = sys_le16_to_cpu(cmd->max_sdu);
	max_pdu = sys_le16_to_cpu(cmd->max_pdu);

	status = ll_big_test_create(cmd->big_handle, cmd->adv_handle,
				    cmd->num_bis, sdu_interval, iso_interval,
				    cmd->nse, max_sdu, max_pdu, cmd->phy,
				    cmd->packing, cmd->framing, cmd->bn,
				    cmd->irc, cmd->pto, cmd->encryption,
				    cmd->bcode);

	*evt = cmd_status(status);
}

static void le_terminate_big(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_terminate_big *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_big_terminate(cmd->big_handle, cmd->reason);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
static void le_set_scan_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_param *cmd = (void *)buf->data;
	uint16_t interval;
	uint16_t window;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

	interval = sys_le16_to_cpu(cmd->interval);
	window = sys_le16_to_cpu(cmd->window);

	status = ll_scan_params_set(cmd->scan_type, interval, window,
				    cmd->addr_type, cmd->filter_policy);

	*evt = cmd_complete_status(status);
}

static void le_set_scan_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_scan_enable *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_legacy_check(evt)) {
		return;
	}

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	/* Initialize duplicate filtering */
	if (cmd->enable && cmd->filter_dup) {
		if (0) {

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
		} else if (dup_count == DUP_FILTER_DISABLED) {
			dup_scan = true;

			/* All entries reset */
			dup_count = 0;
			dup_curr = 0U;
		} else if (!dup_scan) {
			dup_scan = true;
			dup_ext_adv_reset();
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */

		} else {
			/* All entries reset */
			dup_count = 0;
			dup_curr = 0U;
		}
	} else {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
		dup_scan = false;
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
		dup_count = DUP_FILTER_DISABLED;
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_scan_enable(cmd->enable, 0, 0);
#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_scan_enable(cmd->enable);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	/* NOTE: As filter duplicates is implemented here in HCI source code,
	 *       enabling of already enabled scanning shall succeed after
	 *       updates to filter duplicates is handled in the above
	 *       statements. Refer to BT Spec v5.0 Vol 2 Part E Section 7.8.11.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_SCAN_ENABLE_STRICT) &&
	    (status == BT_HCI_ERR_CMD_DISALLOWED)) {
		status = BT_HCI_ERR_SUCCESS;
	}

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
static void le_big_create_sync(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_big_create_sync *cmd = (void *)buf->data;
	uint8_t status;
	uint16_t sync_handle;
	uint16_t sync_timeout;

	sync_handle = sys_le16_to_cpu(cmd->sync_handle);
	sync_timeout = sys_le16_to_cpu(cmd->sync_timeout);

	status = ll_big_sync_create(cmd->big_handle, sync_handle,
				    cmd->encryption, cmd->bcode, cmd->mse,
				    sync_timeout, cmd->num_bis, cmd->bis);

	*evt = cmd_status(status);
}


static void le_big_terminate_sync(struct net_buf *buf, struct net_buf **evt,
				  void **node_rx)
{
	struct bt_hci_cp_le_big_terminate_sync *cmd = (void *)buf->data;
	struct bt_hci_rp_le_big_terminate_sync *rp;
	uint8_t big_handle;
	uint8_t status;

	big_handle = cmd->big_handle;
	status = ll_big_sync_terminate(big_handle, node_rx);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->big_handle = big_handle;
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CENTRAL)
static uint8_t check_cconn_params(bool ext, uint16_t scan_interval,
				  uint16_t scan_window,
				  uint16_t conn_interval_max,
				  uint16_t conn_latency,
				  uint16_t supervision_timeout)
{
	if (scan_interval < 0x0004 || scan_window < 0x0004 ||
	    (!ext && (scan_interval > 0x4000 || scan_window > 0x4000))) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (conn_interval_max < 0x0006 || conn_interval_max > 0x0C80) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (conn_latency > 0x01F3) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (supervision_timeout < 0x000A || supervision_timeout > 0x0C80) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	/* sto * 10ms > (1 + lat) * ci * 1.25ms * 2
	 * sto * 10 > (1 + lat) * ci * 2.5
	 * sto * 2 > (1 + lat) * ci * 0.5
	 * sto * 4 > (1 + lat) * ci
	 */
	if ((supervision_timeout << 2) <= ((1 + conn_latency) *
					   conn_interval_max)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	return 0;
}

static void le_create_connection(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_create_conn *cmd = (void *)buf->data;
	uint16_t supervision_timeout;
	uint16_t conn_interval_max;
	uint16_t scan_interval;
	uint16_t conn_latency;
	uint16_t scan_window;
	uint8_t status;

	if (adv_cmds_legacy_check(NULL)) {
		*evt = cmd_status(BT_HCI_ERR_CMD_DISALLOWED);
		return;
	}

	scan_interval = sys_le16_to_cpu(cmd->scan_interval);
	scan_window = sys_le16_to_cpu(cmd->scan_window);
	conn_interval_max = sys_le16_to_cpu(cmd->conn_interval_max);
	conn_latency = sys_le16_to_cpu(cmd->conn_latency);
	supervision_timeout = sys_le16_to_cpu(cmd->supervision_timeout);

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		status = check_cconn_params(false, scan_interval,
					    scan_window,
					    conn_interval_max,
					    conn_latency,
					    supervision_timeout);
		if (status) {
			*evt = cmd_status(status);
			return;
		}
	}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	status = ll_create_connection(scan_interval, scan_window,
				      cmd->filter_policy,
				      cmd->peer_addr.type,
				      &cmd->peer_addr.a.val[0],
				      cmd->own_addr_type, conn_interval_max,
				      conn_latency, supervision_timeout,
				      PHY_LEGACY);
	if (status) {
		*evt = cmd_status(status);
		return;
	}

	status = ll_connect_enable(0U);

#else /* !CONFIG_BT_CTLR_ADV_EXT */
	status = ll_create_connection(scan_interval, scan_window,
				      cmd->filter_policy,
				      cmd->peer_addr.type,
				      &cmd->peer_addr.a.val[0],
				      cmd->own_addr_type, conn_interval_max,
				      conn_latency, supervision_timeout);
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

	*evt = cmd_status(status);
}

static void le_create_conn_cancel(struct net_buf *buf, struct net_buf **evt,
				  void **node_rx)
{
	uint8_t status;

	status = ll_connect_disable(node_rx);

	*evt = cmd_complete_status(status);
}

static void le_set_host_chan_classif(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_host_chan_classif *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_chm_update(&cmd->ch_map[0]);

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_start_encryption(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_start_encryption *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_enc_req_send(handle,
				 (uint8_t *)&cmd->rand,
				 (uint8_t *)&cmd->ediv,
				 &cmd->ltk[0]);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
static void le_set_cig_parameters(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_cig_params *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_cig_params *rp;
	uint32_t c_interval;
	uint32_t p_interval;
	uint16_t c_latency;
	uint16_t p_latency;
	uint8_t cis_count;
	uint8_t cig_id;
	uint8_t status;
	uint8_t i;

	c_interval = sys_get_le24(cmd->c_interval);
	p_interval = sys_get_le24(cmd->p_interval);
	c_latency = sys_le16_to_cpu(cmd->c_latency);
	p_latency = sys_le16_to_cpu(cmd->p_latency);

	cig_id = cmd->cig_id;
	cis_count = cmd->num_cis;

	/* Create CIG or start modifying existing CIG */
	status = ll_cig_parameters_open(cig_id, c_interval, p_interval,
					cmd->sca, cmd->packing, cmd->framing,
					c_latency, p_latency, cis_count);

	/* Configure individual CISes */
	for (i = 0; !status && i < cis_count; i++) {
		struct bt_hci_cis_params *params = &cmd->cis[i];
		uint16_t c_sdu;
		uint16_t p_sdu;

		c_sdu = sys_le16_to_cpu(params->c_sdu);
		p_sdu = sys_le16_to_cpu(params->p_sdu);

		status = ll_cis_parameters_set(params->cis_id, c_sdu, p_sdu,
					       params->c_phy, params->p_phy,
					       params->c_rtn, params->p_rtn);
	}

	rp = hci_cmd_complete(evt, sizeof(*rp) + cis_count * sizeof(uint16_t));
	rp->cig_id = cig_id;
	rp->num_handles = cis_count;

	/* Only apply parameters if all went well */
	if (!status) {
		status = ll_cig_parameters_commit(cig_id);

		if (status == BT_HCI_ERR_SUCCESS) {
			struct ll_conn_iso_group *cig;
			uint16_t handle;

			cig = ll_conn_iso_group_get_by_id(cig_id);
			handle = UINT16_MAX;

			for (uint8_t i = 0; i < cis_count; i++) {
				(void)ll_conn_iso_stream_get_by_group(cig, &handle);
				rp->handle[i] = sys_cpu_to_le16(handle);
			}
		}
	}

	rp->status = status;
}

static void le_set_cig_params_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_cig_params_test *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_cig_params_test *rp;

	uint32_t c_interval;
	uint32_t p_interval;
	uint16_t iso_interval;
	uint8_t cis_count;
	uint8_t cig_id;
	uint8_t status;
	uint8_t i;

	c_interval = sys_get_le24(cmd->c_interval);
	p_interval = sys_get_le24(cmd->p_interval);
	iso_interval = sys_le16_to_cpu(cmd->iso_interval);

	cig_id = cmd->cig_id;
	cis_count = cmd->num_cis;

	/* Create CIG or start modifying existing CIG */
	status = ll_cig_parameters_test_open(cig_id, c_interval,
					     p_interval, cmd->c_ft,
					     cmd->p_ft, iso_interval,
					     cmd->sca, cmd->packing,
					     cmd->framing,
					     cis_count);

	/* Configure individual CISes */
	for (i = 0; !status && i < cis_count; i++) {
		struct bt_hci_cis_params_test *params = &cmd->cis[i];
		uint16_t c_sdu;
		uint16_t p_sdu;
		uint16_t c_pdu;
		uint16_t p_pdu;
		uint8_t  nse;

		nse   = params->nse;
		c_sdu = sys_le16_to_cpu(params->c_sdu);
		p_sdu = sys_le16_to_cpu(params->p_sdu);
		c_pdu = sys_le16_to_cpu(params->c_pdu);
		p_pdu = sys_le16_to_cpu(params->p_pdu);

		status = ll_cis_parameters_test_set(params->cis_id, nse,
						    c_sdu, p_sdu,
						    c_pdu, p_pdu,
						    params->c_phy,
						    params->p_phy,
						    params->c_bn,
						    params->p_bn);
	}

	rp = hci_cmd_complete(evt, sizeof(*rp) + cis_count * sizeof(uint16_t));
	rp->cig_id = cig_id;
	rp->num_handles = cis_count;

	/* Only apply parameters if all went well */
	if (!status) {
		status = ll_cig_parameters_commit(cig_id);

		if (status == BT_HCI_ERR_SUCCESS) {
			struct ll_conn_iso_group *cig;
			uint16_t handle;

			cig = ll_conn_iso_group_get_by_id(cig_id);
			handle = UINT16_MAX;

			for (uint8_t i = 0; i < cis_count; i++) {
				(void)ll_conn_iso_stream_get_by_group(cig, &handle);
				rp->handle[i] = sys_cpu_to_le16(handle);
			}
		}
	}

	rp->status = status;
}

static void le_create_cis(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_create_cis *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t i;

	/*
	 * Creating new CISes is disallowed until all previous CIS
	 * established events have been generated
	 */
	if (cis_pending_count) {
		*evt = cmd_status(BT_HCI_ERR_CMD_DISALLOWED);
		return;
	}

	/* Check all handles before actually starting to create CISes */
	status = 0x00;
	for (i = 0; !status && i < cmd->num_cis; i++) {
		uint16_t cis_handle;
		uint16_t acl_handle;

		cis_handle = sys_le16_to_cpu(cmd->cis[i].cis_handle);
		acl_handle = sys_le16_to_cpu(cmd->cis[i].acl_handle);
		status = ll_cis_create_check(cis_handle, acl_handle);
	}

	if (status) {
		*evt = cmd_status(status);
		return;
	}

	/*
	 * Actually create CISes, any errors are to be reported
	 * through CIS established events
	 */
	cis_pending_count = cmd->num_cis;
	for (i = 0; i < cmd->num_cis; i++) {
		uint16_t cis_handle;
		uint16_t acl_handle;

		cis_handle = sys_le16_to_cpu(cmd->cis[i].cis_handle);
		acl_handle = sys_le16_to_cpu(cmd->cis[i].acl_handle);
		ll_cis_create(cis_handle, acl_handle);
	}

	*evt = cmd_status(status);
}

static void le_remove_cig(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_remove_cig *cmd = (void *)buf->data;
	struct bt_hci_rp_le_remove_cig *rp;
	uint8_t status;

	status = ll_cig_remove(cmd->cig_id);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->cig_id = cmd->cig_id;
}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static void le_iso_transmit_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_iso_transmit_test *cmd = (void *)buf->data;
	struct bt_hci_rp_le_iso_transmit_test *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_iso_transmit_test(handle, cmd->payload_type);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_read_iso_tx_sync(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_iso_tx_sync *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_iso_tx_sync *rp;
	uint16_t handle_le16;
	uint32_t timestamp;
	uint32_t offset;
	uint16_t handle;
	uint8_t status;
	uint16_t seq;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);

	status = ll_read_iso_tx_sync(handle, &seq, &timestamp, &offset);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = handle_le16;
	rp->seq       = sys_cpu_to_le16(seq);
	rp->timestamp = sys_cpu_to_le32(timestamp);
	sys_put_le24(offset, rp->offset);
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
static void le_iso_receive_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_iso_receive_test *cmd = (void *)buf->data;
	struct bt_hci_rp_le_iso_receive_test *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_iso_receive_test(handle, cmd->payload_type);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_iso_read_test_counters(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_test_counters *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_test_counters *rp;
	uint32_t received_cnt;
	uint32_t missed_cnt;
	uint32_t failed_cnt;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_iso_read_test_counters(handle, &received_cnt,
					   &missed_cnt, &failed_cnt);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
	rp->received_cnt = sys_cpu_to_le32(received_cnt);
	rp->missed_cnt   = sys_cpu_to_le32(missed_cnt);
	rp->failed_cnt   = sys_cpu_to_le32(failed_cnt);
}

#if defined(CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY)
static void le_read_iso_link_quality(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_iso_link_quality *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_iso_link_quality *rp;
	uint32_t tx_last_subevent_packets;
	uint32_t retransmitted_packets;
	uint32_t rx_unreceived_packets;
	uint32_t tx_unacked_packets;
	uint32_t tx_flushed_packets;
	uint32_t crc_error_packets;
	uint32_t duplicate_packets;
	uint16_t handle_le16;
	uint16_t handle;
	uint8_t status;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);
	status = ll_read_iso_link_quality(handle, &tx_unacked_packets,
					  &tx_flushed_packets,
					  &tx_last_subevent_packets,
					  &retransmitted_packets,
					  &crc_error_packets,
					  &rx_unreceived_packets,
					  &duplicate_packets);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = handle_le16;
	rp->tx_unacked_packets = sys_cpu_to_le32(tx_unacked_packets);
	rp->tx_flushed_packets = sys_cpu_to_le32(tx_flushed_packets);
	rp->tx_last_subevent_packets =
		sys_cpu_to_le32(tx_last_subevent_packets);
	rp->retransmitted_packets = sys_cpu_to_le32(retransmitted_packets);
	rp->crc_error_packets     = sys_cpu_to_le32(crc_error_packets);
	rp->rx_unreceived_packets = sys_cpu_to_le32(rx_unreceived_packets);
	rp->duplicate_packets     = sys_cpu_to_le32(duplicate_packets);
}
#endif /* CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY */

#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ISO)
static void le_setup_iso_path(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_setup_iso_path *cmd = (void *)buf->data;
	struct bt_hci_rp_le_setup_iso_path *rp;
	uint32_t controller_delay;
	uint8_t *codec_config;
	uint8_t coding_format;
	uint16_t vs_codec_id;
	uint16_t company_id;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	coding_format = cmd->codec_id.coding_format;
	company_id = sys_le16_to_cpu(cmd->codec_id.company_id);
	vs_codec_id = sys_le16_to_cpu(cmd->codec_id.vs_codec_id);
	controller_delay = sys_get_le24(cmd->controller_delay);
	codec_config = &cmd->codec_config[0];

	status = ll_setup_iso_path(handle, cmd->path_dir, cmd->path_id,
				   coding_format, company_id, vs_codec_id,
				   controller_delay, cmd->codec_config_len,
				   codec_config);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_remove_iso_path(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_remove_iso_path *cmd = (void *)buf->data;
	struct bt_hci_rp_le_remove_iso_path *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_remove_iso_path(handle, cmd->path_dir);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_iso_test_end(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_iso_test_end *cmd = (void *)buf->data;
	struct bt_hci_rp_le_iso_test_end *rp;
	uint32_t received_cnt;
	uint32_t missed_cnt;
	uint32_t failed_cnt;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_iso_test_end(handle, &received_cnt, &missed_cnt,
				 &failed_cnt);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
	rp->received_cnt = sys_cpu_to_le32(received_cnt);
	rp->missed_cnt   = sys_cpu_to_le32(missed_cnt);
	rp->failed_cnt   = sys_cpu_to_le32(failed_cnt);
}
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_SET_HOST_FEATURE)
static void le_set_host_feature(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_host_feature *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_host_feature *rp;
	uint8_t status;

	status = ll_set_host_feature(cmd->bit_number, cmd->bit_value);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
}
#endif /* CONFIG_BT_CTLR_SET_HOST_FEATURE */

#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_ltk_req_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_ltk_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_reply *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_start_enc_req_send(handle, 0x00, &cmd->ltk[0]);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_ltk_req_neg_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_ltk_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_ltk_req_neg_reply *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_start_enc_req_send(handle, BT_HCI_ERR_PIN_OR_KEY_MISSING,
				       NULL);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_le16_to_cpu(handle);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
static void le_accept_cis(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_accept_cis *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_cis_accept(handle);
	*evt = cmd_status(status);
}

static void le_reject_cis(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_reject_cis *cmd = (void *)buf->data;
	struct bt_hci_rp_le_reject_cis *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_cis_reject(handle, cmd->reason);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
static void le_req_peer_sca(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_req_peer_sca *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_req_peer_sca(handle);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
static void le_read_remote_features(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_remote_features *cmd = (void *)buf->data;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_feature_req_send(handle);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */

static void le_read_chan_map(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_chan_map *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_chan_map *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	status = ll_chm_get(handle, rp->ch_map);

	rp->status = status;
	rp->handle = sys_le16_to_cpu(handle);
}

static void le_conn_update(struct net_buf *buf, struct net_buf **evt)
{
	struct hci_cp_le_conn_update *cmd = (void *)buf->data;
	uint16_t supervision_timeout;
	uint16_t conn_interval_min;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	conn_interval_min = sys_le16_to_cpu(cmd->conn_interval_min);
	conn_interval_max = sys_le16_to_cpu(cmd->conn_interval_max);
	conn_latency = sys_le16_to_cpu(cmd->conn_latency);
	supervision_timeout = sys_le16_to_cpu(cmd->supervision_timeout);

	status = ll_conn_update(handle, 0, 0, conn_interval_min,
				conn_interval_max, conn_latency,
				supervision_timeout, NULL);

	*evt = cmd_status(status);
}

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void le_conn_param_req_reply(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_param_req_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_reply *rp;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	interval_min = sys_le16_to_cpu(cmd->interval_min);
	interval_max = sys_le16_to_cpu(cmd->interval_max);
	latency = sys_le16_to_cpu(cmd->latency);
	timeout = sys_le16_to_cpu(cmd->timeout);

	status = ll_conn_update(handle, 2, 0, interval_min, interval_max,
				latency, timeout, NULL);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_conn_param_req_neg_reply(struct net_buf *buf,
					struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_param_req_neg_reply *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_param_req_neg_reply *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	status = ll_conn_update(handle, 2, cmd->reason, 0, 0, 0, 0, NULL);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void le_set_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_data_len *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_data_len *rp;
	uint16_t tx_octets;
	uint16_t tx_time;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	tx_octets = sys_le16_to_cpu(cmd->tx_octets);
	tx_time = sys_le16_to_cpu(cmd->tx_time);
	status = ll_length_req_send(handle, tx_octets, tx_time);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
}

static void le_read_default_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_default_data_len *rp;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	ll_length_default_get(&max_tx_octets, &max_tx_time);

	rp->max_tx_octets = sys_cpu_to_le16(max_tx_octets);
	rp->max_tx_time = sys_cpu_to_le16(max_tx_time);
	rp->status = 0x00;
}

static void le_write_default_data_len(struct net_buf *buf,
				      struct net_buf **evt)
{
	struct bt_hci_cp_le_write_default_data_len *cmd = (void *)buf->data;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint8_t status;

	max_tx_octets = sys_le16_to_cpu(cmd->max_tx_octets);
	max_tx_time = sys_le16_to_cpu(cmd->max_tx_time);
	status = ll_length_default_set(max_tx_octets, max_tx_time);

	*evt = cmd_complete_status(status);
}

static void le_read_max_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_max_data_len *rp;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	ll_length_max_get(&max_tx_octets, &max_tx_time,
			  &max_rx_octets, &max_rx_time);

	rp->max_tx_octets = sys_cpu_to_le16(max_tx_octets);
	rp->max_tx_time = sys_cpu_to_le16(max_tx_time);
	rp->max_rx_octets = sys_cpu_to_le16(max_rx_octets);
	rp->max_rx_time = sys_cpu_to_le16(max_rx_time);
	rp->status = 0x00;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static void le_read_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_phy *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_phy *rp;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	status = ll_phy_get(handle, &rp->tx_phy, &rp->rx_phy);

	rp->status = status;
	rp->handle = sys_cpu_to_le16(handle);
	rp->tx_phy = find_lsb_set(rp->tx_phy);
	rp->rx_phy = find_lsb_set(rp->rx_phy);
}

static void le_set_default_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_default_phy *cmd = (void *)buf->data;
	uint8_t status;

	if (cmd->all_phys & BT_HCI_LE_PHY_TX_ANY) {
		cmd->tx_phys = 0x07;
	}
	if (cmd->all_phys & BT_HCI_LE_PHY_RX_ANY) {
		cmd->rx_phys = 0x07;
	}

	status = ll_phy_default_set(cmd->tx_phys, cmd->rx_phys);

	*evt = cmd_complete_status(status);
}

static void le_set_phy(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_phy *cmd = (void *)buf->data;
	uint16_t phy_opts;
	uint8_t mask_phys;
	uint16_t handle;
	uint8_t status;

	handle = sys_le16_to_cpu(cmd->handle);
	phy_opts = sys_le16_to_cpu(cmd->phy_opts);

	mask_phys = BT_HCI_LE_PHY_PREFER_1M;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_2M)) {
		mask_phys |= BT_HCI_LE_PHY_PREFER_2M;
	}
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		mask_phys |= BT_HCI_LE_PHY_PREFER_CODED;
	}

	if (cmd->all_phys & BT_HCI_LE_PHY_TX_ANY) {
		cmd->tx_phys |= mask_phys;
	}
	if (cmd->all_phys & BT_HCI_LE_PHY_RX_ANY) {
		cmd->rx_phys |= mask_phys;
	}

	if ((cmd->tx_phys | cmd->rx_phys) & ~mask_phys) {
		*evt = cmd_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);

		return;
	}

	if (!(cmd->tx_phys & 0x07) ||
	    !(cmd->rx_phys & 0x07)) {
		*evt = cmd_status(BT_HCI_ERR_INVALID_PARAM);

		return;
	}

	if (phy_opts & 0x03) {
		phy_opts -= 1U;
		phy_opts &= 1;
	} else {
		phy_opts = 0U;
	}

	status = ll_phy_req_send(handle, cmd->tx_phys, phy_opts,
				 cmd->rx_phys);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PRIVACY)
static void le_add_dev_to_rl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_add_dev_to_rl *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_rl_add(&cmd->peer_id_addr, cmd->peer_irk, cmd->local_irk);

	*evt = cmd_complete_status(status);
}

static void le_rem_dev_from_rl(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rem_dev_from_rl *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_rl_remove(&cmd->peer_id_addr);

	*evt = cmd_complete_status(status);
}

static void le_clear_rl(struct net_buf *buf, struct net_buf **evt)
{
	uint8_t status;

	status = ll_rl_clear();

	*evt = cmd_complete_status(status);
}

static void le_read_rl_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_rl_size *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->rl_size = ll_rl_size_get();
	rp->status = 0x00;
}

static void le_read_peer_rpa(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_peer_rpa *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_peer_rpa *rp;
	bt_addr_le_t peer_id_addr;

	bt_addr_le_copy(&peer_id_addr, &cmd->peer_id_addr);
	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = ll_rl_crpa_get(&peer_id_addr, &rp->peer_rpa);
}

static void le_read_local_rpa(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_read_local_rpa *cmd = (void *)buf->data;
	struct bt_hci_rp_le_read_local_rpa *rp;
	bt_addr_le_t peer_id_addr;

	bt_addr_le_copy(&peer_id_addr, &cmd->peer_id_addr);
	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = ll_rl_lrpa_get(&peer_id_addr, &rp->local_rpa);
}

static void le_set_addr_res_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_addr_res_enable *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_rl_enable(cmd->enable);

	*evt = cmd_complete_status(status);
}

static void le_set_rpa_timeout(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_rpa_timeout *cmd = (void *)buf->data;
	uint16_t timeout = sys_le16_to_cpu(cmd->rpa_timeout);

	ll_rl_timeout_set(timeout);

	*evt = cmd_complete_status(0x00);
}

static void le_set_privacy_mode(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_privacy_mode *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_priv_mode_set(&cmd->id_addr, cmd->mode);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_PRIVACY */

static void le_read_tx_power(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_tx_power *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	ll_tx_pwr_get(&rp->min_tx_power, &rp->max_tx_power);
}

#if defined(CONFIG_BT_CTLR_DF)
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
static void le_df_set_cl_cte_tx_params(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_le_set_cl_cte_tx_params *cmd = (void *)buf->data;
	uint8_t adv_handle;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &adv_handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_df_set_cl_cte_tx_params(adv_handle, cmd->cte_len,
					    cmd->cte_type, cmd->cte_count,
					    cmd->switch_pattern_len,
					    cmd->ant_ids);

	*evt = cmd_complete_status(status);
}

static void le_df_set_cl_cte_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_cl_cte_tx_enable *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_df_set_cl_cte_tx_enable(handle, cmd->cte_enable);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
static void le_df_set_cl_iq_sampling_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_cl_cte_sampling_enable *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_cl_cte_sampling_enable *rp;
	uint16_t sync_handle;
	uint8_t status;

	sync_handle = sys_le16_to_cpu(cmd->sync_handle);

	status = ll_df_set_cl_iq_sampling_enable(sync_handle,
						 cmd->sampling_enable,
						 cmd->slot_durations,
						 cmd->max_sampled_cte,
						 cmd->switch_pattern_len,
						 cmd->ant_ids);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = status;
	rp->sync_handle = sys_cpu_to_le16(sync_handle);
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT) ||      \
	defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
static int8_t iq_convert_12_to_8_bits(int16_t data)
{
	if (data == IQ_SAMPLE_SATURATED_16_BIT) {
		return IQ_SAMPLE_SATURATED_8_BIT;
	}

#if defined(CONFIG_BT_CTLR_DF_IQ_SAMPLES_CONVERT_USE_8_LSB)
	return (data > INT8_MAX || data < INT8_MIN) ? IQ_SAMPLE_SATURATED_8_BIT
						    : IQ_SAMPLE_CONVERT_12_TO_8_BIT(data);
#else  /* !CONFIG_BT_CTLR_DF_IQ_SAMPLES_CONVERT_USE_8_LSB */
	int16_t data_conv = IQ_SAMPLE_CONVERT_12_TO_8_BIT(data);

	return (data_conv > INT8_MAX || data_conv < INT8_MIN) ? IQ_SAMPLE_SATURATED_8_BIT
							      : (int8_t)data_conv;
#endif /* CONFIG_BT_CTLR_DF_IQ_SAMPLES_CONVERT_USE_8_LSB */
}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT
	* || CONFIG_BT_CTLR_DF_CONN_CTE_RX
	*/

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
static void le_df_connectionless_iq_report(struct pdu_data *pdu_rx,
					   struct node_rx_pdu *node_rx,
					   struct net_buf *buf)
{
	struct bt_hci_evt_le_connectionless_iq_report *sep;
	struct node_rx_iq_report *iq_report;
	struct lll_sync *lll;
	uint8_t samples_cnt;
	int16_t rssi;
	uint16_t sync_handle;
	uint16_t per_evt_counter;
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	struct ll_sync_set *sync = NULL;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	iq_report =  (struct node_rx_iq_report *)node_rx;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONNECTIONLESS_IQ_REPORT)) {
		return;
	}

	lll = iq_report->hdr.rx_ftr.param;

	/* If there is not LLL context and CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT is enabled
	 * the controller is in the Direct Test Mode and may generate
	 * the Connectionless IQ Report.
	 */
	if (!lll && IS_ENABLED(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)) {
		/* Set sync_handle to 0x0FFF according to the BT Core 5.3 specification
		 * Vol 4 7.7.65.21
		 */
		sync_handle = 0x0FFF;
		/* Set periodic event counter to 0 since there is not periodic advertising train. */
		per_evt_counter = 0;
	}

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	else {
		sync = HDR_LLL2ULL(lll);

		/* TX LL thread has higher priority than RX thread. It may happen that
		 * host successfully disables CTE sampling in the meantime.
		 * It should be verified here, to avoid reporting IQ samples after
		 * the functionality was disabled or if sync was lost.
		 */
		if (ull_df_sync_cfg_is_not_enabled(&lll->df_cfg) ||
		    !sync->timeout_reload) {
			/* Drop further processing of the event. */
			return;
		}

		/* Get the sync handle corresponding to the LLL context passed in the
		 * node rx footer field.
		 */
		sync_handle = ull_sync_handle_get(sync);
		per_evt_counter = iq_report->event_counter;
	}
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	/* If packet status does not indicate insufficient resources for IQ samples and for
	 * some reason sample_count is zero, inform Host about lack of valid IQ samples by
	 * storing single I_sample and Q_sample with BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE value.
	 */
	if (iq_report->packet_status == BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		samples_cnt = 0U;
	} else {
		samples_cnt = MAX(1, iq_report->sample_count);
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONNECTIONLESS_IQ_REPORT,
		       (sizeof(*sep) +
			(samples_cnt * sizeof(struct bt_hci_le_iq_sample))));

	rssi = RSSI_DBM_TO_DECI_DBM(iq_report->hdr.rx_ftr.rssi);


	sep->sync_handle = sys_cpu_to_le16(sync_handle);
	sep->rssi = sys_cpu_to_le16(rssi);
	sep->rssi_ant_id = iq_report->rssi_ant_id;
	sep->cte_type = iq_report->cte_info.type;

	sep->chan_idx = iq_report->chan_idx;
	sep->per_evt_counter = sys_cpu_to_le16(per_evt_counter);

	if (sep->cte_type == BT_HCI_LE_AOA_CTE) {
		sep->slot_durations = iq_report->local_slot_durations;
	} else if (sep->cte_type == BT_HCI_LE_AOD_CTE_1US) {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;
	} else {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US;
	}

	sep->packet_status = iq_report->packet_status;

	if (iq_report->packet_status != BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		if (iq_report->sample_count == 0U) {
			sep->sample[0].i = BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE;
			sep->sample[0].q = BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE;
		} else {
			for (uint8_t idx = 0U; idx < samples_cnt; ++idx) {
				sep->sample[idx].i =
					iq_convert_12_to_8_bits(iq_report->sample[idx].i);
				sep->sample[idx].q =
					iq_convert_12_to_8_bits(iq_report->sample[idx].q);
			}
		}
	}

	sep->sample_count = samples_cnt;
}
#endif /* defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT) */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
static void le_df_set_conn_cte_tx_params(struct net_buf *buf,
					 struct net_buf **evt)
{
	struct bt_hci_cp_le_set_conn_cte_tx_params *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_conn_cte_tx_params *rp;
	uint16_t handle, handle_le16;
	uint8_t status;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);

	status = ll_df_set_conn_cte_tx_params(handle, cmd->cte_types,
					      cmd->switch_pattern_len,
					      cmd->ant_ids);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = status;
	rp->handle = handle_le16;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
static void le_df_set_conn_cte_rx_params(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_conn_cte_rx_params *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_conn_cte_rx_params *rp;
	uint16_t handle, handle_le16;
	uint8_t status;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);

	status = ll_df_set_conn_cte_rx_params(handle, cmd->sampling_enable, cmd->slot_durations,
					      cmd->switch_pattern_len, cmd->ant_ids);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = status;
	rp->handle = handle_le16;
}

static void le_df_connection_iq_report(struct node_rx_pdu *node_rx, struct net_buf *buf)
{
	struct bt_hci_evt_le_connection_iq_report *sep;
	struct node_rx_iq_report *iq_report;
	struct lll_conn *lll;
	uint8_t samples_cnt;
	uint8_t phy_rx;
	int16_t rssi;

	iq_report = (struct node_rx_iq_report *)node_rx;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONNECTION_IQ_REPORT)) {
		return;
	}

	lll = iq_report->hdr.rx_ftr.param;

#if defined(CONFIG_BT_CTLR_PHY)
	phy_rx = lll->phy_rx;

	/* Make sure the report is generated for connection on PHY UNCODED */
	LL_ASSERT(phy_rx != PHY_CODED);
#else
	phy_rx = PHY_1M;
#endif /* CONFIG_BT_CTLR_PHY */

	/* TX LL thread has higher priority than RX thread. It may happen that host succefully
	 * disables CTE sampling in the meantime. It should be verified here, to avoid reporing
	 * IQ samples after the functionality was disabled.
	 */
	if (ull_df_conn_cfg_is_not_enabled(&lll->df_rx_cfg)) {
		/* Dropp further processing of the event. */
		return;
	}

	/* If packet status does not indicate insufficient resources for IQ samples and for
	 * some reason sample_count is zero, inform Host about lack of valid IQ samples by
	 * storing single I_sample and Q_sample with BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE value.
	 */
	if (iq_report->packet_status == BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		samples_cnt = 0;
	} else {
		samples_cnt = MAX(1, iq_report->sample_count);
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONNECTION_IQ_REPORT,
		       (sizeof(*sep) + (samples_cnt * sizeof(struct bt_hci_le_iq_sample))));

	rssi = RSSI_DBM_TO_DECI_DBM(iq_report->hdr.rx_ftr.rssi);

	sep->conn_handle = sys_cpu_to_le16(iq_report->hdr.handle);
	sep->rx_phy = phy_rx;
	sep->rssi = sys_cpu_to_le16(rssi);
	sep->rssi_ant_id = iq_report->rssi_ant_id;
	sep->cte_type = iq_report->cte_info.type;

	sep->data_chan_idx = iq_report->chan_idx;
	sep->conn_evt_counter = sys_cpu_to_le16(iq_report->event_counter);

	if (sep->cte_type == BT_HCI_LE_AOA_CTE) {
		sep->slot_durations = iq_report->local_slot_durations;
	} else if (sep->cte_type == BT_HCI_LE_AOD_CTE_1US) {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;
	} else {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US;
	}

	sep->packet_status = iq_report->packet_status;

	if (iq_report->packet_status != BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		if (iq_report->sample_count == 0U) {
			sep->sample[0].i = BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE;
			sep->sample[0].q = BT_HCI_LE_CTE_REPORT_NO_VALID_SAMPLE;
		} else {
			for (uint8_t idx = 0U; idx < samples_cnt; ++idx) {
				sep->sample[idx].i =
					iq_convert_12_to_8_bits(iq_report->sample[idx].i);
				sep->sample[idx].q =
					iq_convert_12_to_8_bits(iq_report->sample[idx].q);
			}
		}
	}

	sep->sample_count = samples_cnt;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
static void le_df_set_conn_cte_req_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_cte_req_enable *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_cte_req_enable *rp;
	uint16_t handle, handle_le16;
	uint8_t status;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);

	status = ll_df_set_conn_cte_req_enable(handle, cmd->enable,
					       sys_le16_to_cpu(cmd->cte_request_interval),
					       cmd->requested_cte_length, cmd->requested_cte_type);
	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = status;
	rp->handle = handle_le16;
}

static void le_df_cte_req_failed(uint8_t error_code, uint16_t handle, struct net_buf *buf)
{
	struct bt_hci_evt_le_cte_req_failed *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CTE_REQUEST_FAILED)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CTE_REQUEST_FAILED, sizeof(*sep));

	sep->status = error_code;
	sep->conn_handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
static void le_df_set_conn_cte_rsp_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_conn_cte_rsp_enable *cmd = (void *)buf->data;
	struct bt_hci_rp_le_conn_cte_rsp_enable *rp;
	uint16_t handle, handle_le16;
	uint8_t status;

	handle_le16 = cmd->handle;
	handle = sys_le16_to_cpu(handle_le16);

	status = ll_df_set_conn_cte_rsp_enable(handle, cmd->enable);
	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = status;
	rp->handle = handle_le16;
}
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */

static void le_df_read_ant_inf(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_ant_info *rp;
	uint8_t max_switch_pattern_len;
	uint8_t switch_sample_rates;
	uint8_t max_cte_len;
	uint8_t num_ant;

	ll_df_read_ant_inf(&switch_sample_rates, &num_ant,
			   &max_switch_pattern_len, &max_cte_len);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->max_switch_pattern_len = max_switch_pattern_len;
	rp->switch_sample_rates = switch_sample_rates;
	rp->max_cte_len = max_cte_len;
	rp->num_ant = num_ant;
	rp->status = 0x00;
}
#endif /* CONFIG_BT_CTLR_DF */

#if defined(CONFIG_BT_CTLR_DTM_HCI)
static void le_rx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rx_test *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_rx(cmd->rx_ch, BT_HCI_LE_RX_PHY_1M, BT_HCI_LE_MOD_INDEX_STANDARD,
			    BT_HCI_LE_TEST_CTE_DISABLED, BT_HCI_LE_TEST_CTE_TYPE_ANY,
			    BT_HCI_LE_TEST_SLOT_DURATION_ANY, BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY,
			    NULL);

	*evt = cmd_complete_status(status);
}

static void le_tx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_tx_test *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload,
			    BT_HCI_LE_TX_PHY_1M, BT_HCI_LE_TEST_CTE_DISABLED,
			    BT_HCI_LE_TEST_CTE_TYPE_ANY, BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY,
			    NULL, BT_HCI_TX_TEST_POWER_MAX_SET);

	*evt = cmd_complete_status(status);
}

static void le_test_end(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_test_end *rp;
	uint16_t rx_pkt_count;
	uint8_t status;

	status = ll_test_end(&rx_pkt_count);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->rx_pkt_count = sys_cpu_to_le16(rx_pkt_count);
}

static void le_enh_rx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_enh_rx_test *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_rx(cmd->rx_ch, cmd->phy, cmd->mod_index, BT_HCI_LE_TEST_CTE_DISABLED,
			    BT_HCI_LE_TEST_CTE_TYPE_ANY, BT_HCI_LE_TEST_SLOT_DURATION_ANY,
			    BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY, NULL);

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_DTM_HCI_RX_V3)
static void le_rx_test_v3(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rx_test_v3 *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_rx(cmd->rx_ch, cmd->phy, cmd->mod_index, cmd->expected_cte_len,
			    cmd->expected_cte_type, cmd->slot_durations, cmd->switch_pattern_len,
			    cmd->ant_ids);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_DTM_HCI_RX_V3 */

static void le_enh_tx_test(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_enh_tx_test *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload, cmd->phy,
			    BT_HCI_LE_TEST_CTE_DISABLED, BT_HCI_LE_TEST_CTE_TYPE_ANY,
			    BT_HCI_LE_TEST_SWITCH_PATTERN_LEN_ANY, NULL,
			    BT_HCI_TX_TEST_POWER_MAX_SET);

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V3)
static void le_tx_test_v3(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_tx_test_v3 *cmd = (void *)buf->data;
	uint8_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload, cmd->phy,
			    cmd->cte_len, cmd->cte_type, cmd->switch_pattern_len, cmd->ant_ids,
			    BT_HCI_TX_TEST_POWER_MAX_SET);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_DTM_HCI_TX_V3 */

#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V4)
static void le_tx_test_v4(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_tx_test_v4 *cmd = (void *)buf->data;
	struct bt_hci_cp_le_tx_test_v4_tx_power *tx_power = (void *)(buf->data +
			sizeof(struct bt_hci_cp_le_tx_test_v4) + cmd->switch_pattern_len);
	uint8_t status;

	status = ll_test_tx(cmd->tx_ch, cmd->test_data_len, cmd->pkt_payload, cmd->phy,
			    cmd->cte_len, cmd->cte_type, cmd->switch_pattern_len, cmd->ant_ids,
			    tx_power->tx_power);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_DTM_HCI_TX_V4 */
#endif /* CONFIG_BT_CTLR_DTM_HCI */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)

static void le_set_adv_set_random_addr(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_cp_le_set_adv_set_random_addr *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_aux_random_addr_set(handle, &cmd->bdaddr.val[0]);

	*evt = cmd_complete_status(status);
}

static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_adv_param *cmd = (void *)buf->data;
	struct bt_hci_rp_le_set_ext_adv_param *rp;
	uint32_t min_interval;
	uint16_t evt_prop;
	uint8_t tx_pwr;
	uint8_t status;
	uint8_t phy_p;
	uint8_t phy_s;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	if (cmd->handle > BT_HCI_LE_ADV_HANDLE_MAX) {
		*evt = cmd_complete_status(BT_HCI_ERR_INVALID_PARAM);
		return;
	}

	min_interval = sys_get_le24(cmd->prim_min_interval);

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		const uint32_t max_interval =
					sys_get_le24(cmd->prim_max_interval);

		/* Compare advertising interval maximum with implementation
		 * supported advertising interval maximum value defined in the
		 * Kconfig CONFIG_BT_CTLR_ADV_INTERVAL_MAX.
		 */
		if ((min_interval > max_interval) ||
		    (min_interval < BT_HCI_LE_PRIM_ADV_INTERVAL_MIN) ||
		    (max_interval > CONFIG_BT_CTLR_ADV_INTERVAL_MAX)) {
			*evt = cmd_complete_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
			return;
		}
	}

	status = ll_adv_set_by_hci_handle_get_or_new(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	evt_prop = sys_le16_to_cpu(cmd->props);
	tx_pwr = cmd->tx_power;
	phy_p = BIT(cmd->prim_adv_phy - 1);
	phy_s = BIT(cmd->sec_adv_phy - 1);

	status = ll_adv_params_set(handle, evt_prop, min_interval,
				   PDU_ADV_TYPE_EXT_IND, cmd->own_addr_type,
				   cmd->peer_addr.type, cmd->peer_addr.a.val,
				   cmd->prim_channel_map, cmd->filter_policy,
				   &tx_pwr, phy_p, cmd->sec_adv_max_skip, phy_s,
				   cmd->sid, cmd->scan_req_notify_enable);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->tx_power = tx_pwr;
}

static void le_set_ext_adv_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_adv_data *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_aux_ad_data_set(handle, cmd->op, cmd->frag_pref,
					cmd->len, cmd->data);

	*evt = cmd_complete_status(status);
}

static void le_set_ext_scan_rsp_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_scan_rsp_data *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_aux_sr_data_set(handle, cmd->op, cmd->frag_pref,
					cmd->len, cmd->data);

	*evt = cmd_complete_status(status);
}

static void le_set_ext_adv_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_adv_enable *cmd = (void *)buf->data;
	struct bt_hci_ext_adv_set *s;
	uint8_t set_num;
	uint8_t enable;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	set_num = cmd->set_num;
	if (!set_num) {
		if (cmd->enable) {
			*evt = cmd_complete_status(BT_HCI_ERR_INVALID_PARAM);
			return;
		}

		/* FIXME: Implement disable of all advertising sets */
		*evt = cmd_complete_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);

		return;
	}

	s = (void *) cmd->s;
	enable = cmd->enable;
	do {
		status = ll_adv_set_by_hci_handle_get(s->handle, &handle);
		if (status) {
			break;
		}

		/* TODO: duration and events parameter use. */
#if defined(CONFIG_BT_HCI_MESH_EXT)
		status = ll_adv_enable(handle, cmd->enable, 0, 0, 0, 0, 0);
#else /* !CONFIG_BT_HCI_MESH_EXT */
		status = ll_adv_enable(handle, cmd->enable,
				       sys_le16_to_cpu(s->duration), s->max_ext_adv_evts);
#endif /* !CONFIG_BT_HCI_MESH_EXT */
		if (status) {
			/* TODO: how to handle succeeded ones before this
			 * error.
			 */
			break;
		}

		s++;
	} while (--set_num);

	*evt = cmd_complete_status(status);
}

static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;
	uint16_t max_adv_data_len;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));

	max_adv_data_len = ll_adv_aux_max_data_length_get();

	rp->max_adv_data_len = sys_cpu_to_le16(max_adv_data_len);
	rp->status = 0x00;
}

static void le_read_num_adv_sets(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_num_adv_sets *rp;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->num_sets = ll_adv_aux_set_count_get();
	rp->status = 0x00;
}

static void le_remove_adv_set(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_remove_adv_set *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_aux_set_remove(handle);

	*evt = cmd_complete_status(status);
}

static void le_clear_adv_sets(struct net_buf *buf, struct net_buf **evt)
{
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_aux_set_clear();

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
static void le_set_per_adv_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_per_adv_param *cmd = (void *)buf->data;
	uint16_t max_interval;
	uint16_t flags;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	max_interval = sys_le16_to_cpu(cmd->max_interval);

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		const uint32_t min_interval =
					sys_le16_to_cpu(cmd->min_interval);

		/* Compare periodic advertising interval maximum with
		 * implementation supported periodic advertising interval
		 * maximum value defined in the Kconfig
		 * CONFIG_BT_CTLR_ADV_PERIODIC_INTERVAL_MAX.
		 */
		if ((min_interval > max_interval) ||
		    (min_interval < BT_HCI_LE_PER_ADV_INTERVAL_MIN) ||
		    (max_interval > CONFIG_BT_CTLR_ADV_PERIODIC_INTERVAL_MAX)) {
			*evt = cmd_complete_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
			return;
		}
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	flags = sys_le16_to_cpu(cmd->props);

	status = ll_adv_sync_param_set(handle, max_interval, flags);

	*evt = cmd_complete_status(status);
}

static void le_set_per_adv_data(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_per_adv_data *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_sync_ad_data_set(handle, cmd->op, cmd->len,
					 cmd->data);

	*evt = cmd_complete_status(status);
}

static void le_set_per_adv_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_per_adv_enable *cmd = (void *)buf->data;
	uint8_t status;
	uint8_t handle;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_adv_set_by_hci_handle_get(cmd->handle, &handle);
	if (status) {
		*evt = cmd_complete_status(status);
		return;
	}

	status = ll_adv_sync_enable(handle, cmd->enable);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
static void le_set_ext_scan_param(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_scan_param *cmd = (void *)buf->data;
	struct bt_hci_ext_scan_phy *p;
	uint8_t own_addr_type;
	uint8_t filter_policy;
	uint8_t phys_bitmask;
	uint8_t status;
	uint8_t phys;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	/* Number of bits set indicate scan sets to be configured by calling
	 * ll_scan_params_set function.
	 */
	phys_bitmask = BT_HCI_LE_EXT_SCAN_PHY_1M;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		phys_bitmask |= BT_HCI_LE_EXT_SCAN_PHY_CODED;
	}

	phys = cmd->phys;
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    (((phys & phys_bitmask) == 0) || (phys & ~phys_bitmask))) {
		*evt = cmd_complete_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);

		return;
	}

	own_addr_type = cmd->own_addr_type;
	filter_policy = cmd->filter_policy;
	p = cmd->p;

	/* Irrespective of enabled PHYs to scan for, ll_scan_params_set needs
	 * to be called to initialise the scan sets.
	 * Passing interval and window as 0, disable the particular scan set
	 * from being enabled.
	 */
	do {
		uint16_t interval;
		uint16_t window;
		uint8_t type;
		uint8_t phy;

		/* Get single PHY bit from the loop bitmask */
		phy = BIT(find_lsb_set(phys_bitmask) - 1);

		/* Pass the PHY (1M or Coded) of scan set in MSbits of type
		 * parameter
		 */
		type = (phy << 1);

		/* If current PHY is one of the PHY in the Scanning_PHYs,
		 * pick the supplied scan type, interval and window.
		 */
		if (phys & phy) {
			type |= (p->type & 0x01);
			interval = sys_le16_to_cpu(p->interval);
			window = sys_le16_to_cpu(p->window);
			p++;
		} else {
			interval = 0U;
			window = 0U;
		}

		status = ll_scan_params_set(type, interval, window,
					    own_addr_type, filter_policy);
		if (status) {
			break;
		}

		phys_bitmask &= (phys_bitmask - 1);
	} while (phys_bitmask);

	*evt = cmd_complete_status(status);
}

static void le_set_ext_scan_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_ext_scan_enable *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	/* Initialize duplicate filtering */
	if (cmd->enable && cmd->filter_dup) {
		if (0) {

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
		} else if (dup_count == DUP_FILTER_DISABLED) {
			dup_scan = true;

			/* All entries reset */
			dup_count = 0;
			dup_curr = 0U;
		} else if (!dup_scan) {
			dup_scan = true;
			dup_ext_adv_reset();
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */

		} else {
			/* All entries reset */
			dup_count = 0;
			dup_curr = 0U;
		}
	} else {
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
		dup_scan = false;
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
		dup_count = DUP_FILTER_DISABLED;
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	status = ll_scan_enable(cmd->enable, cmd->duration, cmd->period);

	/* NOTE: As filter duplicates is implemented here in HCI source code,
	 *       enabling of already enabled scanning shall succeed after
	 *       updates to filter duplicates is handled in the above
	 *       statements. Refer to BT Spec v5.0 Vol 2 Part E Section 7.8.11.
	 */
	if (!IS_ENABLED(CONFIG_BT_CTLR_SCAN_ENABLE_STRICT) &&
	    (status == BT_HCI_ERR_CMD_DISALLOWED)) {
		status = BT_HCI_ERR_SUCCESS;
	}

	*evt = cmd_complete_status(status);
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static void le_per_adv_create_sync(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_per_adv_create_sync *cmd = (void *)buf->data;
	uint16_t sync_timeout;
	uint8_t status;
	uint16_t skip;

	if (adv_cmds_ext_check(NULL)) {
		*evt = cmd_status(BT_HCI_ERR_CMD_DISALLOWED);
		return;
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST) &&
	    (cmd->options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_USE_LIST)) {
		*evt = cmd_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	if (!IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT) &&
	    (cmd->options &
	     (BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_REPORTS_DISABLED |
	      BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_FILTER_DUPLICATE)) ==
	    BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_FILTER_DUPLICATE) {
		*evt = cmd_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
		return;
	}

	/* FIXME: Check for HCI LE Set Periodic Advertising Receive Enable
	 * command support and if reporting is initially disabled then
	 * return error code Connection Failed to be Established /
	 * Synchronization Timeout (0x3E).
	 */

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	/* Initialize duplicate filtering */
	if (cmd->options & BT_HCI_LE_PER_ADV_CREATE_SYNC_FP_FILTER_DUPLICATE) {
		if (!dup_scan || (dup_count == DUP_FILTER_DISABLED)) {
			dup_count = 0;
			dup_curr = 0U;
		} else {
			/* NOTE: Invalidate dup_ext_adv_mode array entries is
			 *       done when sync is established.
			 */
		}
	} else if (!dup_scan) {
		dup_count = DUP_FILTER_DISABLED;
	}
#endif

	skip = sys_le16_to_cpu(cmd->skip);
	sync_timeout = sys_le16_to_cpu(cmd->sync_timeout);

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING)
	if ((cmd->cte_type & BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_INVALID_VALUE) != 0) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
#else
	if (cmd->cte_type != BT_HCI_LE_PER_ADV_CREATE_SYNC_CTE_TYPE_NO_FILTERING) {
		status = BT_HCI_ERR_INVALID_PARAM;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_CTE_TYPE_FILTERING */
	} else {
		status = ll_sync_create(cmd->options, cmd->sid, cmd->addr.type, cmd->addr.a.val,
					skip, sync_timeout, cmd->cte_type);
	}
	*evt = cmd_status(status);
}

static void le_per_adv_create_sync_cancel(struct net_buf *buf,
					  struct net_buf **evt, void **node_rx)
{
	struct bt_hci_evt_cc_status *ccst;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_sync_create_cancel(node_rx);

	ccst = hci_cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_per_adv_terminate_sync(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_per_adv_terminate_sync *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint16_t handle;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_sync_terminate(handle);

	ccst = hci_cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

static void le_per_adv_recv_enable(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_set_per_adv_recv_enable *cmd = (void *)buf->data;
	struct bt_hci_evt_cc_status *ccst;
	uint16_t handle;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	handle = sys_le16_to_cpu(cmd->handle);

	status = ll_sync_recv_enable(handle, cmd->enable);

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (!status) {
		if (cmd->enable &
		    BT_HCI_LE_SET_PER_ADV_RECV_ENABLE_FILTER_DUPLICATE) {
			if (!dup_scan || (dup_count == DUP_FILTER_DISABLED)) {
				dup_count = 0;
				dup_curr = 0U;
			} else {
				/* NOTE: Invalidate dup_ext_adv_mode array
				 *       entries is done when sync is
				 *       established.
				 */
			}
		} else if (!dup_scan) {
			dup_count = DUP_FILTER_DISABLED;
		}
	}
#endif

	ccst = hci_cmd_complete(evt, sizeof(*ccst));
	ccst->status = status;
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
static void le_add_dev_to_pal(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_add_dev_to_per_adv_list *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_pal_add(&cmd->addr, cmd->sid);

	*evt = cmd_complete_status(status);
}

static void le_rem_dev_from_pal(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_rem_dev_from_per_adv_list *cmd = (void *)buf->data;
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_pal_remove(&cmd->addr, cmd->sid);

	*evt = cmd_complete_status(status);
}

static void le_clear_pal(struct net_buf *buf, struct net_buf **evt)
{
	uint8_t status;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	status = ll_pal_clear();

	*evt = cmd_complete_status(status);
}

static void le_read_pal_size(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_le_read_per_adv_list_size *rp;

	if (adv_cmds_ext_check(evt)) {
		return;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;

	rp->list_size = ll_pal_size_get();
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CENTRAL)
static void le_ext_create_connection(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_le_ext_create_conn *cmd = (void *)buf->data;
	struct bt_hci_ext_conn_phy *p;
	uint8_t peer_addr_type;
	uint8_t own_addr_type;
	uint8_t filter_policy;
	uint8_t phys_bitmask;
	uint8_t *peer_addr;
	uint8_t status;
	uint8_t phys;

	if (adv_cmds_ext_check(NULL)) {
		*evt = cmd_status(BT_HCI_ERR_CMD_DISALLOWED);
		return;
	}

	/* Number of bits set indicate scan sets to be configured by calling
	 * ll_create_connection function.
	 */
	phys_bitmask = BT_HCI_LE_EXT_SCAN_PHY_1M;
	if (IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
		phys_bitmask |= BT_HCI_LE_EXT_SCAN_PHY_CODED;
	}

	phys = cmd->phys;
	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK) &&
	    (((phys & phys_bitmask) == 0) || (phys & ~phys_bitmask))) {
		*evt = cmd_status(BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);

		return;
	}

	filter_policy = cmd->filter_policy;
	own_addr_type = cmd->own_addr_type;
	peer_addr_type = cmd->peer_addr.type;
	peer_addr = cmd->peer_addr.a.val;
	p = cmd->p;

	do {
		uint16_t supervision_timeout;
		uint16_t conn_interval_max;
		uint16_t scan_interval;
		uint16_t conn_latency;
		uint16_t scan_window;
		uint8_t phy;

		phy = BIT(find_lsb_set(phys_bitmask) - 1);

		if (phys & phy) {
			scan_interval = sys_le16_to_cpu(p->scan_interval);
			scan_window = sys_le16_to_cpu(p->scan_window);
			conn_interval_max =
				sys_le16_to_cpu(p->conn_interval_max);
			conn_latency = sys_le16_to_cpu(p->conn_latency);
			supervision_timeout =
				sys_le16_to_cpu(p->supervision_timeout);

			if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
				status = check_cconn_params(true, scan_interval,
							    scan_window,
							    conn_interval_max,
							    conn_latency,
							    supervision_timeout);
				if (status) {
					*evt = cmd_status(status);
					return;
				}
			}

			status = ll_create_connection(scan_interval,
						      scan_window,
						      filter_policy,
						      peer_addr_type,
						      peer_addr,
						      own_addr_type,
						      conn_interval_max,
						      conn_latency,
						      supervision_timeout,
						      phy);
			p++;
		} else {
			uint8_t type;

			type = (phy << 1);
			/* NOTE: Pass invalid interval value to reset the PHY
			 *       value in the scan instance so not to start
			 *       scanning on the unselected PHY.
			 */
			status = ll_scan_params_set(type, 0, 0, 0, 0);
		}

		if (status) {
			*evt = cmd_status(status);
			return;
		}

		phys_bitmask &= (phys_bitmask - 1);
	} while (phys_bitmask);

	status = ll_connect_enable(phys & BT_HCI_LE_EXT_SCAN_PHY_CODED);

	*evt = cmd_status(status);
}
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
static void le_cis_request(struct pdu_data *pdu_data,
			   struct node_rx_pdu *node_rx,
			   struct net_buf *buf)
{
	struct bt_hci_evt_le_cis_req *sep;
	struct node_rx_conn_iso_req *req;
	void *node;

	/* Check for pdu field being aligned before accessing CIS established
	 * event.
	 */
	node = pdu_data;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_conn_iso_estab));

	req = node;
	if (!(ll_feat_get() & BIT64(BT_LE_FEAT_BIT_ISO_CHANNELS)) ||
	    !(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CIS_REQ)) {
		ll_cis_reject(req->cis_handle, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE);
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CIS_REQ, sizeof(*sep));
	sep->acl_handle = sys_cpu_to_le16(node_rx->hdr.handle);
	sep->cis_handle = sys_cpu_to_le16(req->cis_handle);
	sep->cig_id = req->cig_id;
	sep->cis_id = req->cis_id;
}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
static void le_cis_established(struct pdu_data *pdu_data,
			       struct node_rx_pdu *node_rx,
			       struct net_buf *buf)
{
	struct lll_conn_iso_stream_rxtx *lll_cis_c;
	struct lll_conn_iso_stream_rxtx *lll_cis_p;
	struct bt_hci_evt_le_cis_established *sep;
	struct lll_conn_iso_stream *lll_cis;
	struct node_rx_conn_iso_estab *est;
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	bool is_central;
	void *node;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CIS_ESTABLISHED)) {
		return;
	}

	cis = node_rx->hdr.rx_ftr.param;
	cig = cis->group;

	sep = meta_evt(buf, BT_HCI_EVT_LE_CIS_ESTABLISHED, sizeof(*sep));

	/* Check for pdu field being aligned before accessing CIS established
	 * event.
	 */
	node = pdu_data;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_conn_iso_estab));

	est = node;
	sep->status = est->status;
	sep->conn_handle = sys_cpu_to_le16(est->cis_handle);

	if (!cig) {
		/* CIS was not established and instance was released */
		return;
	}

	lll_cis = &cis->lll;
	is_central = cig->lll.role == BT_CONN_ROLE_CENTRAL;
	lll_cis_c = is_central ? &lll_cis->tx : &lll_cis->rx;
	lll_cis_p = is_central ? &lll_cis->rx : &lll_cis->tx;

	sys_put_le24(cig->sync_delay, sep->cig_sync_delay);
	sys_put_le24(cis->sync_delay, sep->cis_sync_delay);
	sys_put_le24(cig->c_latency, sep->c_latency);
	sys_put_le24(cig->p_latency, sep->p_latency);
	sep->c_phy = lll_cis_c->phy;
	sep->p_phy = lll_cis_p->phy;
	sep->nse = lll_cis->nse;
	sep->c_bn = lll_cis_c->bn;
	sep->p_bn = lll_cis_p->bn;
	sep->c_ft = lll_cis_c->ft;
	sep->p_ft = lll_cis_p->ft;
	sep->c_max_pdu = sys_cpu_to_le16(lll_cis_c->max_pdu);
	sep->p_max_pdu = sys_cpu_to_le16(lll_cis_p->max_pdu);
	sep->interval = sys_cpu_to_le16(cig->iso_interval);

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	if (is_central) {
		cis_pending_count--;
	}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

static int controller_cmd_handle(uint16_t  ocf, struct net_buf *cmd,
				 struct net_buf **evt, void **node_rx)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_LE_SET_EVENT_MASK):
		le_set_event_mask(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE):
		le_read_buffer_size(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	case BT_OCF(BT_HCI_OP_LE_READ_BUFFER_SIZE_V2):
		le_read_buffer_size_v2(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_FEATURES):
		le_read_local_features(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_RANDOM_ADDRESS):
		le_set_random_address(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
	case BT_OCF(BT_HCI_OP_LE_READ_FAL_SIZE):
		le_read_fal_size(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CLEAR_FAL):
		le_clear_fal(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_FAL):
		le_add_dev_to_fal(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_FAL):
		le_rem_dev_from_fal(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

	case BT_OCF(BT_HCI_OP_LE_ENCRYPT):
		le_encrypt(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_RAND):
		le_rand(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_SUPP_STATES):
		le_read_supp_states(cmd, evt);
		break;

#if defined(CONFIG_BT_BROADCASTER)
	case BT_OCF(BT_HCI_OP_LE_SET_ADV_PARAM):
		le_set_adv_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_ADV_CHAN_TX_POWER):
		le_read_adv_chan_tx_power(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_DATA):
		le_set_adv_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_RSP_DATA):
		le_set_scan_rsp_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_ADV_ENABLE):
		le_set_adv_enable(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	case BT_OCF(BT_HCI_OP_LE_CREATE_BIG):
		le_create_big(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_BIG_TEST):
		le_create_big_test(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_TERMINATE_BIG):
		le_terminate_big(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_PARAM):
		le_set_scan_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_SCAN_ENABLE):
		le_set_scan_enable(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	case BT_OCF(BT_HCI_OP_LE_BIG_CREATE_SYNC):
		le_big_create_sync(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_BIG_TERMINATE_SYNC):
		le_big_terminate_sync(cmd, evt, node_rx);
		break;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CENTRAL)
	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN):
		le_create_connection(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CREATE_CONN_CANCEL):
		le_create_conn_cancel(cmd, evt, node_rx);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_HOST_CHAN_CLASSIF):
		le_set_host_chan_classif(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case BT_OCF(BT_HCI_OP_LE_START_ENCRYPTION):
		le_start_encryption(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	case BT_OCF(BT_HCI_OP_LE_SET_CIG_PARAMS):
		le_set_cig_parameters(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_CIG_PARAMS_TEST):
		le_set_cig_params_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_CREATE_CIS):
		le_create_cis(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_REMOVE_CIG):
		le_remove_cig(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */
#endif /* CONFIG_BT_CENTRAL */

#if defined(CONFIG_BT_PERIPHERAL)
#if defined(CONFIG_BT_CTLR_LE_ENC)
	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_REPLY):
		le_ltk_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_LTK_REQ_NEG_REPLY):
		le_ltk_req_neg_reply(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case BT_OCF(BT_HCI_OP_LE_ACCEPT_CIS):
		le_accept_cis(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_REJECT_CIS):
		le_reject_cis(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */
#endif /* CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case BT_OCF(BT_HCI_OP_LE_REQ_PEER_SC):
		le_req_peer_sca(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_ISO)
	case BT_OCF(BT_HCI_OP_LE_SETUP_ISO_PATH):
		le_setup_iso_path(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_REMOVE_ISO_PATH):
		le_remove_iso_path(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_ISO_TEST_END):
		le_iso_test_end(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	case BT_OCF(BT_HCI_OP_LE_ISO_TRANSMIT_TEST):
		le_iso_transmit_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_ISO_TX_SYNC):
		le_read_iso_tx_sync(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
	case BT_OCF(BT_HCI_OP_LE_ISO_RECEIVE_TEST):
		le_iso_receive_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_ISO_READ_TEST_COUNTERS):
		le_iso_read_test_counters(cmd, evt);
		break;
#if defined(CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY)
	case BT_OCF(BT_HCI_OP_LE_READ_ISO_LINK_QUALITY):
		le_read_iso_link_quality(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_READ_ISO_LINK_QUALITY */
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SET_HOST_FEATURE)
	case BT_OCF(BT_HCI_OP_LE_SET_HOST_FEATURE):
		le_set_host_feature(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_SET_HOST_FEATURE */

#if defined(CONFIG_BT_CONN)
	case BT_OCF(BT_HCI_OP_LE_READ_CHAN_MAP):
		le_read_chan_map(cmd, evt);
		break;

#if defined(CONFIG_BT_CENTRAL) || defined(CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG)
	case BT_OCF(BT_HCI_OP_LE_READ_REMOTE_FEATURES):
		le_read_remote_features(cmd, evt);
		break;
#endif /* CONFIG_BT_CENTRAL || CONFIG_BT_CTLR_PER_INIT_FEAT_XCHG */

	case BT_OCF(BT_HCI_OP_LE_CONN_UPDATE):
		le_conn_update(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY):
		le_conn_param_req_reply(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY):
		le_conn_param_req_neg_reply(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case BT_OCF(BT_HCI_OP_LE_SET_DATA_LEN):
		le_set_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN):
		le_read_default_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN):
		le_write_default_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_MAX_DATA_LEN):
		le_read_max_data_len(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
	case BT_OCF(BT_HCI_OP_LE_READ_PHY):
		le_read_phy(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_DEFAULT_PHY):
		le_set_default_phy(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_PHY):
		le_set_phy(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	case BT_OCF(BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR):
		le_set_adv_set_random_addr(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_EXT_ADV_PARAM):
		le_set_ext_adv_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_EXT_ADV_DATA):
		le_set_ext_adv_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_EXT_SCAN_RSP_DATA):
		le_set_ext_scan_rsp_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_EXT_ADV_ENABLE):
		le_set_ext_adv_enable(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN):
		le_read_max_adv_data_len(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_NUM_ADV_SETS):
		le_read_num_adv_sets(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_REMOVE_ADV_SET):
		le_remove_adv_set(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_CLEAR_ADV_SETS):
		le_clear_adv_sets(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	case BT_OCF(BT_HCI_OP_LE_SET_PER_ADV_PARAM):
		le_set_per_adv_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_PER_ADV_DATA):
		le_set_per_adv_data(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_PER_ADV_ENABLE):
		le_set_per_adv_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	case BT_OCF(BT_HCI_OP_LE_SET_EXT_SCAN_PARAM):
		le_set_ext_scan_param(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE):
		le_set_ext_scan_enable(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case BT_OCF(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC):
		le_per_adv_create_sync(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL):
		le_per_adv_create_sync_cancel(cmd, evt, node_rx);
		break;

	case BT_OCF(BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC):
		le_per_adv_terminate_sync(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE):
		le_per_adv_recv_enable(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_PER_ADV_LIST):
		le_add_dev_to_pal(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_PER_ADV_LIST):
		le_rem_dev_from_pal(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_CLEAR_PER_ADV_LIST):
		le_clear_pal(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_LE_READ_PER_ADV_LIST_SIZE):
		le_read_pal_size(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	case BT_OCF(BT_HCI_OP_LE_EXT_CREATE_CONN):
		le_ext_create_connection(cmd, evt);
		break;
#endif /* CONFIG_BT_CENTRAL */
#endif /* CONFIG_BT_CONN */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	case BT_OCF(BT_HCI_OP_LE_ADD_DEV_TO_RL):
		le_add_dev_to_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_REM_DEV_FROM_RL):
		le_rem_dev_from_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_CLEAR_RL):
		le_clear_rl(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_RL_SIZE):
		le_read_rl_size(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_PEER_RPA):
		le_read_peer_rpa(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_READ_LOCAL_RPA):
		le_read_local_rpa(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE):
		le_set_addr_res_enable(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_RPA_TIMEOUT):
		le_set_rpa_timeout(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_PRIVACY_MODE):
		le_set_privacy_mode(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	case BT_OCF(BT_HCI_OP_LE_READ_TX_POWER):
		le_read_tx_power(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_DF)
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
	case BT_OCF(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS):
		le_df_set_cl_cte_tx_params(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE):
		le_df_set_cl_cte_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	case BT_OCF(BT_HCI_OP_LE_SET_CL_CTE_SAMPLING_ENABLE):
		le_df_set_cl_iq_sampling_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
	case BT_OCF(BT_HCI_OP_LE_READ_ANT_INFO):
		le_df_read_ant_inf(cmd, evt);
		break;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX)
	case BT_OCF(BT_HCI_OP_LE_SET_CONN_CTE_TX_PARAMS):
		le_df_set_conn_cte_tx_params(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	case BT_OCF(BT_HCI_OP_LE_SET_CONN_CTE_RX_PARAMS):
		le_df_set_conn_cte_rx_params(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case BT_OCF(BT_HCI_OP_LE_CONN_CTE_REQ_ENABLE):
		le_df_set_conn_cte_req_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RSP)
	case BT_OCF(BT_HCI_OP_LE_CONN_CTE_RSP_ENABLE):
		le_df_set_conn_cte_rsp_enable(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RSP */
#endif /* CONFIG_BT_CTLR_DF */

#if defined(CONFIG_BT_CTLR_DTM_HCI)
	case BT_OCF(BT_HCI_OP_LE_RX_TEST):
		le_rx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TX_TEST):
		le_tx_test(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_TEST_END):
		le_test_end(cmd, evt);
		break;
	case BT_OCF(BT_HCI_OP_LE_ENH_RX_TEST):
		le_enh_rx_test(cmd, evt);
		break;
#if defined(CONFIG_BT_CTLR_DTM_HCI_RX_V3)
	case BT_OCF(BT_HCI_OP_LE_RX_TEST_V3):
		le_rx_test_v3(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DTM_HCI_RX_V3 */
	case BT_OCF(BT_HCI_OP_LE_ENH_TX_TEST):
		le_enh_tx_test(cmd, evt);
		break;
#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V3)
	case BT_OCF(BT_HCI_OP_LE_TX_TEST_V3):
		le_tx_test_v3(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DTM_HCI_TX_V3 */
#if defined(CONFIG_BT_CTLR_DTM_HCI_TX_V4)
	case BT_OCF(BT_HCI_OP_LE_TX_TEST_V4):
		le_tx_test_v4(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_DTM_HCI_TX_V4 */
#endif /* CONFIG_BT_CTLR_DTM_HCI */

	default:
		return -EINVAL;
	}

	return 0;
}

/* If Zephyr VS HCI commands are not enabled provide this functionality directly
 */
#if !defined(CONFIG_BT_HCI_VS_EXT)
uint8_t bt_read_static_addr(struct bt_hci_vs_static_addr addrs[], uint8_t size)
{
	return hci_vendor_read_static_addr(addrs, size);
}
#endif /* !defined(CONFIG_BT_HCI_VS_EXT) */


#if defined(CONFIG_BT_HCI_VS)
static void vs_read_version_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_version_info *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->hw_platform = sys_cpu_to_le16(BT_HCI_VS_HW_PLAT);
	rp->hw_variant = sys_cpu_to_le16(BT_HCI_VS_HW_VAR);

	rp->fw_variant = 0U;
	rp->fw_version = (KERNEL_VERSION_MAJOR & 0xff);
	rp->fw_revision = sys_cpu_to_le16(KERNEL_VERSION_MINOR);
	rp->fw_build = sys_cpu_to_le32(KERNEL_PATCHLEVEL & 0xffff);
}

static void vs_read_supported_commands(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_commands *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->commands[0], 0, sizeof(rp->commands));

	/* Set Version Information, Supported Commands, Supported Features. */
	rp->commands[0] |= BIT(0) | BIT(1) | BIT(2);
#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Write BD_ADDR, Read Build Info */
	rp->commands[0] |= BIT(5) | BIT(7);
	/* Read Static Addresses, Read Key Hierarchy Roots */
	rp->commands[1] |= BIT(0) | BIT(1);
#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	/* Write Tx Power, Read Tx Power */
	rp->commands[1] |= BIT(5) | BIT(6);
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
#if defined(CONFIG_USB_DEVICE_BLUETOOTH_VS_H4)
	/* Read Supported USB Transport Modes */
	rp->commands[1] |= BIT(7);
	/* Set USB Transport Mode */
	rp->commands[2] |= BIT(0);
#endif /* USB_DEVICE_BLUETOOTH_VS_H4 */
#endif /* CONFIG_BT_HCI_VS_EXT */
}

static void vs_read_supported_features(struct net_buf *buf,
				       struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_supported_features *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	(void)memset(&rp->features[0], 0x00, sizeof(rp->features));
}

uint8_t __weak hci_vendor_read_static_addr(struct bt_hci_vs_static_addr addrs[],
					uint8_t size)
{
	ARG_UNUSED(addrs);
	ARG_UNUSED(size);

	return 0;
}

#if defined(CONFIG_BT_HCI_VS_EXT)
static void vs_write_bd_addr(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_write_bd_addr *cmd = (void *)buf->data;

	ll_addr_set(0, &cmd->bdaddr.val[0]);

	*evt = cmd_complete_status(0x00);
}

static void vs_read_build_info(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_build_info *rp;

#define HCI_VS_BUILD_INFO "Zephyr OS v" \
	KERNEL_VERSION_STRING CONFIG_BT_CTLR_HCI_VS_BUILD_INFO

	const char build_info[] = HCI_VS_BUILD_INFO;

#define BUILD_INFO_EVT_LEN (sizeof(struct bt_hci_evt_hdr) + \
			    sizeof(struct bt_hci_evt_cmd_complete) + \
			    sizeof(struct bt_hci_rp_vs_read_build_info) + \
			    sizeof(build_info))

	BUILD_ASSERT(CONFIG_BT_BUF_EVT_RX_SIZE >= BUILD_INFO_EVT_LEN);

	rp = hci_cmd_complete(evt, sizeof(*rp) + sizeof(build_info));
	rp->status = 0x00;
	memcpy(rp->info, build_info, sizeof(build_info));
}

void __weak hci_vendor_read_key_hierarchy_roots(uint8_t ir[16], uint8_t er[16])
{
	/* Mark IR as invalid */
	(void)memset(ir, 0x00, 16);

	/* Mark ER as invalid */
	(void)memset(er, 0x00, 16);
}

static void vs_read_static_addrs(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_static_addrs *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp) +
				   sizeof(struct bt_hci_vs_static_addr));
	rp->status = 0x00;
	rp->num_addrs = hci_vendor_read_static_addr(rp->a, 1);
}

static void vs_read_key_hierarchy_roots(struct net_buf *buf,
					struct net_buf **evt)
{
	struct bt_hci_rp_vs_read_key_hierarchy_roots *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = 0x00;
	hci_vendor_read_key_hierarchy_roots(rp->ir, rp->er);
}

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
static void vs_set_min_used_chans(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_set_min_num_used_chans *cmd = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(cmd->handle);
	uint8_t status;

	status = ll_set_min_used_chans(handle, cmd->phys, cmd->min_used_chans);

	*evt = cmd_complete_status(status);
}
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
static void vs_write_tx_power_level(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_write_tx_power_level *cmd = (void *)buf->data;
	struct bt_hci_rp_vs_write_tx_power_level *rp;
	uint8_t handle_type;
	uint16_t handle;
	uint8_t status;

	handle_type = cmd->handle_type;
	handle = sys_le16_to_cpu(cmd->handle);

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->selected_tx_power = cmd->tx_power_level;

	status = ll_tx_pwr_lvl_set(handle_type, handle, &rp->selected_tx_power);

	rp->status = status;
	rp->handle_type = handle_type;
	rp->handle = sys_cpu_to_le16(handle);
}

static void vs_read_tx_power_level(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_vs_read_tx_power_level *cmd = (void *)buf->data;
	struct bt_hci_rp_vs_read_tx_power_level *rp;
	uint8_t handle_type;
	uint16_t handle;
	uint8_t status;

	handle_type = cmd->handle_type;
	handle = sys_le16_to_cpu(cmd->handle);

	rp = hci_cmd_complete(evt, sizeof(*rp));

	status = ll_tx_pwr_lvl_get(handle_type, handle, 0, &rp->tx_power_level);

	rp->status = status;
	rp->handle_type = handle_type;
	rp->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
/* A memory pool for vandor specific events for fatal error reporting purposes. */
NET_BUF_POOL_FIXED_DEFINE(vs_err_tx_pool, 1, BT_BUF_EVT_RX_SIZE, 8, NULL);

/* The alias for convenience of Controller HCI implementation. Controller is build for
 * a particular architecture hence the alias will allow to avoid conditional compilation.
 * Host may be not aware of hardware architecture the Controller is working on, hence
 * all CPU data types for supported architectures should be available during build, hence
 * the alias is defined here.
 */
#if defined(CONFIG_CPU_CORTEX_M)
typedef struct bt_hci_vs_fata_error_cpu_data_cortex_m bt_hci_vs_fatal_error_cpu_data;

static void vs_err_fatal_cpu_data_fill(bt_hci_vs_fatal_error_cpu_data *cpu_data,
				       const z_arch_esf_t *esf)
{
	cpu_data->a1 = sys_cpu_to_le32(esf->basic.a1);
	cpu_data->a2 = sys_cpu_to_le32(esf->basic.a2);
	cpu_data->a3 = sys_cpu_to_le32(esf->basic.a3);
	cpu_data->a4 = sys_cpu_to_le32(esf->basic.a4);
	cpu_data->ip = sys_cpu_to_le32(esf->basic.ip);
	cpu_data->lr = sys_cpu_to_le32(esf->basic.lr);
	cpu_data->xpsr = sys_cpu_to_le32(esf->basic.xpsr);
}
#endif /* CONFIG_CPU_CORTEX_M */

static struct net_buf *vs_err_evt_create(uint8_t subevt, uint8_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&vs_err_tx_pool, K_FOREVER);
	if (buf) {
		struct bt_hci_evt_le_meta_event *me;
		struct bt_hci_evt_hdr *hdr;

		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, BT_BUF_EVT);

		hdr = net_buf_add(buf, sizeof(*hdr));
		hdr->evt = BT_HCI_EVT_VENDOR;
		hdr->len = len + sizeof(*me);

		me = net_buf_add(buf, sizeof(*me));
		me->subevent = subevt;
	}

	return buf;
}

struct net_buf *hci_vs_err_stack_frame(unsigned int reason, const z_arch_esf_t *esf)
{
	/* Prepare vendor specific HCI Fatal Error event */
	struct bt_hci_vs_fatal_error_stack_frame *sf;
	bt_hci_vs_fatal_error_cpu_data *cpu_data;
	struct net_buf *buf;

	buf = vs_err_evt_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_STACK_FRAME,
				sizeof(*sf) + sizeof(*cpu_data));
	if (buf != NULL) {
		sf = net_buf_add(buf, (sizeof(*sf) + sizeof(*cpu_data)));
		sf->reason = sys_cpu_to_le32(reason);
		sf->cpu_type = BT_HCI_EVT_VS_ERROR_CPU_TYPE_CORTEX_M;

		vs_err_fatal_cpu_data_fill(
			(bt_hci_vs_fatal_error_cpu_data *)sf->cpu_data, esf);
	} else {
		LOG_ERR("Can't create HCI Fatal Error event");
	}

	return buf;
}

static struct net_buf *hci_vs_err_trace_create(uint8_t data_type,
					       const char *file_path,
					       uint32_t line, uint64_t pc)
{
	uint32_t file_name_len = 0U, pos = 0U;
	struct net_buf *buf = NULL;

	if (file_path) {
		/* Extract file name from a path */
		while (file_path[file_name_len] != '\0') {
			if (file_path[file_name_len] == '/') {
				pos = file_name_len + 1;
			}
			file_name_len++;
		}
		file_path += pos;
		file_name_len -= pos;

		/* If file name was found in file_path, in other words: file_path is not empty
		 * string and is not `foo/bar/`.
		 */
		if (file_name_len) {
			/* Total data length: len = file name strlen + \0 + sizeof(line number)
			 * Maximum length of an HCI event data is BT_BUF_EVT_RX_SIZE. If total data
			 * length exceeds this maximum, truncate file name.
			 */
			uint32_t data_len = 1 + sizeof(line);

			/* If a buffer is created for a TRACE data, include sizeof(pc) in total
			 * length.
			 */
			if (data_type == BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE) {
				data_len += sizeof(pc);
			}

			if (data_len + file_name_len > BT_BUF_EVT_RX_SIZE) {
				uint32_t overflow_len =
					file_name_len + data_len - BT_BUF_EVT_RX_SIZE;

				/* Truncate the file name length by number of overflow bytes */
				file_name_len -= overflow_len;
			}

			/* Get total event data length including file name length */
			data_len += file_name_len;

			/* Prepare vendor specific HCI Fatal Error event */
			buf = vs_err_evt_create(data_type, data_len);
			if (buf != NULL) {
				if (data_type == BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE) {
					net_buf_add_le64(buf, pc);
				}
				net_buf_add_mem(buf, file_path, file_name_len);
				net_buf_add_u8(buf, STR_NULL_TERMINATOR);
				net_buf_add_le32(buf, line);
			} else {
				LOG_ERR("Can't create HCI Fatal Error event");
			}
		}
	}

	return buf;
}

struct net_buf *hci_vs_err_trace(const char *file, uint32_t line, uint64_t pc)
{
	return hci_vs_err_trace_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_TRACE, file, line, pc);
}

struct net_buf *hci_vs_err_assert(const char *file, uint32_t line)
{
	/* ASSERT data does not contain PC counter, because of that zero constant is used */
	return hci_vs_err_trace_create(BT_HCI_EVT_VS_ERROR_DATA_TYPE_CTRL_ASSERT, file, line, 0U);
}
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */

#if defined(CONFIG_BT_CTLR_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES)
static void vs_le_df_connectionless_iq_report(struct pdu_data *pdu_rx, struct node_rx_pdu *node_rx,
					      struct net_buf *buf)
{
	struct bt_hci_evt_vs_le_connectionless_iq_report *sep;
	struct node_rx_iq_report *iq_report;
	struct lll_sync *lll;
	uint8_t samples_cnt;
	int16_t rssi;
	uint16_t sync_handle;
	uint16_t per_evt_counter;
	struct ll_sync_set *sync = NULL;

	iq_report = (struct node_rx_iq_report *)node_rx;

	if (!(vs_events_mask & BT_EVT_MASK_VS_LE_CONNECTIONLESS_IQ_REPORT)) {
		return;
	}

	lll = iq_report->hdr.rx_ftr.param;

	sync = HDR_LLL2ULL(lll);

	/* TX LL thread has higher priority than RX thread. It may happen that
	 * host successfully disables CTE sampling in the meantime.
	 * It should be verified here, to avoid reporting IQ samples after
	 * the functionality was disabled or if sync was lost.
	 */
	if (ull_df_sync_cfg_is_not_enabled(&lll->df_cfg) || !sync->timeout_reload) {
		/* Drop further processing of the event. */
		return;
	}

	/* Get the sync handle corresponding to the LLL context passed in the
	 * node rx footer field.
	 */
	sync_handle = ull_sync_handle_get(sync);
	per_evt_counter = iq_report->event_counter;

	/* If packet status does not indicate insufficient resources for IQ samples and for
	 * some reason sample_count is zero, inform Host about lack of valid IQ samples by
	 * storing single I_sample and Q_sample with BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE
	 * value.
	 */
	if (iq_report->packet_status == BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		samples_cnt = 0U;
	} else {
		samples_cnt = MAX(1, iq_report->sample_count);
	}

	sep = vs_event(buf, BT_HCI_EVT_VS_LE_CONNECTIONLESS_IQ_REPORT,
		       (sizeof(*sep) + (samples_cnt * sizeof(struct bt_hci_le_iq_sample16))));

	rssi = RSSI_DBM_TO_DECI_DBM(iq_report->hdr.rx_ftr.rssi);

	sep->sync_handle = sys_cpu_to_le16(sync_handle);
	sep->rssi = sys_cpu_to_le16(rssi);
	sep->rssi_ant_id = iq_report->rssi_ant_id;
	sep->cte_type = iq_report->cte_info.type;

	sep->chan_idx = iq_report->chan_idx;
	sep->per_evt_counter = sys_cpu_to_le16(per_evt_counter);

	if (sep->cte_type == BT_HCI_LE_AOA_CTE) {
		sep->slot_durations = iq_report->local_slot_durations;
	} else if (sep->cte_type == BT_HCI_LE_AOD_CTE_1US) {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;
	} else {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US;
	}

	sep->packet_status = iq_report->packet_status;

	if (iq_report->packet_status != BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		if (iq_report->sample_count == 0U) {
			sep->sample[0].i = sys_cpu_to_le16(BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE);
			sep->sample[0].q = sys_cpu_to_le16(BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE);
		} else {
			for (uint8_t idx = 0U; idx < samples_cnt; ++idx) {
				sep->sample[idx].i = sys_cpu_to_le16(iq_report->sample[idx].i);
				sep->sample[idx].q = sys_cpu_to_le16(iq_report->sample[idx].q);
			}
		}
	}

	sep->sample_count = samples_cnt;
}
#endif /* CONFIG_BT_CTLR_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES */

#if defined(CONFIG_BT_CTLR_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES)
static void vs_le_df_connection_iq_report(struct node_rx_pdu *node_rx, struct net_buf *buf)
{
	struct bt_hci_evt_vs_le_connection_iq_report *sep;
	struct node_rx_iq_report *iq_report;
	struct lll_conn *lll;
	uint8_t samples_cnt;
	uint8_t phy_rx;
	int16_t rssi;

	iq_report = (struct node_rx_iq_report *)node_rx;

	if (!(vs_events_mask & BT_EVT_MASK_VS_LE_CONNECTION_IQ_REPORT)) {
		return;
	}

	lll = iq_report->hdr.rx_ftr.param;

#if defined(CONFIG_BT_CTLR_PHY)
	phy_rx = lll->phy_rx;

	/* Make sure the report is generated for connection on PHY UNCODED */
	LL_ASSERT(phy_rx != PHY_CODED);
#else
	phy_rx = PHY_1M;
#endif /* CONFIG_BT_CTLR_PHY */

	/* TX LL thread has higher priority than RX thread. It may happen that host succefully
	 * disables CTE sampling in the meantime. It should be verified here, to avoid reporing
	 * IQ samples after the functionality was disabled.
	 */
	if (ull_df_conn_cfg_is_not_enabled(&lll->df_rx_cfg)) {
		/* Dropp further processing of the event. */
		return;
	}

	/* If packet status does not indicate insufficient resources for IQ samples and for
	 * some reason sample_count is zero, inform Host about lack of valid IQ samples by
	 * storing single I_sample and Q_sample with BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE value.
	 */
	if (iq_report->packet_status == BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		samples_cnt = 0U;
	} else {
		samples_cnt = MAX(1, iq_report->sample_count);
	}

	sep = vs_event(buf, BT_HCI_EVT_VS_LE_CONNECTION_IQ_REPORT,
			(sizeof(*sep) + (samples_cnt * sizeof(struct bt_hci_le_iq_sample16))));

	rssi = RSSI_DBM_TO_DECI_DBM(iq_report->hdr.rx_ftr.rssi);

	sep->conn_handle = sys_cpu_to_le16(iq_report->hdr.handle);
	sep->rx_phy = phy_rx;
	sep->rssi = sys_cpu_to_le16(rssi);
	sep->rssi_ant_id = iq_report->rssi_ant_id;
	sep->cte_type = iq_report->cte_info.type;

	sep->data_chan_idx = iq_report->chan_idx;
	sep->conn_evt_counter = sys_cpu_to_le16(iq_report->event_counter);

	if (sep->cte_type == BT_HCI_LE_AOA_CTE) {
		sep->slot_durations = iq_report->local_slot_durations;
	} else if (sep->cte_type == BT_HCI_LE_AOD_CTE_1US) {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_1US;
	} else {
		sep->slot_durations = BT_HCI_LE_ANTENNA_SWITCHING_SLOT_2US;
	}

	sep->packet_status = iq_report->packet_status;

	if (iq_report->packet_status != BT_HCI_LE_CTE_INSUFFICIENT_RESOURCES) {
		if (iq_report->sample_count == 0U) {
			sep->sample[0].i = sys_cpu_to_le16(BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE);
			sep->sample[0].q = sys_cpu_to_le16(BT_HCI_VS_LE_CTE_REPORT_NO_VALID_SAMPLE);
		} else {
			for (uint8_t idx = 0U; idx < samples_cnt; ++idx) {
				sep->sample[idx].i = sys_cpu_to_le16(iq_report->sample[idx].i);
				sep->sample[idx].q = sys_cpu_to_le16(iq_report->sample[idx].q);
			}
		}
	}

	sep->sample_count = samples_cnt;
}
#endif /* CONFIG_BT_CTLR_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES */

#endif /* CONFIG_BT_HCI_VS_EXT */

#if defined(CONFIG_BT_HCI_MESH_EXT)
static void mesh_get_opts(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_rp_mesh_get_opts *rp;

	rp = hci_cmd_complete(evt, sizeof(*rp));

	rp->status = 0x00;
	rp->opcode = BT_HCI_OC_MESH_GET_OPTS;

	rp->revision = BT_HCI_MESH_REVISION;
	rp->ch_map = 0x7;
	/*@todo: nRF51 only */
	rp->min_tx_power = -30;
	/*@todo: nRF51 only */
	rp->max_tx_power = 4;
	rp->max_scan_filter = CONFIG_BT_CTLR_MESH_SCAN_FILTERS;
	rp->max_filter_pattern = CONFIG_BT_CTLR_MESH_SF_PATTERNS;
	rp->max_adv_slot = 1U;
	rp->evt_prefix_len = 0x01;
	rp->evt_prefix = BT_HCI_MESH_EVT_PREFIX;
}

static void mesh_set_scan_filter(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_mesh_set_scan_filter *cmd = (void *)buf->data;
	struct bt_hci_rp_mesh_set_scan_filter *rp;
	uint8_t filter = cmd->scan_filter - 1;
	struct scan_filter *f;
	uint8_t status = 0x00;
	uint8_t i;

	if (filter > ARRAY_SIZE(scan_filters) ||
	    cmd->num_patterns > CONFIG_BT_CTLR_MESH_SF_PATTERNS) {
		status = BT_HCI_ERR_INVALID_PARAM;
		goto exit;
	}

	if (filter == sf_curr) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
		goto exit;
	}

	/* duplicate filtering not supported yet */
	if (cmd->filter_dup) {
		status = BT_HCI_ERR_INVALID_PARAM;
		goto exit;
	}

	f = &scan_filters[filter];
	for (i = 0U; i < cmd->num_patterns; i++) {
		if (!cmd->patterns[i].pattern_len ||
		    cmd->patterns[i].pattern_len >
		    BT_HCI_MESH_PATTERN_LEN_MAX) {
			status = BT_HCI_ERR_INVALID_PARAM;
			goto exit;
		}
		f->lengths[i] = cmd->patterns[i].pattern_len;
		memcpy(f->patterns[i], cmd->patterns[i].pattern, f->lengths[i]);
	}

	f->count = cmd->num_patterns;

exit:
	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->opcode = BT_HCI_OC_MESH_SET_SCAN_FILTER;
	rp->scan_filter = filter + 1;
}

static void mesh_advertise(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_mesh_advertise *cmd = (void *)buf->data;
	struct bt_hci_rp_mesh_advertise *rp;
	uint8_t adv_slot = cmd->adv_slot;
	uint8_t status;

	status = ll_mesh_advertise(adv_slot,
				   cmd->own_addr_type, cmd->random_addr.val,
				   cmd->ch_map, cmd->tx_power,
				   cmd->min_tx_delay, cmd->max_tx_delay,
				   cmd->retx_count, cmd->retx_interval,
				   cmd->scan_duration, cmd->scan_delay,
				   cmd->scan_filter, cmd->data_len, cmd->data);
	if (!status) {
		/* Yields 0xFF if no scan filter selected */
		sf_curr = cmd->scan_filter - 1;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->opcode = BT_HCI_OC_MESH_ADVERTISE;
	rp->adv_slot = adv_slot;
}

static void mesh_advertise_cancel(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_cp_mesh_advertise_cancel *cmd = (void *)buf->data;
	struct bt_hci_rp_mesh_advertise_cancel *rp;
	uint8_t adv_slot = cmd->adv_slot;
	uint8_t status;

	status = ll_mesh_advertise_cancel(adv_slot);
	if (!status) {
		/* Yields 0xFF if no scan filter selected */
		sf_curr = 0xFF;
	}

	rp = hci_cmd_complete(evt, sizeof(*rp));
	rp->status = status;
	rp->opcode = BT_HCI_OC_MESH_ADVERTISE_CANCEL;
	rp->adv_slot = adv_slot;
}

static int mesh_cmd_handle(struct net_buf *cmd, struct net_buf **evt)
{
	struct bt_hci_cp_mesh *cp_mesh;
	uint8_t mesh_op;

	if (cmd->len < sizeof(*cp_mesh)) {
		LOG_ERR("No HCI VSD Command header");
		return -EINVAL;
	}

	cp_mesh = net_buf_pull_mem(cmd, sizeof(*cp_mesh));
	mesh_op = cp_mesh->opcode;

	switch (mesh_op) {
	case BT_HCI_OC_MESH_GET_OPTS:
		mesh_get_opts(cmd, evt);
		break;

	case BT_HCI_OC_MESH_SET_SCAN_FILTER:
		mesh_set_scan_filter(cmd, evt);
		break;

	case BT_HCI_OC_MESH_ADVERTISE:
		mesh_advertise(cmd, evt);
		break;

	case BT_HCI_OC_MESH_ADVERTISE_CANCEL:
		mesh_advertise_cancel(cmd, evt);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_HCI_MESH_EXT */

int hci_vendor_cmd_handle_common(uint16_t ocf, struct net_buf *cmd,
				 struct net_buf **evt)
{
	switch (ocf) {
	case BT_OCF(BT_HCI_OP_VS_READ_VERSION_INFO):
		vs_read_version_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_COMMANDS):
		vs_read_supported_commands(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_SUPPORTED_FEATURES):
		vs_read_supported_features(cmd, evt);
		break;

#if defined(CONFIG_USB_DEVICE_BLUETOOTH_VS_H4)
	case BT_OCF(BT_HCI_OP_VS_READ_USB_TRANSPORT_MODE):
		break;
	case BT_OCF(BT_HCI_OP_VS_SET_USB_TRANSPORT_MODE):
		reset(cmd, evt);
		break;
#endif /* CONFIG_USB_DEVICE_BLUETOOTH_VS_H4 */

#if defined(CONFIG_BT_HCI_VS_EXT)
	case BT_OCF(BT_HCI_OP_VS_READ_BUILD_INFO):
		vs_read_build_info(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_WRITE_BD_ADDR):
		vs_write_bd_addr(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_STATIC_ADDRS):
		vs_read_static_addrs(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS):
		vs_read_key_hierarchy_roots(cmd, evt);
		break;

#if defined(CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL)
	case BT_OCF(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL):
		vs_write_tx_power_level(cmd, evt);
		break;

	case BT_OCF(BT_HCI_OP_VS_READ_TX_POWER_LEVEL):
		vs_read_tx_power_level(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_TX_PWR_DYNAMIC_CONTROL */
#endif /* CONFIG_BT_HCI_VS_EXT */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	case BT_OCF(BT_HCI_OP_VS_MESH):
		mesh_cmd_handle(cmd, evt);
		break;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_MIN_USED_CHAN) && defined(CONFIG_BT_PERIPHERAL)
	case BT_OCF(BT_HCI_OP_VS_SET_MIN_NUM_USED_CHANS):
		vs_set_min_used_chans(cmd, evt);
		break;
#endif /* CONFIG_BT_CTLR_MIN_USED_CHAN && CONFIG_BT_PERIPHERAL */

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

struct net_buf *hci_cmd_handle(struct net_buf *cmd, void **node_rx)
{
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt = NULL;
	uint16_t ocf;
	int err;

	if (cmd->len < sizeof(*chdr)) {
		LOG_ERR("No HCI Command header");
		return NULL;
	}

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	if (cmd->len < chdr->param_len) {
		LOG_ERR("Invalid HCI CMD packet length");
		return NULL;
	}

	/* store in a global for later CC/CS event creation */
	_opcode = sys_le16_to_cpu(chdr->opcode);

	ocf = BT_OCF(_opcode);

	switch (BT_OGF(_opcode)) {
	case BT_OGF_LINK_CTRL:
		err = link_control_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_BASEBAND:
		err = ctrl_bb_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_INFO:
		err = info_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_STATUS:
		err = status_cmd_handle(ocf, cmd, &evt);
		break;
	case BT_OGF_LE:
		err = controller_cmd_handle(ocf, cmd, &evt, node_rx);
		break;
#if defined(CONFIG_BT_HCI_VS)
	case BT_OGF_VS:
		err = hci_vendor_cmd_handle(ocf, cmd, &evt);
		break;
#endif
	default:
		err = -EINVAL;
		break;
	}

	if (err == -EINVAL) {
		evt = cmd_status(BT_HCI_ERR_UNKNOWN_CMD);
	}

	return evt;
}

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO) || \
	defined(CONFIG_BT_CTLR_CONN_ISO)
static void data_buf_overflow(struct net_buf **buf, uint8_t link_type)
{
	struct bt_hci_evt_data_buf_overflow *ep;

	if (!(event_mask & BT_EVT_MASK_DATA_BUFFER_OVERFLOW)) {
		return;
	}

	*buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	hci_evt_create(*buf, BT_HCI_EVT_DATA_BUF_OVERFLOW, sizeof(*ep));
	ep = net_buf_add(*buf, sizeof(*ep));

	ep->link_type = link_type;
}
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_SYNC_ISO ||
	* CONFIG_BT_CTLR_CONN_ISO
	*/

#if defined(CONFIG_BT_CONN)
int hci_acl_handle(struct net_buf *buf, struct net_buf **evt)
{
	struct node_tx *node_tx;
	struct bt_hci_acl_hdr *acl;
	struct pdu_data *pdu_data;
	uint16_t handle;
	uint8_t flags;
	uint16_t len;

	*evt = NULL;

	if (buf->len < sizeof(*acl)) {
		LOG_ERR("No HCI ACL header");
		return -EINVAL;
	}

	acl = net_buf_pull_mem(buf, sizeof(*acl));
	len = sys_le16_to_cpu(acl->len);
	handle = sys_le16_to_cpu(acl->handle);

	if (buf->len < len) {
		LOG_ERR("Invalid HCI ACL packet length");
		return -EINVAL;
	}

	if (len > LL_LENGTH_OCTETS_TX_MAX) {
		LOG_ERR("Invalid HCI ACL Data length");
		return -EINVAL;
	}

	/* assigning flags first because handle will be overwritten */
	flags = bt_acl_flags(handle);
	handle = bt_acl_handle(handle);

	node_tx = ll_tx_mem_acquire();
	if (!node_tx) {
		LOG_ERR("Tx Buffer Overflow");
		data_buf_overflow(evt, BT_OVERFLOW_LINK_ACL);
		return -ENOBUFS;
	}

	pdu_data = (void *)node_tx->pdu;

	if (bt_acl_flags_bc(flags) != BT_ACL_POINT_TO_POINT) {
		return -EINVAL;
	}

	switch (bt_acl_flags_pb(flags)) {
	case BT_ACL_START_NO_FLUSH:
		pdu_data->ll_id = PDU_DATA_LLID_DATA_START;
		break;
	case BT_ACL_CONT:
		pdu_data->ll_id = PDU_DATA_LLID_DATA_CONTINUE;
		break;
	default:
		/* BT_ACL_START and BT_ACL_COMPLETE not allowed on LE-U
		 * from Host to Controller
		 */
		return -EINVAL;
	}

	pdu_data->len = len;
	memcpy(&pdu_data->lldata[0], buf->data, len);

	if (ll_tx_mem_enqueue(handle, node_tx)) {
		LOG_ERR("Invalid Tx Enqueue");
		ll_tx_mem_release(node_tx);
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
int hci_iso_handle(struct net_buf *buf, struct net_buf **evt)
{
	struct bt_hci_iso_data_hdr *iso_data_hdr;
	struct isoal_sdu_tx sdu_frag_tx;
	struct bt_hci_iso_hdr *iso_hdr;
	struct ll_iso_datapath *dp_in;
	struct ll_iso_stream_hdr *hdr;
	uint32_t *time_stamp;
	uint16_t handle;
	uint8_t pb_flag;
	uint8_t ts_flag;
	uint8_t flags;
	uint16_t len;

	iso_data_hdr = NULL;
	*evt  = NULL;
	hdr   = NULL;
	dp_in = NULL;

	if (buf->len < sizeof(*iso_hdr)) {
		LOG_ERR("No HCI ISO header");
		return -EINVAL;
	}

	iso_hdr = net_buf_pull_mem(buf, sizeof(*iso_hdr));
	handle = sys_le16_to_cpu(iso_hdr->handle);
	len = sys_le16_to_cpu(iso_hdr->len);

	if (buf->len < len) {
		LOG_ERR("Invalid HCI ISO packet length");
		return -EINVAL;
	}

	/* Assigning flags first because handle will be overwritten */
	flags = bt_iso_flags(handle);
	pb_flag = bt_iso_flags_pb(flags);
	ts_flag = bt_iso_flags_ts(flags);
	handle = bt_iso_handle(handle);

	/* Extract time stamp */
	/* Set default to current time
	 * BT Core V5.3 : Vol 6 Low Energy Controller : Part G IS0-AL:
	 * 3.1 Time_Offset in framed PDUs :
	 * The Controller transmitting a SDU may use any of the following
	 * methods to determine the value of the SDU reference time:
	 * -- A captured time stamp of the SDU
	 * -- A time stamp provided by the higher layer
	 * -- A computed time stamp based on a sequence counter provided by the
	 *    higher layer (Not implemented)
	 * -- Any other method of determining Time_Offset (Not implemented)
	 */
	if (ts_flag) {
		/* Overwrite time stamp with HCI provided time stamp */
		time_stamp = net_buf_pull_mem(buf, sizeof(*time_stamp));
		len -= sizeof(*time_stamp);
		sdu_frag_tx.time_stamp = sys_le32_to_cpu(*time_stamp);
	} else {
		sdu_frag_tx.time_stamp =
			HAL_TICKER_TICKS_TO_US(ticker_ticks_now_get());
	}

	/* Extract ISO data header if included (PB_Flag 0b00 or 0b10) */
	if ((pb_flag & 0x01) == 0) {
		iso_data_hdr = net_buf_pull_mem(buf, sizeof(*iso_data_hdr));
		len -= sizeof(*iso_data_hdr);
		sdu_frag_tx.packet_sn = sys_le16_to_cpu(iso_data_hdr->sn);
		sdu_frag_tx.iso_sdu_length = sys_le16_to_cpu(iso_data_hdr->slen);
	} else {
		sdu_frag_tx.packet_sn = 0;
		sdu_frag_tx.iso_sdu_length = 0;
	}

	/* Packet boudary flags should be bitwise identical to the SDU state
	 * 0b00 BT_ISO_START
	 * 0b01 BT_ISO_CONT
	 * 0b10 BT_ISO_SINGLE
	 * 0b11 BT_ISO_END
	 */
	sdu_frag_tx.sdu_state = pb_flag;
	/* Fill in SDU buffer fields */
	sdu_frag_tx.dbuf = buf->data;
	sdu_frag_tx.size = len;

	if (false) {

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	/* Extract source handle from CIS or BIS handle by way of header and
	 * data path
	 */
	} else if (IS_CIS_HANDLE(handle)) {
		struct ll_conn_iso_stream *cis =
			ll_iso_stream_connected_get(handle);
		if (!cis) {
			return -EINVAL;
		}

		struct ll_conn_iso_group *cig = cis->group;
		uint8_t event_offset;

		hdr = &(cis->hdr);

		/* We must ensure sufficient time for ISO-AL to fragment SDU and
		 * deliver PDUs to the TX queue. By checking ull_ref_get, we
		 * know if we are within the subevents of an ISO event. If so,
		 * we can assume that we have enough time to deliver in the next
		 * ISO event. If we're not active within the ISO event, we don't
		 * know if there is enough time to deliver in the next event,
		 * and for safety we set the target to current event + 2.
		 *
		 * For FT > 1, we have the opportunity to retransmit in later
		 * event(s), in which case we have the option to target an
		 * earlier event (this or next) because being late does not
		 * instantly flush the payload.
		 */

		event_offset = ull_ref_get(&cig->ull) ? 1 : 2;

		if (cis->lll.tx.ft > 1) {
			/* FT > 1, target an earlier event */
			event_offset -= 1;
		}

		sdu_frag_tx.target_event = cis->lll.event_count + event_offset;
		sdu_frag_tx.grp_ref_point = isoal_get_wrapped_time_us(cig->cig_ref_point,
						(event_offset * cig->iso_interval *
							ISO_INT_UNIT_US));

		/* Get controller's input data path for CIS */
		dp_in = hdr->datapath_in;
		if (!dp_in || dp_in->path_id != BT_HCI_DATAPATH_ID_HCI) {
			LOG_ERR("Input data path not set for HCI");
			return -EINVAL;
		}

		/* Get input data path's source handle */
		isoal_source_handle_t source = dp_in->source_hdl;

		/* Start Fragmentation */
		isoal_status_t isoal_status =
			isoal_tx_sdu_fragment(source, &sdu_frag_tx);

		if (isoal_status) {
			if (isoal_status & ISOAL_STATUS_ERR_PDU_ALLOC) {
				data_buf_overflow(evt, BT_OVERFLOW_LINK_ISO);
				return -ENOBUFS;
			}

			return -EINVAL;
		}

		/* TODO: Assign *evt if an immediate response is required */
		return 0;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	} else if (IS_ADV_ISO_HANDLE(handle)) {
		struct lll_adv_iso_stream *stream;
		struct ll_adv_iso_set *adv_iso;
		struct lll_adv_iso *lll_iso;
		uint16_t stream_handle;
		uint16_t slen;

		/* FIXME: Code only expects header present */
		slen = iso_data_hdr ? iso_data_hdr->slen : 0;

		/* Check invalid BIS PDU length */
		if (slen > LL_BIS_OCTETS_TX_MAX) {
			LOG_ERR("Invalid HCI ISO Data length");
			return -EINVAL;
		}

		/* Get BIS stream handle and stream context */
		stream_handle = LL_BIS_ADV_IDX_FROM_HANDLE(handle);
		stream = ull_adv_iso_stream_get(stream_handle);
		if (!stream || !stream->dp) {
			LOG_ERR("Invalid BIS stream");
			return -EINVAL;
		}

		adv_iso = ull_adv_iso_by_stream_get(stream_handle);
		if (!adv_iso) {
			LOG_ERR("No BIG associated with stream handle");
			return -EINVAL;
		}

		/* FIXME: convey group start */
		sdu_frag_tx.grp_ref_point = 0;

		/* FIXME: temporary interface to enable ISOAL data Tx
		 * Create provide proper interface between client
		 * (using ISOAL target_event) and ISOAL, preferably
		 * without dependence on peeking at LL data.
		 * Problem is that client must specify a value greater
		 * than LL bisPayloadCounter or no data is sent.
		 */
		lll_iso = &adv_iso->lll;

		/* FIXME: Remove the below temporary hack to buffer up ISO data
		 * if the SDU interval and ISO interval misalign.
		 */
		uint64_t pkt_seq_num = lll_iso->payload_count / lll_iso->bn;

		if (((pkt_seq_num - stream->pkt_seq_num) & BIT64_MASK(39)) <=
		    BIT64_MASK(38)) {
			stream->pkt_seq_num = pkt_seq_num;
		} else {
			pkt_seq_num = stream->pkt_seq_num;
		}

		sdu_frag_tx.target_event = pkt_seq_num;

		stream->pkt_seq_num++;

		/* Start Fragmentation */
		/* FIXME: need to ensure ISO-AL returns proper isoal_status.
		 * Currently there are cases where ISO-AL calls LL_ASSERT.
		 */
		isoal_status_t isoal_status =
			isoal_tx_sdu_fragment(stream->dp->source_hdl, &sdu_frag_tx);

		if (isoal_status) {
			if (isoal_status & ISOAL_STATUS_ERR_PDU_ALLOC) {
				data_buf_overflow(evt, BT_OVERFLOW_LINK_ISO);
				return -ENOBUFS;
			}

			return -EINVAL;
		}

		return 0;
#endif /* CONFIG_BT_CTLR_ADV_ISO */

	}

	return -EINVAL;
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
#if defined(CONFIG_BT_CTLR_ADV_EXT)
static void dup_ext_adv_adi_store(struct dup_ext_adv_mode *dup_mode,
				  const struct pdu_adv_adi *adi,
				  uint8_t data_status)
{
	struct dup_ext_adv_set *adv_set;

	adv_set = &dup_mode->set[dup_mode->set_curr];

	adv_set->data_cmplt = (data_status ==
			       BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE) ?
			      1U : 0U;

	if (adi) {
		(void)memcpy(&adv_set->adi, adi, sizeof(*adi));
	} else {
		(void)memset(&adv_set->adi, 0U, sizeof(*adi));
	}

	if (dup_mode->set_count < CONFIG_BT_CTLR_DUP_FILTER_ADV_SET_MAX) {
		dup_mode->set_count++;
		dup_mode->set_curr = dup_mode->set_count;
	} else {
		dup_mode->set_curr++;
	}

	if (dup_mode->set_curr == CONFIG_BT_CTLR_DUP_FILTER_ADV_SET_MAX) {
		dup_mode->set_curr = 0U;
	}
}

static void dup_ext_adv_mode_reset(struct dup_ext_adv_mode *dup_adv_mode)
{
	uint8_t adv_mode;

	for (adv_mode = 0U; adv_mode < DUP_EXT_ADV_MODE_COUNT;
	     adv_mode++) {
		struct dup_ext_adv_mode *dup_mode;

		dup_mode = &dup_adv_mode[adv_mode];
		dup_mode->set_count = 0U;
		dup_mode->set_curr = 0U;
	}
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
static void dup_ext_adv_reset(void)
{
	for (int32_t i = 0; i < dup_count; i++) {
		struct dup_entry *dup;

		dup = &dup_filter[i];
		dup->mask = 0U;
		dup_ext_adv_mode_reset(dup->adv_mode);
	}
}

static void dup_periodic_adv_reset(uint8_t addr_type, const uint8_t *addr,
				   uint8_t sid)
{
	for (int32_t addr_idx = 0; addr_idx < dup_count; addr_idx++) {
		struct dup_ext_adv_mode *dup_mode;
		struct dup_entry *dup;

		dup = &dup_filter[addr_idx];
		if (memcmp(addr, dup->addr.a.val, sizeof(bt_addr_t)) ||
		    (addr_type != dup->addr.type)) {
			continue;
		}

		dup_mode = &dup->adv_mode[DUP_EXT_ADV_MODE_PERIODIC];
		for (uint16_t set_idx = 0; set_idx < dup_mode->set_count;
		     set_idx++) {
			struct dup_ext_adv_set *adv_set;

			adv_set = &dup_mode->set[set_idx];
			if (adv_set->adi.sid != sid) {
				continue;
			}

			/* reset data complete state */
			adv_set->data_cmplt = 0U;

			return;
		}

		return;
	}
}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

static inline bool is_dup_or_update(struct dup_entry *dup, uint8_t adv_type,
				    uint8_t adv_mode,
				    const struct pdu_adv_adi *adi,
				    uint8_t data_status)
{
	if (!(dup->mask & BIT(adv_type))) {
		/* report different adv types */
		dup->mask |= BIT(adv_type);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		dup_ext_adv_adi_store(&dup->adv_mode[adv_mode], adi,
				      data_status);

		return false;
	} else if (adv_type != PDU_ADV_TYPE_EXT_IND) {
		/* drop duplicate legacy advertising */
		return true;
	} else if (dup->adv_mode[adv_mode].set_count == 0U) {
		/* report different extended adv mode */
		dup_ext_adv_adi_store(&dup->adv_mode[adv_mode], adi,
				      data_status);
		return false;
	} else if (adi) {
		struct dup_ext_adv_mode *dup_mode;
		uint8_t j;

		dup_mode = &dup->adv_mode[adv_mode];
		for (j = 0; j < dup_mode->set_count; j++) {
			struct dup_ext_adv_set *adv_set;

			adv_set = &dup_mode->set[j];
			if (adv_set->adi.sid != adi->sid) {
				continue;
			}

			if (adv_set->adi.did != adi->did) {
				/* report different DID */
				adv_set->adi.did = adi->did;
				/* set new data status */
				if (data_status == BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE) {
					adv_set->data_cmplt = 1U;
				} else {
					adv_set->data_cmplt = 0U;
				}

				return false;
			} else if (!adv_set->data_cmplt &&
				   (data_status ==
				    BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE)) {
				/* report data complete */
				adv_set->data_cmplt = 1U;
				return false;
			} else if (!adv_set->data_cmplt) {
				/* report partial and incomplete data */
				return false;
			}

			return true;
		}

		dup_ext_adv_adi_store(&dup->adv_mode[adv_mode], adi,
				      data_status);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		return false;
	}

	return true;
}

static bool dup_found(uint8_t adv_type, uint8_t addr_type, const uint8_t *addr,
		      uint8_t adv_mode, const struct pdu_adv_adi *adi,
		      uint8_t data_status)
{
	/* check for duplicate filtering */
	if (dup_count >= 0) {
		struct dup_entry *dup;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		__ASSERT((adv_mode < ARRAY_SIZE(dup_filter[0].adv_mode)),
			 "adv_mode index out-of-bound");
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		/* find for existing entry and update if changed */
		for (int32_t i = 0; i < dup_count; i++) {
			dup = &dup_filter[i];
			if (memcmp(addr, &dup->addr.a.val[0],
				   sizeof(bt_addr_t)) ||
			    (addr_type != dup->addr.type)) {
				continue;
			}

			/* still duplicate or update entry with change */
			return is_dup_or_update(dup, adv_type, adv_mode, adi,
						data_status);
		}

		/* insert into the duplicate filter */
		dup = &dup_filter[dup_curr];
		(void)memcpy(&dup->addr.a.val[0], addr, sizeof(bt_addr_t));
		dup->addr.type = addr_type;
		dup->mask = BIT(adv_type);

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		dup_ext_adv_mode_reset(dup->adv_mode);
		dup_ext_adv_adi_store(&dup->adv_mode[adv_mode], adi,
				      data_status);
#endif /* CONFIG_BT_CTLR_ADV_EXT */

		if (dup_count < CONFIG_BT_CTLR_DUP_FILTER_LEN) {
			dup_count++;
			dup_curr = dup_count;
		} else {
			dup_curr++;
		}

		if (dup_curr == CONFIG_BT_CTLR_DUP_FILTER_LEN) {
			dup_curr = 0U;
		}
	}

	return false;
}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
static inline void le_dir_adv_report(struct pdu_adv *adv, struct net_buf *buf,
				     int8_t rssi, uint8_t rl_idx)
{
	struct bt_hci_evt_le_direct_adv_report *drp;
	struct bt_hci_evt_le_direct_adv_info *dir_info;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_DIRECT_ADV_REPORT)) {
		return;
	}

	LL_ASSERT(adv->type == PDU_ADV_TYPE_DIRECT_IND);

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (dup_scan &&
	    dup_found(adv->type, adv->tx_addr, adv->adv_ind.addr, 0, NULL, 0)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	drp = meta_evt(buf, BT_HCI_EVT_LE_DIRECT_ADV_REPORT,
		       sizeof(*drp) + sizeof(*dir_info));

	drp->num_reports = 1U;
	dir_info = (void *)(((uint8_t *)drp) + sizeof(*drp));

	/* Directed Advertising */
	dir_info->evt_type = BT_HCI_ADV_DIRECT_IND;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &dir_info->addr.type,
				  &dir_info->addr.a.val[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		dir_info->addr.type += 2U;
	} else {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */
		dir_info->addr.type = adv->tx_addr;
		memcpy(&dir_info->addr.a.val[0], &adv->direct_ind.adv_addr[0],
		       sizeof(bt_addr_t));
	}

	dir_info->dir_addr.type = adv->rx_addr;
	memcpy(&dir_info->dir_addr.a.val[0],
	       &adv->direct_ind.tgt_addr[0], sizeof(bt_addr_t));

	dir_info->rssi = rssi;
}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_HCI_MESH_EXT)
static inline bool scan_filter_apply(uint8_t filter, uint8_t *data, uint8_t len)
{
	struct scan_filter *f = &scan_filters[filter];

	/* No patterns means filter out all advertising packets */
	for (uint8_t i = 0; i < f->count; i++) {
		/* Require at least the length of the pattern */
		if (len >= f->lengths[i] &&
		    !memcmp(data, f->patterns[i], f->lengths[i])) {
			return true;
		}
	}

	return false;
}

static inline void le_mesh_scan_report(struct pdu_adv *adv,
				       struct node_rx_pdu *node_rx,
				       struct net_buf *buf, int8_t rssi)
{
	uint8_t data_len = (adv->len - BDADDR_SIZE);
	struct bt_hci_evt_mesh_scanning_report *mep;
	struct bt_hci_evt_mesh_scan_report *sr;
	uint32_t instant;
	uint8_t chan;

	LL_ASSERT(adv->type == PDU_ADV_TYPE_NONCONN_IND);

	/* Filter based on currently active Scan Filter */
	if (sf_curr < ARRAY_SIZE(scan_filters) &&
	    !scan_filter_apply(sf_curr, &adv->adv_ind.data[0], data_len)) {
		/* Drop the report */
		return;
	}

	chan = node_rx->hdr.rx_ftr.chan;
	instant = node_rx->hdr.rx_ftr.anchor_ticks;

	mep = mesh_evt(buf, BT_HCI_EVT_MESH_SCANNING_REPORT,
			    sizeof(*mep) + sizeof(*sr));

	mep->num_reports = 1U;
	sr = (void *)(((uint8_t *)mep) + sizeof(*mep));
	sr->addr.type = adv->tx_addr;
	memcpy(&sr->addr.a.val[0], &adv->adv_ind.addr[0], sizeof(bt_addr_t));
	sr->chan = chan;
	sr->rssi = rssi;
	sys_put_le32(instant, (uint8_t *)&sr->instant);

	sr->data_len = data_len;
	memcpy(&sr->data[0], &adv->adv_ind.data[0], data_len);
}
#endif /* CONFIG_BT_HCI_MESH_EXT */

static void le_advertising_report(struct pdu_data *pdu_data,
				  struct node_rx_pdu *node_rx,
				  struct net_buf *buf)
{
	const uint8_t c_adv_type[] = { 0x00, 0x01, 0x03, 0xff, 0x04,
				    0xff, 0x02 };
	struct bt_hci_evt_le_advertising_report *sep;
	struct pdu_adv *adv = (void *)pdu_data;
	struct bt_hci_evt_le_advertising_info *adv_info;
	uint8_t data_len;
	uint8_t info_len;
	int8_t rssi;
#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	uint8_t direct_report;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */
	int8_t *prssi;

	rssi = -(node_rx->hdr.rx_ftr.rssi);
#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = node_rx->hdr.rx_ftr.rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */
#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	direct_report = node_rx->hdr.rx_ftr.direct;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (adv->tx_addr) {
		/* Update current RPA */
		ll_rl_crpa_set(0x00, NULL, rl_idx, &adv->adv_ind.addr[0]);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
	if (direct_report) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
		le_dir_adv_report(adv, buf, rssi, rl_idx);
#else
		le_dir_adv_report(adv, buf, rssi, 0xFF);
#endif /* CONFIG_BT_CTLR_PRIVACY */
		return;
	}
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	if (node_rx->hdr.type == NODE_RX_TYPE_MESH_REPORT) {
		le_mesh_scan_report(adv, node_rx, buf, rssi);
		return;
	}
#endif /* CONFIG_BT_HCI_MESH_EXT */

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_ADVERTISING_REPORT)) {
		return;
	}

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (dup_scan &&
	    dup_found(adv->type, adv->tx_addr, adv->adv_ind.addr, 0, NULL, 0)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
		data_len = (adv->len - BDADDR_SIZE);
	} else {
		data_len = 0U;
	}
	info_len = sizeof(struct bt_hci_evt_le_advertising_info) + data_len +
		   sizeof(*prssi);
	sep = meta_evt(buf, BT_HCI_EVT_LE_ADVERTISING_REPORT,
		       sizeof(*sep) + info_len);

	sep->num_reports = 1U;
	adv_info = (void *)(((uint8_t *)sep) + sizeof(*sep));

	adv_info->evt_type = c_adv_type[adv->type];

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &adv_info->addr.type,
				  &adv_info->addr.a.val[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		adv_info->addr.type += 2U;
	} else {
#else
	if (1) {
#endif /* CONFIG_BT_CTLR_PRIVACY */

		adv_info->addr.type = adv->tx_addr;
		memcpy(&adv_info->addr.a.val[0], &adv->adv_ind.addr[0],
		       sizeof(bt_addr_t));
	}

	adv_info->length = data_len;
	memcpy(&adv_info->data[0], &adv->adv_ind.data[0], data_len);
	/* RSSI */
	prssi = &adv_info->data[0] + data_len;
	*prssi = rssi;
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static void le_ext_adv_legacy_report(struct pdu_data *pdu_data,
				     struct node_rx_pdu *node_rx,
				     struct net_buf *buf)
{
	/* Lookup event type based on pdu_adv_type set by LLL */
	const uint8_t evt_type_lookup[] = {
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY | BT_HCI_LE_ADV_EVT_TYPE_SCAN |
		 BT_HCI_LE_ADV_EVT_TYPE_CONN),   /* ADV_IND */
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY | BT_HCI_LE_ADV_EVT_TYPE_DIRECT |
		 BT_HCI_LE_ADV_EVT_TYPE_CONN),   /* DIRECT_IND */
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY), /* NONCONN_IND */
		0xff,                            /* Invalid index lookup */
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY |
		 BT_HCI_LE_ADV_EVT_TYPE_SCAN_RSP |
		 BT_HCI_LE_ADV_EVT_TYPE_SCAN),   /* SCAN_RSP to an ADV_SCAN_IND
						  */
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY |
		 BT_HCI_LE_ADV_EVT_TYPE_SCAN_RSP |
		 BT_HCI_LE_ADV_EVT_TYPE_SCAN |
		 BT_HCI_LE_ADV_EVT_TYPE_CONN), /* SCAN_RSP to an ADV_IND,
						* NOTE: LLL explicitly sets
						* adv_type to
						* PDU_ADV_TYPE_ADV_IND_SCAN_RSP
						*/
		(BT_HCI_LE_ADV_EVT_TYPE_LEGACY |
		 BT_HCI_LE_ADV_EVT_TYPE_SCAN)    /* SCAN_IND */
	};
	struct bt_hci_evt_le_ext_advertising_info *adv_info;
	struct bt_hci_evt_le_ext_advertising_report *sep;
	struct pdu_adv *adv = (void *)pdu_data;
	uint8_t data_len;
	uint8_t info_len;
	int8_t rssi;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_EXT_ADVERTISING_REPORT)) {
		return;
	}

	/* The Link Layer currently returns RSSI as an absolute value */
	rssi = -(node_rx->hdr.rx_ftr.rssi);

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = node_rx->hdr.rx_ftr.rl_idx;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (adv->tx_addr) {
		/* Update current RPA */
		ll_rl_crpa_set(0x00, NULL, rl_idx, &adv->adv_ind.addr[0]);
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (dup_scan &&
	    dup_found(adv->type, adv->tx_addr, adv->adv_ind.addr, 0, NULL, 0)) {
		return;
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	if (adv->type != PDU_ADV_TYPE_DIRECT_IND) {
		data_len = (adv->len - BDADDR_SIZE);
	} else {
		data_len = 0U;
	}

	info_len = sizeof(struct bt_hci_evt_le_ext_advertising_info) +
		   data_len;
	sep = meta_evt(buf, BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT,
		       sizeof(*sep) + info_len);

	sep->num_reports = 1U;
	adv_info = (void *)(((uint8_t *)sep) + sizeof(*sep));

	adv_info->evt_type = evt_type_lookup[adv->type];

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &adv_info->addr.type,
				  &adv_info->addr.a.val[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		adv_info->addr.type += 2U;
	} else
#endif /* CONFIG_BT_CTLR_PRIVACY */
	{
		adv_info->addr.type = adv->tx_addr;
		memcpy(&adv_info->addr.a.val[0], &adv->adv_ind.addr[0],
		       sizeof(bt_addr_t));
	}

	adv_info->prim_phy = BT_HCI_LE_EXT_SCAN_PHY_1M;
	adv_info->sec_phy = 0U;
	adv_info->sid = 0xff;
	adv_info->tx_power = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
	adv_info->rssi = rssi;
	adv_info->interval = 0U;

	if (adv->type == PDU_ADV_TYPE_DIRECT_IND) {
		adv_info->direct_addr.type = adv->rx_addr;
		bt_addr_copy(&adv_info->direct_addr.a,
			     (void *)adv->direct_ind.tgt_addr);
	} else {
		adv_info->direct_addr.type = 0U;
		(void)memset(adv_info->direct_addr.a.val, 0U,
			     sizeof(adv_info->direct_addr.a.val));
	}

	adv_info->length = data_len;
	memcpy(&adv_info->data[0], &adv->adv_ind.data[0], data_len);
}

static uint8_t ext_adv_direct_addr_type(struct lll_scan *lll,
					bool peer_resolved, bool direct_report,
					uint8_t rx_addr_type,
					const uint8_t *const rx_addr)
{
	/* The directed address is resolvable private address, but Controller
	 * could not resolve it.
	 */
	if (direct_report) {
		return BT_ADDR_LE_UNRESOLVED;
	}

	if (0) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
	/* Peer directed advertiser's address was resolved */
	} else if (peer_resolved) {
		struct ll_scan_set *scan;

		scan = HDR_LLL2ULL(lll);
		if ((rx_addr_type == lll->init_addr_type) &&
		    !memcmp(lll->init_addr, rx_addr, BDADDR_SIZE)) {
			/* Peer directed advertiser used local scanner's
			 * initiator address.
			 */
			return scan->own_addr_type;
		}

		/* Peer directed advertiser used directed resolvable
		 * private address generated from the local scanner's
		 * Identity Resolution Key.
		 */
		return scan->own_addr_type | BIT(1);
#endif /* CONFIG_BT_CTLR_PRIVACY */
	} else {
		struct ll_scan_set *scan;

		scan = HDR_LLL2ULL(lll);

		/* Peer directed advertiser used local scanner's
		 * initiator address.
		 */
		return scan->own_addr_type;
	}
}

static uint8_t ext_adv_data_get(const struct node_rx_pdu *node_rx_data,
				uint8_t *const sec_phy, int8_t *const tx_pwr,
				const uint8_t **const data)
{
	const struct pdu_adv *adv = (void *)node_rx_data->pdu;
	const struct pdu_adv_com_ext_adv *p;
	const struct pdu_adv_ext_hdr *h;
	uint8_t hdr_buf_len;
	const uint8_t *ptr;
	uint8_t hdr_len;

	*tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;

	p = (void *)&adv->adv_ext_ind;
	h = (void *)p->ext_hdr_adv_data;
	ptr = (void *)h;

	if (!p->ext_hdr_len) {
		hdr_len = PDU_AC_EXT_HEADER_SIZE_MIN;

		goto no_ext_hdr;
	}

	ptr = h->data;

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->tgt_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	if (h->aux_ptr) {
		struct pdu_adv_aux_ptr *aux_ptr;

		aux_ptr = (void *)ptr;
		ptr += sizeof(*aux_ptr);

		*sec_phy = HCI_AUX_PHY_TO_HCI_PHY(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));
	}

	if (h->sync_info) {
		ptr += sizeof(struct pdu_adv_sync_info);
	}

	if (h->tx_pwr) {
		*tx_pwr = *(int8_t *)ptr;
		ptr++;
	}

	hdr_len = ptr - (uint8_t *)p;
	hdr_buf_len = PDU_AC_EXT_HEADER_SIZE_MIN + p->ext_hdr_len;
	if (hdr_len < hdr_buf_len) {
		uint8_t acad_len = hdr_buf_len - hdr_len;

		ptr += acad_len;
		hdr_len += acad_len;
	}

no_ext_hdr:
	if (hdr_len < adv->len) {
		*data = ptr;

		return adv->len - hdr_len;
	}

	return 0;
}

static void node_rx_extra_list_release(struct node_rx_pdu *node_rx_extra)
{
	while (node_rx_extra) {
		struct node_rx_pdu *node_rx_curr;

		node_rx_curr = node_rx_extra;
		node_rx_extra = node_rx_curr->hdr.rx_ftr.extra;

		node_rx_curr->hdr.next = NULL;
		ll_rx_mem_release((void **)&node_rx_curr);
	}
}

static void ext_adv_info_fill(uint8_t evt_type, uint8_t phy, uint8_t sec_phy,
			      uint8_t adv_addr_type, const uint8_t *adv_addr,
			      uint8_t direct_addr_type,
			      const uint8_t *direct_addr, uint8_t rl_idx,
			      int8_t tx_pwr, int8_t rssi,
			      uint16_t interval_le16,
			      const struct pdu_adv_adi *adi, uint8_t data_len,
			      const uint8_t *data, struct net_buf *buf)
{
	struct bt_hci_evt_le_ext_advertising_info *adv_info;
	struct bt_hci_evt_le_ext_advertising_report *sep;
	uint8_t info_len;

	info_len = sizeof(struct bt_hci_evt_le_ext_advertising_info) +
		   data_len;
	sep = meta_evt(buf, BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT,
		       sizeof(*sep) + info_len);

	sep->num_reports = 1U;
	adv_info = (void *)(((uint8_t *)sep) + sizeof(*sep));

	adv_info->evt_type = evt_type;

	if (0) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
	} else if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &adv_info->addr.type,
				  adv_info->addr.a.val);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		adv_info->addr.type += 2U;
#else /* !CONFIG_BT_CTLR_PRIVACY */
		ARG_UNUSED(rl_idx);
#endif /* !CONFIG_BT_CTLR_PRIVACY */
	} else if (adv_addr) {
		adv_info->addr.type = adv_addr_type;
		(void)memcpy(adv_info->addr.a.val, adv_addr, sizeof(bt_addr_t));
	} else {
		adv_info->addr.type = 0U;
		(void)memset(adv_info->addr.a.val, 0, sizeof(bt_addr_t));
	}

	adv_info->prim_phy = find_lsb_set(phy);
	adv_info->sec_phy = sec_phy;
	adv_info->sid = (adi) ? adi->sid : BT_HCI_LE_EXT_ADV_SID_INVALID;
	adv_info->tx_power = tx_pwr;
	adv_info->rssi = rssi;
	adv_info->interval = interval_le16;

	if (evt_type & BT_HCI_LE_ADV_EVT_TYPE_DIRECT) {
		adv_info->direct_addr.type = direct_addr_type;
		(void)memcpy(adv_info->direct_addr.a.val, direct_addr,
			     sizeof(bt_addr_t));
	} else {
		adv_info->direct_addr.type = 0U;
		(void)memset(adv_info->direct_addr.a.val, 0, sizeof(bt_addr_t));
	}

	adv_info->length = data_len;
	(void)memcpy(adv_info->data, data, data_len);
}

static void ext_adv_pdu_frag(uint8_t evt_type, uint8_t phy, uint8_t sec_phy,
			     uint8_t adv_addr_type, const uint8_t *adv_addr,
			     uint8_t direct_addr_type,
			     const uint8_t *direct_addr, uint8_t rl_idx,
			     int8_t tx_pwr, int8_t rssi, uint16_t interval_le16,
			     const struct pdu_adv_adi *adi,
			     uint8_t data_len_max,
			     uint16_t *const data_len_total,
			     uint8_t *const data_len,
			     const uint8_t **const data, struct net_buf *buf,
			     struct net_buf **const evt_buf)
{
	const uint8_t data_len_frag = MIN(*data_len, data_len_max);

	do {
		/* Prepare a fragment of PDU data in a HCI event */
		ext_adv_info_fill(evt_type, phy, sec_phy, adv_addr_type,
				  adv_addr, direct_addr_type, direct_addr,
				  rl_idx, tx_pwr, rssi, interval_le16, adi,
				  data_len_frag, *data, *evt_buf);

		*data += data_len_frag;
		*data_len -= data_len_frag;
		*data_len_total -= data_len_frag;

		*evt_buf = bt_buf_get_rx(BT_BUF_EVT, BUF_GET_TIMEOUT);
		LL_ASSERT(*evt_buf);

		net_buf_frag_add(buf, *evt_buf);

		/* Continue to fragment until last partial PDU data fragment,
		 * remainder PDU data's HCI event will be prepare by caller.
		 */
	} while (*data_len > data_len_max);
}

static void ext_adv_data_frag(const struct node_rx_pdu *node_rx_data,
			      uint8_t evt_type, uint8_t phy,
			      uint8_t *const sec_phy, uint8_t adv_addr_type,
			      const uint8_t *adv_addr, uint8_t direct_addr_type,
			      const uint8_t *direct_addr, uint8_t rl_idx,
			      int8_t *const tx_pwr, int8_t rssi,
			      uint16_t interval_le16,
			      const struct pdu_adv_adi *adi,
			      uint8_t data_len_max, uint16_t data_len_total,
			      uint8_t *const data_len,
			      const uint8_t **const data, struct net_buf *buf,
			      struct net_buf **const evt_buf)
{
	evt_type |= (BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL << 5);

	do {
		/* Fragment the PDU data */
		ext_adv_pdu_frag(evt_type, phy, *sec_phy, adv_addr_type,
				 adv_addr, direct_addr_type, direct_addr,
				 rl_idx, *tx_pwr, rssi, interval_le16, adi,
				 data_len_max, &data_len_total, data_len,
				 data, buf, evt_buf);

		/* Check if more PDUs in the list */
		node_rx_data = node_rx_data->hdr.rx_ftr.extra;
		if (node_rx_data) {
			if (*data_len >= data_len_total) {
				/* Last fragment restricted to maximum scan
				 * data length, caller will prepare the last
				 * HCI fragment event.
				 */
				break;
			} else if (*data_len) {
				/* Last fragment of current PDU data */
				ext_adv_pdu_frag(evt_type, phy, *sec_phy,
						 adv_addr_type, adv_addr,
						 direct_addr_type, direct_addr,
						 rl_idx, *tx_pwr, rssi,
						 interval_le16, adi,
						 data_len_max, &data_len_total,
						 data_len, data, buf, evt_buf);
			}

			/* Get next PDU data in list */
			*data_len = ext_adv_data_get(node_rx_data, sec_phy,
						     tx_pwr, data);

			/* Restrict PDU data to maximum scan data length */
			if (*data_len > data_len_total) {
				*data_len = data_len_total;
			}
		}

		/* Continue to fragment if current PDU data length less than
		 * total data length or current PDU data length greater than
		 * HCI event max length.
		 */
	} while ((*data_len < data_len_total) || (*data_len > data_len_max));
}

static void le_ext_adv_report(struct pdu_data *pdu_data,
			      struct node_rx_pdu *node_rx,
			      struct net_buf *buf, uint8_t phy)
{
	int8_t scan_rsp_tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
	int8_t tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
	struct node_rx_pdu *node_rx_scan_data = NULL;
	struct node_rx_pdu *node_rx_data = NULL;
	const struct pdu_adv_adi *adi = NULL;
	uint16_t scan_data_len_total = 0U;
	struct node_rx_pdu *node_rx_curr;
	struct node_rx_pdu *node_rx_next;
	const uint8_t *scan_data = NULL;
	uint8_t scan_data_status = 0U;
	uint8_t direct_addr_type = 0U;
	uint16_t data_len_total = 0U;
	uint8_t *direct_addr = NULL;
	uint16_t interval_le16 = 0U;
	const uint8_t *data = NULL;
	uint8_t scan_data_len = 0U;
	uint8_t adv_addr_type = 0U;
	uint8_t sec_phy_scan = 0U;
	uint8_t *adv_addr = NULL;
	uint8_t data_status = 0U;
	struct net_buf *evt_buf;
	bool devmatch = false;
	uint8_t data_len = 0U;
	uint8_t evt_type = 0U;
	uint8_t sec_phy = 0U;
	uint8_t data_len_max;
	uint8_t rl_idx = 0U;
	struct pdu_adv *adv;
	int8_t rssi;

	/* NOTE: This function uses a lot of initializers before the check and
	 * return below, as an exception to initializing close to their locality
	 * of reference. This is acceptable as the return is unlikely in typical
	 * Controller use.
	 */
	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_EXT_ADVERTISING_REPORT)) {
		node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);
		return;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = ll_rl_size_get();
#endif /* CONFIG_BT_CTLR_PRIVACY */

	adv = (void *)pdu_data;
	node_rx_curr = node_rx;
	node_rx_next = node_rx_curr->hdr.rx_ftr.extra;
	do {
		int8_t tx_pwr_curr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
		struct pdu_adv_adi *adi_curr = NULL;
		uint8_t direct_addr_type_curr = 0U;
		bool direct_resolved_curr = false;
		uint8_t *direct_addr_curr = NULL;
		uint8_t adv_addr_type_curr = 0U;
		struct pdu_adv_com_ext_adv *p;
		uint8_t *adv_addr_curr = NULL;
		uint8_t data_len_curr = 0U;
		uint8_t *data_curr = NULL;
		struct pdu_adv_ext_hdr *h;
		uint8_t sec_phy_curr = 0U;
		uint8_t evt_type_curr;
		uint8_t hdr_buf_len;
		uint8_t hdr_len;
		uint8_t *ptr;

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
		bool direct_report_curr = node_rx_curr->hdr.rx_ftr.direct;
#endif /* CONFIG_BT_CTLR_EXT_SCAN_FP */

#if defined(CONFIG_BT_CTLR_PRIVACY)
		uint8_t rl_idx_curr = node_rx_curr->hdr.rx_ftr.rl_idx;

		direct_resolved_curr = node_rx_curr->hdr.rx_ftr.direct_resolved;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
		const bool devmatch_curr = node_rx_curr->hdr.rx_ftr.devmatch;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

		/* The Link Layer currently returns RSSI as an absolute value */
		rssi = -(node_rx_curr->hdr.rx_ftr.rssi);

		LOG_DBG("phy= 0x%x, type= 0x%x, len= %u, tat= %u, rat= %u,"
		       " rssi=%d dB", phy, adv->type, adv->len, adv->tx_addr,
		       adv->rx_addr, rssi);

		p = (void *)&adv->adv_ext_ind;
		h = (void *)p->ext_hdr_adv_data;
		ptr = (void *)h;

		LOG_DBG("    Ext. adv mode= 0x%x, hdr len= %u", p->adv_mode, p->ext_hdr_len);

		evt_type_curr = p->adv_mode;

		if (!p->ext_hdr_len) {
			hdr_len = PDU_AC_EXT_HEADER_SIZE_MIN;

			goto no_ext_hdr;
		}

		ptr = h->data;

		if (h->adv_addr) {
			bt_addr_le_t addr;

			adv_addr_type_curr = adv->tx_addr;
			adv_addr_curr = ptr;

			addr.type = adv->tx_addr;
			(void)memcpy(addr.a.val, ptr, sizeof(bt_addr_t));
			ptr += BDADDR_SIZE;

			LOG_DBG("    AdvA: %s", bt_addr_le_str(&addr));
		}

		if (h->tgt_addr) {
			struct lll_scan *lll;
			bt_addr_le_t addr;

			lll = node_rx->hdr.rx_ftr.param;

#if defined(CONFIG_BT_CTLR_EXT_SCAN_FP)
			direct_addr_type_curr =
				ext_adv_direct_addr_type(lll,
							 direct_resolved_curr,
							 direct_report_curr,
							 adv->rx_addr, ptr);
#else /* !CONFIG_BT_CTLR_EXT_SCAN_FP */
			direct_addr_type_curr =
				ext_adv_direct_addr_type(lll,
							 direct_resolved_curr,
							 false, adv->rx_addr,
							 ptr);
#endif /* !CONFIG_BT_CTLR_EXT_SCAN_FP */

			direct_addr_curr = ptr;
			ptr += BDADDR_SIZE;

			addr.type = adv->rx_addr;
			(void)memcpy(addr.a.val, direct_addr_curr,
				     sizeof(bt_addr_t));

			LOG_DBG("    TgtA: %s", bt_addr_le_str(&addr));
		}

		if (h->adi) {
			adi_curr = (void *)ptr;

			ptr += sizeof(*adi);

			LOG_DBG("    AdvDataInfo DID = 0x%x, SID = 0x%x", adi_curr->did,
				adi_curr->sid);
		}

		if (h->aux_ptr) {
			struct pdu_adv_aux_ptr *aux_ptr;
			uint8_t aux_phy;

			aux_ptr = (void *)ptr;
			if (PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) > EXT_ADV_AUX_PHY_LE_CODED) {
				struct node_rx_ftr *ftr;

				ftr = &node_rx->hdr.rx_ftr;
				node_rx_extra_list_release(ftr->extra);
				return;
			}

			ptr += sizeof(*aux_ptr);

			sec_phy_curr = HCI_AUX_PHY_TO_HCI_PHY(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));

			aux_phy = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));

			LOG_DBG("    AuxPtr chan_idx = %u, ca = %u, offs_units "
			       "= %u offs = 0x%x, phy = 0x%x",
			       aux_ptr->chan_idx, aux_ptr->ca,
			       aux_ptr->offs_units, PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr), aux_phy);
		}

		if (h->sync_info) {
			struct pdu_adv_sync_info *si;

			si = (void *)ptr;
			ptr += sizeof(*si);

			interval_le16 = si->interval;

			LOG_DBG("    SyncInfo offs = %u, offs_unit = 0x%x, "
			       "interval = 0x%x, sca = 0x%x, "
			       "chan map = 0x%x 0x%x 0x%x 0x%x 0x%x, "
			       "AA = 0x%x, CRC = 0x%x 0x%x 0x%x, "
			       "evt cntr = 0x%x",
			       sys_le16_to_cpu(si->offs),
			       si->offs_units,
			       sys_le16_to_cpu(si->interval),
			       ((si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &
				 PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK) >>
				PDU_SYNC_INFO_SCA_CHM_SCA_BIT_POS),
			       si->sca_chm[0], si->sca_chm[1], si->sca_chm[2],
			       si->sca_chm[3],
			       (si->sca_chm[PDU_SYNC_INFO_SCA_CHM_SCA_BYTE_OFFSET] &
				~PDU_SYNC_INFO_SCA_CHM_SCA_BIT_MASK),
			       sys_le32_to_cpu(si->aa),
			       si->crc_init[0], si->crc_init[1],
			       si->crc_init[2], sys_le16_to_cpu(si->evt_cntr));
		}

		if (h->tx_pwr) {
			tx_pwr_curr = *(int8_t *)ptr;
			ptr++;

			LOG_DBG("    Tx pwr= %d dB", tx_pwr_curr);
		}

		hdr_len = ptr - (uint8_t *)p;
		hdr_buf_len = PDU_AC_EXT_HEADER_SIZE_MIN + p->ext_hdr_len;
		if (hdr_len > hdr_buf_len) {
			LOG_WRN("    Header length %u/%u, INVALID.", hdr_len, p->ext_hdr_len);
		} else {
			uint8_t acad_len = hdr_buf_len - hdr_len;

			if (acad_len) {
				ptr += acad_len;
				hdr_len += acad_len;
			}
		}

no_ext_hdr:
		if (hdr_len < adv->len) {
			data_len_curr = adv->len - hdr_len;
			data_curr = ptr;

			LOG_DBG("    AD Data (%u): <todo>", data_len);
		}

		if (node_rx_curr == node_rx) {
			evt_type = evt_type_curr;
			adv_addr_type = adv_addr_type_curr;
			adv_addr = adv_addr_curr;
			direct_addr_type = direct_addr_type_curr;
			direct_addr = direct_addr_curr;
			adi = adi_curr;
			sec_phy = sec_phy_curr;
			node_rx_data = node_rx_curr;
			data_len = data_len_curr;
			data_len_total = data_len;
			data = data_curr;
			scan_data_len_total = 0U;
			tx_pwr = tx_pwr_curr;

#if defined(CONFIG_BT_CTLR_PRIVACY)
			rl_idx = rl_idx_curr;
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
			devmatch = devmatch_curr;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */

		} else {
			/* TODO: Validate current value with previous */

			/* Detect the scan response in the list of node_rx */
			if (node_rx_curr->hdr.rx_ftr.scan_rsp) {
				node_rx_scan_data = node_rx_curr;
				if (sec_phy_curr) {
					sec_phy_scan = sec_phy_curr;
				} else {
					sec_phy_scan = sec_phy;
				}
				scan_data_len = data_len_curr;
				scan_data = data_curr;
				scan_rsp_tx_pwr = tx_pwr_curr;
			}

			if (!adv_addr) {
				adv_addr_type = adv_addr_type_curr;
				adv_addr = adv_addr_curr;
			}

			if (!direct_addr) {
				direct_addr_type = direct_addr_type_curr;
				direct_addr = direct_addr_curr;
			}

			if (scan_data) {
				scan_data_len_total += data_len_curr;
			} else if (!data) {
				node_rx_data = node_rx_curr;
				data_len = data_len_curr;
				data_len_total = data_len;
				data = data_curr;
				tx_pwr = tx_pwr_curr;
			} else {
				data_len_total += data_len_curr;
			}

#if defined(CONFIG_BT_CTLR_PRIVACY)
			if (rl_idx >= ll_rl_size_get()) {
				rl_idx = rl_idx_curr;
			}
#endif /* CONFIG_BT_CTLR_PRIVACY */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC) && \
	defined(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)
			if (!devmatch) {
				devmatch = devmatch_curr;
			}
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC && CONFIG_BT_CTLR_FILTER_ACCEPT_LIST */
		}

		if (!node_rx_next) {
			bool has_aux_ptr = !!sec_phy_curr;

			if (scan_data) {
				if (has_aux_ptr) {
					scan_data_status =
				  BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
				}
			} else if (has_aux_ptr) {
				data_status =
				  BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
			}

			break;
		}

		node_rx_curr = node_rx_next;
		node_rx_next = node_rx_curr->hdr.rx_ftr.extra;
		adv = (void *)node_rx_curr->pdu;
	} while (1);

	if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC) &&
	    IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) &&
	    !devmatch) {
		node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);
		return;
	}

#if CONFIG_BT_CTLR_DUP_FILTER_LEN > 0
	if (adv_addr) {
		if (dup_scan &&
		    dup_found(PDU_ADV_TYPE_EXT_IND, adv_addr_type, adv_addr,
			      (evt_type & BIT_MASK(2)), adi, data_status)) {
			node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);
			return;
		}
	}
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 */

	/* If data incomplete */
	if (data_status) {
		/* Data incomplete and no more to come */
		if (!(adv_addr ||
		      (adi && ((tx_pwr != BT_HCI_LE_ADV_TX_POWER_NO_PREF) ||
			       data)))) {
			/* No device address and no valid AD data parsed or
			 * Tx Power present for this PDU chain that has ADI,
			 * skip HCI event generation.
			 * In other terms, generate HCI event if device address
			 * is present or if Tx pwr and/or data is present from
			 * anonymous device.
			 */
			node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);
			return;
		}
	}

	/* Restrict data length to maximum scan data length */
	if (data_len_total > CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX) {
		data_len_total = CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX;
		if (data_len > data_len_total) {
			data_len = data_len_total;
		}

		data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
	}

	/* Set directed advertising bit */
	if (direct_addr) {
		evt_type |= BT_HCI_LE_ADV_EVT_TYPE_DIRECT;
	}

	/* HCI fragment */
	evt_buf = buf;
	data_len_max = CONFIG_BT_BUF_EVT_RX_SIZE -
		       sizeof(struct bt_hci_evt_le_meta_event) -
		       sizeof(struct bt_hci_evt_le_ext_advertising_report) -
		       sizeof(struct bt_hci_evt_le_ext_advertising_info);

	/* If PDU data length less than total data length or PDU data length
	 * greater than maximum HCI event data length, then fragment.
	 */
	if ((data_len < data_len_total) || (data_len > data_len_max)) {
		ext_adv_data_frag(node_rx_data, evt_type, phy, &sec_phy,
				  adv_addr_type, adv_addr, direct_addr_type,
				  direct_addr, rl_idx, &tx_pwr, rssi,
				  interval_le16, adi, data_len_max,
				  data_len_total, &data_len, &data, buf,
				  &evt_buf);
	}

	/* Set data status bits */
	evt_type |= (data_status << 5);

	/* Start constructing the adv event for remainder of the PDU data */
	ext_adv_info_fill(evt_type, phy, sec_phy, adv_addr_type, adv_addr,
			  direct_addr_type, direct_addr, rl_idx, tx_pwr, rssi,
			  interval_le16, adi, data_len, data, evt_buf);

	/* If scan response event to be constructed */
	if (!scan_data) {
		node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);

		return;
	}

	/* Restrict scan response data length to maximum scan data length */
	if (scan_data_len_total > CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX) {
		scan_data_len_total = CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX;
		if (scan_data_len > scan_data_len_total) {
			scan_data_len = scan_data_len_total;
		}

		scan_data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
	}

	/* Set scan response bit */
	evt_type |= BT_HCI_LE_ADV_EVT_TYPE_SCAN_RSP;

	/* Clear the data status bits */
	evt_type &= ~(BIT_MASK(2) << 5);

	/* Allocate, append as buf fragement and construct the scan response
	 * event.
	 */
	evt_buf = bt_buf_get_rx(BT_BUF_EVT, BUF_GET_TIMEOUT);
	LL_ASSERT(evt_buf);

	net_buf_frag_add(buf, evt_buf);

	/* If PDU data length less than total data length or PDU data length
	 * greater than maximum HCI event data length, then fragment.
	 */
	if ((scan_data_len < scan_data_len_total) ||
	    (scan_data_len > data_len_max)) {
		ext_adv_data_frag(node_rx_scan_data, evt_type, phy,
				  &sec_phy_scan, adv_addr_type, adv_addr,
				  direct_addr_type, direct_addr, rl_idx,
				  &scan_rsp_tx_pwr, rssi, interval_le16, adi,
				  data_len_max, scan_data_len_total,
				  &scan_data_len, &scan_data, buf, &evt_buf);
	}

	/* set scan data status bits */
	evt_type |= (scan_data_status << 5);

	/* Start constructing the event for remainder of the PDU data */
	ext_adv_info_fill(evt_type, phy, sec_phy_scan, adv_addr_type, adv_addr,
			  direct_addr_type, direct_addr, rl_idx,
			  scan_rsp_tx_pwr, rssi, interval_le16, adi,
			  scan_data_len, scan_data, evt_buf);

	node_rx_extra_list_release(node_rx->hdr.rx_ftr.extra);
}

static void le_adv_ext_report(struct pdu_data *pdu_data,
			      struct node_rx_pdu *node_rx,
			      struct net_buf *buf, uint8_t phy)
{
	struct pdu_adv *adv = (void *)pdu_data;

	if ((adv->type == PDU_ADV_TYPE_EXT_IND) && adv->len) {
		le_ext_adv_report(pdu_data, node_rx, buf, phy);
	} else {
		le_ext_adv_legacy_report(pdu_data, node_rx, buf);
	}
}

static void le_adv_ext_1M_report(struct pdu_data *pdu_data,
				 struct node_rx_pdu *node_rx,
				 struct net_buf *buf)
{
	le_adv_ext_report(pdu_data, node_rx, buf, BT_HCI_LE_EXT_SCAN_PHY_1M);
}

static void le_adv_ext_2M_report(struct pdu_data *pdu_data,
				 struct node_rx_pdu *node_rx,
				 struct net_buf *buf)
{
	le_adv_ext_report(pdu_data, node_rx, buf, BT_HCI_LE_EXT_SCAN_PHY_2M);
}

static void le_adv_ext_coded_report(struct pdu_data *pdu_data,
				    struct node_rx_pdu *node_rx,
				    struct net_buf *buf)
{
	le_adv_ext_report(pdu_data, node_rx, buf, BT_HCI_LE_EXT_SCAN_PHY_CODED);
}

static void le_scan_timeout(struct pdu_data *pdu_data,
			    struct node_rx_pdu *node_rx, struct net_buf *buf)
{
	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_SCAN_TIMEOUT)) {
		return;
	}

	meta_evt(buf, BT_HCI_EVT_LE_SCAN_TIMEOUT, 0U);
}

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
static void le_per_adv_sync_established(struct pdu_data *pdu_data,
					struct node_rx_pdu *node_rx,
					struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_sync_established *sep;
	struct ll_scan_set *scan;
	struct node_rx_sync *se;
	void *node;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_PER_ADV_SYNC_ESTABLISHED)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED,
		       sizeof(*sep));

	/* Check for pdu field being aligned before accessing sync established
	 * event.
	 */
	node = pdu_data;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_sync));

	se = node;
	sep->status = se->status;

	if (se->status == BT_HCI_ERR_OP_CANCELLED_BY_HOST) {
		return;
	}

	scan = node_rx->hdr.rx_ftr.param;

#if (CONFIG_BT_CTLR_DUP_FILTER_LEN > 0) && \
	defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
	dup_periodic_adv_reset(scan->periodic.adv_addr_type,
			       scan->periodic.adv_addr,
			       scan->periodic.sid);
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 &&
	* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT
	*/

	sep->handle = sys_cpu_to_le16(node_rx->hdr.handle);

	/* Resolved address, if private, has been populated in ULL */
	sep->adv_addr.type = scan->periodic.adv_addr_type;
	(void)memcpy(sep->adv_addr.a.val, scan->periodic.adv_addr, BDADDR_SIZE);

	sep->sid = scan->periodic.sid;
	sep->phy = find_lsb_set(se->phy);
	sep->interval = sys_cpu_to_le16(se->interval);
	sep->clock_accuracy = se->sca;
}

static void le_per_adv_sync_report(struct pdu_data *pdu_data,
				   struct node_rx_pdu *node_rx,
				   struct net_buf *buf)
{
	struct node_rx_ftr *ftr = &node_rx->hdr.rx_ftr;
	int8_t tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
	struct pdu_adv *adv = (void *)pdu_data;
	struct pdu_adv_aux_ptr *aux_ptr = NULL;
	const struct pdu_adv_adi *adi = NULL;
	uint8_t cte_type = BT_HCI_LE_NO_CTE;
	const struct ll_sync_set *sync;
	struct pdu_adv_com_ext_adv *p;
	struct pdu_adv_ext_hdr *h;
	uint16_t data_len_total;
	struct net_buf *evt_buf;
	uint8_t data_len = 0U;
	uint8_t acad_len = 0U;
	uint8_t *data = NULL;
	uint8_t data_len_max;
	uint8_t *acad = NULL;
	uint8_t hdr_buf_len;
	uint8_t hdr_len;
	uint8_t *ptr;
	int8_t rssi;
	bool accept;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    (!(le_event_mask & BT_EVT_MASK_LE_PER_ADVERTISING_REPORT) &&
	     !(le_event_mask & BT_EVT_MASK_LE_BIGINFO_ADV_REPORT))) {
		return;
	}

	/* NOTE: The timeout_reload field in the sync context is checked under
	 *       race condition between HCI Tx and Rx thread wherein a sync
	 *       terminate was performed which resets the timeout_reload field
	 *       before releasing the sync context back into its memory pool.
	 *       It is important that timeout_reload field is at safe offset
	 *       inside the sync context such that it is not corrupt while being
	 *       in the memory pool.
	 *
	 *       This check ensures reports are not sent out after sync
	 *       terminate.
	 */
	sync = HDR_LLL2ULL(ftr->param);
	if (unlikely(!sync->timeout_reload)) {
		return;
	}

	if ((le_event_mask & BT_EVT_MASK_LE_PER_ADVERTISING_REPORT) &&
	    node_rx->hdr.rx_ftr.aux_failed) {
		struct bt_hci_evt_le_per_advertising_report *sep;

		sep = meta_evt(buf,
			       BT_HCI_EVT_LE_PER_ADVERTISING_REPORT,
			       sizeof(*sep));

		sep->handle = sys_cpu_to_le16(node_rx->hdr.handle);
		sep->tx_power = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
		sep->rssi = BT_HCI_LE_RSSI_NOT_AVAILABLE;
		sep->cte_type = BT_HCI_LE_NO_CTE;
		sep->data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
		sep->length = 0;

		return;
	}

	/* The Link Layer currently returns RSSI as an absolute value */
	rssi = -(node_rx->hdr.rx_ftr.rssi);

	LOG_DBG("len = %u, rssi = %d", adv->len, rssi);

	p = (void *)&adv->adv_ext_ind;
	h = (void *)p->ext_hdr_adv_data;
	ptr = (void *)h;

	LOG_DBG("    Per. adv mode= 0x%x, hdr len= %u", p->adv_mode, p->ext_hdr_len);

	if (!p->ext_hdr_len) {
		hdr_len = PDU_AC_EXT_HEADER_SIZE_MIN;

		goto no_ext_hdr;
	}

	ptr = h->data;

	if (h->adv_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->tgt_addr) {
		ptr += BDADDR_SIZE;
	}

	if (h->cte_info) {
		struct pdu_cte_info *cte_info;

		cte_info = (void *)ptr;
		cte_type = cte_info->type;
		ptr++;

		LOG_DBG("    CTE type= %d", cte_type);
	}

	if (h->adi) {
		adi = (void *)ptr;

		ptr += sizeof(struct pdu_adv_adi);
	}

	/* AuxPtr */
	if (h->aux_ptr) {
		uint8_t aux_phy;

		aux_ptr = (void *)ptr;
		if (PDU_ADV_AUX_PTR_PHY_GET(aux_ptr) > EXT_ADV_AUX_PHY_LE_CODED) {
			return;
		}

		ptr += sizeof(*aux_ptr);

		aux_phy = BIT(PDU_ADV_AUX_PTR_PHY_GET(aux_ptr));

		LOG_DBG("    AuxPtr chan_idx = %u, ca = %u, offs_units "
		       "= %u offs = 0x%x, phy = 0x%x",
		       aux_ptr->chan_idx, aux_ptr->ca,
		       aux_ptr->offs_units, PDU_ADV_AUX_PTR_OFFSET_GET(aux_ptr), aux_phy);
	}

	/* No SyncInfo */
	if (h->sync_info) {
		ptr += sizeof(struct pdu_adv_sync_info);
	}

	/* Tx Power */
	if (h->tx_pwr) {
		tx_pwr = *(int8_t *)ptr;
		ptr++;

		LOG_DBG("    Tx pwr= %d dB", tx_pwr);
	}

	hdr_len = ptr - (uint8_t *)p;
	hdr_buf_len = PDU_AC_EXT_HEADER_SIZE_MIN + p->ext_hdr_len;
	if (hdr_len > hdr_buf_len) {
		LOG_WRN("    Header length %u/%u, INVALID.", hdr_len, p->ext_hdr_len);
	} else {
		acad_len = hdr_buf_len - hdr_len;
		if (acad_len) {
			acad = ptr;

			ptr += acad_len;
			hdr_len += acad_len;
		}
	}

no_ext_hdr:
	if (hdr_len < adv->len) {
		data_len = adv->len - hdr_len;
		data = ptr;

		LOG_DBG("    AD Data (%u): <todo>", data_len);
	}

	if (0) {

#if (CONFIG_BT_CTLR_DUP_FILTER_LEN > 0) && \
	defined(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT)
	} else if (IS_ENABLED(CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT) &&
		   adi) {
		uint8_t data_status;

		data_status = (aux_ptr) ?
			      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL :
			      BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE;

		accept = sync->rx_enable && ftr->sync_rx_enabled &&
			 (!sync->nodups ||
			  !dup_found(PDU_ADV_TYPE_EXT_IND,
				     sync->peer_id_addr_type,
				     sync->peer_id_addr,
				     DUP_EXT_ADV_MODE_PERIODIC,
				     adi, data_status));
#endif /* CONFIG_BT_CTLR_DUP_FILTER_LEN > 0 &&
	* CONFIG_BT_CTLR_SYNC_PERIODIC_ADI_SUPPORT
	*/

	} else {
		accept = sync->rx_enable && ftr->sync_rx_enabled;
	}

	data_len_max = CONFIG_BT_BUF_EVT_RX_SIZE -
		       sizeof(struct bt_hci_evt_le_meta_event) -
		       sizeof(struct bt_hci_evt_le_per_advertising_report);
	data_len_total = node_rx->hdr.rx_ftr.aux_data_len;

	evt_buf = buf;

	if ((le_event_mask & BT_EVT_MASK_LE_PER_ADVERTISING_REPORT) && accept &&
	    ((data_len_total - data_len) < CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX)) {

		/* Pass verdict in LL.TS.p19 section 4.2.3.6 Extended Scanning,
		 * Passive, Periodic Advertising Report, RSSI and TX_Power
		 * states:
		 * TX_Power is set to value of the TxPower field for the
		 * AUX_SYNC_IND received, and RSSI set to a valid value.
		 * Subsequent reports with data and the status set to
		 * "Incomplete, more data to come" or "complete" can have the
		 * TX_Power field set to 0x7F.
		 *
		 * In the implementation data_len_total is the running total
		 * AD data length so far, data_len is the current PDU's AD data
		 * length. For AUX_SYNC_IND received, data_len_total ==
		 * data_len.
		 */
		if (data_len_total > data_len) {
			/* Subsequent reports */
			tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
		}

		data_len = MIN(data_len, (CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX +
					  data_len - data_len_total));

		do {
			struct bt_hci_evt_le_per_advertising_report *sep;
			uint8_t data_len_frag;
			uint8_t data_status;

			data_len_frag = MIN(data_len, data_len_max);

			/* Start constructing periodic advertising report */
			sep = meta_evt(evt_buf,
				       BT_HCI_EVT_LE_PER_ADVERTISING_REPORT,
				       sizeof(*sep) + data_len_frag);

			sep->handle = sys_cpu_to_le16(node_rx->hdr.handle);
			sep->tx_power = tx_pwr;
			sep->rssi = rssi;
			sep->cte_type = cte_type;
			sep->length = data_len_frag;
			memcpy(&sep->data[0], data, data_len_frag);

			data += data_len_frag;
			data_len -= data_len_frag;

			if (data_len > 0) {
				/* Some data left in PDU, mark as partial data. */
				data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL;

				evt_buf = bt_buf_get_rx(BT_BUF_EVT, BUF_GET_TIMEOUT);
				LL_ASSERT(evt_buf);

				net_buf_frag_add(buf, evt_buf);

				tx_pwr = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
			} else if (!aux_ptr &&
				   (data_len_total <= CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX)) {
				/* No data left, no AuxPtr, mark as complete data. */
				data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE;
			} else if (ftr->aux_sched &&
				   (data_len_total < CONFIG_BT_CTLR_SCAN_DATA_LEN_MAX)) {
				/* No data left, but have AuxPtr and scheduled aux scan,
				 * mark as partial data.
				 */
				data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL;
			} else {
				/* No data left, have AuxPtr but not aux scan scheduled,
				 * mark as incomplete data.
				 */
				data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE;
			}

			sep->data_status = data_status;
		} while (data_len > 0);

		evt_buf = NULL;
	}

	if ((le_event_mask & BT_EVT_MASK_LE_BIGINFO_ADV_REPORT) && acad &&
	    (acad_len >= (PDU_BIG_INFO_CLEARTEXT_SIZE +
			  PDU_ADV_DATA_HEADER_SIZE))) {
		struct bt_hci_evt_le_biginfo_adv_report *sep;
		struct pdu_big_info *bi;
		uint8_t bi_size;

		/* FIXME: Parse and find the BIGInfo */
		if (acad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] != BT_DATA_BIG_INFO) {
			return;
		}

		bi_size = acad[PDU_ADV_DATA_HEADER_LEN_OFFSET];
		bi = (void *)&acad[PDU_ADV_DATA_HEADER_DATA_OFFSET];

		/* Allocate new event buffer if periodic advertising report was
		 * constructed with the caller supplied buffer.
		 */
		if (!evt_buf) {
			evt_buf = bt_buf_get_rx(BT_BUF_EVT, BUF_GET_TIMEOUT);
			LL_ASSERT(evt_buf);

			net_buf_frag_add(buf, evt_buf);
		}

		/* Start constructing BIGInfo  advertising report */
		sep = meta_evt(evt_buf, BT_HCI_EVT_LE_BIGINFO_ADV_REPORT,
			       sizeof(*sep));

		sep->sync_handle = sys_cpu_to_le16(node_rx->hdr.handle);

		/* NOTE: both sep and bi struct store little-endian values,
		 *       explicit endian-ness conversion not required.
		 */
		sep->num_bis = bi->num_bis;
		sep->nse = bi->nse;
		sep->iso_interval = bi->iso_interval;
		sep->bn = bi->bn;
		sep->pto = bi->pto;
		sep->irc = bi->irc;
		sep->max_pdu = bi->max_pdu;
		sys_put_le24(sys_le24_to_cpu(bi->sdu_interval),
			     sep->sdu_interval);
		sep->max_sdu = bi->max_sdu;
		sep->phy = HCI_AUX_PHY_TO_HCI_PHY(bi->chm_phy[4] >> 5);
		sep->framing = (bi->payload_count_framing[4] >> 7) & 0x01;
		if (bi_size == (PDU_BIG_INFO_ENCRYPTED_SIZE + 1)) {
			sep->encryption = 1U;
		} else {
			sep->encryption = 0U;
		}
	}
}

static void le_per_adv_sync_lost(struct pdu_data *pdu_data,
				 struct node_rx_pdu *node_rx,
				 struct net_buf *buf)
{
	struct bt_hci_evt_le_per_adv_sync_lost *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_PER_ADV_SYNC_LOST)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_PER_ADV_SYNC_LOST, sizeof(*sep));
	sep->handle = sys_cpu_to_le16(node_rx->hdr.handle);
}

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
static void le_big_sync_established(struct pdu_data *pdu,
				    struct node_rx_pdu *node_rx,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_established *sep;
	struct ll_sync_iso_set *sync_iso;
	struct node_rx_sync_iso *se;
	struct lll_sync_iso *lll;
	size_t evt_size;
	void *node;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_BIG_SYNC_ESTABLISHED)) {
		return;
	}

	sync_iso = node_rx->hdr.rx_ftr.param;
	lll = &sync_iso->lll;

	evt_size = sizeof(*sep) + (lll->num_bis * sizeof(uint16_t));

	sep = meta_evt(buf, BT_HCI_EVT_LE_BIG_SYNC_ESTABLISHED, evt_size);
	sep->big_handle = sys_cpu_to_le16(node_rx->hdr.handle);

	/* Check for pdu field being aligned before accessing ISO sync
	 * established event.
	 */
	node = pdu;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_sync_iso));

	se = node;
	sep->status = se->status;
	if (sep->status) {
		return;
	}

	/* FIXME: Fill latency */
	sys_put_le24(0, sep->latency);

	sep->nse = lll->nse;
	sep->bn = lll->bn;
	sep->pto = lll->pto;
	sep->irc = lll->irc;
	sep->max_pdu = sys_cpu_to_le16(lll->max_pdu);
	sep->iso_interval = sys_cpu_to_le16(lll->iso_interval);
	sep->num_bis = lll->stream_count;

	/* Connection handle list of all BISes synchronized in the BIG */
	for (uint8_t i = 0U; i < lll->stream_count; i++) {
		uint16_t handle;

		handle = LL_BIS_SYNC_HANDLE_FROM_IDX(lll->stream_handle[i]);
		sep->handle[i] = sys_cpu_to_le16(handle);
	}
}

static void le_big_sync_lost(struct pdu_data *pdu,
			     struct node_rx_pdu *node_rx,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_big_sync_lost *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_BIG_SYNC_LOST)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_BIG_SYNC_LOST, sizeof(*sep));
	sep->big_handle = sys_cpu_to_le16(node_rx->hdr.handle);
	sep->reason = *((uint8_t *)pdu);
}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
static void le_adv_ext_terminate(struct pdu_data *pdu_data,
				    struct node_rx_pdu *node_rx,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_adv_set_terminated *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_ADV_SET_TERMINATED)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_ADV_SET_TERMINATED, sizeof(*sep));
	sep->status = node_rx->hdr.rx_ftr.param_adv_term.status;
	sep->adv_handle = ll_adv_set_hci_handle_get(node_rx->hdr.handle & 0xff);
	sep->conn_handle =
		sys_cpu_to_le16(node_rx->hdr.rx_ftr.param_adv_term.conn_handle);
	sep->num_completed_ext_adv_evts =
		node_rx->hdr.rx_ftr.param_adv_term.num_events;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO)
static void le_big_complete(struct pdu_data *pdu_data,
			    struct node_rx_pdu *node_rx,
			    struct net_buf *buf)
{
	struct bt_hci_evt_le_big_complete *sep;
	struct ll_adv_iso_set *adv_iso;
	struct lll_adv_iso *lll;
	size_t evt_size;

	adv_iso = node_rx->hdr.rx_ftr.param;
	lll = &adv_iso->lll;

	evt_size = sizeof(*sep) + (lll->num_bis * sizeof(uint16_t));

	sep = meta_evt(buf, BT_HCI_EVT_LE_BIG_COMPLETE, evt_size);

	sep->status = BT_HCI_ERR_SUCCESS;
	sep->big_handle = sys_cpu_to_le16(node_rx->hdr.handle);

	if (sep->status) {
		return;
	}

	/* FIXME: Fill sync delay and latency */
	sys_put_le24(0, sep->sync_delay);
	sys_put_le24(0, sep->latency);

	sep->phy = find_lsb_set(lll->phy);
	sep->nse = lll->nse;
	sep->bn = lll->bn;
	sep->pto = lll->pto;
	sep->irc = lll->irc;
	sep->max_pdu = sys_cpu_to_le16(lll->max_pdu);
	sep->num_bis = lll->num_bis;

	/* Connection handle list of all BISes in the BIG */
	for (uint8_t i = 0U; i < lll->num_bis; i++) {
		uint16_t handle;

		handle = LL_BIS_ADV_HANDLE_FROM_IDX(lll->stream_handle[i]);
		sep->handle[i] = sys_cpu_to_le16(handle);
	}
}

static void le_big_terminate(struct pdu_data *pdu,
			     struct node_rx_pdu *node_rx,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_big_terminate *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_BIG_TERMINATED)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_BIG_TERMINATE, sizeof(*sep));
	sep->big_handle = sys_cpu_to_le16(node_rx->hdr.handle);
	sep->reason = *((uint8_t *)pdu);
}
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
static void le_scan_req_received(struct pdu_data *pdu_data,
				 struct node_rx_pdu *node_rx,
				 struct net_buf *buf)
{
	struct pdu_adv *adv = (void *)pdu_data;
	struct bt_hci_evt_le_scan_req_received *sep;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	uint8_t rl_idx;
#endif

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_SCAN_REQ_RECEIVED)) {
		bt_addr_le_t addr;
		uint8_t handle;
		int8_t rssi;

		handle = ll_adv_set_hci_handle_get(node_rx->hdr.handle & 0xff);
		addr.type = adv->tx_addr;
		memcpy(&addr.a.val[0], &adv->scan_req.scan_addr[0],
		       sizeof(bt_addr_t));

		/* The Link Layer currently returns RSSI as an absolute value */
		rssi = -(node_rx->hdr.rx_ftr.rssi);

		LOG_DBG("handle: %d, addr: %s, rssi: %d dB.", handle, bt_addr_le_str(&addr), rssi);

		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_SCAN_REQ_RECEIVED, sizeof(*sep));
	sep->handle = ll_adv_set_hci_handle_get(node_rx->hdr.handle & 0xff);
	sep->addr.type = adv->tx_addr;
	memcpy(&sep->addr.a.val[0], &adv->scan_req.scan_addr[0],
	       sizeof(bt_addr_t));

#if defined(CONFIG_BT_CTLR_PRIVACY)
	rl_idx = node_rx->hdr.rx_ftr.rl_idx;
	if (rl_idx < ll_rl_size_get()) {
		/* Store identity address */
		ll_rl_id_addr_get(rl_idx, &sep->addr.type,
				  &sep->addr.a.val[0]);
		/* Mark it as identity address from RPA (0x02, 0x03) */
		sep->addr.type += 2U;
	} else {
#else
	if (1) {
#endif
		sep->addr.type = adv->tx_addr;
		memcpy(&sep->addr.a.val[0], &adv->adv_ind.addr[0],
		       sizeof(bt_addr_t));
	}
}
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
static void le_conn_complete(struct pdu_data *pdu_data, uint16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *lecc;
	struct node_rx_cc *cc;
	uint8_t status;
	void *node;

	/* Check for pdu field being aligned before accessing connection
	 * complete event.
	 */
	node = pdu_data;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_cc));

	cc = node;
	status = cc->status;

#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (!status) {
		/* Update current RPA */
		ll_rl_crpa_set(cc->peer_addr_type,
			       &cc->peer_addr[0], 0xff,
			       &cc->peer_rpa[0]);
	}
#endif

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    (!(le_event_mask & BT_EVT_MASK_LE_CONN_COMPLETE) &&
#if defined(CONFIG_BT_CTLR_PRIVACY) || defined(CONFIG_BT_CTLR_ADV_EXT)
	     !(le_event_mask & BT_EVT_MASK_LE_ENH_CONN_COMPLETE))) {
#else
	     1)) {
#endif /* CONFIG_BT_CTLR_PRIVACY || CONFIG_BT_CTLR_ADV_EXT */
		return;
	}

	if (!status) {
		conn_count++;
	}

#if defined(CONFIG_BT_CTLR_PRIVACY) || defined(CONFIG_BT_CTLR_ADV_EXT)
	if (le_event_mask & BT_EVT_MASK_LE_ENH_CONN_COMPLETE) {
		struct bt_hci_evt_le_enh_conn_complete *leecc;

		leecc = meta_evt(buf, BT_HCI_EVT_LE_ENH_CONN_COMPLETE,
				 sizeof(*leecc));

		if (status) {
			(void)memset(leecc, 0x00, sizeof(*leecc));
			leecc->status = status;
			return;
		}

		leecc->status = 0x00;
		leecc->handle = sys_cpu_to_le16(handle);
		leecc->role = cc->role;

		leecc->peer_addr.type = cc->peer_addr_type;
		memcpy(&leecc->peer_addr.a.val[0], &cc->peer_addr[0],
		       BDADDR_SIZE);

#if defined(CONFIG_BT_CTLR_PRIVACY)
		memcpy(&leecc->local_rpa.val[0], &cc->local_rpa[0],
		       BDADDR_SIZE);
		memcpy(&leecc->peer_rpa.val[0], &cc->peer_rpa[0],
		       BDADDR_SIZE);
#else /* !CONFIG_BT_CTLR_PRIVACY */
		memset(&leecc->local_rpa.val[0], 0, BDADDR_SIZE);
		memset(&leecc->peer_rpa.val[0], 0, BDADDR_SIZE);
#endif /* !CONFIG_BT_CTLR_PRIVACY */

		leecc->interval = sys_cpu_to_le16(cc->interval);
		leecc->latency = sys_cpu_to_le16(cc->latency);
		leecc->supv_timeout = sys_cpu_to_le16(cc->timeout);
		leecc->clock_accuracy = cc->sca;
		return;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY || CONFIG_BT_CTLR_ADV_EXT */

	lecc = meta_evt(buf, BT_HCI_EVT_LE_CONN_COMPLETE, sizeof(*lecc));

	if (status) {
		(void)memset(lecc, 0x00, sizeof(*lecc));
		lecc->status = status;
		return;
	}

	lecc->status = 0x00;
	lecc->handle = sys_cpu_to_le16(handle);
	lecc->role = cc->role;
	lecc->peer_addr.type = cc->peer_addr_type & 0x1;
	memcpy(&lecc->peer_addr.a.val[0], &cc->peer_addr[0], BDADDR_SIZE);
	lecc->interval = sys_cpu_to_le16(cc->interval);
	lecc->latency = sys_cpu_to_le16(cc->latency);
	lecc->supv_timeout = sys_cpu_to_le16(cc->timeout);
	lecc->clock_accuracy = cc->sca;
}

void hci_disconn_complete_encode(struct pdu_data *pdu_data, uint16_t handle,
				 struct net_buf *buf)
{
	struct bt_hci_evt_disconn_complete *ep;

	if (!(event_mask & BT_EVT_MASK_DISCONN_COMPLETE)) {
		return;
	}

	hci_evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->reason = *((uint8_t *)pdu_data);
}

void hci_disconn_complete_process(uint16_t handle)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	/* Clear any pending packets upon disconnection */
	/* Note: This requires linear handle values starting from 0 */
	if (handle >= ARRAY_SIZE(hci_hbuf_pend)) {
		return;
	}

	hci_hbuf_acked += hci_hbuf_pend[handle];
	hci_hbuf_pend[handle] = 0U;
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

	conn_count--;
}

static void le_conn_update_complete(struct pdu_data *pdu_data, uint16_t handle,
				    struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_update_complete *sep;
	struct node_rx_cu *cu;
	void *node;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONN_UPDATE_COMPLETE)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE, sizeof(*sep));

	/* Check for pdu field being aligned before accessing connection
	 * update complete event.
	 */
	node = pdu_data;
	LL_ASSERT(IS_PTR_ALIGNED(node, struct node_rx_cu));

	cu = node;
	sep->status = cu->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->interval = sys_cpu_to_le16(cu->interval);
	sep->latency = sys_cpu_to_le16(cu->latency);
	sep->supv_timeout = sys_cpu_to_le16(cu->timeout);
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void enc_refresh_complete(struct pdu_data *pdu_data, uint16_t handle,
				 struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_key_refresh_complete *ep;

	if (!(event_mask & BT_EVT_MASK_ENCRYPT_KEY_REFRESH_COMPLETE)) {
		return;
	}

	hci_evt_create(buf, BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE,
		       sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_LE_PING)
static void auth_payload_timeout_exp(struct pdu_data *pdu_data, uint16_t handle,
				     struct net_buf *buf)
{
	struct bt_hci_evt_auth_payload_timeout_exp *ep;

	if (!(event_mask_page_2 & BT_EVT_MASK_AUTH_PAYLOAD_TIMEOUT_EXP)) {
		return;
	}

	hci_evt_create(buf, BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->handle = sys_cpu_to_le16(handle);
}
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
static void le_chan_sel_algo(struct pdu_data *pdu_data, uint16_t handle,
			     struct net_buf *buf)
{
	struct bt_hci_evt_le_chan_sel_algo *sep;
	struct node_rx_cs *cs;

	cs = (void *)pdu_data;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CHAN_SEL_ALGO)) {
		LOG_DBG("handle: 0x%04x, CSA: %x.", handle, cs->csa);
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CHAN_SEL_ALGO, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->chan_sel_algo = cs->csa;
}
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CTLR_PHY)
static void le_phy_upd_complete(struct pdu_data *pdu_data, uint16_t handle,
				struct net_buf *buf)
{
	struct bt_hci_evt_le_phy_update_complete *sep;
	struct node_rx_pu *pu;

	pu = (void *)pdu_data;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_PHY_UPDATE_COMPLETE)) {
		LOG_WRN("handle: 0x%04x, status: %x, tx: %x, rx: %x.", handle, pu->status,
			find_lsb_set(pu->tx), find_lsb_set(pu->rx));
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_PHY_UPDATE_COMPLETE, sizeof(*sep));

	sep->status = pu->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->tx_phy = find_lsb_set(pu->tx);
	sep->rx_phy = find_lsb_set(pu->rx);
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
static void le_req_peer_sca_complete(struct pdu_data *pdu, uint16_t handle,
				struct net_buf *buf)
{
	struct bt_hci_evt_le_req_peer_sca_complete *sep;
	struct node_rx_sca *scau;

	scau = (void *)pdu;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_REQ_PEER_SCA_COMPLETE)) {
		LOG_WRN("handle: 0x%04x, status: %x, sca: %x.", handle,
			scau->status,
			scau->sca);
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_REQ_PEER_SCA_COMPLETE, sizeof(*sep));

	sep->status = scau->status;
	sep->handle = sys_cpu_to_le16(handle);
	sep->sca = scau->sca;
}
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_HCI_MESH_EXT)
static void mesh_adv_cplt(struct pdu_data *pdu_data,
			  struct node_rx_pdu *node_rx,
			  struct net_buf *buf)
{
	struct bt_hci_evt_mesh_adv_complete *mep;

	mep = mesh_evt(buf, BT_HCI_EVT_MESH_ADV_COMPLETE, sizeof(*mep));
	mep->adv_slot = ((uint8_t *)pdu_data)[0];
}
#endif /* CONFIG_BT_HCI_MESH_EXT */

/**
 * @brief Encode a control-PDU into an HCI buffer
 * @details Execution context: Host thread
 *
 * @param node_rx_pdu[in] RX node containing header and PDU
 * @param pdu_data[in]    PDU. Same as node_rx_pdu->pdu, but more convenient
 * @param net_buf[out]    Upwards-going HCI buffer to fill
 */
static void encode_control(struct node_rx_pdu *node_rx,
			   struct pdu_data *pdu_data, struct net_buf *buf)
{
	uint16_t handle;

	handle = node_rx->hdr.handle;

	switch (node_rx->hdr.type) {
#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:
		le_advertising_report(pdu_data, node_rx, buf);
		break;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
		le_adv_ext_1M_report(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_EXT_2M_REPORT:
		le_adv_ext_2M_report(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_EXT_CODED_REPORT:
		le_adv_ext_coded_report(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_EXT_SCAN_TERMINATE:
		le_scan_timeout(pdu_data, node_rx, buf);
		break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC:
		le_per_adv_sync_established(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_SYNC_REPORT:
		le_per_adv_sync_report(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_SYNC_LOST:
		le_per_adv_sync_lost(pdu_data, node_rx, buf);
		break;

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	case NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT:
#if defined(CONFIG_BT_CTLR_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES)
		vs_le_df_connectionless_iq_report(pdu_data, node_rx, buf);
#else
		le_df_connectionless_iq_report(pdu_data, node_rx, buf);
#endif /* CONFIG_BT_CTLR_DF_VS_CL_IQ_REPORT_16_BITS_IQ_SAMPLES */
		break;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	case NODE_RX_TYPE_SYNC_ISO:
		le_big_sync_established(pdu_data, node_rx, buf);
		break;

	case NODE_RX_TYPE_SYNC_ISO_LOST:
		le_big_sync_lost(pdu_data, node_rx, buf);
		break;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_ADV_TERMINATE:
		le_adv_ext_terminate(pdu_data, node_rx, buf);
		break;

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	case NODE_RX_TYPE_BIG_COMPLETE:
		le_big_complete(pdu_data, node_rx, buf);
		break;
	case NODE_RX_TYPE_BIG_TERMINATE:
		le_big_terminate(pdu_data, node_rx, buf);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
		le_scan_req_received(pdu_data, node_rx, buf);
		break;
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
		le_conn_complete(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_TERMINATE:
		hci_disconn_complete_encode(pdu_data, handle, buf);
		break;

	case NODE_RX_TYPE_CONN_UPDATE:
		le_conn_update_complete(pdu_data, handle, buf);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case NODE_RX_TYPE_ENC_REFRESH:
		enc_refresh_complete(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_LE_PING)
	case NODE_RX_TYPE_APTO:
		auth_payload_timeout_exp(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
	case NODE_RX_TYPE_CHAN_SEL_ALGO:
		le_chan_sel_algo(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CTLR_PHY)
	case NODE_RX_TYPE_PHY_UPDATE:
		le_phy_upd_complete(pdu_data, handle, buf);
		return;
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	case NODE_RX_TYPE_RSSI:
		LOG_INF("handle: 0x%04x, rssi: -%d dB.", handle, pdu_data->rssi);
		return;
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case NODE_RX_TYPE_CIS_REQUEST:
		le_cis_request(pdu_data, node_rx, buf);
		return;
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case NODE_RX_TYPE_CIS_ESTABLISHED:
		le_cis_established(pdu_data, node_rx, buf);
		return;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE:
		le_req_peer_sca_complete(pdu_data, handle, buf);
		return;
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	case NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT:
#if defined(CONFIG_BT_CTLR_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES)
		vs_le_df_connection_iq_report(node_rx, buf);
#else
		le_df_connection_iq_report(node_rx, buf);
#endif /* CONFIG_BT_CTLR_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES */
		return;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
		LOG_INF("Advertised.");
		return;
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	case NODE_RX_TYPE_SCAN_INDICATION:
		LOG_INF("Scanned.");
		return;
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
		LOG_INF("l: %u, %u, %u; t: %u, %u, %u; cpu: %u, %u, %u, %u.",
			pdu_data->profile.lcur, pdu_data->profile.lmin, pdu_data->profile.lmax,
			pdu_data->profile.cur, pdu_data->profile.min, pdu_data->profile.max,
			pdu_data->profile.radio, pdu_data->profile.lll, pdu_data->profile.ull_high,
			pdu_data->profile.ull_low);
		return;
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	case NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT:
		le_df_connectionless_iq_report(pdu_data, node_rx, buf);
		return;
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	case NODE_RX_TYPE_MESH_ADV_CPLT:
		mesh_adv_cplt(pdu_data, node_rx, buf);
		return;

	case NODE_RX_TYPE_MESH_REPORT:
		le_advertising_report(pdu_data, node_rx, buf);
		return;
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if CONFIG_BT_CTLR_USER_EVT_RANGE > 0
	case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END - 1:
		hci_user_ext_encode_control(node_rx, pdu_data, buf);
		return;
#endif /* CONFIG_BT_CTLR_USER_EVT_RANGE > 0 */

	default:
		LL_ASSERT(0);
		return;
	}
}

#if defined(CONFIG_BT_CTLR_LE_ENC)
static void le_ltk_request(struct pdu_data *pdu_data, uint16_t handle,
			   struct net_buf *buf)
{
	struct bt_hci_evt_le_ltk_request *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_LTK_REQUEST)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_LTK_REQUEST, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	memcpy(&sep->rand, pdu_data->llctrl.enc_req.rand, sizeof(uint64_t));
	memcpy(&sep->ediv, pdu_data->llctrl.enc_req.ediv, sizeof(uint16_t));
}

static void encrypt_change(uint8_t err, uint16_t handle,
			   struct net_buf *buf)
{
	struct bt_hci_evt_encrypt_change *ep;

	if (!(event_mask & BT_EVT_MASK_ENCRYPT_CHANGE)) {
		return;
	}

	hci_evt_create(buf, BT_HCI_EVT_ENCRYPT_CHANGE, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ep->status = err;
	ep->handle = sys_cpu_to_le16(handle);
	ep->encrypt = !err ? 1 : 0;
}
#endif /* CONFIG_BT_CTLR_LE_ENC */

static void le_remote_feat_complete(uint8_t status, struct pdu_data *pdu_data,
				    uint16_t handle, struct net_buf *buf)
{
	struct bt_hci_evt_le_remote_feat_complete *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_REMOTE_FEAT_COMPLETE)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_REMOTE_FEAT_COMPLETE, sizeof(*sep));

	sep->status = status;
	sep->handle = sys_cpu_to_le16(handle);
	if (!status) {
		memcpy(&sep->features[0],
		       &pdu_data->llctrl.feature_rsp.features[0],
		       sizeof(sep->features));
	} else {
		(void)memset(&sep->features[0], 0x00, sizeof(sep->features));
	}
}

static void le_unknown_rsp(struct pdu_data *pdu_data, uint16_t handle,
			   struct net_buf *buf)
{

	switch (pdu_data->llctrl.unknown_rsp.type) {
	case PDU_DATA_LLCTRL_TYPE_PER_INIT_FEAT_XCHG:
		le_remote_feat_complete(BT_HCI_ERR_UNSUPP_REMOTE_FEATURE,
					    NULL, handle, buf);
		break;
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PDU_DATA_LLCTRL_TYPE_CTE_REQ:
		le_df_cte_req_failed(BT_HCI_ERR_UNSUPP_REMOTE_FEATURE, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		LOG_WRN("type: 0x%02x",	pdu_data->llctrl.unknown_rsp.type);
		break;
	}
}

static void le_reject_ext_ind(struct pdu_data *pdu, uint16_t handle, struct net_buf *buf)
{
	switch (pdu->llctrl.reject_ext_ind.reject_opcode) {
#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PDU_DATA_LLCTRL_TYPE_CTE_REQ:
		le_df_cte_req_failed(pdu->llctrl.reject_ext_ind.error_code, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */
	default:
		LOG_WRN("reject opcode: 0x%02x", pdu->llctrl.reject_ext_ind.reject_opcode);
		break;
	}
}
#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
static void le_conn_param_req(struct pdu_data *pdu_data, uint16_t handle,
			      struct net_buf *buf)
{
	struct bt_hci_evt_le_conn_param_req *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_CONN_PARAM_REQ)) {
		/* event masked, reject the conn param req */
		ll_conn_update(handle, 2, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE, 0,
			       0, 0, 0, NULL);

		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_CONN_PARAM_REQ, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->interval_min = pdu_data->llctrl.conn_param_req.interval_min;
	sep->interval_max = pdu_data->llctrl.conn_param_req.interval_max;
	sep->latency = pdu_data->llctrl.conn_param_req.latency;
	sep->timeout = pdu_data->llctrl.conn_param_req.timeout;
}
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static void le_data_len_change(struct pdu_data *pdu_data, uint16_t handle,
			       struct net_buf *buf)
{
	struct bt_hci_evt_le_data_len_change *sep;

	if (!(event_mask & BT_EVT_MASK_LE_META_EVENT) ||
	    !(le_event_mask & BT_EVT_MASK_LE_DATA_LEN_CHANGE)) {
		return;
	}

	sep = meta_evt(buf, BT_HCI_EVT_LE_DATA_LEN_CHANGE, sizeof(*sep));

	sep->handle = sys_cpu_to_le16(handle);
	sep->max_tx_octets = pdu_data->llctrl.length_rsp.max_tx_octets;
	sep->max_tx_time = pdu_data->llctrl.length_rsp.max_tx_time;
	sep->max_rx_octets = pdu_data->llctrl.length_rsp.max_rx_octets;
	sep->max_rx_time = pdu_data->llctrl.length_rsp.max_rx_time;
}
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_REMOTE_VERSION)
static void remote_version_info_encode(struct pdu_data *pdu_data,
				       uint16_t handle, struct net_buf *buf)
{
	struct pdu_data_llctrl_version_ind *ver_ind;
	struct bt_hci_evt_remote_version_info *ep;

	if (!(event_mask & BT_EVT_MASK_REMOTE_VERSION_INFO)) {
		return;
	}

	hci_evt_create(buf, BT_HCI_EVT_REMOTE_VERSION_INFO, sizeof(*ep));
	ep = net_buf_add(buf, sizeof(*ep));

	ver_ind = &pdu_data->llctrl.version_ind;
	ep->status = 0x00;
	ep->handle = sys_cpu_to_le16(handle);
	ep->version = ver_ind->version_number;
	ep->manufacturer = ver_ind->company_id;
	ep->subversion = ver_ind->sub_version_number;
}
#endif /* CONFIG_BT_REMOTE_VERSION */

static void encode_data_ctrl(struct node_rx_pdu *node_rx,
			     struct pdu_data *pdu_data, struct net_buf *buf)
{
	uint16_t handle = node_rx->hdr.handle;

	switch (pdu_data->llctrl.opcode) {

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
		le_ltk_request(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
		encrypt_change(0x00, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_REMOTE_VERSION)
	case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
		remote_version_info_encode(pdu_data, handle, buf);
		break;
#endif /* defined(CONFIG_BT_REMOTE_VERSION) */

	case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
		le_remote_feat_complete(0x00, pdu_data, handle, buf);
		break;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
		encrypt_change(pdu_data->llctrl.reject_ind.error_code, handle,
			       buf);
		break;
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_PARAM_REQ)
	case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
		le_conn_param_req(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_CONN_PARAM_REQ */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
	case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
	case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
		le_data_len_change(pdu_data, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_REQ)
	case PDU_DATA_LLCTRL_TYPE_CTE_RSP:
		le_df_cte_req_failed(BT_HCI_CTE_REQ_STATUS_RSP_WITHOUT_CTE, handle, buf);
		break;
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_REQ */

	case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
		le_unknown_rsp(pdu_data, handle, buf);
		break;

	case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
		le_reject_ext_ind(pdu_data, handle, buf);
		break;

	default:
		LL_ASSERT(0);
		return;
	}
}

#if defined(CONFIG_BT_CONN)
void hci_acl_encode(struct node_rx_pdu *node_rx, struct net_buf *buf)
{
	struct pdu_data *pdu_data = (void *)node_rx->pdu;
	struct bt_hci_acl_hdr *acl;
	uint16_t handle_flags;
	uint16_t handle;
	uint8_t *data;

	handle = node_rx->hdr.handle;

	switch (pdu_data->ll_id) {
	case PDU_DATA_LLID_DATA_CONTINUE:
	case PDU_DATA_LLID_DATA_START:
		acl = (void *)net_buf_add(buf, sizeof(*acl));
		if (pdu_data->ll_id == PDU_DATA_LLID_DATA_START) {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_START);
		} else {
			handle_flags = bt_acl_handle_pack(handle, BT_ACL_CONT);
		}
		acl->handle = sys_cpu_to_le16(handle_flags);
		acl->len = sys_cpu_to_le16(pdu_data->len);
		data = (void *)net_buf_add(buf, pdu_data->len);
		memcpy(data, pdu_data->lldata, pdu_data->len);
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
		if (hci_hbuf_total > 0) {
			LL_ASSERT((hci_hbuf_sent - hci_hbuf_acked) <
				  hci_hbuf_total);
			hci_hbuf_sent++;
			/* Note: This requires linear handle values starting
			 * from 0
			 */
			LL_ASSERT(handle < ARRAY_SIZE(hci_hbuf_pend));
			hci_hbuf_pend[handle]++;
		}
#endif
		break;

	default:
		LL_ASSERT(0);
		break;
	}
}
#endif /* CONFIG_BT_CONN */

void hci_evt_encode(struct node_rx_pdu *node_rx, struct net_buf *buf)
{
	struct pdu_data *pdu_data = (void *)node_rx->pdu;

	if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {
		encode_control(node_rx, pdu_data, buf);
	} else if (IS_ENABLED(CONFIG_BT_CONN)) {
		encode_data_ctrl(node_rx, pdu_data, buf);
	}
}

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO) || \
	defined(CONFIG_BT_CTLR_CONN_ISO)
void hci_num_cmplt_encode(struct net_buf *buf, uint16_t handle, uint8_t num)
{
	struct bt_hci_evt_num_completed_packets *ep;
	struct bt_hci_handle_count *hc;
	uint8_t num_handles;
	uint8_t len;

	num_handles = 1U;

	len = (sizeof(*ep) + (sizeof(*hc) * num_handles));
	hci_evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, len);

	ep = net_buf_add(buf, len);
	ep->num_handles = num_handles;
	hc = &ep->h[0];
	hc->handle = sys_cpu_to_le16(handle);
	hc->count = sys_cpu_to_le16(num);
}
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

uint8_t hci_get_class(struct node_rx_pdu *node_rx)
{
#if defined(CONFIG_BT_CONN)
	struct pdu_data *pdu_data = (void *)node_rx->pdu;
#endif

	if (node_rx->hdr.type != NODE_RX_TYPE_DC_PDU) {

		switch (node_rx->hdr.type) {
#if defined(CONFIG_BT_OBSERVER) || \
	defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
	defined(CONFIG_BT_CTLR_ADV_INDICATION) || \
	defined(CONFIG_BT_CTLR_SCAN_INDICATION) || \
	defined(CONFIG_BT_CTLR_PROFILE_ISR)
#if defined(CONFIG_BT_OBSERVER)
		case NODE_RX_TYPE_REPORT:
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
		case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */
			return HCI_CLASS_EVT_DISCARDABLE;
#endif

#if defined(CONFIG_BT_HCI_MESH_EXT)
		case NODE_RX_TYPE_MESH_ADV_CPLT:
		case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
		case NODE_RX_TYPE_EXT_ADV_TERMINATE:

#if defined(CONFIG_BT_CTLR_ADV_ISO)
		case NODE_RX_TYPE_BIG_COMPLETE:
		case NODE_RX_TYPE_BIG_TERMINATE:
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_2M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
		case NODE_RX_TYPE_EXT_SCAN_TERMINATE:

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		case NODE_RX_TYPE_SYNC:
		case NODE_RX_TYPE_SYNC_REPORT:
		case NODE_RX_TYPE_SYNC_LOST:

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
		case NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		case NODE_RX_TYPE_SYNC_ISO:
		case NODE_RX_TYPE_SYNC_ISO_LOST:
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_OBSERVER */

			return HCI_CLASS_EVT_REQUIRED;
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONNECTION:

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
		case NODE_RX_TYPE_CIS_REQUEST:
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
		case NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE:
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
		case NODE_RX_TYPE_CIS_ESTABLISHED:
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
		case NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

			return HCI_CLASS_EVT_REQUIRED;

		case NODE_RX_TYPE_TERMINATE:
		case NODE_RX_TYPE_CONN_UPDATE:

#if defined(CONFIG_BT_CTLR_LE_ENC)
		case NODE_RX_TYPE_ENC_REFRESH:
#endif /* CONFIG_BT_CTLR_LE_ENC */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
		case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */

#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
		case NODE_RX_TYPE_CHAN_SEL_ALGO:
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */

#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

			return HCI_CLASS_EVT_CONNECTION;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_SYNC_ISO) || defined(CONFIG_BT_CTLR_CONN_ISO)
		case NODE_RX_TYPE_ISO_PDU:
			return HCI_CLASS_ISO_DATA;
#endif /* CONFIG_BT_CTLR_SYNC_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
		case NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT:
			return HCI_CLASS_EVT_REQUIRED;
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

#if CONFIG_BT_CTLR_USER_EVT_RANGE > 0
		case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END - 1:
			return hci_user_ext_get_class(node_rx);
#endif /* CONFIG_BT_CTLR_USER_EVT_RANGE > 0 */

		default:
			return HCI_CLASS_NONE;
		}

#if defined(CONFIG_BT_CONN)
	} else if (pdu_data->ll_id == PDU_DATA_LLID_CTRL) {
		return HCI_CLASS_EVT_LLCP;
	} else {
		return HCI_CLASS_ACL_DATA;
	}
#else
	} else {
		return HCI_CLASS_NONE;
	}
#endif
}

void hci_init(struct k_poll_signal *signal_host_buf)
{
#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	hbuf_signal = signal_host_buf;
#endif
	reset(NULL, NULL);
}
