/**************************************************************************//**
 * @file     dbi.h
 * @brief    Define DBI's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef DBI_H
#define DBI_H

/**
  \brief  Structure type to access Debug Port Interface - 0x80/0x81 (DBI).
 */
typedef volatile struct
{
  uint32_t DBICFG;                                    /*Configuration Register */
  uint8_t  DBIIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  DBIPF;                                     /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2;                                 /*Reserved */ 
  uint8_t  DBIODP;                                    /*Output Data Port Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  DBIIDP;                                    /*Input Data Port Register */
  uint8_t  Reserved4[3];                              /*Reserved */             
}  DBI_T;

//#define dbi0                ((DBI_T *) DBI0_BASE)             /* DBI Struct       */
//#define dbi1                ((DBI_T *) DBI1_BASE)             /* DBI Struct       */
//#define DBI                ((volatile unsigned long  *) DBI_BASE)     /* DBI  array       */

//***************************************************************
// User Define                                                
//***************************************************************
// Debug Port 80/81 Setting
#define HIF_BIOSDebugPort0Enable    USR_HIF_BIOSDebugPort0Enable    //0: Disable; 1: Enable
#define HIF_BIOSDebugPort0Address   USR_HIF_BIOSDebugPort0Address   //Base Address
#define HIF_BIOSDebugPort0Interrupt USR_HIF_BIOSDebugPort0Interrupt //0 : Disable

#define HIF_BIOSDebugPort1Enable    USR_HIF_BIOSDebugPort1Enable    //0: Disable; 1: Enable
#define HIF_BIOSDebugPort1Address   USR_HIF_BIOSDebugPort1Address   //Base Address
#define HIF_BIOSDebugPort1Interrupt USR_HIF_BIOSDebugPort1Interrupt //0 : Disable(Default)

// Debug Port Shift Mode
// ShiftMode *first 81 after 80, first high nibble after low byte*
#define HIF_DBI_ShiftMode_7SegEncode        USR_HIF_DBI_ShiftMode_7SegEncode    //0: no encode, 1: encode
#define HIF_DBI_ShiftMode_7SegPolarity      USR_HIF_DBI_ShiftMode_7SegPolarity  //0: common cathode, 1: command anode
#define HIF_DBI_ShiftMode_80_En             USR_HIF_DBI_ShiftMode_80_En         //0: disable debug port 0 to LPC shiftmode, 1:enable debug port 0 to LPC shiftmode
#define HIF_DBI_ShiftMode_81_En             USR_HIF_DBI_ShiftMode_81_En         //0: disable debug port 1 to LPC shiftmode, 1:enable debug port 1 to LPC shiftmode
#define HIF_DBI_ShiftMode_Frequency         USR_HIF_DBI_ShiftMode_Frequency    //0: no encode, 1: encode


//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define HIF_DBI2SC_OFFSET            0x10

//-- Macro Define -----------------------------------------------
#define mClr_DBI0_Event             dbi0->DBIPF = 0x01
#define mGet_DBI0_Event             (dbi0->DBIPF&0x01)
#define mGet_DBI0_Data              dbi0->DBIIDP
#define mDBI0_Write_Data(dat)       dbi0->DBIODP = dat

#define mClr_DBI1_Event             dbi1->DBIPF = 0x01
#define mGet_DBI1_Event             (dbi1->DBIPF&0x01)
#define mGet_DBI1_Data              dbi1->DBIIDP
#define mDBI1_Write_Data(dat)       dbi1->DBIODP = dat

#define mDBI_ShiftMode_En()         {DBI[HIF_DBI2SC_OFFSET]=(HIF_DBI_ShiftMode_Frequency<<8)|(HIF_DBI_ShiftMode_7SegEncode<<7)|(HIF_DBI_ShiftMode_81_En<<5)|(HIF_DBI_ShiftMode_80_En<<4)|(HIF_DBI_ShiftMode_7SegPolarity<<1)|0x09; mGPIO5C_ALT_P80DAT; mGPIO5D_ALT_P80CLK;}
#define mDBI_ShiftMode_Dis()        {DBI[HIF_DBI2SC_OFFSET]=0x00; mGPIO5C_ALT_KSO6; mGPIO5D_ALT_KSO7;}

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void DBI_Init(void);
#if HIF_BIOSDebugPort0Interrupt && HIF_BIOSDebugPort0Enable
void EnESrvcDBI0(void);
#endif  //HIF_BIOSDebugPort1Interrupt && HIF_BIOSDebugPort1Enable
#if HIF_BIOSDebugPort1Interrupt && HIF_BIOSDebugPort1Enable
void EnESrvcDBI1(void);
#endif  //HIF_BIOSDebugPort1Interrupt && HIF_BIOSDebugPort1Enable

//-- For OEM Use -----------------------------------------------
#if HIF_BIOSDebugPort0Enable || HIF_BIOSDebugPort1Enable
void HIF_DisplayPort80(unsigned char);
#endif  //HIF_BIOSDebugPort0Enable || HIF_BIOSDebugPort1Enable

#endif //DBI_H
