/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_host_sub

/**
 * @file
 * @brief Nuvoton NCT host sub modules driver
 *
 * This file contains the drivers of NCT Host Sub-Modules that serve as an
 * interface between the Host and Core domains. Please refer the block diagram.
 *
 *                                        +------------+
 *                                        |   KCS/PM   |<--->|
 *                                  +<--->|   Channels |     |
 *                                  |     +------------+     |
 *                                  |     +------------+     |
 *                +------------+    |<--->|    Core    |<--->|
 *   eSPI_CLK --->|  eSPI Bus  |    |     |   to Host  |     |
 *   eSPI_RST --->| Controller |    |     +------------+     |
 * eSPI_IO3-0 <-->|            |<-->|     +------------+     |
 *    eSPI_CS --->| (eSPI mode)|    |     |   Shared   |     |
 * eSPI_ALERT <-->|            |    |<--->|   Memory   |<--->|
 *                +------------+    |     +------------+     |
 *                                  |     +------------+     |
 *                                  |<--->|    MSWC    |<--->|
 *                                  |     +------------+     |
 *                                  |                        |
 *                                  |                        |
 *                                  |                        |
 *                                  |                        |
 *                                  |                        |
 *                                HMIB                       | Core Bus
 *                     (Host Modules Internal Bus)           +------------
 *
 *
 * For most of them, the Host can configure these modules via eSPI(Peripheral
 * Channel)/LPC by accessing 'Configuration and Control register Set' which IO
 * base address is 0x2E as default. (The table below illustrates structure of
 * 'Configuration and Control Register Set') And the interrupts in core domain
 * help handling any events from host side.
 *
 *   Index |     Configuration and Control Register Set
 * --------|--------------------------------------------------+   Bank Select
 *    07h  |      Logical Device Number Register (LDN)        |---------+
 * --------|---------------------------------------------------         |
 *  20-2Fh |        SuperI/O Configuration Registers          |         |
 * ------------------------------------------------------------         |
 * --------|---------------------------------------------------_        |
 *    30h  |      Logical Device Control Register             | |_      |
 * --------|--------------------------------------------------- | |_    |
 *  60-63h |   I/O Space Configuration Registers              | | | |   |
 * --------|--------------------------------------------------- | | |   |
 *  70-71h |     Interrupt Configuration Registers            | | | |   |
 * --------|--------------------------------------------------- | | |<--+
 *  F0-FFh | Special Logical Device Configuration Registers   | | | |
 * --------|--------------------------------------------------- | | |
 *           |--------------------------------------------------- | |
 *             |--------------------------------------------------- |
 *               |---------------------------------------------------
 *
 *
 * This driver introduces four host sub-modules. It includes:
 *
 * 1. KCS/Power Management (PM) channels.
 *   嚙踝蕭嚙?KCS/PM channel registers
 *     嚙踝蕭 Command/Status register
 *     嚙踝蕭 Data register
 *       channel 1: legacy 62h, 66h; channel 2: legacy 68h, 6Ch
 *       (Zephyr setting: 200h, 204h);
 *       channel 3: legacy 6Ah, 6Eh; channel 4: legacy 6Bh, 6Fh;
 *   嚙踝蕭嚙?KCS/PM interrupt using:
 *     嚙踝蕭 Serial IRQ
 *     嚙踝蕭 SMI
 *     嚙踝蕭 EC_SCI
 *   嚙踝蕭嚙?Configured by four logical devices: KCS/PM1/2/3/4 (LDN 0x11/0x12/0x17/0x1E)
 *
 * 2. Shared Memory mechanism (SHM).
 *   This module allows sharing of the on-chip RAM by both Core and the Host.
 *   It also supports the following features:
 *   嚙踝蕭嚙?Four Core/Host communication windows for direct RAM access
 *   嚙踝蕭嚙?Eight Protection regions for each access window
 *   嚙踝蕭嚙?Host IRQ and SMI generation
 *   嚙踝蕭嚙?Port 80 debug support
 *   嚙踝蕭嚙?Configured by one logical device: SHM (LDN 0x0F)
 *
 * 3. Core Access to Host Modules (C2H).
 *   嚙踝蕭嚙?A interface to access module registers in host domain.
 *     It enables the Core to access the registers in host domain (i.e., Host
 *     Configuration, Serial Port, SHM, and MSWC), through HMIB.
 *
 * 4. Mobile System Wake-Up functions (MSWC).
 *   It detects and handles wake-up events from various sources in the Host
 *   modules and alerts the Core for better power consumption.
 *
 * INCLUDE FILES: soc_host.h
 *
 */

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include "espi_utils.h"
#include "soc_host.h"
#include "soc_espi.h"
#include "soc_miwu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(host_sub_nct, LOG_LEVEL_ERR);

struct host_sub_nct_config {
	/* host module instances */
	struct mswc_reg *const inst_mswc;
	struct shm_reg *const inst_shm;
	struct c2h_reg *const inst_c2h;
	struct bbram_reg *const inst_bbram;
	struct pmch_reg *const inst_pm_acpi;
	struct pmch_reg *const inst_pm_hcmd;
	struct pmch_reg *const inst_pmch3;
	struct pmch_reg *const inst_pmch4;
	/* clock configuration */
	const uint8_t clks_size;
	const struct nct_clk_cfg *clks_list;
	/* mapping table between host access signals and wake-up input */
	struct nct_wui host_acc_wui;
};

struct host_sub_nct_data {
	sys_slist_t *callbacks; /* pointer on the espi callback list */
	uint8_t plt_rst_asserted; /* current PLT_RST# status */
	uint8_t espi_rst_asserted; /* current ESPI_RST# status */
	const struct device *host_bus_dev; /* device for eSPI/LPC bus */
};

//static const struct nct_clk_cfg host_dev_clk_cfg[] =
//		                         DT_INST_PHA(0, clocks, clk_cfg);

struct host_sub_nct_config host_sub_cfg = {
	.inst_mswc = (struct mswc_reg *)DT_INST_REG_ADDR_BY_NAME(0, mswc),
	.inst_shm = (struct shm_reg *)DT_INST_REG_ADDR_BY_NAME(0, shm),
	.inst_c2h = (struct c2h_reg *)DT_REG_ADDR(DT_NODELABEL(c2h)),
	.inst_bbram = (struct bbram_reg *)DT_INST_REG_ADDR_BY_NAME(0, bbram),
	.inst_pm_acpi = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pm_acpi),
	.inst_pm_hcmd = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pm_hcmd),
	.inst_pmch3 = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pmch3),
	.inst_pmch4 = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pmch4),
	.host_acc_wui = NCT_DT_WUI_ITEM_BY_NAME(0, host_acc_wui),
//	.clks_size = ARRAY_SIZE(host_dev_clk_cfg),
//	.clks_list = host_dev_clk_cfg,
};

struct host_sub_nct_data host_sub_data;

/* IO base address of EC Logical Device Configuration */
#define NCT_EC_CFG_IO_ADDR 0x2E

/* Timeout to wait for Core-to-Host transaction to be completed. */
#define NCT_C2H_TRANSACTION_TIMEOUT_US 200

/* Logical Device Number Assignments */
#define EC_CFG_LDN_SHM   0x0F
#define EC_CFG_LDN_ACPI  0x11 /* KCS/PM Channel 1 */
#define EC_CFG_LDN_HCMD  0x12 /* KCS/PM Channel 2 */
#define EC_CFG_LDN_PMCH3 0x17 /* KCS3/PM Channel 3 */
#define EC_CFG_LDN_PMCH4 0x1E /* KCS4/PM Channel 4 */

/* Index of EC (2E/2F or 4E/4F) Configuration Register */
#define EC_CFG_IDX_LDN             0x07
#define EC_CFG_IDX_CTRL            0x30
#define EC_CFG_IDX_DATA_IO_ADDR_H  0x60
#define EC_CFG_IDX_DATA_IO_ADDR_L  0x61
#define EC_CFG_IDX_CMD_IO_ADDR_H   0x62
#define EC_CFG_IDX_CMD_IO_ADDR_L   0x63

/* Index of Special Logical Device Configuration (Shared Memory Module) */
#define EC_CFG_IDX_SHM_CFG         0xF1
#define EC_CFG_IDX_SHM_WND1_ADDR_0 0xF4
#define EC_CFG_IDX_SHM_WND1_ADDR_1 0xF5
#define EC_CFG_IDX_SHM_WND1_ADDR_2 0xF6
#define EC_CFG_IDX_SHM_WND1_ADDR_3 0xF7
#define EC_CFG_IDX_SHM_WND2_ADDR_0 0xF8
#define EC_CFG_IDX_SHM_WND2_ADDR_1 0xF9
#define EC_CFG_IDX_SHM_WND2_ADDR_2 0xFA
#define EC_CFG_IDX_SHM_WND2_ADDR_3 0xFB

#define EC_C2H_DEV_CFG 0x01
#define EC_C2H_OFFSET_INDEX 0x00
#define EC_C2H_OFFSET_DATA  0x01

typedef enum {
	hs_SHM_WIN1 = 0,
	hs_SHM_WIN2,
	hs_SHM_WIN3,
	hs_SHM_WIN4,
	hs_SHM_WIN5,
	hs_SHM_IMA_WIN1,
	hs_SHM_IMA_WIN2,
} HS_SHM_DEVICE_T;

typedef enum {
	hs_PMCH_ACPI = 0,
	hs_PMCH_HCMD,
	hs_PMCH3,
	hs_PMCH4,
} HS_PMCH_T;

/* Host sub-device local inline functions */
static inline uint8_t host_shd_mem_wnd_size_sl(uint32_t size)
{
	/* The minimum supported shared memory region size is 8 bytes */
	if (size <= 8U) {
		size = 8U;
	}

	/* The maximum supported shared memory region size is 4K bytes */
	if (size >= 4096U) {
		size = 4096U;
	}

	/*
	 * If window size is not a power-of-two, it is rounded-up to the next
	 * power-of-two value, and return value corresponds to RWINx_SIZE field.
	 */
	return (32 - __builtin_clz(size - 1U)) & 0xff;
}

/* MSWC API */
#define MSWC_REG_SMIP_LEN 1
#define MSWC_REG_HOST_CTL 2
#define MSWC_REG_CTL1     3
#define MSWC_REG_LPC_STS  4

void host_mswc_WriteReg(uint8_t index, uint8_t val)
{
	struct mswc_reg *const inst_mswc = host_sub_cfg.inst_mswc;

	switch(index)
	{
	case MSWC_REG_SMIP_LEN:
		inst_mswc->SMIP_LEN = val;
		break;
	case MSWC_REG_HOST_CTL:
		inst_mswc->HOST_CTL = val;
		break;
	case MSWC_REG_CTL1:
		inst_mswc->MSWCTL1 = val;
		break;
	case MSWC_REG_LPC_STS:
		inst_mswc->LPC_STS = val;
		break;
	}
}

uint8_t host_mswc_ReadReg(uint8_t index)
{
	struct mswc_reg *const inst_mswc = host_sub_cfg.inst_mswc;
	uint8_t ret = 0;

	switch(index)
	{
	case MSWC_REG_SMIP_LEN:
		ret = inst_mswc->SMIP_LEN;
		break;
	case MSWC_REG_HOST_CTL:
		ret = inst_mswc->HOST_CTL;
		break;
	case MSWC_REG_CTL1:
		ret = inst_mswc->MSWCTL1;
		break;
	case MSWC_REG_LPC_STS:
		ret = inst_mswc->LPC_STS;
		break;
	}

	return ret;
}

/* BBRAM API */
uint32_t host_bbram_GetBBramSpaceAdr(void)
{
	struct bbram_reg *const inst_bbram = host_sub_cfg.inst_bbram;

	return (uint32_t)inst_bbram->BBRAM_SPACE;
}

bool host_bbram_BKUPSTS_IsSet(uint8_t mask)
{
	struct bbram_reg *const inst_bbram = host_sub_cfg.inst_bbram;
	bool ret;

	if(inst_bbram->BKUP_STS & mask) {ret = true;}
	else {ret = false;}

	return ret;
}

void host_bbram_BKUPSTS_Clear(uint8_t mask)
{
	struct bbram_reg *const inst_bbram = host_sub_cfg.inst_bbram;

	inst_bbram->BKUP_STS = mask;
}

/* Host ACPI sub-device local functions */


#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO) || \
				defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH3) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH4)

static void host_pmch_init(struct pmch_reg *pmch)
{
	struct pmch_reg *const inst_pmch = pmch;

	/* Use SMI/SCI postive polarity by default */
	inst_pmch->HIPMCTL &= ~BIT(NCT_HIPMCTL_SCIPOL);
	inst_pmch->HIPMIC &= ~BIT(NCT_HIPMIC_SMIPOL);

	/* Set SMIB/SCIB bits to make sure SMI#/SCI# are driven to high */
	inst_pmch->HIPMIC |= BIT(NCT_HIPMIC_SMIB) | BIT(NCT_HIPMIC_SCIB);

	/*
	 * Allow SMI#/SCI# generated from PM module.
	 * On eSPI bus, we suggest set VW value of SCI#/SMI# directly.
	 */
	inst_pmch->HIPMIE |= BIT(NCT_HIPMIE_SCIE);
	inst_pmch->HIPMIE |= BIT(NCT_HIPMIE_SMIE);

	/*
	 * Init KCS PM channel (Host IO) with:
	 * 1. Enable Input-Buffer Full (IBF) core interrupt.
	 * 2. BIT 7 must be 1.
	 */
	inst_pmch->HIPMCTL |= BIT(7) | BIT(NCT_HIPMCTL_IBFIE);
}
#endif

typedef void (*host_shm_MBI_CB)(void);
host_shm_MBI_CB host_shm_MBICBFn;

void host_shm_SetWinBaseAddr(uint8_t win, volatile uint8_t* addr)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->WIN_BASE1 = (uint32_t)addr;
			break;
		case hs_SHM_WIN2:
			inst_shm->WIN_BASE2 = (uint32_t)addr;
			break;
		case hs_SHM_WIN3:
			inst_shm->WIN_BASE3 = (uint32_t)addr;
			break;
		case hs_SHM_WIN4:
			inst_shm->WIN_BASE4 = (uint32_t)addr;
			break;
		case hs_SHM_WIN5:
			inst_shm->WIN_BASE5 = (uint32_t)addr;
			break;
	}
}
uint32_t host_shm_GetWrProtect(uint8_t win)
{
	uint32_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			ret = inst_shm->WIN1_WR_PROT;
			break;
		case hs_SHM_WIN2:
			ret = inst_shm->WIN2_WR_PROT;
			break;
		case hs_SHM_WIN3:
			ret = inst_shm->WIN3_WR_PROT;
			break;
		case hs_SHM_WIN4:
			ret = inst_shm->WIN4_WR_PROT;
			break;
		case hs_SHM_WIN5:
		default:
			ret = inst_shm->WIN5_WR_PROT;
			break;
	}
	return ret;
}
void host_shm_SetWrProtect(uint8_t win, uint8_t val)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->WIN1_WR_PROT = val;
			break;
		case hs_SHM_WIN2:
			inst_shm->WIN2_WR_PROT = val;
			break;
		case hs_SHM_WIN3:
			inst_shm->WIN3_WR_PROT = val;
			break;
		case hs_SHM_WIN4:
			inst_shm->WIN4_WR_PROT = val;
			break;
		case hs_SHM_WIN5:
			inst_shm->WIN5_WR_PROT = val;
			break;
	}
}
void host_shm_SetRdProtect(uint8_t win, uint8_t val)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->WIN1_RD_PROT = val;
			break;
		case hs_SHM_WIN2:
			inst_shm->WIN2_RD_PROT = val;
			break;
		case hs_SHM_WIN3:
			inst_shm->WIN3_RD_PROT = val;
			break;
		case hs_SHM_WIN4:
			inst_shm->WIN4_RD_PROT = val;
			break;
		case hs_SHM_WIN5:
			inst_shm->WIN5_RD_PROT = val;
			break;
	}
}
void host_shm_SetOffset(uint8_t win, uint16_t offset)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->COFS1 = ((uint16_t)(offset) & 0x0FFF);
			break;
		case hs_SHM_WIN2:
			inst_shm->COFS2 = ((uint16_t)(offset) & 0x0FFF);
			break;
		case hs_SHM_WIN3:
			inst_shm->COFS3 = ((uint16_t)(offset) & 0x0FFF);
			break;
		case hs_SHM_WIN4:
			inst_shm->COFS4 = ((uint16_t)(offset) & 0x0FFF);
			break;
		case hs_SHM_WIN5:
			inst_shm->COFS5 = ((uint16_t)(offset) & 0x0FFF);
			break;
	}

}
bool host_shm_IsRdOffsetIE(uint8_t win)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			if(inst_shm->HOFS_STS & 0x01) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN2:
			if(inst_shm->HOFS_STS & 0x04) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN3:
			if(inst_shm->HOFS_STS & 0x10) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN4:
			if(inst_shm->HOFS_STS & 0x40) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN5:
		default:
			if(inst_shm->HOFS_STS2 & 0x01) {ret = 1;}
			else {ret = 0;}
			break;
	}
	return ret;
}
bool host_shm_IsWrOffsetIE(uint8_t win)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			if(inst_shm->HOFS_STS & 0x02) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN2:
			if(inst_shm->HOFS_STS & 0x08) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN3:
			if(inst_shm->HOFS_STS & 0x20) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN4:
			if(inst_shm->HOFS_STS & 0x80) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN5:
		default:
			if(inst_shm->HOFS_STS2 & 0x02) {ret = 1;}
			else {ret = 0;}
			break;
	}
	return ret;
}
void host_shm_ClrRdOffsetSts(uint8_t win)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->HOFS_STS = 0x01;
			break;
		case hs_SHM_WIN2:
			inst_shm->HOFS_STS = 0x04;
			break;
		case hs_SHM_WIN3:
			inst_shm->HOFS_STS = 0x10;
			break;
		case hs_SHM_WIN4:
			inst_shm->HOFS_STS = 0x40;
			break;
		case hs_SHM_WIN5:
			inst_shm->HOFS_STS2 = 0x01;
			break;
	}
}
void host_shm_ClrWrOffsetSts(uint8_t win)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->HOFS_STS = 0x02;
			break;
		case hs_SHM_WIN2:
			inst_shm->HOFS_STS = 0x08;
			break;
		case hs_SHM_WIN3:
			inst_shm->HOFS_STS = 0x20;
			break;
		case hs_SHM_WIN4:
			inst_shm->HOFS_STS = 0x80;
			break;
		case hs_SHM_WIN5:
			inst_shm->HOFS_STS2 = 0x02;
			break;
	}
}
void host_shm_EnableSemaphore(uint8_t flags)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	inst_shm->SHCFG &= ~(flags);
}
void host_shm_SetHostSemaphore(uint8_t win, uint8_t val)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->SHAW1_SEM = val;
			break;
		case hs_SHM_WIN2:
			inst_shm->SHAW2_SEM = val;
			break;
		case hs_SHM_WIN3:
			inst_shm->SHAW3_SEM = val;
			break;
		case hs_SHM_WIN4:
			inst_shm->SHAW4_SEM = val;
			break;
		case hs_SHM_WIN5:
			inst_shm->SHAW5_SEM = val;
			break;
	}
}
uint8_t host_shm_GetHostSemaphore(uint8_t win)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			ret = ((uint8_t)(inst_shm->SHAW1_SEM & 0x0F));
			break;
		case hs_SHM_WIN2:
			ret = ((uint8_t)(inst_shm->SHAW2_SEM & 0x0F));
			break;
		case hs_SHM_WIN3:
			ret = ((uint8_t)(inst_shm->SHAW3_SEM & 0x0F));
			break;
		case hs_SHM_WIN4:
			ret = ((uint8_t)(inst_shm->SHAW4_SEM & 0x0F));
			break;
		case hs_SHM_WIN5:
		default:
			ret = ((uint8_t)(inst_shm->SHAW5_SEM & 0x0F));
			break;
	}
	return ret;
}
bool host_shm_IsHostSemIE(uint8_t win)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			if(inst_shm->SMC_STS & 0x10) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN2:
			if(inst_shm->SMC_STS & 0x20) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN3:
			if(inst_shm->SMC_STS & 0x04) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN4:
			if(inst_shm->SMC_STS & 0x80) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN5:
		default:
			if(inst_shm->SMC_STS2 & 0x04) {ret = 1;}
			else {ret = 0;}
			break;
	}
	return ret;
}
bool host_shm_IsHostSemEnable(uint8_t win)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			if(inst_shm->SMC_CTL & 0x08) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN2:
			if(inst_shm->SMC_CTL & 0x10) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN3:
			if(inst_shm->SMC_CTL2 & 0x01) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN4:
			if(inst_shm->SMC_CTL2 & 0x02) {ret = 1;}
			else {ret = 0;}
			break;
		case hs_SHM_WIN5:
		default:
			if(inst_shm->SMC_STS2 & 0x10) {ret = 1;}
			else {ret = 0;}
			break;
	}
	return ret;
}
void host_shm_ClrHostSemSts(uint8_t win)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
			inst_shm->SMC_STS = 0x10;
			break;
		case hs_SHM_WIN2:
			inst_shm->SMC_STS = 0x20;
			break;
		case hs_SHM_WIN3:
			inst_shm->SMC_STS = 0x04;
			break;
		case hs_SHM_WIN4:
			inst_shm->SMC_STS = 0x80;
			break;
		case hs_SHM_WIN5:
			inst_shm->SMC_STS2 = 0x04;
			break;
	}
}

void host_shm_SetWinSize(uint32_t win, uint8_t size)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch(win)
	{
		case hs_SHM_WIN1:
		case hs_SHM_WIN2:
			inst_shm->WIN_SIZE &= 0xF0 >> 4*(win - hs_SHM_WIN1);
			inst_shm->WIN_SIZE |= size << 4*(win - hs_SHM_WIN1);
			break;
		case hs_SHM_WIN3:
		case hs_SHM_WIN4:
			inst_shm->WIN_SIZE2 &= 0xF0 >> 4*(win - hs_SHM_WIN3);
			inst_shm->WIN_SIZE2 |= size << 4*(win - hs_SHM_WIN3);
			break;
		case hs_SHM_WIN5:
			inst_shm->WIN_SIZE3 = (size & 0x0F);
			break;
		case hs_SHM_IMA_WIN1:
		case hs_SHM_IMA_WIN2:
			size <<= 4*(win - hs_SHM_IMA_WIN1);
			inst_shm->IMA_WIN_SIZE ^= (size ^ inst_shm->IMA_WIN_SIZE) & (0x0F << 4*(win - hs_SHM_IMA_WIN1));
			break;
	}
}

void host_shm_EnableOffsetInterrupt(uint8_t win, uint8_t flags)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	if(win != hs_SHM_WIN5) {
		// window 1~4
		flags <<= 2*(win - hs_SHM_WIN1);
		inst_shm->HOFS_CTL ^= (flags ^ inst_shm->HOFS_CTL) & flags;
	} else {
		inst_shm->HOFS_CTL2 |= flags;
	}
}

void host_shm_EnableSemaphoreIE(uint8_t win)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch (win)
	{
		case hs_SHM_WIN1:
		case hs_SHM_WIN2:
			inst_shm->SMC_CTL |= (((0x01) << 3) << win);
			break;
		case hs_SHM_WIN3:
		case hs_SHM_WIN4:
			inst_shm->SMC_CTL2 |= ((0x01) << (win - hs_SHM_WIN3));
			inst_shm->SHCFG &= ~((0x01)<<(4 - (win - hs_SHM_WIN3)));
			break;
		case hs_SHM_WIN5:
			inst_shm->SMC_CTL2 |= (0x1 << NCT_SMC_CTL2_HSEM5_IE);
			break;
	}
}

void host_shm_DisableSemaphoreIE(uint8_t win)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	switch (win)
	{
		case hs_SHM_WIN1:
		case hs_SHM_WIN2:
			inst_shm->SMC_CTL &= ~(((0x01) << 3) << win);
			break;
		case hs_SHM_WIN3:
		case hs_SHM_WIN4:
			inst_shm->SMC_CTL2 &= ~((0x01) << (win - hs_SHM_WIN3));
			break;
		case hs_SHM_WIN5:
			inst_shm->SMC_CTL2 &= ~(0x1 << NCT_SMC_CTL2_HSEM5_IE);
			break;
		default:
			inst_shm->SMC_CTL = 0;
			break;
	}
}
void host_shm_AddCBtoShmISR(void* CB)
{
	host_shm_MBICBFn = CB;
}

#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
/* Host command argument and memory map buffers */
static void host_shm_mai_isr(void *arg)
{
	ARG_UNUSED(arg);
	host_shm_MBICBFn();
}

static void host_shared_mem_region_init(void)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
//	uint32_t win_size = CONFIG_ESPI_NCT_PERIPHERAL_ACPI_SHD_MEM_SIZE;

	/* Don't stall SHM transactions */
	inst_shm->SHM_CTL &= ~0x40;
	/* Disable Window 2 protection */
	inst_shm->WIN2_WR_PROT = 0;
	inst_shm->WIN2_RD_PROT = 0;

	/* Configure Win2 size for ACPI shared mem region. */
//	SET_FIELD(inst_shm->WIN_SIZE, NCT_WIN_SIZE_RWIN2_SIZE_FIELD,
//		host_shd_mem_wnd_size_sl(win_size));
//	inst_shm->WIN_BASE2 = (uint32_t)shm_acpi_mmap;
	/* Enable write protect of Share memory window 2 */
	inst_shm->WIN2_WR_PROT = 0xFF;

	/*
	 * TODO: Initialize shm_acpi_mmap buffer for host command flags. We
	 * might use EACPI_GET_SHARED_MEMORY in espi_api_lpc_read_request()
	 * instead of setting host command flags here directly.
	 */
}
#endif

typedef void (*host_pmch_ibf_CB)(void);
host_pmch_ibf_CB host_pmch_ibfCBFn;

void host_pmch_AddCBtopmchibfISR(void* CB)
{
	host_pmch_ibfCBFn = CB;
}

#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO) || \
				defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH3) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH4)

/* Host pm (host io) sub-module isr function for all channels such as ACPI. */
static void host_pmch_ibf_isr(void *arg)
{
	ARG_UNUSED(arg);
	host_pmch_ibfCBFn();
}
#endif

void host_shm_SetP80Ctrl(uint8_t val)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	inst_shm->DP80CTL |= val;
}
bool host_shm_IsP80STS(uint8_t val)
{
	uint8_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	if(inst_shm->DP80STS & val) {ret = 1;}
	else {ret = 0;}
	return ret;

}
uint32_t host_shm_GetP80Buf(uint8_t Buf)
{
	uint32_t ret;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	if(Buf == 0) {ret = inst_shm->DP80BUF;}
	else {ret = inst_shm->DP80BUF1;}
	return ret;
}

/* Host port80 sub-device local functions */
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)
static void host_port80_isr(void *arg)
{
	ARG_UNUSED(arg);
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		(ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
		ESPI_PERIPHERAL_NODATA
	};
	uint8_t status = inst_shm->DP80STS;

	LOG_DBG("%s: p80 status 0x%02X", __func__, status);

	/* Read out port80 data continuously if FIFO is not empty */
	while (IS_BIT_SET(inst_shm->DP80STS, NCT_DP80STS_FNE)) {
		LOG_DBG("p80: %04x", inst_shm->DP80BUF);
		evt.evt_data = inst_shm->DP80BUF;
		espi_send_callbacks(host_sub_data.callbacks,
				host_sub_data.host_bus_dev, evt);
	}

	/* If FIFO is overflow, show error msg */
	if (IS_BIT_SET(status, NCT_DP80STS_FOR)) {
		inst_shm->DP80STS |= BIT(NCT_DP80STS_FOR);
		LOG_ERR("Port80 FIFO Overflow!");
	}

	/* Clear all pending bit indicates that FIFO was written by host */
	inst_shm->DP80STS |= BIT(NCT_DP80STS_FWR);
}

static void host_port80_init(void)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;

	/*
	 * Init PORT80 which includes:
	 * Enables a Core interrupt on every Host write to the FIFO,
	 * SYNC mode (It must be 1 in eSPI mode), Read Auto Advance mode, and
	 * Port80 module itself.
	 */
	inst_shm->DP80CTL = BIT(NCT_DP80CTL_RAA)
			| BIT(NCT_DP80CTL_DP80EN) | BIT(NCT_DP80CTL_SYNCEN);
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
static void host_cus_opcode_enable_interrupts(void)
{
	/* Enable host PM channel (Host IO) sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_PMCH3) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_PMCH4)) {
		irq_enable(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq));
	}

	/* Enable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		irq_enable(DT_INST_IRQ_BY_NAME(0, p80_fifo, irq));
	}

	/* Enable host interface interrupts if its interface is eSPI */
	if (IS_ENABLED(CONFIG_ESPI)) {
		nct_espi_enable_interrupts(host_sub_data.host_bus_dev);
	}
}

static void host_cus_opcode_disable_interrupts(void)
{
	/* Disable host PM channel (Host IO) sub-device interrupt */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO) ||
		IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) ||
		IS_ENABLED(CONFIG_ESPI_PERIPHERAL_PMCH3) ||
		IS_ENABLED(CONFIG_ESPI_PERIPHERAL_PMCH4)) {
		irq_disable(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq));
	}

	/* Disable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)) {
		irq_disable(DT_INST_IRQ_BY_NAME(0, p80_fifo, irq));
	}

	/* Disable host interface interrupts if its interface is eSPI */
	if (IS_ENABLED(CONFIG_ESPI)) {
		nct_espi_disable_interrupts(host_sub_data.host_bus_dev);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#if defined(CONFIG_ESPI_PERIPHERAL_UART)
/* Host UART sub-device local functions */
void host_uart_init(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Make sure unlock host access of serial port */
	inst_c2h->LKSIOHA &= ~BIT(NCT_LKSIOHA_LKSPHA);
	/* Clear 'Host lock violation occurred' bit of serial port initially */
	inst_c2h->SIOLV |= BIT(NCT_SIOLV_SPLV);
}
#endif

/* host core-to-host interface local functions */
static void host_c2h_wait_write_done(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;
	uint32_t elapsed_cycles;
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t max_wait_cycles =
			k_us_to_cyc_ceil32(NCT_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NCT_SIBCTRL_CSWR)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h write transaction expired!");
			break;
		}
	}
}

static void host_c2h_wait_read_done(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;
	uint32_t elapsed_cycles;
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t max_wait_cycles =
			k_us_to_cyc_ceil32(NCT_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NCT_SIBCTRL_CSRD)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h read transaction expired!");
			break;
		}
	}
}

void host_c2h_write_reg(uint8_t c2h_device, uint8_t reg_index, uint8_t reg_data)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NCT_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	//inst_c2h->CRSMAE |= BIT(NCT_CRSMAE_CFGAE);
	inst_c2h->CRSMAE |= c2h_device;

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_write_done();

	inst_c2h->IHIOA = reg_index;
	inst_c2h->IHD = reg_data;

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE = 0;
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NCT_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);
}

uint8_t host_c2h_read_reg(uint8_t c2h_device, uint8_t reg_index)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;
	uint8_t data_val;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NCT_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	inst_c2h->CRSMAE = c2h_device;

	inst_c2h->IHIOA = reg_index;
	inst_c2h->SIBCTRL |= BIT(NCT_SIBCTRL_CSRD);
	host_c2h_wait_read_done();
	data_val = inst_c2h->IHD;

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE = 0;
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NCT_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);

	return data_val;
}

/* Platform specific host sub modules functions */
int nct_host_periph_read_request(enum lpc_peripheral_opcode op,
								uint32_t *data)
{
	if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmch_reg *const inst_acpi = host_sub_cfg.inst_pm_acpi;

		/* Make sure pm channel for apci is on */
		if (!IS_BIT_SET(inst_acpi->HIPMCTL, NCT_HIPMCTL_IBFIE)) {
			return -ENOTSUP;
		}

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = IS_BIT_SET(inst_acpi->HIPMST, NCT_HIPMST_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = IS_BIT_SET(inst_acpi->HIPMST, NCT_HIPMST_IBF);
			break;
		case EACPI_READ_STS:
			*data = inst_acpi->HIPMST;
			break;
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
		case EACPI_GET_SHARED_MEMORY:
//			*data = (uint32_t)shm_acpi_mmap;
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
		default:
			return -EINVAL;
		}
	}
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		/* Other customized op codes */
		switch (op) {
		case ECUSTOM_HOST_CMD_GET_PARAM_MEMORY:
			*data = (uint32_t)shm_host_cmd;
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

int nct_host_periph_write_request(enum lpc_peripheral_opcode op,
							const uint32_t *data)
{
	volatile uint32_t __attribute__((unused)) dummy;

	if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
		struct pmch_reg *const inst_acpi = host_sub_cfg.inst_pm_acpi;

		/* Make sure pm channel for apci is on */
		if (!IS_BIT_SET(inst_acpi->HIPMCTL, NCT_HIPMCTL_IBFIE)) {
			return -ENOTSUP;
		}

		switch (op) {
		case EACPI_WRITE_CHAR:
			inst_acpi->HIPMDO = (*data & 0xff);
			break;
		case EACPI_WRITE_STS:
			inst_acpi->HIPMST = (*data & 0xff);
			break;
		default:
			return -EINVAL;
		}
	}
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		/* Other customized op codes */
		struct pmch_reg *const inst_hcmd = host_sub_cfg.inst_pm_hcmd;

		switch (op) {
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			if (*data != 0) {
				host_cus_opcode_enable_interrupts();
			} else {
				host_cus_opcode_disable_interrupts();
			}
			break;
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			/*
			 * Write result to the data byte.  This sets the TOH
			 * status bit.
			 */
			inst_hcmd->HIPMDO = (*data & 0xff);
			/* Clear processing flag */
			inst_hcmd->HIPMST &= ~BIT(NCT_HIPMST_F0);
			break;
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
	else {
		return -ENOTSUP;
	}

	return 0;
}

void nct_host_init_subs_host_domain(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Enable Core-to-Host access module */
	inst_c2h->SIBCTRL |= BIT(NCT_SIBCTRL_CSAE);

	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_HOST_IO)) {

		/*
		 * Select ACPI bank which LDN are 0x11 (PM Channel 1) and enable
		 * module by setting bit 0 in its Control (index is 0x30) reg.
		 */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_ACPI);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_LDN);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, EC_CFG_LDN_ACPI);

		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_CTRL);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, 0x01);

	}

	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) ||
	    IS_ENABLED(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)) {
		/* Select 'Host Command' bank which LDN are 0x12 (PM chan 2) */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_HCMD);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_LDN);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, EC_CFG_LDN_HCMD);

		/* Enable 'Host Command' io port (PM Channel 2) */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_CTRL);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, 0x01);

		/* Select 'Shared Memory' bank which LDN are 0x0F */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_SHM);

		/* WIN 1 & 2 mapping to IO space */
		//host_c2h_write_io_cfg_reg(0xF1,
		//		host_c2h_read_io_cfg_reg(0xF1) | 0x30);
		/* WIN1 as Host Command on the IO address (default: 0x0800) */

		/* Set WIN2 as MEMMAP on the configured IO address */
	/* Enable SHM direct memory access */
	//host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);

	}

	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_PMCH4)) {
		/* Select 'Host Command' bank which LDN are 0x1E (KCS4/PM chan 4) */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_PMCH4);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_LDN);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, EC_CFG_LDN_PMCH4);
		/* Enable 'Host Command' io port (KCS4/PM Channel 4) */
		//host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_INDEX, EC_CFG_IDX_CTRL);
		host_c2h_write_reg(EC_C2H_DEV_CFG, EC_C2H_OFFSET_DATA, 0x01);
	}

	LOG_DBG("Hos sub-modules configurations are done!");
}

void nct_host_enable_access_interrupt(void)
{
	nct_miwu_irq_get_and_clear_pending(&host_sub_cfg.host_acc_wui);

	nct_miwu_irq_enable(&host_sub_cfg.host_acc_wui);
}

void nct_host_disable_access_interrupt(void)
{
	nct_miwu_irq_disable(&host_sub_cfg.host_acc_wui);
}

int nct_host_init_subs_core_domain(const struct device *host_bus_dev,
							sys_slist_t *callbacks)
{
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
//	const struct device *const clk_dev =
//					device_get_binding(NCT_CLK_CTRL_NAME);
//	int i;
	uint8_t shm_sts;

	host_sub_data.callbacks = callbacks;
	host_sub_data.host_bus_dev = host_bus_dev;

	/* Turn on all host necessary sub-module clocks first */
//	for (i = 0; i < host_sub_cfg.clks_size; i++) {
//		int ret;

//		ret = clock_control_on(clk_dev, (clock_control_subsys_t *)
//				&host_sub_cfg.clks_list[i]);
//		if (ret < 0)
//			return ret;
//	}

	/*
	 * Set HOSTWAIT bit and avoid the other settings, then host can freely
	 * communicate with slave (EC).
	 */
	inst_shm->SMC_CTL &= BIT(NCT_SMC_CTL_HOSTWAIT);
	/* Clear shared memory status */
	shm_sts = inst_shm->SMC_STS;
	inst_shm->SMC_STS = shm_sts;

	/* host sub-module initialization in core domain */
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
	host_pmch_init(host_sub_cfg.inst_pm_acpi);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
	host_pmch_init(host_sub_cfg.inst_pm_hcmd);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_PMCH3)
	host_pmch_init(host_sub_cfg.inst_pmch3);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_PMCH4)
	host_pmch_init(host_sub_cfg.inst_pmch4);
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	host_shared_mem_region_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)
	host_port80_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_UART)
	host_uart_init();
#endif

/* Host share memory sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, shm_mai, irq),
			DT_INST_IRQ_BY_NAME(0, shm_mai, priority),
			host_shm_mai_isr,
			NULL, 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, shm_mai, irq));
#endif

	/* Host PM channel (Host IO) sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO) || \
				defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH3) || \
				defined(CONFIG_ESPI_PERIPHERAL_PMCH4)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq),
		    DT_INST_IRQ_BY_NAME(0, pmch_ibf, priority),
		    host_pmch_ibf_isr,
		    NULL, 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq));
#endif

	/* Host Port80 sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, p80_fifo, irq),
		    DT_INST_IRQ_BY_NAME(0, p80_fifo, priority),
		    host_port80_isr,
		    NULL, 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, p80_fifo, irq));
#endif

	if (IS_ENABLED(CONFIG_PM)) {
		/*
		 * Configure the host access wake-up event triggered from a host
		 * transaction on eSPI/LPC bus. Do not enable it here. Or plenty
		 * of interrupts will jam the system in S0.
		 */
		nct_miwu_interrupt_configure(&host_sub_cfg.host_acc_wui,
				NCT_MIWU_MODE_EDGE, NCT_MIWU_TRIG_HIGH);
	}

	return 0;
}


/* Check power management channel is OBF */
bool host_pmch_is_obf(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return IS_BIT_SET(host_sub_cfg.inst_pm_acpi->HIPMST, NCT_HIPMST_OBF);
		case hs_PMCH_HCMD:
			return IS_BIT_SET(host_sub_cfg.inst_pm_hcmd->HIPMST, NCT_HIPMST_OBF);
		case hs_PMCH3:
			return IS_BIT_SET(host_sub_cfg.inst_pmch3->HIPMST, NCT_HIPMST_OBF);
		case hs_PMCH4:
			return IS_BIT_SET(host_sub_cfg.inst_pmch4->HIPMST, NCT_HIPMST_OBF);
		default:
			return 0;
	}
}

void host_pmch_ibf_int_enable(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMCTL |= BIT(NCT_HIPMCTL_IBFIE);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMCTL |= BIT(NCT_HIPMCTL_IBFIE); 
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMCTL |= BIT(NCT_HIPMCTL_IBFIE); 
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMCTL |= BIT(NCT_HIPMCTL_IBFIE); 
			break;
	}
}

/* Check power management channel is IBF */
bool host_pmch_is_ibf(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return IS_BIT_SET(host_sub_cfg.inst_pm_acpi->HIPMST, NCT_HIPMST_IBF);
		case hs_PMCH_HCMD:
			return IS_BIT_SET(host_sub_cfg.inst_pm_hcmd->HIPMST, NCT_HIPMST_IBF);
		case hs_PMCH3:
			return IS_BIT_SET(host_sub_cfg.inst_pmch3->HIPMST, NCT_HIPMST_IBF);
		case hs_PMCH4:
			return IS_BIT_SET(host_sub_cfg.inst_pmch4->HIPMST, NCT_HIPMST_IBF);
		default:
			return 0;
	}
}

void host_pmch_write_data(uint8_t pmch, uint8_t data)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMDO = data;
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMDO = data;
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMDO = data;
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMDO = data;
			break;
	}
}

void host_pmch_write_data_with_smi(uint8_t pmch, uint8_t data)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMDOM = data;
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMDOM = data;
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMDOM = data;
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMDOM = data;
			break;
	}
}

uint8_t host_pmch_read_data(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return host_sub_cfg.inst_pm_acpi-> HIPMDI;
		case hs_PMCH_HCMD:
			return host_sub_cfg.inst_pm_hcmd-> HIPMDI;
		case hs_PMCH3:
			return host_sub_cfg.inst_pmch3-> HIPMDI;
		case hs_PMCH4:
			return host_sub_cfg.inst_pmch4-> HIPMDI;
		default:
			return 0;
	}
}

uint8_t host_pmch_shadow_read_data(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return host_sub_cfg.inst_pm_acpi-> SHIPMDI;
		case hs_PMCH_HCMD:
			return host_sub_cfg.inst_pm_hcmd-> SHIPMDI;
		case hs_PMCH3:
			return host_sub_cfg.inst_pmch3-> SHIPMDI;
		case hs_PMCH4:
			return host_sub_cfg.inst_pmch4-> SHIPMDI;
		default:
			return 0;
	}
}

bool host_pmch_is_rcv_cmd(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return IS_BIT_SET(host_sub_cfg.inst_pm_acpi->HIPMST, NCT_HIPMST_CMD);
		case hs_PMCH_HCMD:
			return IS_BIT_SET(host_sub_cfg.inst_pm_hcmd->HIPMST, NCT_HIPMST_CMD);
		case hs_PMCH3:
			return IS_BIT_SET(host_sub_cfg.inst_pmch3->HIPMST, NCT_HIPMST_CMD);
		case hs_PMCH4:
			return IS_BIT_SET(host_sub_cfg.inst_pmch4->HIPMST, NCT_HIPMST_CMD);
		default:
			return 0;
	}
}

void host_pmch_manual_hw_sci_enable(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMIE |= BIT(NCT_HIPMIE_SCIE);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMIE |= BIT(NCT_HIPMIE_SCIE); 
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMIE |= BIT(NCT_HIPMIE_SCIE); 
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMIE |= BIT(NCT_HIPMIE_SCIE); 
			break;
	}
}

void host_pmch_auto_hw_sci_enable(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMIE |= BIT(NCT_HIPMIE_SCIE) | BIT(NCT_HIPMIE_HSCIE);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMIE |= BIT(NCT_HIPMIE_SCIE) | BIT(NCT_HIPMIE_HSCIE);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMIE |= BIT(NCT_HIPMIE_SCIE) | BIT(NCT_HIPMIE_HSCIE);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMIE |= BIT(NCT_HIPMIE_SCIE) | BIT(NCT_HIPMIE_HSCIE);
			break;
	}
}

void host_pmch_set_sci_mode(uint8_t pmch, uint8_t type)
{
	if(type > 6) {return;}
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMCTL &= ~(NCT_HIPMCTL_PLMS_MSK);
			host_sub_cfg.inst_pm_acpi-> HIPMCTL |= (type << NCT_HIPMCTL_PLMS);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMCTL &= ~(NCT_HIPMCTL_PLMS_MSK);
			host_sub_cfg.inst_pm_hcmd-> HIPMCTL |= (type << NCT_HIPMCTL_PLMS);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMCTL &= ~(NCT_HIPMCTL_PLMS_MSK);
			host_sub_cfg.inst_pmch3-> HIPMCTL |= (type << NCT_HIPMCTL_PLMS);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMCTL &= ~(NCT_HIPMCTL_PLMS_MSK);
			host_sub_cfg.inst_pmch4-> HIPMCTL |= (type << NCT_HIPMCTL_PLMS);
			break;
	}
}

void host_pmch_gen_sci_on_ibf_start(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB)|BIT(NCT_HIPMIC_SCIB));
			host_sub_cfg.inst_pm_acpi-> HIPMIC |= BIT(NCT_HIPMIC_SCIIS);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB)|BIT(NCT_HIPMIC_SCIB));
			host_sub_cfg.inst_pm_hcmd-> HIPMIC |= BIT(NCT_HIPMIC_SCIIS);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB)|BIT(NCT_HIPMIC_SCIB));
			host_sub_cfg.inst_pmch3-> HIPMIC |= BIT(NCT_HIPMIC_SCIIS);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB)|BIT(NCT_HIPMIC_SCIB));
			host_sub_cfg.inst_pmch4-> HIPMIC |= BIT(NCT_HIPMIC_SCIIS);
			break;
	}
}

void host_pmch_gen_sci_manually(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB));
			host_sub_cfg.inst_pm_acpi-> HIPMIC |= BIT(NCT_HIPMIC_SCIB);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB));
			host_sub_cfg.inst_pm_hcmd-> HIPMIC |= BIT(NCT_HIPMIC_SCIB);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB));
			host_sub_cfg.inst_pmch3-> HIPMIC |= BIT(NCT_HIPMIC_SCIB);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMIC &= ~(BIT(NCT_HIPMIC_IRQB)|BIT(NCT_HIPMIC_SMIB));
			host_sub_cfg.inst_pmch4-> HIPMIC |= BIT(NCT_HIPMIC_SCIB);
			break;
	}
}

void host_pmch_write_data_with_sci(uint8_t pmch, uint8_t data)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMDOC = data;
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMDOC = data;
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMDOC = data;
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMDOC = data;
			break;
	}
}

uint8_t host_pmch_read_data_with_sci(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return host_sub_cfg.inst_pm_acpi-> HIPMDIC;
		case hs_PMCH_HCMD:
			return host_sub_cfg.inst_pm_hcmd-> HIPMDIC;
		case hs_PMCH3:
			return host_sub_cfg.inst_pmch3-> HIPMDIC;
		case hs_PMCH4:
			return host_sub_cfg.inst_pmch4-> HIPMDIC;
		default:
			return 0;
	}
}

uint8_t host_pmch_get_st(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			return host_sub_cfg.inst_pm_acpi-> HIPMST;
		case hs_PMCH_HCMD:
			return host_sub_cfg.inst_pm_hcmd-> HIPMST;
		case hs_PMCH3:
			return host_sub_cfg.inst_pmch3-> HIPMST;
		case hs_PMCH4:
			return host_sub_cfg.inst_pmch4-> HIPMST;
		default:
			return 0;
	}
}

void host_pmch_set_st(uint8_t pmch, uint8_t msk)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMST |= msk;
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMST |= msk;
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMST |= msk;
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMST |= msk;
			break;
	}
}

void host_pmch_clr_st(uint8_t pmch, uint8_t msk)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMST &= ~(msk);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMST &= ~(msk);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMST &= ~(msk);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMST &= ~(msk);
			break;
	}
}

void host_pmch_set_enhance_mode(uint8_t pmch)
{
	switch(pmch) 
	{
		case hs_PMCH_ACPI:
			host_sub_cfg.inst_pm_acpi-> HIPMCTL |= BIT(NCT_HIPMCTL_EME);
			break;
		case hs_PMCH_HCMD:
			host_sub_cfg.inst_pm_hcmd-> HIPMCTL |= BIT(NCT_HIPMCTL_EME);
			break;
		case hs_PMCH3:
			host_sub_cfg.inst_pmch3-> HIPMCTL |= BIT(NCT_HIPMCTL_EME);
			break;
		case hs_PMCH4:
			host_sub_cfg.inst_pmch4-> HIPMCTL |= BIT(NCT_HIPMCTL_EME);
			break;
	}
}

void host_pmch_ibf_irp_enable(void)
{
	irq_enable(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq));
}

void host_pmch_ibf_irp_disable(void)
{
	irq_disable(DT_INST_IRQ_BY_NAME(0, pmch_ibf, irq));
}