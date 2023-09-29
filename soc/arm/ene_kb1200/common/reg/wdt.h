/****************************************************************************
 * @file     wdt.h
 * @brief    Define WDT's function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ****************************************************************************/

#ifndef WDT_H
#define WDT_H

/**
  \brief  Structure type to access Watch Dog Timer (WDT).
 */
typedef volatile struct
{
  uint8_t  WDTCFG;                                    /*Configuration Register */
  uint8_t  WDTCFG_T;                                  /*Configuration Reset Type Register */
  uint16_t Reserved0;                                 /*Reserved */
  uint8_t  WDTIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  WDTPF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint16_t WDTM;                                      /*WDT Match Value Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint8_t  WDTSCR[4];                                 /*FW Scratch(4 bytes) Register */
}  WDT_T;


#define WDT_RESET_WHOLE_CHIP_WO_GPIO        0
#define WDT_RESET_WHOLE_CHIP                1
#define WDT_RESET_ONLY_MCU                  2
#define WDT_ADCO32K                         0x00
#define WDT_PHER32K                         0x02


#endif //WDT_H
