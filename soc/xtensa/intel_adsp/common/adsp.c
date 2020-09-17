/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 *         Rander Wang <rander.wang@intel.com>
 *         Janusz Jankowski <janusz.jankowski@linux.intel.com>
 */
#include <device.h>
#include <init.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sof);

#include <ipc.h>
#include <soc/shim.h>
#include <adsp/io.h>

#if defined(CONFIG_SOC_SERIES_INTEL_ADSP_BAYTRAIL)
#include <soc/mailbox.h>
#else
#include <cavs/mailbox.h>
#endif

/* TODO: Cleanup further */

#if CONFIG_DW_GPIO

#include <sof/drivers/gpio.h>

const struct gpio_pin_config gpio_data[] = {
	{	/* GPIO0 */
		.mux_id = 1,
		.mux_config = {.bit = 0, .mask = 3, .fn = 1},
	}, {	/* GPIO1 */
		.mux_id = 1,
		.mux_config = {.bit = 2, .mask = 3, .fn = 1},
	}, {	/* GPIO2 */
		.mux_id = 1,
		.mux_config = {.bit = 4, .mask = 3, .fn = 1},
	}, {	/* GPIO3 */
		.mux_id = 1,
		.mux_config = {.bit = 6, .mask = 3, .fn = 1},
	}, {	/* GPIO4 */
		.mux_id = 1,
		.mux_config = {.bit = 8, .mask = 3, .fn = 1},
	}, {	/* GPIO5 */
		.mux_id = 1,
		.mux_config = {.bit = 10, .mask = 3, .fn = 1},
	}, {	/* GPIO6 */
		.mux_id = 1,
		.mux_config = {.bit = 12, .mask = 3, .fn = 1},
	}, {	/* GPIO7 */
		.mux_id = 1,
		.mux_config = {.bit = 14, .mask = 3, .fn = 1},
	}, {	/* GPIO8 */
		.mux_id = 1,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO9 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO10 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO11 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO12 */
		.mux_id = 0,
		.mux_config = {.bit = 11, .mask = 1, .fn = 1},
	}, {	/* GPIO13 */
		.mux_id = 0,
		.mux_config = {.bit = 0, .mask = 1, .fn = 1},
	}, {	/* GPIO14 */
		.mux_id = 0,
		.mux_config = {.bit = 1, .mask = 1, .fn = 1},
	}, {	/* GPIO15 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO16 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO17 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO18 */
		.mux_id = 0,
		.mux_config = {.bit = 9, .mask = 1, .fn = 1},
	}, {	/* GPIO19 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO20 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO21 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO22 */
		.mux_id = 0,
		.mux_config = {.bit = 10, .mask = 1, .fn = 1},
	}, {	/* GPIO23 */
		.mux_id = 0,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO24 */
		.mux_id = 0,
		.mux_config = {.bit = 16, .mask = 1, .fn = 1},
	}, {	/* GPIO25 */
		.mux_id = 0,
		.mux_config = {.bit = 26, .mask = 1, .fn = 1},
	},
};

const int n_gpios = ARRAY_SIZE(gpio_data);

#if CONFIG_INTEL_IOMUX

#include <sof/drivers/iomux.h>

struct iomux iomux_data[] = {
	{.base = EXT_CTRL_BASE + 0x30,},
	{.base = EXT_CTRL_BASE + 0x34,},
	{.base = EXT_CTRL_BASE + 0x38,},
};

const int n_iomux = ARRAY_SIZE(iomux_data);

#endif

#endif

#define SRAM_WINDOW_HOST_OFFSET(x) (0x80000 + x * 0x20000)

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

#if !defined(CONFIG_SOF)
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

#if defined(CONFIG_SOC_SERIES_INTEL_CAVS_V15)
	sys_write32(SRAM_WINDOW_HOST_OFFSET(0) >> 12,
		    IPC_HOST_BASE + IPC_DIPCIE);
	sys_write32(0x80000000 | ADSP_IPC_FW_READY,
		    IPC_HOST_BASE + IPC_DIPCI);
#elif defined(CONFIG_SOC_SERIES_INTEL_ADSP_BAYTRAIL)
	shim_write(SHIM_IPCDL, SOF_IPC_FW_READY | MAILBOX_HOST_OFFSET >> 3);
	shim_write(SHIM_IPCDH, SHIM_IPCDH_BUSY);
#else
	sys_write32(SRAM_WINDOW_HOST_OFFSET(0) >> 12,
		    IPC_HOST_BASE + IPC_DIPCIDD);
	sys_write32(0x80000000 | ADSP_IPC_FW_READY,
		    IPC_HOST_BASE + IPC_DIPCIDR);
#endif
}

static int adsp_init(const struct device *dev)
{
	prepare_host_windows();

	send_fw_ready();

	return 0;
}

/* Init after IPM initialization and before logging (uses memory windows) */
SYS_INIT(adsp_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif /* ! CONFIG_SOF */
