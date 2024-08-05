/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Nuclie's Extended Core Interrupt Controller
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>

#define DT_DRV_COMPAT nuclei_eclic

union CLICCFG {
	struct {
		uint8_t _reserved0 : 1;
		/** number of interrupt level bits */
		uint8_t nlbits : 4;
		uint8_t _reserved1 : 2;
		uint8_t _reserved2 : 1;
	} b;
	uint8_t w;
};

union CLICINFO {
	struct {
		/** number of max supported interrupts */
		uint32_t numint : 13;
		/** architecture version */
		uint32_t version : 8;
		/** supported bits in the clicintctl */
		uint32_t intctlbits : 4;
		uint32_t _reserved0 : 7;
	} b;
	uint32_t qw;
};

union CLICMTH {
	uint8_t w;
};

union CLICINTIP {
	struct {
		/** Interrupt Pending */
		uint8_t IP : 1;
		uint8_t reserved0 : 7;
	} b;
	uint8_t w;
};

union CLICINTIE {
	struct {
		/** Interrupt Enabled */
		uint8_t IE : 1;
		uint8_t reserved0 : 7;
	} b;
	uint8_t w;
};

union CLICINTATTR {
	struct {
		/** 0: non-vectored 1:vectored */
		uint8_t shv : 1;
		/** 0: level 1: rising edge 2: falling edge */
		uint8_t trg : 2;
		uint8_t reserved0 : 3;
		uint8_t reserved1 : 2;
	} b;
	uint8_t w;
};

struct CLICCTRL {
	volatile union CLICINTIP INTIP;
	volatile union CLICINTIE INTIE;
	volatile union CLICINTATTR INTATTR;
	volatile uint8_t INTCTRL;
};

/** CLIC INTATTR: TRIG Mask */
#define CLIC_INTATTR_TRIG_Msk  0x3U

#define ECLIC_CFG       (*((volatile union CLICCFG  *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(eclic), 0))))
#define ECLIC_INFO      (*((volatile union CLICINFO *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(eclic), 1))))
#define ECLIC_MTH       (*((volatile union CLICMTH  *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(eclic), 2))))
#define ECLIC_CTRL      ((volatile  struct CLICCTRL *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(eclic), 3)))
#define ECLIC_CTRL_SIZE (DT_REG_SIZE_BY_IDX(DT_NODELABEL(eclic), 3))

static uint8_t nlbits;
static uint8_t intctlbits;
static uint8_t max_prio;
static uint8_t max_level;
static uint8_t intctrl_mask;

static inline uint8_t leftalign8(uint8_t val, uint8_t shift)
{
	return (val << (8U - shift));
}

static inline uint8_t mask8(uint8_t len)
{
	return ((1 << len) - 1) & 0xFFFFU;
}

/**
 * @brief Enable interrupt
 */
void riscv_clic_irq_enable(uint32_t irq)
{
	ECLIC_CTRL[irq].INTIE.b.IE = 1;
}

/**
 * @brief Disable interrupt
 */
void riscv_clic_irq_disable(uint32_t irq)
{
	ECLIC_CTRL[irq].INTIE.b.IE = 0;
}

/**
 * @brief Get enable status of interrupt
 */
int riscv_clic_irq_is_enabled(uint32_t irq)
{
	return ECLIC_CTRL[irq].INTIE.b.IE;
}

/**
 * @brief Set priority and level of interrupt
 */
void riscv_clic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	const uint8_t prio = leftalign8(MIN(pri, max_prio), intctlbits);
	const uint8_t level =  leftalign8(max_level, nlbits);
	const uint8_t intctrl = (prio | level) | (~intctrl_mask);

	ECLIC_CTRL[irq].INTCTRL = intctrl;

	union CLICINTATTR intattr = {.w = 0};

	/* Set non-vectoring as default. */
	intattr.b.shv = 0;
	intattr.b.trg = (uint8_t)(flags & CLIC_INTATTR_TRIG_Msk);
	ECLIC_CTRL[irq].INTATTR = intattr;
}

/**
 * @brief Set vector mode of interrupt
 */
void riscv_clic_irq_vector_set(uint32_t irq)
{
	/* Set Selective Hardware Vectoring. */
	union CLICINTATTR intattr = ECLIC_CTRL[irq].INTATTR;

	intattr.b.shv = 1;
	ECLIC_CTRL[irq].INTATTR = intattr;
}

/**
 * @brief Set pending bit of an interrupt
 */
void riscv_clic_irq_set_pending(uint32_t irq)
{
	ECLIC_CTRL[irq].INTIP.b.IP = 1;
}

static int nuclei_eclic_init(const struct device *dev)
{
	ECLIC_MTH.w = 0;
	ECLIC_CFG.w = 0;
	ECLIC_CFG.b.nlbits = 0;
	for (int i = 0; i < ECLIC_CTRL_SIZE; i++) {
		ECLIC_CTRL[i] = (struct CLICCTRL) { 0 };
	}

	nlbits = ECLIC_CFG.b.nlbits;
	intctlbits = ECLIC_INFO.b.intctlbits;
	max_prio = mask8(intctlbits - nlbits);
	max_level = mask8(nlbits);
	intctrl_mask = leftalign8(mask8(intctlbits), intctlbits);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, nuclei_eclic_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
