/**************************************************************************//**
 * @file     gptd.h
 * @brief    Define GPTD's registers
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef GPTD_H
#define GPTD_H

/**
  \brief  Structure type to access GPIO Trigger Detector (GPTD).
 */
typedef volatile struct
{
  uint32_t GPTDIExx[4];                               /*Interrupt Enable Register */
  uint32_t GPTDPFxx[4];                               /*Event Pending Flag Register */
  uint32_t GPTDCHGxx[4];                              /*Change Trigger Register */
  uint32_t GPTDELxx[4];                               /*Level/Edge Trigger Register */
  uint32_t GPTDPSxx[4];                               /*Polarity Selection Register */
  uint32_t GPTDWExx[4];                               /*WakeUP Enable Register */
}  GPTD_T;

#define gptd                ((GPTD_T *) GPTD_BASE)          /* GPTD Struct       */

#define GPTD                ((volatile unsigned long  *) GPTD_BASE)     /* GPTD  array       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define --------------------------------------------
#define GPTD_GPIO000F_GP                    0x00
#define GPTD_GPIO101F_GP                    0x01
#define GPTD_GPIO202F_GP                    0x02
#define GPTD_GPIO303F_GP                    0x03
#define GPTD_GPIO404F_GP                    0x04
#define GPTD_GPIO505F_GP                    0x05
#define GPTD_GPIO606F_GP                    0x06
#define GPTD_GPIO707F_GP                    0x07

#define GPTD_IE_OFFSET                      0x00
#define GPTD_PF_OFFSET                      0x04
#define GPTD_CHG_OFFSET                     0x08
#define GPTD_EL_OFFSET                      0x0C
#define GPTD_PS_OFFSET                      0x10
#define GPTD_WE_OFFSET                      0x14
//GPTD Configuration Define                 
#define GPTD_bTYPE_BITNum                   4       //except IE and PF
#define GPTD_bTYPE_IE                       0x01
#define GPTD_bTYPE_PF                       0x02
#define GPTD_bTYPE_CHG                      0x04
#define GPTD_bTYPE_EL                       0x08
#define GPTD_bTYPE_PS                       0x10
#define GPTD_bTYPE_WE                       0x20

//-- Macro Define -----------------------------------------------
#define mCLEARGPTDPF(gpio_group,clear_flag)         gptd->GPTDPFxx[gpio_group] = clear_flag
#define mGETGPTD_INT_VALIDFLAG(gpio_group)          (gptd->GPTDIExx[gpio_group]&gptd->GPTDPFxx[gpio_group])
#define mGETGPTD_WU_VALIDFLAG(gpio_group)           (gptd->GPTDWExx[gpio_group]&gptd->GPTDPFxx[gpio_group])
#define mSETGPTD_WU(gpio_group,wu)                  gptd->GPTDWExx[gpio_group] |= wu
#define mCLEARGPTD_WU(gpio_group,wu)                gptd->GPTDWExx[gpio_group] &= ~wu

#define mSETGPTD_PS(gpio_group,ps)                  gptd->GPTDPSxx[gpio_group] |= ps
#define mCLEARGPTD_PS(gpio_group,ps)                gptd->GPTDPSxx[gpio_group] &= ~ps

#define mGPTDIE_Dis(GPIONum)                        gptd->GPTDIExx[GPIONum>>4] &= ~(1UL<<((GPIONum)&0x0F))
#define mGPTDIE_En(GPIONum)                         gptd->GPTDIExx[GPIONum>>4] |=  (1UL<<((GPIONum)&0x0F))
#define mGPTDPF_Clr(GPIONum)                        gptd->GPTDPFxx[GPIONum>>4]  =  (1UL<<((GPIONum)&0x0F))
#define mGPTDPF_Get(GPIONum)                        ((gptd->GPTDPFxx[GPIONum>>4] & (1UL<<((GPIONum)&0x0F)))?1:0)
#define mGPTDPS_Low(GPIONum)                        gptd->GPTDPSxx[GPIONum>>4] &= ~(1UL<<((GPIONum)&0x0F))
#define mGPTDPS_High(GPIONum)                       gptd->GPTDPSxx[GPIONum>>4] |=  (1UL<<((GPIONum)&0x0F))
#define mGPTDPS_Chg(GPIONum)                        gptd->GPTDPSxx[GPIONum>>4] ^=  (1UL<<((GPIONum)&0x0F))
#define mGPTDPS_Get(GPIONum)                        ((gptd->GPTDPSxx[GPIONum>>4] & (1UL<<((GPIONum)&0x0F)))?1:0)
#define mGPTDEL_Edge(GPIONum)                       gptd->GPTDELxx[GPIONum>>4] &= ~(1UL<<((GPIONum)&0x0F))
#define mGPTDEL_Level(GPIONum)                      gptd->GPTDELxx[GPIONum>>4] |=  (1UL<<((GPIONum)&0x0F))
#define mGPTDCHG_Dis(GPIONum)                       gptd->GPTDCHGxx[GPIONum>>4] &= ~(1UL<<((GPIONum)&0x0F))
#define mGPTDCHG_En(GPIONum)                        gptd->GPTDCHGxx[GPIONum>>4] |=  (1UL<<((GPIONum)&0x0F))
#define mGPTDWE_Dis(GPIONum)                        gptd->GPTDWExx[GPIONum>>4] &= ~(1UL<<((GPIONum)&0x0F))
#define mGPTDWE_En(GPIONum)                         gptd->GPTDWExx[GPIONum>>4] |=  (1UL<<((GPIONum)&0x0F))
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
void GPTD_TypeConfig(unsigned char bGPIONum, unsigned char bType);

#endif //GPTD_H
