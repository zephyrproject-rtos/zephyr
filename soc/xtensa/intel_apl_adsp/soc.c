/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <xtensa_api.h>
#include <xtensa/xtruntime.h>
#include <irq_nextlevel.h>
#include <xtensa/hal.h>
#include <init.h>

#include "soc.h"

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

static u32_t ref_clk_freq;

void z_soc_irq_enable(u32_t irq)
{
	struct device *dev_cavs, *dev_ictl;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case DT_CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case DT_CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case DT_CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		z_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		LOG_DBG("board: CAVS device binding failed");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in either CAVS interrupt logic or DW interrupt controller
	 */
	z_xtensa_irq_enable(XTENSA_IRQ_NUMBER(irq));

	switch (CAVS_IRQ_NUMBER(irq)) {
	default:
		/* The source of the interrupt is in CAVS interrupt logic */
		irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_ictl) {
		LOG_DBG("board: DW intr_control device binding failed");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in DW interrupt controller
	 */
	irq_enable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

	/* Manipulate the relevant bit in the interrupt controller
	 * register as needed
	 */
	irq_enable_next_level(dev_ictl, INTR_CNTL_IRQ_NUM(irq));
}

void z_soc_irq_disable(u32_t irq)
{
	struct device *dev_cavs, *dev_ictl;

	switch (XTENSA_IRQ_NUMBER(irq)) {
	case DT_CAVS_ICTL_0_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_0_NAME);
		break;
	case DT_CAVS_ICTL_1_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_1_NAME);
		break;
	case DT_CAVS_ICTL_2_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_2_NAME);
		break;
	case DT_CAVS_ICTL_3_IRQ:
		dev_cavs = device_get_binding(CONFIG_CAVS_ICTL_3_NAME);
		break;
	default:
		/* regular interrupt */
		z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		return;
	}

	if (!dev_cavs) {
		LOG_DBG("board: CAVS device binding failed");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in either CAVS interrupt logic or DW interrupt controller
	 */

	switch (CAVS_IRQ_NUMBER(irq)) {
	default:
		/* The source of the interrupt is in CAVS interrupt logic */
		irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

		/* Disable the parent IRQ if all children are disabled */
		if (!irq_is_enabled_next_level(dev_cavs)) {
			z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		}
		return;
	}

	if (!dev_ictl) {
		LOG_DBG("board: DW intr_control device binding failed");
		return;
	}

	/* If the control comes here it means the specified interrupt
	 * is in DW interrupt controller.
	 * Manipulate the relevant bit in the interrupt controller
	 * register as needed
	 */
	irq_disable_next_level(dev_ictl, INTR_CNTL_IRQ_NUM(irq));

	/* Disable the parent IRQ if all children are disabled */
	if (!irq_is_enabled_next_level(dev_ictl)) {
		irq_disable_next_level(dev_cavs, CAVS_IRQ_NUMBER(irq));

		if (!irq_is_enabled_next_level(dev_cavs)) {
			z_xtensa_irq_disable(XTENSA_IRQ_NUMBER(irq));
		}
	}
}

static inline void soc_set_resource_ownership(void)
{
	volatile struct soc_resource_alloc_regs *regs =
		(volatile struct soc_resource_alloc_regs *)
		SOC_RESOURCE_ALLOC_REG_BASE;
	int index;


	/* set ownership of DMA controllers and channels */
	for (index = 0; index < SOC_NUM_LPGPDMAC; index++) {
		regs->lpgpdmacxo[index] = SOC_LPGPDMAC_OWNER_DSP;
	}

	/* set ownership of I2S and DMIC controllers */
	regs->dspiopo = SOC_DSPIOP_I2S_OWNSEL_DSP |
		SOC_DSPIOP_DMIC_OWNSEL_DSP;

	/* set ownership of timestamp and M/N dividers */
	regs->geno = SOC_GENO_TIMESTAMP_OWNER_DSP |
		SOC_GENO_MNDIV_OWNER_DSP;
}

u32_t soc_get_ref_clk_freq(void)
{
	return ref_clk_freq;
}

static inline void soc_set_audio_mclk(void)
{
#if (CONFIG_AUDIO)
	int mclk;
	volatile struct soc_mclk_control_regs *mclk_regs =
		(volatile struct soc_mclk_control_regs *)SOC_MCLK_DIV_CTRL_BASE;

	for (mclk = 0; mclk < SOC_NUM_MCLK_OUTPUTS; mclk++) {
		/*
		 * set divider to bypass mode which makes MCLK output frequency
		 * to be the same as referece clock frequency
		 */
		mclk_regs->mdivxr[mclk] = SOC_MDIVXR_SET_DIVIDER_BYPASS;
		mclk_regs->mdivctrl |= SOC_MDIVCTRL_MCLK_OUT_EN(mclk);
	}
#endif
}

static inline void soc_set_dmic_power(void)
{
#if (CONFIG_AUDIO_INTEL_DMIC)
	volatile struct soc_dmic_shim_regs *dmic_shim_regs =
		(volatile struct soc_dmic_shim_regs *)SOC_DMIC_SHIM_REG_BASE;

	/* enable power */
	dmic_shim_regs->dmiclctl |= SOC_DMIC_SHIM_DMICLCTL_SPA;

	while ((dmic_shim_regs->dmiclctl & SOC_DMIC_SHIM_DMICLCTL_CPA) == 0U) {
		/* wait for power status */
	}
#endif
}

static inline void soc_set_gna_power(void)
{
#if (CONFIG_INTEL_GNA)
	volatile struct soc_global_regs *regs =
		(volatile struct soc_global_regs *)SOC_S1000_GLB_CTRL_BASE;

	/* power on GNA block */
	regs->gna_power_control |= SOC_GNA_POWER_CONTROL_SPA;
	while ((regs->gna_power_control & SOC_GNA_POWER_CONTROL_CPA) == 0U) {
		/* wait for power status */
	}

	/* enable clock for GNA block */
	regs->gna_power_control |= SOC_GNA_POWER_CONTROL_CLK_EN;
#endif
}

static inline void soc_set_power_and_clock(void)
{
	volatile struct soc_dsp_shim_regs *dsp_shim_regs =
		(volatile struct soc_dsp_shim_regs *)SOC_DSP_SHIM_REG_BASE;

	dsp_shim_regs->clkctl |= SOC_CLKCTL_REQ_FAST_CLK |
		SOC_CLKCTL_OCS_FAST_CLK;
	dsp_shim_regs->pwrctl |= SOC_PWRCTL_DISABLE_PWR_GATING_DSP1 |
		SOC_PWRCTL_DISABLE_PWR_GATING_DSP0;

	soc_set_dmic_power();
	soc_set_gna_power();
	soc_set_audio_mclk();
}

static inline void soc_read_bootstraps(void)
{
	volatile struct soc_global_regs *regs =
		(volatile struct soc_global_regs *)SOC_S1000_GLB_CTRL_BASE;
	u32_t bootstrap;

	bootstrap = regs->straps;

	bootstrap &= SOC_S1000_STRAP_REF_CLK;

	switch (bootstrap) {
	case SOC_S1000_STRAP_REF_CLK_19P2:
		ref_clk_freq = 19200000U;
		break;
	case SOC_S1000_STRAP_REF_CLK_24P576:
		ref_clk_freq = 24576000U;
		break;
	case SOC_S1000_STRAP_REF_CLK_38P4:
	default:
		ref_clk_freq = 38400000U;
		break;
	}
}

/* host windows */
#define DMWBA(x)	(HOST_WIN_BASE(x) + 0x0)
#define DMWLO(x)	(HOST_WIN_BASE(x) + 0x4)
#define DMWBA_ENABLE	(1 << 0)
#define DMWBA_READONLY	(1 << 1)

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

static int soc_init(struct device *dev)
{
#if 0
	soc_read_bootstraps();

	LOG_INF("Reference clock frequency: %u Hz", ref_clk_freq);

	soc_set_resource_ownership();
	soc_set_power_and_clock();
#endif

	prepare_host_windows();

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

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);

#define SOF_GLB_TYPE_SHIFT			28
#define SOF_GLB_TYPE(x)				((x) << SOF_GLB_TYPE_SHIFT)
#define SOF_IPC_FW_READY			SOF_GLB_TYPE(0x7U)

struct sof_ipc_hdr {
	uint32_t size;			/**< size of structure */
} __attribute__((packed));

struct sof_ipc_cmd_hdr {
	uint32_t size;			/**< size of structure */
	uint32_t cmd;			/**< SOF_IPC_GLB_ + cmd */
} __attribute__((packed));

/* FW version - SOF_IPC_GLB_VERSION */
struct sof_ipc_fw_version {
	struct sof_ipc_hdr hdr;
	uint16_t major;
	uint16_t minor;
	uint16_t micro;
	uint16_t build;
	uint8_t date[12];
	uint8_t time[10];
	uint8_t tag[6];
	uint32_t abi_version;

	/* reserved for future use */
	uint32_t reserved[4];
} __attribute__((packed));

/* FW ready Message - sent by firmware when boot has completed */
struct sof_ipc_fw_ready {
	struct sof_ipc_cmd_hdr hdr;
	uint32_t dspbox_offset;	 /* dsp initiated IPC mailbox */
	uint32_t hostbox_offset; /* host initiated IPC mailbox */
	uint32_t dspbox_size;
	uint32_t hostbox_size;
	struct sof_ipc_fw_version version;

	/* Miscellaneous flags */
	uint64_t flags;

	/* reserved for future use */
	uint32_t reserved[4];
} __attribute__((packed));

static const struct sof_ipc_fw_ready fw_ready_apl
	__attribute__((section(".fw_ready"))) __attribute__((used)) = {
	.hdr = {
		.cmd = SOF_IPC_FW_READY,
		.size = sizeof(struct sof_ipc_fw_ready),
	},
	.version = {
		.hdr.size = sizeof(struct sof_ipc_fw_version),
		.micro = 3,
		.minor = 2,
		.major = 1,
#ifdef CONFIG_DEBUG
		/* only added in debug for reproducability in releases */
		.date = __DATE__,
		.time = __TIME__,
#endif
		.tag = "1234",
		.abi_version = 0x1234,
	},
	.flags = 0,
};

enum sof_ipc_ext_data {
	SOF_IPC_EXT_DMA_BUFFER = 0,
	SOF_IPC_EXT_WINDOW,
};

enum sof_ipc_region {
	SOF_IPC_REGION_DOWNBOX  = 0,
	SOF_IPC_REGION_UPBOX,
	SOF_IPC_REGION_TRACE,
	SOF_IPC_REGION_DEBUG,
	SOF_IPC_REGION_STREAM,
	SOF_IPC_REGION_REGS,
	SOF_IPC_REGION_EXCEPTION,
};

struct sof_ipc_ext_data_hdr {
	struct sof_ipc_cmd_hdr hdr;
	uint32_t type;		/**< SOF_IPC_EXT_ */
} __attribute__((packed));

struct sof_ipc_window_elem {
	struct sof_ipc_hdr hdr;
	uint32_t type;		/**< SOF_IPC_REGION_ */
	uint32_t id;		/**< platform specific - used to map to host memory */
	uint32_t flags;		/**< R, W, RW, etc - to define */
	uint32_t size;		/**< size of region in bytes */
	/* offset in window region as windows can be partitioned */
	uint32_t offset;
} __attribute__((packed));

/* extended data memory windows for IPC, trace and debug */
struct sof_ipc_window {
	struct sof_ipc_ext_data_hdr ext_hdr;
	uint32_t num_windows;
	struct sof_ipc_window_elem window[];
} __attribute__((packed));

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

static int soc_boot_complete(struct device *dev)
{
	mailbox_dspbox_write(0, &fw_ready_apl, sizeof(fw_ready_apl));
        mailbox_dspbox_write(sizeof(fw_ready_apl), &sram_window,
			     sram_window.ext_hdr.hdr.size);

	ipc_write(IPC_DIPCIE, 0);
	ipc_write(IPC_DIPCI, (0x80000000 | SOF_IPC_FW_READY));

	return 0;
}

SYS_INIT(soc_boot_complete, POST_KERNEL, 99);
