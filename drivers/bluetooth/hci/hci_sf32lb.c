/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_mailbox

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>

#include <common/bt_str.h>
#include <bf0_hal.h>
#include <ipc_queue.h>
#include <ipc_hw.h>
#include <bf0_mbox_common.h>

LOG_MODULE_REGISTER(hci_sf32lb, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

#define IPC_TIMEOUT_MS               10
#define BT_HCI_EXT_SF32LB52_BT_READY BT_OP(BT_OGF_VS, 0x11)

struct bt_sf32lb_data {
	struct {
		struct net_buf *buf;
		struct k_fifo fifo;

		uint16_t remaining;
		uint16_t discard;

		bool have_hdr;
		bool discardable;
		bool ready;

		uint8_t hdr_len;

		uint8_t type;
		union {
			struct bt_hci_evt_hdr evt;
			struct bt_hci_acl_hdr acl;
			struct bt_hci_iso_hdr iso;
			struct bt_hci_sco_hdr sco;
			uint8_t hdr[4];
		};
	} rx;

	struct {
		uint8_t type;
		struct net_buf *buf;
		struct k_fifo fifo;
	} tx;

	struct k_sem sem;
	bt_hci_recv_t recv;
	ipc_queue_handle_t ipc_port;
};

struct bt_sf32lb_config {
	const struct device *mbox;
	k_thread_stack_t *rx_thread_stack;
	size_t rx_thread_stack_size;
	struct k_thread *rx_thread;
	ipc_queue_handle_t ipc_port;
};

static inline void process_tx(const struct device *dev);
static inline void process_rx(const struct device *dev);

static int32_t mbox_rx_ind(ipc_queue_handle_t handle, size_t size)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(mailbox));
	struct bt_sf32lb_data *hci = dev->data;

	k_sem_give(&(hci->sem));
	return 0;
}

static void mbox_sf32lb_isr(const struct device *dev)
{
	LCPU2HCPU_IRQHandler();
}

static int zbt_config_mailbox(const struct device *dev)
{
	ipc_queue_cfg_t q_cfg;
	struct bt_sf32lb_data *hci = dev->data;

	HAL_HPAON_StartGTimer();
	k_sem_init(&(hci->sem), 0, 1);
	q_cfg.qid = 0;
	q_cfg.tx_buf_size = HCPU2LCPU_MB_CH1_BUF_SIZE;
	q_cfg.tx_buf_addr = HCPU2LCPU_MB_CH1_BUF_START_ADDR;
	q_cfg.tx_buf_addr_alias = HCPU_ADDR_2_LCPU_ADDR(HCPU2LCPU_MB_CH1_BUF_START_ADDR);

#ifndef SF32LB52X
	/* Config IPC queue. */
	q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_START_ADDR);
#else
	uint8_t rev_id = __HAL_SYSCFG_GET_REVID();

	if (rev_id < HAL_CHIP_REV_ID_A4) {
		q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_START_ADDR);
	} else {
		q_cfg.rx_buf_addr = LCPU_ADDR_2_HCPU_ADDR(LCPU2HCPU_MB_CH1_BUF_REV_B_START_ADDR);
	}
#endif

	q_cfg.rx_ind = NULL;
	q_cfg.user_data = 0;

	if (q_cfg.rx_ind == NULL) {
		q_cfg.rx_ind = mbox_rx_ind;
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mbox_sf32lb_isr,
		    DEVICE_DT_INST_GET(0), 0);
	hci->ipc_port = ipc_queue_init(&q_cfg);
	if (IPC_QUEUE_INVALID_HANDLE == hci->ipc_port || ipc_queue_open(hci->ipc_port) != 0) {
		LOG_ERR("Could not open IPC %d", hci->ipc_port);
		return -EIO;
	}
	return 0;
}

static inline void hci_get_type(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	/* Get packet type */
	if (ipc_queue_read(hci->ipc_port, &hci->rx.type, 1) != 1) {
		LOG_WRN("Unable to read H:4 packet type");
		hci->rx.type = BT_HCI_H4_NONE;
		return;
	}

	switch (hci->rx.type) {
	case BT_HCI_H4_EVT:
		hci->rx.remaining = sizeof(hci->rx.evt);
		hci->rx.hdr_len = hci->rx.remaining;
		break;
	case BT_HCI_H4_ACL:
		hci->rx.remaining = sizeof(hci->rx.acl);
		hci->rx.hdr_len = hci->rx.remaining;
		break;
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_H4_SCO:
		hci->rx.remaining = sizeof(hci->rx.sco);
		hci->rx.hdr_len = hci->rx.remaining;
		break;
#endif
#if defined(CONFIG_BT_ISO)
	case BT_HCI_H4_ISO:
		hci->rx.remaining = sizeof(hci->rx.iso);
		hci->rx.hdr_len = hci->rx.remaining;
		break;
#endif
	default:
		goto error;
	}
	return;
error:
	LOG_ERR("Unknown HCI type 0x%02x", hci->rx.type);
	hci->rx.type = BT_HCI_H4_NONE;
}

static void hci_read_hdr(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;
	int bytes_read = hci->rx.hdr_len - hci->rx.remaining;
	int ret;

	ret = ipc_queue_read(hci->ipc_port, hci->rx.hdr + bytes_read, hci->rx.remaining);
	if (unlikely((ret < 0) || (ret > hci->rx.remaining))) {
		LOG_ERR("Unable to read from IPC mailbox (ret %d)", ret);
	} else {
		hci->rx.remaining -= ret;
	}
}

static inline void get_acl_hdr(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	hci_read_hdr(dev);

	if (hci->rx.remaining == 0) {
		struct bt_hci_acl_hdr *hdr = &hci->rx.acl;

		hci->rx.remaining = sys_le16_to_cpu(hdr->len);
		LOG_DBG("Got ACL header. Payload %u bytes", hci->rx.remaining);
		hci->rx.have_hdr = true;
	}
}

static inline void get_sco_hdr(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	hci_read_hdr(dev);

	if (hci->rx.remaining == 0) {
		struct bt_hci_sco_hdr *hdr = &hci->rx.sco;

		hci->rx.remaining = hdr->len;
		LOG_DBG("Got SCO header. Payload %u bytes", hci->rx.remaining);
		hci->rx.have_hdr = true;
	}
}

static inline void get_iso_hdr(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	hci_read_hdr(dev);

	if (hci->rx.remaining == 0) {
		struct bt_hci_iso_hdr *hdr = &hci->rx.iso;

		hci->rx.remaining = bt_iso_hdr_len(sys_le16_to_cpu(hdr->len));
		LOG_DBG("Got ISO header. Payload %u bytes", hci->rx.remaining);

		hci->rx.have_hdr = true;
	}
}

static inline void get_evt_hdr(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	struct bt_hci_evt_hdr *hdr = &hci->rx.evt;

	hci_read_hdr(dev);

	if (hci->rx.hdr_len == sizeof(*hdr) && hci->rx.remaining < sizeof(*hdr)) {
		switch (hci->rx.evt.evt) {
		case BT_HCI_EVT_LE_META_EVENT:
			hci->rx.remaining++;
			hci->rx.hdr_len++;
			break;
#if defined(CONFIG_BT_CLASSIC)
		case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
		case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
			hci->rx.discardable = true;
			break;
#endif
		default:
			break;
		}
	}

	if (hci->rx.remaining == 0) {
		if (hci->rx.evt.evt == BT_HCI_EVT_LE_META_EVENT &&
		    hci->rx.hdr[sizeof(*hdr)] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
			LOG_DBG("Marking adv report as discardable");
			hci->rx.discardable = true;
		}

		hci->rx.remaining = hdr->len - (hci->rx.hdr_len - sizeof(*hdr));
		LOG_DBG("Got event header. Payload %u bytes", hdr->len);
		hci->rx.have_hdr = true;
	}
}

static inline void copy_hdr(struct bt_sf32lb_data *hci)
{
	net_buf_add_mem(hci->rx.buf, hci->rx.hdr, hci->rx.hdr_len);
}

static void reset_rx(struct bt_sf32lb_data *hci)
{
	hci->rx.type = BT_HCI_H4_NONE;
	hci->rx.remaining = 0U;
	hci->rx.have_hdr = false;
	hci->rx.hdr_len = 0U;
	hci->rx.discardable = false;
}

static struct net_buf *get_rx(struct bt_sf32lb_data *hci)
{
	LOG_DBG("type 0x%02x, evt 0x%02x", hci->rx.type, hci->rx.evt.evt);

	switch (hci->rx.type) {
	case BT_HCI_H4_EVT:
		return bt_buf_get_evt(hci->rx.evt.evt, hci->rx.discardable, K_NO_WAIT);
	case BT_HCI_H4_ACL:
		return bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	case BT_HCI_H4_SCO:
		if (IS_ENABLED(CONFIG_BT_CLASSIC)) {
			LOG_ERR("SCO not supported by host stack.");
		}
		break;
	case BT_HCI_H4_ISO:
		if (IS_ENABLED(CONFIG_BT_ISO)) {
			return bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
		}
		break;
	default:
		LOG_ERR("Invalid rx type 0x%02x", hci->rx.type);
	}

	return NULL;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;

	lcpu_power_on();

	while (1) {
		int len;
		struct bt_sf32lb_data *hci = dev->data;

		k_sem_take(&(hci->sem), K_FOREVER);
		len = ipc_queue_get_rx_size(hci->ipc_port);
		while (len > 0) {
			struct net_buf *buf;

			LOG_DBG("len %d", len);
			process_rx(dev);
			while ((buf = k_fifo_get(&hci->rx.fifo, K_NO_WAIT)) != NULL) {
				LOG_DBG("Calling bt_recv(%p),len=%d,data=%p", (void *)buf, buf->len,
					(void *)buf->data);

				if (buf->len == 0 || buf->data == NULL) {
					break;
				}
				if (hci->recv != NULL && hci->rx.ready) {
					hci->recv(dev, buf);
					buf = k_fifo_get(&hci->rx.fifo, K_NO_WAIT);
					continue;
				}
				if (buf->data[0] == BT_HCI_H4_EVT &&
				    buf->data[1] == BT_HCI_EVT_CMD_COMPLETE &&
				    sys_get_le16(&buf->data[4]) == BT_HCI_EXT_SF32LB52_BT_READY) {
					/*Bluetooth LCPU RX ready*/
					hci->rx.ready = true;
				}
				if (!hci->rx.ready) {
					LOG_WRN("Got unexpected packet\n");
				}
				net_buf_unref(buf);
			};
			len = ipc_queue_get_rx_size(hci->ipc_port);
		}
		process_tx(dev);
	}
}

static size_t hci_discard(const struct device *dev, size_t len)
{
	struct bt_sf32lb_data *hci = dev->data;
	uint8_t buf[33];
	int err;

	err = ipc_queue_read(hci->ipc_port, buf, MIN(len, sizeof(buf)));
	if (unlikely(err < 0)) {
		LOG_ERR("Unable to read from UART (err %d)", err);
		err = 0;
	}
	return err;
}

static inline void read_payload(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;
	struct net_buf *buf;
	int read;

	if (hci->rx.buf == NULL) {
		size_t buf_tailroom;

		hci->rx.buf = get_rx(hci);
		if (!hci->rx.buf) {
			if (hci->rx.discardable) {
				LOG_WRN("Discarding event 0x%02x", hci->rx.evt.evt);
				hci->rx.discard = hci->rx.remaining;
				reset_rx(hci);
				return;
			}
			LOG_WRN("Failed to allocate, deferring to rx_thread");
			return;
		}
		LOG_DBG("Allocated rx.buf %p", (void *)hci->rx.buf);

		buf_tailroom = net_buf_tailroom(hci->rx.buf);
		if (buf_tailroom < (hci->rx.remaining + hci->rx.hdr_len)) {
			LOG_ERR("Not enough space in buffer %u/%zu", hci->rx.remaining,
				buf_tailroom);
			hci->rx.discard = hci->rx.remaining;
			reset_rx(hci);
			return;
		}
		copy_hdr(hci);
	}

	read = ipc_queue_read(hci->ipc_port, net_buf_tail(hci->rx.buf), hci->rx.remaining);
	if (unlikely((read < 0) || (read > hci->rx.remaining))) {
		LOG_ERR("Failed to read UART (err read length %d)", read);
		return;
	}

	net_buf_add(hci->rx.buf, read);
	hci->rx.remaining -= read;

	LOG_DBG("got %d bytes, remaining %u", read, hci->rx.remaining);
	LOG_DBG("Payload (len %u): %s", hci->rx.buf->len,
		bt_hex(hci->rx.buf->data, hci->rx.buf->len));

	if (hci->rx.remaining > 0) {
		return;
	}

	buf = hci->rx.buf;
	hci->rx.buf = NULL;

	reset_rx(hci);

	LOG_DBG("Putting buf %p to rx fifo", (void *)buf);
	k_fifo_put(&hci->rx.fifo, buf);
}

static inline void read_header(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	switch (hci->rx.type) {
	case BT_HCI_H4_NONE:
		hci_get_type(dev);
		return;
	case BT_HCI_H4_EVT:
		get_evt_hdr(dev);
		break;
	case BT_HCI_H4_ACL:
		get_acl_hdr(dev);
		break;
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_H4_SCO:
		get_sco_hdr(dev);
		break;
#endif
#if defined(CONFIG_BT_ISO)
	case BT_HCI_H4_ISO:
		get_iso_hdr(dev);
		break;
#endif
	default:
		LOG_ERR("Invalid rx type %d\n", hci->rx.type);
		CODE_UNREACHABLE;
		return;
	}

	if (hci->rx.have_hdr && hci->rx.buf != NULL) {
		if (hci->rx.remaining > net_buf_tailroom(hci->rx.buf)) {
			LOG_ERR("Not enough space in buffer");
			hci->rx.discard = hci->rx.remaining;
			reset_rx(hci);
		} else {
			copy_hdr(hci);
		}
	}
}

static inline void process_tx(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;
	int bytes;

	if (hci->tx.buf == NULL) {
		hci->tx.buf = k_fifo_get(&hci->tx.fifo, K_NO_WAIT);
		if (!hci->tx.buf) {
			return;
		}
	} else {
		LOG_DBG("Other tx is running");
		return;
	}

	while (hci->tx.buf != NULL) {
		LOG_DBG("data %p, type %d len %d", (void *)hci->tx.buf->data, hci->tx.buf->data[0],
			hci->tx.buf->len);
		bytes = ipc_queue_write(hci->ipc_port, hci->tx.buf->data, hci->tx.buf->len,
					IPC_TIMEOUT_MS);
		if (unlikely(bytes < 0)) {
			LOG_ERR("Unable to write to UART (err %d)", bytes);
		} else {
			LOG_DBG("bytes %d", bytes);
			net_buf_pull(hci->tx.buf, bytes);
		}
		if (hci->tx.buf->len > 0) {
			return;
		}
		hci->tx.type = BT_HCI_H4_NONE;
		net_buf_unref(hci->tx.buf);
		hci->tx.buf = k_fifo_get(&hci->tx.fifo, K_NO_WAIT);
	}
}

static inline void process_rx(const struct device *dev)
{
	struct bt_sf32lb_data *hci = dev->data;

	LOG_DBG("remaining %u discard %u have_hdr %u rx.buf %p len %u", hci->rx.remaining,
		hci->rx.discard, hci->rx.have_hdr, (void *)hci->rx.buf,
		hci->rx.buf ? hci->rx.buf->len : 0);

	if (hci->rx.discard > 0) {
		hci->rx.discard -= hci_discard(dev, hci->rx.discard);
		return;
	}

	if (hci->rx.have_hdr) {
		read_payload(dev);
	} else {
		read_header(dev);
	}
}

static int bt_hci_sf32lb_send(const struct device *dev, struct net_buf *buf)
{
	struct bt_sf32lb_data *hci = dev->data;

	LOG_DBG("buf %p type %u len %u", (void *)buf, buf->data[0], buf->len);

	k_fifo_put(&hci->tx.fifo, buf);
	k_sem_give(&(hci->sem));
	return 0;
}

static int bt_hci_sf32lb_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_sf32lb_data *hci = dev->data;
	const struct bt_sf32lb_config *cfg = dev->config;
	k_tid_t tid;
	int r;

	LOG_DBG("hci open %p", (void *)recv);
	hci->recv = recv;
	r = zbt_config_mailbox(dev);
	if (r == 0) {
		tid = k_thread_create(cfg->rx_thread, cfg->rx_thread_stack,
				      cfg->rx_thread_stack_size, rx_thread, (void *)dev, NULL, NULL,
				      K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);
		k_thread_name_set(tid, "hci_rx_th");
	}
	return r;
}

static const DEVICE_API(bt_hci, hci_sf32lb_driver_api) = {
	.open = bt_hci_sf32lb_open,
	.send = bt_hci_sf32lb_send,
};

#define BT_MBOX_DEVICE_INIT(inst)                                                                  \
	static K_KERNEL_STACK_DEFINE(rx_thread_stack_##inst, CONFIG_BT_DRV_RX_STACK_SIZE);         \
	static struct k_thread rx_thread_##inst;                                                   \
	static const struct bt_sf32lb_config hci_config_##inst = {                                 \
		.rx_thread_stack = rx_thread_stack_##inst,                                         \
		.rx_thread_stack_size = K_KERNEL_STACK_SIZEOF(rx_thread_stack_##inst),             \
		.rx_thread = &rx_thread_##inst,                                                    \
	};                                                                                         \
	static struct bt_sf32lb_data hci_data_##inst = {                                           \
		.rx =                                                                              \
			{                                                                          \
				.fifo = Z_FIFO_INITIALIZER(hci_data_##inst.rx.fifo),               \
			},                                                                         \
		.tx =                                                                              \
			{                                                                          \
				.fifo = Z_FIFO_INITIALIZER(hci_data_##inst.tx.fifo),               \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_data_##inst, &hci_config_##inst, POST_KERNEL, \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &hci_sf32lb_driver_api)

BT_MBOX_DEVICE_INIT(0)
