/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 * Copyright (c) 2025 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_

/* CLIC relative CSR number */
#define CSR_MTVT       (0x307)
#define CSR_MNXTI      (0x345)
#define CSR_MINTTHRESH (0x347)
#define CSR_MISELECT   (0x350)
#define CSR_MIREG      (0x351)
#define CSR_MIREG2     (0x352)

#ifndef __ASSEMBLER__

#include <stddef.h>

#ifdef CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS
/* CLIC Memory mapped register offset */
#define CLIC_CFG          (0x0)
#define CLIC_CTRL(irq)    (0x1000 + 4 * (irq))
#define CLIC_INTIP(irq)   (CLIC_CTRL(irq) + offsetof(union CLICCTRL, w.INTIP))
#define CLIC_INTIE(irq)   (CLIC_CTRL(irq) + offsetof(union CLICCTRL, w.INTIE))
#define CLIC_INTATTR(irq) (CLIC_CTRL(irq) + offsetof(union CLICCTRL, w.INTATTR))
#define CLIC_INTCTRL(irq) (CLIC_CTRL(irq) + offsetof(union CLICCTRL, w.INTCTRL))
#else
/* Indirect CSR Access miselect offset */
#define CLIC_CFG          (0x14A0)
#define CLIC_CTRL(irq)    (0x0) /* Dummy value for driver compatibility */
#define CLIC_INTIP(irq)   (0x1400 + (irq) / 32)
#define CLIC_INTIE(irq)   (0x1400 + (irq) / 32)
#define CLIC_INTATTR(irq) (0x1000 + (irq) / 4)
#define CLIC_INTCTRL(irq) (0x1000 + (irq) / 4)
#endif /* !CONFIG_LEGACY_CLIC_MEMORYMAP_ACCESS */

/* Nuclei ECLIC memory mapped register offset */
#define CLIC_INFO (0x4)
#define CLIC_MTH  (0x8)

/* CLIC register structure */
union CLICCFG {
	struct {
#ifdef CONFIG_NUCLEI_ECLIC
		uint32_t _reserved0: 1;
#endif /* CONFIG_NUCLEI_ECLIC */
		/** number of interrupt level bits */
		uint32_t nlbits: 4;
		/** number of clicintattr[i].MODE bits */
		uint32_t nmbits: 2;
		uint32_t _reserved1: 25;
	} w;
	uint32_t qw;
};

union CLICINTIP {
	struct {
		/** Interrupt Pending */
		uint8_t IP: 1;
		uint8_t reserved0: 7;
	} b;
	uint8_t w;
};

union CLICINTIE {
	struct {
		/** Interrupt Enabled */
		uint8_t IE: 1;
		uint8_t reserved0: 7;
	} b;
	uint8_t w;
};

union CLICINTATTR {
	struct {
		/** 0: non-vectored 1:vectored */
		uint8_t shv: 1;
		/** 0: level 1: rising edge 2: falling edge */
		uint8_t trg: 2;
		uint8_t reserved0: 3;
		uint8_t mode: 2;
	} b;
	uint8_t w;
};

union CLICCTRL {
	struct {
		volatile union CLICINTIP INTIP;
		volatile union CLICINTIE INTIE;
		volatile union CLICINTATTR INTATTR;
		volatile uint8_t INTCTRL;
	} w;
	uint32_t qw;
};

/* Nuclei ECLIC register structure */
union CLICINFO {
	struct {
		/** number of max supported interrupts */
		uint32_t numint: 13;
		/** architecture version */
		uint32_t version: 8;
		/** supported bits in the clicintctl */
		uint32_t intctlbits: 4;
		uint32_t _reserved0: 7;
	} b;
	uint32_t qw;
};

union CLICMTH {
	struct {
		uint32_t reserved0: 24;
		/**  machine mode interrupt level threshold */
		uint32_t mth: 8;
	} b;
	uint32_t qw;
};

#endif /*__ASSEMBLER__*/

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_ */
