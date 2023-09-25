/**************************************************************************//**
 * @file     espipher.h
 * @brief    Define ESPIPHER's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ESPIPHER_H
#define ESPIPHER_H

/**
  \brief  Structure type to access ESPI Peripheral (ESPIPHER).
 */
typedef volatile struct
{
  uint32_t Reserved0[2];                              /*Reserved */
  uint32_t ESPIPHEF;                                  /*Error Flag Register */
}  ESPIPHER_T;

//#define espipher                ((ESPIPHER_T *) ESPIPHER_BASE)             /* ESPIPHER Struct       */
//#define ESPIPHER                ((volatile unsigned long  *) ESPIPHER_BASE)     /* ESPIPHER  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define SUPPORT_ESPIPH              1

//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------

//-- Macro Define -----------------------------------------------

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------

#endif //ESPIPHER_H
