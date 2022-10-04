/* main.c - Host long advertising receive */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

#include <errno.h>
#include <tc_util.h>
#include <ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host_test_app);

struct test_adv_report {
	uint8_t data[CONFIG_BT_EXT_SCAN_BUF_SIZE];
	uint8_t length;
	uint16_t evt_prop;
	bt_addr_le_t addr;
};

#define COMPLETE BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_COMPLETE << 5
#define MORE_TO_COME BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_PARTIAL << 5
#define TRUNCATED BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS_INCOMPLETE << 5

/* Command handler structure for cmd_handle(). */
struct cmd_handler {
	uint16_t opcode; /* HCI command opcode */
	uint8_t len; /* HCI command response length */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

/* Add event to net_buf. */
static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void le_meta_evt_create(struct bt_hci_evt_le_meta_event *evt, uint8_t subevent)
{
	evt->subevent = subevent;
}

static void adv_info_create(struct bt_hci_evt_le_ext_advertising_info *evt, uint16_t evt_type,
			    const bt_addr_le_t *const addr, uint8_t length)
{
	evt->evt_type = evt_type;
	bt_addr_le_copy(&evt->addr, addr);
	evt->prim_phy = 0;
	evt->sec_phy = 0;
	evt->sid = 0;
	evt->tx_power = 0;
	evt->rssi = 0;
	evt->interval = 0;
	bt_addr_le_copy(&evt->direct_addr, BT_ADDR_LE_NONE);
	evt->length = length;
}

/* Create a command complete event. */
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

/* Loop over handlers to try to handle the command given by opcode. */
static int cmd_handle_helper(uint16_t opcode, struct net_buf *cmd, struct net_buf **evt,
			     const struct cmd_handler *handlers, size_t num_handlers)
{
	for (size_t i = 0; i < num_handlers; i++) {
		const struct cmd_handler *handler = &handlers[i];

		if (handler->opcode != opcode) {
			continue;
		}

		if (handler->handler) {
			handler->handler(cmd, evt, handler->len, opcode);

			return 0;
		}
	}

	zassert_unreachable("opcode %X failed", opcode);

	return -EINVAL;
}

/* Lookup the command opcode and invoke handler. */
static int cmd_handle(struct net_buf *cmd, const struct cmd_handler *handlers, size_t num_handlers)
{
	struct net_buf *evt = NULL;
	struct bt_hci_evt_cc_status *ccst;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;
	int err;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	err = cmd_handle_helper(opcode, cmd, &evt, handlers, num_handlers);

	if (err == -EINVAL) {
		ccst = cmd_complete(&evt, sizeof(*ccst), opcode);
		ccst->status = BT_HCI_ERR_UNKNOWN_CMD;
	}

	if (evt) {
		bt_recv_prio(evt);
	}

	return err;
}

/* Generic command complete with success status. */
static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, len, opcode);

	/* Fill any event parameters with zero */
	(void)memset(ccst, 0, len);

	ccst->status = BT_HCI_ERR_SUCCESS;
}

/* Bogus handler for BT_HCI_OP_READ_LOCAL_FEATURES. */
static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(rp->features, 0xFF, sizeof(rp->features));
}

/* Bogus handler for BT_HCI_OP_READ_SUPPORTED_COMMANDS. */
static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
	rp->status = 0x00;
}

/* Bogus handler for BT_HCI_OP_LE_READ_LOCAL_FEATURES. */
static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(rp->features, 0xFF, sizeof(rp->features));
}

/* Bogus handler for BT_HCI_OP_LE_READ_SUPP_STATES. */
static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(&rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* Setup handlers needed for bt_enable to function. */
static const struct cmd_handler cmds[] = {
	{ BT_HCI_OP_READ_LOCAL_VERSION_INFO, sizeof(struct bt_hci_rp_read_local_version_info),
	  generic_success },
	{ BT_HCI_OP_READ_SUPPORTED_COMMANDS, sizeof(struct bt_hci_rp_read_supported_commands),
	  read_supported_commands },
	{ BT_HCI_OP_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_read_local_features),
	  read_local_features },
	{ BT_HCI_OP_READ_BD_ADDR, sizeof(struct bt_hci_rp_read_bd_addr), generic_success },
	{ BT_HCI_OP_SET_EVENT_MASK, sizeof(struct bt_hci_evt_cc_status), generic_success },
	{ BT_HCI_OP_LE_SET_EVENT_MASK, sizeof(struct bt_hci_evt_cc_status), generic_success },
	{ BT_HCI_OP_LE_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_le_read_local_features),
	  le_read_local_features },
	{ BT_HCI_OP_LE_READ_SUPP_STATES, sizeof(struct bt_hci_rp_le_read_supp_states),
	  le_read_supp_states },
	{ BT_HCI_OP_LE_RAND, sizeof(struct bt_hci_rp_le_rand), generic_success },
	{ BT_HCI_OP_LE_SET_RANDOM_ADDRESS, sizeof(struct bt_hci_cp_le_set_random_address),
	  generic_success },
	{ BT_HCI_OP_RESET, 0, generic_success },
};

/* HCI driver open. */
static int driver_open(void)
{
	return 0;
}

/*  HCI driver send.  */
static int driver_send(struct net_buf *buf)
{
	zassert_true(cmd_handle(buf, cmds, ARRAY_SIZE(cmds)) == 0, "Unknown HCI command");

	net_buf_unref(buf);

	return 0;
}

/* HCI driver structure. */
static const struct bt_hci_driver drv = {
	.name = "test",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = driver_open,
	.send = driver_send,
	.quirks = 0,
};

struct bt_recv_job_data {
	struct k_work work; /* Work item */
	struct k_sem *sync; /* Semaphore to synchronize with */
	struct net_buf *buf; /* Net buffer to be passed to bt_recv() */
} job_data[CONFIG_BT_BUF_EVT_RX_COUNT];

#define job(buf) (&job_data[net_buf_id(buf)])

/* Work item handler for bt_recv() jobs. */
static void bt_recv_job_cb(struct k_work *item)
{
	struct bt_recv_job_data *data = CONTAINER_OF(item, struct bt_recv_job_data, work);

	/* Send net buffer to host */
	bt_recv(data->buf);

	/* Wake up bt_recv_job_submit */
	k_sem_give(job(data->buf)->sync);
}

/* Prepare a job to call bt_recv() to be submitted to the system workqueue. */
static void bt_recv_job_submit(struct net_buf *buf)
{
	struct k_sem sync_sem;

	/* Store the net buffer to be passed to bt_recv */
	job(buf)->buf = buf;

	/* Initialize job work item/semaphore */
	k_work_init(&job(buf)->work, bt_recv_job_cb);
	k_sem_init(&sync_sem, 0, 1);
	job(buf)->sync = &sync_sem;

	/* Make sure the buffer stays around until the command completes */
	buf = net_buf_ref(buf);

	/* Submit the work item */
	k_work_submit(&job(buf)->work);

	/* Wait for bt_recv_job_cb to be done */
	k_sem_take(&sync_sem, K_FOREVER);

	net_buf_unref(buf);
}

/* Semaphore to test if the prop callback was called. */
static K_SEM_DEFINE(prop_cb_sem, 0, 1);

static void *adv_report_evt(struct net_buf *buf, uint8_t data_len, uint16_t evt_type,
			    const bt_addr_le_t *const addr)
{
	struct bt_hci_evt_le_meta_event *meta_evt;
	struct bt_hci_evt_le_ext_advertising_info *evt;

	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta_evt) + sizeof(*evt) + data_len + 1);
	meta_evt = net_buf_add(buf, sizeof(*meta_evt));
	le_meta_evt_create(meta_evt, BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT);
	net_buf_add_u8(buf, 1); /* Number of reports */
	evt = net_buf_add(buf, sizeof(*evt));
	adv_info_create(evt, evt_type, addr, data_len);

	return net_buf_add(buf, data_len);
}

/* Send a prop event report wit the given data. */
static void send_adv_report(const struct test_adv_report *report)
{
	LOG_DBG("Sending adv report");
	struct net_buf *buf;
	uint8_t *adv_data;

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	adv_data = adv_report_evt(buf, report->length, report->evt_prop, &report->addr);
	memcpy(adv_data, &report->data, report->length);

	/* Submit job */
	bt_recv_job_submit(buf);
}

static uint16_t get_expected_length(void)
{
	return ztest_get_return_value();
}

static uint8_t *get_expected_data(void)
{
	return ztest_get_return_value_ptr();
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	ARG_UNUSED(info);

	LOG_DBG("Received event with length %u", buf->len);

	const uint16_t expected_length = get_expected_length();
	const uint8_t *expected_data = get_expected_data();

	zassert_equal(buf->len, expected_length, "Lengths should be equal");
	zassert_mem_equal(buf->data, expected_data, buf->len, "Data should be equal");
}

static void scan_timeout_cb(void)
{
	zassert_unreachable("Timeout should not happen");
}

static void generate_sequence(uint8_t *dest, uint16_t len, uint8_t range_start, uint8_t range_end)
{
	uint16_t written = 0;
	uint8_t value = range_start;

	while (written < len) {
		*dest++ = value++;
		written++;
		if (value > range_end) {
			value = range_start;
		}
	}
}

ZTEST_SUITE(long_adv_rx_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(long_adv_rx_tests, test_host_long_adv_recv)
{
	/* Register the test HCI driver */
	bt_hci_driver_register(&drv);

	/* Go! Wait until Bluetooth initialization is done  */
	zassert_true((bt_enable(NULL) == 0), "bt_enable failed");

	static struct bt_le_scan_cb scan_callbacks = { .recv = scan_recv_cb,
						       .timeout = scan_timeout_cb };
	bt_le_scan_cb_register(&scan_callbacks);
	bt_addr_le_t addr_a;
	bt_addr_le_t addr_b;
	bt_addr_le_t addr_c;
	bt_addr_le_t addr_d;

	bt_addr_le_create_static(&addr_a);
	bt_addr_le_create_static(&addr_b);
	bt_addr_le_create_static(&addr_c);
	bt_addr_le_create_static(&addr_d);

	struct test_adv_report report_a_1 = { .length = 30, .evt_prop = MORE_TO_COME };
	struct test_adv_report report_a_2 = { .length = 30, .evt_prop = COMPLETE };

	bt_addr_le_copy(&report_a_1.addr, &addr_a);
	bt_addr_le_copy(&report_a_2.addr, &addr_a);

	struct test_adv_report report_b_1 = { .length = 30, .evt_prop = MORE_TO_COME };
	struct test_adv_report report_b_2 = { .length = 30, .evt_prop = COMPLETE };

	bt_addr_le_copy(&report_b_1.addr, &addr_b);
	bt_addr_le_copy(&report_b_2.addr, &addr_b);

	struct test_adv_report report_c = { .length = 30,
					    .evt_prop = COMPLETE | BT_HCI_LE_ADV_EVT_TYPE_LEGACY };

	bt_addr_le_copy(&report_c.addr, &addr_c);

	struct test_adv_report report_d = { .length = 30, .evt_prop = TRUNCATED };

	bt_addr_le_copy(&report_c.addr, &addr_c);

	struct test_adv_report report_a_combined = { .length = report_a_1.length +
							       report_a_2.length };

	struct test_adv_report report_a_1_repeated = { .length = CONFIG_BT_EXT_SCAN_BUF_SIZE };

	struct test_adv_report report_b_combined = { .length = report_b_1.length +
							       report_b_2.length };

	generate_sequence(report_a_combined.data, report_a_combined.length, 'A', 'Z');
	generate_sequence(report_b_combined.data, report_b_combined.length, 'a', 'z');
	generate_sequence(report_c.data, report_c.length, '0', '9');

	(void)memcpy(report_a_1.data, report_a_combined.data, report_a_1.length);
	(void)memcpy(report_a_2.data, &report_a_combined.data[report_a_1.length],
		     report_a_2.length);

	for (int i = 0; i < report_a_1_repeated.length; i += report_a_1.length) {
		memcpy(&report_a_1_repeated.data[i], report_a_1.data,
		       MIN(report_a_1.length, (report_a_1_repeated.length - i)));
	}

	(void)memcpy(report_b_1.data, report_b_combined.data, report_b_1.length);
	(void)memcpy(report_b_2.data, &report_b_combined.data[report_b_1.length],
		     report_b_2.length);

	/* Check that non-interleaved fragmented adv reports work */
	ztest_returns_value(get_expected_data, report_a_combined.data);
	ztest_returns_value(get_expected_length, report_a_combined.length); /* Expect a */
	ztest_returns_value(get_expected_data, report_b_combined.data);
	ztest_returns_value(get_expected_length, report_b_combined.length); /* Then b */
	send_adv_report(&report_a_1);
	send_adv_report(&report_a_2);
	send_adv_report(&report_b_1);
	send_adv_report(&report_b_2);

	/* Check that legacy adv reports interleaved with fragmented adv reports work */
	ztest_returns_value(get_expected_data, report_c.data);
	ztest_returns_value(get_expected_length, report_c.length); /* Expect c */
	ztest_returns_value(get_expected_data, report_a_combined.data);
	ztest_returns_value(get_expected_length, report_a_combined.length); /* Then a */
	send_adv_report(&report_a_1);
	send_adv_report(&report_c); /* Interleaved legacy adv report */
	send_adv_report(&report_a_2);

	/* Check that complete adv reports interleaved with fragmented adv reports work */
	ztest_returns_value(get_expected_data, report_b_2.data);
	ztest_returns_value(get_expected_length, report_b_2.length); /* Expect b */
	ztest_returns_value(get_expected_data, report_a_combined.data);
	ztest_returns_value(get_expected_length, report_a_combined.length); /* Then a */
	send_adv_report(&report_a_1);
	send_adv_report(&report_b_2); /* Interleaved short extended adv report */
	send_adv_report(&report_a_2);

	/* Check that fragmented adv reports from one peer are received,
	 * and that interleaved fragmented adv reports from other peers are discarded
	 */
	ztest_returns_value(get_expected_data, report_a_combined.data);
	ztest_returns_value(get_expected_length, report_a_combined.length); /* Expect a */
	ztest_returns_value(get_expected_data, report_b_2.data);
	ztest_returns_value(get_expected_length,
			    report_b_2.length); /* Then b, INCOMPLETE REPORT */
	send_adv_report(&report_a_1);
	send_adv_report(&report_b_1); /* Interleaved fragmented adv report, NOT SUPPORTED */
	send_adv_report(&report_a_2);
	send_adv_report(&report_b_2);

	/* Check that host discards the data if the controller keeps sending
	 * incomplete packets.
	 */
	for (int i = 0; i < (2 + (CONFIG_BT_EXT_SCAN_BUF_SIZE / report_a_1.length)); i++) {
		send_adv_report(&report_a_1);
	}
	send_adv_report(&report_a_2);

	/* Check that controller truncated reports do not generate events */
	send_adv_report(&report_d);

	/* Check that reports from a different advertiser works after truncation */
	ztest_returns_value(get_expected_data, report_b_combined.data);
	ztest_returns_value(get_expected_length, report_b_combined.length); /* Expect b */
	send_adv_report(&report_b_1);
	send_adv_report(&report_b_2);
}
