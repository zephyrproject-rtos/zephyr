/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT amd_acp_intc

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/sw_isr_table.h>
#include "intc_acp.h"
#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#endif
#ifdef CONFIG_SOC_ACP_7_X
#include <acp7x_chip_offsets.h>
#include <acp7x_chip_reg.h>
#endif
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(acp_intc, CONFIG_INTC_AMD_ACP_LOG_LEVEL);

#define INTERRUPT_DISABLE 0
#define INTERRUPT_ENABLE  1
#define INTERRUPT_CLEAR   0
#define IRQ_INT_MASK(irq) (1 << (irq))
#define RESERVED_IRQS_NUM 0
#define IRQS_NUM          9
#define IRQS_PER_LINE     1

#ifdef CONFIG_SOC_ACP_7_X
/* dsp_interrupt_routing_ctrl *_intr_level field values. */
#define ACP_INTR_ROUTE_LEVEL_3 0
#define ACP_INTR_ROUTE_LEVEL_5 2
/*
 * Route the generic DMA interrupt to EXT_LEVEL3 and the host->DSP
 * software (IPC) interrupt to EXT_LEVEL5, so the log DMA completion
 * (EXT_LEVEL3) and IPC (EXT_LEVEL5) use independent Xtensa lines.
 */
static void acp_intr_route(void)
{
	dsp_interrupt_routing_ctrl_0_t rout0;
	dsp_interrupt_routing_ctrl_1_t rout1;

	rout0 = (dsp_interrupt_routing_ctrl_0_t)io_reg_read(
		PU_REGISTER_BASE + ACP_DSP_INTERRUPT_ROUTING_CTRL_0);
	rout0.bits.dma_intr_level = ACP_INTR_ROUTE_LEVEL_3;
	io_reg_write(PU_REGISTER_BASE + ACP_DSP_INTERRUPT_ROUTING_CTRL_0, rout0.u32all);

	rout1 = (dsp_interrupt_routing_ctrl_1_t)io_reg_read(
		PU_REGISTER_BASE + ACP_DSP_INTERRUPT_ROUTING_CTRL_1);
	rout1.bits.host_to_dsp_intr1_level = ACP_INTR_ROUTE_LEVEL_5;
	io_reg_write(PU_REGISTER_BASE + ACP_DSP_INTERRUPT_ROUTING_CTRL_1, rout1.u32all);
}
#else
/* Other ACP SoCs keep IPC on EXT_LEVEL3 and program no routing. */
#define IRQ_NUM_EXT_LEVEL5       IRQ_NUM_EXT_LEVEL3

static inline void acp_intr_route(void)
{
	/* No interrupt routing required on non-ACP-7.X SoCs. */
}
#endif /* CONFIG_SOC_ACP_7_X */

/* Enable ACP DSP software interrupt */
void acp_dsp_sw_intr_enable(void)
{
	acp_dsp_sw_intr_cntl_t sw_intr_ctrl_reg;

	sw_intr_ctrl_reg =
		(acp_dsp_sw_intr_cntl_t)io_reg_read((PU_REGISTER_BASE + ACP_DSP_SW_INTR_CNTL));
	sw_intr_ctrl_reg.bits.dsp0_to_host_intr_mask = INTERRUPT_ENABLE;
	/* Write the Software Interrupt controller register */
	io_reg_write((PU_REGISTER_BASE + ACP_DSP_SW_INTR_CNTL), sw_intr_ctrl_reg.u32all);
	/* Enabling software interrupts */
	irq_enable(IRQ_NUM_EXT_LEVEL5);
}

/* Disable ACP DSP software interrupt */
void acp_dsp_sw_intr_disable(void)
{
	acp_dsp_sw_intr_cntl_t sw_intr_ctrl_reg;

	/* Read the register */
	sw_intr_ctrl_reg =
		(acp_dsp_sw_intr_cntl_t)io_reg_read((PU_REGISTER_BASE + ACP_DSP_SW_INTR_CNTL));
	sw_intr_ctrl_reg.bits.dsp0_to_host_intr_mask = INTERRUPT_DISABLE;
	/* Write the Software Interrupt controller register */
	io_reg_write((PU_REGISTER_BASE + ACP_DSP_SW_INTR_CNTL), sw_intr_ctrl_reg.u32all);
	irq_disable(IRQ_NUM_EXT_LEVEL5);
}

int acp_intc_init(void)
{
	acp_external_intr_enb_t ext_interrupt_enb;
	/* Route DMA -> EXT_LEVEL3, IPC (host->DSP sw intr) -> EXT_LEVEL5. */
	acp_intr_route();

	/* Enable the ACP to Host interrupts */
	ext_interrupt_enb.bits.acpextintrenb = 1;
	io_reg_write((PU_REGISTER_BASE + ACP_EXTERNAL_INTR_ENB), ext_interrupt_enb.u32all);
	acp_dsp_sw_intr_enable();

	return 0;
}

SYS_INIT(acp_intc_init, POST_KERNEL, 0);
