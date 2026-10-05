/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Registers of the Qorvo DW3000 family, restricted to what the driver uses.
 *
 * Section, table and page numbers refer to the "DW3000 Family User Manual",
 * version 1.1. "Datasheet" is the "DW3000 Datasheet", version 1.3.
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_DW3000_REGS_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_DW3000_REGS_H_

#include <zephyr/sys/util.h>

/*
 * A register address is a 5-bit register file ID and a 7-bit octet offset
 * into that file (2.3.1.2, Figure 2, p13). The driver carries both in one
 * 16-bit value.
 */
#define DW3000_REG(file, offset) (((file) << 8) | (offset))
#define DW3000_REG_FILE          GENMASK(12, 8)
#define DW3000_REG_OFFSET        GENMASK(6, 0)

/*
 * SPI transaction header (2.3.1.2, Table 3, p12 and Figure 2, p13).
 *
 * Fast command, one octet:      1 0 C4..C0 1
 * Short addressed, one octet:   RW 0 B4..B0 0
 * Full addressed, two octets:   RW 1 B4..B0 S6, then S5..S0 M1 M0
 *
 * RW is 1 for a write, B is the register file, S the octet offset and C the
 * command. M selects a masked write and is 0 for a plain read or write.
 */
#define DW3000_SPI_WRITE     BIT(7)
#define DW3000_SPI_FULL_ADDR BIT(6)
#define DW3000_SPI_FILE      GENMASK(5, 1)
#define DW3000_SPI_CMD       GENMASK(5, 1)
#define DW3000_SPI_FAST_CMD  BIT(0)
#define DW3000_SPI_MODE      GENMASK(1, 0)

/* Fast commands (9, Table 46, p238) */
#define DW3000_CMD_TXRXOFF 0x00U
#define DW3000_CMD_RX      0x02U
#define DW3000_CMD_TX_W4R  0x0cU

/*
 * DEV_ID: device identifier (8.2.2.1, p74). The DW3110 and the DW3210 differ
 * in package only (1.1, Table 1, p7) and report the same identifier; REV
 * "will be updated for minor corrections" and is not part of it.
 */
#define DW3000_DEV_ID        DW3000_REG(0x00, 0x00)
#define DW3000_DEV_ID_REV    GENMASK(3, 0)
#define DW3000_DEV_ID_DW3X10 0xdeca0302U

/*
 * SYS_ENABLE: system event enable mask, same bits as SYS_STATUS (8.2.2.13,
 * p88). Its upper two octets enable command and SPI error events and are
 * 0x0f00 after reset (p89).
 */
#define DW3000_SYS_ENABLE    DW3000_REG(0x00, 0x3c)
#define DW3000_SYS_ENABLE_HI DW3000_REG(0x00, 0x40)

/* SYS_STATUS: system event status, bits are cleared by writing 1 (8.2.2.14, p93) */
#define DW3000_SYS_STATUS         DW3000_REG(0x00, 0x44)
#define DW3000_SYS_STATUS_CPLOCK  BIT(1)
#define DW3000_SYS_STATUS_TXFRS   BIT(7)
#define DW3000_SYS_STATUS_RXSFDD  BIT(9)
#define DW3000_SYS_STATUS_CIADONE BIT(10)
#define DW3000_SYS_STATUS_RXPHE   BIT(12)
#define DW3000_SYS_STATUS_RXFR    BIT(13)
#define DW3000_SYS_STATUS_RXFCG   BIT(14)
#define DW3000_SYS_STATUS_RXFCE   BIT(15)
#define DW3000_SYS_STATUS_RXFSL   BIT(16)
#define DW3000_SYS_STATUS_CIAERR  BIT(18)
#define DW3000_SYS_STATUS_SPIRDY  BIT(23)
#define DW3000_SYS_STATUS_RXSTO   BIT(26)

/* TX_FCTRL: transmit frame control, low 32 bits (8.2.2.8, p84) */
#define DW3000_TX_FCTRL          DW3000_REG(0x00, 0x24)
#define DW3000_TX_FCTRL_TXFLEN   GENMASK(9, 0)
#define DW3000_TX_FCTRL_TXBR     BIT(10)
#define DW3000_TX_FCTRL_TXPSR    GENMASK(15, 12)
#define DW3000_TX_FCTRL_TXPSR_64 0x1U

/* RX_FINFO: receive frame information (8.2.2.15, p100) */
#define DW3000_RX_FINFO        DW3000_REG(0x00, 0x4c)
#define DW3000_RX_FINFO_RXFLEN GENMASK(9, 0)

/* RX_TIME and TX_TIME: 40-bit receive and transmit time stamps (p102, p103) */
#define DW3000_RX_TIME        DW3000_REG(0x00, 0x64)
#define DW3000_TX_TIME        DW3000_REG(0x00, 0x74)
#define DW3000_TIME_STAMP_LEN 5U

/* CHAN_CTRL: channel control, 16 bits (8.2.2.22, p110) */
#define DW3000_CHAN_CTRL               DW3000_REG(0x01, 0x14)
#define DW3000_CHAN_CTRL_RF_CHAN       BIT(0)
#define DW3000_CHAN_CTRL_SFD_TYPE      GENMASK(2, 1)
#define DW3000_CHAN_CTRL_SFD_TYPE_IEEE 0x0U
#define DW3000_CHAN_CTRL_TX_PCODE      GENMASK(7, 3)
#define DW3000_CHAN_CTRL_RX_PCODE      GENMASK(12, 8)

/* Receiver tuning parameters and the values to set them to (8.2.4, Table 24, p126) */
#define DW3000_DGC_CFG              DW3000_REG(0x03, 0x18)
#define DW3000_DGC_CFG_RX_TUNE_EN   BIT(0)
#define DW3000_DGC_CFG_THR_64       GENMASK(14, 9)
#define DW3000_DGC_CFG_THR_64_TUNED 0x32U
#define DW3000_DGC_CFG0             DW3000_REG(0x03, 0x1c)
#define DW3000_DGC_CFG0_TUNED       0x10000240U
#define DW3000_DGC_CFG1             DW3000_REG(0x03, 0x20)
#define DW3000_DGC_CFG1_TUNED       0x1b6da489U
#define DW3000_DGC_LUT(n)           DW3000_REG(0x03, 0x38 + 4 * (n))

/*
 * RX_CAL_RESI, RX_CAL_RESQ and RX_CAL_STS: result and status of the receiver
 * calibration (8.2.5.3 and 8.2.5.4, p129; 8.2.5.5, p130). A result is 29 bits
 * wide, and all of them set if the calibration failed.
 */
#define DW3000_RX_CAL_RESI       DW3000_REG(0x04, 0x14)
#define DW3000_RX_CAL_RESQ       DW3000_REG(0x04, 0x1c)
#define DW3000_RX_CAL_RES        GENMASK(28, 0)
#define DW3000_RX_CAL_RES_FAILED 0x1fffffffU
#define DW3000_RX_CAL_STS        DW3000_REG(0x04, 0x20)
#define DW3000_RX_CAL_STS_DONE   BIT(0)

/* DTUNE0 and DTUNE3: digital receiver tuning (8.2.7.1, p145 and 8.2.7.4, p147) */
#define DW3000_DTUNE0       DW3000_REG(0x06, 0x00)
#define DW3000_DTUNE0_DT0B4 BIT(4)
#define DW3000_DTUNE3       DW3000_REG(0x06, 0x0c)
#define DW3000_DTUNE3_TUNED 0xaf5f35ccU

/*
 * Analog transmitter control and LDO tuning, with the values to set them to
 * (8.2.8.4, p151; 8.2.8.5, Table 30, p152; 8.2.8.10, p155). LDO_TUNE holds the
 * trim value of a transceiver, 61 bits in eight octets (8.2.8.8, p154).
 */
#define DW3000_RF_TX_CTRL_1       DW3000_REG(0x07, 0x1a)
#define DW3000_RF_TX_CTRL_1_TUNED 0x0eU
#define DW3000_RF_TX_CTRL_2       DW3000_REG(0x07, 0x1c)
#define DW3000_RF_TX_CTRL_2_CH5   0x1c071134U
#define DW3000_RF_TX_CTRL_2_CH9   0x1c010034U
#define DW3000_LDO_TUNE           DW3000_REG(0x07, 0x40)
#define DW3000_LDO_TUNE_LEN       8U
#define DW3000_LDO_RLOAD          DW3000_REG(0x07, 0x51)
#define DW3000_LDO_RLOAD_TUNED    0x14U

/*
 * PLL_CFG and PLL_CAL: RF PLL configuration and calibration (8.2.10, p162 to
 * p164), with the configuration values per channel (Table 35, p163). The
 * manual gives the PLL_CAL configuration as a value of the whole low octet:
 * 0x31 after reset, "a more optimal setting of 0x81 should be used".
 */
#define DW3000_PLL_CFG        DW3000_REG(0x09, 0x00)
#define DW3000_PLL_CFG_CH5    0x1f3cU
#define DW3000_PLL_CFG_CH9    0x0f3cU
#define DW3000_PLL_CAL        DW3000_REG(0x09, 0x08)
#define DW3000_PLL_CAL_CONFIG 0x81U
#define DW3000_PLL_CAL_CAL_EN BIT(8)

/*
 * AON_DIG_CFG, AON_CTRL and AON_CFG: what the transceiver does when it wakes
 * up, and how it is sent to sleep (8.2.11.1, p166; 8.2.11.2, p167 and
 * 8.2.11.6, p172)
 */
#define DW3000_AON_DIG_CFG             DW3000_REG(0x0a, 0x00)
#define DW3000_AON_DIG_CFG_ONW_GO2IDLE BIT(8)
#define DW3000_AON_DIG_CFG_ONW_PGFCAL  BIT(11)
#define DW3000_AON_CTRL                DW3000_REG(0x0a, 0x04)
#define DW3000_AON_CTRL_SAVE           BIT(1)
#define DW3000_AON_CFG                 DW3000_REG(0x0a, 0x14)
#define DW3000_AON_CFG_SLEEP_EN        BIT(0)
#define DW3000_AON_CFG_WAKE_CSN        BIT(3)

/*
 * OTP_CFG: the kick bits have the transceiver copy trim values from its OTP
 * memory to their registers (8.2.12.3, p175 and p176)
 */
#define DW3000_OTP_CFG           DW3000_REG(0x0b, 0x08)
#define DW3000_OTP_CFG_LDO_KICK  BIT(7)
#define DW3000_OTP_CFG_BIAS_KICK BIT(8)

/* CLK_CTRL: clock control (8.2.15.2, p219) */
#define DW3000_CLK_CTRL                 DW3000_REG(0x11, 0x04)
#define DW3000_CLK_CTRL_SYS_CLK         GENMASK(1, 0)
#define DW3000_CLK_CTRL_SYS_CLK_AUTO    0x0U
#define DW3000_CLK_CTRL_SYS_CLK_FAST_RC 0x3U

/* SEQ_CTRL: sequencing control (8.2.15.3, p222) */
#define DW3000_SEQ_CTRL            DW3000_REG(0x11, 0x08)
#define DW3000_SEQ_CTRL_AINIT2IDLE BIT(8)
#define DW3000_SEQ_CTRL_FORCE2INIT BIT(23)

/* BIAS_CTRL: bias trim value of a transceiver, 16 bits (8.2.15.7, p226) */
#define DW3000_BIAS_CTRL DW3000_REG(0x11, 0x1f)

/* RX_BUFFER_0 and TX_BUFFER: frame data buffers (p227) */
#define DW3000_RX_BUFFER_0 DW3000_REG(0x12, 0x00)
#define DW3000_TX_BUFFER   DW3000_REG(0x14, 0x00)

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_DW3000_REGS_H_ */
