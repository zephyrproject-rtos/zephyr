/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/lldp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include "eth.h"

LOG_MODULE_REGISTER(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_ethernet

#define ETH_STM32_HAL_MTU NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

#define ETH_STM32_RX_BUF_SIZE HAL_ETH_MAX_PACKET_SIZE_BYTE /* full-frame RX DMA buffer */
#define ETH_STM32_TX_BUF_SIZE HAL_ETH_MAX_PACKET_SIZE_BYTE /* full-frame TX DMA buffer */

#define ETH_STM32_DESC_SIZE 24U /* size of HAL_ETH DMA descriptor */

#define ETH_STM32_TX_DESC_COUNT (2U * CONFIG_ETH_STM32_HAL_TX_MAX_BUF)
#define ETH_STM32_RX_DESC_COUNT (2U * CONFIG_ETH_STM32_HAL_RX_DMA_BUF_COUNT)

#define ETH_STM32_TX_DESC_MEM_SIZE (ETH_STM32_TX_DESC_COUNT * ETH_STM32_DESC_SIZE)
#define ETH_STM32_RX_DESC_MEM_SIZE (ETH_STM32_RX_DESC_COUNT * ETH_STM32_DESC_SIZE)

#define ETH_STM32_DESC_MEM_ALIGN ETH_BUS_DATA_WIDTH_BYTE

NET_BUF_POOL_DEFINE(rx_net_buf_pool, CONFIG_ETH_STM32_HAL_RX_BUF_POOL_COUNT, ETH_STM32_RX_BUF_SIZE,
		    0, NULL);

static struct net_buf *rx_net_buf[CONFIG_ETH_STM32_HAL_RX_DMA_BUF_COUNT];

static hal_eth_tx_channel_config_t tx_ch_cfg;
static hal_eth_rx_channel_config_t rx_ch_cfg;

static const struct device *eth_stm32_phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle));

struct eth_stm32_hal_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	hal_eth_handle_t heth;
	struct k_event rx_event;
	struct k_sem tx_int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_STM32_HAL_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;

	uint8_t tx_desc_mem[ETH_STM32_TX_DESC_MEM_SIZE] __aligned(ETH_STM32_DESC_MEM_ALIGN);
	uint8_t rx_desc_mem[ETH_STM32_RX_DESC_MEM_SIZE] __aligned(ETH_STM32_DESC_MEM_ALIGN);
};

static struct eth_stm32_hal_dev_data eth0_data = {.heth = {.instance = HAL_ETH1}};

struct eth_stm32_hal_dev_cfg {
	void (*config_func)(void);
	struct net_eth_mac_config mac_config;
	const struct stm32_pclken *pclken;
	size_t pclken_cnt;
	const struct pinctrl_dev_config *pcfg;
};

hal_status_t tx_channel_callback(hal_eth_handle_t *heth, uint32_t channel, void *addr,
				 hal_eth_tx_cb_pkt_data_t data)
{
	ARG_UNUSED(channel);
	ARG_UNUSED(addr);

	__ASSERT_NO_MSG(heth != NULL);
	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth, struct eth_stm32_hal_dev_data, heth);

	/* Free net_pkt when last descriptor is done */
#if defined(HAL_ETH_TX_STATUS_LD)
	if ((data.status & HAL_ETH_TX_STATUS_LD) != 0U) {
		if (data.p_data != NULL) {
			net_pkt_unref((struct net_pkt *)data.p_data);
		}
	}
#else
	if (data.p_data != NULL) {
		net_pkt_unref((struct net_pkt *)data.p_data);
	}
#endif

	k_sem_give(&dev_data->tx_int_sem);

	return HAL_OK;
}

hal_status_t rx_channel_callback(hal_eth_handle_t *h_eth, uint32_t channel, void *addr,
				 uint32_t size, hal_eth_rx_cb_pkt_data_t data)
{
	ARG_UNUSED(channel);
	ARG_UNUSED(addr);

	__ASSERT_NO_MSG(h_eth != NULL);
	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(h_eth, struct eth_stm32_hal_dev_data, heth);

	int idx = (int)(uintptr_t)data.p_data;

	if (idx < 0 || idx >= CONFIG_ETH_STM32_HAL_RX_DMA_BUF_COUNT) {
		LOG_ERR("RX: Invalid buffer index");
		return HAL_ERROR;
	}

	struct net_buf *frag = rx_net_buf[idx];

	rx_net_buf[idx] = NULL;

	if (!frag || size == 0U || size > frag->size) {
		LOG_ERR("RX: Invalid frag or size");
		if (frag) {
			net_buf_unref(frag);
		}
		return HAL_ERROR;
	}

	frag->len = size;
	struct net_pkt *pkt = net_pkt_rx_alloc(K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("RX: net_pkt_rx_alloc failed");
		net_buf_unref(frag);
		return HAL_ERROR;
	}

	pkt->frags = frag;
	int ret = net_recv_data(dev_data->iface, pkt);

	if (ret < 0) {
		LOG_ERR("RX: net_recv_data failed");
		net_pkt_unref(pkt);
		return HAL_ERROR;
	}

	return HAL_OK;
}

static void eth_rx_allocate_cb(hal_eth_handle_t *p_eth, uint32_t channel, uint32_t rx_buf_size,
			       void **p_rx_buffer, void **p_app_context)
{
	ARG_UNUSED(p_eth);
	ARG_UNUSED(channel);
	ARG_UNUSED(rx_buf_size);

	for (int idx = 0; idx < CONFIG_ETH_STM32_HAL_RX_DMA_BUF_COUNT; idx++) {
		if (rx_net_buf[idx] == NULL) {
			struct net_buf *frag = net_buf_alloc(&rx_net_buf_pool, K_NO_WAIT);

			if (!frag) {
				break;
			}

			rx_net_buf[idx] = frag;
			*p_rx_buffer = frag->data;
			*p_app_context = (void *)(uintptr_t)idx;
			return;
		}
	}

	*p_rx_buffer = NULL;
	*p_app_context = NULL;
}

static void app_data_event_cb(hal_eth_handle_t *heth, uint32_t channels_mask)
{
	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth, struct eth_stm32_hal_dev_data, heth);

	k_event_post(&dev_data->rx_event, channels_mask);
}

static void eth_stm32_set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_eth_mac_config_t mac_conf = {0};
	hal_status_t hal_status;

	HAL_ETH_MAC_GetConfig(heth, &mac_conf);
	mac_conf.link_config.duplex_mode = PHY_LINK_IS_FULL_DUPLEX(state->speed)
							? HAL_ETH_MAC_FULL_DUPLEX_MODE
							: HAL_ETH_MAC_HALF_DUPLEX_MODE;
	mac_conf.link_config.speed = PHY_LINK_IS_SPEED_100M(state->speed)
							? HAL_ETH_MAC_SPEED_100M
							: HAL_ETH_MAC_SPEED_10M;

	hal_status = HAL_ETH_MAC_SetConfig(heth, &mac_conf);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_MAC_SetConfig failed: %x", hal_status);
	}
}

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	ARG_UNUSED(phy_dev);

	if (state->is_up) {
		eth_stm32_set_mac_config(dev, state);
		net_eth_carrier_on(dev_data->iface);
	} else {
		net_eth_carrier_off(dev_data->iface);
	}
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	uint32_t channels_mask;
	uint32_t return_mask;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		channels_mask = k_event_wait_safe(&dev_data->rx_event,
						  UINT32_MAX, false, K_FOREVER);

		if (channels_mask != 0U) {
			HAL_ETH_ExecDataHandler(&dev_data->heth, channels_mask, &return_mask);
		}
	}
}

static int eth_stm32_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_eth_buffer_t eth_buffer[CONFIG_ETH_STM32_HAL_TX_MAX_BUF];
	uint32_t buffer_count = 0;
	hal_status_t hal_status;
	uint32_t output_channel_mask = 0;
	size_t total_len = 0;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	/* Build TX buffer chain directly from net_buf fragments */
	for (struct net_buf *frag = pkt->frags; frag != NULL; frag = frag->frags) {
		if (frag->len == 0U) {
			/* Skip empty fragments */
			continue;
		}
		if (buffer_count >= CONFIG_ETH_STM32_HAL_TX_MAX_BUF) {
			return -ENOBUFS;
		}
		eth_buffer[buffer_count].p_buffer = frag->data;
		eth_buffer[buffer_count].len_byte = frag->len;
		total_len += frag->len;
		buffer_count++;
	}
	if (buffer_count == 0U || total_len == 0U) {
		return -EINVAL;
	}

	if (total_len > ETH_STM32_TX_BUF_SIZE) {
		LOG_ERR("TX packet too big");
		return -EFBIG;
	}

	/* Keep pkt alive until TX complete callback */
	net_pkt_ref(pkt);

	hal_eth_tx_pkt_config_t tx_pkt_conf = {0};

	tx_pkt_conf.attributes = HAL_ETH_TX_PKT_CTRL_CRCPAD;
	tx_pkt_conf.crc_pad_ctrl = HAL_ETH_TX_PKT_CRC_PAD_INSERT;
	tx_pkt_conf.notify = HAL_ETH_TX_PKT_NOTIFY_ENABLE;
	tx_pkt_conf.csum_ctrl = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM)
					? HAL_ETH_TX_PKT_CSUM_PAYLOAD_HEADER_INSERT
					: HAL_ETH_TX_PKT_CSUM_DISABLE;
	if (IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM)) {
		tx_pkt_conf.attributes |= HAL_ETH_TX_PKT_CTRL_CSUM;
	}

	tx_pkt_conf.p_data = pkt;

	hal_status = HAL_ETH_RequestTx(heth, HAL_ETH_TX_CHANNEL_0, eth_buffer, buffer_count,
				       &tx_pkt_conf);

	if (hal_status == HAL_BUSY) {
		HAL_ETH_ExecDataHandler(heth, HAL_ETH_TX_CHANNEL_0, &output_channel_mask);
		net_pkt_unref(pkt);
		return -EAGAIN;
	}

	if (hal_status != HAL_OK) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;

	dev_data->iface = iface;

	net_if_set_link_addr(iface, dev_data->mac_addr, sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);
	ethernet_init(iface);

	net_if_carrier_off(iface);

	net_lldp_set_lldpdu(iface);

	if (device_is_ready(eth_stm32_phy_dev)) {
		phy_link_callback_set(eth_stm32_phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	__ASSERT_NO_MSG(cfg->config_func != NULL);
	cfg->config_func();

	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack), rx_thread,
			(void *)dev, NULL, NULL,
			IS_ENABLED(CONFIG_ETH_STM32_HAL_RX_THREAD_PREEMPTIVE)
				? K_PRIO_PREEMPT(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO)
				: K_PRIO_COOP(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&dev_data->rx_thread, "stm_eth");
}

static const struct device *eth_stm32_hal_get_phy(const struct device *dev __unused,
						  struct net_if *iface __unused)
{
	return eth_stm32_phy_dev;
}

static enum ethernet_hw_caps eth_stm32_hal_get_capabilities(const struct device *dev __unused,
							    struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_ETH_STM32_HW_CHECKSUM)
		| ETHERNET_HW_RX_CHKSUM_OFFLOAD | ETHERNET_HW_TX_CHKSUM_OFFLOAD
#endif
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
		;
}

static int eth_stm32_hal_channel_config(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	hal_status_t hal_status;
	hal_eth_handle_t *heth = &dev_data->heth;

	hal_eth_channel_alloc_needs_t alloc_needs_tx = {0};
	hal_eth_channel_alloc_needs_t alloc_needs_rx = {0};

	HAL_ETH_RegisterDataCallback(heth, &app_data_event_cb);

	/* TX channel config */
	HAL_ETH_GetConfigTxChannel(heth, HAL_ETH_TX_CHANNEL_ALL, &tx_ch_cfg);

	tx_ch_cfg.max_app_buffers_num = CONFIG_ETH_STM32_HAL_TX_MAX_BUF;
	tx_ch_cfg.req_desc_size_align_byte = 1UL;
	tx_ch_cfg.fifo_event_config.event_mode = HAL_ETH_FIFO_EVENT_ALWAYS;
	tx_ch_cfg.dma_channel_config.tx_dma_burst_length = HAL_ETH_DMA_TX_BLEN_4_BEAT;
	tx_ch_cfg.dma_channel_config.tx_pbl_x8_mode = HAL_ETH_DMA_TX_PBL_X8_DISABLE;
	tx_ch_cfg.dma_channel_config.tx_second_pkt_operate = HAL_ETH_DMA_TX_SEC_PKT_OP_ENABLE;
	tx_ch_cfg.mtl_queue_config.queue_size_byte = HAL_ETH_MTL_TX_QUEUE_SZ_2048_BYTE;
	tx_ch_cfg.mtl_queue_config.transmit_queue_mode = HAL_ETH_MTL_TX_Q_STORE_AND_FORWARD;
	tx_ch_cfg.mtl_queue_config.queue_op_mode = HAL_ETH_MTL_TX_QUEUE_ENABLED;

	hal_status = HAL_ETH_SetConfigTxChannel(heth, HAL_ETH_TX_CHANNEL_ALL, &tx_ch_cfg);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_SetConfigTxChannel failed: %x", hal_status);
		return -EIO;
	}

	HAL_ETH_GetChannelAllocNeeds(heth, HAL_ETH_TX_CHANNEL_ALL, &alloc_needs_tx);

	if ((alloc_needs_tx.mem_size_byte > sizeof(dev_data->tx_desc_mem)) ||
	    (alloc_needs_tx.mem_addr_align_byte > ETH_STM32_DESC_MEM_ALIGN)) {
		LOG_ERR("TX descriptor mismatch");
		return -ENOMEM;
	}

	/* RX allocate callback */
	hal_status = HAL_ETH_RegisterChannelRxAllocateCallback(heth, HAL_ETH_RX_CHANNEL_ALL,
							       eth_rx_allocate_cb);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_RegisterChannelRxAllocateCallback failed: %x", hal_status);
		return -EIO;
	}

	/* RX channel config */
	HAL_ETH_GetConfigRxChannel(heth, HAL_ETH_RX_CHANNEL_ALL, &rx_ch_cfg);

	rx_ch_cfg.max_app_buffers_num = CONFIG_ETH_STM32_HAL_RX_DMA_BUF_COUNT;
	rx_ch_cfg.req_desc_size_align_byte = 1UL;
	rx_ch_cfg.fifo_event_config.event_mode = HAL_ETH_FIFO_EVENT_ALWAYS;
	rx_ch_cfg.dma_channel_config.rx_dma_burst_length = HAL_ETH_DMA_RX_BLEN_4_BEAT;
	rx_ch_cfg.mtl_queue_config.queue_size_byte = HAL_ETH_MTL_RX_QUEUE_SZ_2048_BYTE;
	rx_ch_cfg.mtl_queue_config.receive_queue_mode = HAL_ETH_MTL_RX_Q_STORE_AND_FORWARD;
	rx_ch_cfg.mtl_queue_config.queue_op_mode = HAL_ETH_MTL_RX_QUEUE_ENABLED;
	rx_ch_cfg.mtl_queue_config.drop_tcp_ip_csum_error_pkt = HAL_ETH_MTL_RX_DROP_CS_ERR_ENABLE;
	rx_ch_cfg.mtl_queue_config.fwd_error_pkt = HAL_ETH_MTL_RX_FWD_ERR_PKT_DISABLE;
	rx_ch_cfg.mtl_queue_config.fwd_undersized_good_pkt = HAL_ETH_MTL_RX_FWD_USZ_PKT_ENABLE;

	/* Use full-frame DMA buffer size */
	rx_ch_cfg.dma_channel_config.rx_buffer_len_byte = ETH_STM32_RX_BUF_SIZE;

	hal_status = HAL_ETH_SetConfigRxChannel(heth, HAL_ETH_RX_CHANNEL_ALL, &rx_ch_cfg);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_SetConfigRxChannel failed: %x", hal_status);
		return -EIO;
	}

	HAL_ETH_GetChannelAllocNeeds(heth, HAL_ETH_RX_CHANNEL_ALL, &alloc_needs_rx);

	if ((alloc_needs_rx.mem_size_byte > sizeof(dev_data->rx_desc_mem)) ||
	    (alloc_needs_rx.mem_addr_align_byte > ETH_STM32_DESC_MEM_ALIGN)) {
		LOG_ERR("RX descriptor mismatch");
		return -ENOMEM;
	}

	void *p_tx_desc = dev_data->tx_desc_mem;
	void *p_rx_desc = dev_data->rx_desc_mem;

	/* TX complete callback */
	hal_status = HAL_ETH_RegisterChannelTxCptCallback(heth, HAL_ETH_TX_CHANNEL_ALL,
							  tx_channel_callback);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_RegisterChannelTxCptCallback failed: %x", hal_status);
		return -EIO;
	}

	/* RX complete callback */
	hal_status = HAL_ETH_RegisterChannelRxCptCallback(heth, HAL_ETH_RX_CHANNEL_ALL,
							  rx_channel_callback);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_RegisterChannelRxCptCallback failed: %x", hal_status);
		return -EIO;
	}

	hal_status = HAL_ETH_StartChannel(heth, HAL_ETH_TX_CHANNEL_ALL, p_tx_desc,
					  alloc_needs_tx.mem_size_byte);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_StartChannel TX failed: %x", hal_status);
		return -EIO;
	}

	hal_status = HAL_ETH_StartChannel(heth, HAL_ETH_RX_CHANNEL_ALL, p_rx_desc,
					  alloc_needs_rx.mem_size_byte);
	if (hal_status != HAL_OK) {
		LOG_ERR("HAL_ETH_StartChannel RX failed: %x", hal_status);
		LOG_ERR("Closing TX channels");
		HAL_ETH_StopChannel(heth, HAL_ETH_TX_CHANNEL_ALL);
		return -EIO;
	}

	return 0;
}

static int eth_initialize(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_eth_config_t config_eth = (hal_eth_config_t){0};
	int ret = 0;

	k_sem_init(&dev_data->tx_int_sem, 0, 1);
	k_event_init(&dev_data->rx_event);

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Set up gated and source clocks */
	for (size_t n = 0; n < cfg->pclken_cnt; n++) {
		if (IN_RANGE(cfg->pclken[n].bus, STM32_PERIPH_BUS_MIN, STM32_PERIPH_BUS_MAX)) {
			ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t)&cfg->pclken[n]);
		} else {
			ret = clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t)&cfg->pclken[n],
					NULL);
		}

		if (ret != 0) {
			LOG_ERR("Failed to setup ethernet clock #%zu", n);
			return -EIO;
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	if (HAL_ETH_Init(heth, heth->instance) != HAL_OK) {
		LOG_ERR("Failed to initialize HAL");
		return -EIO;
	}

	HAL_ETH_GetConfig(heth, &config_eth);

	ret = net_eth_mac_load(&cfg->mac_config, dev_data->mac_addr);
	if (ret == -ENODATA) {
		uint8_t unique_device_ID_12_bytes[12];
		uint32_t result_mac_32_bits;

		/**
		 * Set MAC address locally administered bit (LAA) as this is not assigned by the
		 * manufacturer
		 */
		dev_data->mac_addr[0] = ST_OUI_B0 | 0x02;
		dev_data->mac_addr[1] = ST_OUI_B1;
		dev_data->mac_addr[2] = ST_OUI_B2;

		/* Nothing defined by the user, use device id */
		hwinfo_get_device_id(unique_device_ID_12_bytes, 12);
		result_mac_32_bits = crc32_ieee((uint8_t *)unique_device_ID_12_bytes, 12);
		memcpy(&dev_data->mac_addr[3], &result_mac_32_bits, 3);

		ret = 0;
	}

	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
		return ret;
	}

	memcpy(config_eth.mac_addr, dev_data->mac_addr, sizeof(dev_data->mac_addr));

	config_eth.media_interface = HAL_ETH_MEDIA_IF_RMII;
	HAL_RCC_SBS_EnableClock();

	if (HAL_ETH_SetConfig(heth, &config_eth) != HAL_OK) {
		LOG_ERR("Failed to set config");
		return -EIO;
	}

	ret = eth_stm32_hal_channel_config(dev);
	if (ret < 0) {
		LOG_ERR("Failed to config channels, err:%x", ret);
		return ret;
	}

	LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		config_eth.mac_addr[0], config_eth.mac_addr[1],
		config_eth.mac_addr[2], config_eth.mac_addr[3],
		config_eth.mac_addr[4], config_eth.mac_addr[5]);

	return 0;
}

static void eth_isr(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;

	HAL_ETH_IRQHandler(heth);
}

static int eth_stm32_hal_set_config(const struct device *dev,
				    struct net_if *iface __unused,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	hal_eth_handle_t *heth = &dev_data->heth;
	hal_eth_config_t eth_cfg = {0};

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		HAL_ETH_GetConfig(heth, &eth_cfg);
		memcpy(eth_cfg.mac_addr, dev_data->mac_addr, 6);

		if (HAL_ETH_SetConfig(heth, &eth_cfg) != HAL_OK) {
			return -EIO;
		}
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_stm32_hal_get_capabilities,
	.get_phy = eth_stm32_hal_get_phy,
	.send = eth_stm32_tx,
	.set_config = eth_stm32_hal_set_config,
};

static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

PINCTRL_DT_INST_DEFINE(0);

static const struct stm32_pclken eth0_pclken[] = STM32_DT_CLOCKS(DT_DRV_INST(0));

static const struct eth_stm32_hal_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.mac_config = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
	.pclken = eth0_pclken,
	.pclken_cnt = ARRAY_SIZE(eth0_pclken),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize, NULL, &eth0_data, &eth0_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU);
