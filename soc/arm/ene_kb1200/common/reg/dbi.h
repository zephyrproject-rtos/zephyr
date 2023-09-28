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


#endif //DBI_H
