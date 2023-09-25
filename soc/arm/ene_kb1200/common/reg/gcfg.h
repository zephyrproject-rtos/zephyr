/**************************************************************************//**
 * @file     gcfg.h
 * @brief    Define GCFG's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef GCFG_H
#define GCFG_H

/**
  \brief  Structure type to access General Configuration (GCFG).
 */
typedef volatile struct
{
  uint8_t  IDV;                                       /*Version ID Register */
  uint8_t  Reserved0;                                 /*Reserved */
  uint16_t IDC;                                       /*Chip ID Register */
  uint32_t FWID;                                      /*Firmware ID Register */
  uint32_t MCURST;                                    /*MCU Reset Control Register */
  uint32_t RSTFLAG;                                   /*Reset Pending Flag Register */
  uint32_t GPIOALT;                                   /*GPIO Alternate Register */
  uint8_t  VCCSTA;                                    /*VCC Status Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint16_t GPIOMUX;                                   /*GPIO MUX Control Register */
  uint16_t Reserved2;                                 /*Reserved */ 
  uint16_t I2CSPMS;                                   /*I2CS Pin Map Selection Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint8_t  CLKCFG;                                    /*Clock Configuration Register */
  uint8_t  Reserved4[3];                              /*Reserved */ 
  uint32_t DPLLFREQ;                                  /*DPLL Frequency Register */
  uint32_t Reserved5;                                 /*Reserved */
  uint32_t GCFGMISC;                                  /*Misc. Register */
  uint8_t  EXTIE;                                     /*Extended Command Interrupt Enable Register */
  uint8_t  Reserved6[3];                              /*Reserved */ 
  uint8_t  EXTPF;                                     /*Extended Command Pending Flag Register */
  uint8_t  Reserved7[3];                              /*Reserved */           
  uint32_t EXTARG;                                    /*Extended Command Argument0/1/2 Register */
  uint8_t  EXTCMD;                                    /*Extended Command Port Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint32_t ADCOTR;                                    /*ADCO Register */
  uint32_t IDSR;                                      /*IDSR Register */                                                  
  uint32_t Reserved9[14];                             /*Reserved */
  uint32_t TRAPMODE;
  uint32_t CLK1UCFG;
  uint32_t LDO15TRIM;
  uint32_t Reserved10;
  uint32_t WWTR;
  uint32_t ECMISC2;
  uint32_t DPLLCTRL;
}  GCFG_T;

//#define gcfg                ((GCFG_T *) GCFG_BASE)             /* GCFG Struct       */
//#define GCFG                ((volatile unsigned long  *) GCFG_BASE)     /* GCFG  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define AHBAPB_CLK_SETTING                  USR_AHBAPB_CLK_SETTING 
#define PHER32K_CLK_SETTING                 USR_PHER32K_CLK_SETTING
#define SUPPORT_AUX_PBO                     USR_SUPPORT_AUX_PBO
#define SUPPORT_CPUExtCMD                   USR_SUPPORT_CPUExtCMD
#define SUPPORT_PLC                         USR_SUPPORT_PLC

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define LDO15_OFFSET                        0x22            //0x88
#define ECMISC2_OFFSET                      0x25            //0x94
#define OSC32K                              1
#define External_CLOCK                      0
#define External_CLOCK_GPIO_Num             0x5D
//#define FREQ_48M24M                         1
//#define FREQ_24M12M                         0

#define FREQ_96M48M                         0
#define FREQ_48M24M                         1
#define FREQ_24M12M                         2
#define FREQ_12M06M                         3

#define DPLL_32K_Target_Value               0x05B9UL
#define DPLL_SOF_Target_Value               0xBB80UL
#define DPLL_Dont_Care_Value                3
#define DPLL_Disable                        0
#define DPLL_Source_SOF                     1
#define DPLL_Source_OSC32K                  2
#define DPLL_Source_ExtCLK                  3
// gcfg->FWID status flag
#define ECFV_NORMAL_RUN                     0x00
#define ECFV_PREPARE_UPDATE_ESCD            0x01
#define ECFV_UPDATE_ESCD                    0x81
#define ECFV_PREPARE_UPDATE_BIOS            0x02
#define ECFV_UPDATE_BIOS                    0x82
#define ECFV_PREPARE_ENTER_DEEP_SLEEP       0x03
#define ECFV_ENTER_DEEP_SLEEP               0x83
#define S0i3_ENTER_STOP                     0x04
#define NON_DEEP_SLEEP_STATE                0x05
#define MAIN_LOOP_ENTER_IDLE                0x06

//VCC Status
#define VCC_0V                              0x00
#define VCC_3p3V                            0x01
#define VCC_1p8V                            0x02

//-- Macro Define -----------------------------------------------
//FWID
#define mSetFWID(id)                        gcfg->FWID = id
#define mGetFWID                            gcfg->FWID
//Reset Flag                                            
#define mGet_RST_Flag                       (gcfg->RSTFLAG&1)
#define mClear_RST_Flag                     gcfg->RSTFLAG = 1
//GPIO Alternate
//#define mGPIO0B_ALT_SCL5                    gcfg->GPIOALT |= 0x00000001
//#define mGPIO0B_ALT_ESBCLK                  gcfg->GPIOALT &= ~0x00000001
//#define mGPIO0C_ALT_SDA5                    gcfg->GPIOALT |= 0x00000002
//#define mGPIO0C_ALT_ESBDAT                  gcfg->GPIOALT &= ~0x00000002
//#define mGPIO0D_ALT_SDA4                    gcfg->GPIOALT |= 0x00000004
//#define mGPIO0D_ALT_RLCTX2                  gcfg->GPIOALT &= ~0x00000004
//#define mGPIO19_ALT_PWMLED0                 gcfg->GPIOALT |= 0x00000008
//#define mGPIO19_ALT_PWM3                    gcfg->GPIOALT &= ~0x00000008
//#define mGPIO30_ALT_NKROKSO0                gcfg->GPIOALT |= 0x00000020
//#define mGPIO30_ALT_SERTXD                  gcfg->GPIOALT &= ~0x00000020
//#define mGPIO48_ALT_UARTSOUT2               gcfg->GPIOALT |= 0x00000040
//#define mGPIO48_ALT_KSO16                   gcfg->GPIOALT &= ~0x00000040
//#define mGPIO4C_ALT_SCL3                    gcfg->GPIOALT |= 0x00000080
//#define mGPIO4C_ALT_PSCLK2                  gcfg->GPIOALT &= ~0x00000080
//#define mGPIO4D_ALT_SDA3                    gcfg->GPIOALT |= 0x00000100
//#define mGPIO4D_ALT_PSDAT2                  gcfg->GPIOALT &= ~0x00000100
//#define mGPIO4E_ALT_KSO18                   gcfg->GPIOALT |= 0x00000200
//#define mGPIO4E_ALT_PSCLK3                  gcfg->GPIOALT &= ~0x00000200
//#define mGPIO4F_ALT_KSO19                   gcfg->GPIOALT |= 0x00000400
//#define mGPIO4F_ALT_PSDAT3                  gcfg->GPIOALT &= ~0x00000400
//#define mGPIO4A_ALT_USBDM                   gcfg->GPIOALT |= 0x03000000
//#define mGPIO4A_ALT_PSCLK1                  gcfg->GPIOALT &= ~0x03000000
//#define mGPIO4A_ALT_SCL2                    {gcfg->GPIOALT &= ~0x03000000; gcfg->GPIOALT |= 0x01000000;}
//#define mGPIO4B_ALT_USBDP                   gcfg->GPIOALT |= 0x0C000000
//#define mGPIO4B_ALT_PSDAT1                  gcfg->GPIOALT &= ~0x0C000000
//#define mGPIO4B_ALT_SDA2                    {gcfg->GPIOALT &= ~0x0C000000; gcfg->GPIOALT |= 0x04000000;}

#define mGPIO00_ALT_PWM8                    gcfg->GPIOALT |= 0x00000001
#define mGPIO00_ALT_PWMLED0                 gcfg->GPIOALT &= ~0x00000001
#define mGPIO22_ALT_PWM9                    gcfg->GPIOALT |= 0x00000002
#define mGPIO22_ALT_PWMLED1                 gcfg->GPIOALT &= ~0x00000002
#define mGPIO28_ALT_SERRXD2                 gcfg->GPIOALT |= 0x00000004
#define mGPIO28_ALT_32KOUT                  gcfg->GPIOALT &= ~0x00000004
#define mGPIO36_ALT_SERTXD2                 gcfg->GPIOALT |= 0x00000008
#define mGPIO36_ALT_UARTSOUT                gcfg->GPIOALT &= ~0x00000008
#define mGPIO5C_ALT_P80DAT                  gcfg->GPIOALT |= 0x00000010
#define mGPIO5C_ALT_KSO6                    gcfg->GPIOALT &= ~0x00000010
#define mGPIO5D_ALT_P80CLK                  gcfg->GPIOALT |= 0x00000020
#define mGPIO5D_ALT_KSO7                    gcfg->GPIOALT &= ~0x00000020
#define mGPIO5E_ALT_SERRXD1                 gcfg->GPIOALT |= 0x00000040
#define mGPIO5E_ALT_KSO8                    gcfg->GPIOALT &= ~0x00000040
#define mGPIO5F_ALT_SERTXD1                 gcfg->GPIOALT |= 0x00000080
#define mGPIO5F_ALT_KSO9                    gcfg->GPIOALT &= ~0x00000080
#define mGPIO71_ALT_UARTRTS                 gcfg->GPIOALT |= 0x00000100
#define mGPIO71_ALT_SDA8                    gcfg->GPIOALT &= ~0x00000100
#define mGPIO38_ALT_PWM1                    gcfg->GPIOALT |= 0x00000200
#define mGPIO38_ALT_SCL4                    gcfg->GPIOALT &= ~0x00000200

#define mVCC_HIF_Status_Set(volt)          gcfg->VCCSTA = (gcfg->VCCSTA&0x0000000C)|(volt)
#define mVCC_IO2_Status_Set(volt)          gcfg->VCCSTA = (gcfg->VCCSTA&0x00000003)|(volt<<2)

//// Pass Through Function
//#define mGPIO_PassThrough15to13En           gcfg->GPIOBPC1513 |= 0x01
//#define mGPIO_PassThrough15to13Dis          gcfg->GPIOBPC1513 &= ~0x01
//#define mGPIO_PassThrough15to13PinInit      {mGPIOIE_En(0x15); mGPIOOE_En(0x13);}
//#define mGPIO_PassThrough15to13InvertedEn   gcfg->GPIOBPC1513 |= 0x02
//#define mGPIO_PassThrough15to13InvertedDis  gcfg->GPIOBPC1513 &= ~0x02
//#define mGPIO_PassThrough15to13Delay(unit,counter) gcfg->GPIOBPC1513 = (gcfg->GPIOBPC1513&0x03)|((unit&1)<<2)|(counter<<4)
////unit : 0 = 4us, 1 = 64us; counter = 0~15;
//#define mGPIO_PassThrough17to18En           gcfg->GPIOBPC1718 |= 0x01
//#define mGPIO_PassThrough17to18Dis          gcfg->GPIOBPC1718 &= ~0x01
//#define mGPIO_PassThrough17to18PinInit      {mGPIOIE_En(0x17); mGPIOOE_En(0x18);}
//#define mGPIO_PassThrough17to18InvertedEn   gcfg->GPIOBPC1718 |= 0x02
//#define mGPIO_PassThrough17to18InvertedDis  gcfg->GPIOBPC1718 &= ~0x02
//#define mGPIO_PassThrough17to18Delay(unit,counter) gcfg->GPIOBPC1718 = (gcfg->GPIOBPC1718&0x03)|((unit&1)<<2)|(counter<<4)
////unit : 0 = 4us, 1 = 64us; counter = 0~15;
//#define mGPIO_PassThrough7Eto6BEn           gcfg->GPIOBPC7E6B |= 0x01
//#define mGPIO_PassThrough7Eto6BDis          gcfg->GPIOBPC7E6B &= ~0x01
//#define mGPIO_PassThrough7Eto6BPinInit      {mGPIOIE_En(0x7E); mGPIOOE_En(0x6B);}
//#define mGPIO_PassThrough7Eto6BInvertedEn   gcfg->GPIOBPC7E6B |= 0x02
//#define mGPIO_PassThrough7Eto6BInvertedDis  gcfg->GPIOBPC7E6B &= ~0x02
//#define mGPIO_PassThrough7Eto6BDelay(unit,counter) gcfg->GPIOBPC7E6B = (gcfg->GPIOBPC7E6B&0x03)|((unit&1)<<2)|(counter<<4)
////unit : 0 = 4us, 1 = 64us; counter = 0~15;

//GPIO MUX
//#define mSPIH_PINOUT_GPIO24252627           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x01;}
//#define mSPIH_PINOUT_GPIO5A585C5B           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x02;}
//#define mSPIH_PINOUT_GPIO60616278           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x03;}
#define mSPIH_PINOUT_GPIO5A5B5C5D           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x01;}
#define mSPIH_PINOUT_GPIO45474446           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x02;}
#define mSPIH_PINOUT_GPIO4140423E           {gcfg->GPIOMUX &= ~0x03; gcfg->GPIOMUX |= 0x03;}

#define mSHAREROM_To_ESPI_Enable            gcfg->GPIOMUX |= 0x08
#define mSHAREROM_To_ESPI_Disable           gcfg->GPIOMUX &= ~0x08
#define mSBUSwitch_Enable                   gcfg->GPIOMUX |= 0x10
#define mSBUSwitch_Disable                  gcfg->GPIOMUX &= ~0x10
#define mSBUSwitch_Swap_H1_C1               gcfg->GPIOMUX &= ~0x20
#define mSBUSwitch_Swap_H1_C2               gcfg->GPIOMUX |= 0x20
//#define mGPIO1617_Swap_Enable               gcfg->GPIOMUX |= 0x0100
//#define mGPIO1617_Swap_Disable              gcfg->GPIOMUX &= ~0x0100
//#define mGPIO1617_SERTXRX                   gcfg->GPIOMUX &= ~0x0600
//#define mGPIO1617_UARTSOUTSIN               gcfg->GPIOMUX = (gcfg->GPIOMUX&(~0x0600))|0x0200
//#define mGPIO1617_SBUDebug                  gcfg->GPIOMUX |= 0x0600
#define mGPIO0103_Swap_Enable               gcfg->GPIOMUX |= 0x0100
#define mGPIO0103_Swap_Disable              gcfg->GPIOMUX &= ~0x0100
#define mGPIO0103_SERTXRX                   gcfg->GPIOMUX &= ~0x0600
#define mGPIO0103_UARTSOUTSIN               gcfg->GPIOMUX = (gcfg->GPIOMUX&(~0x0600))|0x0200
#define mGPIO0103_SBUDebug                  gcfg->GPIOMUX |= 0x0600

#define mSPIS_Interface_Enable              gcfg->GPIOMUX |= 0x0800
#define mSPIS_Interface_Disable             gcfg->GPIOMUX &= ~0x0800

//I2CS pinout selection
//#define mI2CS_PINOUT_SELECTION(no,sel)      gcfg->I2CSPMS = gcfg->I2CSPMS&(~(0x0F<<((no&1)*4)))|((sel&7)<<((no&1)*4))
#define mI2CS_PINOUT_SELECTION(no,sel)      gcfg->I2CSPMS = gcfg->I2CSPMS&(~(0x0F<<((no&3)*4)))|((sel&0x0F)<<((no&3)*4))

//Clock Configuration
#define mExternal_Clock_Enable              {mGPIOIE_En(External_CLOCK_GPIO_Num); gcfg->CLKCFG |= 0x01; mPHER32K_SELECTION(External_CLOCK);}
#define mExternal_Crystal_Clock_Enable      {mGPIOIE_En(External_CLOCK_GPIO_Num); gcfg->CLKCFG |= 0x02; mPHER32K_SELECTION(External_CLOCK);}
#define mExternal_Clock_Disable             {mPHER32K_SELECTION(OSC32K); gcfg->CLKCFG &= ~0x03; mGPIOIE_Dis(External_CLOCK_GPIO_Num);}

#define mPHER32K_SELECTION(sel)             gcfg->CLKCFG = gcfg->CLKCFG&(~0x04)|((sel&1)<<2)
#define mAHBAPB_Clock_SELECTION(sel)        gcfg->CLKCFG = gcfg->CLKCFG&(~0x30)|((sel&3)<<4)

#define mDPLL_Disable                       gcfg->DPLLFREQ &= ~0x00000003UL
//#define mDPLL_Set_USBSOF                    gcfg->DPLLFREQ = (DPLL_SOF_Target_Value<<16)|(DPLL_Dont_Care_Value<<8)|DPLL_Source_SOF 
#define mDPLL_Set_OSC32K                    gcfg->DPLLFREQ = (DPLL_32K_Target_Value<<16)|(DPLL_Dont_Care_Value<<8)|DPLL_Source_OSC32K
#define mDPLL_Set_ExtCLK                    gcfg->DPLLFREQ = (DPLL_32K_Target_Value<<16)|(DPLL_Dont_Care_Value<<8)|DPLL_Source_ExtCLK
////Auto-Reset when power sequence fail
//#define mPowerSeqFail_Reset_Enable          gcfg->GPIORPSC_C |= 0x01
//#define mPowerSeqFail_Reset_Disable         gcfg->GPIORPSC_C &= ~0x01
//#define mPowerButton_DriveGPIO_Enable       gcfg->GPIORPSC_C |= 0x20
//#define mPowerButton_DriveGPIO_Disable      gcfg->GPIORPSC_C &= ~0x20
//#define mDrive_GPIO5X_Before_Reset_Set(no)  gcfg->GPIORPSC_5X = no
//#define mDrive_GPIO6X_Before_Reset_Set(no)  gcfg->GPIORPSC_6X = no

//GCFG Misc.
//#define mSPIHDI_Select_DI0_GPIO78           gcfg->GCFGMISC &= ~0x00000001
//#define mSPIHDI_Select_DI1_GPIO6B           gcfg->GCFGMISC |= 0x00000001
#define mPLC_Disable                        gcfg->GCFGMISC &= ~0x00000008
//#define mShunt_NMOS_Enable                  gcfg->GCFGMISC |= 0x00000010
#define mAUX_PBO_Enable                     gcfg->GCFGMISC |= 0x00000020
#define mAUX_PBO_Disable                    gcfg->GCFGMISC &= ~0x00000020
#define mPROCHOT_GPIO54_Enable              gcfg->GCFGMISC |= 0x00000040
#define mPROCHOT_GPIO54_Disable             gcfg->GCFGMISC &= ~0x00000040
//#define mPROCHOT_GPIO08_Enable              gcfg->GCFGMISC |= 0x00000080
//#define mPROCHOT_GPIO08_Disable             gcfg->GCFGMISC &= ~0x00000080
#define mFDA_Enable                         gcfg->GCFGMISC |= 0x00000100
#define mFDA_Disable                        gcfg->GCFGMISC &= ~0x00000100
#define mFDA_Mode_Set(mod)                  gcfg->GCFGMISC = gcfg->GCFGMISC&(~0x00000C00)|((unsigned short)mod<<10)
#define mESPI_CLK_16mA_Enable               gcfg->GCFGMISC |= 0x00000200
#define mESPI_CLK_16mA_Disable              gcfg->GCFGMISC &= ~0x00000200
#define mSWD_Enable                         gcfg->GCFGMISC |= 0x00001000
#define mSWD_Disable                        gcfg->GCFGMISC &= ~0x00001000
//#define mBootTimer_Disable                  gcfg->GCFGMISC &= ~0x00002000
//#define mGPIO50_Lock_Enable                 gcfg->GCFGMISC |= 0x00004000
//#define mGPIO50_Lock_Disable                gcfg->GCFGMISC &= ~0x00004000
//#define mSPIH_GPIO5x_Inhibit_Enable         gcfg->GCFGMISC |= 0x00008000
//#define mSPIH_GPIO5x_Inhibit_Disable        gcfg->GCFGMISC &= ~0x00008000
#define mSPIH_SHRRom_Pinout_Inhibit_Enable  gcfg->GCFGMISC |= 0x00008000
#define mSPIH_SHRRom_Pinout_Inhibit_Disable gcfg->GCFGMISC &= ~0x00008000
//#define mGA20_OD_Enable                     gcfg->GCFGMISC |= 0x00010000
//#define mGA20_OD_Disable                    gcfg->GCFGMISC &= ~0x00010000
//CPU Extended Command
#define mCPU_EXTCMD_Enable                  gcfg->EXTIE |= 0x01
#define mCPU_EXTCMD_Disable                 gcfg->EXTIE &= ~0x01
#define mCPU_EXTCMD_Clear_PF                gcfg->EXTPF = 0x01
#define mGet_EXTCMD_PF                      (gcfg->EXTPF&0x01)
#define mSet_EXTCMD(cmd)                    gcfg->EXTCMD = cmd
#define mSet_EXTARG(arg)                    gcfg->EXTARG = arg
#define mGet_EXTCMD                         gcfg->EXTCMD
#define mGet_EXTARG                         gcfg->EXTARG
#define mCPU_EXTCMD_Clr                     gcfg->EXTCMD = 0x00
#define mCPU_EXTARG_Clr                     gcfg->EXTARG = 0x00UL
                 
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void GCFG_Init(void);
void CPU_EXTCMD_Handle_ISR(void);

//-- For OEM Use -----------------------------------------------

#endif //GCFG_H
