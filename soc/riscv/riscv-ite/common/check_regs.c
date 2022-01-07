/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>

/* EC2I register structure check */
IT8XXX2_REG_SIZE_CHECK(ec2i_regs, 0x06);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IHIOA, 0x00);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IHD, 0x01);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, LSIOHA, 0x02);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IBMAE, 0x04);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IBCTL, 0x05);

/* KBC register structure check */
IT8XXX2_REG_SIZE_CHECK(kbc_regs, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHICR, 0x00);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBIRQR, 0x02);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHISR, 0x04);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIKDOR, 0x06);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIMDOR, 0x08);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIDIR, 0x0a);

/* PMC register structure check */
IT8XXX2_REG_SIZE_CHECK(pmc_regs, 0x100);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1STS, 0x00);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1DO, 0x01);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1DI, 0x04);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1CTL, 0x06);

/* eSPI slave register structure check */
IT8XXX2_REG_SIZE_CHECK(espi_slave_regs, 0xd8);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, GCAPCFG1, 0x05);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_PC_CAPCFG3, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_VW_CAPCFG3, 0x0f);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_OOB_CAPCFG3, 0x13);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG3, 0x17);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG2_3, 0x1b);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPCTRL0, 0x90);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL0, 0xa0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL1, 0xa1);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL2, 0xa2);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESUCTRL0, 0xb0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL0, 0xc0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL1, 0xc1);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC0, 0xd0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC7, 0xd7);

/* eSPI vw register structure check */
IT8XXX2_REG_SIZE_CHECK(espi_vw_regs, 0x9a);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VW_INDEX, 0x00);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL0, 0x90);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL1, 0x91);

/* eSPI Queue 0 registers structure check */
IT8XXX2_REG_SIZE_CHECK(espi_queue0_regs, 0xd0);
IT8XXX2_REG_OFFSET_CHECK(espi_queue0_regs, PUT_OOB_DATA, 0x80);

/* eSPI Queue 1 registers structure check */
IT8XXX2_REG_SIZE_CHECK(espi_queue1_regs, 0xc0);
IT8XXX2_REG_OFFSET_CHECK(espi_queue1_regs, UPSTREAM_DATA, 0x00);
IT8XXX2_REG_OFFSET_CHECK(espi_queue1_regs, PUT_FLASH_NP_DATA, 0x80);

/* GCTRL register structure check */
IT8XXX2_REG_SIZE_CHECK(gctrl_it8xxx2_regs, 0x88);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTS, 0x06);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_BADRSEL, 0x0a);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_WNCKR, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_SPCTRL1, 0x0d);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_SPCTRL4, 0x1c);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTC5, 0x21);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_MCCR2, 0x44);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_P80H81HSR, 0x50);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_P80HDR, 0x51);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_H2ROFSR, 0x53);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_ECHIPID2, 0x86);

/* KSCAN register structure check */
IT8XXX2_REG_SIZE_CHECK(kscan_it8xxx2_regs, 0x0F);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOL, 0x00);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOCTRL, 0x02);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSI, 0x04);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSIGDAT, 0x08);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOLGOEN, 0x0e);
