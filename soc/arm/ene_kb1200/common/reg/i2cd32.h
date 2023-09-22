/**************************************************************************//**
 * @file     i2cd32.h
 * @brief    Define I2CD32's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef I2CD32_H
#define I2CD32_H

/**
  \brief  Structure type to access I2C 32-bit Device (I2CD32).
 */
typedef volatile struct
{
  uint32_t I2CDCFG;                                   /*Configuration Register */
  uint8_t  I2CDIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  I2CDPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint8_t  I2CDSTS;                                   /*Status Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  I2CDDAT[16];                               /*Data Register */
  uint8_t  I2CDCMD;                                   /*Command Register */
  uint8_t  Reserved4[3];                              /*Reserved */ 
  uint16_t I2CDCNT;                                   /*Count Register */
  uint16_t Reserved5;                                 /*Reserved */                              
}  I2CD32_T;

#define i2cd32                ((I2CD32_T *) I2CD32_BASE)             /* I2CD32 Struct       */

#define I2CD32                ((volatile unsigned long  *) I2CD32_BASE)     /* I2CD32  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_I2CD32                      USR_SUPPORT_I2CD32
#define SUPPORT_I2CD32_HW_MODE              USR_SUPPORT_I2CD32_HW_MODE
#define SUPPORT_I2CD32_SLV_ADDR             USR_SUPPORT_I2CD32_SLV_ADDR

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------

//-- Macro Define -----------------------------------------------
#define mI2CD32_Function_Enable             i2cd32->I2CDCFG |= 0x01UL
#define mI2CD32_Function_Disable            i2cd32->I2CDCFG &= ~0x01UL

#define mI2CD32_HW_Mode_Enable              i2cd32->I2CDCFG |= 0x02UL
#define mI2CD32_HW_Mode_Disable             i2cd32->I2CDCFG &= ~0x02UL

#define mI2CD32_Slave_Addr_Set(Addr)        i2cd32->I2CDCFG = (i2cd32->I2CDCFG&(~0x00FF0000))|(Addr<<16)

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------
void I2CD32_Init(void);

//-- For OEM Use -----------------------------------------------

#endif //I2CD32_H
