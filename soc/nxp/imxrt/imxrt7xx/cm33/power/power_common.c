/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared RT7xx CM33 low-power entry sequence.
 *
 * One power_enter_common() drives every (domain, mode) pair: deep sleep and DSR on
 * the Compute domain, deep sleep on Sense, and DPD/FDPD on either. The per-domain
 * and per-mode facts arrive as data (struct power_domain / struct power_mode_desc);
 * this file owns the sequencing and the one resource -> register translation, so
 * the two domains cannot drift apart.
 *
 * The translation is a single loop over enum power_resource. A mode says which
 * resources it needs; power_keepalive_collect() closes that over the dependency
 * graph; power_commit() votes down everything the resolved request does not name,
 * because RM 31.5.2 requires a core to power down every common resource it does
 * not use -- an idle 0 vote there defeats the other core's power-down. There is
 * no per-register step and no hand-written polarity flip: the "1 means powered
 * down" inversion happens once, below.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arch_interface.h>

#include "power_domain.h"
#include "power_regmap.h"
#include "power_resources.h"
#include "power_cross_domain.h"

#include <fsl_common.h>

/*
 * Mode bits. Not resources -- they select which low-power mode the votes are
 * evaluated for. Cleared before each entry so a stale value cannot leak, then
 * re-applied from the mode descriptor. PMICMODE is in the set because the
 * regulator ops program it right after the commit.
 */
#define PDSLPCFG0_MODE_BITS (\
	PMC_PDSLEEPCFG0_FDSR_MASK | PMC_PDSLEEPCFG0_DPD_MASK | PMC_PDSLEEPCFG0_FDPD_MASK | \
	PMC_PDSLEEPCFG0_PMICMODE_MASK)

/*
 * Translate the resolved request into the PDSLEEPCFG/SLEEPCFG registers.
 *
 * Every bit this domain's res_map claims is "managed": it is rewritten from the
 * request, set when the resource is not requested and cleared when it is. Bits
 * outside the managed set keep their live value -- that is how the body-bias
 * fields of PDSLEEPCFG0, which this driver does not own, survive an entry.
 *
 * The RM 31.2 / Table 329 legality rules the old per-register programmers had to
 * repair after the fact ("VNCOM_DSR requested but VDDN_MEDIA kept powered") are
 * no longer expressible here: power_resolve() has already closed the request over
 * res_deps[], so an illegal combination cannot reach this function.
 */
static void power_commit(const struct power_domain *dom, const struct power_mode_desc *mode,
			 const struct power_request *req)
{
	uint32_t managed[PWR_REG_COUNT] = {0};
	uint32_t off[PWR_REG_COUNT] = {0};

	for (unsigned int r = 0U; r < PWR_RES_COUNT; r++) {
		const struct power_res_map *map = &dom->res_map[r];

		if (map->mask == 0U) {
			/* Not a resource this domain controls. */
			continue;
		}

		managed[map->reg] |= map->mask;

		if (!power_requested(req, (enum power_resource)r)) {
			off[map->reg] |= map->mask;
		}
	}

	/*
	 * The main SRAM partitions are numbered rather than named, so they are a
	 * bitmap rather than res_map entries: bit n of the request is partition Pn,
	 * which is also its PDSLEEPCFG2 bit. Same inversion, applied at once.
	 */
	managed[PWR_REG_PDSLP2] |= PWR_SRAM_ALL;
	off[PWR_REG_PDSLP2] |= PWR_SRAM_ALL & ~req->sram;

	/*
	 * PDSLEEPCFG3 and PDSLEEPCFG5 hold the *access path* to those same memories,
	 * at the same bit positions as the array registers above. Nothing ever
	 * requests one: the core is stopped for the whole window, so it cannot reach
	 * the contents it just asked to retain. So they are voted down wholesale --
	 * but only over the bits this domain claimed for the matching array register,
	 * so a domain that does not vote on a memory does not start voting on its
	 * access path either.
	 */
	managed[PWR_REG_PDSLP3] |= PWR_SRAM_ALL;
	off[PWR_REG_PDSLP3] |= PWR_SRAM_ALL;
	managed[PWR_REG_PDSLP5] |= managed[PWR_REG_PDSLP4];
	off[PWR_REG_PDSLP5] |= managed[PWR_REG_PDSLP4];

	uint32_t mode_bits = 0U;

	switch (mode->low_power_mode) {
	case LP_DSR:
		mode_bits |= PMC_PDSLEEPCFG0_FDSR_MASK;
		break;
	case LP_DPD:
		mode_bits |= PMC_PDSLEEPCFG0_DPD_MASK;
		break;
	case LP_FDPD:
		mode_bits |= PMC_PDSLEEPCFG0_FDPD_MASK;
		/*
		 * FDPD leaves the voltage references in high-power mode by default so
		 * that a rising VDD1V8 can wake the chip, which makes FDPD draw more
		 * than plain DPD. Nothing here uses that wake-up path, so put the band
		 * gap in low-power mode instead. Cleared by every cold reset -
		 * including the FDPD wake-up itself - hence set on every entry.
		 * (Read-only in the Sense slot copy, where the write is a no-op.)
		 */
		SOC_PMC->POWERCFG |= PMC_POWERCFG_FDPDBGLP_MASK;
		break;
	case LP_DEEP_SLEEP:
	default:
		break;
	}

	/*
	 * SLEEPCFG first, matching the order the pre-refactor driver used. Bit 30
	 * (FRO1_GATE per RM Table 345) has no mask in the SDK header and so is not
	 * a resource yet; it keeps its live value, which is its 0 reset value.
	 */
	SOC_SLEEPCON->SLEEPCFG = (SOC_SLEEPCON->SLEEPCFG & ~managed[PWR_REG_SLEEPCFG]) |
				 off[PWR_REG_SLEEPCFG];

	SOC_PMC->PDSLEEPCFG0 = (SOC_PMC->PDSLEEPCFG0 &
				~(managed[PWR_REG_PDSLP0] | PDSLPCFG0_MODE_BITS)) |
			       off[PWR_REG_PDSLP0] | mode_bits;
	SOC_PMC->PDSLEEPCFG1 = (SOC_PMC->PDSLEEPCFG1 & ~managed[PWR_REG_PDSLP1]) |
			       off[PWR_REG_PDSLP1];
	SOC_PMC->PDSLEEPCFG2 = (SOC_PMC->PDSLEEPCFG2 & ~managed[PWR_REG_PDSLP2]) |
			       off[PWR_REG_PDSLP2];
	SOC_PMC->PDSLEEPCFG3 = (SOC_PMC->PDSLEEPCFG3 & ~managed[PWR_REG_PDSLP3]) |
			       off[PWR_REG_PDSLP3];
	SOC_PMC->PDSLEEPCFG4 = (SOC_PMC->PDSLEEPCFG4 & ~managed[PWR_REG_PDSLP4]) |
			       off[PWR_REG_PDSLP4];
	SOC_PMC->PDSLEEPCFG5 = (SOC_PMC->PDSLEEPCFG5 & ~managed[PWR_REG_PDSLP5]) |
			       off[PWR_REG_PDSLP5];

	/*
	 * DPD (not FDPD) keeps VDD1V8 alive across the cold boot; clear the DSR
	 * request bits in the run config so they cannot leak into the boot state.
	 */
	if (mode->low_power_mode == LP_DPD) {
		SOC_PMC->PDRUNCFG0 &= ~(PMC_PDRUNCFG0_V2NMED_DSR_MASK |
					PMC_PDRUNCFG0_VNCOM_DSR_MASK);
	}
}

/*
 * Default on-chip LDO regulator ops: program PMICMODE and drive the LDOs from
 * the POWERCFG.LDOxPD strapping. A board on an external PMIC replaces this.
 *
 * Runs after power_commit(), which has just cleared PMICMODE along with the other
 * mode bits. The regulator fields are not resources -- they are a per-mode
 * voltage decision, not something a device can request -- so they stay out of
 * res_map and are programmed here.
 */
static void ldo_regulator_program(const struct power_domain *dom,
				  const struct power_mode_desc *mode)
{
	SOC_PMC->PDSLEEPCFG0 = (SOC_PMC->PDSLEEPCFG0 & ~PMC_PDSLEEPCFG0_PMICMODE_MASK) |
			       PMC_PDSLEEPCFG0_PMICMODE(mode->pmic_mode);

	/*
	 * Ask for the low voltage on the rail this domain is allowed to lower. The
	 * PMC aggregates the two domains' VSEL fields and the higher setting wins, so
	 * this only takes effect if the other domain is not asking for the high one --
	 * which is what makes it safe to do unconditionally.
	 */
	SOC_PMC->PDSLEEPCFG0 &= ~dom->ldo_vsel_clear;

	/* Rail on PMIC -> bypass LDO in low power; rail on LDO -> LDO low-power. */
	if ((SOC_PMC->POWERCFG & PMC_POWERCFG_LDO1PD_MASK) != 0U) {
		SOC_PMC->PDSLEEPCFG0 &= ~PMC_PDSLEEPCFG0_LDO1_MODE_MASK;
	} else {
		SOC_PMC->PDSLEEPCFG0 |= PMC_PDSLEEPCFG0_LDO1_MODE_MASK;
	}

	if ((SOC_PMC->POWERCFG & PMC_POWERCFG_LDO2PD_MASK) != 0U) {
		SOC_PMC->PDSLEEPCFG0 &= ~PMC_PDSLEEPCFG0_LDO2_MODE_MASK;
	} else {
		SOC_PMC->PDSLEEPCFG0 |= PMC_PDSLEEPCFG0_LDO2_MODE_MASK;
	}
}

const struct power_regulator_ops power_ldo_regulator_ops = {
	.program = ldo_regulator_program,
};

/* Clear sticky PMC wake/power flags so the next wake reason reads cleanly. */
static void pmc_clear_event_flags(void)
{
	SOC_PMC->FLAGS = SOC_PMC->FLAGS;
	SOC_PMC->PWRFLAGS = SOC_PMC->PWRFLAGS;
}

/*
 * Disable LVD/AGDET-driven resets across the window: the regulator LP switch
 * briefly dips the rail and would otherwise be mistaken for a brown-out. Returns
 * the saved CTRL for restore (poweroff paths do not restore).
 */
static uint32_t lvd_save_disable(void)
{
	uint32_t saved = SOC_PMC->CTRL;

	SOC_PMC->CTRL = saved & ~(PMC_CTRL_LVDNRE_MASK | PMC_CTRL_LVD2RE_MASK |
				  PMC_CTRL_LVD1RE_MASK | PMC_CTRL_AGDET2RE_MASK |
				  PMC_CTRL_AGDET1RE_MASK);
	return saved;
}

/*
 * Clear the LVD/AGDET status flags set by the sleep-time rail dip, then restore
 * CTRL. Order matters: clear first, then re-enable RE, or the stale flags would
 * trip an immediate reset.
 *
 * Safe to call after the XSPI hand-over: SOC_PMC is a compile-time constant, not
 * a read through const data that XIP teardown has taken away.
 */
static void lvd_restore(uint32_t saved_ctrl)
{
	SOC_PMC->FLAGS = PMC_FLAGS_LVDVDD1F_MASK | PMC_FLAGS_LVDVDD2F_MASK |
			 PMC_FLAGS_LVDVDDNF_MASK | PMC_FLAGS_AGDET1F_MASK |
			 PMC_FLAGS_AGDET2F_MASK;
	SOC_PMC->CTRL = saved_ctrl;
}

/* Latch all PMC programming above: drain any in-flight update, pulse APPLYCFG,
 * then wait for completion before WFI.
 */
static void pmc_apply_and_wait(void)
{
	while ((SOC_PMC->STATUS & PMC_STATUS_BUSY_MASK) != 0U) {
	}
	SOC_PMC->CTRL |= PMC_CTRL_APPLYCFG_MASK;
	while ((SOC_PMC->STATUS & PMC_STATUS_BUSY_MASK) != 0U) {
	}
}

/*
 * Peer-domain needs declared by board or driver code. Cumulative, written during
 * init and read at every entry, all from this core. Over-declaring only spends
 * power, so a declaration that turns out to be unnecessary is harmless.
 */
static power_res_mask_t cross_domain_res;

void power_cross_domain_request(enum power_resource res)
{
	cross_domain_res |= PWR_RES(res);
}

power_res_mask_t power_cross_domain_requested(void)
{
	return cross_domain_res;
}

struct power_request power_keepalive_collect(const struct power_domain *dom,
					     const struct power_mode_desc *mode)
{
	struct power_request req = mode->keep;

	/*
	 * RM 31.3.3: the rail holding the SLEEPCON that has to see the wake-up
	 * event must stay powered. Adding it here rather than in each mode's keep
	 * set means a mode cannot forget it, and a mode that never comes back
	 * (DPD/FDPD) does not pay for it.
	 */
	if (power_mode_returns(mode) && dom->res_sleepcon_rail != PWR_RES_NONE) {
		power_request_add(&req, dom->res_sleepcon_rail);
	}

	/*
	 * LPOSC clocks the PMC itself, so it is what steps the power-up sequence that
	 * a wake-up event kicks off. It is shared, with a vote bit in both SLEEPCONs
	 * that the PMC ANDs: if both domains vote it down -- which each of them does
	 * when it is not using it as a clock source -- it really goes away, and then
	 * no wake-up event can bring either core back, whatever else was kept. So it
	 * is not a resource one domain keeps on behalf of the other; both need it to
	 * return at all, so both claim it and the aggregate holds. Only for a mode
	 * that returns: DPD and FDPD come back through a reset, which the PMC drives
	 * off its own always-on path.
	 */
	if (power_mode_returns(mode)) {
		power_request_add(&req, PWR_RES_LPOSC);
	}

	/*
	 * Resources the peer domain needs but cannot vote on. For everything the
	 * PMC arbitrates (RM 30.3.1 Table 257) the peer's own vote protects it and
	 * this domain must stay silent, or it defeats the peer's power-down; for the
	 * handful of fields only this domain's SLEEPCON has, there is no peer vote
	 * to defeat and no peer software that could act -- RM 12.5.1: "CPU1 software
	 * has no control over the power states of modules connected to VDD2 or
	 * VDDN." The mask is what keeps the two cases apart: a hook can only ever
	 * hold up a resource this domain is the sole voter for.
	 */
	if (dom->peer_needs != NULL) {
		req.res |= dom->peer_needs() & dom->res_vote_exclusive;
	}

	/* What the board's clock tree makes the request above imply -- see the field. */
	if (dom->res_clock_tree != NULL) {
		req.res |= dom->res_clock_tree(&req);
	}

	/*
	 * Everything above is what is actually used. Close it over the dependency
	 * graph so the votes handed to power_commit() are a legal combination.
	 */
	power_resolve(&req);

	return req;
}

/*
 * Mask interrupts for the low-power window the way arch_pm_state_set_prepare()
 * does, minus the CONFIG_PM-only context save. BASEPRI inhibits WFI from
 * observing the wake event, so PRIMASK takes over as the IRQ lock.
 */
static ALWAYS_INLINE void pm_mask_irqs_for_wfi(void)
{
	__disable_irq();
	__set_BASEPRI(0);
	__DSB();
	__ISB();
}

AT_QUICKACCESS_SECTION_CODE(void power_enter_common(const struct power_domain *dom,
						    const struct power_mode_desc *mode))
{
	const struct power_request req = power_keepalive_collect(dom, mode);

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	power_commit(dom, mode, &req);
	dom->regulator->program(dom, mode);

	/*
	 * A domain-owned clock being powered down has a power-down-ready that
	 * really arrives, so make SLEEPCON wait for that handshake. Only the
	 * Compute domain owns such clocks (FRO0/FRO1); Sense has none, so the hook
	 * is NULL there. Shared clocks (FRO2/LPOSC) go the other way and are
	 * handled by arm_shared_clock_pdr_ignores below.
	 */
	if (dom->prep_owned_clock_pdr != NULL) {
		dom->prep_owned_clock_pdr(&req);
	}

	if (dom->stall_dsp_if_powered_down != NULL) {
		dom->stall_dsp_if_powered_down();
	}

	pmc_apply_and_wait();

	pmc_clear_event_flags();
	uint32_t saved_ctrl = lvd_save_disable();

	/*
	 * Everything used from here on is cached in locals first. This function is
	 * RAM-resident, but struct power_domain and struct power_mode_desc are const
	 * data and so live in XIP flash: once xip_suspend() has torn the XSPI down,
	 * any dom->/mode-> read returns garbage. That includes xip_resume itself, the
	 * very pointer needed to bring the interface back, so reading it afterwards
	 * cannot work. A stale read of a function pointer branches to whatever the
	 * dead interface returns -- 0 in practice, which faults with INVSTATE and
	 * escalates to lockup. The XCACHEs can mask this when a mode keeps them alive
	 * and the lines happen to still be resident, so it must not be left to chance.
	 *
	 * req needs no such treatment: it is a local on the RAM stack, and
	 * power_requested() is inlined from the header rather than called through
	 * flash. SOC_PMC / SOC_SLEEPCON are compile-time constants, so code below the
	 * hand-over reaches those blocks without loading anything.
	 */
	const soc_clock_pdr_fn arm_shared_pdr_ignores = dom->arm_shared_clock_pdr_ignores;
	void (*xip_suspend)(bool, bool) = dom->xip_suspend;
	void (*xip_resume)(void) = dom->xip_resume;
	const bool returns = power_mode_returns(mode);

	/*
	 * Deep sleep / DSR return, so save arch state (and, on XIP, hand the XSPI
	 * over) around WFI. DPD/FDPD are one-way: there is no state to save, so
	 * they only mask interrupts.
	 *
	 * arch_pm_state_set_prepare()/_finish() exist only under CONFIG_PM -- both
	 * the weak fallback in arch/common/pm.c and the Cortex-M override in
	 * cortex_m/cpu_idle.c are CONFIG_PM-gated -- while this file also builds
	 * for POWEROFF-only configs. Testing `returns` at run time is not enough,
	 * the reference still has to resolve at link time, so the calls need a
	 * compile-time guard. A POWEROFF-only build only ever gets here through
	 * DPD/FDPD, which do not return anyway.
	 */
	unsigned int key = 0;

#if defined(CONFIG_PM)
	if (returns) {
		key = arch_pm_state_set_prepare();
	} else {
		pm_mask_irqs_for_wfi();
	}
#else
	pm_mask_irqs_for_wfi();
#endif

	if (xip_suspend != NULL) {
		/*
		 * A cache loses its contents unless both its array stays powered and
		 * the rail the cache logic sits in (VDD2_COMP) stays up -- so it needs
		 * a flush unless the request holds up both. DSR is exactly the case
		 * that separates them: it retains the cache arrays while collapsing
		 * VDD2_COMP, which is why the rail cannot be inferred from the array
		 * request and both have to be asked about.
		 *
		 * Mind the numbering: CPU0's system cache is the XCACHE0 block and
		 * its code cache is XCACHE1, so the SCACHE resource drives the xcache0
		 * argument and CCACHE drives xcache1.
		 */
		const bool comp_kept = power_requested(&req, PWR_RES_VDD2_COMP);
		bool flush_xcache0 =
			!comp_kept || !power_requested(&req, PWR_RES_MEM_CPU0_SCACHE);
		bool flush_xcache1 =
			!comp_kept || !power_requested(&req, PWR_RES_MEM_CPU0_CCACHE);

		xip_suspend(flush_xcache0, flush_xcache1);
	}

	arm_shared_pdr_ignores(&req);

	__WFI();

	if (!returns) {
		/* DPD/FDPD power the domain off; WFI never returns (cold boot). */
		CODE_UNREACHABLE;
		return;
	}

	if (xip_resume != NULL) {
		xip_resume();
	}

#if defined(CONFIG_PM)
	/* Only modes that return get here, and those all took the hooks above. */
	arch_pm_state_set_finish(key);
#else
	ARG_UNUSED(key);
#endif
	lvd_restore(saved_ctrl);

	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}
