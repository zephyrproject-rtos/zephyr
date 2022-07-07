/*
 * Xilinx Processor System Gigabit Ethernet controller (GEM) driver
 *
 * Driver private data declarations
 *
 * Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_ETHERNET_ETH_XLNX_GEM_PRIV_H_
#define _ZEPHYR_DRIVERS_ETHERNET_ETH_XLNX_GEM_PRIV_H_

#define DT_DRV_COMPAT xlnx_gem

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/net_pkt.h>

#include "phy_xlnx_gem.h"

#define ETH_XLNX_BUFFER_ALIGNMENT			4 /* RX/TX buffer alignment (in bytes) */

/* Buffer descriptor (BD) related defines */

/* Receive Buffer Descriptor bits & masks: comp. Zynq-7000 TRM, Table 16-2. */

/*
 * Receive Buffer Descriptor address word:
 * [31 .. 02] Mask for effective buffer address -> excludes [1..0]
 * [01]       Wrap bit, last BD in RX BD ring
 * [00]       BD used bit
 */
#define ETH_XLNX_GEM_RXBD_WRAP_BIT			0x00000002
#define ETH_XLNX_GEM_RXBD_USED_BIT			0x00000001
#define ETH_XLNX_GEM_RXBD_BUFFER_ADDR_MASK		0xFFFFFFFC

/*
 * Receive Buffer Descriptor control word:
 * [31]       Broadcast detected
 * [30]       Multicast hash match detected
 * [29]       Unicast hash match detected
 * [27]       Specific address match detected
 * [26 .. 25] Bits indicating which specific address register was matched
 * [24]       this bit has different semantics depending on whether RX checksum
 *            offloading is enabled or not
 * [23 .. 22] These bits have different semantics depending on whether RX check-
 *            sum offloading is enabled or not
 * [21]       VLAN tag (type ID 0x8100) detected
 * [20]       Priority tag: VLAN tag (type ID 0x8100) and null VLAN identifier
 *            detected
 * [19 .. 17] VLAN priority
 * [16]       Canonical format indicator bit
 * [15]       End-of-frame bit
 * [14]       Start-of-frame bit
 * [13]       FCS status bit for FCS ignore mode
 * [12 .. 00] Data length of received frame
 */
#define ETH_XLNX_GEM_RXBD_BCAST_BIT			0x80000000
#define ETH_XLNX_GEM_RXBD_MCAST_HASH_MATCH_BIT		0x40000000
#define ETH_XLNX_GEM_RXBD_UCAST_HASH_MATCH_BIT		0x20000000
#define ETH_XLNX_GEM_RXBD_SPEC_ADDR_MATCH_BIT		0x08000000
#define ETH_XLNX_GEM_RXBD_SPEC_ADDR_MASK		0x00000003
#define ETH_XLNX_GEM_RXBD_SPEC_ADDR_SHIFT		25
#define ETH_XLNX_GEM_RXBD_BIT24				0x01000000
#define ETH_XLNX_GEM_RXBD_BITS23_22_MASK		0x00000003
#define ETH_XLNX_GEM_RXBD_BITS23_22_SHIFT		22
#define ETH_XLNX_GEM_RXBD_VLAN_TAG_DETECTED_BIT		0x00200000
#define ETH_XLNX_GEM_RXBD_PRIO_TAG_DETECTED_BIT		0x00100000
#define ETH_XLNX_GEM_RXBD_VLAN_PRIORITY_MASK		0x00000007
#define ETH_XLNX_GEM_RXBD_VLAN_PRIORITY_SHIFT		17
#define ETH_XLNX_GEM_RXBD_CFI_BIT			0x00010000
#define ETH_XLNX_GEM_RXBD_END_OF_FRAME_BIT		0x00008000
#define ETH_XLNX_GEM_RXBD_START_OF_FRAME_BIT		0x00004000
#define ETH_XLNX_GEM_RXBD_FCS_STATUS_BIT		0x00002000
#define ETH_XLNX_GEM_RXBD_FRAME_LENGTH_MASK		0x00001FFF

/* Transmit Buffer Descriptor bits & masks: comp. Zynq-7000 TRM, Table 16-3. */

/*
 * Transmit Buffer Descriptor control word:
 * [31]       BD used marker
 * [30]       Wrap bit, last BD in TX BD ring
 * [29]       Retry limit exceeded
 * [27]       TX frame corruption due to AHB/AXI error, HRESP errors or buffers
 *            exhausted mid-frame
 * [26]       Late collision, TX error detected
 * [22 .. 20] Transmit IP/TCP/UDP checksum generation offload error bits
 * [16]       No CRC appended by MAC
 * [15]       Last buffer bit, indicates end of current TX frame
 * [13 .. 00] Data length in the BD's associated buffer
 */
#define ETH_XLNX_GEM_TXBD_USED_BIT			0x80000000
#define ETH_XLNX_GEM_TXBD_WRAP_BIT			0x40000000
#define ETH_XLNX_GEM_TXBD_RETRY_BIT			0x20000000
#define ETH_XLNX_GEM_TXBD_TX_FRAME_CORRUPT_BIT		0x08000000
#define ETH_XLNX_GEM_TXBD_LATE_COLLISION_BIT		0x04000000
#define ETH_XLNX_GEM_TXBD_CKSUM_OFFLOAD_ERROR_MASK	0x00000007
#define ETH_XLNX_GEM_TXBD_CKSUM_OFFLOAD_ERROR_SHIFT	20
#define ETH_XLNX_GEM_TXBD_NO_CRC_BIT			0x00010000
#define ETH_XLNX_GEM_TXBD_LAST_BIT			0x00008000
#define ETH_XLNX_GEM_TXBD_LEN_MASK			0x00003FFF
#define ETH_XLNX_GEM_TXBD_ERR_MASK			0x3C000000

#define ETH_XLNX_GEM_CKSUM_NO_ERROR			0x00000000
#define ETH_XLNX_GEM_CKSUM_VLAN_HDR_ERROR		0x00000001
#define ETH_XLNX_GEM_CKSUM_SNAP_HDR_ERROR		0x00000002
#define ETH_XLNX_GEM_CKSUM_IP_TYPE_OR_LEN_ERROR		0x00000003
#define ETH_XLNX_GEM_CKSUM_NOT_VLAN_SNAP_IP_ERROR	0x00000004
#define ETH_XLNX_GEM_CKSUM_UNSUPP_PKT_FRAG_ERROR	0x00000005
#define ETH_XLNX_GEM_CKSUM_NOT_TCP_OR_UDP_ERROR		0x00000006
#define ETH_XLNX_GEM_CKSUM_PREMATURE_END_ERROR		0x00000007

#if defined(CONFIG_SOC_FAMILY_XILINX_ZYNQ7000)
/*
 * Zynq-7000 TX clock configuration:
 *
 * SLCR unlock & lock registers, magic words:
 * comp. Zynq-7000 TRM, chapter B.28, registers SLCR_LOCK and SLCR_UNLOCK,
 * p. 1576f.
 *
 * GEMx_CLK_CTRL (SLCR) registers:
 * [25 .. 20] Reference clock divisor 1
 * [13 .. 08] Reference clock divisor 0
 * [00]       Clock active bit
 */
#define ETH_XLNX_SLCR_LOCK_REGISTER_ADDRESS		0xF8000004
#define ETH_XLNX_SLCR_UNLOCK_REGISTER_ADDRESS		0xF8000008
#define ETH_XLNX_SLCR_LOCK_KEY				0x767B
#define ETH_XLNX_SLCR_UNLOCK_KEY			0xDF0D
#define ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR_MASK	0x0000003F
#define ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR1_SHIFT	20
#define ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR0_SHIFT	8
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_CLKACT_BIT       0x02000000
#elif defined(CONFIG_SOC_XILINX_ZYNQMP)
/*
 * UltraScale TX clock configuration: comp.
 * https://www.xilinx.com/html_docs/registers/ug1087/ug1087-zynq-ultrascale-registers.html
 *
 * CRL_WPROT (CRL_APB) register:
 * [00] CRL APB register space write protection bit
 *
 * GEMx_REF_CTRL (CRL_APB) registers:
 * [30]       RX channel clock active bit
 * [29]       Clock active bit
 * [21 .. 16] Reference clock divisor 1
 * [13 .. 08] Reference clock divisor 0
 */
#define ETH_XLNX_CRL_APB_WPROT_REGISTER_ADDRESS		0xFF5E001C
#define ETH_XLNX_CRL_APB_WPROT_BIT			0x00000001
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR_MASK	0x0000003F
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR1_SHIFT	16
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR0_SHIFT	8
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_RX_CLKACT_BIT    0x04000000
#define ETH_XLNX_CRL_APB_GEMX_REF_CTRL_CLKACT_BIT       0x02000000
#endif /* CONFIG_SOC_FAMILY_XILINX_ZYNQ7000 || CONFIG_SOC_XILINX_ZYNQMP */

/*
 * Register offsets within the respective GEM's address space:
 * NWCTRL   = gem.net_ctrl       Network Control           register
 * NWCFG    = gem.net_cfg        Network Configuration     register
 * NWSR     = gem.net_status     Network Status            register
 * DMACR    = gem.dma_cfg        DMA Control               register
 * TXSR     = gem.tx_status      TX Status                 register
 * RXQBASE  = gem.rx_qbar        RXQ base address          register
 * TXQBASE  = gem.tx_qbar        TXQ base address          register
 * RXSR     = gem.rx_status      RX Status                 register
 * ISR      = gem.intr_status    Interrupt status          register
 * IER      = gem.intr_en        Interrupt enable          register
 * IDR      = gem.intr_dis       Interrupt disable         register
 * IMR      = gem.intr_mask      Interrupt mask            register
 * PHYMNTNC = gem.phy_maint      PHY maintenance           register
 * LADDR1L  = gem.spec_addr1_bot Specific address 1 bottom register
 * LADDR1H  = gem.spec_addr1_top Specific address 1 top    register
 * LADDR2L  = gem.spec_addr2_bot Specific address 2 bottom register
 * LADDR2H  = gem.spec_addr2_top Specific address 2 top    register
 * LADDR3L  = gem.spec_addr3_bot Specific address 3 bottom register
 * LADDR3H  = gem.spec_addr3_top Specific address 3 top    register
 * LADDR4L  = gem.spec_addr4_bot Specific address 4 bottom register
 * LADDR4H  = gem.spec_addr4_top Specific address 4 top    register
 */
#define ETH_XLNX_GEM_NWCTRL_OFFSET			0x00000000
#define ETH_XLNX_GEM_NWCFG_OFFSET			0x00000004
#define ETH_XLNX_GEM_NWSR_OFFSET			0x00000008
#define ETH_XLNX_GEM_DMACR_OFFSET			0x00000010
#define ETH_XLNX_GEM_TXSR_OFFSET			0x00000014
#define ETH_XLNX_GEM_RXQBASE_OFFSET			0x00000018
#define ETH_XLNX_GEM_TXQBASE_OFFSET			0x0000001C
#define ETH_XLNX_GEM_RXSR_OFFSET			0x00000020
#define ETH_XLNX_GEM_ISR_OFFSET				0x00000024
#define ETH_XLNX_GEM_IER_OFFSET				0x00000028
#define ETH_XLNX_GEM_IDR_OFFSET				0x0000002C
#define ETH_XLNX_GEM_IMR_OFFSET				0x00000030
#define ETH_XLNX_GEM_PHY_MAINTENANCE_OFFSET		0x00000034
#define ETH_XLNX_GEM_LADDR1L_OFFSET			0x00000088
#define ETH_XLNX_GEM_LADDR1H_OFFSET			0x0000008C
#define ETH_XLNX_GEM_LADDR2L_OFFSET			0x00000090
#define ETH_XLNX_GEM_LADDR2H_OFFSET			0x00000094
#define ETH_XLNX_GEM_LADDR3L_OFFSET			0x00000098
#define ETH_XLNX_GEM_LADDR3H_OFFSET			0x0000009C
#define ETH_XLNX_GEM_LADDR4L_OFFSET			0x000000A0
#define ETH_XLNX_GEM_LADDR4H_OFFSET			0x000000A4

/*
 * Masks for clearing registers during initialization:
 * gem.net_ctrl  [clear_stat_regs]
 * gem.tx_status [7..0]
 * gem.rx_status [3..0]
 * gem.intr_dis  [26..0]
 */
#define ETH_XLNX_GEM_STATCLR_MASK			0x00000020
#define ETH_XLNX_GEM_TXSRCLR_MASK			0x000000FF
#define ETH_XLNX_GEM_RXSRCLR_MASK			0x0000000F
#define ETH_XLNX_GEM_IDRCLR_MASK			0x07FFFFFF

/* (Shift) masks for individual registers' bits / bitfields */

/*
 * gem.net_ctrl:
 * [15]       Store 1588 receive timestamp in CRC field
 * [12]       Transmit zero quantum pause frame
 * [11]       Transmit pause frame
 * [10]       Halt transmission after current frame
 * [09]       Start transmission (tx_go)
 * [07]       Enable writing to statistics counters
 * [06]       Increment statistics registers - for testing purposes only
 * [05]       Clear statistics registers
 * [04]       Enable MDIO port
 * [03]       Enable transmit
 * [02]       Enable receive
 * [01]       Local loopback mode
 */
#define ETH_XLNX_GEM_NWCTRL_RXTSTAMP_BIT		0x00008000
#define ETH_XLNX_GEM_NWCTRL_ZEROPAUSETX_BIT		0x00001000
#define ETH_XLNX_GEM_NWCTRL_PAUSETX_BIT			0x00000800
#define ETH_XLNX_GEM_NWCTRL_HALTTX_BIT			0x00000400
#define ETH_XLNX_GEM_NWCTRL_STARTTX_BIT			0x00000200
#define ETH_XLNX_GEM_NWCTRL_STATWEN_BIT			0x00000080
#define ETH_XLNX_GEM_NWCTRL_STATINC_BIT			0x00000040
#define ETH_XLNX_GEM_NWCTRL_STATCLR_BIT			0x00000020
#define ETH_XLNX_GEM_NWCTRL_MDEN_BIT			0x00000010
#define ETH_XLNX_GEM_NWCTRL_TXEN_BIT			0x00000008
#define ETH_XLNX_GEM_NWCTRL_RXEN_BIT			0x00000004
#define ETH_XLNX_GEM_NWCTRL_LOOPEN_BIT			0x00000002

/*
 * gem.net_cfg:
 * [30]       Ignore IPG RX Error
 * [29]       Disable rejection of non-standard preamble
 * [28]       Enable IPG stretch
 * [27]       Enable SGMII mode
 * [26]       Disable rejection of frames with FCS errors
 * [25]       Enable frames to be received in HDX mode while transmitting
 * [24]       Enable RX checksum offload to hardware
 * [23]       Do not copy pause frames to memory
 * [22 .. 21] Data bus width
 * [20 .. 18] MDC clock division setting
 * [17]       Discard FCS from received frames
 * [16]       RX length field error frame discard enable
 * [15 .. 14] Receive buffer offset, # of bytes
 * [13]       Enable pause TX upon 802.3 pause frame reception
 * [12]       Retry test - for testing purposes only
 * [11]       Use TBI instead of the GMII/MII interface
 * [10]       Gigabit mode enable
 * [09]       External address match enable
 * [08]       Enable 1536 byte frames reception
 * [07]       Receive unicast hash frames enable
 * [06]       Receive multicast hash frames enable
 * [05]       Disable broadcast frame reception
 * [04]       Copy all frames = promiscuous mode
 * [02]       Discard non-VLAN frames enable
 * [01]       Full duplex enable
 * [00]       Speed selection: 1 = 100Mbit/s, 0 = 10 Mbit/s, GBE mode is
 *            set separately in bit [10]
 */
#define ETH_XLNX_GEM_NWCFG_IGNIPGRXERR_BIT		0x40000000
#define ETH_XLNX_GEM_NWCFG_BADPREAMBEN_BIT		0x20000000
#define ETH_XLNX_GEM_NWCFG_IPG_STRETCH_BIT		0x10000000
#define ETH_XLNX_GEM_NWCFG_SGMIIEN_BIT			0x08000000
#define ETH_XLNX_GEM_NWCFG_FCSIGNORE_BIT		0x04000000
#define ETH_XLNX_GEM_NWCFG_HDRXEN_BIT			0x02000000
#define ETH_XLNX_GEM_NWCFG_RXCHKSUMEN_BIT		0x01000000
#define ETH_XLNX_GEM_NWCFG_PAUSECOPYDI_BIT		0x00800000
#define ETH_XLNX_GEM_NWCFG_DBUSW_MASK			0x3
#define ETH_XLNX_GEM_NWCFG_DBUSW_SHIFT			21
#define ETH_XLNX_GEM_NWCFG_MDC_MASK			0x7
#define ETH_XLNX_GEM_NWCFG_MDC_SHIFT			18
#define ETH_XLNX_GEM_NWCFG_MDCCLKDIV_MASK		0x001C0000
#define ETH_XLNX_GEM_NWCFG_FCSREM_BIT			0x00020000
#define ETH_XLNX_GEM_NWCFG_LENGTHERRDSCRD_BIT		0x00010000
#define ETH_XLNX_GEM_NWCFG_RXOFFS_MASK			0x00000003
#define ETH_XLNX_GEM_NWCFG_RXOFFS_SHIFT			14
#define ETH_XLNX_GEM_NWCFG_PAUSEEN_BIT			0x00002000
#define ETH_XLNX_GEM_NWCFG_RETRYTESTEN_BIT		0x00001000
#define ETH_XLNX_GEM_NWCFG_TBIINSTEAD_BIT		0x00000800
#define ETH_XLNX_GEM_NWCFG_1000_BIT			0x00000400
#define ETH_XLNX_GEM_NWCFG_EXTADDRMATCHEN_BIT		0x00000200
#define ETH_XLNX_GEM_NWCFG_1536RXEN_BIT			0x00000100
#define ETH_XLNX_GEM_NWCFG_UCASTHASHEN_BIT		0x00000080
#define ETH_XLNX_GEM_NWCFG_MCASTHASHEN_BIT		0x00000040
#define ETH_XLNX_GEM_NWCFG_BCASTDIS_BIT			0x00000020
#define ETH_XLNX_GEM_NWCFG_COPYALLEN_BIT		0x00000010
#define ETH_XLNX_GEM_NWCFG_NVLANDISC_BIT		0x00000004
#define ETH_XLNX_GEM_NWCFG_FDEN_BIT			0x00000002
#define ETH_XLNX_GEM_NWCFG_100_BIT			0x00000001

/*
 * gem.dma_cfg:
 * [24]       Discard packets when AHB resource is unavailable
 * [23 .. 16] RX buffer size, n * 64 bytes
 * [11]       Enable/disable TCP|UDP/IP TX checksum offload
 * [10]       TX buffer half/full memory size
 * [09 .. 08] Receiver packet buffer memory size select
 * [07]       Endianness configuration
 * [06]       Descriptor access endianness configuration
 * [04 .. 00] AHB fixed burst length for DMA data operations
 */
#define ETH_XLNX_GEM_DMACR_DISCNOAHB_BIT		0x01000000
#define ETH_XLNX_GEM_DMACR_RX_BUF_MASK			0x000000FF
#define ETH_XLNX_GEM_DMACR_RX_BUF_SHIFT			16
#define ETH_XLNX_GEM_DMACR_TCP_CHKSUM_BIT		0x00000800
#define ETH_XLNX_GEM_DMACR_TX_SIZE_BIT			0x00000400
#define ETH_XLNX_GEM_DMACR_RX_SIZE_MASK			0x00000300
#define ETH_XLNX_GEM_DMACR_RX_SIZE_SHIFT		8
#define ETH_XLNX_GEM_DMACR_ENDIAN_BIT			0x00000080
#define ETH_XLNX_GEM_DMACR_DESCR_ENDIAN_BIT		0x00000040
#define ETH_XLNX_GEM_DMACR_AHB_BURST_LENGTH_MASK	0x0000001F

/*
 * gem.intr_* interrupt status/enable/disable bits:
 * [25]       PTP pdelay_resp frame transmitted
 * [24]       PTP pdelay_req frame transmitted
 * [23]       PTP pdelay_resp frame received
 * [22]       PTP delay_req frame received
 * [21]       PTP sync frame transmitted
 * [20]       PTP delay_req frame transmitted
 * [19]       PTP sync frame received
 * [18]       PTP delay_req frame received
 * [17]       PCS link partner page mask
 * [16]       Auto-negotiation completed
 * [15]       External interrupt
 * [14]       Pause frame transmitted
 * [13]       Pause time has reached zero
 * [12]       Pause frame received with non-zero pause quantum
 * [11]       hresp not OK
 * [10]       Receive overrun
 * [09]       Link change
 * [07]       Transmit complete
 * [06]       Transmit frame corruption due to AHB/AXI error
 * [05]       Retry limit exceeded or late collision
 * [04]       Transmit buffer underrun
 * [03]       Set 'used' bit in TX BD encountered
 * [02]       Set 'used' bit in RX BD encountered
 * [01]       Frame received
 * [00]       PHY management done
 */
#define ETH_XLNX_GEM_IXR_PTPPSTX_BIT			0x02000000
#define ETH_XLNX_GEM_IXR_PTPPDRTX_BIT			0x01000000
#define ETH_XLNX_GEM_IXR_PTPSTX_BIT			0x00800000
#define ETH_XLNX_GEM_IXR_PTPDRTX_BIT			0x00400000
#define ETH_XLNX_GEM_IXR_PTPPSRX_BIT			0x00200000
#define ETH_XLNX_GEM_IXR_PTPPDRRX_BIT			0x00100000
#define ETH_XLNX_GEM_IXR_PTPSRX_BIT			0x00080000
#define ETH_XLNX_GEM_IXR_PTPDRRX_BIT			0x00040000
#define ETH_XLNX_GEM_IXR_PARTNER_PGRX_BIT		0x00020000
#define ETH_XLNX_GEM_IXR_AUTONEG_COMPLETE_BIT		0x00010000
#define ETH_XLNX_GEM_IXR_EXTERNAL_INT_BIT		0x00008000
#define ETH_XLNX_GEM_IXR_PAUSE_TX_BIT			0x00004000
#define ETH_XLNX_GEM_IXR_PAUSE_ZERO_BIT			0x00002000
#define ETH_XLNX_GEM_IXR_PAUSE_NONZERO_BIT		0x00001000
#define ETH_XLNX_GEM_IXR_HRESP_NOT_OK_BIT		0x00000800
#define ETH_XLNX_GEM_IXR_RX_OVERRUN_BIT			0x00000400
#define ETH_XLNX_GEM_IXR_LINK_CHANGE                    0x00000200
#define ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT		0x00000080
#define ETH_XLNX_GEM_IXR_TX_CORRUPT_BIT			0x00000040
#define ETH_XLNX_GEM_IXR_RETRY_LIMIT_OR_LATE_COLL_BIT	0x00000020
#define ETH_XLNX_GEM_IXR_TX_UNDERRUN_BIT		0x00000010
#define ETH_XLNX_GEM_IXR_TX_USED_BIT			0x00000008
#define ETH_XLNX_GEM_IXR_RX_USED_BIT			0x00000004
#define ETH_XLNX_GEM_IXR_FRAME_RX_BIT			0x00000002
#define ETH_XLNX_GEM_IXR_PHY_MGMNT_BIT			0x00000001
#define ETH_XLNX_GEM_IXR_ALL_MASK			0x03FC7FFE
#define ETH_XLNX_GEM_IXR_ERRORS_MASK			0x00000C60

/* Bits / bit masks relating to the GEM's MDIO interface */

/*
 * gem.net_status:
 * [02]       PHY management idle bit
 * [01]       MDIO input status
 */
#define ETH_XLNX_GEM_MDIO_IDLE_BIT			0x00000004
#define ETH_XLNX_GEM_MDIO_IN_STATUS_BIT			0x00000002

/*
 * gem.phy_maint:
 * [31 .. 30] constant values
 * [17 .. 16] constant values
 * [29]       Read operation control bit
 * [28]       Write operation control bit
 * [27 .. 23] PHY address
 * [22 .. 18] Register address
 * [15 .. 00] 16-bit data word
 */
#define ETH_XLNX_GEM_PHY_MAINT_CONST_BITS		0x40020000
#define ETH_XLNX_GEM_PHY_MAINT_READ_OP_BIT		0x20000000
#define ETH_XLNX_GEM_PHY_MAINT_WRITE_OP_BIT		0x10000000
#define ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_MASK		0x0000001F
#define ETH_XLNX_GEM_PHY_MAINT_PHY_ADDRESS_SHIFT	23
#define ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_MASK		0x0000001F
#define ETH_XLNX_GEM_PHY_MAINT_REGISTER_ID_SHIFT	18
#define ETH_XLNX_GEM_PHY_MAINT_DATA_MASK		0x0000FFFF

/* Device initialization macro */
#define ETH_XLNX_GEM_NET_DEV_INIT(port) \
ETH_NET_DEVICE_DT_INST_DEFINE(port,\
	eth_xlnx_gem_dev_init,\
	NULL,\
	&eth_xlnx_gem##port##_dev_data,\
	&eth_xlnx_gem##port##_dev_cfg,\
	CONFIG_ETH_INIT_PRIORITY,\
	&eth_xlnx_gem_apis,\
	NET_ETH_MTU);

/* Device configuration data declaration macro */
#define ETH_XLNX_GEM_DEV_CONFIG(port) \
static const struct eth_xlnx_gem_dev_cfg eth_xlnx_gem##port##_dev_cfg = {\
	.base_addr			= DT_REG_ADDR_BY_IDX(DT_INST(port, xlnx_gem), 0),\
	.config_func			= eth_xlnx_gem##port##_irq_config,\
	.pll_clock_frequency		= DT_INST_PROP(port, clock_frequency),\
	.clk_ctrl_reg_address		= DT_REG_ADDR_BY_IDX(DT_INST(port, xlnx_gem), 1),\
	.mdc_divider			= (enum eth_xlnx_mdc_clock_divider)\
		(DT_INST_PROP(port, mdc_divider)),\
	.max_link_speed			= (enum eth_xlnx_link_speed)\
		(DT_INST_PROP(port, link_speed)),\
	.init_phy			= DT_INST_PROP(port, init_mdio_phy),\
	.phy_mdio_addr_fix		= DT_INST_PROP(port, mdio_phy_address),\
	.phy_advertise_lower		= DT_INST_PROP(port, advertise_lower_link_speeds),\
	.phy_poll_interval		= DT_INST_PROP(port, phy_poll_interval),\
	.defer_rxp_to_queue		= !DT_INST_PROP(port, handle_rx_in_isr),\
	.defer_txd_to_queue		= DT_INST_PROP(port, handle_tx_in_workq),\
	.amba_dbus_width		= (enum eth_xlnx_amba_dbus_width)\
		(DT_INST_PROP(port, amba_ahb_dbus_width)),\
	.ahb_burst_length		= (enum eth_xlnx_ahb_burst_length)\
		(DT_INST_PROP(port, amba_ahb_burst_length)),\
	.hw_rx_buffer_size		= (enum eth_xlnx_hwrx_buffer_size)\
		(DT_INST_PROP(port, hw_rx_buffer_size)),\
	.hw_rx_buffer_offset		= (uint8_t)\
		(DT_INST_PROP(port, hw_rx_buffer_offset)),\
	.rxbd_count			= (uint8_t)\
		(DT_INST_PROP(port, rx_buffer_descriptors)),\
	.txbd_count			= (uint8_t)\
		(DT_INST_PROP(port, tx_buffer_descriptors)),\
	.rx_buffer_size			= (((uint16_t)(DT_INST_PROP(port, rx_buffer_size)) +\
		(ETH_XLNX_BUFFER_ALIGNMENT-1)) & ~(ETH_XLNX_BUFFER_ALIGNMENT-1)),\
	.tx_buffer_size			= (((uint16_t)(DT_INST_PROP(port, tx_buffer_size)) +\
		(ETH_XLNX_BUFFER_ALIGNMENT-1)) & ~(ETH_XLNX_BUFFER_ALIGNMENT-1)),\
	.ignore_ipg_rxer		= DT_INST_PROP(port, ignore_ipg_rxer),\
	.disable_reject_nsp		= DT_INST_PROP(port, disable_reject_nsp),\
	.enable_ipg_stretch		= DT_INST_PROP(port, ipg_stretch),\
	.enable_sgmii_mode		= DT_INST_PROP(port, sgmii_mode),\
	.disable_reject_fcs_crc_errors	= DT_INST_PROP(port, disable_reject_fcs_crc_errors),\
	.enable_rx_halfdup_while_tx	= DT_INST_PROP(port, rx_halfdup_while_tx),\
	.enable_rx_chksum_offload	= DT_INST_PROP(port, rx_checksum_offload),\
	.disable_pause_copy		= DT_INST_PROP(port, disable_pause_copy),\
	.discard_rx_fcs			= DT_INST_PROP(port, discard_rx_fcs),\
	.discard_rx_length_errors	= DT_INST_PROP(port, discard_rx_length_errors),\
	.enable_pause			= DT_INST_PROP(port, pause_frame),\
	.enable_tbi			= DT_INST_PROP(port, tbi),\
	.ext_addr_match			= DT_INST_PROP(port, ext_address_match),\
	.enable_1536_frames		= DT_INST_PROP(port, long_frame_rx_support),\
	.enable_ucast_hash		= DT_INST_PROP(port, unicast_hash),\
	.enable_mcast_hash		= DT_INST_PROP(port, multicast_hash),\
	.disable_bcast			= DT_INST_PROP(port, reject_broadcast),\
	.copy_all_frames		= DT_INST_PROP(port, promiscuous_mode),\
	.discard_non_vlan		= DT_INST_PROP(port, discard_non_vlan),\
	.enable_fdx			= DT_INST_PROP(port, full_duplex),\
	.disc_rx_ahb_unavail		= DT_INST_PROP(port, discard_rx_frame_ahb_unavail),\
	.enable_tx_chksum_offload	= DT_INST_PROP(port, tx_checksum_offload),\
	.tx_buffer_size_full		= DT_INST_PROP(port, hw_tx_buffer_size_full),\
	.enable_ahb_packet_endian_swap	= DT_INST_PROP(port, ahb_packet_endian_swap),\
	.enable_ahb_md_endian_swap	= DT_INST_PROP(port, ahb_md_endian_swap)\
};

/* Device run-time data declaration macro */
#define ETH_XLNX_GEM_DEV_DATA(port) \
static struct eth_xlnx_gem_dev_data eth_xlnx_gem##port##_dev_data = {\
	.mac_addr        = DT_INST_PROP(port, local_mac_address),\
	.started         = 0,\
	.eff_link_speed  = LINK_DOWN,\
	.phy_addr        = 0,\
	.phy_id          = 0,\
	.phy_access_api  = NULL,\
	.first_rx_buffer = NULL,\
	.first_tx_buffer = NULL\
};

/* DMA memory area declaration macro */
#define ETH_XLNX_GEM_DMA_AREA_DECL(port) \
struct eth_xlnx_dma_area_gem##port {\
	struct eth_xlnx_gem_bd rx_bd[DT_INST_PROP(port, rx_buffer_descriptors)];\
	struct eth_xlnx_gem_bd tx_bd[DT_INST_PROP(port, tx_buffer_descriptors)];\
	uint8_t rx_buffer\
		[DT_INST_PROP(port, rx_buffer_descriptors)]\
		[((DT_INST_PROP(port, rx_buffer_size)\
		+ (ETH_XLNX_BUFFER_ALIGNMENT - 1))\
		& ~(ETH_XLNX_BUFFER_ALIGNMENT - 1))];\
	uint8_t tx_buffer\
		[DT_INST_PROP(port, tx_buffer_descriptors)]\
		[((DT_INST_PROP(port, tx_buffer_size)\
		+ (ETH_XLNX_BUFFER_ALIGNMENT - 1))\
		& ~(ETH_XLNX_BUFFER_ALIGNMENT - 1))];\
};

/* DMA memory area instantiation macro */
#define ETH_XLNX_GEM_DMA_AREA_INST(port) \
static struct eth_xlnx_dma_area_gem##port eth_xlnx_gem##port##_dma_area\
	__ocm_bss_section __aligned(4096);

/* Interrupt configuration function macro */
#define ETH_XLNX_GEM_CONFIG_IRQ_FUNC(port) \
static void eth_xlnx_gem##port##_irq_config(const struct device *dev)\
{\
	ARG_UNUSED(dev);\
	IRQ_CONNECT(DT_INST_IRQN(port), DT_INST_IRQ(port, priority),\
	eth_xlnx_gem_isr, DEVICE_DT_INST_GET(port), 0);\
	irq_enable(DT_INST_IRQN(port));\
}

/* RX/TX BD Ring initialization macro */
#define ETH_XLNX_GEM_INIT_BD_RING(port) \
if (dev_conf->base_addr == DT_REG_ADDR_BY_IDX(DT_INST(port, xlnx_gem), 0)) {\
	dev_data->rxbd_ring.first_bd = &(eth_xlnx_gem##port##_dma_area.rx_bd[0]);\
	dev_data->txbd_ring.first_bd = &(eth_xlnx_gem##port##_dma_area.tx_bd[0]);\
	dev_data->first_rx_buffer = (uint8_t *)eth_xlnx_gem##port##_dma_area.rx_buffer;\
	dev_data->first_tx_buffer = (uint8_t *)eth_xlnx_gem##port##_dma_area.tx_buffer;\
}

/* Top-level device initialization macro - bundles all of the above */
#define ETH_XLNX_GEM_INITIALIZE(port) \
ETH_XLNX_GEM_CONFIG_IRQ_FUNC(port);\
ETH_XLNX_GEM_DEV_CONFIG(port);\
ETH_XLNX_GEM_DEV_DATA(port);\
ETH_XLNX_GEM_DMA_AREA_DECL(port);\
ETH_XLNX_GEM_DMA_AREA_INST(port);\
ETH_XLNX_GEM_NET_DEV_INIT(port);\

/* IRQ handler function type */
typedef void (*eth_xlnx_gem_config_irq_t)(const struct device *dev);

/* Enums for bitfields representing configuration settings */

/**
 * @brief Link speed configuration enumeration type.
 *
 * Enumeration type for link speed indication, contains 'link down'
 * plus all link speeds supported by the controller (10/100/1000).
 */
enum eth_xlnx_link_speed {
	/* The values of this enum are consecutively numbered */
	LINK_DOWN = 0,
	LINK_10MBIT,
	LINK_100MBIT,
	LINK_1GBIT
};

/**
 * @brief AMBA AHB data bus width configuration enumeration type.
 *
 * Enumeration type containing the supported width options for the
 * AMBA AHB data bus. This is a configuration item in the controller's
 * net_cfg register.
 */
enum eth_xlnx_amba_dbus_width {
	/* The values of this enum are consecutively numbered */
	AMBA_AHB_DBUS_WIDTH_32BIT = 0,
	AMBA_AHB_DBUS_WIDTH_64BIT,
	AMBA_AHB_DBUS_WIDTH_128BIT
};

/**
 * @brief MDC clock divider configuration enumeration type.
 *
 * Enumeration type containing the supported clock divider values
 * used to generate the MDIO interface clock (MDC) from either the
 * cpu_1x clock (Zynq-7000) or the LPD LSBUS clock (UltraScale).
 * This is a configuration item in the controller's net_cfg register.
 */
enum eth_xlnx_mdc_clock_divider {
	/* The values of this enum are consecutively numbered */
	MDC_DIVIDER_8 = 0,
	MDC_DIVIDER_16,
	MDC_DIVIDER_32,
	MDC_DIVIDER_48,
#ifdef CONFIG_SOC_FAMILY_XILINX_ZYNQ7000
	/* Dividers > 48 are only available in the Zynq-7000 */
	MDC_DIVIDER_64,
	MDC_DIVIDER_96,
	MDC_DIVIDER_128,
	MDC_DIVIDER_224
#endif
};

/**
 * @brief DMA RX buffer size configuration enumeration type.
 *
 * Enumeration type containing the supported size options for the
 * DMA receive buffer size in AHB system memory. This is a configuration
 * item in the controller's dma_cfg register.
 */
enum eth_xlnx_hwrx_buffer_size {
	/* The values of this enum are consecutively numbered */
	HWRX_BUFFER_SIZE_1KB = 0,
	HWRX_BUFFER_SIZE_2KB,
	HWRX_BUFFER_SIZE_4KB,
	HWRX_BUFFER_SIZE_8KB
};

/**
 * @brief AHB burst length configuration enumeration type.
 *
 * Enumeration type containing the supported burst length options
 * for the AHB fixed burst length for DMA data operations. This is a
 * configuration item in the controller's dma_cfg register.
 */
enum eth_xlnx_ahb_burst_length {
	/* The values of this enum are one-hot encoded */
	AHB_BURST_SINGLE = 1,
	/* 2 = also AHB_BURST_SINGLE */
	AHB_BURST_INCR4  = 4,
	AHB_BURST_INCR8  = 8,
	AHB_BURST_INCR16 = 16
};

/**
 * @brief DMA memory area buffer descriptor.
 *
 * An array of these descriptors for each RX and TX is used to
 * describe the respective DMA memory area. Each address word
 * points to the start of a RX or TX buffer within the DMA memory
 * area, while the control word is used for buffer status exchange
 * with the controller.
 */
struct eth_xlnx_gem_bd {
	/* TODO for Cortex-A53: 64-bit addressing */
	/* TODO: timestamping support */
	/* Buffer physical address (absolute address) */
	uint32_t		addr;
	/* Buffer control word (different contents for RX and TX) */
	uint32_t		ctrl;
};

/**
 * @brief DMA memory area buffer descriptor ring management structure.
 *
 * The DMA memory area buffer descriptor ring management structure
 * is used to manage either the RX or TX buffer descriptor array
 * (while the buffer descriptors are just an array from the software
 * point of view, the controller treats them as a ring, in which the
 * last descriptor's control word has a special last-in-ring bit set).
 * It contains a pointer to the start of the descriptor array, a
 * semaphore as a means of preventing concurrent access, a free entry
 * counter as well as indices used to determine which BD shall be used
 * or evaluated for the next RX/TX operation.
 */
struct eth_xlnx_gem_bdring {
	/* Concurrent modification protection */
	struct k_sem		ring_sem;
	/* Pointer to the first BD in the list */
	struct eth_xlnx_gem_bd	*first_bd;
	/* Index of the next BD to be used for TX */
	uint8_t			next_to_use;
	/* Index of the next BD to be processed (both RX/TX) */
	uint8_t			next_to_process;
	/* Number of currently available BDs in this ring */
	uint8_t			free_bds;
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all device configuration data for a GEM
 * controller instance which is constant. The data herein is
 * either acquired from the generated header file based on the
 * data from Kconfig, or from header file based on the device tree
 * data. Some of the data contained, in particular data relating
 * to clock sources, is specific to either the Zynq-7000 or the
 * UltraScale SoCs, which both contain the GEM.
 */
struct eth_xlnx_gem_dev_cfg {
	uint32_t			base_addr;
	eth_xlnx_gem_config_irq_t	config_func;

	uint32_t			pll_clock_frequency;
	uint32_t			clk_ctrl_reg_address;
	enum eth_xlnx_mdc_clock_divider	mdc_divider;

	enum eth_xlnx_link_speed	max_link_speed;
	bool				init_phy;
	uint8_t				phy_mdio_addr_fix;
	uint8_t				phy_advertise_lower;
	uint32_t			phy_poll_interval;
	uint8_t				defer_rxp_to_queue;
	uint8_t				defer_txd_to_queue;

	enum eth_xlnx_amba_dbus_width	amba_dbus_width;
	enum eth_xlnx_ahb_burst_length	ahb_burst_length;
	enum eth_xlnx_hwrx_buffer_size	hw_rx_buffer_size;
	uint8_t				hw_rx_buffer_offset;

	uint8_t				rxbd_count;
	uint8_t				txbd_count;
	uint16_t			rx_buffer_size;
	uint16_t			tx_buffer_size;

	bool				ignore_ipg_rxer : 1;
	bool				disable_reject_nsp : 1;
	bool				enable_ipg_stretch : 1;
	bool				enable_sgmii_mode : 1;
	bool				disable_reject_fcs_crc_errors : 1;
	bool				enable_rx_halfdup_while_tx : 1;
	bool				enable_rx_chksum_offload : 1;
	bool				disable_pause_copy : 1;
	bool				discard_rx_fcs : 1;
	bool				discard_rx_length_errors : 1;
	bool				enable_pause : 1;
	bool				enable_tbi : 1;
	bool				ext_addr_match : 1;
	bool				enable_1536_frames : 1;
	bool				enable_ucast_hash : 1;
	bool				enable_mcast_hash : 1;
	bool				disable_bcast : 1;
	bool				copy_all_frames : 1;
	bool				discard_non_vlan : 1;
	bool				enable_fdx : 1;
	bool				disc_rx_ahb_unavail : 1;
	bool				enable_tx_chksum_offload : 1;
	bool				tx_buffer_size_full : 1;
	bool				enable_ahb_packet_endian_swap : 1;
	bool				enable_ahb_md_endian_swap : 1;
};

/**
 * @brief Run-time device configuration data structure.
 *
 * This struct contains all device configuration data for a GEM
 * controller instance which is modifyable at run-time, such as
 * data relating to the attached PHY or the auxiliary thread.
 */
struct eth_xlnx_gem_dev_data {
	struct net_if			*iface;
	uint8_t				mac_addr[6];
	enum eth_xlnx_link_speed	eff_link_speed;

	struct k_work			tx_done_work;
	struct k_work			rx_pend_work;
	struct k_sem			tx_done_sem;

	uint8_t				phy_addr;
	uint32_t			phy_id;
	struct k_work_delayable		phy_poll_delayed_work;
	struct phy_xlnx_gem_api		*phy_access_api;

	uint8_t				*first_rx_buffer;
	uint8_t				*first_tx_buffer;

	struct eth_xlnx_gem_bdring	rxbd_ring;
	struct eth_xlnx_gem_bdring	txbd_ring;

#ifdef CONFIG_NET_STATISTICS_ETHERNET
	struct net_stats_eth		stats;
#endif

	bool				started;
};

#endif /* _ZEPHYR_DRIVERS_ETHERNET_ETH_XLNX_GEM_PRIV_H_ */
