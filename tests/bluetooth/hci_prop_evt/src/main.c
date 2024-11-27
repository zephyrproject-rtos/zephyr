/* main.c - HCI prop event test */

/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

struct driver_data {
	bt_hci_recv_t recv;
};

/* HCI Proprietary vendor event */
const uint8_t hci_prop_evt_prefix[2] = { 0xAB, 0xBA };

struct hci_evt_prop {
	uint8_t  prefix[2];
} __packed;

struct hci_evt_prop_report {
	uint8_t  data_len;
	uint8_t  data[0];
} __packed;

/* Command handler structure for cmd_handle(). */
struct cmd_handler {
	uint16_t opcode; /* HCI command opcode */
	uint8_t len;     /* HCI command response length */
	void (*handler)(struct net_buf *buf, struct net_buf **evt,
			uint8_t len, uint16_t opcode);
};

/* Add event to net_buf. */
static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
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
static int cmd_handle_helper(uint16_t opcode, struct net_buf *cmd,
			     struct net_buf **evt,
			     const struct cmd_handler *handlers,
			     size_t num_handlers)
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

	return -EINVAL;
}

/* Lookup the command opcode and invoke handler. */
static int cmd_handle(const struct device *dev,
		      struct net_buf *cmd,
		      const struct cmd_handler *handlers,
		      size_t num_handlers)
{
	struct driver_data *drv = dev->data;
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
		drv->recv(dev, evt);
	}

	return err;
}

/* Generic command complete with success status. */
static void generic_success(struct net_buf *buf, struct net_buf **evt,
			    uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = cmd_complete(evt, len, opcode);

	/* Fill any event parameters with zero */
	(void)memset(ccst, 0, len);

	ccst->status = BT_HCI_ERR_SUCCESS;
}

/* Bogus handler for BT_HCI_OP_READ_LOCAL_FEATURES. */
static void read_local_features(struct net_buf *buf, struct net_buf **evt,
				uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(&rp->features[0], 0xFF, sizeof(rp->features));
}

/* Bogus handler for BT_HCI_OP_READ_SUPPORTED_COMMANDS. */
static void read_supported_commands(struct net_buf *buf, struct net_buf **evt,
				    uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(&rp->commands[0], 0xFF, sizeof(rp->commands));
	rp->status = 0x00;
}

/* Bogus handler for BT_HCI_OP_LE_READ_LOCAL_FEATURES. */
static void le_read_local_features(struct net_buf *buf, struct net_buf **evt,
				   uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(&rp->features[0], 0xFF, sizeof(rp->features));
}

/* Bogus handler for BT_HCI_OP_LE_READ_SUPP_STATES. */
static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt,
				uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = 0x00;
	(void)memset(&rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* Setup handlers needed for bt_enable to function. */
static const struct cmd_handler cmds[] = {
	{ BT_HCI_OP_READ_LOCAL_VERSION_INFO,
	  sizeof(struct bt_hci_rp_read_local_version_info),
	  generic_success },
	{ BT_HCI_OP_READ_SUPPORTED_COMMANDS,
	  sizeof(struct bt_hci_rp_read_supported_commands),
	  read_supported_commands },
	{ BT_HCI_OP_READ_LOCAL_FEATURES,
	  sizeof(struct bt_hci_rp_read_local_features),
	  read_local_features },
	{ BT_HCI_OP_READ_BD_ADDR,
	  sizeof(struct bt_hci_rp_read_bd_addr),
	  generic_success },
	{ BT_HCI_OP_SET_EVENT_MASK,
	  sizeof(struct bt_hci_evt_cc_status),
	  generic_success },
	{ BT_HCI_OP_LE_SET_EVENT_MASK,
	  sizeof(struct bt_hci_evt_cc_status),
	  generic_success },
	{ BT_HCI_OP_LE_READ_LOCAL_FEATURES,
	  sizeof(struct bt_hci_rp_le_read_local_features),
	  le_read_local_features },
	{ BT_HCI_OP_LE_READ_SUPP_STATES,
	  sizeof(struct bt_hci_rp_le_read_supp_states),
	  le_read_supp_states },
	{ BT_HCI_OP_LE_RAND,
	  sizeof(struct bt_hci_rp_le_rand),
	  generic_success },
	{ BT_HCI_OP_LE_SET_RANDOM_ADDRESS,
	  sizeof(struct bt_hci_cp_le_set_random_address),
	  generic_success },
	{ BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN,
	  sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
	  generic_success },
};

/* HCI driver open. */
static int driver_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct driver_data *drv = dev->data;

	drv->recv = recv;

	return 0;
}

/*  HCI driver send.  */
static int driver_send(const struct device *dev, struct net_buf *buf)
{
	zassert_true(cmd_handle(dev, buf, cmds, ARRAY_SIZE(cmds)) == 0,
		     "Unknown HCI command");

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst) \
	static struct driver_data driver_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

struct bt_recv_job_data {
	struct k_work work;  /* Work item */
	struct k_sem *sync;  /* Semaphore to synchronize with */
	struct net_buf *buf; /* Net buffer to be passed to bt_recv() */
} job_data[CONFIG_BT_BUF_EVT_RX_COUNT];

#define job(buf) (&job_data[net_buf_id(buf)])

/* Work item handler for bt_recv() jobs. */
static void bt_recv_job_cb(struct k_work *item)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct driver_data *drv = dev->data;
	struct bt_recv_job_data *data =
		CONTAINER_OF(item, struct bt_recv_job_data, work);
	struct k_sem *sync = job(data->buf)->sync;

	/* Send net buffer to host */
	drv->recv(dev, data->buf);
	data->buf = NULL;

	/* Wake up bt_recv_job_submit */
	k_sem_give(sync);
}

/* Prepare a job to call bt_recv() to be submitted to the system workqueue. */
static void bt_recv_job_submit(struct net_buf *buf)
{
	struct k_sem sync_sem;

	/* Store the net buffer to be passed to bt_recv */
	job(buf)->buf = net_buf_ref(buf);

	/* Initialize job work item/semaphore */
	k_work_init(&job(buf)->work, bt_recv_job_cb);
	k_sem_init(&sync_sem, 0, 1);
	job(buf)->sync = &sync_sem;

	/* Submit the work item */
	k_work_submit(&job(buf)->work);

	/* Wait for bt_recv_job_cb to be done */
	k_sem_take(&sync_sem, K_FOREVER);

	net_buf_unref(buf);
}

/* Semaphore to test if the prop callback was called. */
static K_SEM_DEFINE(prop_cb_sem, 0, 1);

/* Used to verify prop event data. */
static uint8_t *prop_cb_data;
static uint8_t prop_cb_data_len;

/* Prop callback. */
static bool prop_cb(struct net_buf_simple *buf)
{
	struct hci_evt_prop *pe;

	pe = net_buf_simple_pull_mem(buf, sizeof(*pe));

	if (memcmp(&pe->prefix[0], &hci_prop_evt_prefix[0],
		   ARRAY_SIZE(hci_prop_evt_prefix)) == 0) {
		struct hci_evt_prop_report *per;

		per = net_buf_simple_pull_mem(buf, sizeof(*per));

		uint8_t data_len = per->data_len;
		uint8_t *data = &per->data[0];

		/* Allocate memory for storing the data */
		prop_cb_data = k_malloc(data_len);
		zassert_not_null(prop_cb_data, "Cannot allocate memory");

		/* Copy data so it can be verified later */
		memcpy(prop_cb_data, data, data_len);
		prop_cb_data_len = data_len;

		/* Give control back to test */
		k_sem_give(&prop_cb_sem);

		return true;
	}

	return false;
}

/* Create a HCI Vendor Specific event to carry the prop event report. */
static void *prop_evt(struct net_buf *buf, uint8_t pelen)
{
	struct hci_evt_prop *pe;

	evt_create(buf, BT_HCI_EVT_VENDOR, sizeof(*pe) + pelen);
	pe = net_buf_add(buf, sizeof(*pe));
	memcpy(&pe->prefix[0], &hci_prop_evt_prefix[0],
	       ARRAY_SIZE(hci_prop_evt_prefix));

	return net_buf_add(buf, pelen);
}

/* Send a prop event report wit the given data. */
static void send_prop_report(uint8_t *data, uint8_t data_len)
{
	struct net_buf *buf;
	struct hci_evt_prop_report *per;

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	per = prop_evt(buf, sizeof(*per) + data_len);
	per->data_len = data_len;
	memcpy(&per->data[0], data, data_len);

	/* Submit job */
	bt_recv_job_submit(buf);
}

ZTEST_SUITE(test_hci_prop_evt, NULL, NULL, NULL, NULL, NULL);

/* Test. */
ZTEST(test_hci_prop_evt, test_hci_prop_evt_entry)
{
	/* Go! Wait until Bluetooth initialization is done  */
	zassert_true((bt_enable(NULL) == 0),
		     "bt_enable failed");

	/* Register the prop callback */
	bt_hci_register_vnd_evt_cb(prop_cb);

	/* Generate some data */
	uint8_t data_len = 10;
	uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	/* Send the prop event report */
	send_prop_report(&data[0], data_len);

	/* Wait for the prop callback to be called */
	zassert_true(k_sem_take(&prop_cb_sem, K_MSEC(100)) == 0,
		     "prop_cb was not called within timeout");

	/* Verify the data length */
	zassert_true(prop_cb_data_len == data_len,
		     "prop_cb_data_len invalid");

	/* Verify the data itself */
	zassert_true(memcmp(prop_cb_data, data, data_len) == 0,
		     "prop_cb_data invalid");

	/* Free the data memory */
	k_free(prop_cb_data);
}
