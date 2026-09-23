/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#define DT_DRV_COMPAT nuvoton_npcx_host_sub_v2

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <soc.h>
#include "espi_utils.h"
#include "soc_host.h"
#include "soc_espi.h"
#include "soc_miwu.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(host_sub_npcx_v2, CONFIG_ESPI_LOG_LEVEL);

#define ESPI_HOST_SUB_NODE   DT_NODELABEL(host_sub_v2)
#define HOST_SUB_SHM_NODE    DT_NODELABEL(shm)
#define HOST_SUB_DP80_NODE   DT_NODELABEL(dp80)
#define HOST_SUB_KBC_NODE    DT_NODELABEL(kbc)
#define HOST_SUB_PM_GRP_NODE DT_NODELABEL(pm_group)
#define HOST_SUB_PM_CH1_NODE DT_NODELABEL(pm_ch1)
#define HOST_SUB_PM_CH2_NODE DT_NODELABEL(pm_ch2)
#define HOST_SUB_PM_CH3_NODE DT_NODELABEL(pm_ch3)
#define HOST_SUB_PM_CH4_NODE DT_NODELABEL(pm_ch4)

#define HOST_SUB_HAS_SHM               DT_NODE_HAS_STATUS_OKAY(HOST_SUB_SHM_NODE)
#define HOST_SUB_HAS_DP80              DT_NODE_HAS_STATUS_OKAY(HOST_SUB_DP80_NODE)
#define HOST_SUB_HAS_KBC               DT_NODE_HAS_STATUS_OKAY(HOST_SUB_KBC_NODE)
#define HOST_SUB_HAS_PM_CH1            DT_NODE_HAS_STATUS_OKAY(HOST_SUB_PM_CH1_NODE)
#define HOST_SUB_HAS_PM_CH2            DT_NODE_HAS_STATUS_OKAY(HOST_SUB_PM_CH2_NODE)
#define HOST_SUB_HAS_PM_CH3            DT_NODE_HAS_STATUS_OKAY(HOST_SUB_PM_CH3_NODE)
#define HOST_SUB_HAS_PM_CH4            DT_NODE_HAS_STATUS_OKAY(HOST_SUB_PM_CH4_NODE)
#define HOST_SUB_PM_COMPAT             nuvoton_npcx_host_pm_channel
#define HOST_SUB_HAS_ANY_PM_CH_ENABLED DT_HAS_COMPAT_STATUS_OKAY(HOST_SUB_PM_COMPAT)

/* LDN (Logical Device Number) from devicetree */
#define HOST_SUB_KBC_LDN_MOUSE DT_PROP_BY_IDX(HOST_SUB_KBC_NODE, ldn, 0)
#define HOST_SUB_KBC_LDN_KBC   DT_PROP_BY_IDX(HOST_SUB_KBC_NODE, ldn, 1)
#define HOST_SUB_SHM_LDN       DT_PROP(HOST_SUB_SHM_NODE, ldn)
#define HOST_SUB_PM_CH1_LDN    DT_PROP(HOST_SUB_PM_CH1_NODE, ldn)
#define HOST_SUB_PM_CH2_LDN    DT_PROP(HOST_SUB_PM_CH2_NODE, ldn)
#define HOST_SUB_PM_CH3_LDN    DT_PROP(HOST_SUB_PM_CH3_NODE, ldn)
#define HOST_SUB_PM_CH4_LDN    DT_PROP(HOST_SUB_PM_CH4_NODE, ldn)

/*
 * Macros for extracting opcode base and PM channel index from opcode.
 * The opcode format is: bits[7:0] = base opcode, bits[15:8] = PM channel index
 * PM channel index: 0 = PM_CH1, 1 = PM_CH2, 2 = PM_CH3, 3 = PM_CH4
 */
#define ESPI_OP_GET_BASE(op)   ((op) & 0xFFU)
#define ESPI_OP_GET_PM_IDX(op) (((op) >> 8) & 0xFFU)

#define NPCX_DT_WUI_ITEM_BY_NODE(node_id, name)                                                    \
	{                                                                                          \
		.table = DT_PROP(DT_PHANDLE(DT_PHANDLE(node_id, name), miwus), index),             \
		.group = DT_PHA(DT_PHANDLE(node_id, name), miwus, group),                          \
		.bit = DT_PHA(DT_PHANDLE(node_id, name), miwus, bit),                              \
	}

struct host_sub_npcx_config {
	/* host module instances */
	struct mswc_reg *const inst_mswc;
	struct c2h_reg *const inst_c2h;
	struct shm_reg *const inst_shm;

#if HOST_SUB_HAS_KBC
	struct kbc_reg *const inst_kbc;
#endif

#if HOST_SUB_HAS_PM_CH1
	struct pmch_reg *const inst_pm_ch1;
#endif

#if HOST_SUB_HAS_PM_CH2
	struct pmch_reg *const inst_pm_ch2;
#endif

#if HOST_SUB_HAS_PM_CH3
	struct pmch_reg *const inst_pm_ch3;
#endif

#if HOST_SUB_HAS_PM_CH4
	struct pmch_reg *const inst_pm_ch4;
#endif
	/* mapping table between host access signals and wake-up input */
	struct npcx_wui host_acc_wui;
};

struct host_sub_npcx_data {
	sys_slist_t *callbacks;            /* pointer on the espi callback list */
	uint8_t plt_rst_asserted;          /* current PLT_RST# status */
	uint8_t espi_rst_level;            /* current ESPI_RST# status */
	const struct device *host_bus_dev; /* device for eSPI/LPC bus */
#ifdef CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_MULTI_BYTE
	struct ring_buf port80_ring_buf;
	uint8_t port80_data[CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_RING_BUF_SIZE];
	struct k_work work;
#endif
};

struct npcx_dp80_buf {
	union {
		uint16_t offset_code_16;
		uint8_t offset_code[2];
	};
};

#define NPCX_DT_CLK_CFG_ITEM_BY_IDX_NODE(node_id, idx)                                             \
	{                                                                                          \
		.bus = DT_CLOCKS_CELL_BY_IDX(node_id, idx, bus),                                   \
		.ctrl = DT_CLOCKS_CELL_BY_IDX(node_id, idx, ctl),                                  \
		.bit = DT_CLOCKS_CELL_BY_IDX(node_id, idx, bit),                                   \
	}

#define NPCX_DT_CLK_CFG_ITEMS_LEN_BY_NODE(node_id) DT_PROP_LEN(node_id, clocks)

#define NPCX_DT_CLK_CFG_ITEMS_FUNC_BY_NODE(child, node_id)                                         \
	NPCX_DT_CLK_CFG_ITEM_BY_IDX_NODE(node_id, child)

#define NPCX_DT_CLK_CFG_ITEMS_LIST_BY_NODE(node_id)                                                \
	LISTIFY(NPCX_DT_CLK_CFG_ITEMS_LEN_BY_NODE(node_id),                                        \
		NPCX_DT_CLK_CFG_ITEMS_FUNC_BY_NODE, (,), node_id)

#define NPCX_DT_CLK_CFG_ITEMS_IF_HAS_CLOCKS(node_id)                                               \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks),                                             \
		(NPCX_DT_CLK_CFG_ITEMS_LIST_BY_NODE(node_id),),                                    \
		())

static const struct npcx_clk_cfg host_dev_clk_cfg[] = {
	NPCX_DT_CLK_CFG_ITEMS_LIST_BY_NODE(ESPI_HOST_SUB_NODE),
#if HOST_SUB_HAS_SHM
	NPCX_DT_CLK_CFG_ITEMS_IF_HAS_CLOCKS(HOST_SUB_SHM_NODE)
#endif
#if HOST_SUB_HAS_DP80
		NPCX_DT_CLK_CFG_ITEMS_IF_HAS_CLOCKS(HOST_SUB_DP80_NODE)
#endif
};

struct host_sub_npcx_config host_sub_cfg = {
	.inst_mswc = (struct mswc_reg *)DT_REG_ADDR_BY_NAME(ESPI_HOST_SUB_NODE, mswc),
	.inst_c2h = (struct c2h_reg *)DT_REG_ADDR_BY_NAME(ESPI_HOST_SUB_NODE, c2h),
	.inst_shm = (struct shm_reg *)DT_REG_ADDR(HOST_SUB_SHM_NODE),
#if HOST_SUB_HAS_KBC
	.inst_kbc = (struct kbc_reg *)DT_REG_ADDR(HOST_SUB_KBC_NODE),
#endif
#if HOST_SUB_HAS_PM_CH1
	.inst_pm_ch1 = (struct pmch_reg *)DT_REG_ADDR(HOST_SUB_PM_CH1_NODE),
#endif
#if HOST_SUB_HAS_PM_CH2
	.inst_pm_ch2 = (struct pmch_reg *)DT_REG_ADDR(HOST_SUB_PM_CH2_NODE),
#endif
#if HOST_SUB_HAS_PM_CH3
	.inst_pm_ch3 = (struct pmch_reg *)DT_REG_ADDR(HOST_SUB_PM_CH3_NODE),
#endif
#if HOST_SUB_HAS_PM_CH4
	.inst_pm_ch4 = (struct pmch_reg *)DT_REG_ADDR(HOST_SUB_PM_CH4_NODE),
#endif
	.host_acc_wui = NPCX_DT_WUI_ITEM_BY_NODE(ESPI_HOST_SUB_NODE, host_acc_wui),
};

struct host_sub_npcx_data host_sub_data;

/* Application shouldn't touch these flags in KBC status register directly */
#define NPCX_KBC_STS_MASK (BIT(NPCX_HIKMST_IBF) | BIT(NPCX_HIKMST_OBF) | BIT(NPCX_HIKMST_A2))

/* IO base address of EC Logical Device Configuration */
#define NPCX_EC_CFG_IO_ADDR 0x4E

/* Timeout to wait for Core-to-Host transaction to be completed. */
#define NPCX_C2H_TRANSACTION_TIMEOUT_US 200

/* Logical Device Number Assignments */
#define EC_CFG_LDN_SP 0x03

/* Index of EC (4E/4F) Configuration Register */
#define EC_CFG_IDX_LDN            0x07
#define EC_CFG_IDX_CTRL           0x30
#define EC_CFG_IDX_DATA_IO_ADDR_H 0x60
#define EC_CFG_IDX_DATA_IO_ADDR_L 0x61
#define EC_CFG_IDX_CMD_IO_ADDR_H  0x62
#define EC_CFG_IDX_CMD_IO_ADDR_L  0x63

/* LDN Activation Enable */
#define EC_CFG_IDX_CTRL_LDN_ENABLE 0x01

/* Index of SuperI/O Control and Configuration Registers */
#define EC_CFG_IDX_SUPERIO_SIOCF9      0x29
#define EC_CFG_IDX_SUPERIO_SIOCF9_CKEN 2

/* Index of Special Logical Device Configuration (Shared Memory Module) */
#define EC_CFG_IDX_SHM_CFG             0xF1
#define EC_CFG_IDX_SHM_WND1_ADDR_0     0xF4
#define EC_CFG_IDX_SHM_WND1_ADDR_1     0xF5
#define EC_CFG_IDX_SHM_WND1_ADDR_2     0xF6
#define EC_CFG_IDX_SHM_WND1_ADDR_3     0xF7
#define EC_CFG_IDX_SHM_WND2_ADDR_0     0xF8
#define EC_CFG_IDX_SHM_WND2_ADDR_1     0xF9
#define EC_CFG_IDX_SHM_WND2_ADDR_2     0xFA
#define EC_CFG_IDX_SHM_WND2_ADDR_3     0xFB
#define EC_CFG_IDX_SHM_DP80_ADDR_RANGE 0xFD

/* Index of Special Logical Device Configuration (Serial Port/Host UART) */
#define EC_CFG_IDX_SP_CFG              0xF0
/* Enable selection of bank 2 and 3 for the Serial Port */
#define EC_CFG_IDX_SP_CFG_BK_SL_ENABLE 7

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

#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
/*
 * Get PM channel instance by index.
 * index 0: PM channel 1
 * index 1: PM channel 2
 * index 2: PM channel 3
 * index 3: PM channel 4
 * Returns NULL if the requested channel is not enabled.
 */
static struct pmch_reg *host_get_pm_channel_by_index(uint8_t index)
{
	switch (index) {
#if HOST_SUB_HAS_PM_CH1
	case 0:
		return host_sub_cfg.inst_pm_ch1;
#endif
#if HOST_SUB_HAS_PM_CH2
	case 1:
		return host_sub_cfg.inst_pm_ch2;
#endif
#if HOST_SUB_HAS_PM_CH3
	case 2:
		return host_sub_cfg.inst_pm_ch3;
#endif
#if HOST_SUB_HAS_PM_CH4
	case 3:
		return host_sub_cfg.inst_pm_ch4;
#endif
	default:
		return NULL;
	}
}
#endif /* HOST_SUB_HAS_ANY_PM_CH_ENABLED */

/* Host KBC sub-device local functions */
#if HOST_SUB_HAS_KBC
static void host_kbc_ibf_isr(const void *arg)
{
	ARG_UNUSED(arg);
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, ESPI_PERIPHERAL_8042_KBC,
				 ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	/* KBC Input Buffer Full event */
	kbc_evt->evt = HOST_KBC_EVT_IBF;

	/*
	 * Reading HIKMDI causes the IBF flag to deassert and allows the host to write a new byte
	 * into the input buffer. So, capture the status before reading HIKMDI to make sure we can
	 * get the correct currnt status value.
	 */

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	kbc_evt->type = IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_A2);

	/* The data in KBC Input Buffer */
	kbc_evt->data = inst_kbc->HIKMDI;

	LOG_DBG("%s: kbc data 0x%02x", __func__, evt.evt_data);
	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev, evt);
}

static void host_kbc_obe_isr(const void *arg)
{
	ARG_UNUSED(arg);
	struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION, ESPI_PERIPHERAL_8042_KBC,
				 ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	/* Disable KBC OBE interrupt first */
	inst_kbc->HICTRL &= ~BIT(NPCX_HICTRL_OBECIE);

	LOG_DBG("%s: kbc status 0x%02x", __func__, inst_kbc->HIKMST);

	if (IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
		inst_kbc->HIIRQC &= ~(BIT(NPCX_HIIRQC_IRQ1B) | BIT(NPCX_HIIRQC_IRQ12B));
	}
	/*
	 * Notify application that host already read out data. The application
	 * might need to clear status register via espi_api_lpc_write_request()
	 * with E8042_CLEAR_FLAG opcode in callback.
	 */
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	kbc_evt->data = 0;
	kbc_evt->type = 0;

	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev, evt);
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
	if (IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
		inst_kbc->HICTRL = BIT(NPCX_HICTRL_IBFCIE);
	} else {
		inst_kbc->HICTRL =
			BIT(NPCX_HICTRL_IBFCIE) | BIT(NPCX_HICTRL_OBFMIE) | BIT(NPCX_HICTRL_OBFKIE);
	}
	/* Configure SIRQ 1/12 type (level + high) */
	inst_kbc->HIIRQC = 0x00;
}
#endif

/* Host ACPI sub-device local functions */
#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
static void host_pmch_process_ibf_data(struct pmch_reg *const pm_reg_base_addr,
				       enum espi_virtual_peripheral evt_details)
{
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = evt_details,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;
	uint8_t data;

	/* Set processing flag before reading command byte */
	pm_reg_base_addr->HIPMST |= BIT(NPCX_HIPMST_F0);

	/*
	 * Indicates if the host sent a command or data.
	 * 0 = data
	 * 1 = Command.
	 */
	acpi_evt->type = IS_BIT_SET(pm_reg_base_addr->HIPMST, NPCX_HIPMST_CMD);

	/* Read out input data and clear IBF pending bit */
	data = pm_reg_base_addr->HIPMDI;

	LOG_DBG("%s: acpi data 0x%02x", __func__, data);
	acpi_evt->data = data;

	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev, evt);
}

static void host_pm_ch_init(struct pmch_reg *const pm_reg_base_addr)
{
	/* Use SMI/SCI positive polarity by default */
	pm_reg_base_addr->HIPMCTL &= ~BIT(NPCX_HIPMCTL_SCIPOL);
	pm_reg_base_addr->HIPMIC &= ~BIT(NPCX_HIPMIC_SMIPOL);

	/* Set SMIB/SCIB bits to make sure SMI#/SCI# are driven to high */
	pm_reg_base_addr->HIPMIC |= BIT(NPCX_HIPMIC_SMIB) | BIT(NPCX_HIPMIC_SCIB);

	/*
	 * Allow SMI#/SCI# generated from PM module.
	 * On eSPI bus, we suggest set VW value of SCI#/SMI# directly.
	 */
	pm_reg_base_addr->HIPMIE |= BIT(NPCX_HIPMIE_SCIE);
	pm_reg_base_addr->HIPMIE |= BIT(NPCX_HIPMIE_SMIE);

	/*
	 * Clear processing flag before enabling host's interrupts in case
	 * it's set by the other command during sysjump.
	 */
	pm_reg_base_addr->HIPMST &= ~BIT(NPCX_HIPMST_F0);

	/*
	 * Init ACPI PM channel (Host IO) with:
	 * 1. Enable Input-Buffer Full (IBF) core interrupt.
	 * 2. BIT 7 must be 1.
	 */
	pm_reg_base_addr->HIPMCTL |= BIT(7) | BIT(NPCX_HIPMCTL_IBFIE);
}
#endif

#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
/* Host pm (host io) sub-module isr function for all channels such as ACPI. */
static void host_pmch_ibf_isr(const void *arg)
{
	ARG_UNUSED(arg);

#if HOST_SUB_HAS_PM_CH1
	/* Host put data on input buffer of PM channel 1 */
	if (IS_BIT_SET(host_sub_cfg.inst_pm_ch1->HIPMST, NPCX_HIPMST_IBF)) {
		host_pmch_process_ibf_data(host_sub_cfg.inst_pm_ch1, ESPI_PERIPHERAL_HOST_IO);
	}
#endif

#if HOST_SUB_HAS_PM_CH2
	/* Host put data on input buffer of PM channel 2 */
	if (IS_BIT_SET(host_sub_cfg.inst_pm_ch2->HIPMST, NPCX_HIPMST_IBF)) {
		host_pmch_process_ibf_data(host_sub_cfg.inst_pm_ch2, ESPI_PERIPHERAL_HOST_IO_PVT);
	}
#endif

#if HOST_SUB_HAS_PM_CH3
	/* Host put data on input buffer of PM channel 3 */
	if (IS_BIT_SET(host_sub_cfg.inst_pm_ch3->HIPMST, NPCX_HIPMST_IBF)) {
		host_pmch_process_ibf_data(host_sub_cfg.inst_pm_ch3, ESPI_PERIPHERAL_HOST_IO_PVT2);
	}
#endif

#if HOST_SUB_HAS_PM_CH4
	/* Host put data on input buffer of PM channel 4 */
	if (IS_BIT_SET(host_sub_cfg.inst_pm_ch4->HIPMST, NPCX_HIPMST_IBF)) {
		host_pmch_process_ibf_data(host_sub_cfg.inst_pm_ch4, ESPI_PERIPHERAL_HOST_IO_PVT3);
	}
#endif
}

static void host_pmch_process_obe_data(struct pmch_reg *const pm_reg_base_addr,
				       enum espi_virtual_peripheral evt_details)
{
	struct espi_event evt = {.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 .evt_details = evt_details,
				 .evt_data = ESPI_PERIPHERAL_NODATA};
	struct espi_evt_data_acpi *acpi_evt = (struct espi_evt_data_acpi *)&evt.evt_data;

	/* Disable OBE interrupt first */
	pm_reg_base_addr->HIPMCTL &= ~BIT(NPCX_HIPMCTL_OBEIE);

	LOG_DBG("%s: status 0x%02x", __func__, pm_reg_base_addr->HIPMST);

	/*
	 * Notify application that host already read out data. The application
	 * might need to clear status register or send next byte.
	 */
	acpi_evt->type = ESPI_EVENT_DATA_ACPI_TYPE_HOST_RD_EC_TO_HOST;
	acpi_evt->data = 0;

	espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev, evt);
}

static void host_pmch_obe_isr(const void *arg)
{
	ARG_UNUSED(arg);

#if HOST_SUB_HAS_PM_CH1
	/* Host read data from output buffer of PM channel 1 */
	if (!IS_BIT_SET(host_sub_cfg.inst_pm_ch1->HIPMST, NPCX_HIPMST_OBF) &&
	    IS_BIT_SET(host_sub_cfg.inst_pm_ch1->HIPMCTL, NPCX_HIPMCTL_OBEIE)) {
		host_pmch_process_obe_data(host_sub_cfg.inst_pm_ch1, ESPI_PERIPHERAL_HOST_IO);
	}
#endif

#if HOST_SUB_HAS_PM_CH2
	/* Host read data from output buffer of PM channel 2 */
	if (!IS_BIT_SET(host_sub_cfg.inst_pm_ch2->HIPMST, NPCX_HIPMST_OBF) &&
	    IS_BIT_SET(host_sub_cfg.inst_pm_ch2->HIPMCTL, NPCX_HIPMCTL_OBEIE)) {
		host_pmch_process_obe_data(host_sub_cfg.inst_pm_ch2, ESPI_PERIPHERAL_HOST_IO_PVT);
	}
#endif

#if HOST_SUB_HAS_PM_CH3
	/* Host read data from output buffer of PM channel 3 */
	if (!IS_BIT_SET(host_sub_cfg.inst_pm_ch3->HIPMST, NPCX_HIPMST_OBF) &&
	    IS_BIT_SET(host_sub_cfg.inst_pm_ch3->HIPMCTL, NPCX_HIPMCTL_OBEIE)) {
		host_pmch_process_obe_data(host_sub_cfg.inst_pm_ch3, ESPI_PERIPHERAL_HOST_IO_PVT2);
	}
#endif

#if HOST_SUB_HAS_PM_CH4
	/* Host read data from output buffer of PM channel 4 */
	if (!IS_BIT_SET(host_sub_cfg.inst_pm_ch4->HIPMST, NPCX_HIPMST_OBF) &&
	    IS_BIT_SET(host_sub_cfg.inst_pm_ch4->HIPMCTL, NPCX_HIPMCTL_OBEIE)) {
		host_pmch_process_obe_data(host_sub_cfg.inst_pm_ch4, ESPI_PERIPHERAL_HOST_IO_PVT3);
	}
#endif
}
#endif

/* Host port80 sub-device local functions */
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80
#if defined(CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_MULTI_BYTE)
static void host_port80_work_handler(struct k_work *item)
{
	uint32_t code = 0;
	struct host_sub_npcx_data *data = CONTAINER_OF(item, struct host_sub_npcx_data, work);
	struct ring_buf *rbuf = &data->port80_ring_buf;
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 (ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
				 ESPI_PERIPHERAL_NODATA};

	while (!ring_buf_is_empty(rbuf)) {
		struct npcx_dp80_buf dp80_buf;
		uint8_t offset;

		ring_buf_get(rbuf, &dp80_buf.offset_code[0], sizeof(dp80_buf.offset_code));
		offset = dp80_buf.offset_code[1];
		code |= dp80_buf.offset_code[0] << (8 * offset);
		if (ring_buf_is_empty(rbuf)) {
			evt.evt_data = code;
			espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev,
					    evt);
			break;
		}
		/* peek the offset of the next byte */
		ring_buf_peek(rbuf, &dp80_buf.offset_code[0], sizeof(dp80_buf.offset_code));
		offset = dp80_buf.offset_code[1];
		/*
		 * If the peeked next byte's offset is 0, it is the start of the new code.
		 * Pass the current code to the application layer to handle the Port80 code.
		 */
		if (offset == 0) {
			evt.evt_data = code;
			espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev,
					    evt);
			code = 0;
		}
	}
}
#endif

static void host_port80_isr(const void *arg)
{
	ARG_UNUSED(arg);
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	uint8_t status = inst_shm->DP80STS;

#ifdef CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_MULTI_BYTE
	struct ring_buf *rbuf = &host_sub_data.port80_ring_buf;

	while (IS_BIT_SET(inst_shm->DP80STS, NPCX_DP80STS_FNE)) {
		struct npcx_dp80_buf dp80_buf;

		dp80_buf.offset_code_16 = inst_shm->DP80BUF;
		ring_buf_put(rbuf, &dp80_buf.offset_code[0], sizeof(dp80_buf.offset_code));
	}
	k_work_submit(&host_sub_data.work);
#else
	struct espi_event evt = {ESPI_BUS_PERIPHERAL_NOTIFICATION,
				 (ESPI_PERIPHERAL_INDEX_0 << 16) | ESPI_PERIPHERAL_DEBUG_PORT80,
				 ESPI_PERIPHERAL_NODATA};

	/* Read out port80 data continuously if FIFO is not empty */
	while (IS_BIT_SET(inst_shm->DP80STS, NPCX_DP80STS_FNE)) {
		LOG_DBG("p80: %04x", inst_shm->DP80BUF);
		evt.evt_data = inst_shm->DP80BUF;
		espi_send_callbacks(host_sub_data.callbacks, host_sub_data.host_bus_dev, evt);
	}
#endif
	LOG_DBG("%s: p80 status 0x%02X", __func__, status);

	/* If FIFO is overflow, show error msg */
	if (IS_BIT_SET(status, NPCX_DP80STS_FOR)) {
		inst_shm->DP80STS |= BIT(NPCX_DP80STS_FOR);
		LOG_DBG("Port80 FIFO Overflow!");
	}

	/* If there are pending post codes remains in FIFO after processing and sending previous
	 * post codes, do not clear the FNE bit. This allows this handler to be called again
	 * immediately after it exists.
	 */
	if (!IS_BIT_SET(inst_shm->DP80STS, NPCX_DP80STS_FNE)) {
		/* Clear all pending bit indicates that FIFO was written by host */
		inst_shm->DP80STS |= BIT(NPCX_DP80STS_FWR);
	}
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
	inst_shm->DP80CTL = BIT(NPCX_DP80CTL_CIEN) | BIT(NPCX_DP80CTL_RAA) |
			    BIT(NPCX_DP80CTL_DP80EN) | BIT(NPCX_DP80CTL_SYNCEN);
}
#endif

int espi_host_interrupt_config(uint32_t espi_flags, uint32_t espi_vendor_flags)
{
#if HOST_SUB_HAS_KBC
	if (espi_flags & ESPI_PERIPHERAL_8042_KBC_EVENTS) {
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_ibf, irq));
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_obe, irq));
	} else {
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_ibf, irq));
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_obe, irq));
	}
#endif

#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
	/* Enable/disable host PM channel (Host IO) sub-device interrupt */
	if (espi_flags & ESPI_PERIPHERAL_HOST_IO_EVENTS) {
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, irq));
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, irq));
	} else {
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, irq));
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, irq));
	}
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80
	/* Enable/disable host Port80 sub-device interrupt */
	if (espi_flags & ESPI_PERIPHERAL_DEBUG_PORT80_EVENTS) {
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, irq));
	} else {
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, irq));
	}
#endif

	return 0;
}

#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
static void host_cus_opcode_enable_interrupts(void)
{
	/* Enable host KBC sub-device interrupt */
	if (HOST_SUB_HAS_KBC) {
		irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc), kbc_ibf, irq));
		irq_enable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc), kbc_obe, irq));
	}

	/* Enable host PM channel (Host IO) sub-device interrupt */
	if (HOST_SUB_HAS_ANY_PM_CH_ENABLED) {
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, irq));
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, irq));
	}

	/* Enable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80) {
		irq_enable(DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, irq));
	}

	/* Enable host interface interrupts if its interface is eSPI */
	if (IS_ENABLED(CONFIG_ESPI)) {
		npcx_espi_enable_interrupts(host_sub_data.host_bus_dev);
	}
}

static void host_cus_opcode_disable_interrupts(void)
{
	/* Disable host KBC sub-device interrupt */
	if (HOST_SUB_HAS_KBC) {
		irq_disable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc), kbc_ibf, irq));
		irq_disable(DT_IRQ_BY_NAME(DT_NODELABEL(kbc), kbc_obe, irq));
	}

	/* Disable host PM channel (Host IO) sub-device interrupt */
	if (HOST_SUB_HAS_ANY_PM_CH_ENABLED) {
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, irq));
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, irq));
	}

	/* Disable host Port80 sub-device interrupt installation */
	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80) {
		irq_disable(DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, irq));
	}

	/* Disable host interface interrupts if its interface is eSPI */
	if (IS_ENABLED(CONFIG_ESPI)) {
		npcx_espi_disable_interrupts(host_sub_data.host_bus_dev);
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */

#if defined(CONFIG_ESPI_PERIPHERAL_UART)
/* host uart pinmux configuration */
PINCTRL_DT_DEFINE(DT_INST(0, nuvoton_npcx_host_uart));
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nuvoton_npcx_host_uart) == 1,
	     "only one 'nuvoton_npcx_host_uart' compatible node may be present");
const struct pinctrl_dev_config *huart_cfg =
	PINCTRL_DT_DEV_CONFIG_GET(DT_INST(0, nuvoton_npcx_host_uart));
/* Host UART sub-device local functions */
void host_uart_init(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Configure pin-mux for serial port device */
	pinctrl_apply_state(huart_cfg, PINCTRL_STATE_DEFAULT);

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
	uint32_t max_wait_cycles = k_us_to_cyc_ceil32(NPCX_C2H_TRANSACTION_TIMEOUT_US);

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
	uint32_t max_wait_cycles = k_us_to_cyc_ceil32(NPCX_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NPCX_SIBCTRL_CSRD)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h read transaction expired!");
			break;
		}
	}
}

static void host_c2h_write_io_cfg_reg(uint8_t reg_index, uint8_t reg_data)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Disable interrupts */
	unsigned int key = irq_lock();

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
	unsigned int key = irq_lock();

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

/* Platform specific host sub modules functions */
int npcx_host_periph_read_request(enum lpc_peripheral_opcode opcode, uint32_t *data)
{
	/* Extract base opcode and PM channel index from opcode */
	uint8_t op = ESPI_OP_GET_BASE(opcode);
	__maybe_unused uint8_t pm_idx = ESPI_OP_GET_PM_IDX(opcode);

	if (0) {
		/* Anchor conditional chain for optional branches. */
	}
#if HOST_SUB_HAS_KBC
	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;

		/* Make sure kbc 8042 is on */
		if (!IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
			if (!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFKIE) ||
			    !IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFMIE)) {
				return -ENOTSUP;
			}
		}

		switch (op) {
		case E8042_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			if (IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
				struct espi_reg *const inst_espi =
					(struct espi_reg *)DT_REG_ADDR(DT_NODELABEL(espi0));

				*data = (IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_OBF) ||
					 IS_BIT_SET(inst_espi->STATUS_IMG,
						    NPCX_STATUS_IMG_VWIRE_AVAIL));
			} else {
				*data = IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_OBF);
			}
			break;
		case E8042_IBF_HAS_CHAR:
			*data = IS_BIT_SET(inst_kbc->HIKMST, NPCX_HIKMST_IBF);
			break;
		case E8042_READ_KB_STS:
			*data = inst_kbc->HIKMST;
			break;
		case E8042_READ_KB_CHAR:
		case E8042_READ_MB_CHAR:
			/* Read character from host input data register */
			*data = inst_kbc->HIKMDI;
			break;
		default:
			return -EINVAL;
		}
	}
#endif
	else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
		struct pmch_reg *const inst_acpi = host_get_pm_channel_by_index(pm_idx);

		if (inst_acpi == NULL) {
			return -ENOTSUP;
		}

		/* Make sure pm channel for apci is on */
		if (!IS_BIT_SET(inst_acpi->HIPMCTL, NPCX_HIPMCTL_IBFIE)) {
			return -ENOTSUP;
		}

		switch (op) {
		case EACPI_OBF_HAS_CHAR:
			/* EC has written data back to host. OBF is
			 * automatically cleared after host reads
			 * the data
			 */
			*data = IS_BIT_SET(inst_acpi->HIPMST, NPCX_HIPMST_OBF);
			break;
		case EACPI_IBF_HAS_CHAR:
			*data = IS_BIT_SET(inst_acpi->HIPMST, NPCX_HIPMST_IBF);
			break;
		case EACPI_READ_STS:
			*data = inst_acpi->HIPMST;
			break;
		case EACPI_READ_CHAR:
			/* Read character from host input data register */
			*data = inst_acpi->HIPMDI;
			break;
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION) && HOST_SUB_HAS_SHM
		case EACPI_GET_SHARED_MEMORY:
			*data = (uint32_t)shm_acpi_mmap;
			break;
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION && HOST_SUB_HAS_SHM */
		default:
			return -EINVAL;
		}
#else
		return -ENOTSUP;
#endif
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int npcx_host_periph_write_request(enum lpc_peripheral_opcode opcode, const uint32_t *data)
{
	volatile uint32_t __attribute__((unused)) dummy;

	/* Extract base opcode and PM channel index from opcode */
	uint8_t op = ESPI_OP_GET_BASE(opcode);
	__maybe_unused uint8_t pm_idx = ESPI_OP_GET_PM_IDX(opcode);

	if (0) {
		/* Anchor conditional chain for optional branches. */
	}

#if HOST_SUB_HAS_KBC
	if (op >= E8042_START_OPCODE && op <= E8042_MAX_OPCODE) {
		struct kbc_reg *const inst_kbc = host_sub_cfg.inst_kbc;
		/* Make sure kbc 8042 is on */
		if (!IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
			if (!IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFKIE) ||
			    !IS_BIT_SET(inst_kbc->HICTRL, NPCX_HICTRL_OBFMIE)) {
				return -ENOTSUP;
			}
		}
		if (data) {
			LOG_DBG("op 0x%x data %x", op, *data);
		} else {
			LOG_DBG("op 0x%x only", op);
		}

		switch (op) {
		case E8042_WRITE_KB_CHAR:
			inst_kbc->HIKDO = *data & 0xff;
			/*
			 * Enable KBC OBE interrupt after putting data in
			 * keyboard data register.
			 */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_OBECIE);
			if (IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
				inst_kbc->HIIRQC |= BIT(NPCX_HIIRQC_IRQ1B);
			}
			break;
		case E8042_WRITE_MB_CHAR:
			inst_kbc->HIMDO = *data & 0xff;
			/*
			 * Enable KBC OBE interrupt after putting data in
			 * mouse data register.
			 */
			inst_kbc->HICTRL |= BIT(NPCX_HICTRL_OBECIE);
			if (IS_ENABLED(CONFIG_ESPI_NPCX_i8042_KBC_AUX_VWIRE_IRQ_WORKAROUND)) {
				inst_kbc->HIIRQC |= BIT(NPCX_HIIRQC_IRQ12B);
			}
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
			inst_kbc->HIKMST |= *data & ~NPCX_KBC_STS_MASK;
			break;
		case E8042_CLEAR_FLAG:
			/* FW shouldn't modify these flags directly */
			inst_kbc->HIKMST &= ~(*data | NPCX_KBC_STS_MASK);
			break;
		default:
			return -EINVAL;
		}
	}
#endif
	else if (op >= EACPI_START_OPCODE && op <= EACPI_MAX_OPCODE) {
#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
		struct pmch_reg *const inst_acpi = host_get_pm_channel_by_index(pm_idx);

		if (inst_acpi == NULL) {
			return -ENOTSUP;
		}

		/* Make sure pm channel for apci is on */
		if (!IS_BIT_SET(inst_acpi->HIPMCTL, NPCX_HIPMCTL_IBFIE)) {
			return -ENOTSUP;
		}

		switch (op) {
		case EACPI_WRITE_CHAR:
			inst_acpi->HIPMDO = (*data & 0xff);
			inst_acpi->HIPMCTL |= BIT(NPCX_HIPMCTL_OBEIE);
			break;
		case EACPI_WRITE_STS:
			inst_acpi->HIPMST = (*data & 0xff);
			break;
		case EACPI_RESUME_IRQ:
			/* Enable ACPI IBF interrupt */
			inst_acpi->HIPMCTL |= BIT(NPCX_HIPMCTL_IBFIE);
			break;
		case EACPI_PAUSE_IRQ:
			/* Disable ACPI IBF interrupt */
			inst_acpi->HIPMCTL &= ~BIT(NPCX_HIPMCTL_IBFIE);
			break;
		default:
			return -EINVAL;
		}
#else
		return -ENOTSUP;
#endif
	}
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
	else if (op >= ECUSTOM_START_OPCODE && op <= ECUSTOM_MAX_OPCODE) {
		/* Other customized op codes */

		switch (op) {
		case ECUSTOM_HOST_SUBS_INTERRUPT_EN:
			if (*data != 0) {
				host_cus_opcode_enable_interrupts();
			} else {
				host_cus_opcode_disable_interrupts();
			}
			break;
#if HOST_SUB_HAS_PM_CH2
		case ECUSTOM_HOST_CMD_SEND_RESULT:
			struct pmch_reg *const inst_hcmd = host_sub_cfg.inst_pm_ch2;
			/*
			 * Write result to the data byte.  This sets the TOH
			 * status bit.
			 */
			inst_hcmd->HIPMDO = (*data & 0xff);
			/* Clear processing flag */
			inst_hcmd->HIPMST &= ~BIT(NPCX_HIPMST_F0);
			break;
#endif
		default:
			return -EINVAL;
		}
	}
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE && HOST_SUB_HAS_PM_CH2 */
	else {
		return -ENOTSUP;
	}

	return 0;
}

void npcx_host_init_subs_host_domain(void)
{
	struct c2h_reg *const inst_c2h = host_sub_cfg.inst_c2h;

	/* Enable Core-to-Host access module */
	inst_c2h->SIBCTRL |= BIT(NPCX_SIBCTRL_CSAE);

	if (HOST_SUB_HAS_KBC) {
		/*
		 * Select Keyboard/Mouse banks which LDN are 0x06/05 and enable
		 * modules by setting bit 0 in its Control (index is 0x30) reg.
		 */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_KBC_LDN_KBC);
#if DT_NODE_HAS_PROP(HOST_SUB_KBC_NODE, host_io_data_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_KBC_NODE, host_io_data_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L,
					  DT_PROP(HOST_SUB_KBC_NODE, host_io_data_addr) & 0xff);
#endif

#if DT_NODE_HAS_PROP(HOST_SUB_KBC_NODE, host_io_cmd_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_KBC_NODE, host_io_cmd_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L,
					  DT_PROP(HOST_SUB_KBC_NODE, host_io_cmd_addr) & 0xff);
#endif
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);

		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_KBC_LDN_MOUSE);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	if (HOST_SUB_HAS_PM_CH1) {
		/*
		 * Select ACPI bank which LDN are 0x11 (PM Channel 1) and enable
		 * module by setting bit 0 in its Control (index is 0x30) reg.
		 */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_PM_CH1_LDN);
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH1_NODE, host_io_data_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH1_NODE, host_io_data_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH1_NODE, host_io_data_addr) & 0xff);
#endif
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH1_NODE, host_io_cmd_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH1_NODE, host_io_cmd_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH1_NODE, host_io_cmd_addr) & 0xff);
#endif
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	if (HOST_SUB_HAS_PM_CH2) {
		/*
		 * Select PM Channel 2 bank and enable module by setting bit 0
		 * in its Control (index is 0x30) reg.
		 */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_PM_CH2_LDN);
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH2_NODE, host_io_data_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH2_NODE, host_io_data_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH2_NODE, host_io_data_addr) & 0xff);
#endif
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH2_NODE, host_io_cmd_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH2_NODE, host_io_cmd_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH2_NODE, host_io_cmd_addr) & 0xff);
#endif
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	if (HOST_SUB_HAS_PM_CH3) {
		/*
		 * Select PM Channel 3 bank and enable module by setting bit 0
		 * in its Control (index is 0x30) reg.
		 */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_PM_CH3_LDN);
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH3_NODE, host_io_data_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH3_NODE, host_io_data_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH3_NODE, host_io_data_addr) & 0xff);
#endif
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH3_NODE, host_io_cmd_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH3_NODE, host_io_cmd_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH3_NODE, host_io_cmd_addr) & 0xff);
#endif
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	if (HOST_SUB_HAS_PM_CH4) {
		/*
		 * Select PM Channel 4 bank and enable module by setting bit 0
		 * in its Control (index is 0x30) reg.
		 */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_PM_CH4_LDN);
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH4_NODE, host_io_data_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH4_NODE, host_io_data_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_DATA_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH4_NODE, host_io_data_addr) & 0xff);
#endif
#if DT_NODE_HAS_PROP(HOST_SUB_PM_CH4_NODE, host_io_cmd_addr)
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_H,
					  (DT_PROP(HOST_SUB_PM_CH4_NODE, host_io_cmd_addr) >> 8) &
						  0xff);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CMD_IO_ADDR_L,
					  DT_PROP(HOST_SUB_PM_CH4_NODE, host_io_cmd_addr) & 0xff);
#endif
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	if (IS_ENABLED(CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_MULTI_BYTE)) {
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, HOST_SUB_SHM_LDN);
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_SHM_DP80_ADDR_RANGE, 0x0f);
	}

	if (IS_ENABLED(CONFIG_ESPI_PERIPHERAL_UART)) {
		/* Select Serial Port banks which LDN are 0x03. */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_LDN, EC_CFG_LDN_SP);
		/* Enable SIO_CLK */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_SUPERIO_SIOCF9,
					  host_c2h_read_io_cfg_reg(EC_CFG_IDX_SUPERIO_SIOCF9) |
						  BIT(EC_CFG_IDX_SUPERIO_SIOCF9_CKEN));
		/* Enable Bank Select */
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_SP_CFG,
					  host_c2h_read_io_cfg_reg(EC_CFG_IDX_SP_CFG) |
						  BIT(EC_CFG_IDX_SP_CFG_BK_SL_ENABLE));
		host_c2h_write_io_cfg_reg(EC_CFG_IDX_CTRL, EC_CFG_IDX_CTRL_LDN_ENABLE);
	}

	LOG_DBG("Hos sub-modules configurations are done!");
}

void npcx_host_enable_access_interrupt(void)
{
	npcx_miwu_irq_get_and_clear_pending(&host_sub_cfg.host_acc_wui);

	npcx_miwu_irq_enable(&host_sub_cfg.host_acc_wui);
}

void npcx_host_disable_access_interrupt(void)
{
	npcx_miwu_irq_disable(&host_sub_cfg.host_acc_wui);
}

int npcx_host_init_subs_core_domain(const struct device *host_bus_dev, sys_slist_t *callbacks)
{
	struct mswc_reg *const inst_mswc = host_sub_cfg.inst_mswc;
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	struct shm_reg *const inst_shm = host_sub_cfg.inst_shm;
	uint8_t shm_sts;
	int i;

	host_sub_data.callbacks = callbacks;
	host_sub_data.host_bus_dev = host_bus_dev;

	/* Turn on all host necessary sub-module clocks first */
	if (!device_is_ready(clk_dev)) {
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(host_dev_clk_cfg); i++) {
		int ret = clock_control_on(clk_dev, (clock_control_subsys_t)&host_dev_clk_cfg[i]);

		if (ret < 0) {
			return ret;
		}
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
	shm_sts = inst_shm->SMC_STS;
	inst_shm->SMC_STS = shm_sts;

	/* host sub-module initialization in core domain */
#if HOST_SUB_HAS_KBC
	host_kbc_init();
#endif
#if HOST_SUB_HAS_PM_CH1
	host_pm_ch_init(host_sub_cfg.inst_pm_ch1);
#endif
#if HOST_SUB_HAS_PM_CH2
	host_pm_ch_init(host_sub_cfg.inst_pm_ch2);
#endif
#if HOST_SUB_HAS_PM_CH3
	host_pm_ch_init(host_sub_cfg.inst_pm_ch3);
#endif
#if HOST_SUB_HAS_PM_CH4
	host_pm_ch_init(host_sub_cfg.inst_pm_ch4);
#endif

#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80
	host_port80_init();
#if defined(CONFIG_ESPI_NPCX_PERIPHERAL_DEBUG_PORT_80_MULTI_BYTE)
	ring_buf_init(&host_sub_data.port80_ring_buf, sizeof(host_sub_data.port80_data),
		      host_sub_data.port80_data);
	k_work_init(&host_sub_data.work, host_port80_work_handler);
#endif
#endif
#if defined(CONFIG_ESPI_PERIPHERAL_UART)
	host_uart_init();
#endif

	/* Host KBC sub-device interrupt installation */
#if HOST_SUB_HAS_KBC
	IRQ_CONNECT(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_ibf, irq),
		    DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_ibf, priority), host_kbc_ibf_isr, NULL,
		    0);

	IRQ_CONNECT(DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_obe, irq),
		    DT_IRQ_BY_NAME(HOST_SUB_KBC_NODE, kbc_obe, priority), host_kbc_obe_isr, NULL,
		    0);
#endif

	/* Host PM channel (Host IO) sub-device interrupt installation */
#if HOST_SUB_HAS_ANY_PM_CH_ENABLED
	IRQ_CONNECT(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, irq),
		    DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_ibf, priority), host_pmch_ibf_isr,
		    NULL, 0);

	IRQ_CONNECT(DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, irq),
		    DT_IRQ_BY_NAME(HOST_SUB_PM_GRP_NODE, pmch_obe, priority), host_pmch_obe_isr,
		    NULL, 0);
#endif

	/* Host Port80 sub-device interrupt installation */
#if defined(CONFIG_ESPI_PERIPHERAL_DEBUG_PORT_80) && HOST_SUB_HAS_DP80
	IRQ_CONNECT(DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, irq),
		    DT_IRQ_BY_NAME(HOST_SUB_DP80_NODE, dp80_fifo, priority), host_port80_isr, NULL,
		    0);
#endif

	if (IS_ENABLED(CONFIG_PM)) {
		/*
		 * Configure the host access wake-up event triggered from a host
		 * transaction on eSPI/LPC bus. Do not enable it here. Or plenty
		 * of interrupts will jam the system in S0.
		 */
		npcx_miwu_interrupt_configure(&host_sub_cfg.host_acc_wui, NPCX_MIWU_MODE_EDGE,
					      NPCX_MIWU_TRIG_HIGH);
	}

	return 0;
}
