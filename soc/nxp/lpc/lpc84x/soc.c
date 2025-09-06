#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>

#define CLOCK_SETUP_NODE DT_NODELABEL(clock_setup)

static void board_clock_init(void)
{
#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, enable_froout)
	POWER_DisablePD(kPDRUNCFG_PD_FRO_OUT);
#endif

#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, enable_fro)
	POWER_DisablePD(kPDRUNCFG_PD_FRO);
#endif

#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, fro_freq)
	if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, fro_freq), "18M")) {
		CLOCK_SetFroOscFreq(kCLOCK_FroOscOut18M);
		SystemCoreClock = 18000000U;
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, fro_freq), "24M")) {
		CLOCK_SetFroOscFreq(kCLOCK_FroOscOut24M);
		SystemCoreClock = 24000000U;
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, fro_freq), "30M")) {
		CLOCK_SetFroOscFreq(kCLOCK_FroOscOut30M);
		SystemCoreClock = 30000000U;
	} else {
		 printk("Unsupported FRO Oscillator Frequency");
	}
#endif

	CLOCK_SetFroOutClkSrc(kCLOCK_FroSrcFroOsc);

#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, enable_sysosc)
	POWER_DisablePD(kPDRUNCFG_PD_SYSOSC);
#endif

#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, extclk_src)
	if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, extclk_src), "sysosc")) {
		CLOCK_Select(kEXT_Clk_From_SysOsc);
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, extclk_src), "clkin")) {
		CLOCK_Select(kEXT_Clk_From_ClkIn);
	} else {
		printk("Unsupported external clock source");
	}
#else
	CLOCK_Select(kEXT_Clk_From_SysOsc);
#endif

#if DT_NODE_HAS_PROP(CLOCK_SETUP_NODE, mainclk_src)
	if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, mainclk_src), "fro")) {
		CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcFro);
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, mainclk_src), "extclk")) {
		CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcExtClk);
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, mainclk_src), "wdosc")) {
		CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcWdtOsc);
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, mainclk_src), "frodiv")) {
		CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcFroDiv);
	} else if (!strcmp(DT_PROP(CLOCK_SETUP_NODE, mainclk_src), "syspll")) {
		CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcSysPll);
	} else {
		printk("Unsupported main clock source");
	}
#else
	CLOCK_SetMainClkSrc(kCLOCK_MainClkSrcFro);
#endif
	CLOCK_SetCoreSysClkDiv(1U);
}

static int nxp_lpc84x_init(void)
{
	board_clock_init();

	return 0;
}

SYS_INIT(nxp_lpc84x_init, PRE_KERNEL_1, 0);
