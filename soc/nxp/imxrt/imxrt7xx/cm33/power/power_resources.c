/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Resource dependency closure for the RT7xx CM33 low-power driver.
 *
 * The hardware has rules about which blocks may be powered down together, and
 * RM 31.2 Table 329 is blunt about who enforces them: "Software must maintain
 * the power domain state relationships shown in the table above. Hardware does
 * not prevent illegal power switch configurations."
 *
 * The table is written in the power-down direction ("VDD2_COM can be off only
 * when VDD2_COMP, VDD2_DSP, VDD2_MEDIA, VDDN_MEDIA and VDDN_COM are all off").
 * A keep-alive request is the contrapositive: if any of those is kept on, then
 * VDD2_COM must be kept on too. Every rule then becomes a single edge "keeping X
 * alive requires keeping Y alive", and closing the request set over those edges
 * makes an illegal combination unrepresentable.
 *
 * These are relations between resources, not between bits: no register knowledge
 * here.
 */

#include "power_resources.h"

/*
 * res_deps[X] = what must also be kept alive when X is kept alive. Only the direct
 * parent is listed; the closure below reaches the transitive cases (keeping the
 * media rails alive reaches VDD2_COM through VDDN_COM without that edge being
 * written out).
 *
 * The FROs, XTAL and LPOSC deliberately get no PMCREF edge: whether they need an
 * accurate reference is unconfirmed, and the edge would pin PMCREF high on every
 * dual-core window, which always keeps FRO2 and LPOSC alive for the Sense core.
 * That is a power decision to make on measurement, not a refactor.
 *
 * One edge that could be written here and is deliberately not: the CPU0 caches are
 * not made to depend on VDD2_COMP. Their arrays have their own retention supply,
 * which is exactly what DSR uses -- the arrays retain while the compute logic
 * collapses. What the collapse does cost is the cache *controller* state, so the
 * XIP path flushes on a collapse regardless of the array request; see
 * power_enter_common().
 *
 * And one that cannot be written here at all: the main SRAM partitions are not
 * resources, so nothing relates them to the RAM Arbiter clocks. That is no loss,
 * because a sleeping core is not accessing them. RM 31.6.6's required dummy read
 * before the arbiter clock stops applies when the *other* core will access the
 * partition during the window -- a separate concern from retention, and not
 * modelled anywhere in this driver.
 */
static const power_res_mask_t res_deps[PWR_RES_COUNT] = {
	/* "VDD2_DSP depends on VDD2_COMP." */
	[PWR_RES_VDD2_DSP] = PWR_RES(PWR_RES_VDD2_COMP),
	/* VDD2_COMP off is only legal with VDD2_COM off. */
	[PWR_RES_VDD2_COMP] = PWR_RES(PWR_RES_VDD2_COM),
	/* "VDDN_COM can be off only when both VDD2_MEDIA and VDDN_MEDIA are
	 * off", and VDD2_COM only once VDDN_COM is off as well.
	 */
	[PWR_RES_MEDIA] = PWR_RES(PWR_RES_VDDN_COM),
	[PWR_RES_VDDN_COM] = PWR_RES(PWR_RES_VDD2_COM),
	/* The MIPI PHY sits in the media domain (RM 31.2.1 Table 327). */
	[PWR_RES_VDD2_MIPI] = PWR_RES(PWR_RES_MEDIA),

	/* An accurate PMC reference is required by the PLL LDOs ... */
	[PWR_RES_MAIN_PLL] = PWR_RES(PWR_RES_PMC_REF),
	[PWR_RES_AUDIO_PLL] = PWR_RES(PWR_RES_PMC_REF),
	/* ... and by any brown-out / power-on-reset detector left watching. */
	[PWR_RES_DETECTOR_VDD1] = PWR_RES(PWR_RES_PMC_REF),
	[PWR_RES_DETECTOR_VDD2] = PWR_RES(PWR_RES_PMC_REF),
	[PWR_RES_DETECTOR_VDDN] = PWR_RES(PWR_RES_PMC_REF),
	[PWR_RES_DETECTOR_VDD1V8] = PWR_RES(PWR_RES_PMC_REF),
	/* Same for the analog glitch detectors and the temperature sensor. The
	 * other PDSLEEPCFG1 residents -- ROM, OTP, the SRAM sleep request -- are
	 * digital and get no edge.
	 */
	[PWR_RES_AGDET] = PWR_RES(PWR_RES_PMC_REF),
	[PWR_RES_TEMP_SENSOR] = PWR_RES(PWR_RES_PMC_REF),
};

void power_resolve(struct power_request *req)
{
	power_res_mask_t prev;

	/*
	 * Fixed-point iteration rather than a topological walk: the graph is a
	 * shallow forest (the longest chain is MIPI -> media -> VDDN_COM ->
	 * VDD2_COM), so this converges in a handful of passes and needs no ordering
	 * assumption about the enum. Runs once per low-power entry, before any
	 * register access, with flash still live.
	 */
	do {
		prev = req->res;

		for (unsigned int r = 0U; r < PWR_RES_COUNT; r++) {
			if ((req->res & PWR_RES(r)) != 0U) {
				req->res |= res_deps[r];
			}
		}
	} while (req->res != prev);
}
