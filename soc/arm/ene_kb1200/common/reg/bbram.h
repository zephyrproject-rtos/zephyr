/**************************************************************************//**
 * @file     bbram.h
 * @brief    Define BBRAM's function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/
 
#ifndef BBRAM_H
#define BBRAM_H

/**
  \brief  Structure type to access BBRAM.
 */
typedef volatile struct
{
  uint32_t BKUPSTS;                              /*VBAT Battery-Backed Status Register */
  uint8_t  BBRM[0x7C];                           /*VBAT Battery-Backed RAM Register */
  uint32_t INTRCTR;                              /*VBAT INTRUSION# Control Register */
}  BBRAM_T;

#define KB1200_BBRAM_SIZE 0x7C

#endif //BBRAM_H
