/* ieee802154_rf2xx_regs.h - ATMEL RF2XX transceiver registers */

/*
 * Copyright (c) 2019 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_REGS_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_REGS_H_

/*- Definitions ------------------------------------------------------------*/
#define RF2XX_AES_BLOCK_SIZE                16
#define RF2XX_AES_CORE_CYCLE_TIME           24          /* us */
#define RF2XX_RANDOM_NUMBER_UPDATE_INTERVAL 1           /* us */
#define RX2XX_FRAME_HEADER_SIZE             2
#define RX2XX_FRAME_FOOTER_SIZE             3
#define RX2XX_FRAME_FCS_LENGTH              2
#define RX2XX_FRAME_MIN_PHR_SIZE            5
#define RX2XX_FRAME_PHR_INDEX               1
#define RX2XX_FRAME_LQI_INDEX               2
#define RX2XX_FRAME_ED_INDEX                3
#define RX2XX_FRAME_TRAC_INDEX              4
#define RF2XX_MAX_PSDU_LENGTH               127
#define RX2XX_MAX_FRAME_SIZE                132

#define RF2XX_RSSI_BPSK_20                  -100
#define RF2XX_RSSI_BPSK_40                  -99
#define RF2XX_RSSI_OQPSK_SIN_RC_100         -98
#define RF2XX_RSSI_OQPSK_SIN_250            -97
#define RF2XX_RSSI_OQPSK_RC_250             -97

/*- Types ------------------------------------------------------------------*/
#define RF2XX_TRX_STATUS_REG                0x01
#define RF2XX_TRX_STATE_REG                 0x02
#define RF2XX_TRX_CTRL_0_REG                0x03
#define RF2XX_TRX_CTRL_1_REG                0x04
#define RF2XX_PHY_TX_PWR_REG                0x05
#define RF2XX_PHY_RSSI_REG                  0x06
#define RF2XX_PHY_ED_LEVEL_REG              0x07
#define RF2XX_PHY_CC_CCA_REG                0x08
#define RF2XX_CCA_THRES_REG                 0x09
#define RF2XX_RX_CTRL_REG                   0x0a
#define RF2XX_SFD_VALUE_REG                 0x0b
#define RF2XX_TRX_CTRL_2_REG                0x0c
#define RF2XX_ANT_DIV_REG                   0x0d
#define RF2XX_IRQ_MASK_REG                  0x0e
#define RF2XX_IRQ_STATUS_REG                0x0f
#define RF2XX_VREG_CTRL_REG                 0x10
#define RF2XX_BATMON_REG                    0x11
#define RF2XX_XOSC_CTRL_REG                 0x12
#define RF2XX_CC_CTRL_0_REG                 0x13
#define RF2XX_CC_CTRL_1_REG                 0x14
#define RF2XX_RX_SYN_REG                    0x15
#define RF2XX_TRX_RPC_REG                   0x16
#define RF2XX_RF_CTRL_0_REG                 0x16
#define RF2XX_XAH_CTRL_1_REG                0x17
#define RF2XX_FTN_CTRL_REG                  0x18
#define RF2XX_RF_CTRL_1_REG                 0x19
#define RF2XX_XAH_CTRL_2_REG                0x19
#define RF2XX_PLL_CF_REG                    0x1a
#define RF2XX_PLL_DCU_REG                   0x1b
#define RF2XX_PART_NUM_REG                  0x1c
#define RF2XX_VERSION_NUM_REG               0x1d
#define RF2XX_MAN_ID_0_REG                  0x1e
#define RF2XX_MAN_ID_1_REG                  0x1f
#define RF2XX_SHORT_ADDR_0_REG              0x20
#define RF2XX_SHORT_ADDR_1_REG              0x21
#define RF2XX_PAN_ID_0_REG                  0x22
#define RF2XX_PAN_ID_1_REG                  0x23
#define RF2XX_IEEE_ADDR_0_REG               0x24
#define RF2XX_IEEE_ADDR_1_REG               0x25
#define RF2XX_IEEE_ADDR_2_REG               0x26
#define RF2XX_IEEE_ADDR_3_REG               0x27
#define RF2XX_IEEE_ADDR_4_REG               0x28
#define RF2XX_IEEE_ADDR_5_REG               0x29
#define RF2XX_IEEE_ADDR_6_REG               0x2a
#define RF2XX_IEEE_ADDR_7_REG               0x2b
#define RF2XX_XAH_CTRL_0_REG                0x2c
#define RF2XX_CSMA_SEED_0_REG               0x2d
#define RF2XX_CSMA_SEED_1_REG               0x2e
#define RF2XX_CSMA_BE_REG                   0x2f
#define RF2XX_TST_CTRL_DIGI_REG             0x36
#define RF2XX_PHY_TX_TIME_REG               0x3b
#define RF2XX_PHY_PMU_VALUE_REG             0x3b
#define RF2XX_TST_AGC_REG                   0x3c
#define RF2XX_TST_SDM_REG                   0x3d

#define RF2XX_AES_STATUS_REG                0x82
#define RF2XX_AES_CTRL_REG                  0x83
#define RF2XX_AES_KEY_REG                   0x84
#define RF2XX_AES_STATE_REG                 0x84
#define RF2XX_AES_CTRL_M_REG                0x94

/* TRX_STATUS */
#define RF2XX_CCA_DONE                      7
#define RF2XX_CCA_STATUS                    6
#define RF2XX_TRX_STATUS                    0

/* TRX_STATE */
#define RF2XX_TRAC_STATUS                   5
#define RF2XX_TRX_CMD                       0
#define RF2XX_TRAC_BIT_MASK                 7

/* TRX_CTRL_0 */
#define RF2XX_TOM_EN                        7
#define RF2XX_PAD_IO                        6
#define RF2XX_PAD_IO_CLKM                   4
#define RF2XX_PMU_EN                        4
#define RF2XX_PMU_IF_INVERSE                4
#define RF2XX_CLKM_SHA_SEL                  3
#define RF2XX_CLKM_CTRL                     0

/* TRX_CTRL_1 */
#define RF2XX_PA_EXT_EN                     7
#define RF2XX_IRQ_2_EXT_EN                  6
#define RF2XX_TX_AUTO_CRC_ON                5
#define RF2XX_RX_BL_CTRL                    4
#define RF2XX_SPI_CMD_MODE                  2
#define RF2XX_IRQ_MASK_MODE                 1
#define RF2XX_IRQ_POLARITY                  0

/* PHY_TX_PWR */
#define RF2XX_PA_BOOST                      7
#define RF2XX_PA_BUF_LT                     6
#define RF2XX_GC_PA                         5
#define RF2XX_PA_SHR_LT                     4
#define RF2XX_TX_PWR                        0

/* PHY_RSSI */
#define RF2XX_RX_CRC_VALID                  7
#define RF2XX_RND_VALUE                     5
#define RF2XX_RSSI                          0
#define RF2XX_RSSI_MASK                     0x1F

/* PHY_CC_CCA */
#define RF2XX_CCA_REQUEST                   7
#define RF2XX_CCA_MODE                      5
#define RF2XX_CHANNEL                       0

/* CCA_THRES */
#define RF2XX_CCA_CS_THRES                  4
#define RF2XX_CCA_ED_THRES                  0

/* RX_CTRL_REG */
#define RF2XX_PEL_SHIFT_VALUE               6
#define RF2XX_JCM_EN                        5
#define RF2XX_PDT_THRES                     0

/* TRX_CTRL_2 */
#define RF2XX_RX_SAFE_MODE                  7
#define RF2XX_TRX_OFF_AVDD_EN               6
#define RF2XX_OQPSK_SCRAM_EN                5
#define RF2XX_OQPSK_SUB1_RC_EN              4
#define RF2XX_ALT_SPECTRUM                  4
#define RF2XX_BPSK_OQPSK                    3
#define RF2XX_SUB_MODE                      2
#define RF2XX_OQPSK_DATA_RATE               0
#define RF2XX_SUB_CHANNEL_MASK              0x3F
#define RF2XX_CC_BPSK_20                    0x00
#define RF2XX_CC_BPSK_40                    0x04
#define RF2XX_CC_OQPSK_SIN_RC_100           0x08
#define RF2XX_CC_OQPSK_SIN_250              0x0C
#define RF2XX_CC_OQPSK_RC_250               0x1C

/* ANT_DIV */
#define RF2XX_ANT_SEL                       7
#define RF2XX_ANT_DIV_EN                    3
#define RF2XX_ANT_EXT_SW_EN                 2
#define RF2XX_ANT_CTRL                      0

/* IRQ_MASK, IRQ_STATUS */
#define RF2XX_BAT_LOW                       7
#define RF2XX_TRX_UR                        6
#define RF2XX_AMI                           5
#define RF2XX_CCA_ED_DONE                   4
#define RF2XX_TRX_END                       3
#define RF2XX_RX_START                      2
#define RF2XX_PLL_UNLOCK                    1
#define RF2XX_PLL_LOCK                      0

/* VREG_CTRL */
#define RF2XX_AVREG_EXT                     7
#define RF2XX_AVDD_OK                       6
#define RF2XX_DVREG_EXT                     3
#define RF2XX_DVDD_OK                       2

/* BATMON */
#define RF2XX_PLL_LOCK_CP                   7
#define RF2XX_BATMON_OK                     5
#define RF2XX_BATMON_HR                     4
#define RF2XX_BATMON_VTH                    0

/* XOSC_CTRL */
#define RF2XX_XTAL_MODE                     4
#define RF2XX_XTAL_TRIM                     0

/* CC_CTRL_1 */
#define RF2XX_CC_BAND                       0

/* RX_SYN */
#define RF2XX_RX_PDT_DIS                    7
#define RF2XX_RX_OVERRIDE                   4
#define RF2XX_RX_PDT_LEVEL                  0

/* TRX_RPC */
#define RF2XX_RX_RPC_CTRL                   6
#define RF2XX_RX_RPC_EN                     5
#define RF2XX_PDT_RPC_EN                    4
#define RF2XX_PLL_RPC_EN                    3
#define RF2XX_XAH_TX_RPC_EN                 2
#define RF2XX_IPAN_RPC_EN                   1

/* RF_CTRL_0 */
#define RF2XX_PA_CHIP_LT                    6
#define RF2XX_F_SHIFT_MODE                  2
#define RF2XX_GC_TX_OFFS                    0
#define RF2XX_GC_TX_OFFS_MASK               3

/* XAH_CTRL_1 */
#define RF2XX_ARET_TX_TS_EN                 7
#define RF2XX_CSMA_LBT_MODE                 6
#define RF2XX_AACK_FLTR_RES_FT              5
#define RF2XX_AACK_UPLD_RES_FT              4
#define RF2XX_AACK_ACK_TIME                 2
#define RF2XX_AACK_PROM_MODE                1
#define RF2XX_AACK_SPC_EN                   0

/* FTN_CTRL */
#define RF2XX_FTN_START                     7
#define RF2XX_FTNV                          0

/* RF_CTRL_1 */
#define RF2XX_RF_MC                         4

/* XAH_CTRL_2 */
#define RF2XX_ARET_FRAME_RETRIES            4
#define RF2XX_ARET_CSMA_RETRIES             1

/* PLL_CF */
#define RF2XX_PLL_CF_START                  7
#define RF2XX_PLL_CF                        0

/* PLL_DCU */
#define RF2XX_PLL_DCU_START                 7

/* XAH_CTRL_0 */
#define RF2XX_MAX_FRAME_RETRES              4
#define RF2XX_MAX_CSMA_RETRES               1
#define RF2XX_SLOTTED_OPERATION             0

/* CSMA_SEED_1 */
#define RF2XX_AACK_FVN_MODE                 6
#define RF2XX_AACK_SET_PD                   5
#define RF2XX_AACK_DIS_ACK                  4
#define RF2XX_AACK_I_AM_COORD               3
#define RF2XX_CSMA_SEED_1                   0

/* CSMA_BE */
#define RF2XX_MAX_BE                        4
#define RF2XX_MIN_BE                        0

/* TST_CTRL_DIGI */
#define RF2XX_TST_CTRL_DIG                  0

/* PHY_TX_TIME_REG */
#define RF2XX_IRC_TX_TIME                   0

/* TST_AGC_REG */
#define RF2XX_AGC_HOLD_SEL                  5
#define RF2XX_AGC_RST                       4
#define RF2XX_AGC_OFF                       3
#define RF2XX_AGC_HOLD                      2
#define RF2XX_GC                            0

/* TST_SDM_REG */
#define RF2XX_MOD_SEL                       7
#define RF2XX_MOD                           6
#define RF2XX_TX_RX                         5
#define RF2XX_TX_RX_SEL                     4

/* AES_CTRL */
#define RF2XX_AES_CTRL_DIR                  3
#define RF2XX_AES_CTRL_MODE                 4
#define RF2XX_AES_CTRL_REQUEST              7

/* AES_STATUS */
#define RF2XX_AES_STATUS_DONE               0
#define RF2XX_AES_STATUS_ER                 7

#define RF2XX_RF_CMD_REG_W                  ((1 << 7) | (1 << 6))
#define RF2XX_RF_CMD_REG_R                  ((1 << 7) | (0 << 6))
#define RF2XX_RF_CMD_FRAME_W                ((0 << 7) | (1 << 6) | (1 << 5))
#define RF2XX_RF_CMD_FRAME_R                ((0 << 7) | (0 << 6) | (1 << 5))
#define RF2XX_RF_CMD_SRAM_W                 ((0 << 7) | (1 << 6) | (0 << 5))
#define RF2XX_RF_CMD_SRAM_R                 ((0 << 7) | (0 << 6) | (0 << 5))

/* RX_STATUS */
#define RF2XX_RX_TRAC_STATUS                4
#define RF2XX_RX_TRAC_BIT_MASK              RF2XX_TRAC_BIT_MASK

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_REGS_H_ */
