/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_eth

#include <string.h>
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
#include <hal/emac_periph.h>
#include <soc/rtc.h>
#include <soc/gpio_periph.h>
#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <clk_ctrl_os.h>

#include "eth.h"
#include "eth_esp32_priv.h"

LOG_MODULE_REGISTER(eth_esp32, CONFIG_ETHERNET_LOG_LEVEL);

#define MAC_RESET_TIMEOUT_MS 100
#define ETH_CRC_LENGTH       4

struct eth_esp32_dma_data {
	uint8_t descriptors[CONFIG_ETH_DMA_RX_BUFFER_NUM * sizeof(eth_dma_rx_descriptor_t) +
			    CONFIG_ETH_DMA_TX_BUFFER_NUM * sizeof(eth_dma_tx_descriptor_t)]
		__aligned(4);
	uint8_t rx_buf[CONFIG_ETH_DMA_RX_BUFFER_NUM][CONFIG_ETH_DMA_BUFFER_SIZE];
	uint8_t tx_buf[CONFIG_ETH_DMA_TX_BUFFER_NUM][CONFIG_ETH_DMA_BUFFER_SIZE];
};

struct eth_esp32_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	emac_hal_context_t hal;
	struct eth_esp32_dma_data *dma;
	eth_dma_rx_descriptor_t *rx_desc;
	eth_dma_tx_descriptor_t *tx_desc;
	uint8_t txb[NET_ETH_MAX_FRAME_SIZE];
	uint8_t rxb[NET_ETH_MAX_FRAME_SIZE];
	uint8_t *dma_rx_buf[CONFIG_ETH_DMA_RX_BUFFER_NUM];
	uint8_t *dma_tx_buf[CONFIG_ETH_DMA_TX_BUFFER_NUM];
	struct k_sem int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_ESP32_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

static const struct device *eth_esp32_phy_dev = DEVICE_DT_GET(
		DT_INST_PHANDLE(0, phy_handle));

static void eth_esp32_reset_desc_chain(struct eth_esp32_dev_data *dev_data)
{
	/* Reset DMA descriptor pointers */
	dev_data->rx_desc = (eth_dma_rx_descriptor_t *)(dev_data->dma->descriptors);
	dev_data->tx_desc = (eth_dma_tx_descriptor_t *)(dev_data->dma->descriptors +
							sizeof(eth_dma_rx_descriptor_t) *
								CONFIG_ETH_DMA_RX_BUFFER_NUM);

	/* Initialize RX descriptor chain */
	for (int i = 0; i < CONFIG_ETH_DMA_RX_BUFFER_NUM; i++) {
		dev_data->rx_desc[i].RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		dev_data->rx_desc[i].RDES1.SecondAddressChained = 1;
		dev_data->rx_desc[i].RDES1.ReceiveBuffer1Size = CONFIG_ETH_DMA_BUFFER_SIZE;
		dev_data->rx_desc[i].RDES1.DisableInterruptOnComplete = 0;
		dev_data->rx_desc[i].Buffer1Addr = (uint32_t)(dev_data->dma_rx_buf[i]);
		dev_data->rx_desc[i].Buffer2NextDescAddr = (uint32_t)(dev_data->rx_desc + i + 1);

		if (i == CONFIG_ETH_DMA_RX_BUFFER_NUM - 1) {
			dev_data->rx_desc[i].Buffer2NextDescAddr = (uint32_t)(dev_data->rx_desc);
		}
	}

	/* Initialize TX descriptor chain */
	for (int i = 0; i < CONFIG_ETH_DMA_TX_BUFFER_NUM; i++) {
		dev_data->tx_desc[i].TDES0.Own = EMAC_LL_DMADESC_OWNER_CPU;
		dev_data->tx_desc[i].TDES0.SecondAddressChained = 1;
		dev_data->tx_desc[i].TDES1.TransmitBuffer1Size = CONFIG_ETH_DMA_BUFFER_SIZE;
		dev_data->tx_desc[i].Buffer1Addr = (uint32_t)(dev_data->dma_tx_buf[i]);
		dev_data->tx_desc[i].Buffer2NextDescAddr = (uint32_t)(dev_data->tx_desc + i + 1);

		if (i == CONFIG_ETH_DMA_TX_BUFFER_NUM - 1) {
			dev_data->tx_desc[i].Buffer2NextDescAddr = (uint32_t)(dev_data->tx_desc);
		}
	}

	/* Set base address of descriptors */
	emac_hal_set_rx_tx_desc_addr(&dev_data->hal, dev_data->rx_desc, dev_data->tx_desc);
}

static uint32_t eth_esp32_transmit_frame(struct eth_esp32_dev_data *dev_data, uint8_t *buf,
					 uint32_t length)
{
	uint32_t bufcount = 0;
	uint32_t lastlen = length;
	uint32_t sentout = 0;

	/* Calculate number of TX buffers needed */
	while (lastlen > CONFIG_ETH_DMA_BUFFER_SIZE) {
		lastlen -= CONFIG_ETH_DMA_BUFFER_SIZE;
		bufcount++;
	}
	if (lastlen) {
		bufcount++;
	}
	if (bufcount > CONFIG_ETH_DMA_TX_BUFFER_NUM) {
		return 0;
	}

	eth_dma_tx_descriptor_t *desc_iter = dev_data->tx_desc;

	/* Fill descriptors */
	for (size_t i = 0; i < bufcount; i++) {
		if (desc_iter->TDES0.Own != EMAC_LL_DMADESC_OWNER_CPU) {
			return 0;
		}

		desc_iter->TDES0.FirstSegment = 0;
		desc_iter->TDES0.LastSegment = 0;

		if (i == 0) {
			desc_iter->TDES0.FirstSegment = 1;
		}
		if (i == (bufcount - 1)) {
			desc_iter->TDES0.LastSegment = 1;
			desc_iter->TDES1.TransmitBuffer1Size = lastlen;
			memcpy((void *)(desc_iter->Buffer1Addr),
			       buf + i * CONFIG_ETH_DMA_BUFFER_SIZE, lastlen);
			sentout += lastlen;
		} else {
			desc_iter->TDES1.TransmitBuffer1Size = CONFIG_ETH_DMA_BUFFER_SIZE;
			memcpy((void *)(desc_iter->Buffer1Addr),
			       buf + i * CONFIG_ETH_DMA_BUFFER_SIZE, CONFIG_ETH_DMA_BUFFER_SIZE);
			sentout += CONFIG_ETH_DMA_BUFFER_SIZE;
		}
		desc_iter = (eth_dma_tx_descriptor_t *)(desc_iter->Buffer2NextDescAddr);
	}

	/* Give descriptors to DMA */
	for (size_t i = 0; i < bufcount; i++) {
		dev_data->tx_desc->TDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		dev_data->tx_desc =
			(eth_dma_tx_descriptor_t *)(dev_data->tx_desc->Buffer2NextDescAddr);
	}
	emac_hal_transmit_poll_demand(&dev_data->hal);

	return sentout;
}

static void eth_esp32_flush_rx_frame(eth_dma_rx_descriptor_t *first_desc,
				     eth_dma_rx_descriptor_t *last_desc)
{
	eth_dma_rx_descriptor_t *desc = first_desc;

	while (desc != last_desc) {
		desc->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc = (eth_dma_rx_descriptor_t *)(desc->Buffer2NextDescAddr);
	}
	desc->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
}

static uint32_t eth_esp32_receive_frame(struct eth_esp32_dev_data *dev_data, uint8_t *buf,
					uint32_t size, uint32_t *frames_remaining)
{
	eth_dma_rx_descriptor_t *desc_iter = dev_data->rx_desc;
	eth_dma_rx_descriptor_t *first_desc = NULL;
	uint32_t ret_len = 0;
	uint32_t copy_len = 0;
	uint32_t used_descs = 0;
	uint32_t frame_count = 0;

	*frames_remaining = 0;

	/* Find a complete frame and count remaining frames */
	while ((desc_iter->RDES0.Own == EMAC_LL_DMADESC_OWNER_CPU) &&
	       (used_descs < CONFIG_ETH_DMA_RX_BUFFER_NUM)) {
		used_descs++;

		if (desc_iter->RDES0.FirstDescriptor) {
			first_desc = desc_iter;
		}

		if (desc_iter->RDES0.LastDescriptor) {
			frame_count++;
			if (frame_count == 1 && first_desc != NULL) {
				if (desc_iter->RDES0.ErrSummary) {
					eth_esp32_flush_rx_frame(first_desc, desc_iter);
					dev_data->rx_desc =
						(eth_dma_rx_descriptor_t
							 *)(desc_iter->Buffer2NextDescAddr);
					emac_hal_receive_poll_demand(&dev_data->hal);
					return 0;
				}
				ret_len = desc_iter->RDES0.FrameLength - ETH_CRC_LENGTH;
			}
		}
		desc_iter = (eth_dma_rx_descriptor_t *)(desc_iter->Buffer2NextDescAddr);
	}

	*frames_remaining = (frame_count > 1) ? (frame_count - 1) : 0;

	if (ret_len == 0 || first_desc == NULL) {
		return 0;
	}

	/* Copy frame data */
	copy_len = (ret_len > size) ? size : ret_len;
	desc_iter = first_desc;
	uint32_t remaining = copy_len;

	while (remaining > CONFIG_ETH_DMA_BUFFER_SIZE) {
		memcpy(buf, (void *)(desc_iter->Buffer1Addr), CONFIG_ETH_DMA_BUFFER_SIZE);
		buf += CONFIG_ETH_DMA_BUFFER_SIZE;
		remaining -= CONFIG_ETH_DMA_BUFFER_SIZE;
		desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc_iter = (eth_dma_rx_descriptor_t *)(desc_iter->Buffer2NextDescAddr);
	}
	memcpy(buf, (void *)(desc_iter->Buffer1Addr), remaining);

	/* Return descriptors including any that held CRC */
	while (!desc_iter->RDES0.LastDescriptor) {
		desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc_iter = (eth_dma_rx_descriptor_t *)(desc_iter->Buffer2NextDescAddr);
	}
	desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;

	dev_data->rx_desc = (eth_dma_rx_descriptor_t *)(desc_iter->Buffer2NextDescAddr);
	emac_hal_receive_poll_demand(&dev_data->hal);

	return copy_len;
}

static void eth_esp32_iomux_rmii_clk_input(void)
{
	const emac_iomux_info_t *pin = emac_rmii_iomux_pins.clki;

	/* ESP32 EMAC uses dedicated IOMUX pins, not GPIO matrix */
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}
}

static void eth_esp32_iomux_init_mii(void)
{
	const emac_iomux_info_t *pin;

	/*
	 * ESP32 EMAC uses dedicated IOMUX pins, not GPIO matrix.
	 * Only PIN_FUNC_SELECT is needed to route signals.
	 */

	/* TX_CLK - input */
	pin = emac_mii_iomux_pins.clk_tx;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	/* TX_EN - output */
	pin = emac_mii_iomux_pins.tx_en;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	/* TXD0-3 - outputs */
	pin = emac_mii_iomux_pins.txd0;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.txd1;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.txd2;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.txd3;
	if (pin != NULL) {
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	/* RX_CLK - input */
	pin = emac_mii_iomux_pins.clk_rx;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	/* RX_DV - input */
	pin = emac_mii_iomux_pins.rx_dv;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	/* RXD0-3 - inputs */
	pin = emac_mii_iomux_pins.rxd0;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.rxd1;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.rxd2;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}

	pin = emac_mii_iomux_pins.rxd3;
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}
}

static enum ethernet_hw_caps eth_esp32_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

static int eth_esp32_set_config(const struct device *dev,
				enum ethernet_config_type type,
				const struct ethernet_config *config)
{
	struct eth_esp32_dev_data *const dev_data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		emac_hal_set_address(&dev_data->hal, dev_data->mac_addr);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static int eth_esp32_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_esp32_dev_data *dev_data = dev->data;
	size_t len = net_pkt_get_len(pkt);

	if (net_pkt_read(pkt, dev_data->txb, len)) {
		return -EIO;
	}

	uint32_t sent_len = eth_esp32_transmit_frame(dev_data, dev_data->txb, len);

	int res = len == sent_len ? 0 : -EIO;

	return res;
}

static struct net_pkt *eth_esp32_rx(
	struct eth_esp32_dev_data *const dev_data, uint32_t *frames_remaining)
{
	uint32_t receive_len = eth_esp32_receive_frame(dev_data, dev_data->rxb,
						       sizeof(dev_data->rxb), frames_remaining);
	if (receive_len == 0) {
		/* Nothing to receive */
		return NULL;
	}

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(
		dev_data->iface, receive_len, NET_AF_UNSPEC, 0, K_MSEC(100));
	if (pkt == NULL) {
		eth_stats_update_errors_rx(dev_data->iface);
		LOG_ERR("Could not allocate rx buffer");
		return NULL;
	}

	if (net_pkt_write(pkt, dev_data->rxb, receive_len) != 0) {
		LOG_ERR("Unable to write frame into the pkt");
		eth_stats_update_errors_rx(dev_data->iface);
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

	/* clock is shared, so do not bail out if already enabled */
	res = clock_control_on(clock_dev, clock_subsys);
	if (res < 0 && res != -EALREADY) {
		goto err;
	}

	/* Convert 2D array DMA buffers to arrays of pointers */
	for (int i = 0; i < CONFIG_ETH_DMA_RX_BUFFER_NUM; i++) {
		dev_data->dma_rx_buf[i] = dev_data->dma->rx_buf[i];
	}
	for (int i = 0; i < CONFIG_ETH_DMA_TX_BUFFER_NUM; i++) {
		dev_data->dma_tx_buf[i] = dev_data->dma->tx_buf[i];
	}

	/* Initialize HAL context */
	emac_hal_init(&dev_data->hal);

	/* Configure ISR */
	res = esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, irq),
			ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, priority)) |
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, flags)) |
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
	const char *phy_connection_type = DT_INST_PROP_OR(0,
						phy_connection_type,
						"rmii");

	if (strcmp(phy_connection_type, "rmii") == 0) {
		esp32_emac_iomux_init_rmii();
#if !DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
		eth_esp32_iomux_rmii_clk_input();
		emac_hal_clock_enable_rmii_input(&dev_data->hal);
#endif
	} else if (strcmp(phy_connection_type, "mii") == 0) {
		eth_esp32_iomux_init_mii();
		emac_hal_clock_enable_mii(&dev_data->hal);
	} else {
		res = -EINVAL;
		goto err;
	}

	/* Reset mac registers and wait until ready */
	emac_hal_reset(&dev_data->hal);
	bool reset_success = false;

	for (uint32_t t_ms = 0; t_ms < MAC_RESET_TIMEOUT_MS; t_ms += 10) {
		/* Busy wait rather than sleep in case kernel is not yet initialized */
		k_busy_wait(10 * 1000);
		if (emac_hal_is_reset_done(&dev_data->hal)) {
			reset_success = true;
			break;
		}
	}
	if (!reset_success) {
		res = -ETIMEDOUT;
		goto err;
	}

	/* Set dma_burst_len as ETH_DMA_BURST_LEN_32 by default */
	emac_hal_dma_config_t dma_config = { .dma_burst_len = 0 };

	eth_esp32_reset_desc_chain(dev_data);
	emac_hal_init_mac_default(&dev_data->hal);
	emac_hal_init_dma_default(&dev_data->hal, &dma_config);

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

static const struct device *eth_esp32_phy_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return eth_esp32_phy_dev;
}

static void eth_esp32_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_esp32_dev_data *dev_data = dev->data;

	dev_data->iface = iface;

	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	if (device_is_ready(eth_esp32_phy_dev)) {
		/* Do not start the interface until PHY link is up */
		net_if_carrier_off(iface);

		phy_link_callback_set(eth_esp32_phy_dev, phy_link_state_changed,
				      (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static const struct ethernet_api eth_esp32_api = {
	.iface_api.init		= eth_esp32_iface_init,
	.get_capabilities	= eth_esp32_caps,
	.set_config		= eth_esp32_set_config,
	.get_phy		= eth_esp32_phy_get,
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
