/**************************************************************************//**
 * @file     uart.h
 * @brief    Define UART's register and function
 *           
 * @version  V1.0.0
 * @date     02. July 2021
 ******************************************************************************/

#ifndef UART_H
#define UART_H

/**
  \brief  Structure type to access Universal Asynchronous Receiver/Transmitter (UART).
 */
typedef volatile struct
{
  uint32_t UARTCFG;                                   /*Configuration Register */
  uint8_t  UARTIE;                                    /*Interrupt Enable Register */
  uint8_t  Reserved0[3];                              /*Reserved */
  uint8_t  UARTSIRQ;                                  /*SIRQ Register */
  uint8_t  Reserved1[3];                              /*Reserved */
  uint8_t  UARTTRV_THR;                               /*TX/RX Value Register(UARTTHR value) */        
  uint8_t  UARTTRV_FCR;                               /*TX/RX Value Register(UARTFCR value) */
  uint8_t  Reserved2[2];                              /*Reserved */
  uint8_t  UARTDAT;                                   /*Receive/Transmitter Register */
  uint8_t  Reserved3[3];                              /*Reserved */
  uint8_t  UARTEE;                                    /*Event Enable Register */
  uint8_t  Reserved4[3];                              /*Reserved */
  uint16_t UARTDIV;                                   /*Divisor Latch Register */
  uint16_t Reserved5;                                 /*Reserved */
  uint8_t  UARTFCR_UARTIIR;                           /*FIFO Control Register */
  uint8_t  Reserved6[3];                              /*Reserved */
  uint8_t  UARTLCR;                                   /*Line Control Register */
  uint8_t  Reserved7[3];                              /*Reserved */
  uint8_t  UARTMCR;                                   /*Modem Control Register */
  uint8_t  Reserved8[3];                              /*Reserved */
  uint8_t  UARTLSR;                                   /*Line Status Register */
  uint8_t  Reserved9[3];                              /*Reserved */
  uint8_t  UARTMSR;                                   /*Modem Status Register */
  uint8_t  Reserved10[3];                             /*Reserved */
  uint8_t  UARTSCR;                                   /*Scratch Register */
  uint8_t  Reserved11[3];                             /*Reserved */
}  UART_T;

#define uart                ((UART_T *) UART_BASE)             /* UART Struct       */

#define UART                ((volatile unsigned long  *) UART_BASE)     /* UART  array       */

//***************************************************************
// User Define                                                
//***************************************************************
#define HIF_UARTEnable                  USR_HIF_UARTEnable      // 0: No Support; 1: Support
    #define HIF_UARTIOaddress           USR_HIF_UARTIOaddress   //IO address
    #define HIF_UART_SIRQ_En            USR_HIF_UART_SIRQ_En    //0: SIRQ Enable, 1: SIRQ Disable
    #define HIF_UART_SIRQNUM            USR_HIF_UART_SIRQNUM    //0~15
    #define HIF_UART_PINOUT             USR_HIF_UART_PINOUT     //0: Full Function Pinout; 1: Only UART_SOUT/UART_SIN
//***************************************************************
//  Kernel Define                                             
//***************************************************************
//-- Constant Define -------------------------------------------
//#define UART_TX_GPIO_Num                0x16
//#define UART_RX_GPIO_Num                0x17
//#define UART_TX2_GPIO_Num               0x48
//#define UART_RX2_GPIO_Num               0x49

#define UART_RI_GPIO_Num                0x0D
#define UART_SIN_GPIO_Num               0x28
#define UART_DCD_GPIO_Num               0x34
#define UART_DSR_GPIO_Num               0x35
#define UART_CTS_GPIO_Num               0x70

#define UART_DTR_GPIO_Num               0x15
#define UART_SOUT_GPIO_Num              0x36
#define UART_RTS_GPIO_Num               0x71
#define UART_CTS_GPIO_Num               0x70

//-- Macro Define -----------------------------------------------
#define mUART_DLAB_Enable               uart->UARTLCR |= 0x80
#define mUART_DLAB_Disable              uart->UARTLCR &=~0x80
#define mUART_Divisor_Set(value)        uart->UARTDIV  = value

#define mUART_Interrupt_Enable          uart->UARTIE |= 0x01
#define mUART_Interrupt_Disable         uart->UARTIE &=~0x01

#define mUART_Function_Enable           uart->UARTCFG |= 0x01
#define mUART_Function_Disable          uart->UARTCFG &=~0x01

#define mUART_MCU_clr_Flag_Enable       uart->UARTCFG |= 0x10

#define mUART_Full_Pinout_Enable        uart->UARTCFG &=~0x4000 
#define mUART_Full_Pinout_Disable       uart->UARTCFG |= 0x4000

//***************************************************************
//  Extern Varible Declare                                          
//***************************************************************

//***************************************************************
//  Extern Function Declare                                          
//***************************************************************
//-- Kernel Use -----------------------------------------------

//-- For OEM Use -----------------------------------------------
#if HIF_UARTEnable
void Init_UART(void);
void UART_GPIO_ENABLE(void);
void UART_GPIO_DISABLE(void);
#endif  //HIF_UARTEnable

#endif //UART_H
