/* spi.c - SPI based Bluetooth driver */

/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <init.h>
#include <spi.h>
#include <misc/byteorder.h>
#include <misc/util.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include <bluetooth/log.h>
#include <bluetooth/hci_driver.h>

#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04

/* Special Values */
#define SPI_WRITE		0x0A
#define SPI_READ		0x0B
#define READY_NOW		0x02

#define EVT_BLUE_INITIALIZED	0x01

/* Offsets */
#define STATUS_HEADER_READY	0
#define STATUS_HEADER_TOREAD	3

#define PACKET_TYPE		0
#define EVT_HEADER_TYPE		0
#define EVT_HEADER_EVENT	1
#define EVT_HEADER_SIZE		2
#define EVT_VENDOR_CODE_LSB	3
#define EVT_VENDOR_CODE_MSB	4

#define CMD_OGF			1
#define CMD_OCF			2

#define GPIO_IRQ_PIN		CONFIG_BLUETOOTH_SPI_IRQ_PIN
#define GPIO_RESET_PIN		CONFIG_BLUETOOTH_SPI_RESET_PIN
#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
#define GPIO_CS_PIN		CONFIG_BLUETOOTH_SPI_CHIP_SELECT_PIN
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */

/* Max SPI buffer length for transceive operations.
 *
 * Buffer size needs to be at least the size of the larger RX/TX buffer
 * required by the SPI slave, as spi_transceive requires both RX/TX
 * to be the same length. Size also needs to be compatible with the
 * slave device used (e.g. nRF5X max buffer length for SPIS is 255).
 */
#define SPI_MAX_MSG_LEN		255 /* As defined by X-NUCLEO-IDB04A1 BSP */

static uint8_t rxmsg[SPI_MAX_MSG_LEN];
static uint8_t txmsg[SPI_MAX_MSG_LEN];

static struct device		*spi_dev;
#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
static struct device		*cs_dev;
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */
static struct device		*irq_dev;
static struct device		*rst_dev;

static struct gpio_callback	gpio_cb;

static K_SEM_DEFINE(sem_initialised, 0, 1);
static K_SEM_DEFINE(sem_request, 0, 1);
static K_SEM_DEFINE(sem_busy, 1, 1);

static BT_STACK_NOINIT(rx_stack, 448);

static struct spi_config spi_conf = {
	.config = SPI_WORD(8),
	.max_sys_freq = CONFIG_BLUETOOTH_SPI_MAX_CLK_FREQ,
};

#if defined(CONFIG_BLUETOOTH_DEBUG_HCI_DRIVER)
#include <misc/printk.h>
static inline void spi_dump_message(const uint8_t *pre, uint8_t *buf,
				    uint8_t size)
{
	uint8_t i, c;

	printk("%s (%d): ", pre, size);
	for (i = 0; i < size; i++) {
		c = buf[i];
		printk("%x ", c);
		if (c >= 31 && c <= 126) {
			printk("[%c] ", c);
		} else {
			printk("[.] ");
		}
	}
	printk("\n");
}
#else
static inline
void spi_dump_message(const uint8_t *pre, uint8_t *buf, uint8_t size) {}
#endif

static inline uint16_t bt_spi_get_cmd(uint8_t *txmsg)
{
	return (txmsg[CMD_OCF] << 8) | txmsg[CMD_OGF];
}

static inline uint16_t bt_spi_get_evt(uint8_t *rxmsg)
{
	return (rxmsg[EVT_VENDOR_CODE_MSB] << 8) | rxmsg[EVT_VENDOR_CODE_LSB];
}

static void bt_spi_isr(struct device *unused1, struct gpio_callback *unused2,
			unsigned int unused3)
{
	k_sem_give(&sem_request);
}

static void bt_spi_handle_vendor_evt(uint8_t *rxmsg)
{
	switch (bt_spi_get_evt(rxmsg)) {
	case EVT_BLUE_INITIALIZED:
		k_sem_give(&sem_initialised);
	default:
		break;
	}
}

static void bt_spi_rx_thread(void)
{
	struct net_buf *buf;
	uint8_t header_master[5] = { SPI_READ, 0x00, 0x00, 0x00, 0x00 };
	uint8_t header_slave[5];
	struct bt_hci_acl_hdr acl_hdr;
	uint8_t size;

	memset(&txmsg, 0xFF, SPI_MAX_MSG_LEN);

	while (true) {
		k_sem_take(&sem_request, K_FOREVER);
		/* Disable IRQ pin callback to avoid spurious IRQs */
		gpio_pin_disable_callback(irq_dev, GPIO_IRQ_PIN);
		k_sem_take(&sem_busy, K_FOREVER);

		do {
#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
			gpio_pin_write(cs_dev, GPIO_CS_PIN, 1);
			gpio_pin_write(cs_dev, GPIO_CS_PIN, 0);
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */
			spi_transceive(spi_dev,
				       header_master, 5, header_slave, 5);
		} while (header_slave[STATUS_HEADER_TOREAD] == 0 ||
			 header_slave[STATUS_HEADER_TOREAD] == 0xFF);

		size = header_slave[STATUS_HEADER_TOREAD];
		do {
			spi_transceive(spi_dev, &txmsg, size, &rxmsg, size);
		} while (rxmsg[0] == 0);

		gpio_pin_enable_callback(irq_dev, GPIO_IRQ_PIN);
#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
		gpio_pin_write(cs_dev, GPIO_CS_PIN, 1);
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */
		k_sem_give(&sem_busy);

		spi_dump_message("RX:ed", rxmsg, size);

		switch (rxmsg[PACKET_TYPE]) {
		case HCI_EVT:
			/* Vendor events are currently unsupported */
			if (rxmsg[EVT_HEADER_EVENT] == BT_HCI_EVT_VENDOR) {
				bt_spi_handle_vendor_evt(rxmsg);
				continue;
			}

			buf = bt_buf_get_rx(K_FOREVER);
			bt_buf_set_type(buf, BT_BUF_EVT);
			net_buf_add_mem(buf, &rxmsg[1],
					rxmsg[EVT_HEADER_SIZE] + 2);
			break;
		case HCI_ACL:
			buf = bt_buf_get_rx(K_FOREVER);
			bt_buf_set_type(buf, BT_BUF_ACL_IN);
			memcpy(&acl_hdr, &rxmsg[1], sizeof(acl_hdr));
			net_buf_add_mem(buf, &acl_hdr, sizeof(acl_hdr));
			net_buf_add_mem(buf, &rxmsg[5],
					sys_le16_to_cpu(acl_hdr.len));
			break;
		default:
			BT_ERR("Unknown BT buf type %d", rxmsg[0]);
			continue;
		}

		if (rxmsg[PACKET_TYPE] == HCI_EVT &&
		    bt_hci_evt_is_prio(rxmsg[EVT_HEADER_EVENT])) {
			bt_recv_prio(buf);
		} else {
			bt_recv(buf);
		}
	}
}

static int bt_spi_send(struct net_buf *buf)
{
	uint8_t header[5] = { SPI_WRITE, 0x00,  0x00,  0x00,  0x00 };
	uint32_t pending;

	/* Buffer needs an additional byte for type */
	if (buf->len >= SPI_MAX_MSG_LEN) {
		BT_ERR("Message too long");
		return -EINVAL;
	}

	/* Allow time for the read thread to handle interrupt */
	while (true) {
		gpio_pin_read(irq_dev, GPIO_IRQ_PIN, &pending);
		if (!pending) {
			break;
		}
		k_sleep(1);
	}

	k_sem_take(&sem_busy, K_FOREVER);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, HCI_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, HCI_CMD);
		break;
	default:
		BT_ERR("Unsupported type");
		k_sem_give(&sem_busy);
		return -EINVAL;
	}

	/* Poll sanity values until device has woken-up */
	do {
#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
		gpio_pin_write(cs_dev, GPIO_CS_PIN, 1);
		gpio_pin_write(cs_dev, GPIO_CS_PIN, 0);
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */
		spi_transceive(spi_dev, header, 5, rxmsg, 5);

		/*
		 * RX Header (rxmsg) must contain a sanity check Byte and size
		 * information.  If it does not contain BOTH then it is
		 * sleeping or still in the initialisation stage (waking-up).
		 */
	} while (rxmsg[STATUS_HEADER_READY] != READY_NOW ||
		 (rxmsg[1] | rxmsg[2] | rxmsg[3] | rxmsg[4]) == 0);

	/* Transmit the message */
	do {
		spi_transceive(spi_dev, buf->data, buf->len, rxmsg, buf->len);
	} while (rxmsg[0] == 0);

#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
	/* Deselect chip */
	gpio_pin_write(cs_dev, GPIO_CS_PIN, 1);
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */
	k_sem_give(&sem_busy);

	spi_dump_message("TX:ed", buf->data, buf->len);

#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
	/*
	 * Since a RESET has been requested, the chip will now restart.
	 * Unfortunately the BlueNRG will reply with "reset received" but
	 * since it does not send back a NOP, we have no way to tell when the
	 * RESET has actually taken palce.  Instead, we use the vendor command
	 * EVT_BLUE_INITIALIZED as an indication that it is safe to proceed.
	 */
	if (bt_spi_get_cmd(buf->data) == BT_HCI_OP_RESET) {
		k_sem_take(&sem_initialised, K_FOREVER);
	}
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */

	net_buf_unref(buf);

	return 0;
}

static int bt_spi_open(void)
{
	/* Configure RST pin and hold BLE in Reset */
	gpio_pin_configure(rst_dev, GPIO_RESET_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(rst_dev, GPIO_RESET_PIN, 0);

	spi_configure(spi_dev, &spi_conf);

#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
	/* Configure the CS (Chip Select) pin */
	gpio_pin_configure(cs_dev, GPIO_CS_PIN,
			   GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(cs_dev, GPIO_CS_PIN, 1);
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */

	/* Configure IRQ pin and the IRQ call-back/handler */
	gpio_pin_configure(irq_dev, GPIO_IRQ_PIN,
			   GPIO_DIR_IN | GPIO_INT |
			   GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH);

	gpio_init_callback(&gpio_cb, bt_spi_isr, BIT(GPIO_IRQ_PIN));

	if (gpio_add_callback(irq_dev, &gpio_cb)) {
		return -EINVAL;
	}

	if (gpio_pin_enable_callback(irq_dev, GPIO_IRQ_PIN)) {
		return -EINVAL;
	}

	/* Start RX thread */
	k_thread_spawn(rx_stack, sizeof(rx_stack),
		       (k_thread_entry_t)bt_spi_rx_thread,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Take BLE out of reset */
	gpio_pin_write(rst_dev, GPIO_RESET_PIN, 1);

	/* Device will let us know when it's ready */
	k_sem_take(&sem_initialised, K_FOREVER);

	return 0;
}

static struct bt_hci_driver drv = {
	.name		= "BT SPI",
	.bus		= BT_HCI_DRIVER_BUS_SPI,
	.open		= bt_spi_open,
	.send		= bt_spi_send,
};

static int _bt_spi_init(struct device *unused)
{
	ARG_UNUSED(unused);

	spi_dev = device_get_binding(CONFIG_BLUETOOTH_SPI_DEV_NAME);
	if (!spi_dev) {
		BT_ERR("Failed to initialize SPI driver: %s",
		       CONFIG_BLUETOOTH_SPI_DEV_NAME);
		return -EIO;
	}

#if defined(CONFIG_BLUETOOTH_SPI_BLUENRG)
	cs_dev = device_get_binding(CONFIG_BLUETOOTH_SPI_CHIP_SELECT_DEV_NAME);
	if (!cs_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       CONFIG_BLUETOOTH_SPI_CHIP_SELECT_DEV_NAME);
		return -EIO;
	}
#endif /* CONFIG_BLUETOOTH_SPI_BLUENRG */

	irq_dev = device_get_binding(CONFIG_BLUETOOTH_SPI_IRQ_DEV_NAME);
	if (!irq_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       CONFIG_BLUETOOTH_SPI_IRQ_DEV_NAME);
		return -EIO;
	}

	rst_dev = device_get_binding(CONFIG_BLUETOOTH_SPI_RESET_DEV_NAME);
	if (!rst_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       CONFIG_BLUETOOTH_SPI_RESET_DEV_NAME);
		return -EIO;
	}

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(_bt_spi_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
