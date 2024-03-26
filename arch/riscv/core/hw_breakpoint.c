/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/debug/hw_breakpoint.h>
#include <zephyr/arch/riscv/csr.h>

#define MAX_BREAKPOINTS 16

static struct {
	hw_bp_callback_t callback;
	int reg_index;
	uintptr_t addr;
	void *data;
} breakpoints[MAX_BREAKPOINTS];

void z_riscv_hw_bp_handler(z_arch_esf_t *esf)
{
	int handle = -1;
	/* mtval holds the address that triggered the breakpoint. */
	uintptr_t addr = csr_read(mtval);

	/* mcontrol does have a hit bit that gets set, but it's optional. */
	for (int i = 0; i < ARRAY_SIZE(breakpoints); i++) {
		if (breakpoints[i].reg_index == -1) {
			/* Hit the end of available breakpoints. */
			break;
		}

		if (breakpoints[i].addr == addr) {
			handle = i;
			break;
		}
	}

	if (handle == -1) {
		return;
	}

	if (breakpoints[handle].callback) {
		breakpoints[handle].callback(handle, esf, breakpoints[handle].data);
	}

	/* Breakpoints fire before, so execution will get stuck. */
	/*
	 * TODO: See if we can work around this by temporarily pointing this BP
	 * to the next instruction.
	 */
	hw_bp_remove(handle);
}

void hw_bp_init(void)
{
	int handle = 0;

	for (int i = 0; i < ARRAY_SIZE(breakpoints); i++) {
		breakpoints[i].reg_index = -1;
	}

	for (int i = 0; i < MAX_BREAKPOINTS; i++) {
		csr_write(tselect, i);
		if (i != csr_read(tselect)) {
			/* trigger at this index doesn't exist. */
			break;
		}

		/* Currently only support address/data matches. */
		if (csr_read(tdata1) != MCONTROL_TYPE_MATCH) {
			continue;
		}

		breakpoints[handle].reg_index = i;
		handle++;
	}
}

int hw_bp_query(enum hw_bp_type_t type)
{
	int count = 0;

	switch (type) {
	case HW_BP_TYPE_INSTRUCTION:
	case HW_BP_TYPE_MEMORY:
	case HW_BP_TYPE_COMBINED:
		for (int i = 0; i < ARRAY_SIZE(breakpoints); i++) {
			if (breakpoints[i].reg_index == -1) {
				count = i;
				break;
			}
		}

		return count;

	default:
		return 0;
	}
}

int hw_bp_set(uintptr_t addr, enum hw_bp_type_t type, enum hw_bp_flags_t flags,
		hw_bp_callback_t cb, void *data)
{
	uintptr_t control = MCONTROL_M;
	int handle = -1;

	for (int i = 0; i < ARRAY_SIZE(breakpoints); i++) {
		if (!breakpoints[i].callback) {
			handle = i;
			break;
		}
	}

	if (handle == -1) {
		return -EAGAIN;
	}

	if (type == HW_BP_TYPE_INSTRUCTION || type == HW_BP_TYPE_COMBINED) {
		control |= MCONTROL_EXECUTE;
	}

	if (type == HW_BP_TYPE_MEMORY || type == HW_BP_TYPE_COMBINED) {
		if (flags & HW_BP_FLAGS_LOAD) {
			control |= MCONTROL_LOAD;
		}

		if (flags & HW_BP_FLAGS_STORE) {
			control |= MCONTROL_STORE;
		}
	}

	csr_write(tselect, breakpoints[handle].reg_index);
	csr_write(tdata2, addr);
	csr_write(tdata1, control);

	breakpoints[handle].callback = cb;
	breakpoints[handle].data = data;
	breakpoints[handle].addr = addr;

	return handle;
}

int hw_bp_remove(int handle)
{
	if (handle < 0 || handle >= MAX_BREAKPOINTS) {
		return -EINVAL;
	}

	csr_write(tselect, breakpoints[handle].reg_index);
	csr_write(tdata1, 0);
	csr_write(tdata2, 0);

	breakpoints[handle].callback = NULL;
	return 0;
}
