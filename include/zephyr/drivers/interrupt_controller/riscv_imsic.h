/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_IMSIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_IMSIC_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/arch/riscv/csr.h>

/* IMSIC direct CSRs (M-mode) */
#define CSR_MTOPEI      0x35C
#define CSR_MTOPI       0xFB0
#define CSR_MISELECT    0x350
#define CSR_MIREG       0x351
#define CSR_SETEIPNUM_M 0xFC0 /* Write EIID to set pending bit */
#define CSR_CLREIPNUM_M 0xFC1 /* Write EIID to clear pending bit */

/* MTOPEI register field masks */
#define MTOPEI_EIID_MASK  0x7FF /* Bits [10:0]: External Interrupt ID (0-2047) */
#define MTOPEI_PRIO_SHIFT 16    /* Bits [23:16]: Priority level */
#define MTOPEI_PRIO_MASK  (0xFF << MTOPEI_PRIO_SHIFT)

/* IMSIC indirect CSR addresses (per privilege file) */
#define ICSR_EIDELIVERY 0x70
#define ICSR_EITHRESH   0x72
#define ICSR_EIP0       0x80
#define ICSR_EIP1       0x81
#define ICSR_EIP2       0x82
#define ICSR_EIP3       0x83
#define ICSR_EIP4       0x84
#define ICSR_EIP5       0x85
#define ICSR_EIP6       0x86
#define ICSR_EIP7       0x87
#define ICSR_EIE0       0xC0
#define ICSR_EIE1       0xC1
#define ICSR_EIE2       0xC2
#define ICSR_EIE3       0xC3
#define ICSR_EIE4       0xC4
#define ICSR_EIE5       0xC5
#define ICSR_EIE6       0xC6
#define ICSR_EIE7       0xC7

#define EIDELIVERY_ENABLE    BIT(0)
#define EIDELIVERY_MODE_MMSI (0U << 29) /* MMSI only: 00 = 0x00000000 */

/* IMSIC API functions (implemented by drivers) */
uint32_t riscv_imsic_claim(void);

/**
 * @brief Enable an EIID in the CURRENT CPU's IMSIC
 *
 * This function uses CSR instructions that operate on the CPU executing
 * this code. To enable an EIID on a specific hart, this function MUST
 * be called from that hart (e.g., using k_thread_cpu_mask_enable).
 *
 * Following PLIC pattern: no parameter validation at API level.
 * Invalid EIIDs are caught in the ISR if they fire.
 *
 * @param eiid External Interrupt ID to enable (0-2047)
 */
void riscv_imsic_enable_eiid(uint32_t eiid);

/**
 * @brief Disable an EIID in the CURRENT CPU's IMSIC
 *
 * This function uses CSR instructions that operate on the CPU executing
 * this code. To disable an EIID on a specific hart, this function MUST
 * be called from that hart.
 *
 * Following PLIC pattern: no parameter validation at API level.
 * Invalid EIIDs are caught in the ISR if they fire.
 *
 * @param eiid External Interrupt ID to disable (0-2047)
 */
void riscv_imsic_disable_eiid(uint32_t eiid);

/**
 * @brief Check if an EIID is enabled in the CURRENT CPU's IMSIC
 *
 * @param eiid External Interrupt ID to check (0-2047)
 * @return 1 if enabled, 0 if disabled
 */
int riscv_imsic_is_enabled(uint32_t eiid);

#if defined(CONFIG_SMP)
/**
 * @brief Initialize IMSIC on secondary CPU
 *
 * Called during secondary CPU boot to configure that hart's IMSIC.
 * Configures EIDELIVERY, EITHRESHOLD, and enables MEXT interrupt.
 */
void z_riscv_imsic_secondary_init(void);
#endif /* CONFIG_SMP */

#endif
