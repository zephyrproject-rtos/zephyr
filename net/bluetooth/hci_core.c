/* hci_core.c - HCI core Bluetooth handling */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>

#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>

#include "hci_core.h"
#include "conn.h"

/* Stacks for the fibers */
#define RX_STACK_SIZE	1024
#define CMD_STACK_SIZE	256
static char rx_fiber_stack[RX_STACK_SIZE];
static char cmd_fiber_stack[CMD_STACK_SIZE];

/* Available (free) buffers queue */
#define NUM_BUFS		5
static struct bt_buf		buffers[NUM_BUFS];
static struct nano_fifo		free_bufs;

static struct bt_dev dev;

struct bt_buf *bt_buf_get_reserve(size_t reserve_head)
{
	struct bt_buf *buf;

	buf = nano_fifo_get(&free_bufs);
	if (!buf) {
		BT_ERR("Failed to get free buffer\n");
		return NULL;
	}

	buf->data = buf->buf + reserve_head;
	buf->len = 0;
	buf->sync = NULL;

	BT_DBG("buf %p reserve %u\n", buf, reserve_head);

	return buf;
}

struct bt_buf *bt_buf_get(void)
{
	return bt_buf_get_reserve(0);
}

void bt_buf_put(struct bt_buf *buf)
{
	BT_DBG("buf %p\n", buf);

	nano_fifo_put(&free_bufs, buf);
}

uint8_t *bt_buf_add(struct bt_buf *buf, size_t len)
{
	uint8_t *tail = buf->data + buf->len;
	buf->len += len;
	return tail;
}

uint8_t *bt_buf_push(struct bt_buf *buf, size_t len)
{
	buf->data -= len;
	buf->len += len;
	return buf->data;
}

uint8_t *bt_buf_pull(struct bt_buf *buf, size_t len)
{
	buf->len -= len;
	return buf->data += len;
}

size_t bt_buf_headroom(struct bt_buf *buf)
{
	return buf->data - buf->buf;
}

size_t bt_buf_tailroom(struct bt_buf *buf)
{
	return BT_BUF_MAX_DATA - bt_buf_headroom(buf) - buf->len;
}

static struct bt_buf *bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	struct bt_hci_cmd_hdr *hdr;
	struct bt_buf *buf;

	BT_DBG("opcode %x param_len %u\n", opcode, param_len);

	buf = bt_buf_get_reserve(dev.drv->head_reserve);
	if (!buf) {
		BT_ERR("Cannot get free buffer\n");
		return NULL;
	}

	BT_DBG("buf %p\n", buf);

	buf->type = BT_CMD;
	buf->opcode = opcode;
	buf->sync = NULL;

	hdr = (void *)bt_buf_add(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = param_len;

	return buf;
}

static int bt_hci_cmd_send(uint16_t opcode, struct bt_buf *buf)
{
	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf)
			return -ENOBUFS;
	}

	BT_DBG("opcode %x len %u\n", opcode, buf->len);

	nano_fifo_put(&dev.cmd_queue, buf);

	return 0;
}

static int bt_hci_cmd_send_sync(uint16_t opcode, struct bt_buf *buf)
{
	struct nano_sem sync_sem;

	if (!buf) {
		buf = bt_hci_cmd_create(opcode, 0);
		if (!buf)
			return -ENOBUFS;
	}

	BT_DBG("opcode %x len %u\n", opcode, buf->len);

	nano_sem_init(&sync_sem);
	buf->sync = &sync_sem;

	nano_fifo_put(&dev.cmd_queue, buf);

	nano_sem_take_wait(&sync_sem);

	return 0;
}

static void hci_acl(struct bt_buf *buf)
{
	struct bt_hci_acl_hdr *hdr = (void *)buf->data;
	uint16_t handle, len = sys_le16_to_cpu(hdr->len);
	struct bt_conn *conn;
	uint8_t flags;

	handle = sys_le16_to_cpu(hdr->handle);
	flags = (handle >> 12);
	handle = bt_acl_handle(handle);

	bt_buf_pull(buf, sizeof(*hdr));

	BT_DBG("handle %u len %u flags %u\n", handle, len, flags);

	if (buf->len != len) {
		BT_ERR("ACL data length mismatch (%u != %u)\n", buf->len, len);
		bt_buf_put(buf);
		return;
	}

	conn = bt_conn_lookup(handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u\n", handle);
		bt_buf_put(buf);
		return;
	}

	bt_conn_recv(conn, buf, flags);
}

/* HCI event processing */

static void hci_disconn_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_disconn_complete *evt = (void *)buf->data;
	uint16_t handle = bt_acl_handle(sys_le16_to_cpu(evt->handle));
	struct bt_conn *conn;

	BT_DBG("status %u handle %u reason %u\n", evt->status, handle,
	       evt->reason);

	conn = bt_conn_lookup(handle);
	if (!conn) {
		BT_ERR("Unable to look up conn with handle %u\n", handle);
		return;
	}

	bt_conn_del(conn);

	if (dev.adv_enable) {
		struct bt_buf *buf;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
		if (buf) {
			memcpy(bt_buf_add(buf, 1), &dev.adv_enable, 1);
			bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_ENABLE, buf);
		}
	}
}

static void hci_reset_complete(struct bt_buf *buf)
{
	uint8_t status = buf->data[0];

	BT_DBG("status %u\n", status);

	if (status)
		return;
}

static void hci_read_local_ver_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_local_version_info *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	if (rp->status)
		return;

	dev.hci_version = rp->hci_version;
	dev.hci_revision = sys_le16_to_cpu(rp->hci_revision);
	dev.manufacturer = sys_le16_to_cpu(rp->manufacturer);
}

static void hci_read_features_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	memcpy(dev.features, rp->features, sizeof(dev.features));
}

static void hci_read_buffer_size_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_buffer_size *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	if (rp->status)
		return;

	/* If LE-side has buffers we can ignore the BR/EDR values */
	if (dev.le_mtu)
		return;

	dev.le_mtu = sys_le16_to_cpu(rp->acl_max_len);
	dev.le_pkts = sys_le16_to_cpu(rp->acl_max_num);
}

static void hci_read_bdaddr_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_read_bd_addr *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	if (rp->status)
		return;

	memcpy(dev.bdaddr, rp->bdaddr, sizeof(dev.bdaddr));
}

static void hci_le_read_buffer_size_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_le_read_buffer_size *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	if (rp->status)
		return;

	dev.le_mtu = sys_le16_to_cpu(rp->le_max_len);
	dev.le_pkts = rp->le_max_num;
}

static void hci_le_read_features_complete(struct bt_buf *buf)
{
	struct bt_hci_rp_le_read_local_features *rp = (void *)buf->data;

	BT_DBG("status %u\n", rp->status);

	memcpy(dev.le_features, rp->features, sizeof(dev.le_features));
}

static void hci_cmd_done(uint16_t opcode)
{
	struct bt_buf *sent = dev.sent_cmd;

	if (dev.sent_cmd->opcode != opcode) {
		BT_ERR("Unexpected completion of opcode %x\n", opcode);
		return;
	}

	dev.sent_cmd = NULL;

	/* If the command was synchronous wake up bt_hci_cmd_send_sync() */
	if (sent->sync)
		nano_fiber_sem_give(sent->sync);

	bt_buf_put(sent);
}

static void hci_cmd_complete(struct bt_buf *buf)
{
	struct hci_evt_cmd_complete *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);

	BT_DBG("opcode %x\n", opcode);

	bt_buf_pull(buf, sizeof(*evt));

	switch (opcode) {
	case BT_HCI_OP_RESET:
		hci_reset_complete(buf);
		break;
	case BT_HCI_OP_READ_LOCAL_VERSION_INFO:
		hci_read_local_ver_complete(buf);
		break;
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		hci_read_features_complete(buf);
		break;
	case BT_HCI_OP_READ_BUFFER_SIZE:
		hci_read_buffer_size_complete(buf);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		hci_read_bdaddr_complete(buf);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		hci_le_read_buffer_size_complete(buf);
		break;
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		hci_le_read_features_complete(buf);
		break;
	default:
		BT_ERR("Unknown opcode %x\n", opcode);
		break;
	}

	hci_cmd_done(opcode);

	if (evt->ncmd && !dev.ncmd) {
		/* Allow next command to be sent */
		dev.ncmd = 1;
		nano_fiber_sem_give(&dev.ncmd_sem);
	}
}

static void hci_cmd_status(struct bt_buf *buf)
{
	struct bt_hci_evt_cmd_status *evt = (void *)buf->data;
	uint16_t opcode = sys_le16_to_cpu(evt->opcode);

	BT_DBG("opcode %x\n", opcode);

	bt_buf_pull(buf, sizeof(*evt));

	switch (opcode) {
	default:
		BT_ERR("Unknown opcode %x", opcode);
		break;
	}

	hci_cmd_done(opcode);

	if (evt->ncmd && !dev.ncmd) {
		/* Allow next command to be sent */
		dev.ncmd = 1;
		nano_fiber_sem_give(&dev.ncmd_sem);
	}
}

static void hci_num_completed_packets(struct bt_buf *buf)
{
	struct bt_hci_evt_num_completed_packets *evt = (void *)buf->data;
	uint16_t i, num_handles = sys_le16_to_cpu(evt->num_handles);

	BT_DBG("num_handles %u\n", num_handles);

	for (i = 0; i < num_handles; i++) {
		uint16_t handle, count;

		handle = sys_le16_to_cpu(evt->h[i].handle);
		count = sys_le16_to_cpu(evt->h[i].count);

		BT_DBG("handle %u count %u\n", handle, count);

		while (count--)
			nano_fiber_sem_give(&dev.le_pkts_sem);
	}
}

static void le_conn_complete(struct bt_buf *buf)
{
	struct bt_hci_evt_le_conn_complete *evt = (void *)buf->data;
	uint16_t handle = sys_le16_to_cpu(evt->handle);
	struct bt_conn *conn;

	BT_DBG("status %u handle %u role %u peer_type %u\n", evt->status,
	       handle, evt->role, evt->peer_addr_type);

	conn = bt_conn_add(&dev, handle);
	if (!conn) {
		BT_ERR("Unable to add new conn for handle %u\n", handle);
		return;
	}

	conn->le_conn_interval = sys_le16_to_cpu(evt->interval);
}

static void hci_le_meta_event(struct bt_buf *buf)
{
	struct bt_hci_evt_le_meta_event *evt = (void *)buf->data;

	bt_buf_pull(buf, sizeof(*evt));

	switch (evt->subevent) {
	case BT_HCI_EVT_LE_CONN_COMPLETE:
		le_conn_complete(buf);
		break;
	default:
		BT_DBG("Unhandled LE event %x\n", evt->subevent);
		break;
	}
}

static void hci_event(struct bt_buf *buf)
{
	struct bt_hci_evt_hdr *hdr = (void *)buf->data;

	BT_DBG("event %u\n", hdr->evt);

	bt_buf_pull(buf, sizeof(*hdr));

	switch (hdr->evt) {
	case BT_HCI_EVT_DISCONN_COMPLETE:
		hci_disconn_complete(buf);
		break;
	case BT_HCI_EVT_CMD_COMPLETE:
		hci_cmd_complete(buf);
		break;
	case BT_HCI_EVT_CMD_STATUS:
		hci_cmd_status(buf);
		break;
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
		hci_num_completed_packets(buf);
		break;
	case BT_HCI_EVT_LE_META_EVENT:
		hci_le_meta_event(buf);
		break;
	default:
		BT_ERR("Unknown event %u\n", hdr->evt);
		break;

	}

	bt_buf_put(buf);
}

static void hci_cmd_fiber(void)
{
	struct bt_driver *drv = dev.drv;

	BT_DBG("\n");

	while (1) {
		struct bt_buf *buf;

		/* Wait until ncmd > 0 */
		nano_fiber_sem_take_wait(&dev.ncmd_sem);

		/* Get next command - wait if necessary */
		buf = nano_fifo_get_wait(&dev.cmd_queue);
		dev.ncmd = 0;

		BT_DBG("Sending command (buf %p) to driver\n", buf);

		drv->send(buf);
		dev.sent_cmd = buf;
	}
}

static void hci_rx_fiber(void)
{
	struct bt_buf *buf;

	BT_DBG("\n");

	while (1) {
		buf = nano_fifo_get_wait(&dev.rx_queue);

		BT_DBG("buf %p type %u len %u\n", buf, buf->type, buf->len);

		switch (buf->type) {
			case BT_ACL:
				hci_acl(buf);
				break;
			case BT_EVT:
				hci_event(buf);
				break;
			default:
				BT_ERR("Unknown buf type %u\n", buf->type);
				return;
		}

	}
}

static int hci_init(void)
{
	struct bt_hci_cp_set_event_mask *ev;
	struct bt_buf *buf;
	uint8_t i;

	/* Send HCI_RESET */
	bt_hci_cmd_send(BT_HCI_OP_RESET, NULL);

	/* Read Local Supported Features */
	bt_hci_cmd_send(BT_HCI_OP_READ_LOCAL_FEATURES, NULL);

	/* Read Local Version Information */
	bt_hci_cmd_send(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL);

	/* Read Bluetooth Address */
	bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL);

	/* For now we only support LE capable controllers */
	if (!lmp_le_capable(dev)) {
		BT_ERR("Non-LE capable controller detected!\n");
		return -ENODEV;
	}

	/* Read Low Energy Supported Features */
	bt_hci_cmd_send(BT_HCI_OP_LE_READ_LOCAL_FEATURES, NULL);

	/* Read LE Buffer Size */
	bt_hci_cmd_send(BT_HCI_OP_LE_READ_BUFFER_SIZE, NULL);

	buf = bt_hci_cmd_create(BT_HCI_OP_SET_EVENT_MASK, 8);
	if (!buf)
		return -ENOBUFS;

	ev = (void *)bt_buf_add(buf, sizeof(*ev));
	memset(ev, 0, sizeof(*ev));
	ev->events[0] |= 0x10; /* Disconnection Complete */
	ev->events[1] |= 0x08; /* Read Remote Version Information Complete */
	ev->events[1] |= 0x20; /* Command Complete */
	ev->events[1] |= 0x40; /* Command Status */
	ev->events[1] |= 0x80; /* Hardware Error */
	ev->events[2] |= 0x04; /* Number of Completed Packets */
	ev->events[3] |= 0x02; /* Data Buffer Overflow */
	ev->events[7] |= 0x20; /* LE Meta-Event */

	if (dev.le_features[0] & BT_HCI_LE_ENCRYPTION) {
		ev->events[0] |= 0x80; /* Encryption Change */
		ev->events[5] |= 0x80; /* Encryption Key Refresh Complete */
	}

	bt_hci_cmd_send_sync(BT_HCI_OP_SET_EVENT_MASK, buf);

	if (lmp_bredr_capable(dev)) {
		struct bt_hci_cp_write_le_host_supp *cp;

		/* Use BR/EDR buffer size if LE reports zero buffers */
		if (!dev.le_mtu)
			bt_hci_cmd_send(BT_HCI_OP_READ_BUFFER_SIZE, NULL);


		buf = bt_hci_cmd_create(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP,
					sizeof(*cp));
		if (!buf)
			return -ENOBUFS;

		cp = (void *)bt_buf_add(buf, sizeof*cp);

		/* Excplicitly enable LE for dual-mode controllers */
		cp->le = 0x01;
		cp->simul = 0x00;
		bt_hci_cmd_send_sync(BT_HCI_OP_LE_WRITE_LE_HOST_SUPP, buf);
	}

	BT_DBG("HCI ver %u rev %u, manufacturer %u\n", dev.hci_version,
	       dev.hci_revision, dev.manufacturer);
	BT_DBG("ACL buffers: pkts %u mtu %u\n", dev.le_pkts, dev.le_mtu);

	/* Initialize & prime the semaphore for counting controller-side
	 * available ACL packet buffers.
	 */
	nano_sem_init(&dev.le_pkts_sem);
	for (i = 0; i < dev.le_pkts; i++)
		nano_sem_give(&dev.le_pkts_sem);

	return 0;
}

int bt_hci_reset(void)
{
	return hci_init();
}

/* Interface to HCI driver layer */

void bt_recv(struct bt_buf *buf)
{
	nano_fifo_put(&dev.rx_queue, buf);
}

int bt_driver_register(struct bt_driver *drv)
{
	if (dev.drv)
		return -EALREADY;

	if (!drv->open || !drv->send)
		return -EINVAL;

	dev.drv = drv;

	return 0;
}

void bt_driver_unregister(struct bt_driver *drv)
{
	dev.drv = NULL;
}

/* fibers, fifos and semaphores initialization */

static void cmd_queue_init(void)
{
	nano_fifo_init(&dev.cmd_queue);
	nano_sem_init(&dev.ncmd_sem);

	/* Give cmd_sem allowing to send first HCI_Reset cmd */
	dev.ncmd = 1;
	nano_task_sem_give(&dev.ncmd_sem);

	fiber_start(cmd_fiber_stack, CMD_STACK_SIZE,
		    (nano_fiber_entry_t) hci_cmd_fiber, 0, 0, 7, 0);
}

static void rx_queue_init(void)
{
	nano_fifo_init(&dev.rx_queue);

	fiber_start(rx_fiber_stack, RX_STACK_SIZE,
		    (nano_fiber_entry_t) hci_rx_fiber, 0, 0, 7, 0);
}

static void free_queue_init(void)
{
	nano_fifo_init(&free_bufs);

	for (int i = 0; i < NUM_BUFS; i++)
		nano_fifo_put(&free_bufs, &buffers[i]);
}

int bt_init(void)
{
	struct bt_driver *drv = dev.drv;
	int err;

	if (!drv)
		return -ENODEV;

	free_queue_init();
	cmd_queue_init();
	rx_queue_init();

	err = drv->open();
	if (err)
		return err;

	return hci_init();
}

int bt_start_advertising(uint8_t type, const char *name, uint8_t name_len)
{
	struct bt_buf *buf;
	struct bt_hci_cp_le_set_adv_data *set_data;
	struct bt_hci_cp_le_set_adv_data *scan_rsp;
	struct bt_hci_cp_le_set_adv_parameters *set_param;

	/* We don't support shortening of names, for now */
	if (name_len > 29)
		return -EINVAL;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_DATA, sizeof(*set_data));
	if (!buf)
		return -ENOBUFS;

	set_data = (void *)bt_buf_add(buf, sizeof(*set_data));

	memset(set_data, 0, sizeof(*set_data));

	/* Advertising flags */
	set_data->data[0] = 0x02; /* Length */
	set_data->data[1] = BT_EIR_FLAGS;
	set_data->data[2] = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

	set_data->len = 3;
	bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_DATA, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_SCAN_RSP_DATA,
				sizeof(*scan_rsp));
	if (!buf)
		return -ENOBUFS;

	scan_rsp = (void *)bt_buf_add(buf, sizeof(*scan_rsp));

	memset(scan_rsp, 0, sizeof(*scan_rsp));

	/* Friendly name */
	scan_rsp->data[0] = name_len + 1;
	scan_rsp->data[1] = BT_EIR_NAME_COMPLETE;
	memcpy(scan_rsp->data + 2, name, name_len);

	scan_rsp->len = name_len + 2;
	bt_hci_cmd_send(BT_HCI_OP_LE_SET_SCAN_RSP_DATA, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_PARAMETERS,
				sizeof(*set_param));
	if (!buf)
		return -ENOBUFS;

	set_param = (void *)bt_buf_add(buf, sizeof(*set_param));

	memset(set_param, 0, sizeof(*set_param));
	set_param->min_interval		= sys_cpu_to_le16(0x0800);
	set_param->max_interval		= sys_cpu_to_le16(0x0800);
	set_param->type			= type;
	set_param->channel_map		= 0x07;

	bt_hci_cmd_send(BT_HCI_OP_LE_SET_ADV_PARAMETERS, buf);

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_ENABLE, 1);
	if (!buf)
		return -ENOBUFS;

	dev.adv_enable = 0x01;
	memcpy(bt_buf_add(buf, 1), &dev.adv_enable, 1);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_ENABLE, buf);
}
