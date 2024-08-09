/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp1

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/sys/device_mmio.h>

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

#define PCIE_EXT_CFG_DATA 0x8000

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

#define BCM2712_BAR0_REGION_START 0x410000
#define BCM2712_BAR1_REGION_START 0x0
#define BCM2712_BAR2_REGION_START 0x400000

#define BCM2712_BURST_SIZE 0x1

#define BCM2712_CLOCK_RATE 750000000ULL /* 750Mhz */

#define BCM2712_UBUS_TIMEOUT_NS    250000000ULL /* 250ms */
#define BCM2712_UBUS_TIMEOUT_TICKS (BCM2712_UBUS_TIMEOUT_NS * BCM2712_CLOCK_RATE / 1000000000ULL)

#define BCM2712_RC_CONFIG_RETRY_TIMEOUT_NS 240000000ULL /* 240ms */
#define BCM2712_RC_CONFIG_RETRY_TIMEOUT_TICKS                                                      \
	(BCM2712_UBUS_TIMEOUT_NS * BCM2712_CLOCK_RATE / 1000000000ULL)

#define BCM2712_PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE 0x060400

struct mfd_rp1_config {
	uintptr_t cfg_phys_addr;
	size_t cfg_size;
};

struct mfd_rp1_data {
	mm_reg_t cfg_addr;
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
	uint32_t tmp = ilog2(size);

	if (tmp >= 12 && tmp <= 15) {
		return (tmp - 12) + 0x1c;
	} else if (tmp >= 16 && tmp <= 36) {
		return tmp - 15;
	}

	return 0;
}

static int mfd_rp1_init(const struct device *port)
{
	const struct mfd_rp1_config *config = port->config;
	struct mfd_rp1_data *data = port->data;
	uint32_t tmp;
	uint16_t tmp16;

	device_map(&data->cfg_addr, config->cfg_phys_addr, config->cfg_size, K_MEM_CACHE_NONE);

	/* PCIe Setup */

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp |= PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK;
	tmp |= PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK;
	tmp &= ~PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK;
	tmp |= (BCM2712_BURST_SIZE << PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_LSB);
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_MISC_CTRL);

	tmp = lower_32_bits(BCM2712_RC_BAR2_OFFSET);
	tmp &= ~PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK;
	tmp |= encode_ibar_size(BCM2712_RC_BAR2_SIZE) << PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_LO);
	sys_write32(upper_32_bits(BCM2712_RC_BAR2_OFFSET),
		    data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_HI);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);
	tmp |= PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_ACCESS_ENABLE_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);

	/* Set SCB Size */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp &= ~PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK;
	tmp |= (ilog2(BCM2712_SCB0_SIZE) - 15) << PCIE_MISC_MISC_CTRL_SCB0_SIZE_LSB;
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

	tmp = lower_32_bits(BCM2712_RC_BAR4_PCI);
	tmp &= ~PCIE_MISC_RC_BAR_CONFIG_LO_SIZE_MASK;
	tmp |= encode_ibar_size(BCM2712_RC_BAR4_SIZE);
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR4_CONFIG_LO);
	sys_write32(upper_32_bits(BCM2712_RC_BAR4_PCI),
		    data->cfg_addr + PCIE_MISC_RC_BAR4_CONFIG_HI);

	tmp = upper_32_bits(BCM2712_RC_BAR4_CPU) & PCIE_MISC_UBUS_BAR_CONFIG_REMAP_HI_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_HI);
	tmp = lower_32_bits(BCM2712_RC_BAR4_CPU) & PCIE_MISC_UBUS_BAR_CONFIG_REMAP_LO_MASK;
	sys_write32(tmp | PCIE_MISC_UBUS_BAR_CONFIG_REMAP_ENABLE,
		    data->cfg_addr + PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_LO);

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
	tmp &= PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE_MASK;
	tmp |= BCM2712_PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE;
	sys_write32(tmp, data->cfg_addr + PCIE_RC_CFG_PRIV1_ID_VAL3);

	tmp = sys_read32(data->cfg_addr + PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1);
	tmp &= ~PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_MASK;
	tmp |= PCIE_RC_CFG_VENDOR_SPECIFIC_REG1_LITTLE_ENDIAN
	       << PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1_ENDIAN_MODE_BAR2_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1);

	/* Assert PERST# */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_PCIE_CTRL);
	tmp |= PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_PCIE_CTRL);

	/* Enable resources and bus-mastering */
	tmp = sys_read32(data->cfg_addr + PCI_COMMAND);
	tmp |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	sys_write32(tmp, data->cfg_addr + PCI_COMMAND);

	/* Assign resources to BARs */
	/* Wait until the registers become accessible */
	k_msleep(50);
	sys_write32(BCM2712_BAR0_REGION_START,
		    data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0);
	sys_write32(BCM2712_BAR1_REGION_START,
		    data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0 + 0x4);
	sys_write32(BCM2712_BAR2_REGION_START,
		    data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0 + 0x8);

	/* Enable resources */
	tmp = sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);
	tmp |= PCI_COMMAND_MEMORY;
	sys_write32(tmp, data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);

	return 0;
}

#define MFD_RP1_INIT(n)                                                                            \
	static struct mfd_rp1_data mfd_rp1_data_##n;                                               \
                                                                                                   \
	static const struct mfd_rp1_config mfd_rp1_cfg_##n = {                                     \
		.cfg_phys_addr = DT_INST_REG_ADDR(n),                                              \
		.cfg_size = DT_INST_REG_SIZE(n),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mfd_rp1_init, NULL, &mfd_rp1_data_##n, &mfd_rp1_cfg_##n,          \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_RP1_INIT)
