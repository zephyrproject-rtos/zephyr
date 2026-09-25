/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_DOMAIN_H_
#define SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_DOMAIN_H_

#include <stdbool.h>
#include <stdint.h>

#include <fsl_common.h>

#include "power_regmap.h"
#include "power_resources.h"

struct power_domain;
struct power_mode_desc;

/*
 * Which PDSLEEPCFG0 mode bit a low power mode asserts. The four are mutually
 * exclusive in hardware, so they are one field rather than independent flags.
 */
enum low_power_mode {
	LP_DEEP_SLEEP = 0,	/* no mode bit */
	LP_DSR,			/* FDSR */
	LP_DPD,			/* DPD */
	LP_FDPD,		/* FDPD */
};

/*
 * PMIC / regulator decision hook.
 *
 * Programs the PMICMODE field and the LDO/rail decision into PDSLEEPCFG0. The
 * default implementation (power_common.c) drives the on-chip LDOs from the PMC
 * POWERCFG.LDOxPD strapping, which is what the SoC does today. A board that powers
 * a rail from an external PMIC instead can supply a different ops table, or
 * override the per-mode pmic_mode, without touching the programmer or the mode
 * descriptors.
 */
struct power_regulator_ops {
	void (*program)(const struct power_domain *dom, const struct power_mode_desc *mode);
};

/*
 * A clock power-down-ready hook. Both of them below take the resolved request,
 * because a clock the request holds up never powers down and so never produces
 * the ready signal SLEEPCON would otherwise wait for. The SLEEPCON they program
 * is SOC_SLEEPCON, so it is not a parameter.
 */
typedef void (*soc_clock_pdr_fn)(const struct power_request *req);

struct power_domain {
	/* Where each resource's control bits live for this domain, indexed by
	 * enum power_resource. Resources this domain does not have carry a zero
	 * mask and are skipped. This is the only place in the driver that knows
	 * a resource maps to a register field.
	 */
	const struct power_res_map *res_map;

	/* The rail that must stay powered for this domain's SLEEPCON to see a
	 * wake-up event (RM 31.3.3: "the power domain containing the controlling
	 * SLEEPCON instance must remain powered ... VDD2_COM must be powered to
	 * allow SLEEPCON0 to wake VDD2_COMP"). power_keepalive_collect() adds it to
	 * every mode that returns, so no mode has to remember to. PWR_RES_NONE when
	 * the rail is not one of this driver's resources -- SLEEPCON1 sits in
	 * VDD1_SENSE, which no PDSLEEPCFG0 field of PMC1 controls.
	 */
	enum power_resource res_sleepcon_rail;

	/* Resources this domain is the sole voter for, so its vote is the whole
	 * decision and voting one down while the peer uses it breaks the peer:
	 * PWR_RES_COMPUTE_ONLY on Compute, zero on Sense.
	 */
	power_res_mask_t res_vote_exclusive;

	/* What the peer needs out of this domain's `res_vote_exclusive`, for this
	 * domain to add to its keep-alive set. NULL on Sense, whose
	 * `res_vote_exclusive` is empty and so leaves nothing to ask about.
	 */
	power_res_mask_t (*peer_needs)(void);

	/* Resources the request implies once the board's clock tree is taken into
	 * account, which a compile-time keep set cannot name. Keeping a PLL is the
	 * case that needs it: a PLL is only useful if whatever feeds it is still
	 * there, and the board chose that source, not this driver. Called with the
	 * request as the modes stated it, before the dependency closure, so what it
	 * returns is closed over too. NULL for a domain whose modes keep nothing with
	 * a configurable input.
	 *
	 * This reads clock selects, not power votes: a select is a fact about how the
	 * board wired itself up at boot, while the votes are what this driver is in
	 * the middle of deciding and must never be inferred from their current value.
	 */
	power_res_mask_t (*res_clock_tree)(const struct power_request *req);

	/* `LDO*_VSEL` bit this domain votes low so aggregation yields control of
	 * that rail's voltage to the other domain. Used by the default LDO ops.
	 */
	uint32_t	ldo_vsel_clear;

	/* Stall the HiFi DSP if VDD2_DSP is about to power down while the DSP might
	 * still be running: an in-flight AHB transaction at power-off would hang the
	 * bus. The stall is not lifted on resume -- the domain was powered off, so
	 * whoever owns the DSP (dsp_ctrl / remoteproc) must re-release it with a fresh
	 * image. SYSCON0 lives only in the Compute (core0) header set, so this access
	 * stays in this TU.
	 */
	void (*stall_dsp_if_powered_down)(void);

	/* If this domain owns clocks that it is powering down, make SLEEPCON wait for
	 * their power-down-ready handshake before the domain powers down (IGN_*PDR:
	 * 0 = wait, 1 = ignore). Sole ownership is what makes waiting safe -- no other
	 * domain can hold the clock up, so the signal really does arrive. Only Compute
	 * owns such clocks (FRO0/FRO1, which SLEEPCON1 has no control bits for at
	 * all); NULL on Sense. The bits are sticky and nothing else clears them, so
	 * clearing re-asserts the reset default defensively. A clock this mode keeps
	 * alive never powers down, so PWRDOWN_WAIT does not apply to it and its bit is
	 * left untouched. Runs before the XSPI hand-over, flash still live, so it need
	 * not be RAM-resident.
	 */
	soc_clock_pdr_fn prep_owned_clock_pdr;

	/* Tell the PMC not to wait for the shared FRO2/LPOSC power-down-ready before
	 * it lets this domain sleep. Two independent reasons that handshake may never
	 * arrive, and either one hangs the PMC state machine (WFI degrades to plain
	 * sleep):
	 *
	 *   - this mode keeps the clock alive, so it is not powering down at all;
	 *   - the peer is not in deep sleep, so it is contributing its run-bank vote
	 *     to the aggregation. That vote may or may not hold the clock up -- the
	 *     peer need not have written it -- which is exactly why waiting is wrong:
	 *     the ready signal is not this domain's to predict.
	 *
	 * Compute tests both (DSSENS for the second) and runs from RAM, because it
	 * runs after the XSPI hand-over. The first case is why DSSENS alone will not
	 * do: Sense parked in deep sleep with the clock kept alive on its account
	 * would block forever. Sense cannot see the peer's state and so does it
	 * unconditionally. Set-and-never-clear, matching MCUXpresso.
	 */
	soc_clock_pdr_fn arm_shared_clock_pdr_ignores;

	/* PMIC / regulator decision. Defaults to the on-chip LDO ops. */
	const struct power_regulator_ops *regulator;

	/* XIP suspend/resume. Non-NULL only on the Compute domain, and only when
	 * CONFIG_SOC_MIMXRT7XX_PM_XIP_HANDOVER is built. The Sense TU never links
	 * the XIP object, so these stay NULL there.
	 */
	void (*xip_suspend)(bool flush_scache, bool flush_ccache);
	void (*xip_resume)(void);
};

/*
 * A low-power mode: what the mode needs, independent of which domain runs it.
 * One const table per domain, because the resources a mode names are only
 * meaningful against that domain's res_map.
 */
struct power_mode_desc {
	enum low_power_mode low_power_mode;

	/* The resources this mode needs to survive the low-power window. Everything
	 * absent is voted down, so a mode lists exactly what it uses.
	 */
	struct power_request keep;

	uint8_t	pmic_mode;
};

/*
 * DPD and FDPD exit through a cold boot; every other mode resumes after WFI.
 *
 * Two things follow from returning, and neither has a mode of its own: the arch
 * hooks have CPU state worth saving, and the SLEEPCON has to stay powered to see
 * the wake-up event. A mode that did one without the other would either resume on
 * clobbered state or hold a rail up for a wake-up that never comes.
 */
static inline bool power_mode_returns(const struct power_mode_desc *mode)
{
	return mode->low_power_mode < LP_DPD;
}

/*
 * Build the resolved request for (domain, mode): the mode's compile-time base,
 * plus the SLEEPCON rail when the mode returns, closed over res_deps[]. This is
 * the single seam where device runtime PM will later fold in per-device retention
 * requirements.
 */
struct power_request power_keepalive_collect(const struct power_domain *dom,
					     const struct power_mode_desc *mode);

/*
 * Everything declared through power_cross_domain_request(). Serves directly as a
 * domain's peer_needs when the declarations are all it has to go on; a domain with
 * a readable register to consult wraps it instead.
 */
power_res_mask_t power_cross_domain_requested(void);

/* The one shared low-power entry sequence, driven by (domain, mode). */
void power_enter_common(const struct power_domain *dom, const struct power_mode_desc *mode);

/* Default on-chip LDO regulator ops (power_common.c). */
extern const struct power_regulator_ops power_ldo_regulator_ops;

#endif /* SOC_NXP_IMXRT_IMXRT7XX_CM33_POWER_POWER_DOMAIN_H_ */
