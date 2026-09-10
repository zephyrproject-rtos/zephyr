/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_INTEL_ADSP_INTERRUPT_H_
#define ZEPHYR_SOC_INTEL_ADSP_INTERRUPT_H_

#include <stdint.h>

/* Low priority interrupt indices */
#define ACE_INTL_HIPC	  0
#define ACE_INTL_SBIPC	  1
#define ACE_INTL_ML	  2
#define ACE_INTL_IDCA	  3
#define ACE_INTL_LPVML	  4
#define ACE_INTL_SHA	  5
#define ACE_INTL_L1L2M	  6
#define ACE_INTL_I2S	  7
#define ACE_INTL_DMIC	  8
#define ACE_INTL_SNDW	  9
#define ACE_INTL_TTS	  10
#define ACE_INTL_WDT	  11
#define ACE_INTL_HDAHIDMA 12
#define ACE_INTL_HDAHODMA 13
#define ACE_INTL_HDALIDMA 14
#define ACE_INTL_HDALODMA 15
#define ACE_INTL_I3C	  16
#define ACE_INTL_GPDMA	  17
#define ACE_INTL_PWM	  18
#define ACE_INTL_I2C	  19
#define ACE_INTL_SPI	  20
#define ACE_INTL_UART	  21
#define ACE_INTL_GPIO	  22
#define ACE_INTL_UAOL	  23
#define ACE_INTL_IDCB	  24
#define ACE_INTL_DCW	  25
#define ACE_INTL_DTF	  26
#define ACE_INTL_FLV	  27
#define ACE_INTL_DPDMA	  28

/* Device interrupt control for the low priority interrupts.  It
 * provides per-core masking and status checking: ACE_DINT is an array
 * of these structs, one per core.  The state is in the bottom bits,
 * indexed by ACE_INTL_*.  Note that some of these use more than one
 * bit to discriminate sources (e.g. TTS's bits 0-2 are
 * timestamp/comparator0/comparator1).  It seems safe to write all 1's
 * to the short to "just enable everything", but drivers should
 * probably implement proper logic.
 *
 * Note that this block is independent of the Designware controller
 * that manages the shared IRQ.  Interrupts need to unmasked in both
 * in order to be delivered to software.  Per simulator source code,
 * this is "upstream" of DW: an interrupt will not be latched into the
 * status registers of the DW controller unless the IE bits here are
 * set.  That seems unlikely to correctly capture the hardware
 * behavior (it would mean that the DW controller was being
 * independently triggered multiple times by each core!).  Beware.
 *
 * Enable an interrupt for a core with e.g.:
 *
 *    ACE_DINT[core_id].ie[ACE_INTL_TTS] = 0xffff;
 */
struct ace_dint {
	uint16_t ie[32];  /* enable */
	uint16_t is[32];  /* status (potentially masked by ie)   */
	uint16_t irs[32]; /* "raw" status (hardware input state) */
	uint32_t unused[16];
};

/* This register enables (1) or disable (0) the interrupt of
 * each host inter-processor communication capability instance in a single register.
 */
#define DXHIPCIE_REG 0x78840

#define ACE_DINT	     ((volatile struct ace_dint *)DXHIPCIE_REG)
#define XTENSA_IRQ_NUM_MASK  0xff
#define XTENSA_IRQ_NUM_SHIFT 0

#define XTENSA_IRQ_NUMBER(_irq)	  ((_irq >> XTENSA_IRQ_NUM_SHIFT) & XTENSA_IRQ_NUM_MASK)
/* Convert between IRQ_CONNECT() numbers and ACE_INTL_* interrupts */
#define ACE_IRQ_NUM_SHIFT	  8
#define ACE_IRQ_NUM_MASK	  0xFFU
#define ACE_IRQ_FROM_ZEPHYR(_irq) (((_irq >> ACE_IRQ_NUM_SHIFT) & ACE_IRQ_NUM_MASK) - 1)

#define ACE_INTC_IRQ DT_IRQN(DT_NODELABEL(ace_intc))
#define ACE_IRQ_TO_ZEPHYR(_irq)                                                                    \
	((((_irq + 1) & ACE_IRQ_NUM_MASK) << ACE_IRQ_NUM_SHIFT) + ACE_INTC_IRQ)

#endif
