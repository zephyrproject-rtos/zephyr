/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME eth_stm32_hal
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <errno.h>
#include <stdbool.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <soc.h>
#include <misc/printk.h>
#include <clock_control.h>
#include <clock_control/stm32_clock_control.h>

#include "eth_stm32_hal_priv.h"

#if defined(CONFIG_PTP_CLOCK_STM32)
#include <ptp_clock.h>
#include <net/gptp.h>
#endif


static ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __aligned(4);
static ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __aligned(4);
static u8_t dma_rx_buffer[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __aligned(4);
static u8_t dma_tx_buffer[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __aligned(4);


static inline void disable_mcast_filter(ETH_HandleTypeDef *heth)
{
	__ASSERT_NO_MSG(heth != NULL);

	u32_t tmp = heth->Instance->MACFFR;

	/* disable multicast filtering */
	tmp &= ~(ETH_MULTICASTFRAMESFILTER_PERFECTHASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_HASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_PERFECT);

	/* enable receiving all multicast frames */
	tmp |= ETH_MULTICASTFRAMESFILTER_NONE;

	heth->Instance->MACFFR = tmp;

	/* Wait until the write operation will be taken into account:
	 * at least four TX_CLK/RX_CLK clock cycles
	 */
	tmp = heth->Instance->MACFFR;
	k_sleep(1);
	heth->Instance->MACFFR = tmp;
}

#if defined(CONFIG_PTP_CLOCK_STM32)
static bool eth_get_ptp_data(struct net_if *iface, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_VLAN)
	struct net_eth_vlan_hdr *hdr_vlan;
	struct ethernet_context *eth_ctx;
	bool vlan_enabled = false;

	eth_ctx = net_if_l2_data(iface);
	if (net_eth_is_vlan_enabled(eth_ctx, iface)) {
		hdr_vlan = (struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
		vlan_enabled = true;

		if (ntohs(hdr_vlan->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}
	} else
#endif
	{
		if (ntohs(NET_ETH_HDR(pkt)->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	return true;
}
#endif

static int eth_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;
	u8_t *dma_buffer;
	int res;
	struct net_buf *frag;
	u16_t total_len;
	__IO ETH_DMADescTypeDef *dma_tx_desc;
#if defined(CONFIG_PTP_CLOCK_STM32)
        ETH_PTP_TIMESTAM time_stamp;
	bool timestamped_frame;
#endif

	__ASSERT_NO_MSG(iface != NULL);
	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	dev = net_if_get_device(iface);
	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	total_len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);
	if (total_len > ETH_TX_BUF_SIZE) {
		LOG_ERR("PKT to big");
		res = -EIO;
		goto error;
	}

	dma_tx_desc = heth->TxDesc;
	while ((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (u32_t)RESET) {
		k_yield();
	}
        
	dma_buffer = (u8_t *)(dma_tx_desc->Buffer1Addr);

	memcpy(dma_buffer, net_pkt_ll(pkt),
		net_pkt_ll_reserve(pkt) + pkt->frags->len);
	dma_buffer += net_pkt_ll_reserve(pkt) + pkt->frags->len;

	frag = pkt->frags->frags;
	while (frag) {
		memcpy(dma_buffer, frag->data, frag->len);
		dma_buffer += frag->len;
		frag = frag->frags;
	}


#if defined(CONFIG_PTP_CLOCK_STM32)
        if(HAL_ETH_PtpTransmitFrame(heth, total_len, &time_stamp) == HAL_OK){
                timestamped_frame = eth_get_ptp_data(iface, pkt);
                if (timestamped_frame) {
                        pkt->timestamp.nanosecond = time_stamp.nsec;
		        pkt->timestamp.second = time_stamp.sec;
                        net_if_add_tx_timestamp(pkt);
                }
        }
        else{
                LOG_ERR("HAL_ETH_TransmitFrame failed");
		res = -EIO;
		goto error;
        }
#else
        if (HAL_ETH_TransmitFrame(heth, total_len) != HAL_OK) {
		LOG_ERR("HAL_ETH_TransmitFrame failed");
		res = -EIO;
		goto error;
	}
#endif

	/* When Transmit Underflow flag is set, clear it and issue a
	 * Transmit Poll Demand to resume transmission.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_TUS) != (u32_t)RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_TUS;
		/* Resume DMA transmission*/
		heth->Instance->DMATPDR = 0;
		res = -EIO;
		goto error;
	}

	net_pkt_unref(pkt);

	res = 0;
error:
	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

static struct net_pkt *eth_rx(struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;
	__IO ETH_DMADescTypeDef *dma_rx_desc;
	struct net_pkt *pkt;
	u16_t total_len;
	u8_t *dma_buffer;
	int i;
#if defined(CONFIG_PTP_CLOCK_STM32)
        ETH_PTP_TIMESTAM time_stamp;
#endif

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

#if defined(CONFIG_PTP_CLOCK_STM32)
        if (HAL_ETH_PtpGetReceivedFrame_IT(heth, &time_stamp) != HAL_OK) {
		return NULL;
        }
#else
        if (HAL_ETH_GetReceivedFrame_IT(heth) != HAL_OK) {
		return NULL;
	}
#endif

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (u8_t *)heth->RxFrameInfos.buffer;

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}
#if defined(CONFIG_PTP_CLOCK_STM32)
        pkt->timestamp.nanosecond = time_stamp.nsec;
        pkt->timestamp.second = time_stamp.sec;    
#endif
	if (!net_pkt_append_all(pkt, total_len, dma_buffer, K_NO_WAIT)) {
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
	for (i = 0; i < heth->RxFrameInfos.SegCount; i++) {
		dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
		dma_rx_desc = (ETH_DMADescTypeDef *)
			(dma_rx_desc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth->RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it
	 * and resume reception.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_RBUS) != (u32_t)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		heth->Instance->DMARPDR = 0;
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	struct net_pkt *pkt;
	int res;

	__ASSERT_NO_MSG(arg1 != NULL);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	dev = (struct device *)arg1;
	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	while (1) {
		k_sem_take(&dev_data->rx_int_sem, K_FOREVER);

		while ((pkt = eth_rx(dev)) != NULL) {
			net_pkt_print_frags(pkt);
			res = net_recv_data(dev_data->iface, pkt);
			if (res < 0) {
				LOG_ERR("Failed to enqueue frame "
					"into RX queue: %d", res);
				net_pkt_unref(pkt);
			}
		}
	}
}

static void eth_isr(void *arg)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	__ASSERT_NO_MSG(heth != NULL);

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

static int eth_initialize(struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data;
	struct eth_stm32_hal_dev_cfg *cfg;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	dev_data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(dev_data->clock != NULL);

	/* enable clock */
	clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken);
	clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_tx);
	clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_rx);
	clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_ptp);

	__ASSERT_NO_MSG(cfg->config_func != NULL);

	cfg->config_func();

	return 0;
}

#if defined(CONFIG_PTP_CLOCK_STM32)
static void ETH_PTPStart(ETH_HandleTypeDef *heth, uint32_t UpdateMethod) 
{
        
        /* Check the parameters */
        assert_param(IS_ETH_PTP_UPDATE(UpdateMethod));

        /* Mask the Time stamp trigger interrupt by setting bit 9 in the MACIMR register. */
        ETH_MACITConfig(heth, ETH_MAC_IT_TST, DISABLE);
        
        ETH_PTPTimeStampCmd(heth, ENABLE);

        /* Program the Subsecond increment register based on the PTP clock frequency. */
        ETH_SetPTPSubSecondIncrement(heth, ETH_STM32_PTP_INCREMENT);

        if (UpdateMethod == ETH_PTP_FineUpdate) {
                /* If you are using the Fine correction method, program the Time stamp addend register
                * and set Time stamp control register bit 5 (addend register update). */
                ETH_SetPTPTimeStampAddend(heth, ETH_STM32_PTP_ADDEND);
                ETH_EnablePTPTimeStampAddend(heth);
                /* Poll the Time stamp control register until bit 5 is cleared. */
                while(ETH_GetPTPFlagStatus(heth, ETH_PTP_FLAG_TSARU) == SET);
        }

        /* To select the Fine correction method (if required),
        * program Time stamp control register  bit 1. */
        ETH_PTPUpdateMethodConfig(heth, UpdateMethod);
        /* Program the Time stamp high update and Time stamp low update registers
        * with the appropriate time value. */
        ETH_SetPTPTimeStampUpdate(heth, ETH_PTP_PositiveTime, 0, 0);
        /* Set Time stamp control register bit 2 (Time stamp init). */
        ETH_InitializePTPTimeStamp(heth);
}
#endif

#if defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
static void generate_mac(u8_t *mac_addr)
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[3] = entropy >> 8;
	mac_addr[4] = entropy >> 16;
	/* Locally administered, unicast */
	mac_addr[5] = ((entropy >> 0) & 0xfc) | 0x02;
}
#endif

static void eth_iface_init(struct net_if *iface)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;
	u8_t hal_ret;

	__ASSERT_NO_MSG(iface != NULL);

	dev = net_if_get_device(iface);
	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	dev_data->iface = iface;

#if defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
	generate_mac(dev_data->mac_addr);
#endif

	heth->Init.MACAddr = dev_data->mac_addr;

	hal_ret = HAL_ETH_Init(heth);

	if (hal_ret == HAL_TIMEOUT) {
		/* HAL Init time out. This could be linked to */
		/* a recoverable error. Log the issue and continue */
		/* driver initialisation */
		LOG_ERR("HAL_ETH_Init Timed out");
	} else if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return;
	}

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, UINT_MAX);

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_THREAD_STACK_SIZEOF(dev_data->rx_thread_stack),
			rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO),
			0, K_NO_WAIT);


	HAL_ETH_DMARxDescListInit(heth, dma_rx_desc_tab,
		&dma_rx_buffer[0][0], ETH_RXBUFNB);
#if defined(CONFIG_PTP_CLOCK_STM32)
	HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
		&dma_tx_buffer[0][0], ETH_TXBUFNB, true);
        ETH_PTPStart(heth, ETH_PTP_FineUpdate); 
#else
        HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
	        &dma_tx_buffer[0][0], ETH_TXBUFNB, false);
#endif

	HAL_ETH_Start(heth);

	disable_mcast_filter(heth);

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static enum ethernet_hw_caps eth_stm32_hal_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T 
#if defined(CONFIG_PTP_CLOCK_STM32)
	        |ETHERNET_PTP;
#else
                ;
#endif
}


#if defined(CONFIG_PTP_CLOCK_STM32)
static struct device *eth_stm32_get_ptp_clock(struct device *dev)
{
        struct eth_stm32_hal_dev_data *dev_data  = dev->driver_data;

	return dev_data->ptp_clock;
}
#endif

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.iface_api.send = eth_tx,

	.get_capabilities = eth_stm32_hal_get_capabilities,

#if defined(CONFIG_PTP_CLOCK_STM32)
	.get_ptp_clock = eth_stm32_get_ptp_clock,
#endif
};

static struct device DEVICE_NAME_GET(eth0_stm32_hal);

static void eth0_irq_config(void)
{
	IRQ_CONNECT(ETH_IRQn, CONFIG_ETH_STM32_HAL_IRQ_PRI, eth_isr,
		    DEVICE_GET(eth0_stm32_hal), 0);
	irq_enable(ETH_IRQn);
}

static const struct eth_stm32_hal_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.pclken   =   { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMAC },
	.pclken_tx =  { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACTX },
	.pclken_rx =  { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACRX },
	.pclken_ptp = { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACPTP },
};

static struct eth_stm32_hal_dev_data eth0_data = {
	.heth = {
		.Instance = ETH,
		.Init = {
			.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE,
			.PhyAddress = CONFIG_ETH_STM32_HAL_PHY_ADDRESS,
			.RxMode = ETH_RXINTERRUPT_MODE,
			.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE,
			.MediaInterface = ETH_MEDIA_INTERFACE_RMII,
		},
	},
	.mac_addr = {
		/* ST's OUI */
		0x00,
		0x80,
		0xE1,
#if !defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
		CONFIG_ETH_STM32_HAL_MAC3,
		CONFIG_ETH_STM32_HAL_MAC4,
		CONFIG_ETH_STM32_HAL_MAC5
#endif
	},
};

NET_DEVICE_INIT(eth0_stm32_hal, CONFIG_ETH_STM32_HAL_NAME, eth_initialize,
	&eth0_data, &eth0_config, CONFIG_ETH_INIT_PRIORITY, &eth_api,
	ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), ETH_STM32_HAL_MTU);

#if defined(CONFIG_PTP_CLOCK_STM32)
struct ptp_eth_stm32_hal_dev_data {
	struct eth_stm32_hal_dev_data *eth_dev_data;
};

static struct ptp_eth_stm32_hal_dev_data ptp_stm32_0_dev_data;

static int ptp_clock_stm32_set(struct device *dev, struct net_ptp_time *tm)
{
        struct ptp_eth_stm32_hal_dev_data *ptp_dev_data = dev->driver_data;
        struct eth_stm32_hal_dev_data *dev_data = ptp_dev_data->eth_dev_data;
        ETH_HandleTypeDef *heth;
        uint32_t SubSecondValue;

        heth = &dev_data->heth; 
        
        /* convert nanosecond to subseconds */
        SubSecondValue = ETH_PTPNanoSecond2SubSecond(tm->nanosecond);

        ETH_SetPTPTimeStampUpdate(heth, ETH_PTP_PositiveTime, tm->second, SubSecondValue);
        ETH_InitializePTPTimeStamp(heth);
        
        while(ETH_GetPTPFlagStatus(heth, ETH_PTP_FLAG_TSSTI) == SET);

        return 0;
}

static int ptp_clock_stm32_get(struct device *dev, struct net_ptp_time *tm)
{
        struct ptp_eth_stm32_hal_dev_data *ptp_dev_data = dev->driver_data;
        struct eth_stm32_hal_dev_data *dev_data = ptp_dev_data->eth_dev_data;
        ETH_HandleTypeDef *heth;

        heth = &dev_data->heth;

        tm->nanosecond = ETH_PTPSubSecond2NanoSecond(ETH_GetPTPRegister(heth, ETH_PTPTSLR));
        tm->second = ETH_GetPTPRegister(heth, ETH_PTPTSHR);

	return 0;
}

static int ptp_clock_stm32_adjust(struct device *dev, int increment)
{
        uint32_t Sign;
        uint32_t SecondValue;
        uint32_t NanoSecondValue;
        uint32_t SubSecondValue;
        uint32_t addend;
        struct ptp_eth_stm32_hal_dev_data *ptp_dev_data = dev->driver_data;
        struct eth_stm32_hal_dev_data *dev_data = ptp_dev_data->eth_dev_data;
        ETH_HandleTypeDef *heth;

        heth = &dev_data->heth;

        if ((increment <= -NSEC_PER_SEC) || (increment >= NSEC_PER_SEC))
		return -EINVAL;

        if(increment < 0) {
                Sign = ETH_PTP_NegativeTime;
                SecondValue = 0;
                NanoSecondValue = -increment;
        } else {
                Sign = ETH_PTP_PositiveTime;
                SecondValue = 0;
                NanoSecondValue = increment;
        }

        SubSecondValue = ETH_PTPNanoSecond2SubSecond(NanoSecondValue);
 
        addend = ETH_GetPTPRegister(heth, ETH_PTPTSAR);

        while(ETH_GetPTPFlagStatus(heth, ETH_PTP_FLAG_TSSTU) == SET);
        while(ETH_GetPTPFlagStatus(heth, ETH_PTP_FLAG_TSSTI) == SET);

        ETH_SetPTPTimeStampUpdate(heth, Sign, SecondValue, SubSecondValue);
        ETH_EnablePTPTimeStampUpdate(heth);
        while(ETH_GetPTPFlagStatus(heth, ETH_PTP_FLAG_TSSTU) == SET);      
        
        ETH_SetPTPTimeStampAddend(heth, addend);
        ETH_EnablePTPTimeStampAddend(heth);
       
	return 0;
}

static int ptp_clock_stm32_rate_adjust(struct device *dev, float ratio)
{
        /*uint32_t addend_old addend_new;
        struct ptp_eth_stm32_hal_dev_data *ptp_dev_data = dev->driver_data;
        struct eth_stm32_hal_dev_data *dev_data = ptp_dev_data->eth_dev_data;
        ETH_HandleTypeDef *heth;

        heth = &dev_data->heth;

	if (ratio == 1) {
		return 0;
	}

	if (ratio < 0) {
		return -EINVAL;
	}

	if (ratio < 0.5) {
		ratio = 0.5;
	} else if (ratio > 1.5) {
		ratio = 1.5;
	}

        addend = ETH_GetPTPRegister(heth, ETH_PTPTSAR);
        addend *= ratio;

        ETH_SetPTPTimeStampAddend(heth, increment);
        ETH_EnablePTPTimeStampAddend(heth);

	return 0;*/
 
        return -ENOTSUP; 
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_stm32_set,
	.get = ptp_clock_stm32_get,
	.adjust = ptp_clock_stm32_adjust,
	.rate_adjust = ptp_clock_stm32_rate_adjust,
};

static int ptp_stm32_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth0_stm32_hal);
	struct eth_stm32_hal_dev_data *dev_data = eth_dev->driver_data;
	struct ptp_eth_stm32_hal_dev_data *ptp_dev_data = port->driver_data;

	dev_data->ptp_clock = port;
	ptp_dev_data->eth_dev_data = dev_data;

	return 0;
}

DEVICE_AND_API_INIT(stm32_ptp_clock_0, PTP_CLOCK_NAME, ptp_stm32_init,
		    &ptp_stm32_0_dev_data, NULL, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &api);

#endif
