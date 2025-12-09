/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dsa.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/lldp.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_stm32_hal_priv.h"

LOG_MODULE_REGISTER(eth_stm32_hal, CONFIG_ETHERNET_LOG_LEVEL);

#if DT_INST_PROP(0, zephyr_random_mac_address)
#define ETH_STM32_RANDOM_MAC
#endif

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    !DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm))
#error DTCM for DMA buffer is activated but zephyr,dtcm is not present in dts
#endif

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

#define ETH_STM32_HAL_MTU NET_ETH_MTU
#define ETH_STM32_HAL_FRAME_SIZE_MAX (ETH_STM32_HAL_MTU + 18)

uint8_t dma_rx_buffer[ETH_RXBUFNB][ETH_STM32_RX_BUF_SIZE] __eth_stm32_buf;
uint8_t dma_tx_buffer[ETH_TXBUFNB][ETH_STM32_TX_BUF_SIZE] __eth_stm32_buf;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
ETH_DMADescTypeDef dma_rx_desc_tab[ETH_DMA_RX_CH_CNT][ETH_RXBUFNB] __eth_stm32_desc __aligned(32);
ETH_DMADescTypeDef dma_tx_desc_tab[ETH_DMA_TX_CH_CNT][ETH_TXBUFNB] __eth_stm32_desc __aligned(32);
#else
ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __eth_stm32_desc;
ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __eth_stm32_desc;
#endif

const struct device *eth_stm32_phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle));

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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
/**
 * Configures the RISAF (RIF Security Attribute Framework) for Ethernet on STM32N6.
 * This function sets up the master and slave security attributes for the Ethernet peripheral.
 */

static void RISAF_Config(void)
{
	/* Define and initialize the master configuration structure */
	RIMC_MasterConfig_t RIMC_master = {0};

	/* Enable the clock for the RIFSC (RIF Security Controller) */
	__HAL_RCC_RIFSC_CLK_ENABLE();

	RIMC_master.MasterCID = RIF_CID_1;
	RIMC_master.SecPriv = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV;

	/* Configure the master attributes for the Ethernet peripheral (ETH1) */
	HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_ETH1, &RIMC_master);

	/* Set the secure and privileged attributes for the Ethernet peripheral (ETH1) as a slave */
	HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_ETH1,
					      RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_PRIV);
}
#endif

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

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
	/* RISAF Configuration */
	RISAF_Config();
#endif

	/* enable clock */
	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken);
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken_tx);
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken_rx);
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk_ptp)
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
		(clock_control_subsys_t)&cfg->pclken_ptp);
#endif
#if DT_INST_CLOCKS_HAS_NAME(0, eth_ker)
	ret |= clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				       (clock_control_subsys_t)&cfg->pclken_ker,
				       NULL);
#endif
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk)
	ret |= clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t)&cfg->pclken_mac);
#endif

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

	ret = eth_stm32_hal_init(dev);
	if (ret) {
		LOG_ERR("Failed to initialize HAL");
		return -EIO;
	}

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

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
	eth_stm32_hal_stop(dev);
	if (state->is_up) {
		eth_stm32_set_mac_config(dev, state);
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
	dsa_register_master_tx(iface, &eth_stm32_tx);
#endif

	ethernet_init(iface);

	eth_stm32_setup_mac_filter(heth);

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
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
		| ETHERNET_PTP
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
#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
	.get_ptp_clock = eth_stm32_get_ptp_clock,
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */
	.get_capabilities = eth_stm32_hal_get_capabilities,
	.set_config = eth_stm32_hal_set_config,
	.get_phy = eth_stm32_hal_get_phy,
#if defined(CONFIG_NET_DSA_DEPRECATED)
	.send = dsa_tx,
#else
	.send = eth_stm32_tx,
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
	.pclken = STM32_CLOCK_INFO_BY_NAME(DT_INST_PARENT(0), stm_eth),
	.pclken_tx = STM32_DT_INST_CLOCK_INFO_BY_NAME(0, mac_clk_tx),
	.pclken_rx = STM32_DT_INST_CLOCK_INFO_BY_NAME(0, mac_clk_rx),
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk_ptp)
	.pclken_ptp = STM32_DT_INST_CLOCK_INFO_BY_NAME(0, mac_clk_ptp),
#endif
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk)
	.pclken_mac = {.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk, bus),
		       .enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk, bits)},
#endif
#if DT_INST_CLOCKS_HAS_NAME(0, eth_ker)
	.pclken_ker = {.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, eth_ker, bus),
		       .enr = DT_INST_CLOCKS_CELL_BY_NAME(0, eth_ker, bits)},
#endif
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, mii)
	|| DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rmii)
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet),
	(|| DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, rgmii)
	 || DT_INST_ENUM_HAS_VALUE(0, phy_connection_type, gmii))),
			"Unsupported PHY connection type");

static struct eth_stm32_hal_dev_data eth0_data = {
	.heth = {
		.Instance = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(0)),
		.Init = {
#if defined(CONFIG_ETH_STM32_HAL_API_V1)
			.AutoNegotiation = ETH_STM32_AUTO_NEGOTIATION_ENABLE ?
					   ETH_AUTONEGOTIATION_ENABLE : ETH_AUTONEGOTIATION_DISABLE,
			.PhyAddress = DT_REG_ADDR(DT_INST_PHANDLE(0, phy_handle)),
			.RxMode = ETH_RXINTERRUPT_MODE,
			.ChecksumMode = IS_ENABLED(CONFIG_ETH_STM32_HW_CHECKSUM) ?
					ETH_CHECKSUM_BY_HARDWARE : ETH_CHECKSUM_BY_SOFTWARE,
#endif /* CONFIG_ETH_STM32_HAL_API_V1 */
			.MediaInterface = STM32_ETH_PHY_MODE(0),
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp13_ethernet)
			.ClockSelection = DT_INST_PROP(0, st_ext_phyclk) ? HAL_ETH1_REF_CLK_RCC
							     : HAL_ETH1_REF_CLK_RX_CLK_PIN,
#endif
		},
	},
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize,
		    NULL, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU);
