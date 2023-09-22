/**************************************************************************//**
 * @file     omst.h
 * @brief    Define OMST's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef OMST_H
#define OMST_H

/**
  \brief  Structure type to access One Micro Second Timer (OMST).
 */
typedef volatile struct
{
  uint8_t  OMSTIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  OMSTPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */        
  uint16_t OMSTCR;                                    /*Control Register */
  uint16_t Reserved2[3];                              /*Reserved */ 
  uint16_t OMST0MV;                                   /*OMST0 Match Value Register */
  uint16_t Reserved3;                                 /*Reserved */                 
  uint16_t OMST1MV;                                   /*OMST1 Match Value Register */
  uint16_t Reserved4;                                 /*Reserved */
  uint16_t OMST0CNT;                                  /*OMST0 Counter Value Register */
  uint16_t Reserved5;                                 /*Reserved */   
  uint16_t OMST1CNT;                                  /*OMST1 Counter Value Register */
  uint16_t Reserved6;                                 /*Reserved */                        
}  OMST_T;

#define omst                ((OMST_T *) OMST_BASE)             /* OMST Struct       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define OMST0                       0
#define OMST1                       1

#define OMST1_Match_Value           1000                    //us

//-- Macro Define -----------------------------------------------
#define mOMST_ENABLE_INT(x)         omst->OMSTIE |= 1<<x
#define mOMST_DISABLE_INT(x)        omst->OMSTIE &= ~(1<<x)
#define mOMST_GET_PF(x)             (omst->OMSTPF&(1<<x))
#define mOMST_CLEAR_PF(x)           omst->OMSTPF = 1<<x
#define mOMST_START_CNT(x)          omst->OMSTCR |= 1<<x
#define mOMST_STOP_CNT(x)           omst->OMSTCR &= ~(1<<x)
#define mOMST_CLEAR_CNT(x)          omst->OMSTCR |= 0x100<<x
#define mOMST0_SET_VALUE(value)     omst->OMST0MV = value     
#define mOMST1_SET_VALUE(value)     omst->OMST1MV = value
//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
//void OMST1_Init(void);
//void OMST1_ISR(void);

//-- For OEM Use -----------------------------------------------
void usDelay(unsigned short uSecDelayTime);

#endif //OMST_H
