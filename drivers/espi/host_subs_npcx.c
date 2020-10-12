/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_host_sub

/**
 * @file
 * @brief Nuvoton NPCX host sub modules driver
 *
 * This file contains the drivers of NPCX Host Sub-Modules that serve as an
 * interface between the Host and Core domains. Please refer the block diagram.
 *
 *                                        +------------+
 *                                        |   Serial   |---> TXD
 *                                  +<--->|    Port    |<--- RXD
 *                                  |     |            |<--> ...
 *                                  |     +------------+
 *                                  |     +------------+     |
 *                +------------+    |<--->|  KBC & PM  |<--->|
 *   eSPI_CLK --->|  eSPI Bus  |    |     |  Channels  |     |
 *   eSPI_RST --->| Controller |    |     +------------+     |
 * eSPI_IO3-0 <-->|            |<-->|     +------------+     |
 *    eSPI_CS --->| (eSPI mode)|    |     |   Shared   |     |
 * eSPI_ALERT <-->|            |    |<--->|   Memory   |<--->|
 *                +------------+    |     +------------+     |
 *                                  |     +------------+     |
 *                                  |<--->|    MSWC    |<--->|
 *                                  |     +------------+     |
 *                                  |     +------------+     |
 *                                  |     |    Core    |     |
 *                                  |<--->|   to Host  |<--->|
 *                                  |     |   Access   |     |
 *                                  |     +------------+     |
 *                                HMIB                       | Core Bus
 *                     (Host Modules Internal Bus)           +------------
 *
 *
 * For most of them, the Host can configure these modules via eSPI(Peripheral
 * Channel)/LPC by accessing 'Configuration and Control register Set' which IO
 * base address is 0x4E as default. (The table below illustrates structure of
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
 * --------|--------------------------------------------------- | | |   |
 *  73-74h | DMA Configuration Registers (No support in NPCX) | | | |   |
 * --------|--------------------------------------------------- | | |<--+
 *  F0-FFh | Special Logical Device Configuration Registers   | | | |
 * --------|--------------------------------------------------- | | |
 *           |--------------------------------------------------- | |
 *             |--------------------------------------------------- |
 *               |---------------------------------------------------
 *
 *
 * This driver introduces six host sub-modules. It includes:
 *
 * 1. Keyboard and Mouse Controller (KBC) interface.
 *   ● Intel 8051SL-compatible Host interface
 *     — 8042 KBD standard interface (ports 60h, 64h)
 *     — Legacy IRQ: IRQ1 (KBD) and IRQ12 (mouse) support
 *   ● Configured by two logical devices: Keyboard and Mouse (LDN 0x06/0x05)
 *
 * 2. Power Management (PM) channels.
 *   ● PM channel registers
 *     — Command/Status register
 *     — Data register
 *       channel 1: legacy 62h, 66h; channel 2: legacy 68h, 6Ch;
 *       channel 3: legacy 6Ah, 6Eh; channel 4: legacy 6Bh, 6Fh;
 *   ● PM interrupt using:
 *     — Serial IRQ
 *     — SMI
 *     — EC_SCI
 *   ● Configured by four logical devices: PM1/2/3/4 (LDN 0x11/0x12/0x17/0x1E)
 *
 * 3. Shared Memory mechanism (SHM).
 *   This module allows sharing of the on-chip RAM by both Core and the Host.
 *   It also supports the following features:
 *   ● Four Core/Host communication windows for direct RAM access
 *   ● Eight Protection regions for each access window
 *   ● Host IRQ and SMI generation
 *   ● Port 80 debug support
 *   ● Configured by one logical device: SHM (LDN 0x0F)
 *
 * 4. Core Access to Host Modules (C2H).
 *   ● A interface to access module registers in host domain.
 *     It enables the Core to access the registers in host domain (i.e., Host
 *     Configuration, Serial Port, SHM, and MSWC), through HMIB.
 *
 * 5. Mobile System Wake-Up functions (MSWC).
 *   It detects and handles wake-up events from various sources in the Host
 *   modules and alerts the Core for better power consumption.
 *
 * 6. Serial Port (Legacy UART)
 *   It provides UART functionality by supporting serial data communication
 *   with a remote peripheral device or a modem.
 *
 * INCLUDE FILES: soc_host.h
 *
 */

#include <assert.h>
#include <drivers/espi.h>
#include <drivers/gpio.h>
#include <drivers/clock_control.h>
#include <kernel.h>
#include <soc.h>
#include "espi_utils.h"
#include "soc_host.h"
#include "soc_miwu.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(host_sub_npcx, LOG_LEVEL_ERR);

struct host_sub_npcx_config {
	/* host module instances */
	struct mswc_reg *const inst_mswc;
	struct shm_reg *const inst_shm;
	struct c2h_reg *const inst_c2h;
	struct kbc_reg *const inst_kbc;
	struct pmch_reg *const inst_pm_acpi;
	struct pmch_reg *const inst_pm_hcmd;
	/* clock configuration */
	const uint8_t clks_size;
	const struct npcx_clk_cfg *clks_list;
	/* mapping table between host access signals and wake-up input */
	struct npcx_wui host_acc_wui;
};

struct host_sub_npcx_data {
	sys_slist_t *callbacks; /* pointer on the espi callback list */
	uint8_t plt_rst_asserted; /* current PLT_RST# status */
	uint8_t espi_rst_asserted; /* current ESPI_RST# status */
	const struct device *host_bus_dev; /* device for eSPI/LPC bus */
};

static const struct npcx_clk_cfg host_dev_clk_cfg[] =
					DT_NPCX_CLK_CFG_ITEMS_LIST(0);

struct host_sub_npcx_config host_sub_cfg = {
	.inst_mswc = (struct mswc_reg *)DT_INST_REG_ADDR_BY_NAME(0, mswc),
	.inst_shm = (struct shm_reg *)DT_INST_REG_ADDR_BY_NAME(0, shm),
	.inst_c2h = (struct c2h_reg *)DT_INST_REG_ADDR_BY_NAME(0, c2h),
	.inst_kbc = (struct kbc_reg *)DT_INST_REG_ADDR_BY_NAME(0, kbc),
	.inst_pm_acpi = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pm_acpi),
	.inst_pm_hcmd = (struct pmch_reg *)DT_INST_REG_ADDR_BY_NAME(0, pm_hcmd),
	.host_acc_wui = DT_NPCX_WUI_ITEM_BY_NAME(0, host_acc_wui),
	.clks_size = ARRAY_SIZE(host_dev_clk_cfg),
	.clks_list = host_dev_clk_cfg,
};

struct host_sub_npcx_data host_sub_data;

/* Application shouldn't touch these flags in KBC status register directly */
#define NPCX_KBC_STS_MASK (BIT(NPCX_HIKMST_IBF) | BIT(NPCX_HIKMST_OBF) \
						| BIT(NPCX_HIKMST_A2))

/* IO base address of EC Logical Device Configuration */
#define NPCX_EC_CFG_IO_ADDR 0x4E

/* 8042 event data */
#define NPCX_8042_DATA_POS 8U
#define NPCX_8042_TYPE_POS 0U

/* Timeout to wait for Core-to-Host transaction to be completed. */
#define NPCX_C2H_TRANSACTION_TIMEOUT_US 200

/* Logical Device Number Assignments */
#define EC_CFG_LDN_MOUSE 0x05
#define EC_CFG_LDN_KBC   0x06
#define EC_CFG_LDN_SHM   0x0F
#define EC_CFG_LDN_ACPI  0x11 /* PM Channel 1 */
#define EC_CFG_LDN_HCMD  0x12 /* PM Channel 2 */

/* Index of EC (4E/4F) Configuration Register */
#define EC_CFG_IDX_LDN             0x07
#define EC_CFG_IDX_CTRL            0x30
#define EC_CFG_IDX_CMD_IO_ADDR_H   0x60
#define EC_CFG_IDX_CMD_IO_ADDR_L   0x61
#define EC_CFG_IDX_DATA_IO_ADDR_H  0x62
#define EC_CFG_IDX_DATA_IO_ADDR_L  0x63

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

/* Host KBC sub-device local functions */
#if defined(CONFIG_ESPI_PERIPHERAL_8042_KBC)
static void host_kbc_ibf_isr(void *arg)
{
	ARG_UNUSED(arg);
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;
	struct espi_event evt = { ESPI_BUS_PERIPHERAL_NOTIFICATION,
		ESPI_PERIPHERAL_8042_KBC, ESPI_PERIPHERAL_NODATA
	};

	/*
	 * The high byte contains information from the host, and the lower byte
	 * indicates if the host sent a command or data. 1 = Command.
	 */
	evt.evt_data = (inst_kbc->HIKMDI << NPCX_8042_DATA_POS) |
		       (IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_A2) <<
				       NPCX_8042_TYPE_POS);

	LOG_DBG("%s: kbc data 0x%02x", __func__, evt.evt_data);
	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev,
							evt);
}

static void host_kbc_obe_isr(void *arg)
{
	ARG_UNUSED(arg);
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;

	/* Disable KBC OBE interrupt first */
	inst_kbc->HICTRL &= ~BIT(NPCX_HICTRL_OBECIE);

	LOG_DBG("%s: kbc status 0x%02x", __func__, inst_kbc->HIKMST);

	/* TODO: notify application that host already read out data here? */
}

static void host_kbc_init(void)
{
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;

	/* Make sure the previous OBF and IRQ has been sent out. */
	k_busy_wait(4);
	/* Set FW_OBF to clear OBF flag in both STATUS and HIKMST to 0 */
	inst_kbc->HICTRL |= BIT(NPCX_HICTRL_FW_OBF);
	/* Ensure there is no OBF set in this period. */
	k_busy_wait(4);

	/*
	 * Init KBC with:
	 * 1. Enable Input Buffer Full (IBF) core interrupt for Keyboard/mouse.
	 * 2. Enable Output Buffer Full Mouse(OBFM) SIRQ 12.
	 * 3. Enable Output Buffer Full Keyboard (OBFK) SIRQ 1.
	 */
	inst_kbc->HICTRL = BIT(NPCX_HICTRL_IBFCIE) | BIT(NPCX_HICTRL_OBFMIE)
						| BIT(NPCX_HICTRL_OBFKIE);

	/* Configure SIRQ 1/12 type (level + high) */
	inst_kbc->HIIRQC = 0x00;
}
#endif

/* Host ACPI sub-device local functions */
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
static void host_acpi_process_input_data(uint8_t data)
{
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO,
		.evt_data = ESPI_PERIPHERAL_NODATA
	};

	LOG_DBG("%s: acpi data 0x%02x", __func__, data);
	evt.evt_data = data;
	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev,
							evt);
}

static void host_acpi_init(void)
{
	struct pmch_reg *const inst_acpi = host_sub_cfg.inst_pm_acpi;

	/* Use SMI/SCI postive polarity by default */
	inst_acpi->HIPMCTL &= ~BIT(NPCX_HIPMCTL_SCIPOL);
	inst_acpi->HIPMIC &= ~BIT(NPCX_HIPMIC_SMIPOL);

	/* Set SMIB/SCIB bits to make sure SMI#/SCI# are driven to high */
	inst_acpi->HIPMIC |= BIT(NPCX_HIPMIC_SMIB) | BIT(NPCX_HIPMIC_SCIB);

	/*
	 * Allow SMI#/SCI# generated from PM module.
	 * On eSPI bus, we suggest set VW value of SCI#/SMI# directly.
	 */
	inst_acpi->HIPMIE |= BIT(NPCX_HIPMIE_SCIE);
	inst_acpi->HIPMIE |= BIT(NPCX_HIPMIE_SMIE);

	/*
	 * Init ACPI PM channel (Host IO) with:
	 * 1. Enable Input-Buffer Full (IBF) core interrupt.
	 * 2. BIT 7 must be 1.
	 */
	inst_acpi->HIPMCTL |= BIT(7) | BIT(NPCX_HIPMCTL_IBFIE);
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
/* Host command argument and memory map buffers */
static uint8_t	shm_mem_host_cmd[256] __aligned(8);
static uint8_t	shm_memmap[256] __aligned(8);

/* Host command sub-device local functions */
static void host_hcmd_process_input_data(uint8_t data)
{
	/* TODO: Handle host request data here */
}

static void host_hcmd_init(void)
{
	struct pmch_reg *const inst_hcmd = host_sub_cfg.inst_pm_hcmd;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;

	/* Don't stall SHM transactions */
	inst_shm->SHM_CTL &= ~0x40;
	/* Disable Window 1 & 2 protection */
	inst_shm->WIN1_WR_PROT = 0;
	inst_shm->WIN2_WR_PROT = 0;
	inst_shm->WIN1_RD_PROT = 0;
	inst_shm->WIN2_RD_PROT = 0;
	/* Configure Win1/2 for Host CMD and MEMMAP. Their sizes are 256B */
	inst_shm->WIN_SIZE = 0x88;
	inst_shm->WIN_BASE1 = (uint32_t)shm_mem_host_cmd;
	inst_shm->WIN_BASE2 = (uint32_t)shm_memmap;
	/* Enable write protect of Share memory window 2 */
	inst_shm->WIN2_WR_PROT = 0xFF;

	/* TODO: Initialize shm_memmap buffer for host command flags here */

	/*
	 * Clear processing flag before enabling host's interrupts in case
	 * it's set by the other command during sysjump.
	 */
	inst_hcmd->HIPMST &= ~BIT(NPCX_HIPMST_F0);

	/*
	 * Init Host Command PM channel (Host IO) with:
	 * 1. Enable Input-Buffer Full (IBF) core interrupt.
	 * 2. BIT 7 must be 1.
	 */
	inst_hcmd->HIPMCTL |= BIT(7) | BIT(NPCX_HIPMCTL_IBFIE);

}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO) || \
				defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
/* Host pm (host io) sub-module isr function for all channels such as ACPI. */
static void host_pmch_ibf_isr(void *arg)
{
	ARG_UNUSED(arg);
	struct pmch_reg *const inst_acpi = host_sub_cfg.inst_pm_acpi;
	struct pmch_reg *const inst_hcmd = host_sub_cfg.inst_pm_hcmd;
	uint8_t in_data;

	/* Host put data on input buffer of ACPI channel */
	if (IS_BIT_SET(inst_acpi->HIPMST, NPCX_HIPMST_IBF)) {
		/* Set processing flag before reading command byte */
		inst_acpi->HIPMST |= BIT(NPCX_HIPMST_F0);
		/* Read out input data and clear IBF pending bit */
		in_data = inst_acpi->HIPMDI;
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
		host_acpi_process_input_data(in_data);
#endif
	}

	/* Host put data on input buffer of HOSTCMD channel */
	if (IS_BIT_SET(inst_hcmd->HIPMST, NPCX_HIPMST_IBF)) {
		/* Set processing flag before reading command byte */
		inst_hcmd->HIPMST |= BIT(NPCX_HIPMST_F0);
		/* Read out input data and clear IBF pending bit */
		in_data = inst_hcmd->HIPMDI;
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
		host_hcmd_process_input_data(in_data);
#endif
	}
}
#endif

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
	while (IS_BIT_SET(inst_shm->DP80STS, NPCX_DP80STS_FNE)) {
		LOG_DBG("p80: %04x", inst_shm->DP80BUF);
		evt.evt_data = inst_shm->DP80BUF;
		espi_send_callbacks(host_sub_data.callbacks,
				host_sub_data.host_bus_dev, evt);
	}

	/* If FIFO is overflow, show error msg */
	if (IS_BIT_SET(status, NPCX_DP80STS_FOR)) {
		inst_shm->DP80STS |= BIT(NPCX_DP80STS_FOR);
		LOG_ERR("Port80 FIFO Overflow!");
	}

	/* Clear all pending bit indicates that FIFO was written by host */
	inst_shm->DP80STS |= BIT(NPCX_DP80STS_FWR);
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
	inst_shm->DP80CTL = BIT(NPCX_DP80CTL_CIEN) | BIT(NPCX_DP80CTL_RAA)
			| BIT(NPCX_DP80CTL_DP80EN) | BIT(NPCX_DP80CTL_SYNCEN);
}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_UART)
/* host uart pinmux configuration */
static const struct npcx_alt host_uart_alts[] =
					DT_NPCX_ALT_ITEMS_LIST(0);
/* Host UART sub-device local functions */
void host_uart_init(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Configure pin-mux for serial port device */
	soc_pinctrl_mux_configure(host_uart_alts, ARRAY_SIZE(host_uart_alts),
									1);
	/* Make sure unlock host access of serial port */
	inst_c2h->LKSIOHA &= ~BIT(NPCX_LKSIOHA_LKSPHA);
	/* Clear 'Host lock violation occurred' bit of serial port initially */
	inst_c2h->SIOLV |= BIT(NPCX_SIOLV_SPLV);
}
#endif

/* host core-to-host interface local functions */
static void host_c2h_wait_write_done(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;
	uint32_t elapsed_cycles;
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t max_wait_cycles =
			k_us_to_cyc_ceil32(NPCX_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NPCX_SIBCTRL_CSWR)) {
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
			k_us_to_cyc_ceil32(NPCX_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NPCX_SIBCTRL_CSRD)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h read transaction expired!");
			break;
		}
	}
}

void host_c2h_write_io_cfg_reg(uint8_t reg_index, uint8_t reg_data)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCX_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	inst_c2h->CRSMAE |= BIT(NPCX_CRSMAE_CFGAE);

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_read_done();
	host_c2h_wait_write_done();

	/*
	 * Specifying the in-direct IO address which A0 = 0 indicates the index
	 * register is accessed. Then write index address directly and it starts
	 * a write transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = NPCX_EC_CFG_IO_ADDR;
	inst_c2h->IHD = reg_index;
	host_c2h_wait_write_done();

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write data directly and it starts a write
	 * transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = NPCX_EC_CFG_IO_ADDR + 1;
	inst_c2h->IHD = reg_data;
	host_c2h_wait_write_done();

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE &= ~BIT(NPCX_CRSMAE_CFGAE);
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCX_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);
}

uint8_t host_c2h_read_io_cfg_reg(uint8_t reg_index)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;
	uint8_t data_val;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCX_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	inst_c2h->CRSMAE |= BIT(NPCX_CRSMAE_CFGAE);

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_read_done();
	host_c2h_wait_write_done();

	/*
	 * Specifying the in-direct IO address which A0 = 0 indicates the index
	 * register is accessed. Then write index address directly and it starts
	 * a write transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = NPCX_EC_CFG_IO_ADDR;
	inst_c2h->IHD = reg_index;
	host_c2h_wait_write_done();

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write CSRD bit in SIBCTRL to issue a read
	 * transaction to host sub-module on LPC/eSPI bus. Once it was done,
	 * read data out from IHD.
	 */
	inst_c2h->IHIOA = NPCX_EC_CFG_IO_ADDR + 1;
	inst_c2h->SIBCTRL |= BIT(NPCX_SIBCTRL_CSRD);
	host_c2h_wait_read_done();
	data_val = inst_c2h->IHD;

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE &= ~BIT(NPCX_CRSMAE_CFGAE);
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCX_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);

	return data_val;
}

/* Soc specific host sub modules functions */
int soc_host_periph_read_request(enum lpc_peripheral_opcode op,
								uint32_t *data)
{
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFKIE) ||
			!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFMIE)) {
			return -ENOTSUP;
		}

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_OBF);
			break;
		case E8042_IBF_HAS_CHAR:
			*data = IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = inst_kbc->HIKMST;
			break;
		default:
			return -EINVAL;
		}
	} else {
		/* TODO: Support more op code besides 8042 here */
		return -ENOTSUP;
	}

	return 0;
}

int soc_host_periph_write_request(enum lpc_peripheral_opcode op,
								uint32_t *data)
{
	volatile uint32_t __attribute__((unused)) dummy;
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;

	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		/* Make sure kbc 8042 is on */
		if (!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFKIE) ||
			!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFMIE)) {
			return -ENOTSUP;
		}
		if (data)
			LOG_INF("%s: op 0x%x data %x", __func__, op, *data);
		else
			LOG_INF("%s: op 0x%x only", __func__, op);

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			inst_kbc->HIKDO = *data & 0xff;
			/*
			 * Enable KBC OBE interrupt after putting data in
			 * keyboard data register.
			 */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_OBECIE);
			break;
		case E8042_WRITE_MB_CHAR:
			inst_kbc->HIMDO = *data & 0xff;
			/*
			 * Enable KBC OBE interrupt after putting data in
			 * mouse data register.
			 */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_OBECIE);
			break;
		case E8042_RESUME_IRQ:
			/* Enable KBC IBF interrupt */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_IBFCIE);
			break;
		case E8042_PAUSE_IRQ:
			/* Disable KBC IBF interrupt */
			inst_kbc->HICTRL &= ~BIT(NPCX_HICTRL_IBFCIE);
			break;
		case E8042_CLEAR_OBF:
			/* Clear OBF flag in both STATUS and HIKMST to 0 */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_FW_OBF);
			break;
		case E8042_SET_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~NPCX_KBC_STS_MASK;
			inst_kbc->HIKMST |= *data;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			*data &= ~NPCX_KBC_STS_MASK;
			inst_kbc->HIKMST &= ~(*data);
			break;
		default:
			return -EINVAL;
		}
	} else {
		/* TODO: Support more op code besides 8042 here! */
		return -ENOTSUP;
	}

	return 0;
}

void soc_host_init_subs_host_domain(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Enable Core-to-Host access module */
	inst_c2h->SIBCTRL |= BIT(NPCX_SIBCTRL_CSAE);

#if defined(CONFIG_ESPI_PERIPHERAL_8042_KBC)
	/*
	 * Select Keyboard/Mouse banks which LDN are 0x06/05 and enable modules
	 * by setting bit 0 in its Control (index is 0x30) register.
	 */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_KBC);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);

	host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_MOUSE);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
	/*
	 * Select ACPI bank which LDN are 0x11 (PM Channel 1) and enable
	 * module by setting bit 0 in its Control (index is 0x30) register.
	 */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_ACPI);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
	/* Select 'Host Command' bank which LDN are 0x12 (PM Channel 2) */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_HCMD);
	/* Configure IO address of CMD port (0x200) */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H, 0x02);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L, 0x00);
	/* Configure IO address of Data port (0x204) */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H, 0x02);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L, 0x04);
	/* Enable 'Host Command' io port (PM Channel 2) */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);

	/* Select 'Shared Memory' bank which LDN are 0x0F */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_SHM);
	/* WIN 1 & 2 mapping to IO space */
	host_c2h_write_io_cfg_reg(0xF1,
			host_c2h_read_io_cfg_reg(0xF1) | 0x30);
	/* WIN1 as Host Command on the IO address 0x0800 */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_SHM_WND1_ADDR_1, 0x08);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_SHM_WND1_ADDR_0, 0x00);
	/* WIN2 as MEMMAP on the IO address 0x900 */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_SHM_WND2_ADDR_1, 0x09);
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_SHM_WND2_ADDR_0, 0x00);
	/* Enable SHM direct memory access */
	host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, 0x01);
#endif
	LOG_DBG("Hos sub-modules configurations are done!");
}

int soc_host_init_subs_core_domain(const struct device *host_bus_dev,
							sys_slist_t *callbacks)
{
	struct mswc_reg *const inst_mswc = host_sub_cfg.inst_mswc;
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	const struct device *const clk_dev =
					device_get_binding(NPCX_CLK_CTRL_NAME);
	int i;

	host_sub_data.callbacks = callbacks;
	host_sub_data.host_bus_dev = host_bus_dev;

	/* Turn on all host necessary sub-module clocks first */
	for (i = 0; i < host_sub_cfg.clks_size; i++)
		if (clock_control_on(clk_dev, (clock_control_subsys_t *)
				&host_sub_cfg.clks_list[i]) != 0) {
			return -EIO;
		}

	/* Configure EC legacy configuration IO base address to 0x4E. */
	if (!IS_BIT_SET(inst_mswc->MSWCTL1, NPCX_MSWCTL1_VHCFGA)) {
		inst_mswc->HCBAL = NPCX_EC_CFG_IO_ADDR;
		inst_mswc->HCBAH = 0x0;
	}

	/*
	 * Set HOSTWAIT bit and avoid the other settings, then host can freely
	 * communicate with slave (EC).
	 */
	inst_shm->SMC_CTL &= BIT(NPCX_SMC_CTL_HOSTWAIT);
	/* Clear shared memory status */
	inst_shm->SMC_STS = inst_shm->SMC_STS;

	/* host sub-module initialization in core domain */
#if defined(CONFIG_ESPI_PERIPHERAL_8042_KBC)
	host_kbc_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO)
	host_acpi_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
	host_hcmd_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80)
	host_port80_init();
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_UART)
	host_uart_init();
#endif

	/* Host KBC sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_8042_KBC)
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, kbc_ibf, irq),
		    DT_INST_IRQ_BY_NAME(0, kbc_ibf, priority),
		    host_kbc_ibf_isr,
		    NULL, 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, kbc_ibf, irq));

	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, kbc_obe, irq),
		    DT_INST_IRQ_BY_NAME(0, kbc_obe, priority),
		    host_kbc_obe_isr,
		    NULL, 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, kbc_obe, irq));
#endif

	/* Host PM channel (Host IO) sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_HOST_IO) || \
				defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
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

	return 0;
}
