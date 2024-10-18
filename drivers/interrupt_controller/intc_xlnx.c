/*
 * Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file implements AXI Interrupt Controller (INTC)
 * For more details about the INTC see PG 099
 *
 * The functionality has been based on intc_v3_12 package
 *
 * Right now the implementation:
 *  - does not support fast interrupt mode
 *  - does not support Cascade mode
 *  - does not support XIN_SVC_SGL_ISR_OPTION
 */

#include <errno.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_xlnx.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(xlnx_intc);

#define DT_DRV_COMPAT xlnx_xps_intc_1_00_a

#define BASE_ADDRESS	 DT_INST_REG_ADDR(0)
#define INTC_REG(offset) (uint32_t *)(BASE_ADDRESS + offset)

#define xlnx_intc_read(offset)	      sys_read32(BASE_ADDRESS + offset)
#define xlnx_intc_write(data, offset) sys_write32(data, BASE_ADDRESS + offset)

#define XIN_SVC_SGL_ISR_OPTION	1UL
#define XIN_SVC_ALL_ISRS_OPTION 2UL

#define XIN_ISR_OFFSET 0x0  /* Interrupt Status Register */
#define XIN_IPR_OFFSET 0x4  /* Interrupt Pending Register */
#define XIN_IER_OFFSET 0x8  /* Interrupt Enable Register */
#define XIN_IAR_OFFSET 0xc /* Interrupt Acknowledge Register */
#define XIN_SIE_OFFSET 0x10 /* Set Interrupt Enable Register */
#define XIN_CIE_OFFSET 0x14 /* Clear Interrupt Enable Register */
#define XIN_IVR_OFFSET 0x18 /* Interrupt Vector Register */
#define XIN_MER_OFFSET 0x1C /* Master Enable Register */
#define XIN_IMR_OFFSET 0x20 /* Interrupt Mode Register, only for Fast Interrupt */
#define XIN_IVAR_OFFSET	0x100 /* Interrupt Vector Address Register, only for Fast Interrupt */

/* Bit definitions for the bits of the MER register */
#define XIN_INT_MASTER_ENABLE_MASK	BIT(0)
#define XIN_INT_HARDWARE_ENABLE_MASK	BIT(1) /* once set cannot be cleared */

struct xlnx_intc_state {
	bool is_ready;	 /* Device is initialized and ready */
	bool is_started; /* Device has been started */
};

static struct xlnx_intc_state intc_state = {
	.is_ready = false,
	.is_started = false,
};

uint32_t xlnx_intc_irq_get_enabled(void)
{
	return xlnx_intc_read(XIN_IER_OFFSET);
}

uint32_t xlnx_intc_get_status_register(void)
{
	return xlnx_intc_read(XIN_ISR_OFFSET);
}

uint32_t xlnx_intc_irq_pending(void)
{
#if defined(CONFIG_XLNX_INTC_USE_IPR)
	return xlnx_intc_read(XIN_IPR_OFFSET);
#else
	uint32_t enabled = xlnx_intc_irq_get_enabled();
	uint32_t interrupt_status_register = xlnx_intc_get_status_register();

	return enabled & interrupt_status_register;
#endif
}

uint32_t xlnx_intc_irq_pending_vector(void)
{
#if defined(CONFIG_XLNX_INTC_USE_IVR)
	return xlnx_intc_read(XIN_IVR_OFFSET);
#else
	return find_lsb_set(xlnx_intc_irq_pending()) - 1;
#endif
}

void xlnx_intc_irq_enable(uint32_t irq)
{
	__ASSERT_NO_MSG(irq < 32);

	if (intc_state.is_ready != true) {
		LOG_DBG("Interrupt controller is not ready\n");
		k_panic();
	}

	uint32_t mask = BIT(irq);

#if defined(CONFIG_XLNX_INTC_USE_SIE)
	xlnx_intc_write(mask, XIN_SIE_OFFSET);
#else
	atomic_or((atomic_t *)INTC_REG(XIN_IER_OFFSET), mask);
#endif /* CONFIG_XLNX_INTC_USE_SIE */
}

void xlnx_intc_irq_disable(uint32_t irq)
{
	__ASSERT_NO_MSG(irq < 32);

	uint32_t mask = BIT(irq);

#if defined(CONFIG_XLNX_INTC_USE_CIE)
	xlnx_intc_write(mask, XIN_CIE_OFFSET);
#else
	atomic_and((atomic_t *)INTC_REG(XIN_IER_OFFSET), ~mask);
#endif /* CONFIG_XLNX_INTC_USE_CIE */
}

void xlnx_intc_irq_acknowledge(uint32_t mask)
{
	xlnx_intc_write(mask, XIN_IAR_OFFSET);
}

int32_t xlnx_intc_controller_init(void)
{
	if (intc_state.is_started == true) {
		return -EEXIST;
	}

	/*
	 * Disable IRQ output signal
	 * Disable all interrupt sources
	 * Acknowledge all sources
	 * Disable fast interrupt mode
	 */
	xlnx_intc_write(0, XIN_MER_OFFSET);
	xlnx_intc_write(0, XIN_IER_OFFSET);
	xlnx_intc_write(0xFFFFFFFF, XIN_IAR_OFFSET);

#if defined(CONFIG_XLNX_INTC_INITIALIZE_IVAR_REGISTERS)
	xlnx_intc_write(0, XIN_IMR_OFFSET);

	for (int idx = 0; idx < 32; idx++) {
		xlnx_intc_write(0x10, XIN_IVAR_OFFSET + (idx * 4));
	}
#endif

	intc_state.is_ready = true;

	return 0;
}

int32_t xlnx_intc_irq_start(void)
{
	if (intc_state.is_started != false) {
		return -EEXIST;
	}
	if (intc_state.is_ready != true) {
		return -ENOENT;
	}

	intc_state.is_started = true;

	xlnx_intc_write(XIN_INT_MASTER_ENABLE_MASK | XIN_INT_HARDWARE_ENABLE_MASK, XIN_MER_OFFSET);

	return 0;
}

static void xlnx_irq_handler(const void *arg)
{
	uint32_t irq, irq_mask;
	struct _isr_table_entry *ite;
	uint32_t level = POINTER_TO_UINT(arg);

	/* Get the IRQ number generating the interrupt */
	irq_mask = xlnx_intc_irq_pending();
	irq = find_lsb_set(irq_mask);

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (irq == 0U || irq >= 32)
		z_irq_spurious(NULL);

	/*
	 * xlnx_intc_irq_pending is returning values >= 1 but because it is second level offset
	 * needs to be found and _sw_isr_table starting from 0 not 1.
	 */
	irq -= 1;

	/* Apply level offset from registration as primary or secondary controller */
	irq += level;

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[irq];

	/* Table is already filled by z_irq_spurious calls that's why likely save to call it */
	ite->isr(ite->arg);

	/* When IRQ is handled and solved by driver, irq should be ACK to interrupt controller */
	xlnx_intc_irq_acknowledge(irq_mask);
}

static int xlnx_intc_init(const struct device *dev)
{
	int32_t status = xlnx_intc_controller_init();

	if (status != 0) {
		return status;
	}

#if defined(RISCV_IRQ_MEXT)
	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(RISCV_IRQ_MEXT, 0, xlnx_irq_handler,
		    UINT_TO_POINTER(CONFIG_2ND_LVL_ISR_TBL_OFFSET), 0);

	/* Enable external IRQ */
	irq_enable(RISCV_IRQ_MEXT);
#endif

	return xlnx_intc_irq_start();
}

#define XILINX_INTC_INIT(inst) \
	DEVICE_DT_INST_DEFINE(inst, &xlnx_intc_init, NULL, NULL, NULL, \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(XILINX_INTC_INIT)
