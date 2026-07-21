/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief SPI transport for the MCUmgr SMP protocol.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_smp_spi

#define SMP_SPI_NODE DT_DRV_INST(0)

#define SMP_SPI_OPERATION (SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

static const struct spi_dt_spec smp_spi_spec = SPI_DT_SPEC_GET(SMP_SPI_NODE, SMP_SPI_OPERATION);
static const struct gpio_dt_spec data_ready_gpio =
	GPIO_DT_SPEC_GET(SMP_SPI_NODE, data_ready_gpios);

static uint8_t rx_buf[CONFIG_MCUMGR_TRANSPORT_SPI_MTU];
static uint8_t tx_buf[CONFIG_MCUMGR_TRANSPORT_SPI_MTU];
static uint8_t tx_packet[CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE];
static struct spi_buf rx_spi_buf = {
	.buf = rx_buf,
	.len = sizeof(rx_buf),
};
static struct spi_buf tx_spi_buf = {
	.buf = tx_buf,
	.len = sizeof(tx_buf),
};
static const struct spi_buf_set rx_spi_buf_set = {
	.buffers = &rx_spi_buf,
	.count = 1,
};
static const struct spi_buf_set tx_spi_buf_set = {
	.buffers = &tx_spi_buf,
	.count = 1,
};

/*
 * tx_done_sem serializes complete SMP responses: MCUmgr output takes it
 * before queuing a response and SPI completion gives it after the controller
 * drains the final response chunk. tx_lock protects the shared TX buffers and
 * offsets because MCUmgr output and async SPI completion run in different
 * contexts while a response is active.
 */
static struct k_mutex tx_lock;
static K_SEM_DEFINE(tx_done_sem, 1, 1);
static uint16_t tx_len;
static uint16_t tx_offset;
static uint16_t tx_chunk_len;
static int spi_completion_result;

static struct smp_transport smp_spi_transport;

#ifdef CONFIG_SMP_CLIENT
static struct smp_client_transport_entry smp_spi_client_transport = {
	.smpt = &smp_spi_transport,
	.smpt_type = SMP_SPI_TRANSPORT,
};
#endif

static void smp_spi_callback(const struct device *dev, int result, void *userdata);
static void smp_spi_work_handler(struct k_work *work);
static K_WORK_DEFINE(smp_spi_work, smp_spi_work_handler);

static bool smp_spi_empty_header(const uint8_t *data)
{
	for (size_t i = 0; i < sizeof(struct smp_hdr); i++) {
		if (data[i] != 0) {
			return false;
		}
	}

	return true;
}

static void smp_spi_data_ready_set(bool ready)
{
	int ret;

	ret = gpio_pin_set_dt(&data_ready_gpio, ready ? 1 : 0);
	if (ret < 0) {
		LOG_ERR("Failed to set SPI SMP data-ready GPIO: %d", ret);
	}
}

static int smp_spi_reassembly_collect(const uint8_t *data, uint16_t len)
{
	uint16_t chunk_len;
	int expected;
	int rc;

	expected = smp_reassembly_expected(&smp_spi_transport);
	if (len == 0) {
		if (expected >= 0) {
			return smp_reassembly_drop(&smp_spi_transport);
		}

		return 0;
	}

	if (expected < 0) {
		const struct smp_hdr *hdr = (const struct smp_hdr *)data;
		uint16_t packet_len;

		if (len < sizeof(struct smp_hdr)) {
			LOG_ERR("Invalid SPI SMP packet length: %u", len);
			return -EINVAL;
		}

		if (smp_spi_empty_header(data)) {
			return 0;
		}

		packet_len = sys_be16_to_cpu(hdr->nh_len) + sizeof(struct smp_hdr);
		if (packet_len > CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE) {
			LOG_ERR("SPI SMP packet length exceeds net_buf size: %u", packet_len);
			return -EMSGSIZE;
		}

		chunk_len = MIN(packet_len, len);
	} else {
		chunk_len = MIN((uint16_t)expected, len);
	}

	rc = smp_reassembly_collect(&smp_spi_transport, data, chunk_len);
	if (rc == 0) {
		rc = smp_reassembly_complete(&smp_spi_transport, false);
		if (rc) {
			LOG_ERR("SPI SMP reassembly complete failed: %d", rc);
		}
	} else if (rc < 0) {
		LOG_ERR("SPI SMP reassembly collect failed: %d", rc);
	}

	return rc;
}

static int smp_spi_tx_pkt(struct net_buf *nb)
{
	int ret = 0;

	k_sem_take(&tx_done_sem, K_FOREVER);
	k_mutex_lock(&tx_lock, K_FOREVER);

	if (nb->len > sizeof(tx_packet)) {
		LOG_ERR("SPI SMP response too large: %u", nb->len);
		ret = -EMSGSIZE;
		goto out;
	}

	memcpy(tx_packet, nb->data, nb->len);
	tx_len = nb->len;
	tx_offset = 0;
	smp_spi_data_ready_set(true);

out:
	k_mutex_unlock(&tx_lock);
	if (ret != 0) {
		k_sem_give(&tx_done_sem);
	}
	smp_packet_free(nb);

	return ret;
}

static uint16_t smp_spi_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	return CONFIG_MCUMGR_TRANSPORT_SPI_MTU;
}

/* The peripheral drives data-ready-gpios while response chunks are queued. */
static uint16_t smp_spi_prepare_tx_locked(void)
{
	uint16_t chunk_len = 0;

	memset(tx_buf, 0, sizeof(tx_buf));

	if (tx_offset < tx_len) {
		chunk_len = MIN((uint16_t)(tx_len - tx_offset), (uint16_t)sizeof(tx_buf));
		memcpy(tx_buf, &tx_packet[tx_offset], chunk_len);
	}

	return chunk_len;
}

static void smp_spi_finish_tx(void)
{
	bool drained = false;

	k_mutex_lock(&tx_lock, K_FOREVER);

	if (tx_chunk_len > 0) {
		tx_offset += tx_chunk_len;
	}

	if (tx_offset >= tx_len) {
		tx_len = 0;
		tx_offset = 0;
		drained = tx_chunk_len > 0;
		tx_chunk_len = 0;
		smp_spi_data_ready_set(false);
	} else {
		smp_spi_data_ready_set(true);
	}

	k_mutex_unlock(&tx_lock);

	if (drained) {
		k_sem_give(&tx_done_sem);
	}
}

static int smp_spi_arm_transfer(void)
{
	int ret;

	k_mutex_lock(&tx_lock, K_FOREVER);

	memset(rx_buf, 0, sizeof(rx_buf));
	tx_chunk_len = smp_spi_prepare_tx_locked();

	k_mutex_unlock(&tx_lock);

	ret = spi_transceive_cb(smp_spi_spec.bus, &smp_spi_spec.config, &tx_spi_buf_set,
				&rx_spi_buf_set, smp_spi_callback, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to arm SPI SMP transfer: %d", ret);
	}

	return ret;
}

static void smp_spi_callback(const struct device *dev, int result, void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(userdata);

	spi_completion_result = result;
	(void)k_work_submit(&smp_spi_work);
}

static void smp_spi_work_handler(struct k_work *work)
{
	uint16_t rx_len;
	int ret;

	ARG_UNUSED(work);

	if (spi_completion_result < 0) {
		LOG_ERR("SPI SMP transfer failed: %d", spi_completion_result);
	} else {
		rx_len = MIN((uint16_t)spi_completion_result, (uint16_t)sizeof(rx_buf));
		smp_spi_finish_tx();
		(void)smp_spi_reassembly_collect(rx_buf, rx_len);
	}

	ret = smp_spi_arm_transfer();
	if (ret < 0) {
		LOG_ERR("Failed to re-arm SPI SMP transfer: %d", ret);
	}
}

static void smp_spi_start(void)
{
	int rc;

	if (!spi_is_ready_dt(&smp_spi_spec)) {
		LOG_ERR("SPI device not ready");
		return;
	}

	if (!gpio_is_ready_dt(&data_ready_gpio)) {
		LOG_ERR("SPI SMP data-ready GPIO not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&data_ready_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to configure SPI SMP data-ready GPIO: %d", rc);
		return;
	}

	k_mutex_init(&tx_lock);

	smp_spi_transport.functions.output = smp_spi_tx_pkt;
	smp_spi_transport.functions.get_mtu = smp_spi_get_mtu;

	rc = smp_transport_init(&smp_spi_transport);
	if (rc != 0) {
		LOG_ERR("SPI SMP transport init failed: %d", rc);
		return;
	}

#ifdef CONFIG_SMP_CLIENT
	smp_client_transport_register(&smp_spi_client_transport);
#endif

	rc = smp_spi_arm_transfer();
	if (rc < 0) {
		LOG_ERR("Failed to arm SPI SMP transfer: %d", rc);
	}
}

MCUMGR_HANDLER_DEFINE(smp_spi, smp_spi_start);
