/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

static int soc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* CDCG register structure check */
NPCX_REG_SIZE_CHECK(cdcg_reg, 0x116);
NPCX_REG_OFFSET_CHECK(cdcg_reg, HFCBCD, 0x010);
NPCX_REG_OFFSET_CHECK(cdcg_reg, HFCBCD2, 0x014);
NPCX_REG_OFFSET_CHECK(cdcg_reg, LFCGCTL, 0x100);
NPCX_REG_OFFSET_CHECK(cdcg_reg, LFCGCTL2, 0x114);

/* PMC register structure check */
NPCX_REG_SIZE_CHECK(pmc_reg, 0x025);
NPCX_REG_OFFSET_CHECK(pmc_reg, ENIDL_CTL, 0x003);
NPCX_REG_OFFSET_CHECK(pmc_reg, PWDWN_CTL1, 0x008);
NPCX_REG_OFFSET_CHECK(pmc_reg, PWDWN_CTL7, 0x024);

/* SCFG register structure check */
NPCX_REG_SIZE_CHECK(scfg_reg, 0x02F);
NPCX_REG_OFFSET_CHECK(scfg_reg, DEV_CTL4, 0x006);
NPCX_REG_OFFSET_CHECK(scfg_reg, DEVALT0, 0x010);
NPCX_REG_OFFSET_CHECK(scfg_reg, LV_GPIO_CTL0, 0x02a);

/* GLUE register structure check */
NPCX_REG_SIZE_CHECK(glue_reg, 0x028);
NPCX_REG_OFFSET_CHECK(glue_reg, SMB_EEN, 0x003);
NPCX_REG_OFFSET_CHECK(glue_reg, SDPD0, 0x010);
NPCX_REG_OFFSET_CHECK(glue_reg, SMB_SEL, 0x021);
NPCX_REG_OFFSET_CHECK(glue_reg, PSL_CTS, 0x027);

/* UART register structure check */
NPCX_REG_SIZE_CHECK(uart_reg, 0x027);
NPCX_REG_OFFSET_CHECK(uart_reg, UPSR, 0x00e);
NPCX_REG_OFFSET_CHECK(uart_reg, UFTSTS, 0x020);
NPCX_REG_OFFSET_CHECK(uart_reg, UFRCTL, 0x026);

/* GPIO register structure check */
NPCX_REG_SIZE_CHECK(gpio_reg, 0x008);
NPCX_REG_OFFSET_CHECK(gpio_reg, PLOCK_CTL, 0x007);

/* PWM register structure check */
NPCX_REG_SIZE_CHECK(pwm_reg, 0x00e);
NPCX_REG_OFFSET_CHECK(pwm_reg, PWMCTL, 0x004);
NPCX_REG_OFFSET_CHECK(pwm_reg, DCR, 0x006);
NPCX_REG_OFFSET_CHECK(pwm_reg, PWMCTLEX, 0x00c);

/* ADC register structure check */
NPCX_REG_SIZE_CHECK(adc_reg, 0x54);
NPCX_REG_OFFSET_CHECK(adc_reg, THRCTL1, 0x014);
NPCX_REG_OFFSET_CHECK(adc_reg, ADCCNF2, 0x020);
NPCX_REG_OFFSET_CHECK(adc_reg, CHNDAT, 0x040);

/* TWD register structure check */
NPCX_REG_SIZE_CHECK(twd_reg, 0x012);
NPCX_REG_OFFSET_CHECK(twd_reg, T0CSR, 0x006);
NPCX_REG_OFFSET_CHECK(twd_reg, TWMWD, 0x00e);
NPCX_REG_OFFSET_CHECK(twd_reg, WDCP, 0x010);

/* ESPI register structure check */
NPCX_REG_SIZE_CHECK(espi_reg, 0x500);
NPCX_REG_OFFSET_CHECK(espi_reg, FLASHCFG, 0x034);
NPCX_REG_OFFSET_CHECK(espi_reg, VWEVMS, 0x140);
NPCX_REG_OFFSET_CHECK(espi_reg, VWCTL, 0x2fc);
NPCX_REG_OFFSET_CHECK(espi_reg, OOBTXBUF, 0x380);
NPCX_REG_OFFSET_CHECK(espi_reg, OOBCTL_DIRECT, 0x3fc);
NPCX_REG_OFFSET_CHECK(espi_reg, FLASHTXBUF, 0x480);
NPCX_REG_OFFSET_CHECK(espi_reg, FLASHCTL_DIRECT, 0x4fc);

/* MSWC register structure check */
NPCX_REG_SIZE_CHECK(mswc_reg, 0x030);
NPCX_REG_OFFSET_CHECK(mswc_reg, HCBAL, 0x008);
NPCX_REG_OFFSET_CHECK(mswc_reg, HCBAH, 0x00a);
NPCX_REG_OFFSET_CHECK(mswc_reg, SRID_CR, 0x01c);
NPCX_REG_OFFSET_CHECK(mswc_reg, SID_CR, 0x020);
NPCX_REG_OFFSET_CHECK(mswc_reg, VW_SLPST1, 0x02e);

/* SHM register structure check */
NPCX_REG_SIZE_CHECK(shm_reg, 0x050);
NPCX_REG_OFFSET_CHECK(shm_reg, IMA_WIN_SIZE, 0x005);
NPCX_REG_OFFSET_CHECK(shm_reg, WIN_SIZE, 0x007);
NPCX_REG_OFFSET_CHECK(shm_reg, IMA_SEM, 0x00b);
NPCX_REG_OFFSET_CHECK(shm_reg, SHCFG, 0x00e);
NPCX_REG_OFFSET_CHECK(shm_reg, WIN1_WR_PROT, 0x010);
NPCX_REG_OFFSET_CHECK(shm_reg, IMA_WR_PROT, 0x016);
NPCX_REG_OFFSET_CHECK(shm_reg, WIN_BASE1, 0x020);
NPCX_REG_OFFSET_CHECK(shm_reg, WIN_BASE2, 0x024);
NPCX_REG_OFFSET_CHECK(shm_reg, RST_CFG, 0x03a);
NPCX_REG_OFFSET_CHECK(shm_reg, DP80BUF, 0x040);
NPCX_REG_OFFSET_CHECK(shm_reg, DP80CTL, 0x044);
NPCX_REG_OFFSET_CHECK(shm_reg, HOFS_STS, 0x048);
NPCX_REG_OFFSET_CHECK(shm_reg, COFS1, 0x04C);

/* KBC register structure check */
NPCX_REG_SIZE_CHECK(kbc_reg, 0x00c);
NPCX_REG_OFFSET_CHECK(kbc_reg, HIKMDI, 0x00a);
NPCX_REG_OFFSET_CHECK(kbc_reg, SHIKMDI, 0x00b);

/* PMCH register structure check */
NPCX_REG_SIZE_CHECK(pmch_reg, 0x012);
NPCX_REG_OFFSET_CHECK(pmch_reg, HIPMDO, 0x002);
NPCX_REG_OFFSET_CHECK(pmch_reg, HIPMDOC, 0x006);
NPCX_REG_OFFSET_CHECK(pmch_reg, HIPMDOM, 0x008);
NPCX_REG_OFFSET_CHECK(pmch_reg, HIPMDIC, 0x00a);
NPCX_REG_OFFSET_CHECK(pmch_reg, HIPMIE, 0x010);

/* C2H register structure check */
NPCX_REG_SIZE_CHECK(c2h_reg, 0x00c);
NPCX_REG_OFFSET_CHECK(c2h_reg, LKSIOHA, 0x004);
NPCX_REG_OFFSET_CHECK(c2h_reg, CRSMAE, 0x008);
NPCX_REG_OFFSET_CHECK(c2h_reg, SIBCTRL, 0x00a);

/* SMB register structure check */
NPCX_REG_SIZE_CHECK(smb_reg, 0x020);
NPCX_REG_OFFSET_CHECK(smb_reg, SMBCTL1, 0x006);
NPCX_REG_OFFSET_CHECK(smb_reg, SMBT_OUT, 0x00f);
NPCX_REG_OFFSET_CHECK(smb_reg, SMBADDR6, 0x016);
NPCX_REG_OFFSET_CHECK(smb_reg, SMBSCLHT, 0x01e);

NPCX_REG_SIZE_CHECK(smb_fifo_reg, 0x020);
NPCX_REG_OFFSET_CHECK(smb_fifo_reg, SMBT_OUT, 0x00f);
NPCX_REG_OFFSET_CHECK(smb_fifo_reg, SMBCST2, 0x018);
NPCX_REG_OFFSET_CHECK(smb_fifo_reg, SMBTXF_STS, 0x01a);
NPCX_REG_OFFSET_CHECK(smb_fifo_reg, SMBRXF_CTL, 0x01e);
