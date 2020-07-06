/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for ARM Generic Interrupt Controller
 *
 * The Generic Interrupt Controller (GIC) is the default interrupt controller
 * for the ARM A and R profile cores.  This driver is used by the ARM arch
 * implementation to handle interrupts.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_GIC_H_

#include <zephyr/types.h>
#include <device.h>

/*
 * Common Helper Constants
 */
#define GIC_SGI_INT_BASE		0
#define GIC_PPI_INT_BASE		16
#define GIC_SPI_INT_BASE		32

#define GIC_IS_SGI(intid)		(((intid) >= GIC_SGI_INT_BASE) && \
					 ((intid) < GIC_PPI_INT_BASE))
/* GIC special interrupt id */
#define GIC_INTID_SPURIOUS		1023

/* Fixme: update from platform specific define or dt */
#define GIC_NUM_CPU_IF			1

#ifndef _ASMLANGUAGE

/**
 * @brief raise SGI to target cores
 *
 * @param sgi_id      SGI ID 0 to 15
 * @param target_aff  target affinity in mpidr form.
 *                    Aff level 1 2 3 will be extracted by api.
 * @param target_list bitmask of target cores
 */
void gic_raise_sgi(unsigned int sgi_id, uint64_t target_aff,
		   uint16_t target_list);

/*
 * GIC Driver Interface Functions
 */

/**
 * @brief Initialise ARM GIC driver
 *
 * @return 0 if successful
 */
int arm_gic_init(void);

/**
 * @brief Enable interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_enable(unsigned int irq);

/**
 * @brief Disable interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_irq_disable(unsigned int irq);

/**
 * @brief Check if an interrupt is enabled
 *
 * @param irq interrupt ID
 * @return Returns true if interrupt is enabled, false otherwise
 */
bool arm_gic_irq_is_enabled(unsigned int irq);

/**
 * @brief Set interrupt priority
 *
 * @param irq interrupt ID
 * @param prio interrupt priority
 * @param flags interrupt flags
 */
void arm_gic_irq_set_priority(
	unsigned int irq, unsigned int prio, unsigned int flags);

/**
 * @brief Get active interrupt ID
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int arm_gic_get_active(void);

/**
 * @brief Signal end-of-interrupt
 *
 * @param irq interrupt ID
 */
void arm_gic_eoi(unsigned int irq);

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GIC_H_ */
