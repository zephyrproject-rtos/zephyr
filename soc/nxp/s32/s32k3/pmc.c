/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32k3_pmc

#include <zephyr/kernel.h>
#include <zephyr/init.h>

/* Low Voltage Status and Control Register */
#define PMC_LVSC                 0x0
#define PMC_LVSC_HVDAF_MASK      BIT(0)
#define PMC_LVSC_HVDAF(v)        FIELD_PREP(PMC_LVSC_HVDAF_MASK, (v))
#define PMC_LVSC_HVDBF_MASK      BIT(1)
#define PMC_LVSC_HVDBF(v)        FIELD_PREP(PMC_LVSC_HVDBF_MASK, (v))
#define PMC_LVSC_HVD25F_MASK     BIT(2)
#define PMC_LVSC_HVD25F(v)       FIELD_PREP(PMC_LVSC_HVD25F_MASK, (v))
#define PMC_LVSC_HVD11F_MASK     BIT(3)
#define PMC_LVSC_HVD11F(v)       FIELD_PREP(PMC_LVSC_HVD11F_MASK, (v))
#define PMC_LVSC_LVD5AF_MASK     BIT(4)
#define PMC_LVSC_LVD5AF(v)       FIELD_PREP(PMC_LVSC_LVD5AF_MASK, (v))
#define PMC_LVSC_LVD15F_MASK     BIT(5)
#define PMC_LVSC_LVD15F(v)       FIELD_PREP(PMC_LVSC_LVD15F_MASK, (v))
#define PMC_LVSC_HVDAS_MASK      BIT(8)
#define PMC_LVSC_HVDAS(v)        FIELD_PREP(PMC_LVSC_HVDAS_MASK, (v))
#define PMC_LVSC_HVDBS_MASK      BIT(9)
#define PMC_LVSC_HVDBS(v)        FIELD_PREP(PMC_LVSC_HVDBS_MASK, (v))
#define PMC_LVSC_HVD25S_MASK     BIT(10)
#define PMC_LVSC_HVD25S(v)       FIELD_PREP(PMC_LVSC_HVD25S_MASK, (v))
#define PMC_LVSC_HVD11S_MASK     BIT(11)
#define PMC_LVSC_HVD11S(v)       FIELD_PREP(PMC_LVSC_HVD11S_MASK, (v))
#define PMC_LVSC_LVD5AS_MASK     BIT(12)
#define PMC_LVSC_LVD5AS(v)       FIELD_PREP(PMC_LVSC_LVD5AS_MASK, (v))
#define PMC_LVSC_LVD15S_MASK     BIT(13)
#define PMC_LVSC_LVD15S(v)       FIELD_PREP(PMC_LVSC_LVD15S_MASK, (v))
#define PMC_LVSC_LVRAF_MASK      BIT(16)
#define PMC_LVSC_LVRAF(v)        FIELD_PREP(PMC_LVSC_LVRAF_MASK, (v))
#define PMC_LVSC_LVRALPF_MASK    BIT(17)
#define PMC_LVSC_LVRALPF(v)      FIELD_PREP(PMC_LVSC_LVRALPF_MASK, (v))
#define PMC_LVSC_LVRBF_MASK      BIT(18)
#define PMC_LVSC_LVRBF(v)        FIELD_PREP(PMC_LVSC_LVRBF_MASK, (v))
#define PMC_LVSC_LVRBLPF_MASK    BIT(19)
#define PMC_LVSC_LVRBLPF(v)      FIELD_PREP(PMC_LVSC_LVRBLPF_MASK, (v))
#define PMC_LVSC_LVR25F_MASK     BIT(20)
#define PMC_LVSC_LVR25F(v)       FIELD_PREP(PMC_LVSC_LVR25F_MASK, (v))
#define PMC_LVSC_LVR25LPF_MASK   BIT(21)
#define PMC_LVSC_LVR25LPF(v)     FIELD_PREP(PMC_LVSC_LVR25LPF_MASK, (v))
#define PMC_LVSC_LVR11F_MASK     BIT(22)
#define PMC_LVSC_LVR11F(v)       FIELD_PREP(PMC_LVSC_LVR11F_MASK, (v))
#define PMC_LVSC_LVR11LPF_MASK   BIT(23)
#define PMC_LVSC_LVR11LPF(v)     FIELD_PREP(PMC_LVSC_LVR11LPF_MASK, (v))
#define PMC_LVSC_GNG25OSCF_MASK  BIT(24)
#define PMC_LVSC_GNG25OSCF(v)    FIELD_PREP(PMC_LVSC_GNG25OSCF_MASK, (v))
#define PMC_LVSC_GNG11OSCF_MASK  BIT(25)
#define PMC_LVSC_GNG11OSCF(v)    FIELD_PREP(PMC_LVSC_GNG11OSCF_MASK, (v))
#define PMC_LVSC_PORF_MASK       BIT(31)
#define PMC_LVSC_PORF(v)         FIELD_PREP(PMC_LVSC_PORF_MASK, (v))
/* PMC Configuration Register */
#define PMC_CONFIG               0x4
#define PMC_CONFIG_LMEN_MASK     BIT(0)
#define PMC_CONFIG_LMEN(v)       FIELD_PREP(PMC_CONFIG_LMEN_MASK, (v))
#define PMC_CONFIG_LMBCTLEN_MASK BIT(1)
#define PMC_CONFIG_LMBCTLEN(v)   FIELD_PREP(PMC_CONFIG_LMBCTLEN_MASK, (v))
#define PMC_CONFIG_FASTREC_MASK  BIT(2)
#define PMC_CONFIG_FASTREC(v)    FIELD_PREP(PMC_CONFIG_FASTREC_MASK, (v))
#define PMC_CONFIG_LPM25EN_MASK  BIT(3)
#define PMC_CONFIG_LPM25EN(v)    FIELD_PREP(PMC_CONFIG_LPM25EN_MASK, (v))
#define PMC_CONFIG_LVRBLPEN_MASK BIT(4)
#define PMC_CONFIG_LVRBLPEN(v)   FIELD_PREP(PMC_CONFIG_LVRBLPEN_MASK, (v))
#define PMC_CONFIG_HVDIE_MASK    BIT(8)
#define PMC_CONFIG_HVDIE(v)      FIELD_PREP(PMC_CONFIG_HVDIE_MASK, (v))
#define PMC_CONFIG_LVDIE_MASK    BIT(9)
#define PMC_CONFIG_LVDIE(v)      FIELD_PREP(PMC_CONFIG_LVDIE_MASK, (v))
#define PMC_CONFIG_LMAUTOEN_MASK BIT(16)
#define PMC_CONFIG_LMAUTOEN(v)   FIELD_PREP(PMC_CONFIG_LMAUTOEN_MASK, (v))
#define PMC_CONFIG_LMSTAT_MASK   BIT(17)
#define PMC_CONFIG_LMSTAT(v)     FIELD_PREP(PMC_CONFIG_LMSTAT_MASK, (v))
/* Version ID register */
#define PMC_VERID                0xc
#define PMC_VERID_LMFEAT_MASK    BIT(0)
#define PMC_VERID_LMFEAT(v)      FIELD_PREP(PMC_VERID_LMFEAT_MASK, (v))
#define PMC_VERID_MINOR_MASK     GENMASK(23, 16)
#define PMC_VERID_MINOR(v)       FIELD_PREP(PMC_VERID_MINOR_MASK, (v))
#define PMC_VERID_MAJOR_MASK     GENMASK(31, 24)
#define PMC_VERID_MAJOR(v)       FIELD_PREP(PMC_VERID_MAJOR_MASK, (v))

/* Handy accessors */
#define REG_READ(r)     sys_read32((mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))
#define REG_WRITE(r, v) sys_write32((v), (mem_addr_t)(DT_INST_REG_ADDR(0) + (r)))

#define PMC_TIMEOUT_US 10000U

static int pmc_init(const struct device *dev)
{
	uint32_t reg_val;

	ARG_UNUSED(dev);

	/* Clear Low Voltage Status flags after initial power ramp-up */
	REG_WRITE(PMC_LVSC, 0xffffffffU);

	/* Last Mile regulator initialization */
	reg_val = PMC_CONFIG_LMEN(DT_INST_PROP(0, lm_reg)) |
		  PMC_CONFIG_LMAUTOEN(DT_INST_PROP(0, lm_reg_auto)) |
		  PMC_CONFIG_LMBCTLEN(DT_INST_PROP(0, lm_reg_base_control));

	if (DT_INST_PROP(0, lm_reg)) {
		REG_WRITE(PMC_CONFIG, reg_val & ~PMC_CONFIG_LMEN_MASK);
		/*
		 * If external BJT is using on the PCB board, wait for the LVD15 to go above
		 * low-voltage detect threshold before enabling LMEN
		 */
		if (DT_INST_PROP(0, lm_reg_base_control)) {
			if (!WAIT_FOR(FIELD_GET(PMC_LVSC_LVD15S_MASK, REG_READ(PMC_LVSC)) == 0U,
				      PMC_TIMEOUT_US, k_busy_wait(10U))) {
				return -ETIMEDOUT;
			}
		}
		REG_WRITE(PMC_CONFIG, REG_READ(PMC_CONFIG) | PMC_CONFIG_LMEN(1U));
	} else {
		REG_WRITE(PMC_CONFIG, reg_val);
		if (DT_INST_PROP(0, lm_reg_auto)) {
			/* Wait for Last Mile regulator to be turned on */
			if (!WAIT_FOR(FIELD_GET(PMC_CONFIG_LMSTAT_MASK, REG_READ(PMC_CONFIG)) == 1U,
				      PMC_TIMEOUT_US, k_busy_wait(10U))) {
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

DEVICE_DT_INST_DEFINE(0, pmc_init, NULL, NULL, 0, PRE_KERNEL_1, 1, NULL);
