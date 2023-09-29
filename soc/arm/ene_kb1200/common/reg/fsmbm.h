/****************************************************************************
 * @file     fsmbm.h
 * @brief    Define FSMBM's register and function
 *
 * @version  V1.0.0
 * @date     02. July 2021
 ****************************************************************************/

#ifndef FSMBM_H
#define FSMBM_H

/**
  \brief  Structure type to access Flexible SMBus Master (FSMBM).
 */
typedef volatile struct
{
  uint32_t FSMBMCFG;                                  /*Configuration Register */
  uint8_t  FSMBMIE;                                   /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  FSMBMPF;                                   /*Event Pending Flag Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  FSMBMFRT;                                  /*Protocol Control Register */
  uint8_t  Reserved2[3];                              /*Reserved */
  uint16_t FSMBMPEC;                                  /*PEC Value Register */
  uint16_t Reserved3;                                 /*Reserved */
  uint8_t  FSMBMSTS;                                  /*Status Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint8_t  FSMBMADR;                                  /*Slave Address Register */
  uint8_t  Reserved5[3];                              /*Reserved */
  uint8_t  FSMBMCMD;                                  /*Command Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  FSMBMDAT[32];                              /*Data Register */
  uint8_t  FSMBMPRTC_P;                               /*Protocol Register */
  uint8_t  FSMBMPRTC_C;                               /*Protocol  Register */
  uint16_t Reserved7;                                 /*Reserved */
  uint8_t  FSMBMNADR;                                 /*HostNotify Slave Address Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint16_t FSMBMNDAT;                                 /*HostNotify Data Register */
  uint16_t Reserved9;                                 /*Reserved */
}  FSMBM_T;

//-- Constant Define -------------------------------------------
#define FSMBM_NUM                       10
#define FSMBM0                          0x00
#define FSMBM1                          0x01
#define FSMBM2                          0x02
#define FSMBM3                          0x03
#define FSMBM4                          0x04
#define FSMBM5                          0x05
#define FSMBM6                          0x06
#define FSMBM7                          0x07
#define FSMBM8                          0x08
#define FSMBM9                          0x09
#define FSMSM_NoMapping                 0xFE

#define FSMBM0_CLK_GPIO_Num             0x2C
#define FSMBM0_DAT_GPIO_Num             0x2D
#define FSMBM1_CLK_GPIO_Num             0x2E
#define FSMBM1_DAT_GPIO_Num             0x2F
#define FSMBM2_CLK_GPIO_Num             0x32
#define FSMBM2_DAT_GPIO_Num             0x33
#define FSMBM3_CLK_GPIO_Num             0x34
#define FSMBM3_DAT_GPIO_Num             0x35
#define FSMBM4_CLK_GPIO_Num             0x38
#define FSMBM4_DAT_GPIO_Num             0x39
#define FSMBM5_CLK_GPIO_Num             0x4A
#define FSMBM5_DAT_GPIO_Num             0x4B
#define FSMBM6_CLK_GPIO_Num             0x4C
#define FSMBM6_DAT_GPIO_Num             0x4D
#define FSMBM7_CLK_GPIO_Num             0x50
#define FSMBM7_DAT_GPIO_Num             0x51
#define FSMBM8_CLK_GPIO_Num             0x70
#define FSMBM8_DAT_GPIO_Num             0x71
#define FSMBM9_CLK_GPIO_Num             0x72
#define FSMBM9_DAT_GPIO_Num             0x73

//PROTOCOL
//H/W SMBUS Protocol 0x00~0x1F
#define SMBUS_QUICK_WRITE               0x02
#define SMBUS_QUICK_READ                0x03
#define SMBUS_SEND_BYTE                 0x04
#define SMBUS_RECEIVE_BYTE              0x05
#define SMBUS_WRITE_BYTE                0x06
#define SMBUS_READ_BYTE                 0x07
#define SMBUS_WRITE_WORD                0x08
#define SMBUS_READ_WORD                 0x09
#define SMBUS_WRITE_BLOCK               0x0A
#define SMBUS_READ_BLOCK                0x0B
#define SMBUS_WORD_PROCESS              0x0C
#define SMBUS_BLOCK_PROCESS             0x0D
#define SMBUS_HW_PROTOCOL_MAX_VALUE     0x1F
//I2C Protocol with 1 byte length 0x20~0x2F
#define I2C_WRITE_BLOCK                 0x20
#define I2C_WRITE_BLOCK_CMD             0x22
#define I2C_READ_BLOCK                  0x21
#define I2C_READ_BLOCK_CNT              0x23
#define I2C_WRITE_READ                  0x24
#define I2C_WRITE_READ_CNT              0x25
#define I2C_1BL_PROTOCOL_MAX_VALUE      0x2F
//SMBUS3.0 Protocol with 1 byte length 0x30~0x3F
#define SMBUS30_WRITE_32                0x34
#define SMBUS30_READ_32                 0x35
#define SMBUS30_WRITE_64                0x38
#define SMBUS30_READ_64                 0x39
#define SMBUS30_BLOCK_PROCESS           0x3D
#define SMBUS30_PROTOCOL_MAX_VALUE      0x3F
//I2C Protocol with 2 bytes length 0x40~0x4F
#define I2C_READ_BLOCK_CNT2L            0x41                //The length includes itself for HIDoverI2C
#define I2C_WRITE_READ_CNT2L            0x42                //The length includes itself for HIDoverI2C
#define I2C_2BL_PROTOCOL_MAX_VALUE      0x4F
//Flexible Protocol 0x7F with 1 byte length
#define FLEXIBLE_PROTOCOL               0x7F

//Error code
#define FSMBM_NO_ERROR                  0x00
#define FSMBM_DEVICE_ADDR_NO_ACK        0x10
#define FSMBM_CMD_NO_ACK                0x12
#define FSMBM_DEVICE_DATA_NO_ACK        0x13
#define FSMBM_LOST_ARBITRATION          0x17
#define FSMBM_SMBUS_TIMEOUT             0x18
#define FSMBM_UNSUPPORTED_PRTC          0x19
#define FSMBM_SMBUS_BUSY                0x1A
#define FSMBM_STOP_FAIL                 0x1E
#define FSMBM_PEC_ERROR                 0x1F
//Packet Form
#define ___NONE                         0x00
#define ___STOP                         0x01
#define __PEC_                          0x02
#define __PEC_STOP                      0x03
#define _CNT__                          0x04
#define _CNT__STOP                      0x05
#define _CNT_PEC_                       0x06
#define _CNT_PEC_STOP                   0x07
#define CMD___                          0x08
#define CMD___STOP                      0x09
#define CMD__PEC_                       0x0A
#define CMD__PEC_STOP                   0x0B
#define CMD_CNT__                       0x0C
#define CMD_CNT__STOP                   0x0D
#define CMD_CNT_PEC_                    0x0E
#define CMD_CNT_PEC_STOP                0x0F

#define FLEXIBLE_CMD                    0x08
#define FLEXIBLE_CNT                    0x04
#define FLEXIBLE_PEC                    0x02
#define FLEXIBLE_STOP                   0x01
//HW
#define FSMBM_BufferSize                0x20
#define FSMBM_Write                     0x00
#define FSMBM_Read                      0x01
//hwFSMBM.Flag
#define FSMBM_BUSY                      0x80
#define FSMBM_R_PRTO                    0x40
#define FSMBM_WRITE_READ                0x20
#define FSMBM_TIMEOUT                   0x10
#define FSMBM_FIRST                     0x08
#define FSMBM_2BL                       0x04

//Timming for F/W Time Out
#define FSMBM_msTimeOut                 100     // Unit: ms, Range: 7.5~1912.5 , Base Clock: 7.5ms(GPT1), Max. deviation: 3.75ms
//Clock Setting = 1 / (1u + (1u * N))  //50% Duty Cycle(bit7 == 0)
#define FSMBM_CLK_1M                    0
#define FSMBM_CLK_500K                  1
#define FSMBM_CLK_333K                  2
#define FSMBM_CLK_250K                  3
#define FSMBM_CLK_200K                  4
#define FSMBM_CLK_167K                  5
#define FSMBM_CLK_143K                  6
#define FSMBM_CLK_125K                  7
#define FSMBM_CLK_111K                  8
#define FSMBM_CLK_100K                  9
#define FSMBM_CLK_91K                   10
#define FSMBM_CLK_83K                   11
#define FSMBM_CLK_71K                   13
#define FSMBM_CLK_63K                   15
#define FSMBM_CLK_50K                   19
#define FSMBM_CLK_40K                   24
#define FSMBM_CLK_30K                   32
#define FSMBM_CLK_20K                   49
#define FSMBM_CLK_10K                   99
//Other(bit7 == 1; bit6~4 = clock high length; bit3~0 = clock low length)
#define FSMBM_CLK_400K                  (0x12|0x80)


#endif //FSMBM_H
