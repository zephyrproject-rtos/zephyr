/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
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
#include <zephyr/sys/__assert.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys_clock.h>
#include <zephyr/arch/cpu.h>
#include <soc.h>

#define DT_DRV_COMPAT xlnx_intc

#define SOC_INTC_DEVICE_ID 0

#define BASE_ADDRESS	 DT_INST_REG_ADDR(SOC_INTC_DEVICE_ID)
#define INTC_REG(offset) (uint32_t *)(BASE_ADDRESS + offset)

#define xlnx_intc_read(offset)	      sys_read32(BASE_ADDRESS + offset)
#define xlnx_intc_write(data, offset) sys_write32(data, BASE_ADDRESS + offset)

#define XIN_SVC_SGL_ISR_OPTION	1UL
#define XIN_SVC_ALL_ISRS_OPTION 2UL

#define XIN_ISR_OFFSET 0  /* Interrupt Status Register */
#define XIN_IPR_OFFSET 4  /* Interrupt Pending Register */
#define XIN_IER_OFFSET 8  /* Interrupt Enable Register */
#define XIN_IAR_OFFSET 12 /* Interrupt Acknowledge Register */
#define XIN_SIE_OFFSET 16 /* Set Interrupt Enable Register */
#define XIN_CIE_OFFSET 20 /* Clear Interrupt Enable Register */
#define XIN_IVR_OFFSET 24 /* Interrupt Vector Register */
#define XIN_MER_OFFSET 28 /* Master Enable Register */
#define XIN_IMR_OFFSET 32 /* Interrupt Mode Register , this is present only for Fast Interrupt */
#define XIN_IVAR_OFFSET                                                                            \
	0x100 /* Interrupt Vector Address Register , this is present only for Fast Interrupt */

/* Bit definitions for the bits of the MER register */

#define XIN_INT_MASTER_ENABLE_MASK   0x1UL
#define XIN_INT_HARDWARE_ENABLE_MASK 0x2UL /* once set cannot be cleared */

#define MICROBLAZE_INTERRUPT_VECTOR_ADDRESS 0x10 /* set it to standard interrupt vector */

struct xlnx_intc_state {
	bool is_ready;	 /**< Device is initialized and ready */
	bool is_started; /**< Device has been started */
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

int32_t xlnx_intc_controller_init(uint16_t device_id)
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

	uint32_t enable_mask = (XIN_INT_MASTER_ENABLE_MASK | XIN_INT_HARDWARE_ENABLE_MASK);

	xlnx_intc_write(enable_mask, XIN_MER_OFFSET);

	return 0;
}

static int xlnx_intc_interrupt_init(void)
{
	int32_t status = xlnx_intc_controller_init(SOC_INTC_DEVICE_ID);

	if (status != 0) {
		return status;
	}

	status = xlnx_intc_irq_start();

	return status;
}

SYS_INIT(xlnx_intc_interrupt_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
