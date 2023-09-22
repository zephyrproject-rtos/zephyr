/**************************************************************************//**
 * @file     itim.h
 * @brief    Define VBAT module's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ITIM_H
#define ITIM_H

/**
  \brief  Structure type to access ITIM Module .
 */
typedef struct
{
  uint32_t  ITIMCFG;                              /*ITIM Configuration Register */ 
  uint32_t  ITIMPF;                               /*ITIM Event Pending Flag Register */
  uint32_t  ITIMM0031;                            /*ITIM Match Value register bit31~00 */ 
  uint32_t  ITIMM3263;                            /*ITIM Match Value register bit63~32 */ 
} ITIM_T;

#define itim                                ((ITIM_T *) ITIM_BASE)                      /* ITIM Struct       */
#define ITIM                                ((volatile unsigned long  *) ITIM_BASE)     /* ITIM  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_ITIM                        USR_SUPPORT_ITIM

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define ITIM_16MHZ          0
#define ITIM_PHER32K        1

//-- Macro Define -----------------------------------------------
#define mITIM_Enable                            itim->ITIMCFG |= 0x01 
#define mITIM_Disable                           itim->ITIMCFG &= ~0x01 
#define mITIM_Interrupt_Enable                  itim->ITIMCFG |= 0x02   
#define mITIM_Interrupt_Disable                 itim->ITIMCFG &= ~0x02  
#define mGet_ITIM_PF                            (itim->ITIMPF&0x01)     
#define mClear_ITIM_PF                          itim->ITIMPF = 0x01
#define mITIM_BaseClock_Set(Source,prescaler)   itim->ITIMCFG = ((itim->ITIMCFG&0x03)|(Source<<2)|(prescaler<<8))
//#define mITIM_MatchValue_Set(mv)                {itim->ITIMM3263= mv; itim->ITIMM0031=mv;}
#define mITIM_MatchValue_Set(mv)                itim->ITIMM0031=mv
#define mITIM_SecToCnt(Sec)                     (Sec)                 

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void EnESrvcITIM(void);

//-- For OEM Use -----------------------------------------------
void ITIM_Init(unsigned char ClockSource, unsigned char Prescaler, unsigned long ExpirationValue);

#endif //ITIM_H
