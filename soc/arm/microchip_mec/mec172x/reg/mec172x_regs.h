/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEC172X_REGS_H
#define MEC172X_REGS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ACPI EC Registers (ACPI_EC) */
struct acpi_ec_regs {
	volatile uint32_t OS_DATA;
	volatile uint8_t OS_CMD_STS;
	volatile uint8_t OS_BYTE_CTRL;
	uint8_t RSVD1[0x100u - 0x06u];
	volatile uint32_t EC2OS_DATA;
	volatile uint8_t EC_STS;
	volatile uint8_t EC_BYTE_CTRL;
	uint8_t RSVD2[2];
	volatile uint32_t OS2EC_DATA;
};

/** @brief ACPI PM1 Registers (ACPI_PM1) */
struct acpi_pm1_regs {
	volatile uint8_t RT_STS1;
	volatile uint8_t RT_STS2;
	volatile uint8_t RT_EN1;
	volatile uint8_t RT_EN2;
	volatile uint8_t RT_CTRL1;
	volatile uint8_t RT_CTRL2;
	volatile uint8_t RT_CTRL21;
	volatile uint8_t RT_CTRL22;
	uint8_t RSVD1[(0x100u - 0x008u)];
	volatile uint8_t EC_STS1;
	volatile uint8_t EC_STS2;
	volatile uint8_t EC_EN1;
	volatile uint8_t EC_EN2;
	volatile uint8_t EC_CTRL1;
	volatile uint8_t EC_CTRL2;
	volatile uint8_t EC_CTRL21;
	volatile uint8_t EC_CTRL22;
	uint8_t RSVD2[(0x0110u - 0x0108u)];
	volatile uint8_t EC_PM_STS;
	uint8_t RSVD3[3];
};

/** @brief Analog to Digital Converter Registers (ADC) */
struct adc_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t DELAY;
	volatile uint32_t STATUS;
	volatile uint32_t SINGLE;
	volatile uint32_t REPEAT;
	volatile uint32_t RD[8];
	uint8_t RSVD1[0x7c - 0x34];
	volatile uint32_t CONFIG;
	volatile uint32_t VREF_CHAN_SEL;
	volatile uint32_t VREF_CTRL;
	volatile uint32_t SARADC_CTRL;
};

/** @brief Basic Timer(32 and 16 bit) registers. Total size = 20(0x14) bytes */
struct btmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint8_t STS;
	uint8_t RSVDC[3];
	volatile uint8_t IEN;
	uint8_t RSVDD[3];
	volatile uint32_t CTRL;
};

/** @brief Capture/Compare Timer */
struct cct_regs {
	volatile uint32_t CTRL;
	volatile uint32_t CAP0_CTRL;
	volatile uint32_t CAP1_CTRL;
	volatile uint32_t FREE_RUN;
	volatile uint32_t CAP0;
	volatile uint32_t CAP1;
	volatile uint32_t CAP2;
	volatile uint32_t CAP3;
	volatile uint32_t CAP4;
	volatile uint32_t CAP5;
	volatile uint32_t COMP0;
	volatile uint32_t COMP1;
};

/** @brief 16-bit Counter Timer */
struct ctmr_regs {
	volatile uint32_t CTRL;
	volatile uint32_t CEV_CTRL;
	volatile uint32_t RELOAD;
	volatile uint32_t COUNT;
};

/** @brief DMA Main (DMAM) */
struct dma_main_regs {
	volatile uint8_t ACTRST;
	uint8_t RSVDA[3];
	volatile uint32_t DATA_PKT;
	volatile uint32_t ARB_FSM;
};

/** @brief DMA Channels 0 and 1 with ALU */
struct dma_chan_alu_regs {
	volatile uint32_t ACTV;
	volatile uint32_t MSTART;
	volatile uint32_t MEND;
	volatile uint32_t DSTART;
	volatile uint32_t CTRL;
	volatile uint32_t ISTS;
	volatile uint32_t IEN;
	volatile uint32_t FSM;
	volatile uint32_t ALU_EN;
	volatile uint32_t ALU_DATA;
	volatile uint32_t ALU_STS;
	volatile uint32_t ALU_FSM;
	uint32_t RSVD6[4];
};

/** @brief DMA Channels 2 through 11 no ALU */
struct dma_chan_regs {
	volatile uint32_t ACTV;
	volatile uint32_t MSTART;
	volatile uint32_t MEND;
	volatile uint32_t DSTART;
	volatile uint32_t CTRL;
	volatile uint32_t ISTS;
	volatile uint32_t IEN;
	volatile uint32_t FSM;
	uint32_t RSVD4[8];
};

/** @brief DMA block: main and channels */
struct dma_regs {
	volatile uint32_t ACTRST;
	volatile uint32_t DATA_PKT;
	volatile uint32_t ARB_FSM;
	uint32_t RSVD2[13];
	struct dma_chan_alu_regs CHAN[16];
};

/** @brief GIRQ registers. Total size = 20(0x14) bytes */
struct girq_regs {
	volatile uint32_t SRC;
	volatile uint32_t EN_SET;
	volatile uint32_t RESULT;
	volatile uint32_t EN_CLR;
	uint32_t RSVD1[1];
};

/** @brief ECIA registers with each GIRQ elucidated */
struct ecia_named_regs {
	struct girq_regs GIRQ08;
	struct girq_regs GIRQ09;
	struct girq_regs GIRQ10;
	struct girq_regs GIRQ11;
	struct girq_regs GIRQ12;
	struct girq_regs GIRQ13;
	struct girq_regs GIRQ14;
	struct girq_regs GIRQ15;
	struct girq_regs GIRQ16;
	struct girq_regs GIRQ17;
	struct girq_regs GIRQ18;
	struct girq_regs GIRQ19;
	struct girq_regs GIRQ20;
	struct girq_regs GIRQ21;
	struct girq_regs GIRQ22;
	struct girq_regs GIRQ23;
	struct girq_regs GIRQ24;
	struct girq_regs GIRQ25;
	struct girq_regs GIRQ26;
	uint8_t RSVD2[(0x0200 - 0x017c)];
	volatile uint32_t BLK_EN_SET;
	volatile uint32_t BLK_EN_CLR;
	volatile uint32_t BLK_ACTIVE;
};

/** @brief ECIA registers with array of GIRQ's */
struct ecia_regs {
	struct girq_regs GIRQ[19];
	uint8_t RSVD2[(0x200 - 0x17c)];
	volatile uint32_t BLK_EN_SET;
	volatile uint32_t BLK_EN_CLR;
	volatile uint32_t BLK_ACTIVE;
};

/**  @brief EC Subsystem (ECS) */
struct ecs_regs {
	uint32_t RSVD1[1];
	volatile uint32_t AHB_ERR_ADDR;
	uint32_t RSVD2[2];
	volatile uint32_t OSC_ID;
	volatile uint32_t AHB_ERR_CTRL;
	volatile uint32_t INTR_CTRL;
	volatile uint32_t ETM_CTRL;
	volatile uint32_t DEBUG_CTRL;
	volatile uint32_t OTP_LOCK;
	volatile uint32_t WDT_CNT;
	uint32_t RSVD3[5];
	volatile uint32_t PECI_DIS;
	uint32_t RSVD4[3];
	volatile uint32_t VCI_FW_OVR;
	uint32_t RSVD5[1];
	volatile uint32_t CRYPTO_CFG;
	uint32_t RSVD6[5];
	volatile uint32_t JTAG_MCFG;
	volatile uint32_t JTAG_MSTS;
	volatile uint32_t JTAG_MTDO;
	volatile uint32_t JTAG_MTDI;
	volatile uint32_t JTAG_MTMS;
	volatile uint32_t JTAG_MCMD;
	uint32_t RSVD7[2];
	volatile uint32_t VW_FW_OVR;
	volatile uint32_t CMP_CTRL;
	volatile uint32_t CMP_SLP_CTRL;
	uint32_t RSVD8[(0x144 - 0x09c) / 4];
	volatile uint32_t SLP_STS_MIRROR;
};

/** @brief EMI Registers (EMI) */
struct emi_regs {
	volatile uint8_t OS_H2E_MBOX;
	volatile uint8_t OS_E2H_MBOX;
	volatile uint8_t OS_EC_ADDR_LSB;
	volatile uint8_t OS_EC_ADDR_MSB;
	volatile uint32_t OS_EC_DATA;
	volatile uint8_t OS_INT_SRC_LSB;
	volatile uint8_t OS_INT_SRC_MSB;
	volatile uint8_t OS_INT_MASK_LSB;
	volatile uint8_t OS_INT_MASK_MSB;
	volatile uint32_t OS_APP_ID;
	uint8_t RSVD1[0x100u - 0x10u];
	volatile uint8_t H2E_MBOX;
	volatile uint8_t E2H_MBOX;
	uint8_t RSVD2[2];
	volatile uint32_t MEM_BASE_0;
	volatile uint32_t MEM_LIMIT_0;
	volatile uint32_t MEM_BASE_1;
	volatile uint32_t MEM_LIMIT_1;
	volatile uint16_t EC_OS_INT_SET;
	volatile uint16_t EC_OS_INT_CLR_EN;
};

/** @brief Glocal Configuration Registers */
struct global_cfg_regs {
	volatile uint8_t RSVD0[2];
	volatile uint8_t TEST02;
	volatile uint8_t RSVD1[4];
	volatile uint8_t LOG_DEV_NUM;
	volatile uint8_t RSVD2[20];
	volatile uint32_t DEV_REV_ID;
	volatile uint8_t LEGACY_DEV_ID;
	volatile uint8_t RSVD3[14];
};

/** @brief All GPIO register as arrays of registers */
struct gpio_regs {
	volatile uint32_t CTRL[192];
	volatile uint32_t PARIN[6];
	uint32_t RSVD1[(0x380 - 0x318) / 4];
	volatile uint32_t PAROUT[6];
	uint32_t RSVD2[(0x3ec - 0x398) / 4];
	volatile uint32_t LOCK[6];
	uint32_t RSVD3[(0x500 - 0x400) / 4];
	volatile uint32_t CTRL2[192];
};

/** @brief GPIO control registers by pin name */
struct gpio_ctrl_regs {
	volatile uint32_t CTRL_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL_0002;
	volatile uint32_t CTRL_0003;
	volatile uint32_t CTRL_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL_0007;
	volatile uint32_t CTRL_0010;
	volatile uint32_t CTRL_0011;
	volatile uint32_t CTRL_0012;
	volatile uint32_t CTRL_0013;
	volatile uint32_t CTRL_0014;
	volatile uint32_t CTRL_0015;
	volatile uint32_t CTRL_0016;
	volatile uint32_t CTRL_0017;
	volatile uint32_t CTRL_0020;
	volatile uint32_t CTRL_0021;
	volatile uint32_t CTRL_0022;
	volatile uint32_t CTRL_0023;
	volatile uint32_t CTRL_0024;
	volatile uint32_t CTRL_0025;
	volatile uint32_t CTRL_0026;
	volatile uint32_t CTRL_0027;
	volatile uint32_t CTRL_0030;
	volatile uint32_t CTRL_0031;
	volatile uint32_t CTRL_0032;
	volatile uint32_t CTRL_0033;
	volatile uint32_t CTRL_0034;
	volatile uint32_t CTRL_0035;
	volatile uint32_t CTRL_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL_0042;
	volatile uint32_t CTRL_0043;
	volatile uint32_t CTRL_0044;
	volatile uint32_t CTRL_0045;
	volatile uint32_t CTRL_0046;
	volatile uint32_t CTRL_0047;
	volatile uint32_t CTRL_0050;
	volatile uint32_t CTRL_0051;
	volatile uint32_t CTRL_0052;
	volatile uint32_t CTRL_0053;
	volatile uint32_t CTRL_0054;
	volatile uint32_t CTRL_0055;
	volatile uint32_t CTRL_0056;
	volatile uint32_t CTRL_0057;
	volatile uint32_t CTRL_0060;
	volatile uint32_t CTRL_0061;
	volatile uint32_t CTRL_0062;
	volatile uint32_t CTRL_0063;
	volatile uint32_t CTRL_0064;
	volatile uint32_t CTRL_0065;
	volatile uint32_t CTRL_0066;
	volatile uint32_t CTRL_0067;
	volatile uint32_t CTRL_0070;
	volatile uint32_t CTRL_0071;
	volatile uint32_t CTRL_0072;
	volatile uint32_t CTRL_0073;
	uint32_t RSVD5[4];
	volatile uint32_t CTRL_0100;
	volatile uint32_t CTRL_0101;
	volatile uint32_t CTRL_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL_0104;
	volatile uint32_t CTRL_0105;
	volatile uint32_t CTRL_0106;
	volatile uint32_t CTRL_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL_0112;
	volatile uint32_t CTRL_0113;
	volatile uint32_t CTRL_0114;
	volatile uint32_t CTRL_0115;
	uint32_t RSVD8[2];
	volatile uint32_t CTRL_0120;
	volatile uint32_t CTRL_0121;
	volatile uint32_t CTRL_0122;
	volatile uint32_t CTRL_0123;
	volatile uint32_t CTRL_0124;
	volatile uint32_t CTRL_0125;
	volatile uint32_t CTRL_0126;
	volatile uint32_t CTRL_0127;
	volatile uint32_t CTRL_0130;
	volatile uint32_t CTRL_0131;
	volatile uint32_t CTRL_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL_0140;
	volatile uint32_t CTRL_0141;
	volatile uint32_t CTRL_0142;
	volatile uint32_t CTRL_0143;
	volatile uint32_t CTRL_0144;
	volatile uint32_t CTRL_0145;
	volatile uint32_t CTRL_0146;
	volatile uint32_t CTRL_0147;
	volatile uint32_t CTRL_0150;
	volatile uint32_t CTRL_0151;
	volatile uint32_t CTRL_0152;
	volatile uint32_t CTRL_0153;
	volatile uint32_t CTRL_0154;
	volatile uint32_t CTRL_0155;
	volatile uint32_t CTRL_0156;
	volatile uint32_t CTRL_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL_0161;
	volatile uint32_t CTRL_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL_0170;
	volatile uint32_t CTRL_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL_0200;
	volatile uint32_t CTRL_0201;
	volatile uint32_t CTRL_0202;
	volatile uint32_t CTRL_0203;
	volatile uint32_t CTRL_0204;
	volatile uint32_t CTRL_0205;
	volatile uint32_t CTRL_0206;
	volatile uint32_t CTRL_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL_0221;
	volatile uint32_t CTRL_0222;
	volatile uint32_t CTRL_0223;
	volatile uint32_t CTRL_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL_0226;
	volatile uint32_t CTRL_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL_0240;
	volatile uint32_t CTRL_0241;
	volatile uint32_t CTRL_0242;
	volatile uint32_t CTRL_0243;
	volatile uint32_t CTRL_0244;
	volatile uint32_t CTRL_0245;
	volatile uint32_t CTRL_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL_0254;
	volatile uint32_t CTRL_0255;
};

/** @brief GPIO Control 2 registers by pin name */
struct gpio_ctrl2_regs {
	volatile uint32_t CTRL2_0000;
	uint32_t RSVD1[1];
	volatile uint32_t CTRL2_0002;
	volatile uint32_t CTRL2_0003;
	volatile uint32_t CTRL2_0004;
	uint32_t RSVD2[2];
	volatile uint32_t CTRL2_0007;
	volatile uint32_t CTRL2_0010;
	volatile uint32_t CTRL2_0011;
	volatile uint32_t CTRL2_0012;
	volatile uint32_t CTRL2_0013;
	volatile uint32_t CTRL2_0014;
	volatile uint32_t CTRL2_0015;
	volatile uint32_t CTRL2_0016;
	volatile uint32_t CTRL2_0017;
	volatile uint32_t CTRL2_0020;
	volatile uint32_t CTRL2_0021;
	volatile uint32_t CTRL2_0022;
	volatile uint32_t CTRL2_0023;
	volatile uint32_t CTRL2_0024;
	volatile uint32_t CTRL2_0025;
	volatile uint32_t CTRL2_0026;
	volatile uint32_t CTRL2_0027;
	volatile uint32_t CTRL2_0030;
	volatile uint32_t CTRL2_0031;
	volatile uint32_t CTRL2_0032;
	volatile uint32_t CTRL2_0033;
	volatile uint32_t CTRL2_0034;
	volatile uint32_t CTRL2_0035;
	volatile uint32_t CTRL2_0036;
	uint32_t RSVD3[1];
	volatile uint32_t CTRL2_0040;
	uint32_t RSVD4[1];
	volatile uint32_t CTRL2_0042;
	volatile uint32_t CTRL2_0043;
	volatile uint32_t CTRL2_0044;
	volatile uint32_t CTRL2_0045;
	volatile uint32_t CTRL2_0046;
	volatile uint32_t CTRL2_0047;
	volatile uint32_t CTRL2_0050;
	volatile uint32_t CTRL2_0051;
	volatile uint32_t CTRL2_0052;
	volatile uint32_t CTRL2_0053;
	volatile uint32_t CTRL2_0054;
	volatile uint32_t CTRL2_0055;
	volatile uint32_t CTRL2_0056;
	volatile uint32_t CTRL2_0057;
	volatile uint32_t CTRL2_0060;
	volatile uint32_t CTRL2_0061;
	volatile uint32_t CTRL2_0062;
	volatile uint32_t CTRL2_0063;
	volatile uint32_t CTRL2_0064;
	volatile uint32_t CTRL2_0065;
	volatile uint32_t CTRL2_0066;
	volatile uint32_t CTRL2_0067;
	volatile uint32_t CTRL2_0070;
	volatile uint32_t CTRL2_0071;
	volatile uint32_t CTRL2_0072;
	volatile uint32_t CTRL2_0073;
	uint32_t RSVD5[4];
	volatile uint32_t CTRL2_0100;
	volatile uint32_t CTRL2_0101;
	volatile uint32_t CTRL2_0102;
	uint32_t RSVD6[1];
	volatile uint32_t CTRL2_0104;
	volatile uint32_t CTRL2_0105;
	volatile uint32_t CTRL2_0106;
	volatile uint32_t CTRL2_0107;
	uint32_t RSVD7[2];
	volatile uint32_t CTRL2_0112;
	volatile uint32_t CTRL2_0113;
	volatile uint32_t CTRL2_0114;
	volatile uint32_t CTRL2_0115;
	uint32_t RSVD8[2];
	volatile uint32_t CTRL2_0120;
	volatile uint32_t CTRL2_0121;
	volatile uint32_t CTRL2_0122;
	volatile uint32_t CTRL2_0123;
	volatile uint32_t CTRL2_0124;
	volatile uint32_t CTRL2_0125;
	volatile uint32_t CTRL2_0126;
	volatile uint32_t CTRL2_0127;
	volatile uint32_t CTRL2_0130;
	volatile uint32_t CTRL2_0131;
	volatile uint32_t CTRL2_0132;
	uint32_t RSVD9[5];
	volatile uint32_t CTRL2_0140;
	volatile uint32_t CTRL2_0141;
	volatile uint32_t CTRL2_0142;
	volatile uint32_t CTRL2_0143;
	volatile uint32_t CTRL2_0144;
	volatile uint32_t CTRL2_0145;
	volatile uint32_t CTRL2_0146;
	volatile uint32_t CTRL2_0147;
	volatile uint32_t CTRL2_0150;
	volatile uint32_t CTRL2_0151;
	volatile uint32_t CTRL2_0152;
	volatile uint32_t CTRL2_0153;
	volatile uint32_t CTRL2_0154;
	volatile uint32_t CTRL2_0155;
	volatile uint32_t CTRL2_0156;
	volatile uint32_t CTRL2_0157;
	uint32_t RSVD10[1];
	volatile uint32_t CTRL2_0161;
	volatile uint32_t CTRL2_0162;
	uint32_t RSVD11[2];
	volatile uint32_t CTRL2_0165;
	uint32_t RSVD12[2];
	volatile uint32_t CTRL2_0170;
	volatile uint32_t CTRL2_0171;
	uint32_t RSVD13[3];
	volatile uint32_t CTRL2_0175;
	uint32_t RSVD14[2];
	volatile uint32_t CTRL2_0200;
	volatile uint32_t CTRL2_0201;
	volatile uint32_t CTRL2_0202;
	volatile uint32_t CTRL2_0203;
	volatile uint32_t CTRL2_0204;
	volatile uint32_t CTRL2_0205;
	volatile uint32_t CTRL2_0206;
	volatile uint32_t CTRL2_0207;
	uint32_t RSVD15[9];
	volatile uint32_t CTRL2_0221;
	volatile uint32_t CTRL2_0222;
	volatile uint32_t CTRL2_0223;
	volatile uint32_t CTRL2_0224;
	uint32_t RSVD16[1];
	volatile uint32_t CTRL2_0226;
	volatile uint32_t CTRL2_0227;
	uint32_t RSVD17[8];
	volatile uint32_t CTRL2_0240;
	volatile uint32_t CTRL2_0241;
	volatile uint32_t CTRL2_0242;
	volatile uint32_t CTRL2_0243;
	volatile uint32_t CTRL2_0244;
	volatile uint32_t CTRL2_0245;
	volatile uint32_t CTRL2_0246;
	uint32_t RSVD18[5];
	volatile uint32_t CTRL2_0254;
	volatile uint32_t CTRL2_0255;
};

/** @brief GPIO Parallel Input register. 32 GPIO pins per bank */
struct gpio_parin_regs {
	volatile uint32_t PARIN0;
	volatile uint32_t PARIN1;
	volatile uint32_t PARIN2;
	volatile uint32_t PARIN3;
	volatile uint32_t PARIN4;
	volatile uint32_t PARIN5;
};

/** @brief GPIO Parallel Output register. 32 GPIO pins per bank */
struct gpio_parout_regs {
	volatile uint32_t PAROUT0;
	volatile uint32_t PAROUT1;
	volatile uint32_t PAROUT2;
	volatile uint32_t PAROUT3;
	volatile uint32_t PAROUT4;
	volatile uint32_t PAROUT5;
};

/** @brief GPIO Lock registers. 32 GPIO pins per bank. Write-once bits */
struct gpio_lock_regs {
	volatile uint32_t LOCK5;
	volatile uint32_t LOCK4;
	volatile uint32_t LOCK3;
	volatile uint32_t LOCK2;
	volatile uint32_t LOCK1;
	volatile uint32_t LOCK0;
};

/** @brief General Purpose SPI Controller (GPSPI) */
struct gpspi_regs {
	volatile uint32_t BLOCK_EN;
	volatile uint32_t CONTROL;
	volatile uint32_t STATUS;
	volatile uint8_t TX_DATA;
	uint8_t RSVD1[3];
	volatile uint8_t RX_DATA;
	uint8_t RSVD2[3];
	volatile uint32_t CLK_CTRL;
	volatile uint32_t CLK_GEN;
};

/** @brief Hibernation Timer (HTMR) */
struct htmr_regs {
	volatile uint16_t PRLD;
	uint16_t RSVD1[1];
	volatile uint16_t CTRL;
	uint16_t RSVD2[1];
	volatile uint16_t CNT;
	uint16_t RSVD3[1];
};

/** @brief I2C-SMBus with network layer registers.  */
struct i2c_smb_regs {
	volatile uint8_t CTRLSTS;
	uint8_t RSVD1[3];
	volatile uint32_t OWN_ADDR;
	volatile uint8_t I2CDATA;
	uint8_t RSVD2[3];
	volatile uint32_t MCMD;
	volatile uint32_t SCMD;
	volatile uint8_t PEC;
	uint8_t RSVD3[3];
	volatile uint32_t RSHTM;
	volatile uint32_t EXTLEN;
	volatile uint32_t COMPL;
	volatile uint32_t IDLSC;
	volatile uint32_t CFG;
	volatile uint32_t BUSCLK;
	volatile uint32_t BLKID;
	volatile uint32_t BLKREV;
	volatile uint8_t BBCTRL;
	uint8_t RSVD7[3];
	volatile uint32_t CLKSYNC;
	volatile uint32_t DATATM;
	volatile uint32_t TMOUTSC;
	volatile uint8_t SLV_TXB;
	uint8_t RSVD8[3];
	volatile uint8_t SLV_RXB;
	uint8_t RSVD9[3];
	volatile uint8_t MTR_TXB;
	uint8_t RSVD10[3];
	volatile uint8_t MTR_RXB;
	uint8_t RSVD11[3];
	volatile uint32_t FSM;
	volatile uint32_t FSM_SMB;
	volatile uint8_t WAKE_STS;
	uint8_t RSVD12[3];
	volatile uint8_t WAKE_EN;
	uint32_t RSVD13[2];
	volatile uint32_t PROM_ISTS;
	volatile uint32_t PROM_IEN;
	volatile uint32_t PROM_CTRL;
	volatile uint32_t SHADOW_DATA;
}; /* Size = 128(0x80) */

/** @brief 8042 Emulated Keyboard controller. Size = 820(0x334) */
struct kbc_regs {
	volatile uint32_t HOST_AUX_DATA;
	volatile uint32_t KBC_STS_RD;
	uint8_t RSVD1[0x100 - 0x08];
	volatile uint32_t EC_DATA;
	volatile uint32_t EC_KBC_STS;
	volatile uint32_t KBC_CTRL;
	volatile uint32_t EC_AUX_DATA;
	uint32_t RSVD2[1];
	volatile uint32_t PCOBF;
	uint8_t RSVD3[0x0330 - 0x0118];
	volatile uint32_t KBC_PORT92_EN;
};

/** @brief Keyboard scan matrix controller. Size = 24(0x18) */
struct kscan_regs {
	uint32_t RSVD[1];
	volatile uint32_t KSO_SEL;
	volatile uint32_t KSI_IN;
	volatile uint32_t KSI_STS;
	volatile uint32_t KSI_IEN;
	volatile uint32_t EXT_CTRL;
};

/** @brief Breathing-Blinking LED (LED) */
struct led_regs {
	volatile uint32_t CFG;
	volatile uint32_t LIMIT;
	volatile uint32_t DLY;
	volatile uint32_t STEP;
	volatile uint32_t INTRVL;
	volatile uint32_t OUTDLY;
};

/** @brief Mailbox Registers (MBOX) */
struct mbox_regs {
	volatile uint8_t OS_IDX;
	volatile uint8_t OS_DATA;
	uint8_t RSVD1[0x100u - 0x02u];
	volatile uint32_t HOST_TO_EC;
	volatile uint32_t EC_TO_HOST;
	volatile uint32_t SMI_SRC;
	volatile uint32_t SMI_MASK;
	union {
		volatile uint32_t MBX32[8];
		volatile uint8_t MBX8[32];
	};
};

/** @brief BIOS Debug Port 80h and Alias port capture registers. */
struct p80bd_regs {
	volatile uint32_t HDATA;
	uint8_t RSVD1[0x100 - 0x04];
	volatile uint32_t EC_DA;
	volatile uint32_t CONFIG;
	volatile uint32_t STS_IEN;
	volatile uint32_t SNAPSHOT;
	volatile uint32_t CAPTURE;
	uint8_t RSVD2[0x330 - 0x114];
	volatile uint32_t ACTV;
	uint8_t RSVD3[0x400 - 0x334];
	volatile uint8_t ALIAS_HDATA;
	uint8_t RSVD4[0x730 - 0x401];
	volatile uint32_t ALIAS_ACTV;
	uint8_t RSVD5[0x7f0 - 0x734];
	volatile uint32_t ALIAS_BLS;
};

/** @brief Power Control Reset (PCR) */
struct pcr_regs {
	volatile uint32_t SYS_SLP_CTRL;
	volatile uint32_t PROC_CLK_CTRL;
	volatile uint32_t SLOW_CLK_CTRL;
	volatile uint32_t OSC_ID;
	volatile uint32_t PWR_RST_STS;
	volatile uint32_t PWR_RST_CTRL;
	volatile uint32_t SYS_RST;
	volatile uint32_t TURBO_CLK;
	volatile uint32_t TEST20;
	uint32_t RSVD1[3];
	volatile uint32_t SLP_EN[5];
	uint32_t RSVD2[3];
	volatile uint32_t CLK_REQ[5];
	uint32_t RSVD3[3];
	volatile uint32_t RST_EN[5];
	volatile uint32_t RST_EN_LOCK;
	volatile uint32_t VBAT_SRST;
	volatile uint32_t CLK32K_SRC_VTR;
	volatile uint32_t TEST90;
	uint32_t RSVD4[(0x00c0 - 0x0094) / 4];
	volatile uint32_t CNT32K_PER;
	volatile uint32_t CNT32K_PULSE_HI;
	volatile uint32_t CNT32K_PER_MIN;
	volatile uint32_t CNT32K_PER_MAX;
	volatile uint32_t CNT32K_DV;
	volatile uint32_t CNT32K_DV_MAX;
	volatile uint32_t CNT32K_VALID;
	volatile uint32_t CNT32K_VALID_MIN;
	volatile uint32_t CNT32K_CTRL;
	volatile uint32_t CLK32K_MON_ISTS;
	volatile uint32_t CLK32K_MON_IEN;
};

/** @brief PECI controller. Size = 76(0x4c) */
struct peci_regs {
	volatile uint8_t WR_DATA;
	uint8_t RSVD1[3];
	volatile uint8_t RD_DATA;
	uint8_t RSVD2[3];
	volatile uint8_t CONTROL;
	uint8_t RSVD3[3];
	volatile uint8_t STATUS1;
	uint8_t RSVD4[3];
	volatile uint8_t STATUS2;
	uint8_t RSVD5[3];
	volatile uint8_t ERROR;
	uint8_t RSVD6[3];
	volatile uint8_t INT_EN1;
	uint8_t RSVD7[3];
	volatile uint8_t INT_EN2;
	uint8_t RSVD8[3];
	volatile uint8_t OPT_BIT_TIME_LSB;
	uint8_t RSVD9[3];
	volatile uint8_t OPT_BIT_TIME_MSB;
	uint8_t RSVD10[3];
	volatile uint8_t REQ_TMR_LSB;
	uint8_t RSVD11[3];
	volatile uint8_t REQ_TMR_MSB;
	uint8_t RSVD12[3];
	volatile uint8_t BAUD_CTRL;
	uint8_t RSVD13[3];
	uint32_t RSVD14[3];
	volatile uint8_t BLK_ID;
	uint8_t RSVD15[3];
	volatile uint8_t BLK_REV;
	uint8_t RSVD16[3];
	volatile uint8_t SST_CTL1;
	uint8_t RSVD17[3];
};

/** @brief Fast Port92h Registers (PORT92) */
struct port92_regs {
	volatile uint32_t HOST_P92;
	uint8_t RSVD1[0x100u - 0x04u];
	volatile uint32_t GATEA20_CTRL;
	uint32_t RSVD2[1];
	volatile uint32_t SETGA20L;
	volatile uint32_t RSTGA20L;
	uint8_t RSVD3[0x0330ul - 0x0110ul];
	volatile uint32_t ACTV;
};

/** @brief Processor Hot Interface (PROCHOT) */
struct prochot_regs {
	volatile uint32_t CUMUL_COUNT;
	volatile uint32_t DUTY_COUNT;
	volatile uint32_t DUTY_PERIOD;
	volatile uint32_t CTRL_STS;
	volatile uint32_t ASSERT_COUNT;
	volatile uint32_t ASSERT_LIMIT;
};

/** @brief PS/2 controller */
struct ps2_regs {
	volatile uint32_t TRX_BUFF;
	volatile uint32_t CTRL;
	volatile uint32_t STATUS;
};

/** @brief PWM controller */
struct pwm_regs {
	volatile uint32_t COUNT_ON;
	volatile uint32_t COUNT_OFF;
	volatile uint32_t CONFIG;
};

/** @brief QMSPI Local DMA channel registers */
struct qldma_chan {
	volatile uint32_t CTRL;
	volatile uint32_t MSTART;
	volatile uint32_t LEN;
	uint32_t RSVD1[1];
};

/** @brief QMSPI controller. Size = 368(0x170) */
struct qmspi_regs {
	volatile uint32_t MODE;
	volatile uint32_t CTRL;
	volatile uint32_t EXE;
	volatile uint32_t IFCTRL;
	volatile uint32_t STS;
	volatile uint32_t BCNT_STS;
	volatile uint32_t IEN;
	volatile uint32_t BCNT_TRIG;
	volatile uint32_t TX_FIFO;
	volatile uint32_t RX_FIFO;
	volatile uint32_t CSTM;
	uint32_t RSVD1[1];
	volatile uint32_t DESCR[16];
	uint32_t RSVD2[16];
	volatile uint32_t ALIAS_CTRL;
	uint32_t RSVD3[3];
	volatile uint32_t MODE_ALT1;
	uint32_t RSVD4[3];
	volatile uint32_t TM_TAPS;
	volatile uint32_t TM_TAPS_ADJ;
	volatile uint32_t TM_TAPS_CTRL;
	uint32_t RSVD5[9];
	volatile uint32_t LDMA_RX_DESCR_BM;
	volatile uint32_t LDMA_TX_DESCR_BM;
	uint32_t RSVD6[2];
	struct qldma_chan LDRX[3];
	struct qldma_chan LDTX[3];
};

/** @brief RPM to PWM Fan Registers (RPMFAN) */
struct rpmfan_regs {
	volatile uint16_t FAN_SETTING;
	volatile uint16_t FAN_CONFIG;
	volatile uint8_t PWM_DIVIDE;
	volatile uint8_t GAIN;
	volatile uint8_t FSPU_CFG;
	volatile uint8_t FAN_STEP;
	volatile uint8_t FAN_MIN_DRV;
	volatile uint8_t VALID_TACH_CNT;
	volatile uint16_t FAN_DFB;
	volatile uint16_t TACH_TARGET;
	volatile uint16_t TACH_READING;
	volatile uint8_t PWM_DBF;
	volatile uint8_t FAN_STATUS;
	uint8_t RSVD1[6];
};

/** @brief RTOS Timer (RTMR) */
struct rtmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint32_t CTRL;
	volatile uint32_t SOFTIRQ;
};

/** @brief Tachometer Registers (TACH) */
struct tach_regs {
	volatile uint32_t CONTROL;
	volatile uint32_t STATUS;
	volatile uint32_t LIMIT_HI;
	volatile uint32_t LIMIT_LO;
};

/** @brief Trace FIFO Debug Port Registers (TFDP) */
struct tfdp_regs {
	volatile uint8_t DATA_OUT;
	uint8_t RSVD1[3];
	volatile uint32_t CTRL;
};

/** @brief 16550 compatible UART. Size = 1012(0x3f4) */
struct uart_regs {
	volatile uint8_t RTXB;
	volatile uint8_t IER;
	volatile uint8_t IIR_FCR;
	volatile uint8_t LCR;
	volatile uint8_t MCR;
	volatile uint8_t LSR;
	volatile uint8_t MSR;
	volatile uint8_t SCR;
	uint8_t RSVDA[0x330 - 0x08];
	volatile uint8_t ACTV;
	uint8_t RSVDB[0x3f0 - 0x331];
	volatile uint8_t CFG_SEL;
};

/** @brief VBAT Register Bank (VBATR) */
struct vbatr_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	uint32_t RSVD2[5];
	volatile uint32_t MCNT_LO;
	volatile uint32_t MCNT_HI;
	uint32_t RSVD3[3];
	volatile uint32_t EMBRD_EN;
};

/** @brief VBAT powered control interface (VCI) */
struct vci_regs {
	volatile uint32_t CONFIG;
	volatile uint32_t LATCH_EN;
	volatile uint32_t LATCH_RST;
	volatile uint32_t INPUT_EN;
	volatile uint32_t HOLD_OFF;
	volatile uint32_t POLARITY;
	volatile uint32_t PEDGE_DET;
	volatile uint32_t NEDGE_DET;
	volatile uint32_t BUFFER_EN;
};

/** @brief Watchdog timer. Size = 24(0x18) */
struct wdt_regs {
	volatile uint16_t LOAD;
	uint8_t RSVD1[2];
	volatile uint16_t CTRL;
	uint8_t RSVD2[2];
	volatile uint8_t KICK;
	uint8_t RSVD3[3];
	volatile uint16_t CNT;
	uint8_t RSVD4[2];
	volatile uint16_t STS;
	uint8_t RSVD5[2];
	volatile uint8_t IEN;
};

/* eSPI */
struct espi_io_mbar { /* 80-bit register */
	volatile uint16_t  LDN_MASK;
	volatile uint16_t  RESERVED[4];
}; /* Size = 10 (0xa) */

struct espi_mbar_host {
	volatile uint16_t  VALID;
	volatile uint16_t  HADDR_LSH;
	volatile uint16_t  HADDR_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_bar {
	volatile uint16_t  VACCSZ;
	volatile uint16_t  EC_SRAM_BASE_LSH;
	volatile uint16_t  EC_SRAM_BASE_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

struct espi_sram_host_bar {
	volatile uint16_t  ACCSZ;
	volatile uint16_t  HBASE_LSH;
	volatile uint16_t  HBASE_MSH;
	volatile uint16_t  RESERVED[2];
}; /* Size = 10 (0xa) */

/** @brief eSPI Capabilities, I/O and Memory components in one structure */
struct espi_iom_regs { /* @ 0x400F3400 */
	volatile uint8_t   RTIDX;		/* @ 0x0000 */
	volatile uint8_t   RTDAT;		/* @ 0x0001 */
	volatile uint16_t  RESERVED;
	volatile uint32_t  RESERVED1[63];
	volatile uint32_t  PCLC[3];		/* @ 0x0100 */
	volatile uint32_t  PCERR[2];		/* @ 0x010C */
	volatile uint32_t  PCSTS;		/* @ 0x0114 */
	volatile uint32_t  PCIEN;		/* @ 0x0118 */
	volatile uint32_t  RESERVED2;
	volatile uint32_t  PCBINH[2];		/* @ 0x0120 */
	volatile uint32_t  PCBINIT;		/* @ 0x0128 */
	volatile uint32_t  PCECIRQ;		/* @ 0x012C */
	volatile uint32_t  PCCKNP;		/* @ 0x0130 */
	volatile uint32_t  PCBARI[29];		/* @ 0x0134 */
	volatile uint32_t  RESERVED3[30];
	volatile uint32_t  PCLTRSTS;		/* @ 0x0220 */
	volatile uint32_t  PCLTREN;		/* @ 0x0224 */
	volatile uint32_t  PCLTRCTL;		/* @ 0x0228 */
	volatile uint32_t  PCLTRM;		/* @ 0x022C */
	volatile uint32_t  RESERVED4[4];
	volatile uint32_t  OOBRXA[2];		/* @ 0x0240 */
	volatile uint32_t  OOBTXA[2];		/* @ 0x0248 */
	volatile uint32_t  OOBRXL;		/* @ 0x0250 */
	volatile uint32_t  OOBTXL;		/* @ 0x0254 */
	volatile uint32_t  OOBRXC;		/* @ 0x0258 */
	volatile uint32_t  OOBRXIEN;		/* @ 0x025C */
	volatile uint32_t  OOBRXSTS;		/* @ 0x0260 */
	volatile uint32_t  OOBTXC;		/* @ 0x0264 */
	volatile uint32_t  OOBTXIEN;		/* @ 0x0268 */
	volatile uint32_t  OOBTXSTS;		/* @ 0x026C */
	volatile uint32_t  RESERVED5[4];
	volatile uint32_t  FCFA[2];		/* @ 0x0280 */
	volatile uint32_t  FCBA[2];		/* @ 0x0288 */
	volatile uint32_t  FCLEN;		/* @ 0x0290 */
	volatile uint32_t  FCCTL;		/* @ 0x0294 */
	volatile uint32_t  FCIEN;		/* @ 0x0298 */
	volatile uint32_t  FCCFG;		/* @ 0x029C */
	volatile uint32_t  FCSTS;		/* @ 0x02A0 */
	volatile uint32_t  RESERVED6[3];
	volatile uint32_t  VWSTS;		/* @ 0x02B0 */
	volatile uint32_t  RESERVED7[11];
	volatile uint8_t   CAPID;		/* @ 0x02E0 */
	volatile uint8_t   CAP0;		/* @ 0x02E1 */
	volatile uint8_t   CAP1;		/* @ 0x02E2 */
	volatile uint8_t   CAPPC;		/* @ 0x02E3 */
	volatile uint8_t   CAPVW;		/* @ 0x02E4 */
	volatile uint8_t   CAPOOB;		/* @ 0x02E5 */
	volatile uint8_t   CAPFC;		/* @ 0x02E6 */
	volatile uint8_t   PCRDY;		/* @ 0x02E7 */
	volatile uint8_t   OOBRDY;		/* @ 0x02E8 */
	volatile uint8_t   FCRDY;		/* @ 0x02E9 */
	volatile uint8_t   ERIS;		/* @ 0x02EA */
	volatile uint8_t   ERIE;		/* @ 0x02EB */
	volatile uint8_t   PLTSRC;		/* @ 0x02EC */
	volatile uint8_t   VWRDY;		/* @ 0x02ED */
	volatile uint8_t   SAFEBS;		/* @ 0x02EE */
	volatile uint8_t   RESERVED8;
	volatile uint32_t  RESERVED9[16];
	volatile uint32_t  ACTV;		/* @ 0x0330 */
	volatile uint32_t  IOHBAR[29];		/* @ 0x0334 */
	volatile uint32_t  RESERVED10;
	volatile uint8_t   SIRQ[19];		/* @ 0x03ac */
	volatile uint8_t   RESERVED11;
	volatile uint32_t  RESERVED12[12];
	volatile uint32_t  VWERREN;		/* @ 0x03f0 */
	volatile uint32_t  RESERVED13[79];
	struct espi_io_mbar MBAR[10];		/* @ 0x0530 */
	volatile uint32_t  RESERVED14[6];
	struct espi_sram_bar SRAMBAR[2];	/* @ 0x05AC */
	volatile uint32_t  RESERVED15[16];
	volatile uint32_t  BM_STATUS;		/* @ 0x0600 */
	volatile uint32_t  BM_IEN;		/* @ 0x0604 */
	volatile uint32_t  BM_CONFIG;		/* @ 0x0608 */
	volatile uint32_t  RESERVED16;
	volatile uint32_t  BM_CTRL1;		/* @ 0x0610 */
	volatile uint32_t  BM_HADDR1_LSW;	/* @ 0x0614 */
	volatile uint32_t  BM_HADDR1_MSW;	/* @ 0x0618 */
	volatile uint32_t  BM_EC_ADDR1_LSW;	/* @ 0x061C */
	volatile uint32_t  BM_EC_ADDR1_MSW;	/* @ 0x0620 */
	volatile uint32_t  BM_CTRL2;		/* @ 0x0624 */
	volatile uint32_t  BM_HADDR2_LSW;	/* @ 0x0628 */
	volatile uint32_t  BM_HADDR2_MSW;	/* @ 0x062C */
	volatile uint32_t  BM_EC_ADDR2_LSW;	/* @ 0x0630 */
	volatile uint32_t  BM_EC_ADDR2_MSW;	/* @ 0x0634 */
	volatile uint32_t  RESERVED17[62];
	struct espi_mbar_host HMBAR[10];	/* @ 0x0730 */
	volatile uint32_t  RESERVED18[6];
	struct espi_sram_host_bar HSRAMBAR[2];	/* @ 0x07AC */
}; /* Size = 1984 (0x7c0) */

/** @brief eSPI 96-bit Host-to-Target Virtual Wire register */
struct espi_msvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t MTOS;
	uint8_t RSVD1[2];
	volatile uint32_t SRC_IRQ_SEL;
	volatile uint32_t SRC;
};

/** @brief eSPI 96-bit Host-to-Target Virtual Wire register as bytes */
struct espi_msvwb_reg {
	volatile uint8_t HTVWB[12];
};

/** @brief HW implements 11 Host-to-Target VW registers as an array */
struct espi_msvw_ar_regs {
	struct espi_msvw_reg MSVW[11];
};

/** @brief HW implements 11 Host-to-Target VW registers as named registers */
struct espi_msvw_named_regs {
	struct espi_msvw_reg MSVW00;
	struct espi_msvw_reg MSVW01;
	struct espi_msvw_reg MSVW02;
	struct espi_msvw_reg MSVW03;
	struct espi_msvw_reg MSVW04;
	struct espi_msvw_reg MSVW05;
	struct espi_msvw_reg MSVW06;
	struct espi_msvw_reg MSVW07;
	struct espi_msvw_reg MSVW08;
	struct espi_msvw_reg MSVW09;
	struct espi_msvw_reg MSVW10;
};

/** @brief eSPI M2S VW registers as an array of words at 0x400F9C00 */
struct espi_msvw32_regs {
	volatile uint32_t MSVW32[11 * 3];
};

/** @brief eSPI 64-bit Target-to-Host Virtual Wire register */
struct espi_smvw_reg {
	volatile uint8_t INDEX;
	volatile uint8_t STOM;
	volatile uint8_t SRC_CHG;
	uint8_t RSVD1[1];
	volatile uint32_t SRC;
};

/** @brief eSPI 64-bit Target-to-Host Virtual Wire register as bytes */
struct espi_smvwb_reg {
	volatile uint8_t THVWB[8];
};


/** @brief HW implements 11 Target-to-Host VW registers as an array */
struct espi_smvw_ar_regs {
	struct espi_smvw_reg SMVW[11];
};

/** @brief HW implements 11 Target-to-Host VW registers as named registers */
struct espi_smvw_named_regs {
	struct espi_smvw_reg SMVW00;
	struct espi_smvw_reg SMVW01;
	struct espi_smvw_reg SMVW02;
	struct espi_smvw_reg SMVW03;
	struct espi_smvw_reg SMVW04;
	struct espi_smvw_reg SMVW05;
	struct espi_smvw_reg SMVW06;
	struct espi_smvw_reg SMVW07;
	struct espi_smvw_reg SMVW08;
	struct espi_smvw_reg SMVW09;
	struct espi_smvw_reg SMVW10;
};

/** @brief eSPI S2M VW registers as an array of words at 0x400F9E00 */
struct espi_smvw32_regs {
	volatile uint32_t SMVW[11 * 2];
};

/* eSPI SAF */
/** @brief SAF SPI Opcodes and descriptor indices */
struct mchp_espi_saf_op {
	volatile uint32_t OPA;
	volatile uint32_t OPB;
	volatile uint32_t OPC;
	volatile uint32_t OP_DESCR;
};

/** @brief SAF protection regions contain 4 32-bit registers. */
struct mchp_espi_saf_pr {
	volatile uint32_t START;
	volatile uint32_t LIMIT;
	volatile uint32_t WEBM;
	volatile uint32_t RDBM;
};

/** @brief eSPI SAF configuration and control registers at 0x40008000 */
struct mchp_espi_saf {
	uint32_t RSVD1[6];
	volatile uint32_t SAF_ECP_CMD;
	volatile uint32_t SAF_ECP_FLAR;
	volatile uint32_t SAF_ECP_START;
	volatile uint32_t SAF_ECP_BFAR;
	volatile uint32_t SAF_ECP_STATUS;
	volatile uint32_t SAF_ECP_INTEN;
	volatile uint32_t SAF_FL_CFG_SIZE_LIM;
	volatile uint32_t SAF_FL_CFG_THRH;
	volatile uint32_t SAF_FL_CFG_MISC;
	volatile uint32_t SAF_ESPI_MON_STATUS;
	volatile uint32_t SAF_ESPI_MON_INTEN;
	volatile uint32_t SAF_ECP_BUSY;
	uint32_t RSVD2[1];
	struct mchp_espi_saf_op SAF_CS_OP[2];
	volatile uint32_t SAF_FL_CFG_GEN_DESCR;
	volatile uint32_t SAF_PROT_LOCK;
	volatile uint32_t SAF_PROT_DIRTY;
	volatile uint32_t SAF_TAG_MAP[3];
	struct mchp_espi_saf_pr SAF_PROT_RG[17];
	volatile uint32_t SAF_POLL_TMOUT;
	volatile uint32_t SAF_POLL_INTRVL;
	volatile uint32_t SAF_SUS_RSM_INTRVL;
	volatile uint32_t SAF_CONSEC_RD_TMOUT;
	volatile uint16_t SAF_CS0_CFG_P2M;
	volatile uint16_t SAF_CS1_CFG_P2M;
	volatile uint32_t SAF_FL_CFG_SPM;
	volatile uint32_t SAF_SUS_CHK_DLY;
	volatile uint16_t SAF_CS0_CM_PRF;
	volatile uint16_t SAF_CS1_CM_PRF;
	volatile uint32_t SAF_DNX_PROT_BYP;
};

struct mchp_espi_saf_comm { /* @ 0x40071000 */
	uint32_t TEST0;
	uint32_t TEST1;
	uint32_t TEST2;
	uint32_t TEST3;
	uint32_t TEST4;
	uint32_t TEST5;
	uint32_t TEST6;
	uint32_t RSVD1[(0x2b8 - 0x01c) / 4];
	uint32_t SAF_COMM_MODE; /* @ 0x400712b8 */
	uint32_t TEST7;
};

/* AHB Peripheral base address */
#define PERIPH_BASE		0x40000000u

/*
 * Delay register address. Write n to delay for n + 1 microseconds where
 * 0 <= n <= 31.
 * Implementation stalls the CPU fetching instructions including blocking
 * interrupts.
 */
#define DELAY_US_BASE			0x08000000u

/* ARM Cortex-M4 input clock from PLL */
#define MCHP_EC_CLOCK_INPUT_HZ		96000000u

#define MCHP_ACMP_INSTANCES		1
#define MCHP_ACPI_EC_INSTANCES		5
#define MCHP_ACPI_PM1_INSTANCES		1
#define MCHP_ADC_INSTANCES		1
#define MCHP_BCL_INSTANCES		1
#define MCHP_BTMR16_INSTANCES		4
#define MCHP_BTMR32_INSTANCES		2
#define MCHP_CCT_INSTANCES		1
#define MCHP_CTMR_INSTANCES		4
#define MCHP_DMA_INSTANCES		1
#define MCHP_ECIA_INSTANCES		1
#define MCHP_EMI_INSTANCES		3
#define MCHP_HTMR_INSTANCES		2
#define MCHP_I2C_INSTANCES		0
#define MCHP_I2C_SMB_INSTANCES		5
#define MCHP_LED_INSTANCES		4
#define MCHP_MBOX_INSTANCES		1
#define MCHP_OTP_INSTANCES		1
#define MCHP_P80BD_INSTANCES		1
#define MCHP_PECI_INSTANCES		1
#define MCHP_PROCHOT_INSTANCES		1
#define MCHP_PS2_INSTANCES		1
#define MCHP_PWM_INSTANCES		9
#define MCHP_QMSPI_INSTANCES		1
#define MCHP_RCID_INSTANCES		3
#define MCHP_RPMFAN_INSTANCES		2
#define MCHP_RTC_INSTANCES		1
#define MCHP_RTMR_INSTANCES		1
#define MCHP_SPIP_INSTANCES		1
#define MCHP_TACH_INSTANCES		4
#define MCHP_TFDP_INSTANCES		1
#define MCHP_UART_INSTANCES		2
#define MCHP_WDT_INSTANCES		1
#define MCHP_WKTMR_INSTANCES		1

#define MCHP_ACMP_CHANNELS		2
#define MCHP_ADC_CHANNELS		8
#define MCHP_BGPO_GPIO_PINS		2
#define MCHP_DMA_CHANNELS		16
#define MCHP_ESPI_SAF_TAGMAP_MAX	3
#define MCHP_GIRQS			19
#define MCHP_GPIO_PINS			123
#define MCHP_GPIO_PORTS			6
#define MCHP_GPTP_PORTS			6
#define MCHP_I2C_SMB_PORTS		15
#define MCHP_I2C_PORTMAP		0xf7ffu
#define MCHP_QMSPI_PORTS		3
#define MCHP_PS2_PORTS			2
#define MCHP_VCI_IN_PINS		4
#define MCHP_VCI_OUT_PINS		1
#define MCHP_VCI_OVRD_IN_PINS		1

#define SHLU32(v, n)			((uint32_t)(v) << (n))

#ifdef __cplusplus
}
#endif
#endif /* MEC172X_REGS_H */
