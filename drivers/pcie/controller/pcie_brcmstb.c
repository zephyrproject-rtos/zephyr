/*
 * Copyright (c) 2024 Junho Lee <junho@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcie_brcmstb, LOG_LEVEL_ERR);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/controller.h>
#ifdef CONFIG_GIC_V3_ITS
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#endif

#define DT_DRV_COMPAT brcm_brcmstb_pcie

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
	(BCM2712_RC_CONFIG_RETRY_TIMEOUT_NS * BCM2712_CLOCK_RATE / 1000000000ULL)

#define BCM2712_PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_CODE 0x060400

/*
 * PCIe Controllers Regions
 *
 * TOFIX:
 * - handle prefetchable regions
 */
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


static uint32_t pcie_brcmstb_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	return 0;
}

void pcie_brcmstb_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
}

bool pcie_brcmstb_region_allocate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64, size_t bar_size, uintptr_t *bar_bus_addr)
{
	return true;
}

bool pcie_brcmstb_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64, size_t align, uintptr_t *bar_base_addr)
{
	return true;
}

bool pcie_brcmstb_region_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64, uintptr_t bar_bus_addr, uintptr_t *bar_addr)
{
	return true;
}

static struct pcie_ctrl_driver_api pcie_brcmstb_api = {
	.conf_read = pcie_brcmstb_conf_read,
	.conf_write = pcie_brcmstb_conf_write,
	.region_allocate = pcie_brcmstb_region_allocate,
	.region_get_allocate_base = pcie_brcmstb_region_get_allocate_base,
	.region_translate = pcie_brcmstb_region_translate,
};

static int pcie_brcmstb_setup(const struct device *dev) {
	struct pcie_brcmstb_data *data = dev->data;
	uint32_t tmp;

	/* This is for BCM2712 only */
	// brcm_pcie_munge_pll();

	sys_write32(0x1f, data->cfg_addr + 0x1100);
	sys_write32(0x80001600, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x16, data->cfg_addr + 0x1100);
	sys_write32(0x800050b9, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x17, data->cfg_addr + 0x1100);
	sys_write32(0x8000bda1, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x18, data->cfg_addr + 0x1100);
	sys_write32(0x80000094, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x19, data->cfg_addr + 0x1100);
	sys_write32(0x800097b4, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x1b, data->cfg_addr + 0x1100);
	sys_write32(0x80005030, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x1c, data->cfg_addr + 0x1100);
	sys_write32(0x80005030, data->cfg_addr + 0x1104);
	k_busy_wait(300);
	sys_write32(0x1e, data->cfg_addr + 0x1100);
	sys_write32(0x80000007, data->cfg_addr + 0x1104);
	k_busy_wait(300);

	// brcm_pcie_munge_pll();
	tmp = sys_read32(data->cfg_addr + PCIE_RC_PL_PHY_CTL_15);
	tmp &= ~PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD_MASK;
	tmp |= 0x12;  // PM clock 18.52ns as ticks, TODO: use macros
	sys_write32(tmp, data->cfg_addr + PCIE_RC_PL_PHY_CTL_15);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp |= PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN_MASK;
	tmp |= PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE_MASK;
	tmp &= ~PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK;
	tmp |= (BCM2712_BURST_SIZE << PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_LSB);
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_MISC_CTRL);

	// brcm_pcie_set_tc_qos();

	uint64_t rc_bar2_offset = ((data->cfg_phys_addr == 0x1000110000) ? 0x1000000000 : BCM2712_RC_BAR2_OFFSET);
	uint32_t rc_bar2_size = ((data->cfg_phys_addr == 0x1000110000) ? 0x1000000000 : BCM2712_RC_BAR2_SIZE);
	tmp = lower_32_bits(rc_bar2_offset);
	tmp &= ~PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_MASK;
	tmp |= encode_ibar_size(rc_bar2_size) << PCIE_MISC_RC_BAR2_CONFIG_LO_SIZE_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_LO);
	sys_write32(upper_32_bits(rc_bar2_offset),
		    data->cfg_addr + PCIE_MISC_RC_BAR2_CONFIG_HI);

	return 0;
}

static int pcie_brcmstb_init(const struct device *dev)
{
	const struct pcie_ctrl_config *config = dev->config;
	struct pcie_brcmstb_data *data = dev->data;
	uint32_t tmp;
	uint16_t tmp16;

	data->cfg_phys_addr = config->cfg_addr;
	data->cfg_size = config->cfg_size;

	device_map(&data->cfg_addr, data->cfg_phys_addr, data->cfg_size, K_MEM_CACHE_NONE);

	/* PCIe Setup */
	pcie_brcmstb_setup(dev);

	tmp = sys_read32(data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);
	tmp |= PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_ACCESS_ENABLE_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_UBUS_BAR2_CONFIG_REMAP);

	/* Set SCB Size */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL);
	tmp &= ~PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK;
	tmp |= ((data->cfg_phys_addr == 0x1000110000) ? 15 : (ilog2(BCM2712_SCB0_SIZE) - 15)) << PCIE_MISC_MISC_CTRL_SCB0_SIZE_LSB;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_MISC_CTRL);
	printk("%x\n", sys_read32(data->cfg_addr + PCIE_MISC_MISC_CTRL));

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

	if (data->cfg_phys_addr != 0x1000110000) {
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
	}

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

	if (data->cfg_phys_addr == 0x1000110000) {
		// outbound
		sys_write32(0x0, data->cfg_addr + 0x400c);
		sys_write32(0x0, data->cfg_addr + 0x4010);
		sys_write32(0xfff00000, data->cfg_addr + 0x4070);
		sys_write32(0x1b, data->cfg_addr + 0x4080);
		sys_write32(0x1b, data->cfg_addr + 0x4084);

		sys_write32(0x0, data->cfg_addr + 0x4014);
		sys_write32(0x4, data->cfg_addr + 0x4018);
		sys_write32(0xfff00000, data->cfg_addr + 0x4074);
		sys_write32(0x18, data->cfg_addr + 0x4088);
		sys_write32(0x1a, data->cfg_addr + 0x408c);
	}

	/* Assert PERST# */
	tmp = sys_read32(data->cfg_addr + PCIE_MISC_PCIE_CTRL);
	tmp |= PCIE_MISC_PCIE_CTRL_PCIE_PERSTB_MASK;
	sys_write32(tmp, data->cfg_addr + PCIE_MISC_PCIE_CTRL);
	// printk("CTRL %x\n ", sys_read32(data->cfg_addr + PCIE_MISC_PCIE_CTRL));

	k_busy_wait(300000);
	/*
	for (int i = 0; i < 100; i++) {
		k_busy_wait(5000);
		uint32_t aaa = sys_read32(data->cfg_addr + 0x4068);
		printk("waiting %x %x %x\n", aaa, (aaa & 0x20) != 0, (aaa & 0x10) != 0);
	}
	*/

	/* Enable resources and bus-mastering */
	tmp = sys_read32(data->cfg_addr + PCI_COMMAND);
	tmp |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	sys_write32(tmp, data->cfg_addr + PCI_COMMAND);
	// printk("%x\n", sys_read32(data->cfg_addr + PCI_COMMAND));

	// printk("PCIE_STATUS %x\n", sys_read32(data->cfg_addr + 0x4068));

	/* Assign resources to BARs */
	/* Wait until the registers become accessible */
	if (data->cfg_phys_addr == 0x1000110000) {
		k_busy_wait(500000);
		// printk("INDEX %x\n", sys_read32(data->cfg_addr + 0x9000));
		sys_write32(0x100000, data->cfg_addr + 0x9000);
		// printk("INDEX %x\n", sys_read32(data->cfg_addr + 0x9000));
		// printk("PRIMARY_BUS %x\n", sys_read32(data->cfg_addr + 0x18));
		sys_write32(0xff0100, data->cfg_addr + 0x18);
		// printk("PRIMARY_BUS %x\n", sys_read32(data->cfg_addr + 0x18));
	}
	k_busy_wait(500000);

	// printk("%x\n", sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0));

	sys_write32(data->cfg_phys_addr == 0x1000110000 ? 0 : BCM2712_BAR0_REGION_START,
			data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0);
	sys_write32(data->cfg_phys_addr == 0x1000110000 ? 0x8000000 : BCM2712_BAR1_REGION_START,
			data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0 + 0x4);
	sys_write32(data->cfg_phys_addr == 0x1000110000 ? 0 : BCM2712_BAR2_REGION_START,
			data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0 + 0x8);
	printk("%x\n", sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_BASE_ADDRESS_0));

	/* Enable resources */
	tmp = sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);
	tmp |= PCI_COMMAND_MEMORY;
	sys_write32(tmp, data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND);
	printk("COMMAND %x\n", sys_read32(data->cfg_addr + PCIE_EXT_CFG_DATA + PCI_COMMAND));

	if (data->cfg_phys_addr == 0x1000110000) {
		k_busy_wait(500000);
		mem_addr_t asdf;
		device_map(&asdf, 0x1b00000000, 0x4000, K_MEM_CACHE_NONE);
		printk("TIMESTAMP_COUNT %x\n", sys_read32(asdf + 0x0300));
		printk("SYS_CLOCK_HI %x\n", sys_read32(asdf + 0x0380));
		printk("SYS_CLOCK_LO %x\n", sys_read32(asdf + 0x0384));
		printk("CYCLE_1S %x\n", sys_read32(asdf + 0x0034));
		sys_write32(125000000, asdf + 0x0034);
		printk("CYCLE_1S %x\n", sys_read32(asdf + 0x0034));
	}

	return 0;
}

#define PCIE_BRCMSTB_INIT(n)                                                                            \
	static struct pcie_brcmstb_data pcie_brcmstb_data_##n;                                               \
                                                                                                   \
	static const struct pcie_ctrl_config pcie_brcmstb_cfg_##n = {                                     \
		.cfg_addr = DT_INST_REG_ADDR(n),                                              \
		.cfg_size = DT_INST_REG_SIZE(n),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, pcie_brcmstb_init, NULL, &pcie_brcmstb_data_##n, &pcie_brcmstb_cfg_##n,          \
			      POST_KERNEL, 97, &pcie_brcmstb_api);  // TODO: POST_KERNEL is set to use printk, revert this after the development is done

DT_INST_FOREACH_STATUS_OKAY(PCIE_BRCMSTB_INIT)
