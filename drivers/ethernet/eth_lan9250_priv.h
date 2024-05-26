/* LAN9250 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2024 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#ifndef _LAN9250_
#define _LAN9250_

#define LAN9250_DEFAULT_NUMOF_RETRIES 3U
#define LAN9250_PHY_TIMEOUT   2000
#define LAN9250_MAC_TIMEOUT   2000
#define LAN9250_RESET_TIMEOUT 5000

#define LAN9250_ALIGN(v) (((v) + 3) & (~3))

 /* SPI instructions */
#define LAN9250_SPI_INSTR_WRITE    0x02
#define LAN9250_SPI_INSTR_READ     0x03

/* TX command 'A' format */
#define LAN9250_TX_CMD_A_INT_ON_COMP      0x80000000
#define LAN9250_TX_CMD_A_BUFFER_ALIGN_4B  0x00000000
#define LAN9250_TX_CMD_A_START_OFFSET_0B  0x00000000
#define LAN9250_TX_CMD_A_FIRST_SEG        0x00002000
#define LAN9250_TX_CMD_A_LAST_SEG         0x00001000

/* TX command 'B' format */
#define LAN9250_TX_CMD_B_PACKET_TAG     0xFFFF0000

/* RX status format */
#define LAN9250_RX_STS_PACKET_LEN      0x3FFF0000

/* LAN9250 System registers */
#define LAN9250_RX_DATA_FIFO                   0x0000
#define LAN9250_TX_DATA_FIFO                   0x0020
#define LAN9250_RX_STATUS_FIFO                 0x0040
#define LAN9250_TX_STATUS_FIFO                 0x0048
#define LAN9250_IRQ_CFG                        0x0054
#define LAN9250_INT_STS                        0x0058
#define LAN9250_INT_EN                         0x005C
#define LAN9250_BYTE_TEST                      0x0064
#define LAN9250_FIFO_INT                       0x0068
#define LAN9250_RX_CFG                         0x006C
#define LAN9250_TX_CFG                         0x0070
#define LAN9250_HW_CFG                         0x0074
#define LAN9250_RX_FIFO_INF                    0x007C
#define LAN9250_TX_FIFO_INF                    0x0080
#define LAN9250_PMT_CTRL                       0x0084
#define LAN9250_MAC_CSR_CMD                    0x00A4
#define LAN9250_MAC_CSR_DATA                   0x00A8
#define LAN9250_AFC_CFG                        0x00AC
#define LAN9250_RESET_CTL                      0x01F8

/* LAN9250 Host MAC registers */
#define LAN9250_HMAC_CR                   0x01
#define LAN9250_HMAC_ADDRH                0x02
#define LAN9250_HMAC_ADDRL                0x03
#define LAN9250_HMAC_MII_ACC              0x06
#define LAN9250_HMAC_MII_DATA             0x07
#define LAN9250_HMAC_FLOW                 0x08

/* LAN9250 PHY registers */
#define LAN9250_PHY_BASIC_CONTROL            0x00
#define LAN9250_PHY_AN_ADV                   0x04
#define LAN9250_PHY_SPECIAL_MODES            0x12
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND 0x1B
#define LAN9250_PHY_INTERRUPT_SOURCE         0x1D
#define LAN9250_PHY_INTERRUPT_MASK           0x1E
#define LAN9250_PHY_SPECIAL_CONTROL_STATUS   0x1F

/* Interrupt Configuration register */
#define LAN9250_IRQ_CFG_INT_DEAS_100US 0x0A000000
#define LAN9250_IRQ_CFG_IRQ_EN         0x00000100
#define LAN9250_IRQ_CFG_IRQ_TYPE_PP    0x00000001

/* Interrupt Status register */
#define LAN9250_INT_STS_PHY_INT    0x04000000
#define LAN9250_INT_STS_RSFL       0x00000008

/* Interrupt Enable register */
#define LAN9250_INT_EN_PHY_INT_EN    0x04000000
#define LAN9250_INT_EN_TDFA_EN       0x00000200
#define LAN9250_INT_EN_RSFL_EN       0x00000008

/* Byte Order Test register */
#define LAN9250_BYTE_TEST_DEFAULT 0x87654321
#define BOTR_MASK                 0xffffffff

/* FIFO Level Interrupt register */
#define LAN9250_FIFO_INT_TX_DATA_AVAILABLE_LEVEL 0xFF000000
#define LAN9250_FIFO_INT_TX_STATUS_LEVEL         0x00FF0000
#define LAN9250_FIFO_INT_RX_STATUS_LEVEL         0x000000FF

/* Transmit Configuration register*/
#define LAN9250_TX_CFG_TXS_DUMP 0x00008000
#define LAN9250_TX_CFG_TXD_DUMP 0x00004000
#define LAN9250_TX_CFG_TXSAO    0x00000004
#define LAN9250_TX_CFG_TX_ON    0x00000002
#define LAN9250_TX_CFG_STOP_TX  0x00000001

/* Hardware Configuration register */
#define LAN9250_HW_CFG_DEVICE_READY         0x08000000
#define LAN9250_HW_CFG_AMDIX_EN_STRAP_STATE 0x02000000
#define LAN9250_HW_CFG_MBO                  0x00100000
#define LAN9250_HW_CFG_TX_FIF_SZ            0x000F0000
#define LAN9250_HW_CFG_TX_FIF_SZ_2KB        0x00020000
#define LAN9250_HW_CFG_TX_FIF_SZ_3KB        0x00030000
#define LAN9250_HW_CFG_TX_FIF_SZ_4KB        0x00040000
#define LAN9250_HW_CFG_TX_FIF_SZ_5KB        0x00050000
#define LAN9250_HW_CFG_TX_FIF_SZ_6KB        0x00060000
#define LAN9250_HW_CFG_TX_FIF_SZ_7KB        0x00070000
#define LAN9250_HW_CFG_TX_FIF_SZ_8KB        0x00080000
#define LAN9250_HW_CFG_TX_FIF_SZ_9KB        0x00090000
#define LAN9250_HW_CFG_TX_FIF_SZ_10KB       0x000A0000
#define LAN9250_HW_CFG_TX_FIF_SZ_11KB       0x000B0000
#define LAN9250_HW_CFG_TX_FIF_SZ_12KB       0x000C0000
#define LAN9250_HW_CFG_TX_FIF_SZ_13KB       0x000D0000
#define LAN9250_HW_CFG_TX_FIF_SZ_14KB       0x000E0000

/* RX FIFO Information register */
#define LAN9250_RX_FIFO_INF_RXSUSED 0x00FF0000
#define LAN9250_RX_FIFO_INF_RXDUSED 0x0000FFFF

/* TX FIFO Information register */
#define LAN9250_TX_FIFO_INF_TXSUSED 0x00FF0000
#define LAN9250_TX_FIFO_INF_TXFREE  0x0000FFFF

/* Power Management Control register */
#define LAN9250_PMT_CTRL_PM_MODE           0xE0000000
#define LAN9250_PMT_CTRL_PM_SLEEP_EN       0x10000000
#define LAN9250_PMT_CTRL_PM_WAKE           0x08000000
#define LAN9250_PMT_CTRL_LED_DIS           0x04000000
#define LAN9250_PMT_CTRL_1588_DIS          0x02000000
#define LAN9250_PMT_CTRL_1588_TSU_DIS      0x00400000
#define LAN9250_PMT_CTRL_HMAC_DIS          0x00080000
#define LAN9250_PMT_CTRL_HMAC_SYS_ONLY_DIS 0x00040000
#define LAN9250_PMT_CTRL_ED_STS            0x00010000
#define LAN9250_PMT_CTRL_ED_EN             0x00004000
#define LAN9250_PMT_CTRL_WOL_EN            0x00000200
#define LAN9250_PMT_CTRL_PME_TYPE          0x00000040
#define LAN9250_PMT_CTRL_WOL_STS           0x00000020
#define LAN9250_PMT_CTRL_PME_IND           0x00000008
#define LAN9250_PMT_CTRL_PME_POL           0x00000004
#define LAN9250_PMT_CTRL_PME_EN            0x00000002
#define LAN9250_PMT_CTRL_READY             0x00000001

/* General Purpose Timer Configuration register */
#define LAN9250_GPT_CFG_TIMER_EN 0x20000000
#define LAN9250_GPT_CFG_GPT_LOAD 0x0000FFFF

/* General Purpose Timer Count register */
#define LAN9250_GPT_CNT_GPT_CNT 0x0000FFFF

/* Free Running 25MHz Counter register */
#define LAN9250_FREE_RUN_FR_CNT 0xFFFFFFFF

/* Host MAC RX Dropped Frames Counter register */
#define LAN9250_RX_DROP_RX_DFC 0xFFFFFFFF

/* Host MAC CSR Interface Command register */
#define LAN9250_MAC_CSR_CMD_BUSY  0x80000000
#define LAN9250_MAC_CSR_CMD_WRITE 0x00000000
#define LAN9250_MAC_CSR_CMD_READ  0x40000000
#define LAN9250_MAC_CSR_CMD_ADDR  0x000000FF

/* Host MAC Automatic Flow Control Configuration register */
#define LAN9250_AFC_CFG_AFC_HI   0x00FF0000
#define HMAFCCFGR_AFCHL_S        16
#define LAN9250_AFC_CFG_AFC_LO   0x0000FF00
#define LAN9250_AFC_CFG_BACK_DUR 0x000000F0
#define LAN9250_AFC_CFG_FCMULT   0x00000008
#define LAN9250_AFC_CFG_FCBRD    0x00000004
#define LAN9250_AFC_CFG_FCADD    0x00000002
#define LAN9250_AFC_CFG_FCANY    0x00000001

/* Reset Control register */
#define LAN9250_RESET_CTL_HMAC_RST    0x00000020
#define LAN9250_RESET_CTL_PHY_RST     0x00000002
#define LAN9250_RESET_CTL_DIGITAL_RST 0x00000001

/* Host MAC Control register */
#define LAN9250_HMAC_CR_RXALL           0x80000000
#define LAN9250_HMAC_CR_HMAC_EEE_ENABLE 0x02000000
#define LAN9250_HMAC_CR_RCVOWN          0x00800000
#define LAN9250_HMAC_CR_LOOPBK          0x00200000
#define LAN9250_HMAC_CR_FDPX            0x00100000
#define LAN9250_HMAC_CR_MCPAS           0x00080000
#define LAN9250_HMAC_CR_PRMS            0x00040000
#define LAN9250_HMAC_CR_INVFILT         0x00020000
#define LAN9250_HMAC_CR_PASSBAD         0x00010000
#define LAN9250_HMAC_CR_HO              0x00008000
#define LAN9250_HMAC_CR_HPFILT          0x00002000
#define LAN9250_HMAC_CR_BCAST           0x00000800
#define LAN9250_HMAC_CR_DISRTY          0x00000400
#define LAN9250_HMAC_CR_PADSTR          0x00000100
#define LAN9250_HMAC_CR_BOLMT           0x000000C0
#define LAN9250_HMAC_CR_BOLMT_10_BITS   0x00000000
#define LAN9250_HMAC_CR_BOLMT_8_BITS    0x00000040
#define LAN9250_HMAC_CR_BOLMT_4_BITS    0x00000080
#define LAN9250_HMAC_CR_BOLMT_1_BIT     0x000000C0
#define LAN9250_HMAC_CR_DFCHK           0x00000020
#define LAN9250_HMAC_CR_TXEN            0x00000008
#define LAN9250_HMAC_CR_RXEN            0x00000004

/* Host MAC Address High register */
#define LAN9250_HMAC_ADDRH_PHY_ADR_47_32 0x0000FFFF

/* Host MAC Address Low register */
#define LAN9250_HMAC_ADDRL_PHY_ADR_31_0 0xFFFFFFFF

/* Host MAC MII Access register */
#define LAN9250_HMAC_MII_ACC_PHY_ADDR         0x0000F800
#define LAN9250_HMAC_MII_ACC_PHY_ADDR_DEFAULT 0x00000800
#define LAN9250_HMAC_MII_ACC_MIIRINDA         0x000007C0
#define HMACMIIAR_MIIRX_S                     6
#define LAN9250_HMAC_MII_ACC_MIIW_R           0x00000002
#define LAN9250_HMAC_MII_ACC_MIIBZY           0x00000001

/* Host MAC MII Data register */
#define LAN9250_HMAC_MII_DATA_MII_DATA 0x0000FFFF

/* Host MAC Flow Control register */
#define LAN9250_HMAC_FLOW_FCPT   0xFFFF0000
#define HMACFCR_PT_S             16
#define LAN9250_HMAC_FLOW_FCPASS 0x00000004
#define LAN9250_HMAC_FLOW_FCEN   0x00000002
#define LAN9250_HMAC_FLOW_FCBSY  0x00000001

/* PHY Basic Control register */
#define LAN9250_PHY_BASIC_CONTROL_PHY_SRST          0x8000
#define LAN9250_PHY_BASIC_CONTROL_PHY_LOOPBACK      0x4000
#define LAN9250_PHY_BASIC_CONTROL_PHY_SPEED_SEL_LSB 0x2000
#define LAN9250_PHY_BASIC_CONTROL_PHY_AN            0x1000
#define LAN9250_PHY_BASIC_CONTROL_PHY_PWR_DWN       0x0800
#define LAN9250_PHY_BASIC_CONTROL_PHY_RST_AN        0x0200
#define LAN9250_PHY_BASIC_CONTROL_PHY_DUPLEX        0x0100
#define LAN9250_PHY_BASIC_CONTROL_PHY_COL_TEST      0x0080


/* PHY Auto-Negotiation Advertisement register */
#define LAN9250_PHY_AN_ADV_NEXT_PAGE          0x8000
#define LAN9250_PHY_AN_ADV_REMOTE_FAULT       0x2000
#define LAN9250_PHY_AN_ADV_EXTENDED_NEXT_PAGE 0x1000
#define LAN9250_PHY_AN_ADV_ASYM_PAUSE         0x0800
#define LAN9250_PHY_AN_ADV_SYM_PAUSE          0x0400
#define LAN9250_PHY_AN_ADV_100BTX_FD          0x0100
#define LAN9250_PHY_AN_ADV_100BTX_HD          0x0080
#define LAN9250_PHY_AN_ADV_10BT_FD            0x0040
#define LAN9250_PHY_AN_ADV_10BT_HD            0x0020
#define LAN9250_PHY_AN_ADV_SELECTOR           0x001F
#define LAN9250_PHY_AN_ADV_SELECTOR_DEFAULT   0x0001

/* PHY Mode Control/Status register */
#define LAN9250_PHY_MODE_CONTROL_STATUS_ALTINT    0x0040


/* PHY Special Modes register */
#define LAN9250_PHY_SPECIAL_MODES_FX_MODE         0x0400
#define LAN9250_PHY_SPECIAL_MODES_MODE            0x00E0
#define LAN9250_PHY_SPECIAL_MODES_MODE_10BT_HD    0x0000
#define LAN9250_PHY_SPECIAL_MODES_MODE_10BT_FD    0x0020
#define LAN9250_PHY_SPECIAL_MODES_MODE_100BTX_HD  0x0040
#define LAN9250_PHY_SPECIAL_MODES_MODE_100BTX_FD  0x0060
#define LAN9250_PHY_SPECIAL_MODES_MODE_POWER_DOWN 0x00C0
#define LAN9250_PHY_SPECIAL_MODES_MODE_AN         0x00E0
#define LAN9250_PHY_SPECIAL_MODES_PHYADD          0x001F


/* PHY Special Control/Status Indication register */
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_AMDIXCTRL  0x8000
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_AMDIXEN    0x4000
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_AMDIXSTATE 0x2000
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_SQEOFF     0x0800
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_FEFI_EN    0x0020
#define LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_XPOL       0x0010

/* PHY Interrupt Source Flags register */
#define LAN9250_PHY_INTERRUPT_SOURCE_LINK_UP               0x0200
#define LAN9250_PHY_INTERRUPT_SOURCE_ENERGYON              0x0080
#define LAN9250_PHY_INTERRUPT_SOURCE_AN_COMPLETE           0x0040
#define LAN9250_PHY_INTERRUPT_SOURCE_REMOTE_FAULT          0x0020
#define LAN9250_PHY_INTERRUPT_SOURCE_LINK_DOWN             0x0010
#define LAN9250_PHY_INTERRUPT_SOURCE_AN_LP_ACK             0x0008
#define LAN9250_PHY_INTERRUPT_SOURCE_PARALLEL_DETECT_FAULT 0x0004
#define LAN9250_PHY_INTERRUPT_SOURCE_AN_PAGE_RECEIVED      0x0002

/* PHY Interrupt Mask register */
#define LAN9250_PHY_INTERRUPT_MASK_LINK_UP               0x0200
#define LAN9250_PHY_INTERRUPT_MASK_ENERGYON              0x0080
#define LAN9250_PHY_INTERRUPT_MASK_AN_COMPLETE           0x0040
#define LAN9250_PHY_INTERRUPT_MASK_REMOTE_FAULT          0x0020
#define LAN9250_PHY_INTERRUPT_MASK_LINK_DOWN             0x0010
#define LAN9250_PHY_INTERRUPT_MASK_AN_LP_ACK             0x0008
#define LAN9250_PHY_INTERRUPT_MASK_PARALLEL_DETECT_FAULT 0x0004
#define LAN9250_PHY_INTERRUPT_MASK_AN_PAGE_RECEIVED      0x0002

struct lan9250_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	uint8_t full_duplex;
	int32_t timeout;
};

struct lan9250_runtime {
	struct net_if *iface;
	const struct device *dev;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_LAN9250_RX_THREAD_STACK_SIZE);

	struct k_thread thread;
	uint8_t mac_address[6];
	struct gpio_callback gpio_cb;
	struct k_sem tx_rx_sem;
	struct k_sem int_sem;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];
};

#endif /*_LAN9250_*/
