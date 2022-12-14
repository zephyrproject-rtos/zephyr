/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_ethernet


#include <soc.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/net/ethernet.h>
#include "ethernet/eth_stats.h"
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>

#include "eth_numaker_priv.h"

#include "NuMicro.h"
#include "synopGMAC_network_interface.h"
#ifdef CONFIG_SOC_M467
#include "m460_eth.h"
#endif

#include <zephyr/logging/log.h>
//LOG_MODULE_REGISTER(eth_numaker, LOG_LEVEL_DBG);
LOG_MODULE_REGISTER(eth_numaker, LOG_LEVEL_INF);

#define NU_GMAC_INTF            0       // Device EMAC Interface port


//#define NU_DEBUGF(x) { printf x; }
#define NU_DEBUGF(x)

extern synopGMACdevice GMACdev[GMAC_CNT];
extern struct sk_buff tx_buf[GMAC_CNT][TRANSMIT_DESC_SIZE];
extern struct sk_buff rx_buf[GMAC_CNT][RECEIVE_DESC_SIZE];

static uint32_t  eth_phy_addr;

/* Device config */
struct eth_numaker_config {
    /* GMAC Base address */
    uint32_t gmacBase;
    uint32_t id_rst;
    uint32_t phy_addr;
    /* clock configuration */
	uint32_t clk_modidx;
    uint32_t clk_src;
    uint32_t clk_div;
    const struct device *clk_dev;
    const struct pinctrl_dev_config *pincfg;
};

/* Driver context/data */
struct eth_numaker_data {
    synopGMACdevice *gmacdev;
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_mutex tx_frame_buf_mutex;
    struct k_spinlock rx_frame_buf_lock;
};


/* Delay execution for given amount of ticks for SDK-HAL */
void plat_delay(uint32_t delay)
{
    volatile uint32_t loop = delay*200;
    while (loop--);
}

static void mdio_write(synopGMACdevice *gmacdev, uint32_t addr, uint32_t reg, int data)
{
    synopGMAC_write_phy_reg((u32 *) gmacdev->MacBase, addr, reg, data);
}

static int mdio_read(synopGMACdevice *gmacdev, uint32_t addr, uint32_t reg)
{
    uint16_t data;
    synopGMAC_read_phy_reg((u32 *) gmacdev->MacBase, addr, reg, &data);
    return data;
}

static int numaker_eth_link_ok(synopGMACdevice *gmacdev)
{
    /* first, a dummy read to latch */
    mdio_read(gmacdev, eth_phy_addr, MII_BMSR);
    if (mdio_read(gmacdev, eth_phy_addr, MII_BMSR) & BMSR_LSTATUS) {
        return 1;
    }
    return 0;
}

static int reset_phy(synopGMACdevice *gmacdev)
{

    uint16_t reg;
    uint32_t delayCnt;
    int retVal = 0;
    
    mdio_write(gmacdev, eth_phy_addr, MII_BMCR, BMCR_RESET);

    delayCnt = 2000;
    while (delayCnt > 0) {
        delayCnt--;
        if ((mdio_read(gmacdev, eth_phy_addr, MII_BMCR) & BMCR_RESET) == 0) {
            break;
        }
    }

    if (delayCnt == 0) {
        NU_DEBUGF(("Reset phy failed\n"));
        return (-1);
    }

    delayCnt = 200000;
    while (delayCnt > 0) {
        delayCnt--;
        if (numaker_eth_link_ok(gmacdev)) {
            gmacdev->LinkState = LINKUP;
            NU_DEBUGF(("Link Up\n"));
            break;
        }
    }    
    if (delayCnt == 0) {
        gmacdev->LinkState = LINKDOWN;
        NU_DEBUGF(("Link Down\n"));
        return (-1);
    }
    
    mdio_write(gmacdev, eth_phy_addr, MII_ADVERTISE, ADVERTISE_CSMA |
               ADVERTISE_10HALF |
               ADVERTISE_10FULL |
               ADVERTISE_100HALF |
               ADVERTISE_100FULL);

    reg = mdio_read(gmacdev, eth_phy_addr, MII_BMCR);
    mdio_write(gmacdev, eth_phy_addr, MII_BMCR, reg | BMCR_ANRESTART);

    delayCnt = 200000;
    while (delayCnt > 0) {
        delayCnt--;
        if ((mdio_read(gmacdev, eth_phy_addr, MII_BMSR) & (BMSR_ANEGCOMPLETE | BMSR_LSTATUS))
                == (BMSR_ANEGCOMPLETE | BMSR_LSTATUS)) {
            break;
        }
    }
    
    if( delayCnt == 0 ) {
        NU_DEBUGF(("AN failed. Set to 100 FULL\n"));
        synopGMAC_set_full_duplex(gmacdev);
        synopGMAC_set_mode(NU_GMAC_INTF, 1);    // Set mode 1: 100Mbps; 2: 10Mbps
        return (-1);
    } else {
        reg = mdio_read(gmacdev, eth_phy_addr, MII_LPA);
        if (reg & ADVERTISE_100FULL) {
            NU_DEBUGF(("100 full\n"));
            gmacdev->DuplexMode = FULLDUPLEX;
            gmacdev->Speed = SPEED100;
            synopGMAC_set_full_duplex(gmacdev);
            synopGMAC_set_mode(NU_GMAC_INTF, 1);    // Set mode 1: 100Mbps; 2: 10Mbps
        } else if (reg & ADVERTISE_100HALF) {
            NU_DEBUGF(("100 half\n"));
            gmacdev->DuplexMode = HALFDUPLEX;
            gmacdev->Speed = SPEED100;
            synopGMAC_set_half_duplex(gmacdev);
            synopGMAC_set_mode(NU_GMAC_INTF, 1);    // Set mode 1: 100Mbps; 2: 10Mbps
        } else if (reg & ADVERTISE_10FULL) {
            NU_DEBUGF(("10 full\n"));
            gmacdev->DuplexMode = FULLDUPLEX;
            gmacdev->Speed = SPEED10;
            synopGMAC_set_full_duplex(gmacdev);
            synopGMAC_set_mode(NU_GMAC_INTF, 2);    // Set mode 1: 100Mbps; 2: 10Mbps
        } else {
            NU_DEBUGF(("10 half\n"));
            gmacdev->DuplexMode = HALFDUPLEX;
            gmacdev->Speed = SPEED10;
            synopGMAC_set_half_duplex(gmacdev);
            synopGMAC_set_mode(NU_GMAC_INTF, 2);    // Set mode 1: 100Mbps; 2: 10Mbps
        }
    }    
    
    printf("PHY ID 1:0x%x\r\n", mdio_read(gmacdev, eth_phy_addr, MII_PHYSID1));
    printf("PHY ID 2:0x%x\r\n", mdio_read(gmacdev, eth_phy_addr, MII_PHYSID2));

    return (retVal);
    
}


static void __numaker_read_mac_addr(char *mac)
{
    uint32_t uID1;
    // Fetch word 0
    uint32_t word0 = *(uint32_t *)0xFF804; // 2KB Data Flash at 0xFF800
    // Fetch word 1
    // we only want bottom 16 bits of word1 (MAC bits 32-47)
    // and bit 9 forced to 1, bit 8 forced to 0
    // Locally administered MAC, reduced conflicts
    // http://en.wikipedia.org/wiki/MAC_address
    uint32_t word1 = *(uint32_t *)0xFF800; // 2KB Data Flash at 0xFF800

    if (word0 == 0xFFFFFFFF) {       // Not burn any mac address at 1st 2 words of Data Flash
        // with a semi-unique MAC address from the UUID
        /* Enable FMC ISP function */
        SYS_UnlockReg();
        FMC_Open();
        // = FMC_ReadUID(0);
        uID1 = FMC_ReadUID(1);
        word1 = (uID1 & 0x003FFFFF) | ((uID1 & 0x030000) << 6) >> 8;
        word0 = ((FMC_ReadUID(0) >> 4) << 20) | ((uID1 & 0xFF) << 12) | (FMC_ReadUID(2) & 0xFFF);
        /* Disable FMC ISP function */
        FMC_Close();
        /* Lock protected registers */
        SYS_LockReg();
    }

    word1 |= 0x00000200;
    word1 &= 0x0000FEFF;

    mac[0] = (word1 & 0x0000ff00) >> 8;
    mac[1] = (word1 & 0x000000ff);
    mac[2] = (word0 & 0xff000000) >> 24;
    mac[3] = (word0 & 0x00ff0000) >> 16;
    mac[4] = (word0 & 0x0000ff00) >> 8;
    mac[5] = (word0 & 0x000000ff);

    printf("mac address %02x-%02x-%02x-%02x-%02x-%02x \r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void __numaker_gmacdev_enable(synopGMACdevice *gmacdev)
{
    
    synopGMAC_clear_interrupt(gmacdev);

    /* Enable INT & TX/RX */
    synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
    synopGMAC_enable_dma_rx(gmacdev);
    synopGMAC_enable_dma_tx(gmacdev);
    
    synopGMAC_tx_enable(gmacdev);
    synopGMAC_rx_enable(gmacdev);

}

static int __numaker_gmacdev_init(synopGMACdevice *gmacdev, uint8_t *mac_addr, uint32_t gmacBase)
{
    int status = 0;
    int i;
    uint32_t offload_needed = 0;    
    struct sk_buff *skb;

    LOG_DBG("=== %s... ---Start---\r\n", __FUNCTION__);
//    NVIC_DisableIRQ(EMAC0_TXRX_IRQn);

    /*Attach the device to MAC struct This will configure all the required base addresses
      such as Mac base, configuration base, phy base address(out of 32 possible phys )*/
    synopGMAC_attach(gmacdev, gmacBase + MACBASE, gmacBase + DMABASE, DEFAULT_PHY_BASE);
    synopGMAC_disable_interrupt_all(gmacdev);
    
    // Reset MAC
    synopGMAC_reset(gmacdev);
    gmacdev->Intf = NU_GMAC_INTF;
    synopGMAC_read_version(gmacdev);
    
    /*Check for Phy initialization*/
    synopGMAC_set_mdc_clk_div(gmacdev,GmiiCsrClk5);
    gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

    /*Reset PHY*/
    //status = synopGMAC_check_phy_init(gmacdev);
    status = reset_phy(gmacdev);

    /*Set up the tx and rx descriptor queue/ring*/
    synopGMAC_setup_tx_desc_queue(gmacdev,TRANSMIT_DESC_SIZE, RINGMODE);
    synopGMAC_init_tx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

    synopGMAC_setup_rx_desc_queue(gmacdev,RECEIVE_DESC_SIZE, RINGMODE);
    synopGMAC_init_rx_desc_base(gmacdev);	//Program the transmit descriptor base address in to DmaTxBase addr

    /*Initialize the dma interface*/
    synopGMAC_dma_bus_mode_init(gmacdev, DmaBurstLength32 | DmaDescriptorSkip0/*DmaDescriptorSkip2*/ | DmaDescriptor8Words ); synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward |DmaTxSecondFrame|DmaRxThreshCtrl128);

    /*Initialize the mac interface*/
    synopGMAC_mac_init(gmacdev);
    synopGMAC_promisc_enable(gmacdev);

    synopGMAC_pause_control(gmacdev); // This enables the pause control in Full duplex mode of operation

#if defined(NU_USING_HW_CHECKSUM)
    /*IPC Checksum offloading is enabled for this driver. Should only be used if Full Ip checksumm offload engine is configured in the hardware*/
    offload_needed = 1;
    synopGMAC_enable_rx_chksum_offload(gmacdev);  	//Enable the offload engine in the receive path
    synopGMAC_rx_tcpip_chksum_drop_enable(gmacdev); // This is default configuration, DMA drops the packets if error in encapsulated ethernet payload
#endif

    for(i = 0; i < RECEIVE_DESC_SIZE; i ++) {
        skb = &rx_buf[NU_GMAC_INTF][i];
        synopGMAC_set_rx_qptr(gmacdev, (u32)((u64)(skb->data) & 0xFFFFFFFF), sizeof(skb->data), (u32)((u64)skb & 0xFFFFFFFF));
    }

    for(i = 0; i < TRANSMIT_DESC_SIZE; i ++) {
        skb = &tx_buf[NU_GMAC_INTF][i];
        synopGMAC_set_tx_qptr(gmacdev, (u32)((u64)(skb->data) & 0xFFFFFFFF), sizeof(skb->data), (u32)((u64)skb & 0xFFFFFFFF), offload_needed ,0);
    }    

    synopGMAC_set_mac_address(NU_GMAC_INTF, mac_addr);
    
    synopGMAC_clear_interrupt(gmacdev);
//    NVIC_EnableIRQ(EMAC0_TXRX_IRQn);

	return 0;
}

static void dump_desc(DmaDesc *rxdesc)
{
}

static int __numaker_gmacdev_get_rx_buf(synopGMACdevice *gmacdev, uint16_t *len, uint8_t **buf)
{

    LOG_DBG("=== %s... ---Start---\r\n", __FUNCTION__);
    DmaDesc * rxdesc = gmacdev->RxBusyDesc;
    dump_desc(rxdesc);
    if(synopGMAC_is_desc_owned_by_dma(rxdesc))
        return -1;
    if(synopGMAC_is_desc_empty(rxdesc))
        return -1;
    
//    synopGMAC_disable_dma_rx(gmacdev);    // it will encounter DMA interrupt status as "Receiver stopped seeing Rx interrupts"
    *len = synop_handle_received_data(NU_GMAC_INTF, buf);
    dump_desc(gmacdev->RxBusyDesc);
    if( *len <= 0 ) return -1; /* No available RX frame */

    // length of payload should be <= 1514
    if (*len > (NU_ETH_MAX_FLEN - 4)) {
        LOG_DBG("%s... unexpected long packet length=%d, buf=0x%x\r\n", __FUNCTION__, *len, (uint32_t)*buf);
        *len = 0; // Skip this unexpected long packet
    }
    LOG_DBG("=== %s...  ---End---\r\n", __FUNCTION__);
    return 0;
}

static void __numaker_gmacdev_rx_next(synopGMACdevice *gmacdev)
{
    LOG_DBG("=== %s... ---Start---\r\n", __FUNCTION__);
    /* Already did in synop_handle_received_data */
    /* No-op at this stage */    
#if 0    
    DmaDesc * rxdesc = (gmacdev->RxBusyDesc - 1);
	rxdesc->status = DescOwnByDma;
#endif

}

static void __numaker_gmacdev_trigger_rx(synopGMACdevice *gmacdev)
{
    LOG_DBG("=== %s...  ---Start---\r\n", __FUNCTION__);
//    dump_desc(gmacdev->RxBusyDesc);
    /* Enable the interrupt */
    synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
    /* Trigger RX DMA */
    synopGMAC_enable_dma_rx(gmacdev);
    synopGMAC_resume_dma_rx(gmacdev);
    LOG_DBG("%s... resume RX DMA\r\n", __FUNCTION__);
    LOG_DBG("=== %s...  ---End---\r\n", __FUNCTION__);
}

static void __numaker_gmacdev_packet_rx(const struct device *dev)
{
	struct eth_numaker_data *data = dev->data;
    synopGMACdevice *gmacdev = data->gmacdev;
    uint8_t *buffer;
    uint16_t len = 0;
    struct net_pkt *pkt;
    k_spinlock_key_t key;
    
    /* Get exclusive access, use spin-lock instead of mutex in ISR */    
    key = k_spin_lock(&data->rx_frame_buf_lock);

    /* Two approach: 1. recv all RX packets in one time.                  */
    /*               2. recv one RX and set pending interrupt for rx-next. */
    while(1) {
        /* get received frame */
        if (__numaker_gmacdev_get_rx_buf(gmacdev, &len, &buffer) != 0) {
            break; //return -1;
        }
        
        if (len > 0) {
            /* Allocate a memory buffer chain from buffer pool */
            /* Using root iface. It will be updated in net_recv_data() */
            pkt = net_pkt_rx_alloc_with_buffer(data->iface, len,
                           AF_UNSPEC, 0, K_NO_WAIT);
            if (!pkt) {
                LOG_ERR("pkt alloc frame-len=%d failed", len);
                goto next;
            }
            
            LOG_DBG("%s... length=%d, pkt=0x%x", __FUNCTION__, len, (uint32_t)pkt);
     
            /* deliver RX packet to upper layer */
            /* pack as one net_pkt */
            if (net_pkt_write(pkt, buffer, len)) {
                LOG_ERR("Unable to write RX frame into the pkt");
                net_pkt_unref(pkt);
                goto error;
            }
            
            if (pkt != NULL) {
                int res = net_recv_data(data->iface, pkt);
                if (res < 0) {
                    LOG_ERR("net_recv_data: %d", res);
                    net_pkt_unref(pkt);
                    goto error;
                }
            }
next:
            __numaker_gmacdev_rx_next(gmacdev);
        } else {
            break;
        }
    }
    __numaker_gmacdev_trigger_rx(gmacdev);

error:
    ;
    k_spin_unlock(&data->rx_frame_buf_lock, key);

}


static uint8_t *__numaker_gmacdev_get_tx_buf(synopGMACdevice *gmacdev)
{

    DmaDesc * txdesc = gmacdev->TxNextDesc;

    if(!synopGMAC_is_desc_empty(txdesc)) {
        return  (NULL);
    }
    
    if (synopGMAC_is_desc_owned_by_dma(txdesc))
    {
        return (NULL);
    } else {
//        dump_desc(txdesc);
        return (uint8_t *) (txdesc->buffer1);
    }
        
}

static void __numaker_gmacdev_trigger_tx(synopGMACdevice *gmacdev, uint16_t length)
{
    DmaDesc * txdesc = gmacdev->TxNextDesc;
    uint32_t  txnext = gmacdev->TxNext;
#if defined(NU_USING_HW_CHECKSUM)
    uint32_t offload_needed = 1;
#else
    uint32_t offload_needed = 0;
#endif
    
    (gmacdev->BusyTxDesc)++; //busy tx descriptor is incremented by one as it will be handed over to DMA    
    txdesc->length |= ((length <<DescSize1Shift) & DescSize1Mask);
    txdesc->status |=  (DescTxFirst | DescTxLast | DescTxIntEnable ); //ENH_DESC

    if(offload_needed) {
        /*
         Make sure that the OS you are running supports the IP and TCP checksum offloading,
         before calling any of the functions given below.
        */
        synopGMAC_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
    } else {
      	synopGMAC_tx_checksum_offload_bypass(gmacdev, txdesc);
    }
    __DSB();
    txdesc->status |= DescOwnByDma;//ENH_DESC

    gmacdev->TxNext = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? 0 : txnext + 1;
    gmacdev->TxNextDesc = synopGMAC_is_last_tx_desc(gmacdev,txdesc) ? gmacdev->TxDesc : (txdesc + 1);
    
    /* Enable the interrupt */
    synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);    
    /* Trigger TX DMA */    
    synopGMAC_resume_dma_tx(gmacdev);
//    synopGMAC_enable_dma_tx(gmacdev);

}


static int numaker_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_numaker_data *data = dev->data;
    synopGMACdevice *gmacdev = data->gmacdev;
	uint16_t total_len = net_pkt_get_len(pkt);
    uint8_t *buffer = NULL;

    /* Get exclusive access */    
	k_mutex_lock(&data->tx_frame_buf_mutex, K_FOREVER);

    if(total_len > NET_ETH_MAX_FRAME_SIZE) {
        /* NuMaker SDK reserve 2048 for tx_buf */
        LOG_ERR("%s TX packet length [%d] over max [%d]", __FUNCTION__, total_len, NET_ETH_MAX_FRAME_SIZE);
		goto error;
    }

    buffer = __numaker_gmacdev_get_tx_buf(gmacdev);
    LOG_DBG("%s ... buffer=0x%x", __FUNCTION__, (uint32_t)buffer);
    if (buffer == NULL) {
        goto error;
    }    

	if (net_pkt_read(pkt, buffer, total_len)) {
		goto error;
	}

    /* Prepare transmit descriptors to give to DMA */
    __numaker_gmacdev_trigger_tx(gmacdev, total_len);


	k_mutex_unlock(&data->tx_frame_buf_mutex);

	return 0;

error:
	LOG_ERR("%s Writing pkt to TX descriptor failed", __FUNCTION__);
	k_mutex_unlock(&data->tx_frame_buf_mutex);
	return -1;
}

static void numaker_eth_if_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_numaker_data *data = dev->data;
//    const struct eth_numaker_config *cfg = dev->config;
    synopGMACdevice *gmacdev = data->gmacdev;

	LOG_DBG("eth_if_init");
    
    /* Read mac address */
	__numaker_read_mac_addr(data->mac_addr);


	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);

	data->iface = iface;

	ethernet_init(iface);
    
    /* Enable GMAC device INT & TX/RX */
    __numaker_gmacdev_enable(gmacdev);

}

static int numaker_eth_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_numaker_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr,
		       config->mac_address.addr,
		       sizeof(data->mac_addr));
        synopGMAC_set_mac_address(NU_GMAC_INTF, data->mac_addr);
		net_if_set_link_addr(data->iface, data->mac_addr,
				     sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_DBG("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			data->mac_addr[0], data->mac_addr[1],
			data->mac_addr[2], data->mac_addr[3],
			data->mac_addr[4], data->mac_addr[5]);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static enum ethernet_hw_caps numaker_eth_get_cap(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

static const struct ethernet_api eth_numaker_driver_api = {
	.iface_api.init = numaker_eth_if_init,
	.get_capabilities = numaker_eth_get_cap,
	.set_config		= numaker_eth_set_config,
	.send = numaker_eth_tx,
};


/*----------------------------------------------------------------------------
  EMAC IRQ Handler
 *----------------------------------------------------------------------------*/
static void eth_numaker_isr(const struct device *dev)
{
	struct eth_numaker_data *data = dev->data;
    synopGMACdevice *gmacdev = data->gmacdev;
    uint32_t interrupt,dma_status_reg, mac_status_reg;
    int status;

    // Check GMAC interrupt
    mac_status_reg = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacInterruptStatus);
    if(mac_status_reg & GmacTSIntSts) {
    	gmacdev->synopGMACNetStats.ts_int = 1;
    	status = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacTSStatus);
    	if(!(status & (1 << 1))) {
    		NU_DEBUGF(("TS alarm flag not set??\n"));
    	} else {
    		NU_DEBUGF(("TS alarm!!!!!!!!!!!!!!!!\n"));
        }
    }
    if(mac_status_reg & GmacLPIIntSts) {
    	//NU_DEBUGF("LPI\n");
    	//LPIStsChange = 1;
    	//LPIReg = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacLPICtrlSts);
        ;
    }
    if(mac_status_reg & GmacRgmiiIntSts) {
    	uint32_t volatile reg;
    	reg = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacRgmiiCtrlSts);

    }
    synopGMACWriteReg((u32 *)gmacdev->MacBase, GmacInterruptStatus ,mac_status_reg);

    /*Read the Dma interrupt status to know whether the interrupt got generated by our device or not*/
    dma_status_reg = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaStatus);
    NU_DEBUGF(("i %08x %08x\n", mac_status_reg, dma_status_reg));

    if(dma_status_reg == 0)
        return;

    synopGMAC_disable_interrupt_all(gmacdev);

    NU_DEBUGF(("%s:Dma Status Reg: 0x%08x\n",__FUNCTION__,dma_status_reg));

    if(dma_status_reg & GmacPmtIntr) {
        NU_DEBUGF(("%s:: Interrupt due to PMT module\n",__FUNCTION__));
        synopGMAC_powerup_mac(gmacdev);
    }

    if(dma_status_reg & GmacLineIntfIntr) {
        NU_DEBUGF(("%s:: Interrupt due to GMAC LINE module\n",__FUNCTION__));
    }

    /*Now lets handle the DMA interrupts*/
    interrupt = synopGMAC_get_interrupt_type(gmacdev);
    NU_DEBUGF(("%s:Interrupts to be handled: 0x%08x\n",__FUNCTION__,interrupt));

    if(interrupt & synopGMACDmaError) {
        NU_DEBUGF(("%s::Fatal Bus Error Interrupt Seen\n",__FUNCTION__));
        synopGMAC_disable_dma_tx(gmacdev);
        synopGMAC_disable_dma_rx(gmacdev);

        synopGMAC_take_desc_ownership_tx(gmacdev);
        synopGMAC_take_desc_ownership_rx(gmacdev);

        synopGMAC_init_tx_rx_desc_queue(gmacdev);

        synopGMAC_reset(gmacdev);//reset the DMA engine and the GMAC ip
        synopGMAC_set_mac_address(NU_GMAC_INTF, data->mac_addr);
        synopGMAC_dma_bus_mode_init(gmacdev,DmaFixedBurstEnable| DmaBurstLength8 | DmaDescriptorSkip0/*DmaDescriptorSkip2*/ );
        synopGMAC_dma_control_init(gmacdev,DmaStoreAndForward);
        synopGMAC_init_rx_desc_base(gmacdev);
        synopGMAC_init_tx_desc_base(gmacdev);
        synopGMAC_mac_init(gmacdev);
        synopGMAC_enable_dma_rx(gmacdev);
        synopGMAC_enable_dma_tx(gmacdev);

    }

    if(interrupt & synopGMACDmaRxNormal) {
        NU_DEBUGF(("%s:: Rx Normal \r\n", __FUNCTION__));
        // to handle received data
        __numaker_gmacdev_packet_rx(dev);
    }

    if(interrupt & synopGMACDmaRxAbnormal) {
        LOG_ERR("%s::Abnormal Rx Interrupt Seen \r\n",__FUNCTION__);
    	gmacdev->synopGMACNetStats.rx_over_errors++;
        if(gmacdev->GMAC_Power_down == 0) {	// If Mac is not in powerdown
            synopGMAC_resume_dma_rx(gmacdev);//To handle GBPS with 12 descriptors
        }
    }

    if(interrupt & synopGMACDmaRxStopped) {
        LOG_ERR("%s::Receiver stopped seeing Rx interrupts \r\n",__FUNCTION__); //Receiver gone in to stopped state
        if(gmacdev->GMAC_Power_down == 0) {	// If Mac is not in powerdown
        	gmacdev->synopGMACNetStats.rx_over_errors++;
            synopGMAC_enable_dma_rx(gmacdev);
        }
    }

    if(interrupt & synopGMACDmaTxNormal) {
        NU_DEBUGF(("%s::Finished Normal Transmission \n",__FUNCTION__));
        synop_handle_transmit_over(0);//Do whatever you want after the transmission is over
        /* No-op at this stage for TX INT */
    }

    if(interrupt & synopGMACDmaTxAbnormal) {
        LOG_ERR("%s::Abnormal Tx Interrupt Seen\n",__FUNCTION__);
        if(gmacdev->GMAC_Power_down == 0) {	// If Mac is not in powerdown
            synop_handle_transmit_over(0);
            /* No-op at this stage for TX INT */
        }
    }

    if(interrupt & synopGMACDmaTxStopped) {
        LOG_ERR("%s::Transmitter stopped sending the packets\n",__FUNCTION__);
        if(gmacdev->GMAC_Power_down == 0) {	// If Mac is not in powerdown
            synopGMAC_disable_dma_tx(gmacdev);
            synopGMAC_take_desc_ownership_tx(gmacdev);

            synopGMAC_enable_dma_tx(gmacdev);
            LOG_ERR("%s::Transmission Resumed\n",__FUNCTION__);
        }
    }

    /* Enable the interrupt before returning from ISR*/
//    if( !(interrupt & synopGMACDmaRxNormal)) {  /* RxNormal will enable INT in numaker_eth_trigger_rx */
        synopGMAC_enable_interrupt(gmacdev,DmaIntEnable);
//    }
    return;

}

/* Declare pin-ctrl __pinctrl_dev_config__device_dts_ord_xx before PINCTRL_DT_INST_DEV_CONFIG_GET() */
PINCTRL_DT_INST_DEFINE(0);

static int eth_numaker_init(const struct device *dev)
{
    struct eth_numaker_config *cfg = dev->config;
	struct eth_numaker_data *data = dev->data;
    synopGMACdevice *gmacdev;
    
    /* Init MAC Address based on UUID*/
    uint8_t mac_addr[6]; 
	int ret = 0;
    struct numaker_scc_subsys scc_subsys;
    
    gmacdev = &GMACdev[NU_GMAC_INTF];
    data->gmacdev = gmacdev;

	k_mutex_init(&data->tx_frame_buf_mutex);

    /* Set config based on DTS */
	cfg->gmacBase = (uint32_t)DT_INST_REG_ADDR(0);
    cfg->id_rst = DT_INST_PROP(0, reset);
    cfg->phy_addr = DT_INST_PROP(0, phy_addr);
    cfg->clk_modidx = DT_INST_CLOCKS_CELL(0, clock_module_index);
    cfg->clk_src = DT_INST_CLOCKS_CELL(0, clock_source);
    cfg->clk_div = DT_INST_CLOCKS_CELL(0, clock_divider);
	cfg->clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0)));
    cfg->pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);

    eth_phy_addr = cfg->phy_addr;

    /* CLK controller */
    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = cfg->clk_modidx;
    scc_subsys.pcc.clk_src      = cfg->clk_src;
    scc_subsys.pcc.clk_div      = cfg->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t) &scc_subsys);
    if (ret != 0) {
        goto move_exit;
    }
    /* For EMAC, not need CLK_SetModuleClock() */

    SYS_UnlockReg();

    irq_disable(DT_INST_IRQN(0));
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state");
        goto move_exit;
	}

    // Reset this module
    SYS_ResetModule(cfg->id_rst);

    /* Read mac address */
	__numaker_read_mac_addr(mac_addr);

	/* Configure GMAC device */
	ret = __numaker_gmacdev_init(gmacdev, mac_addr, cfg->gmacBase);

	if (ret != 0) {
		LOG_ERR("GMAC failed to initialize");
        goto move_exit;
	}

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    eth_numaker_isr, DEVICE_DT_INST_GET(0), 0);    

	irq_enable(DT_INST_IRQN(0));

move_exit:
	SYS_LockReg();
	return ret;
}

static struct eth_numaker_data eth_numaker_data_inst;
static struct eth_numaker_config eth_numaker_cfg_inst;

ETH_NET_DEVICE_DT_INST_DEFINE(0,
		eth_numaker_init, NULL, &eth_numaker_data_inst,
		&eth_numaker_cfg_inst, CONFIG_ETH_INIT_PRIORITY, &eth_numaker_driver_api,
		NET_ETH_MTU /*MTU*/);
