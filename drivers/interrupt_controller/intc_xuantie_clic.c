/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for XuanTie's Core Local Interrupt Controller
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>

/* clang-format off */

#define DT_DRV_COMPAT xuantie_clic

#define CLIC_INFO_CLICINTCTLBITS_Pos           21U
#define CLIC_INFO_CLICINTCTLBITS_Msk           (0xFUL << CLIC_INFO_CLICINTCTLBITS_Pos)

/*!< CLIC CLICCFG: NLBIT Position */
#define CLIC_CLICCFG_NLBIT_Pos                 1U
/*!< CLIC CLICCFG: NLBIT Mask */
#define CLIC_CLICCFG_NLBIT_Msk                 (0xFUL << CLIC_CLICCFG_NLBIT_Pos)

/*!< CLIC INTCFG: INTCFG Position */
#define CLIC_INTCFG_PRIO_Pos                   5U
/*!< CLIC INTCFG: INTCFG Mask */
#define CLIC_INTCFG_PRIO_Msk                   (0x7UL << CLIC_INTCFG_PRIO_Pos)

union CLICCFG {
	struct {
		uint8_t _reserved0 : 1;
		/** number of interrupt level bits */
		uint8_t nlbits     : 4;
		uint8_t _reserved1 : 2;
		uint8_t _reserved2 : 1;
	} b;
	uint8_t w;
};

union CLICINFO {
	struct {
		/** number of max supported interrupts */
		uint32_t numint     : 13;
		/** architecture version */
		uint32_t version    : 8;
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
		uint8_t IP          : 1;
		uint8_t reserved0   : 7;
	} b;
	uint8_t w;
};

union CLICINTIE {
	struct {
		/** Interrupt Enabled */
		uint8_t IE        : 1;
		uint8_t reserved0 : 7;
	} b;
	uint8_t w;
};

union CLICINTATTR {
	struct {
		/** 0: non-vectored 1:vectored */
		uint8_t shv       : 1;
		/** 0: level 1: rising edge 2: falling edge */
		uint8_t trg       : 2;
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

#define CLIC_CFG       (*((volatile union CLICCFG  *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(clic), 0))))
#define CLIC_INFO      (*((volatile union CLICINFO *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(clic), 1))))
#define CLIC_MTH       (*((volatile union CLICMTH  *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(clic), 2))))
#define CLIC_CTRL      ((volatile  struct CLICCTRL *)(DT_REG_ADDR_BY_IDX(DT_NODELABEL(clic), 3)))
#define CLIC_CTRL_SIZE (DT_REG_SIZE_BY_IDX(DT_NODELABEL(clic), 3))

/* clang-format no */

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
	CLIC_CTRL[irq].INTIE.b.IE = 1;
}

/**
 * @brief Disable interrupt
 */
void riscv_clic_irq_disable(uint32_t irq)
{
	CLIC_CTRL[irq].INTIE.b.IE = 0;
}

/**
 * @brief Get enable status of interrupt
 */
int riscv_clic_irq_is_enabled(uint32_t irq)
{
	return CLIC_CTRL[irq].INTIE.b.IE;
}

/**
 * @brief Set priority and level of interrupt
 */
void riscv_clic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	uint32_t clic_info = (uint32_t)CLIC_INFO.qw;
	uint32_t lnlbits =
		(clic_info & CLIC_INFO_CLICINTCTLBITS_Msk) >> CLIC_INFO_CLICINTCTLBITS_Pos;
	CLIC_CTRL[irq].INTCTRL =
		((unsigned long)CLIC_CTRL[irq].INTCTRL
			& (~CLIC_INTCFG_PRIO_Msk)) | (pri << (8 - lnlbits));

	union CLICINTATTR intattr = {.w = 0};
#if defined(CONFIG_RISCV_VECTORED_MODE) && !defined(CONFIG_LEGACY_CLIC)
	/*
	 * Set Selective Hardware Vectoring.
	 * Legacy SiFive does not implement smclicshv extension and vectoring is
	 * enabled in the mode bits of mtvec.
	 */
	intattr.b.shv = 1;
#else
	intattr.b.shv = 0;
#endif
	intattr.b.trg = (uint8_t)(flags & CLIC_INTATTR_TRIG_Msk);
	CLIC_CTRL[irq].INTATTR = intattr;
}

/**
 * @brief Set pending bit of an interrupt
 */
void riscv_clic_irq_set_pending(uint32_t irq)
{
	CLIC_CTRL[irq].INTIP.b.IP = 1;
}

void riscv_clic_irq_clear_pending(uint32_t irq)
{
	CLIC_CTRL[irq].INTIP.b.IP = 0;
}

static int xuantie_clic_init(const struct device *dev)
{
	CLIC_MTH.w = 0;
	CLIC_CFG.w =
		(((CLIC_INFO.qw & CLIC_INFO_CLICINTCTLBITS_Msk)
			>> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);
	for (int i = 0; i < CLIC_CTRL_SIZE / sizeof(struct CLICCTRL); i++) {
		CLIC_CTRL[i] = (struct CLICCTRL) { 0 };
	}

	nlbits = CLIC_CFG.b.nlbits;
	intctlbits = CLIC_INFO.b.intctlbits;
	max_prio = mask8(intctlbits - nlbits);
	max_level = mask8(nlbits);
	intctrl_mask = leftalign8(mask8(intctlbits), intctlbits);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, xuantie_clic_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);
