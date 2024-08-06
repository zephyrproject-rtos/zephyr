/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_mc_rgm

#include <zephyr/kernel.h>
#include <zephyr/init.h>

/* Destructive Event Status Register */
#define MC_RGM_DES                      0x0
#define MC_RGM_DES_F_POR_MASK           BIT(0)
#define MC_RGM_DES_F_POR(v)             FIELD_PREP(MC_RGM_DES_F_POR_MASK, (v))
/* Functional / External Reset Status Register */
#define MC_RGM_FES                      0x8
#define MC_RGM_FES_F_EXR_MASK           BIT(0)
#define MC_RGM_FES_F_EXR(v)             FIELD_PREP(MC_RGM_FES_F_EXR_MASK, (v))
/* Functional Event Reset Disable Register */
#define MC_RGM_FERD                     0xc
/* Functional Bidirectional Reset Enable Register */
#define MC_RGM_FBRE                     0x10
/* Functional Reset Escalation Counter Register */
#define MC_RGM_FREC                     0x14
#define MC_RGM_FREC_FREC_MASK           GENMASK(3, 0)
#define MC_RGM_FREC_FREC(v)             FIELD_PREP(MC_RGM_FREC_FREC_MASK, (v))
/* Functional Reset Escalation Threshold Register */
#define MC_RGM_FRET                     0x18
#define MC_RGM_FRET_FRET_MASK           GENMASK(3, 0)
#define MC_RGM_FRET_FRET(v)             FIELD_PREP(MC_RGM_FRET_FRET_MASK, (v))
/* Destructive Reset Escalation Threshold Register */
#define MC_RGM_DRET                     0x1c
#define MC_RGM_DRET_DRET_MASK           GENMASK(3, 0)
#define MC_RGM_DRET_DRET(v)             FIELD_PREP(MC_RGM_DRET_DRET_MASK, (v))
/* External Reset Control Register */
#define MC_RGM_ERCTRL                   0x20
#define MC_RGM_ERCTRL_ERASSERT_MASK     BIT(0)
#define MC_RGM_ERCTRL_ERASSERT(v)       FIELD_PREP(MC_RGM_ERCTRL_ERASSERT_MASK, (v))
/* Reset During Standby Status Register */
#define MC_RGM_RDSS                     0x24
#define MC_RGM_RDSS_DES_RES_MASK        BIT(0)
#define MC_RGM_RDSS_DES_RES(v)          FIELD_PREP(MC_RGM_RDSS_DES_RES_MASK, (v))
#define MC_RGM_RDSS_FES_RES_MASK        BIT(1)
#define MC_RGM_RDSS_FES_RES(v)          FIELD_PREP(MC_RGM_RDSS_FES_RES_MASK, (v))
/* Functional Reset Entry Timeout Control Register */
#define MC_RGM_FRENTC                   0x28
#define MC_RGM_FRENTC_FRET_EN_MASK      BIT(0)
#define MC_RGM_FRENTC_FRET_EN(v)        FIELD_PREP(MC_RGM_FRENTC_FRET_EN_MASK, (v))
#define MC_RGM_FRENTC_FRET_TIMEOUT_MASK GENMASK(31, 1)
#define MC_RGM_FRENTC_FRET_TIMEOUT(v)   FIELD_PREP(MC_RGM_FRENTC_FRET_TIMEOUT_MASK, (v))
/* Low Power Debug Control Register */
#define MC_RGM_LPDEBUG                  0x2c
#define MC_RGM_LPDEBUG_LP_DBG_EN_MASK   BIT(0)
#define MC_RGM_LPDEBUG_LP_DBG_EN(v)     FIELD_PREP(MC_RGM_LPDEBUG_LP_DBG_EN_MASK, (v))

#define MC_RGM_TIMEOUT_US 50000U

/* Handy accessors */
#define REG_READ(r)     sys_read32((mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))
#define REG_WRITE(r, v) sys_write32((v), (mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))

static int mc_rgm_clear_reset_status(uint32_t reg)
{
	bool timeout;

	/*
	 * Register bits are cleared on write 1 if the triggering event has already
	 * been cleared at the source
	 */
	timeout = !WAIT_FOR(REG_READ(reg) == 0U, MC_RGM_TIMEOUT_US, REG_WRITE(reg, 0xffffffff));
	return timeout ? -ETIMEDOUT : 0U;
}

static int mc_rgm_init(const struct device *dev)
{
	int err = 0;
	uint32_t rst_status;

	ARG_UNUSED(dev);

	/*
	 * MC_RGM_FES must be cleared before writing 1 to any of the fields in MC_RGM_FERD,
	 * otherwise an interrupt request may occur
	 */
	rst_status = REG_READ(MC_RGM_FES);
	if (rst_status != 0U) {
		err = mc_rgm_clear_reset_status(MC_RGM_FES);
		if (err) {
			return err;
		}
	}

	/* All functional reset sources generate a reset event */
	REG_WRITE(MC_RGM_FERD, 0U);

	rst_status = REG_READ(MC_RGM_DES);
	if ((DT_INST_PROP(0, func_reset_threshold) != 0U) && (rst_status != 0U)) {
		/* MC_RGM_FRET value is reset on power-on and any destructive reset */
		REG_WRITE(MC_RGM_FRET, MC_RGM_FRET_FRET(DT_INST_PROP(0, func_reset_threshold)));
	} else {
		REG_WRITE(MC_RGM_FRET, MC_RGM_FRET_FRET(0U));
	}

	if ((DT_INST_PROP(0, dest_reset_threshold) != 0U) &&
	    (FIELD_GET(MC_RGM_DES_F_POR_MASK, rst_status) != 0U)) {
		/* MC_RGM_DRET value is reset only on power-on reset */
		REG_WRITE(MC_RGM_DRET, MC_RGM_DRET_DRET(DT_INST_PROP(0, dest_reset_threshold)));
	} else {
		REG_WRITE(MC_RGM_DRET, MC_RGM_DRET_DRET(0U));
	}

	/* Clear destructive reset status to avoid persisting it across resets */
	err = mc_rgm_clear_reset_status(MC_RGM_DES);
	if (err) {
		return err;
	}

	return err;
}

DEVICE_DT_INST_DEFINE(0, mc_rgm_init, NULL, NULL, 0, PRE_KERNEL_1, 1, NULL);
