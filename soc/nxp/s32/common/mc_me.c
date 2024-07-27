/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_mc_me

#include <zephyr/kernel.h>
#if defined(CONFIG_REBOOT)
#include <zephyr/sys/reboot.h>
#endif /* CONFIG_REBOOT */

/* Control Key Register */
#define MC_ME_CTL_KEY                   0x0
#define MC_ME_CTL_KEY_KEY_MASK          GENMASK(15, 0)
#define MC_ME_CTL_KEY_KEY(v)            FIELD_PREP(MC_ME_CTL_KEY_KEY_MASK, (v))
/* Mode Configuration Register */
#define MC_ME_MODE_CONF                 0x4
#define MC_ME_MODE_CONF_DEST_RST_MASK   BIT(0)
#define MC_ME_MODE_CONF_DEST_RST(v)     FIELD_PREP(MC_ME_MODE_CONF_DEST_RST_MASK, (v))
#define MC_ME_MODE_CONF_FUNC_RST_MASK   BIT(1)
#define MC_ME_MODE_CONF_FUNC_RST(v)     FIELD_PREP(MC_ME_MODE_CONF_FUNC_RST_MASK, (v))
#define MC_ME_MODE_CONF_STANDBY_MASK    BIT(15)
#define MC_ME_MODE_CONF_STANDBY(v)      FIELD_PREP(MC_ME_MODE_CONF_STANDBY_MASK, (v))
/* Mode Update Register */
#define MC_ME_MODE_UPD                  0x8
#define MC_ME_MODE_UPD_MODE_UPD_MASK    BIT(0)
#define MC_ME_MODE_UPD_MODE_UPD(v)      FIELD_PREP(MC_ME_MODE_UPD_MODE_UPD_MASK, (v))
/* Mode Status Register */
#define MC_ME_MODE_STAT                 0xc
#define MC_ME_MODE_STAT_PREV_MODE_MASK  BIT(0)
#define MC_ME_MODE_STAT_PREV_MODE(v)    FIELD_PREP(MC_ME_MODE_STAT_PREV_MODE_MASK, (v))
/* Main Core ID Register */
#define MC_ME_MAIN_COREID               0x10
#define MC_ME_MAIN_COREID_CIDX_MASK     GENMASK(2, 0)
#define MC_ME_MAIN_COREID_CIDX(v)       FIELD_PREP(MC_ME_MAIN_COREID_CIDX_MASK, (v))
#define MC_ME_MAIN_COREID_PIDX_MASK     GENMASK(12, 8)
#define MC_ME_MAIN_COREID_PIDX(v)       FIELD_PREP(MC_ME_MAIN_COREID_PIDX_MASK, (v))
/* Partition p Process Configuration Register */
#define MC_ME_PRTN_PCONF(p)             (0x100 + 0x200 * (p))
#define MC_ME_PRTN_PCONF_PCE_MASK       BIT(0)
#define MC_ME_PRTN_PCONF_PCE(v)         FIELD_PREP(MC_ME_PRTN_PCONF_PCE_MASK, (v))
/* Partition p Process Update Register */
#define MC_ME_PRTN_PUPD(p)              (0x104 + 0x200 * (p))
#define MC_ME_PRTN_PUPD_PCUD_MASK       BIT(0)
#define MC_ME_PRTN_PUPD_PCUD(v)         FIELD_PREP(MC_ME_PRTN_PUPD_PCUD_MASK, (v))
/* Partition p Status Register */
#define MC_ME_PRTN_STAT(p)              (0x108 + 0x200 * (p))
#define MC_ME_PRTN_STAT_PCS_MASK        BIT(0)
#define MC_ME_PRTN_STAT_PCS(v)          FIELD_PREP(MC_ME_PRTN_STAT_PCS_MASK, (v))
/* Partition p COFB c Clock Status Register */
#define MC_ME_PRTN_COFB_STAT(p, c)      (0x110 + 0x200 * (p) + 0x4 * (c))
/* Partition p COFB c Clock Enable Register */
#define MC_ME_PRTN_COFB_CLKEN(p, c)     (0x130 + 0x200 * (p) + 0x4 * (c))
/* Partition p Core c Process Configuration Register */
#define MC_ME_PRTN_CORE_PCONF(p, c)     (0x140 + 0x200 * (p) + 0x20 * (c))
#define MC_ME_PRTN_CORE_PCONF_CCE_MASK  BIT(0)
#define MC_ME_PRTN_CORE_PCONF_CCE(v)    FIELD_PREP(MC_ME_PRTN_CORE_PCONF_CCE_MASK, (v))
/* Partition n Core c Process Update Register */
#define MC_ME_PRTN_CORE_PUPD(p, c)      (0x144 + 0x200 * (p) + 0x20 * (c))
#define MC_ME_PRTN_CORE_PUPD_CCUPD_MASK BIT(0)
#define MC_ME_PRTN_CORE_PUPD_CCUPD(v)   FIELD_PREP(MC_ME_PRTN_CORE_PUPD_CCUPD_MASK, (v))
/* Partition n Core c Status Register */
#define MC_ME_PRTN_CORE_STAT(p, c)      (0x148 + 0x200 * (p) + 0x20 * (c))
#define MC_ME_PRTN_CORE_STAT_CCS_MASK   BIT(0)
#define MC_ME_PRTN_CORE_STAT_CCS(v)     FIELD_PREP(MC_ME_PRTN_CORE_STAT_CCS_MASK, (v))
#define MC_ME_PRTN_CORE_STAT_WFI_MASK   BIT(31)
#define MC_ME_PRTN_CORE_STAT_WFI(v)     FIELD_PREP(MC_ME_PRTN_CORE_STAT_WFI_MASK, (v))
/* Partition n Core c Address Register */
#define MC_ME_PRTN_CORE_ADDR(p, c)      (0x14c + 0x200 * (p) + 0x20 * (c))
#define MC_ME_PRTN_CORE_ADDR_ADDR_MASK  GENMASK(31, 2)
#define MC_ME_PRTN_CORE_ADDR_ADDR(v)    FIELD_PREP(MC_ME_PRTN_CORE_ADDR_ADDR_MASK, (v))

#define MC_ME_CTL_KEY_DIRECT_KEY   0x00005af0U
#define MC_ME_CTL_KEY_INVERTED_KEY 0x0000a50fU

/* Handy accessors */
#define REG_READ(r)     sys_read32((mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))
#define REG_WRITE(r, v) sys_write32((v), (mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))

/** MC_ME power mode */
enum mc_me_power_mode {
	/** Destructive Reset Mode */
	MC_ME_DEST_RESET_MODE,
	/** Functional Reset Mode */
	MC_ME_FUNC_RESET_MODE,
};

#if defined(CONFIG_REBOOT)
static inline void mc_me_write_ctl_key(void)
{
	REG_WRITE(MC_ME_CTL_KEY, MC_ME_CTL_KEY_KEY(MC_ME_CTL_KEY_DIRECT_KEY));
	REG_WRITE(MC_ME_CTL_KEY, MC_ME_CTL_KEY_KEY(MC_ME_CTL_KEY_INVERTED_KEY));
}

static inline void mc_me_trigger_mode_update(void)
{
	REG_WRITE(MC_ME_MODE_UPD, MC_ME_MODE_UPD_MODE_UPD(1U));
	mc_me_write_ctl_key();
}

static int mc_me_set_mode(enum mc_me_power_mode mode)
{
	int err = 0;

	switch (mode) {
	case MC_ME_DEST_RESET_MODE:
		REG_WRITE(MC_ME_MODE_CONF, MC_ME_MODE_CONF_DEST_RST(1U));
		mc_me_trigger_mode_update();
		break;
	case MC_ME_FUNC_RESET_MODE:
		REG_WRITE(MC_ME_MODE_CONF, MC_ME_MODE_CONF_FUNC_RST(1U));
		mc_me_trigger_mode_update();
		break;
	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

/*
 * Overrides default weak implementation of system reboot.
 *
 * SYS_REBOOT_COLD (Destructive Reset):
 * - Leads most parts of the chip, except a few modules, to reset. SRAM content
 *   is lost after this reset event.
 * - Flash is always reset, so an updated value of the option bits is reloaded
 *   in volatile registers outside of the Flash array.
 * - Trimming is lost.
 * - STCU is reset and configured BISTs are executed.
 *
 * SYS_REBOOT_WARM (Functional Reset):
 * - Leads all the communication peripherals and cores to reset. The communication
 *   protocols' sanity is not guaranteed and they are assumed to be reinitialized
 *   after reset. The SRAM content, and the functionality of certain modules, is
 *   preserved across functional reset.
 * - The volatile registers are not reset; in case of a reset event, the
 *   trimming is maintained.
 * - No BISTs are executed after functional reset.
 */
void sys_arch_reboot(int type)
{
	switch (type) {
	case SYS_REBOOT_COLD:
		mc_me_set_mode(MC_ME_DEST_RESET_MODE);
		break;
	case SYS_REBOOT_WARM:
		mc_me_set_mode(MC_ME_FUNC_RESET_MODE);
		break;
	default:
		/* Do nothing */
		break;
	}
}
#endif /* CONFIG_REBOOT */
