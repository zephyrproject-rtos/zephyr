/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_ethernet

#define LOG_MODULE_NAME eth_npcm
#define LOG_LEVEL LOG_LEVEL_WRN

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <stdbool.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <soc.h>
#include <zephyr/sys/printk.h>
#include <common/reg/reg_def.h>
#include <zephyr/dt-bindings/clock/npcm_clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include "eth_npcm.h"
#include "eth.h"

#define ETH_NPCM_MTU 1500
#define PHY_ADDR	PHY_ADDRESS

#define NUVOTON_OUI_B0 0x08
#define NUVOTON_OUI_B1 0x00
#define NUVOTON_OUI_B2 0x27

#define DEV_CFG(dev)	((const struct eth_npcm_dev_cfg *)(dev)->config)

#define ETH_NPCM_REG_BASE    ((struct emac_reg *)(DT_INST_REG_ADDR(0)))

#define GET_FIRST_DMA_TX_DESC(heth)	(heth->TxDesc)
#define IS_ETH_DMATXDESC_OWN(dma_tx_desc)	(dma_tx_desc->Status & \
							ETH_DMATXDESC_OWN)

/* Device constant configuration parameters */
struct eth_npcm_dev_cfg {
	void (*config_func)(void);
	uint32_t clk_cfg;
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct eth_npcm_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct ETH_HANDLE_TYPE heth;
	struct k_mutex tx_mutex;
	struct k_sem rx_int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack,
		CONFIG_ETH_NPCM_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
	bool link_up;
};

#define __eth_npcm_desc __aligned(4)
#define __eth_npcm_buf  __aligned(4)
/* Ethernet Rx DMA Descriptors */
static struct ETH_DMA_DESCRIPTOR  DMARxDscrTab[ETH_RXBUFNB] __eth_npcm_desc;
/* Ethernet Tx DMA Descriptors */
static struct ETH_DMA_DESCRIPTOR  DMATxDscrTab[ETH_TXBUFNB] __eth_npcm_desc;
/* Ethernet Receive Buffers */
static uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __eth_npcm_buf;
/* Ethernet Transmit Buffers */
static uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __eth_npcm_buf;

static inline int __ETH_LOCK(struct ETH_HANDLE_TYPE *__HANDLE__)
{
	if ((__HANDLE__)->Lock == ETH_LOCK_LOCKED) {
		return ETH_RET_BUSY;
	} else {
		return 0;
	}
}

static inline void  __ETH_UNLOCK(struct ETH_HANDLE_TYPE *__HANDLE__)
{
	(__HANDLE__)->Lock = ETH_LOCK_UNLOCKED;
}

enum ETH_RET_STATUS ETH_WritePHYRegister(struct ETH_HANDLE_TYPE *heth,
		uint16_t PHYReg, uint32_t RegValue)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t tmpreg = 0;
	uint32_t timeout = 0xFFFF;

	/* Check the ETH peripheral state */
	if (heth->State == ETH_STATE_BUSY_WR) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY_WR;

	/* Keep only the CSR Clock Range CR[2:0] bits value */
	tmpreg = (emac_regs->MACMIIAR & (0x0F << NPCM_MACMIIAR_CR));
	/* Prepare the MII register address value */
	/* Set the PHY device address */
	tmpreg |= (((uint32_t)heth->InitParm.PhyAddress << 11) & (0x1F << NPCM_MACMIIAR_PA));
	/* Set the PHY register address */
	tmpreg |= (((uint32_t)PHYReg << 6) & (0x1F << NPCM_MACMIIAR_MR));
	/* Set the write mode */
	tmpreg |= BIT(NPCM_MACMIIAR_MW);
	/* Set the MII Busy bit */
	tmpreg |= BIT(NPCM_MACMIIAR_MB);

	/* Give the value to the MII data register */
	emac_regs->MACMIIDR = (uint16_t)RegValue;
	/* Write the result value into the MII Address register */
	emac_regs->MACMIIAR = tmpreg;

	/* Check for the Busy flag */
	while ((tmpreg & BIT(NPCM_MACMIIAR_MB)) == BIT(NPCM_MACMIIAR_MB)) {
		/* Check for the Timeout */
		if ((timeout--) == 0) {
			heth->State = ETH_STATE_READY;
			__ETH_UNLOCK(heth);        /* Process Unlocked */

			return ETH_RET_TIMEOUT;
		}
		tmpreg = emac_regs->MACMIIAR;
	}

	heth->State = ETH_STATE_READY;

	return ETH_RET_OK;
}

enum ETH_RET_STATUS ETH_ReadPHYRegister(struct ETH_HANDLE_TYPE *heth,
		uint16_t PHYReg, uint32_t *RegValue)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t tmpreg = 0;
	uint32_t timeout = 0xFFFF;

	/* Check the ETH peripheral state */
	if (heth->State == ETH_STATE_BUSY_RD) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY_RD;

	/* Keep only the CSR Clock Range CR[2:0] bits value */
	tmpreg = emac_regs->MACMIIAR & (0x0F << NPCM_MACMIIAR_CR);
	/* Prepare the MII address register value */
	/* Set the PHY device address   */
	tmpreg |= (((uint32_t)heth->InitParm.PhyAddress << 11) & (0x1F << NPCM_MACMIIAR_PA));
	/* Set the PHY register address */
	tmpreg |= (((uint32_t)PHYReg << 6) & (0x1F << NPCM_MACMIIAR_MR));
	/* Set the read mode            */
	tmpreg &= ~BIT(NPCM_MACMIIAR_MW);
	/* Set the MII Busy bit         */
	tmpreg |= BIT(NPCM_MACMIIAR_MB);
	/* Write the result value into the MII Address register */
	emac_regs->MACMIIAR = tmpreg;

	/* Check for the Busy flag */
	while ((tmpreg & BIT(NPCM_MACMIIAR_MB)) == BIT(NPCM_MACMIIAR_MB)) {
		/* Check for the Timeout */
		if ((timeout--) == 0) {
			heth->State = ETH_STATE_READY;
			__ETH_UNLOCK(heth);                  /* Process Unlocked */

			return ETH_RET_TIMEOUT;
		}
		tmpreg = emac_regs->MACMIIAR;
	}

	/* Get MACMIIDR value */
	*RegValue = (uint16_t)(emac_regs->MACMIIDR);

	heth->State = ETH_STATE_READY;

	return ETH_RET_OK;
}

enum ETH_RET_STATUS ETH_TransmitFrame(struct ETH_HANDLE_TYPE *heth, uint32_t FrameLength)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t bufcount = 0, size = 0, i = 0;

	if (__ETH_LOCK(heth)) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY;

	if (FrameLength == 0) {
		heth->State = ETH_STATE_READY;
		__ETH_UNLOCK(heth);

		LOG_ERR("FrameLength error!\r\n");
		return ETH_RET_ERROR;
	}

	/* Check if the descriptor is owned by the ETHERNET DMA (when set) or CPU (when reset) */
	if (((heth->TxDesc)->Status & ETH_DMATXDESC_OWN) != 0) {
		/* OWN bit set */
		heth->State = ETH_STATE_BUSY_TX;
		__ETH_UNLOCK(heth);

		LOG_DBG("OWN bit set!\r\n");
		return ETH_RET_ERROR;
	}

	/* Get the number of needed Tx buffers for the current frame */
	if (FrameLength > ETH_TX_BUF_SIZE) {
		bufcount = FrameLength / ETH_TX_BUF_SIZE;
		if (FrameLength % ETH_TX_BUF_SIZE) {
			bufcount++;
		}
	} else {
		bufcount = 1;
	}

	if (bufcount == 1) {
		/* Set LAST and FIRST segment */
		heth->TxDesc->Status |= ETH_DMATXDESC_FS|ETH_DMATXDESC_LS;
		/* Set frame size */
		heth->TxDesc->ControlBufferSize = (FrameLength & ETH_DMATXDESC_TBS1);
		/* Set Own bit of the Tx descriptor Status: gives the buffer back to ETHERNET DMA */
		heth->TxDesc->Status |= ETH_DMATXDESC_OWN;
		/* Point to next descriptor */
		heth->TxDesc = (struct ETH_DMA_DESCRIPTOR *)(heth->TxDesc->Buffer2NextDescAddr);
	} else {
		for (i = 0; i < bufcount; i++) {
			/* Clear FIRST and LAST segment bits */
			heth->TxDesc->Status &= ~(ETH_DMATXDESC_FS | ETH_DMATXDESC_LS);
			if (i == 0) {
				/* Setting the first segment bit */
				heth->TxDesc->Status |= ETH_DMATXDESC_FS;
			}
			/* Program size */
			heth->TxDesc->ControlBufferSize = (ETH_TX_BUF_SIZE & ETH_DMATXDESC_TBS1);
			if (i == (bufcount-1)) {
				/* Setting the last segment bit */
				heth->TxDesc->Status |= ETH_DMATXDESC_LS;
				size = FrameLength - (bufcount-1)*ETH_TX_BUF_SIZE;
				heth->TxDesc->ControlBufferSize = (size & ETH_DMATXDESC_TBS1);
			}
			/* Set Own bit of the Tx descriptor Status: gives the buffer
			 * back to ETHERNET DMA
			 */
			heth->TxDesc->Status |= ETH_DMATXDESC_OWN;
			/* point to next descriptor */
			heth->TxDesc = (struct ETH_DMA_DESCRIPTOR *)
				(heth->TxDesc->Buffer2NextDescAddr);
		}
	}

	/* When Tx Buffer unavailable flag is set: clear it and resume transmission */
	if ((emac_regs->DMASR & BIT(NPCM_DMASR_TBUS)) != 0) {
		/* Clear TBUS ETHERNET DMA flag */
		emac_regs->DMASR = BIT(NPCM_DMASR_TBUS);
		/* Resume DMA transmission*/
		emac_regs->DMATPDR = 0;
	}

	heth->State = ETH_STATE_READY;

	__ETH_UNLOCK(heth);

	return ETH_RET_OK;
}

enum ETH_RET_STATUS ETH_GetReceivedFrame(struct ETH_HANDLE_TYPE *heth)
{
	uint32_t descriptorscancounter = 0U;

	/* Process Locked */
	if (__ETH_LOCK(heth)) {
		return ETH_RET_BUSY;
	}

	/* Set ETH HAL State to BUSY */
	heth->State = ETH_STATE_BUSY;

	/* Scan descriptors owned by CPU */
	while (((heth->RxDesc->Status & ETH_DMARXDESC_OWN) == 0) &&
			(descriptorscancounter < ETH_RXBUFNB)) {
		/* Just for security */
		descriptorscancounter++;

		/* Check if first segment in frame */
		if ((heth->RxDesc->Status & (ETH_DMARXDESC_FS | ETH_DMARXDESC_LS)) ==
				(uint32_t)ETH_DMARXDESC_FS) {
			heth->RxFrameInfos.FSRxDesc = heth->RxDesc;
			heth->RxFrameInfos.SegCount = 1U;
			/* Point to next descriptor */
			heth->RxDesc = (struct ETH_DMA_DESCRIPTOR *)
				(heth->RxDesc->Buffer2NextDescAddr);
		}
		/* Check if intermediate segment */
		else if ((heth->RxDesc->Status & (ETH_DMARXDESC_LS | ETH_DMARXDESC_FS)) == 0) {
			/* Increment segment count */
			(heth->RxFrameInfos.SegCount)++;
			/* Point to next descriptor */
			heth->RxDesc = (struct ETH_DMA_DESCRIPTOR *)
				(heth->RxDesc->Buffer2NextDescAddr);
		} else {
			/* Should be last segment */

			/* Last segment */
			heth->RxFrameInfos.LSRxDesc = heth->RxDesc;

			/* Increment segment count */
			(heth->RxFrameInfos.SegCount)++;

			/* Check if last segment is first segment: one segment contains the frame */
			if ((heth->RxFrameInfos.SegCount) == 1U) {
				heth->RxFrameInfos.FSRxDesc = heth->RxDesc;
			}

			/* Get the Frame Length of the received packet:
			 * substruct 4 bytes of the CRC
			 */
			heth->RxFrameInfos.length = (((heth->RxDesc)->Status &
						ETH_DMARXDESC_FL) >> 16) - 4U;

			/* Get the address of the buffer start address */
			heth->RxFrameInfos.buffer = ((heth->RxFrameInfos).FSRxDesc)->Buffer1Addr;

			/* Point to next descriptor */
			heth->RxDesc = (struct ETH_DMA_DESCRIPTOR *)
				(heth->RxDesc->Buffer2NextDescAddr);

			/* Set HAL State to Ready */
			heth->State = ETH_STATE_READY;

			/* Process Unlocked */
			__ETH_UNLOCK(heth);

			/* Return function status */
			return ETH_RET_OK;
		}
	}
	/* Set HAL State to Ready */
	heth->State = ETH_STATE_READY;
	/* Process Unlocked */
	__ETH_UNLOCK(heth);
	/* Return function status */
	return ETH_RET_ERROR;
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	struct eth_npcm_dev_data *dev_data = dev->data;
	struct ETH_HANDLE_TYPE *heth;
	uint8_t *dma_buffer;
	int res;
	size_t total_len;
	struct ETH_DMA_DESCRIPTOR *dma_tx_desc;
	enum ETH_RET_STATUS hal_ret = ETH_RET_OK;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	total_len = net_pkt_get_len(pkt);
	if (total_len > ETH_TX_BUF_SIZE) {
		LOG_ERR("PKT too big");
		res = -EIO;
		goto error;
	}

	dma_tx_desc = GET_FIRST_DMA_TX_DESC(heth);
	while (IS_ETH_DMATXDESC_OWN(dma_tx_desc) != 0) {
		k_yield();
	}

	dma_buffer = (uint8_t *)(dma_tx_desc->Buffer1Addr);

	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		res = -ENOBUFS;
		goto error;
	}


	hal_ret = ETH_TransmitFrame(heth, total_len);

	if (hal_ret != ETH_RET_OK) {
		LOG_ERR("HAL_ETH_Transmit: failed!");
		res = -EIO;
		goto error;
	}

	if ((emac_regs->DMASR & BIT(NPCM_DMASR_TUS)) != 0) {
		/* Clear TUS ETHERNET DMA flag */
		emac_regs->DMASR = BIT(NPCM_DMASR_TUS);
		/* Resume DMA transmission*/
		emac_regs->DMATPDR = 0;
		res = -EIO;
		goto error;
	}

	res = 0;
error:
	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

static struct net_if *get_iface(struct eth_npcm_dev_data *ctx,
				uint16_t vlan_tag)
{
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
}

static struct net_pkt *eth_rx(const struct device *dev, uint16_t *vlan_tag)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	struct eth_npcm_dev_data *dev_data;
	struct ETH_HANDLE_TYPE *heth;
	struct ETH_DMA_DESCRIPTOR *dma_rx_desc;

	struct net_pkt *pkt;
	size_t total_len;
	uint8_t *dma_buffer;
	enum ETH_RET_STATUS hal_ret = ETH_RET_OK;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = dev->data;

	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;


	hal_ret = ETH_GetReceivedFrame(heth);
	if (hal_ret != ETH_RET_OK) {
		/* no frame available */
		return NULL;
	}

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (uint8_t *)heth->RxFrameInfos.buffer;

	pkt = net_pkt_rx_alloc_with_buffer(get_iface(dev_data, *vlan_tag),
					   total_len, AF_UNSPEC, 0, K_NO_WAIT);
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
		dma_rx_desc = (struct ETH_DMA_DESCRIPTOR *)
			(dma_rx_desc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth->RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it
	 * and resume reception.
	 */
	if ((emac_regs->DMASR & BIT(NPCM_DMASR_RBUS)) != 0) {
		/* Clear RBUS ETHERNET DMA flag */
		emac_regs->DMASR = BIT(NPCM_DMASR_RBUS);
		/* Resume DMA reception */
		emac_regs->DMARPDR = 0;
	}

	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(dev_data, *vlan_tag));
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	const struct device *dev;
	struct eth_npcm_dev_data *dev_data;
	struct net_pkt *pkt;
	int res;
	uint32_t status;
	enum ETH_RET_STATUS hal_ret = ETH_RET_OK;

	__ASSERT_NO_MSG(arg1 != NULL);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	dev = (const struct device *)arg1;
	dev_data = dev->data;

	__ASSERT_NO_MSG(dev_data != NULL);

	while (1) {
		res = k_sem_take(&dev_data->rx_int_sem,
			K_MSEC(CONFIG_ETH_NPCM_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS));
		if (res == 0) {
			/* semaphore taken, update link status and receive packets */
			if (dev_data->link_up != true) {
				dev_data->link_up = true;
				net_eth_carrier_on(get_iface(dev_data,
							     vlan_tag));
			}
			while ((pkt = eth_rx(dev, &vlan_tag)) != NULL) {
				res = net_recv_data(net_pkt_iface(pkt), pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(
							net_pkt_iface(pkt));
					LOG_ERR("Failed to enqueue frame "
						"into RX queue: %d", res);
					net_pkt_unref(pkt);
				}
			}
		} else if (res == -EAGAIN) {
			/* semaphore timeout period expired, check link status */
			hal_ret = ETH_ReadPHYRegister(&dev_data->heth,
				    PHY_BSR, (uint32_t *) &status);
			if (hal_ret == ETH_RET_OK) {
				if ((status & PHY_LINKED_STATUS) == PHY_LINKED_STATUS) {
					if (dev_data->link_up != true) {
						dev_data->link_up = true;
						net_eth_carrier_on(
							get_iface(dev_data,
								  vlan_tag));
					}
				} else {
					if (dev_data->link_up != false) {
						dev_data->link_up = false;
						net_eth_carrier_off(
							get_iface(dev_data,
								  vlan_tag));
					}
				}
			}
		}
	}
}

static void eth_isr(const struct device *dev)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	struct eth_npcm_dev_data *dev_data;
	uint32_t sts = emac_regs->DMASR;

	dev_data = dev->data;

	/* Frame received */
	if (sts & BIT(NPCM_DMASR_RS)) {
		/* Clear the Eth DMA Rx IT pending bits */
		emac_regs->DMASR = BIT(NPCM_DMASR_RS);
		dev_data->heth.State = ETH_STATE_READY;
		/* TODO: Need to check */
		k_sem_give(&dev_data->rx_int_sem);
		__ETH_UNLOCK(&dev_data->heth);
	}
	/* Frame transmitted */
	if (sts & BIT(NPCM_DMASR_TS)) {
		/* Clear the Eth DMA Rx IT pending bits */
		emac_regs->DMASR = BIT(NPCM_DMASR_TS);

	}
	/* Normal Interrupt Summary  */
	if (sts & BIT(NPCM_DMASR_NIS)) {
		emac_regs->DMASR = BIT(NPCM_DMASR_NIS);
	}

	/* Abnormal Interrupt Summary */
	if (sts & BIT(NPCM_DMASR_AIS)) {
		/* Clear the ETH DMA Error flags */
		emac_regs->DMASR = BIT(NPCM_DMASR_AIS);
	}
}

#if defined(CONFIG_ETH_NPCM_RANDOM_MAC)
static void generate_mac(uint8_t *mac_addr)
{
	gen_random_mac(mac_addr, NUVOTON_OUI_B0, NUVOTON_OUI_B1, NUVOTON_OUI_B2);
}
#endif

void __SetSMIClock(const struct device *dev)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(pcc));
	const struct eth_npcm_dev_cfg *config = dev->config;
	uint32_t core_clk;
	int ret;
	uint32_t value = 0;

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clk_cfg,
			&core_clk);
	if (ret < 0) {
		LOG_ERR("Get ethernet clock source rate error %d", ret);
		return;
	}

	/* Clock Range (1 MHz ~ 2.5 MHz) */
	value = (emac_regs->MACMIIAR & (0x0F << NPCM_MACMIIAR_CR));

	if (core_clk < 10000000) {
		value |= (0x08 << NPCM_MACMIIAR_CR); /* DIV 4 */
	} else if ((core_clk >= 10000000) && (core_clk < 20000000)) {
		value |= (0x0A << NPCM_MACMIIAR_CR); /* DIV 8 */
	} else if ((core_clk >= 20000000) && (core_clk <= 35000000)) {
		value |= (0x02 << NPCM_MACMIIAR_CR); /* DIV 16 */
	} else if ((core_clk >= 35000000) && (core_clk < 60000000)) {
		value |= (0x03 << NPCM_MACMIIAR_CR); /* DIV 26 */
	} else if ((core_clk >= 60000000) && (core_clk < 100000000)) {
		value |= (0x00 << NPCM_MACMIIAR_CR); /* DIV 42 */
	}

	emac_regs->MACMIIAR = value;
}



void ETH_MACAddressConfig(uint32_t MacAddr, uint8_t *Addr, uint8_t IsSrcAddr, uint8_t MacAddrMsk)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t addr_L = 0;
	uint32_t addr_H = 0;

	if (MacAddr > 31) {
		return;
	}

	addr_H = ((uint32_t)Addr[5] << 8) | (uint32_t)Addr[4];
	addr_L = ((uint32_t)Addr[3] << 24) | ((uint32_t)Addr[2] << 16) |
		((uint32_t)Addr[1] << 8) | Addr[0];

	if (MacAddr >= 1) {
		addr_H |= BIT(NPCM_MACA1HR_AE);
		if (IsSrcAddr) {
			addr_H |= BIT(NPCM_MACA1HR_SA);
		}
		if (MacAddrMsk) {
			addr_H |= ((MacAddrMsk & 0x3F) << NPCM_MACA1HR_MBC);
		}
	}

	if (MacAddr <= 15) {
		*((uint32_t *)((uint32_t)(&(emac_regs->MACA0HR)) + (MacAddr*8))) = addr_H;
		*((uint32_t *)((uint32_t)(&(emac_regs->MACA0LR)) + (MacAddr*8))) = addr_L;
	} else {
		*((uint32_t *)((uint32_t)(&(emac_regs->MACA16HR)) + ((MacAddr-16)*8))) = addr_H;
		*((uint32_t *)((uint32_t)(&(emac_regs->MACA16LR)) + ((MacAddr-16)*8))) = addr_L;
	}
}

void ETH_MACDMAConfig(struct ETH_HANDLE_TYPE *heth, uint32_t err)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t reg = 0;

	/* Auto-negotiation failed */
	if (err == 1) {
		/* Set Ethernet duplex mode to Full-duplex, and speed to 100Mbps*/
		(heth->InitParm).DuplexMode = ETH_MODE_FULLDUPLEX;
		(heth->InitParm).Speed = ETH_SPEED_100M;
	}

	/*------------------------ ETHERNET MACCR Configuration ---------------------------*/
	/* Speed */
	if (heth->InitParm.Speed == ETH_SPEED_100M) {
		reg |= BIT(NPCM_MACCR_FES);
	} else {
		reg &= ~BIT(NPCM_MACCR_FES);
	}

	/* Duplex mode */
	if (heth->InitParm.DuplexMode == ETH_MODE_FULLDUPLEX) {
		reg |= BIT(NPCM_MACCR_DM);
	} else {
		reg &= ~BIT(NPCM_MACCR_DM);
	}

	/* Ipv4 checksum */
	if (heth->InitParm.ChecksumMode == ETH_CHECKSUM_BY_HARDWARE) {
		reg |= BIT(NPCM_MACCR_IPCO);
	} else {
		reg &= ~BIT(NPCM_MACCR_IPCO);
	}

	/* Retry disable */
	reg |= BIT(NPCM_MACCR_RD);
	emac_regs->MACCR = reg;

	/*----------------------- ETHERNET MACFFR Configuration ---------------------------*/
	/* forwards all control frames to application except Pause frame. */
	emac_regs->MACFFR = BIT(NPCM_MACFFR_PCF_ForwardExcptPause);

	/*----------------------- ETHERNET MACHTHR and MACHTLR Configuration --------------*/
	emac_regs->MACHTHR = 0;
	emac_regs->MACHTLR = 0;

	/*----------------------- ETHERNET MACFCR Configuration ---------------------------*/
	/* Zero-quanta pause disable */
	emac_regs->MACFCR |= BIT(NPCM_MACFCR_ZQPD);

	/*----------------------- ETHERNET MACVLANTR Configuration ------------------------*/
	emac_regs->MACVLANTR = 0;

	/* Ethernet DMA default initialization ************************************/
	/*----------------------- ETHERNET DMAOMR Configuration ---------------------------*/
	emac_regs->DMAOMR = (BIT(NPCM_DMAOMR_RSF) | BIT(NPCM_DMAOMR_TSF));
	/*----------------------- ETHERNET DMABMR Configuration ------------------*/
	emac_regs->DMABMR = (0x80000 | 0x400 | BIT(NPCM_DMABMR_EDE));

	if ((heth->InitParm).RxMode == ETH_RXINTERRUPT_MODE) {
		/* Enable the Ethernet Rx Interrupt */
		emac_regs->DMAIER |= (BIT(NPCM_DMAIER_NISE) | BIT(NPCM_DMAIER_RIE));
	}

	/* Initialize MAC address in ethernet MAC */
	ETH_MACAddressConfig(0, heth->InitParm.MACAddr, 0, 0);
}

void ETH_Set_Negotiation(struct ETH_HANDLE_TYPE *heth)
{
	uint32_t value = 0;
	/* Read the result */
#if (EMAC_PHY == IP101A)
	ETH_ReadPHYRegister(heth, PHY_SMR, &value);
	if (value & PHY_SMR_DUPLEX) {
		(heth->InitParm).DuplexMode = ETH_MODE_FULLDUPLEX;
		LOG_DBG("full-");
	} else {
		(heth->InitParm).DuplexMode = ETH_MODE_HALFDUPLEX;
		LOG_DBG("half-");
	}
	if (value & PHY_SMR_SPEED) {
		(heth->InitParm).Speed = ETH_SPEED_100M;
		LOG_DBG("100 \r\n");
	} else {
		(heth->InitParm).Speed = ETH_SPEED_10M;
		LOG_DBG("10 \r\n");
	}

#elif (EMAC_PHY == DP83848)
	ETH_ReadPHYRegister(heth, PHY_STS, &value);
	if (value & PHY_STS_DUPLEX) {
		(heth->InitParm).DuplexMode = ETH_MODE_FULLDUPLEX;
		LOG_DBG("full-");
	} else {
		(heth->InitParm).DuplexMode = ETH_MODE_HALFDUPLEX;
		LOG_DBG("half-");
	}
	if (value & PHY_STS_SPEED) {
		(heth->InitParm).Speed = ETH_SPEED_10M;
		LOG_DBG("10 \r\n");
	} else {
		(heth->InitParm).Speed = ETH_SPEED_100M;
		LOG_DBG("100 \r\n");
	}
#endif
	/*-------------------- MAC DMA initialization and configuration ----------------*/
	/* Configure MAC and DMA */
	ETH_MACDMAConfig(heth, 0);
}

enum ETH_RET_STATUS ETH_DMATxDescListInit(struct ETH_HANDLE_TYPE *heth,
		struct ETH_DMA_DESCRIPTOR *DMATxDescTab, uint8_t *TxBuff, uint32_t TxBuffCount)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t i = 0;
	struct ETH_DMA_DESCRIPTOR *dmatxdesc;

	if (__ETH_LOCK(heth)) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY;

	/* Set the DMATxDescToSet pointer with the first one of the DMATxDescTab list */
	heth->TxDesc = DMATxDescTab;

	/* Fill each DMATxDesc descriptor with the right values */
	for (i = 0; i < TxBuffCount; i++) {
		/* Get the pointer on the member (i) of the Tx Desc list */
		dmatxdesc = DMATxDescTab + i;
		/* Set Second Address Chained bit */
		dmatxdesc->Status = ETH_DMATXDESC_TCH;
		/* Set Buffer1 address pointer */
		dmatxdesc->Buffer1Addr = (uint32_t)(&TxBuff[i*ETH_TX_BUF_SIZE]);
		if ((heth->InitParm).ChecksumMode == ETH_CHECKSUM_BY_HARDWARE) {
			/* Set the DMA Tx descriptors checksum insertion */
			dmatxdesc->Status |= ETH_DMATXDESC_CIC_TCPUDPICMP_FULL;
		}

		/* Initialize the next descriptor with the Next Descriptor Polling Enable */
		if (i < (TxBuffCount-1)) {
			/* Set next descriptor address register with next descriptor base address */
			dmatxdesc->Buffer2NextDescAddr = (uint32_t)(DMATxDescTab+i+1);
		} else {
			/* For last descriptor, set next descriptor address register equal to the
			 * first descriptor base address
			 */
			dmatxdesc->Buffer2NextDescAddr = (uint32_t) DMATxDescTab;
		}
	}
	/* Set Transmit Descriptor List Address Register */
	emac_regs->DMATDLAR = (uint32_t) DMATxDescTab;

	heth->State = ETH_STATE_READY;

	__ETH_UNLOCK(heth);

	return ETH_RET_OK;
}

/*------------------------------------------------------------------------------------------------*/
/**
 * @brief                           Initializes the DMA Rx descriptors in chain mode.
 * @param [in]      heth            The Ethernet handler
 * @param [in]      DMARxDescTab    Pointer to the first Rx desc list
 * @param [in]      RxBuff          Pointer to the first RxBuffer list
 * @param [in]      RxBuffCount     Number of the used Rx desc in the list
 * @return                          The return status
 */
/*------------------------------------------------------------------------------------------------*/
enum ETH_RET_STATUS ETH_DMARxDescListInit(struct ETH_HANDLE_TYPE *heth,
		struct ETH_DMA_DESCRIPTOR *DMARxDescTab, uint8_t *RxBuff, uint32_t RxBuffCount)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	uint32_t i = 0;
	struct ETH_DMA_DESCRIPTOR *DMARxDesc;

	if (__ETH_LOCK(heth)) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY;

	/* Set the Ethernet RxDesc pointer with the first one of the DMARxDescTab list */
	heth->RxDesc = DMARxDescTab;

	/* Fill each DMARxDesc descriptor with the right values */
	for (i = 0; i < RxBuffCount; i++) {
		/* Get the pointer on the member (i) of the Rx Desc list */
		DMARxDesc = DMARxDescTab+i;
		/* Set Own bit of the Rx descriptor Status */
		DMARxDesc->Status = ETH_DMARXDESC_OWN;
		/* Set Buffer1 size and Second Address Chained bit */
		DMARxDesc->ControlBufferSize = (ETH_DMARXDESC_RCH | ETH_RX_BUF_SIZE);
		/* Set Buffer1 address pointer */
		DMARxDesc->Buffer1Addr = (uint32_t)(&RxBuff[i*ETH_RX_BUF_SIZE]);
		if ((heth->InitParm).RxMode == ETH_RXINTERRUPT_MODE) {
			/* Enable Ethernet DMA Rx Descriptor interrupt */
			DMARxDesc->ControlBufferSize &= ~ETH_DMARXDESC_DIC;
		}

		/* Initialize the next descriptor with the Next Descriptor Polling Enable */
		if (i < (RxBuffCount-1)) {
			/* Set next descriptor address register with next descriptor base address */
			DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab+i+1);
		} else {
			/* For last descriptor, set next descriptor address register equal to the
			 * first descriptor base address
			 */
			DMARxDesc->Buffer2NextDescAddr = (uint32_t)(DMARxDescTab);
		}
	}

	/* Set Receive Descriptor List Address Register */
	emac_regs->DMARDLAR = (uint32_t) DMARxDescTab;

	heth->State = ETH_STATE_READY;

	__ETH_UNLOCK(heth);

	return ETH_RET_OK;
}

enum ETH_RET_STATUS ETH_Start(struct ETH_HANDLE_TYPE *heth)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;

	if (__ETH_LOCK(heth)) {
		return ETH_RET_BUSY;
	}

	heth->State = ETH_STATE_BUSY;

	/* Enable the MAC transmission */
	emac_regs->MACCR |= BIT(NPCM_MACCR_TE);
	/* Enable the MAC reception */
	emac_regs->MACCR |= BIT(NPCM_MACCR_RE);
	/* Set the Flush Transmit FIFO bit */
	emac_regs->DMAOMR |= BIT(NPCM_DMAOMR_FTF);
	/* Enable the DMA transmission */
	emac_regs->DMAOMR |= BIT(NPCM_DMAOMR_ST);
	/* Enable the DMA reception */
	emac_regs->DMAOMR |= BIT(NPCM_DMAOMR_SR);

	LOG_DBG("emac registers :\r\n");
	LOG_DBG("MACCR = %08x\r\n", emac_regs->MACCR);
	LOG_DBG("MACFFR = %08x\r\n", emac_regs->MACFFR);
	LOG_DBG("MACFCR = %08x\r\n", emac_regs->MACFCR);

	LOG_DBG("DMABMR = %08x\r\n", emac_regs->DMABMR);
	LOG_DBG("DMAOMR = %08x\r\n", emac_regs->DMAOMR);

	heth->State = ETH_STATE_READY;

	__ETH_UNLOCK(heth);

	return ETH_RET_OK;
}

static int eth_initialize(const struct device *dev)
{
	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	struct eth_npcm_dev_data *dev_data;
	struct ETH_HANDLE_TYPE *heth;
	uint32_t value = 0;
	uint32_t timeout = 0;
	const struct eth_npcm_dev_cfg *dev_cfg;
	int ret;

	dev_data = dev->data;
	heth = &dev_data->heth;

	heth->InitParm.MACAddr = dev_data->mac_addr;

	dev_cfg = dev->config;
	/* Configure pin-mux for EMAC device */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("EMAC pinctrl setup failed (%d)", ret);
		return -ENOTSUP;
	}

	if (heth->State == ETH_STATE_RESET) {
		__ETH_UNLOCK(heth);
	}

	/* EMAC Software reset */
	RegSetBit(emac_regs->DMABMR, BIT(NPCM_DMABMR_SR));
	timeout = 0;
	while ((emac_regs->DMABMR & BIT(NPCM_DMABMR_SR)) != 0) {
		timeout++;
		if (timeout > 0xFFFFFF) {
			return -ENOTSUP;
		}
	}

#if defined(CONFIG_ETH_NPCM_RANDOM_MAC)
	generate_mac(dev_data->mac_addr);
	heth->InitParm.MACAddr = dev_data->mac_addr;
#endif

	/*-------------------------------- MAC Initialization ----------------------*/
	/* Clock Range (1 MHz ~ 2.5 MHz) */
	__SetSMIClock(dev);

	/*-------------------- PHY initialization and configuration ----------------*/
	/* Reset PHY */
	if (ETH_WritePHYRegister(heth, PHY_BCR, PHY_BCR_RESET) != ETH_RET_OK) {
		ETH_MACDMAConfig(heth, 1);
		heth->State = ETH_STATE_READY;

		LOG_DBG("Reset PHY error.\r\n");
		return ETH_RET_ERROR;
	}
	/* Delay to assure PHY reset */
	timeout = 0xFFF;
	while (timeout--)
		;

	/* PHY loopback mode */
	if ((heth->InitParm).PhyLoopback == 1) {
		ETH_ReadPHYRegister(heth, PHY_BCR, &value);
		value |= PHY_BCR_LOOPBACK;
		ETH_WritePHYRegister(heth, PHY_BCR, value);
	}

	/* Negotiation */
	if ((heth->InitParm).AutoNegotiation == 1) {
		timeout = 0xFFFF;
		/* We wait for linked status and auto-negotiation completed status */
		while (timeout-- > 0) {
			ETH_ReadPHYRegister(heth, PHY_BSR, &value);
			if ((value & (PHY_BSR_LINKED_STATUS | PHY_BSR_AUTONEGO_COMPLETE)) ==
					(PHY_BSR_LINKED_STATUS | PHY_BSR_AUTONEGO_COMPLETE)) {
				LOG_DBG("PHY_BSR : %4xX\r\n", value);
				break;
			}

			if (timeout == 0) {
				ETH_MACDMAConfig(heth, 1);
				heth->State = ETH_STATE_READY;
				__ETH_UNLOCK(heth);
				LOG_ERR("Wait error.\r\n");

				return -ENOTSUP;
			}
		}

		LOG_DBG("auto-nego. : ");
	} else {
		/* Manual negotiation */
		if (((heth->InitParm).DuplexMode == ETH_MODE_FULLDUPLEX) &&
				((heth->InitParm).Speed == ETH_SPEED_100M)) {
			value = PHY_BCR_FULLDUPLEX_100M;
		} else if (((heth->InitParm).DuplexMode == ETH_MODE_FULLDUPLEX) &&
				((heth->InitParm).Speed == ETH_SPEED_10M)) {
			value = PHY_BCR_FULLDUPLEX_10M;
		} else if (((heth->InitParm).DuplexMode == ETH_MODE_HALFDUPLEX) &&
				((heth->InitParm).Speed == ETH_SPEED_100M)) {
			value = PHY_BCR_HALFDUPLEX_100M;
		} else if (((heth->InitParm).DuplexMode == ETH_MODE_HALFDUPLEX) &&
				((heth->InitParm).Speed == ETH_SPEED_10M)) {
			value = PHY_BCR_HALFDUPLEX_10M;
		}

		/* PHY loopback mode */
		if ((heth->InitParm).PhyLoopback == 1) {
			value |= PHY_BCR_LOOPBACK;
		}

		if (ETH_WritePHYRegister(heth, PHY_BCR, value) != ETH_RET_OK) {
			ETH_MACDMAConfig(heth, 1);
			heth->State = ETH_STATE_READY;

			return ETH_RET_ERROR;
		}
		timeout = 0x7FFF;
		/* We wait for linked status */
		while (timeout-- > 0) {
			ETH_ReadPHYRegister(heth, PHY_BSR, &value);
			if (value & PHY_BSR_LINKED_STATUS) {
				LOG_DBG("PHY_BSR : %4x\r\n", value);
				break;
			}

			if (timeout == 0) {
				ETH_MACDMAConfig(heth, 1);
				heth->State = ETH_STATE_READY;
				__ETH_UNLOCK(heth);
				LOG_ERR("Wait error.\r\n");

				return ETH_RET_TIMEOUT;
			}
		}

		LOG_DBG("manual-nego.: ");
	}
	ETH_Set_Negotiation(heth);

	heth->State = ETH_STATE_READY;

	dev_data->link_up = false;

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack),
			rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_NPCM_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	k_thread_name_set(&dev_data->rx_thread, "stm_eth");

	/* Initialize Tx Descriptors list: Chain Mode */
	ETH_DMATxDescListInit(heth, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);

	/* Initialize Rx Descriptors list: Chain Mode  */
	ETH_DMARxDescListInit(heth, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

	if (ETH_Start(heth) != ETH_RET_OK) {
		LOG_ERR("ETH_Start{_IT} failed");
	}

	/* Mask all Rx interrupt */
	emac_regs->MMCRIMR = 0xFFFFFFFF;
	/* Mask all Tx interrupt */
	emac_regs->MMCTIMR = 0xFFFFFFFF;
	emac_regs->mmc_ipc_intr_mask_rx = 0xFFFFFFFF;

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev;
	struct eth_npcm_dev_data *dev_data;
	const struct eth_npcm_dev_cfg *dev_cfg;
	bool is_first_init = false;

	__ASSERT_NO_MSG(iface != NULL);

	dev = net_if_get_device(iface);
	__ASSERT_NO_MSG(dev != NULL);

	dev_data = dev->data;
	__ASSERT_NO_MSG(dev_data != NULL);
	dev_cfg = dev->config;

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (dev_data->iface == NULL) {
		dev_data->iface = iface;
		is_first_init = true;
	}

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_PROMISC);

	if (is_first_init) {
		/* Now that the iface is setup, we are safe to enable IRQs. */
		__ASSERT_NO_MSG(DEV_CFG(dev)->config_func != NULL);
		dev_cfg->config_func();
	}
}

static enum ethernet_hw_caps eth_npcm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_PROMISC_MODE;
}

static int eth_npcm_set_config(const struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{

	struct emac_reg *const emac_regs = ETH_NPCM_REG_BASE;
	struct eth_npcm_dev_data *dev_data;
	struct ETH_HANDLE_TYPE *heth;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		dev_data = dev->data;
		heth = &dev_data->heth;

		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		heth->InitParm.MACAddr = dev_data->mac_addr;

		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		emac_regs->MACFFR |= BIT(NPCM_MACFFR_PM);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_npcm_get_capabilities,
	.set_config = eth_npcm_set_config,
	.send = eth_tx,
};

static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

PINCTRL_DT_INST_DEFINE(0);

static const struct eth_npcm_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.clk_cfg = DT_INST_PHA(0, clocks, clk_cfg),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct eth_npcm_dev_data eth0_data = {
	.mac_addr = {
		NUVOTON_OUI_B0,
		NUVOTON_OUI_B1,
		NUVOTON_OUI_B2,
#if !defined(CONFIG_ETH_NPCM_RANDOM_MAC)
		CONFIG_ETH_NPCM_MAC3,
		CONFIG_ETH_NPCM_MAC4,
		CONFIG_ETH_NPCM_MAC5
	#endif
	},
	.heth = {
		.InitParm = {
			.AutoNegotiation = 1,
			.Speed = ETH_SPEED_100M,
			.DuplexMode = ETH_MODE_FULLDUPLEX,
			.RxMode = ETH_RXINTERRUPT_MODE,
			.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE, /* ETH_CHECKSUM_BY_HARDWARE */
			.PhyAddress = PHY_ADDRESS,
			.PhyLoopback = 0,
		},
		.State = ETH_STATE_RESET,
	},


};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize,
		    NULL, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_NPCM_MTU);
