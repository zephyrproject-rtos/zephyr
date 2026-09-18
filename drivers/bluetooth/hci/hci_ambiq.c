/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Ambiq SPI based Bluetooth HCI driver.
 */

#define DT_DRV_COMPAT ambiq_bt_hci_spi

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/bluetooth/hci_lockstep.h>
#include <zephyr/bluetooth/hci.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver);

#include "apollox_blue.h"

/* Offset of special item */
#define PACKET_TYPE         0
#define PACKET_TYPE_SIZE    1
#define EVT_HEADER_TYPE     0
#define EVT_CMD_COMP_OP_LSB 3
#define EVT_CMD_COMP_OP_MSB 4
#define EVT_CMD_COMP_DATA   5

#define EVT_OK      0
#define EVT_DISCARD 1
#define EVT_NOP     2

#define BT_FEAT_SET_BIT(feat, octet, bit) (feat[octet] |= BIT(bit))
#define BT_FEAT_SET_NO_BREDR(feat)        BT_FEAT_SET_BIT(feat, 4, 5)
#define BT_FEAT_SET_LE(feat)              BT_FEAT_SET_BIT(feat, 4, 6)

/* Max SPI buffer length for transceive operations.
 * The maximum TX packet number is 512 bytes data + 12 bytes header.
 * The maximum RX packet number is 255 bytes data + 3 header.
 */
#define SPI_MAX_TX_MSG_LEN 524
#define SPI_MAX_RX_MSG_LEN 258

/* The controller may be unavailable to receive packets because it is busy
 * on processing something or have packets to send to host. Need to free the
 * SPI bus and wait some moment to try again.
 */
#define SPI_BUSY_WAIT_INTERVAL_MS 25
#define SPI_BUSY_TX_ATTEMPTS      200

static uint8_t __noinit rxmsg[SPI_MAX_RX_MSG_LEN];

static struct spi_dt_spec spi_bus =
	SPI_DT_SPEC_INST_GET(0,
			     SPI_OP_MODE_CONTROLLER | SPI_HALF_DUPLEX | SPI_TRANSFER_MSB |
				     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8));

static K_KERNEL_STACK_DEFINE(spi_rx_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread spi_rx_thread_data;

static struct spi_buf spi_tx_buf;
static struct spi_buf spi_rx_buf;
static const struct spi_buf_set spi_tx = {.buffers = &spi_tx_buf, .count = 1};
static const struct spi_buf_set spi_rx = {.buffers = &spi_rx_buf, .count = 1};

static K_SEM_DEFINE(sem_irq, 0, 1);
static K_SEM_DEFINE(sem_spi_available, 1, 1);

/* Whether the transport is open, and a count of how often that has changed.
 * open() and close() update both while holding sem_spi_available, so that a
 * send finds out, once it owns the semaphore, that the controller is down, or
 * that it has been down since the send began although the transport is open
 * again. The count is atomic because a send notes it before it has the
 * semaphore: what counts is the session the call began in, not the one in
 * which it first gets the semaphore.
 */
static bool transport_open;
static atomic_t transport_session;

void bt_packet_irq_isr(const struct device *unused1, struct gpio_callback *unused2,
		       uint32_t unused3)
{
	bt_apollo_rcv_isr_preprocess();
	k_sem_give(&sem_irq);
}

static inline int bt_spi_transceive(void *tx, uint32_t tx_len, void *rx, uint32_t rx_len)
{
	spi_tx_buf.buf = tx;
	spi_tx_buf.len = (size_t)tx_len;
	spi_rx_buf.buf = rx;
	spi_rx_buf.len = (size_t)rx_len;

	/* Before sending packet to controller the host needs to poll the status of
	 * controller to know it's ready, or before reading packets from controller
	 * the host needs to get the payload size of coming packets by sending specific
	 * command and putting the status or size to the rx buffer, the CS should be
	 * held at this moment to continue to send or receive packets.
	 */
	if (tx_len && rx_len) {
		spi_bus.config.operation |= SPI_HOLD_ON_CS;
	} else {
		spi_bus.config.operation &= ~SPI_HOLD_ON_CS;
	}
	return spi_transceive_dt(&spi_bus, &spi_tx, &spi_rx);
}

struct ambiq_data {
	/* bt_hci_driver_data must be first */
	struct bt_hci_driver_data common;
	struct bt_hci_lockstep lockstep;
};

static int spi_send_packet(uint8_t *data, uint16_t len)
{
	atomic_val_t session = atomic_get(&transport_session);
	int ret;
	uint16_t fail_count = 0;

	do {
		/* Wait for SPI bus to be available */
		k_sem_take(&sem_spi_available, K_FOREVER);

		/* The semaphore is free between the attempts, so close() may
		 * have run while this call waited for it or slept.
		 */
		if (!transport_open || atomic_get(&transport_session) != session) {
			k_sem_give(&sem_spi_available);
			return -ENETDOWN;
		}

		/* Send the SPI packet to controller */
		ret = bt_apollo_spi_send(data, len, bt_spi_transceive);

		/* Free the SPI bus */
		k_sem_give(&sem_spi_available);

		if (ret) {
			/* Give some chance to controller to complete the processing or
			 * packets sending.
			 */
			k_sleep(K_MSEC(SPI_BUSY_WAIT_INTERVAL_MS));
		} else {
			break;
		}
	} while (fail_count++ < SPI_BUSY_TX_ATTEMPTS);

	return ret;
}

static int spi_receive_packet(uint8_t *data, uint16_t *len)
{
	int ret;

	/* Wait for SPI bus to be available */
	k_sem_take(&sem_spi_available, K_FOREVER);

	/* Receive the SPI packet from controller */
	ret = bt_apollo_spi_rcv(data, len, bt_spi_transceive);

	/* Free the SPI bus */
	k_sem_give(&sem_spi_available);

	return ret;
}

static int hci_event_filter(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[EVT_HEADER_TYPE];

	switch (evt_type) {
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return EVT_DISCARD;
		default:
			return EVT_OK;
		}
	}
	case BT_HCI_EVT_CMD_COMPLETE: {
		uint16_t opcode = (uint16_t)(evt_data[EVT_CMD_COMP_OP_LSB] +
					     (evt_data[EVT_CMD_COMP_OP_MSB] << 8));

		switch (opcode) {
		case BT_OP_NOP:
			return EVT_NOP;
		case BT_HCI_OP_READ_LOCAL_FEATURES: {
			/* The BLE controller of some Ambiq Apollox Blue SOC may have issue to
			 * report the expected supported features bitmask successfully, thought the
			 * features are actually supportive. Need to correct them before going to
			 * the host stack.
			 */
			struct bt_hci_rp_read_local_features *rp =
				(void *)&evt_data[EVT_CMD_COMP_DATA];
			if (rp->status == 0) {
				BT_FEAT_SET_NO_BREDR(rp->features);
				BT_FEAT_SET_LE(rp->features);
			}
			return EVT_OK;
		}
		default:
			return EVT_OK;
		}
	}
	default:
		return EVT_OK;
	}
}

static struct net_buf *bt_hci_evt_recv(uint8_t *data, size_t len)
{
	int evt_filter;
	bool discardable = false;
	struct bt_hci_evt_hdr hdr = {0};
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	evt_filter = hci_event_filter(data);
	if (evt_filter == EVT_NOP) {
		/* The controller sends NOP event when wakes up based on
		 * hardware specific requirement, do not post this event to
		 * host stack.
		 */
		return NULL;
	} else if (evt_filter == EVT_DISCARD) {
		discardable = true;
	}

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	len -= sizeof(hdr);

	if (len != hdr.len) {
		LOG_ERR("Event payload length is not correct");
		return NULL;
	}

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (!buf) {
		if (discardable) {
			LOG_DBG("Discardable buffer pool full, ignoring event");
		} else {
			LOG_ERR("No available event buffers!");
		}
		return buf;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static struct net_buf *bt_hci_acl_recv(uint8_t *data, size_t len)
{
	struct bt_hci_acl_hdr hdr = {0};
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		len -= sizeof(hdr);
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (len != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static void bt_spi_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct ambiq_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_buf *buf;
	int ret;
	uint16_t len = 0;

	while (true) {
		/* Wait for controller interrupt */
		k_sem_take(&sem_irq, K_FOREVER);

		do {
			/* Recevive the HCI packet via SPI */
			ret = spi_receive_packet(&rxmsg[0], &len);
			if (ret) {
				break;
			}

			/* Check if needs to handle the vendor specific events which are
			 * incompatible with the standard Bluetooth HCI format.
			 */
			if (bt_apollo_vnd_rcv_ongoing(&rxmsg[0], len)) {
				break;
			}

			/* Responses to the driver's own commands, sent while
			 * opening
			 */
			if (bt_hci_lockstep_feed(&data->lockstep, &rxmsg[0], len)) {
				break;
			}

			switch (rxmsg[PACKET_TYPE]) {
			case BT_HCI_H4_EVT:
				buf = bt_hci_evt_recv(&rxmsg[PACKET_TYPE + PACKET_TYPE_SIZE],
						      (len - PACKET_TYPE_SIZE));
				break;
			case BT_HCI_H4_ACL:
				buf = bt_hci_acl_recv(&rxmsg[PACKET_TYPE + PACKET_TYPE_SIZE],
						      (len - PACKET_TYPE_SIZE));
				break;
			default:
				buf = NULL;
				LOG_WRN("Unknown BT buf type %d", rxmsg[PACKET_TYPE]);
				break;
			}

			/* Post the RX message to host stack to process */
			if (buf) {
				bt_hci_recv(dev, buf);
			}
		} while (0);
	}
}

static int bt_apollo_send_raw(const struct device *dev, const uint8_t *pkt, size_t len)
{
	ARG_UNUSED(dev);

	if (len > SPI_MAX_TX_MSG_LEN) {
		LOG_ERR("Message too long");
		return -EINVAL;
	}

	return spi_send_packet((uint8_t *)pkt, (uint16_t)len);
}

static int bt_apollo_send(const struct device *dev, struct net_buf *buf)
{
	int ret;

	ret = bt_apollo_send_raw(dev, buf->data, buf->len);
	if (ret != 0) {
		return ret;
	}

	net_buf_unref(buf);

	return 0;
}

static int bt_apollo_open(const struct device *dev)
{
	struct ambiq_data *data = dev->data;
	int ret;

	ret = bt_hci_transport_setup(spi_bus.bus);
	if (ret) {
		return ret;
	}

	/* A new session, which the controller initialization below already
	 * sends in, through spi_send_packet().
	 */
	ret = k_sem_take(&sem_spi_available, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	transport_open = true;
	(void)atomic_inc(&transport_session);
	k_sem_give(&sem_spi_available);

	/* Start RX thread */
	k_thread_create(&spi_rx_thread_data, spi_rx_stack, K_KERNEL_STACK_SIZEOF(spi_rx_stack),
			(k_thread_entry_t)bt_spi_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);

	ret = bt_apollo_controller_init(spi_send_packet);
	if (ret == 0) {
		ret = bt_apollo_vnd_setup(&data->lockstep);
	}

	if (ret != 0) {
		/* A failed open() is not followed by close(), so undo what this
		 * function started. The SPI semaphore is held across the abort
		 * so that the RX thread cannot be stopped while it owns it, and
		 * the caller may try again: k_thread_create() on a thread that
		 * is still running is a fault of its own.
		 */
		k_sem_take(&sem_spi_available, K_FOREVER);
		k_thread_abort(&spi_rx_thread_data);
		(void)bt_apollo_controller_deinit();
		k_sem_give(&sem_spi_available);
	}

	return ret;
}

static int bt_apollo_close(const struct device *dev)
{
	int ret;

	/* The SPI semaphore is held while the controller is taken down and the
	 * RX thread is stopped, so that neither happens in the middle of a
	 * transfer and the thread is not stopped owning a semaphore that
	 * k_thread_abort() does not give back.
	 */
	ret = k_sem_take(&sem_spi_available, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = bt_apollo_controller_deinit();
	if (ret != 0) {
		k_sem_give(&sem_spi_available);
		return ret;
	}

	/* A send that is waiting for the semaphore, or sleeping between its
	 * attempts, gives up instead of talking to a controller that is down,
	 * or to the one a later open() brings up.
	 */
	transport_open = false;
	(void)atomic_inc(&transport_session);

	/* Stop RX thread */
	if (k_current_get() == (k_tid_t)&spi_rx_thread_data) {
		/* close() from the receive callback aborts the calling thread,
		 * which does not return here, so nothing may be held across it.
		 */
		k_sem_give(&sem_spi_available);
		k_thread_abort(&spi_rx_thread_data);
	} else {
		k_thread_abort(&spi_rx_thread_data);
		k_sem_give(&sem_spi_available);
	}

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = bt_apollo_open,
	.close = bt_apollo_close,
	.send = bt_apollo_send,
};

static int bt_apollo_init(const struct device *dev)
{
	struct ambiq_data *data = dev->data;
	int ret;

	if (!device_is_ready(spi_bus.bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	ret = bt_apollo_dev_init();
	if (ret) {
		return ret;
	}

	bt_hci_lockstep_init(&data->lockstep, dev, bt_apollo_send_raw);

	LOG_DBG("BT HCI initialized");

	return 0;
}

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct ambiq_data hci_data_##inst;                                                  \
	static const struct bt_hci_driver_config hci_config_##inst =                               \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, bt_apollo_init, NULL, &hci_data_##inst, &hci_config_##inst,    \
			      POST_KERNEL, CONFIG_BT_HCI_INIT_PRIORITY, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
