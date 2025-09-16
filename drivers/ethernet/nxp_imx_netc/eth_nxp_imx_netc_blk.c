/* NXP NETC Block Controller Driver
 *
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_imx_netc_blk_ctrl

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_imx_netc_blk);

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/* NETC integrated endpoint register block register */
#define IERB_EMDIOFAUXR 0x344
#define IERB_T0FAUXR    0x444
#define IERB_ETBCR(a)   (0x300c + 0x100 * (a))
#define IERB_EFAUXR(a)  (0x3044 + 0x100 * (a))
#define IERB_VFAUXR(a)  (0x4004 + 0x40 * (a))

/* NETC privileged register block register */
#define PRB_NETCRR  0x100
#define NETCRR_SR   BIT(0)
#define NETCRR_LOCK BIT(1)

#define PRB_NETCSR   0x104
#define NETCSR_ERROR BIT(0)
#define NETCSR_STATE BIT(1)

/* NETCMIX CFG Link register */
#ifdef CONFIG_SOC_MIMX9596

#define CFG_LINK_MII_PROT 0x10
enum {
	MII,
	RMII,
	RGMII,
	reserved,
	SGMII,
	XGMII,
};
#define CFG_LINK_MII_PROT_0_SHIFT 0
#define CFG_LINK_MII_PROT_1_SHIFT 4
#define CFG_LINK_MII_PROT_2_SHIFT 8
#define MII_PROT_N(prot, n)	  ((prot) << CFG_LINK_MII_PROT_##n##_SHIFT)

#elif defined(CONFIG_SOC_MIMX94398)

enum {
	MII,
	RMII,
	RGMII,
	SGMII,
};

#define NETC_LINK_CFG0 0x4c
#define NETC_LINK_CFG1 0x50
#define NETC_LINK_CFG2 0x54
#define NETC_LINK_CFG3 0x58
#define NETC_LINK_CFG4 0x5c
#define NETC_LINK_CFG5 0x60

#endif

/* NETCMIX PCS protocol register */
#define CFG_LINK_PCS_PROT_0           0x14
#define CFG_LINK_PCS_PROT_1           0x18
#define CFG_LINK_PCS_PROT_2           0x1c
#if defined(CONFIG_SOC_MIMX94398)
#define CFG_LINK_PCS_PROT_3           0x20
#define CFG_LINK_PCS_PROT_4           0x24
#define CFG_LINK_PCS_PROT_5           0x28
#endif
/* PCS Protocols */
#define CFG_LINK_PCS_PROT_1G_SGMII    BIT(0)
#define CFG_LINK_PCS_PROT_2500M_SGMII BIT(1)
#define CFG_LINK_PCS_PROT_XFI         BIT(3)
#define CFG_LINK_PCS_PROT_10G_SXGMII  BIT(6)

#if defined(CONFIG_SOC_MIMX94398)
#define EXT_PIN_CONTROL     0x10
#define MAC2_MAC3_SEL_SHIFT 1
#define SET_MAC2(x)         ((x) & ~(BIT(MAC2_MAC3_SEL_SHIFT)))
#define SET_MAC3(x)         ((x) | BIT(MAC2_MAC3_SEL_SHIFT))
#endif

struct eth_nxp_imx_netc_blk_config {
	DEVICE_MMIO_NAMED_ROM(ierb);
	DEVICE_MMIO_NAMED_ROM(prb);
	DEVICE_MMIO_NAMED_ROM(netcmix);
};

struct eth_nxp_imx_netc_blk_data {
	DEVICE_MMIO_NAMED_RAM(ierb);
	DEVICE_MMIO_NAMED_RAM(prb);
	DEVICE_MMIO_NAMED_RAM(netcmix);
};

#define DEV_CFG(_dev)  ((const struct eth_nxp_imx_netc_blk_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct eth_nxp_imx_netc_blk_data *)(_dev)->data)

#define read_and_poll_timeout(reg, val, cond)                                                      \
	({                                                                                         \
		unsigned int count = 1000000; /* 1s! */                                            \
		while (1) {                                                                        \
			(val) = sys_read32(reg);                                                   \
			if (cond) {                                                                \
				break;                                                             \
			}                                                                          \
			count--;                                                                   \
			if (!count) {                                                              \
				break;                                                             \
			}                                                                          \
			k_usleep(1);                                                               \
		}                                                                                  \
		(cond) ? 0 : -ETIMEDOUT;                                                           \
	})

static bool ierb_is_locked(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, prb);

	return sys_read32(base + PRB_NETCRR) & NETCRR_LOCK;
}

static int ierb_lock(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, prb);
	uint32_t val;

	sys_write32(NETCRR_LOCK, base + PRB_NETCRR);

	return read_and_poll_timeout(base + PRB_NETCSR, val, !(val & NETCSR_STATE));
}

static int ierb_unlock(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, prb);
	uint32_t val;

	sys_write32(1, base + PRB_NETCRR);

	return read_and_poll_timeout(base + PRB_NETCRR, val, !(val & NETCRR_LOCK));
}

#ifdef CONFIG_SOC_MIMX9596
/* Set LDID */
static int ierb_init(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, ierb);

	/* EMDIO : No MSI-X interrupt */
	sys_write32(0, base + IERB_EMDIOFAUXR);
	/* ENETC0 PF */
	sys_write32(0, base + IERB_EFAUXR(0));
	/* ENETC0 VF0 */
	sys_write32(1, base + IERB_VFAUXR(0));
	/* ENETC0 VF1 */
	sys_write32(2, base + IERB_VFAUXR(1));
	/* ENETC1 PF */
	sys_write32(3, base + IERB_EFAUXR(1));
	/* ENETC1 VF0 : Disabled on 19x19 board dts */
	sys_write32(5, base + IERB_VFAUXR(2));
	/* ENETC1 VF1 : Disabled on 19x19 board dts */
	sys_write32(6, base + IERB_VFAUXR(3));
	/* ENETC2 PF */
	sys_write32(4, base + IERB_EFAUXR(2));
	/* ENETC2 VF0 : Disabled on 15x15 board dts */
	sys_write32(5, base + IERB_VFAUXR(4));
	/* ENETC2 VF1 : Disabled on 15x15 board dts */
	sys_write32(6, base + IERB_VFAUXR(5));
	/* NETC TIMER */
	sys_write32(7, base + IERB_T0FAUXR);

	return 0;
}

static int netcmix_init(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, netcmix);
	uint32_t reg_val;

	/* ToDo: configure PSC protocol and MII protocol according to PHY mode */
	reg_val = MII_PROT_N(RGMII, 0) | MII_PROT_N(RGMII, 1) | MII_PROT_N(XGMII, 2);
	sys_write32(reg_val, base + CFG_LINK_MII_PROT);
	sys_write32(CFG_LINK_PCS_PROT_10G_SXGMII, base + CFG_LINK_PCS_PROT_2);

	return 0;
}
#elif defined(CONFIG_SOC_MIMX94398)
static int ierb_init(const struct device *dev)
{
	return 0;
}


static int netcmix_init(const struct device *dev)
{
	uintptr_t base = DEVICE_MMIO_NAMED_GET(dev, netcmix);
	uint32_t reg_val;

	/* ToDo: configure PSC protocol and MII protocol according to PHY mode */
	sys_write32(CFG_LINK_PCS_PROT_2500M_SGMII, base + CFG_LINK_PCS_PROT_0);
	sys_write32(CFG_LINK_PCS_PROT_2500M_SGMII, base + CFG_LINK_PCS_PROT_1);
	sys_write32(CFG_LINK_PCS_PROT_1G_SGMII, base + CFG_LINK_PCS_PROT_2);
	sys_write32(CFG_LINK_PCS_PROT_1G_SGMII, base + CFG_LINK_PCS_PROT_3);
	sys_write32(CFG_LINK_PCS_PROT_1G_SGMII, base + CFG_LINK_PCS_PROT_4);
	sys_write32(CFG_LINK_PCS_PROT_1G_SGMII, base + CFG_LINK_PCS_PROT_5);

	sys_write32(MII, base + NETC_LINK_CFG0);
	sys_write32(MII, base + NETC_LINK_CFG1);
	sys_write32(RGMII, base + NETC_LINK_CFG2);
	sys_write32(RGMII, base + NETC_LINK_CFG3);
	sys_write32(RGMII, base + NETC_LINK_CFG4);
	sys_write32(RGMII, base + NETC_LINK_CFG5);

	reg_val = sys_read32(base + EXT_PIN_CONTROL);
	reg_val = SET_MAC3(reg_val);
	sys_write32(reg_val, base + EXT_PIN_CONTROL);

	return 0;
}
#endif

static int eth_nxp_imx_netc_blk_init(const struct device *dev)
{
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, ierb, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	DEVICE_MMIO_NAMED_MAP(dev, prb, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	DEVICE_MMIO_NAMED_MAP(dev, netcmix, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (ierb_is_locked(dev)) {
		ret = ierb_unlock(dev);
		if (ret) {
			LOG_ERR("Unlock IERB failed.");
			return ret;
		}
	}

	if (ierb_init(dev) != 0) {
		LOG_ERR("Failed to initialize IERB");
		return -EIO;
	}

	ret = ierb_lock(dev);
	if (ret) {
		LOG_ERR("Lock IERB failed.");
		return ret;
	}

	ret = netcmix_init(dev);
	if (ret) {
		LOG_ERR("NETCMIX init failed.");
		return ret;
	}

	return 0;
}

#define ETH_NXP_IMX_NETC_BLK_INIT(inst)                                                            \
	static struct eth_nxp_imx_netc_blk_data eth_nxp_imx_netc_blk_data_##inst;                  \
	static const struct eth_nxp_imx_netc_blk_config eth_nxp_imx_netc_blk_config_##inst = {     \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ierb, DT_DRV_INST(inst)),                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(prb, DT_DRV_INST(inst)),                        \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(netcmix, DT_DRV_INST(inst)),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, eth_nxp_imx_netc_blk_init, NULL,                               \
			      &eth_nxp_imx_netc_blk_data_##inst,                                   \
			      &eth_nxp_imx_netc_blk_config_##inst, POST_KERNEL,                    \
			      CONFIG_MDIO_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ETH_NXP_IMX_NETC_BLK_INIT)
