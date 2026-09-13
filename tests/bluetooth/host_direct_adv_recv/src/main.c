/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for the LE Directed Advertising Report handling in the host.
 *
 * The events are fed to the host through a fake HCI driver, so the LE event
 * mask, the meta event dispatch and the scanner state are all exercised, and
 * the reports are observed where an application would see them: in the scan
 * recv callback.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define TEST_RSSI (-42)

/* Advertiser address carried by the crafted events. */
static const bt_addr_le_t test_adv_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0xc6}},
};

/* A resolvable private address, i.e. the two most significant bits are 0b01. */
static const bt_addr_le_t test_target_rpa = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {.val = {0x9a, 0x3d, 0xe0, 0x55, 0xb1, 0x7c}},
};

/* LE features the fake Controller reports. Set before bt_enable(). */
static uint8_t test_le_features[8];

/* LE event mask the host asked the fake Controller for. */
static uint64_t captured_le_event_mask;

/* Scanning filter policy from the last scan parameter command. */
static uint8_t captured_filter_policy;

/* Captured state from the last scan recv callback invocation. */
struct captured_report {
	bt_addr_le_t addr;
	bt_addr_le_t direct_addr;
	bool has_direct_addr;
	uint8_t adv_type;
	uint16_t adv_props;
	int8_t rssi;
	uint8_t sid;
	uint16_t buf_len;
};

static struct captured_report last_report;
static unsigned int recv_call_count;

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	recv_call_count++;

	bt_addr_le_copy(&last_report.addr, info->addr);
	last_report.adv_type = info->adv_type;
	last_report.adv_props = info->adv_props;
	last_report.rssi = info->rssi;
	last_report.sid = info->sid;
	last_report.buf_len = buf->len;

	last_report.has_direct_addr = info->direct_addr != NULL;
	if (info->direct_addr != NULL) {
		bt_addr_le_copy(&last_report.direct_addr, info->direct_addr);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv_cb,
};

/*
 * Fake HCI driver
 */

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

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

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, len, opcode);
	(void)memset(ccst, 0, len);
	ccst->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(rp->features, 0xFF, sizeof(rp->features));
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
	rp->status = 0x00;
}

/* Report the LE features the running test asked for. */
static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(rp->features, 0, sizeof(rp->features));
	memcpy(rp->features, test_le_features, MIN(sizeof(rp->features), sizeof(test_le_features)));
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(&rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* Capture the LE event mask so that the test can check which events the host
 * allowed the Controller to generate.
 */
static void le_set_event_mask(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	struct bt_hci_cp_le_set_event_mask *cp = (void *)buf->data;

	captured_le_event_mask = sys_get_le64(cp->events);

	generic_success(buf, evt, len, opcode);
}

/* Capture the scanning filter policy the host requests, so that the test can check
 * which of the four policies each option combination maps to.
 */
static void le_set_scan_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	struct bt_hci_cp_le_set_scan_param *cp = (void *)buf->data;

	captured_filter_policy = cp->filter_policy;

	generic_success(buf, evt, len, opcode);
}

static void le_set_ext_scan_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				  uint16_t opcode)
{
	struct bt_hci_cp_le_set_ext_scan_param *cp = (void *)buf->data;

	captured_filter_policy = cp->filter_policy;

	generic_success(buf, evt, len, opcode);
}

static const struct cmd_handler cmds[] = {
	{BT_HCI_OP_READ_LOCAL_VERSION_INFO, sizeof(struct bt_hci_rp_read_local_version_info),
	 generic_success},
	{BT_HCI_OP_READ_SUPPORTED_COMMANDS, sizeof(struct bt_hci_rp_read_supported_commands),
	 read_supported_commands},
	{BT_HCI_OP_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_read_local_features),
	 read_local_features},
	{BT_HCI_OP_READ_BD_ADDR, sizeof(struct bt_hci_rp_read_bd_addr), generic_success},
	{BT_HCI_OP_SET_EVENT_MASK, sizeof(struct bt_hci_evt_cc_status), generic_success},
	{BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(struct bt_hci_evt_cc_status), le_set_event_mask},
	{BT_HCI_OP_LE_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_le_read_local_features),
	 le_read_local_features},
	{BT_HCI_OP_LE_READ_SUPP_STATES, sizeof(struct bt_hci_rp_le_read_supp_states),
	 le_read_supp_states},
	{BT_HCI_OP_LE_RAND, sizeof(struct bt_hci_rp_le_rand), generic_success},
	{BT_HCI_OP_LE_SET_RANDOM_ADDRESS, sizeof(struct bt_hci_cp_le_set_random_address),
	 generic_success},
	{BT_HCI_OP_LE_SET_SCAN_PARAM, sizeof(struct bt_hci_evt_cc_status), le_set_scan_param},
	{BT_HCI_OP_LE_SET_SCAN_ENABLE, sizeof(struct bt_hci_evt_cc_status), generic_success},
	{BT_HCI_OP_LE_SET_EXT_SCAN_PARAM, sizeof(struct bt_hci_evt_cc_status),
	 le_set_ext_scan_param},
	{BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE, sizeof(struct bt_hci_evt_cc_status), generic_success},
	{BT_HCI_OP_RESET, sizeof(struct bt_hci_evt_cc_status), generic_success},
};

static int cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_evt_cc_status *ccst;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;
	bool handled = false;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	for (size_t i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			handled = true;
			break;
		}
	}

	if (!handled) {
		ccst = cmd_complete(&evt, sizeof(*ccst), opcode);
		ccst->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	if (evt != NULL) {
		bt_hci_recv(dev, evt);
	}

	return 0;
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Needed so that the test can cycle bt_disable()/bt_enable() to change the LE
 * features the fake Controller reports.
 */
static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	(void)cmd_handle(dev, buf);
	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {};                                  \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT);

/*
 * Pushing events to the host
 */

struct recv_job_data {
	struct k_work work;
	struct k_sem *sync;
	struct net_buf *buf;
};

static struct recv_job_data job_data[CONFIG_BT_BUF_EVT_RX_COUNT];

#define job(buf) (&job_data[net_buf_id(buf)])

static void recv_job_cb(struct k_work *item)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct recv_job_data *data = CONTAINER_OF(item, struct recv_job_data, work);

	bt_hci_recv(dev, data->buf);
	k_sem_give(data->sync);
}

/* Push an event to the host from the system workqueue and wait until it has
 * been consumed.
 */
static void recv_job_submit(struct net_buf *buf)
{
	struct k_sem sync_sem;

	job(buf)->buf = buf;
	k_work_init(&job(buf)->work, recv_job_cb);
	k_sem_init(&sync_sem, 0, 1);
	job(buf)->sync = &sync_sem;

	buf = net_buf_ref(buf);
	k_work_submit(&job(buf)->work);
	k_sem_take(&sync_sem, K_FOREVER);
	net_buf_unref(buf);
}

static void direct_adv_info_create(struct bt_hci_evt_le_direct_adv_info *info,
				   const bt_addr_le_t *addr, const bt_addr_le_t *dir_addr,
				   int8_t rssi)
{
	info->evt_type = BT_HCI_ADV_DIRECT_IND;
	bt_addr_le_copy(&info->addr, addr);
	bt_addr_le_copy(&info->dir_addr, dir_addr);
	info->rssi = rssi;
}

/* Send an LE Directed Advertising Report event carrying @p num_reports report
 * entries, all with @p dir_addr as the target address. @p claimed_reports is
 * what the event says it carries, which lets a test build a truncated event.
 */
static void send_direct_adv_report(const bt_addr_le_t *dir_addr, uint8_t num_reports,
				   uint8_t claimed_reports)
{
	struct bt_hci_evt_le_meta_event *meta_evt;
	struct bt_hci_evt_le_direct_adv_info *info;
	struct net_buf *buf;
	uint8_t info_len = num_reports * sizeof(*info);

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta_evt) + 1 + info_len);
	meta_evt = net_buf_add(buf, sizeof(*meta_evt));
	meta_evt->subevent = BT_HCI_EVT_LE_DIRECT_ADV_REPORT;
	net_buf_add_u8(buf, claimed_reports);

	for (uint8_t i = 0; i < num_reports; i++) {
		info = net_buf_add(buf, sizeof(*info));
		direct_adv_info_create(info, &test_adv_addr, dir_addr, TEST_RSSI);
	}

	recv_job_submit(buf);
}

/* Send an ordinary LE Advertising Report event with no advertising data. */
static void send_adv_report(uint8_t evt_type)
{
	struct bt_hci_evt_le_meta_event *meta_evt;
	struct bt_hci_evt_le_advertising_info *info;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta_evt) + 1 + sizeof(*info) + 1);
	meta_evt = net_buf_add(buf, sizeof(*meta_evt));
	meta_evt->subevent = BT_HCI_EVT_LE_ADVERTISING_REPORT;
	net_buf_add_u8(buf, 1);
	info = net_buf_add(buf, sizeof(*info));
	info->evt_type = evt_type;
	bt_addr_le_copy(&info->addr, &test_adv_addr);
	info->length = 0;
	net_buf_add_u8(buf, (uint8_t)TEST_RSSI);

	recv_job_submit(buf);
}

/* Send an LE Extended Advertising Report event for a legacy directed
 * advertisement with the given target address.
 */
static void send_ext_directed_report(const bt_addr_le_t *dir_addr)
{
	struct bt_hci_evt_le_meta_event *meta_evt;
	struct bt_hci_evt_le_ext_advertising_info *info;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta_evt) + 1 + sizeof(*info));
	meta_evt = net_buf_add(buf, sizeof(*meta_evt));
	meta_evt->subevent = BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT;
	net_buf_add_u8(buf, 1);

	info = net_buf_add(buf, sizeof(*info));
	info->evt_type =
		sys_cpu_to_le16(BT_HCI_LE_ADV_EVT_TYPE_LEGACY | BT_HCI_LE_ADV_EVT_TYPE_CONN |
				BT_HCI_LE_ADV_EVT_TYPE_DIRECT);
	bt_addr_le_copy(&info->addr, &test_adv_addr);
	info->prim_phy = BT_HCI_LE_PHY_1M;
	info->sec_phy = 0;
	info->sid = BT_GAP_SID_INVALID;
	info->tx_power = BT_HCI_LE_ADV_TX_POWER_NO_PREF;
	info->rssi = TEST_RSSI;
	info->interval = 0;
	bt_addr_le_copy(&info->direct_addr, dir_addr);
	info->length = 0;

	recv_job_submit(buf);
}

static void start_scan(uint8_t options)
{
	struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = options,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	zassert_ok(bt_le_scan_start(&param, NULL), "Failed to start the scanner");
}

/* Start the scanner with the given options, check which filter policy the host asked the
 * Controller for, then stop it again so the next combination starts cleanly.
 */
static void check_filter_policy(uint8_t options, uint8_t expected)
{
	captured_filter_policy = 0xFF;

	start_scan(options);

	zassert_equal(captured_filter_policy, expected,
		      "Options 0x%02x requested filter policy 0x%02x, expected 0x%02x", options,
		      captured_filter_policy, expected);

	zassert_ok(bt_le_scan_stop(), "Failed to stop the scanner");
}

static void *suite_setup(void)
{
	/* Extended Scanner Filter Policies supported. */
	memset(test_le_features, 0, sizeof(test_le_features));
	test_le_features[0] = BIT(BT_LE_FEAT_BIT_EXT_SCAN);

	zassert_ok(bt_enable(NULL), "bt_enable failed");
	zassert_ok(bt_le_scan_cb_register(&scan_callbacks), "Failed to register scan callbacks");

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&last_report, 0, sizeof(last_report));
	recv_call_count = 0;
}

static void test_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)bt_le_scan_stop();
}

ZTEST_SUITE(bt_direct_adv_recv, NULL, suite_setup, test_before, test_after, NULL);

/* The host allows the Controller to generate the LE Directed Advertising Report
 * event when the Controller supports the Extended Scanner Filter Policies.
 */
ZTEST(bt_direct_adv_recv, test_event_mask_bit_set)
{
	zassert_true((captured_le_event_mask & BT_EVT_MASK_LE_DIRECT_ADV_REPORT) != 0,
		     "LE Directed Advertising Report not unmasked, mask 0x%016llx",
		     captured_le_event_mask);
}

/* Without Controller support for the Extended Scanner Filter Policies the host
 * neither unmasks the event nor lets the scanner be started with the option.
 */
ZTEST(bt_direct_adv_recv, test_no_controller_support)
{
	struct bt_le_scan_param param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_EXT_FILTER_POLICY,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	zassert_ok(bt_disable(), "bt_disable failed");

	test_le_features[0] &= ~BIT(BT_LE_FEAT_BIT_EXT_SCAN);
	captured_le_event_mask = 0;

	zassert_ok(bt_enable(NULL), "bt_enable failed");

	zassert_equal(captured_le_event_mask & BT_EVT_MASK_LE_DIRECT_ADV_REPORT, 0,
		      "LE Directed Advertising Report unmasked without Controller support");
	zassert_equal(bt_le_scan_start(&param, NULL), -ENOTSUP,
		      "Scanner started without Controller support");

	/* Restore the Controller support for the remaining tests. */
	zassert_ok(bt_disable(), "bt_disable failed");
	test_le_features[0] |= BIT(BT_LE_FEAT_BIT_EXT_SCAN);
	zassert_ok(bt_enable(NULL), "bt_enable failed");
	zassert_true((captured_le_event_mask & BT_EVT_MASK_LE_DIRECT_ADV_REPORT) != 0,
		     "Failed to restore Controller support");
}

/* A directed advertisement whose target address the Controller could not
 * resolve is reported to the application with the target address marked as
 * unresolved.
 */
ZTEST(bt_direct_adv_recv, test_unresolved_target_is_reported)
{
	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_direct_adv_report(&test_target_rpa, 1, 1);

	zassert_equal(recv_call_count, 1, "Expected exactly one report, got %u", recv_call_count);
	zassert_equal(last_report.adv_type, BT_GAP_ADV_TYPE_ADV_DIRECT_IND,
		      "Unexpected advertising type 0x%02x", last_report.adv_type);
	zassert_equal(last_report.adv_props, BT_GAP_ADV_PROP_CONNECTABLE | BT_GAP_ADV_PROP_DIRECTED,
		      "Unexpected advertising properties 0x%04x", last_report.adv_props);
	zassert_equal(last_report.rssi, TEST_RSSI, "Unexpected RSSI %d", last_report.rssi);
	zassert_equal(last_report.sid, BT_GAP_SID_INVALID, "Unexpected SID 0x%02x",
		      last_report.sid);
	zassert_equal(last_report.buf_len, 0, "Unexpected advertising data length %u",
		      last_report.buf_len);
	zassert_true(bt_addr_le_eq(&last_report.addr, &test_adv_addr),
		     "Unexpected advertiser address");

	zassert_true(last_report.has_direct_addr, "Target address was not reported");
	zassert_equal(last_report.direct_addr.type, BT_ADDR_LE_UNRESOLVED,
		      "Target address not marked as unresolved, type 0x%02x",
		      last_report.direct_addr.type);
	zassert_mem_equal(last_report.direct_addr.a.val, test_target_rpa.a.val, BT_ADDR_SIZE,
			  "Unexpected target address");
}

/* A target address the Controller did resolve identifies the local device, so
 * it stays subject to the privacy rule that drops directed reports while
 * scanning with a non-resolvable private address.
 */
ZTEST(bt_direct_adv_recv, test_resolved_target_follows_privacy_rules)
{
	bt_addr_le_t resolved_target = test_target_rpa;

	resolved_target.type = BT_ADDR_LE_RANDOM_ID;

	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_direct_adv_report(&resolved_target, 1, 1);

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && !IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY)) {
		zassert_equal(recv_call_count, 0,
			      "Directed report for the local identity was not dropped");
		return;
	}

	zassert_equal(recv_call_count, 1, "Expected exactly one report, got %u", recv_call_count);
	zassert_true(last_report.has_direct_addr, "Target address was not reported");
	zassert_equal(last_report.direct_addr.type, BT_ADDR_LE_RANDOM,
		      "Identity address not converted, type 0x%02x", last_report.direct_addr.type);
	zassert_mem_equal(last_report.direct_addr.a.val, test_target_rpa.a.val, BT_ADDR_SIZE,
			  "Unexpected target address");
}

/* Every report of an event carrying more than one is delivered. */
ZTEST(bt_direct_adv_recv, test_multiple_reports)
{
	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_direct_adv_report(&test_target_rpa, 3, 3);

	zassert_equal(recv_call_count, 3, "Expected three reports, got %u", recv_call_count);
}

/* An event whose num_reports field claims more reports than the payload holds is
 * parsed up to the entries that are actually present. Those complete entries are
 * delivered, and parsing stops where the payload runs out.
 */
ZTEST(bt_direct_adv_recv, test_truncated_event_delivers_complete_reports)
{
	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_direct_adv_report(&test_target_rpa, 1, 2);

	zassert_equal(recv_call_count, 1, "Expected only the complete report, got %u",
		      recv_call_count);
}

/* An event with no report payload at all is discarded. */
ZTEST(bt_direct_adv_recv, test_empty_event_is_not_reported)
{
	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_direct_adv_report(&test_target_rpa, 0, 1);

	zassert_equal(recv_call_count, 0, "Expected no report, got %u", recv_call_count);
}

/* Reports are discarded while the application is not scanning. */
ZTEST(bt_direct_adv_recv, test_not_reported_when_not_scanning)
{
	send_direct_adv_report(&test_target_rpa, 1, 1);

	zassert_equal(recv_call_count, 0, "Expected no report, got %u", recv_call_count);
}

/* An ordinary LE Advertising Report carries no target address, so direct_addr
 * stays NULL for it.
 */
ZTEST(bt_direct_adv_recv, test_legacy_adv_report_has_no_target_address)
{
	start_scan(BT_LE_SCAN_OPT_NONE);

	send_adv_report(BT_GAP_ADV_TYPE_ADV_IND);

	zassert_equal(recv_call_count, 1, "Expected exactly one report, got %u", recv_call_count);
	zassert_false(last_report.has_direct_addr, "Unexpected target address reported");
}

/* A directed advertisement whose target address the Controller resolved is reported
 * through the LE Advertising Report, which carries no target address, so it reaches the
 * application with BT_GAP_ADV_PROP_DIRECTED set and direct_addr NULL. Whether it is
 * delivered at all depends on the privacy rule in le_adv_recv().
 */
ZTEST(bt_direct_adv_recv, test_legacy_directed_report_has_no_target_address)
{
	start_scan(BT_LE_SCAN_OPT_NONE);

	send_adv_report(BT_GAP_ADV_TYPE_ADV_DIRECT_IND);

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && !IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY)) {
		zassert_equal(recv_call_count, 0,
			      "Directed report for the local identity was not dropped");
		return;
	}

	zassert_equal(recv_call_count, 1, "Expected exactly one report, got %u", recv_call_count);
	zassert_true((last_report.adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0U,
		     "Report not marked as directed, props 0x%04x", last_report.adv_props);
	zassert_false(last_report.has_direct_addr, "Unexpected target address reported");
}

/* The extended advertising report carries the target address directly. An
 * unresolved one must survive to the application unchanged: BT_ADDR_LE_UNRESOLVED
 * has the identity address bit set, so it must not be treated as an identity
 * address and converted.
 */
ZTEST(bt_direct_adv_recv, test_ext_report_unresolved_target)
{
	bt_addr_le_t unresolved_target = test_target_rpa;

	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		ztest_test_skip();
	}

	unresolved_target.type = BT_ADDR_LE_UNRESOLVED;

	start_scan(BT_LE_SCAN_OPT_EXT_FILTER_POLICY);

	send_ext_directed_report(&unresolved_target);

	zassert_equal(recv_call_count, 1, "Expected exactly one report, got %u", recv_call_count);
	zassert_true(last_report.has_direct_addr, "Target address was not reported");
	zassert_equal(last_report.direct_addr.type, BT_ADDR_LE_UNRESOLVED,
		      "Target address not reported as unresolved, type 0x%02x",
		      last_report.direct_addr.type);
	zassert_mem_equal(last_report.direct_addr.a.val, test_target_rpa.a.val, BT_ADDR_SIZE,
			  "Unexpected target address");
}

/* All four scanning filter policies are reachable, and each option combination maps to
 * the right one. Which command carries the policy depends on CONFIG_BT_EXT_ADV, so the
 * configurations of this suite cover the legacy and the extended scan parameter commands
 * between them.
 */
ZTEST(bt_direct_adv_recv, test_scan_filter_policy)
{
	check_filter_policy(BT_LE_SCAN_OPT_NONE, BT_HCI_LE_SCAN_FP_BASIC_NO_FILTER);
	check_filter_policy(BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST, BT_HCI_LE_SCAN_FP_BASIC_FILTER);
	check_filter_policy(BT_LE_SCAN_OPT_EXT_FILTER_POLICY, BT_HCI_LE_SCAN_FP_EXT_NO_FILTER);
	check_filter_policy(BT_LE_SCAN_OPT_EXT_FILTER_POLICY | BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST,
			    BT_HCI_LE_SCAN_FP_EXT_FILTER);
}
