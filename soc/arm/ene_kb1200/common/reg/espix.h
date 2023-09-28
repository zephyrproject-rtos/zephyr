/**************************************************************************//**
 * @file     espi.h
 * @brief    Define ESPI's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef ESPI_H
#define ESPI_H

/**
  \brief  Structure type to access Enhanced Serial Peripheral Interface (ESPI).
 */
typedef volatile struct
{
  uint32_t Reserved0;                                 /*Reserved */
  uint32_t ESPIID;                                    /*ID Register */
  uint32_t ESPIGENCFG;                                /*General Configuration Register */
  uint32_t Reserved1;                                 /*Reserved */
  uint32_t ESPIC0CFG;                                 /*Channel-0(Peripheral) Configuration Register */
  uint32_t Reserved2[3];                              /*Reserved */
  uint32_t ESPIC1CFG;                                 /*Channel-1(Virtual Wire) Configuration Register */
  uint32_t Reserved3[3];                              /*Reserved */
  uint32_t ESPIC2CFG;                                 /*Channel-2(Out-Of-Band) Configuration Register */
  uint32_t Reserved4[3];                              /*Reserved */
  uint32_t ESPIC3CFG;                                 /*Channel-3(Flash Access) Configuration Register */
  uint32_t ESPIC3CFG2;                                /*Channel-3(Flash Access) Configuration2 Register */
  uint32_t Reserved5[2];                              /*Reserved */
  uint32_t ESPISTA;                                   /*Status Register */
  uint32_t ESPIRF;                                    /*Reset Flag Register */
  uint16_t ESPIGEF;                                   /*Global Error Flag Register */
  uint16_t Reserved6;                                 /*Reserved */
  uint32_t ESPICMD;                                   /*Command Register */
  uint32_t ESPICRC;                                   /*CRC Register */
  uint32_t ESPIRST;                                   /*Reset Register */
}  ESPI_T;


#define ESPI_IO_SINGLE                  0
#define ESPI_IO_SINGLE_DUAL             1
#define ESPI_IO_SINGLE_QUAD             2
#define ESPI_IO_SINGLE_DUAL_QUAD        3
#define ESPI_ALERT_OD                   1
#define ESPI_FREQ_MAX_33M               2

//***************************************************************
// User Define
//***************************************************************
#define ESPI_IO_MODE                ESPI_IO_SINGLE
#define ESPI_FREQ_MAX               ESPI_FREQ_MAX_33M

//-- Constant Define -------------------------------------------
#define ESPI_RST_GPIO_Num               0x07

#define ESPI_FREQ_MAX_20M               0
#define ESPI_FREQ_MAX_25M               1
#define ESPI_FREQ_MAX_33M               2
#define ESPI_FREQ_MAX_50M               3
#define ESPI_FREQ_MAX_66M               4

#define ESPI_IO_SINGLE                  0
#define ESPI_IO_SINGLE_DUAL             1
#define ESPI_IO_SINGLE_QUAD             2
#define ESPI_IO_SINGLE_DUAL_QUAD        3

#define ESPI_ALERT_OD                   1

#define ESPI_SUPPORT_ESPIPH             0x01
#define ESPI_SUPPORT_ESPIVW             0x02
#define ESPI_SUPPORT_ESPIOOB            0x04
#define ESPI_SUPPORT_ESPIFA             0x08

//For OOB and FA
#define ESPI_ERROR_FW_TIMEOUT           0x0200
#define ESPI_ERROR_CH_DISABLE           0x0400


#endif //ESPI_H
