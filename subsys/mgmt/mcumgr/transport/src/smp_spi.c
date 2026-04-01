/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief SPI transport for the mcumgr SMP protocol.
 *
 * SPI peripheral-mode transport for MCUmgr. The device operates as an SPI
 * target and exchanges raw SMP packets with the SPI initiator. A GPIO
 * data-ready line is asserted when at least one outbound response is queued
 * for the initiator to clock out.
 *
 * Devicetree configuration (compatible: "zephyr,smp-spi"):
 *   smp_spi: smp-spi {
 *       compatible = "zephyr,smp-spi";
 *       spi-dev = <&spi1>;
 *       data-ready-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
 *   };
 *
 * Each SPI transceive exchanges up to CONFIG_MCUMGR_TRANSPORT_SPI_MTU bytes on
 * each line. Outbound SMP data is copied verbatim (no UART-style framing).
 * Inbound data is passed to SMP as one fragment per transaction, optionally via
 * reassembly when CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY is enabled.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY)
#include <mgmt/mcumgr/transport/smp_reassembly.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_spi, CONFIG_MCUMGR_TRANSPORT_SPI_LOG_LEVEL);

static uint16_t smp_spi_get_mtu(const struct net_buf *nb);
static int smp_spi_tx_pkt(struct net_buf *nb);

#define SMP_SPI_NODE DT_NODELABEL(smp_spi)

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_SPI_MTU != 0,
	     "CONFIG_MCUMGR_TRANSPORT_SPI_MTU must be > 0");

#define SMP_SPI_BUF_SIZE CONFIG_MCUMGR_TRANSPORT_SPI_MTU

#define SMP_SPI_TX_FIFO_MAX 8

struct smp_spi_tx_fifo_item {
	void *fifo_reserved;
	struct net_buf *nb;
};

static const struct device *spi_dev = DEVICE_DT_GET(DT_PHANDLE(SMP_SPI_NODE, spi_dev));
static const struct gpio_dt_spec data_ready_gpio =
	GPIO_DT_SPEC_GET(SMP_SPI_NODE, data_ready_gpios);

static struct smp_transport smp_spi_transport = {
	.functions.output = smp_spi_tx_pkt,
	.functions.get_mtu = smp_spi_get_mtu,
};

K_MEM_SLAB_DEFINE_STATIC(smp_spi_tx_slab, sizeof(struct smp_spi_tx_fifo_item),
			 SMP_SPI_TX_FIFO_MAX, 4);
K_FIFO_DEFINE(smp_spi_tx_fifo);

static K_THREAD_STACK_DEFINE(smp_spi_rx_stack,
			     CONFIG_MCUMGR_TRANSPORT_SPI_RX_THREAD_STACK_SIZE);
static struct k_thread smp_spi_rx_thread_data;

static uint8_t smp_spi_rx_buf[SMP_SPI_BUF_SIZE];
static uint8_t smp_spi_tx_buf[SMP_SPI_BUF_SIZE];

static const struct spi_config smp_spi_cfg = {
	.frequency = 0,
	.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

static uint16_t smp_spi_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	return CONFIG_MCUMGR_TRANSPORT_SPI_MTU;
}

static int smp_spi_tx_pkt(struct net_buf *nb)
{
	struct smp_spi_tx_fifo_item *item;

	if (nb->len > CONFIG_MCUMGR_TRANSPORT_SPI_MTU) {
		LOG_ERR("SPI SMP TX payload %u exceeds MTU %d", nb->len,
			CONFIG_MCUMGR_TRANSPORT_SPI_MTU);
		smp_packet_free(nb);
		return -ENOMEM;
	}

	if (k_mem_slab_alloc(&smp_spi_tx_slab, (void **)&item, K_NO_WAIT) != 0) {
		LOG_ERR("SPI SMP TX queue full");
		smp_packet_free(nb);
		return -ENOMEM;
	}

	item->nb = nb;
	k_fifo_put(&smp_spi_tx_fifo, item);
	gpio_pin_set_dt(&data_ready_gpio, 1);

	return 0;
}

static void smp_spi_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct spi_buf rx_spi_buf = {
			.buf = smp_spi_rx_buf,
			.len = sizeof(smp_spi_rx_buf),
		};
		const struct spi_buf_set rx_set = {
			.buffers = &rx_spi_buf,
			.count = 1,
		};
		struct spi_buf tx_spi_buf = {
			.buf = smp_spi_tx_buf,
			.len = 0,
		};
		const struct spi_buf_set tx_set = {
			.buffers = &tx_spi_buf,
			.count = 1,
		};
		struct smp_spi_tx_fifo_item *item;
		struct net_buf *nb_tx = NULL;
		int rc;

		item = k_fifo_get(&smp_spi_tx_fifo, K_NO_WAIT);
		if (item != NULL) {
			nb_tx = item->nb;
			memcpy(smp_spi_tx_buf, nb_tx->data, nb_tx->len);
			tx_spi_buf.len = nb_tx->len;
		}

		rc = spi_transceive(spi_dev, &smp_spi_cfg, &tx_set, &rx_set);
		if (rc != 0) {
			LOG_ERR("SPI SMP transceive failed: %d", rc);
			if (item != NULL) {
				item->nb = nb_tx;
				k_fifo_put(&smp_spi_tx_fifo, item);
			}
			continue;
		}

		if (item != NULL) {
			smp_packet_free(nb_tx);
			k_mem_slab_free(&smp_spi_tx_slab, item);
		}

		if (k_fifo_is_empty(&smp_spi_tx_fifo)) {
			gpio_pin_set_dt(&data_ready_gpio, 0);
		} else {
			gpio_pin_set_dt(&data_ready_gpio, 1);
		}

		if (rx_spi_buf.len == 0U) {
			continue;
		}

		if (rx_spi_buf.len > CONFIG_MCUMGR_TRANSPORT_SPI_MTU) {
			LOG_WRN("SPI SMP RX length %u exceeds MTU %d, dropping", rx_spi_buf.len,
				CONFIG_MCUMGR_TRANSPORT_SPI_MTU);
			continue;
		}

#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY)
		rc = smp_reassembly_collect(&smp_spi_transport, smp_spi_rx_buf,
					    (uint16_t)rx_spi_buf.len);
		if (rc < 0) {
			if (rc != -ENODATA) {
				LOG_ERR("SPI SMP reassembly collect failed: %d", rc);
			}
			(void)smp_reassembly_drop(&smp_spi_transport);
			continue;
		}

		if (rc == 0) {
			rc = smp_reassembly_complete(&smp_spi_transport, false);
			if (rc < 0 && rc != -ENODATA) {
				LOG_ERR("SPI SMP reassembly complete failed: %d", rc);
			}
		}
#else
		{
			struct net_buf *nb;

			if (rx_spi_buf.len < sizeof(struct smp_hdr)) {
				continue;
			}

			nb = smp_packet_alloc();
			if (nb == NULL) {
				LOG_ERR("SPI SMP failed to allocate net_buf");
				continue;
			}

			if (net_buf_tailroom(nb) < rx_spi_buf.len) {
				LOG_ERR("SPI SMP net_buf too small for payload (%u)", rx_spi_buf.len);
				smp_packet_free(nb);
				continue;
			}

			net_buf_add_mem(nb, smp_spi_rx_buf, rx_spi_buf.len);
			smp_rx_req(&smp_spi_transport, nb);
		}
#endif
	}
}

static void smp_spi_start(void)
{
	int rc;

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device not ready");
		return;
	}

	if (!gpio_is_ready_dt(&data_ready_gpio)) {
		LOG_ERR("Data-ready GPIO not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&data_ready_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		LOG_ERR("Failed to configure data-ready GPIO: %d", rc);
		return;
	}

	rc = smp_transport_init(&smp_spi_transport);
	if (rc != 0) {
		LOG_ERR("SPI SMP transport init failed: %d", rc);
		return;
	}

	k_thread_create(&smp_spi_rx_thread_data, smp_spi_rx_stack,
			K_THREAD_STACK_SIZEOF(smp_spi_rx_stack), smp_spi_rx_thread, NULL, NULL,
			NULL, CONFIG_MCUMGR_TRANSPORT_SPI_RX_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&smp_spi_rx_thread_data, "smp_spi_rx");
}

MCUMGR_HANDLER_DEFINE(smp_spi, smp_spi_start);
