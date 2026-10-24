/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "thead_csr.h"

void enable_branchpred(bool yes)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	if (yes) {
		tmp |= THEAD_MHCR_BPE | THEAD_MHCR_BTB;
	} else {
		tmp &= ~(THEAD_MHCR_BPE | THEAD_MHCR_BTB);
	}
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

void enable_thead_isa_ext(void)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, " THEAD_MXSTATUS
		: "=r"(tmp));
	tmp |= THEAD_MXSTATUS_THEADISAEE;
	__asm__ volatile(
		"csrw " THEAD_MXSTATUS ", %0"
		:
		: "r"(tmp));
}

void set_thead_enforce_aligned(bool enable)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, " THEAD_MXSTATUS
		: "=r"(tmp));
	if (enable) {
		tmp &= ~THEAD_MXSTATUS_MM;
	} else {
		tmp |= THEAD_MXSTATUS_MM;
	}
	__asm__ volatile(
		"csrw " THEAD_MXSTATUS ", %0"
		:
		: "r"(tmp));
}

void disable_interrupt_autostacking(void)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, " THEAD_MEXSTATUS
		: "=r"(tmp));
	tmp &= ~(THEAD_MEXSTATUS_SPUSH | THEAD_MEXSTATUS_SPSWAP);
	__asm__ volatile(
		"csrw " THEAD_MEXSTATUS ", %0"
		:
		: "r"(tmp));
}

void set_thead_mhint(bool dpld, enum thead_amr amr, enum thead_prefetch_n pn)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, " THEAD_MHINT
		: "=r"(tmp));
	if (dpld) {
		tmp |= THEAD_MHINT_DPLD;
	} else {
		tmp &= ~THEAD_MHINT_DPLD;
	}
	tmp &= ~THEAD_MHINT_AMR;
	tmp &= ~THEAD_MHINT_PREF_N;
	tmp |= FIELD_PREP(THEAD_MHINT_AMR, amr);
	tmp |= FIELD_PREP(THEAD_MHINT_PREF_N, pn);
	__asm__ volatile(
		"csrw " THEAD_MHINT ", %0"
		:
		: "r"(tmp));
}
