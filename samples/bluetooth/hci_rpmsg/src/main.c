/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/ipc/ipc_service.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/hci_vs.h>

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
#include <zephyr/logging/log_ctrl.h>
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */

#define BT_DBG_ENABLED 0
#define LOG_MODULE_NAME hci_rpmsg
#include "common/log.h"

static struct ipc_ept hci_ept;

static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;
static K_FIFO_DEFINE(tx_queue);
static K_SEM_DEFINE(ipc_bound_sem, 0, 1);
#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER) || defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
/* A flag used to store information if the IPC endpoint has already been bound. The end point can't
 * be used before that happens.
 */
static bool ipc_ept_ready;
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER || CONFIG_BT_HCI_VS_FATAL_ERROR */

#define HCI_RPMSG_CMD 0x01
#define HCI_RPMSG_ACL 0x02
#define HCI_RPMSG_SCO 0x03
#define HCI_RPMSG_EVT 0x04
#define HCI_RPMSG_ISO 0x05

#define HCI_FATAL_ERR_MSG true
#define HCI_REGULAR_MSG false

static struct net_buf *hci_rpmsg_cmd_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_cmd_hdr *hdr = (void *)data;
	struct net_buf *buf;

	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enough data for command header");
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
		LOG_ERR("Not enough data for ACL header");
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

static struct net_buf *hci_rpmsg_iso_recv(uint8_t *data, size_t remaining)
{
	struct bt_hci_iso_hdr *hdr = (void *)data;
	struct net_buf *buf;

	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return NULL;
	}

	buf = bt_buf_get_tx(BT_BUF_ISO_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available ISO buffers!");
		return NULL;
	}

	if (remaining != bt_iso_hdr_len(sys_le16_to_cpu(hdr->len))) {
		LOG_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %zu", remaining);
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

	case HCI_RPMSG_ISO:
		buf = hci_rpmsg_iso_recv(data, remaining);
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

static void hci_rpmsg_send(struct net_buf *buf, bool is_fatal_err)
{
	uint8_t pkt_indicator;
	uint8_t retries = 0;
	int ret;

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
	case BT_BUF_ISO_IN:
		pkt_indicator = HCI_RPMSG_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return;
	}
	net_buf_push_u8(buf, pkt_indicator);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	do {
		ret = ipc_service_send(&hci_ept, buf->data, buf->len);
		if (ret < 0) {
			retries++;
			if (retries > 10) {
				/* Default backend (rpmsg_virtio) has a timeout of 150ms. */
				LOG_WRN("IPC send has been blocked for 1.5 seconds.");
				retries = 0;
			}

			/* The function can be called by the application main thread,
			 * bt_ctlr_assert_handle and k_sys_fatal_error_handler. In case of a call by
			 * Bluetooth Controller assert handler or system fatal error handler the
			 * call can be from ISR context, hence there is no thread to yield. Besides
			 * that both handlers implement a policy to provide error information and
			 * stop the system in an infinite loop. The goal is to prevent any other
			 * damage to the system if one of such exeptional situations occur, hence
			 * call to k_yield is against it.
			 */
			if (is_fatal_err) {
				LOG_ERR("IPC service send error: %d", ret);
			} else {
				k_yield();
			}
		}
	} while (ret < 0);

	LOG_INF("Sent message of %d bytes.", ret);

	net_buf_unref(buf);
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
	/* Disable interrupts, this is unrecoverable */
	(void)irq_lock();

	/* Generate an error event only when IPC service endpoint is already bound. */
	if (ipc_ept_ready) {
		/* Prepare vendor specific HCI debug event */
		struct net_buf *buf;

		buf = hci_vs_err_assert(file, line);
		if (buf == NULL) {
			/* Send the event over rpmsg */
			hci_rpmsg_send(buf, HCI_FATAL_ERR_MSG);
		} else {
			LOG_ERR("Can't create Fatal Error HCI event: %s at %d", __FILE__, __LINE__);
		}
	} else {
		LOG_ERR("IPC endpoint is not redy yet: %s at %d", __FILE__, __LINE__);
	}

	LOG_ERR("Halting system");

	while (true) {
	};
#else
	LOG_ERR("Controller assert in: %s at %d", file, line);
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

#if defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	LOG_PANIC();

	/* Disable interrupts, this is unrecoverable */
	(void)irq_lock();

	/* Generate an error event only when there is a stack frame and IPC service endpoint is
	 * already bound.
	 */
	if (esf != NULL && ipc_ept_ready) {
		/* Prepare vendor specific HCI debug event */
		struct net_buf *buf;

		buf = hci_vs_err_stack_frame(reason, esf);
		if (buf != NULL) {
			hci_rpmsg_send(buf, HCI_FATAL_ERR_MSG);
		} else {
			LOG_ERR("Can't create Fatal Error HCI event.\n");
		}
	}

	LOG_ERR("Halting system");

	while (true) {
	};

	CODE_UNREACHABLE;
}
#endif /* CONFIG_BT_HCI_VS_FATAL_ERROR */

static void hci_ept_bound(void *priv)
{
	k_sem_give(&ipc_bound_sem);
#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER) || defined(CONFIG_BT_HCI_VS_FATAL_ERROR)
	ipc_ept_ready = true;
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER || CONFIG_BT_HCI_VS_FATAL_ERROR */
}

static void hci_ept_recv(const void *data, size_t len, void *priv)
{
	LOG_INF("Received message of %u bytes.", len);
	hci_rpmsg_rx((uint8_t *) data, len);
}

static struct ipc_ept_cfg hci_ept_cfg = {
	.name = "nrf_bt_hci",
	.cb = {
		.bound    = hci_ept_bound,
		.received = hci_ept_recv,
	},
};

void main(void)
{
	int err;
	const struct device *hci_ipc_instance =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_rpmsg_ipc));

	/* incoming events and data from the controller */
	static K_FIFO_DEFINE(rx_queue);

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

	/* Initialize IPC service instance and register endpoint. */
	err = ipc_service_open_instance(hci_ipc_instance);
	if (err) {
		LOG_ERR("IPC service instance initialization failed: %d\n", err);
	}

	err = ipc_service_register_endpoint(hci_ipc_instance, &hci_ept, &hci_ept_cfg);
	if (err) {
		LOG_ERR("Registering endpoint failed with %d", err);
	}

	k_sem_take(&ipc_bound_sem, K_FOREVER);

	while (1) {
		struct net_buf *buf;

		buf = net_buf_get(&rx_queue, K_FOREVER);
		hci_rpmsg_send(buf, HCI_REGULAR_MSG);
	}
}
