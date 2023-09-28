/**************************************************************************//**
 * @file     ec.h
 * @brief    Define EC's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef EC_H
#define EC_H


/**
  \brief  Structure type to access ECECECx (EC).
 */
typedef volatile struct
{
  uint32_t ECICFG;                                    /*Configuration Register */
  uint8_t  ECIIE;                                     /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  ECIPF;                                    /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint32_t Reserved2;                                 /*Reserved */
  uint8_t  ECISTS;                                   /*Status Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  ECICMD;                                   /*Command Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  ECIODP;                                   /*Output Data Port Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  ECIIDP;                                   /*Input Dtat Port Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  ECISCID;                                  /*SCI ID Write Port Register */
  uint8_t  Reserved7[3];                              /*Reserved */
}  EC_T;


#define EC_Read_CMD                         0x80
#define EC_Write_CMD                        0x81
#define EC_Burst_Enable_CMD                 0x82
#define EC_Burst_Disable_CMD                0x83
#define EC_Query_CMD                        0x84


#endif //EC_H
