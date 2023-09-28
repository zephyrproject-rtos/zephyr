/****************************************************************************
 * @file     gcfg.h
 * @brief    Define GCFG's function
 *
 * @version  V1.0.0
 * @date     02. July 2021
****************************************************************************/

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

#endif //GCFG_H
