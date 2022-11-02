/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_eth

#include <ethernet/eth_stats.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>

#include <esp_attr.h>
#include <esp_mac.h>
#include <hal/emac_hal.h>
#include <hal/emac_ll.h>

#include "eth.h"

LOG_MODULE_REGISTER(eth_esp32, CONFIG_ETHERNET_LOG_LEVEL);

#define MAC_RESET_TIMEOUT_MS 100

struct eth_esp32_dma_data {
	uint8_t descriptors[
		CONFIG_ETH_DMA_RX_BUFFER_NUM * sizeof(eth_dma_rx_descriptor_t) +
		CONFIG_ETH_DMA_TX_BUFFER_NUM * sizeof(eth_dma_tx_descriptor_t)];
	uint8_t rx_buf[CONFIG_ETH_DMA_RX_BUFFER_NUM][CONFIG_ETH_DMA_BUFFER_SIZE];
	uint8_t tx_buf[CONFIG_ETH_DMA_TX_BUFFER_NUM][CONFIG_ETH_DMA_BUFFER_SIZE];
};

struct eth_esp32_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	emac_hal_context_t hal;
	struct eth_esp32_dma_data *dma;
	uint8_t txb[NET_ETH_MAX_FRAME_SIZE];
	uint8_t rxb[NET_ETH_MAX_FRAME_SIZE];
	uint8_t *dma_rx_buf[CONFIG_ETH_DMA_RX_BUFFER_NUM];
	uint8_t *dma_tx_buf[CONFIG_ETH_DMA_TX_BUFFER_NUM];
	struct k_sem int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_ESP32_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

static enum ethernet_hw_caps eth_esp32_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

static int eth_esp32_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_esp32_dev_data *dev_data = dev->data;
	size_t len = net_pkt_get_len(pkt);

	if (net_pkt_read(pkt, dev_data->txb, len)) {
		return -EIO;
	}

	uint32_t sent_len = emac_hal_transmit_frame(&dev_data->hal, dev_data->txb, len);

	int res = len == sent_len ? 0 : -EIO;

	return res;
}

static struct net_pkt *eth_esp32_rx(
	struct eth_esp32_dev_data *const dev_data, uint32_t *frames_remaining)
{
	uint32_t free_rx_descriptor;
	uint32_t receive_len = emac_hal_receive_frame(
		&dev_data->hal, dev_data->rxb, sizeof(dev_data->rxb),
		frames_remaining, &free_rx_descriptor);
	if (receive_len == 0) {
		/* Nothing to receive */
		return NULL;
	}

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(
		dev_data->iface, receive_len, AF_UNSPEC, 0, K_MSEC(100));
	if (pkt == NULL) {
		eth_stats_update_errors_rx(ctx->iface);
		LOG_ERR("Could not allocate rx buffer");
		return NULL;
	}

	if (net_pkt_write(pkt, dev_data->rxb, receive_len) != 0) {
		LOG_ERR("Unable to write frame into the pkt");
		eth_stats_update_errors_rx(ctx->iface);
		net_pkt_unref(pkt);
		return NULL;
	}

	return pkt;
}

FUNC_NORETURN static void eth_esp32_rx_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct eth_esp32_dev_data *const dev_data = dev->data;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		k_sem_take(&dev_data->int_sem, K_FOREVER);

		uint32_t frames_remaining;

		do {
			struct net_pkt *pkt = eth_esp32_rx(
				dev_data, &frames_remaining);
			if (pkt == NULL) {
				break;
			}

			if (net_recv_data(dev_data->iface, pkt) < 0) {
				/* Upper layers are not ready to receive packets */
				net_pkt_unref(pkt);
			}
		} while (frames_remaining > 0);
	}
}

IRAM_ATTR static void eth_esp32_isr(void *arg)
{
	const struct device *dev = arg;
	struct eth_esp32_dev_data *const dev_data = dev->data;
	uint32_t intr_stat = emac_ll_get_intr_status(dev_data->hal.dma_regs);

	emac_ll_clear_corresponding_intr(dev_data->hal.dma_regs, intr_stat);

	if (intr_stat & EMAC_LL_DMA_RECEIVE_FINISH_INTR) {
		k_sem_give(&dev_data->int_sem);
	}
}

static int generate_mac_addr(uint8_t mac_addr[6])
{
	int res = 0;
#if DT_INST_PROP(0, zephyr_random_mac_address)
	gen_random_mac(mac_addr, 0x24, 0xD7, 0xEB);
#elif NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	static const uint8_t addr[6] = DT_INST_PROP(0, local_mac_address);

	memcpy(mac_addr, addr, sizeof(addr));
#else
	if (esp_read_mac(mac_addr, ESP_MAC_ETH) != ESP_OK) {
		res = -EIO;
	}
#endif
	return res;
}

static void phy_link_state_changed(const struct device *phy_dev,
				   struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_esp32_dev_data *const dev_data = dev->data;

	ARG_UNUSED(phy_dev);

	if (state->is_up) {
		net_eth_carrier_on(dev_data->iface);
	} else {
		net_eth_carrier_off(dev_data->iface);
	}
}

int eth_esp32_initialize(const struct device *dev)
{
	struct eth_esp32_dev_data *const dev_data = dev->data;
	int res;

	k_sem_init(&dev_data->int_sem, 0, 1);

	const struct device *clock_dev =
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(eth)));
	clock_control_subsys_t clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(eth), offset);

	res = clock_control_on(clock_dev, clock_subsys);
	if (res != 0) {
		goto err;
	}

	/* Convert 2D array DMA buffers to arrays of pointers */
	for (int i = 0; i < CONFIG_ETH_DMA_RX_BUFFER_NUM; i++) {
		dev_data->dma_rx_buf[i] = dev_data->dma->rx_buf[i];
	}
	for (int i = 0; i < CONFIG_ETH_DMA_TX_BUFFER_NUM; i++) {
		dev_data->dma_tx_buf[i] = dev_data->dma->tx_buf[i];
	}

	emac_hal_init(&dev_data->hal, dev_data->dma->descriptors,
		      dev_data->dma_rx_buf, dev_data->dma_tx_buf);

	/* Configure ISR */
	res = esp_intr_alloc(DT_IRQN(DT_NODELABEL(eth)),
		       ESP_INTR_FLAG_IRAM,
		       eth_esp32_isr,
		       (void *)dev,
		       NULL);
	if (res != 0) {
		goto err;
	}

	/* Configure phy for Media-Independent Interface (MII) or
	 * Reduced Media-Independent Interface (RMII) mode
	 */
	const char *phy_connection_type = DT_INST_PROP(0, phy_connection_type);

	if (strcmp(phy_connection_type, "rmii") == 0) {
		emac_hal_iomux_init_rmii();
		emac_hal_iomux_rmii_clk_input();
		emac_ll_clock_enable_rmii_input(dev_data->hal.ext_regs);
	} else if (strcmp(phy_connection_type, "mii") == 0) {
		emac_hal_iomux_init_mii();
		emac_ll_clock_enable_mii(dev_data->hal.ext_regs);
	} else {
		res = -EINVAL;
		goto err;
	}

	/* Reset mac registers and wait until ready */
	emac_ll_reset(dev_data->hal.dma_regs);
	bool reset_success = false;

	for (uint32_t t_ms = 0; t_ms < MAC_RESET_TIMEOUT_MS; t_ms += 10) {
		/* Busy wait rather than sleep in case kernel is not yet initialized */
		k_busy_wait(10 * 1000);
		if (emac_ll_is_reset_done(dev_data->hal.dma_regs)) {
			reset_success = true;
			break;
		}
	}
	if (!reset_success) {
		res = -ETIMEDOUT;
		goto err;
	}

	emac_hal_reset_desc_chain(&dev_data->hal);
	emac_hal_init_mac_default(&dev_data->hal);
	emac_hal_init_dma_default(&dev_data->hal);

	res = generate_mac_addr(dev_data->mac_addr);
	if (res != 0) {
		goto err;
	}
	emac_hal_set_address(&dev_data->hal, dev_data->mac_addr);

	k_tid_t tid = k_thread_create(
		&dev_data->rx_thread, dev_data->rx_thread_stack,
		K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack),
		eth_esp32_rx_thread,
		(void *)dev, NULL, NULL,
		CONFIG_ETH_ESP32_RX_THREAD_PRIORITY,
		K_ESSENTIAL, K_NO_WAIT);
	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(tid, "esp32_eth");
	}

	emac_hal_start(&dev_data->hal);

	return 0;

err:
	return res;
}

static void eth_esp32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_esp32_dev_data *dev_data = dev->data;
	const struct device *phy_dev = DEVICE_DT_GET(DT_INST_CHILD(0, phy));

	dev_data->iface = iface;

	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	if (device_is_ready(phy_dev)) {
		phy_link_callback_set(phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);
}

static const struct ethernet_api eth_esp32_api = {
	.iface_api.init		= eth_esp32_iface_init,
	.get_capabilities	= eth_esp32_caps,
	.send			= eth_esp32_send,
};

/* DMA data must be in DRAM */
static struct eth_esp32_dma_data eth_esp32_dma_data WORD_ALIGNED_ATTR DRAM_ATTR;

static struct eth_esp32_dev_data eth_esp32_dev = {
	.dma = &eth_esp32_dma_data,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0,
		    eth_esp32_initialize,
		    NULL,
		    &eth_esp32_dev,
		    NULL,
		    CONFIG_ETH_INIT_PRIORITY,
		    &eth_esp32_api,
		    NET_ETH_MTU);
