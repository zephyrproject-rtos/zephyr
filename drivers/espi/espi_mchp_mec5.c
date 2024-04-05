/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_espi

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi/espi_mchp_mec5.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/espi/mchp-mec5-espi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include "espi_utils.h"
#include "espi_mchp_mec5_private.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_retval.h>
#include <mec_pcr_api.h>
#include <mec_espi_api.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

/* #define MEC5_ESPI_DEBUG_VW_TABLE */

/* eSPI virtual wire table entry
 * signal is a enum espi_vwire_signal from espi.h, we hope this enum remains
 * zero based and actual numeric values do not exceed 255.
 * host_idx is the Host Index containing this vwire as defined in the eSPI specification.
 * source is the bit postion [0:3] in the host index and MEC5 HW.
 * reg_idx is the index of the MEC5 vwire register group for this Host Index
 * flags indicate the group is Controller-to-Target or Target-to-Controller, reset source,
 * interrupt detection, etc.
 */
struct espi_mec5_vwire {
	uint8_t signal;
	uint8_t host_idx;
	uint8_t source;
	uint8_t reg_idx;
	uint8_t flags;
};

#define E8042_CHOSEN_NODE_ID	DT_CHOSEN(espi_host_em8042)
#define E8042_DEV_PTR		DEVICE_DT_GET(E8042_CHOSEN_NODE_ID)

#define EACPI_CHOSEN_NODE_ID	DT_CHOSEN(espi_os_acpi)
#define EACPI_DEV_PTR		DEVICE_DT_GET(EACPI_CHOSEN_NODE_ID)

#define HOST_CMD_CHOSEN_NODE_ID DT_CHOSEN(espi_host_cmd_acpi)
#define HOST_CMD_DEV_PTR        DEVICE_DT_GET(HOST_CMD_CHOSEN_NODE_ID)

#define SHM_CHOSEN_NODE_ID	DT_CHOSEN(espi_host_shm)
#define SHM_DEV_PTR		DEVICE_DT_GET(SHM_CHOSEN_NODE_ID)

#define HOST_UART1_CHOSEN_NODE_ID DT_CHOSEN(espi_host_uart1)
#define HOST_UART2_CHOSEN_NODE_ID DT_CHOSEN(espi_host_uart2)
#define ESPI_MBOX_CHOSEN_NODE_ID DT_CHOSEN(espi_host_mailbox)
#define ESPI_P80_CAP_CHOSEN_NODE_ID DT_CHOSEN(espi, host-io-capture)

#define MEC5_DT_ESPI_CT_VWIRES_NODE DT_PATH(mchp_mec5_espi_ct_vwires)
#define MEC5_DT_ESPI_TC_VWIRES_NODE DT_PATH(mchp_mec5_espi_tc_vwires)

/* DT macros used to generate the tables of Controller-to-Target and
 * Target-to-Controller virutal wires enabled on the platform.
 * Table entries depend upon device tree configuration of individual
 * virtual wires and groups.
 */
#define MCHP_DT_ESPI_CTVW_BY_NAME(name) DT_CHILD(MEC5_DT_ESPI_CT_VWIRES_NODE, name)
#define MCHP_DT_ESPI_TCVW_BY_NAME(name) DT_CHILD(MEC5_DT_ESPI_TC_VWIRES_NODE, name)

#define MEC5_VW_SIGNAL(node_id) DT_STRING_UPPER_TOKEN(node_id, vw_name)
#define MEC5_VW_SOURCE(node_id) DT_PROP(node_id, source)

#define MEC5_VW_HOST_IDX(node_id) \
	DT_PROP_BY_PHANDLE(node_id, vw_group, host_index)
#define MEC5_VW_HW_REG_IDX(node_id) \
	DT_PROP_BY_PHANDLE(node_id, vw_group, vw_reg)

#define MEC5_VW_RST_SRC(node_id) \
	DT_ENUM_IDX(DT_PHANDLE(node_id, vw_group), reset_source)

#define MEC5_VW_CT_FLAGS(node_id) \
	 ((DT_ENUM_IDX(DT_PHANDLE_BY_IDX(node_id, vw_group, 0), direction) & 0x1) |\
	  ((DT_PROP(node_id, reset_state) & 0x1) << 1) |\
	  ((MEC5_VW_RST_SRC(node_id) & 0x3) << 2) |\
	  ((DT_ENUM_IDX_OR(node_id, irq_sel, 0) & 0x7) << 4))

#define MEC5_VW_TC_FLAGS(node_id) \
	 ((DT_ENUM_IDX(DT_PHANDLE_BY_IDX(node_id, vw_group, 0), direction) & 0x1) |\
	  ((DT_PROP(node_id, reset_state) & 0x1) << 1) |\
	  ((MEC5_VW_RST_SRC(node_id) & 0x3) << 2))

#define MEC5_ESPI_CTVW_ENTRY(node_id) \
	{\
		.signal = MEC5_VW_SIGNAL(node_id), \
		.host_idx = MEC5_VW_HOST_IDX(node_id), \
		.source = MEC5_VW_SOURCE(node_id), \
		.reg_idx = MEC5_VW_HW_REG_IDX(node_id), \
		.flags = MEC5_VW_CT_FLAGS(node_id), \
	},

#define MEC5_ESPI_TCVW_ENTRY(node_id) \
	{\
		.signal = MEC5_VW_SIGNAL(node_id), \
		.host_idx = MEC5_VW_HOST_IDX(node_id), \
		.source = MEC5_VW_SOURCE(node_id), \
		.reg_idx = MEC5_VW_HW_REG_IDX(node_id), \
		.flags = MEC5_VW_TC_FLAGS(node_id), \
	},

#define MEC5_ESPI_CTVW_FLAGS_DIR(x) ((uint8_t)(x) & 0x01u)
#define MEC5_ESPI_CTVW_FLAGS_RST_STATE(x) (((uint8_t)(x) & 0x02u) >> 1)
#define MEC5_ESPI_CTVW_FLAGS_RST_SRC(x) (((uint8_t)(x) & 0x0cu) >> 2)
#define MEC5_ESPI_CTVW_FLAGS_IRQSEL(x) (((uint8_t)(x) & 0x70u) >> 4)

#define MEC5_ESPI_VW_FLAGS_DIR_IS_TC(x) ((uint8_t)(x) & 0x01u)

const struct espi_mec5_vwire espi_mec5_ct_vwires[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(MEC5_DT_ESPI_CT_VWIRES_NODE, MEC5_ESPI_CTVW_ENTRY)
};

const struct espi_mec5_vwire espi_mec5_tc_vwires[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(MEC5_DT_ESPI_TC_VWIRES_NODE, MEC5_ESPI_TCVW_ENTRY)
};

static struct espi_mec5_vwire const *find_vw(const struct device *dev,
					     enum espi_vwire_signal signal)
{
	size_t nct = ARRAY_SIZE(espi_mec5_ct_vwires);
	size_t ntc = ARRAY_SIZE(espi_mec5_tc_vwires);

	for (size_t n = 0; n < MAX(nct, ntc); n++) {
		if (n < nct) {
			if (signal == (uint8_t)espi_mec5_ct_vwires[n].signal) {
				return &espi_mec5_ct_vwires[n];
			}
		}
		if (n < ntc) {
			if (signal == (uint8_t)espi_mec5_tc_vwires[n].signal) {
				return &espi_mec5_tc_vwires[n];
			}
		}
	}

	return NULL;
}

static int find_ct_vw_signal(uint8_t ctidx, uint8_t ctpos)
{
	size_t nct = ARRAY_SIZE(espi_mec5_ct_vwires);

	for (size_t n = 0; n < nct; n++) {
		if ((ctidx == (uint8_t)espi_mec5_ct_vwires[n].reg_idx) &&
		    (ctpos == (uint8_t)espi_mec5_ct_vwires[n].source)) {
			/* Does the C compiler sign extend 0xff to 0xffffffff ? */
			return (int)espi_mec5_ct_vwires[n].signal;
		}
	}

	return -1;
}

/* Initialize MEC5 eSPI target virtual wire registers static configuration
 * set by DT. Configuration which is not changed by ESPI_nRESET or nPLTRST.
 */
static int espi_mec5_init_vwires(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_vw_regs *vw_regs = devcfg->vwb;
	uint32_t vwcfg = 0;
	int ret = 0;

	for (size_t n = 0; n < ARRAY_SIZE(espi_mec5_ct_vwires); n++) {
		const struct espi_mec5_vwire *vw = &espi_mec5_ct_vwires[n];

		vwcfg = ((((uint32_t)vw->flags >> 2) & 0x3u) << MEC_ESPI_VW_CFG_RSTSRC_POS)
			& MEC_ESPI_VW_CFG_RSTSRC_MSK;
		vwcfg |= BIT(MEC_ESPI_VW_CFG_RSTSRC_DO_POS);
		vwcfg |= ((((uint32_t)vw->flags >> 1) & 0x1u) << MEC_ESPI_VW_CFG_RSTVAL_POS);
		vwcfg |= BIT(MEC_ESPI_VW_CFG_RSTVAL_DO_POS);
		if (!(vw->flags & BIT(0))) { /* Controller-to-Target? */
			vwcfg |= ((((uint32_t)(vw->flags) >> 4) << MEC_ESPI_VW_CFG_IRQSEL_POS)
				  & MEC_ESPI_VW_CFG_IRQSEL_MSK);
			vwcfg |= BIT(MEC_ESPI_VW_CFG_IRQSEL_DO_POS);
		}
		ret = mec_espi_vw_config(vw_regs, vw->reg_idx, vw->source, vw->host_idx, vwcfg);
		if (ret) {
			break;
		}
	}

	return ret;
}

/* CT VWire handlers
 * MEC5_ESPI_NUM_CTVW each with 4 VWires (11 * 4) = 44 entries
 * Each entry:
 *  function pointer: 4 bytes
 *  Total = 176 bytes
 *
 * Maximum Intel defined CT VWires = 16
 *
 * Can we have a common CT VWire handler using parameters to
 * indicate what it should do?
 */


/* SoC devices exposed to the Host via eSPI Peripheral Channel
 * eSPI controller implements:
 * Perpiheral I/O and Memory BARs to map the peripheral to Host address space.
 * Two SRAM BARs allowing SoC memory to be mapped to Host address space with R/W attributes.
 * Serial IRQ Host interrupt mapping for those peripherals capable of generating an interrupt
 * to the Host.
 * NOTE: MCHP eSPI peripheral device I/O BARs, device Memory BARs, SoC SRAM BAR's, and Serial IRQ
 * configuration registers are cleared on assertion of internal signal RESET_HOST/RESET_SIO.
 * This signal is a combination of SoC chip reset, external VCC power good, and platform/PCI reset.
 * For eSPI systems platform reset is usually configured as the PLTRST# virtual wire which defaults
 * to 0 (active). Therefore, BAR's and Serial IRQ can only be configured after RESET_HOST
 * de-asserts. The MCHP PCR Power Control Reset status has a read-only bit indicating the state of
 * RESET_HOST. When RESET_HOST de-asserts, i.e. PLTRST# VWire 0 -> 1 we must configure all eSPI
 * registers affected by the reset.
 */

struct espi_mec5_host_dev_cfg {
	uint32_t temp;
};

struct espi_mec5_host_dev_data {
	const struct device *espi_bus_dev;
};

/* host_dev = pointer to peripheral device's struct device
 * host_addr = Host address
 * ldn = Fixed logical device number of this pc device
 * hdcfg
 *   b[3:0] = number of sirqs (0, 1, or 2)
 *   b[4] = 0(host I/O space), 1(host memory space)
 * sirqs[2] = Serial IRQ slot numbers for up to two
 *            SIRQ's per peripheral.
 */
struct espi_mec5_hdi {
	const struct device *host_dev;
	uint32_t host_addr;
	uint8_t ldn;
	uint8_t hdcfg;
	uint8_t sirqs[2];
};

struct espi_mec5_sram_bar {
	uint32_t host_addr_lsw;
	uint32_t sram_base;
	uint8_t sram_size;
	uint8_t access;
	uint8_t bar_id;
};

#define MEC5_DT_ESPI_HD_NODE DT_PATH(mchp_mec5_espi_host_dev)
#define MEC5_DT_ESPI_SB_NODE DT_PATH(mchp_mec5_espi_sram_bars)

#define MEC5_ESPI_SB_ENTRY(node_id) \
	{ \
		.host_addr_lsw = DT_PROP(node_id, host_address_lsw), \
		.sram_base = DT_PROP(node_id, region_base), \
		.sram_size = DT_ENUM_IDX(node_id, region_size), \
		.access = DT_ENUM_IDX(node_id, access), \
		.bar_id = DT_PROP(node_id, id), \
	},

/* Construct a table of enabled host devices from DTS */
struct espi_mec5_hdi2 {
	const struct device *dev;
};

#define MEC5_ESPI_HDI_NSIRQS2(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, sirqs), (DT_PROP_LEN(node_id, sirqs)), (0))

#define MEC5_ESPI_HDI_CFG2(node_id) \
	((MEC5_ESPI_HDI_NSIRQS(node_id) & 0xfu) \
	 | ((DT_PROP(node_id, host_address_space) & 0x1u) << 4))

#define MEC5_ESPI_HDI_ENTRY2(node_id) \
	{ \
		.dev = (const struct device *)DEVICE_DT_GET(node_id), \
	},

const struct espi_mec5_hdi2 espi_mec5_hdi_tbl2[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(espi0), MEC5_ESPI_HDI_ENTRY2)
};

/* Construct a table of SRAM BARs from DTS */
const struct espi_mec5_sram_bar espi_mec5_sram_bar_tbl[] = {
	DT_FOREACH_CHILD_STATUS_OKAY(MEC5_DT_ESPI_SB_NODE, MEC5_ESPI_SB_ENTRY)
};

static int mec5_pcd_sram_bars(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_mem_regs *memb = devcfg->memb;
	int ret = 0, ret_hal = 0;

	for (size_t n = 0; n < ARRAY_SIZE(espi_mec5_sram_bar_tbl); n++) {
		const struct espi_mec5_sram_bar *sb = &espi_mec5_sram_bar_tbl[n];
		const struct espi_mec5_sram_bar_cfg sbcfg = {
			.haddr = sb->host_addr_lsw,
			.maddr = sb->sram_base,
			.size = sb->sram_size,
			.access = sb->access,
		};

		LOG_DBG("SRAM BAR %u hAddrLsw=0x%0x mAddr=0x%0x sz=%u access=%u", sb->bar_id,
			sb->host_addr_lsw, sb->sram_base, sb->sram_size, sb->access);

		ret_hal = mec_espi_sram_bar_cfg(memb, &sbcfg, sb->bar_id, 1);
		if (ret_hal != MEC_RET_OK) {
			LOG_ERR("SRAM BAR config error!");
			ret = -EIO;
		}
	}

	return 0;
}

/* After eSPI platform reset has de-asserted, configure host access to the
 * peripheral channel devices. Program host address and valid bit in the
 * respective I/O or Memory BAR for the device.
 */
static int mec5_pcd_config_access(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mem_regs *memb = devcfg->memb;
	uint32_t ha_cfg = 0;
	int ret = 0;


	ret = mec_espi_iobar_cfg(iob, MEC_ESPI_LDN_IOC, devcfg->cfg_io_addr, 1u);
	if (ret != MEC_RET_OK) {
		LOG_ERR("eSPI config IO BAR error");
		return -EIO;
	}

	ret = mec_espi_mbar_extended_addr_set(memb, devcfg->membar_hi);
	if (ret) {
		LOG_ERR("LDN Ext MBAR cfg error");
		return -EIO;
	}

	for (size_t n = 0; n < ARRAY_SIZE(espi_mec5_hdi_tbl2); n++) {
		const struct espi_mec5_hdi2 *hdi = &espi_mec5_hdi_tbl2[n];

		if (hdi->dev) {
			ret = espi_pc_host_access(hdi->dev, 1, ha_cfg);
			if (ret) {
				LOG_ERR("PC host access enable error");
			}
		}
	}

	ret = mec_espi_sram_bar_extended_addr_set(memb, devcfg->srambar_hi);
	if (ret) {
		LOG_ERR("SRAM Ext MBAR cfg error");
		ret = -EIO;
	}

	ret = mec5_pcd_sram_bars(dev);
	if (ret) {
		LOG_ERR("SRAM MBAR cfg error");
	}

	return ret;
}

int espi_mec5_bar_config(const struct device *espi_dev, uint32_t haddr, uint32_t cfg)
{
	if (!espi_dev) {
		return -EINVAL;
	}

	const struct espi_mec5_dev_config *devcfg = espi_dev->config;
	uint8_t ldn = (uint8_t)((cfg & ESPI_MEC5_BAR_CFG_LDN_MSK) >> ESPI_MEC5_BAR_CFG_LDN_POS);
	uint8_t enable = (uint8_t)(cfg >> ESPI_MEC5_BAR_CFG_EN_POS) & BIT(0);
	int ret = 0;

	if (cfg & BIT(ESPI_MEC5_BAR_CFG_MEM_BAR_POS)) {
		ret = mec_espi_mbar_cfg(devcfg->memb, ldn, haddr, enable);
	} else {
		ret = mec_espi_iobar_cfg(devcfg->iob, ldn, haddr & 0xffffu, enable);
	}

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

int espi_mec5_sirq_config(const struct device *espi_dev, uint32_t cfg)
{
	if (!espi_dev) {
		return -EINVAL;
	}

	const struct espi_mec5_dev_config *devcfg = espi_dev->config;
	uint8_t ldn = (cfg & ESPI_MEC5_SIRQ_CFG_LDN_MSK) >> ESPI_MEC5_SIRQ_CFG_LDN_POS;
	uint8_t idx = (cfg & ESPI_MEC5_SIRQ_CFG_LDN_IDX_MSK) >> ESPI_MEC5_SIRQ_CFG_LDN_IDX_POS;
	uint8_t slot = (cfg & ESPI_MEC5_SIRQ_CFG_SLOT_MSK) >> ESPI_MEC5_SIRQ_CFG_SLOT_POS;

	mec_espi_ld_sirq_set(devcfg->iob, ldn, idx, slot);

	return 0;
}

/* -------- end eSPI Host Device configuration -------- */

static int espi_mec5_cfg_max_freq(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	uint32_t cap = 0u;
	int ret = 0;

	switch (cfg->max_freq) {
	case 20:
		cap = MEC_ESPI_MAX_SUPP_FREQ_20M;
		break;
	case 25:
		cap = MEC_ESPI_MAX_SUPP_FREQ_25M;
		break;
	case 33:
		cap = MEC_ESPI_MAX_SUPP_FREQ_33M;
		break;
	case 50:
		cap = MEC_ESPI_MAX_SUPP_FREQ_50M;
		break;
	case 66:
		cap = MEC_ESPI_MAX_SUPP_FREQ_66M;
		break;
	default:
		return -EINVAL;
	}

	cap = (cap << MEC_ESPI_CFG_MAX_SUPP_FREQ_POS) & MEC_ESPI_CFG_MAX_SUPP_FREQ_MSK;
	ret = mec_espi_capability_set(iob, MEC_ESPI_CAP_MAX_FREQ, cap);
	if (ret != MEC_RET_OK) {
		ret = -EINVAL;
	}

	return ret;
}

static int espi_mec5_cfg_io_mode(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	uint32_t cap = 0u;
	int ret = 0;

	cap = MEC_ESPI_IO_MODE_1;
	if (cfg->io_caps & ESPI_IO_MODE_DUAL_LINES) {
		cap = MEC_ESPI_IO_MODE_1_2;
		if (cfg->io_caps & ESPI_IO_MODE_QUAD_LINES) {
			cap = MEC_ESPI_IO_MODE_1_2_4;
		}
	} else if (cfg->io_caps & ESPI_IO_MODE_QUAD_LINES) {
		cap = MEC_ESPI_IO_MODE_1_4;
	}

	cap = (cap << MEC_ESPI_CFG_IO_MODE_SUPP_POS) & MEC_ESPI_CFG_IO_MODE_SUPP_MSK;
	ret = mec_espi_capability_set(iob, MEC_ESPI_CAP_IO_MODE, cap);
	if (ret != MEC_RET_OK) {
		return -EINVAL;
	}

	return 0;
}

/* ---------- Peripheral Channel device configuration -------- */

static int mec5_espi_host_dev_config(const struct device *dev)
{
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	struct espi_mec5_data *data = dev->data;
	struct mchp_emi_mem_region mr = { 0 };

	mr.memptr = &data->hcmd_sram[0];
	mr.rdsz = MEC5_ACPI_EC_HCMD_SHM_RD_SIZE;
	mr.wrsz = MEC5_ACPI_EC_HCMD_SHM_WR_SIZE;

	int ret = mchp_espi_pc_emi_config_mem_region(SHM_DEV_PTR, &mr, MCHP_EMI_MR_0);

	if (ret) {
		return ret;
	}

#endif
	return 0;
}

/* ---------- Public API ------------- */
static int espi_mec5_configure(const struct device *dev,
			       struct espi_cfg *cfg)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	uint32_t cap;
	int ret;

	if (!cfg) {
		return -EINVAL;
	}

	ret = espi_mec5_cfg_max_freq(dev, cfg);
	if (ret) {
		LOG_ERR("Max frequency not supported");
		return ret;
	}

	ret = espi_mec5_cfg_io_mode(dev, cfg);
	if (ret) {
		LOG_ERR("IO mode not supported");
		return ret;
	}

	cap = 0;
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		if (!IS_ENABLED(CONFIG_ESPI_PERIPHERAL_CHANNEL)) {
			LOG_ERR("Peripheral channel not supported");
			return -EINVAL;
		}
		cap = BIT(MEC_ESPI_CFG_PERIPH_CHAN_SUP_POS);
	}
	mec_espi_capability_set(iob, MEC_ESPI_CAP_PERIPH_CHAN, cap);

	cap = 0;
	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		if (!IS_ENABLED(CONFIG_ESPI_VWIRE_CHANNEL)) {
			LOG_ERR("VWire channel not supported");
			return -EINVAL;
		}
		cap = BIT(MEC_ESPI_CFG_VW_CHAN_SUP_POS);
	}
	mec_espi_capability_set(iob, MEC_ESPI_CAP_VWIRE_CHAN, cap);

	cap = 0;
	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		if (!IS_ENABLED(CONFIG_ESPI_OOB_CHANNEL)) {
			LOG_ERR("OOB channel not supported");
			return -EINVAL;
		}
		cap = BIT(MEC_ESPI_CFG_OOB_CHAN_SUP_POS);
	}
	mec_espi_capability_set(iob, MEC_ESPI_CAP_OOB_CHAN, cap);

	cap = 0;
	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		if (!IS_ENABLED(CONFIG_ESPI_FLASH_CHANNEL)) {
			LOG_ERR("Flash channel not supported");
			return -EINVAL;
		}
		cap = BIT(MEC_ESPI_CFG_FLASH_CHAN_SUP_POS);
	}
	mec_espi_capability_set(iob, MEC_ESPI_CAP_FLASH_CHAN, cap);

	ret = mec5_espi_host_dev_config(dev);
	if (ret) {
		return ret;
	}

	mec_espi_activate(iob, 1u);
	LOG_DBG("eSPI block activated");

	return 0;
}

static bool espi_mec5_get_chan_status(const struct device *dev,
				      enum espi_channel chan)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	int ready = 0;

	if (chan == ESPI_CHANNEL_PERIPHERAL) {
		ready = mec_espi_pc_is_ready(iob);
	} else if (chan == ESPI_CHANNEL_VWIRE) {
		ready = mec_espi_vw_is_ready(iob);
	} else if (chan == ESPI_CHANNEL_OOB) {
		ready = mec_espi_oob_is_ready(iob);
	} else if (chan == ESPI_CHANNEL_FLASH) {
		ready = mec_espi_fc_is_ready(iob);
	}

	if (ready) {
		return true;
	}

	return false;
}

static int espi_mec5_vw_send(const struct device *dev, enum espi_vwire_signal signal,
			     uint8_t level)
{
	int ret = -ENOTSUP;
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	struct mec_espi_vw mvw = {0};
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_vw_regs *regs = devcfg->vwb;
	const struct espi_mec5_vwire *vw = find_vw(dev, signal);

	if (!vw) {
		return -EINVAL;
	}

	mvw.vwidx = vw->reg_idx;
	mvw.srcidx = vw->source;
	mvw.val = level;

	ret = mec_espi_vw_set_src(regs, &mvw, 0);
	if (ret == MEC_RET_OK) {
		ret = 0;
	} else {
		ret = -EIO;
	}

#endif
	return ret;
}

static int espi_mec5_vw_receive(const struct device *dev, enum espi_vwire_signal signal,
				uint8_t *level)
{
	int ret = -ENOTSUP;
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	struct mec_espi_vw mvw = {0};
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_vw_regs *regs = devcfg->vwb;
	const struct espi_mec5_vwire *vw = find_vw(dev, signal);

	if (!vw || !level) {
		return -EINVAL;
	}

	mvw.vwidx = vw->reg_idx;
	mvw.srcidx = vw->source;

	ret = mec_espi_vw_get_src(regs, &mvw, 0);
	if (ret == MEC_RET_OK) {
		*level = mvw.val;
		ret = 0;
	} else {
		ret = -EIO;
	}

#endif
	return ret;
}

static int espi_mec5_manage_callback(const struct device *dev,
				     struct espi_callback *callback,
				     bool set)
{
	struct espi_mec5_data *const data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

/* Private helpers used by logical device drivers */
void espi_mec5_send_callbacks(const struct device *dev, struct espi_event evt)
{
	struct espi_mec5_data *const data = dev->data;

	return espi_send_callbacks(&data->callbacks, dev, evt);
}

uint32_t espi_mec5_shm_addr_get(const struct device *espi_dev, enum lpc_peripheral_opcode op)
{
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	struct espi_mec5_data *const data = espi_dev->data;
	uint32_t shm_addr = 0;

	if (!espi_dev) {
		return shm_addr;
	}

	if (op == ECUSTOM_HOST_CMD_GET_PARAM_MEMORY) {
		shm_addr = (uint32_t)&data->hcmd_sram[0];
	} else if (op == EACPI_GET_SHARED_MEMORY) {
		shm_addr = (uint32_t)&data->hcmd_sram[MEC5_ACPI_EC_HCMD_SHM_SOFS];
	}

	return shm_addr;
#else
	return 0;
#endif
}

uint32_t espi_mec_shm_size_get(const struct device *espi_dev, enum lpc_peripheral_opcode op)
{
#ifdef CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD
	uint32_t shmsz = 0;

	if (espi_dev) {
		return shmsz;
	}

	if (op == ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE) {
		shmsz = (uint32_t)(CONFIG_ESPI_MEC5_PERIPHERAL_HOST_CMD_PARAM_SIZE);
	} else {
		/* NOTE: there is no opcode for this. Does the application use
		 * the Kconfig buffer size settings directly? If yes, then why
		 * is there a ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE?
		 */
		shmsz = (uint32_t)(CONFIG_ESPI_MEC5_PERIPHERAL_ACPI_SHD_MEM_SIZE);
	}

	return shmsz;
#else
	return 0;
#endif
}
/* end private helpers */

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
/* Peripheral Channel API's allowing access to Host visible SoC peripheral devices.
 * Host visible devices. The eSPI driver LPC read/write request API's pass an
 * enum lpc_peripheral_opcode and a pointer to uint32_t for generic data.
 * Common LPC read/write request helper.
 */
static int espi_mec5_lpc_req(const struct device *dev,
			     enum lpc_peripheral_opcode op,
			     uint32_t *data, uint32_t flags)
{
	int ret = -EINVAL;

	if ((op >= E8042_START_OPCODE) && (op <= E8042_MAX_OPCODE)) {
		ret = mchp_espi_pc_kbc_lpc_request((const struct device *)(E8042_DEV_PTR),
						   op, data, flags);
	} else if ((op >= EACPI_START_OPCODE) && (op <= EACPI_MAX_OPCODE)) {
		ret = mchp_espi_pc_aec_lpc_request((const struct device *)(EACPI_DEV_PTR),
						   op, data, flags);
	} else if ((op >= ECUSTOM_START_OPCODE) && (op <= ECUSTOM_MAX_OPCODE)) {
		ret = mchp_espi_pc_aec_lpc_request((const struct device *)(HOST_CMD_DEV_PTR),
						   op, data, flags);
	}

	return ret;
}

int espi_mec5_lpc_req_rd(const struct device *dev,
			enum lpc_peripheral_opcode op,
			uint32_t *data)
{
	return espi_mec5_lpc_req(dev, op, data, 0);
}

int espi_mec5_lpc_req_wr(const struct device *dev,
			 enum lpc_peripheral_opcode op,
			 uint32_t *data)
{
	return espi_mec5_lpc_req(dev, op, data, BIT(0));
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

/* Called on de-assertion of ESPI_nRESET. Arm our eSPI channels
 * to detect channel set enable from the Host.
 */
static void espi_mec5_arm_chan_enables(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	uint32_t msk = 0;

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	msk = BIT(MEC_ESPI_PC_INTR_CHEN_CHG_POS);
	mec_espi_pc_intr_en(iob, msk);
#endif
#ifdef CONFIG_ESPI_VWIRE_CHANNEL
	if (!mec_espi_vw_is_enabled(iob)) {
		mec_espi_vw_en_ien(1);
	} else {
		mec_espi_vw_en_ien(0);
	}
#endif
#ifdef CONFIG_ESPI_OOB_CHANNEL
	msk = BIT(MEC_ESPI_OOB_UP_INTR_CHEN_CHG_POS);
	mec_espi_oob_intr_ctrl(iob, msk, 1);
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	msk = BIT(MEC_ESPI_FC_INTR_CHEN_CHG_POS);
	mec_espi_fc_intr_ctrl(iob, msk, 1);
#endif
}

#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
/* The eSPI target signals the Host it has completed all its configuration.
 * The Host is then allowed to use all features of the eSPI channels.
 * If the current value of the Boot Done VWire is not asserted (1) then we
 * transmit both Boot Done = 1 and Boot Status = 1 VWires at the same time to the Host.
 * NOTE: The eSPI specification defines Boot Done and Boot Status to be in the same Host Index.
 */
static void send_boot_done_to_host(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_vw_regs *vwregs = devcfg->vwb;
	int ret = 0;
	uint8_t boot_done = 0u, groupval = 0u, groupmsk = 0u;

	ret = espi_mec5_vw_receive(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, &boot_done);
	if (!ret && !boot_done) {
		const struct espi_mec5_vwire *bdone_vw =
			find_vw(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE);
		const struct espi_mec5_vwire *bsts_vw =
			find_vw(dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_STS);

		groupval = BIT(bdone_vw->source) | BIT(bsts_vw->source);
		groupmsk = groupval;
		ret = mec_espi_vw_set_group(vwregs, bdone_vw->host_idx, groupval, groupmsk, 0);
	}
}
#endif

/* ---------- Interrupt Service Routines ---------- */

/* ISR for either edge detection on ESPI_RESET# pin
 * Get pin state, clear interrupt status using HAL.
 * Invoke callback if registered
 * If driver built with OOB channel support initialize OOB
 * If driver built with flash channel support initialize FC
 * NOTE: if ESPI_RESET# is asserted the channels are reset therefore
 * we initialize only on de-assertion. This assumes ESPI_RESET# pulse
 * width is not too short.
 */
static void espi_mec5_ereset_isr(const struct device *dev)
{
	struct espi_mec5_data *data = dev->data;
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_event evt = { ESPI_BUS_RESET, 0, 0 };
	uint32_t erst;

	data->espi_reset_cnt++;

	erst = mec_espi_reset_state(iob);
	mec_espi_reset_change_clr(iob);

	LOG_DBG("ISR ESPI_RESET:0x%02x", erst);

	if (erst & MEC_ESPI_RESET_HI) { /* rising edge */
		data->espi_reset_asserted = 0;
	} else {
		data->espi_reset_asserted = 1u;
		evt.evt_data = 1u;
	}

	if (!evt.evt_data) {
		espi_mec5_arm_chan_enables(dev);
	}

	if (!mec_espi_vw_is_enabled(iob)) {
		mec_espi_vw_en_ien(1u);
	}

	espi_send_callbacks(&data->callbacks, dev, evt);
}

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
/* ISR for Virtual Wire channel enable change by Host.
 * NOTE: HW uses level active signal. Disable after the
 * channel is enabled. We can't detect disable via interrupts.
 */
static void espi_mec5_vw_chen_isr(const struct device *dev)
{	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *ioregs = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = ESPI_CHANNEL_VWIRE,
				  .evt_data = 0 };
	int vw_chan_enable = 0;

	mec_espi_vw_en_ien(0); /* disable level triggered interrupt */
	mec_espi_vw_en_status_clr(); /* clear status */
	vw_chan_enable = mec_espi_vw_is_enabled(ioregs);

	LOG_DBG("ISR VW Chan Enable: en=%d", vw_chan_enable);

	if (vw_chan_enable) {
		mec_espi_vw_ready_set(ioregs);
		evt.evt_data = 1;
#ifdef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
		send_boot_done_to_host(dev);
#endif
	} else { /* VW channel was disabled, safe to re-enable interrupt */
		mec_espi_vw_en_ien(1);
	}

	espi_send_callbacks(&data->callbacks, dev, evt);
}

/* MEC5 Controller to Target VWire groups 0 - 6
 * Special handling of the following Host Controller to Target VWires is required:
 * nPLTRST de-assertion: re-configure eSPI HW cleared by PLTRST assertion.
 * SLP_S3, SLP_S4, or SLP_S5: Record the edge
 * HOST_RST_WARN, SUS_WARN, or DNX_WARN: if CONFIG_ESPI_AUTOMATIC_WARNING_ACK=y then
 * set the corresponding ACK VWire to 1.
 */

/* eSPI bus groups VWires into groups of 4. The protocol allows the Host controller
 * to read/write up to 64 groups in the same bus packet. The VW packet contains an
 * opcode, 0-based VWire group count, sequence of 2-byte VWire groups, response byte,
 * 16-bit status and CRC byte.
 * Note 1: One packet can change the state of multiple VWires.
 * Note 2: the same VW group can be in the packet up to two times to create a pulse.
 *
 * Intel eSPI Compatibility spec. r0p7 states during initial VW sequence for Host attached Flash
 * Target de-asserts RSMRST#
 * Host de-asserts ESPI_RESET#
 * Host begins eSPI link HW training with default settings
 * Host sets max VW group count to 8 (0-based so HW value is 7).
 * Host sets OOB max. packet size to 64 bytes (actually 73 bytes for MCTP)
 * Host sends VW channel enable.
 * Host sends OOB channel enable.
 * Host updates VW and OOB configurations (could increase number of VW groups)
 * Host sends Flash channel enable.
 * Target can load its Firmware via Flash channel
 * Target configures itself.
 * Target sends TARGET_BOOT_LOAD_DONE & TARGET_BOOT_LOAD_STATUS VWire to Host. Host Index 0x05[0,3]
 * Host sends SUS_WARN# VW de-assertion (value=1) to Target. One VW packet. Host Index 0x41[0]
 * Host sends all VWire's with reset values to Target. One or multiple VW packets?
 * Target sends SUS_ACK# de-assertion (value=1) to Host. Host Index 0x40[0]
 * Host sends SLP_S5# VWire de-assertion (1) to Target. Host Index 0x02[2]
 * Host sends SLP_S4# VWire de-assertion (1) to Target. Host Index 0x02[1]
 * Host sends SLP_S3# VWire de-assertion (1) to Target. Host Index 0x02[0]
 * Host sends SLP_A, SLP_WLAN, and SLP_LAN de-assertion to Target. Host Index 0x41[3, 1, 0]
 * Host sends SUS_STAT# de-assertion (1) to Target. Host Index 0x03[0]
 * Host sends PLTRST# de-assertion (1) to Target. Host Index 0x03[1].
 * Target configures its peripherals connected to Peripheral channel.
 * Host performs configuration of devices on Peripheral channel.
 *
 * Special handling of VWires:
 * SLP_S3, SLP_S4, SLP_S3. Record transitions in driver data: 0=none,1=rising edge, 2=falling edge.
 * nPLTRST de-assertion. Invoke code to program peripheral channel devices since their registers
 * are now out of reset.
 */
static void espi_mec5_ctvw_common_isr(const struct device *dev, uint8_t bank)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_vw_regs *vwregs = devcfg->vwb;
	struct espi_mec5_data *data = dev->data;
	struct espi_event evt = { ESPI_BUS_EVENT_VWIRE_RECEIVED, 0, 0 };
	uint32_t result = 0;
	int signal = 0;
	unsigned int pos = 0;
	uint8_t ctidx = MEC_ESPI_VW_MAX_REG_IDX, ctsrc = MEC_ESPI_VW_SOURCE_MAX;

	result = mec_espi_vw_ct_girq_bank_result(bank);
	pos = find_lsb_set(result);

	if (pos) {
		--pos;
		LOG_DBG("ISR VW bank %u result=0x%08x  pos=%d", bank, result, pos);
	} else {
		LOG_ERR("CTVW no ISR result bit!");
		return;
	}

	mec_espi_vw_ct_girq_bank_clr(bank, BIT(pos));
	mec_espi_vw_ct_from_girq_pos(bank, pos, &ctidx, &ctsrc);
	mec_espi_vw_ct_wire_get(vwregs, ctidx, ctsrc, (uint8_t *)&evt.evt_data);

	signal = find_ct_vw_signal(ctidx, ctsrc);
	if (signal < 0) {
		LOG_ERR("CTVW ISR: bad ctidx=%u ctsrc=%u", ctidx, ctsrc);
		return;
	}

	/* special handling */
	if (signal == ESPI_VWIRE_SIGNAL_SLP_S5) {
		data->slp_s5_edge = evt.evt_data ? 1 : 2;
		LOG_DBG("eSPI MEC5 nSLP_S5: %u is 1=RisingEdge, 2=FallingEdge", evt.evt_data);
	} else if (signal == ESPI_VWIRE_SIGNAL_SLP_S4) {
		data->slp_s4_edge = evt.evt_data ? 1 : 2;
		LOG_DBG("eSPI MEC5 nSLP_S4: %u is 1=RisingEdge, 2=FallingEdge", evt.evt_data);
	} else if (signal == ESPI_VWIRE_SIGNAL_SLP_S3) {
		data->slp_s3_edge = evt.evt_data ? 1 : 2;
		LOG_DBG("eSPI MEC5 nSLP_S3: %u is 1=RisingEdge, 2=FallingEdge", evt.evt_data);
	} else if (signal == ESPI_VWIRE_SIGNAL_PLTRST) {
		if (evt.evt_data) {
			mec5_pcd_config_access(dev);
			mec_espi_pc_ready_set(iob);
			LOG_DBG("eSPI MEC5 nPLTRST deasserted");
		}
	}

	evt.evt_details = (uint32_t)signal;
	espi_send_callbacks(&data->callbacks, dev, evt);
}

static void espi_mec5_ctvw_0_6_isr(const struct device *dev)
{
	espi_mec5_ctvw_common_isr(dev, 0);
}

/* MEC5 Controller to Target VWire groups 7 - 10 */
static void espi_mec5_ctvw_7_10_isr(const struct device *dev)
{
	espi_mec5_ctvw_common_isr(dev, 1);
}
#endif /* CONFIG_ESPI_VWIRE_CHANNEL */

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL

/* PC interrupt enable register fields are reset on RESET_ESPI.
 * Peripheral channel enable set by the Host is affected by nPLTRST VWire and
 * ESPI_nRESET.
 * PC enable is forced to 0 if nPLTRST or ESPI_nRESET is asserted.
 * But if either signal de-asserts hardware set PC enable to 1.
 * Host device registers held in reset by nPLTRST are configured when
 * nPLTRST de-asserts.
 * The same registers require configuration if the Host pulses PC Enable
 * while nPLTRST is de-asserted.
 * This routine is called from the PC ISR when PC Enable when PC Enable has
 * a 0 -> 1 transition.
 */
static int espi_mec5_pc_cfg(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;

	LOG_DBG("PC enable 0->1");

	mec_espi_pc_status_clr_all(iob);
	mec_espi_pc_intr_en(iob, BIT(MEC_ESPI_PC_INTR_CHEN_CHG_POS));

	mec5_pcd_config_access(dev);
	mec_espi_pc_ready_set(iob);

	return 0;
}

/* ISR for Peripheral Channel events:
 * Channel enable change by Host
 * Bus Master enable change by Host
 * PC cycle errors
 */
static void espi_mec5_pc_isr(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_event evt = { ESPI_BUS_EVENT_CHANNEL_READY, ESPI_CHANNEL_PERIPHERAL, 1 };
	int ret = 0;
	uint32_t status = mec_espi_pc_status(iob);

	mec_espi_pc_status_clr(iob, status);
	LOG_DBG("ISR PC.Status=0x%0x", status);

	if (status & BIT(MEC_ESPI_PC_ISTS_BERR_POS)) {
		LOG_ERR("PC bus error");
	}

	if (status & BIT(MEC_ESPI_PC_ISTS_CHEN_CHG_POS)) {
		if (status & BIT(MEC_ESPI_PC_ISTS_CHEN_STATE_POS)) {
			ret = espi_mec5_pc_cfg(dev);
			if (ret) {
				LOG_ERR("PC enable: config error");
			}
		} else {
			LOG_DBG("Host disabled PC");
		}
	}

	if (status & BIT(MEC_ESPI_PC_ISTS_BMEN_CHG_POS)) {
		if (status & BIT(MEC_ESPI_PC_ISTS_BMEN_STATE_POS)) {
			/* signal PC bus mastering enabled by Host */
			LOG_WRN("eSPI PC BM enable by Host");
			evt.evt_data = ESPI_PC_EVT_BUS_MASTER_ENABLE;
			espi_send_callbacks(&data->callbacks, dev, evt);
		}
	}
}
#endif /* CONFIG_ESPI_PERIPHERAL_CHANNEL */

/* ---------- Driver API ------------ */
static const struct espi_driver_api espi_mec5_driver_api = {
	.config = espi_mec5_configure,
	.get_channel_status = espi_mec5_get_chan_status,
	.send_vwire = espi_mec5_vw_send,
	.receive_vwire = espi_mec5_vw_receive,
	.manage_callback = espi_mec5_manage_callback,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = mec5_espi_oob_upstream,
	.receive_oob = mec5_espi_oob_downstream,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = mec5_espi_fc_read,
	.flash_write = mec5_espi_fc_write,
	.flash_erase = mec5_espi_fc_erase,
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
	.read_lpc_request = espi_mec5_lpc_req_rd,
	.write_lpc_request = espi_mec5_lpc_req_wr,
#endif
};

/* ---------- Driver Init ------------ */

#ifdef MEC5_ESPI_DEBUG_VW_TABLE
void espi_mec5_debug_vw_table(void)
{
	size_t n;
	size_t tbl_size = ARRAY_SIZE(espi_mec5_ct_vwires);

	LOG_DBG("CT VW table has %u entries", tbl_size);
	for (n = 0; n < tbl_size; n++) {
		const struct espi_mec5_vwire *p = &espi_mec5_ct_vwires[n];

		LOG_DBG("CTVW[%u] signal=%u host_idx=0x%x source=%u reg_idx=%u flags=0x%x",
			n, p->signal, p->host_idx, p->source, p->reg_idx, p->flags);
	}

	tbl_size = ARRAY_SIZE(espi_mec5_tc_vwires);
	LOG_DBG("TC VW table has %u entries", tbl_size);
	for (n = 0; n < tbl_size; n++) {
		const struct espi_mec5_vwire *p = &espi_mec5_tc_vwires[n];

		LOG_DBG("CTVW[%u] signal=%u host_idx=0x%x source=%u reg_idx=%u flags=0x%x",
			n, p->signal, p->host_idx, p->source, p->reg_idx, p->flags);
	}
}
#endif /* MEC5_ESPI_DEBUG_VW_TABLE */

/* Common eSPI interrupt configuration.
 * VW channel enable interrupt is level, disable if channel is enabled.
 * Install interrupt handlers for host devices.
 * Enable interrupts on ESPI_RESET# edges.
 */
static int espi_mec5_irq_config_common(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;

	if (mec_espi_vw_is_enabled(iob)) {
		mec_espi_vw_en_ien(0);
	} else {
		mec_espi_vw_en_ien(1);
	}

	mec_espi_reset_change_intr_en(iob, 1);
	mec_espi_reset_girq_ctrl(1);

	/* VWire GIRQs */
	mec_espi_vw_ct_girq_clr_all();
	mec_espi_vw_ct_girq_ctrl_all(1);

	return 0;
}

/* eSPI driver initialization invoked by zephyr kernel before application main.
 * 1. configure eSPI pins
 * 2. install interrupt service routines.
 * 3. common eSPI subsystem interrupt enables:
 *    if SoC Boot-ROM enabled eSPI to load FW via MAF then disable VW channel
 *    enable interrupt.
 *    for SoC devices visible to host via eSPI configure interrupts
 *    enable ESPI_nRESET interrupt.
 */
static int espi_mec5_dev_init(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;

	int ret;

#ifdef MEC5_ESPI_DEBUG_VW_TABLE
	espi_mec5_debug_vw_table();
#endif
	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("pinctrl dflt state (%d)", ret);
		return ret;
	}

	if (!mec_espi_is_activated(iob)) {
		/* clear spurious edge caused by enabling pins */
		/* DEBUG: Not clearing spurious edge */
		mec_espi_reset_change_clr(iob);
	}

	espi_mec5_init_vwires(dev); /* TODO move to config API */

	if (devcfg->irq_cfg_func) {
		devcfg->irq_cfg_func(dev);
	}

	return 0;
}

#define MEC5_ESPI_IO_BA(inst)						\
	(struct espi_io_regs *)(DT_INST_REG_ADDR_BY_NAME(inst, io))

#define MEC5_ESPI_MEM_BA(inst)						\
	(struct espi_mem_regs *)(DT_INST_REG_ADDR_BY_NAME(inst, mem))

#define MEC5_ESPI_VW_BA(inst)						\
	(struct espi_vw_regs *)(DT_INST_REG_ADDR_BY_NAME(inst, vw))

#define MEC5_ESPI_IRQ_CONNECT(inst)						\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, erst, irq),			\
		DT_INST_IRQ_BY_NAME(inst, erst, priority),			\
		espi_mec5_ereset_isr,						\
		DEVICE_DT_INST_GET(inst), 0);					\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, erst, irq));			\

#ifdef CONFIG_ESPI_PERIPHERAL_CHANNEL
#define MEC5_ESPI_PC_IRQ_CONNECT(inst)						\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, pc, irq),				\
		DT_INST_IRQ_BY_NAME(inst, pc, priority),			\
		espi_mec5_pc_isr,						\
		DEVICE_DT_INST_GET(inst), 0);					\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, pc, irq));				\
	mec_espi_pc_girq_ctrl(1);
#else
#define MEC5_ESPI_PC_IRQ_CONNECT(inst)
#endif

#ifdef CONFIG_ESPI_VWIRE_CHANNEL
#define MEC5_ESPI_VW_IRQ_CONNECT(inst)						\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, vw_chan_en, irq),			\
			DT_INST_IRQ_BY_NAME(inst, vw_chan_en, priority),	\
			espi_mec5_vw_chen_isr,					\
			DEVICE_DT_INST_GET(inst), 0);				\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, vw_chan_en, irq));			\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, vwct_0_6, irq),			\
			DT_INST_IRQ_BY_NAME(inst, vwct_0_6, priority),		\
			espi_mec5_ctvw_0_6_isr,					\
			DEVICE_DT_INST_GET(inst), 0);				\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, vwct_0_6, irq));			\
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, vwct_7_10, irq),			\
		DT_INST_IRQ_BY_NAME(inst, vwct_7_10, priority),			\
		espi_mec5_ctvw_7_10_isr,					\
		DEVICE_DT_INST_GET(inst), 0);					\
	irq_enable(DT_INST_IRQ_BY_NAME(inst, vwct_7_10, irq));
#else
#define MEC5_ESPI_VW_IRQ_CONNECT(inst)
#endif

#ifdef CONFIG_ESPI_OOB_CHANNEL
#define MEC5_ESPI_OOB_IRQ_CONNECT(inst, dev) mec5_espi_oob_irq_connect(dev)
#else
#define MEC5_ESPI_OOB_IRQ_CONNECT(inst, dev)
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
#define MEC5_ESPI_FC_IRQ_CONNECT(inst, dev) mec5_espi_fc_irq_connect(dev)
#else
#define MEC5_ESPI_FC_IRQ_CONNECT(inst)
#endif

#define MEC5_ESPI_DEVICE(inst) \
	static struct espi_mec5_data espi_mec5_data_##inst; \
	PINCTRL_DT_INST_DEFINE(inst); \
	static void espi_mec5_irq_config_##inst(const struct device *dev) \
	{ \
		MEC5_ESPI_IRQ_CONNECT(inst);		\
		MEC5_ESPI_PC_IRQ_CONNECT(inst);		\
		MEC5_ESPI_VW_IRQ_CONNECT(inst);		\
		MEC5_ESPI_OOB_IRQ_CONNECT(inst, dev);	\
		MEC5_ESPI_FC_IRQ_CONNECT(inst, dev);	\
		espi_mec5_irq_config_common(dev);	\
	} \
	static const struct espi_mec5_dev_config espi_mec5_dev_cfg_##inst = { \
		.iob = MEC5_ESPI_IO_BA(inst), \
		.memb = MEC5_ESPI_MEM_BA(inst), \
		.vwb = MEC5_ESPI_VW_BA(inst), \
		.membar_hi = DT_INST_PROP(inst, host_memmap_addr_high),	\
		.srambar_hi = DT_INST_PROP(inst, sram_bar_addr_high),	\
		.cfg_io_addr = DT_INST_PROP(inst, config_io_addr), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst), \
		.irq_cfg_func = espi_mec5_irq_config_##inst, \
	}; \
	DEVICE_DT_INST_DEFINE(inst, \
			      &espi_mec5_dev_init, \
			      NULL, \
			      &espi_mec5_data_##inst, \
			      &espi_mec5_dev_cfg_##inst, \
			      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, \
			      &espi_mec5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_ESPI_DEVICE)
