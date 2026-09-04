/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Named power resources for the RT7xx CM33 low-power driver.
 *
 * The low-power entry path decides *which blocks to keep powered* over a sleep
 * window, and this header is the vocabulary for that decision. It contains no
 * register knowledge at all: a resource is an identity, not a bit.
 *
 * That separation matters because a resource's control bit is not stable. LPOSC is
 * voted on in SLEEPCON.RUNCFG while the domain is active and in SLEEPCON.SLEEPCFG
 * for the sleep window; a FRO has a power-down bit *and* an output-gate bit; a
 * detector group is three bits. Naming the resource once and mapping it to
 * (register, mask) in exactly one place -- struct power_res_map, power_regmap.h --
 * is what spares callers all of that, whether the block is controlled from SLEEPCON
 * or from the PMC and whether it is a clock source, an analog reference or a memory.
 * The one thing addressed differently is the main SRAM, whose partitions are
 * numbered rather than named; see struct power_request.
 *
 * The polarity flip lives in that same one place: every managed bit in SLEEPCFG and
 * PDSLEEPCFG0-5 means "power this down", while a request means "keep this alive".
 * power_commit() is the only code that crosses between the two.
 *
 * Layering:
 *   1. enum power_resource      identity            (this header)
 *   2. struct power_request     "what I need alive" (this header)
 *   3. power_resolve()          dependency closure  (power_resources.c)
 *   4. power_commit()           resource -> bits    (power_common.c + regmap)
 *   5. power_enter_common()     sequencing          (power_common.c)
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_RESOURCES_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_RESOURCES_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

/*
 * Every resource that has a name: clock sources, main clocks, voltage domains, the
 * analog references and the peripheral memories.
 *
 * The line drawn here is naming, not size. A named block is one resource however
 * large, so the caches and the OCOTP shadow belong next to the PLLs -- they are kept
 * or dropped by name, and being memories buys them nothing. What stays out is the
 * main SRAM, whose partitions are numbered rather than named; they are the `sram`
 * bitmap of a request.
 *
 * Not every resource exists on both domains. Compute owns FRO0/FRO1 and the compute
 * main clock outright (SLEEPCON1 has no bits for them at all); the Sense map just
 * gives those a zero mask rather than needing a separate enum.
 */
enum power_resource {
	/* Main clocks (SLEEPCFG *_MAINCLK_SHUTOFF / *_CLK_SHUTOFF). */
	PWR_RES_CLK_COMPUTE_MAIN,
	PWR_RES_CLK_SENSE_MAIN_P,
	PWR_RES_CLK_SENSE_MAIN_S,
	PWR_RES_CLK_MEDIA_MAIN,
	PWR_RES_CLK_COMN_MAIN,
	PWR_RES_CLK_RAM0,
	PWR_RES_CLK_RAM1,

	/* Clock sources and analog. A source covers both its power-down and its
	 * output-gate bit where both exist: keeping the source alive means not
	 * gating it either.
	 */
	PWR_RES_FRO0,
	PWR_RES_FRO1,
	PWR_RES_FRO2,
	PWR_RES_XTAL,
	PWR_RES_LPOSC,
	PWR_RES_MAIN_PLL,
	PWR_RES_AUDIO_PLL,
	PWR_RES_ADC0,

	/* Voltage domains (PDSLEEPCFG0). VDD2_MEDIA and VDDN_MEDIA share one
	 * power switch and therefore one bit, so they are one resource.
	 */
	PWR_RES_VDD2_COMP,
	PWR_RES_VDD2_COM,
	PWR_RES_VDDN_COM,
	PWR_RES_MEDIA,
	PWR_RES_VDD2_DSP,
	PWR_RES_VDD2_MIPI,

	/* Analog references and detectors (PDSLEEPCFG1). The per-rail detector
	 * groups bundle that rail's POR + LVD + HVD, which are only ever kept or
	 * dropped together.
	 */
	PWR_RES_PMC_REF,
	PWR_RES_DETECTOR_VDD1,
	PWR_RES_DETECTOR_VDD2,
	PWR_RES_DETECTOR_VDDN,
	PWR_RES_DETECTOR_VDD1V8,
	PWR_RES_AGDET,
	PWR_RES_TEMP_SENSOR,
	PWR_RES_ROM,
	/* The OTP block itself (PDSLEEPCFG1). Distinct from the OCOTP shadow RAM,
	 * which is a peripheral memory -- PWR_RES_MEM_OCOTP below.
	 */
	PWR_RES_OTP,
	/* PDSLEEPCFG1[SRAMSLEEP] drops the SRAM into its sleep state; requesting
	 * this resource keeps it awake instead. Register-wide, not per partition --
	 * per-partition control is the `sram` bitmap of a request.
	 */
	PWR_RES_SRAM_AWAKE,

	/* Peripheral memories: the caches, TCMs, the OCOTP shadow and the DMA /
	 * USB / SDHC buffers. Requesting one retains its contents; the access path
	 * is not retained, because a core in a low-power window is not reaching
	 * them (see power_commit()). The MEM infix says "stored contents, not a
	 * live block", and is what keeps PWR_RES_OTP and PWR_RES_MEM_OCOTP apart.
	 *
	 * Mind the cache numbering: both caches belong to CPU0 (CPU1 has none) and
	 * the instances do not follow the core numbers -- the code cache is XCACHE1,
	 * the system cache is XCACHE0. These names mirror the PMC field names so
	 * they cannot be read as "XCACHE instance 0".
	 */
	PWR_RES_MEM_SDHC0,
	PWR_RES_MEM_SDHC1,
	PWR_RES_MEM_USB0,
	PWR_RES_MEM_USB1,
	PWR_RES_MEM_JPEG,
	PWR_RES_MEM_PNG,
	PWR_RES_MEM_MIPI,
	PWR_RES_MEM_GPU,
	PWR_RES_MEM_DMA0_1,
	PWR_RES_MEM_DMA2_3,
	PWR_RES_MEM_CPU0_CCACHE,
	PWR_RES_MEM_CPU0_SCACHE,
	PWR_RES_MEM_DSP_ICACHE,
	PWR_RES_MEM_DSP_DCACHE,
	PWR_RES_MEM_DSP_ITCM,
	PWR_RES_MEM_DSP_DTCM,
	PWR_RES_MEM_EZH_TCM,
	PWR_RES_MEM_NPU,
	PWR_RES_MEM_XSPI0,
	PWR_RES_MEM_XSPI1,
	PWR_RES_MEM_XSPI2,
	PWR_RES_MEM_LCD,
	PWR_RES_MEM_OCOTP,

	PWR_RES_COUNT,
	/* "No such resource", for optional per-domain resource references. */
	PWR_RES_NONE = PWR_RES_COUNT,
};

/*
 * A set of named resources. One bit per enum power_resource, so a request set is
 * a single scalar and can be written as a compile-time initializer:
 *
 *   .res = PWR_RES(PWR_RES_MAIN_PLL) | PWR_RES(PWR_RES_VDD2_COMP)
 */
typedef uint64_t power_res_mask_t;

#define PWR_RES(res) ((power_res_mask_t)1ULL << (res))

BUILD_ASSERT(PWR_RES_COUNT <= 64, "power_res_mask_t cannot hold every resource");

/*
 * Everything a domain needs kept alive across one low-power window. Two fields
 * because the hardware keeps two kinds of thing, addressed differently: `res` is
 * everything with a name, one bit per enum power_resource; `sram` is the main SRAM
 * window, one bit per physical partition P0..P29.
 *
 * Nobody writes a partition by hand -- `sram` comes from devicetree at build time
 * (POWER_SRAM_KEEPALIVE, sram_banks.h), which is exactly why naming the partitions
 * individually would buy nothing.
 */
struct power_request {
	power_res_mask_t res;
	uint32_t	 sram;
};

/* True when res is in the request set. */
static inline bool power_requested(const struct power_request *req, enum power_resource res)
{
	return (req->res & PWR_RES(res)) != 0U;
}

/* Add res to the request set. */
static inline void power_request_add(struct power_request *req, enum power_resource res)
{
	req->res |= PWR_RES(res);
}

/*
 * Close the request set over the hardware's resource dependencies: for every
 * resource kept alive, keep alive everything it needs. Idempotent.
 *
 * This is what makes an illegal power configuration unrepresentable rather than
 * something the register programming has to detect and undo. RM 31.2 Table 329
 * is explicit that hardware does not block illegal combinations, so the check
 * has to happen here -- once, before any register is touched.
 */
void power_resolve(struct power_request *req);

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_RESOURCES_H_ */
