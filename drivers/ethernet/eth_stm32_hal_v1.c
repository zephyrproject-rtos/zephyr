/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ethernet

#define LOG_MODULE_NAME eth_stm32_hal
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <ethernet/eth_stats.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/net/lldp.h>
#include <zephyr/drivers/hwinfo.h>

#if defined(CONFIG_NET_DSA_DEPRECATED)
#include <zephyr/net/dsa.h>
#endif

#include "eth.h"
#include "eth_stm32_hal_priv.h"

#if DT_INST_PROP(0, zephyr_random_mac_address)
#define ETH_STM32_RANDOM_MAC
#endif

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    !DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#error DTCM for DMA buffer is activated but zephyr,dtcm is not present in dts
#endif

static const struct device *eth_stm32_phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle));

#define ETH_STM32_AUTO_NEGOTIATION_ENABLE                                                          \
	UTIL_NOT(DT_NODE_HAS_PROP(DT_INST_PHANDLE(0, phy_handle), fixed_link))

#define IS_ETH_DMATXDESC_OWN(dma_tx_desc)	(dma_tx_desc->Status & \
							ETH_DMATXDESC_OWN)

#define ETH_MII_MODE	ETH_MEDIA_INTERFACE_MII
#define ETH_RMII_MODE	ETH_MEDIA_INTERFACE_RMII

#define STM32_ETH_PHY_MODE(inst) \
	(DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ? \
		ETH_MII_MODE : ETH_RMII_MODE)

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#define __eth_stm32_desc __dtcm_noinit_section
#define __eth_stm32_buf  __dtcm_noinit_section
#elif defined(CONFIG_NOCACHE_MEMORY)
#define __eth_stm32_desc __nocache __aligned(4)
#define __eth_stm32_buf  __nocache __aligned(4)
#else
#define __eth_stm32_desc __aligned(4)
#define __eth_stm32_buf  __aligned(4)
#endif

static ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __eth_stm32_desc;
static ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __eth_stm32_desc;

static uint8_t dma_rx_buffer[ETH_RXBUFNB][ETH_STM32_RX_BUF_SIZE] __eth_stm32_buf;
static uint8_t dma_tx_buffer[ETH_TXBUFNB][ETH_STM32_TX_BUF_SIZE] __eth_stm32_buf;

static void setup_mac_filter(ETH_HandleTypeDef *heth)
{
	__ASSERT_NO_MSG(heth != NULL);
	uint32_t tmp = heth->Instance->MACFFR;

	/* clear all multicast filter bits, resulting in perfect filtering */
	tmp &= ~(ETH_MULTICASTFRAMESFILTER_PERFECTHASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_HASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_PERFECT |
		 ETH_MULTICASTFRAMESFILTER_NONE);

	if (IS_ENABLED(CONFIG_ETH_STM32_MULTICAST_FILTER)) {
		/* enable multicast hash receive filter */
		tmp |= ETH_MULTICASTFRAMESFILTER_HASHTABLE;
	} else {
		/* enable receiving all multicast frames */
		tmp |= ETH_MULTICASTFRAMESFILTER_NONE;
	}

	heth->Instance->MACFFR = tmp;

	/* Wait until the write operation will be taken into account:
	 * at least four TX_CLK/RX_CLK clock cycles
	 */
	tmp = heth->Instance->MACFFR;
	k_sleep(K_MSEC(1));
	heth->Instance->MACFFR = tmp;
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int res;
	size_t total_len;
	uint8_t *dma_buffer;

	__IO ETH_DMADescTypeDef *dma_tx_desc;

	HAL_StatusTypeDef hal_ret = HAL_OK;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	total_len = net_pkt_get_len(pkt);
	if (total_len > (ETH_STM32_TX_BUF_SIZE * ETH_TXBUFNB)) {
		LOG_ERR("PKT too big");
		return -EIO;
	}

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	dma_tx_desc = heth->TxDesc;
	while (IS_ETH_DMATXDESC_OWN(dma_tx_desc) != (uint32_t)RESET) {
		k_yield();
	}

	dma_buffer = (uint8_t *)(dma_tx_desc->Buffer1Addr);

	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		res = -ENOBUFS;
		goto error;
	}

	hal_ret = HAL_ETH_TransmitFrame(heth, total_len);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
		goto error;
	}

	/* When Transmit Underflow flag is set, clear it and issue a
	 * Transmit Poll Demand to resume transmission.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_TUS) != (uint32_t)RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_TUS;
		/* Resume DMA transmission*/
		heth->Instance->DMATPDR = 0;
		res = -EIO;
		goto error;
	}

	res = 0;
error:

	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

static struct net_if *get_iface(struct eth_stm32_hal_dev_data *ctx)
{
	return ctx->iface;
}

static struct net_pkt *eth_rx(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	struct net_pkt *pkt;
	size_t total_len = 0;
	__IO ETH_DMADescTypeDef *dma_rx_desc;
	uint8_t *dma_buffer;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	hal_ret = HAL_ETH_GetReceivedFrame_IT(heth);
	if (hal_ret != HAL_OK) {
		/* no frame available */
		return NULL;
	}

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (uint8_t *)heth->RxFrameInfos.buffer;

	pkt = net_pkt_rx_alloc_with_buffer(get_iface(dev_data),
					   total_len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, dma_buffer, total_len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

release_desc:
	/* Release descriptors to DMA */
	/* Point to first descriptor */
	dma_rx_desc = heth->RxFrameInfos.FSRxDesc;
	/* Set Own bit in Rx descriptors: gives the buffers back to DMA */
	for (int i = 0; i < heth->RxFrameInfos.SegCount; i++) {
		dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
		dma_rx_desc = (ETH_DMADescTypeDef *)
			(dma_rx_desc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth->RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it
	 * and resume reception.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_RBUS) != (uint32_t)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		heth->Instance->DMARPDR = 0;
	}

	if (!pkt) {
		goto out;
	}

out:
	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(dev_data));
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	struct net_if *iface;
	struct net_pkt *pkt;
	int res;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		res = k_sem_take(&dev_data->rx_int_sem, K_FOREVER);
		if (res == 0) {
			/* semaphore taken and receive packets */
			while ((pkt = eth_rx(dev)) != NULL) {
				iface = net_pkt_iface(pkt);
#if defined(CONFIG_NET_DSA_DEPRECATED)
				iface = dsa_net_recv(iface, &pkt);
#endif
				res = net_recv_data(iface, pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(
							net_pkt_iface(pkt));
					LOG_ERR("Failed to enqueue frame "
						"into RX queue: %d", res);
					net_pkt_unref(pkt);
				}
			}
		}
	}
}

static void eth_isr(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;

	HAL_ETH_IRQHandler(heth);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth_handle)
{
	__ASSERT_NO_MSG(heth_handle != NULL);

	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth_handle, struct eth_stm32_hal_dev_data, heth);

	__ASSERT_NO_MSG(dev_data != NULL);

	k_sem_give(&dev_data->rx_int_sem);
}

static void generate_mac(uint8_t *mac_addr)
{
#if defined(ETH_STM32_RANDOM_MAC)
	/* "zephyr,random-mac-address" is set, generate a random mac address */
	gen_random_mac(mac_addr, ST_OUI_B0, ST_OUI_B1, ST_OUI_B2);
#else /* Use user defined mac address */
	mac_addr[0] = ST_OUI_B0;
	mac_addr[1] = ST_OUI_B1;
	mac_addr[2] = ST_OUI_B2;
#if NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	mac_addr[3] = NODE_MAC_ADDR_OCTET(DT_DRV_INST(0), 3);
	mac_addr[4] = NODE_MAC_ADDR_OCTET(DT_DRV_INST(0), 4);
	mac_addr[5] = NODE_MAC_ADDR_OCTET(DT_DRV_INST(0), 5);
#else
	uint8_t unique_device_ID_12_bytes[12];
	uint32_t result_mac_32_bits;

	/* Nothing defined by the user, use device id */
	hwinfo_get_device_id(unique_device_ID_12_bytes, 12);
	result_mac_32_bits = crc32_ieee((uint8_t *)unique_device_ID_12_bytes, 12);
	memcpy(&mac_addr[3], &result_mac_32_bits, 3);

	/**
	 * Set MAC address locally administered bit (LAA) as this is not assigned by the
	 * manufacturer
	 */
	mac_addr[0] |= 0x02;

#endif /* NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))) */
#endif
}

static int eth_initialize(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int ret = 0;

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enable clock */
	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken);
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken_tx);
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken_rx);

	if (ret) {
		LOG_ERR("Failed to enable ethernet clock");
		return -EIO;
	}

	/* configure pinmux */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	generate_mac(dev_data->mac_addr);

	heth->Init.MACAddr = dev_data->mac_addr;

	HAL_StatusTypeDef hal_ret = HAL_OK;

	if (!ETH_STM32_AUTO_NEGOTIATION_ENABLE) {
		struct phy_link_state state;

		phy_get_link_state(eth_stm32_phy_dev, &state);

		heth->Init.DuplexMode = PHY_LINK_IS_FULL_DUPLEX(state.speed) ? ETH_MODE_FULLDUPLEX
									     : ETH_MODE_HALFDUPLEX;
		heth->Init.Speed =
			PHY_LINK_IS_SPEED_100M(state.speed) ? ETH_SPEED_100M : ETH_SPEED_10M;
	}

	hal_ret = HAL_ETH_Init(heth);
	if (hal_ret == HAL_TIMEOUT) {
		/* HAL Init time out. This could be linked to */
		/* a recoverable error. Log the issue and continue */
		/* driver initialisation */
		LOG_WRN("HAL_ETH_Init timed out (cable not connected?)");
	} else if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return -EINVAL;
	}

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);

	HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
		&dma_tx_buffer[0][0], ETH_TXBUFNB);
	HAL_ETH_DMARxDescListInit(heth, dma_rx_desc_tab,
		&dma_rx_buffer[0][0], ETH_RXBUFNB);

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	return 0;
}

#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
static void eth_stm32_mcast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct eth_stm32_hal_dev_data *dev_data = (struct eth_stm32_hal_dev_data *)dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	uint32_t crc;
	uint32_t hash_table[2];
	uint32_t hash_index;

	crc = __RBIT(crc32_ieee(filter->mac_address.addr, sizeof(struct net_eth_addr)));
	hash_index = (crc >> 26) & 0x3f;

	__ASSERT_NO_MSG(hash_index < ARRAY_SIZE(dev_data->hash_index_cnt));

	hash_table[0] = heth->Instance->MACHTLR;
	hash_table[1] = heth->Instance->MACHTHR;

	if (filter->set) {
		dev_data->hash_index_cnt[hash_index]++;
		hash_table[hash_index / 32] |= (1 << (hash_index % 32));
	} else {
		if (dev_data->hash_index_cnt[hash_index] == 0) {
			__ASSERT_NO_MSG(false);
			return;
		}

		dev_data->hash_index_cnt[hash_index]--;
		if (dev_data->hash_index_cnt[hash_index] == 0) {
			hash_table[hash_index / 32] &= ~(1 << (hash_index % 32));
		}
	}

	heth->Instance->MACHTLR = hash_table[0];
	heth->Instance->MACHTHR = hash_table[1];
}

#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */

static void set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	heth->Init.DuplexMode =
		PHY_LINK_IS_FULL_DUPLEX(state->speed) ? ETH_MODE_FULLDUPLEX : ETH_MODE_HALFDUPLEX;

	heth->Init.Speed = PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_SPEED_100M : ETH_SPEED_10M;

	hal_ret = HAL_ETH_ConfigMAC(heth, NULL);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_ConfigMAC: failed: %d", hal_ret);
	}
}

static int eth_stm32_hal_start(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Starting ETH HAL driver");

	hal_ret = HAL_ETH_Start(heth);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Start{_IT} failed");
	}

	return 0;
}

static int eth_stm32_hal_stop(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Stopping ETH HAL driver");

	hal_ret = HAL_ETH_Stop(heth);
	if (hal_ret != HAL_OK) {
		/* HAL_ETH_Stop{_IT} returns HAL_ERROR only if ETH is already stopped */
		LOG_DBG("HAL_ETH_Stop{_IT} returned error (Ethernet is already stopped)");
	}

	return 0;
}

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	ARG_UNUSED(phy_dev);

	/* The hal also needs to be stopped before changing the MAC config.
	 * The speed can change without receiving a link down callback before.
	 */
	eth_stm32_hal_stop(dev);
	if (state->is_up) {
		set_mac_config(dev, state);
		eth_stm32_hal_start(dev);
		net_eth_carrier_on(dev_data->iface);
	} else {
		net_eth_carrier_off(dev_data->iface);
	}
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	bool is_first_init = false;

	if (dev_data->iface == NULL) {
		dev_data->iface = iface;
		is_first_init = true;
	}

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

#if defined(CONFIG_NET_DSA_DEPRECATED)
	dsa_register_master_tx(iface, &eth_tx);
#endif

	ethernet_init(iface);

	setup_mac_filter(heth);

	net_if_carrier_off(iface);

	net_lldp_set_lldpdu(iface);

	if (device_is_ready(eth_stm32_phy_dev)) {
		phy_link_callback_set(eth_stm32_phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	if (is_first_init) {
		const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
		/* Now that the iface is setup, we are safe to enable IRQs. */
		__ASSERT_NO_MSG(cfg->config_func != NULL);
		cfg->config_func();

		/* Start interruption-poll thread */
		k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack),
				rx_thread, (void *) dev, NULL, NULL,
				IS_ENABLED(CONFIG_ETH_STM32_HAL_RX_THREAD_PREEMPTIVE)
					? K_PRIO_PREEMPT(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO)
					: K_PRIO_COOP(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO),
				0, K_NO_WAIT);

		k_thread_name_set(&dev_data->rx_thread, "stm_eth");
	}
}

static enum ethernet_hw_caps eth_stm32_hal_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
#if defined(CONFIG_NET_LLDP)
		| ETHERNET_LLDP
#endif
#if defined(CONFIG_ETH_STM32_HW_CHECKSUM)
		| ETHERNET_HW_RX_CHKSUM_OFFLOAD
		| ETHERNET_HW_TX_CHKSUM_OFFLOAD
#endif
#if defined(CONFIG_NET_DSA_DEPRECATED)
		| ETHERNET_DSA_CONDUIT_PORT
#endif
#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
		| ETHERNET_HW_FILTERING
#endif
		;
}

static int eth_stm32_hal_set_config(const struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		heth->Instance->MACA0HR = (dev_data->mac_addr[5] << 8) |
			dev_data->mac_addr[4];
		heth->Instance->MACA0LR = (dev_data->mac_addr[3] << 24) |
			(dev_data->mac_addr[2] << 16) |
			(dev_data->mac_addr[1] << 8) |
			dev_data->mac_addr[0];
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode) {
			heth->Instance->MACFFR |= ETH_MACFFR_PM;
		} else {
			heth->Instance->MACFFR &= ~ETH_MACFFR_PM;
		}
		return 0;
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
	case ETHERNET_CONFIG_TYPE_FILTER:
		eth_stm32_mcast_filter(dev, &config->filter);
		return 0;
#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct device *eth_stm32_hal_get_phy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return eth_stm32_phy_dev;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_stm32_hal_get_stats(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	return &dev_data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_stm32_hal_get_capabilities,
	.set_config = eth_stm32_hal_set_config,
	.get_phy = eth_stm32_hal_get_phy,
#if defined(CONFIG_NET_DSA_DEPRECATED)
	.send = dsa_tx,
#else
	.send = eth_tx,
#endif
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_stm32_hal_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

PINCTRL_DT_INST_DEFINE(0);

static const struct eth_stm32_hal_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.pclken = {.bus = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(0), stm_eth, bus),
		   .enr = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(0), stm_eth, bits)},
	.pclken_tx = {.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_tx, bus),
		      .enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_tx, bits)},
	.pclken_rx = {.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_rx, bus),
		      .enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_rx, bits)},
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
	|| DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii),
			"Unsupported PHY connection type");

static struct eth_stm32_hal_dev_data eth0_data = {
	.heth = {
		.Instance = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(0)),
		.Init = {
			.AutoNegotiation = ETH_STM32_AUTO_NEGOTIATION_ENABLE ?
					   ETH_AUTONEGOTIATION_ENABLE : ETH_AUTONEGOTIATION_DISABLE,
			.PhyAddress = DT_REG_ADDR(DT_INST_PHANDLE(0, phy_handle)),
			.RxMode = ETH_RXINTERRUPT_MODE,
			.ChecksumMode = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?
					ETH_CHECKSUM_BY_HARDWARE : ETH_CHECKSUM_BY_SOFTWARE,
			.MediaInterface = STM32_ETH_PHY_MODE(0),
		},
	},
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize,
		    NULL, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU);
