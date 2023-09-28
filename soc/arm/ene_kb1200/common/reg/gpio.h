/****************************************************************************
 * @file     gpio.h
 * @brief    Define GPIO's registers
 *
 * @version  V1.0.0
 * @date     02. July 2021
****************************************************************************/

#ifndef GPIO_H
#define GPIO_H


/**
  \brief  Structure type to access General Purpose Input/Output (GPIO).
 */
typedef volatile struct
{
  uint32_t GPIOFSxx[4];                               /*Function Selection Register */
  uint32_t GPIOOExx[4];                               /*Output Enable Register */
  uint32_t GPIODxx[4];                                /*Output Data Register */
  uint32_t GPIOINxx[4];                               /*Input Data Register */
  uint32_t GPIOPUxx[4];                               /*Pull Up Register */
  uint32_t GPIOODxx[4];                               /*Open Drain Register */
  uint32_t GPIOIExx[4];                               /*Input Enable Register */
  uint32_t GPIODCxx[4];                               /*Driving Control Register */
  uint32_t GPIOLVxx[4];                               /*Low Voltage Mode Enable Register */
  uint32_t GPIOPDxx[4];                               /*Pull Down Register */
  uint32_t GPIOFLxx[4];                               /*Function Lock Register */
}  GPIO_T;

//-- Constant Define --------------------------------------------
#define GPIO00_PWMLED0_PWM8             0x00
#define GPIO01_SERRXD1_UARTSIN          0x01
#define GPIO03_SERTXD1_UARTSOUT         0x03
#define GPIO22_ESBDAT_PWM9              0x22
#define GPIO28_32KOUT_SERCLK2           0x28
#define GPIO36_UARTSOUT_SERTXD2         0x36
#define GPIO5C_KSO6_P80DAT              0x5C
#define GPIO5D_KSO7_P80CLK              0x5D
#define GPIO5E_KSO8_SERRXD1             0x5E
#define GPIO5F_KSO9_SERTXD1             0x5F
#define GPIO71_SDA8_UARTRTS             0x71
#define GPIO38_SCL4_PWM1                0x38

#define GPIO_FS_OFFSET                  0x00
#define GPIO_OE_OFFSET                  0x04
#define GPIO_DO_OFFSET                  0x08
#define GPIO_IN_OFFSET                  0x0C
#define GPIO_PU_OFFSET                  0x10
#define GPIO_OD_OFFSET                  0x14
#define GPIO_IE_OFFSET                  0x18
#define GPIO_DC_OFFSET                  0x1C
#define GPIO_LV_OFFSET                  0x20
#define GPIO_PD_OFFSET                  0x24
#define GPIO_FL_OFFSET                  0x28

//GPIO Configuration Define
#define GPIO_bTYPE_BITNum               10//8
#define GPIO_bTYPE_FS                   0x01
#define GPIO_bTYPE_OE                   0x02
#define GPIO_bTYPE_DO                   0x04
#define GPIO_bTYPE_PU                   0x08
#define GPIO_bTYPE_OD                   0x10
#define GPIO_bTYPE_IE                   0x20
#define GPIO_bTYPE_LV                   0x40
#define GPIO_bTYPE_DC                   0x80
#define GPIO_bTYPE_PD                   0x100
#define GPIO_bTYPE_FL                   0x200

#endif //GPIO_H
