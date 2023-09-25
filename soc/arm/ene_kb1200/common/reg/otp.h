/**************************************************************************//**
 * @file     otp.h
 * @brief    Define otp's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef OTP_H
#define OTP_H

/**
  \brief  Structure type to access Debug Port Interface - 0x80/0x81 (DBI).
 */
typedef struct
{
  uint16_t OTPCFG;                                    /*OTP Configuration Register */
  uint16_t Reserved;                                  /*Reserved */
  uint32_t OTPCTR;                                    /*OTP Control Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  OTPRPC;                                    /*OTP Read Protect Control Register */
  uint32_t OTPTEST;                                   /*OTP Test Mode Control Register */
  uint32_t Reserved3[28];                             /*Reserved */
  uint8_t  OTPRDAT[256];                              /*OTP Read Data Register */
} OTP_T;

#define otp                 ((OTP_T *) OTP_BASE)                    /* DBI Struct       */
#define OTP                 ((volatile unsigned long*) OTP_BASE)    /* DBI  array       */

//***************************************************************
// User Define                                                
//***************************************************************

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
#define OTPRPC_OFFSET                        0x02

//-- Macro Define -----------------------------------------------
#define mOTP_Enable                     otp->OTPCFG |= 0x01
#define mOTP_Disable                    otp->OTPCFG &= ~0x01
#define mOTP_Busy                       (otp->OTPCFG&0x0100)
#define mGet_InvaidWrite_Event          (otp->OTPCFG&0x0200)

#define mOTP_Data_Write(offset,data)    otp->OTPCTR = ((unsigned short)data<<16)|(offset<<8)|0x01
#define mOTP_ReadProtect_Set            otp->OTPRPC |=0x80

#define mOTP_Data_Read(offset)          otp->OTPRDAT[offset]


//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------

#endif //OTP_H
