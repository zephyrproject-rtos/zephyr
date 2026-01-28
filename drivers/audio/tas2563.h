/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TAS2563_REG_H_
#define ZEPHYR_DRIVERS_AUDIO_TAS2563_REG_H_

/* Register Page and Book selection */
#define TAS2563_PAGE_REG                0x00
#define TAS2563_BOOK_REG                0x7F

/* Page 0 Registers */
#define TAS2563_SW_RESET                0x01
#define TAS2563_PWR_CTL                 0x02
#define TAS2563_PB_CFG1                 0x03
#define TAS2563_MISC_CFG1               0x04
#define TAS2563_MISC_CFG2               0x05

/* TDM Configuration Registers */
#define TAS2563_TDM_CFG0                0x06
#define TAS2563_TDM_CFG1                0x07
#define TAS2563_TDM_CFG2                0x08
#define TAS2563_TDM_CFG3                0x09
#define TAS2563_TDM_CFG4                0x0A
#define TAS2563_TDM_CFG5                0x0B
#define TAS2563_TDM_CFG6                0x0C
#define TAS2563_TDM_CFG7                0x0D
#define TAS2563_TDM_CFG8                0x0E
#define TAS2563_TDM_CFG9                0x0F
#define TAS2563_TDM_CFG10               0x10

/* TDM Clock Detection Monitor */
#define TAS2563_TDM_DET                 0x11

/* Limiter Configuration */
#define TAS2563_LIM_CFG0                0x12
#define TAS2563_LIM_CFG1                0x13

/* Brown Out Prevention Configuration */
#define TAS2563_BOP_CFG0                0x14
#define TAS2563_BOP_CFG1                0x15

/* Interrupt Masks */
#define TAS2563_INT_MASK0               0x1A
#define TAS2563_INT_MASK1               0x1B
#define TAS2563_INT_MASK2               0x1C
#define TAS2563_INT_MASK3               0x1D

/* Interrupt Live Status */
#define TAS2563_INT_LIVE0               0x1F
#define TAS2563_INT_LIVE1               0x20
#define TAS2563_INT_LIVE2               0x21
#define TAS2563_INT_LIVE3               0x22

/* Interrupt Latched Status */
#define TAS2563_INT_LTCH0               0x24
#define TAS2563_INT_LTCH1               0x25
#define TAS2563_INT_LTCH2               0x26
#define TAS2563_INT_LTCH3               0x27

/* SAR ADC Conversion Registers */
#define TAS2563_VBAT_MSB                0x2A
#define TAS2563_VBAT_LSB                0x2B
#define TAS2563_TEMP                    0x2C

/* Clock and Interrupt Configuration */
#define TAS2563_INT_CLK_CFG             0x30

/* Digital Input Pull Down */
#define TAS2563_DIN_PD                  0x31

/* Miscellaneous Configuration */
#define TAS2563_MISC                    0x32

/* Boost Configuration */
#define TAS2563_BOOST_CFG1              0x33
#define TAS2563_BOOST_CFG2              0x34
#define TAS2563_BOOST_CFG3              0x35

/* Boost Current Limit Configuration */
#define TAS2563_BST_ILIM_CFG0           0x40

/* PDM Configuration */
#define TAS2563_PDM_CONFIG0             0x41
#define TAS2563_PDM_CONFIG3             0x42

/* Revision and PG ID */
#define TAS2563_REV_ID                  0x7D
#define TAS2563_I2C_CKSUM               0x7E

/* Bit field definitions */

/* SW_RESET (0x01) */
#define TAS2563_SW_RESET_BIT            BIT(0)

/* PWR_CTL (0x02) */
#define TAS2563_PWR_MODE_MASK           0x03
#define TAS2563_PWR_MODE_ACTIVE         0x00
#define TAS2563_PWR_MODE_MUTE           0x01
#define TAS2563_PWR_MODE_SHUTDOWN       0x02
#define TAS2563_PWR_MODE_LOAD_DIAG      0x03

#define TAS2563_VSNS_PD_MASK            BIT(2)
#define TAS2563_ISNS_PD_MASK            BIT(3)

/* PB_CFG1 (0x03) - Playback Configuration 1 */
#define TAS2563_AMP_LEVEL_MASK          0x3E
#define TAS2563_AMP_LEVEL_SHIFT         1
#define TAS2563_DC_BLOCKER_DIS          BIT(6)

/* Amplifier Level Settings (in dBV) */
#define TAS2563_AMP_LEVEL_8_5DBV        0x01
#define TAS2563_AMP_LEVEL_9_0DBV        0x02
#define TAS2563_AMP_LEVEL_10_0DBV       0x04
#define TAS2563_AMP_LEVEL_11_0DBV       0x06
#define TAS2563_AMP_LEVEL_12_0DBV       0x08
#define TAS2563_AMP_LEVEL_13_0DBV       0x0A
#define TAS2563_AMP_LEVEL_14_0DBV       0x0C
#define TAS2563_AMP_LEVEL_15_0DBV       0x0E
#define TAS2563_AMP_LEVEL_16_0DBV       0x10
#define TAS2563_AMP_LEVEL_17_0DBV       0x12
#define TAS2563_AMP_LEVEL_18_0DBV       0x14
#define TAS2563_AMP_LEVEL_19_0DBV       0x16
#define TAS2563_AMP_LEVEL_20_0DBV       0x18
#define TAS2563_AMP_LEVEL_21_0DBV       0x1A
#define TAS2563_AMP_LEVEL_22_0DBV       0x1C

/* MISC_CFG2 (0x05) */
#define TAS2563_I2C_GLOBAL_EN           BIT(1)

/* TDM_CFG0 (0x06) */
#define TAS2563_FRAME_START             BIT(0)
#define TAS2563_SAMP_RATE_MASK          0x0E
#define TAS2563_SAMP_RATE_SHIFT         1
#define TAS2563_AUTO_RATE               BIT(4)
#define TAS2563_RAMP_RATE               BIT(5)
#define TAS2563_CLASS_D_SYNC            BIT(6)

/* TDM_CFG0 (0x06) */
/* Sample Rates */
#define TAS2563_SR_8KHZ                 0x00
#define TAS2563_SR_16KHZ                0x01
#define TAS2563_SR_24KHZ                0x02
#define TAS2563_SR_32KHZ                0x03
#define TAS2563_SR_48KHZ                0x04
#define TAS2563_SR_96KHZ                0x05
#define TAS2563_SR_192KHZ               0x06

/* TDM_CFG1 (0x07) */
#define TAS2563_RX_EDGE                 BIT(0)
#define TAS2563_RX_OFFSET_MASK          0x3E
#define TAS2563_RX_OFFSET_SHIFT         1
#define TAS2563_RX_JUSTIFY              BIT(6)

/* TDM_CFG2 (0x08) */
#define TAS2563_RX_SLEN_MASK            0x03
#define TAS2563_RX_SLEN_16BITS          0x00
#define TAS2563_RX_SLEN_24BITS          0x01
#define TAS2563_RX_SLEN_32BITS          0x02

#define TAS2563_RX_WLEN_MASK            0x0C
#define TAS2563_RX_WLEN_SHIFT           2
#define TAS2563_RX_WLEN_16BITS          0x00
#define TAS2563_RX_WLEN_20BITS          0x01
#define TAS2563_RX_WLEN_24BITS          0x02
#define TAS2563_RX_WLEN_32BITS          0x03

#define TAS2563_RX_SCFG_MASK            0x30
#define TAS2563_RX_SCFG_SHIFT           4
#define TAS2563_RX_SCFG_MONO_I2C        0x00
#define TAS2563_RX_SCFG_MONO_LEFT       0x01
#define TAS2563_RX_SCFG_MONO_RIGHT      0x02
#define TAS2563_RX_SCFG_STEREO_DOWNMIX  0x03

#define TAS2563_CFG2_CONFIG_MASK	GENMASK(5, 0)

/* TDM_CFG3 (0x09) - Time slot selection */
#define TAS2563_RX_SLOT_LEFT_MASK       0x0F
#define TAS2563_RX_SLOT_RIGHT_MASK      0xF0
#define TAS2563_RX_SLOT_RIGHT_SHIFT     4

/* TDM_CFG4 (0x0A) */
#define TAS2563_TX_EDGE                 BIT(0)
#define TAS2563_TX_OFFSET_MASK          0x0E
#define TAS2563_TX_OFFSET_SHIFT         1
#define TAS2563_TX_FILL                 BIT(4)
#define TAS2563_TX_KEEP_EN              BIT(5)

/* INT_LTCH0(0x24) */
#define TAS2563_INT_OTE			BIT(0)
#define TAS2563_INT_OI			BIT(1)
#define TAS2563_INT_TDMCKE		BIT(2)

/* BOOST_CFG1 (0x33) */
#define TAS2563_BOOST_EN                BIT(5)
#define TAS2563_BOOST_MODE_MASK         0xC0
#define TAS2563_BOOST_MODE_CLASS_H      0x00
#define TAS2563_BOOST_MODE_CLASS_G      0x40
#define TAS2563_BOOST_MODE_ALWAYS_ON    0x80
#define TAS2563_BOOST_MODE_ALWAYS_OFF   0xC0

/* INT_CLK_CFG (0x30) */
#define TAS2563_IRQ_PIN_CFG_MASK        0x03
#define TAS2563_IRQ_PIN_CFG_LIVE        0x00
#define TAS2563_IRQ_PIN_CFG_LATCHED     0x01
#define TAS2563_CLR_INT_LTCH            BIT(2)

/* MISC (0x32) */
#define TAS2563_IRQ_POL                 BIT(7)

/* Default Values */
#define TAS2563_DEFAULT_PAGE            0x00
#define TAS2563_DEFAULT_BOOK            0x00

/* I2C Addresses (7-bit) */
#define TAS2563_I2C_ADDR_DEFAULT        0x4C
#define TAS2563_I2C_ADDR_GLOBAL         0x48

#endif
