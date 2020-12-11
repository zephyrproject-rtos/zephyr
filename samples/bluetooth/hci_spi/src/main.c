/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <debug/stack.h>

#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#define LOG_MODULE_NAME hci_spi
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define HCI_CMD                0x01
#define HCI_ACL                0x02
#define HCI_SCO                0x03
#define HCI_EVT                0x04

/* Special Values */
#define SPI_WRITE              0x0A
#define SPI_READ               0x0B
#define READY_NOW              0x02
#define SANITY_CHECK           0x02

/* Offsets */
#define STATUS_HEADER_READY    0
#define STATUS_HEADER_TOREAD   3

#define PACKET_TYPE            0
#define EVT_BLUE_INITIALIZED   0x01

#define GPIO_IRQ_PIN           DT_GPIO_PIN(DT_INST(0, zephyr_bt_hci_spi_slave), irq_gpios)
#define GPIO_IRQ_FLAGS         DT_GPIO_FLAGS(DT_INST(0, zephyr_bt_hci_spi_slave), irq_gpios)

/* Needs to be aligned with the SPI master buffer size */
#define SPI_MAX_MSG_LEN        255

static uint8_t rxmsg[SPI_MAX_MSG_LEN];
static struct spi_buf rx;
const static struct spi_buf_set rx_bufs = {
	.buffers = &rx,
	.count = 1,
};

static uint8_t txmsg[SPI_MAX_MSG_LEN];
static struct spi_buf tx;
const static struct spi_buf_set tx_bufs = {
	.buffers = &tx,
	.count = 1,
};

/* HCI buffer pools */
#define CMD_BUF_SIZE BT_BUF_RX_SIZE

static const struct device *spi_hci_dev;
static struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_OP_MODE_SLAVE,
};
static const struct device *gpio_dev;
static K_THREAD_STACK_DEFINE(bt_tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread bt_tx_thread_data;

static K_SEM_DEFINE(sem_spi_rx, 0, 1);
static K_SEM_DEFINE(sem_spi_tx, 0, 1);

static inline int spi_send(struct net_buf *buf)
{
	uint8_t header_master[5] = { 0 };
	uint8_t header_slave[5] = { READY_NOW, SANITY_CHECK,
				    0x00, 0x00, 0x00 };
	int ret;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		net_buf_push_u8(buf, HCI_ACL);
		break;
	case BT_BUF_EVT:
		net_buf_push_u8(buf, HCI_EVT);
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}

	if (buf->len > SPI_MAX_MSG_LEN) {
		LOG_ERR("TX message too long");
		net_buf_unref(buf);
		return -EINVAL;
	}
	header_slave[STATUS_HEADER_TOREAD] = buf->len;

	gpio_pin_set(gpio_dev, GPIO_IRQ_PIN, 1);

	/* Coordinate transfer lock with the spi rx thread */
	k_sem_take(&sem_spi_tx, K_FOREVER);

	tx.buf = header_slave;
	tx.len = 5;
	rx.buf = header_master;
	rx.len = 5;
	do {
		ret = spi_transceive(spi_hci_dev, &spi_cfg, &tx_bufs, &rx_bufs);
		if (ret < 0) {
			LOG_ERR("SPI transceive error: %d", ret);
		}
	} while (header_master[STATUS_HEADER_READY] != SPI_READ);

	tx.buf = buf->data;
	tx.len = buf->len;

	ret = spi_write(spi_hci_dev, &spi_cfg, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("SPI transceive error: %d", ret);
	}
	net_buf_unref(buf);

	gpio_pin_set(gpio_dev, GPIO_IRQ_PIN, 0);
	k_sem_give(&sem_spi_rx);

	return 0;
}

static void bt_tx_thread(void *p1, void *p2, void *p3)
{
	uint8_t header_master[5];
	uint8_t header_slave[5] = { READY_NOW, SANITY_CHECK,
				    0x00, 0x00, 0x00 };
	struct net_buf *buf = NULL;

	union {
		struct bt_hci_cmd_hdr *cmd_hdr;
		struct bt_hci_acl_hdr *acl_hdr;
	} hci_hdr;
	hci_hdr.cmd_hdr = (struct bt_hci_cmd_hdr *)&rxmsg[1];
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)memset(txmsg, 0xFF, SPI_MAX_MSG_LEN);

	while (1) {
		tx.buf = header_slave;
		tx.len = 5;
		rx.buf = header_master;
		rx.len = 5;

		do {
			ret = spi_transceive(spi_hci_dev, &spi_cfg,
					     &tx_bufs, &rx_bufs);
			if (ret < 0) {
				LOG_ERR("SPI transceive error: %d", ret);
			}
		} while ((header_master[STATUS_HEADER_READY] != SPI_READ) &&
			 (header_master[STATUS_HEADER_READY] != SPI_WRITE));

		if (header_master[STATUS_HEADER_READY] == SPI_READ) {
			/* Unblock the spi tx thread and wait for it */
			k_sem_give(&sem_spi_tx);
			k_sem_take(&sem_spi_rx, K_FOREVER);
			continue;
		}

		tx.buf = txmsg;
		tx.len = SPI_MAX_MSG_LEN;
		rx.buf = rxmsg;
		rx.len = SPI_MAX_MSG_LEN;

		/* Receiving data from the SPI Host */
		ret = spi_transceive(spi_hci_dev, &spi_cfg,
				     &tx_bufs, &rx_bufs);
		if (ret < 0) {
			LOG_ERR("SPI transceive error: %d", ret);
			continue;
		}

		switch (rxmsg[PACKET_TYPE]) {
		case HCI_CMD:
			buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT,
					    hci_hdr.cmd_hdr,
					    sizeof(*hci_hdr.cmd_hdr));
			if (buf) {
				net_buf_add_mem(buf, &rxmsg[4],
						hci_hdr.cmd_hdr->param_len);
			} else {
				LOG_ERR("No available command buffers!");
				continue;
			}
			break;
		case HCI_ACL:
			buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_NO_WAIT,
					    hci_hdr.acl_hdr,
					    sizeof(*hci_hdr.acl_hdr));
			if (buf) {
				net_buf_add_mem(buf, &rxmsg[5],
						sys_le16_to_cpu(hci_hdr.acl_hdr->len));
			} else {
				LOG_ERR("No available ACL buffers!");
				continue;
			}
			break;
		default:
			LOG_ERR("Unknown BT HCI buf type");
			continue;
		}

		LOG_DBG("buf %p type %u len %u",
			buf, bt_buf_get_type(buf), buf->len);

		ret = bt_send(buf);
		if (ret) {
			LOG_ERR("Unable to send (ret %d)", ret);
			net_buf_unref(buf);
		}

		/* Make sure other threads get a chance to run */
		k_yield();
	}
}

static int hci_spi_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	LOG_DBG("");

	spi_hci_dev = device_get_binding(DT_BUS_LABEL(DT_INST(0, zephyr_bt_hci_spi_slave)));
	if (!spi_hci_dev) {
		return -EINVAL;
	}

	gpio_dev = device_get_binding(
		DT_GPIO_LABEL(DT_INST(0, zephyr_bt_hci_spi_slave), irq_gpios));
	if (!gpio_dev) {
		return -EINVAL;
	}
	gpio_pin_configure(gpio_dev, GPIO_IRQ_PIN,
			   GPIO_OUTPUT_INACTIVE | GPIO_IRQ_FLAGS);

	return 0;
}

SYS_DEVICE_DEFINE("hci_spi", hci_spi_init, NULL,
		  APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

void main(void)
{
	static K_FIFO_DEFINE(rx_queue);
	struct bt_hci_evt_hdr *evt_hdr;
	struct net_buf *buf;
	k_tid_t tx_id;
	int err;

	LOG_DBG("Start");

	err = bt_enable_raw(&rx_queue);
	if (err) {
		LOG_ERR("bt_enable_raw: %d; aborting", err);
		return;
	}

	/* Spawn the TX thread, which feeds cmds and data to the controller */
	tx_id = k_thread_create(&bt_tx_thread_data, bt_tx_thread_stack,
				K_THREAD_STACK_SIZEOF(bt_tx_thread_stack),
				bt_tx_thread, NULL, NULL, NULL, K_PRIO_COOP(7),
				0, K_NO_WAIT);
	k_thread_name_set(&bt_tx_thread_data, "bt_tx_thread");

	/* Send a vendor event to announce that the slave is initialized */
	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
	evt_hdr = net_buf_add(buf, sizeof(*evt_hdr));
	evt_hdr->evt = BT_HCI_EVT_VENDOR;
	evt_hdr->len = 2U;
	net_buf_add_le16(buf, EVT_BLUE_INITIALIZED);
	err = spi_send(buf);
	if (err) {
		LOG_ERR("can't send initialization event; aborting");
		k_thread_abort(tx_id);
		return;
	}

	while (1) {
		buf = net_buf_get(&rx_queue, K_FOREVER);
		err = spi_send(buf);
		if (err) {
			LOG_ERR("Failed to send");
		}
	}
}
