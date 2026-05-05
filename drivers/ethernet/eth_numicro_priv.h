/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NUMICRO_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NUMICRO_PRIV_H_

#include <zephyr/types.h>

struct numicro_eth_dma_desc
{
	volatile uint32_t status;
	uint32_t buffer_addr;
	uint32_t tx_status;
	uint32_t next_desc;

	/* Backup register to support timestamp data */
	uint32_t buffer_addr_back;
	uint32_t next_desc_back;
};

#define NUMICRO_ETH_DMA_DESC_OWN_Pos 31

#define NUMICRO_ETH_DMA_DESC_RX_RTSAS_Pos 23
#define NUMICRO_ETH_DMA_DESC_RX_RPIF_Pos 22
#define NUMICRO_ETH_DMA_DESC_RX_ALIEIF_Pos 21
#define NUMICRO_ETH_DMA_DESC_RX_RXGDIF_Pos 20
#define NUMICRO_ETH_DMA_DESC_RX_LPIF_Pos 19
#define NUMICRO_ETH_DMA_DESC_RX_CRCEIF_Pos 17
#define NUMICRO_ETH_DMA_DESC_RX_RXIF_Pos 16

#define NUMICRO_ETH_DMA_DESC_RX_RBC_Pos 0
#define NUMICRO_ETH_DMA_DESC_RX_RBC_Msk 0xff

#define NUMICRO_ETH_DMA_DESC_TX_TTSEN_Pos 3
#define NUMICRO_ETH_DMA_DESC_TX_INTEN_Pos 2
#define NUMICRO_ETH_DMA_DESC_TX_CRCAPP_Pos 1
#define NUMICRO_ETH_DMA_DESC_TX_PADEN_Pos 0

#define NUMICRO_ETH_DMA_DESC_IS_CPU_OWNED(desc) \
((desc->status & BIT(NUMICRO_ETH_DMA_DESC_OWN_Pos)) == 0)

/* Rx Frame Descriptor Status */
#define EMAC_RXFD_RTSAS   0x0080UL  /*!<  Time Stamp Available */
#define EMAC_RXFD_RP      0x0040UL  /*!<  Runt Packet */
#define EMAC_RXFD_ALIE    0x0020UL  /*!<  Alignment Error */
#define EMAC_RXFD_RXGD    0x0010UL  /*!<  Receiving Good packet received */
#define EMAC_RXFD_PTLE    0x0008UL  /*!<  Packet Too Long Error */
#define EMAC_RXFD_CRCE    0x0002UL  /*!<  CRC Error */
#define EMAC_RXFD_RXINTR  0x0001UL  /*!<  Interrupt on receive */

/* Tx Frame Descriptor's Control bits */
#define EMAC_TXFD_TTSEN     0x08UL      /*!<  Tx time stamp enable */
#define EMAC_TXFD_INTEN     0x04UL      /*!<  Tx interrupt enable */
#define EMAC_TXFD_CRCAPP    0x02UL      /*!<  Append CRC */
#define EMAC_TXFD_PADEN     0x01UL      /*!<  Padding mode enable */

/* Tx Frame Descriptor Status */
#define EMAC_TXFD_TXINTR 0x0001UL  /*!<  Interrupt on Transmit */
#define EMAC_TXFD_DEF    0x0002UL  /*!<  Transmit deferred  */
#define EMAC_TXFD_TXCP   0x0008UL  /*!<  Transmission Completion  */
#define EMAC_TXFD_EXDEF  0x0010UL  /*!<  Exceed Deferral */
#define EMAC_TXFD_NCS    0x0020UL  /*!<  No Carrier Sense Error */
#define EMAC_TXFD_TXABT  0x0040UL  /*!<  Transmission Abort  */
#define EMAC_TXFD_LC     0x0080UL  /*!<  Late Collision  */
#define EMAC_TXFD_TXHA   0x0100UL  /*!<  Transmission halted */
#define EMAC_TXFD_PAU    0x0200UL  /*!<  Paused */
#define EMAC_TXFD_SQE    0x0400UL  /*!<  SQE error  */
#define EMAC_TXFD_TTSAS  0x0800UL  /*!<  Time Stamp available */



/* TODO check values */
#define ETH_RXBUF_NB CONFIG_ETH_NUMICRO_RXBUF_COUNT
#define ETH_TXBUF_NB CONFIG_ETH_NUMICRO_TXBUF_COUNT
#define ETH_RXBUF_SIZE NET_ETH_MAX_FRAME_SIZE
#define ETH_TXBUF_SIZE NET_ETH_MAX_FRAME_SIZE

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NUMICRO_PRIV_H_ */