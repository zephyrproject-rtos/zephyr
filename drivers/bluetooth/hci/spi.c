/* spi.c - SPI based Bluetooth driver */

#define DT_DRV_COMPAT zephyr_bt_hci_spi

/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <init.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_driver
#include "common/log.h"

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

#define GPIO_IRQ_PIN		DT_INST_GPIO_PIN(0, irq_gpios)
#define GPIO_IRQ_FLAGS		DT_INST_GPIO_FLAGS(0, irq_gpios)
#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_RESET_FLAGS	DT_INST_GPIO_FLAGS(0, reset_gpios)
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define GPIO_CS_FLAGS		DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

/* Max SPI buffer length for transceive operations.
 *
 * Buffer size needs to be at least the size of the larger RX/TX buffer
 * required by the SPI slave, as the legacy spi_transceive requires both RX/TX
 * to be the same length. Size also needs to be compatible with the
 * slave device used (e.g. nRF5X max buffer length for SPIS is 255).
 */
#define SPI_MAX_MSG_LEN		255 /* As defined by X-NUCLEO-IDB04A1 BSP */

static uint8_t rxmsg[SPI_MAX_MSG_LEN];
static uint8_t txmsg[SPI_MAX_MSG_LEN];

static const struct device *irq_dev;
static const struct device *rst_dev;

static struct gpio_callback	gpio_cb;

static K_SEM_DEFINE(sem_initialised, 0, 1);
static K_SEM_DEFINE(sem_request, 0, 1);
static K_SEM_DEFINE(sem_busy, 1, 1);

static K_KERNEL_STACK_DEFINE(spi_rx_stack, 256);
static struct k_thread spi_rx_thread_data;

#if defined(CONFIG_BT_DEBUG_HCI_DRIVER)
#include <sys/printk.h>
static inline void spi_dump_message(const uint8_t *pre, uint8_t *buf,
				    uint8_t size)
{
	uint8_t i, c;

	printk("%s (%d): ", pre, size);
	for (i = 0U; i < size; i++) {
		c = buf[i];
		printk("%x ", c);
		if (c >= 31U && c <= 126U) {
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

#if defined(CONFIG_BT_SPI_BLUENRG)
static const struct device *cs_dev;
/* Define a limit when reading IRQ high */
/* It can be required to be increased for */
/* some particular cases. */
#define IRQ_HIGH_MAX_READ 3
static uint8_t attempts;
#endif /* CONFIG_BT_SPI_BLUENRG */

#if defined(CONFIG_BT_BLUENRG_ACI)
#define BLUENRG_ACI_WRITE_CONFIG_DATA       BT_OP(BT_OGF_VS, 0x000C)
#define BLUENRG_ACI_WRITE_CONFIG_CMD_LL     0x2C
#define BLUENRG_ACI_LL_MODE                 0x01

struct bluenrg_aci_cmd_ll_param {
    uint8_t cmd;
    uint8_t length;
    uint8_t value;
};
static int bt_spi_send_aci_config_data_controller_mode(void);
#endif /* CONFIG_BT_BLUENRG_ACI */

static const struct device *spi_dev;

static struct spi_config spi_conf = {
	.frequency = DT_INST_PROP(0, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) |
		      SPI_LINES_SINGLE),
	.slave     = 0,
	.cs        = NULL,
};
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
	return spi_transceive(spi_dev, &spi_conf, &spi_tx, &spi_rx);
}

static inline uint16_t bt_spi_get_cmd(uint8_t *txmsg)
{
	return (txmsg[CMD_OCF] << 8) | txmsg[CMD_OGF];
}

static inline uint16_t bt_spi_get_evt(uint8_t *rxmsg)
{
	return (rxmsg[EVT_VENDOR_CODE_MSB] << 8) | rxmsg[EVT_VENDOR_CODE_LSB];
}

static void bt_spi_isr(const struct device *unused1,
		       struct gpio_callback *unused2,
		       uint32_t unused3)
{
	BT_DBG("");

	k_sem_give(&sem_request);
}

static void bt_spi_handle_vendor_evt(uint8_t *rxmsg)
{
	switch (bt_spi_get_evt(rxmsg)) {
	case EVT_BLUE_INITIALIZED:
		k_sem_give(&sem_initialised);
#if defined(CONFIG_BT_BLUENRG_ACI)
		/* force BlueNRG to be on controller mode */
		bt_spi_send_aci_config_data_controller_mode();
#endif
	default:
		break;
	}
}

#if defined(CONFIG_BT_SPI_BLUENRG)
/* BlueNRG has a particuliar way to wake up from sleep and be ready.
 * All is done through its CS line:
 * If it is in sleep mode, the first transaction will not return ready
 * status. At this point, it's necessary to release the CS and retry
 * within 2ms the same transaction. And again when it's required to
 * know the amount of byte to read.
 * (See section 5.2 of BlueNRG-MS datasheet)
 */
static int configure_cs(void)
{
	cs_dev = device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!cs_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
		return -EIO;
	}

	/* Configure pin as output and set to active */
	gpio_pin_configure(cs_dev, GPIO_CS_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_CS_FLAGS);


	return 0;
}

static void kick_cs(void)
{
	gpio_pin_set(cs_dev, GPIO_CS_PIN, 1);
	gpio_pin_set(cs_dev, GPIO_CS_PIN, 0);
}

static void release_cs(void)
{
	gpio_pin_set(cs_dev, GPIO_CS_PIN, 1);
}

static bool irq_pin_high(void)
{
	int pin_state;

	pin_state = gpio_pin_get(irq_dev, GPIO_IRQ_PIN);

	BT_DBG("IRQ Pin: %d", pin_state);

	return pin_state > 0;
}

static void init_irq_high_loop(void)
{
	attempts = IRQ_HIGH_MAX_READ;
}

static bool exit_irq_high_loop(void)
{
	/* Limit attempts on BlueNRG-MS as we might */
	/* enter this loop with nothing to read */

	attempts--;

	return attempts;
}

#else

static int configure_cs(void)
{
#ifdef GPIO_CS_PIN
	static struct spi_cs_control spi_conf_cs;

	spi_conf_cs.gpio_pin = GPIO_CS_PIN;
	spi_conf_cs.gpio_dt_flags = GPIO_CS_FLAGS;
	spi_conf_cs.gpio_dev = device_get_binding(
		DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!spi_conf_cs.gpio_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
		return -EIO;
	}

	spi_conf.cs = &spi_conf_cs;
#endif /* GPIO_CS_PIN */

	return 0;
}
#define kick_cs(...)
#define release_cs(...)
#define irq_pin_high(...) 0
#define init_irq_high_loop(...)
#define exit_irq_high_loop(...) 1

#endif /* CONFIG_BT_SPI_BLUENRG */

#if defined(CONFIG_BT_BLUENRG_ACI)
static int bt_spi_send_aci_config_data_controller_mode(void)
{
	struct bluenrg_aci_cmd_ll_param *param;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BLUENRG_ACI_WRITE_CONFIG_DATA, sizeof(*param));
	if (!buf) {
		return -ENOBUFS;
	}

	param = net_buf_add(buf, sizeof(*param));
	param->cmd = BLUENRG_ACI_WRITE_CONFIG_CMD_LL;
	param->length = 0x1;
	/* Force BlueNRG-MS roles to Link Layer only mode */
	param->value = BLUENRG_ACI_LL_MODE;

	bt_hci_cmd_send(BLUENRG_ACI_WRITE_CONFIG_DATA, buf);

	return 0;
}
#endif /* CONFIG_BT_BLUENRG_ACI */

static void bt_spi_rx_thread(void)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;
	struct net_buf *buf;
	uint8_t header_master[5] = { SPI_READ, 0x00, 0x00, 0x00, 0x00 };
	uint8_t header_slave[5];
	struct bt_hci_acl_hdr acl_hdr;
	uint8_t size = 0U;
	int ret;

	(void)memset(&txmsg, 0xFF, SPI_MAX_MSG_LEN);

	while (true) {
		k_sem_take(&sem_request, K_FOREVER);
		/* Disable IRQ pin callback to avoid spurious IRQs */

		gpio_pin_interrupt_configure(irq_dev, GPIO_IRQ_PIN,
					     GPIO_INT_DISABLE);
		k_sem_take(&sem_busy, K_FOREVER);

		BT_DBG("");

		do {
			init_irq_high_loop();
			do {
				kick_cs();
				ret = bt_spi_transceive(header_master, 5,
							header_slave, 5);
			} while ((((header_slave[STATUS_HEADER_TOREAD] == 0U ||
				    header_slave[STATUS_HEADER_TOREAD] == 0xFF) &&
				   !ret)) && exit_irq_high_loop());

			size = header_slave[STATUS_HEADER_TOREAD];
			if (!ret || size != 0) {
				do {
					ret = bt_spi_transceive(&txmsg, size,
								&rxmsg, size);
				} while (rxmsg[0] == 0U && ret == 0);
			}

			release_cs();
			gpio_pin_interrupt_configure(irq_dev, GPIO_IRQ_PIN,
						     GPIO_INT_EDGE_TO_ACTIVE);

			k_sem_give(&sem_busy);

			if (ret || size == 0) {
				if (ret) {
					BT_ERR("Error %d", ret);
				}
				continue;
			}

			spi_dump_message("RX:ed", rxmsg, size);

			switch (rxmsg[PACKET_TYPE]) {
			case HCI_EVT:
				switch (rxmsg[EVT_HEADER_EVENT]) {
				case BT_HCI_EVT_VENDOR:
					/* Vendor events are currently unsupported */
					bt_spi_handle_vendor_evt(rxmsg);
					continue;
				default:
					if (rxmsg[1] == BT_HCI_EVT_LE_META_EVENT &&
					    (rxmsg[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT ||
					     rxmsg[3] == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT)) {
						discardable = true;
						timeout = K_NO_WAIT;
					}

					buf = bt_buf_get_evt(rxmsg[EVT_HEADER_EVENT],
							     discardable, timeout);
					if (!buf) {
						BT_DBG("Discard adv report due to insufficient buf");
						continue;
					}
				}

				net_buf_add_mem(buf, &rxmsg[1],
						rxmsg[EVT_HEADER_SIZE] + 2);
				break;
			case HCI_ACL:
				buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
				memcpy(&acl_hdr, &rxmsg[1], sizeof(acl_hdr));
				net_buf_add_mem(buf, &acl_hdr, sizeof(acl_hdr));
				net_buf_add_mem(buf, &rxmsg[5],
						sys_le16_to_cpu(acl_hdr.len));
				break;
			default:
				BT_ERR("Unknown BT buf type %d", rxmsg[0]);
				continue;
			}

			bt_recv(buf);

		/* On BlueNRG-MS, host is expected to read */
		/* as long as IRQ pin is high */
		} while (irq_pin_high());
	}
}

static int bt_spi_send(struct net_buf *buf)
{
	uint8_t header[5] = { SPI_WRITE, 0x00,  0x00,  0x00,  0x00 };
	int pending;
	int ret;

	BT_DBG("");

	/* Buffer needs an additional byte for type */
	if (buf->len >= SPI_MAX_MSG_LEN) {
		BT_ERR("Message too long");
		return -EINVAL;
	}

	/* Allow time for the read thread to handle interrupt */
	while (true) {
		pending = gpio_pin_get(irq_dev, GPIO_IRQ_PIN);
		if (pending <= 0) {
			break;
		}
		k_sleep(K_MSEC(1));
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
		kick_cs();
		ret = bt_spi_transceive(header, 5, rxmsg, 5);

		/*
		 * RX Header (rxmsg) must contain a sanity check Byte and size
		 * information.  If it does not contain BOTH then it is
		 * sleeping or still in the initialisation stage (waking-up).
		 */
	} while ((rxmsg[STATUS_HEADER_READY] != READY_NOW ||
		  (rxmsg[1] | rxmsg[2] | rxmsg[3] | rxmsg[4]) == 0U) && !ret);


	k_sem_give(&sem_busy);

	if (!ret) {
		/* Transmit the message */
		do {
			ret = bt_spi_transceive(buf->data, buf->len,
						rxmsg, buf->len);
		} while (rxmsg[0] == 0U && !ret);
	}

	release_cs();

	if (ret) {
		BT_ERR("Error %d", ret);
		goto out;
	}

	spi_dump_message("TX:ed", buf->data, buf->len);

#if defined(CONFIG_BT_SPI_BLUENRG)
	/*
	 * Since a RESET has been requested, the chip will now restart.
	 * Unfortunately the BlueNRG will reply with "reset received" but
	 * since it does not send back a NOP, we have no way to tell when the
	 * RESET has actually taken place.  Instead, we use the vendor command
	 * EVT_BLUE_INITIALIZED as an indication that it is safe to proceed.
	 */
	if (bt_spi_get_cmd(buf->data) == BT_HCI_OP_RESET) {
		k_sem_take(&sem_initialised, K_FOREVER);
	}
#endif /* CONFIG_BT_SPI_BLUENRG */
out:
	net_buf_unref(buf);

	return ret;
}

static int bt_spi_open(void)
{
	/* Configure RST pin and hold BLE in Reset */
	gpio_pin_configure(rst_dev, GPIO_RESET_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_RESET_FLAGS);

	/* Configure IRQ pin and the IRQ call-back/handler */
	gpio_pin_configure(irq_dev, GPIO_IRQ_PIN,
			   GPIO_INPUT | GPIO_IRQ_FLAGS);

	gpio_init_callback(&gpio_cb, bt_spi_isr, BIT(GPIO_IRQ_PIN));

	if (gpio_add_callback(irq_dev, &gpio_cb)) {
		return -EINVAL;
	}

	gpio_pin_interrupt_configure(irq_dev, GPIO_IRQ_PIN,
				     GPIO_INT_EDGE_TO_ACTIVE);

	/* Start RX thread */
	k_thread_create(&spi_rx_thread_data, spi_rx_stack,
			K_KERNEL_STACK_SIZEOF(spi_rx_stack),
			(k_thread_entry_t)bt_spi_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	/* Take BLE out of reset */
	gpio_pin_set(rst_dev, GPIO_RESET_PIN, 0);

	/* Device will let us know when it's ready */
	k_sem_take(&sem_initialised, K_FOREVER);

	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= DT_INST_LABEL(0),
	.bus		= BT_HCI_DRIVER_BUS_SPI,
#if defined(CONFIG_BT_BLUENRG_ACI)
	.quirks		= BT_QUIRK_NO_RESET,
#endif /* CONFIG_BT_BLUENRG_ACI */
	.open		= bt_spi_open,
	.send		= bt_spi_send,
};

static int bt_spi_init(const struct device *unused)
{
	ARG_UNUSED(unused);

	spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!spi_dev) {
		BT_ERR("Failed to initialize SPI driver: %s",
		       DT_INST_BUS_LABEL(0));
		return -EIO;
	}

	if (configure_cs()) {
		return -EIO;
	}

	irq_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, irq_gpios));
	if (!irq_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       DT_INST_GPIO_LABEL(0, irq_gpios));
		return -EIO;
	}

	rst_dev = device_get_binding(
		DT_INST_GPIO_LABEL(0, reset_gpios));
	if (!rst_dev) {
		BT_ERR("Failed to initialize GPIO driver: %s",
		       DT_INST_GPIO_LABEL(0, reset_gpios));
		return -EIO;
	}

	bt_hci_driver_register(&drv);


	BT_DBG("BT SPI initialized");

	return 0;
}

SYS_INIT(bt_spi_init, POST_KERNEL, CONFIG_BT_SPI_INIT_PRIORITY);
