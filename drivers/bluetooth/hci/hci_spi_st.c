/* hci_spi_st.c - STMicroelectronics HCI SPI Bluetooth driver */

/*
 * Copyright (c) 2017 Linaro Ltd.
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_DT_HAS_ST_HCI_SPI_V1_ENABLED)
#define DT_DRV_COMPAT st_hci_spi_v1

#elif defined(CONFIG_DT_HAS_ST_HCI_SPI_V2_ENABLED)
#define DT_DRV_COMPAT st_hci_spi_v2

#endif /* CONFIG_DT_HAS_ST_HCI_SPI_V1_ENABLED */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/bluetooth/hci_raw.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04
/* ST Proprietary extended event */
#define HCI_EXT_EVT		0x82

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
/* packet type (1) + opcode (2) + Parameter Total Length (1) + max parameter length (255) */
#define SPI_MAX_MSG_LEN		259

/* Single byte header denoting the buffer type */
#define H4_HDR_SIZE 1

/* Maximum L2CAP MTU that can fit in a single packet */
#define MAX_MTU (SPI_MAX_MSG_LEN - H4_HDR_SIZE - BT_L2CAP_HDR_SIZE - BT_HCI_ACL_HDR_SIZE)

#if CONFIG_BT_L2CAP_TX_MTU > MAX_MTU
#warning CONFIG_BT_L2CAP_TX_MTU is too large and can result in packets that cannot \
	be transmitted across this HCI link
#endif /* CONFIG_BT_L2CAP_TX_MTU > MAX_MTU */

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

#define BLUENRG_ACI_WRITE_CONFIG_DATA       BT_OP(BT_OGF_VS, 0x000C)
#define BLUENRG_CONFIG_PUBADDR_OFFSET       0x00
#define BLUENRG_CONFIG_PUBADDR_LEN          0x06
#define BLUENRG_CONFIG_LL_ONLY_OFFSET       0x2C
#define BLUENRG_CONFIG_LL_ONLY_LEN          0x01

static int bt_spi_send_aci_config(uint8_t offset, const uint8_t *value, size_t value_len);

static const struct spi_dt_spec bus = SPI_DT_SPEC_INST_GET(
	0, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LOCK_ON, 0);

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

struct bt_hci_ext_evt_hdr {
	uint8_t evt;
	uint16_t len;
} __packed;

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
#if defined(CONFIG_BT_BLUENRG_ACI)
		handled = true;
#endif
	}
	default:
		break;
	}
	return handled;
}

#define IS_IRQ_HIGH gpio_pin_get_dt(&irq_gpio)

#if DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1)

/* Define a limit when reading IRQ high */
#define IRQ_HIGH_MAX_READ 15

/* On BlueNRG-MS, host is expected to read */
/* as long as IRQ pin is high */
#define READ_CONDITION IS_IRQ_HIGH

static void release_cs(bool data_transaction)
{
	ARG_UNUSED(data_transaction);
	spi_release_dt(&bus);
}

static int bt_spi_get_header(uint8_t op, uint16_t *size)
{
	uint8_t header_master[5] = {op, 0, 0, 0, 0};
	uint8_t header_slave[5];
	uint8_t size_offset, attempts;
	int ret;

	if (op == SPI_READ) {
		if (!IS_IRQ_HIGH) {
			*size = 0;
			return 0;
		}
		size_offset = STATUS_HEADER_TOREAD;
	} else if (op == SPI_WRITE) {
		size_offset = STATUS_HEADER_TOWRITE;
	} else {
		return -EINVAL;
	}
	attempts = IRQ_HIGH_MAX_READ;
	do {
		if (op == SPI_READ) {
			/* Keep checking that IRQ is still high, if we need to read */
			if (!IS_IRQ_HIGH) {
				*size = 0;
				return 0;
			}
		}
		/* Make sure CS is raised before a new attempt */
		gpio_pin_set_dt(&bus.config.cs.gpio, 0);
		ret = bt_spi_transceive(header_master, 5, header_slave, 5);
		if (ret) {
			/* SPI transaction failed */
			break;
		}

		*size = (header_slave[STATUS_HEADER_READY] == READY_NOW) ?
				header_slave[size_offset] : 0;
		attempts--;
	} while ((*size == 0) && attempts);

	return ret;
}

#elif DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2)

#define READ_CONDITION false

static void release_cs(bool data_transaction)
{
	/* Consume possible event signals */
	while (k_sem_take(&sem_request, K_NO_WAIT) == 0) {
	}
	if (data_transaction) {
		/* Wait for IRQ to become low only when data phase has been performed */
		while (IS_IRQ_HIGH) {
		}
	}
	gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	spi_release_dt(&bus);
}

static int bt_spi_get_header(uint8_t op, uint16_t *size)
{
	uint8_t header_master[5] = {op, 0, 0, 0, 0};
	uint8_t header_slave[5];
	uint16_t cs_delay;
	uint8_t size_offset;
	int ret;

	if (op == SPI_READ) {
		if (!IS_IRQ_HIGH) {
			*size = 0;
			return 0;
		}
		cs_delay = 0;
		size_offset = STATUS_HEADER_TOREAD;
	} else if (op == SPI_WRITE) {
		/* To make sure we have a minimum delay from previous release cs */
		cs_delay = 100;
		size_offset = STATUS_HEADER_TOWRITE;
	} else {
		return -EINVAL;
	}

	if (cs_delay) {
		k_sleep(K_USEC(cs_delay));
	}
	/* Perform a zero byte SPI transaction to acquire the SPI lock and lower CS
	 * while waiting for IRQ to be raised
	 */
	bt_spi_transceive(header_master, 0, header_slave, 0);
	gpio_pin_interrupt_configure_dt(&irq_gpio, GPIO_INT_DISABLE);

	/* Wait up to a maximum time of 100 ms */
	if (!WAIT_FOR(IS_IRQ_HIGH, 100000, k_usleep(100))) {
		LOG_ERR("IRQ pin did not raise");
		return -EIO;
	}

	ret = bt_spi_transceive(header_master, 5, header_slave, 5);
	*size = header_slave[size_offset] | (header_slave[size_offset + 1] << 8);
	return ret;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1) */

#if defined(CONFIG_BT_BLUENRG_ACI)
static int bt_spi_send_aci_config(uint8_t offset, const uint8_t *value, size_t value_len)
{
	struct net_buf *buf;
	uint8_t *cmd_data;
	size_t data_len = 2 + value_len;
#if defined(CONFIG_BT_HCI_RAW)
	struct bt_hci_cmd_hdr hdr;

	hdr.opcode = sys_cpu_to_le16(BLUENRG_ACI_WRITE_CONFIG_DATA);
	hdr.param_len = data_len;
	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, &hdr, sizeof(hdr));
#else
	buf = bt_hci_cmd_create(BLUENRG_ACI_WRITE_CONFIG_DATA, data_len);
#endif /* CONFIG_BT_HCI_RAW */

	if (!buf) {
		return -ENOBUFS;
	}

	cmd_data = net_buf_add(buf, data_len);
	cmd_data[0] = offset;
	cmd_data[1] = value_len;
	memcpy(&cmd_data[2], value, value_len);

#if defined(CONFIG_BT_HCI_RAW)
	return bt_send(buf);
#else
	return bt_hci_cmd_send(BLUENRG_ACI_WRITE_CONFIG_DATA, buf);
#endif /* CONFIG_BT_HCI_RAW */
}

#if !defined(CONFIG_BT_HCI_RAW)
static int bt_spi_bluenrg_setup(const struct bt_hci_setup_params *params)
{
	int ret;
	const bt_addr_t *addr = &params->public_addr;

	/* force BlueNRG to be on controller mode */
	uint8_t data = 1;

	bt_spi_send_aci_config(BLUENRG_CONFIG_LL_ONLY_OFFSET, &data, 1);

	if (!bt_addr_eq(addr, BT_ADDR_NONE) && !bt_addr_eq(addr, BT_ADDR_ANY)) {
		ret = bt_spi_send_aci_config(
			BLUENRG_CONFIG_PUBADDR_OFFSET,
			addr->val, sizeof(addr->val));

		if (ret != 0) {
			LOG_ERR("Failed to set BlueNRG public address (%d)", ret);
			return ret;
		}
	}

	return 0;
}
#endif /* !CONFIG_BT_HCI_RAW */

#endif /* CONFIG_BT_BLUENRG_ACI */

static struct net_buf *bt_spi_rx_buf_construct(uint8_t *msg)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;
	struct bt_hci_acl_hdr acl_hdr;
	struct net_buf *buf;
	int len;

	switch (msg[PACKET_TYPE]) {
#if DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2)
	case HCI_EXT_EVT:
		struct bt_hci_ext_evt_hdr *evt = (struct bt_hci_ext_evt_hdr *) (msg + 1);
		struct bt_hci_evt_hdr *evt2 = (struct bt_hci_evt_hdr *) (msg + 1);

		if (evt->len > 0xff) {
			return NULL;
		}
		/* Use memmove instead of memcpy due to buffer overlapping */
		memmove(msg + (1 + sizeof(*evt2)), msg + (1 + sizeof(*evt)), evt2->len);
		/* Manage event as regular HCI_EVT */
		__fallthrough;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2) */
	case HCI_EVT:
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
	case HCI_ACL:
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
	ARG_UNUSED(p1);
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

		do {
			/* Wait for SPI bus to be available */
			k_sem_take(&sem_busy, K_FOREVER);
			ret = bt_spi_get_header(SPI_READ, &size);

			/* Read data */
			if (ret == 0 && size != 0) {
				ret = bt_spi_transceive(&txmsg, size, &rxmsg, size);
			}

			release_cs(size > 0);

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
				bt_recv(buf);
			}
		} while (READ_CONDITION);
	}
}

static int bt_spi_send(struct net_buf *buf)
{
	uint16_t size;
	uint8_t rx_first[1];
	int ret;
	uint8_t *data_ptr;
	uint16_t remaining_bytes;

	LOG_DBG("");

	/* Buffer needs an additional byte for type */
	if (buf->len >= SPI_MAX_MSG_LEN) {
		LOG_ERR("Message too long (%d)", buf->len);
		return -EINVAL;
	}

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, HCI_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, HCI_CMD);
		break;
	default:
		LOG_ERR("Unsupported type");
		return -EINVAL;
	}

	/* Wait for SPI bus to be available */
	k_sem_take(&sem_busy, K_FOREVER);
	data_ptr = buf->data;
	remaining_bytes = buf->len;
	do {
		ret = bt_spi_get_header(SPI_WRITE, &size);
		size = MIN(remaining_bytes, size);

#if DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2)

		if (size < remaining_bytes) {
			LOG_WRN("Unable to write full data, skipping");
			size = 0;
			ret = -ECANCELED;
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v2) */

		if (!ret) {
			/* Transmit the message */
			ret = bt_spi_transceive(data_ptr, size, rx_first, 1);
		}
		remaining_bytes -= size;
		data_ptr += size;
	} while (remaining_bytes > 0 && !ret);

	release_cs(size > 0);

	k_sem_give(&sem_busy);

	if (ret) {
		LOG_ERR("Error %d", ret);
		return ret;
	}

	LOG_HEXDUMP_DBG(buf->data, buf->len, "SPI TX");

#if DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1)
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
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_hci_spi_v1) */
	net_buf_unref(buf);

	return ret;
}

static int bt_spi_open(void)
{
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

	/* Take BLE out of reset */
	k_sleep(K_MSEC(DT_INST_PROP_OR(0, reset_assert_duration_ms, 0)));
	gpio_pin_set_dt(&rst_gpio, 0);

	/* Start RX thread */
	k_thread_create(&spi_rx_thread_data, spi_rx_stack,
			K_KERNEL_STACK_SIZEOF(spi_rx_stack),
			bt_spi_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	/* Device will let us know when it's ready */
	k_sem_take(&sem_initialised, K_FOREVER);

#if defined(CONFIG_BT_HCI_RAW) && defined(CONFIG_BT_BLUENRG_ACI)
	/* force BlueNRG to be on controller mode */
	uint8_t data = 1;

	bt_spi_send_aci_config(BLUENRG_CONFIG_LL_ONLY_OFFSET, &data, 1);
#endif /* CONFIG_BT_HCI_RAW && CONFIG_BT_BLUENRG_ACI */
	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= DEVICE_DT_NAME(DT_DRV_INST(0)),
	.bus		= BT_HCI_DRIVER_BUS_SPI,
	.quirks		= BT_QUIRK_NO_RESET,
#if defined(CONFIG_BT_BLUENRG_ACI) && !defined(CONFIG_BT_HCI_RAW)
	.setup          = bt_spi_bluenrg_setup,
#endif /* CONFIG_BT_BLUENRG_ACI && !CONFIG_BT_HCI_RAW */
	.open		= bt_spi_open,
	.send		= bt_spi_send,
};

static int bt_spi_init(void)
{

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

	bt_hci_driver_register(&drv);


	LOG_DBG("BT SPI initialized");

	return 0;
}

SYS_INIT(bt_spi_init, POST_KERNEL, CONFIG_BT_SPI_INIT_PRIORITY);
