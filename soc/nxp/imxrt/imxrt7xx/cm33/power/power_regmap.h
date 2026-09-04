/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Resource -> register mapping for the RT7xx CM33 low-power driver.
 *
 * This is the boundary where the resource model (power_resources.h) meets the
 * hardware: everything above it speaks in enum power_resource, everything below
 * it is bits. Nothing else in the driver needs a PDSLEEPCFG or SLEEPCFG mask.
 *
 * Each domain assembles its table from the lists below rather than writing out
 * masks. Which resources both domains can vote on and which only Compute can is a
 * hardware fact -- RM 30.3.1 Table 257 for the SLEEPCFG fields, the per-field
 * Sense-slot access notes in RM 27.7.2 for the PMC ones -- needed both as register
 * masks and as a resource mask, so it is stated once here and projected two ways.
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_REGMAP_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_REGMAP_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#include <fsl_common.h>

#include "power_resources.h"

/*
 * The PMC and SLEEPCON instances this build's core owns.
 *
 * A property of the build, not a per-domain choice: a core's device header defines
 * only its own instances, so writing PMC1 in a core0 build does not compile and
 * there is nothing for a runtime pointer to select between. The selector is the
 * domain Kconfig symbol -- the same one CMakeLists uses to pick which domain module
 * to compile -- so the pairing of a domain with its instances has one statement in
 * the tree and cannot disagree with itself.
 *
 * This needs less of the hardware than a shared type alias would: the two SLEEPCON
 * layouts only have to agree on field *names*, not offsets, since each build
 * compiles against its own type. SOC_SLEEPCON_INST is the bare instance number, for
 * pasting into the SLEEPCONn_* field-mask names.
 */
#if defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_COMPUTE)
#define SOC_PMC		  PMC0
#define SOC_SLEEPCON	  SLEEPCON0
#define SOC_SLEEPCON_INST 0
#elif defined(CONFIG_SOC_IMXRT7XX_POWER_DOMAIN_SENSE)
#define SOC_PMC		  PMC1
#define SOC_SLEEPCON	  SLEEPCON1
#define SOC_SLEEPCON_INST 1
#else
#error "RT7xx CM33 power: core is in neither the Compute nor the Sense domain"
#endif

/* SLEEPCFG field mask of whichever SLEEPCON instance this build owns. */
#define _PWR_SLPCFG_CAT(_inst, _field)	SLEEPCON##_inst##_SLEEPCFG_##_field##_MASK
#define _PWR_SLPCFG_EXP(_inst, _field)	_PWR_SLPCFG_CAT(_inst, _field)
#define PWR_SLPCFG(_field)		_PWR_SLPCFG_EXP(SOC_SLEEPCON_INST, _field)

/* The registers this driver programs from the resource model. */
enum power_reg {
	PWR_REG_SLEEPCFG, /* SLEEPCONn.SLEEPCFG   clocks, oscillators, PLLs, ADC */
	PWR_REG_PDSLP0,   /* PMCn.PDSLEEPCFG0     voltage domains */
	PWR_REG_PDSLP1,   /* PMCn.PDSLEEPCFG1     analog refs, detectors, ROM/OTP */
	PWR_REG_PDSLP2,   /* PMCn.PDSLEEPCFG2     main memory, array */
	PWR_REG_PDSLP3,   /* PMCn.PDSLEEPCFG3     main memory, periphery */
	PWR_REG_PDSLP4,   /* PMCn.PDSLEEPCFG4     peripheral memory, array */
	PWR_REG_PDSLP5,   /* PMCn.PDSLEEPCFG5     peripheral memory, periphery */
	PWR_REG_COUNT,
};

/*
 * Where one resource's control lives. mask may span several bits when one resource
 * has more than one field in the same register -- a clock source with both a
 * power-down and an output gate, or a rail's POR/LVD/HVD group. All bits of a mask
 * are "1 means powered down", which is what lets power_commit() do the
 * keep-alive-to-power-down polarity flip in one place.
 *
 * A zero mask means "this domain does not have this resource".
 */
struct power_res_map {
	uint8_t  reg;
	uint32_t mask;
};

/*
 * Main memory: one bit per physical SRAM partition P0..P29, same bit assignment
 * in PDSLEEPCFG2 (array) and PDSLEEPCFG3 (periphery). This is also the bit
 * assignment of a request's `sram` field, which is what lets the devicetree-derived
 * masks of sram_banks.h go straight into the register.
 */
#define PWR_SRAM_ALL GENMASK(29, 0)

BUILD_ASSERT(PWR_SRAM_ALL ==
	     (PMC_PDSLEEPCFG2_SRAM29_MASK | (PMC_PDSLEEPCFG2_SRAM29_MASK - 1U)),
	     "main memory map is not P0..P29 contiguous from bit 0");

/*
 * Peripheral memories, as (resource, PDSLEEPCFG4 field) pairs. Both domains can
 * vote on all of them, and PDSLEEPCFG5 (periphery) repeats the bit assignment of
 * PDSLEEPCFG4 (array), so one list covers the lot.
 *
 * The field names are not derivable from the resource names -- the SDK spells the
 * SDHC/USB buffers with a _SRAM suffix and DMA0_1 with a _P_E one -- so both are
 * given. Every macro below pastes the field name itself, rather than calling a
 * shared helper to do it: several of these names (XSPI0, OCOTP, ...) are also the
 * SDK's peripheral base-pointer macros, and a name handed to a helper would be
 * expanded to one before the paste. As an operand of ## it is left alone.
 */
#define PWR_MEM_SHARED_RESOURCES(_entry)					\
	_entry(MEM_SDHC0, SDHC0_SRAM)						\
	_entry(MEM_SDHC1, SDHC1_SRAM)						\
	_entry(MEM_USB0, USB0_SRAM)						\
	_entry(MEM_USB1, USB1_SRAM)						\
	_entry(MEM_JPEG, JPEG)							\
	_entry(MEM_PNG, PNG)							\
	_entry(MEM_MIPI, MIPI)							\
	_entry(MEM_GPU, GPU)							\
	_entry(MEM_DMA0_1, DMA0_1_P_E)						\
	_entry(MEM_DMA2_3, DMA2_3)						\
	_entry(MEM_CPU0_CCACHE, CPU0_CCACHE)	/* CPU0's code cache -> XCACHE1 */	\
	_entry(MEM_CPU0_SCACHE, CPU0_SCACHE)	/* CPU0's system cache -> XCACHE0 */	\
	_entry(MEM_DSP_ICACHE, DSP_ICACHE)					\
	_entry(MEM_DSP_DCACHE, DSP_DCACHE)					\
	_entry(MEM_DSP_ITCM, DSP_ITCM)						\
	_entry(MEM_DSP_DTCM, DSP_DTCM)						\
	_entry(MEM_EZH_TCM, EZH_TCM)						\
	_entry(MEM_NPU, NPU)							\
	_entry(MEM_XSPI0, XSPI0)						\
	_entry(MEM_XSPI1, XSPI1)						\
	_entry(MEM_XSPI2, XSPI2)						\
	_entry(MEM_LCD, LCD)							\
	_entry(MEM_OCOTP, OCOTP)

#define _PWR_MEM_BIT(_res, _field)	| PMC_PDSLEEPCFG4_##_field##_MASK
#define _PWR_MEM_ASSERT(_res, _field)						\
	(PMC_PDSLEEPCFG4_##_field##_MASK == PMC_PDSLEEPCFG5_##_field##_MASK) &&

/* Every peripheral-memory bit, for the "managed" set of PDSLEEPCFG4/5. */
#define PWR_MEM_PERIPH_ALL (0U PWR_MEM_SHARED_RESOURCES(_PWR_MEM_BIT))

BUILD_ASSERT(PWR_MEM_SHARED_RESOURCES(_PWR_MEM_ASSERT) true,
	     "PDSLEEPCFG4 and PDSLEEPCFG5 bit assignments differ");

/* One res_map entry. */
#define PWR_MAP(_reg, _mask) { .reg = (_reg), .mask = (_mask) }

/* Resources both CPU0 and CPU1 control, through bits in their own PMC instance. */
#define PWR_RES_MAP_PMC_SHARED									\
	[PWR_RES_MEDIA] = PWR_MAP(PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_V2NMED_DSR_MASK),		\
	[PWR_RES_VDD2_COM] = PWR_MAP(PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_V2COM_DSR_MASK),		\
	[PWR_RES_VDDN_COM] = PWR_MAP(PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_VNCOM_DSR_MASK),		\
	[PWR_RES_VDD2_MIPI] = PWR_MAP(PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_V2MIPI_PD_MASK),		\
												\
	[PWR_RES_TEMP_SENSOR] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_TEMP_PD_MASK),		\
	[PWR_RES_PMC_REF] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_PMCREF_LP_MASK),		\
	[PWR_RES_DETECTOR_VDD1] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_POR1_LP_MASK |	\
							  PMC_PDSLEEPCFG1_LVD1_LP_MASK |	\
							  PMC_PDSLEEPCFG1_HVD1_PD_MASK),	\
	[PWR_RES_DETECTOR_VDD2] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_POR2_LP_MASK |	\
							  PMC_PDSLEEPCFG1_LVD2_LP_MASK |	\
							  PMC_PDSLEEPCFG1_HVD2_PD_MASK),	\
	[PWR_RES_DETECTOR_VDDN] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_PORN_LP_MASK |	\
							  PMC_PDSLEEPCFG1_LVDN_LP_MASK |	\
							  PMC_PDSLEEPCFG1_HVDN_PD_MASK),	\
	[PWR_RES_DETECTOR_VDD1V8] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_HVD1V8_PD_MASK),	\
	[PWR_RES_AGDET] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_AGDET1_PD_MASK |		\
						  PMC_PDSLEEPCFG1_AGDET2_PD_MASK),		\
	[PWR_RES_SRAM_AWAKE] = PWR_MAP(PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_SRAMSLEEP_MASK),

/* Resources only CPU0 controls, through bits in PMC0. */
#define PWR_PMC_COMPUTE_ONLY_RESOURCES(_entry)					\
	_entry(VDD2_COMP, PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_V2COMP_DSR_MASK)	\
	_entry(VDD2_DSP, PWR_REG_PDSLP0, PMC_PDSLEEPCFG0_V2DSP_PD_MASK)		\
	_entry(OTP, PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_OTP_PD_MASK)		\
	_entry(ROM, PWR_REG_PDSLP1, PMC_PDSLEEPCFG1_ROM_PD_MASK)

/* Resources both CPU0 and CPU1 control, through bits in their own SLEEPCON. */
#define PWR_SLEEPCON_SHARED_RESOURCES(_entry)					\
	_entry(CLK_SENSE_MAIN_P, PWR_SLPCFG(SENSEP_MAINCLK_SHUTOFF))		\
	_entry(CLK_SENSE_MAIN_S, PWR_SLPCFG(SENSES_MAINCLK_SHUTOFF))		\
	_entry(CLK_RAM0, PWR_SLPCFG(RAM0_CLK_SHUTOFF))				\
	_entry(CLK_RAM1, PWR_SLPCFG(RAM1_CLK_SHUTOFF))				\
	_entry(CLK_COMN_MAIN, PWR_SLPCFG(COMN_MAINCLK_SHUTOFF))			\
	_entry(CLK_MEDIA_MAIN, PWR_SLPCFG(MEDIA_MAINCLK_SHUTOFF))		\
	_entry(XTAL, PWR_SLPCFG(XTAL_PD))					\
	_entry(FRO2, PWR_SLPCFG(FRO2_PD) | PWR_SLPCFG(FRO2_GATE))		\
	_entry(LPOSC, PWR_SLPCFG(LPOSC_PD))					\
	_entry(MAIN_PLL, PWR_SLPCFG(PLLANA_PD) | PWR_SLPCFG(PLLLDO_PD))		\
	_entry(AUDIO_PLL, PWR_SLPCFG(AUDPLLANA_PD) | PWR_SLPCFG(AUDPLLLDO_PD))	\
	_entry(ADC0, PWR_SLPCFG(ADC0_PD))

/* Resources only CPU0 controls, through bits in SLEEPCON0. */
#define PWR_SLEEPCON0_ONLY_RESOURCES(_entry)					\
	_entry(CLK_COMPUTE_MAIN, PWR_SLPCFG(COMP_MAINCLK_SHUTOFF))		\
	_entry(FRO0, PWR_SLPCFG(FRO0_PD) | PWR_SLPCFG(FRO0_GATE))		\
	_entry(FRO1, PWR_SLPCFG(FRO1_PD))

#define _PWR_MAP_SLPCFG(_res, _fields) [PWR_RES_##_res] = PWR_MAP(PWR_REG_SLEEPCFG, _fields),
#define _PWR_MAP_REG(_res, _reg, _fields) [PWR_RES_##_res] = PWR_MAP(_reg, _fields),
#define _PWR_MAP_MEM(_res, _field)						\
	[PWR_RES_##_res] = PWR_MAP(PWR_REG_PDSLP4, PMC_PDSLEEPCFG4_##_field##_MASK),
#define _PWR_RES_BIT(_res, ...) | PWR_RES(PWR_RES_##_res)

/*
 * res_map entries. A Sense table takes the three shared thirds; Compute takes all
 * five. A peripheral memory maps to its PDSLEEPCFG4 (array) bit only: PDSLEEPCFG5
 * holds the access path, which no request keeps -- see power_commit().
 */
#define PWR_RES_MAP_SLEEPCON_SHARED	PWR_SLEEPCON_SHARED_RESOURCES(_PWR_MAP_SLPCFG)
#define PWR_RES_MAP_MEM_SHARED		PWR_MEM_SHARED_RESOURCES(_PWR_MAP_MEM)
#define PWR_RES_MAP_PMC_COMPUTE_ONLY	PWR_PMC_COMPUTE_ONLY_RESOURCES(_PWR_MAP_REG)
#define PWR_RES_MAP_SLEEPCON0_ONLY	PWR_SLEEPCON0_ONLY_RESOURCES(_PWR_MAP_SLPCFG)

#define PWR_RES_SLEEPCON_SHARED	((power_res_mask_t)0						\
				 PWR_SLEEPCON_SHARED_RESOURCES(_PWR_RES_BIT))
#define PWR_RES_COMPUTE_ONLY	((power_res_mask_t)0 PWR_SLEEPCON0_ONLY_RESOURCES(_PWR_RES_BIT)	\
				 PWR_PMC_COMPUTE_ONLY_RESOURCES(_PWR_RES_BIT))

BUILD_ASSERT((PWR_RES_SLEEPCON_SHARED & PWR_RES_COMPUTE_ONLY) == 0,
	     "a resource is either arbitrated between the domains or owned by one");

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_REGMAP_H_ */
