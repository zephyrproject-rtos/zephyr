/*
 * Copyright (C) 2025 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/symtab.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#define EXIDX_CANTUNWIND	0x1
#define EHABI_ENTRY_MASK	0xff000000
#define EHABI_ENTRY_SU16	0x80000000
#define EHABI_ENTRY_LU16	0x81000000

struct unwind_control_block {
	uint32_t vrs[16];
	const uint32_t *insn;
	int total;
	int byte;
};

struct unwind_index {
	uint32_t offset;
	uint32_t insn;
};

extern const struct unwind_index __exidx_start[];
extern const struct unwind_index __exidx_end[];

static ALWAYS_INLINE uint32_t prel31_to_addr(const uint32_t *prel31)
{
	int offset = (((int)(*prel31)) << 1) >> 1;

	return (uint32_t)prel31 + offset;
}

static const struct unwind_index *unwind_find_index(const struct unwind_index *start,
						    const struct unwind_index *end,
						    uint32_t pc)
{
	const struct unwind_index *mid;

	while (start < end - 1) {
		mid = start + ((end - start + 1) / 2);
		if (pc < prel31_to_addr(&mid->offset)) {
			end = mid;
		} else {
			start = mid;
		}
	}

	return start;
}

static uint8_t unwind_exec_get_next(struct unwind_control_block *ucb)
{
	uint8_t insn;

	insn = ((*ucb->insn) >> (ucb->byte * 8)) & 0xff;

	if (ucb->byte == 0) {
		++ucb->insn;
		ucb->byte = 3;
	} else {
		--ucb->byte;
	}
	--ucb->total;

	return insn;
}

typedef bool (*unwind_insn_fn)(struct unwind_control_block *ucb, uint8_t insn);
struct unwind_insn_entry {
	uint8_t insn_msk;
	uint8_t insn_val;
	unwind_insn_fn insn_fn;
};

/*
 * vsp = vsp + (xxxxxx << 2) + 4
 */
static bool insn_00xxxxxx(struct unwind_control_block *ucb, uint8_t insn)
{
	ucb->vrs[13] += ((insn & 0x3f) << 2) + 4;

	return true;
}

/*
 * vsp = vsp - (xxxxxx << 2) - 4
 */
static bool insn_01xxxxxx(struct unwind_control_block *ucb, uint8_t insn)
{
	ucb->vrs[13] -= ((insn & 0x3f) << 2) + 4;

	return true;
}

/*
 * 10000000_00000000: Refuse to unwind
 * 1000iiii_iiiiiiii(i not all 0): Pop {r15-r12}, {r11-r4}
 */
static bool insn_1000iiii_iiiiiiii(struct unwind_control_block *ucb, uint8_t insn)
{
	uint16_t mask = (insn & 0xf) << 8 | unwind_exec_get_next(ucb);
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];
	bool pop_r13 = mask & (1 << (13 - 4));
	uint8_t reg;

	/* refuse to unwind */
	if (mask == 0) {
		return false;
	}

	for (reg = 4; mask; mask >>= 1, ++reg) {
		if ((mask & 1)) {
			ucb->vrs[reg] = *vsp++;
		}
	}

	if (!pop_r13) {
		ucb->vrs[13] = (uint32_t)vsp;
	}

	return true;
}

/*
 * Reserved for arm register to register moves
 */
static bool insn_10011101(struct unwind_control_block *ucb, uint8_t insn)
{
	return true;
}

/*
 * Reserved for iwmmx register to register moves
 */
static bool insn_10011111(struct unwind_control_block *ucb, uint8_t insn)
{
	return true;
}

/*
 * vsp = r[nnnn]
 */
static bool insn_1001nnnn(struct unwind_control_block *ucb, uint8_t insn)
{
	ucb->vrs[13] = ucb->vrs[insn & 0xf];

	return true;
}

/*
 * Pop r4-r[4+nnn]
 */
static bool insn_10100nnn(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];
	uint8_t reg;

	for (reg = 4; reg <= (insn & 0x7) + 4; ++reg) {
		ucb->vrs[reg] = *vsp++;
	}

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Pop r4-r[4+nnn], r14
 */
static bool insn_10101nnn(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];
	uint8_t reg;

	for (reg = 4; reg <= (insn & 0x7) + 4; ++reg) {
		ucb->vrs[reg] = *vsp++;
	}
	ucb->vrs[14] = *vsp++;

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Finish
 */
static bool insn_10110000(struct unwind_control_block *ucb, uint8_t insn)
{
	ucb->total = 0;
	return true;
}

/*
 * Pop {r3, r2, r1, r0}
 */
static bool insn_10110001_0000iiii(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];
	uint8_t reg, mask = unwind_exec_get_next(ucb);

	for (reg = 0; mask; mask >>= 1, ++reg) {
		if ((mask & 1)) {
			ucb->vrs[reg] = *vsp++;
		}
	}

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * vsp = vsp + 0x204 + (uleb128 << 2)
 */
static bool insn_10110010_uleb128(struct unwind_control_block *ucb, uint8_t insn)
{
	ucb->vrs[13] += 0x204 + (unwind_exec_get_next(ucb) << 2);

	return true;
}

/*
 * Pop VFP D[ssss]-D[ssss+cccc] saved by FSTMFDX
 */
static bool insn_10110011_sssscccc(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];

	vsp += 2 * ((unwind_exec_get_next(ucb) & 0xf) + 1) + 1;

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Pop VFP D[8]-D[8+nnn] saved by FSTMFDX
 */
static bool insn_10111nnn(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];

	vsp += 2 * ((insn & 0x7) + 1) + 1;

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Pop VFP D[16+ssss]-D[16+ssss+cccc] saved by VPUSH
 */
static bool insn_11001000_sssscccc(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];

	vsp += 2 * ((unwind_exec_get_next(ucb) & 0xf) + 1);

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Pop VFP D[ssss]-D[ssss+cccc] saved by VPUSH
 */
static bool insn_11001001_sssscccc(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];

	vsp += 2 * ((unwind_exec_get_next(ucb) & 0xf) + 1);

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

/*
 * Pop VFP D[8]-D[8+nnn] saved by VPUSH
 */
static bool insn_11010nnn(struct unwind_control_block *ucb, uint8_t insn)
{
	uint32_t *vsp = (uint32_t *)ucb->vrs[13];

	vsp += 2 * ((insn & 0x7) + 1);

	ucb->vrs[13] = (uint32_t)vsp;

	return true;
}

static const struct unwind_insn_entry unwind_insns[] = {
	{
		.insn_msk = 0xc0,
		.insn_val = 0x00,
		.insn_fn = insn_00xxxxxx,
	},
	{
		.insn_msk = 0xc0,
		.insn_val = 0x40,
		.insn_fn = insn_01xxxxxx,
	},
	{
		.insn_msk = 0xf0,
		.insn_val = 0x80,
		.insn_fn = insn_1000iiii_iiiiiiii,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0x9d,
		.insn_fn = insn_10011101,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0x9f,
		.insn_fn = insn_10011111,
	},
	{
		.insn_msk = 0xf0,
		.insn_val = 0x90,
		.insn_fn = insn_1001nnnn,
	},
	{
		.insn_msk = 0xf8,
		.insn_val = 0xa0,
		.insn_fn = insn_10100nnn,
	},
	{
		.insn_msk = 0xf8,
		.insn_val = 0xa8,
		.insn_fn = insn_10101nnn,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xb0,
		.insn_fn = insn_10110000,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xb1,
		.insn_fn = insn_10110001_0000iiii,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xb2,
		.insn_fn = insn_10110010_uleb128,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xb3,
		.insn_fn = insn_10110011_sssscccc,
	},
	{
		.insn_msk = 0xf8,
		.insn_val = 0xb8,
		.insn_fn = insn_10111nnn,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xc8,
		.insn_fn = insn_11001000_sssscccc,
	},
	{
		.insn_msk = 0xff,
		.insn_val = 0xc9,
		.insn_fn = insn_11001001_sssscccc,
	},
	{
		.insn_msk = 0xf8,
		.insn_val = 0xd0,
		.insn_fn = insn_11010nnn,
	},
};

static bool unwind_exec_insn(struct unwind_control_block *ucb)
{
	int i;
	uint8_t insn = unwind_exec_get_next(ucb);

	for (i = 0; i < (sizeof(unwind_insns) / sizeof((unwind_insns)[0])); i++) {
		if ((insn & unwind_insns[i].insn_msk) == unwind_insns[i].insn_val) {
			if (!unwind_insns[i].insn_fn(ucb, insn)) {
				return false;
			}
			break;
		}
	}

	if (i == (sizeof(unwind_insns) / sizeof((unwind_insns)[0]))) {
		return false;
	}

	return true;
}

static bool unwind_one_frame(struct unwind_control_block *ucb)
{
	const struct unwind_index *index;
	const uint32_t *insn;

	index = unwind_find_index(__exidx_start, __exidx_end, ucb->vrs[15]);
	if (index->insn == EXIDX_CANTUNWIND) {
		return false;
	}

	if (index->insn & (1 << 31)) {
		insn = &index->insn;
	} else {
		insn = (uint32_t *)prel31_to_addr(&index->insn);
	}

	ucb->insn = insn;

	if ((*insn & EHABI_ENTRY_MASK) == EHABI_ENTRY_SU16) {
		ucb->total = 3;
		ucb->byte = 2;
	} else if ((*insn & EHABI_ENTRY_MASK) == EHABI_ENTRY_LU16) {
		ucb->total = 4 * ((*insn >> 16) & 0xff) + 2;
		ucb->byte = 1;
	} else {
		return false;
	}

	ucb->vrs[15] = 0;
	while (ucb->total) {
		if (!unwind_exec_insn(ucb)) {
			return false;
		}
	}

	/* if unwind_exec_insn() didn't update PC, load LR */
	if (ucb->vrs[15] == 0) {
		ucb->vrs[15] = ucb->vrs[14];
	}

	return true;
}

static void walk_stackframe(stack_trace_callback_fn cb, void *cookie,
			    const struct arch_esf *esf)
{
	struct unwind_control_block ucb = {};
	int i;

	if (esf == NULL || esf->extra_info.callee == NULL) {
		return;
	}

	ucb.vrs[7] = esf->extra_info.callee->v4;
	ucb.vrs[13] = esf->extra_info.callee->psp + sizeof(esf->basic);
	ucb.vrs[14] = esf->basic.lr;
	ucb.vrs[15] = esf->basic.pc;

	for (i = 0; i < CONFIG_ARCH_STACKWALK_MAX_FRAMES; i++) {
		if (!cb(cookie, ucb.vrs[15])) {
			break;
		}
		if (!unwind_one_frame(&ucb)) {
			break;
		}
	}
}

void arch_stack_walk(stack_trace_callback_fn callback_fn, void *cookie,
		     const struct k_thread *thread, const struct arch_esf *esf)
{
	ARG_UNUSED(thread);

	walk_stackframe(callback_fn, cookie, esf);
}

#ifdef CONFIG_EXCEPTION_STACK_TRACE
static bool print_trace_address(void *arg, unsigned long lr)
{
	int *i = arg;
#ifdef CONFIG_SYMTAB
	uint32_t offset = 0;
	const char *name = symtab_find_symbol_name(lr, &offset);

	EXCEPTION_DUMP("     %d: lr: 0x%08lx [%s+0x%x]",
			(*i)++, lr, name, offset);
#else
	EXCEPTION_DUMP("     %d: lr: 0x%08lx", (*i)++, lr);
#endif /* CONFIG_SYMTAB */

	return true;
}

void z_arm_unwind_stack(const struct arch_esf *esf)
{
	int i = 0;

	EXCEPTION_DUMP("call trace:");
	walk_stackframe(print_trace_address, &i, esf);
	EXCEPTION_DUMP("");
}
#endif /* CONFIG_EXCEPTION_STACK_TRACE */
