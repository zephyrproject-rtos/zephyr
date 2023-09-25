/**************************************************************************//**
 * @file     wdt.h
 * @brief    Define WDT's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

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

//#define wdt                ((WDT_T *) WDT_BASE)             /* WDT Struct       */

//***************************************************************
// User Define                                                
//***************************************************************
//#define WDTResetSetting                     USR_WDTResetSetting

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define WDT_RESET_WHOLE_CHIP_WO_GPIO        0
#define WDT_RESET_WHOLE_CHIP                1
#define WDT_RESET_ONLY_MCU                  2
#define WDT_ADCO32K                         0x00
#define WDT_PHER32K                         0x02

//-- Macro Define -----------------------------------------------
//#define mWDT_All_PendFlagClr                wdt->WDTPF = 0x03
//#define mWDT_Reset_PendFlagClr              wdt->WDTPF = 0x02
//#define mWDT_Event_PendFlagClr              wdt->WDTPF = 0x01
//#define mWDT_Get_All_PF                     (wdt->WDTPF&0x03)
//#define mWDT_Get_Reset_PF                   (wdt->WDTPF&0x02)
//#define mWDT_Get_Event_PF                   (wdt->WDTPF&0x01)
//#define mWDT_Set_Reset_Type(tp)             wdt->WDTCFG_T = tp
//#define mWDT_Set_Clock_Source(src)          wdt->WDTCFG = (wdt->WDTCFG&(~0x02))|src

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void WDT_Init(void);
void WDT_Wait_Reset(unsigned char);
void EnESrvcWDT(void);

//-- For OEM Use -----------------------------------------------
void Enable_WDT_Clock_Source(unsigned char);
void Disable_WDT_Clock_Source(unsigned char);
void WDT_CntSetStart(unsigned short wResetCnt);
void WDT_Clear(void);
void WDT_Stop(void);

#endif //WDT_H
