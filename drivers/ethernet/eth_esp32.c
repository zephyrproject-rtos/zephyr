/*
 * Copyright (c) 2022 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
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
#include <soc/soc.h>
#include <clk_ctrl_os.h>

#include "eth.h"
#include "eth_esp32_priv.h"

LOG_MODULE_REGISTER(eth_esp32, CONFIG_ETHERNET_LOG_LEVEL);

#define MAC_RESET_TIMEOUT_MS 100
#define ETH_CRC_LENGTH       4

/*
 * On SoCs with L2 cache (ESP32-P4), DMA data is accessed by the CPU
 * through the non-cacheable address alias (adding SOC_NON_CACHEABLE_OFFSET)
 * while the DMA engine uses the original cacheable address.
 * ADDR_DMA_TO_CPU converts a DMA-visible address to a CPU non-cacheable one.
 * ADDR_CPU_TO_DMA converts back for storing in descriptors.
 */
#if SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE
#define ADDR_DMA_TO_CPU(addr) ((void *)((uintptr_t)(addr) + SOC_NON_CACHEABLE_OFFSET_SRAM))
#define ADDR_CPU_TO_DMA(addr) ((uint32_t)((uintptr_t)(addr) - SOC_NON_CACHEABLE_OFFSET_SRAM))
#else
#define ADDR_DMA_TO_CPU(addr) ((void *)(addr))
#define ADDR_CPU_TO_DMA(addr) ((uint32_t)(addr))
#endif

struct eth_esp32_dma_data {
	uint8_t descriptors[CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM * sizeof(eth_dma_rx_descriptor_t) +
			    CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM * sizeof(eth_dma_tx_descriptor_t)]
		__aligned(EMAC_HAL_DMA_DESC_SIZE);
	uint8_t rx_buf[CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM][CONFIG_ETH_ESP32_DMA_BUFFER_SIZE];
	uint8_t tx_buf[CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM][CONFIG_ETH_ESP32_DMA_BUFFER_SIZE];
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
	uint8_t *dma_rx_buf[CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM];
	uint8_t *dma_tx_buf[CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM];
	struct k_sem int_sem;
	struct k_sem tx_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_ESP32_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
};

struct eth_esp32_config {
	const struct pinctrl_dev_config *pcfg;
};

static const struct device *eth_esp32_phy_dev = DEVICE_DT_GET(
		DT_INST_PHANDLE(0, phy_handle));

PINCTRL_DT_INST_DEFINE(0);
static const struct eth_esp32_config eth_esp32_config = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static eth_dma_rx_descriptor_t *rx_desc_next(struct eth_esp32_dev_data *dev_data,
					     eth_dma_rx_descriptor_t *desc)
{
	if (EMAC_HAL_DMA_DESC_SIZE > 32) {
		eth_dma_rx_descriptor_t *base =
			(eth_dma_rx_descriptor_t *)dev_data->dma->descriptors;
		uint32_t idx = desc - base;

		return &base[(idx + 1) % CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM];
	}
	return ADDR_DMA_TO_CPU(desc->Buffer2NextDescAddr);
}

static eth_dma_tx_descriptor_t *tx_desc_next(struct eth_esp32_dev_data *dev_data,
					     eth_dma_tx_descriptor_t *desc)
{
	if (EMAC_HAL_DMA_DESC_SIZE > 32) {
		eth_dma_tx_descriptor_t *base =
			(eth_dma_tx_descriptor_t *)(dev_data->dma->descriptors +
						    sizeof(eth_dma_rx_descriptor_t) *
							    CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM);
		uint32_t idx = desc - base;

		return &base[(idx + 1) % CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM];
	}
	return ADDR_DMA_TO_CPU(desc->Buffer2NextDescAddr);
}

static void eth_esp32_reset_desc_chain(struct eth_esp32_dev_data *dev_data)
{
	/* Reset DMA descriptor pointers */
	dev_data->rx_desc = (eth_dma_rx_descriptor_t *)(dev_data->dma->descriptors);
	dev_data->tx_desc = (eth_dma_tx_descriptor_t *)(dev_data->dma->descriptors +
							sizeof(eth_dma_rx_descriptor_t) *
								CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM);

	/* Initialize RX descriptor chain/ring */
	for (int i = 0; i < CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM; i++) {
		dev_data->rx_desc[i].RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		dev_data->rx_desc[i].RDES1.ReceiveBuffer1Size = CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		dev_data->rx_desc[i].RDES1.DisableInterruptOnComplete = 0;
		dev_data->rx_desc[i].Buffer1Addr = ADDR_CPU_TO_DMA(dev_data->dma_rx_buf[i]);

		if (EMAC_HAL_DMA_DESC_SIZE > 32) {
			/* Ring mode: DMA strides sequentially, wraps at end-of-ring */
			dev_data->rx_desc[i].RDES1.SecondAddressChained = 0;
			if (i == CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM - 1) {
				dev_data->rx_desc[i].RDES1.ReceiveEndOfRing = 1;
			}
		} else {
			/* Chain mode: DMA follows Buffer2NextDescAddr */
			dev_data->rx_desc[i].RDES1.SecondAddressChained = 1;
			dev_data->rx_desc[i].Buffer2NextDescAddr =
				ADDR_CPU_TO_DMA(dev_data->rx_desc + i + 1);
			if (i == CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM - 1) {
				dev_data->rx_desc[i].Buffer2NextDescAddr =
					ADDR_CPU_TO_DMA(dev_data->rx_desc);
			}
		}
	}

	/* Initialize TX descriptor chain/ring */
	for (int i = 0; i < CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM; i++) {
		dev_data->tx_desc[i].TDES0.Own = EMAC_LL_DMADESC_OWNER_CPU;
		dev_data->tx_desc[i].TDES1.TransmitBuffer1Size = CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		dev_data->tx_desc[i].Buffer1Addr = ADDR_CPU_TO_DMA(dev_data->dma_tx_buf[i]);

		if (EMAC_HAL_DMA_DESC_SIZE > 32) {
			dev_data->tx_desc[i].TDES0.SecondAddressChained = 0;
			if (i == CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM - 1) {
				dev_data->tx_desc[i].TDES0.TransmitEndRing = 1;
			}
		} else {
			dev_data->tx_desc[i].TDES0.SecondAddressChained = 1;
			dev_data->tx_desc[i].Buffer2NextDescAddr =
				ADDR_CPU_TO_DMA(dev_data->tx_desc + i + 1);
			if (i == CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM - 1) {
				dev_data->tx_desc[i].Buffer2NextDescAddr =
					ADDR_CPU_TO_DMA(dev_data->tx_desc);
			}
		}
	}

	/* DMA needs cacheable (bus) addresses for descriptor base */
	emac_hal_set_rx_tx_desc_addr(&dev_data->hal,
				     (eth_dma_rx_descriptor_t *)ADDR_CPU_TO_DMA(dev_data->rx_desc),
				     (eth_dma_tx_descriptor_t *)ADDR_CPU_TO_DMA(dev_data->tx_desc));
}

static uint32_t eth_esp32_transmit_frame(struct eth_esp32_dev_data *dev_data, uint8_t *buf,
					 uint32_t length)
{
	uint32_t bufcount = 0;
	uint32_t lastlen = length;
	uint32_t sentout = 0;

	/* Calculate number of TX buffers needed */
	while (lastlen > CONFIG_ETH_ESP32_DMA_BUFFER_SIZE) {
		lastlen -= CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		bufcount++;
	}
	if (lastlen) {
		bufcount++;
	}
	if (bufcount > CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM) {
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
			memcpy(ADDR_DMA_TO_CPU(desc_iter->Buffer1Addr),
			       buf + i * CONFIG_ETH_ESP32_DMA_BUFFER_SIZE, lastlen);
			sentout += lastlen;
		} else {
			desc_iter->TDES1.TransmitBuffer1Size = CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
			memcpy(ADDR_DMA_TO_CPU(desc_iter->Buffer1Addr),
			       buf + i * CONFIG_ETH_ESP32_DMA_BUFFER_SIZE,
			       CONFIG_ETH_ESP32_DMA_BUFFER_SIZE);
			sentout += CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		}
		desc_iter = tx_desc_next(dev_data, desc_iter);
	}

	/* Give descriptors to DMA */
	for (size_t i = 0; i < bufcount; i++) {
		dev_data->tx_desc->TDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		dev_data->tx_desc = tx_desc_next(dev_data, dev_data->tx_desc);
	}
	emac_hal_transmit_poll_demand(&dev_data->hal);

	return sentout;
}

static void eth_esp32_flush_rx_frame(struct eth_esp32_dev_data *dev_data,
				     eth_dma_rx_descriptor_t *first_desc,
				     eth_dma_rx_descriptor_t *last_desc)
{
	eth_dma_rx_descriptor_t *desc = first_desc;

	while (desc != last_desc) {
		desc->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc = rx_desc_next(dev_data, desc);
	}
	desc->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
}

/* Discard a faulty frame and hand its descriptors back to the DMA. */
static void eth_esp32_drop_rx_frame(struct eth_esp32_dev_data *dev_data,
				    eth_dma_rx_descriptor_t *first_desc,
				    eth_dma_rx_descriptor_t *last_desc)
{
	eth_esp32_flush_rx_frame(dev_data, first_desc, last_desc);
	dev_data->rx_desc = rx_desc_next(dev_data, last_desc);
	emac_hal_receive_poll_demand(&dev_data->hal);
}

/*
 * Locate the descriptor that starts the next pending frame. In ring mode
 * the DMA may have wrapped, so if the tracked descriptor is still owned by
 * the DMA, scan the ring for a CPU-owned first descriptor.
 */
static eth_dma_rx_descriptor_t *eth_esp32_find_rx_start(struct eth_esp32_dev_data *dev_data)
{
	eth_dma_rx_descriptor_t *rx_base = (eth_dma_rx_descriptor_t *)dev_data->dma->descriptors;
	eth_dma_rx_descriptor_t *desc_iter = dev_data->rx_desc;

	if (desc_iter->RDES0.Own != EMAC_LL_DMADESC_OWNER_DMA) {
		return desc_iter;
	}

	for (int i = 0; i < CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM; i++) {
		if (rx_base[i].RDES0.Own == EMAC_LL_DMADESC_OWNER_CPU &&
		    rx_base[i].RDES0.FirstDescriptor) {
			dev_data->rx_desc = &rx_base[i];
			return &rx_base[i];
		}
	}

	return desc_iter;
}

/*
 * Validate the length of a complete frame. Returns the payload length to
 * copy, or 0 if the frame is faulty (in which case it has been dropped).
 *
 * A return of 0 means "no usable frame": faulty frames are dropped here,
 * while the caller treats 0 as nothing to receive. A valid frame is at
 * least a minimum-size Ethernet frame, so the payload length is always
 * greater than 0.
 */
static uint32_t eth_esp32_validate_rx_frame(struct eth_esp32_dev_data *dev_data,
					    eth_dma_rx_descriptor_t *first_desc,
					    eth_dma_rx_descriptor_t *last_desc)
{
	uint32_t ret_len;

	if (last_desc->RDES0.ErrSummary) {
		eth_esp32_drop_rx_frame(dev_data, first_desc, last_desc);
		return 0;
	}

	ret_len = last_desc->RDES0.FrameLength - ETH_CRC_LENGTH;
	if (ret_len > NET_ETH_MAX_FRAME_SIZE) {
		eth_esp32_drop_rx_frame(dev_data, first_desc, last_desc);
		return 0;
	}

	return ret_len;
}

static uint32_t eth_esp32_receive_frame(struct eth_esp32_dev_data *dev_data, uint8_t *buf,
					uint32_t size, uint32_t *frames_remaining)
{
	eth_dma_rx_descriptor_t *desc_iter = eth_esp32_find_rx_start(dev_data);
	eth_dma_rx_descriptor_t *first_desc = NULL;
	uint32_t ret_len = 0;
	uint32_t copy_len = 0;
	uint32_t used_descs = 0;
	uint32_t frame_count = 0;

	*frames_remaining = 0;

	/* Find a complete frame and count remaining frames */
	while ((desc_iter->RDES0.Own == EMAC_LL_DMADESC_OWNER_CPU) &&
	       (used_descs < CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM)) {
		used_descs++;

		if (desc_iter->RDES0.FirstDescriptor && first_desc == NULL) {
			first_desc = desc_iter;
		}

		if (desc_iter->RDES0.LastDescriptor) {
			frame_count++;
			if (frame_count == 1 && first_desc != NULL) {
				ret_len = eth_esp32_validate_rx_frame(dev_data, first_desc,
								      desc_iter);
				if (ret_len == 0) {
					return 0;
				}
			}
		}
		desc_iter = rx_desc_next(dev_data, desc_iter);
	}

	*frames_remaining = (frame_count > 1) ? (frame_count - 1) : 0;

	if (ret_len == 0 || first_desc == NULL) {
		return 0;
	}

	/* Copy frame data */
	copy_len = (ret_len > size) ? size : ret_len;
	desc_iter = first_desc;
	uint32_t remaining = copy_len;

	while (remaining > CONFIG_ETH_ESP32_DMA_BUFFER_SIZE) {
		memcpy(buf, ADDR_DMA_TO_CPU(desc_iter->Buffer1Addr),
		       CONFIG_ETH_ESP32_DMA_BUFFER_SIZE);
		buf += CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		remaining -= CONFIG_ETH_ESP32_DMA_BUFFER_SIZE;
		desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc_iter = rx_desc_next(dev_data, desc_iter);
	}
	memcpy(buf, ADDR_DMA_TO_CPU(desc_iter->Buffer1Addr), remaining);

	/* Return descriptors including any that held CRC */
	while (!desc_iter->RDES0.LastDescriptor) {
		desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;
		desc_iter = rx_desc_next(dev_data, desc_iter);
	}
	desc_iter->RDES0.Own = EMAC_LL_DMADESC_OWNER_DMA;

	dev_data->rx_desc = rx_desc_next(dev_data, desc_iter);
	emac_hal_receive_poll_demand(&dev_data->hal);

	return copy_len;
}

#if !DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
static void eth_esp32_iomux_rmii_clk_input(int gpio_num)
{
	const emac_iomux_info_t *pin;

	pin = esp32_emac_iomux_find(emac_rmii_iomux_pins.clki, gpio_num);
	if (pin != NULL) {
		PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin->gpio_num]);
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin->gpio_num], pin->func);
	}
}
#endif

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

static enum ethernet_hw_caps eth_esp32_caps(const struct device *dev __unused,
					    struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

static int eth_esp32_set_config(const struct device *dev,
				struct net_if *iface __unused,
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

	/*
	 * transmit_frame returns 0 when the next TX descriptor is still owned
	 * by the DMA. Wait for the TX-finish interrupt to recycle one and
	 * retry, rather than dropping the frame outright. Bound the total
	 * wait so a saturated link cannot stall the interface TX lock for
	 * long. send() runs under net_if_tx_lock(), so it is never reentered.
	 */
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(100));

	do {
		if (eth_esp32_transmit_frame(dev_data, dev_data->txb, len) == len) {
			return 0;
		}
	} while (k_sem_take(&dev_data->tx_sem, sys_timepoint_timeout(deadline)) == 0);

	return -EIO;
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

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(dev_data->iface, receive_len,
							   NET_AF_UNSPEC, 0, K_NO_WAIT);
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
		/*
		 * Woken by the receive-finished or receive-buffer-unavailable
		 * interrupt. After draining, the poll demand below resumes the
		 * DMA if it had suspended for lack of RX descriptors.
		 */
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

		emac_hal_receive_poll_demand(&dev_data->hal);
	}
}

IRAM_ATTR static void eth_esp32_isr(void *arg)
{
	const struct device *dev = arg;
	struct eth_esp32_dev_data *const dev_data = dev->data;
	uint32_t intr_stat = emac_ll_get_intr_status(dev_data->hal.dma_regs);

	emac_ll_clear_corresponding_intr(dev_data->hal.dma_regs, intr_stat);

	/*
	 * Wake the RX thread on a finished frame or when the DMA suspends
	 * for lack of RX descriptors; the thread re-issues the receive poll
	 * demand to resume reception.
	 */
	if (intr_stat &
	    (EMAC_LL_DMA_RECEIVE_FINISH_INTR | EMAC_LL_DMA_RECEIVE_BUFF_UNAVAILABLE_INTR)) {
		k_sem_give(&dev_data->int_sem);
	}

	if (intr_stat & EMAC_LL_DMA_TRANSMIT_FINISH_INTR) {
		k_sem_give(&dev_data->tx_sem);
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

static void phy_link_state_changed(const struct device *phy_dev __unused,
				   struct phy_link_state *state,
				   void *user_data)
{
	struct net_if *iface = (struct net_if *)user_data;

	net_eth_carrier_set(iface, state->is_up);
}

int eth_esp32_initialize(const struct device *dev)
{
	struct eth_esp32_dev_data *const dev_data = dev->data;
	const struct eth_esp32_config *const cfg = dev->config;
	int res;

	k_sem_init(&dev_data->int_sem, 0, 1);
	k_sem_init(&dev_data->tx_sem, 0, 1);

	const struct device *clock_dev =
		DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(eth)));
	clock_control_subsys_t clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(eth), offset);

	/* clock is shared, so do not bail out if already enabled */
	res = clock_control_on(clock_dev, clock_subsys);
	if (res < 0 && res != -EALREADY) {
		goto err;
	}

	/*
	 * On SoCs with L2 cache, remap DMA data to non-cacheable
	 * alias so CPU and DMA see the same memory without cache ops.
	 */
#if SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE
	dev_data->dma = ADDR_DMA_TO_CPU(dev_data->dma);
#endif

	/* Convert 2D array DMA buffers to arrays of pointers */
	for (int i = 0; i < CONFIG_ETH_ESP32_DMA_RX_BUFFER_NUM; i++) {
		dev_data->dma_rx_buf[i] = dev_data->dma->rx_buf[i];
	}
	for (int i = 0; i < CONFIG_ETH_ESP32_DMA_TX_BUFFER_NUM; i++) {
		dev_data->dma_tx_buf[i] = dev_data->dma->tx_buf[i];
	}

	/* Initialize HAL context */
	emac_hal_init(&dev_data->hal);

	/* Configure ISR */
	res = esp_intr_alloc(
		DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, priority)) |
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(eth), 0, flags)) |
			ESP_INTR_FLAG_IRAM,
		eth_esp32_isr, (void *)(uintptr_t)dev, NULL);
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
		int rmii_clk_gpio = -1;

		res = esp32_emac_iomux_init_rmii_pinctrl(cfg->pcfg, &rmii_clk_gpio);
		if (res != 0) {
			goto err;
		}
#if !DT_INST_NODE_HAS_PROP(0, ref_clk_output_gpios)
		eth_esp32_iomux_rmii_clk_input(rmii_clk_gpio);
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

	/*
	 * Enable the TX-finish interrupt (for TX descriptor recycling) and
	 * the RX buffer-unavailable interrupt (raised when the DMA suspends
	 * after running out of CPU-owned RX descriptors). The latter is
	 * reported under the abnormal interrupt summary, so enable that too.
	 */
	emac_ll_enable_corresponding_intr(dev_data->hal.dma_regs,
					  EMAC_LL_INTR_TRANSMIT_ENABLE |
						  EMAC_LL_INTR_RECEIVE_BUFF_UNAVAILABLE_ENABLE |
						  EMAC_LL_INTR_ABNORMAL_SUMMARY_ENABLE);

	/*
	 * The HAL sets desc_skip_len=0 assuming 32-byte descriptors.
	 * On SoCs with cache-aligned descriptors (64B on P4), tell
	 * the DMA to skip the padding between descriptors.
	 */
	if (EMAC_HAL_DMA_DESC_SIZE > 32) {
		emac_ll_set_desc_skip_len(dev_data->hal.dma_regs,
					  (EMAC_HAL_DMA_DESC_SIZE - 32) / 4);
	}

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

static const struct device *eth_esp32_phy_get(const struct device *dev __unused,
					      struct net_if *iface __unused)
{
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
				      (void *)iface);
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

/* DMA data must be in DRAM, descriptors must be aligned to EMAC_HAL_DMA_DESC_SIZE */
static struct eth_esp32_dma_data eth_esp32_dma_data __aligned(EMAC_HAL_DMA_DESC_SIZE) DRAM_ATTR;

static struct eth_esp32_dev_data eth_esp32_dev = {
	.dma = &eth_esp32_dma_data,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_esp32_initialize, NULL, &eth_esp32_dev, &eth_esp32_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_esp32_api, NET_ETH_MTU);
