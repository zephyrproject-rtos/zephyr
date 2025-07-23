/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TIC_H_

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/interrupt-controller/tcc-tic.h>
#include <zephyr/sys/util_macro.h>

typedef struct tic_distributor {
	uint32_t dist_ctrl;                  /* Distributor Control Register. */
	uint32_t dist_type;                  /* Interrupt Controller Type Register. */
	uint32_t dist_impl_id;               /* Distributor Implementer Identification Register. */
	uint32_t dist_rsvd1[29];             /* Reserved. */
	uint32_t dist_intr_sec[32];          /* Interrupt Security Registers. */
	uint32_t dist_intr_set_en[32];       /* Interrupt Set-Enable Registers. */
	uint32_t dist_intr_clr_en[32];       /* Interrupt Clear-Enable Registers. */
	uint32_t dist_intr_set_pend[32];     /* Interrupt Set-Pending Registers. */
	uint32_t dist_intr_clr_pend[32];     /* Interrupt Clear-Pending Registers. */
	uint32_t dist_intr_set_act[32];      /* Interrupt Set-Active Registers. */
	uint32_t dist_intr_clr_act[32];      /* Interrupt Clear-Active Registers. */
	uint32_t dist_intr_pri[255];         /* Interrupt Priority Registers. */
	uint32_t dist_rsvd3[1];              /* Reserved. */
	uint32_t dist_intr_proc_target[255]; /* Interrupt Processor Target Registers. */
	uint32_t dist_rsvd4[1];              /* Reserved. */
	uint32_t dist_intr_config[64];       /* Interrupt Configuration Registers. */
	uint32_t dist_ppisr[1];              /* TICD_PPISR. */
	uint32_t dist_spisr[15];             /* TICD_SPISRn */
	uint32_t dist_rsvd5[112];            /* Reserved. */
	uint32_t dist_sw_gen_intr;           /* Software Generate Interrupt Register. */
	uint32_t dist_rsvd6[3];              /* Reserved. */
	uint32_t dist_sg_intr_clr_act[4];    /* SGInterrupt Clear-Active Registers. */
	uint32_t dist_sg_intr_set_act[4];    /* SGInterrupt Set-Active Registers. */
} tic_distributor;

typedef struct tic_cpu_interface {
	uint32_t cpu_ctlr;                  /* CPU Interface Control Register. */
	uint32_t cpu_pri_mask;              /* Interrupt Priority Mask Register. */
	uint32_t cpu_bin_ptr;               /* Binary Point Register. */
	uint32_t cpu_intr_ack;              /* Interrupt Acknowledge Register. */
	uint32_t cpu_end_intr;              /* End Interrupt Register. */
	uint32_t cpu_run_pri;               /* Running Priority Register. */
	uint32_t cpu_high_pend_intr;        /* Highest Pending Interrupt Register. */
	uint32_t cpu_alias_bin_ptr;         /* Aliased Binary Point Register. */
	uint32_t cpu_alias_intr_ack;        /* Aliased Interrupt Acknowledge Register */
	uint32_t cpu_alias_end_intr;        /* Aliased End Interrupt Register. */
	uint32_t cpu_aliase_high_pend_intr; /* Aliased Highest Pending Interrupt Register. */
	uint32_t cpu_rsvd[52];              /* Reserved. */
	uint32_t cpu_if_ident;              /* CPU Interface Identification Register. */
} tic_cpu_interface;

typedef void (*tic_isr_func)(void *pArg);

typedef struct tic_irq_func_ptr {
	tic_isr_func if_func_ptr;
	uint8_t if_is_both_edge;
	void *if_arg_ptr;
} tic_irq_func_ptr;

#define VCP_TIC_DIST_BASE (MCU_BSP_TIC_BASE + 0x1000UL)
#define VCP_TIC_CPU_BASE  (MCU_BSP_TIC_BASE + 0x2000UL)

#define tic_distributer ((volatile tic_distributor *)(VCP_TIC_DIST_BASE))
#define tic_cpu_if      ((volatile tic_cpu_interface *)(VCP_TIC_CPU_BASE))

/* ----------- DISTRIBUTOR CONTROL REGISTER -----------                 */
#define ARM_BIT_TIC_DIST_ICDDCR_EN 0x00000001UL /* Global TIC enable. */

/* ----------- CPU INTERFACE CONTROL REGISTER ---------                 */
#define TIC_CPUIF_CTRL_ENABLEGRP0 0x00000001UL /* Enable secure interrupts.      */
#define TIC_CPUIF_CTRL_ENABLEGRP1 0x00000002UL /* Enable non-secure interrupts.  */
#define TIC_CPUIF_CTRL_ACKCTL     0x00000004UL /* Secure ack of NS interrupts.   */

#define TIC_SGI_TO_TARGETLIST 0

#define MAX_API_CALL_INTERRUPT_PRIORITY 0

#define UNIQUE_INTERRUPT_PRIORITIES 32

/* The number of bits to shift for an interrupt priority is dependent on the
 * number of bits implemented by the interrupt controller.
 */
#if UNIQUE_INTERRUPT_PRIORITIES == 16
#define PRIORITY_SHIFT         4
#define MAX_BINARY_POINT_VALUE 3
#elif UNIQUE_INTERRUPT_PRIORITIES == 32
#define PRIORITY_SHIFT         3
#define MAX_BINARY_POINT_VALUE 2
#elif UNIQUE_INTERRUPT_PRIORITIES == 64
#define PRIORITY_SHIFT         2
#define MAX_BINARY_POINT_VALUE 1
#elif UNIQUE_INTERRUPT_PRIORITIES == 128
#define PRIORITY_SHIFT         1
#define MAX_BINARY_POINT_VALUE 0
#elif UNIQUE_INTERRUPT_PRIORITIES == 256
#define PRIORITY_SHIFT         0
#define MAX_BINARY_POINT_VALUE 0
#else
#error Invalid UNIQUE_INTERRUPT_PRIORITIES setting.  UNIQUE_INTERRUPT_PRIORITIES must be \
set to the number of unique priorities implemented by the target hardware
#endif

#define cpu_irq_disable()                                                                          \
	__asm volatile("CPSID i");                                                                 \
	__asm volatile("DSB");                                                                     \
	__asm volatile("ISB");

#define cpu_irq_enable()                                                                           \
	__asm volatile("CPSIE i");                                                                 \
	__asm volatile("DSB");                                                                     \
	__asm volatile("ISB");

#define UNMASK_VALUE (0xFFUL)

#define clear_interrupt_mask()                                                                     \
	{                                                                                          \
		cpu_irq_disable();                                                                 \
		tic_cpu_if->cpu_pri_mask = UNMASK_VALUE;                                           \
		__asm volatile("DSB\n"                                                             \
			       "ISB\n");                                                           \
		cpu_irq_enable();                                                                  \
	}

/*
 * TIC Driver Interface Functions
 */

/**
 * @brief Get active interrupt ID.
 *
 * @return Returns the ID of an active interrupt.
 */
unsigned int z_tic_irq_get_active(void);

/**
 * @brief Signal end-of-interrupt.
 *
 * @param irq interrupt ID.
 */
void z_tic_irq_eoi(unsigned int irq);

/**
 * @brief Interrupt controller initialization.
 */
void z_tic_irq_init(void);

/**
 * @brief Configure priority, irq type for the interrupt ID.
 *
 * @param irq interrupt ID.
 * @param prio interrupt priority.
 * @param flags interrupt flags.
 */
void z_tic_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

/**
 * @brief Enable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_tic_irq_enable(unsigned int irq);

/**
 * @brief Disable Interrupt.
 *
 * @param irq interrupt ID.
 */
void z_tic_irq_disable(unsigned int irq);

/**
 * @brief Check if an interrupt is enabled.
 *
 * @param irq interrupt ID.
 *
 * @retval 0 If interrupt is disabled.
 * @retval 1 If interrupt is enabled.
 */
bool z_tic_irq_is_enabled(unsigned int irq);

/**
 * @brief Raise a software interrupt.
 *
 * @param irq interrupt ID.
 */
void z_tic_arm_enter_irq(int irq);

/**
 * @brief Set an interrupt vector.
 *
 * @param irq interrupt ID.
 * @param prio interrupt priority.
 * @param irq_type interrupt type.
 * @param irq_func interrupt service routine.
 * @param irq_arg argument pointer.
 */
void tic_irq_vector_set(uint32_t irq, uint32_t prio, uint8_t irq_type, tic_isr_func irq_func,
			void *irq_arg);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_TIC_H_ */
