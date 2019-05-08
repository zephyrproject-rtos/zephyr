/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#ifdef CONFIG_BOARD_MEC15XXEVB_ASSY6853
#include <soc.h>

#define B16TMR0_IRQ_INFO \
	MCHP_IRQ_INFO(MCHP_NVIC_AGGR_GIRQ23, 0, 23, MCHP_IRQ_DIRECT_B16TMR0)

volatile u32_t vg1;

#ifndef CONFIG_MCHP_XEC_INTMUX
static void girq08_aggr_isr(void *isr_args)
{
	printk("GIRQ08 Aggregated ISR\n");
}

static void girq09_aggr_isr(void *isr_args)
{
	printk("GIRQ09 Aggregated ISR\n");
}

static void girq10_aggr_isr(void *isr_args)
{
	printk("GIRQ10 Aggregated ISR\n");
}

static void girq11_aggr_isr(void *isr_args)
{
	printk("GIRQ11 Aggregated ISR\n");
}

static void girq12_aggr_isr(void *isr_args)
{
	printk("GIRQ12 Aggregated ISR\n");
}

static void girq26_aggr_isr(void *isr_args)
{
	printk("GIRQ26 Aggregated ISR\n");
}
#endif

static void gpio140_handler(void *isr_args)
{
	printk("GIRQ08 bit 0 level 2 handler\n");
}

static void gpio160_handler(void *isr_args)
{
	printk("GIRQ08 bit 16 level 2 handler\n");
}

static void gpio177_handler(void *isr_args)
{
	printk("GIRQ08 bit 31 level 2 handler\n");
}

static void gpio240_handler(void *isr_args)
{
	printk("GIRQ26 bit 0 level 2 handler\n");
}

static void gpio277_handler(void *isr_args)
{
	printk("GIRQ26 bit 31 level 2 handler\n");
}

static void gpio222_handler(void *isr_args)
{
	GIRQ12_REGS->SRC = (1ul << 18); /* clear R/W1C status bit */
	printk("GPIO222 button press!\n");
	GPIO_CTRL_REGS->CTRL_0157 ^= (MCHP_GPIO_CTRL_OUTV_HI);
}

static void gpio163_handler(void *isr_args)
{
	GIRQ08_REGS->SRC = (1ul << 19); /* clear R/W1C status bit */
	printk("GPIO163 button press!\n");
	GPIO_CTRL_REGS->CTRL_0153 ^= (MCHP_GPIO_CTRL_OUTV_HI);
}

static void b16tmr0_handler(void *isr_args)
{
	GIRQ23_REGS->SRC = (1ul << 0);
	NVIC_ClearPendingIRQ(MCHP_IRQ_DIRECT_B16TMR0);
	GPIO_CTRL_REGS->CTRL_0156 ^= (MCHP_GPIO_CTRL_OUTV_HI);
	vg1++;
}
#endif /* #ifdef CONFIG_BOARD_MEC15XXEVB_ASSY6853 */

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

#ifdef CONFIG_BOARD_MEC15XXEVB_ASSY6853
	vg1 = 0;

	/*
	 * MEC15xx EVB GPIO156 connected to LED2
	 * Configure GPIO mode, output, drive high.
	 * LED is on when GPIO driven low.
	 */
	GPIO_CTRL_REGS->CTRL_0156 = MCHP_GPIO_CTRL_PUD_NONE +
		MCHP_GPIO_CTRL_PWRG_VTR_IO + MCHP_GPIO_CTRL_IDET_DISABLE +
		MCHP_GPIO_CTRL_BUFT_PUSHPULL + MCHP_GPIO_CTRL_DIR_OUTPUT +
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +
		MCHP_GPIO_CTRL_INPAD_DIS + MCHP_GPIO_CTRL_MUX_F0 +
		MCHP_GPIO_CTRL_OUTV_HI;

	/* LED3 */
	GPIO_CTRL_REGS->CTRL_0157 = MCHP_GPIO_CTRL_PUD_NONE +
		MCHP_GPIO_CTRL_PWRG_VTR_IO + MCHP_GPIO_CTRL_IDET_DISABLE +
		MCHP_GPIO_CTRL_BUFT_PUSHPULL + MCHP_GPIO_CTRL_DIR_OUTPUT +
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +
		MCHP_GPIO_CTRL_INPAD_DIS + MCHP_GPIO_CTRL_MUX_F0 +
		MCHP_GPIO_CTRL_OUTV_HI;

	/* LED4 */
	GPIO_CTRL_REGS->CTRL_0153 = MCHP_GPIO_CTRL_PUD_NONE +
		MCHP_GPIO_CTRL_PWRG_VTR_IO + MCHP_GPIO_CTRL_IDET_DISABLE +
		MCHP_GPIO_CTRL_BUFT_PUSHPULL + MCHP_GPIO_CTRL_DIR_OUTPUT +
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +
		MCHP_GPIO_CTRL_INPAD_DIS + MCHP_GPIO_CTRL_MUX_F0 +
		MCHP_GPIO_CTRL_OUTV_HI;

#ifndef CONFIG_MCHP_XEC_INTMUX
	/* Aggregated GIRQ08 handler */
	IRQ_CONNECT(0x0000, 2, girq08_aggr_isr, NULL, 0);

	/* Aggregated GIRQ09 handler */
	IRQ_CONNECT(0x0001, 2, girq09_aggr_isr, NULL, 0);

	/* Aggregated GIRQ10 handler */
	IRQ_CONNECT(0x0002, 2, girq10_aggr_isr, NULL, 0);

	/* Aggregated GIRQ11 handler */
	IRQ_CONNECT(0x0003, 2, girq11_aggr_isr, NULL, 0);

	/* Aggregated GIRQ12 handler */
	IRQ_CONNECT(0x0004, 2, girq12_aggr_isr, NULL, 0);

	/* Aggregated GIRQ26 handler */
	IRQ_CONNECT(0x0011, 2, girq26_aggr_isr, NULL, 0);
#endif

	/*
	 * GPIO140 is GIRQ08 bit[0]
	 * Install handler in Level 2 table
	 * irq_num bits[7:8] = 0 for Aggregated GIRQ08
	 * irq_num bits[15:8] = bit number + 1
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO140_176(0140), 2,
			gpio140_handler, NULL, 0);

	/*
	 * GPIO160 is GIRQ08 bit[16]
	 * irq_num bits[7:8] = 0 for Aggregated GIRQ08
	 * irq_num bits[15:8] = bit number + 1
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO140_176(0160), 2,
			gpio160_handler, NULL, 0);

	/*
	 * Unimplemented GPIO177 would be GIRQ08 bit[31]
	 * irq_num bits[7:8] = 0 for Aggregated GIRQ08
	 * irq_num bits[15:8] = bit number + 1
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO140_176(0177), 2,
			gpio177_handler, NULL, 0);

	/*
	 * GPIO240 is GIRQ26 bit[0]
	 * irq_num bits[7:8] = 0x11(17) for Aggregated GIRQ26
	 * irq_num bits[15:8] = bit number + 1
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO240_276(0240), 2,
			gpio240_handler, NULL, 0);

	/*
	 * Unimplemented GPIO277 would be GIRQ26 bit[31]
	 * irq_num bits[7:8] = 0x11(17) for Aggregated GIRQ26
	 * irq_num bits[15:8] = bit number + 1
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO240_276(0277), 2,
			gpio277_handler, NULL, 0);

	/*
	 * GPIO222 is connected to button S3 on MEC1501 EVB.
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO200_236(0222), 2,
			gpio222_handler, NULL, 0);
	/*
	 * Hack GPIO config to enable falling edge interrupt in pin
	 * control register.
	 */
	GPIO_CTRL_REGS->CTRL_0222 = MCHP_GPIO_CTRL_PUD_NONE +
		MCHP_GPIO_CTRL_PWRG_VTR_IO + MCHP_GPIO_CTRL_IDET_FEDGE +
		MCHP_GPIO_CTRL_BUFT_PUSHPULL + MCHP_GPIO_CTRL_DIR_INPUT +
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +
		MCHP_GPIO_CTRL_INPAD_EN + MCHP_GPIO_CTRL_MUX_F0 +
		MCHP_GPIO_CTRL_OUTV_HI;

	/* Enable GPIO222 interrupt in interrupt HW */
	irq_enable(MCHP_IRQ_INFO_GPIO200_236(0222));

	/*
	 * GPIO163 is connected to button S4 on MEC1501 EVB
	 */
	IRQ_CONNECT(MCHP_IRQ_CONNECT_GPIO140_176(0163), 2,
			gpio163_handler, NULL, 0);
	GPIO_CTRL_REGS->CTRL_0163 = MCHP_GPIO_CTRL_PUD_NONE +
		MCHP_GPIO_CTRL_PWRG_VTR_IO + MCHP_GPIO_CTRL_IDET_FEDGE +
		MCHP_GPIO_CTRL_BUFT_PUSHPULL + MCHP_GPIO_CTRL_DIR_INPUT +
		MCHP_GPIO_CTRL_AOD_EN + MCHP_GPIO_CTRL_POL_NON_INVERT +
		MCHP_GPIO_CTRL_INPAD_EN + MCHP_GPIO_CTRL_MUX_F0 +
		MCHP_GPIO_CTRL_OUTV_HI;

	/* Enable GPIO163 interrupt in interrupt HW */
	irq_enable(MCHP_IRQ_INFO_GPIO140_176(0163));

	/* Direct Mode Basic 16-bit Timer 0 */
	IRQ_CONNECT(MCHP_IRQ_DIRECT_B16TMR0, 2, b16tmr0_handler, NULL, 0);

	/* Enable Basic 16-bit Timer 0 interrupt */
	irq_enable(B16TMR0_IRQ_INFO);

	mchp_pcr_periph_slp_ctrl(PCR_B16TMR0, 0);

	B16TMR0_REGS->CTRL = MCHP_BTMR_CTRL_SOFT_RESET;
	B16TMR0_REGS->CTRL = (4799ul << 16)
		+ MCHP_BTMR_CTRL_AUTO_RESTART + MCHP_BTMR_CTRL_ENABLE;
	B16TMR0_REGS->CNT = 10000;
	B16TMR0_REGS->PRLD = 10000;
	B16TMR0_REGS->IEN = MCHP_BTMR_INTEN;
	B16TMR0_REGS->CTRL |= MCHP_BTMR_CTRL_START;

	while (1) {
		k_sleep(1000);
		printk("vg1 = %u\n", vg1);
	}
#endif /* #ifdef CONFIG_BOARD_MEC15XXEVB_ASSY6853 */
}
