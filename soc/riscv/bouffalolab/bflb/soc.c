/*
 * Copyright (c) 2021, ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
 */

#include <device.h>
#include <init.h>

#include <bflb_hbn.h>
#include <bflb_glb.h>
#include <clic.h>

#define ROOT_FCLK_DIV			(0)
#define ROOT_BCLK_DIV			(1)
#define ROOT_UART_CLOCK_DIV		(0)

static void system_bor_init(void)
{
	HBN_BOR_CFG_Type borCfg = { 1 /* pu_bor */, 0 /* irq_bor_en */,
				    1 /* bor_vth */, 1 /* bor_sel */ };
	HBN_Set_BOR_Cfg(&borCfg);
}

static uint32_t mtimer_get_clk_src_div(void)
{
	return ((SystemCoreClockGet() / (GLB_Get_BCLK_Div() + 1))
		/ 1000 / 1000 - 1);
}

static void system_clock_init(void)
{
	GLB_Set_System_CLK(GLB_PLL_XTAL_40M, GLB_SYS_CLK_PLL160M);
	GLB_Set_System_CLK_Div(ROOT_FCLK_DIV, ROOT_BCLK_DIV);
	GLB_Set_MTimer_CLK(1, GLB_MTIMER_CLK_BCLK, mtimer_get_clk_src_div());
}

static void peripheral_clock_init(void)
{
	GLB_Set_UART_CLK(1, HBN_UART_CLK_160M, ROOT_UART_CLOCK_DIV);
}

#ifdef CONFIG_RISCV_GP
ulong_t __soc_get_gp_initial_value(void)
{
	extern uint32_t __global_pointer$;
	return (ulong_t)&__global_pointer$;
}
#endif

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */

static int bl_riscv_init(const struct device *arg)
{
	uint32_t key;
	uint32_t *p;
	uint32_t i = 0;
	uint32_t tmpVal = 0;

	ARG_UNUSED(arg);

	key = irq_lock();

	__disable_irq();

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmpVal = BL_RD_REG(HBN_BASE, HBN_IRQ_MODE);
	tmpVal = BL_CLR_REG_BIT(tmpVal, HBN_REG_EN_HW_PU_PD);
	BL_WR_REG(HBN_BASE, HBN_IRQ_MODE, tmpVal);

	/* GLB_Set_EM_Sel(GLB_EM_0KB); */
	tmpVal = BL_RD_REG(GLB_BASE, GLB_SEAM_MISC);
	tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_EM_SEL, GLB_EM_0KB);
	BL_WR_REG(GLB_BASE, GLB_SEAM_MISC, tmpVal);

	/* Fix 26M xtal clkpll_sdmin */
	tmpVal = BL_RD_REG(PDS_BASE, PDS_CLKPLL_SDM);

	if (BL_GET_REG_BITS_VAL(tmpVal, PDS_CLKPLL_SDMIN) == 0x49D39D) {
		tmpVal = BL_SET_REG_BITS_VAL(tmpVal, PDS_CLKPLL_SDMIN, 0x49D89E);
		BL_WR_REG(PDS_BASE, PDS_CLKPLL_SDM, tmpVal);
	}

	/* Restore default setting*/

	/* GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE); */
	tmpVal = BL_RD_REG(GLB_BASE, GLB_PARM);
	tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_UART_SWAP_SET, UART_SIG_SWAP_NONE);
	BL_WR_REG(GLB_BASE, GLB_PARM, tmpVal);

	/* GLB_JTAG_Sig_Swap_Set(JTAG_SIG_SWAP_NONE); */
	tmpVal = BL_RD_REG(GLB_BASE, GLB_PARM);
	tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_JTAG_SWAP_SET, JTAG_SIG_SWAP_NONE);
	BL_WR_REG(GLB_BASE, GLB_PARM, tmpVal);

	/* CLear all interrupt */
	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	/* init bor for all platform */
	system_bor_init();
	/* global IRQ enable */
	__enable_irq();

	system_clock_init();
	peripheral_clock_init();

	irq_unlock(key);

	return 0;
}

/* identify flash config automatically */
extern BL_Err_Type flash_init(void);

void System_Post_Init(void)
{
	PDS_Trim_RC32M();
	HBN_Trim_RC32K();
	flash_init();
}

SYS_INIT(bl_riscv_init, PRE_KERNEL_1, 0);
