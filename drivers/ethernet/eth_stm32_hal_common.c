/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/lldp.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <ethernet/eth_stats.h>
#include <stdint.h>

#include "eth.h"
#include "eth_stm32_hal_priv.h"

#define DT_DRV_COMPAT st_stm32_ethernet

LOG_MODULE_REGISTER(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

#include "dwc_mac/eth_stm32_dwc.h"

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) &&                                       \
	!DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#error DTCM for DMA buffer is activated but zephyr,dtcm is not present in dts
#endif

#define ETH_STM32_HAL_MTU            NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

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
			while ((pkt = eth_stm32_rx(dev)) != NULL) {
				iface = net_pkt_iface(pkt);
				res = net_recv_data(iface, pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(net_pkt_iface(pkt));
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

static int eth_initialize(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int ret = 0;

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

	/* configure pinmux */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	ret = eth_stm32_net_eth_mac_load(&cfg->mac_cfg, dev_data->mac_addr);
	if (ret < 0) {
		return ret;
	}

	heth->Init.MACAddr = dev_data->mac_addr;

	ret = eth_stm32_hal_init(dev);
	if (ret) {
		LOG_ERR("Failed to initialize HAL");
		return -EIO;
	}

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x", dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3], dev_data->mac_addr[4],
		dev_data->mac_addr[5]);

	return 0;
}

#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
void eth_stm32_mcast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct eth_stm32_hal_dev_data *dev_data = (struct eth_stm32_hal_dev_data *)dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	uint32_t crc;
	uint32_t hash_table[2];
	uint32_t hash_index;

	crc = __RBIT(crc32_ieee(filter->mac_address.addr, sizeof(struct net_eth_addr)));
	hash_index = (crc >> 26) & 0x3f;

	__ASSERT_NO_MSG(hash_index < ARRAY_SIZE(dev_data->hash_index_cnt));

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	hash_table[0] = heth->Instance->MACHT0R;
	hash_table[1] = heth->Instance->MACHT1R;
#else
	hash_table[0] = heth->Instance->MACHTLR;
	hash_table[1] = heth->Instance->MACHTHR;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACHT0R = hash_table[0];
	heth->Instance->MACHT1R = hash_table[1];
#else
	heth->Instance->MACHTLR = hash_table[0];
	heth->Instance->MACHTHR = hash_table[1];
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

#endif /* CONFIG_ETH_STM32_MULTICAST_FILTER */

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	ARG_UNUSED(phy_dev);

	/* The hal also needs to be stopped before changing the MAC config.
	 * The speed can change without receiving a link down callback before.
	 */
	if (state->is_up) {
		eth_stm32_hal_stop(dev, dev_data->iface);
		eth_stm32_set_mac_config(dev, state);
		eth_stm32_hal_start(dev, dev_data->iface);
	}

	net_eth_carrier_set(dev_data->iface, state->is_up);
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;
	ETH_HandleTypeDef *heth = &dev_data->heth;

	dev_data->iface = iface;

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr, sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	eth_stm32_setup_mac_filter(heth);

	net_if_carrier_off(iface);

	net_lldp_set_lldpdu(iface);

	if (device_is_ready(cfg->phy_dev)) {
		phy_link_callback_set(cfg->phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	/* Now that the iface is setup, we are safe to enable IRQs. */
	__ASSERT_NO_MSG(cfg->config_func != NULL);
	cfg->config_func();

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack), rx_thread, (void *)dev,
			NULL, NULL,
			IS_ENABLED(CONFIG_ETH_STM32_HAL_RX_THREAD_PREEMPTIVE)
				? K_PRIO_PREEMPT(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO)
				: K_PRIO_COOP(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	k_thread_name_set(&dev_data->rx_thread, "stm_eth");
}

static enum ethernet_hw_caps eth_stm32_hal_get_capabilities(const struct device *dev __unused,
							    struct net_if *iface __unused)
{
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
	       | ETHERNET_HW_RX_CHKSUM_OFFLOAD | ETHERNET_HW_TX_CHKSUM_OFFLOAD
#endif
#if defined(CONFIG_ETH_STM32_MULTICAST_FILTER)
	       | ETHERNET_HW_FILTERING
#endif
		;
}

static const struct device *eth_stm32_hal_get_phy(const struct device *dev,
						  struct net_if *iface __unused)
{
	const struct eth_stm32_hal_dev_cfg *cfg = dev->config;

	return cfg->phy_dev;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_stm32_hal_get_stats(const struct device *dev,
						     struct net_if *iface __unused)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	return &dev_data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	.get_ptp_clock = eth_stm32_get_ptp_clock,
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */
	.get_capabilities = eth_stm32_hal_get_capabilities,
	.set_config = eth_stm32_hal_set_config,
	.get_phy = eth_stm32_hal_get_phy,
	.send = eth_stm32_tx,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_stm32_hal_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

#define ETH_STM32_HAS_PTP_CLOCK(n) DT_CLOCKS_HAS_NAME(DT_DRV_INST(n), mac_clk_ptp)

#define ETH_STM32_HAL_COMMON_DMA_BUF_DEFN(n)                                                       \
	static struct eth_stm32_dma_buf eth##n##_dma_buf __eth_stm32_buf

#define ETH_STM32_HAL_COMMON_DMA_DESC_DEFN(n)                                                      \
	static struct eth_stm32_dma_desc eth##n##_dma_desc __eth_stm32_desc

#define ETH_STM32_HAL_COMMON_IRQ_CONFIG_DEFN(n)                                                    \
	static void eth##n##_irq_config(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), eth_isr,                    \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define ETH_STM32_HAL_COMMON_PINCTRL_DEFN(n) PINCTRL_DT_INST_DEFINE(n)

#define ETH_STM32_HAL_COMMON_PCLK_DEFN(n)                                                          \
	static const struct stm32_pclken eth##n##_pclken[] = STM32_DT_CLOCKS(DT_DRV_INST(n))

#define ETH_STM32_HAL_COMMON_CFG_DEFN(n)                                                           \
	static const struct eth_stm32_hal_dev_cfg eth##n##_config = {                              \
		.config_func = eth##n##_irq_config,                                                \
		.pclken = eth##n##_pclken,                                                         \
		.pclken_cnt = DT_NUM_CLOCKS(DT_DRV_INST(n)),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(n),                                     \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),                          \
		.dma_buf = &eth##n##_dma_buf,                                                      \
		.dma_desc = &eth##n##_dma_desc,                                                    \
		IF_ENABLED(CONFIG_PTP_CLOCK_STM32_HAL,                                             \
			(.rate_pclken_idx = DT_PHA_ELEM_IDX_BY_NAME(                               \
				DT_DRV_INST(n), clocks,                                            \
				COND_CODE_1(ETH_STM32_HAS_PTP_CLOCK(n),                            \
					    (mac_clk_ptp), (stm_eth))),)) }

#define ETH_STM32_HAL_COMMON_BUILD_ASSERT(n)                                                       \
	BUILD_ASSERT(                                                                              \
		DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, mii) ||                             \
		DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rmii)                               \
			IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet),                 \
				(|| DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, rgmii)          \
				 || DT_INST_ENUM_HAS_VALUE(n, phy_connection_type, gmii))),        \
		"Unsupported PHY connection type")

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
#define ETH_STM32_HAL_COMMON_CLOCK_SELECTION(inst)                                                 \
	.ClockSelection = (DT_INST_PROP(inst, st_ext_phyclk) ? HAL_ETH_REF_CLK_RCC                 \
							     : HAL_ETH_REF_CLK_RX_CLK_PIN),
#else
#define ETH_STM32_HAL_COMMON_CLOCK_SELECTION(inst)
#endif

#define ETH_STM32_HAL_COMMON_DATA_DEFN(n)                                                          \
	static struct eth_stm32_hal_dev_data eth##n##_data = {                                     \
		.heth = {                                                                          \
			.Instance = (ETH_TypeDef *)DT_INST_REG_ADDR(n),                            \
			.Init = {                                                                  \
				IF_ENABLED(CONFIG_ETH_STM32_HAL_API_V1,                            \
					(.RxMode = ETH_RXINTERRUPT_MODE,                           \
					 .ChecksumMode =                                           \
						IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?         \
						ETH_CHECKSUM_BY_HARDWARE :                         \
						ETH_CHECKSUM_BY_SOFTWARE,))                        \
				.MediaInterface = STM32_ETH_PHY_MODE(n),                           \
				ETH_STM32_HAL_COMMON_CLOCK_SELECTION(n)                            \
			},                                                                         \
		},                                                                                 \
	}

#define ETH_STM32_HAL_COMMON_DT_INST_DEFN(n)                                                       \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_initialize, NULL, &eth##n##_data, &eth##n##_config,   \
				      CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU)

#define ETH_STM32_HAL_COMMON_DEVICE(n)                                                             \
	ETH_STM32_HAL_COMMON_DMA_BUF_DEFN(n);                                                      \
	ETH_STM32_HAL_COMMON_DMA_DESC_DEFN(n);                                                     \
	ETH_STM32_HAL_COMMON_IRQ_CONFIG_DEFN(n);                                                   \
	ETH_STM32_HAL_COMMON_PINCTRL_DEFN(n);                                                      \
	ETH_STM32_HAL_COMMON_PCLK_DEFN(n);                                                         \
	ETH_STM32_HAL_COMMON_CFG_DEFN(n);                                                          \
	ETH_STM32_HAL_COMMON_BUILD_ASSERT(n);                                                      \
	ETH_STM32_HAL_COMMON_DATA_DEFN(n);                                                         \
	ETH_STM32_HAL_COMMON_DT_INST_DEFN(n);

DT_INST_FOREACH_STATUS_OKAY(ETH_STM32_HAL_COMMON_DEVICE)
