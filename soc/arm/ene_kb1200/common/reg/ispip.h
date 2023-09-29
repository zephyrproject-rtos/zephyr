/****************************************************************************
 * @file     ispip.h
 * @brief    Define ISPI's function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ****************************************************************************/

#ifndef ISPIP_H
#define ISPIP_H

/**
  \brief  Structure type to access eFlash Protection (ISPIP).
 */
typedef volatile struct
{
  uint32_t ISPIP0ADDR;                                  /*ISPIP0 Protect Address Register */
  uint32_t ISPIP0CR;                                    /*ISPIP0 Protect Control Register */
  uint32_t Reserved0[2];                              /*Reserved */
  uint32_t ISPIP1ADDR;                                  /*ISPIP1 Protect Address Register */
  uint32_t ISPIP1CR;                                    /*ISPIP1 Protect Control Register */
  uint32_t Reserved1[2];                              /*Reserved */
  uint32_t ISPIP2ADDR;                                  /*ISPIP2 Protect Address Register */
  uint32_t ISPIP2CR;                                    /*ISPIP2 Protect Control Register */
  uint32_t Reserved2[2];                              /*Reserved */
  uint32_t ISPIP3ADDR;                                  /*ISPIP3 Protect Address Register */
  uint32_t ISPIP3CR;                                    /*ISPIP3 Protect Control Register */
  uint32_t Reserved3[2];                              /*Reserved */
  uint32_t ISPIP4ADDR;                                  /*ISPIP4 Protect Address Register */
  uint32_t ISPIP4CR;                                    /*ISPIP4 Protect Control Register */
  uint32_t Reserved4[2];                              /*Reserved */
  uint32_t ISPIP5ADDR;                                  /*ISPIP5 Protect Address Register */
  uint32_t ISPIP5CR;                                    /*ISPIP5 Protect Control Register */
  uint32_t Reserved5[2];                              /*Reserved */
  uint32_t ISPIP6ADDR;                                  /*ISPIP6 Protect Address Register */
  uint32_t ISPIP6CR;                                    /*ISPIP6 Protect Control Register */
  uint32_t Reserved6[2];                              /*Reserved */
  uint32_t ISPIP7ADDR;                                  /*ISPIP7 Protect Address Register */
  uint32_t ISPIP7CR;                                    /*ISPIP7 Protect Control Register */
  uint32_t Reserved7;                                 /*Reserved */
  uint32_t ISPIPWPC;
}  ISPIP_T;

//-- Constant Define -------------------------------------------
#define ISPIP_ADDRESS_OFFSET      0
#define ISPIP_CONTROL_OFFSET      1

#define ISPIP_WP                  0x08
#define ISPIP_RP                  0x04

#define ISPIP_ADDR_MAX_VALUE      0x01FF            //512K

//-- Macro Define -----------------------------------------------
#define mISPIP_Protect_Lock(RegionNum)                 ISPIP[ISPIP_CONTROL_OFFSET+(RegionNum<<2)] |= 0x01UL
#define mISPIP_Protect_UnLock(RegionNum)               ISPIP[ISPIP_CONTROL_OFFSET+(RegionNum<<2)] &= (~0x01UL)
#define mISPIP_Protect_Control_Set(RegionNum,Config)   ISPIP[ISPIP_CONTROL_OFFSET+(RegionNum<<2)] = Config
#define mISPIP_Protect_AddrToBase(Address)             ((unsigned long)(Address)>>10)

#define mISPIP_WP_Idle_Low                             ispip->ISPIPWPC = 0x01

#endif //ISPIP_H
