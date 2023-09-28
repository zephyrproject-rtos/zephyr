/****************************************************************************
 * @file     spih.h
 * @brief    Define SPIH's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
****************************************************************************/

#ifndef SPIH_H
#define SPIH_H

/**
  \brief  Structure type to access SPI Host Controller (SPIH).
 */
typedef volatile struct
{
  uint8_t  SPIHCFG;                                   /*Configuration Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  SPIHCTR;                                   /*Control Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint16_t SPIHTBUF;                                  /*Transmit data buffer Register */
  uint16_t Reserved2;                                 /*Reserved */
  uint16_t SPIHRBUF;                                  /*Receive data buffer Register */
  uint16_t Reserved3;                                 /*Reserved */
}  SPIH_T;

//-- Constant Define -------------------------------------------
#define SPIHDI0_GPIO78                  0
#define SPIHDI1_GPIO6B                  1

#define SPIH_Mode0                      0x00
#define SPIH_Mode1                      0x01
#define SPIH_Mode2                      0x02
#define SPIH_Mode3                      0x03

#define SPIH_Clock_16M                  0x00
#define SPIH_Clock_8M                   0x01
#define SPIH_Clock_4M                   0x02
#define SPIH_Clock_2M                   0x03
#define SPIH_Clock_1M                   0x04
#define SPIH_Clock_500K                 0x05

#define SPIH_PINOUT_No_Change           0x00
#define SPIH_PINOUT_KSO                 0x01
#define SPIH_PINOUT_SHR_ROM             0x02
#define SPIH_PINOUT_Normal              0x03

#define SPIH_DUMMY_BYTE                 0xFF

//-- Macro Define -----------------------------------------------
#define mSPIH_BUSY                      (spih->SPIHCTR&0x80)

#define mSPIH_CSLow                     spih->SPIHCTR |= 0x01
#define mSPIH_CSHigh                    spih->SPIHCTR &= ~0x01

#define mSPIH_ModuleEnable              spih->SPIHCFG |= 0x01
#define mSPIH_ModuleDisable             spih->SPIHCFG &= ~0x01
#define mSPIH_Mode_Selection(mode)      spih->SPIHCFG = (spih->SPIHCFG&0xCF)|((mode&0x03)<<4)
#define mSPIH_Clock_Selection(clock)    spih->SPIHCFG = (spih->SPIHCFG&0xF1)|((clock&0x07)<<1)
#define mSPIH_CS_OpenDrain_Enable       spih->SPIHCFG &= ~0x40
#define mSPIH_CS_PushPull_Enable        spih->SPIHCFG |= 0x40

#define mSPIH_Write_TBuf(dat)           spih->SPIHTBUF = dat
#define mGet_SPIH_Read_RBuf             spih->SPIHRBUF
//Nick 20221207
#define mSPIH_MOSI_LSB_Keep_Enable      spih->SPIHCFG |= 0x80
#define mSPIH_MOSI_LSB_Keep_Disable     spih->SPIHCFG &= ~0x80

#define mSPIH_BufferOneByte_Set         spih->SPIHCTR &= ~0x02
#define mSPIH_BufferTwoByte_Set         spih->SPIHCTR |= 0x02

#endif //SPIH_H
