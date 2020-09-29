/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>

#include <platform/ipc.h>
#include <platform/mailbox.h>
#include <platform/shim.h>

#include "soc.h"

static const struct adsp_ipc_fw_ready fw_ready_apl
	__attribute__((section(".fw_ready"))) __attribute__((used)) = {
	.hdr = {
		.cmd = ADSP_IPC_FW_READY,
		.size = sizeof(struct adsp_ipc_fw_ready),
	},
	.version = {
		.hdr.size = sizeof(struct adsp_ipc_fw_version),
		.micro = 0,
		.minor = 1,
		.major = 0,

		.build = 0,
		.date = __DATE__,
		.time = __TIME__,

		.tag = "zephyr",
		.abi_version = 0,
	},
	.flags = 0,
};

#define NUM_WINDOWS			2

static const struct adsp_ipc_window sram_window = {
	.ext_hdr = {
		.hdr.cmd = ADSP_IPC_FW_READY,
		.hdr.size = sizeof(struct adsp_ipc_window) +
			    sizeof(struct adsp_ipc_window_elem) * NUM_WINDOWS,
		.type = ADSP_IPC_EXT_WINDOW,
	},
	.num_windows = NUM_WINDOWS,
	.window = {
		{
			.type   = ADSP_IPC_REGION_REGS,
			.id     = 0,	/* map to host window 0 */
			.flags  = 0,
			.size   = MAILBOX_SW_REG_SIZE,
			.offset = 0,
		},
		{
			.type   = ADSP_IPC_REGION_TRACE,
			.id     = 3,	/* map to host window 3 */
			.flags  = 0,
			.size   = MAILBOX_TRACE_SIZE,
			.offset = 0,
		},
	},
};

/*
 * Sets up the host windows so that the host can see the memory
 * content on the DSP SRAM.
 */
static void prepare_host_windows(void)
{
	/* window0, for fw status */
	sys_write32((HP_SRAM_WIN0_SIZE | 0x7), DMWLO(0));
	sys_write32((HP_SRAM_WIN0_BASE | DMWBA_READONLY | DMWBA_ENABLE),
		    DMWBA(0));
	memset((void *)(HP_SRAM_WIN0_BASE + SRAM_REG_FW_END), 0,
	      HP_SRAM_WIN0_SIZE - SRAM_REG_FW_END);
	SOC_DCACHE_FLUSH((void *)(HP_SRAM_WIN0_BASE + SRAM_REG_FW_END),
			 HP_SRAM_WIN0_SIZE - SRAM_REG_FW_END);

	if (IS_ENABLED(CONFIG_IPM_INTEL_ADSP)) {
		/* window1, for inbox/downlink mbox */
		sys_write32((HP_SRAM_WIN1_SIZE | 0x7), DMWLO(1));
		sys_write32((HP_SRAM_WIN1_BASE | DMWBA_ENABLE), DMWBA(1));
		memset((void *)HP_SRAM_WIN1_BASE, 0, HP_SRAM_WIN1_SIZE);
		SOC_DCACHE_FLUSH((void *)HP_SRAM_WIN1_BASE, HP_SRAM_WIN1_SIZE);
	}

	/* window3, for trace
	 * zeroed by trace initialization
	 */
	sys_write32((HP_SRAM_WIN3_SIZE | 0x7), DMWLO(3));
	sys_write32((HP_SRAM_WIN3_BASE | DMWBA_READONLY | DMWBA_ENABLE),
		    DMWBA(3));
	memset((void *)HP_SRAM_WIN3_BASE, 0, HP_SRAM_WIN3_SIZE);
	SOC_DCACHE_FLUSH((void *)HP_SRAM_WIN3_BASE, HP_SRAM_WIN3_SIZE);
}

/*
 * Sends the firmware ready message so the firmware loader can
 * map the host windows.
 */
static void send_fw_ready(void)
{
	memcpy((void *)MAILBOX_DSPBOX_BASE,
	       &fw_ready_apl, sizeof(fw_ready_apl));

	memcpy((void *)(MAILBOX_DSPBOX_BASE + sizeof(fw_ready_apl)),
	       &sram_window, sizeof(sram_window));

	SOC_DCACHE_FLUSH((void *)MAILBOX_DSPBOX_BASE, MAILBOX_DSPBOX_SIZE);

	ipc_write(IPC_DIPCIE, 0);
	ipc_write(IPC_DIPCI, (0x80000000 | ADSP_IPC_FW_READY));
}

static int adsp_init(const struct device *dev)
{
	prepare_host_windows();

	send_fw_ready();

	return 0;
}

/* Init after IPM initialization and before logging (uses memory windows) */
SYS_INIT(adsp_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
