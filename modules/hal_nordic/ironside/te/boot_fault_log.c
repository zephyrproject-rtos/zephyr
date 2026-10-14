/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ironside_client/services/boot_report.h>

LOG_MODULE_DECLARE(ironside_te, CONFIG_IRONSIDE_TE_LOG_LEVEL);

static const char *exception_str(const struct ironside_fault_report *fault)
{
	if (fault->source == IRONSIDE_FAULT_SOURCE_MPC_MEMACCERR) {
		return "MemAccErr";
	}

	switch (fault->exception) {
	case IRONSIDE_FAULT_EXC_HARD_FAULT:
		return "HardFault";
	case IRONSIDE_FAULT_EXC_MEM_MANAGE:
		return "MemManage";
	case IRONSIDE_FAULT_EXC_BUS_FAULT:
		return "BusFault";
	case IRONSIDE_FAULT_EXC_USAGE_FAULT:
		return "UsageFault";
	case IRONSIDE_FAULT_EXC_SECURE_FAULT:
		return "SecureFault";
	default:
		return "";
	}
}

static void log_fault(const struct ironside_fault_report *fault)
{
	LOG_ERR("Device reset because of a %s (fault #%u), flags 0x%x",
		exception_str(fault), (unsigned int)fault->count, (unsigned int)fault->flags);

	if (fault->source == IRONSIDE_FAULT_SOURCE_MPC_MEMACCERR) {
		LOG_ERR("address 0x%08x, access %c%c%c", (unsigned int)fault->mpc_address,
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_READ) ? 'r' : '-',
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_WRITE) ? 'w' : '-',
			(fault->mpc_access & IRONSIDE_FAULT_MPC_ACCESS_EXECUTE) ? 'x' : '-');
		return;
	}

	LOG_ERR("  cfsr    0x%08x mmfar    0x%08x bfar 0x%08x sfsr 0x%08x",
		(unsigned int)fault->cfsr, (unsigned int)fault->mmfar,
		(unsigned int)fault->bfar, (unsigned int)fault->sfsr);
	LOG_ERR("  cfsr_ns 0x%08x mmfar_ns 0x%08x sfar 0x%08x", (unsigned int)fault->cfsr_ns,
		(unsigned int)fault->mmfar_ns, (unsigned int)fault->sfar);

	if (fault->flags & IRONSIDE_FAULT_FLAG_FRAME_UNAVAILABLE) {
		LOG_ERR("  frame unavailable");
		return;
	}

	LOG_ERR("  r0 0x%08x r1 0x%08x r2 0x%08x r3   0x%08x r12 0x%08x",
		(unsigned int)fault->frame[0], (unsigned int)fault->frame[1],
		(unsigned int)fault->frame[2], (unsigned int)fault->frame[3],
		(unsigned int)fault->frame[4]);
	LOG_ERR("  sp 0x%08x lr 0x%08x pc 0x%08x xpsr 0x%08x",
		(unsigned int)fault->frame_sp, (unsigned int)fault->frame[5],
		(unsigned int)fault->frame[6], (unsigned int)fault->frame[7]);
}

static int boot_fault_log(void)
{
	struct ironside_boot_report report;
	size_t len;
	int32_t err;

	err = ironside_boot_report_get(&report, sizeof(report), &len);
	if (err != 0 || len < sizeof(report)) {
		LOG_WRN("ironside_boot_report_get() failed: %d",
			(int)err);
		return 0;
	}

	if (report.fault.present) {
		log_fault(&report.fault);
	}

	return 0;
}
SYS_INIT(boot_fault_log, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
