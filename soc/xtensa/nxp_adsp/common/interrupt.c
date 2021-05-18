/*
 * Copyright (c) 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Daniel Baluta <daniel.baluta@nxp.com>
 * Author: Paul Olaru <paul.olaru@nxp.com>
 */

#include <zephyr.h>
#include <spinlock.h>

#include <zephyr/types.h>
#include <arch/xtensa/arch.h>

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "interrupt.h"

/*
 * The IRQ_STEER module takes 512 shared interrupts and delivers them
 * via 8 interrupt lines to any other component. It supports 5 channels,
 * one of them being for the DSP itself (channel 0).
 *
 *                        +-----------+
 * IRQ 0-63 ------/64---> |           | ---/8----> Channel 0 (DSP)
 * IRQ 64-127 ----/64---> |           |
 * IRQ 128-191 ---/64---> |           | ---/8----> Channel 1 (M4)
 * IRQ 192-255 ---/64---> | IRQ_STEER |
 * IRQ 256-319 ---/64---> |           | ---/8----> Channel 2 (SCU2)
 * IRQ 320-383 ---/64---> |           | ---/8----> Channel 3 (SCU1)
 * IRQ 384-447 ---/64---> |           |
 * IRQ 448-511 ---/64---> |           | ---/1----> Channel 4 (CTI)
 *                        +-----------+
 *
 * IRQ steer channel block diagram (all 5 channels are identical)
 *
 * +---------------------------------------------------------+
 * |                  +---+          +----+           +---+  |
 * ---> IRQ 0-63 ---> |   |          |    |           |   |  |
 * |  [MASK 0-63] --> | & | --/64--> | OR | ---/1---> | & | ----> OUT[0]
 * |                  |   | [STATUS] |    | [MD0] --> |   |  |
 * |                  +---+          +----+           +---+  |
 * |                                                         |
 * | ... (same for the other IRQ lines and outputs to OUT[7] |
 * |                                                         |
 * +---------------------------------------------------------+
 *
 * In the schematic above:
 *    IRQ 0-511: Input IRQ lines (shared IRQs). IRQs 0-31 are reserved.
 *    MASK 0-511: Configurable mask for interrupts.
 *    MD0-MD7: Master disable register, block an entire output interrupt
 *    line.
 *    STATUS: Read only register which shows what interrupts are active.
 *    OUT: The 8 interrupt lines that lead to the DSP, leading to arch
 *    IRQs IRQ_NUM_IRQSTR_DSP0 through 7.
 *
 * Usage of the hardware: We turn on the hardware itself, then we
 * configure the mask (all mask bits default to 0), enable our arch
 * interrupts and wait for an interrupt on an output line.
 *
 * Upon receiving an arch interrupt, the driver must check the STATUS
 * registers corresponding to the arch interrupt in order to figure out
 * what the actual, input shared interrupt was, and then call any
 * registered callback to handle the condition leading to the interrupt.
 *
 * The hardware also supports forcing an interrupt from the software; I
 * have omitted this from the schematic since it is not relevant to the
 * usage in this driver.
 */

/* The MASK, SET (unused) and STATUS registers are 512-bit registers
 * split into 16 32-bit registers that we can directly access.
 *
 * To get the proper register for the shared interrupt irq, we do
 * IRQSTR_CH_MASK(IRQSTR_INT_REG(irq)) (MASK can be replaced by SET or
 * STATUS).
 *
 * The interrupt mapping to registers is defined in
 * platform/drivers/interrupt.h for each platform.
 *
 * The IRQSTR_CH_* macros perform the second part of this calculation
 * (offset) automatically.
 */

#ifndef __ZEPHYR__

#define IRQSTR_INT_REG(irq)		((irq) / 32)
#define IRQSTR_INT_BIT(irq)		((irq) % 32)
#define IRQSTR_INT_MASK(irq)		(1 << IRQSTR_INT_BIT(irq))

/* HW register access helper methods */

static inline void irqstr_write(uint32_t reg, uint32_t value)
{
	io_reg_write(IRQSTR_BASE_ADDR + reg, value);
}

static inline uint32_t irqstr_read(uint32_t reg)
{
	return io_reg_read(IRQSTR_BASE_ADDR + reg);
}

static inline void irqstr_update_bits(uint32_t reg, uint32_t mask,
				      uint32_t value)
{
	io_reg_update_bits(IRQSTR_BASE_ADDR + reg, mask, value);
}

/* IRQ_STEER helper methods
 * These methods are usable in any IRQ_STEER driver, not specific to SOF
 */

static void irqstr_enable_hw(void)
{
	irqstr_write(IRQSTR_CHANCTL, 1);
}

static void irqstr_disable_hw(void)
{
	irqstr_write(IRQSTR_CHANCTL, 0);
}

/* irqstr_get_status_word() - Get an interrupt status word
 * @index The index of the status word
 *
 * Get the status of interrupts 32*index .. 32*(index+1)-1 in a word.
 * This status is in one hardware register.
 * Return: Status register word for the corresponding interrupts
 */
static uint32_t irqstr_get_status_word(uint32_t index)
{
	if (index < IRQSTR_RESERVED_IRQS_NUM / 32)
		return 0;
	/* On out of range for our platform, be silent */
	if (index >= IRQSTR_IRQS_REGISTERS_NUM)
		return 0;
	return irqstr_read(IRQSTR_CH_STATUS(index));
}

static uint32_t irqstr_fixup_irq(uint32_t irq)
{
#ifdef CONFIG_IMX8M
	return irq - 32;
#else
	return irq;
#endif
}

/* Mask, that is, disable interrupts */
static void irqstr_mask_int(uint32_t irq)
{
	uint32_t mask;

	if (irq < IRQSTR_RESERVED_IRQS_NUM || irq >= IRQSTR_IRQS_NUM)
		/* Unusable interrupts */
		return;

	irq = irqstr_fixup_irq(irq);

	mask = IRQSTR_INT_MASK(irq);
	irqstr_update_bits(IRQSTR_CH_MASK(IRQSTR_INT_REG(irq)), mask, 0);
}

/* Unmask, that is, enable interrupts */
static void irqstr_unmask_int(uint32_t irq)
{
	uint32_t mask;

	if (irq < IRQSTR_RESERVED_IRQS_NUM || irq >= IRQSTR_IRQS_NUM)
		/* Unusable interrupts */
		return;

	irq = irqstr_fixup_irq(irq);

	mask = IRQSTR_INT_MASK(irq);
	irqstr_update_bits(IRQSTR_CH_MASK(IRQSTR_INT_REG(irq)), mask, mask);
}

#define IRQ_MAX_TRIES	1000

/* Extract the 64 status bits corresponding to output interrupt line
 * index (64 input interrupts)
 */
#ifdef CONFIG_IMX8M
static uint64_t get_irqsteer_interrupts(uint32_t index)
{
	uint64_t result = 0;

	result = irqstr_get_status_word(2 * index);
	result <<= 32;

	/* line 0 is special only maps interrupts [63..32],
	 * interval [31..0] is not used
	 */
	if (index == 0)
		return result;

	result |= irqstr_get_status_word(2 * index - 1);
	return result;
}
#else
static uint64_t get_irqsteer_interrupts(uint32_t index)
{
	uint64_t result = irqstr_get_status_word(2 * index + 1);

	result <<= 32;
	result |= irqstr_get_status_word(2 * index);
	return result;
}
#endif

/**
 * get_first_irq() Get the first IRQ bit set in this group.
 * @ints The 64 input interrupts
 *
 * Get the first pending IRQ in the group. For example, get_first_irq(0x40)
 * will return 6 (as 1 << 6 is 0x40), while get_first_irq(0) will return -1.
 *
 * Return: -1 if all interrupts are clear, or a shift value if at least
 * one interrupt is set.
 */
static int get_first_irq(uint64_t ints)
{
	return ffsll(ints) - 1;
}

static inline void handle_irq_batch(struct irq_cascade_desc *cascade,
				    uint32_t line_index, uint64_t status)
{
	int core = cpu_get_id();
	struct list_item *clist;
	struct irq_desc *child = NULL;
	int bit;
	bool handled;

	while (status) {
		bit = get_first_irq(status);
		handled = false;
		status &= ~(1ull << bit); /* Release interrupt */

		spin_lock(&cascade->lock);

		/* Get child if any and run handler */
		list_for_item(clist, &cascade->child[bit].list) {
			child = container_of(clist, struct irq_desc, irq_list);

			if (child->handler && (child->cpu_mask & 1 << core)) {
				/* run handler in non atomic context */
				spin_unlock(&cascade->lock);
				child->handler(child->handler_arg);
				spin_lock(&cascade->lock);

				handled = true;
			}

			platform_shared_commit(child, sizeof(*child));
		}

		platform_shared_commit(cascade, sizeof(*cascade));

		spin_unlock(&cascade->lock);

		if (!handled)
			/* Mask this interrupt so it won't happen again */
			irqstr_mask_int(line_index * IRQSTR_IRQS_PER_LINE + bit);
	}
}

static inline void irq_handler(void *data, uint32_t line_index)
{
	struct irq_desc *parent = data;
	struct irq_cascade_desc *cascade =
		container_of(parent, struct irq_cascade_desc, desc);
	uint64_t status;
	uint32_t tries = IRQ_MAX_TRIES;

	status = get_irqsteer_interrupts(line_index);

	while (status) {
		/* Handle current interrupts */
		handle_irq_batch(cascade, line_index, status);

		/* Any interrupts happened while we were handling the
		 * current ones?
		 */
		status = get_irqsteer_interrupts(line_index);
		if (!status)
			break;

		/* Any device keeping interrupting while we're handling
		 * or can't clear?
		 */

		if (!--tries)
			tries = IRQ_MAX_TRIES;
	}

	platform_shared_commit(parent, sizeof(*parent));
}

#define DEFINE_IRQ_HANDLER(n) \
	static inline void irqstr_irqhandler_##n(void *arg) \
	{ \
		irq_handler(arg, n); \
	}

DEFINE_IRQ_HANDLER(0)
DEFINE_IRQ_HANDLER(1)
DEFINE_IRQ_HANDLER(2)
DEFINE_IRQ_HANDLER(3)
DEFINE_IRQ_HANDLER(4)
DEFINE_IRQ_HANDLER(5)
DEFINE_IRQ_HANDLER(6)
DEFINE_IRQ_HANDLER(7)

static void irq_mask(struct irq_desc *desc, uint32_t irq, unsigned int core)
{
	uint32_t irq_base = desc->irq - IRQ_NUM_IRQSTR_DSP0;

	/* Compute the actual IRQ_STEER IRQ number */
	irq_base *= IRQSTR_IRQS_PER_LINE;
	irq += irq_base;

	irqstr_mask_int(irq);

	platform_shared_commit(desc, sizeof(*desc));
}

static void irq_unmask(struct irq_desc *desc, uint32_t irq, unsigned int core)
{
	uint32_t irq_base = desc->irq - IRQ_NUM_IRQSTR_DSP0;

	/* Compute the actual IRQ_STEER IRQ number */
	irq_base *= IRQSTR_IRQS_PER_LINE;
	irq += irq_base;

	irqstr_unmask_int(irq);

	platform_shared_commit(desc, sizeof(*desc));
}

static const struct irq_cascade_ops irq_ops = {
	.mask = irq_mask,
	.unmask = irq_unmask,
};

/* IRQ_STEER interrupts */
#define IRQSTR_CASCADE_TMPL_DECL(n) \
	{ \
		.name = "irqsteer" #n, \
		.irq = IRQ_NUM_IRQSTR_DSP##n, \
		.handler = irqstr_irqhandler_##n, \
		.ops = &irq_ops, \
		.global_mask = false, \
	},

static const struct irq_cascade_tmpl dsp_irq[] = {
	IRQSTR_CASCADE_TMPL_DECL(0)
	IRQSTR_CASCADE_TMPL_DECL(1)
	IRQSTR_CASCADE_TMPL_DECL(2)
	IRQSTR_CASCADE_TMPL_DECL(3)
	IRQSTR_CASCADE_TMPL_DECL(4)
	IRQSTR_CASCADE_TMPL_DECL(5)
	IRQSTR_CASCADE_TMPL_DECL(6)
	IRQSTR_CASCADE_TMPL_DECL(7)
};


void platform_interrupt_init(void)
{
	int i;

	/* Turn off the hardware so we don't have stray interrupts while
	 * initializing
	 */
	irqstr_disable_hw();
	/* Mask every external IRQ first */
	for (i = 0; i < IRQSTR_IRQS_REGISTERS_NUM; i++)
		irqstr_write(IRQSTR_CH_MASK(i), 0);
	/* Turn on the IRQ_STEER hardware */
	irqstr_enable_hw();

	for (i = 0; i < ARRAY_SIZE(dsp_irq); i++)
		interrupt_cascade_register(dsp_irq + i);
}

void platform_interrupt_set(uint32_t irq)
{
	if (interrupt_is_dsp_direct(irq))
		arch_interrupt_set(irq);
}

void platform_interrupt_clear(uint32_t irq, uint32_t mask)
{
	if (interrupt_is_dsp_direct(irq))
		arch_interrupt_clear(irq);
}

uint32_t platform_interrupt_get_enabled(void)
{
	return 0;
}

void interrupt_mask(uint32_t irq, unsigned int cpu)
{
	struct irq_cascade_desc *cascade = interrupt_get_parent(irq);

	if (cascade && cascade->ops->mask)
		cascade->ops->mask(&cascade->desc, irq - cascade->irq_base,
				   cpu);

	platform_shared_commit(cascade, sizeof(*cascade));
}

void interrupt_unmask(uint32_t irq, unsigned int cpu)
{
	struct irq_cascade_desc *cascade = interrupt_get_parent(irq);

	if (cascade && cascade->ops->unmask)
		cascade->ops->unmask(&cascade->desc, irq - cascade->irq_base,
				     cpu);

	platform_shared_commit(cascade, sizeof(*cascade));
}
#endif /* __ZEPHYR__ */

#ifdef CONFIG_IMX8M

/* Quirk of the driver in SOF (Quirk is specific to 8MP):
 * -> IRQSTR has 5 input channels each with 32 interrupts
 * -> IRQSTR has 3 output channels each with 64 interrupts
 * -> IRQ in[31:0]    => IRQ out[63:32]   (output channel #0)
 * -> IRQ in[63:32]   => IRQ out[95:64]   (output channel #1, low half)
 * -> IRQ in[95:64]   => IRQ out[127:96]  (output channel #1, high half)
 * -> IRQ in[127:96]  => IRQ out[159:128] (output channel #2, low half)
 * -> IRQ in[159:128] => IRQ out[191:160] (output channel #2, high half)
 * Thus in SOF irqsteer we shift everything with 32 and we get:
 * -> Interrupts 0-31 are not usable
 * -> Interrupts 32-63 map to hw irqs 0-31 (irqsteer0)
 * -> Interrupts 64-127 map to hw irqs 32-95 (irqsteer1)
 * -> Interrupts 128-191 map to hw irqs 96-159 (irqsteer2)
 */

const char * const irq_name_irqsteer[] = {
	"irqsteer0",
	"irqsteer1",
	"irqsteer2"
};
#else
/* SOF specific part of the driver */

/* Quirk of the driver in SOF (Quirk is specific to 8QXP/8QM):
 * -> Interrupts 0-31 are hardware
 * -> Interrupts 32-63 are unusable, as they are reserved in irqstr. We
 *  will never get an event on these shared interrupt lines.
 * -> Interrupts 64-543 are usable, mapping to 32-512 in IRQSTR itself
 * The above functions expect the 32-512 interrupts valid, not the
 * shifted SOF ones.
 */

const char * const irq_name_irqsteer[] = {
	"irqsteer0",
	"irqsteer1",
	"irqsteer2",
	"irqsteer3",
	"irqsteer4",
	"irqsteer5",
	"irqsteer6",
	"irqsteer7"
};
#endif /* !__ZEPHYR__ */

int irqstr_get_sof_int(int irqstr_int)
{
	int line, irq;

	/* Is it a valid interrupt? */
	if (irqstr_int < 0 || irqstr_int >= IRQSTR_IRQS_NUM)
		return -EINVAL;

	line = irqstr_int / IRQSTR_IRQS_PER_LINE;
	irq = irqstr_int % IRQSTR_IRQS_PER_LINE;

	return interrupt_get_irq(irq, irq_name_irqsteer[line]);
}
