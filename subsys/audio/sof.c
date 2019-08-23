/* sof.c - Sound Open Firmware handling */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <device.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sof, CONFIG_SOF_LOG_LEVEL);

#include <platform/dai.h>
#include <platform/dma.h>
#include <platform/shim.h>
#include <sof/clk.h>
#include <sof/mailbox.h>
#include <sof/notifier.h>
#include <sof/timer.h>
#include <ipc/info.h>

#include "sof/sof.h"
#include "sof/ipc.h"

/* SRAM window 0 FW "registers" */
#define SRAM_REG_ROM_STATUS                     0x0
#define SRAM_REG_FW_STATUS                      0x4
#define SRAM_REG_FW_TRACEP                      0x8
#define SRAM_REG_FW_IPC_RECEIVED_COUNT          0xc
#define SRAM_REG_FW_IPC_PROCESSED_COUNT         0x10
#define SRAM_REG_FW_END                         0x14

/* Clock control */
#define SHIM_CLKCTL				0x78

/* Power control and status */
#define SHIM_PWRCTL				0x90
#define SHIM_PWRSTS				0x92
#define SHIM_LPSCTL				0x94

#define SHIM_CLKCTL_HDCS_PLL			0
#define SHIM_CLKCTL_LDCS_PLL			0
#define SHIM_CLKCTL_DPCS_DIV1(x)		(0x0 << (8 + x * 2))
#define SHIM_CLKCTL_HPMPCS_DIV2			0
#define SHIM_CLKCTL_LPMPCS_DIV4			BIT(1)
#define SHIM_CLKCTL_TCPAPLLS_DIS		0
#define SHIM_CLKCTL_TCPLCG_DIS(x)		0

/* host windows */
#define DMWBA(x)			(HOST_WIN_BASE(x) + 0x0)
#define DMWLO(x)			(HOST_WIN_BASE(x) + 0x4)
#define DMWBA_ENABLE			(1 << 0)
#define DMWBA_READONLY			(1 << 1)

#define SOF_GLB_TYPE_SHIFT			28
#define SOF_GLB_TYPE(x)				((x) << SOF_GLB_TYPE_SHIFT)
#define SOF_IPC_FW_READY			SOF_GLB_TYPE(0x7U)

static struct sof sof;

#define CASE(x) case TRACE_CLASS_##x: return #x

struct timesource_data platform_generic_queue[] = {
{
	.timer = {
		.id = TIMER3, /* external timer */
		.irq = IRQ_EXT_TSTAMP0_LVL2(0),
	},
	.clk		= CLK_SSP,
	.notifier	= NOTIFIER_ID_SSP_FREQ,
	.timer_set	= platform_timer_set,
	.timer_clear	= platform_timer_clear,
	.timer_get	= platform_timer_get,
},
{
	.timer = {
		.id = TIMER3, /* external timer */
		.irq = IRQ_EXT_TSTAMP0_LVL2(1),
	},
	.clk		= CLK_SSP,
	.notifier	= NOTIFIER_ID_SSP_FREQ,
	.timer_set	= platform_timer_set,
	.timer_clear	= platform_timer_clear,
	.timer_get	= platform_timer_get,
},
};

struct timer *platform_timer =
	&platform_generic_queue[PLATFORM_MASTER_CORE_ID].timer;

static const struct sof_ipc_fw_ready fw_ready_apl
	__attribute__((section(".fw_ready"))) __attribute__((used)) = {
	.hdr = {
		.cmd = SOF_IPC_FW_READY,
		.size = sizeof(struct sof_ipc_fw_ready),
	},
	.version = {
		.hdr.size = sizeof(struct sof_ipc_fw_version),
		.micro = 0,
		.minor = 1,
		.major = 1,
#ifdef CONFIG_DEBUG
		/* only added in debug for reproducability in releases */
		.build = 8,
		.date = __DATE__,
		.time = __TIME__,
#endif
		.tag = "9e2aa",
		.abi_version = (3 << 24) | (9 << 12) | (0 << 0),
	},
	.flags = 0,
};

#define NUM_WINDOWS		7

static const struct sof_ipc_window sram_window = {
	.ext_hdr = {
		.hdr.cmd = SOF_IPC_FW_READY,
		.hdr.size = sizeof(struct sof_ipc_window) +
			    sizeof(struct sof_ipc_window_elem) * NUM_WINDOWS,
		.type = SOF_IPC_EXT_WINDOW,
	},
	.num_windows = NUM_WINDOWS,
	.window = {
		{
			.type   = SOF_IPC_REGION_REGS,
			.id     = 0,    /* map to host window 0 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_SW_REG_SIZE,
			.offset = 0,
		},
		{
			.type   = SOF_IPC_REGION_UPBOX,
			.id     = 0,    /* map to host window 0 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_DSPBOX_SIZE,
			.offset = MAILBOX_SW_REG_SIZE,
		},
		{
			.type   = SOF_IPC_REGION_DOWNBOX,
			.id     = 1,    /* map to host window 1 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_HOSTBOX_SIZE,
			.offset = 0,
		},
		{
			.type   = SOF_IPC_REGION_DEBUG,
			.id     = 2,    /* map to host window 2 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_EXCEPTION_SIZE + MAILBOX_DEBUG_SIZE,
			.offset = 0,
		},
		{
			.type   = SOF_IPC_REGION_EXCEPTION,
			.id     = 2,    /* map to host window 2 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_EXCEPTION_SIZE,
			.offset = MAILBOX_EXCEPTION_OFFSET,
		},
		{
			.type   = SOF_IPC_REGION_STREAM,
			.id     = 2,    /* map to host window 2 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_STREAM_SIZE,
			.offset = MAILBOX_STREAM_OFFSET,
		},
		{
			.type   = SOF_IPC_REGION_TRACE,
			.id     = 3,    /* map to host window 3 */
			.flags  = 0, // TODO: set later
			.size   = MAILBOX_TRACE_SIZE,
			.offset = 0,
		},
	},
};

/* look up subsystem class name from table */
char *get_trace_class(uint32_t trace_class)
{
	switch (trace_class) {
		CASE(IRQ);
		CASE(IPC);
		CASE(PIPE);
		CASE(HOST);
		CASE(DAI);
		CASE(DMA);
		CASE(SSP);
		CASE(COMP);
		CASE(WAIT);
		CASE(LOCK);
		CASE(MEM);
		CASE(MIXER);
		CASE(BUFFER);
		CASE(VOLUME);
		CASE(SWITCH);
		CASE(MUX);
		CASE(SRC);
		CASE(TONE);
		CASE(EQ_FIR);
		CASE(EQ_IIR);
		CASE(SA);
		CASE(DMIC);
		CASE(POWER);
	default: return "unknown";
	}
}

/* print debug messages */
void debug_print(char *message)
{
	LOG_DBG("%s", message);
}

/* FIXME: The following definitions are to satisfy linker errors */
struct dai dai;

/* Make use of dai_install to register DAI drivers, there maybe common code
 * from i2s_cavs.c and codec.h */
struct dai *dai_get(uint32_t type, uint32_t index, uint32_t flags)
{
	return &dai;
}

void dai_put(struct dai *dai)
{
}

int dai_init(void)
{
	return 0;
}

struct dma dma;

struct dma *dma_get(uint32_t dir, uint32_t caps, uint32_t dev, uint32_t flags)
{
	return &dma;
}

void dma_put(struct dma *dma)
{
}

int dma_sg_alloc(struct dma_sg_elem_array *elem_array,
		 int zone,
		 uint32_t direction,
		 uint32_t buffer_count, uint32_t buffer_bytes,
		 uintptr_t dma_buffer_addr, uintptr_t external_addr)
{
	return 0;
}

void dma_sg_free(struct dma_sg_elem_array *elem_array)
{
}

static void prepare_host_windows()
{
	/* window0, for fw status & outbox/uplink mbox */
	sys_write32((HP_SRAM_WIN0_SIZE | 0x7), DMWLO(0));
	sys_write32((HP_SRAM_WIN0_BASE | DMWBA_READONLY | DMWBA_ENABLE),
		    DMWBA(0));
	memset((void *)(HP_SRAM_WIN0_BASE + SRAM_REG_FW_END), 0,
	      HP_SRAM_WIN0_SIZE - SRAM_REG_FW_END);
	SOC_DCACHE_FLUSH((void *)(HP_SRAM_WIN0_BASE + SRAM_REG_FW_END),
			 HP_SRAM_WIN0_SIZE - SRAM_REG_FW_END);

	/* window1, for inbox/downlink mbox */
	sys_write32((HP_SRAM_WIN1_SIZE | 0x7), DMWLO(1));
	sys_write32((HP_SRAM_WIN1_BASE | DMWBA_ENABLE), DMWBA(1));
	memset((void *)HP_SRAM_WIN1_BASE, 0, HP_SRAM_WIN1_SIZE);
	SOC_DCACHE_FLUSH((void *)HP_SRAM_WIN1_BASE, HP_SRAM_WIN1_SIZE);

	/* window2, for debug */
	sys_write32((HP_SRAM_WIN2_SIZE | 0x7), DMWLO(2));
	sys_write32((HP_SRAM_WIN2_BASE | DMWBA_ENABLE), DMWBA(2));
	memset((void *)HP_SRAM_WIN2_BASE, 0, HP_SRAM_WIN2_SIZE);
	SOC_DCACHE_FLUSH((void *)HP_SRAM_WIN2_BASE, HP_SRAM_WIN2_SIZE);

	/* window3, for trace
	 * zeroed by trace initialization
	 */
	sys_write32((HP_SRAM_WIN3_SIZE | 0x7), DMWLO(3));
	sys_write32((HP_SRAM_WIN3_BASE | DMWBA_READONLY | DMWBA_ENABLE),
		    DMWBA(3));
}

static void sys_module_init(void)
{
	LOG_INF("");

	Z_STRUCT_SECTION_FOREACH(_sof_module, mod) {
		mod->init();
		LOG_INF("module %p init", mod);
	}
}

static void hw_init()
{
	/* Setup clocks and power management */
	shim_write(SHIM_CLKCTL,
		   SHIM_CLKCTL_HDCS_PLL | /* HP domain clocked by PLL */
		   SHIM_CLKCTL_LDCS_PLL | /* LP domain clocked by PLL */
		   SHIM_CLKCTL_DPCS_DIV1(0) | /* Core 0 clk not divided */
		   SHIM_CLKCTL_DPCS_DIV1(1) | /* Core 1 clk not divided */
		   SHIM_CLKCTL_HPMPCS_DIV2 | /* HP mem clock div by 2 */
		   SHIM_CLKCTL_LPMPCS_DIV4 | /* LP mem clock div by 4 */
		   SHIM_CLKCTL_TCPAPLLS_DIS |
		   SHIM_CLKCTL_TCPLCG_DIS(0) | SHIM_CLKCTL_TCPLCG_DIS(1));

	shim_write(SHIM_LPSCTL, shim_read(SHIM_LPSCTL));
}

static int sof_boot_complete()
{
	mailbox_dspbox_write(0, &fw_ready_apl, sizeof(fw_ready_apl));
        mailbox_dspbox_write(sizeof(fw_ready_apl), &sram_window,
			     sram_window.ext_hdr.hdr.size);

	ipc_write(IPC_DIPCIE, 0);
	ipc_write(IPC_DIPCI, (0x80000000 | SOF_IPC_FW_READY));

	return 0;
}

/* This is analog of platform_init() */
static int sof_init(struct device *unused)
{
	int ret;

	/* prepare host windows */
	prepare_host_windows();

	hw_init();

	/* init components */
	sys_comp_init();

	/* init static modules */
	sys_module_init();

	/* init clocks in SOF */
	clock_init();

	/* init DMAC */
	ret = dmac_init();
	if (ret < 0) {
		return ret;
	}

	/* init IPC */
	ret = ipc_init(&sof);
	if (ret < 0) {
		LOG_ERR("IPC init: %d", ret);
		return ret;
	}

	LOG_INF("IPC initialized");

	/* init DAIs */
	ret = dai_init();
	if (ret < 0) {
		LOG_ERR("DAI initialization failed");
		return ret;
	}

	/* init scheduler */
	ret = scheduler_init();
	if (ret < 0) {
		LOG_ERR("scheduler init: %d", ret);
		return ret;
	}

	LOG_INF("scheduler initialized");

#if defined(CONFIG_SOF_STATIC_PIPELINE)
	/* init static pipeline */
	ret = init_static_pipeline(sof.ipc);
	if (ret < 0) {
		LOG_ERR("static pipeline init: %d", ret);
		return ret;
	}

	LOG_INF("pipeline initialized");
#endif /* CONFIG_SOF_STATIC_PIPELINE */

	mailbox_sw_reg_write(SRAM_REG_ROM_STATUS, 0xabbac0fe);

	sof_boot_complete();

	LOG_INF("FW Boot Completed");

	return 0;
}

SYS_INIT(sof_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
