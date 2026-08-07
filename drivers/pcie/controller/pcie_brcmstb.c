/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcie_brcmstb, LOG_LEVEL_ERR);

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/sys/device_mmio.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>

#define DT_DRV_COMPAT brcm_brcmstb_pcie

#define PCIE_RC_PL_PHY_CTL_15                    0x184c
#define PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD_MASK 0xff

#define PCIE_MISC_MISC_CTRL                       0x4008
#define PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK    0x1000
#define PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK 0x2000
#define PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK   0x300000
#define PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_LSB    20
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK        0xf8000000
#define PCIE_MISC_MISC_CTRL_SCB0_SIZE_LSB         27

#define PCIE_MISC_RC_BAR_CONFIG_LO_SIZE_MASK 0x1f

#define PCIE_MISC_RC_BAR1_CONFIG_LO           0x402c
#define PCIE_MISC_RC_BAR1_CONFIG_LO_SIZE_MASK 0x1f

#define PCIE_MISC_RC_BAR2_CONFIG_LO           0x4034
#define PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK 0x1f
#define PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_LSB  0
#define PCIE_MISC_RC_BAR2_CONFIG_HI           0x4038

#define PCIE_MISC_RC_BAR3_CONFIG_LO           0x403c
#define PCIE_MISC_RC_BAR3_CONFIG_LO_SIZE_MASK 0x1f

#define PCIE_MISC_RC_BAR4_CONFIG_LO 0x40d4
#define PCIE_MISC_RC_BAR4_CONFIG_HI 0x40d8

#define PCIE_MISC_UBUS_BAR_CONFIG_REMAP_ENABLE  0x1
#define PCIE_MISC_UBUS_BAR_CONFIG_REMAP_LO_MASK 0xfffff000
#define PCIE_MISC_UBUS_BAR_CONFIG_REMAP_HI_MASK 0xff

#define PCIE_MISC_UBUS_BAR2_CONFIG_REMAP                    0x40b4
#define PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_ACCESS_ENABLE_MASK 0x1

#define PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_LO 0x410c
#define PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_HI 0x4110

#define PCIE_MISC_UBUS_CTRL                                 0x40a4
#define PCIE_MISC_UBUS_CTRL_UBUS_PCIE_REPLY_ERR_DIS_MASK    0x2000
#define PCIE_MISC_UBUS_CTRL_UBUS_PCIE_REPLY_DECERR_DIS_MASK 0x80000

#define PCIE_MISC_AXI_READ_ERROR_DATA     0x4170
#define PCIE_MISC_UBUS_TIMEOUT            0x40a8
#define PCIE_MISC_RC_CONFIG_RETRY_TIMEOUT 0x405c

#define PCIE_MISC_PCIE_CTRL                  0x4064
#define PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_MASK 0x4

#define PCIE_RC_CFG_PRIV1_ID_VAL3                 0x043c
#define PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE_MASK 0xffffff

#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1                       0x0188
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_MASK 0xc
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_LSB  2
#define PCIE_RC_CFG_VENDOR_SPECIFIC_REG1_LITTLE_ENDIAN                0x0

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO 0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI 0x4010
#define PCIE_MEM_WIN0_LO(win)            (PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO + (win) * 8)
#define PCIE_MEM_WIN0_HI(win)            (PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI + (win) * 8)

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT            0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_MASK 0xfff00000
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_LSB  20
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_MASK  0xfff0
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_LSB   4

#define PCIE_MEM_WIN0_BASE_LIMIT(win) (PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT + (win) * 4)

/* Hamming weight of PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_MASK */
#define HIGH_ADDR_SHIFT 12

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI           0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_MASK 0xff
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_LSB  0

#define PCIE_MEM_WIN0_BASE_HI(win) (PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI + (win) * 8)

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI            0x4084
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_MASK 0xff
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_LSB  0

#define PCIE_MEM_WIN0_LIMIT_HI(win) (PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI + (win) * 8)

#define PCIE_EXT_CFG_DATA  0x8000
#define PCIE_EXT_CFG_INDEX 0x9000

#define PCI_BASE_ADDRESS_0 0x10

#define PCI_COMMAND        0x0004
#define PCI_COMMAND_MEMORY 0x2
#define PCI_COMMAND_MASTER 0x4

#define PCI_EXP_LNKCAP     0x0c
#define PCI_EXP_LNKCAP_SLS 0xf
#define PCI_EXP_LNKCTL2    0x30

#define BRCM_PCIE_CAP_REGS 0x00ac

#define BCM2712_RC_BAR2_SIZE   0x400000
#define BCM2712_RC_BAR2_OFFSET 0x0
#define BCM2712_RC_BAR4_CPU    0x0
#define BCM2712_RC_BAR4_SIZE   0x0
#define BCM2712_RC_BAR4_PCI    0x0
#define BCM2712_SCB0_SIZE      0x400000

#define BCM2712_BURST_SIZE 0x1

#define BCM2712_CLOCK_RATE 750000000ULL /* 750Mhz */

#define BCM2712_UBUS_TIMEOUT_NS    250000000ULL /* 250ms */
#define BCM2712_UBUS_TIMEOUT_TICKS (BCM2712_UBUS_TIMEOUT_NS * BCM2712_CLOCK_RATE / 1000000000ULL)

#define BCM2712_RC_CONFIG_RETRY_TIMEOUT_NS 240000000ULL /* 240ms */
#define BCM2712_RC_CONFIG_RETRY_TIMEOUT_TICKS                                                      \
	(BCM2712_RC_CONFIG_RETRY_TIMEOUT_NS * BCM2712_CLOCK_RATE / 1000000000ULL)

#define BCM2712_PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE 0x060400

#define MDIO_DATA_DONE_MASK 0x80000000
#define MDIO_CMD_WRITE      0x0
#define MDIO_PORT0          0x0

#define PCIE_RC_DL_MDIO_ADDR                0x1100
#define PCIE_RC_DL_MDIO_WR_DATA             0x1104
#define PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD 0x12 /* 18.52ns as ticks */

#define SET_ADDR_OFFSET 0x1f

#define DMA_RANGES_IDX 2

#define PCIE_ECAM_BDF_SHIFT 12

#define BAR_MAX 8

#define SZ_1M 0x100000

struct pcie_brcmstb_config {
	const struct pcie_ctrl_config *common;
	size_t regs_count;
	struct {
		uintptr_t addr;
		size_t size;
	} regs[BAR_MAX];
};

enum pcie_region_type {
	PCIE_REGION_IO = 0,
	PCIE_REGION_MEM,
	PCIE_REGION_MEM64,
	PCIE_REGION_MAX,
};

struct pcie_brcmstb_data {
	uintptr_t cfg_phys_addr;
	mm_reg_t cfg_addr;
	size_t cfg_size;
	struct {
		uintptr_t phys_start;
		uintptr_t bus_start;
		size_t size;
		size_t allocation_offset;
	} regions[PCIE_REGION_MAX];
	size_t bar_cnt;
};

static inline uint32_t lower_32_bits(uint64_t val)
{
	return val & 0xffffffff;
}

static inline uint32_t upper_32_bits(uint64_t val)
{
	return (val >> 32) & 0xffffffff;
}

static uint32_t encode_ibar_size(uint64_t size)
{
	uint32_t tmp;
	uint32_t size_upper = (uint32_t)(size >> 32);

	if (size_upper > 0) {
		tmp = ilog2(size_upper) + 32;
	} else {
		tmp = ilog2(size);
	}

	if (tmp >= 12 && tmp <= 15) {
		return (tmp - 12) + 0x1c;
	} else if (tmp >= 16 && tmp <= 36) {
		return tmp - 15;
	}

	return 0;
}

static mm_reg_t pcie_brcmstb_map_bus(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct pcie_brcmstb_data *data = dev->data;

	sys_write32(bdf << PCIE_ECAM_BDF_SHIFT, data->cfg_addr + PCIE_EXT_CFG_INDEX);
	return data->cfg_addr + PCIE_EXT_CFG_DATA + reg * sizeof(uint32_t);
}

static uint32_t pcie_brcmstb_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	mm_reg_t conf_addr = pcie_brcmstb_map_bus(dev, bdf, reg);

	if (!conf_addr) {
		return 0xffffffff;
	}

	return sys_read32(conf_addr);
}

void pcie_brcmstb_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
			     uint32_t data)
{
	mm_reg_t conf_addr = pcie_brcmstb_map_bus(dev, bdf, reg);

	if (!conf_addr) {
		return;
	}

	sys_write32(data, conf_addr);
}

static inline enum pcie_region_type pcie_brcmstb_determine_region_type(const struct device *dev,
								       bool mem, bool mem64)
{
	struct pcie_brcmstb_data *data = dev->data;

	if (!mem) {
		return PCIE_REGION_IO;
	}

	if ((data->regions[PCIE_REGION_MEM64].size > 0) &&
	    (mem64 || data->regions[PCIE_REGION_MEM].size == 0)) {
		return PCIE_REGION_MEM64;
	}

	return PCIE_REGION_MEM;
}

static bool pcie_brcmstb_region_allocate_type(const struct device *dev, pcie_bdf_t bdf,
					      size_t bar_size, uintptr_t *bar_bus_addr,
					      enum pcie_region_type type)
{
	const struct pcie_brcmstb_config *config = dev->config;
	struct pcie_brcmstb_data *data = dev->data;
	uintptr_t addr;

	addr = (((data->regions[type].bus_start + config->regs[PCIE_BDF_TO_BUS(bdf) + 1].addr +
		  data->regions[type].allocation_offset) -
		 1) |
		((bar_size)-1)) +
	       1;

	if (addr + bar_size > data->regions[type].bus_start + data->regions[type].size) {
		return false;
	}

	*bar_bus_addr = addr;

	return true;
}

static bool pcie_brcmstb_region_allocate(const struct device *dev, pcie_bdf_t bdf, bool mem,
					 bool mem64, size_t bar_size, uintptr_t *bar_bus_addr)
{
	struct pcie_brcmstb_data *data = dev->data;
	enum pcie_region_type type;

	if (!mem && mem64) {
		return false;
	}

	if (mem && data->regions[PCIE_REGION_MEM64].size == 0 &&
	    data->regions[PCIE_REGION_MEM].size == 0) {
		return false;
	}

	if (!mem && data->regions[PCIE_REGION_IO].size == 0) {
		return false;
	}

	type = pcie_brcmstb_determine_region_type(dev, mem, mem64);

	return pcie_brcmstb_region_allocate_type(dev, bdf, bar_size, bar_bus_addr, type);
}

static bool pcie_brcmstb_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf,
						  bool mem, bool mem64, size_t align,
						  uintptr_t *bar_base_addr)
{
	struct pcie_brcmstb_data *data = dev->data;
	enum pcie_region_type type;

	if (!mem && mem64) {
		return false;
	}

	if (mem && data->regions[PCIE_REGION_MEM64].size == 0 &&
	    data->regions[PCIE_REGION_MEM].size == 0) {
		return false;
	}

	if (!mem && data->regions[PCIE_REGION_IO].size == 0) {
		return false;
	}

	type = pcie_brcmstb_determine_region_type(dev, mem, mem64);

	*bar_base_addr =
		(((data->regions[type].bus_start + data->regions[type].allocation_offset) - 1) |
		 ((align)-1)) +
		1;

	return true;
}

static bool pcie_brcmstb_region_translate(const struct device *dev, pcie_bdf_t bdf, bool mem,
					  bool mem64, uintptr_t bar_bus_addr, uintptr_t *bar_addr)
{
	struct pcie_brcmstb_data *data = dev->data;
	enum pcie_region_type type;

	type = pcie_brcmstb_determine_region_type(dev, mem, mem64);

	*bar_addr = data->regions[type].phys_start + (bar_bus_addr - data->regions[type].bus_start);

	return true;
}

static DEVICE_API(pcie_ctrl, pcie_brcmstb_api) = {
	.conf_read = pcie_brcmstb_conf_read,
	.conf_write = pcie_brcmstb_conf_write,
	.region_allocate = pcie_brcmstb_region_allocate,
	.region_get_allocate_base = pcie_brcmstb_region_get_allocate_base,
	.region_translate = pcie_brcmstb_region_translate,
};

static int pcie_brcmstb_parse_regions(const struct device *dev)
{
	const struct pcie_brcmstb_config *config = dev->config;
	struct pcie_brcmstb_data *data = dev->data;
	enum pcie_region_type type;
	int i;

	for (i = 0; i < DMA_RANGES_IDX; i++) {
		switch ((config->common->ranges[i].flags >> 24) & 0x03) {
		case 0x01:
			type = PCIE_REGION_IO;
			break;
		case 0x02:
			type = PCIE_REGION_MEM;
			break;
		case 0x03:
			type = PCIE_REGION_MEM64;
			break;
		default:
			continue;
		}
		data->regions[type].bus_start = config->common->ranges[i].pcie_bus_addr;
		data->regions[type].phys_start = config->common->ranges[i].host_map_addr;
		data->regions[type].size = config->common->ranges[i].map_length;
	}

	if (!data->regions[PCIE_REGION_IO].size && !data->regions[PCIE_REGION_MEM].size &&
	    !data->regions[PCIE_REGION_MEM64].size) {
		return -EINVAL;
	}

	return 0;
}

static mm_reg_t pcie_brcmstb_mdio_from_pkt(int port, int regad, int cmd)
{
	return (mm_reg_t)cmd << 20 | port << 16 | regad;
}

static void pcie_brcmstb_mdio_write(mm_reg_t base, uint8_t port, uint8_t regad, uint16_t wrdata)
{
	sys_write32(pcie_brcmstb_mdio_from_pkt(port, regad, MDIO_CMD_WRITE),
		    base + PCIE_RC_DL_MDIO_ADDR);
	sys_write32(MDIO_DATA_DONE_MASK | wrdata, base + PCIE_RC_DL_MDIO_WR_DATA);
}

static void pcie_brcmstb_munge_pll(const struct device *dev)
{
	struct pcie_brcmstb_data *data = dev->data;
	int i;

	uint8_t regs[] = {0x16, 0x17, 0x18, 0x19, 0x1b, 0x1c, 0x1e};
	uint16_t vals[] = {0x50b9, 0xbda1, 0x0094, 0x97b4, 0x5030, 0x5030, 0x0007};

	pcie_brcmstb_mdio_write(data->cfg_addr, MDIO_PORT0, SET_ADDR_OFFSET, 0x1600);
	for (i = 0; i < 7; i++) {
		k_busy_wait(300);
		pcie_brcmstb_mdio_write(data->cfg_addr, MDIO_PORT0, regs[i], vals[i]);
	}
}

static void pcie_brcmstb_set_outbound_win(const struct device *dev, uint8_t win, uintptr_t cpu_addr,
					  uintptr_t pcie_addr, size_t size)
{
	struct pcie_brcmstb_data *data = dev->data;
	uint32_t cpu_addr_mb_high, limit_addr_mb_high;
	uintptr_t cpu_addr_mb, limit_addr_mb;
	uint32_t tmp, tmp2;

	sys_write32(lower_32_bits(pcie_addr), data->cfg_addr + PCIE_MEM_WIN0_LO(win));
	sys_write32(upper_32_bits(pcie_addr), data->cfg_addr + PCIE_MEM_WIN0_HI(win));

	cpu_addr_mb = cpu_addr / SZ_1M;
	limit_addr_mb = (cpu_addr + size - 1) / SZ_1M;

	tmp = sys_read32(data->cfg_addr + PCIE_MEM_WIN0_BASE_LIMIT(win));
	tmp &= ~PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_MASK;
	tmp &= ~PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_MASK;
	tmp2 = (cpu_addr_mb << PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_LSB);
	tmp2 &= PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_BASE_MASK;
	tmp |= tmp2;
	tmp2 = (limit_addr_mb << PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_LSB);
	tmp2 &= PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT_LIMIT_MASK;
	tmp |= tmp2;
	sys_write32(tmp, data->cfg_addr + PCIE_MEM_WIN0_BASE_LIMIT(win));

	cpu_addr_mb_high = cpu_addr_mb >> HIGH_ADDR_SHIFT;
	tmp = sys_read32(data->cfg_addr + PCIE_MEM_WIN0_BASE_HI(win));
	tmp &= ~PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_MASK;
	tmp |= (cpu_addr_mb_high << PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI_BASE_LSB);
	sys_write32(tmp, data->cfg_addr + PCIE_MEM_WIN0_BASE_HI(win));

	limit_addr_mb_high = limit_addr_mb >> HIGH_ADDR_SHIFT;
	tmp = sys_read32(data->cfg_addr + PCIE_MEM_WIN0_LIMIT_HI(win));
	tmp &= ~PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_MASK;
	tmp |= (cpu_addr_mb_high << PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI_LIMIT_LSB);
	sys_write32(tmp, data->cfg_addr + PCIE_MEM_WIN0_LIMIT_HI(win));
}

static int pcie_brcmstb_setup(const struct device *dev)
{
	const struct pcie_brcmstb_config *config = dev->config;
	struct pcie_brcmstb_data *data = dev->data;
	uint32_t tmp;
	uint16_t tmp16;

	/* This block is for BCM2712 only */
	pcie_brcmstb_munge_pll(dev);
	tmp = sys_read32(data->cfg_addr + PCIE_RC_PL_PHY_CTL_15);
	tmp &= ~PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD_MASK;
	tmp |= PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD;
	sys_write32(tmp, data->cfg_addr + PCIE_RC_PL_PHY_CTL_15);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp |= PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK;
	tmp |= PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK;
	tmp &= ~PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK;
	tmp |= (BCM2712_BURST_SIZE << PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_LSB);
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_MISC_CTRL);

	uint64_t rc_bar2_offset = config->common->ranges[DMA_RANGES_IDX].host_map_addr -
				  config->common->ranges[DMA_RANGES_IDX].pcie_bus_addr;
	uint64_t rc_bar2_size = config->common->ranges[DMA_RANGES_IDX].map_length;

	tmp = lower_32_bits(rc_bar2_offset);
	tmp &= ~PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK;
	tmp |= encode_ibar_size(rc_bar2_size) << PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_LO);
	sys_write32(upper_32_bits(rc_bar2_offset), data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_HI);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);
	tmp |= PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_ACCESS_ENABLE_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);

	/* Set SCB Size */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp &= ~PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK;
	tmp |= (ilog2(config->common->ranges[DMA_RANGES_IDX].map_length) - 15)
	       << PCIE_MISC_MISC_CTRL_SCB0_SIZE_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_MISC_CTRL);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_UBUS_CTRL);
	tmp |= PCIE_MISC_UBUS_CTRL_UBUS_PCIE_REPLY_ERR_DIS_MASK;
	tmp |= PCIE_MISC_UBUS_CTRL_UBUS_PCIE_REPLY_DECERR_DIS_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_UBUS_CTRL);
	sys_write32(0xffffffff, data->cfg_addr + PCIE_MISC_AXI_READ_ERROR_DATA);

	/* Set timeouts */
	sys_write32(BCM2712_UBUS_TIMEOUT_TICKS, data->cfg_addr + PCIE_MISC_UBUS_TIMEOUT);
	sys_write32(BCM2712_RC_CONFIG_RETRY_TIMEOUT_TICKS,
		    data->cfg_addr + PCIE_MISC_RC_CONFIG_RETRY_TIMEOUT);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_RC_BAR1_CONFIG_LO);
	tmp &= ~PCIE_MISC_RC_BAR1_CONFIG_LO_SIZE_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR1_CONFIG_LO);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_RC_BAR3_CONFIG_LO);
	tmp &= ~PCIE_MISC_RC_BAR3_CONFIG_LO_SIZE_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR3_CONFIG_LO);

	/* Set gen to 2 */
	tmp16 = sys_read16(data->cfg_addr + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCTL2);
	tmp = sys_read32(data->cfg_addr + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCAP);
	tmp &= ~PCI_EXP_LNKCAP_SLS;
	tmp |= 0x2;
	sys_write32(tmp, data->cfg_addr + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCAP);
	tmp16 &= ~0xf;
	tmp16 |= 0x2;
	sys_write16(tmp16, data->cfg_addr + BRCM_PCIE_CAP_REGS + PCI_EXP_LNKCTL2);

	tmp = sys_read32(data->cfg_addr + PCIE_RC_CFG_PRIV1_ID_VAL3);
	tmp &= ~PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE_MASK;
	tmp |= BCM2712_PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE;
	sys_write32(tmp, data->cfg_addr + PCIE_RC_CFG_PRIV1_ID_VAL3);

	tmp = sys_read32(data->cfg_addr + PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1);
	tmp &= ~PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_MASK;
	tmp |= PCIE_RC_CFG_VENDOR_SPECIFIC_REG1_LITTLE_ENDIAN
	       << PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1);

	return 0;
}

static int pcie_brcmstb_init(const struct device *dev)
{
	const struct pcie_brcmstb_config *config = dev->config;
	struct pcie_brcmstb_data *data = dev->data;
	uint32_t tmp;
	int ret;

	if (config->common->ranges_count < DMA_RANGES_IDX) {
		/* Workaround since macros for `dma-ranges` property is not available */
		return -EINVAL;
	}

	ret = pcie_brcmstb_parse_regions(dev);
	if (ret != 0) {
		return ret;
	}

	data->cfg_phys_addr = config->common->cfg_addr;
	data->cfg_size = config->common->cfg_size;

	device_map(&data->cfg_addr, data->cfg_phys_addr, data->cfg_size, K_MEM_CACHE_NONE);

	/* PCIe Setup */
	pcie_brcmstb_setup(dev);

	/* Assert PERST# */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_PCIE_CTRL);
	tmp |= PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_PCIE_CTRL);

	k_busy_wait(500000);

	/* Enable resources and bus-mastering */
	tmp = sys_read32(data->cfg_addr + PCI_COMMAND);
	tmp |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	sys_write32(tmp, data->cfg_addr + PCI_COMMAND);

	for (int i = 0; i < DMA_RANGES_IDX; i++) {
		pcie_brcmstb_set_outbound_win(dev, i, config->common->ranges[i].host_map_addr,
					      config->common->ranges[i].pcie_bus_addr,
					      config->common->ranges[i].map_length);
	}

	/* Assign BARs */
	/* TODO: It might be possible to do this without extra <regs> property */
	for (int i = 1; i < config->regs_count; i++) {
		sys_write32(config->regs[i].addr, data->cfg_addr + PCIE_EXT_CFG_DATA +
							  PCI_BASE_ADDRESS_0 + 0x4 * (i - 1));
	}

	/* Enable resources */
	tmp = sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);
	tmp |= PCI_COMMAND_MEMORY;
	sys_write32(tmp, data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);
	k_busy_wait(500000);

	return 0;
}

#define PCIE_BRCMSTB_INIT(n)                                                                       \
	static struct pcie_brcmstb_data pcie_brcmstb_data_##n = {};                                \
                                                                                                   \
	static const struct pcie_ctrl_config pcie_ctrl_cfg_##n = {                                 \
		.cfg_addr = DT_INST_REG_ADDR(n),                                                   \
		.cfg_size = DT_INST_REG_SIZE(n),                                                   \
		.ranges_count = DT_NUM_RANGES(DT_DRV_INST(n)),                                     \
		.ranges = {DT_FOREACH_RANGE(DT_DRV_INST(n), PCIE_RANGE_FORMAT)},                   \
	};                                                                                         \
                                                                                                   \
	static const struct pcie_brcmstb_config pcie_brcmstb_cfg_##n = {                           \
		.common = &pcie_ctrl_cfg_##n,                                                      \
		.regs_count = DT_NUM_REGS(DT_DRV_INST(n)),                                         \
		.regs =                                                                            \
			{                                                                          \
				{DT_REG_ADDR_BY_IDX(DT_DRV_INST(n), 0),                            \
				 DT_REG_SIZE_BY_IDX(DT_DRV_INST(n), 0)},                           \
				{DT_REG_ADDR_BY_IDX(DT_DRV_INST(n), 1),                            \
				 DT_REG_SIZE_BY_IDX(DT_DRV_INST(n), 1)},                           \
				{DT_REG_ADDR_BY_IDX(DT_DRV_INST(n), 2),                            \
				 DT_REG_SIZE_BY_IDX(DT_DRV_INST(n), 2)},                           \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, pcie_brcmstb_init, NULL, &pcie_brcmstb_data_##n,                  \
			      &pcie_brcmstb_cfg_##n, PRE_KERNEL_1, CONFIG_PCIE_INIT_PRIORITY,      \
			      &pcie_brcmstb_api);

DT_INST_FOREACH_STATUS_OKAY(PCIE_BRCMSTB_INIT)
