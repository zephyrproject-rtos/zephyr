/*
 * Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_rdc.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>

#include <dt-bindings/rdc/imx_rdc.h>

/* OSC/PLL is already initialized by ROM and Cortex-A53 (u-boot) */
static void SOC_RdcInit(void)
{
    /* Move M7 core to specific RDC domain 1 */
    rdc_domain_assignment_t assignment = {0};

    assignment.domainId = M7_DOMAIN_ID;
    RDC_SetMasterDomainAssignment(RDC, kRDC_Master_M7, &assignment);

    /*
     * The M7 core is running at domain 1, now enable the clock gate of the following IP/BUS/PLL in domain 1 in the CCM.
     * In this way, to ensure the clock of the peripherals used by M core not be affected by A core which is running at
     * domain 0.
     */
    CLOCK_EnableClock(kCLOCK_Iomux);

    CLOCK_EnableClock(kCLOCK_Ipmux1);
    CLOCK_EnableClock(kCLOCK_Ipmux2);
    CLOCK_EnableClock(kCLOCK_Ipmux3);
    CLOCK_EnableClock(kCLOCK_Ipmux4);

#if defined(FLASH_TARGET)
    CLOCK_EnableClock(kCLOCK_Qspi);
#endif

    CLOCK_ControlGate(kCLOCK_SysPll1Gate, kCLOCK_ClockNeededAll);   /* Enable the CCGR gate for SysPLL1 in Domain 1 */
    CLOCK_ControlGate(kCLOCK_SysPll2Gate, kCLOCK_ClockNeededAll);   /* Enable the CCGR gate for SysPLL2 in Domain 1 */
    CLOCK_ControlGate(kCLOCK_SysPll3Gate, kCLOCK_ClockNeededAll);   /* Enable the CCGR gate for SysPLL3 in Domain 1 */
    CLOCK_ControlGate(kCLOCK_AudioPll1Gate, kCLOCK_ClockNeededAll); /* Enable the CCGR gate for AudioPLL1 in Domain 1 */
    CLOCK_ControlGate(kCLOCK_AudioPll2Gate, kCLOCK_ClockNeededAll); /* Enable the CCGR gate for AudioPLL2 in Domain 1 */
    CLOCK_ControlGate(kCLOCK_VideoPll1Gate, kCLOCK_ClockNeededAll); /* Enable the CCGR gate for VideoPLL1 in Domain 1 */
}

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Fractional PLLs: Fout = ((mainDiv+dsm/65536) * refSel) / (preDiv * 2^ postDiv) */
/* AUDIO PLL1 configuration */
const ccm_analog_frac_pll_config_t g_audioPll1Config = {
    .refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
    .mainDiv = 262U,
    .dsm     = 9437U,
    .preDiv  = 2U,
    .postDiv = 3U, /*!< AUDIO PLL1 frequency  = 393215996HZ */
};

/* AUDIO PLL2 configuration */
const ccm_analog_frac_pll_config_t g_audioPll2Config = {
    .refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
    .mainDiv = 361U,
    .dsm     = 17511U,
    .preDiv  = 3U,
    .postDiv = 3U, /*!< AUDIO PLL2 frequency  = 361267197HZ */
};

/* SYSTEM PLL3 configuration */
const ccm_analog_integer_pll_config_t g_sysPll3Config = {
    .refSel  = kANALOG_PllRefOsc24M, /*!< PLL reference OSC24M */
    .mainDiv = 300,
    .preDiv  = 3U,
    .postDiv = 2U, /*!< SYSTEM PLL3 frequency  = 600MHZ */
};

static void SOC_ClockInit(void)
{
  /* switch AHB NOC root to 24M first in order to configure the SYSTEM PLL1. */
    CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxOsc24M);

    /* switch AXI M7 root to 24M first in order to configure the SYSTEM PLL3. */
    CLOCK_SetRootMux(kCLOCK_RootM7, kCLOCK_M7RootmuxOsc24M);

    /* Init Audio PLL1/Audio PLL2 */
    CLOCK_InitAudioPll1(&g_audioPll1Config); /* init AUDIO PLL1 run at 393215996HZ */
    CLOCK_InitAudioPll2(&g_audioPll2Config); /* init AUDIO PLL2 run at 361267197HZ */

    /* As ROM not enables PLL3 by default, enable PLL3 to 600M if A core not set it. */
    if (CLOCK_IsPllBypassed(CCM_ANALOG, kCLOCK_SysPll3InternalPll1BypassCtrl) == 1)
    {
        CLOCK_InitSysPll3(&g_sysPll3Config);
    }
    CLOCK_SetRootDivider(kCLOCK_RootM7, 1U, 1U);              /* Set M7 root clock freq to 600M / 1 = 600M */
    CLOCK_SetRootMux(kCLOCK_RootM7, kCLOCK_M7RootmuxSysPll3); /* switch cortex-m7 to SYSTEM PLL3 */

    CLOCK_SetRootDivider(kCLOCK_RootAhb, 1U, 1U);                   /* Set root clock freq to 133M / 1= 133MHZ */
    CLOCK_SetRootMux(kCLOCK_RootAhb, kCLOCK_AhbRootmuxSysPll1Div6); /* switch AHB to SYSTEM PLL1 DIV6 */

    CLOCK_SetRootDivider(kCLOCK_RootAudioAhb, 1U, 2U);                    /* Set root clock freq to 800MHZ/ 2= 400MHZ*/
    CLOCK_SetRootMux(kCLOCK_RootAudioAhb, kCLOCK_AudioAhbRootmuxSysPll1); /* switch AUDIO AHB to SYSTEM PLL1 */

    CLOCK_SetRootDivider(kCLOCK_RootUart4, 1U, 1U);                     /* Set root clock freq to 80MHZ/ 1= 80MHZ */
    CLOCK_SetRootMux(kCLOCK_RootUart4, kCLOCK_UartRootmuxSysPll1Div10); /* Set UART source to SysPLL1 Div10 80MHZ */

    CLOCK_EnableClock(kCLOCK_Rdc);   /* Enable RDC clock */
    CLOCK_EnableClock(kCLOCK_Ocram); /* Enable Ocram clock */

    /* The purpose to enable the following modules clock is to make sure the M7 core could work normally when A53 core
     * enters the low power status.*/
    CLOCK_EnableClock(kCLOCK_Sim_display);
    CLOCK_EnableClock(kCLOCK_Sim_m);
    CLOCK_EnableClock(kCLOCK_Sim_main);
    CLOCK_EnableClock(kCLOCK_Sim_s);
    CLOCK_EnableClock(kCLOCK_Sim_wakeup);
    CLOCK_EnableClock(kCLOCK_Debug);
    CLOCK_EnableClock(kCLOCK_Dram);
    CLOCK_EnableClock(kCLOCK_Sec_Debug);

    /* Update core clock */
    SystemCoreClockUpdate();

	CLOCK_EnableClock(kCLOCK_Uart4);
}

void SoC_InitMemory(void)
{
    /* Disable I cache and D cache */
    if (SCB_CCR_IC_Msk == (SCB_CCR_IC_Msk & SCB->CCR))
    {
        SCB_DisableICache();
    }
    if (SCB_CCR_DC_Msk == (SCB_CCR_DC_Msk & SCB->CCR))
    {
        SCB_DisableDCache();
    }

    /* Disable MPU */
    ARM_MPU_Disable();

    /* MPU configure:
     * Use ARM_MPU_RASR(DisableExec, AccessPermission, TypeExtField, IsShareable, IsCacheable, IsBufferable,
     * SubRegionDisable, Size)
     * API in mpu_armv7.h.
     * param DisableExec       Instruction access (XN) disable bit,0=instruction fetches enabled, 1=instruction fetches
     * disabled.
     * param AccessPermission  Data access permissions, allows you to configure read/write access for User and
     * Privileged mode.
     *      Use MACROS defined in mpu_armv7.h:
     * ARM_MPU_AP_NONE/ARM_MPU_AP_PRIV/ARM_MPU_AP_URO/ARM_MPU_AP_FULL/ARM_MPU_AP_PRO/ARM_MPU_AP_RO
     * Combine TypeExtField/IsShareable/IsCacheable/IsBufferable to configure MPU memory access attributes.
     *  TypeExtField  IsShareable  IsCacheable  IsBufferable   Memory Attribtue    Shareability        Cache
     *     0             x           0           0             Strongly Ordered    shareable
     *     0             x           0           1              Device             shareable
     *     0             0           1           0              Normal             not shareable   Outer and inner write
     * through no write allocate
     *     0             0           1           1              Normal             not shareable   Outer and inner write
     * back no write allocate
     *     0             1           1           0              Normal             shareable       Outer and inner write
     * through no write allocate
     *     0             1           1           1              Normal             shareable       Outer and inner write
     * back no write allocate
     *     1             0           0           0              Normal             not shareable   outer and inner
     * noncache
     *     1             1           0           0              Normal             shareable       outer and inner
     * noncache
     *     1             0           1           1              Normal             not shareable   outer and inner write
     * back write/read acllocate
     *     1             1           1           1              Normal             shareable       outer and inner write
     * back write/read acllocate
     *     2             x           0           0              Device              not shareable
     *  Above are normal use settings, if your want to see more details or want to config different inner/outter cache
     * policy.
     *  please refer to Table 4-55 /4-56 in arm cortex-M7 generic user guide <dui0646b_cortex_m7_dgug.pdf>
     * param SubRegionDisable  Sub-region disable field. 0=sub-region is enabled, 1=sub-region is disabled.
     * param Size              Region size of the region to be configured. use ARM_MPU_REGION_SIZE_xxx MACRO in
     * mpu_armv7.h.
     */

    /* Region 0 [0x0000_0000 - 0x4000_0000] : Memory with Device type, not executable, not shareable, non-cacheable. */
    MPU->RBAR = ARM_MPU_RBAR(0, 0x00000000U);
    MPU->RASR = ARM_MPU_RASR(1, ARM_MPU_AP_FULL, 0, 0, 0, 1, 0, ARM_MPU_REGION_SIZE_1GB);

    /* Region 1 TCML[0x0000_0000 - 0x0001_FFFF]: Memory with Normal type, shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(1, 0x00000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 1, 0, 0, 0, ARM_MPU_REGION_SIZE_128KB);

    /* Region 2 QSPI[0x0800_0000 - 0x0FFF_FFFF]: Memory with Normal type, shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(2, 0x08000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 1, 0, 0, 0, ARM_MPU_REGION_SIZE_128MB);

    /* Region 3 TCMU[0x2000_0000 - 0x2002_0000]: Memory with Normal type, shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(3, 0x20000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 1, 0, 0, 0, ARM_MPU_REGION_SIZE_128KB);

    /* Region 4 DDR[0x4000_0000 - 0x8000_0000]: Memory with Normal type, shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(4, 0x40000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 1, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

    /* Region 5 DDR[0x8000_0000 - 0xF000_0000]: Memory with Normal type, shareable, non-cacheable */
    MPU->RBAR = ARM_MPU_RBAR(5, 0x80000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 1, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

    /*
     * Enable MPU and HFNMIENA feature
     * HFNMIENA ensures that M7 core uses MPU configuration when in hard fault, NMI, and FAULTMASK handlers,
     * otherwise all memory regions are accessed without MPU protection, which has high risks of cacheable,
     * especially for AIPS systems.
     */
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_HFNMIENA_Msk);

    /* Configure the force_incr programmable bit in GPV_5 of PL301_display, which fixes partial write issue.
     * The AXI2AHB bridge is used for masters that access the TCM through system bus.
     * Please refer to errata ERR050362 for more information */
     #define GPV5_BASE_ADDR      (0x32500000)
     #define FORCE_INCR_BIT_MASK (0x2)
     #define FORCE_INCR_OFFSET   (0x4044)
     
    *(uint32_t *)(GPV5_BASE_ADDR + FORCE_INCR_OFFSET) =
 	   *(uint32_t *)(GPV5_BASE_ADDR + FORCE_INCR_OFFSET) | FORCE_INCR_BIT_MASK;
}

static int nxp_mimx8mn6_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	SoC_InitMemory();
	
	/* SoC specific RDC settings */
	SOC_RdcInit();

	/* SoC specific Clock settings */
	SOC_ClockInit();

	return 0;
}

SYS_INIT(nxp_mimx8mn6_init, PRE_KERNEL_1, 0);
