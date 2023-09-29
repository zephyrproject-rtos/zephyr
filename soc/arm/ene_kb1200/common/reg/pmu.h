/******************************************************************************
 * @file     pmu.h
 * @brief    Define PMU's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef PMU_H
#define PMU_H

/**
  \brief  Structure type to access Power Management Unit (PMU).
 */
typedef volatile struct
{
  uint8_t  PMUIDLE;                                   /*IDLE wakeup by Interrupt Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint32_t PMUSTOP;                                   /*STOP Wakeup Source Register */
  uint8_t  PMUSTOPC;                                  /*STOP Control Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  PMUCTRL;                                   /*Control Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint8_t  PMUSTAF;                                   /*Status Flag */
  uint8_t  Reserved3[3];                              /*Reserved */
}  PMU_T;

//-- Constant Define -------------------------------------------
#define INTERRUPT_WAKEUP_IDLE           ENABLE
//STOP Wakeup Source
#define PMU_STOP_WU_GPTD                0x00000001
#define PMU_STOP_WU_VC0                 0x00000002
#define PMU_STOP_WU_VC1                 0x00000004
#define PMU_STOP_WU_IKB                 0x00000010
#define PMU_STOP_WU_WDT                 0x00000100
#define PMU_STOP_WU_HIBTMR              0x00000400
#define PMU_STOP_WU_eSPI                0x00010000
#define PMU_STOP_WU_SPIS                0x00010000
#define PMU_STOP_WU_I2CD32              0x00020000
#define PMU_STOP_WU_EDI32               0x00040000
#define PMU_STOP_WU_SWD                 0x00080000
#define PMU_STOP_WU_ITIM                0x00100000
#define PMU_STOP_WU_I2CS0               0x01000000
#define PMU_STOP_WU_I2CS1               0x02000000
#define PMU_STOP_WU_I2CS2               0x04000000
#define PMU_STOP_WU_I2CS3               0x08000000


#define S0i3_Enter_Stop                 0x80
#define S0i3_Active_Bit                 0x01
// wdt->WDTSCR[1] Shutdown Cause
#define SC_Normal_Run                   0x01        // Normal Shutdown
#define SC_WDT_Reset                    0x02        // WDT Reset
#define SC_ECRST_Reset                  0x03        // ECRST# Reset
#define SC_PWRBTN_Override              0x04        // Power Button Override
#define SC_Unknow_Reset                 0x10        // Other Unknow Reset

//-- Macro Define -----------------------------------------------
#define mSet_STOP_WU_Source(Src)        pmu->PMUSTOP |= (Src)
#define mClr_STOP_WU_Source(Src)        pmu->PMUSTOP &= ~(Src)

#define mSTOP_ADCO_AutoOff_Enable       pmu->PMUSTOPC |= (1UL)
#define mSTOP_ADCO_AutoOff_Disable      pmu->PMUSTOPC &= ~(1UL)

#define mS0i3_Stop_Time_Cnt             (mHIBTMR_mSecToCnt(S0i3_Stop_Time))
#define mS0i3_Run_Time_Cnt              (mGPT_mSecToGPT1Cnt(S0i3_Run_Time))

#define mChipShutdownCauseSet(x)        wdt->WDTSCR[1] = x
#define mChipShutdownCauseGet           wdt->WDTSCR[1]

#define mGet_EC_Reset_Low_PF            (pmu->PMUSTAF&0x01)
#define mClr_EC_Reset_Low_PF            pmu->PMUSTAF=0x01

#endif //PMU_H
