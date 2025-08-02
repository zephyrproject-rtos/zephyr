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

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
#include <zephyr/drivers/ptp_clock.h>
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

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

#if defined(CONFIG_ETH_STM32_HAL_API_V2)
#define ETH_MII_MODE	HAL_ETH_MII_MODE
#define ETH_RMII_MODE	HAL_ETH_RMII_MODE
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
#define ETH_GMII_MODE	HAL_ETH_GMII_MODE
#define ETH_RGMII_MODE	HAL_ETH_RGMII_MODE
#endif

#else
#define ETH_MII_MODE	ETH_MEDIA_INTERFACE_MII
#define ETH_RMII_MODE	ETH_MEDIA_INTERFACE_RMII
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32n6_ethernet)
#define STM32_ETH_PHY_MODE(inst) \
	((DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, rgmii) ? ETH_RGMII_MODE : \
	 (DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, gmii) ? ETH_GMII_MODE : \
	 (DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ? ETH_MII_MODE : \
		 ETH_RMII_MODE))))
#else
#define STM32_ETH_PHY_MODE(inst) \
	(DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ? \
		ETH_MII_MODE : ETH_RMII_MODE)
#endif

static inline void setup_mac_filter(ETH_HandleTypeDef *heth)
{
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	setup_mac_filter_v2(heth);
#else
	setup_mac_filter_v1(heth);
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
}

static int inline eth_tx(const struct device *dev, struct net_pkt *pkt)
{
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	return eth_tx_v2(dev, pkt);
#else
	return eth_tx_v1(dev, pkt);
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
}

struct net_if *get_iface(struct eth_stm32_hal_dev_data *ctx)
{
	return ctx->iface;
}

static inline struct net_pkt *eth_rx(const struct device *dev)
{
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	return eth_rx_v2(dev);
#else
	return eth_rx_v1(dev);
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
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

static inline void set_mac_config(const struct device *dev, struct phy_link_state *state)
{
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	set_mac_config_v2(dev, state);
#else
	set_mac_config_v1(dev, state);
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
}

static int eth_stm32_hal_start(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	HAL_StatusTypeDef hal_ret = HAL_OK;

	LOG_DBG("Starting ETH HAL driver");

#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	hal_ret = HAL_ETH_Start_IT(heth);
#else
	hal_ret = HAL_ETH_Start(heth);
#endif
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

#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	hal_ret = HAL_ETH_Stop_IT(heth);
#else
	hal_ret = HAL_ETH_Stop(heth);
#endif
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

static inline int eth_init_api(const struct device *dev)
{
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
	return eth_init_api_v2(dev);
#else
	return eth_init_api_v1(dev);
#endif
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

	/* This function requires the Ethernet interface to be
	 * properly initialized. In auto-negotiation mode, it reads the speed
	 * and duplex settings to configure the driver accordingly.
	 */
	eth_init_api(dev);

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
#if defined(CONFIG_ETH_STM32_HAL_API_V2)
		ETH_MACFilterConfigTypeDef MACFilterConf;

		HAL_ETH_GetMACFilterConfig(heth, &MACFilterConf);

		MACFilterConf.PromiscuousMode = config->promisc_mode ? ENABLE : DISABLE;

		HAL_ETH_SetMACFilterConfig(heth, &MACFilterConf);
#else
		if (config->promisc_mode) {
			heth->Instance->MACFFR |= ETH_MACFFR_PM;
		} else {
			heth->Instance->MACFFR &= ~ETH_MACFFR_PM;
		}
#endif /* CONFIG_ETH_STM32_HAL_API_V2 */
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

const struct device *eth_stm32_hal_get_phy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return eth_stm32_phy_dev;
}

#if defined(CONFIG_PTP_CLOCK_STM32_HAL)
static const struct device *eth_stm32_get_ptp_clock(const struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	return dev_data->ptp_clock;
}
#endif /* CONFIG_PTP_CLOCK_STM32_HAL */

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
#if DT_INST_CLOCKS_HAS_NAME(0, mac_clk_ptp)
	.pclken_ptp = {.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_ptp, bus),
		       .enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_ptp, bits)},
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
		},
	},
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize,
		    NULL, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU);
