/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <arch/cpu.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <sys/util.h>
#include <drivers/ipm.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME hci_rpmsg
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Configuration defines */
#if !DT_HAS_CHOSEN(zephyr_ipc_shm)
#error "Sample requires definition of shared memory for rpmsg"
#endif

#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_BASE_ADDRESS    DT_REG_ADDR(SHM_NODE)
#define SHM_START_ADDR      (SHM_BASE_ADDRESS + 0x400)
#define SHM_SIZE            0x7c00
#define SHM_DEVICE_NAME     "sram0.shm"

BUILD_ASSERT((SHM_START_ADDR + SHM_SIZE - SHM_BASE_ADDRESS)
		<= DT_REG_SIZE(SHM_NODE),
	"Allocated size exceeds available shared memory reserved for IPC");

#define VRING_COUNT         2
#define VRING_TX_ADDRESS    (SHM_START_ADDR + SHM_SIZE - 0x400)
#define VRING_RX_ADDRESS    (VRING_TX_ADDRESS - 0x400)
#define VRING_ALIGNMENT     4
#define VRING_SIZE          16

#define VDEV_STATUS_ADDR    SHM_BASE_ADDRESS

/* End of configuration defines */

static const struct device *ipm_tx_handle;
static const struct device *ipm_rx_handle;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	.regions = {
		{
			.virt       = (void *) SHM_START_ADDR,
			.physmap    = shm_physmap,
			.size       = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask  = 0xffffffff,
			.mem_flags  = 0,
			.ops        = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static struct virtqueue *vq[2];
static struct rpmsg_endpoint ep;

static struct k_work ipm_work;

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return sys_read8(VDEV_STATUS_ADDR);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

static void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static void virtio_notify(struct virtqueue *vq)
{
	int status;

	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		LOG_ERR("ipm_send failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.set_status = virtio_set_status,
	.get_features = virtio_get_features,
	.notify = virtio_notify,
};

static void ipm_callback_process(struct k_work *work)
{
	virtqueue_notification(vq[1]);
}

static void ipm_callback(const struct device *dev, void *context,
			 uint32_t id, volatile void *data)
{
	LOG_INF("Got callback of id %u", id);
	k_work_submit(&ipm_work);
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;
static K_FIFO_DEFINE(tx_queue);

#define HCI_RPMSG_CMD 0x01
#define HCI_RPMSG_ACL 0x02
#define HCI_RPMSG_SCO 0x03
#define HCI_RPMSG_EVT 0x04

static struct net_buf *hci_rpmsg_cmd_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_cmd_hdr *hdr = (void *)data;
	struct net_buf *buf;

	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enought data for command header");
		return NULL;
	}

	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available command buffers!");
		return NULL;
	}

	if (remaining != hdr->param_len) {
		LOG_ERR("Command payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %u", hdr->param_len);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *hci_rpmsg_acl_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr *hdr = (void *)data;
	struct net_buf *buf;

	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enought data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (remaining != sys_le16_to_cpu(hdr->len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %u", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static void hci_rpmsg_rx(uint8_t *data, size_t len)
{
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;

	LOG_HEXDUMP_DBG(data, len, "RPMSG data:");

	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case HCI_RPMSG_CMD:
		buf = hci_rpmsg_cmd_recv(data, remaining);
		break;

	case HCI_RPMSG_ACL:
		buf = hci_rpmsg_acl_recv(data, remaining);
		break;

	default:
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
		return;
	}

	if (buf) {
		net_buf_put(&tx_queue, buf);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "Final net buffer:");
	}
}

static void tx_thread(void *p1, void *p2, void *p3)
{
	while (1) {
		struct net_buf *buf;
		int err;

		/* Wait until a buffer is available */
		buf = net_buf_get(&tx_queue, K_FOREVER);
		/* Pass buffer to the stack */
		err = bt_send(buf);
		if (err) {
			LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}

		/* Give other threads a chance to run if tx_queue keeps getting
		 * new data all the time.
		 */
		k_yield();
	}
}

static int hci_rpmsg_send(struct net_buf *buf)
{
	uint8_t pkt_indicator;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
		buf->len);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Controller buffer:");

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		pkt_indicator = HCI_RPMSG_ACL;
		break;
	case BT_BUF_EVT:
		pkt_indicator = HCI_RPMSG_EVT;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}
	net_buf_push_u8(buf, pkt_indicator);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");
	rpmsg_send(&ep, buf->data, buf->len);

	net_buf_unref(buf);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	LOG_ERR("Controller assert in: %s at %d", file, line);
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
		void *priv)
{
	LOG_INF("Received message of %u bytes.", len);
	hci_rpmsg_rx((uint8_t *) data, len);

	return RPMSG_SUCCESS;
}

static int hci_rpmsg_init(void)
{
	int err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	static struct virtio_vring_info   rvrings[2];
	static struct virtio_device       vdev;
	static struct rpmsg_device        *rdev;
	static struct rpmsg_virtio_device rvdev;
	static struct metal_io_region     *io;
	static struct metal_device        *device;

	/* Setup IPM workqueue item */
	k_work_init(&ipm_work, ipm_callback_process);

	/* Libmetal setup */
	err = metal_init(&metal_params);
	if (err) {
		LOG_ERR("metal_init: failed - error code %d", err);
		return err;
	}

	err = metal_register_generic_device(&shm_device);
	if (err) {
		LOG_ERR("Couldn't register shared memory device: %d", err);
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		LOG_ERR("metal_device_open failed: %d", err);
		return err;
	}

	io = metal_device_io_region(device, 0);
	if (!io) {
		LOG_ERR("metal_device_io_region failed to get region");
		return -ENODEV;
	}

	/* IPM setup */
	ipm_tx_handle = device_get_binding("IPM_1");
	if (!ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -ENODEV;
	}

	ipm_rx_handle = device_get_binding("IPM_0");
	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]");
		return -ENOMEM;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]");
		return -ENOMEM;
	}

	rvrings[0].io = io;
	rvrings[0].info.vaddr = (void *)VRING_TX_ADDRESS;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = io;
	rvrings[1].info.vaddr = (void *)VRING_RX_ADDRESS;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.role = RPMSG_REMOTE;
	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	vdev.vrings_info = &rvrings[0];

	/* setup rvdev */
	err = rpmsg_init_vdev(&rvdev, &vdev, NULL, io, NULL);
	if (err) {
		LOG_ERR("rpmsg_init_vdev failed %d", err);
		return err;
	}

	rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

	err = rpmsg_create_ept(&ep, rdev, "bt_hci", RPMSG_ADDR_ANY,
				  RPMSG_ADDR_ANY, endpoint_cb,
				  rpmsg_service_unbind);
	if (err) {
		LOG_ERR("rpmsg_create_ept failed %d", err);
		return err;
	}

	return err;
}

void main(void)
{
	int err;

	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);

	/* initialize RPMSG */
	err = hci_rpmsg_init();
	if (err != 0) {
		return;
	}

	LOG_DBG("Start");

	/* Enable the raw interface, this will in turn open the HCI driver */
	bt_enable_raw(&rx_queue);

	/* Spawn the TX thread and start feeding commands and data to the
	 * controller
	 */
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_data, "HCI rpmsg TX");

	while (1) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);
		err = hci_rpmsg_send(buf);
		if (err) {
			LOG_ERR("Failed to send (err %d)", err);
		}
	}
}
