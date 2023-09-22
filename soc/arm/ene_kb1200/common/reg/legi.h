/**************************************************************************//**
 * @file     legi.h
 * @brief    Define LEGI's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef LEGI_H
#define LEGI_H

/**
  \brief  Structure type to access Legacy Interface/Port - 68h/6Ch and 78h/7Ch (LEGI).
 */
typedef volatile struct
{
  uint32_t LEGICFG;                                   /*Configuration Register */
  uint8_t  LEGIIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  LEGIPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2;                                 /*Reserved */           
  uint8_t  LEGISTS;                                   /*Status Register */
  uint8_t  Reserved3[3];                              /*Reserved */  
  uint8_t  LEGICMD;                                   /*Command Port Register */
  uint8_t  Reserved4[3];                              /*Reserved */ 
  uint8_t  LEGIODP;                                   /*Output Data Port Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  LEGIIDP;                                   /*Input Data Port Register */
  uint8_t  Reserved6[3];                              /*Reserved */                       
}  LEGI_T;

//#define legi0                ((LEGI_T *) LEGI0_BASE)             /* LEGI Struct       */
//#define legi1                ((LEGI_T *) LEGI1_BASE)             /* LEGI Struct       */
//#define LEGI                ((volatile unsigned long  *) LEGI_BASE)     /* LEGI  array       */

//***************************************************************
// User Define                                                
//***************************************************************
// Legacy IO(68/6C,78/7C) Port Setting
#define HIF_LEGI_V_OFFSET           USR_HIF_LEGI_V_OFFSET

#define HIF_LegacyIOPort0Enable     USR_HIF_LegacyIOPort0Enable     
#define HIF_LegacyIOPort0BaseAddr   USR_HIF_LegacyIOPort0BaseAddr
#define HIF_LegacyIOPort0DatOffset  USR_HIF_LegacyIOPort0DatOffset  //1~16

#define HIF_LegacyIOPort1Enable     USR_HIF_LegacyIOPort1Enable     
#define HIF_LegacyIOPort1BaseAddr   USR_HIF_LegacyIOPort1BaseAddr
#define HIF_LegacyIOPort1DatOffset  USR_HIF_LegacyIOPort1DatOffset  //1~16

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------

//-- Macro Define -----------------------------------------------
#define mClr_LEGI0_IBF              legi0->LEGISTS = 0x02
#define mLEGI0_OBF_Check            (legi0->LEGISTS&0x01)   // 0: empty  1:full
#define mLEGI0_Event_IBF            (legi0->LEGIPF&0x02)
#define mLEGI0_Event_OBF            (legi0->LEGIPF&0x01)
#define mClr_LEGI0_IBF_Event        legi0->LEGIPF = 0x02
#define mClr_LEGI0_OBF_Event        legi0->LEGIPF = 0x01
#define mLEGI0_Command_Cycle        (legi0->LEGISTS&0x40)
#define mGet_LEGI0_Command          legi0->LEGICMD
#define mGet_LEGI0_Data             legi0->LEGIIDP
#define mLEGI0_Write_Data(dat)      legi0->LEGIODP = dat

#define mClr_LEGI1_IBF              legi1->LEGISTS = 0x02
#define mLEGI1_OBF_Check            (legi1->LEGISTS&0x01)   // 0: empty  1:full
#define mLEGI1_Event_IBF            (legi1->LEGIPF&0x02)
#define mLEGI1_Event_OBF            (legi1->LEGIPF&0x01)
#define mClr_LEGI1_IBF_Event        legi1->LEGIPF = 0x02
#define mClr_LEGI1_OBF_Event        legi1->LEGIPF = 0x01
#define mLEGI1_Command_Cycle        (legi1->LEGISTS&0x40)
#define mGet_LEGI1_Command          legi1->LEGICMD
#define mGet_LEGI1_Data             legi1->LEGIIDP
#define mLEGI1_Write_Data(dat)      legi1->LEGIODP = dat

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void LEGI_Init(void);
#if HIF_LegacyIOPort0Enable
void LEGI0_Handle_ISR(void);
#endif  //HIF_LegacyIOPort0Enable
#if HIF_LegacyIOPort1Enable
void LEGI1_Handle_ISR(void);
#endif  //HIF_LegacyIOPort1Enable

//-- For OEM Use -----------------------------------------------
#if HIF_LegacyIOPort0Enable
void LEGI0_Data_To_Host(unsigned char);
#endif  //HIF_LegacyIOPort0Enable
#if HIF_LegacyIOPort1Enable
void LEGI1_Data_To_Host(unsigned char);
#endif  //HIF_LegacyIOPort1Enable

#endif //LEGI_H
