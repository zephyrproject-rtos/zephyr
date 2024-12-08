/* spi.c - SPI based Bluetooth driver */

#define DT_DRV_COMPAT zephyr_bt_hci_spi

/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

/* Special Values */
#define SPI_WRITE		0x0A
#define SPI_READ		0x0B
#define READY_NOW		0x02

#define EVT_BLUE_INITIALIZED	0x01

/* Offsets */
#define STATUS_HEADER_READY	0
#define STATUS_HEADER_TOREAD	3
#define STATUS_HEADER_TOWRITE	1

#define PACKET_TYPE		0
#define EVT_HEADER_TYPE		0
#define EVT_HEADER_EVENT	1
#define EVT_HEADER_SIZE		2
#define EVT_LE_META_SUBEVENT	3
#define EVT_VENDOR_CODE_LSB	3
#define EVT_VENDOR_CODE_MSB	4

#define CMD_OGF			1
#define CMD_OCF			2

/* Max SPI buffer length for transceive operations.
 *
 * Buffer size needs to be at least the size of the larger RX/TX buffer
 * required by the SPI slave, as the legacy spi_transceive requires both RX/TX
 * to be the same length. Size also needs to be compatible with the
 * slave device used (e.g. nRF5X max buffer length for SPIS is 255).
 */
#define SPI_MAX_MSG_LEN		255 /* As defined by X-NUCLEO-IDB04A1 BSP */

#define DATA_DELAY_US DT_INST_PROP(0, controller_data_delay_us)

/* Single byte header denoting the buffer type */
#define H4_HDR_SIZE 1

/* Maximum L2CAP MTU that can fit in a single packet */
#define MAX_MTU (SPI_MAX_MSG_LEN - H4_HDR_SIZE - BT_L2CAP_HDR_SIZE - BT_HCI_ACL_HDR_SIZE)

#if CONFIG_BT_L2CAP_TX_MTU > MAX_MTU
#warning CONFIG_BT_L2CAP_TX_MTU is too large and can result in packets that cannot \
	be transmitted across this HCI link
#endif /* CONFIG_BT_L2CAP_TX_MTU > MAX_MTU */

struct bt_spi_data {
	bt_hci_recv_t recv;
};

static uint8_t __noinit rxmsg[SPI_MAX_MSG_LEN];
static uint8_t __noinit txmsg[SPI_MAX_MSG_LEN];

static const struct gpio_dt_spec irq_gpio = GPIO_DT_SPEC_INST_GET(0, irq_gpios);
static const struct gpio_dt_spec rst_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios);

static struct gpio_callback	gpio_cb;

static K_SEM_DEFINE(sem_initialised, 0, 1);
static K_SEM_DEFINE(sem_request, 0, 1);
static K_SEM_DEFINE(sem_busy, 1, 1);

static K_KERNEL_STACK_DEFINE(spi_rx_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread spi_rx_thread_data;

static const struct spi_dt_spec bus = SPI_DT_SPEC_INST_GET(
	0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0);

static struct spi_buf spi_tx_buf;
static struct spi_buf spi_rx_buf;
static const struct spi_buf_set spi_tx = {
	.buffers = &spi_tx_buf,
	.count = 1
};
static const struct spi_buf_set spi_rx = {
	.buffers = &spi_rx_buf,
	.count = 1
};

static inline int bt_spi_transceive(void *tx, uint32_t tx_len,
				    void *rx, uint32_t rx_len)
{
	spi_tx_buf.buf = tx;
	spi_tx_buf.len = (size_t)tx_len;
	spi_rx_buf.buf = rx;
	spi_rx_buf.len = (size_t)rx_len;
	return spi_transceive_dt(&bus, &spi_tx, &spi_rx);
}

static inline uint16_t bt_spi_get_cmd(uint8_t *msg)
{
	return (msg[CMD_OCF] << 8) | msg[CMD_OGF];
}

static inline uint16_t bt_spi_get_evt(uint8_t *msg)
{
	return (msg[EVT_VENDOR_CODE_MSB] << 8) | msg[EVT_VENDOR_CODE_LSB];
}

static void bt_spi_isr(const struct device *unused1,
		       struct gpio_callback *unused2,
		       uint32_t unused3)
{
	LOG_DBG("");

	k_sem_give(&sem_request);
}

static bool bt_spi_handle_vendor_evt(uint8_t *msg)
{
	bool handled = false;

	switch (bt_spi_get_evt(msg)) {
	case EVT_BLUE_INITIALIZED: {
		k_sem_give(&sem_initialised);
		handled = true;
	}
	default:
		break;
	}
	return handled;
}

static int bt_spi_get_header(uint8_t op, uint16_t *size)
{
	uint8_t header_master[5] = {op, 0, 0, 0, 0};
	uint8_t header_slave[5];
	bool reading = (op == SPI_READ);
	bool loop_cond;
	uint8_t size_offset;
	int ret;

	if (!(op == SPI_READ || op == SPI_WRITE)) {
		return -EINVAL;
	}
	if (reading) {
		size_offset = STATUS_HEADER_TOREAD;
	}

	do {
		ret = bt_spi_transceive(header_master, 5, header_slave, 5);
		if (ret) {
			break;
		}
		if (reading) {
			/* When reading, keep looping if there is not yet any data */
			loop_cond = header_slave[STATUS_HEADER_TOREAD] == 0U;
		} else {
			/* When writing, keep looping if all bytes are zero */
			loop_cond = ((header_slave[1] | header_slave[2] | header_slave[3] |
					header_slave[4]) == 0U);
		}
	} while ((header_slave[STATUS_HEADER_READY] != READY_NOW) || loop_cond);

	*size = (reading ? header_slave[size_offset] : SPI_MAX_MSG_LEN);

	return ret;
}

static struct net_buf *bt_spi_rx_buf_construct(uint8_t *msg)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;
	struct bt_hci_acl_hdr acl_hdr;
	struct net_buf *buf;
	int len;

	switch (msg[PACKET_TYPE]) {
	case BT_HCI_H4_EVT:
		switch (msg[EVT_HEADER_EVENT]) {
		case BT_HCI_EVT_VENDOR:
			/* Run event through interface handler */
			if (bt_spi_handle_vendor_evt(msg)) {
				return NULL;
			}
			/* Event has not yet been handled */
			__fallthrough;
		default:
			if (msg[EVT_HEADER_EVENT] == BT_HCI_EVT_LE_META_EVENT &&
			    (msg[EVT_LE_META_SUBEVENT] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
				discardable = true;
				timeout = K_NO_WAIT;
			}
			buf = bt_buf_get_evt(msg[EVT_HEADER_EVENT],
					     discardable, timeout);
			if (!buf) {
				LOG_DBG("Discard adv report due to insufficient buf");
				return NULL;
			}
		}

		len = sizeof(struct bt_hci_evt_hdr) + msg[EVT_HEADER_SIZE];
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("Event too long: %d", len);
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, &msg[1], len);
		break;
	case BT_HCI_H4_ACL:
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		memcpy(&acl_hdr, &msg[1], sizeof(acl_hdr));
		len = sizeof(acl_hdr) + sys_le16_to_cpu(acl_hdr.len);
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("ACL too long: %d", len);
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, &msg[1], len);
		break;
	default:
		LOG_ERR("Unknown BT buf type %d", msg[0]);
		return NULL;
	}

	return buf;
}

static void bt_spi_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct bt_spi_data *hci = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_buf *buf;
	uint16_t size = 0U;
	int ret;

	(void)memset(&txmsg, 0xFF, SPI_MAX_MSG_LEN);
	while (true) {

		/* Wait for interrupt pin to be active */
		k_sem_take(&sem_request, K_FOREVER);

		LOG_DBG("");

		/* Wait for SPI bus to be available */
		k_sem_take(&sem_busy, K_FOREVER);
		ret = bt_spi_get_header(SPI_READ, &size);

		/* Delay here is rounded up to next tick */
		k_sleep(K_USEC(DATA_DELAY_US));
		/* Read data */
		if (ret == 0 && size != 0) {
			do {
				ret = bt_spi_transceive(&txmsg, size,
							&rxmsg, size);
				if (rxmsg[0] == 0U) {
					/* Consider increasing controller-data-delay-us
					 * if this message is extremely common.
					 */
					LOG_DBG("Controller not ready for SPI transaction "
						"of %d bytes", size);
				}
			} while (rxmsg[0] == 0U && ret == 0);
		}

		k_sem_give(&sem_busy);

		if (ret || size == 0) {
			if (ret) {
				LOG_ERR("Error %d", ret);
			}
			continue;
		}

		LOG_HEXDUMP_DBG(rxmsg, size, "SPI RX");

		/* Construct net_buf from SPI data */
		buf = bt_spi_rx_buf_construct(rxmsg);
		if (buf) {
			/* Handle the received HCI data */
			hci->recv(dev, buf);
		}
	}
}

static int bt_spi_send(const struct device *dev, struct net_buf *buf)
{
	uint16_t size;
	uint8_t rx_first[1];
	int ret;

	ARG_UNUSED(dev);

	LOG_DBG("");

	/* Buffer needs an additional byte for type */
	if (buf->len >= SPI_MAX_MSG_LEN) {
		LOG_ERR("Message too long (%d)", buf->len);
		return -EINVAL;
	}

	/* Wait for SPI bus to be available */
	k_sem_take(&sem_busy, K_FOREVER);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, BT_HCI_H4_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, BT_HCI_H4_CMD);
		break;
	default:
		LOG_ERR("Unsupported type");
		k_sem_give(&sem_busy);
		return -EINVAL;
	}

	ret = bt_spi_get_header(SPI_WRITE, &size);
	size = MIN(buf->len, size);

	if (size < buf->len) {
		LOG_WRN("Unable to write full data, skipping");
		size = 0;
		ret = -ECANCELED;
	}

	if (!ret) {
		/* Delay here is rounded up to next tick */
		k_sleep(K_USEC(DATA_DELAY_US));
		/* Transmit the message */
		while (true) {
			ret = bt_spi_transceive(buf->data, size,
						rx_first, 1);
			if (rx_first[0] != 0U || ret) {
				break;
			}
			/* Consider increasing controller-data-delay-us
			 * if this message is extremely common.
			 */
			LOG_DBG("Controller not ready for SPI transaction of %d bytes", size);
		}
	}

	k_sem_give(&sem_busy);

	if (ret) {
		LOG_ERR("Error %d", ret);
		goto out;
	}

	LOG_HEXDUMP_DBG(buf->data, buf->len, "SPI TX");

out:
	net_buf_unref(buf);

	return ret;
}

static int bt_spi_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_spi_data *hci = dev->data;
	int err;

	/* Configure RST pin and hold BLE in Reset */
	err = gpio_pin_configure_dt(&rst_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}

	/* Configure IRQ pin and the IRQ call-back/handler */
	err = gpio_pin_configure_dt(&irq_gpio, GPIO_INPUT);
	if (err) {
		return err;
	}

	gpio_init_callback(&gpio_cb, bt_spi_isr, BIT(irq_gpio.pin));
	err = gpio_add_callback(irq_gpio.port, &gpio_cb);
	if (err) {
		return err;
	}

	/* Enable the interrupt line */
	err = gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		return err;
	}

	hci->recv = recv;

	/* Take BLE out of reset */
	k_sleep(K_MSEC(DT_INST_PROP_OR(0, reset_assert_duration_ms, 0)));
	gpio_pin_set_dt(&rst_gpio, 0);

	/* Start RX thread */
	k_thread_create(&spi_rx_thread_data, spi_rx_stack,
			K_KERNEL_STACK_SIZEOF(spi_rx_stack),
			bt_spi_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	/* Device will let us know when it's ready */
	k_sem_take(&sem_initialised, K_FOREVER);

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open		= bt_spi_open,
	.send		= bt_spi_send,
};

static int bt_spi_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (!spi_is_ready_dt(&bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&rst_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	LOG_DBG("BT SPI initialized");

	return 0;
}

#define HCI_DEVICE_INIT(inst) \
	static struct bt_spi_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, bt_spi_init, NULL, &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_BT_SPI_INIT_PRIORITY, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
