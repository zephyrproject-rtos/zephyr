/* This contains a copy of ch32v00x.h and core_riscv.h ch32v00x_conf.h and
 * other misc functions  See copyright notice at end. */

#ifndef __CH32V00x_H
#define __CH32V00x_H

#define CH32V003           1  /* CHANGE ME AS NEEDED */


/*****************************************************************************
	CH32V003 BASICS

	1. Be sure to see configuration section below!

	2. Backend Initialization
		SystemInit();

	3. Arduino-like I/O
		funGpioInitAll();
		funPinMode( PA2, GPIO_CFGLR_OUT_10Mhz_PP );
		funDigitalWrite( PA2, FUN_HIGH );
		funDigitalWrite( PA2, FUN_HIGH );

	4. Delays
		Delay_Us(n)
		Delay_Ms(n)
		DelaySysTick( uint32_t n );

	5. printf
		printf, _write may be semihosted, or printed to UART.

		poll_input, handle_debug_input may be used with semihsoting.

	For UART printf, on:
		CH32V003, Port D5, 115200 8n1
		CH32V203, Port A9, 115200 8n1

	Modifications can be made to SetupUart, or your own version as desired.
*/

/*****************************************************************************
	CH32V003 Fun Configs:

#define FUNCONF_USE_PLL 1               // Use built-in 2x PLL
#define FUNCONF_USE_HSI 1               // Use HSI Internal Oscillator
#define FUNCONF_USE_HSE 0               // Use External Oscillator
#define FUNCONF_HSITRIM 0x10            // Use factory calibration on HSI Trim.
#define FUNCONF_SYSTEM_CORE_CLOCK 48000000  // Computed Clock in Hz (Default only for 003, other chips have other defaults)
#define FUNCONF_HSE_BYPASS 0            // Use HSE Bypass feature (for oscillator input)
#define FUNCONF_USE_CLK_SEC	1			// Use clock security system, enabled by default
#define FUNCONF_USE_DEBUGPRINTF 1
#define FUNCONF_USE_UARTPRINTF  0
#define FUNCONF_NULL_PRINTF 0           // Have printf but direct it "nowhere"
#define FUNCONF_SYSTICK_USE_HCLK 0      // Should systick be at 48 MHz or 6MHz?
#define FUNCONF_TINYVECTOR 0            // If enabled, Does not allow normal interrupts.
#define FUNCONF_UART_PRINTF_BAUD 115200 // Only used if FUNCONF_USE_UARTPRINTF is set.
#define FUNCONF_DEBUGPRINTF_TIMEOUT 160000 // Arbitrary time units
#define FUNCONF_ENABLE_HPE 1            // Enable hardware interrupt stack.  Very good on QingKeV4, i.e. x035, v10x, v20x, v30x, but questionable on 003.
#define FUNCONF_USE_5V_VDD 0            // Enable this if you plan to use your part at 5V - affects USB and PD configration on the x035.
*/

/* Sanity check for when porting old code. */
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
	#if defined(CH32V003)
		#error Cannot define CH32V003 and another arch.
	#endif
#endif

#if !defined(FUNCONF_USE_DEBUGPRINTF) && !defined(FUNCONF_USE_UARTPRINTF)
	#define FUNCONF_USE_DEBUGPRINTF 1
#endif

#if defined(FUNCONF_USE_UARTPRINTF) && FUNCONF_USE_UARTPRINTF && !defined(FUNCONF_UART_PRINTF_BAUD)
	#define FUNCONF_UART_PRINTF_BAUD 115200
#endif

#if defined(FUNCONF_USE_DEBUGPRINTF) && FUNCONF_USE_DEBUGPRINTF && !defined(FUNCONF_DEBUGPRINTF_TIMEOUT)
	#define FUNCONF_DEBUGPRINTF_TIMEOUT 160000
#endif

#if defined(FUNCONF_USE_HSI) && defined(FUNCONF_USE_HSE) && FUNCONF_USE_HSI && FUNCONF_USE_HSE
       #error FUNCONF_USE_HSI and FUNCONF_USE_HSE cannot both be set
#endif

#if !defined(FUNCONF_USE_HSI) && !defined(FUNCONF_USE_HSE)
	#define FUNCONF_USE_HSI 1 // Default to use HSI
	#define FUNCONF_USE_HSE 0
#endif

#if defined( CH32X03x ) && FUNCONF_USE_HSE
	#error No HSE in CH32X03x
#endif

#if !defined( FUNCONF_USE_PLL )
	#if defined( CH32X03x )
		#define FUNCONF_USE_PLL 0 // No PLL on X03x
	#else
		#define FUNCONF_USE_PLL 1 // Default to use PLL
	#endif
#endif

#if defined( CH32X03x ) && FUNCONF_USE_PLL
	#error No PLL on the X03x
#endif

#ifndef FUNCONF_ENABLE_HPE
	#if defined( CH32V003 )
		#define FUNCONF_ENABLE_HPE 0
	#else
		#define FUNCONF_ENABLE_HPE 1
	#endif
#endif


#if !defined( FUNCONF_USE_CLK_SEC )
	#define FUNCONF_USE_CLK_SEC  1// use clock security system by default
#endif

#ifndef HSE_VALUE
	#if defined(CH32V003)
		#define HSE_VALUE                 (24000000) // Value of the External oscillator in Hz, default
	#elif defined(CH32V10x)
		#define HSE_VALUE				  (8000000)
	#elif defined(CH32V20x)
		#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
		#define HSE_VALUE    			  (32000000)
		#else
		#define HSE_VALUE    			  (8000000)
		#endif
	#elif defined(CH32V30x)
		#define HSE_VALUE				  (8000000)
	#endif
#endif

#ifndef HSI_VALUE
	#if defined(CH32V003)
		#define HSI_VALUE                 (24000000) // Value of the Internal oscillator in Hz, default.
	#elif defined(CH32X03x)
		#define HSI_VALUE				  (48000000)
	#elif defined(CH32V10x)
		#define HSI_VALUE				  (8000000)
	#elif defined(CH32V20x)
		#define HSI_VALUE    			  (8000000)
	#elif defined(CH32V30x)
		#define HSI_VALUE				  (8000000)
	#endif
#endif

#ifndef FUNCONF_HSITRIM
	#define FUNCONF_HSITRIM 0x10  // Default (Chip default)
#endif

#ifndef FUNCONF_USE_PLL
	#define FUNCONF_USE_PLL 1     // Default, Use PLL.
#endif

#if !defined( FUNCONF_PLL_MULTIPLIER )
	#if defined(FUNCONF_USE_PLL) && FUNCONF_USE_PLL
		#if defined(CH32V10x)
			#define FUNCONF_PLL_MULTIPLIER 10	// Default: 8 * 10 = 80 MHz
		#elif defined(CH32V20x)
			#define FUNCONF_PLL_MULTIPLIER 18	// Default: 8 * 18 = 144 MHz
		#elif defined(CH32V30x)
			#define FUNCONF_PLL_MULTIPLIER 18	// Default: 8 * 18 = 144 MHz
		#else // CH32V003
			#define FUNCONF_PLL_MULTIPLIER 2	// Default: 24 * 2 = 48 MHz
		#endif
	#else
		#define FUNCONF_PLL_MULTIPLIER 1
	#endif
#endif

#ifndef FUNCONF_SYSTEM_CORE_CLOCK
	#if defined(FUNCONF_USE_HSI) && FUNCONF_USE_HSI
		#define FUNCONF_SYSTEM_CORE_CLOCK ((HSI_VALUE)*(FUNCONF_PLL_MULTIPLIER))
	#elif defined(FUNCONF_USE_HSE) && FUNCONF_USE_HSE
		#define FUNCONF_SYSTEM_CORE_CLOCK ((HSE_VALUE)*(FUNCONF_PLL_MULTIPLIER))
	#else
		#error Must define either FUNCONF_USE_HSI or FUNCONF_USE_HSE to be 1.
	#endif
#endif

#ifndef FUNCONF_USE_5V_VDD
	#define FUNCONF_USE_5V_VDD 0
#endif

// Default package for CH32V20x
#if defined(CH32V20x)
#if !defined(CH32V20x_D8W) && !defined(CH32V20x_D8) && !defined(CH32V20x_D6)
	#define CH32V20x_D6              /* CH32V203F6-CH32V203F8-CH32V203G6-CH32V203G8-CH32V203K6-CH32V203K8-CH32V203C6-CH32V203C8 */
	//#define CH32V20x_D8              /* CH32V203RBT6 */
	//#define CH32V20x_D8W             /* CH32V208 */
	#endif
#endif

// Default package for CH32V30x
#if defined(CH32V30x)
#if !defined(CH32V30x_D8) && !defined(CH32V30x_D8C)
	//#define CH32V30x_D8              /* CH32V303x */
	#define CH32V30x_D8C             /* CH32V307x-CH32V305x */
	#endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
// Legacy, for EVT, CMSIS

#define __MPU_PRESENT             0  /* Other CH32 devices does not provide an MPU */
#define __Vendor_SysTickConfig    0  /* Set to 1 if different SysTick Config is used */

#ifndef __ASSEMBLER__  // Things before this can be used in assembly.


#ifdef __cplusplus
  #define     __I     volatile                /*!< defines 'read only' permissions      */
#else
  #define     __I     volatile const          /*!< defines 'read only' permissions     */
#endif
#define     __O     volatile                  /*!< defines 'write only' permissions     */
#define     __IO    volatile                  /*!< defines 'read / write' permissions   */


#ifdef __cplusplus
extern "C" {
#endif

/* Interrupt Number Definition, according to the selected device */
typedef enum IRQn
{
    /******  RISC-V Processor Exceptions Numbers *******************************************************/
    NonMaskableInt_IRQn = 2, /* 2 Non Maskable Interrupt                             */
    EXC_IRQn = 3,            /* 3 Exception Interrupt                                */
#if defined(CH32V20x) || defined(CH32V30x)
	Ecall_M_Mode_IRQn = 5,   /* 5 Ecall M Mode Interrupt                             */
	Ecall_U_Mode_IRQn = 8,   /* 8 Ecall U Mode Interrupt                             */
	Break_Point_IRQn = 9,    /* 9 Break Point Interrupt                              */
#endif
    SysTicK_IRQn = 12,       /* 12 System timer Interrupt                            */
    Software_IRQn = 14,      /* 14 software Interrupt                                */

#if defined( CH32V003 ) || defined(CH32X03x)
    /******  RISC-V specific Interrupt Numbers *********************************************************/
    WWDG_IRQn = 16,          /* Window WatchDog Interrupt                            */
    PVD_IRQn = 17,           /* PVD through EXTI Line detection Interrupt            */
    FLASH_IRQn = 18,         /* FLASH global Interrupt                               */
    RCC_IRQn = 19,           /* RCC global Interrupt                                 */
    EXTI7_0_IRQn = 20,       /* External Line[7:0] Interrupts                        */
    AWU_IRQn = 21,           /* AWU global Interrupt                                 */
    DMA1_Channel1_IRQn = 22, /* DMA1 Channel 1 global Interrupt                      */
    DMA1_Channel2_IRQn = 23, /* DMA1 Channel 2 global Interrupt                      */
    DMA1_Channel3_IRQn = 24, /* DMA1 Channel 3 global Interrupt                      */
    DMA1_Channel4_IRQn = 25, /* DMA1 Channel 4 global Interrupt                      */
    DMA1_Channel5_IRQn = 26, /* DMA1 Channel 5 global Interrupt                      */
    DMA1_Channel6_IRQn = 27, /* DMA1 Channel 6 global Interrupt                      */
    DMA1_Channel7_IRQn = 28, /* DMA1 Channel 7 global Interrupt                      */
    ADC_IRQn = 29,           /* ADC global Interrupt                                 */
    I2C1_EV_IRQn = 30,       /* I2C1 Event Interrupt                                 */
    I2C1_ER_IRQn = 31,       /* I2C1 Error Interrupt                                 */
    USART1_IRQn = 32,        /* USART1 global Interrupt                              */
    SPI1_IRQn = 33,          /* SPI1 global Interrupt                                */
    TIM1_BRK_IRQn = 34,      /* TIM1 Break Interrupt                                 */
    TIM1_UP_IRQn = 35,       /* TIM1 Update Interrupt                                */
    TIM1_TRG_COM_IRQn = 36,  /* TIM1 Trigger and Commutation Interrupt               */
    TIM1_CC_IRQn = 37,       /* TIM1 Capture Compare Interrupt                       */
    TIM2_IRQn = 38,          /* TIM2 global Interrupt                                */
#if defined(CH32X03x)
	USART2_IRQn = 39,          /* UART2 Interrupt                          */
	EXTI15_8_IRQn = 40,        /* External Line[8:15] Interrupt            */
	EXTI25_16_IRQn = 41,       /* External Line[25:16] Interrupt           */
	USART3_IRQn = 42,          /* UART2 Interrupt                          */
	USART4_IRQn = 43,          /* UART2 Interrupt                          */
	DMA1_Channel8_IRQn = 44,   /* DMA1 Channel 8 global Interrupt          */
	USBFS_IRQn = 45,           /* USB Full-Speed Interrupt                 */
	USBFS_WakeUp_IRQn = 46,    /* USB Full-Speed Wake-Up Interrupt         */
	PIOC_IRQn = 47,            /* Programmable IO Controller Interrupt     */
	OPA_IRQn = 48,             /* Op Amp Interrupt                         */
	USBPD_IRQn = 49,           /* USB Power Delivery Interrupt             */
	USBPD_WKUP_IRQn = 50,      /* USB Power Delivery Wake-Up Interrupt     */
	TIM2_CC_IRQn = 51,         /* Timer 2 Compare Global Interrupt         */
	TIM2_TRG_IRQn = 52,        /* Timer 2 Trigger Global Interrupt         */
	TIM2_BRK_IRQn = 53,        /* Timer 2 Brk Global Interrupt             */
	TIM3_IRQn = 54,            /* Timer 3 Global Interrupt                 */
#endif
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	/******  RISC-V specific Interrupt Numbers *********************************************************/
	WWDG_IRQn = 16,            /* Window WatchDog Interrupt                            */
	PVD_IRQn = 17,             /* PVD through EXTI Line detection Interrupt            */
	TAMPER_IRQn = 18,          /* Tamper Interrupt                                     */
	RTC_IRQn = 19,             /* RTC global Interrupt                                 */
	FLASH_IRQn = 20,           /* FLASH global Interrupt                               */
	RCC_IRQn = 21,             /* RCC global Interrupt                                 */
	EXTI0_IRQn = 22,           /* EXTI Line0 Interrupt                                 */
	EXTI1_IRQn = 23,           /* EXTI Line1 Interrupt                                 */
	EXTI2_IRQn = 24,           /* EXTI Line2 Interrupt                                 */
	EXTI3_IRQn = 25,           /* EXTI Line3 Interrupt                                 */
	EXTI4_IRQn = 26,           /* EXTI Line4 Interrupt                                 */
	DMA1_Channel1_IRQn = 27,   /* DMA1 Channel 1 global Interrupt                      */
	DMA1_Channel2_IRQn = 28,   /* DMA1 Channel 2 global Interrupt                      */
	DMA1_Channel3_IRQn = 29,   /* DMA1 Channel 3 global Interrupt                      */
	DMA1_Channel4_IRQn = 30,   /* DMA1 Channel 4 global Interrupt                      */
	DMA1_Channel5_IRQn = 31,   /* DMA1 Channel 5 global Interrupt                      */
	DMA1_Channel6_IRQn = 32,   /* DMA1 Channel 6 global Interrupt                      */
	DMA1_Channel7_IRQn = 33,   /* DMA1 Channel 7 global Interrupt                      */
	ADC_IRQn = 34,             /* ADC1 and ADC2 global Interrupt                       */
#if !defined(CH32V10x) // CH32V20x/30x only
	USB_HP_CAN1_TX_IRQn = 35,  /* USB Device High Priority or CAN1 TX Interrupts       */
	USB_LP_CAN1_RX0_IRQn = 36, /* USB Device Low Priority or CAN1 RX0 Interrupts       */
	CAN1_RX1_IRQn = 37,        /* CAN1 RX1 Interrupt                                   */
	CAN1_SCE_IRQn = 38,        /* CAN1 SCE Interrupt                                   */
#endif
	EXTI9_5_IRQn = 39,         /* External Line[9:5] Interrupts                        */
	TIM1_BRK_IRQn = 40,        /* TIM1 Break Interrupt                                 */
	TIM1_UP_IRQn = 41,         /* TIM1 Update Interrupt                                */
	TIM1_TRG_COM_IRQn = 42,    /* TIM1 Trigger and Commutation Interrupt               */
	TIM1_CC_IRQn = 43,         /* TIM1 Capture Compare Interrupt                       */
	TIM2_IRQn = 44,            /* TIM2 global Interrupt                                */
	TIM3_IRQn = 45,            /* TIM3 global Interrupt                                */
	TIM4_IRQn = 46,            /* TIM4 global Interrupt                                */
	I2C1_EV_IRQn = 47,         /* I2C1 Event Interrupt                                 */
	I2C1_ER_IRQn = 48,         /* I2C1 Error Interrupt                                 */
	I2C2_EV_IRQn = 49,         /* I2C2 Event Interrupt                                 */
	I2C2_ER_IRQn = 50,         /* I2C2 Error Interrupt                                 */
	SPI1_IRQn = 51,            /* SPI1 global Interrupt                                */
	SPI2_IRQn = 52,            /* SPI2 global Interrupt                                */
	USART1_IRQn = 53,          /* USART1 global Interrupt                              */
	USART2_IRQn = 54,          /* USART2 global Interrupt                              */
	USART3_IRQn = 55,          /* USART3 global Interrupt                              */
	EXTI15_10_IRQn = 56,       /* External Line[15:10] Interrupts                      */
	RTCAlarm_IRQn = 57,        /* RTC Alarm through EXTI Line Interrupt                */
#endif
#if defined(CH32V10x) || defined(CH32V20x)
	USBWakeUp_IRQn = 58,       /* USB Device WakeUp from suspend through EXTI Line Interrupt 	*/
	USBHD_IRQn = 59,           /* USBHD global Interrupt                               */
#endif
#if defined(CH32V20x)
	USBHDWakeUp_IRQn = 60,     /* USB Host/Device WakeUp Interrupt                     */

#ifdef CH32V20x_D6
	UART4_IRQn = 61,         /* UART4 global Interrupt                               */
	DMA1_Channel8_IRQn = 62, /* DMA1 Channel 8 global Interrupt                      */

#elif defined(CH32V20x_D8)
	ETH_IRQn = 61,           /* ETH global Interrupt               	                 */
    ETHWakeUp_IRQn = 62,     /* ETH WakeUp Interrupt                       			 */
    TIM5_IRQn = 65,          /* TIM5 global Interrupt                                */
    UART4_IRQn = 66,         /* UART4 global Interrupt                               */
    DMA1_Channel8_IRQn = 67, /* DMA1 Channel 8 global Interrupt                      */
    OSC32KCal_IRQn = 68,     /* OSC32K global Interrupt                              */
    OSCWakeUp_IRQn = 69,     /* OSC32K WakeUp Interrupt                              */

#elif defined(CH32V20x_D8W)
    ETH_IRQn = 61,           /* ETH global Interrupt               	                 */
    ETHWakeUp_IRQn = 62,     /* ETH WakeUp Interrupt                       			 */
    BB_IRQn = 63,            /* BLE BB global Interrupt                              */
    LLE_IRQn = 64,           /* BLE LLE global Interrupt                             */
    TIM5_IRQn = 65,          /* TIM5 global Interrupt                                */
    UART4_IRQn = 66,         /* UART4 global Interrupt                               */
    DMA1_Channel8_IRQn = 67, /* DMA1 Channel 8 global Interrupt                      */
    OSC32KCal_IRQn = 68,     /* OSC32K global Interrupt                              */
    OSCWakeUp_IRQn = 69,     /* OSC32K WakeUp Interrupt                              */
#endif

#elif defined(CH32V30x)

#ifdef CH32V30x_D8
  TIM8_BRK_IRQn               = 59,      /* TIM8 Break Interrupt                                 */
  TIM8_UP_IRQn                = 60,      /* TIM8 Update Interrupt                                */
  TIM8_TRG_COM_IRQn           = 61,      /* TIM8 Trigger and Commutation Interrupt               */
  TIM8_CC_IRQn                = 62,      /* TIM8 Capture Compare Interrupt                       */
  RNG_IRQn                    = 63,      /* RNG global Interrupt                                 */
  FSMC_IRQn                   = 64,      /* FSMC global Interrupt                                */
  SDIO_IRQn                   = 65,      /* SDIO global Interrupt                                */
  TIM5_IRQn                   = 66,      /* TIM5 global Interrupt                                */
  SPI3_IRQn                   = 67,      /* SPI3 global Interrupt                                */
  UART4_IRQn                  = 68,      /* UART4 global Interrupt                               */
  UART5_IRQn                  = 69,      /* UART5 global Interrupt                               */
  TIM6_IRQn                   = 70,      /* TIM6 global Interrupt                                */
  TIM7_IRQn                   = 71,      /* TIM7 global Interrupt                                */
  DMA2_Channel1_IRQn          = 72,      /* DMA2 Channel 1 global Interrupt                      */
  DMA2_Channel2_IRQn          = 73,      /* DMA2 Channel 2 global Interrupt                      */
  DMA2_Channel3_IRQn          = 74,      /* DMA2 Channel 3 global Interrupt                      */
  DMA2_Channel4_IRQn          = 75,      /* DMA2 Channel 4 global Interrupt                      */
  DMA2_Channel5_IRQn          = 76,      /* DMA2 Channel 5 global Interrupt                      */
  OTG_FS_IRQn                 = 83,      /* OTGFS global Interrupt                               */
  UART6_IRQn                  = 87,      /* UART6 global Interrupt                               */
  UART7_IRQn                  = 88,      /* UART7 global Interrupt                               */
  UART8_IRQn                  = 89,      /* UART8 global Interrupt                               */
  TIM9_BRK_IRQn               = 90,      /* TIM9 Break Interrupt                                 */
  TIM9_UP_IRQn                = 91,      /* TIM9 Update Interrupt                                */
  TIM9_TRG_COM_IRQn           = 92,      /* TIM9 Trigger and Commutation Interrupt               */
  TIM9_CC_IRQn                = 93,      /* TIM9 Capture Compare Interrupt                       */
  TIM10_BRK_IRQn              = 94,      /* TIM10 Break Interrupt                                */
  TIM10_UP_IRQn               = 95,      /* TIM10 Update Interrupt                               */
  TIM10_TRG_COM_IRQn          = 96,      /* TIM10 Trigger and Commutation Interrupt              */
  TIM10_CC_IRQn               = 97,      /* TIM10 Capture Compare Interrupt                      */
  DMA2_Channel6_IRQn          = 98,      /* DMA2 Channel 6 global Interrupt                      */
  DMA2_Channel7_IRQn          = 99,      /* DMA2 Channel 7 global Interrupt                      */
  DMA2_Channel8_IRQn          = 100,     /* DMA2 Channel 8 global Interrupt                      */
  DMA2_Channel9_IRQn          = 101,     /* DMA2 Channel 9 global Interrupt                      */
  DMA2_Channel10_IRQn         = 102,     /* DMA2 Channel 10 global Interrupt                     */
  DMA2_Channel11_IRQn         = 103,     /* DMA2 Channel 11 global Interrupt                     */

#elif defined  (CH32V30x_D8C)
  USBWakeUp_IRQn              = 58,      /* USB Device WakeUp from suspend through EXTI Line Interrupt */
  TIM8_BRK_IRQn               = 59,      /* TIM8 Break Interrupt                                 */
  TIM8_UP_IRQn                = 60,      /* TIM8 Update Interrupt                                */
  TIM8_TRG_COM_IRQn           = 61,      /* TIM8 Trigger and Commutation Interrupt               */
  TIM8_CC_IRQn                = 62,      /* TIM8 Capture Compare Interrupt                       */
  RNG_IRQn                    = 63,      /* RNG global Interrupt                                 */
  FSMC_IRQn                   = 64,      /* FSMC global Interrupt                                */
  SDIO_IRQn                   = 65,      /* SDIO global Interrupt                                */
  TIM5_IRQn                   = 66,      /* TIM5 global Interrupt                                */
  SPI3_IRQn                   = 67,      /* SPI3 global Interrupt                                */
  UART4_IRQn                  = 68,      /* UART4 global Interrupt                               */
  UART5_IRQn                  = 69,      /* UART5 global Interrupt                               */
  TIM6_IRQn                   = 70,      /* TIM6 global Interrupt                                */
  TIM7_IRQn                   = 71,      /* TIM7 global Interrupt                                */
  DMA2_Channel1_IRQn          = 72,      /* DMA2 Channel 1 global Interrupt                      */
  DMA2_Channel2_IRQn          = 73,      /* DMA2 Channel 2 global Interrupt                      */
  DMA2_Channel3_IRQn          = 74,      /* DMA2 Channel 3 global Interrupt                      */
  DMA2_Channel4_IRQn          = 75,      /* DMA2 Channel 4 global Interrupt                      */
  DMA2_Channel5_IRQn          = 76,      /* DMA2 Channel 5 global Interrupt                      */
  ETH_IRQn                    = 77,      /* ETH global Interrupt                                 */
  ETH_WKUP_IRQn               = 78,      /* ETH WakeUp Interrupt                                 */
  CAN2_TX_IRQn                = 79,      /* CAN2 TX Interrupts                                   */
  CAN2_RX0_IRQn               = 80,      /* CAN2 RX0 Interrupts                                  */
  CAN2_RX1_IRQn               = 81,      /* CAN2 RX1 Interrupt                                   */
  CAN2_SCE_IRQn               = 82,      /* CAN2 SCE Interrupt                                   */
  OTG_FS_IRQn                 = 83,      /* OTGFS global Interrupt                               */
  USBHSWakeup_IRQn            = 84,      /* USBHS WakeUp Interrupt                               */
  USBHS_IRQn                  = 85,      /* USBHS global Interrupt                               */
  DVP_IRQn                    = 86,      /* DVP global Interrupt                                 */
  UART6_IRQn                  = 87,      /* UART6 global Interrupt                               */
  UART7_IRQn                  = 88,      /* UART7 global Interrupt                               */
  UART8_IRQn                  = 89,      /* UART8 global Interrupt                               */
  TIM9_BRK_IRQn               = 90,      /* TIM9 Break Interrupt                                 */
  TIM9_UP_IRQn                = 91,      /* TIM9 Update Interrupt                                */
  TIM9_TRG_COM_IRQn           = 92,      /* TIM9 Trigger and Commutation Interrupt               */
  TIM9_CC_IRQn                = 93,      /* TIM9 Capture Compare Interrupt                       */
  TIM10_BRK_IRQn              = 94,      /* TIM10 Break Interrupt                                */
  TIM10_UP_IRQn               = 95,      /* TIM10 Update Interrupt                               */
  TIM10_TRG_COM_IRQn          = 96,      /* TIM10 Trigger and Commutation Interrupt              */
  TIM10_CC_IRQn               = 97,      /* TIM10 Capture Compare Interrupt                      */
  DMA2_Channel6_IRQn          = 98,      /* DMA2 Channel 6 global Interrupt                      */
  DMA2_Channel7_IRQn          = 99,      /* DMA2 Channel 7 global Interrupt                      */
  DMA2_Channel8_IRQn          = 100,     /* DMA2 Channel 8 global Interrupt                      */
  DMA2_Channel9_IRQn          = 101,     /* DMA2 Channel 9 global Interrupt                      */
  DMA2_Channel10_IRQn         = 102,     /* DMA2 Channel 10 global Interrupt                     */
  DMA2_Channel11_IRQn         = 103,     /* DMA2 Channel 11 global Interrupt                     */

#endif

#endif

} IRQn_Type;

#include <stdint.h>
#endif

#define HardFault_IRQn    EXC_IRQn

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
	#define ADC1_2_IRQn       ADC_IRQn
#endif

/* Standard Peripheral Library old definitions (maintained for legacy purpose) */
#define HSI_Value             HSI_VALUE
#define HSE_Value             HSE_VALUE
#define HSEStartUp_TimeOut    HSE_STARTUP_TIMEOUT

#ifndef __ASSEMBLER__
/* Analog to Digital Converter */
typedef struct
{
    __IO uint32_t STATR;
    __IO uint32_t CTLR1;
    __IO uint32_t CTLR2;
    __IO uint32_t SAMPTR1;
    __IO uint32_t SAMPTR2;
    __IO uint32_t IOFR1;
    __IO uint32_t IOFR2;
    __IO uint32_t IOFR3;
    __IO uint32_t IOFR4;
    __IO uint32_t WDHTR;
    __IO uint32_t WDLTR;
    __IO uint32_t RSQR1;
    __IO uint32_t RSQR2;
    __IO uint32_t RSQR3;
    __IO uint32_t ISQR;
    __IO uint32_t IDATAR1;
    __IO uint32_t IDATAR2;
    __IO uint32_t IDATAR3;
    __IO uint32_t IDATAR4;
    __IO uint32_t RDATAR;
#if defined(CH32V20x)
    __IO uint32_t DLYR;
#elif defined(CH32X03x)
    __IO uint32_t CTLR3;
    __IO uint32_t WDTR1;
    __IO uint32_t WDTR2;
    __IO uint32_t WDTR3;
#endif
} ADC_TypeDef;

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/* Backup Registers */
typedef struct
{
	uint32_t      RESERVED0;
	__IO uint16_t DATAR1;
	uint16_t      RESERVED1;
	__IO uint16_t DATAR2;
	uint16_t      RESERVED2;
	__IO uint16_t DATAR3;
	uint16_t      RESERVED3;
	__IO uint16_t DATAR4;
	uint16_t      RESERVED4;
	__IO uint16_t DATAR5;
	uint16_t      RESERVED5;
	__IO uint16_t DATAR6;
	uint16_t      RESERVED6;
	__IO uint16_t DATAR7;
	uint16_t      RESERVED7;
	__IO uint16_t DATAR8;
	uint16_t      RESERVED8;
	__IO uint16_t DATAR9;
	uint16_t      RESERVED9;
	__IO uint16_t DATAR10;
	uint16_t      RESERVED10;
	__IO uint16_t OCTLR;
	uint16_t      RESERVED11;
	__IO uint16_t TPCTLR;
	uint16_t      RESERVED12;
	__IO uint16_t TPCSR;
	uint16_t      RESERVED13[5];
	__IO uint16_t DATAR11;
	uint16_t      RESERVED14;
	__IO uint16_t DATAR12;
	uint16_t      RESERVED15;
	__IO uint16_t DATAR13;
	uint16_t      RESERVED16;
	__IO uint16_t DATAR14;
	uint16_t      RESERVED17;
	__IO uint16_t DATAR15;
	uint16_t      RESERVED18;
	__IO uint16_t DATAR16;
	uint16_t      RESERVED19;
	__IO uint16_t DATAR17;
	uint16_t      RESERVED20;
	__IO uint16_t DATAR18;
	uint16_t      RESERVED21;
	__IO uint16_t DATAR19;
	uint16_t      RESERVED22;
	__IO uint16_t DATAR20;
	uint16_t      RESERVED23;
	__IO uint16_t DATAR21;
	uint16_t      RESERVED24;
	__IO uint16_t DATAR22;
	uint16_t      RESERVED25;
	__IO uint16_t DATAR23;
	uint16_t      RESERVED26;
	__IO uint16_t DATAR24;
	uint16_t      RESERVED27;
	__IO uint16_t DATAR25;
	uint16_t      RESERVED28;
	__IO uint16_t DATAR26;
	uint16_t      RESERVED29;
	__IO uint16_t DATAR27;
	uint16_t      RESERVED30;
	__IO uint16_t DATAR28;
	uint16_t      RESERVED31;
	__IO uint16_t DATAR29;
	uint16_t      RESERVED32;
	__IO uint16_t DATAR30;
	uint16_t      RESERVED33;
	__IO uint16_t DATAR31;
	uint16_t      RESERVED34;
	__IO uint16_t DATAR32;
	uint16_t      RESERVED35;
	__IO uint16_t DATAR33;
	uint16_t      RESERVED36;
	__IO uint16_t DATAR34;
	uint16_t      RESERVED37;
	__IO uint16_t DATAR35;
	uint16_t      RESERVED38;
	__IO uint16_t DATAR36;
	uint16_t      RESERVED39;
	__IO uint16_t DATAR37;
	uint16_t      RESERVED40;
	__IO uint16_t DATAR38;
	uint16_t      RESERVED41;
	__IO uint16_t DATAR39;
	uint16_t      RESERVED42;
	__IO uint16_t DATAR40;
	uint16_t      RESERVED43;
	__IO uint16_t DATAR41;
	uint16_t      RESERVED44;
	__IO uint16_t DATAR42;
	uint16_t      RESERVED45;
} BKP_TypeDef;
#endif

#if defined(CH32V20x) || defined(CH32V30x)
/* Controller Area Network TxMailBox */
typedef struct
{
	__IO uint32_t TXMIR;
	__IO uint32_t TXMDTR;
	__IO uint32_t TXMDLR;
	__IO uint32_t TXMDHR;
} CAN_TxMailBox_TypeDef;

/* Controller Area Network FIFOMailBox */
typedef struct
{
	__IO uint32_t RXMIR;
	__IO uint32_t RXMDTR;
	__IO uint32_t RXMDLR;
	__IO uint32_t RXMDHR;
} CAN_FIFOMailBox_TypeDef;

/* Controller Area Network FilterRegister */
typedef struct
{
	__IO uint32_t FR1;
	__IO uint32_t FR2;
} CAN_FilterRegister_TypeDef;

/* Controller Area Network */
typedef struct
{
	__IO uint32_t              CTLR;
	__IO uint32_t              STATR;
	__IO uint32_t              TSTATR;
	__IO uint32_t              RFIFO0;
	__IO uint32_t              RFIFO1;
	__IO uint32_t              INTENR;
	__IO uint32_t              ERRSR;
	__IO uint32_t              BTIMR;
	uint32_t                   RESERVED0[88];
	CAN_TxMailBox_TypeDef      sTxMailBox[3];
	CAN_FIFOMailBox_TypeDef    sFIFOMailBox[2];
	uint32_t                   RESERVED1[12];
	__IO uint32_t              FCTLR;
	__IO uint32_t              FMCFGR;
	uint32_t                   RESERVED2;
	__IO uint32_t              FSCFGR;
	uint32_t                   RESERVED3;
	__IO uint32_t              FAFIFOR;
	uint32_t                   RESERVED4;
	__IO uint32_t              FWR;
	uint32_t                   RESERVED5[8];
	CAN_FilterRegister_TypeDef sFilterRegister[28];
} CAN_TypeDef;
#endif

/* CRC Calculation Unit */
typedef struct
{
	__IO uint32_t DATAR;
	__IO uint8_t  IDATAR;
	uint8_t       RESERVED0;
	uint16_t      RESERVED1;
	__IO uint32_t CTLR;
} CRC_TypeDef;

#if defined(CH32V10x) || defined(CH32V30x)
/* Digital to Analog Converter */
typedef struct
{
  __IO uint32_t CTLR;
  __IO uint32_t SWTR;
  __IO uint32_t R12BDHR1;
  __IO uint32_t L12BDHR1;
  __IO uint32_t R8BDHR1;
  __IO uint32_t R12BDHR2;
  __IO uint32_t L12BDHR2;
  __IO uint32_t R8BDHR2;
  __IO uint32_t RD12BDHR;
  __IO uint32_t LD12BDHR;
  __IO uint32_t RD8BDHR;
  __IO uint32_t DOR1;
  __IO uint32_t DOR2;
} DAC_TypeDef;
#endif

/* Debug MCU */
typedef struct
{
    __IO uint32_t CFGR0;
    __IO uint32_t CFGR1;
} DBGMCU_TypeDef;

/* DMA Controller */
typedef struct
{
    __IO uint32_t CFGR;
    __IO uint32_t CNTR;
    __IO uint32_t PADDR;
    __IO uint32_t MADDR;
} DMA_Channel_TypeDef;

typedef struct
{
    __IO uint32_t INTFR;
    __IO uint32_t INTFCR;
} DMA_TypeDef;

/* External Interrupt/Event Controller */
typedef struct
{
    __IO uint32_t INTENR;
    __IO uint32_t EVENR;
    __IO uint32_t RTENR;
    __IO uint32_t FTENR;
    __IO uint32_t SWIEVR;
    __IO uint32_t INTFR;
} EXTI_TypeDef;

/* FLASH Registers */
typedef struct
{
    __IO uint32_t ACTLR;
    __IO uint32_t KEYR;
    __IO uint32_t OBKEYR;
    __IO uint32_t STATR;
    __IO uint32_t CTLR;
    __IO uint32_t ADDR;
    __IO uint32_t RESERVED;
    __IO uint32_t OBR;
    __IO uint32_t WPR;
    __IO uint32_t MODEKEYR;
#ifdef CH32V003
    __IO uint32_t BOOT_MODEKEYR;
#endif
} FLASH_TypeDef;

/* Option Bytes Registers */
typedef struct
{
    __IO uint16_t RDPR;
    __IO uint16_t USER;
    __IO uint16_t Data0;
    __IO uint16_t Data1;
    __IO uint16_t WRPR0;
    __IO uint16_t WRPR1;
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint16_t WRPR2;
	__IO uint16_t WRPR3;
#endif
} OB_TypeDef;

typedef struct
{
	__IO uint16_t CAP;
	__IO uint16_t RES1;
	__IO uint32_t RES2;
	__IO uint32_t UID0;
	__IO uint32_t UID1;
	__IO uint32_t UID2;
	__IO uint32_t RES3;
} ESG_TypeDef;

#if defined(CH32V30x)
/* FSMC Bank1 Registers */
typedef struct
{
  __IO uint32_t BTCR[8];
} FSMC_Bank1_TypeDef;

/* FSMC Bank1E Registers */
typedef struct
{
  __IO uint32_t BWTR[7];
} FSMC_Bank1E_TypeDef;

/* FSMC Bank2 Registers */
typedef struct
{
  __IO uint32_t PCR2;
  __IO uint32_t SR2;
  __IO uint32_t PMEM2;
  __IO uint32_t PATT2;
  uint32_t  RESERVED0;
  __IO uint32_t ECCR2;
} FSMC_Bank2_TypeDef;
#endif

/* General Purpose I/O */
typedef enum
{
	GPIO_CFGLR_IN_ANALOG = 0,
	GPIO_CFGLR_IN_FLOAT = 4,
	GPIO_CFGLR_IN_PUPD = 8,
	GPIO_CFGLR_OUT_10Mhz_PP = 1,
	GPIO_CFGLR_OUT_2Mhz_PP = 2,
	GPIO_CFGLR_OUT_50Mhz_PP = 3,
	GPIO_CFGLR_OUT_10Mhz_OD = 5,
	GPIO_CFGLR_OUT_2Mhz_OD = 6,
	GPIO_CFGLR_OUT_50Mhz_OD = 7,
	GPIO_CFGLR_OUT_10Mhz_AF_PP = 9,
	GPIO_CFGLR_OUT_2Mhz_AF_PP = 10,
	GPIO_CFGLR_OUT_50Mhz_AF_PP = 11,
	GPIO_CFGLR_OUT_10Mhz_AF_OD = 13,
	GPIO_CFGLR_OUT_2Mhz_AF_OD = 14,
	GPIO_CFGLR_OUT_50Mhz_AF_OD = 15,
} GPIO_CFGLR_PIN_MODE_Typedef;

typedef union {
	uint32_t __FULL;
	struct {
		GPIO_CFGLR_PIN_MODE_Typedef PIN0 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN1 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN2 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN3 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN4 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN5 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN6 :4;
		GPIO_CFGLR_PIN_MODE_Typedef PIN7 :4;
	};
} GPIO_CFGLR_t;
typedef union {
	uint32_t __FULL;
	const struct {
		uint32_t IDR0 :1;
		uint32_t IDR1 :1;
		uint32_t IDR2 :1;
		uint32_t IDR3 :1;
		uint32_t IDR4 :1;
		uint32_t IDR5 :1;
		uint32_t IDR6 :1;
		uint32_t IDR7 :1;
		uint32_t :24;
	};
} GPIO_INDR_t;
typedef union {
	uint32_t __FULL;
	struct {
		uint32_t ODR0 :1;
		uint32_t ODR1 :1;
		uint32_t ODR2 :1;
		uint32_t ODR3 :1;
		uint32_t ODR4 :1;
		uint32_t ODR5 :1;
		uint32_t ODR6 :1;
		uint32_t ODR7 :1;
		uint32_t :24;
	};
} GPIO_OUTDR_t;
typedef union {
	uint32_t __FULL;
	struct {
		uint32_t BS0 :1;
		uint32_t BS1 :1;
		uint32_t BS2 :1;
		uint32_t BS3 :1;
		uint32_t BS4 :1;
		uint32_t BS5 :1;
		uint32_t BS6 :1;
		uint32_t BS7 :1;
		uint32_t :8;
		uint32_t BR0 :1;
		uint32_t BR1 :1;
		uint32_t BR2 :1;
		uint32_t BR3 :1;
		uint32_t BR4 :1;
		uint32_t BR5 :1;
		uint32_t BR6 :1;
		uint32_t BR7 :1;
		uint32_t :8;
	};
} GPIO_BSHR_t;
typedef union {
	uint32_t __FULL;
	struct {
		uint32_t BR0 :1;
		uint32_t BR1 :1;
		uint32_t BR2 :1;
		uint32_t BR3 :1;
		uint32_t BR4 :1;
		uint32_t BR5 :1;
		uint32_t BR6 :1;
		uint32_t BR7 :1;
		uint32_t :24;
	};
} GPIO_BCR_t;
typedef union {
	uint32_t __FULL;
	struct {
		uint32_t LCK0 :1;
		uint32_t LCK1 :1;
		uint32_t LCK2 :1;
		uint32_t LCK3 :1;
		uint32_t LCK4 :1;
		uint32_t LCK5 :1;
		uint32_t LCK6 :1;
		uint32_t LCK7 :1;
		uint32_t LCKK :1;
		uint32_t :23;
	};
} GPIO_LCKR_t;
typedef struct
{
	__IO uint32_t CFGLR;
	__IO uint32_t CFGHR;
	__I  uint32_t INDR;
	__IO uint32_t OUTDR;
	__IO uint32_t BSHR;
	__IO uint32_t BCR;
	__IO uint32_t LCKR;
#ifdef CH32X03x
	__IO uint32_t CFGXR;
	__IO uint32_t BSXR;
#endif
} GPIO_TypeDef;

#define DYN_GPIO_READ(gpio, field) ((GPIO_##field##_t) { .__FULL = gpio->field })
#define DYN_GPIO_WRITE(gpio, field, ...) gpio->field = ((const GPIO_##field##_t) __VA_ARGS__).__FULL
#define DYN_GPIO_MOD(gpio, field, reg, val) {GPIO_##field##_t tmp; tmp.__FULL = gpio->field; tmp.reg = val; gpio->field = tmp.__FULL;}

/* Alternate Function I/O */
typedef struct
{
#ifdef CH32V003
    uint32_t RESERVED0;
    __IO uint32_t PCFR1;
    __IO uint32_t EXTICR;
#elif defined(CH32X03x)
    uint32_t RESERVED0;
    __IO uint32_t PCFR1;
    __IO uint32_t EXTICR1;
    __IO uint32_t EXTICR2;
    uint32_t RESERVED1;
    uint32_t RESERVED2;
    __IO uint32_t CTLR;
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint32_t ECR;
	__IO uint32_t PCFR1;
	__IO uint32_t EXTICR[4];
	uint32_t      RESERVED0;
	__IO uint32_t PCFR2;
#endif
} AFIO_TypeDef;

/* Inter Integrated Circuit Interface */
typedef struct
{
    __IO uint16_t CTLR1;
    uint16_t      RESERVED0;
    __IO uint16_t CTLR2;
    uint16_t      RESERVED1;
    __IO uint16_t OADDR1;
    uint16_t      RESERVED2;
    __IO uint16_t OADDR2;
    uint16_t      RESERVED3;
    __IO uint16_t DATAR;
    uint16_t      RESERVED4;
    __IO uint16_t STAR1;
    uint16_t      RESERVED5;
    __IO uint16_t STAR2;
    uint16_t      RESERVED6;
    __IO uint16_t CKCFGR;
    uint16_t      RESERVED7;
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint16_t RTR;
	uint16_t      RESERVED8;
#endif
} I2C_TypeDef;

/* Independent WatchDog */
typedef struct
{
    __IO uint32_t CTLR;
    __IO uint32_t PSCR;
    __IO uint32_t RLDR;
    __IO uint32_t STATR;
} IWDG_TypeDef;

/* Power Control */
typedef struct
{
    __IO uint32_t CTLR;
    __IO uint32_t CSR;
#ifdef CH32V003	// AWU is CH32V003-only
    __IO uint32_t AWUCSR;
    __IO uint32_t AWUWR;
    __IO uint32_t AWUPSC;
#endif
} PWR_TypeDef;

/* Reset and Clock Control */
typedef struct
{
    __IO uint32_t CTLR;
    __IO uint32_t CFGR0;
    __IO uint32_t INTR;
    __IO uint32_t APB2PRSTR;
    __IO uint32_t APB1PRSTR;
    __IO uint32_t AHBPCENR;
    __IO uint32_t APB2PCENR;
    __IO uint32_t APB1PCENR;
#ifdef CH32V003
    __IO uint32_t RESERVED0;
    __IO uint32_t RSTSCKR;
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint32_t BDCTLR;
	__IO uint32_t RSTSCKR;
#endif
#if defined(CH32V20x) || defined(CH32V30x)
	__IO uint32_t AHBRSTR;
	__IO uint32_t CFGR2;
#endif
} RCC_TypeDef;

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/* Real-Time Clock */
typedef struct
{
	__IO uint16_t CTLRH;
	uint16_t      RESERVED0;
	__IO uint16_t CTLRL;
	uint16_t      RESERVED1;
	__IO uint16_t PSCRH;
	uint16_t      RESERVED2;
	__IO uint16_t PSCRL;
	uint16_t      RESERVED3;
	__IO uint16_t DIVH;
	uint16_t      RESERVED4;
	__IO uint16_t DIVL;
	uint16_t      RESERVED5;
	__IO uint16_t CNTH;
	uint16_t      RESERVED6;
	__IO uint16_t CNTL;
	uint16_t      RESERVED7;
	__IO uint16_t ALRMH;
	uint16_t      RESERVED8;
	__IO uint16_t ALRML;
	uint16_t      RESERVED9;
} RTC_TypeDef;
#endif

#if defined(CH32V30x)
/* SDIO Registers */
typedef struct
{
  __IO uint32_t POWER;
  __IO uint32_t CLKCR;
  __IO uint32_t ARG;
  __IO uint32_t CMD;
  __I uint32_t RESPCMD;
  __I uint32_t RESP1;
  __I uint32_t RESP2;
  __I uint32_t RESP3;
  __I uint32_t RESP4;
  __IO uint32_t DTIMER;
  __IO uint32_t DLEN;
  __IO uint32_t DCTRL;
  __I uint32_t DCOUNT;
  __I uint32_t STA;
  __IO uint32_t ICR;
  __IO uint32_t MASK;
  uint32_t  RESERVED0[2];
  __I uint32_t FIFOCNT;
  uint32_t  RESERVED1[13];
  __IO uint32_t FIFO;
} SDIO_TypeDef;
#endif

/* Serial Peripheral Interface */
typedef struct
{
    __IO uint16_t CTLR1;
    uint16_t      RESERVED0;
    __IO uint16_t CTLR2;
    uint16_t      RESERVED1;
    __IO uint16_t STATR;
    uint16_t      RESERVED2;
    __IO uint16_t DATAR;
    uint16_t      RESERVED3;
    __IO uint16_t CRCR;
    uint16_t      RESERVED4;
    __IO uint16_t RCRCR;
    uint16_t      RESERVED5;
    __IO uint16_t TCRCR;
    uint16_t      RESERVED6;
#ifdef CH32V003
    uint32_t      RESERVED7;
	uint32_t      RESERVED8;
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint16_t I2SCFGR;
	uint16_t      RESERVED7;
	__IO uint16_t I2SPR;
	uint16_t      RESERVED8;
#endif
#if !defined(CH32V10x)
    __IO uint16_t HSCR;
    uint16_t      RESERVED9;
#endif
} SPI_TypeDef;

/* TIM */
typedef struct
{
    __IO uint16_t CTLR1;
    uint16_t      RESERVED0;
    __IO uint16_t CTLR2;
    uint16_t      RESERVED1;
    __IO uint16_t SMCFGR;
    uint16_t      RESERVED2;
    __IO uint16_t DMAINTENR;
    uint16_t      RESERVED3;
    __IO uint16_t INTFR;
    uint16_t      RESERVED4;
    __IO uint16_t SWEVGR;
    uint16_t      RESERVED5;
    __IO uint16_t CHCTLR1;
    uint16_t      RESERVED6;
    __IO uint16_t CHCTLR2;
    uint16_t      RESERVED7;
    __IO uint16_t CCER;
    uint16_t      RESERVED8;
    __IO uint16_t CNT;
    uint16_t      RESERVED9;
    __IO uint16_t PSC;
    uint16_t      RESERVED10;
    __IO uint16_t ATRLR;
    uint16_t      RESERVED11;
    __IO uint16_t RPTCR;
    uint16_t      RESERVED12;
#ifdef CH32V003
    __IO uint32_t CH1CVR;
    __IO uint32_t CH2CVR;
    __IO uint32_t CH3CVR;
    __IO uint32_t CH4CVR;
    __IO uint16_t BDTR;
    uint16_t      RESERVED13;
    __IO uint16_t DMACFGR;
    uint16_t      RESERVED14;
    __IO uint16_t DMAADR;
    uint16_t      RESERVED15;
#elif defined( CH32X03x )
    __IO uint32_t CH1CVR;
    __IO uint32_t CH2CVR;
    __IO uint32_t CH3CVR;
    __IO uint32_t CH4CVR;
    __IO uint16_t BDTR;
    uint16_t      RESERVED13;
    __IO uint16_t DMACFGR;
    uint16_t      RESERVED14;
    __IO uint16_t DMAADR;
    uint16_t      RESERVED15;
    __IO uint16_t SPEC;
    uint16_t      RESERVED16;
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	__IO uint16_t CH1CVR;
	uint16_t      RESERVED13;
	__IO uint16_t CH2CVR;
	uint16_t      RESERVED14;
	__IO uint16_t CH3CVR;
	uint16_t      RESERVED15;
	__IO uint16_t CH4CVR;
	uint16_t      RESERVED16;
	__IO uint16_t BDTR;
	uint16_t      RESERVED17;
	__IO uint16_t DMACFGR;
	uint16_t      RESERVED18;
	__IO uint16_t DMAADR;
	uint16_t      RESERVED19;
#endif
} TIM_TypeDef;

/* Universal Synchronous Asynchronous Receiver Transmitter */
typedef struct
{
    __IO uint16_t STATR;
    uint16_t      RESERVED0;
    __IO uint16_t DATAR;
    uint16_t      RESERVED1;
    __IO uint16_t BRR;
    uint16_t      RESERVED2;
    __IO uint16_t CTLR1;
    uint16_t      RESERVED3;
    __IO uint16_t CTLR2;
    uint16_t      RESERVED4;
    __IO uint16_t CTLR3;
    uint16_t      RESERVED5;
    __IO uint16_t GPR;
    uint16_t      RESERVED6;
} USART_TypeDef;

/* Window WatchDog */
typedef struct
{
    __IO uint32_t CTLR;
    __IO uint32_t CFGR;
    __IO uint32_t STATR;
} WWDG_TypeDef;

/* Enhanced Registers */
typedef struct
{
    __IO uint32_t EXTEN_CTR;
} EXTEN_TypeDef;

#if defined(CH32V20x) || defined(CH32V30x)
/* OPA Registers */
typedef struct
{
	__IO uint32_t CR;
} OPA_TypeDef;

#if defined(CH32V30x)
/* RNG Registers */
typedef struct
{
  __IO uint32_t CR;
  __IO uint32_t SR;
  __IO uint32_t DR;
} RNG_TypeDef;

/* DVP Registers */
typedef struct
{
  __IO uint8_t CR0;
  __IO uint8_t CR1;
  __IO uint8_t IER;
  __IO uint8_t Reserved0;
  __IO uint16_t ROW_NUM;
  __IO uint16_t COL_NUM;
  __IO uint32_t DMA_BUF0;
  __IO uint32_t DMA_BUF1;
  __IO uint8_t IFR;
  __IO uint8_t STATUS;
  __IO uint16_t Reserved1;
  __IO uint16_t ROW_CNT;
  __IO uint16_t Reserved2;
  __IO uint16_t HOFFCNT;
  __IO uint16_t VST;
  __IO uint16_t CAPCNT;
  __IO uint16_t VLINE;
  __IO uint32_t DR;
} DVP_TypeDef;

/* USBHS Registers */
typedef struct
{
  __IO uint8_t  CONTROL;
  __IO uint8_t  HOST_CTRL;
  __IO uint8_t  INT_EN;
  __IO uint8_t  DEV_AD;
  __IO uint16_t FRAME_NO;
  __IO uint8_t  SUSPEND;
  __IO uint8_t  RESERVED0;
  __IO uint8_t  SPEED_TYPE;
  __IO uint8_t  MIS_ST;
  __IO uint8_t  INT_FG;
  __IO uint8_t  INT_ST;
  __IO uint16_t RX_LEN;
  __IO uint16_t RESERVED1;
  __IO uint32_t ENDP_CONFIG;
  __IO uint32_t ENDP_TYPE;
  __IO uint32_t BUF_MODE;
  __IO uint32_t UEP0_DMA;
  __IO uint32_t UEP1_RX_DMA;
  __IO uint32_t UEP2_RX_DMA;
  __IO uint32_t UEP3_RX_DMA;
  __IO uint32_t UEP4_RX_DMA;
  __IO uint32_t UEP5_RX_DMA;
  __IO uint32_t UEP6_RX_DMA;
  __IO uint32_t UEP7_RX_DMA;
  __IO uint32_t UEP8_RX_DMA;
  __IO uint32_t UEP9_RX_DMA;
  __IO uint32_t UEP10_RX_DMA;
  __IO uint32_t UEP11_RX_DMA;
  __IO uint32_t UEP12_RX_DMA;
  __IO uint32_t UEP13_RX_DMA;
  __IO uint32_t UEP14_RX_DMA;
  __IO uint32_t UEP15_RX_DMA;
  __IO uint32_t UEP1_TX_DMA;
  __IO uint32_t UEP2_TX_DMA;
  __IO uint32_t UEP3_TX_DMA;
  __IO uint32_t UEP4_TX_DMA;
  __IO uint32_t UEP5_TX_DMA;
  __IO uint32_t UEP6_TX_DMA;
  __IO uint32_t UEP7_TX_DMA;
  __IO uint32_t UEP8_TX_DMA;
  __IO uint32_t UEP9_TX_DMA;
  __IO uint32_t UEP10_TX_DMA;
  __IO uint32_t UEP11_TX_DMA;
  __IO uint32_t UEP12_TX_DMA;
  __IO uint32_t UEP13_TX_DMA;
  __IO uint32_t UEP14_TX_DMA;
  __IO uint32_t UEP15_TX_DMA;
  __IO uint16_t UEP0_MAX_LEN;
  __IO uint16_t RESERVED2;
  __IO uint16_t UEP1_MAX_LEN;
  __IO uint16_t RESERVED3;
  __IO uint16_t UEP2_MAX_LEN;
  __IO uint16_t RESERVED4;
  __IO uint16_t UEP3_MAX_LEN;
  __IO uint16_t RESERVED5;
  __IO uint16_t UEP4_MAX_LEN;
  __IO uint16_t RESERVED6;
  __IO uint16_t UEP5_MAX_LEN;
  __IO uint16_t RESERVED7;
  __IO uint16_t UEP6_MAX_LEN;
  __IO uint16_t RESERVED8;
  __IO uint16_t UEP7_MAX_LEN;
  __IO uint16_t RESERVED9;
  __IO uint16_t UEP8_MAX_LEN;
  __IO uint16_t RESERVED10;
  __IO uint16_t UEP9_MAX_LEN;
  __IO uint16_t RESERVED11;
  __IO uint16_t UEP10_MAX_LEN;
  __IO uint16_t RESERVED12;
  __IO uint16_t UEP11_MAX_LEN;
  __IO uint16_t RESERVED13;
  __IO uint16_t UEP12_MAX_LEN;
  __IO uint16_t RESERVED14;
  __IO uint16_t UEP13_MAX_LEN;
  __IO uint16_t RESERVED15;
  __IO uint16_t UEP14_MAX_LEN;
  __IO uint16_t RESERVED16;
  __IO uint16_t UEP15_MAX_LEN;
  __IO uint16_t RESERVED17;
  __IO uint16_t UEP0_TX_LEN;
  __IO uint8_t  UEP0_TX_CTRL;
  __IO uint8_t  UEP0_RX_CTRL;
  __IO uint16_t UEP1_TX_LEN;
  __IO uint8_t  UEP1_TX_CTRL;
  __IO uint8_t  UEP1_RX_CTRL;
  __IO uint16_t UEP2_TX_LEN;
  __IO uint8_t  UEP2_TX_CTRL;
  __IO uint8_t  UEP2_RX_CTRL;
  __IO uint16_t UEP3_TX_LEN;
  __IO uint8_t  UEP3_TX_CTRL;
  __IO uint8_t  UEP3_RX_CTRL;
  __IO uint16_t UEP4_TX_LEN;
  __IO uint8_t  UEP4_TX_CTRL;
  __IO uint8_t  UEP4_RX_CTRL;
  __IO uint16_t UEP5_TX_LEN;
  __IO uint8_t  UEP5_TX_CTRL;
  __IO uint8_t  UEP5_RX_CTRL;
  __IO uint16_t UEP6_TX_LEN;
  __IO uint8_t  UEP6_TX_CTRL;
  __IO uint8_t  UEP6_RX_CTRL;
  __IO uint16_t UEP7_TX_LEN;
  __IO uint8_t  UEP7_TX_CTRL;
  __IO uint8_t  UEP7_RX_CTRL;
  __IO uint16_t UEP8_TX_LEN;
  __IO uint8_t  UEP8_TX_CTRL;
  __IO uint8_t  UEP8_RX_CTRL;
  __IO uint16_t UEP9_TX_LEN;
  __IO uint8_t  UEP9_TX_CTRL;
  __IO uint8_t  UEP9_RX_CTRL;
  __IO uint16_t UEP10_TX_LEN;
  __IO uint8_t  UEP10_TX_CTRL;
  __IO uint8_t  UEP10_RX_CTRL;
  __IO uint16_t UEP11_TX_LEN;
  __IO uint8_t  UEP11_TX_CTRL;
  __IO uint8_t  UEP11_RX_CTRL;
  __IO uint16_t UEP12_TX_LEN;
  __IO uint8_t  UEP12_TX_CTRL;
  __IO uint8_t  UEP12_RX_CTRL;
  __IO uint16_t UEP13_TX_LEN;
  __IO uint8_t  UEP13_TX_CTRL;
  __IO uint8_t  UEP13_RX_CTRL;
  __IO uint16_t UEP14_TX_LEN;
  __IO uint8_t  UEP14_TX_CTRL;
  __IO uint8_t  UEP14_RX_CTRL;
  __IO uint16_t UEP15_TX_LEN;
  __IO uint8_t  UEP15_TX_CTRL;
  __IO uint8_t  UEP15_RX_CTRL;
} USBHSD_TypeDef;

typedef struct  __attribute__((packed))
{
    __IO uint8_t  CONTROL;
    __IO uint8_t  HOST_CTRL;
    __IO uint8_t  INT_EN;
    __IO uint8_t  DEV_AD;
    __IO uint16_t FRAME_NO;
    __IO uint8_t  SUSPEND;
    __IO uint8_t  RESERVED0;
    __IO uint8_t  SPEED_TYPE;
    __IO uint8_t  MIS_ST;
    __IO uint8_t  INT_FG;
    __IO uint8_t  INT_ST;
    __IO uint16_t RX_LEN;
    __IO uint16_t RESERVED1;
    __IO uint32_t HOST_EP_CONFIG;
    __IO uint32_t HOST_EP_TYPE;
    __IO uint32_t RESERVED2;
    __IO uint32_t RESERVED3;
    __IO uint32_t RESERVED4;
    __IO uint32_t HOST_RX_DMA;
    __IO uint32_t RESERVED5;
    __IO uint32_t RESERVED6;
    __IO uint32_t RESERVED7;
    __IO uint32_t RESERVED8;
    __IO uint32_t RESERVED9;
    __IO uint32_t RESERVED10;
    __IO uint32_t RESERVED11;
    __IO uint32_t RESERVED12;
    __IO uint32_t RESERVED13;
    __IO uint32_t RESERVED14;
    __IO uint32_t RESERVED15;
    __IO uint32_t RESERVED16;
    __IO uint32_t RESERVED17;
    __IO uint32_t RESERVED18;
    __IO uint32_t RESERVED19;
    __IO uint32_t HOST_TX_DMA;
    __IO uint32_t RESERVED20;
    __IO uint32_t RESERVED21;
    __IO uint32_t RESERVED22;
    __IO uint32_t RESERVED23;
    __IO uint32_t RESERVED24;
    __IO uint32_t RESERVED25;
    __IO uint32_t RESERVED26;
    __IO uint32_t RESERVED27;
    __IO uint32_t RESERVED28;
    __IO uint32_t RESERVED29;
    __IO uint32_t RESERVED30;
    __IO uint32_t RESERVED31;
    __IO uint32_t RESERVED32;
    __IO uint32_t RESERVED33;
    __IO uint16_t HOST_RX_MAX_LEN;
    __IO uint16_t RESERVED34;
    __IO uint32_t RESERVED35;
    __IO uint32_t RESERVED36;
    __IO uint32_t RESERVED37;
    __IO uint32_t RESERVED38;
    __IO uint32_t RESERVED39;
    __IO uint32_t RESERVED40;
    __IO uint32_t RESERVED41;
    __IO uint32_t RESERVED42;
    __IO uint32_t RESERVED43;
    __IO uint32_t RESERVED44;
    __IO uint32_t RESERVED45;
    __IO uint32_t RESERVED46;
    __IO uint32_t RESERVED47;
    __IO uint32_t RESERVED48;
    __IO uint32_t RESERVED49;
    __IO uint8_t  HOST_EP_PID;
    __IO uint8_t  RESERVED50;
    __IO uint8_t  RESERVED51;
    __IO uint8_t  HOST_RX_CTRL;
    __IO uint16_t HOST_TX_LEN;
    __IO uint8_t  HOST_TX_CTRL;
    __IO uint8_t  RESERVED52;
    __IO uint16_t HOST_SPLIT_DATA;
} USBHSH_TypeDef;

#endif	// #if defined(CH32V30x)

/* USBFS Registers */
typedef struct
{
	__IO uint8_t  BASE_CTRL;
	__IO uint8_t  UDEV_CTRL;
	__IO uint8_t  INT_EN;
	__IO uint8_t  DEV_ADDR;
	__IO uint8_t  Reserve0;
	__IO uint8_t  MIS_ST;
	__IO uint8_t  INT_FG;
	__IO uint8_t  INT_ST;
	__IO uint32_t RX_LEN;
	__IO uint8_t  UEP4_1_MOD;
	__IO uint8_t  UEP2_3_MOD;
	__IO uint8_t  UEP5_6_MOD;
	__IO uint8_t  UEP7_MOD;
	__IO uint32_t UEP0_DMA;
	__IO uint32_t UEP1_DMA;
	__IO uint32_t UEP2_DMA;
	__IO uint32_t UEP3_DMA;
	__IO uint32_t UEP4_DMA;
	__IO uint32_t UEP5_DMA;
	__IO uint32_t UEP6_DMA;
	__IO uint32_t UEP7_DMA;
	__IO uint16_t UEP0_TX_LEN;
	__IO uint8_t  UEP0_TX_CTRL;
	__IO uint8_t  UEP0_RX_CTRL;
	__IO uint16_t UEP1_TX_LEN;
	__IO uint8_t  UEP1_TX_CTRL;
	__IO uint8_t  UEP1_RX_CTRL;
	__IO uint16_t UEP2_TX_LEN;
	__IO uint8_t  UEP2_TX_CTRL;
	__IO uint8_t  UEP2_RX_CTRL;
	__IO uint16_t UEP3_TX_LEN;
	__IO uint8_t  UEP3_TX_CTRL;
	__IO uint8_t  UEP3_RX_CTRL;
	__IO uint16_t UEP4_TX_LEN;
	__IO uint8_t  UEP4_TX_CTRL;
	__IO uint8_t  UEP4_RX_CTRL;
	__IO uint16_t UEP5_TX_LEN;
	__IO uint8_t  UEP5_TX_CTRL;
	__IO uint8_t  UEP5_RX_CTRL;
	__IO uint16_t UEP6_TX_LEN;
	__IO uint8_t  UEP6_TX_CTRL;
	__IO uint8_t  UEP6_RX_CTRL;
	__IO uint16_t UEP7_TX_LEN;
	__IO uint8_t  UEP7_TX_CTRL;
	__IO uint8_t  UEP7_RX_CTRL;
	__IO uint32_t Reserve1;
	__IO uint32_t OTG_CR;
	__IO uint32_t OTG_SR;
} USBOTG_FS_TypeDef;

typedef struct
{
	__IO uint8_t   BASE_CTRL;
	__IO uint8_t   HOST_CTRL;
	__IO uint8_t   INT_EN;
	__IO uint8_t   DEV_ADDR;
	__IO uint8_t   Reserve0;
	__IO uint8_t   MIS_ST;
	__IO uint8_t   INT_FG;
	__IO uint8_t   INT_ST;
	__IO uint16_t  RX_LEN;
	__IO uint16_t  Reserve1;
	__IO uint8_t   Reserve2;
	__IO uint8_t   HOST_EP_MOD;
	__IO uint16_t  Reserve3;
	__IO uint32_t  Reserve4;
	__IO uint32_t  Reserve5;
	__IO uint32_t  HOST_RX_DMA;
	__IO uint32_t  HOST_TX_DMA;
	__IO uint32_t  Reserve6;
	__IO uint32_t  Reserve7;
	__IO uint32_t  Reserve8;
	__IO uint32_t  Reserve9;
	__IO uint32_t  Reserve10;
	__IO uint16_t  Reserve11;
	__IO uint16_t  HOST_SETUP;
	__IO uint8_t   HOST_EP_PID;
	__IO uint8_t   Reserve12;
	__IO uint8_t   Reserve13;
	__IO uint8_t   HOST_RX_CTRL;
	__IO uint16_t  HOST_TX_LEN;
	__IO uint8_t   HOST_TX_CTRL;
	__IO uint8_t   Reserve14;
	__IO uint32_t  Reserve15;
	__IO uint32_t  Reserve16;
	__IO uint32_t  Reserve17;
	__IO uint32_t  Reserve18;
	__IO uint32_t  Reserve19;
	__IO uint32_t  OTG_CR;
	__IO uint32_t  OTG_SR;
} USBOTG_FS_HOST_TypeDef;

#if defined(CH32V30x)
/* Ethernet MAC */
typedef struct
{
  __IO uint32_t MACCR;
  __IO uint32_t MACFFR;
  __IO uint32_t MACHTHR;
  __IO uint32_t MACHTLR;
  __IO uint32_t MACMIIAR;
  __IO uint32_t MACMIIDR;
  __IO uint32_t MACFCR;
  __IO uint32_t MACVLANTR;
       uint32_t RESERVED0[2];
  __IO uint32_t MACRWUFFR;
  __IO uint32_t MACPMTCSR;
       uint32_t RESERVED1[2];
  __IO uint32_t MACSR;
  __IO uint32_t MACIMR;
  __IO uint32_t MACA0HR;
  __IO uint32_t MACA0LR;
  __IO uint32_t MACA1HR;
  __IO uint32_t MACA1LR;
  __IO uint32_t MACA2HR;
  __IO uint32_t MACA2LR;
  __IO uint32_t MACA3HR;
  __IO uint32_t MACA3LR;
       uint32_t RESERVED2[40];
  __IO uint32_t MMCCR;
  __IO uint32_t MMCRIR;
  __IO uint32_t MMCTIR;
  __IO uint32_t MMCRIMR;
  __IO uint32_t MMCTIMR;
       uint32_t RESERVED3[14];
  __IO uint32_t MMCTGFSCCR;
  __IO uint32_t MMCTGFMSCCR;
       uint32_t RESERVED4[5];
  __IO uint32_t MMCTGFCR;
       uint32_t RESERVED5[10];
  __IO uint32_t MMCRFCECR;
  __IO uint32_t MMCRFAECR;
       uint32_t RESERVED6[10];
  __IO uint32_t MMCRGUFCR;
       uint32_t RESERVED7[334];
  __IO uint32_t PTPTSCR;
  __IO uint32_t PTPSSIR;
  __IO uint32_t PTPTSHR;
  __IO uint32_t PTPTSLR;
  __IO uint32_t PTPTSHUR;
  __IO uint32_t PTPTSLUR;
  __IO uint32_t PTPTSAR;
  __IO uint32_t PTPTTHR;
  __IO uint32_t PTPTTLR;
       uint32_t RESERVED8[567];
  __IO uint32_t DMABMR;
  __IO uint32_t DMATPDR;
  __IO uint32_t DMARPDR;
  __IO uint32_t DMARDLAR;
  __IO uint32_t DMATDLAR;
  __IO uint32_t DMASR;
  __IO uint32_t DMAOMR;
  __IO uint32_t DMAIER;
  __IO uint32_t DMAMFBOCR;
       uint32_t RESERVED9[9];
  __IO uint32_t DMACHTDR;
  __IO uint32_t DMACHRDR;
  __IO uint32_t DMACHTBAR;
  __IO uint32_t DMACHRBAR;
} ETH_TypeDef;
#endif // #if defined(CH32V30x)

#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
/* ETH10M Registers */
typedef struct
{
    __IO uint8_t reserved1;
    __IO uint8_t reserved2;
    __IO uint8_t reserved3;
    __IO uint8_t EIE;

    __IO uint8_t EIR;
    __IO uint8_t ESTAT;
    __IO uint8_t ECON2;
    __IO uint8_t ECON1;

    __IO uint16_t ETXST;
    __IO uint16_t ETXLN;

    __IO uint16_t ERXST;
    __IO uint16_t ERXLN;

    __IO uint32_t HTL;
    __IO uint32_t HTH;

    __IO uint8_t ERXFON;
    __IO uint8_t MACON1;
    __IO uint8_t MACON2;
    __IO uint8_t MABBIPG;

    __IO uint16_t EPAUS;
    __IO uint16_t MAMXFL;

    __IO uint16_t MIRD;
    __IO uint16_t reserved4;

    __IO uint8_t MIERGADR;
    __IO uint8_t MISTAT;
    __IO uint16_t MIWR;

    __IO uint32_t MAADRL;

    __IO uint16_t MAADRH;
    __IO uint16_t reserved5;
} ETH10M_TypeDef;
#endif

#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
/* OSC Registers */
typedef struct
{
    __IO uint32_t HSE_CAL_CTRL;
    __IO uint32_t Reserve0;
    __IO uint16_t Reserve1;
    __IO uint16_t LSI32K_TUNE;
    __IO uint32_t Reserve2;
    __IO uint32_t Reserve3;
    __IO uint32_t Reserve4;
    __IO uint32_t Reserve5;
    __IO uint8_t  Reserve6;
    __IO uint8_t  LSI32K_CAL_CFG;
    __IO uint16_t Reserve7;
    __IO uint16_t LSI32K_CAL_STATR;
    __IO uint8_t  LSI32K_CAL_OV_CNT;
    __IO uint8_t  LSI32K_CAL_CTRL;
} OSC_TypeDef;

#endif

#endif // #if defined(CH32V20x) || defined(CH32V30x)


#if defined(CH32X03x)
/* Touch Sensor, Mirrors Analog to Digital Converter */
typedef struct
{
	__IO uint32_t RESERVED0[3];
	__IO uint32_t CHARGE1;
	__IO uint32_t CHARGE2;
	__IO uint32_t RESERVED1[10];
	__IO uint32_t CHGOFFSET;
	__IO uint32_t RESERVED2[3];
	__IO uint32_t DR_ACT_DCG;
} TKEY_TypeDef;

/* Op amp / comparator */
typedef struct
{
	__IO uint16_t CFGR1;
	__IO uint16_t CFGR2;
	__IO uint32_t CTLR1;
	__IO uint32_t CTLR2;
	__IO uint32_t OPA_KEY;
	__IO uint32_t CMP_KEY;
	__IO uint32_t POLL_KEY;
} OPACMP_TypeDef;

/* USB Full Speed Device Mode */
typedef struct
{
	__IO uint8_t BASE_CTRL; //XXX (spelling)
	__IO uint8_t UDEV_CTRL; // or host ctlr
	__IO uint8_t INT_EN;
	__IO uint8_t DEV_ADDR;
	__IO uint8_t RESERVED0;
	__IO uint8_t MIS_ST;
	__IO uint8_t INT_FG;
	__IO uint8_t INT_ST;
	__IO uint16_t RX_LEN;
	__IO uint16_t RESERVED1;
	__IO uint8_t UEP4_1_MOD;
	__IO uint8_t UEP2_3_MOD; // Also HOST_EP_MOD
	__IO uint8_t UEP567_MOD;
	__IO uint8_t RESERVED2;

	__IO uint32_t UEP0_DMA;
	__IO uint32_t UEP1_DMA;
	__IO uint32_t UEP2_DMA; // Also HOST_RX_DMA
	__IO uint32_t UEP3_DMA; // Also HOST_TX_DMA

	//__IO uint32_t UEP0_CTRL;
	__IO uint16_t UEP0_TX_LEN;
	__IO uint16_t UEP0_CTRL_H;

	//__IO uint32_t UEP1_CTRL;
	__IO uint16_t UEP1_TX_LEN;
	__IO uint16_t UEP1_CTRL_H; // Also HOST_SETUP

	//__IO uint32_t UEP2_CTRL;
	__IO uint16_t UEP2_TX_LEN; // Also HOST_PID
	__IO uint16_t UEP2_CTRL_H; // Also HOST_RX_CTL

	//__IO uint32_t UEP3_CTRL;
	__IO uint16_t UEP3_TX_LEN; // Also HOST_TX_LEN
	__IO uint16_t UEP3_CTRL_H; // Also HOST_TX_CTL

	//__IO uint32_t UEP4_CTRL;
	__IO uint16_t UEP4_TX_LEN;
	__IO uint16_t UEP4_CTRL_H;

	__IO uint32_t RESERVED3[8];

	__IO uint32_t UEP5_DMA;
	__IO uint32_t UEP6_DMA;
	__IO uint32_t UEP7_DMA;

	__IO uint32_t RESERVED4;

	//__IO uint32_t UEP5_CTRL;
	__IO uint16_t UEP5_TX_LEN;
	__IO uint16_t UEP5_CTRL_H;

	//__IO uint32_t UEP6_CTRL;
	__IO uint16_t UEP6_TX_LEN;
	__IO uint16_t UEP6_CTRL_H;

	//__IO uint32_t UEP7_CTRL;
	__IO uint16_t UEP7_TX_LEN;
	__IO uint16_t UEP7_CTRL_H;

	__IO uint32_t UEPX_MOD;
} USBFS_TypeDef;



#define USB_PHY_V33 (1<<6)
#define USB_IOEN (1<<7)


#define USBFSD_UEP_MOD_BASE         0x4002340C
#define USBFSD_UEP_DMA_BASE         0x40023410
#define USBFSD_UEP_LEN_BASE         0x40023420
#define USBFSD_UEP_CTL_BASE         0x40023422
#define USBFSD_UEP_RX_EN            0x08
#define USBFSD_UEP_TX_EN            0x04
#define USBFSD_UEP_BUF_MOD          0x01
#define USBFSD_UEP_MOD( N )         (*((volatile uint8_t *)( USBFSD_UEP_MOD_BASE + N )))
#define USBFSD_UEP_TX_CTRL( N )     (*((volatile uint8_t *)( USBFSD_UEP_CTL_BASE + N * 0x04 )))
#define USBFSD_UEP_RX_CTRL( N )     (*((volatile uint8_t *)( USBFSD_UEP_CTL_BASE + N * 0x04 )))
#define USBFSD_UEP_DMA( N )         (*((volatile uint32_t *)( USBFSD_UEP_DMA_BASE + N * 0x04 )))
#define USBFSD_UEP_BUF( N )         ((uint8_t *)(*((volatile uint32_t *)( USBFSD_UEP_DMA_BASE + N * 0x04 ))) + 0x20000000)
#define USBFSD_UEP_TLEN( N )        (*((volatile uint16_t *)( USBFSD_UEP_LEN_BASE + N * 0x04 )))

/* R8_UEPn_TX_CTRL */
#define USBFS_UEP_T_AUTO_TOG        (1<<4)      // enable automatic toggle after successful transfer completion on endpoint 1/2/3: 0=manual toggle, 1=automatic toggle
#define USBFS_UEP_T_TOG             (1<<6)      // prepared data toggle flag of USB endpoint X transmittal (IN): 0=DATA0, 1=DATA1
#define USBFS_UEP_T_RES_MASK        (3<<0)      // bit mask of handshake response type for USB endpoint X transmittal (IN)
#define USBFS_UEP_T_RES_ACK         (0<<1)
#define USBFS_UEP_T_RES_NONE        (1<<0)
#define USBFS_UEP_T_RES_NAK         (1<<1)
#define USBFS_UEP_T_RES_STALL       (3<<0)
// bUEP_T_RES1 & bUEP_T_RES0: handshake response type for USB endpoint X transmittal (IN)
//   00: DATA0 or DATA1 then expecting ACK (ready)
//   01: DATA0 or DATA1 then expecting no response, time out from host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)
// host aux setup

/* R8_UEPn_RX_CTRL, n=0-7 */
#define USBFS_UEP_R_AUTO_TOG        (1<<4)      // enable automatic toggle after successful transfer completion on endpoint 1/2/3: 0=manual toggle, 1=automatic toggle
#define USBFS_UEP_R_TOG             (1<<7)      // expected data toggle flag of USB endpoint X receiving (OUT): 0=DATA0, 1=DATA1
#define USBFS_UEP_R_RES_MASK        (3<<2)      // bit mask of handshake response type for USB endpoint X receiving (OUT)
#define USBFS_UEP_R_RES_ACK         (0<<3)
#define USBFS_UEP_R_RES_NONE        (1<<2)
#define USBFS_UEP_R_RES_NAK         (1<<3)
#define USBFS_UEP_R_RES_STALL       (3<<2)


#define EP1_T_EN					(1<<6)
#define EP2_T_EN					(1<<2)
#define EP3_T_EN					(1<<6)
#define EP4_T_EN					(1<<2)
#define EP1_R_EN					(1<<7)
#define EP2_R_EN					(1<<3)
#define EP3_R_EN					(1<<7)
#define EP4_R_EN					(1<<3)


/* R8_USB_CTRL */
#define USBFS_UC_HOST_MODE          0x80
#define USBFS_UC_LOW_SPEED          0x40
#define USBFS_UC_DEV_PU_EN          0x20
#define USBFS_UC_SYS_CTRL_MASK      0x30
#define USBFS_UC_SYS_CTRL0          0x00
#define USBFS_UC_SYS_CTRL1          0x10
#define USBFS_UC_SYS_CTRL2          0x20
#define USBFS_UC_SYS_CTRL3          0x30
#define USBFS_UC_INT_BUSY           0x08
#define USBFS_UC_RESET_SIE          0x04
#define USBFS_UC_CLR_ALL            0x02
#define USBFS_UC_DMA_EN             0x01

/* R8_USB_INT_EN */
#define USBFS_UIE_DEV_SOF           0x80
#define USBFS_UIE_DEV_NAK           0x40
#define USBFS_UIE_FIFO_OV           0x10
#define USBFS_UIE_HST_SOF           0x08
#define USBFS_UIE_SUSPEND           0x04
#define USBFS_UIE_TRANSFER          0x02
#define USBFS_UIE_DETECT            0x01
#define USBFS_UIE_BUS_RST           0x01

/* R8_USB_DEV_AD */
#define USBFS_UDA_GP_BIT            0x80
#define USBFS_USB_ADDR_MASK         0x7F

/* R8_USB_MIS_ST */
#define USBFS_UMS_SOF_PRES          0x80
#define USBFS_UMS_SOF_ACT           0x40
#define USBFS_UMS_SIE_FREE          0x20
#define USBFS_UMS_R_FIFO_RDY        0x10
#define USBFS_UMS_BUS_RESET         0x08
#define USBFS_UMS_SUSPEND           0x04
#define USBFS_UMS_DM_LEVEL          0x02
#define USBFS_UMS_DEV_ATTACH        0x01




#define USBFS_UDA_GP_BIT            0x80
#define USBFS_USB_ADDR_MASK         0x7F

#define DEF_USBD_UEP0_SIZE		   64	 /* usb hs/fs device end-point 0 size */
#define UEP_SIZE 64

#define DEF_UEP_IN                  0x80
#define DEF_UEP_OUT                 0x00
#define DEF_UEP_BUSY                0x01
#define DEF_UEP_FREE                0x00

#define DEF_UEP0 0
#define DEF_UEP1 1
#define DEF_UEP2 2
#define DEF_UEP3 3
#define DEF_UEP4 4
#define DEF_UEP5 5
#define DEF_UEP6 6
#define DEF_UEP7 7
#define UNUM_EP 8



/* USB Host Mode */

typedef struct
{
	__IO uint8_t RESERVED0;
	__IO uint8_t HOST_CTRL;
	__IO uint8_t RESERVED1;
	__IO uint8_t RESERVED2;
	__IO uint8_t RESERVED3;
	__IO uint8_t RESERVED4;
	__IO uint8_t RESERVED5;
	__IO uint8_t RESERVED6;
	__IO uint16_t RESERVED7;
	__IO uint16_t RESERVED8;
	__IO uint8_t RESERVED9;
	__IO uint8_t HOST_EP_MOD;
	__IO uint8_t RESERVED10;
	__IO uint8_t RESERVED11;

	__IO uint32_t RESERVED12;
	__IO uint32_t RESERVED13;
	__IO uint32_t HOST_RX_DMA;
	__IO uint32_t HOST_TX_DMA;

	__IO uint16_t RESERVED14;
	__IO uint16_t RESERVED15;
	__IO uint16_t RESERVED16;

	__IO uint16_t HOST_SETUP;
	__IO uint16_t HOST_EP_PID;
	__IO uint16_t HOST_RX_CTL;
	__IO uint16_t HOST_TX_LEN;
	__IO uint16_t HOST_TX_CTL;

	__IO uint16_t RESERVED20;
	__IO uint16_t RESERVED21;

	__IO uint32_t RESERVED22[8];

	__IO uint32_t RESERVED23;
	__IO uint32_t RESERVED24;
	__IO uint32_t RESERVED25;

	__IO uint32_t RESERVED26;

	__IO uint16_t RESERVED27;
	__IO uint16_t RESERVED28;

	__IO uint16_t RESERVED29;
	__IO uint16_t RESERVED30;

	__IO uint16_t RESERVED31;
	__IO uint16_t RESERVED32;

	__IO uint32_t RESERVED33;
} USBDH_TypeDef;


/* USB Power Delivery */
typedef struct
{
	__IO uint32_t CONFIG;
	__IO uint32_t CONTROL;
	__IO uint32_t STATUS;
	__IO uint32_t PORT;
	__IO uint32_t DMA;
} USBPD_TypeDef;


/* USB Power Delivery */
typedef struct
{
	__IO uint16_t CONFIG;
	__IO uint16_t BCM_CLK_CNT;

	__IO uint8_t CONTROL;
	__IO uint8_t TX_SEL;
	__IO uint16_t BMC_TX_SZ;

	__IO uint8_t DATA_BUF;
	__IO uint8_t STATUS;
	__IO uint16_t BMC_BYTE_CNT;

	__IO uint16_t PORT_CC1;
	__IO uint16_t PORT_CC2;

	__IO uint32_t USBPD_DMA;
} USBPD_DETAILED_TypeDef;

#endif // #if defined(CH32X03x)


#endif

/* Peripheral memory map */
#ifdef __ASSEMBLER__
#define FLASH_BASE                              (0x08000000) /* FLASH base address in the alias region */
#define SRAM_BASE                               (0x20000000) /* SRAM base address in the alias region */
#define PERIPH_BASE                             (0x40000000) /* Peripheral base address in the alias region */
#define CORE_PERIPH_BASE                        (0xE0000000) /* System peripherals base address in the alias region */
#else
#define FLASH_BASE                              ((uint32_t)0x08000000) /* FLASH base address in the alias region */
#define SRAM_BASE                               ((uint32_t)0x20000000) /* SRAM base address in the alias region */
#define PERIPH_BASE                             ((uint32_t)0x40000000) /* Peripheral base address in the alias region */
#define CORE_PERIPH_BASE                        ((uint32_t)0xE0000000) /* System peripherals base address in the alias region */
#endif

#if defined(CH32V30x)
#ifdef __ASSEMBLER__
#define FSMC_R_BASE           					(b 0xA0000000) /* FSMC registers base address */
#else
#define FSMC_R_BASE           					((uint32_t)0xA0000000) /* FSMC registers base address */
#endif
#endif

#define APB1PERIPH_BASE                         (PERIPH_BASE)
#define APB2PERIPH_BASE                         (PERIPH_BASE + 0x10000)
#define AHBPERIPH_BASE                          (PERIPH_BASE + 0x20000)

#define TIM2_BASE                               (APB1PERIPH_BASE + 0x0000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
#define TIM3_BASE                               (APB1PERIPH_BASE + 0x0400)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define TIM4_BASE                               (APB1PERIPH_BASE + 0x0800)
#define TIM5_BASE                               (APB1PERIPH_BASE + 0x0C00)
#endif // CH32V10x, CH32V20x, CH32V30x
#if defined(CH32V30x)	// CH32V30x
#define TIM6_BASE             					(APB1PERIPH_BASE + 0x1000)
#define TIM7_BASE             					(APB1PERIPH_BASE + 0x1400)
#define UART6_BASE            					(APB1PERIPH_BASE + 0x1800)
#define UART7_BASE            					(APB1PERIPH_BASE + 0x1C00)
#define UART8_BASE            					(APB1PERIPH_BASE + 0x2000)
#endif					// CH32V30x
#if defined(CH32V10x)	// CH32V10x
#define TIM6_BASE                               (APB1PERIPH_BASE + 0x1000)
#define TIM7_BASE                               (APB1PERIPH_BASE + 0x1400)
#define TIM12_BASE                              (APB1PERIPH_BASE + 0x1800)
#define TIM13_BASE                              (APB1PERIPH_BASE + 0x1C00)
#define TIM14_BASE                              (APB1PERIPH_BASE + 0x2000)
#endif					// CH32V10x
#if defined(CH32V003) || defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RTC_BASE                                (APB1PERIPH_BASE + 0x2800)
#endif
#endif
#define WWDG_BASE                               (APB1PERIPH_BASE + 0x2C00)
#define IWDG_BASE                               (APB1PERIPH_BASE + 0x3000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define SPI2_BASE                               (APB1PERIPH_BASE + 0x3800)
#if defined(CH32V10x) || defined(CH32V30x)
#define SPI3_BASE             					(APB1PERIPH_BASE + 0x3C00)
#endif // defined(CH32V30x) || defined(CH32V10x)
#endif
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
#define USART2_BASE                             (APB1PERIPH_BASE + 0x4400)
#define USART3_BASE                             (APB1PERIPH_BASE + 0x4800)
#define UART4_BASE                              (APB1PERIPH_BASE + 0x4C00)
#if defined(CH32V10x) || defined(CH32V30x)
#define UART5_BASE            					(APB1PERIPH_BASE + 0x5000)
#endif // defined(CH32V30x) || defined(CH32V10x)
#endif // defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C1_BASE                               (APB1PERIPH_BASE + 0x5400)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C2_BASE                               (APB1PERIPH_BASE + 0x5800)
#endif
#if defined(CH32V20x) || defined(CH32V30x)
#define CAN1_BASE                               (APB1PERIPH_BASE + 0x6400)
#endif
#if defined(CH32V30x)
#define CAN2_BASE             (APB1PERIPH_BASE + 0x6800)
#endif
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define BKP_BASE                                (APB1PERIPH_BASE + 0x6C00)
#endif
#define PWR_BASE                                (APB1PERIPH_BASE + 0x7000)
#if defined(CH32V10x) || defined(CH32V30x)
#define DAC_BASE              					(APB1PERIPH_BASE + 0x7400)
#endif

#define AFIO_BASE                               (APB2PERIPH_BASE + 0x0000)
#define EXTI_BASE                               (APB2PERIPH_BASE + 0x0400)
#define GPIOA_BASE                              (APB2PERIPH_BASE + 0x0800)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
#define GPIOB_BASE                              (APB2PERIPH_BASE + 0x0C00)
#endif
#define GPIOC_BASE                              (APB2PERIPH_BASE + 0x1000)
#define GPIOD_BASE                              (APB2PERIPH_BASE + 0x1400)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define GPIOE_BASE                              (APB2PERIPH_BASE + 0x1800)
#define GPIOF_BASE                              (APB2PERIPH_BASE + 0x1C00)
#define GPIOG_BASE                              (APB2PERIPH_BASE + 0x2000)
#endif
#define ADC1_BASE                               (APB2PERIPH_BASE + 0x2400)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC2_BASE                               (APB2PERIPH_BASE + 0x2800)
#endif
#define TIM1_BASE                               (APB2PERIPH_BASE + 0x2C00)
#define SPI1_BASE                               (APB2PERIPH_BASE + 0x3000)
#if defined(CH32V10x) || defined(CH32V30x)
#define TIM8_BASE             					(APB2PERIPH_BASE + 0x3400)
#endif
#define USART1_BASE                             (APB2PERIPH_BASE + 0x3800)
#if defined(CH32V10x) || defined(CH32V30x)
#define ADC3_BASE             					(APB2PERIPH_BASE + 0x3C00)
#define TIM15_BASE            					(APB2PERIPH_BASE + 0x4000)
#define TIM16_BASE            					(APB2PERIPH_BASE + 0x4400)
#define TIM17_BASE            					(APB2PERIPH_BASE + 0x4800)
#define TIM9_BASE             					(APB2PERIPH_BASE + 0x4C00)
#define TIM10_BASE            					(APB2PERIPH_BASE + 0x5000)
#define TIM11_BASE           					(APB2PERIPH_BASE + 0x5400)
#endif
#if defined(CH32V30x)
#define SDIO_BASE            					(APB2PERIPH_BASE + 0x8000)
#endif

#define DMA1_BASE                               (AHBPERIPH_BASE + 0x0000)
#define DMA1_Channel1_BASE                      (AHBPERIPH_BASE + 0x0008)
#define DMA1_Channel2_BASE                      (AHBPERIPH_BASE + 0x001C)
#define DMA1_Channel3_BASE                      (AHBPERIPH_BASE + 0x0030)
#define DMA1_Channel4_BASE                      (AHBPERIPH_BASE + 0x0044)
#define DMA1_Channel5_BASE                      (AHBPERIPH_BASE + 0x0058)
#define DMA1_Channel6_BASE                      (AHBPERIPH_BASE + 0x006C)
#define DMA1_Channel7_BASE                      (AHBPERIPH_BASE + 0x0080)
#if defined(CH32V20x)
#define DMA1_Channel8_BASE                      (AHBPERIPH_BASE + 0x0094)
#endif
#if defined(CH32V10x) || defined(CH32V30x)
#define DMA2_BASE             					(AHBPERIPH_BASE + 0x0400)
#define DMA2_Channel1_BASE    					(AHBPERIPH_BASE + 0x0408)
#define DMA2_Channel2_BASE    					(AHBPERIPH_BASE + 0x041C)
#define DMA2_Channel3_BASE    					(AHBPERIPH_BASE + 0x0430)
#define DMA2_Channel4_BASE    					(AHBPERIPH_BASE + 0x0444)
#define DMA2_Channel5_BASE    					(AHBPERIPH_BASE + 0x0458)
#if defined(CH32V30x)
#define DMA2_Channel6_BASE    					(AHBPERIPH_BASE + 0x046C)
#define DMA2_Channel7_BASE    					(AHBPERIPH_BASE + 0x0480)
#define DMA2_Channel8_BASE    					(AHBPERIPH_BASE + 0x0490)
#define DMA2_Channel9_BASE    					(AHBPERIPH_BASE + 0x04A0)
#define DMA2_Channel10_BASE   					(AHBPERIPH_BASE + 0x04B0)
#define DMA2_Channel11_BASE   					(AHBPERIPH_BASE + 0x04C0)
#define DMA2_EXTEN_BASE       					(AHBPERIPH_BASE + 0x04D0)
#endif // defined(CH32V30x)
#endif
#define RCC_BASE                                (AHBPERIPH_BASE + 0x1000)

#define FLASH_R_BASE                            (AHBPERIPH_BASE + 0x2000) /* Flash registers base address */

#if defined(CH32V20x)
#define CRC_BASE                                (AHBPERIPH_BASE + 0x3000)
#define OPA_BASE                                (AHBPERIPH_BASE + 0x3804)
#define ETH10M_BASE                             (AHBPERIPH_BASE + 0x8000)

#define USBFS_BASE                              ((uint32_t)0x50000000)
#elif defined(CH32X03x)

#define OPA_BASE                                (AHBPERIPH_BASE + 0x6000)
#define USBFS_BASE                              (AHBPERIPH_BASE + 0x3400)
#define USBPD_BASE                              (AHBPERIPH_BASE + 0x7000)

#elif defined(CH32V30x)
#define CRC_BASE              					(AHBPERIPH_BASE + 0x3000)
#define USBHS_BASE            					(AHBPERIPH_BASE + 0x3400)
#define OPA_BASE              					(AHBPERIPH_BASE + 0x3804)
#define RNG_BASE              					(AHBPERIPH_BASE + 0x3C00)

#define ETH_BASE              					(AHBPERIPH_BASE + 0x8000)
#define ETH_MAC_BASE          					(ETH_BASE)
#define ETH_MMC_BASE          					(ETH_BASE + 0x0100)
#define ETH_PTP_BASE          					(ETH_BASE + 0x0700)
#define ETH_DMA_BASE          					(ETH_BASE + 0x1000)

#define USBFS_BASE            					((uint32_t)0x50000000)
#define DVP_BASE              					((uint32_t)0x50050000)

#define FSMC_Bank1_R_BASE     					(FSMC_R_BASE + 0x0000)
#define FSMC_Bank1E_R_BASE    					(FSMC_R_BASE + 0x0104)
#define FSMC_Bank2_R_BASE     					(FSMC_R_BASE + 0x0060)
#elif defined(CH32V10x)
#define CRC_BASE                                (AHBPERIPH_BASE + 0x3000)
#define DBGMCU_BASE                             ((uint32_t)0xE000D000)
#endif

#define OB_BASE                                 ((uint32_t)0x1FFFF800)    /* Flash Option Bytes base address */
#define ESIG_BASE                               ((uint32_t)0x1FFFF7E0)

#if defined(CH32V003) || defined(CH32V10x)
#define EXTEN_BASE                              ((uint32_t)0x40023800)
#elif defined(CH32V20x) || defined(CH32V30x)
#define EXTEN_BASE                              (AHBPERIPH_BASE + 0x3800)
#endif

#define PFIC_BASE    (CORE_PERIPH_BASE + 0xE000)
#define SysTick_BASE    (CORE_PERIPH_BASE + 0xF000)

#if defined(CH32V20x)
#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
#define OSC_BASE                                (AHBPERIPH_BASE + 0x202C)
#endif
#endif



// AFIO CTLR Bits
#define PB6_FILT_EN    (1<<27)
#define PB5_FILT_EN    (1<<26)
#define PA4_FILT_EN    (1<<25)
#define PA3_FILT_EN    (1<<24)
#define UDM_BC_CMPO	   (1<<19)
#define UDP_BC_CMPO    (1<<17)
#define UDM_BC_VSRC    (1<<17)
#define UDP_BC_VSRC    (1<<16)
#define USBPD_IN_HVT   (1<<9)
#define USBPD_PHY_V33  (1<<8)
#define USB_IOEN       (1<<7)
#define USB_PHY_V33    (1<<6)
#define UDP_PUE_00     (0b00<<2)
#define UDP_PUE_01     (0b01<<2)
#define UDP_PUE_10     (0b10<<2)
#define UDP_PUE_11     (0b11<<2)
#define UDM_PUE_00     (0b00<<0)
#define UDM_PUE_01     (0b01<<0)
#define UDM_PUE_10     (0b10<<0)
#define UDM_PUE_11     (0b11<<0)
#define UDP_PUE_MASK                0x0000000C
#define UDP_PUE_DISABLE             0x00000000
#define UDP_PUE_35UA                0x00000004
#define UDP_PUE_10K                 0x00000008
#define UDP_PUE_1K5                 0x0000000C
#define UDM_PUE_MASK                0x00000003
#define UDM_PUE_DISABLE             0x00000000
#define UDM_PUE_35UA                0x00000001
#define UDM_PUE_10K                 0x00000002
#define UDM_PUE_1K5                 0x00000003


// USB PD Bits
#define IE_TX_END      (1<<15)
#define IE_RX_RESET    (1<<14)
#define IE_RX_ACT      (1<<13)
#define IE_RX_BYTE     (1<<12)
#define IE_RX_BIT      (1<<11)
#define IE_PD_IO       (1<<10)
#define WAKE_POLAR     (1<<5)
#define PD_RST_EN      (1<<4)
#define PD_DMA_EN      (1<<3)
#define CC_SEL         (1<<2)
#define PD_ALL_CLR     (1<<1)
#define PD_FILT_EN     (1<<0)
#define BMC_CLK_CNT_MASK  (0xff)

//R8_CONTROL
#define BMC_BYTE_HI    (1<<7)
#define TX_BIT_BACK    (1<<6)
#define DATA_FLAG      (1<<5)
#define RX_STATE_MASK  (0x7<<2)
#define RX_STATE_0     (1<<2)
#define RX_STATE_1     (1<<3)
#define RX_STATE_2     (1<<4)
#define BMC_START      (1<<1)
#define PD_TX_EN       (1<<0)

#define TX_SEL4_MASK   (3<<6)
#define TX_SEL4_0      (1<<6)
#define TX_SEL4_1      (1<<7)

#define TX_SEL3_MASK   (3<<4)
#define TX_SEL3_0      (1<<4)
#define TX_SEL3_1      (1<<5)

#define TX_SEL2_MASK   (3<<2)
#define TX_SEL2_0      (1<<2)
#define TX_SEL2_1      (1<<3)

#define TX_SEL1        (1<<0)

#define BMC_TX_SZ_MASK (0x1ff)

//R8_STATUS
#define IF_TX_END      (1<<7)
#define IF_RX_RESET    (1<<6)
#define IF_RX_ACT      (1<<5)
#define IF_RX_BYTE     (1<<4)
#define IF_RX_BIT      (1<<3)
#define IFBUF_ERR      (1<<2)
#define BMC_AUX_MASK   (3<<0)
#define BMC_AUX_1      (1<<1)
#define BMC_AUX_0      (1<<0)

// PORT CC1
#define CC1_CE_MASK    (7<<5)
#define CC1_CE_0       (1<<5)
#define CC1_CE_1       (2<<5)
#define CC1_CE_2       (4<<5)

#define CC1_LVE        (1<<4)
#define CC1_PU_MASK    (3<<2)
#define CC1_PU_DISABLE (0<<2)
#define CC1_PU_330uA   (1<<2)
#define CC1_PU_180uA   (2<<2)
#define CC1_PU_80uA    (3<<2)
#define PA_CC1_AI      (1<<0)

#define CC2_CE_MASK    (7<<5)
#define CC2_CE_0       (1<<5)
#define CC2_CE_1       (2<<5)
#define CC2_CE_2       (4<<5)

#define CC2_LVE        (1<<4)
#define CC2_PU_MASK    (3<<2)
#define CC2_PU_DISABLE (0<<2)
#define CC2_PU_330uA   (1<<2)
#define CC2_PU_180uA   (2<<2)
#define CC2_PU_80uA    (3<<2)
#define PA_CC2_AI      (1<<0)



/* Peripheral declaration */
#define TIM2                                    ((TIM_TypeDef *)TIM2_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define TIM3                                    ((TIM_TypeDef *)TIM3_BASE)
#define TIM4                                    ((TIM_TypeDef *)TIM4_BASE)
#define TIM5                                    ((TIM_TypeDef *)TIM5_BASE)
#if defined(CH32V30x)
#define TIM6                					((TIM_TypeDef *) TIM6_BASE)
#define TIM7                					((TIM_TypeDef *) TIM7_BASE)
#define UART6               					((USART_TypeDef *) UART6_BASE)
#define UART7               					((USART_TypeDef *) UART7_BASE)
#define UART8               					((USART_TypeDef *) UART8_BASE)
#endif // defined(CH32V30x)
#if defined(CH32V10x)
#define TIM6                                    ((TIM_TypeDef *)TIM6_BASE)
#define TIM7                                    ((TIM_TypeDef *)TIM7_BASE)
#define TIM12                                   ((TIM_TypeDef *)TIM12_BASE)
#define TIM13                                   ((TIM_TypeDef *)TIM13_BASE)
#define TIM14                                   ((TIM_TypeDef *)TIM14_BASE)
#endif // defined(CH32V10x)
#define RTC                                     ((RTC_TypeDef *)RTC_BASE)
#endif // defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define WWDG                                    ((WWDG_TypeDef *)WWDG_BASE)
#define IWDG                                    ((IWDG_TypeDef *)IWDG_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define SPI2                                    ((SPI_TypeDef *)SPI2_BASE)
#if defined(CH32V10x) || defined(CH32V30x)
#define SPI3                					((SPI_TypeDef *) SPI3_BASE)
#endif
#define USART2                                  ((USART_TypeDef *)USART2_BASE)
#define USART3                                  ((USART_TypeDef *)USART3_BASE)
#define UART4                                   ((USART_TypeDef *)UART4_BASE)
#if defined(CH32V10x) || defined(CH32V30x)
#define UART5               					((USART_TypeDef *) UART5_BASE)
#endif
#endif // defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C1                                    ((I2C_TypeDef *)I2C1_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C2                                    ((I2C_TypeDef *)I2C2_BASE)
#endif
#if defined(CH32V20x) || defined(CH32V30x)
#define CAN1                                    ((CAN_TypeDef *)CAN1_BASE)
#endif
#if defined(CH32V30x)
#define CAN2                					((CAN_TypeDef *) CAN2_BASE)
#endif
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define BKP                                     ((BKP_TypeDef *)BKP_BASE)
#endif
#define PWR                                     ((PWR_TypeDef *)PWR_BASE)
#if defined(CH32V10x) || defined(CH32V30x)
#define DAC                 					((DAC_TypeDef *) DAC_BASE)
#endif

#define AFIO                                    ((AFIO_TypeDef *)AFIO_BASE)
#define EXTI                                    ((EXTI_TypeDef *)EXTI_BASE)
#define GPIOA                                   ((GPIO_TypeDef *)GPIOA_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
#define GPIOB                                   ((GPIO_TypeDef *)GPIOB_BASE)
#endif
#define GPIOC                                   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD                                   ((GPIO_TypeDef *)GPIOD_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define GPIOE                                   ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF                                   ((GPIO_TypeDef *)GPIOF_BASE)
#define GPIOG                                   ((GPIO_TypeDef *)GPIOG_BASE)
#endif
#define ADC1                                    ((ADC_TypeDef *)ADC1_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC2                                    ((ADC_TypeDef *)ADC2_BASE)
#endif
#ifdef CH32X03x
#define TKey                                    ((TKEY_TypeDef *)ADC1_BASE)
#define OPA										((OPACMP_TypeDef *)OPA_BASE)
#define USBFS									((USBFS_TypeDef *)USBFS_BASE)
#define USBPDWORD								((USBPD_TypeDef *)USBPD_BASE)
#define USBPD									((USBPD_DETAILED_TypeDef *)USBPD_BASE)
#define USBDH									((USBDH_TypeDef *)USBFS_BASE)

#endif
#if defined(CH32V20x) || defined(CH32V30x)
#define TKey1                                   ((ADC_TypeDef *)ADC1_BASE)
#define TKey2                                   ((ADC_TypeDef *)ADC2_BASE)
#endif
#define TIM1                                    ((TIM_TypeDef *)TIM1_BASE)
#define SPI1                                    ((SPI_TypeDef *)SPI1_BASE)
#if defined(CH32V10x) || defined(CH32V30x)
#define TIM8                					((TIM_TypeDef *) TIM8_BASE)
#endif
#define USART1                                  ((USART_TypeDef *)USART1_BASE)
#if defined(CH32V10x) || defined(CH32V30x)
#define ADC3                					((ADC_TypeDef *) ADC3_BASE)
#define TIM15               					((TIM_TypeDef *) TIM15_BASE)
#define TIM16               					((TIM_TypeDef *) TIM16_BASE)
#define TIM17               					((TIM_TypeDef *) TIM17_BASE)
#define TIM9                					((TIM_TypeDef *) TIM9_BASE)
#define TIM10               					((TIM_TypeDef *) TIM10_BASE)
#define TIM11               					((TIM_TypeDef *) TIM11_BASE)
#endif // defined(CH32V10x) || defined(CH32V30x)
#if defined(CH32V30x)
#define SDIO                					((SDIO_TypeDef *) SDIO_BASE)
#endif

#define DMA1                                    ((DMA_TypeDef *)DMA1_BASE)
#define DMA1_Channel1                           ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)
#define DMA1_Channel2                           ((DMA_Channel_TypeDef *)DMA1_Channel2_BASE)
#define DMA1_Channel3                           ((DMA_Channel_TypeDef *)DMA1_Channel3_BASE)
#define DMA1_Channel4                           ((DMA_Channel_TypeDef *)DMA1_Channel4_BASE)
#define DMA1_Channel5                           ((DMA_Channel_TypeDef *)DMA1_Channel5_BASE)
#define DMA1_Channel6                           ((DMA_Channel_TypeDef *)DMA1_Channel6_BASE)
#define DMA1_Channel7                           ((DMA_Channel_TypeDef *)DMA1_Channel7_BASE)
#if defined(CH32V20x) || defined(CH32X03x)
#define DMA1_Channel8                           ((DMA_Channel_TypeDef *)DMA1_Channel8_BASE)
#endif
#if defined(CH32V10x) || defined(CH32V30x)
#define DMA2                					((DMA_TypeDef *) DMA2_BASE)
#define DMA2_EXTEN          					((DMA_TypeDef *) DMA2_EXTEN_BASE)
#define DMA2_Channel1       					((DMA_Channel_TypeDef *) DMA2_Channel1_BASE)
#define DMA2_Channel2       					((DMA_Channel_TypeDef *) DMA2_Channel2_BASE)
#define DMA2_Channel3       					((DMA_Channel_TypeDef *) DMA2_Channel3_BASE)
#define DMA2_Channel4       					((DMA_Channel_TypeDef *) DMA2_Channel4_BASE)
#define DMA2_Channel5       					((DMA_Channel_TypeDef *) DMA2_Channel5_BASE)
#if defined(CH32V30x)
#define DMA2_Channel6       					((DMA_Channel_TypeDef *) DMA2_Channel6_BASE)
#define DMA2_Channel7       					((DMA_Channel_TypeDef *) DMA2_Channel7_BASE)
#define DMA2_Channel8       					((DMA_Channel_TypeDef *) DMA2_Channel8_BASE)
#define DMA2_Channel9       					((DMA_Channel_TypeDef *) DMA2_Channel9_BASE)
#define DMA2_Channel10      					((DMA_Channel_TypeDef *) DMA2_Channel10_BASE)
#define DMA2_Channel11      					((DMA_Channel_TypeDef *) DMA2_Channel11_BASE)
#endif // defined(CH32V30x)
#endif // defined(CH32V10x) || defined(CH32V30x)
#define RCC                                     ((RCC_TypeDef *)RCC_BASE)
#define FLASH                                   ((FLASH_TypeDef *)FLASH_R_BASE)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define CRC                                     ((CRC_TypeDef *)CRC_BASE)
#endif
#if defined(CH32V20x) || defined(CH32V30x)
#if defined(CH32V30x)
#define USBHSD              					((USBHSD_TypeDef *) USBHS_BASE)
#define USBHSH              					((USBHSH_TypeDef *) USBHS_BASE)
#endif // defined(CH32V30x)
#define USBOTG_FS                               ((USBOTG_FS_TypeDef *)USBFS_BASE)
#define USBOTG_H_FS                             ((USBOTG_FS_HOST_TypeDef *)USBFS_BASE)
#define OPA                                     ((OPA_TypeDef *)OPA_BASE)
#if defined(CH32V20x)
#define ETH10M                                  ((ETH10M_TypeDef *)ETH10M_BASE)
#elif defined(CH32V30x)
#define RNG                 					((RNG_TypeDef *) RNG_BASE)
#define ETH                 					((ETH_TypeDef *) ETH_BASE)
#endif
#endif // defined(CH32V20x) || defined(CH32V30x)
#define OB                                      ((OB_TypeDef *)OB_BASE)
#define ESIG                                    ((ESG_TypeDef *)ESIG_BASE)
#define EXTEN                                   ((EXTEN_TypeDef *)EXTEN_BASE)

#if defined(CH32V20x)
#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
#define OSC                                     ((OSC_TypeDef *)OSC_BASE)
#endif
#endif

#if defined(CH32V30x)
#define DVP                 					((DVP_TypeDef *) DVP_BASE)

#define FSMC_Bank1          					((FSMC_Bank1_TypeDef *) FSMC_Bank1_R_BASE)
#define FSMC_Bank1E         					((FSMC_Bank1E_TypeDef *) FSMC_Bank1E_R_BASE)
#define FSMC_Bank2          					((FSMC_Bank2_TypeDef *) FSMC_Bank2_R_BASE)
#endif

#if defined(CH32V10x)
#define DBGMCU                                  ((DBGMCU_TypeDef *)DBGMCU_BASE)
#endif

/******************************************************************************/
/*                         Peripheral Registers Bits Definition               */
/******************************************************************************/

/******************************************************************************/
/*                        Analog to Digital Converter                         */
/******************************************************************************/

/********************  Bit definition for ADC_STATR register  ********************/
#define ADC_AWD                                 ((uint8_t)0x01) /* Analog watchdog flag */
#define ADC_EOC                                 ((uint8_t)0x02) /* End of conversion */
#define ADC_JEOC                                ((uint8_t)0x04) /* Injected channel end of conversion */
#define ADC_JSTRT                               ((uint8_t)0x08) /* Injected channel Start flag */
#define ADC_STRT                                ((uint8_t)0x10) /* Regular channel Start flag */

/*******************  Bit definition for ADC_CTLR1 register  ********************/
#define ADC_AWDCH                               ((uint32_t)0x0000001F) /* AWDCH[4:0] bits (Analog watchdog channel select bits) */
#define ADC_AWDCH_0                             ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_AWDCH_1                             ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_AWDCH_2                             ((uint32_t)0x00000004) /* Bit 2 */
#define ADC_AWDCH_3                             ((uint32_t)0x00000008) /* Bit 3 */
#define ADC_AWDCH_4                             ((uint32_t)0x00000010) /* Bit 4 */

#define ADC_EOCIE                               ((uint32_t)0x00000020) /* Interrupt enable for EOC */
#define ADC_AWDIE                               ((uint32_t)0x00000040) /* Analog Watchdog interrupt enable */
#define ADC_JEOCIE                              ((uint32_t)0x00000080) /* Interrupt enable for injected channels */
#define ADC_SCAN                                ((uint32_t)0x00000100) /* Scan mode */
#define ADC_AWDSGL                              ((uint32_t)0x00000200) /* Enable the watchdog on a single channel in scan mode */
#define ADC_JAUTO                               ((uint32_t)0x00000400) /* Automatic injected group conversion */
#define ADC_DISCEN                              ((uint32_t)0x00000800) /* Discontinuous mode on regular channels */
#define ADC_JDISCEN                             ((uint32_t)0x00001000) /* Discontinuous mode on injected channels */

#define ADC_DISCNUM                             ((uint32_t)0x0000E000) /* DISCNUM[2:0] bits (Discontinuous mode channel count) */
#define ADC_DISCNUM_0                           ((uint32_t)0x00002000) /* Bit 0 */
#define ADC_DISCNUM_1                           ((uint32_t)0x00004000) /* Bit 1 */
#define ADC_DISCNUM_2                           ((uint32_t)0x00008000) /* Bit 2 */

#define ADC_DUALMOD                             ((uint32_t)0x000F0000) /* DUALMOD[3:0] bits (Dual mode selection) */
#define ADC_DUALMOD_0                           ((uint32_t)0x00010000) /* Bit 0 */
#define ADC_DUALMOD_1                           ((uint32_t)0x00020000) /* Bit 1 */
#define ADC_DUALMOD_2                           ((uint32_t)0x00040000) /* Bit 2 */
#define ADC_DUALMOD_3                           ((uint32_t)0x00080000) /* Bit 3 */

#define ADC_JAWDEN                              ((uint32_t)0x00400000) /* Analog watchdog enable on injected channels */
#define ADC_AWDEN                               ((uint32_t)0x00800000) /* Analog watchdog enable on regular channels */

/*******************  Bit definition for ADC_CTLR2 register  ********************/
#define ADC_ADON                                ((uint32_t)0x00000001) /* A/D Converter ON / OFF */
#define ADC_CONT                                ((uint32_t)0x00000002) /* Continuous Conversion */
#define ADC_CAL                                 ((uint32_t)0x00000004) /* A/D Calibration */
#define ADC_RSTCAL                              ((uint32_t)0x00000008) /* Reset Calibration */
#define ADC_DMA                                 ((uint32_t)0x00000100) /* Direct Memory access mode */
#define ADC_ALIGN                               ((uint32_t)0x00000800) /* Data Alignment */

#define ADC_JEXTSEL                             ((uint32_t)0x00007000) /* JEXTSEL[2:0] bits (External event select for injected group) */
#define ADC_JEXTSEL_0                           ((uint32_t)0x00001000) /* Bit 0 */
#define ADC_JEXTSEL_1                           ((uint32_t)0x00002000) /* Bit 1 */
#define ADC_JEXTSEL_2                           ((uint32_t)0x00004000) /* Bit 2 */

#define ADC_JEXTTRIG                            ((uint32_t)0x00008000) /* External Trigger Conversion mode for injected channels */

#define ADC_EXTSEL                              ((uint32_t)0x000E0000) /* EXTSEL[2:0] bits (External Event Select for regular group) */
#define ADC_EXTSEL_0                            ((uint32_t)0x00020000) /* Bit 0 */
#define ADC_EXTSEL_1                            ((uint32_t)0x00040000) /* Bit 1 */
#define ADC_EXTSEL_2                            ((uint32_t)0x00080000) /* Bit 2 */

#define ADC_EXTTRIG                             ((uint32_t)0x00100000) /* External Trigger Conversion mode for regular channels */
#define ADC_JSWSTART                            ((uint32_t)0x00200000) /* Start Conversion of injected channels */
#define ADC_SWSTART                             ((uint32_t)0x00400000) /* Start Conversion of regular channels */
#define ADC_TSVREFE                             ((uint32_t)0x00800000) /* Temperature Sensor and VREFINT Enable */

/******************  Bit definition for ADC_SAMPTR1 register  *******************/
#define ADC_SMP10                               ((uint32_t)0x00000007) /* SMP10[2:0] bits (Channel 10 Sample time selection) */
#define ADC_SMP10_0                             ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_SMP10_1                             ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_SMP10_2                             ((uint32_t)0x00000004) /* Bit 2 */

#define ADC_SMP11                               ((uint32_t)0x00000038) /* SMP11[2:0] bits (Channel 11 Sample time selection) */
#define ADC_SMP11_0                             ((uint32_t)0x00000008) /* Bit 0 */
#define ADC_SMP11_1                             ((uint32_t)0x00000010) /* Bit 1 */
#define ADC_SMP11_2                             ((uint32_t)0x00000020) /* Bit 2 */

#define ADC_SMP12                               ((uint32_t)0x000001C0) /* SMP12[2:0] bits (Channel 12 Sample time selection) */
#define ADC_SMP12_0                             ((uint32_t)0x00000040) /* Bit 0 */
#define ADC_SMP12_1                             ((uint32_t)0x00000080) /* Bit 1 */
#define ADC_SMP12_2                             ((uint32_t)0x00000100) /* Bit 2 */

#define ADC_SMP13                               ((uint32_t)0x00000E00) /* SMP13[2:0] bits (Channel 13 Sample time selection) */
#define ADC_SMP13_0                             ((uint32_t)0x00000200) /* Bit 0 */
#define ADC_SMP13_1                             ((uint32_t)0x00000400) /* Bit 1 */
#define ADC_SMP13_2                             ((uint32_t)0x00000800) /* Bit 2 */

#define ADC_SMP14                               ((uint32_t)0x00007000) /* SMP14[2:0] bits (Channel 14 Sample time selection) */
#define ADC_SMP14_0                             ((uint32_t)0x00001000) /* Bit 0 */
#define ADC_SMP14_1                             ((uint32_t)0x00002000) /* Bit 1 */
#define ADC_SMP14_2                             ((uint32_t)0x00004000) /* Bit 2 */

#define ADC_SMP15                               ((uint32_t)0x00038000) /* SMP15[2:0] bits (Channel 15 Sample time selection) */
#define ADC_SMP15_0                             ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_SMP15_1                             ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_SMP15_2                             ((uint32_t)0x00020000) /* Bit 2 */

#define ADC_SMP16                               ((uint32_t)0x001C0000) /* SMP16[2:0] bits (Channel 16 Sample time selection) */
#define ADC_SMP16_0                             ((uint32_t)0x00040000) /* Bit 0 */
#define ADC_SMP16_1                             ((uint32_t)0x00080000) /* Bit 1 */
#define ADC_SMP16_2                             ((uint32_t)0x00100000) /* Bit 2 */

#define ADC_SMP17                               ((uint32_t)0x00E00000) /* SMP17[2:0] bits (Channel 17 Sample time selection) */
#define ADC_SMP17_0                             ((uint32_t)0x00200000) /* Bit 0 */
#define ADC_SMP17_1                             ((uint32_t)0x00400000) /* Bit 1 */
#define ADC_SMP17_2                             ((uint32_t)0x00800000) /* Bit 2 */

/******************  Bit definition for ADC_SAMPTR2 register  *******************/
#define ADC_SMP0                                ((uint32_t)0x00000007) /* SMP0[2:0] bits (Channel 0 Sample time selection) */
#define ADC_SMP0_0                              ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_SMP0_1                              ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_SMP0_2                              ((uint32_t)0x00000004) /* Bit 2 */

#define ADC_SMP1                                ((uint32_t)0x00000038) /* SMP1[2:0] bits (Channel 1 Sample time selection) */
#define ADC_SMP1_0                              ((uint32_t)0x00000008) /* Bit 0 */
#define ADC_SMP1_1                              ((uint32_t)0x00000010) /* Bit 1 */
#define ADC_SMP1_2                              ((uint32_t)0x00000020) /* Bit 2 */

#define ADC_SMP2                                ((uint32_t)0x000001C0) /* SMP2[2:0] bits (Channel 2 Sample time selection) */
#define ADC_SMP2_0                              ((uint32_t)0x00000040) /* Bit 0 */
#define ADC_SMP2_1                              ((uint32_t)0x00000080) /* Bit 1 */
#define ADC_SMP2_2                              ((uint32_t)0x00000100) /* Bit 2 */

#define ADC_SMP3                                ((uint32_t)0x00000E00) /* SMP3[2:0] bits (Channel 3 Sample time selection) */
#define ADC_SMP3_0                              ((uint32_t)0x00000200) /* Bit 0 */
#define ADC_SMP3_1                              ((uint32_t)0x00000400) /* Bit 1 */
#define ADC_SMP3_2                              ((uint32_t)0x00000800) /* Bit 2 */

#define ADC_SMP4                                ((uint32_t)0x00007000) /* SMP4[2:0] bits (Channel 4 Sample time selection) */
#define ADC_SMP4_0                              ((uint32_t)0x00001000) /* Bit 0 */
#define ADC_SMP4_1                              ((uint32_t)0x00002000) /* Bit 1 */
#define ADC_SMP4_2                              ((uint32_t)0x00004000) /* Bit 2 */

#define ADC_SMP5                                ((uint32_t)0x00038000) /* SMP5[2:0] bits (Channel 5 Sample time selection) */
#define ADC_SMP5_0                              ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_SMP5_1                              ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_SMP5_2                              ((uint32_t)0x00020000) /* Bit 2 */

#define ADC_SMP6                                ((uint32_t)0x001C0000) /* SMP6[2:0] bits (Channel 6 Sample time selection) */
#define ADC_SMP6_0                              ((uint32_t)0x00040000) /* Bit 0 */
#define ADC_SMP6_1                              ((uint32_t)0x00080000) /* Bit 1 */
#define ADC_SMP6_2                              ((uint32_t)0x00100000) /* Bit 2 */

#define ADC_SMP7                                ((uint32_t)0x00E00000) /* SMP7[2:0] bits (Channel 7 Sample time selection) */
#define ADC_SMP7_0                              ((uint32_t)0x00200000) /* Bit 0 */
#define ADC_SMP7_1                              ((uint32_t)0x00400000) /* Bit 1 */
#define ADC_SMP7_2                              ((uint32_t)0x00800000) /* Bit 2 */

#define ADC_SMP8                                ((uint32_t)0x07000000) /* SMP8[2:0] bits (Channel 8 Sample time selection) */
#define ADC_SMP8_0                              ((uint32_t)0x01000000) /* Bit 0 */
#define ADC_SMP8_1                              ((uint32_t)0x02000000) /* Bit 1 */
#define ADC_SMP8_2                              ((uint32_t)0x04000000) /* Bit 2 */

#define ADC_SMP9                                ((uint32_t)0x38000000) /* SMP9[2:0] bits (Channel 9 Sample time selection) */
#define ADC_SMP9_0                              ((uint32_t)0x08000000) /* Bit 0 */
#define ADC_SMP9_1                              ((uint32_t)0x10000000) /* Bit 1 */
#define ADC_SMP9_2                              ((uint32_t)0x20000000) /* Bit 2 */

/******************  Bit definition for ADC_IOFR1 register  *******************/
#define ADC_JOFFSET1                            ((uint16_t)0x0FFF) /* Data offset for injected channel 1 */

/******************  Bit definition for ADC_IOFR2 register  *******************/
#define ADC_JOFFSET2                            ((uint16_t)0x0FFF) /* Data offset for injected channel 2 */

/******************  Bit definition for ADC_IOFR3 register  *******************/
#define ADC_JOFFSET3                            ((uint16_t)0x0FFF) /* Data offset for injected channel 3 */

/******************  Bit definition for ADC_IOFR4 register  *******************/
#define ADC_JOFFSET4                            ((uint16_t)0x0FFF) /* Data offset for injected channel 4 */

/*******************  Bit definition for ADC_WDHTR register  ********************/
#define ADC_HT                                  ((uint16_t)0x0FFF) /* Analog watchdog high threshold */

/*******************  Bit definition for ADC_WDLTR register  ********************/
#define ADC_LT                                  ((uint16_t)0x0FFF) /* Analog watchdog low threshold */

/*******************  Bit definition for ADC_RSQR1 register  *******************/
#define ADC_SQ13                                ((uint32_t)0x0000001F) /* SQ13[4:0] bits (13th conversion in regular sequence) */
#define ADC_SQ13_0                              ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_SQ13_1                              ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_SQ13_2                              ((uint32_t)0x00000004) /* Bit 2 */
#define ADC_SQ13_3                              ((uint32_t)0x00000008) /* Bit 3 */
#define ADC_SQ13_4                              ((uint32_t)0x00000010) /* Bit 4 */

#define ADC_SQ14                                ((uint32_t)0x000003E0) /* SQ14[4:0] bits (14th conversion in regular sequence) */
#define ADC_SQ14_0                              ((uint32_t)0x00000020) /* Bit 0 */
#define ADC_SQ14_1                              ((uint32_t)0x00000040) /* Bit 1 */
#define ADC_SQ14_2                              ((uint32_t)0x00000080) /* Bit 2 */
#define ADC_SQ14_3                              ((uint32_t)0x00000100) /* Bit 3 */
#define ADC_SQ14_4                              ((uint32_t)0x00000200) /* Bit 4 */

#define ADC_SQ15                                ((uint32_t)0x00007C00) /* SQ15[4:0] bits (15th conversion in regular sequence) */
#define ADC_SQ15_0                              ((uint32_t)0x00000400) /* Bit 0 */
#define ADC_SQ15_1                              ((uint32_t)0x00000800) /* Bit 1 */
#define ADC_SQ15_2                              ((uint32_t)0x00001000) /* Bit 2 */
#define ADC_SQ15_3                              ((uint32_t)0x00002000) /* Bit 3 */
#define ADC_SQ15_4                              ((uint32_t)0x00004000) /* Bit 4 */

#define ADC_SQ16                                ((uint32_t)0x000F8000) /* SQ16[4:0] bits (16th conversion in regular sequence) */
#define ADC_SQ16_0                              ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_SQ16_1                              ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_SQ16_2                              ((uint32_t)0x00020000) /* Bit 2 */
#define ADC_SQ16_3                              ((uint32_t)0x00040000) /* Bit 3 */
#define ADC_SQ16_4                              ((uint32_t)0x00080000) /* Bit 4 */

#define ADC_L                                   ((uint32_t)0x00F00000) /* L[3:0] bits (Regular channel sequence length) */
#define ADC_L_0                                 ((uint32_t)0x00100000) /* Bit 0 */
#define ADC_L_1                                 ((uint32_t)0x00200000) /* Bit 1 */
#define ADC_L_2                                 ((uint32_t)0x00400000) /* Bit 2 */
#define ADC_L_3                                 ((uint32_t)0x00800000) /* Bit 3 */

/*******************  Bit definition for ADC_RSQR2 register  *******************/
#define ADC_SQ7                                 ((uint32_t)0x0000001F) /* SQ7[4:0] bits (7th conversion in regular sequence) */
#define ADC_SQ7_0                               ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_SQ7_1                               ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_SQ7_2                               ((uint32_t)0x00000004) /* Bit 2 */
#define ADC_SQ7_3                               ((uint32_t)0x00000008) /* Bit 3 */
#define ADC_SQ7_4                               ((uint32_t)0x00000010) /* Bit 4 */

#define ADC_SQ8                                 ((uint32_t)0x000003E0) /* SQ8[4:0] bits (8th conversion in regular sequence) */
#define ADC_SQ8_0                               ((uint32_t)0x00000020) /* Bit 0 */
#define ADC_SQ8_1                               ((uint32_t)0x00000040) /* Bit 1 */
#define ADC_SQ8_2                               ((uint32_t)0x00000080) /* Bit 2 */
#define ADC_SQ8_3                               ((uint32_t)0x00000100) /* Bit 3 */
#define ADC_SQ8_4                               ((uint32_t)0x00000200) /* Bit 4 */

#define ADC_SQ9                                 ((uint32_t)0x00007C00) /* SQ9[4:0] bits (9th conversion in regular sequence) */
#define ADC_SQ9_0                               ((uint32_t)0x00000400) /* Bit 0 */
#define ADC_SQ9_1                               ((uint32_t)0x00000800) /* Bit 1 */
#define ADC_SQ9_2                               ((uint32_t)0x00001000) /* Bit 2 */
#define ADC_SQ9_3                               ((uint32_t)0x00002000) /* Bit 3 */
#define ADC_SQ9_4                               ((uint32_t)0x00004000) /* Bit 4 */

#define ADC_SQ10                                ((uint32_t)0x000F8000) /* SQ10[4:0] bits (10th conversion in regular sequence) */
#define ADC_SQ10_0                              ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_SQ10_1                              ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_SQ10_2                              ((uint32_t)0x00020000) /* Bit 2 */
#define ADC_SQ10_3                              ((uint32_t)0x00040000) /* Bit 3 */
#define ADC_SQ10_4                              ((uint32_t)0x00080000) /* Bit 4 */

#define ADC_SQ11                                ((uint32_t)0x01F00000) /* SQ11[4:0] bits (11th conversion in regular sequence) */
#define ADC_SQ11_0                              ((uint32_t)0x00100000) /* Bit 0 */
#define ADC_SQ11_1                              ((uint32_t)0x00200000) /* Bit 1 */
#define ADC_SQ11_2                              ((uint32_t)0x00400000) /* Bit 2 */
#define ADC_SQ11_3                              ((uint32_t)0x00800000) /* Bit 3 */
#define ADC_SQ11_4                              ((uint32_t)0x01000000) /* Bit 4 */

#define ADC_SQ12                                ((uint32_t)0x3E000000) /* SQ12[4:0] bits (12th conversion in regular sequence) */
#define ADC_SQ12_0                              ((uint32_t)0x02000000) /* Bit 0 */
#define ADC_SQ12_1                              ((uint32_t)0x04000000) /* Bit 1 */
#define ADC_SQ12_2                              ((uint32_t)0x08000000) /* Bit 2 */
#define ADC_SQ12_3                              ((uint32_t)0x10000000) /* Bit 3 */
#define ADC_SQ12_4                              ((uint32_t)0x20000000) /* Bit 4 */

/*******************  Bit definition for ADC_RSQR3 register  *******************/
#define ADC_SQ1                                 ((uint32_t)0x0000001F) /* SQ1[4:0] bits (1st conversion in regular sequence) */
#define ADC_SQ1_0                               ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_SQ1_1                               ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_SQ1_2                               ((uint32_t)0x00000004) /* Bit 2 */
#define ADC_SQ1_3                               ((uint32_t)0x00000008) /* Bit 3 */
#define ADC_SQ1_4                               ((uint32_t)0x00000010) /* Bit 4 */

#define ADC_SQ2                                 ((uint32_t)0x000003E0) /* SQ2[4:0] bits (2nd conversion in regular sequence) */
#define ADC_SQ2_0                               ((uint32_t)0x00000020) /* Bit 0 */
#define ADC_SQ2_1                               ((uint32_t)0x00000040) /* Bit 1 */
#define ADC_SQ2_2                               ((uint32_t)0x00000080) /* Bit 2 */
#define ADC_SQ2_3                               ((uint32_t)0x00000100) /* Bit 3 */
#define ADC_SQ2_4                               ((uint32_t)0x00000200) /* Bit 4 */

#define ADC_SQ3                                 ((uint32_t)0x00007C00) /* SQ3[4:0] bits (3rd conversion in regular sequence) */
#define ADC_SQ3_0                               ((uint32_t)0x00000400) /* Bit 0 */
#define ADC_SQ3_1                               ((uint32_t)0x00000800) /* Bit 1 */
#define ADC_SQ3_2                               ((uint32_t)0x00001000) /* Bit 2 */
#define ADC_SQ3_3                               ((uint32_t)0x00002000) /* Bit 3 */
#define ADC_SQ3_4                               ((uint32_t)0x00004000) /* Bit 4 */

#define ADC_SQ4                                 ((uint32_t)0x000F8000) /* SQ4[4:0] bits (4th conversion in regular sequence) */
#define ADC_SQ4_0                               ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_SQ4_1                               ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_SQ4_2                               ((uint32_t)0x00020000) /* Bit 2 */
#define ADC_SQ4_3                               ((uint32_t)0x00040000) /* Bit 3 */
#define ADC_SQ4_4                               ((uint32_t)0x00080000) /* Bit 4 */

#define ADC_SQ5                                 ((uint32_t)0x01F00000) /* SQ5[4:0] bits (5th conversion in regular sequence) */
#define ADC_SQ5_0                               ((uint32_t)0x00100000) /* Bit 0 */
#define ADC_SQ5_1                               ((uint32_t)0x00200000) /* Bit 1 */
#define ADC_SQ5_2                               ((uint32_t)0x00400000) /* Bit 2 */
#define ADC_SQ5_3                               ((uint32_t)0x00800000) /* Bit 3 */
#define ADC_SQ5_4                               ((uint32_t)0x01000000) /* Bit 4 */

#define ADC_SQ6                                 ((uint32_t)0x3E000000) /* SQ6[4:0] bits (6th conversion in regular sequence) */
#define ADC_SQ6_0                               ((uint32_t)0x02000000) /* Bit 0 */
#define ADC_SQ6_1                               ((uint32_t)0x04000000) /* Bit 1 */
#define ADC_SQ6_2                               ((uint32_t)0x08000000) /* Bit 2 */
#define ADC_SQ6_3                               ((uint32_t)0x10000000) /* Bit 3 */
#define ADC_SQ6_4                               ((uint32_t)0x20000000) /* Bit 4 */

/*******************  Bit definition for ADC_ISQR register  *******************/
#define ADC_JSQ1                                ((uint32_t)0x0000001F) /* JSQ1[4:0] bits (1st conversion in injected sequence) */
#define ADC_JSQ1_0                              ((uint32_t)0x00000001) /* Bit 0 */
#define ADC_JSQ1_1                              ((uint32_t)0x00000002) /* Bit 1 */
#define ADC_JSQ1_2                              ((uint32_t)0x00000004) /* Bit 2 */
#define ADC_JSQ1_3                              ((uint32_t)0x00000008) /* Bit 3 */
#define ADC_JSQ1_4                              ((uint32_t)0x00000010) /* Bit 4 */

#define ADC_JSQ2                                ((uint32_t)0x000003E0) /* JSQ2[4:0] bits (2nd conversion in injected sequence) */
#define ADC_JSQ2_0                              ((uint32_t)0x00000020) /* Bit 0 */
#define ADC_JSQ2_1                              ((uint32_t)0x00000040) /* Bit 1 */
#define ADC_JSQ2_2                              ((uint32_t)0x00000080) /* Bit 2 */
#define ADC_JSQ2_3                              ((uint32_t)0x00000100) /* Bit 3 */
#define ADC_JSQ2_4                              ((uint32_t)0x00000200) /* Bit 4 */

#define ADC_JSQ3                                ((uint32_t)0x00007C00) /* JSQ3[4:0] bits (3rd conversion in injected sequence) */
#define ADC_JSQ3_0                              ((uint32_t)0x00000400) /* Bit 0 */
#define ADC_JSQ3_1                              ((uint32_t)0x00000800) /* Bit 1 */
#define ADC_JSQ3_2                              ((uint32_t)0x00001000) /* Bit 2 */
#define ADC_JSQ3_3                              ((uint32_t)0x00002000) /* Bit 3 */
#define ADC_JSQ3_4                              ((uint32_t)0x00004000) /* Bit 4 */

#define ADC_JSQ4                                ((uint32_t)0x000F8000) /* JSQ4[4:0] bits (4th conversion in injected sequence) */
#define ADC_JSQ4_0                              ((uint32_t)0x00008000) /* Bit 0 */
#define ADC_JSQ4_1                              ((uint32_t)0x00010000) /* Bit 1 */
#define ADC_JSQ4_2                              ((uint32_t)0x00020000) /* Bit 2 */
#define ADC_JSQ4_3                              ((uint32_t)0x00040000) /* Bit 3 */
#define ADC_JSQ4_4                              ((uint32_t)0x00080000) /* Bit 4 */

#define ADC_JL                                  ((uint32_t)0x00300000) /* JL[1:0] bits (Injected Sequence length) */
#define ADC_JL_0                                ((uint32_t)0x00100000) /* Bit 0 */
#define ADC_JL_1                                ((uint32_t)0x00200000) /* Bit 1 */

/*******************  Bit definition for ADC_IDATAR1 register  *******************/
#define ADC_IDATAR1_JDATA                       ((uint16_t)0xFFFF) /* Injected data */

/*******************  Bit definition for ADC_IDATAR2 register  *******************/
#define ADC_IDATAR2_JDATA                       ((uint16_t)0xFFFF) /* Injected data */

/*******************  Bit definition for ADC_IDATAR3 register  *******************/
#define ADC_IDATAR3_JDATA                       ((uint16_t)0xFFFF) /* Injected data */

/*******************  Bit definition for ADC_IDATAR4 register  *******************/
#define ADC_IDATAR4_JDATA                       ((uint16_t)0xFFFF) /* Injected data */

/********************  Bit definition for ADC_RDATAR register  ********************/
#define ADC_RDATAR_DATA                         ((uint32_t)0x0000FFFF) /* Regular data */
#define ADC_RDATAR_ADC2DATA                     ((uint32_t)0xFFFF0000) /* ADC2 data */

#if defined(CH32V20x) || defined(CH32V30x)
/******************************************************************************/
/*                            Backup registers                                */
/******************************************************************************/

/*******************  Bit definition for BKP_DATAR1 register  ********************/
#define BKP_DATAR1_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR2 register  ********************/
#define BKP_DATAR2_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR3 register  ********************/
#define BKP_DATAR3_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR4 register  ********************/
#define BKP_DATAR4_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR5 register  ********************/
#define BKP_DATAR5_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR6 register  ********************/
#define BKP_DATAR6_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR7 register  ********************/
#define BKP_DATAR7_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR8 register  ********************/
#define BKP_DATAR8_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR9 register  ********************/
#define BKP_DATAR9_D                            ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR10 register  *******************/
#define BKP_DATAR10_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR11 register  *******************/
#define BKP_DATAR11_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR12 register  *******************/
#define BKP_DATAR12_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR13 register  *******************/
#define BKP_DATAR13_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR14 register  *******************/
#define BKP_DATAR14_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR15 register  *******************/
#define BKP_DATAR15_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR16 register  *******************/
#define BKP_DATAR16_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR17 register  *******************/
#define BKP_DATAR17_D                           ((uint16_t)0xFFFF) /* Backup data */

/******************  Bit definition for BKP_DATAR18 register  ********************/
#define BKP_DATAR18_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR19 register  *******************/
#define BKP_DATAR19_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR20 register  *******************/
#define BKP_DATAR20_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR21 register  *******************/
#define BKP_DATAR21_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR22 register  *******************/
#define BKP_DATAR22_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR23 register  *******************/
#define BKP_DATAR23_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR24 register  *******************/
#define BKP_DATAR24_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR25 register  *******************/
#define BKP_DATAR25_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR26 register  *******************/
#define BKP_DATAR26_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR27 register  *******************/
#define BKP_DATAR27_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR28 register  *******************/
#define BKP_DATAR28_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR29 register  *******************/
#define BKP_DATAR29_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR30 register  *******************/
#define BKP_DATAR30_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR31 register  *******************/
#define BKP_DATAR31_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR32 register  *******************/
#define BKP_DATAR32_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR33 register  *******************/
#define BKP_DATAR33_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR34 register  *******************/
#define BKP_DATAR34_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR35 register  *******************/
#define BKP_DATAR35_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR36 register  *******************/
#define BKP_DATAR36_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR37 register  *******************/
#define BKP_DATAR37_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR38 register  *******************/
#define BKP_DATAR38_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR39 register  *******************/
#define BKP_DATAR39_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR40 register  *******************/
#define BKP_DATAR40_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR41 register  *******************/
#define BKP_DATAR41_D                           ((uint16_t)0xFFFF) /* Backup data */

/*******************  Bit definition for BKP_DATAR42 register  *******************/
#define BKP_DATAR42_D                           ((uint16_t)0xFFFF) /* Backup data */

/******************  Bit definition for BKP_OCTLR register  *******************/
#define BKP_CAL                                 ((uint16_t)0x007F) /* Calibration value */
#define BKP_CCO                                 ((uint16_t)0x0080) /* Calibration Clock Output */
#define BKP_ASOE                                ((uint16_t)0x0100) /* Alarm or Second Output Enable */
#define BKP_ASOS                                ((uint16_t)0x0200) /* Alarm or Second Output Selection */

/********************  Bit definition for BKP_TPCTLR register  ********************/
#define BKP_TPE                                 ((uint8_t)0x01) /* TAMPER pin enable */
#define BKP_TPAL                                ((uint8_t)0x02) /* TAMPER pin active level */

/*******************  Bit definition for BKP_TPCSR register  ********************/
#define BKP_CTE                                 ((uint16_t)0x0001) /* Clear Tamper event */
#define BKP_CTI                                 ((uint16_t)0x0002) /* Clear Tamper Interrupt */
#define BKP_TPIE                                ((uint16_t)0x0004) /* TAMPER Pin interrupt enable */
#define BKP_TEF                                 ((uint16_t)0x0100) /* Tamper Event Flag */
#define BKP_TIF                                 ((uint16_t)0x0200) /* Tamper Interrupt Flag */

/******************************************************************************/
/*                         Controller Area Network                            */
/******************************************************************************/

/*******************  Bit definition for CAN_CTLR register  ********************/
#define CAN_CTLR_INRQ                           ((uint16_t)0x0001) /* Initialization Request */
#define CAN_CTLR_SLEEP                          ((uint16_t)0x0002) /* Sleep Mode Request */
#define CAN_CTLR_TXFP                           ((uint16_t)0x0004) /* Transmit FIFO Priority */
#define CAN_CTLR_RFLM                           ((uint16_t)0x0008) /* Receive FIFO Locked Mode */
#define CAN_CTLR_NART                           ((uint16_t)0x0010) /* No Automatic Retransmission */
#define CAN_CTLR_AWUM                           ((uint16_t)0x0020) /* Automatic Wakeup Mode */
#define CAN_CTLR_ABOM                           ((uint16_t)0x0040) /* Automatic Bus-Off Management */
#define CAN_CTLR_TTCM                           ((uint16_t)0x0080) /* Time Triggered Communication Mode */
#define CAN_CTLR_RESET                          ((uint16_t)0x8000) /* CAN software master reset */

/*******************  Bit definition for CAN_STATR register  ********************/
#define CAN_STATR_INAK                          ((uint16_t)0x0001) /* Initialization Acknowledge */
#define CAN_STATR_SLAK                          ((uint16_t)0x0002) /* Sleep Acknowledge */
#define CAN_STATR_ERRI                          ((uint16_t)0x0004) /* Error Interrupt */
#define CAN_STATR_WKUI                          ((uint16_t)0x0008) /* Wakeup Interrupt */
#define CAN_STATR_SLAKI                         ((uint16_t)0x0010) /* Sleep Acknowledge Interrupt */
#define CAN_STATR_TXM                           ((uint16_t)0x0100) /* Transmit Mode */
#define CAN_STATR_RXM                           ((uint16_t)0x0200) /* Receive Mode */
#define CAN_STATR_SAMP                          ((uint16_t)0x0400) /* Last Sample Point */
#define CAN_STATR_RX                            ((uint16_t)0x0800) /* CAN Rx Signal */

/*******************  Bit definition for CAN_TSTATR register  ********************/
#define CAN_TSTATR_RQCP0                        ((uint32_t)0x00000001) /* Request Completed Mailbox0 */
#define CAN_TSTATR_TXOK0                        ((uint32_t)0x00000002) /* Transmission OK of Mailbox0 */
#define CAN_TSTATR_ALST0                        ((uint32_t)0x00000004) /* Arbitration Lost for Mailbox0 */
#define CAN_TSTATR_TERR0                        ((uint32_t)0x00000008) /* Transmission Error of Mailbox0 */
#define CAN_TSTATR_ABRQ0                        ((uint32_t)0x00000080) /* Abort Request for Mailbox0 */
#define CAN_TSTATR_RQCP1                        ((uint32_t)0x00000100) /* Request Completed Mailbox1 */
#define CAN_TSTATR_TXOK1                        ((uint32_t)0x00000200) /* Transmission OK of Mailbox1 */
#define CAN_TSTATR_ALST1                        ((uint32_t)0x00000400) /* Arbitration Lost for Mailbox1 */
#define CAN_TSTATR_TERR1                        ((uint32_t)0x00000800) /* Transmission Error of Mailbox1 */
#define CAN_TSTATR_ABRQ1                        ((uint32_t)0x00008000) /* Abort Request for Mailbox 1 */
#define CAN_TSTATR_RQCP2                        ((uint32_t)0x00010000) /* Request Completed Mailbox2 */
#define CAN_TSTATR_TXOK2                        ((uint32_t)0x00020000) /* Transmission OK of Mailbox 2 */
#define CAN_TSTATR_ALST2                        ((uint32_t)0x00040000) /* Arbitration Lost for mailbox 2 */
#define CAN_TSTATR_TERR2                        ((uint32_t)0x00080000) /* Transmission Error of Mailbox 2 */
#define CAN_TSTATR_ABRQ2                        ((uint32_t)0x00800000) /* Abort Request for Mailbox 2 */
#define CAN_TSTATR_CODE                         ((uint32_t)0x03000000) /* Mailbox Code */

#define CAN_TSTATR_TME                          ((uint32_t)0x1C000000) /* TME[2:0] bits */
#define CAN_TSTATR_TME0                         ((uint32_t)0x04000000) /* Transmit Mailbox 0 Empty */
#define CAN_TSTATR_TME1                         ((uint32_t)0x08000000) /* Transmit Mailbox 1 Empty */
#define CAN_TSTATR_TME2                         ((uint32_t)0x10000000) /* Transmit Mailbox 2 Empty */

#define CAN_TSTATR_LOW                          ((uint32_t)0xE0000000) /* LOW[2:0] bits */
#define CAN_TSTATR_LOW0                         ((uint32_t)0x20000000) /* Lowest Priority Flag for Mailbox 0 */
#define CAN_TSTATR_LOW1                         ((uint32_t)0x40000000) /* Lowest Priority Flag for Mailbox 1 */
#define CAN_TSTATR_LOW2                         ((uint32_t)0x80000000) /* Lowest Priority Flag for Mailbox 2 */

/*******************  Bit definition for CAN_RFIFO0 register  *******************/
#define CAN_RFIFO0_FMP0                         ((uint8_t)0x03) /* FIFO 0 Message Pending */
#define CAN_RFIFO0_FULL0                        ((uint8_t)0x08) /* FIFO 0 Full */
#define CAN_RFIFO0_FOVR0                        ((uint8_t)0x10) /* FIFO 0 Overrun */
#define CAN_RFIFO0_RFOM0                        ((uint8_t)0x20) /* Release FIFO 0 Output Mailbox */

/*******************  Bit definition for CAN_RFIFO1 register  *******************/
#define CAN_RFIFO1_FMP1                         ((uint8_t)0x03) /* FIFO 1 Message Pending */
#define CAN_RFIFO1_FULL1                        ((uint8_t)0x08) /* FIFO 1 Full */
#define CAN_RFIFO1_FOVR1                        ((uint8_t)0x10) /* FIFO 1 Overrun */
#define CAN_RFIFO1_RFOM1                        ((uint8_t)0x20) /* Release FIFO 1 Output Mailbox */

/********************  Bit definition for CAN_INTENR register  *******************/
#define CAN_INTENR_TMEIE                        ((uint32_t)0x00000001) /* Transmit Mailbox Empty Interrupt Enable */
#define CAN_INTENR_FMPIE0                       ((uint32_t)0x00000002) /* FIFO Message Pending Interrupt Enable */
#define CAN_INTENR_FFIE0                        ((uint32_t)0x00000004) /* FIFO Full Interrupt Enable */
#define CAN_INTENR_FOVIE0                       ((uint32_t)0x00000008) /* FIFO Overrun Interrupt Enable */
#define CAN_INTENR_FMPIE1                       ((uint32_t)0x00000010) /* FIFO Message Pending Interrupt Enable */
#define CAN_INTENR_FFIE1                        ((uint32_t)0x00000020) /* FIFO Full Interrupt Enable */
#define CAN_INTENR_FOVIE1                       ((uint32_t)0x00000040) /* FIFO Overrun Interrupt Enable */
#define CAN_INTENR_EWGIE                        ((uint32_t)0x00000100) /* Error Warning Interrupt Enable */
#define CAN_INTENR_EPVIE                        ((uint32_t)0x00000200) /* Error Passive Interrupt Enable */
#define CAN_INTENR_BOFIE                        ((uint32_t)0x00000400) /* Bus-Off Interrupt Enable */
#define CAN_INTENR_LECIE                        ((uint32_t)0x00000800) /* Last Error Code Interrupt Enable */
#define CAN_INTENR_ERRIE                        ((uint32_t)0x00008000) /* Error Interrupt Enable */
#define CAN_INTENR_WKUIE                        ((uint32_t)0x00010000) /* Wakeup Interrupt Enable */
#define CAN_INTENR_SLKIE                        ((uint32_t)0x00020000) /* Sleep Interrupt Enable */

/********************  Bit definition for CAN_ERRSR register  *******************/
#define CAN_ERRSR_EWGF                          ((uint32_t)0x00000001) /* Error Warning Flag */
#define CAN_ERRSR_EPVF                          ((uint32_t)0x00000002) /* Error Passive Flag */
#define CAN_ERRSR_BOFF                          ((uint32_t)0x00000004) /* Bus-Off Flag */

#define CAN_ERRSR_LEC                           ((uint32_t)0x00000070) /* LEC[2:0] bits (Last Error Code) */
#define CAN_ERRSR_LEC_0                         ((uint32_t)0x00000010) /* Bit 0 */
#define CAN_ERRSR_LEC_1                         ((uint32_t)0x00000020) /* Bit 1 */
#define CAN_ERRSR_LEC_2                         ((uint32_t)0x00000040) /* Bit 2 */

#define CAN_ERRSR_TEC                           ((uint32_t)0x00FF0000) /* Least significant byte of the 9-bit Transmit Error Counter */
#define CAN_ERRSR_REC                           ((uint32_t)0xFF000000) /* Receive Error Counter */

/*******************  Bit definition for CAN_BTIMR register  ********************/
#define CAN_BTIMR_BRP                           ((uint32_t)0x000003FF) /* Baud Rate Prescaler */
#define CAN_BTIMR_TS1                           ((uint32_t)0x000F0000) /* Time Segment 1 */
#define CAN_BTIMR_TS2                           ((uint32_t)0x00700000) /* Time Segment 2 */
#define CAN_BTIMR_SJW                           ((uint32_t)0x03000000) /* Resynchronization Jump Width */
#define CAN_BTIMR_LBKM                          ((uint32_t)0x40000000) /* Loop Back Mode (Debug) */
#define CAN_BTIMR_SILM                          ((uint32_t)0x80000000) /* Silent Mode */

/******************  Bit definition for CAN_TXMI0R register  ********************/
#define CAN_TXMI0R_TXRQ                         ((uint32_t)0x00000001) /* Transmit Mailbox Request */
#define CAN_TXMI0R_RTR                          ((uint32_t)0x00000002) /* Remote Transmission Request */
#define CAN_TXMI0R_IDE                          ((uint32_t)0x00000004) /* Identifier Extension */
#define CAN_TXMI0R_EXID                         ((uint32_t)0x001FFFF8) /* Extended Identifier */
#define CAN_TXMI0R_STID                         ((uint32_t)0xFFE00000) /* Standard Identifier or Extended Identifier */

/******************  Bit definition for CAN_TXMDT0R register  *******************/
#define CAN_TXMDT0R_DLC                         ((uint32_t)0x0000000F) /* Data Length Code */
#define CAN_TXMDT0R_TGT                         ((uint32_t)0x00000100) /* Transmit Global Time */
#define CAN_TXMDT0R_TIME                        ((uint32_t)0xFFFF0000) /* Message Time Stamp */

/******************  Bit definition for CAN_TXMDL0R register  *******************/
#define CAN_TXMDL0R_DATA0                       ((uint32_t)0x000000FF) /* Data byte 0 */
#define CAN_TXMDL0R_DATA1                       ((uint32_t)0x0000FF00) /* Data byte 1 */
#define CAN_TXMDL0R_DATA2                       ((uint32_t)0x00FF0000) /* Data byte 2 */
#define CAN_TXMDL0R_DATA3                       ((uint32_t)0xFF000000) /* Data byte 3 */

/******************  Bit definition for CAN_TXMDH0R register  *******************/
#define CAN_TXMDH0R_DATA4                       ((uint32_t)0x000000FF) /* Data byte 4 */
#define CAN_TXMDH0R_DATA5                       ((uint32_t)0x0000FF00) /* Data byte 5 */
#define CAN_TXMDH0R_DATA6                       ((uint32_t)0x00FF0000) /* Data byte 6 */
#define CAN_TXMDH0R_DATA7                       ((uint32_t)0xFF000000) /* Data byte 7 */

/*******************  Bit definition for CAN_TXMI1R register  *******************/
#define CAN_TXMI1R_TXRQ                         ((uint32_t)0x00000001) /* Transmit Mailbox Request */
#define CAN_TXMI1R_RTR                          ((uint32_t)0x00000002) /* Remote Transmission Request */
#define CAN_TXMI1R_IDE                          ((uint32_t)0x00000004) /* Identifier Extension */
#define CAN_TXMI1R_EXID                         ((uint32_t)0x001FFFF8) /* Extended Identifier */
#define CAN_TXMI1R_STID                         ((uint32_t)0xFFE00000) /* Standard Identifier or Extended Identifier */

/*******************  Bit definition for CAN_TXMDT1R register  ******************/
#define CAN_TXMDT1R_DLC                         ((uint32_t)0x0000000F) /* Data Length Code */
#define CAN_TXMDT1R_TGT                         ((uint32_t)0x00000100) /* Transmit Global Time */
#define CAN_TXMDT1R_TIME                        ((uint32_t)0xFFFF0000) /* Message Time Stamp */

/*******************  Bit definition for CAN_TXMDL1R register  ******************/
#define CAN_TXMDL1R_DATA0                       ((uint32_t)0x000000FF) /* Data byte 0 */
#define CAN_TXMDL1R_DATA1                       ((uint32_t)0x0000FF00) /* Data byte 1 */
#define CAN_TXMDL1R_DATA2                       ((uint32_t)0x00FF0000) /* Data byte 2 */
#define CAN_TXMDL1R_DATA3                       ((uint32_t)0xFF000000) /* Data byte 3 */

/*******************  Bit definition for CAN_TXMDH1R register  ******************/
#define CAN_TXMDH1R_DATA4                       ((uint32_t)0x000000FF) /* Data byte 4 */
#define CAN_TXMDH1R_DATA5                       ((uint32_t)0x0000FF00) /* Data byte 5 */
#define CAN_TXMDH1R_DATA6                       ((uint32_t)0x00FF0000) /* Data byte 6 */
#define CAN_TXMDH1R_DATA7                       ((uint32_t)0xFF000000) /* Data byte 7 */

/*******************  Bit definition for CAN_TXMI2R register  *******************/
#define CAN_TXMI2R_TXRQ                         ((uint32_t)0x00000001) /* Transmit Mailbox Request */
#define CAN_TXMI2R_RTR                          ((uint32_t)0x00000002) /* Remote Transmission Request */
#define CAN_TXMI2R_IDE                          ((uint32_t)0x00000004) /* Identifier Extension */
#define CAN_TXMI2R_EXID                         ((uint32_t)0x001FFFF8) /* Extended identifier */
#define CAN_TXMI2R_STID                         ((uint32_t)0xFFE00000) /* Standard Identifier or Extended Identifier */

/*******************  Bit definition for CAN_TXMDT2R register  ******************/
#define CAN_TXMDT2R_DLC                         ((uint32_t)0x0000000F) /* Data Length Code */
#define CAN_TXMDT2R_TGT                         ((uint32_t)0x00000100) /* Transmit Global Time */
#define CAN_TXMDT2R_TIME                        ((uint32_t)0xFFFF0000) /* Message Time Stamp */

/*******************  Bit definition for CAN_TXMDL2R register  ******************/
#define CAN_TXMDL2R_DATA0                       ((uint32_t)0x000000FF) /* Data byte 0 */
#define CAN_TXMDL2R_DATA1                       ((uint32_t)0x0000FF00) /* Data byte 1 */
#define CAN_TXMDL2R_DATA2                       ((uint32_t)0x00FF0000) /* Data byte 2 */
#define CAN_TXMDL2R_DATA3                       ((uint32_t)0xFF000000) /* Data byte 3 */

/*******************  Bit definition for CAN_TXMDH2R register  ******************/
#define CAN_TXMDH2R_DATA4                       ((uint32_t)0x000000FF) /* Data byte 4 */
#define CAN_TXMDH2R_DATA5                       ((uint32_t)0x0000FF00) /* Data byte 5 */
#define CAN_TXMDH2R_DATA6                       ((uint32_t)0x00FF0000) /* Data byte 6 */
#define CAN_TXMDH2R_DATA7                       ((uint32_t)0xFF000000) /* Data byte 7 */

/*******************  Bit definition for CAN_RXMI0R register  *******************/
#define CAN_RXMI0R_RTR                          ((uint32_t)0x00000002) /* Remote Transmission Request */
#define CAN_RXMI0R_IDE                          ((uint32_t)0x00000004) /* Identifier Extension */
#define CAN_RXMI0R_EXID                         ((uint32_t)0x001FFFF8) /* Extended Identifier */
#define CAN_RXMI0R_STID                         ((uint32_t)0xFFE00000) /* Standard Identifier or Extended Identifier */

/*******************  Bit definition for CAN_RXMDT0R register  ******************/
#define CAN_RXMDT0R_DLC                         ((uint32_t)0x0000000F) /* Data Length Code */
#define CAN_RXMDT0R_FMI                         ((uint32_t)0x0000FF00) /* Filter Match Index */
#define CAN_RXMDT0R_TIME                        ((uint32_t)0xFFFF0000) /* Message Time Stamp */

/*******************  Bit definition for CAN_RXMDL0R register  ******************/
#define CAN_RXMDL0R_DATA0                       ((uint32_t)0x000000FF) /* Data byte 0 */
#define CAN_RXMDL0R_DATA1                       ((uint32_t)0x0000FF00) /* Data byte 1 */
#define CAN_RXMDL0R_DATA2                       ((uint32_t)0x00FF0000) /* Data byte 2 */
#define CAN_RXMDL0R_DATA3                       ((uint32_t)0xFF000000) /* Data byte 3 */

/*******************  Bit definition for CAN_RXMDH0R register  ******************/
#define CAN_RXMDH0R_DATA4                       ((uint32_t)0x000000FF) /* Data byte 4 */
#define CAN_RXMDH0R_DATA5                       ((uint32_t)0x0000FF00) /* Data byte 5 */
#define CAN_RXMDH0R_DATA6                       ((uint32_t)0x00FF0000) /* Data byte 6 */
#define CAN_RXMDH0R_DATA7                       ((uint32_t)0xFF000000) /* Data byte 7 */

/*******************  Bit definition for CAN_RXMI1R register  *******************/
#define CAN_RXMI1R_RTR                          ((uint32_t)0x00000002) /* Remote Transmission Request */
#define CAN_RXMI1R_IDE                          ((uint32_t)0x00000004) /* Identifier Extension */
#define CAN_RXMI1R_EXID                         ((uint32_t)0x001FFFF8) /* Extended identifier */
#define CAN_RXMI1R_STID                         ((uint32_t)0xFFE00000) /* Standard Identifier or Extended Identifier */

/*******************  Bit definition for CAN_RXMDT1R register  ******************/
#define CAN_RXMDT1R_DLC                         ((uint32_t)0x0000000F) /* Data Length Code */
#define CAN_RXMDT1R_FMI                         ((uint32_t)0x0000FF00) /* Filter Match Index */
#define CAN_RXMDT1R_TIME                        ((uint32_t)0xFFFF0000) /* Message Time Stamp */

/*******************  Bit definition for CAN_RXMDL1R register  ******************/
#define CAN_RXMDL1R_DATA0                       ((uint32_t)0x000000FF) /* Data byte 0 */
#define CAN_RXMDL1R_DATA1                       ((uint32_t)0x0000FF00) /* Data byte 1 */
#define CAN_RXMDL1R_DATA2                       ((uint32_t)0x00FF0000) /* Data byte 2 */
#define CAN_RXMDL1R_DATA3                       ((uint32_t)0xFF000000) /* Data byte 3 */

/*******************  Bit definition for CAN_RXMDH1R register  ******************/
#define CAN_RXMDH1R_DATA4                       ((uint32_t)0x000000FF) /* Data byte 4 */
#define CAN_RXMDH1R_DATA5                       ((uint32_t)0x0000FF00) /* Data byte 5 */
#define CAN_RXMDH1R_DATA6                       ((uint32_t)0x00FF0000) /* Data byte 6 */
#define CAN_RXMDH1R_DATA7                       ((uint32_t)0xFF000000) /* Data byte 7 */

/*******************  Bit definition for CAN_FCTLR register  ********************/
#define CAN_FCTLR_FINIT                         ((uint8_t)0x01) /* Filter Init Mode */

/*******************  Bit definition for CAN_FMCFGR register  *******************/
#define CAN_FMCFGR_FBM                          ((uint16_t)0x3FFF) /* Filter Mode */
#define CAN_FMCFGR_FBM0                         ((uint16_t)0x0001) /* Filter Init Mode bit 0 */
#define CAN_FMCFGR_FBM1                         ((uint16_t)0x0002) /* Filter Init Mode bit 1 */
#define CAN_FMCFGR_FBM2                         ((uint16_t)0x0004) /* Filter Init Mode bit 2 */
#define CAN_FMCFGR_FBM3                         ((uint16_t)0x0008) /* Filter Init Mode bit 3 */
#define CAN_FMCFGR_FBM4                         ((uint16_t)0x0010) /* Filter Init Mode bit 4 */
#define CAN_FMCFGR_FBM5                         ((uint16_t)0x0020) /* Filter Init Mode bit 5 */
#define CAN_FMCFGR_FBM6                         ((uint16_t)0x0040) /* Filter Init Mode bit 6 */
#define CAN_FMCFGR_FBM7                         ((uint16_t)0x0080) /* Filter Init Mode bit 7 */
#define CAN_FMCFGR_FBM8                         ((uint16_t)0x0100) /* Filter Init Mode bit 8 */
#define CAN_FMCFGR_FBM9                         ((uint16_t)0x0200) /* Filter Init Mode bit 9 */
#define CAN_FMCFGR_FBM10                        ((uint16_t)0x0400) /* Filter Init Mode bit 10 */
#define CAN_FMCFGR_FBM11                        ((uint16_t)0x0800) /* Filter Init Mode bit 11 */
#define CAN_FMCFGR_FBM12                        ((uint16_t)0x1000) /* Filter Init Mode bit 12 */
#define CAN_FMCFGR_FBM13                        ((uint16_t)0x2000) /* Filter Init Mode bit 13 */

/*******************  Bit definition for CAN_FSCFGR register  *******************/
#define CAN_FSCFGR_FSC                          ((uint16_t)0x3FFF) /* Filter Scale Configuration */
#define CAN_FSCFGR_FSC0                         ((uint16_t)0x0001) /* Filter Scale Configuration bit 0 */
#define CAN_FSCFGR_FSC1                         ((uint16_t)0x0002) /* Filter Scale Configuration bit 1 */
#define CAN_FSCFGR_FSC2                         ((uint16_t)0x0004) /* Filter Scale Configuration bit 2 */
#define CAN_FSCFGR_FSC3                         ((uint16_t)0x0008) /* Filter Scale Configuration bit 3 */
#define CAN_FSCFGR_FSC4                         ((uint16_t)0x0010) /* Filter Scale Configuration bit 4 */
#define CAN_FSCFGR_FSC5                         ((uint16_t)0x0020) /* Filter Scale Configuration bit 5 */
#define CAN_FSCFGR_FSC6                         ((uint16_t)0x0040) /* Filter Scale Configuration bit 6 */
#define CAN_FSCFGR_FSC7                         ((uint16_t)0x0080) /* Filter Scale Configuration bit 7 */
#define CAN_FSCFGR_FSC8                         ((uint16_t)0x0100) /* Filter Scale Configuration bit 8 */
#define CAN_FSCFGR_FSC9                         ((uint16_t)0x0200) /* Filter Scale Configuration bit 9 */
#define CAN_FSCFGR_FSC10                        ((uint16_t)0x0400) /* Filter Scale Configuration bit 10 */
#define CAN_FSCFGR_FSC11                        ((uint16_t)0x0800) /* Filter Scale Configuration bit 11 */
#define CAN_FSCFGR_FSC12                        ((uint16_t)0x1000) /* Filter Scale Configuration bit 12 */
#define CAN_FSCFGR_FSC13                        ((uint16_t)0x2000) /* Filter Scale Configuration bit 13 */

/******************  Bit definition for CAN_FAFIFOR register  *******************/
#define CAN_FAFIFOR_FFA                         ((uint16_t)0x3FFF) /* Filter FIFO Assignment */
#define CAN_FAFIFOR_FFA0                        ((uint16_t)0x0001) /* Filter FIFO Assignment for Filter 0 */
#define CAN_FAFIFOR_FFA1                        ((uint16_t)0x0002) /* Filter FIFO Assignment for Filter 1 */
#define CAN_FAFIFOR_FFA2                        ((uint16_t)0x0004) /* Filter FIFO Assignment for Filter 2 */
#define CAN_FAFIFOR_FFA3                        ((uint16_t)0x0008) /* Filter FIFO Assignment for Filter 3 */
#define CAN_FAFIFOR_FFA4                        ((uint16_t)0x0010) /* Filter FIFO Assignment for Filter 4 */
#define CAN_FAFIFOR_FFA5                        ((uint16_t)0x0020) /* Filter FIFO Assignment for Filter 5 */
#define CAN_FAFIFOR_FFA6                        ((uint16_t)0x0040) /* Filter FIFO Assignment for Filter 6 */
#define CAN_FAFIFOR_FFA7                        ((uint16_t)0x0080) /* Filter FIFO Assignment for Filter 7 */
#define CAN_FAFIFOR_FFA8                        ((uint16_t)0x0100) /* Filter FIFO Assignment for Filter 8 */
#define CAN_FAFIFOR_FFA9                        ((uint16_t)0x0200) /* Filter FIFO Assignment for Filter 9 */
#define CAN_FAFIFOR_FFA10                       ((uint16_t)0x0400) /* Filter FIFO Assignment for Filter 10 */
#define CAN_FAFIFOR_FFA11                       ((uint16_t)0x0800) /* Filter FIFO Assignment for Filter 11 */
#define CAN_FAFIFOR_FFA12                       ((uint16_t)0x1000) /* Filter FIFO Assignment for Filter 12 */
#define CAN_FAFIFOR_FFA13                       ((uint16_t)0x2000) /* Filter FIFO Assignment for Filter 13 */

/*******************  Bit definition for CAN_FWR register  *******************/
#define CAN_FWR_FACT                            ((uint16_t)0x3FFF) /* Filter Active */
#define CAN_FWR_FACT0                           ((uint16_t)0x0001) /* Filter 0 Active */
#define CAN_FWR_FACT1                           ((uint16_t)0x0002) /* Filter 1 Active */
#define CAN_FWR_FACT2                           ((uint16_t)0x0004) /* Filter 2 Active */
#define CAN_FWR_FACT3                           ((uint16_t)0x0008) /* Filter 3 Active */
#define CAN_FWR_FACT4                           ((uint16_t)0x0010) /* Filter 4 Active */
#define CAN_FWR_FACT5                           ((uint16_t)0x0020) /* Filter 5 Active */
#define CAN_FWR_FACT6                           ((uint16_t)0x0040) /* Filter 6 Active */
#define CAN_FWR_FACT7                           ((uint16_t)0x0080) /* Filter 7 Active */
#define CAN_FWR_FACT8                           ((uint16_t)0x0100) /* Filter 8 Active */
#define CAN_FWR_FACT9                           ((uint16_t)0x0200) /* Filter 9 Active */
#define CAN_FWR_FACT10                          ((uint16_t)0x0400) /* Filter 10 Active */
#define CAN_FWR_FACT11                          ((uint16_t)0x0800) /* Filter 11 Active */
#define CAN_FWR_FACT12                          ((uint16_t)0x1000) /* Filter 12 Active */
#define CAN_FWR_FACT13                          ((uint16_t)0x2000) /* Filter 13 Active */

/*******************  Bit definition for CAN_F0R1 register  *******************/
#define CAN_F0R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F0R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F0R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F0R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F0R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F0R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F0R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F0R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F0R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F0R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F0R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F0R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F0R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F0R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F0R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F0R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F0R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F0R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F0R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F0R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F0R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F0R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F0R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F0R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F0R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F0R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F0R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F0R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F0R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F0R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F0R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F0R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F1R1 register  *******************/
#define CAN_F1R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F1R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F1R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F1R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F1R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F1R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F1R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F1R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F1R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F1R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F1R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F1R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F1R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F1R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F1R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F1R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F1R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F1R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F1R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F1R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F1R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F1R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F1R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F1R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F1R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F1R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F1R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F1R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F1R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F1R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F1R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F1R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F2R1 register  *******************/
#define CAN_F2R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F2R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F2R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F2R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F2R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F2R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F2R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F2R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F2R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F2R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F2R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F2R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F2R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F2R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F2R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F2R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F2R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F2R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F2R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F2R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F2R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F2R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F2R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F2R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F2R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F2R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F2R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F2R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F2R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F2R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F2R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F2R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F3R1 register  *******************/
#define CAN_F3R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F3R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F3R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F3R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F3R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F3R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F3R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F3R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F3R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F3R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F3R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F3R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F3R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F3R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F3R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F3R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F3R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F3R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F3R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F3R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F3R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F3R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F3R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F3R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F3R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F3R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F3R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F3R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F3R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F3R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F3R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F3R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F4R1 register  *******************/
#define CAN_F4R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F4R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F4R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F4R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F4R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F4R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F4R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F4R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F4R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F4R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F4R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F4R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F4R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F4R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F4R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F4R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F4R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F4R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F4R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F4R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F4R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F4R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F4R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F4R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F4R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F4R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F4R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F4R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F4R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F4R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F4R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F4R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F5R1 register  *******************/
#define CAN_F5R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F5R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F5R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F5R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F5R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F5R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F5R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F5R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F5R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F5R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F5R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F5R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F5R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F5R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F5R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F5R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F5R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F5R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F5R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F5R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F5R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F5R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F5R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F5R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F5R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F5R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F5R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F5R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F5R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F5R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F5R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F5R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F6R1 register  *******************/
#define CAN_F6R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F6R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F6R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F6R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F6R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F6R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F6R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F6R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F6R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F6R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F6R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F6R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F6R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F6R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F6R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F6R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F6R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F6R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F6R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F6R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F6R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F6R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F6R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F6R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F6R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F6R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F6R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F6R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F6R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F6R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F6R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F6R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F7R1 register  *******************/
#define CAN_F7R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F7R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F7R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F7R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F7R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F7R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F7R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F7R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F7R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F7R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F7R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F7R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F7R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F7R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F7R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F7R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F7R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F7R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F7R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F7R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F7R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F7R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F7R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F7R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F7R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F7R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F7R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F7R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F7R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F7R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F7R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F7R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F8R1 register  *******************/
#define CAN_F8R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F8R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F8R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F8R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F8R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F8R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F8R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F8R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F8R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F8R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F8R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F8R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F8R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F8R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F8R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F8R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F8R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F8R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F8R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F8R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F8R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F8R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F8R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F8R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F8R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F8R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F8R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F8R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F8R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F8R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F8R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F8R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F9R1 register  *******************/
#define CAN_F9R1_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F9R1_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F9R1_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F9R1_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F9R1_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F9R1_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F9R1_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F9R1_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F9R1_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F9R1_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F9R1_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F9R1_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F9R1_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F9R1_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F9R1_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F9R1_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F9R1_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F9R1_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F9R1_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F9R1_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F9R1_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F9R1_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F9R1_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F9R1_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F9R1_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F9R1_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F9R1_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F9R1_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F9R1_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F9R1_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F9R1_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F9R1_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F10R1 register  ******************/
#define CAN_F10R1_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F10R1_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F10R1_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F10R1_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F10R1_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F10R1_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F10R1_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F10R1_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F10R1_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F10R1_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F10R1_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F10R1_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F10R1_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F10R1_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F10R1_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F10R1_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F10R1_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F10R1_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F10R1_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F10R1_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F10R1_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F10R1_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F10R1_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F10R1_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F10R1_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F10R1_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F10R1_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F10R1_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F10R1_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F10R1_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F10R1_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F10R1_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F11R1 register  ******************/
#define CAN_F11R1_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F11R1_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F11R1_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F11R1_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F11R1_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F11R1_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F11R1_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F11R1_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F11R1_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F11R1_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F11R1_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F11R1_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F11R1_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F11R1_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F11R1_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F11R1_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F11R1_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F11R1_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F11R1_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F11R1_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F11R1_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F11R1_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F11R1_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F11R1_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F11R1_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F11R1_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F11R1_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F11R1_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F11R1_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F11R1_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F11R1_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F11R1_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F12R1 register  ******************/
#define CAN_F12R1_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F12R1_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F12R1_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F12R1_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F12R1_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F12R1_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F12R1_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F12R1_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F12R1_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F12R1_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F12R1_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F12R1_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F12R1_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F12R1_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F12R1_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F12R1_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F12R1_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F12R1_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F12R1_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F12R1_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F12R1_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F12R1_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F12R1_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F12R1_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F12R1_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F12R1_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F12R1_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F12R1_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F12R1_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F12R1_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F12R1_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F12R1_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F13R1 register  ******************/
#define CAN_F13R1_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F13R1_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F13R1_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F13R1_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F13R1_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F13R1_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F13R1_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F13R1_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F13R1_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F13R1_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F13R1_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F13R1_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F13R1_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F13R1_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F13R1_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F13R1_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F13R1_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F13R1_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F13R1_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F13R1_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F13R1_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F13R1_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F13R1_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F13R1_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F13R1_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F13R1_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F13R1_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F13R1_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F13R1_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F13R1_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F13R1_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F13R1_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F0R2 register  *******************/
#define CAN_F0R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F0R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F0R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F0R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F0R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F0R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F0R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F0R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F0R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F0R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F0R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F0R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F0R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F0R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F0R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F0R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F0R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F0R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F0R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F0R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F0R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F0R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F0R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F0R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F0R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F0R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F0R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F0R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F0R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F0R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F0R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F0R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F1R2 register  *******************/
#define CAN_F1R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F1R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F1R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F1R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F1R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F1R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F1R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F1R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F1R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F1R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F1R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F1R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F1R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F1R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F1R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F1R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F1R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F1R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F1R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F1R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F1R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F1R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F1R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F1R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F1R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F1R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F1R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F1R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F1R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F1R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F1R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F1R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F2R2 register  *******************/
#define CAN_F2R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F2R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F2R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F2R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F2R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F2R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F2R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F2R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F2R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F2R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F2R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F2R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F2R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F2R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F2R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F2R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F2R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F2R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F2R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F2R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F2R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F2R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F2R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F2R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F2R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F2R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F2R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F2R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F2R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F2R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F2R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F2R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F3R2 register  *******************/
#define CAN_F3R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F3R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F3R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F3R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F3R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F3R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F3R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F3R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F3R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F3R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F3R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F3R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F3R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F3R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F3R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F3R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F3R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F3R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F3R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F3R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F3R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F3R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F3R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F3R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F3R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F3R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F3R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F3R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F3R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F3R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F3R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F3R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F4R2 register  *******************/
#define CAN_F4R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F4R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F4R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F4R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F4R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F4R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F4R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F4R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F4R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F4R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F4R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F4R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F4R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F4R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F4R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F4R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F4R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F4R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F4R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F4R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F4R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F4R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F4R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F4R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F4R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F4R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F4R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F4R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F4R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F4R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F4R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F4R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F5R2 register  *******************/
#define CAN_F5R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F5R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F5R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F5R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F5R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F5R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F5R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F5R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F5R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F5R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F5R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F5R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F5R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F5R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F5R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F5R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F5R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F5R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F5R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F5R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F5R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F5R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F5R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F5R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F5R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F5R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F5R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F5R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F5R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F5R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F5R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F5R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F6R2 register  *******************/
#define CAN_F6R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F6R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F6R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F6R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F6R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F6R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F6R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F6R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F6R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F6R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F6R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F6R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F6R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F6R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F6R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F6R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F6R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F6R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F6R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F6R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F6R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F6R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F6R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F6R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F6R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F6R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F6R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F6R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F6R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F6R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F6R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F6R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F7R2 register  *******************/
#define CAN_F7R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F7R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F7R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F7R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F7R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F7R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F7R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F7R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F7R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F7R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F7R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F7R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F7R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F7R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F7R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F7R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F7R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F7R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F7R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F7R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F7R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F7R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F7R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F7R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F7R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F7R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F7R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F7R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F7R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F7R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F7R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F7R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F8R2 register  *******************/
#define CAN_F8R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F8R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F8R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F8R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F8R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F8R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F8R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F8R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F8R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F8R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F8R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F8R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F8R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F8R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F8R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F8R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F8R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F8R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F8R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F8R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F8R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F8R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F8R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F8R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F8R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F8R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F8R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F8R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F8R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F8R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F8R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F8R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F9R2 register  *******************/
#define CAN_F9R2_FB0                            ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F9R2_FB1                            ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F9R2_FB2                            ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F9R2_FB3                            ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F9R2_FB4                            ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F9R2_FB5                            ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F9R2_FB6                            ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F9R2_FB7                            ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F9R2_FB8                            ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F9R2_FB9                            ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F9R2_FB10                           ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F9R2_FB11                           ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F9R2_FB12                           ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F9R2_FB13                           ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F9R2_FB14                           ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F9R2_FB15                           ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F9R2_FB16                           ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F9R2_FB17                           ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F9R2_FB18                           ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F9R2_FB19                           ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F9R2_FB20                           ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F9R2_FB21                           ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F9R2_FB22                           ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F9R2_FB23                           ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F9R2_FB24                           ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F9R2_FB25                           ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F9R2_FB26                           ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F9R2_FB27                           ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F9R2_FB28                           ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F9R2_FB29                           ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F9R2_FB30                           ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F9R2_FB31                           ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F10R2 register  ******************/
#define CAN_F10R2_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F10R2_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F10R2_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F10R2_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F10R2_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F10R2_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F10R2_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F10R2_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F10R2_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F10R2_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F10R2_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F10R2_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F10R2_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F10R2_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F10R2_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F10R2_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F10R2_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F10R2_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F10R2_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F10R2_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F10R2_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F10R2_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F10R2_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F10R2_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F10R2_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F10R2_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F10R2_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F10R2_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F10R2_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F10R2_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F10R2_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F10R2_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F11R2 register  ******************/
#define CAN_F11R2_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F11R2_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F11R2_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F11R2_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F11R2_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F11R2_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F11R2_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F11R2_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F11R2_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F11R2_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F11R2_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F11R2_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F11R2_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F11R2_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F11R2_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F11R2_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F11R2_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F11R2_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F11R2_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F11R2_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F11R2_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F11R2_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F11R2_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F11R2_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F11R2_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F11R2_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F11R2_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F11R2_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F11R2_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F11R2_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F11R2_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F11R2_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F12R2 register  ******************/
#define CAN_F12R2_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F12R2_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F12R2_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F12R2_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F12R2_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F12R2_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F12R2_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F12R2_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F12R2_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F12R2_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F12R2_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F12R2_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F12R2_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F12R2_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F12R2_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F12R2_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F12R2_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F12R2_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F12R2_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F12R2_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F12R2_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F12R2_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F12R2_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F12R2_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F12R2_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F12R2_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F12R2_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F12R2_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F12R2_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F12R2_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F12R2_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F12R2_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/*******************  Bit definition for CAN_F13R2 register  ******************/
#define CAN_F13R2_FB0                           ((uint32_t)0x00000001) /* Filter bit 0 */
#define CAN_F13R2_FB1                           ((uint32_t)0x00000002) /* Filter bit 1 */
#define CAN_F13R2_FB2                           ((uint32_t)0x00000004) /* Filter bit 2 */
#define CAN_F13R2_FB3                           ((uint32_t)0x00000008) /* Filter bit 3 */
#define CAN_F13R2_FB4                           ((uint32_t)0x00000010) /* Filter bit 4 */
#define CAN_F13R2_FB5                           ((uint32_t)0x00000020) /* Filter bit 5 */
#define CAN_F13R2_FB6                           ((uint32_t)0x00000040) /* Filter bit 6 */
#define CAN_F13R2_FB7                           ((uint32_t)0x00000080) /* Filter bit 7 */
#define CAN_F13R2_FB8                           ((uint32_t)0x00000100) /* Filter bit 8 */
#define CAN_F13R2_FB9                           ((uint32_t)0x00000200) /* Filter bit 9 */
#define CAN_F13R2_FB10                          ((uint32_t)0x00000400) /* Filter bit 10 */
#define CAN_F13R2_FB11                          ((uint32_t)0x00000800) /* Filter bit 11 */
#define CAN_F13R2_FB12                          ((uint32_t)0x00001000) /* Filter bit 12 */
#define CAN_F13R2_FB13                          ((uint32_t)0x00002000) /* Filter bit 13 */
#define CAN_F13R2_FB14                          ((uint32_t)0x00004000) /* Filter bit 14 */
#define CAN_F13R2_FB15                          ((uint32_t)0x00008000) /* Filter bit 15 */
#define CAN_F13R2_FB16                          ((uint32_t)0x00010000) /* Filter bit 16 */
#define CAN_F13R2_FB17                          ((uint32_t)0x00020000) /* Filter bit 17 */
#define CAN_F13R2_FB18                          ((uint32_t)0x00040000) /* Filter bit 18 */
#define CAN_F13R2_FB19                          ((uint32_t)0x00080000) /* Filter bit 19 */
#define CAN_F13R2_FB20                          ((uint32_t)0x00100000) /* Filter bit 20 */
#define CAN_F13R2_FB21                          ((uint32_t)0x00200000) /* Filter bit 21 */
#define CAN_F13R2_FB22                          ((uint32_t)0x00400000) /* Filter bit 22 */
#define CAN_F13R2_FB23                          ((uint32_t)0x00800000) /* Filter bit 23 */
#define CAN_F13R2_FB24                          ((uint32_t)0x01000000) /* Filter bit 24 */
#define CAN_F13R2_FB25                          ((uint32_t)0x02000000) /* Filter bit 25 */
#define CAN_F13R2_FB26                          ((uint32_t)0x04000000) /* Filter bit 26 */
#define CAN_F13R2_FB27                          ((uint32_t)0x08000000) /* Filter bit 27 */
#define CAN_F13R2_FB28                          ((uint32_t)0x10000000) /* Filter bit 28 */
#define CAN_F13R2_FB29                          ((uint32_t)0x20000000) /* Filter bit 29 */
#define CAN_F13R2_FB30                          ((uint32_t)0x40000000) /* Filter bit 30 */
#define CAN_F13R2_FB31                          ((uint32_t)0x80000000) /* Filter bit 31 */

/******************************************************************************/
/*                          CRC Calculation Unit                              */
/******************************************************************************/

/*******************  Bit definition for CRC_DATAR register  *********************/
#define CRC_DATAR_DR                            ((uint32_t)0xFFFFFFFF) /* Data register bits */

/*******************  Bit definition for CRC_IDATAR register  ********************/
#define CRC_IDR_IDATAR                          ((uint8_t)0xFF) /* General-purpose 8-bit data register bits */

/********************  Bit definition for CRC_CTLR register  ********************/
#define CRC_CTLR_RESET                          ((uint8_t)0x01) /* RESET bit */
#endif

#if defined(CH32V30x)
/******************************************************************************/
/*                      Digital to Analog Converter                           */
/******************************************************************************/

/********************  Bit definition for DAC_CTLR register  ********************/
#define  DAC_EN1                          ((uint32_t)0x00000001)        /* DAC channel1 enable */
#define  DAC_BOFF1                        ((uint32_t)0x00000002)        /* DAC channel1 output buffer disable */
#define  DAC_TEN1                         ((uint32_t)0x00000004)        /* DAC channel1 Trigger enable */

#define  DAC_TSEL1                        ((uint32_t)0x00000038)        /* TSEL1[2:0] (DAC channel1 Trigger selection) */
#define  DAC_TSEL1_0                      ((uint32_t)0x00000008)        /* Bit 0 */
#define  DAC_TSEL1_1                      ((uint32_t)0x00000010)        /* Bit 1 */
#define  DAC_TSEL1_2                      ((uint32_t)0x00000020)        /* Bit 2 */

#define  DAC_WAVE1                        ((uint32_t)0x000000C0)        /* WAVE1[1:0] (DAC channel1 noise/triangle wave generation enable) */
#define  DAC_WAVE1_0                      ((uint32_t)0x00000040)        /* Bit 0 */
#define  DAC_WAVE1_1                      ((uint32_t)0x00000080)        /* Bit 1 */

#define  DAC_MAMP1                        ((uint32_t)0x00000F00)        /* MAMP1[3:0] (DAC channel1 Mask/Amplitude selector) */
#define  DAC_MAMP1_0                      ((uint32_t)0x00000100)        /* Bit 0 */
#define  DAC_MAMP1_1                      ((uint32_t)0x00000200)        /* Bit 1 */
#define  DAC_MAMP1_2                      ((uint32_t)0x00000400)        /* Bit 2 */
#define  DAC_MAMP1_3                      ((uint32_t)0x00000800)        /* Bit 3 */

#define  DAC_DMAEN1                       ((uint32_t)0x00001000)        /* DAC channel1 DMA enable */
#define  DAC_EN2                          ((uint32_t)0x00010000)        /* DAC channel2 enable */
#define  DAC_BOFF2                        ((uint32_t)0x00020000)        /* DAC channel2 output buffer disable */
#define  DAC_TEN2                         ((uint32_t)0x00040000)        /* DAC channel2 Trigger enable */

#define  DAC_TSEL2                        ((uint32_t)0x00380000)        /* TSEL2[2:0] (DAC channel2 Trigger selection) */
#define  DAC_TSEL2_0                      ((uint32_t)0x00080000)        /* Bit 0 */
#define  DAC_TSEL2_1                      ((uint32_t)0x00100000)        /* Bit 1 */
#define  DAC_TSEL2_2                      ((uint32_t)0x00200000)        /* Bit 2 */

#define  DAC_WAVE2                        ((uint32_t)0x00C00000)        /* WAVE2[1:0] (DAC channel2 noise/triangle wave generation enable) */
#define  DAC_WAVE2_0                      ((uint32_t)0x00400000)        /* Bit 0 */
#define  DAC_WAVE2_1                      ((uint32_t)0x00800000)        /* Bit 1 */

#define  DAC_MAMP2                        ((uint32_t)0x0F000000)        /* MAMP2[3:0] (DAC channel2 Mask/Amplitude selector) */
#define  DAC_MAMP2_0                      ((uint32_t)0x01000000)        /* Bit 0 */
#define  DAC_MAMP2_1                      ((uint32_t)0x02000000)        /* Bit 1 */
#define  DAC_MAMP2_2                      ((uint32_t)0x04000000)        /* Bit 2 */
#define  DAC_MAMP2_3                      ((uint32_t)0x08000000)        /* Bit 3 */

#define  DAC_DMAEN2                       ((uint32_t)0x10000000)        /* DAC channel2 DMA enabled */

/*****************  Bit definition for DAC_SWTR register  ******************/
#define  DAC_SWTRIG1                      ((uint8_t)0x01)               /* DAC channel1 software trigger */
#define  DAC_SWTRIG2                      ((uint8_t)0x02)               /* DAC channel2 software trigger */

/*****************  Bit definition for DAC_R12BDHR1 register  ******************/
#define  DAC_DHR12R1                      ((uint16_t)0x0FFF)            /* DAC channel1 12-bit Right aligned data */

/*****************  Bit definition for DAC_L12BDHR1 register  ******************/
#define  DAC_DHR12L1                      ((uint16_t)0xFFF0)            /* DAC channel1 12-bit Left aligned data */

/******************  Bit definition for DAC_R8BDHR1 register  ******************/
#define  DAC_DHR8R1                       ((uint8_t)0xFF)               /* DAC channel1 8-bit Right aligned data */

/*****************  Bit definition for DAC_R12BDHR2 register  ******************/
#define  DAC_DHR12R2                      ((uint16_t)0x0FFF)            /* DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_L12BDHR2 register  ******************/
#define  DAC_DHR12L2                      ((uint16_t)0xFFF0)            /* DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_R8BDHR2 register  ******************/
#define  DAC_DHR8R2                       ((uint8_t)0xFF)               /* DAC channel2 8-bit Right aligned data */

/*****************  Bit definition for DAC_RD12BDHR register  ******************/
#define  DAC_RD12BDHR_DACC1DHR            ((uint32_t)0x00000FFF)        /* DAC channel1 12-bit Right aligned data */
#define  DAC_RD12BDHR_DACC2DHR            ((uint32_t)0x0FFF0000)        /* DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_LD12BDHR register  ******************/
#define  DAC_LD12BDHR_DACC1DHR            ((uint32_t)0x0000FFF0)        /* DAC channel1 12-bit Left aligned data */
#define  DAC_LD12BDHR_DACC2DHR            ((uint32_t)0xFFF00000)        /* DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_RD8BDHR register  ******************/
#define  DAC_RD8BDHR_DACC1DHR             ((uint16_t)0x00FF)            /* DAC channel1 8-bit Right aligned data */
#define  DAC_RD8BDHR_DACC2DHR             ((uint16_t)0xFF00)            /* DAC channel2 8-bit Right aligned data */

/*******************  Bit definition for DAC_DOR1 register  *******************/
#define  DAC_DACC1DOR                     ((uint16_t)0x0FFF)            /* DAC channel1 data output */

/*******************  Bit definition for DAC_DOR2 register  *******************/
#define  DAC_DACC2DOR                     ((uint16_t)0x0FFF)            /* DAC channel2 data output */
#endif

/******************************************************************************/
/*                             DMA Controller                                 */
/******************************************************************************/

/*******************  Bit definition for DMA_INTFR register  ********************/
#define DMA_GIF1                                ((uint32_t)0x00000001) /* Channel 1 Global interrupt flag */
#define DMA_TCIF1                               ((uint32_t)0x00000002) /* Channel 1 Transfer Complete flag */
#define DMA_HTIF1                               ((uint32_t)0x00000004) /* Channel 1 Half Transfer flag */
#define DMA_TEIF1                               ((uint32_t)0x00000008) /* Channel 1 Transfer Error flag */
#define DMA_GIF2                                ((uint32_t)0x00000010) /* Channel 2 Global interrupt flag */
#define DMA_TCIF2                               ((uint32_t)0x00000020) /* Channel 2 Transfer Complete flag */
#define DMA_HTIF2                               ((uint32_t)0x00000040) /* Channel 2 Half Transfer flag */
#define DMA_TEIF2                               ((uint32_t)0x00000080) /* Channel 2 Transfer Error flag */
#define DMA_GIF3                                ((uint32_t)0x00000100) /* Channel 3 Global interrupt flag */
#define DMA_TCIF3                               ((uint32_t)0x00000200) /* Channel 3 Transfer Complete flag */
#define DMA_HTIF3                               ((uint32_t)0x00000400) /* Channel 3 Half Transfer flag */
#define DMA_TEIF3                               ((uint32_t)0x00000800) /* Channel 3 Transfer Error flag */
#define DMA_GIF4                                ((uint32_t)0x00001000) /* Channel 4 Global interrupt flag */
#define DMA_TCIF4                               ((uint32_t)0x00002000) /* Channel 4 Transfer Complete flag */
#define DMA_HTIF4                               ((uint32_t)0x00004000) /* Channel 4 Half Transfer flag */
#define DMA_TEIF4                               ((uint32_t)0x00008000) /* Channel 4 Transfer Error flag */
#define DMA_GIF5                                ((uint32_t)0x00010000) /* Channel 5 Global interrupt flag */
#define DMA_TCIF5                               ((uint32_t)0x00020000) /* Channel 5 Transfer Complete flag */
#define DMA_HTIF5                               ((uint32_t)0x00040000) /* Channel 5 Half Transfer flag */
#define DMA_TEIF5                               ((uint32_t)0x00080000) /* Channel 5 Transfer Error flag */
#define DMA_GIF6                                ((uint32_t)0x00100000) /* Channel 6 Global interrupt flag */
#define DMA_TCIF6                               ((uint32_t)0x00200000) /* Channel 6 Transfer Complete flag */
#define DMA_HTIF6                               ((uint32_t)0x00400000) /* Channel 6 Half Transfer flag */
#define DMA_TEIF6                               ((uint32_t)0x00800000) /* Channel 6 Transfer Error flag */
#define DMA_GIF7                                ((uint32_t)0x01000000) /* Channel 7 Global interrupt flag */
#define DMA_TCIF7                               ((uint32_t)0x02000000) /* Channel 7 Transfer Complete flag */
#define DMA_HTIF7                               ((uint32_t)0x04000000) /* Channel 7 Half Transfer flag */
#define DMA_TEIF7                               ((uint32_t)0x08000000) /* Channel 7 Transfer Error flag */

#if defined(CH32V20x) || defined(CH32V30x)
#define DMA_GIF8                                ((uint32_t)0x00000001) /* Channel 8 Global interrupt flag */
#define DMA_TCIF8                               ((uint32_t)0x00000002) /* Channel 8 Transfer Complete flag */
#define DMA_HTIF8                               ((uint32_t)0x00000004) /* Channel 8 Half Transfer flag */
#define DMA_TEIF8                               ((uint32_t)0x00000008) /* Channel 8 Transfer Error flag */
#define DMA_GIF9                                ((uint32_t)0x00000010) /* Channel 9 Global interrupt flag */
#define DMA_TCIF9                               ((uint32_t)0x00000020) /* Channel 9 Transfer Complete flag */
#define DMA_HTIF9                               ((uint32_t)0x00000040) /* Channel 9 Half Transfer flag */
#define DMA_TEIF9                               ((uint32_t)0x00000080) /* Channel 9 Transfer Error flag */
#define DMA_GIF10                               ((uint32_t)0x00000100) /* Channel 10 Global interrupt flag */
#define DMA_TCIF10                              ((uint32_t)0x00000200) /* Channel 10 Transfer Complete flag */
#define DMA_HTIF10                              ((uint32_t)0x00000400) /* Channel 10 Half Transfer flag */
#define DMA_TEIF10                              ((uint32_t)0x00000800) /* Channel 10 Transfer Error flag */
#define DMA_GIF11                               ((uint32_t)0x00001000) /* Channel 11 Global interrupt flag */
#define DMA_TCIF11                              ((uint32_t)0x00002000) /* Channel 11 Transfer Complete flag */
#define DMA_HTIF11                              ((uint32_t)0x00004000) /* Channel 11 Half Transfer flag */
#define DMA_TEIF11                              ((uint32_t)0x00008000) /* Channel 11 Transfer Error flag */
#endif

/*******************  Bit definition for DMA_INTFCR register  *******************/
#define DMA_CGIF1                               ((uint32_t)0x00000001) /* Channel 1 Global interrupt clear */
#define DMA_CTCIF1                              ((uint32_t)0x00000002) /* Channel 1 Transfer Complete clear */
#define DMA_CHTIF1                              ((uint32_t)0x00000004) /* Channel 1 Half Transfer clear */
#define DMA_CTEIF1                              ((uint32_t)0x00000008) /* Channel 1 Transfer Error clear */
#define DMA_CGIF2                               ((uint32_t)0x00000010) /* Channel 2 Global interrupt clear */
#define DMA_CTCIF2                              ((uint32_t)0x00000020) /* Channel 2 Transfer Complete clear */
#define DMA_CHTIF2                              ((uint32_t)0x00000040) /* Channel 2 Half Transfer clear */
#define DMA_CTEIF2                              ((uint32_t)0x00000080) /* Channel 2 Transfer Error clear */
#define DMA_CGIF3                               ((uint32_t)0x00000100) /* Channel 3 Global interrupt clear */
#define DMA_CTCIF3                              ((uint32_t)0x00000200) /* Channel 3 Transfer Complete clear */
#define DMA_CHTIF3                              ((uint32_t)0x00000400) /* Channel 3 Half Transfer clear */
#define DMA_CTEIF3                              ((uint32_t)0x00000800) /* Channel 3 Transfer Error clear */
#define DMA_CGIF4                               ((uint32_t)0x00001000) /* Channel 4 Global interrupt clear */
#define DMA_CTCIF4                              ((uint32_t)0x00002000) /* Channel 4 Transfer Complete clear */
#define DMA_CHTIF4                              ((uint32_t)0x00004000) /* Channel 4 Half Transfer clear */
#define DMA_CTEIF4                              ((uint32_t)0x00008000) /* Channel 4 Transfer Error clear */
#define DMA_CGIF5                               ((uint32_t)0x00010000) /* Channel 5 Global interrupt clear */
#define DMA_CTCIF5                              ((uint32_t)0x00020000) /* Channel 5 Transfer Complete clear */
#define DMA_CHTIF5                              ((uint32_t)0x00040000) /* Channel 5 Half Transfer clear */
#define DMA_CTEIF5                              ((uint32_t)0x00080000) /* Channel 5 Transfer Error clear */
#define DMA_CGIF6                               ((uint32_t)0x00100000) /* Channel 6 Global interrupt clear */
#define DMA_CTCIF6                              ((uint32_t)0x00200000) /* Channel 6 Transfer Complete clear */
#define DMA_CHTIF6                              ((uint32_t)0x00400000) /* Channel 6 Half Transfer clear */
#define DMA_CTEIF6                              ((uint32_t)0x00800000) /* Channel 6 Transfer Error clear */
#define DMA_CGIF7                               ((uint32_t)0x01000000) /* Channel 7 Global interrupt clear */
#define DMA_CTCIF7                              ((uint32_t)0x02000000) /* Channel 7 Transfer Complete clear */
#define DMA_CHTIF7                              ((uint32_t)0x04000000) /* Channel 7 Half Transfer clear */
#define DMA_CTEIF7                              ((uint32_t)0x08000000) /* Channel 7 Transfer Error clear */

/*******************  Bit definition for DMA_CFGR1 register  *******************/
#define DMA_CFGR1_EN                            ((uint16_t)0x0001) /* Channel enable*/
#define DMA_CFGR1_TCIE                          ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFGR1_HTIE                          ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFGR1_TEIE                          ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFGR1_DIR                           ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFGR1_CIRC                          ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFGR1_PINC                          ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFGR1_MINC                          ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFGR1_PSIZE                         ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFGR1_PSIZE_0                       ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFGR1_PSIZE_1                       ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFGR1_MSIZE                         ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFGR1_MSIZE_0                       ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFGR1_MSIZE_1                       ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFGR1_PL                            ((uint16_t)0x3000) /* PL[1:0] bits(Channel Priority level) */
#define DMA_CFGR1_PL_0                          ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFGR1_PL_1                          ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFGR1_MEM2MEM                       ((uint16_t)0x4000) /* Memory to memory mode */

/*******************  Bit definition for DMA_CFGR2 register  *******************/
#define DMA_CFGR2_EN                            ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFGR2_TCIE                          ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFGR2_HTIE                          ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFGR2_TEIE                          ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFGR2_DIR                           ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFGR2_CIRC                          ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFGR2_PINC                          ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFGR2_MINC                          ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFGR2_PSIZE                         ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFGR2_PSIZE_0                       ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFGR2_PSIZE_1                       ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFGR2_MSIZE                         ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFGR2_MSIZE_0                       ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFGR2_MSIZE_1                       ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFGR2_PL                            ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFGR2_PL_0                          ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFGR2_PL_1                          ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFGR2_MEM2MEM                       ((uint16_t)0x4000) /* Memory to memory mode */

/*******************  Bit definition for DMA_CFGR3 register  *******************/
#define DMA_CFGR3_EN                            ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFGR3_TCIE                          ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFGR3_HTIE                          ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFGR3_TEIE                          ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFGR3_DIR                           ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFGR3_CIRC                          ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFGR3_PINC                          ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFGR3_MINC                          ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFGR3_PSIZE                         ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFGR3_PSIZE_0                       ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFGR3_PSIZE_1                       ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFGR3_MSIZE                         ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFGR3_MSIZE_0                       ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFGR3_MSIZE_1                       ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFGR3_PL                            ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFGR3_PL_0                          ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFGR3_PL_1                          ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFGR3_MEM2MEM                       ((uint16_t)0x4000) /* Memory to memory mode */

/*******************  Bit definition for DMA_CFG4 register  *******************/
#define DMA_CFG4_EN                             ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFG4_TCIE                           ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFG4_HTIE                           ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFG4_TEIE                           ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFG4_DIR                            ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFG4_CIRC                           ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFG4_PINC                           ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFG4_MINC                           ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFG4_PSIZE                          ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFG4_PSIZE_0                        ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFG4_PSIZE_1                        ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFG4_MSIZE                          ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFG4_MSIZE_0                        ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFG4_MSIZE_1                        ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFG4_PL                             ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFG4_PL_0                           ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFG4_PL_1                           ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFG4_MEM2MEM                        ((uint16_t)0x4000) /* Memory to memory mode */

/******************  Bit definition for DMA_CFG5 register  *******************/
#define DMA_CFG5_EN                             ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFG5_TCIE                           ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFG5_HTIE                           ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFG5_TEIE                           ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFG5_DIR                            ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFG5_CIRC                           ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFG5_PINC                           ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFG5_MINC                           ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFG5_PSIZE                          ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFG5_PSIZE_0                        ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFG5_PSIZE_1                        ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFG5_MSIZE                          ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFG5_MSIZE_0                        ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFG5_MSIZE_1                        ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFG5_PL                             ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFG5_PL_0                           ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFG5_PL_1                           ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFG5_MEM2MEM                        ((uint16_t)0x4000) /* Memory to memory mode enable */

/*******************  Bit definition for DMA_CFG6 register  *******************/
#define DMA_CFG6_EN                             ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFG6_TCIE                           ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFG6_HTIE                           ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFG6_TEIE                           ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFG6_DIR                            ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFG6_CIRC                           ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFG6_PINC                           ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFG6_MINC                           ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFG6_PSIZE                          ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFG6_PSIZE_0                        ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFG6_PSIZE_1                        ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFG6_MSIZE                          ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFG6_MSIZE_0                        ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFG6_MSIZE_1                        ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFG6_PL                             ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFG6_PL_0                           ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFG6_PL_1                           ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFG6_MEM2MEM                        ((uint16_t)0x4000) /* Memory to memory mode */

/*******************  Bit definition for DMA_CFG7 register  *******************/
#define DMA_CFG7_EN                             ((uint16_t)0x0001) /* Channel enable */
#define DMA_CFG7_TCIE                           ((uint16_t)0x0002) /* Transfer complete interrupt enable */
#define DMA_CFG7_HTIE                           ((uint16_t)0x0004) /* Half Transfer interrupt enable */
#define DMA_CFG7_TEIE                           ((uint16_t)0x0008) /* Transfer error interrupt enable */
#define DMA_CFG7_DIR                            ((uint16_t)0x0010) /* Data transfer direction */
#define DMA_CFG7_CIRC                           ((uint16_t)0x0020) /* Circular mode */
#define DMA_CFG7_PINC                           ((uint16_t)0x0040) /* Peripheral increment mode */
#define DMA_CFG7_MINC                           ((uint16_t)0x0080) /* Memory increment mode */

#define DMA_CFG7_PSIZE                          ((uint16_t)0x0300) /* PSIZE[1:0] bits (Peripheral size) */
#define DMA_CFG7_PSIZE_0                        ((uint16_t)0x0100) /* Bit 0 */
#define DMA_CFG7_PSIZE_1                        ((uint16_t)0x0200) /* Bit 1 */

#define DMA_CFG7_MSIZE                          ((uint16_t)0x0C00) /* MSIZE[1:0] bits (Memory size) */
#define DMA_CFG7_MSIZE_0                        ((uint16_t)0x0400) /* Bit 0 */
#define DMA_CFG7_MSIZE_1                        ((uint16_t)0x0800) /* Bit 1 */

#define DMA_CFG7_PL                             ((uint16_t)0x3000) /* PL[1:0] bits (Channel Priority level) */
#define DMA_CFG7_PL_0                           ((uint16_t)0x1000) /* Bit 0 */
#define DMA_CFG7_PL_1                           ((uint16_t)0x2000) /* Bit 1 */

#define DMA_CFG7_MEM2MEM                        ((uint16_t)0x4000) /* Memory to memory mode enable */

/******************  Bit definition for DMA_CNTR1 register  ******************/
#define DMA_CNTR1_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR2 register  ******************/
#define DMA_CNTR2_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR3 register  ******************/
#define DMA_CNTR3_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR4 register  ******************/
#define DMA_CNTR4_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR5 register  ******************/
#define DMA_CNTR5_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR6 register  ******************/
#define DMA_CNTR6_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_CNTR7 register  ******************/
#define DMA_CNTR7_NDT                           ((uint16_t)0xFFFF) /* Number of data to Transfer */

/******************  Bit definition for DMA_PADDR1 register  *******************/
#define DMA_PADDR1_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR2 register  *******************/
#define DMA_PADDR2_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR3 register  *******************/
#define DMA_PADDR3_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR4 register  *******************/
#define DMA_PADDR4_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR5 register  *******************/
#define DMA_PADDR5_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR6 register  *******************/
#define DMA_PADDR6_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_PADDR7 register  *******************/
#define DMA_PADDR7_PA                           ((uint32_t)0xFFFFFFFF) /* Peripheral Address */

/******************  Bit definition for DMA_MADDR1 register  *******************/
#define DMA_MADDR1_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR2 register  *******************/
#define DMA_MADDR2_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR3 register  *******************/
#define DMA_MADDR3_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR4 register  *******************/
#define DMA_MADDR4_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR5 register  *******************/
#define DMA_MADDR5_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR6 register  *******************/
#define DMA_MADDR6_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************  Bit definition for DMA_MADDR7 register  *******************/
#define DMA_MADDR7_MA                           ((uint32_t)0xFFFFFFFF) /* Memory Address */

/******************************************************************************/
/*                    External Interrupt/Event Controller                     */
/******************************************************************************/

/*******************  Bit definition for EXTI_INTENR register  *******************/
#define EXTI_INTENR_MR0                         ((uint32_t)0x00000001) /* Interrupt Mask on line 0 */
#define EXTI_INTENR_MR1                         ((uint32_t)0x00000002) /* Interrupt Mask on line 1 */
#define EXTI_INTENR_MR2                         ((uint32_t)0x00000004) /* Interrupt Mask on line 2 */
#define EXTI_INTENR_MR3                         ((uint32_t)0x00000008) /* Interrupt Mask on line 3 */
#define EXTI_INTENR_MR4                         ((uint32_t)0x00000010) /* Interrupt Mask on line 4 */
#define EXTI_INTENR_MR5                         ((uint32_t)0x00000020) /* Interrupt Mask on line 5 */
#define EXTI_INTENR_MR6                         ((uint32_t)0x00000040) /* Interrupt Mask on line 6 */
#define EXTI_INTENR_MR7                         ((uint32_t)0x00000080) /* Interrupt Mask on line 7 */
#define EXTI_INTENR_MR8                         ((uint32_t)0x00000100) /* Interrupt Mask on line 8 */
#define EXTI_INTENR_MR9                         ((uint32_t)0x00000200) /* Interrupt Mask on line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_INTENR_MR10                        ((uint32_t)0x00000400) /* Interrupt Mask on line 10 */
#define EXTI_INTENR_MR11                        ((uint32_t)0x00000800) /* Interrupt Mask on line 11 */
#define EXTI_INTENR_MR12                        ((uint32_t)0x00001000) /* Interrupt Mask on line 12 */
#define EXTI_INTENR_MR13                        ((uint32_t)0x00002000) /* Interrupt Mask on line 13 */
#define EXTI_INTENR_MR14                        ((uint32_t)0x00004000) /* Interrupt Mask on line 14 */
#define EXTI_INTENR_MR15                        ((uint32_t)0x00008000) /* Interrupt Mask on line 15 */
#define EXTI_INTENR_MR16                        ((uint32_t)0x00010000) /* Interrupt Mask on line 16 */
#define EXTI_INTENR_MR17                        ((uint32_t)0x00020000) /* Interrupt Mask on line 17 */
#define EXTI_INTENR_MR18                        ((uint32_t)0x00040000) /* Interrupt Mask on line 18 */
#define EXTI_INTENR_MR19                        ((uint32_t)0x00080000) /* Interrupt Mask on line 19 */
#endif

/*******************  Bit definition for EXTI_EVENR register  *******************/
#define EXTI_EVENR_MR0                          ((uint32_t)0x00000001) /* Event Mask on line 0 */
#define EXTI_EVENR_MR1                          ((uint32_t)0x00000002) /* Event Mask on line 1 */
#define EXTI_EVENR_MR2                          ((uint32_t)0x00000004) /* Event Mask on line 2 */
#define EXTI_EVENR_MR3                          ((uint32_t)0x00000008) /* Event Mask on line 3 */
#define EXTI_EVENR_MR4                          ((uint32_t)0x00000010) /* Event Mask on line 4 */
#define EXTI_EVENR_MR5                          ((uint32_t)0x00000020) /* Event Mask on line 5 */
#define EXTI_EVENR_MR6                          ((uint32_t)0x00000040) /* Event Mask on line 6 */
#define EXTI_EVENR_MR7                          ((uint32_t)0x00000080) /* Event Mask on line 7 */
#define EXTI_EVENR_MR8                          ((uint32_t)0x00000100) /* Event Mask on line 8 */
#define EXTI_EVENR_MR9                          ((uint32_t)0x00000200) /* Event Mask on line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_EVENR_MR10                         ((uint32_t)0x00000400) /* Event Mask on line 10 */
#define EXTI_EVENR_MR11                         ((uint32_t)0x00000800) /* Event Mask on line 11 */
#define EXTI_EVENR_MR12                         ((uint32_t)0x00001000) /* Event Mask on line 12 */
#define EXTI_EVENR_MR13                         ((uint32_t)0x00002000) /* Event Mask on line 13 */
#define EXTI_EVENR_MR14                         ((uint32_t)0x00004000) /* Event Mask on line 14 */
#define EXTI_EVENR_MR15                         ((uint32_t)0x00008000) /* Event Mask on line 15 */
#define EXTI_EVENR_MR16                         ((uint32_t)0x00010000) /* Event Mask on line 16 */
#define EXTI_EVENR_MR17                         ((uint32_t)0x00020000) /* Event Mask on line 17 */
#define EXTI_EVENR_MR18                         ((uint32_t)0x00040000) /* Event Mask on line 18 */
#define EXTI_EVENR_MR19                         ((uint32_t)0x00080000) /* Event Mask on line 19 */
#endif

/******************  Bit definition for EXTI_RTENR register  *******************/
#define EXTI_RTENR_TR0                          ((uint32_t)0x00000001) /* Rising trigger event configuration bit of line 0 */
#define EXTI_RTENR_TR1                          ((uint32_t)0x00000002) /* Rising trigger event configuration bit of line 1 */
#define EXTI_RTENR_TR2                          ((uint32_t)0x00000004) /* Rising trigger event configuration bit of line 2 */
#define EXTI_RTENR_TR3                          ((uint32_t)0x00000008) /* Rising trigger event configuration bit of line 3 */
#define EXTI_RTENR_TR4                          ((uint32_t)0x00000010) /* Rising trigger event configuration bit of line 4 */
#define EXTI_RTENR_TR5                          ((uint32_t)0x00000020) /* Rising trigger event configuration bit of line 5 */
#define EXTI_RTENR_TR6                          ((uint32_t)0x00000040) /* Rising trigger event configuration bit of line 6 */
#define EXTI_RTENR_TR7                          ((uint32_t)0x00000080) /* Rising trigger event configuration bit of line 7 */
#define EXTI_RTENR_TR8                          ((uint32_t)0x00000100) /* Rising trigger event configuration bit of line 8 */
#define EXTI_RTENR_TR9                          ((uint32_t)0x00000200) /* Rising trigger event configuration bit of line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_RTENR_TR10                         ((uint32_t)0x00000400) /* Rising trigger event configuration bit of line 10 */
#define EXTI_RTENR_TR11                         ((uint32_t)0x00000800) /* Rising trigger event configuration bit of line 11 */
#define EXTI_RTENR_TR12                         ((uint32_t)0x00001000) /* Rising trigger event configuration bit of line 12 */
#define EXTI_RTENR_TR13                         ((uint32_t)0x00002000) /* Rising trigger event configuration bit of line 13 */
#define EXTI_RTENR_TR14                         ((uint32_t)0x00004000) /* Rising trigger event configuration bit of line 14 */
#define EXTI_RTENR_TR15                         ((uint32_t)0x00008000) /* Rising trigger event configuration bit of line 15 */
#define EXTI_RTENR_TR16                         ((uint32_t)0x00010000) /* Rising trigger event configuration bit of line 16 */
#define EXTI_RTENR_TR17                         ((uint32_t)0x00020000) /* Rising trigger event configuration bit of line 17 */
#define EXTI_RTENR_TR18                         ((uint32_t)0x00040000) /* Rising trigger event configuration bit of line 18 */
#define EXTI_RTENR_TR19                         ((uint32_t)0x00080000) /* Rising trigger event configuration bit of line 19 */
#endif

/******************  Bit definition for EXTI_FTENR register  *******************/
#define EXTI_FTENR_TR0                          ((uint32_t)0x00000001) /* Falling trigger event configuration bit of line 0 */
#define EXTI_FTENR_TR1                          ((uint32_t)0x00000002) /* Falling trigger event configuration bit of line 1 */
#define EXTI_FTENR_TR2                          ((uint32_t)0x00000004) /* Falling trigger event configuration bit of line 2 */
#define EXTI_FTENR_TR3                          ((uint32_t)0x00000008) /* Falling trigger event configuration bit of line 3 */
#define EXTI_FTENR_TR4                          ((uint32_t)0x00000010) /* Falling trigger event configuration bit of line 4 */
#define EXTI_FTENR_TR5                          ((uint32_t)0x00000020) /* Falling trigger event configuration bit of line 5 */
#define EXTI_FTENR_TR6                          ((uint32_t)0x00000040) /* Falling trigger event configuration bit of line 6 */
#define EXTI_FTENR_TR7                          ((uint32_t)0x00000080) /* Falling trigger event configuration bit of line 7 */
#define EXTI_FTENR_TR8                          ((uint32_t)0x00000100) /* Falling trigger event configuration bit of line 8 */
#define EXTI_FTENR_TR9                          ((uint32_t)0x00000200) /* Falling trigger event configuration bit of line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_FTENR_TR10                         ((uint32_t)0x00000400) /* Falling trigger event configuration bit of line 10 */
#define EXTI_FTENR_TR11                         ((uint32_t)0x00000800) /* Falling trigger event configuration bit of line 11 */
#define EXTI_FTENR_TR12                         ((uint32_t)0x00001000) /* Falling trigger event configuration bit of line 12 */
#define EXTI_FTENR_TR13                         ((uint32_t)0x00002000) /* Falling trigger event configuration bit of line 13 */
#define EXTI_FTENR_TR14                         ((uint32_t)0x00004000) /* Falling trigger event configuration bit of line 14 */
#define EXTI_FTENR_TR15                         ((uint32_t)0x00008000) /* Falling trigger event configuration bit of line 15 */
#define EXTI_FTENR_TR16                         ((uint32_t)0x00010000) /* Falling trigger event configuration bit of line 16 */
#define EXTI_FTENR_TR17                         ((uint32_t)0x00020000) /* Falling trigger event configuration bit of line 17 */
#define EXTI_FTENR_TR18                         ((uint32_t)0x00040000) /* Falling trigger event configuration bit of line 18 */
#define EXTI_FTENR_TR19                         ((uint32_t)0x00080000) /* Falling trigger event configuration bit of line 19 */
#endif

/******************  Bit definition for EXTI_SWIEVR register  ******************/
#define EXTI_SWIEVR_SWIEVR0                     ((uint32_t)0x00000001) /* Software Interrupt on line 0 */
#define EXTI_SWIEVR_SWIEVR1                     ((uint32_t)0x00000002) /* Software Interrupt on line 1 */
#define EXTI_SWIEVR_SWIEVR2                     ((uint32_t)0x00000004) /* Software Interrupt on line 2 */
#define EXTI_SWIEVR_SWIEVR3                     ((uint32_t)0x00000008) /* Software Interrupt on line 3 */
#define EXTI_SWIEVR_SWIEVR4                     ((uint32_t)0x00000010) /* Software Interrupt on line 4 */
#define EXTI_SWIEVR_SWIEVR5                     ((uint32_t)0x00000020) /* Software Interrupt on line 5 */
#define EXTI_SWIEVR_SWIEVR6                     ((uint32_t)0x00000040) /* Software Interrupt on line 6 */
#define EXTI_SWIEVR_SWIEVR7                     ((uint32_t)0x00000080) /* Software Interrupt on line 7 */
#define EXTI_SWIEVR_SWIEVR8                     ((uint32_t)0x00000100) /* Software Interrupt on line 8 */
#define EXTI_SWIEVR_SWIEVR9                     ((uint32_t)0x00000200) /* Software Interrupt on line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_SWIEVR_SWIEVR10                    ((uint32_t)0x00000400) /* Software Interrupt on line 10 */
#define EXTI_SWIEVR_SWIEVR11                    ((uint32_t)0x00000800) /* Software Interrupt on line 11 */
#define EXTI_SWIEVR_SWIEVR12                    ((uint32_t)0x00001000) /* Software Interrupt on line 12 */
#define EXTI_SWIEVR_SWIEVR13                    ((uint32_t)0x00002000) /* Software Interrupt on line 13 */
#define EXTI_SWIEVR_SWIEVR14                    ((uint32_t)0x00004000) /* Software Interrupt on line 14 */
#define EXTI_SWIEVR_SWIEVR15                    ((uint32_t)0x00008000) /* Software Interrupt on line 15 */
#define EXTI_SWIEVR_SWIEVR16                    ((uint32_t)0x00010000) /* Software Interrupt on line 16 */
#define EXTI_SWIEVR_SWIEVR17                    ((uint32_t)0x00020000) /* Software Interrupt on line 17 */
#define EXTI_SWIEVR_SWIEVR18                    ((uint32_t)0x00040000) /* Software Interrupt on line 18 */
#define EXTI_SWIEVR_SWIEVR19                    ((uint32_t)0x00080000) /* Software Interrupt on line 19 */
#endif

/*******************  Bit definition for EXTI_INTFR register  ********************/
#define EXTI_INTF_INTF0                         ((uint32_t)0x00000001) /* Pending bit for line 0 */
#define EXTI_INTF_INTF1                         ((uint32_t)0x00000002) /* Pending bit for line 1 */
#define EXTI_INTF_INTF2                         ((uint32_t)0x00000004) /* Pending bit for line 2 */
#define EXTI_INTF_INTF3                         ((uint32_t)0x00000008) /* Pending bit for line 3 */
#define EXTI_INTF_INTF4                         ((uint32_t)0x00000010) /* Pending bit for line 4 */
#define EXTI_INTF_INTF5                         ((uint32_t)0x00000020) /* Pending bit for line 5 */
#define EXTI_INTF_INTF6                         ((uint32_t)0x00000040) /* Pending bit for line 6 */
#define EXTI_INTF_INTF7                         ((uint32_t)0x00000080) /* Pending bit for line 7 */
#define EXTI_INTF_INTF8                         ((uint32_t)0x00000100) /* Pending bit for line 8 */
#define EXTI_INTF_INTF9                         ((uint32_t)0x00000200) /* Pending bit for line 9 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_INTF_INTF10                        ((uint32_t)0x00000400) /* Pending bit for line 10 */
#define EXTI_INTF_INTF11                        ((uint32_t)0x00000800) /* Pending bit for line 11 */
#define EXTI_INTF_INTF12                        ((uint32_t)0x00001000) /* Pending bit for line 12 */
#define EXTI_INTF_INTF13                        ((uint32_t)0x00002000) /* Pending bit for line 13 */
#define EXTI_INTF_INTF14                        ((uint32_t)0x00004000) /* Pending bit for line 14 */
#define EXTI_INTF_INTF15                        ((uint32_t)0x00008000) /* Pending bit for line 15 */
#define EXTI_INTF_INTF16                        ((uint32_t)0x00010000) /* Pending bit for line 16 */
#define EXTI_INTF_INTF17                        ((uint32_t)0x00020000) /* Pending bit for line 17 */
#define EXTI_INTF_INTF18                        ((uint32_t)0x00040000) /* Pending bit for line 18 */
#define EXTI_INTF_INTF19                        ((uint32_t)0x00080000) /* Pending bit for line 19 */
#endif

/******************************************************************************/
/*                      FLASH and Option Bytes Registers                      */
/******************************************************************************/

#if defined(CH32V003) || defined(CH32V10x) || defined(CH32X03x)
/*******************  Bit definition for FLASH_ACTLR register  ******************/
#define FLASH_ACTLR_LATENCY                     ((uint8_t)0x03) /* LATENCY[2:0] bits (Latency) */
#define FLASH_ACTLR_LATENCY_0                   ((uint8_t)0x00) /* Bit 0 */
#define FLASH_ACTLR_LATENCY_1                   ((uint8_t)0x01) /* Bit 0 */
#define FLASH_ACTLR_LATENCY_2                   ((uint8_t)0x02) /* Bit 1 */
#endif

#if defined(CH32V10x)
#define FLASH_ACTLR_HLFCYA                      ((uint8_t)0x08) /* Flash Half Cycle Access Enable */
#define FLASH_ACTLR_PRFTBE                      ((uint8_t)0x10) /* Prefetch Buffer Enable */
#define FLASH_ACTLR_PRFTBS                      ((uint8_t)0x20) /* Prefetch Buffer Status */
#endif

/******************  Bit definition for FLASH_KEYR register  ******************/
#define FLASH_KEYR_FKEYR                        ((uint32_t)0xFFFFFFFF) /* FPEC Key */

/*****************  Bit definition for FLASH_OBKEYR register  ****************/
#define FLASH_OBKEYR_OBKEYR                     ((uint32_t)0xFFFFFFFF) /* Option Byte Key */

/******************  Bit definition for FLASH_STATR register  *******************/
#define FLASH_STATR_BSY                         ((uint8_t)0x01) /* Busy */
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define FLASH_STATR_PGERR                       ((uint8_t)0x04) /* Programming Error */
#endif
#define FLASH_STATR_WRPRTERR                    ((uint8_t)0x10) /* Write Protection Error */
#define FLASH_STATR_EOP                         ((uint8_t)0x20) /* End of operation */

/*******************  Bit definition for FLASH_CTLR register  *******************/
#define FLASH_CTLR_PG                           (0x0001)     /* Programming */
#define FLASH_CTLR_PER                          (0x0002)     /* Page Erase 1KByte*/
#define FLASH_CTLR_MER                          (0x0004)     /* Mass Erase */
#define FLASH_CTLR_OPTPG                        (0x0010)     /* Option Byte Programming */
#define FLASH_CTLR_OPTER                        (0x0020)     /* Option Byte Erase */
#define FLASH_CTLR_STRT                         (0x0040)     /* Start */
#define FLASH_CTLR_LOCK                         (0x0080)     /* Lock */
#define FLASH_CTLR_OPTWRE                       (0x0200)     /* Option Bytes Write Enable */
#define FLASH_CTLR_ERRIE                        (0x0400)     /* Error Interrupt Enable */
#define FLASH_CTLR_EOPIE                        (0x1000)     /* End of operation interrupt enable */
#if defined(CH32V20x) || defined(CH32V30x)
#define FLASH_CTLR_FAST_LOCK                    (0x00008000) /* Fast Lock */
#endif
#define FLASH_CTLR_PAGE_PG                      (0x00010000) /* Page Programming 64Byte */
#define FLASH_CTLR_PAGE_ER                      (0x00020000) /* Page Erase 64Byte */
#ifdef CH32V003
#define FLASH_CTLR_BUF_LOAD                     (0x00040000) /* Buffer Load */
#define FLASH_CTLR_BUF_RST                      (0x00080000) /* Buffer Reset */
#elif defined(CH32V20x) || defined(CH32V30x)
#define FLASH_CTLR_PAGE_BER32                   (0x00040000) /* Block Erase 32K */
#define FLASH_CTLR_PAGE_BER64                   (0x00080000) /* Block Erase 64K */
#define FLASH_CTLR_PG_STRT                      (0x00200000) /* Page Programming Start */
#endif

/*******************  Bit definition for FLASH_ADDR register  *******************/
#define FLASH_ADDR_FAR                          ((uint32_t)0xFFFFFFFF) /* Flash Address */

/******************  Bit definition for FLASH_OBR register  *******************/
#define FLASH_OBR_OPTERR                        ((uint16_t)0x0001) /* Option Byte Error */
#define FLASH_OBR_RDPRT                         ((uint16_t)0x0002) /* Read protection */

#define FLASH_OBR_USER                          ((uint16_t)0x03FC) /* User Option Bytes */
#define FLASH_OBR_WDG_SW                        ((uint16_t)0x0004) /* WDG_SW */
#define FLASH_OBR_nRST_STOP                     ((uint16_t)0x0008) /* nRST_STOP */
#define FLASH_OBR_nRST_STDBY                    ((uint16_t)0x0010) /* nRST_STDBY */
#define FLASH_OBR_RST_MODE                      ((uint16_t)0x0060) /* RST_MODE */

/******************  Bit definition for FLASH_WPR register  ******************/
#define FLASH_WPR_WRP                           ((uint32_t)0xFFFFFFFF) /* Write Protect */

/******************  Bit definition for FLASH_RDPR register  *******************/
#define FLASH_RDPR_RDPR                         ((uint32_t)0x000000FF) /* Read protection option byte */
#define FLASH_RDPR_nRDPR                        ((uint32_t)0x0000FF00) /* Read protection complemented option byte */

/******************  Bit definition for FLASH_USER register  ******************/
#define FLASH_USER_USER                         ((uint32_t)0x00FF0000) /* User option byte */
#define FLASH_USER_nUSER                        ((uint32_t)0xFF000000) /* User complemented option byte */

/******************  Bit definition for FLASH_Data0 register  *****************/
#define FLASH_Data0_Data0                       ((uint32_t)0x000000FF) /* User data storage option byte */
#define FLASH_Data0_nData0                      ((uint32_t)0x0000FF00) /* User data storage complemented option byte */

/******************  Bit definition for FLASH_Data1 register  *****************/
#define FLASH_Data1_Data1                       ((uint32_t)0x00FF0000) /* User data storage option byte */
#define FLASH_Data1_nData1                      ((uint32_t)0xFF000000) /* User data storage complemented option byte */

/******************  Bit definition for FLASH_WRPR0 register  ******************/
#define FLASH_WRPR0_WRPR0                       ((uint32_t)0x000000FF) /* Flash memory write protection option bytes */
#define FLASH_WRPR0_nWRPR0                      ((uint32_t)0x0000FF00) /* Flash memory write protection complemented option bytes */

/******************  Bit definition for FLASH_WRPR1 register  ******************/
#define FLASH_WRPR1_WRPR1                       ((uint32_t)0x00FF0000) /* Flash memory write protection option bytes */
#define FLASH_WRPR1_nWRPR1                      ((uint32_t)0xFF000000) /* Flash memory write protection complemented option bytes */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/******************  Bit definition for FLASH_WRPR2 register  ******************/
#define FLASH_WRPR2_WRPR2                       ((uint32_t)0x000000FF) /* Flash memory write protection option bytes */
#define FLASH_WRPR2_nWRPR2                      ((uint32_t)0x0000FF00) /* Flash memory write protection complemented option bytes */

/******************  Bit definition for FLASH_WRPR3 register  ******************/
#define FLASH_WRPR3_WRPR3                       ((uint32_t)0x00FF0000) /* Flash memory write protection option bytes */
#define FLASH_WRPR3_nWRPR3                      ((uint32_t)0xFF000000) /* Flash memory write protection complemented option bytes */
#endif

/******************************************************************************/
/*                General Purpose and Alternate Function I/O                  */
/******************************************************************************/

/*******************  Bit definition for GPIO_CFGLR register  *******************/
#define GPIO_CFGLR_MODE                         ((uint32_t)0x33333333) /* Port x mode bits */

#define GPIO_CFGLR_MODE0                        ((uint32_t)0x00000003) /* MODE0[1:0] bits (Port x mode bits, pin 0) */
#define GPIO_CFGLR_MODE0_0                      ((uint32_t)0x00000001) /* Bit 0 */
#define GPIO_CFGLR_MODE0_1                      ((uint32_t)0x00000002) /* Bit 1 */

#define GPIO_CFGLR_MODE1                        ((uint32_t)0x00000030) /* MODE1[1:0] bits (Port x mode bits, pin 1) */
#define GPIO_CFGLR_MODE1_0                      ((uint32_t)0x00000010) /* Bit 0 */
#define GPIO_CFGLR_MODE1_1                      ((uint32_t)0x00000020) /* Bit 1 */

#define GPIO_CFGLR_MODE2                        ((uint32_t)0x00000300) /* MODE2[1:0] bits (Port x mode bits, pin 2) */
#define GPIO_CFGLR_MODE2_0                      ((uint32_t)0x00000100) /* Bit 0 */
#define GPIO_CFGLR_MODE2_1                      ((uint32_t)0x00000200) /* Bit 1 */

#define GPIO_CFGLR_MODE3                        ((uint32_t)0x00003000) /* MODE3[1:0] bits (Port x mode bits, pin 3) */
#define GPIO_CFGLR_MODE3_0                      ((uint32_t)0x00001000) /* Bit 0 */
#define GPIO_CFGLR_MODE3_1                      ((uint32_t)0x00002000) /* Bit 1 */

#define GPIO_CFGLR_MODE4                        ((uint32_t)0x00030000) /* MODE4[1:0] bits (Port x mode bits, pin 4) */
#define GPIO_CFGLR_MODE4_0                      ((uint32_t)0x00010000) /* Bit 0 */
#define GPIO_CFGLR_MODE4_1                      ((uint32_t)0x00020000) /* Bit 1 */

#define GPIO_CFGLR_MODE5                        ((uint32_t)0x00300000) /* MODE5[1:0] bits (Port x mode bits, pin 5) */
#define GPIO_CFGLR_MODE5_0                      ((uint32_t)0x00100000) /* Bit 0 */
#define GPIO_CFGLR_MODE5_1                      ((uint32_t)0x00200000) /* Bit 1 */

#define GPIO_CFGLR_MODE6                        ((uint32_t)0x03000000) /* MODE6[1:0] bits (Port x mode bits, pin 6) */
#define GPIO_CFGLR_MODE6_0                      ((uint32_t)0x01000000) /* Bit 0 */
#define GPIO_CFGLR_MODE6_1                      ((uint32_t)0x02000000) /* Bit 1 */

#define GPIO_CFGLR_MODE7                        ((uint32_t)0x30000000) /* MODE7[1:0] bits (Port x mode bits, pin 7) */
#define GPIO_CFGLR_MODE7_0                      ((uint32_t)0x10000000) /* Bit 0 */
#define GPIO_CFGLR_MODE7_1                      ((uint32_t)0x20000000) /* Bit 1 */

#define GPIO_CFGLR_CNF                          ((uint32_t)0xCCCCCCCC) /* Port x configuration bits */

#define GPIO_CFGLR_CNF0                         ((uint32_t)0x0000000C) /* CNF0[1:0] bits (Port x configuration bits, pin 0) */
#define GPIO_CFGLR_CNF0_0                       ((uint32_t)0x00000004) /* Bit 0 */
#define GPIO_CFGLR_CNF0_1                       ((uint32_t)0x00000008) /* Bit 1 */

#define GPIO_CFGLR_CNF1                         ((uint32_t)0x000000C0) /* CNF1[1:0] bits (Port x configuration bits, pin 1) */
#define GPIO_CFGLR_CNF1_0                       ((uint32_t)0x00000040) /* Bit 0 */
#define GPIO_CFGLR_CNF1_1                       ((uint32_t)0x00000080) /* Bit 1 */

#define GPIO_CFGLR_CNF2                         ((uint32_t)0x00000C00) /* CNF2[1:0] bits (Port x configuration bits, pin 2) */
#define GPIO_CFGLR_CNF2_0                       ((uint32_t)0x00000400) /* Bit 0 */
#define GPIO_CFGLR_CNF2_1                       ((uint32_t)0x00000800) /* Bit 1 */

#define GPIO_CFGLR_CNF3                         ((uint32_t)0x0000C000) /* CNF3[1:0] bits (Port x configuration bits, pin 3) */
#define GPIO_CFGLR_CNF3_0                       ((uint32_t)0x00004000) /* Bit 0 */
#define GPIO_CFGLR_CNF3_1                       ((uint32_t)0x00008000) /* Bit 1 */

#define GPIO_CFGLR_CNF4                         ((uint32_t)0x000C0000) /* CNF4[1:0] bits (Port x configuration bits, pin 4) */
#define GPIO_CFGLR_CNF4_0                       ((uint32_t)0x00040000) /* Bit 0 */
#define GPIO_CFGLR_CNF4_1                       ((uint32_t)0x00080000) /* Bit 1 */

#define GPIO_CFGLR_CNF5                         ((uint32_t)0x00C00000) /* CNF5[1:0] bits (Port x configuration bits, pin 5) */
#define GPIO_CFGLR_CNF5_0                       ((uint32_t)0x00400000) /* Bit 0 */
#define GPIO_CFGLR_CNF5_1                       ((uint32_t)0x00800000) /* Bit 1 */

#define GPIO_CFGLR_CNF6                         ((uint32_t)0x0C000000) /* CNF6[1:0] bits (Port x configuration bits, pin 6) */
#define GPIO_CFGLR_CNF6_0                       ((uint32_t)0x04000000) /* Bit 0 */
#define GPIO_CFGLR_CNF6_1                       ((uint32_t)0x08000000) /* Bit 1 */

#define GPIO_CFGLR_CNF7                         ((uint32_t)0xC0000000) /* CNF7[1:0] bits (Port x configuration bits, pin 7) */
#define GPIO_CFGLR_CNF7_0                       ((uint32_t)0x40000000) /* Bit 0 */
#define GPIO_CFGLR_CNF7_1                       ((uint32_t)0x80000000) /* Bit 1 */

/*******************  Bit definition for GPIO_CFGHR register  *******************/
#define GPIO_CFGHR_MODE                         ((uint32_t)0x33333333) /* Port x mode bits */

#define GPIO_CFGHR_MODE8                        ((uint32_t)0x00000003) /* MODE8[1:0] bits (Port x mode bits, pin 8) */
#define GPIO_CFGHR_MODE8_0                      ((uint32_t)0x00000001) /* Bit 0 */
#define GPIO_CFGHR_MODE8_1                      ((uint32_t)0x00000002) /* Bit 1 */

#define GPIO_CFGHR_MODE9                        ((uint32_t)0x00000030) /* MODE9[1:0] bits (Port x mode bits, pin 9) */
#define GPIO_CFGHR_MODE9_0                      ((uint32_t)0x00000010) /* Bit 0 */
#define GPIO_CFGHR_MODE9_1                      ((uint32_t)0x00000020) /* Bit 1 */

#define GPIO_CFGHR_MODE10                       ((uint32_t)0x00000300) /* MODE10[1:0] bits (Port x mode bits, pin 10) */
#define GPIO_CFGHR_MODE10_0                     ((uint32_t)0x00000100) /* Bit 0 */
#define GPIO_CFGHR_MODE10_1                     ((uint32_t)0x00000200) /* Bit 1 */

#define GPIO_CFGHR_MODE11                       ((uint32_t)0x00003000) /* MODE11[1:0] bits (Port x mode bits, pin 11) */
#define GPIO_CFGHR_MODE11_0                     ((uint32_t)0x00001000) /* Bit 0 */
#define GPIO_CFGHR_MODE11_1                     ((uint32_t)0x00002000) /* Bit 1 */

#define GPIO_CFGHR_MODE12                       ((uint32_t)0x00030000) /* MODE12[1:0] bits (Port x mode bits, pin 12) */
#define GPIO_CFGHR_MODE12_0                     ((uint32_t)0x00010000) /* Bit 0 */
#define GPIO_CFGHR_MODE12_1                     ((uint32_t)0x00020000) /* Bit 1 */

#define GPIO_CFGHR_MODE13                       ((uint32_t)0x00300000) /* MODE13[1:0] bits (Port x mode bits, pin 13) */
#define GPIO_CFGHR_MODE13_0                     ((uint32_t)0x00100000) /* Bit 0 */
#define GPIO_CFGHR_MODE13_1                     ((uint32_t)0x00200000) /* Bit 1 */

#define GPIO_CFGHR_MODE14                       ((uint32_t)0x03000000) /* MODE14[1:0] bits (Port x mode bits, pin 14) */
#define GPIO_CFGHR_MODE14_0                     ((uint32_t)0x01000000) /* Bit 0 */
#define GPIO_CFGHR_MODE14_1                     ((uint32_t)0x02000000) /* Bit 1 */

#define GPIO_CFGHR_MODE15                       ((uint32_t)0x30000000) /* MODE15[1:0] bits (Port x mode bits, pin 15) */
#define GPIO_CFGHR_MODE15_0                     ((uint32_t)0x10000000) /* Bit 0 */
#define GPIO_CFGHR_MODE15_1                     ((uint32_t)0x20000000) /* Bit 1 */

#define GPIO_CFGHR_CNF                          ((uint32_t)0xCCCCCCCC) /* Port x configuration bits */

#define GPIO_CFGHR_CNF8                         ((uint32_t)0x0000000C) /* CNF8[1:0] bits (Port x configuration bits, pin 8) */
#define GPIO_CFGHR_CNF8_0                       ((uint32_t)0x00000004) /* Bit 0 */
#define GPIO_CFGHR_CNF8_1                       ((uint32_t)0x00000008) /* Bit 1 */

#define GPIO_CFGHR_CNF9                         ((uint32_t)0x000000C0) /* CNF9[1:0] bits (Port x configuration bits, pin 9) */
#define GPIO_CFGHR_CNF9_0                       ((uint32_t)0x00000040) /* Bit 0 */
#define GPIO_CFGHR_CNF9_1                       ((uint32_t)0x00000080) /* Bit 1 */

#define GPIO_CFGHR_CNF10                        ((uint32_t)0x00000C00) /* CNF10[1:0] bits (Port x configuration bits, pin 10) */
#define GPIO_CFGHR_CNF10_0                      ((uint32_t)0x00000400) /* Bit 0 */
#define GPIO_CFGHR_CNF10_1                      ((uint32_t)0x00000800) /* Bit 1 */

#define GPIO_CFGHR_CNF11                        ((uint32_t)0x0000C000) /* CNF11[1:0] bits (Port x configuration bits, pin 11) */
#define GPIO_CFGHR_CNF11_0                      ((uint32_t)0x00004000) /* Bit 0 */
#define GPIO_CFGHR_CNF11_1                      ((uint32_t)0x00008000) /* Bit 1 */

#define GPIO_CFGHR_CNF12                        ((uint32_t)0x000C0000) /* CNF12[1:0] bits (Port x configuration bits, pin 12) */
#define GPIO_CFGHR_CNF12_0                      ((uint32_t)0x00040000) /* Bit 0 */
#define GPIO_CFGHR_CNF12_1                      ((uint32_t)0x00080000) /* Bit 1 */

#define GPIO_CFGHR_CNF13                        ((uint32_t)0x00C00000) /* CNF13[1:0] bits (Port x configuration bits, pin 13) */
#define GPIO_CFGHR_CNF13_0                      ((uint32_t)0x00400000) /* Bit 0 */
#define GPIO_CFGHR_CNF13_1                      ((uint32_t)0x00800000) /* Bit 1 */

#define GPIO_CFGHR_CNF14                        ((uint32_t)0x0C000000) /* CNF14[1:0] bits (Port x configuration bits, pin 14) */
#define GPIO_CFGHR_CNF14_0                      ((uint32_t)0x04000000) /* Bit 0 */
#define GPIO_CFGHR_CNF14_1                      ((uint32_t)0x08000000) /* Bit 1 */

#define GPIO_CFGHR_CNF15                        ((uint32_t)0xC0000000) /* CNF15[1:0] bits (Port x configuration bits, pin 15) */
#define GPIO_CFGHR_CNF15_0                      ((uint32_t)0x40000000) /* Bit 0 */
#define GPIO_CFGHR_CNF15_1                      ((uint32_t)0x80000000) /* Bit 1 */

/*******************  Bit definition for GPIO_INDR register  *******************/
#define GPIO_INDR_IDR0                          ((uint16_t)0x0001) /* Port input data, bit 0 */
#define GPIO_INDR_IDR1                          ((uint16_t)0x0002) /* Port input data, bit 1 */
#define GPIO_INDR_IDR2                          ((uint16_t)0x0004) /* Port input data, bit 2 */
#define GPIO_INDR_IDR3                          ((uint16_t)0x0008) /* Port input data, bit 3 */
#define GPIO_INDR_IDR4                          ((uint16_t)0x0010) /* Port input data, bit 4 */
#define GPIO_INDR_IDR5                          ((uint16_t)0x0020) /* Port input data, bit 5 */
#define GPIO_INDR_IDR6                          ((uint16_t)0x0040) /* Port input data, bit 6 */
#define GPIO_INDR_IDR7                          ((uint16_t)0x0080) /* Port input data, bit 7 */
#define GPIO_INDR_IDR8                          ((uint16_t)0x0100) /* Port input data, bit 8 */
#define GPIO_INDR_IDR9                          ((uint16_t)0x0200) /* Port input data, bit 9 */
#define GPIO_INDR_IDR10                         ((uint16_t)0x0400) /* Port input data, bit 10 */
#define GPIO_INDR_IDR11                         ((uint16_t)0x0800) /* Port input data, bit 11 */
#define GPIO_INDR_IDR12                         ((uint16_t)0x1000) /* Port input data, bit 12 */
#define GPIO_INDR_IDR13                         ((uint16_t)0x2000) /* Port input data, bit 13 */
#define GPIO_INDR_IDR14                         ((uint16_t)0x4000) /* Port input data, bit 14 */
#define GPIO_INDR_IDR15                         ((uint16_t)0x8000) /* Port input data, bit 15 */

/*******************  Bit definition for GPIO_OUTDR register  *******************/
#define GPIO_OUTDR_ODR0                         ((uint16_t)0x0001) /* Port output data, bit 0 */
#define GPIO_OUTDR_ODR1                         ((uint16_t)0x0002) /* Port output data, bit 1 */
#define GPIO_OUTDR_ODR2                         ((uint16_t)0x0004) /* Port output data, bit 2 */
#define GPIO_OUTDR_ODR3                         ((uint16_t)0x0008) /* Port output data, bit 3 */
#define GPIO_OUTDR_ODR4                         ((uint16_t)0x0010) /* Port output data, bit 4 */
#define GPIO_OUTDR_ODR5                         ((uint16_t)0x0020) /* Port output data, bit 5 */
#define GPIO_OUTDR_ODR6                         ((uint16_t)0x0040) /* Port output data, bit 6 */
#define GPIO_OUTDR_ODR7                         ((uint16_t)0x0080) /* Port output data, bit 7 */
#define GPIO_OUTDR_ODR8                         ((uint16_t)0x0100) /* Port output data, bit 8 */
#define GPIO_OUTDR_ODR9                         ((uint16_t)0x0200) /* Port output data, bit 9 */
#define GPIO_OUTDR_ODR10                        ((uint16_t)0x0400) /* Port output data, bit 10 */
#define GPIO_OUTDR_ODR11                        ((uint16_t)0x0800) /* Port output data, bit 11 */
#define GPIO_OUTDR_ODR12                        ((uint16_t)0x1000) /* Port output data, bit 12 */
#define GPIO_OUTDR_ODR13                        ((uint16_t)0x2000) /* Port output data, bit 13 */
#define GPIO_OUTDR_ODR14                        ((uint16_t)0x4000) /* Port output data, bit 14 */
#define GPIO_OUTDR_ODR15                        ((uint16_t)0x8000) /* Port output data, bit 15 */

/******************  Bit definition for GPIO_BSHR register  *******************/
#define GPIO_BSHR_BS0                           ((uint32_t)0x00000001) /* Port x Set bit 0 */
#define GPIO_BSHR_BS1                           ((uint32_t)0x00000002) /* Port x Set bit 1 */
#define GPIO_BSHR_BS2                           ((uint32_t)0x00000004) /* Port x Set bit 2 */
#define GPIO_BSHR_BS3                           ((uint32_t)0x00000008) /* Port x Set bit 3 */
#define GPIO_BSHR_BS4                           ((uint32_t)0x00000010) /* Port x Set bit 4 */
#define GPIO_BSHR_BS5                           ((uint32_t)0x00000020) /* Port x Set bit 5 */
#define GPIO_BSHR_BS6                           ((uint32_t)0x00000040) /* Port x Set bit 6 */
#define GPIO_BSHR_BS7                           ((uint32_t)0x00000080) /* Port x Set bit 7 */
#define GPIO_BSHR_BS8                           ((uint32_t)0x00000100) /* Port x Set bit 8 */
#define GPIO_BSHR_BS9                           ((uint32_t)0x00000200) /* Port x Set bit 9 */
#define GPIO_BSHR_BS10                          ((uint32_t)0x00000400) /* Port x Set bit 10 */
#define GPIO_BSHR_BS11                          ((uint32_t)0x00000800) /* Port x Set bit 11 */
#define GPIO_BSHR_BS12                          ((uint32_t)0x00001000) /* Port x Set bit 12 */
#define GPIO_BSHR_BS13                          ((uint32_t)0x00002000) /* Port x Set bit 13 */
#define GPIO_BSHR_BS14                          ((uint32_t)0x00004000) /* Port x Set bit 14 */
#define GPIO_BSHR_BS15                          ((uint32_t)0x00008000) /* Port x Set bit 15 */

#define GPIO_BSHR_BR0                           ((uint32_t)0x00010000) /* Port x Reset bit 0 */
#define GPIO_BSHR_BR1                           ((uint32_t)0x00020000) /* Port x Reset bit 1 */
#define GPIO_BSHR_BR2                           ((uint32_t)0x00040000) /* Port x Reset bit 2 */
#define GPIO_BSHR_BR3                           ((uint32_t)0x00080000) /* Port x Reset bit 3 */
#define GPIO_BSHR_BR4                           ((uint32_t)0x00100000) /* Port x Reset bit 4 */
#define GPIO_BSHR_BR5                           ((uint32_t)0x00200000) /* Port x Reset bit 5 */
#define GPIO_BSHR_BR6                           ((uint32_t)0x00400000) /* Port x Reset bit 6 */
#define GPIO_BSHR_BR7                           ((uint32_t)0x00800000) /* Port x Reset bit 7 */
#define GPIO_BSHR_BR8                           ((uint32_t)0x01000000) /* Port x Reset bit 8 */
#define GPIO_BSHR_BR9                           ((uint32_t)0x02000000) /* Port x Reset bit 9 */
#define GPIO_BSHR_BR10                          ((uint32_t)0x04000000) /* Port x Reset bit 10 */
#define GPIO_BSHR_BR11                          ((uint32_t)0x08000000) /* Port x Reset bit 11 */
#define GPIO_BSHR_BR12                          ((uint32_t)0x10000000) /* Port x Reset bit 12 */
#define GPIO_BSHR_BR13                          ((uint32_t)0x20000000) /* Port x Reset bit 13 */
#define GPIO_BSHR_BR14                          ((uint32_t)0x40000000) /* Port x Reset bit 14 */
#define GPIO_BSHR_BR15                          ((uint32_t)0x80000000) /* Port x Reset bit 15 */

/*******************  Bit definition for GPIO_BCR register  *******************/
#define GPIO_BCR_BR0                            ((uint16_t)0x0001) /* Port x Reset bit 0 */
#define GPIO_BCR_BR1                            ((uint16_t)0x0002) /* Port x Reset bit 1 */
#define GPIO_BCR_BR2                            ((uint16_t)0x0004) /* Port x Reset bit 2 */
#define GPIO_BCR_BR3                            ((uint16_t)0x0008) /* Port x Reset bit 3 */
#define GPIO_BCR_BR4                            ((uint16_t)0x0010) /* Port x Reset bit 4 */
#define GPIO_BCR_BR5                            ((uint16_t)0x0020) /* Port x Reset bit 5 */
#define GPIO_BCR_BR6                            ((uint16_t)0x0040) /* Port x Reset bit 6 */
#define GPIO_BCR_BR7                            ((uint16_t)0x0080) /* Port x Reset bit 7 */
#define GPIO_BCR_BR8                            ((uint16_t)0x0100) /* Port x Reset bit 8 */
#define GPIO_BCR_BR9                            ((uint16_t)0x0200) /* Port x Reset bit 9 */
#define GPIO_BCR_BR10                           ((uint16_t)0x0400) /* Port x Reset bit 10 */
#define GPIO_BCR_BR11                           ((uint16_t)0x0800) /* Port x Reset bit 11 */
#define GPIO_BCR_BR12                           ((uint16_t)0x1000) /* Port x Reset bit 12 */
#define GPIO_BCR_BR13                           ((uint16_t)0x2000) /* Port x Reset bit 13 */
#define GPIO_BCR_BR14                           ((uint16_t)0x4000) /* Port x Reset bit 14 */
#define GPIO_BCR_BR15                           ((uint16_t)0x8000) /* Port x Reset bit 15 */

/******************  Bit definition for GPIO_LCKR register  *******************/
#define GPIO_LCK0                               ((uint32_t)0x00000001) /* Port x Lock bit 0 */
#define GPIO_LCK1                               ((uint32_t)0x00000002) /* Port x Lock bit 1 */
#define GPIO_LCK2                               ((uint32_t)0x00000004) /* Port x Lock bit 2 */
#define GPIO_LCK3                               ((uint32_t)0x00000008) /* Port x Lock bit 3 */
#define GPIO_LCK4                               ((uint32_t)0x00000010) /* Port x Lock bit 4 */
#define GPIO_LCK5                               ((uint32_t)0x00000020) /* Port x Lock bit 5 */
#define GPIO_LCK6                               ((uint32_t)0x00000040) /* Port x Lock bit 6 */
#define GPIO_LCK7                               ((uint32_t)0x00000080) /* Port x Lock bit 7 */
#define GPIO_LCK8                               ((uint32_t)0x00000100) /* Port x Lock bit 8 */
#define GPIO_LCK9                               ((uint32_t)0x00000200) /* Port x Lock bit 9 */
#define GPIO_LCK10                              ((uint32_t)0x00000400) /* Port x Lock bit 10 */
#define GPIO_LCK11                              ((uint32_t)0x00000800) /* Port x Lock bit 11 */
#define GPIO_LCK12                              ((uint32_t)0x00001000) /* Port x Lock bit 12 */
#define GPIO_LCK13                              ((uint32_t)0x00002000) /* Port x Lock bit 13 */
#define GPIO_LCK14                              ((uint32_t)0x00004000) /* Port x Lock bit 14 */
#define GPIO_LCK15                              ((uint32_t)0x00008000) /* Port x Lock bit 15 */
#define GPIO_LCKK                               ((uint32_t)0x00010000) /* Lock key */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/******************  Bit definition for AFIO_ECR register  *******************/
#define AFIO_ECR_PIN                            ((uint8_t)0x0F) /* PIN[3:0] bits (Pin selection) */
#define AFIO_ECR_PIN_0                          ((uint8_t)0x01) /* Bit 0 */
#define AFIO_ECR_PIN_1                          ((uint8_t)0x02) /* Bit 1 */
#define AFIO_ECR_PIN_2                          ((uint8_t)0x04) /* Bit 2 */
#define AFIO_ECR_PIN_3                          ((uint8_t)0x08) /* Bit 3 */

#define AFIO_ECR_PIN_PX0                        ((uint8_t)0x00) /* Pin 0 selected */
#define AFIO_ECR_PIN_PX1                        ((uint8_t)0x01) /* Pin 1 selected */
#define AFIO_ECR_PIN_PX2                        ((uint8_t)0x02) /* Pin 2 selected */
#define AFIO_ECR_PIN_PX3                        ((uint8_t)0x03) /* Pin 3 selected */
#define AFIO_ECR_PIN_PX4                        ((uint8_t)0x04) /* Pin 4 selected */
#define AFIO_ECR_PIN_PX5                        ((uint8_t)0x05) /* Pin 5 selected */
#define AFIO_ECR_PIN_PX6                        ((uint8_t)0x06) /* Pin 6 selected */
#define AFIO_ECR_PIN_PX7                        ((uint8_t)0x07) /* Pin 7 selected */
#define AFIO_ECR_PIN_PX8                        ((uint8_t)0x08) /* Pin 8 selected */
#define AFIO_ECR_PIN_PX9                        ((uint8_t)0x09) /* Pin 9 selected */
#define AFIO_ECR_PIN_PX10                       ((uint8_t)0x0A) /* Pin 10 selected */
#define AFIO_ECR_PIN_PX11                       ((uint8_t)0x0B) /* Pin 11 selected */
#define AFIO_ECR_PIN_PX12                       ((uint8_t)0x0C) /* Pin 12 selected */
#define AFIO_ECR_PIN_PX13                       ((uint8_t)0x0D) /* Pin 13 selected */
#define AFIO_ECR_PIN_PX14                       ((uint8_t)0x0E) /* Pin 14 selected */
#define AFIO_ECR_PIN_PX15                       ((uint8_t)0x0F) /* Pin 15 selected */

#define AFIO_ECR_PORT                           ((uint8_t)0x70) /* PORT[2:0] bits (Port selection) */
#define AFIO_ECR_PORT_0                         ((uint8_t)0x10) /* Bit 0 */
#define AFIO_ECR_PORT_1                         ((uint8_t)0x20) /* Bit 1 */
#define AFIO_ECR_PORT_2                         ((uint8_t)0x40) /* Bit 2 */

#define AFIO_ECR_PORT_PA                        ((uint8_t)0x00) /* Port A selected */
#define AFIO_ECR_PORT_PB                        ((uint8_t)0x10) /* Port B selected */
#define AFIO_ECR_PORT_PC                        ((uint8_t)0x20) /* Port C selected */
#define AFIO_ECR_PORT_PD                        ((uint8_t)0x30) /* Port D selected */
#define AFIO_ECR_PORT_PE                        ((uint8_t)0x40) /* Port E selected */

#define AFIO_ECR_EVOE                           ((uint8_t)0x80) /* Event Output Enable */
#endif

/******************  Bit definition for AFIO_PCFR1register  *******************/
#define AFIO_PCFR1_SPI1_REMAP                   ((uint32_t)0x00000001) /* SPI1 remapping */
#define AFIO_PCFR1_I2C1_REMAP                   ((uint32_t)0x00000002) /* I2C1 remapping */
#define AFIO_PCFR1_USART1_REMAP                 ((uint32_t)0x00000004) /* USART1 remapping */
#define AFIO_PCFR1_USART2_REMAP                 ((uint32_t)0x00000008) /* USART2 remapping */

#define AFIO_PCFR1_USART3_REMAP                 ((uint32_t)0x00000030) /* USART3_REMAP[1:0] bits (USART3 remapping) */
#define AFIO_PCFR1_USART3_REMAP_0               ((uint32_t)0x00000010) /* Bit 0 */
#define AFIO_PCFR1_USART3_REMAP_1               ((uint32_t)0x00000020) /* Bit 1 */

#define AFIO_PCFR1_USART3_REMAP_NOREMAP         ((uint32_t)0x00000000) /* No remap (TX/PB10, RX/PB11, CK/PB12, CTS/PB13, RTS/PB14) */
#define AFIO_PCFR1_USART3_REMAP_PARTIALREMAP    ((uint32_t)0x00000010) /* Partial remap (TX/PC10, RX/PC11, CK/PC12, CTS/PB13, RTS/PB14) */
#define AFIO_PCFR1_USART3_REMAP_FULLREMAP       ((uint32_t)0x00000030) /* Full remap (TX/PD8, RX/PD9, CK/PD10, CTS/PD11, RTS/PD12) */

#define AFIO_PCFR1_TIM1_REMAP                   ((uint32_t)0x000000C0) /* TIM1_REMAP[1:0] bits (TIM1 remapping) */
#define AFIO_PCFR1_TIM1_REMAP_0                 ((uint32_t)0x00000040) /* Bit 0 */
#define AFIO_PCFR1_TIM1_REMAP_1                 ((uint32_t)0x00000080) /* Bit 1 */

#define AFIO_PCFR1_TIM1_REMAP_NOREMAP           ((uint32_t)0x00000000) /* No remap (ETR/PA12, CH1/PA8, CH2/PA9, CH3/PA10, CH4/PA11, BKIN/PB12, CH1N/PB13, CH2N/PB14, CH3N/PB15) */
#define AFIO_PCFR1_TIM1_REMAP_PARTIALREMAP      ((uint32_t)0x00000040) /* Partial remap (ETR/PA12, CH1/PA8, CH2/PA9, CH3/PA10, CH4/PA11, BKIN/PA6, CH1N/PA7, CH2N/PB0, CH3N/PB1) */
#define AFIO_PCFR1_TIM1_REMAP_FULLREMAP         ((uint32_t)0x000000C0) /* Full remap (ETR/PE7, CH1/PE9, CH2/PE11, CH3/PE13, CH4/PE14, BKIN/PE15, CH1N/PE8, CH2N/PE10, CH3N/PE12) */

#define AFIO_PCFR1_TIM2_REMAP                   ((uint32_t)0x00000300) /* TIM2_REMAP[1:0] bits (TIM2 remapping) */
#define AFIO_PCFR1_TIM2_REMAP_0                 ((uint32_t)0x00000100) /* Bit 0 */
#define AFIO_PCFR1_TIM2_REMAP_1                 ((uint32_t)0x00000200) /* Bit 1 */

#define AFIO_PCFR1_TIM2_REMAP_NOREMAP           ((uint32_t)0x00000000) /* No remap (CH1/ETR/PA0, CH2/PA1, CH3/PA2, CH4/PA3) */
#define AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1     ((uint32_t)0x00000100) /* Partial remap (CH1/ETR/PA15, CH2/PB3, CH3/PA2, CH4/PA3) */
#define AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP2     ((uint32_t)0x00000200) /* Partial remap (CH1/ETR/PA0, CH2/PA1, CH3/PB10, CH4/PB11) */
#define AFIO_PCFR1_TIM2_REMAP_FULLREMAP         ((uint32_t)0x00000300) /* Full remap (CH1/ETR/PA15, CH2/PB3, CH3/PB10, CH4/PB11) */

#define AFIO_PCFR1_TIM3_REMAP                   ((uint32_t)0x00000C00) /* TIM3_REMAP[1:0] bits (TIM3 remapping) */
#define AFIO_PCFR1_TIM3_REMAP_0                 ((uint32_t)0x00000400) /* Bit 0 */
#define AFIO_PCFR1_TIM3_REMAP_1                 ((uint32_t)0x00000800) /* Bit 1 */

#define AFIO_PCFR1_TIM3_REMAP_NOREMAP           ((uint32_t)0x00000000) /* No remap (CH1/PA6, CH2/PA7, CH3/PB0, CH4/PB1) */
#define AFIO_PCFR1_TIM3_REMAP_PARTIALREMAP      ((uint32_t)0x00000800) /* Partial remap (CH1/PB4, CH2/PB5, CH3/PB0, CH4/PB1) */
#define AFIO_PCFR1_TIM3_REMAP_FULLREMAP         ((uint32_t)0x00000C00) /* Full remap (CH1/PC6, CH2/PC7, CH3/PC8, CH4/PC9) */

#define AFIO_PCFR1_TIM4_REMAP                   ((uint32_t)0x00001000) /* TIM4_REMAP bit (TIM4 remapping) */

#define AFIO_PCFR1_CAN_REMAP                    ((uint32_t)0x00006000) /* CAN_REMAP[1:0] bits (CAN Alternate function remapping) */
#define AFIO_PCFR1_CAN_REMAP_0                  ((uint32_t)0x00002000) /* Bit 0 */
#define AFIO_PCFR1_CAN_REMAP_1                  ((uint32_t)0x00004000) /* Bit 1 */

#define AFIO_PCFR1_CAN_REMAP_REMAP1             ((uint32_t)0x00000000) /* CANRX mapped to PA11, CANTX mapped to PA12 */
#define AFIO_PCFR1_CAN_REMAP_REMAP2             ((uint32_t)0x00004000) /* CANRX mapped to PB8, CANTX mapped to PB9 */
#define AFIO_PCFR1_CAN_REMAP_REMAP3             ((uint32_t)0x00006000) /* CANRX mapped to PD0, CANTX mapped to PD1 */

#ifdef CH32V003
#define AFIO_PCFR1_PA12_REMAP                   ((uint32_t)0x00008000) /* Port D0/Port D1 mapping on OSC_IN/OSC_OUT */
#elif defined(CH32V20x) || defined(CH32V30x)
#define AFIO_PCFR1_PD01_REMAP                   ((uint32_t)0x00008000) /* Port D0/Port D1 mapping on OSC_IN/OSC_OUT */
#endif
#define AFIO_PCFR1_TIM5CH4_IREMAP               ((uint32_t)0x00010000) /* TIM5 Channel4 Internal Remap */
#define AFIO_PCFR1_ADC1_ETRGINJ_REMAP           ((uint32_t)0x00020000) /* ADC 1 External Trigger Injected Conversion remapping */
#define AFIO_PCFR1_ADC1_ETRGREG_REMAP           ((uint32_t)0x00040000) /* ADC 1 External Trigger Regular Conversion remapping */
#define AFIO_PCFR1_ADC2_ETRGINJ_REMAP           ((uint32_t)0x00080000) /* ADC 2 External Trigger Injected Conversion remapping */
#define AFIO_PCFR1_ADC2_ETRGREG_REMAP           ((uint32_t)0x00100000) /* ADC 2 External Trigger Regular Conversion remapping */

#define AFIO_PCFR1_SWJ_CFG                      ((uint32_t)0x07000000) /* SWJ_CFG[2:0] bits (Serial Wire JTAG configuration) */
#define AFIO_PCFR1_SWJ_CFG_0                    ((uint32_t)0x01000000) /* Bit 0 */
#define AFIO_PCFR1_SWJ_CFG_1                    ((uint32_t)0x02000000) /* Bit 1 */
#define AFIO_PCFR1_SWJ_CFG_2                    ((uint32_t)0x04000000) /* Bit 2 */

#define AFIO_PCFR1_SWJ_CFG_RESET                ((uint32_t)0x00000000) /* Full SWJ (JTAG-DP + SW-DP) : Reset State */
#define AFIO_PCFR1_SWJ_CFG_NOJNTRST             ((uint32_t)0x01000000) /* Full SWJ (JTAG-DP + SW-DP) but without JNTRST */
#define AFIO_PCFR1_SWJ_CFG_JTAGDISABLE          ((uint32_t)0x02000000) /* JTAG-DP Disabled and SW-DP Enabled */
#define AFIO_PCFR1_SWJ_CFG_DISABLE              ((uint32_t)0x04000000) /* JTAG-DP Disabled and SW-DP Disabled */


#if defined(CH32V003)
/*****************  Bit definition for AFIO_EXTICR register  *****************/
#define AFIO_EXTICR_EXTI0                       ((uint16_t)0x0003) /* EXTI 0 configuration */
#define AFIO_EXTICR_EXTI1                       ((uint16_t)0x000C) /* EXTI 1 configuration */
#define AFIO_EXTICR_EXTI2                       ((uint16_t)0x0030) /* EXTI 2 configuration */
#define AFIO_EXTICR_EXTI3                       ((uint16_t)0x00C0) /* EXTI 3 configuration */
#define AFIO_EXTICR_EXTI4                       ((uint16_t)0x0300) /* EXTI 4 configuration */
#define AFIO_EXTICR_EXTI5                       ((uint16_t)0x0C00) /* EXTI 5 configuration */
#define AFIO_EXTICR_EXTI6                       ((uint16_t)0x3000) /* EXTI 6 configuration */
#define AFIO_EXTICR_EXTI7                       ((uint16_t)0xC000) /* EXTI 7 configuration */

#define AFIO_EXTICR_EXTI0_PC                    ((uint16_t)0x0002) /* PC[0] pin */
#define AFIO_EXTICR_EXTI0_PD                    ((uint16_t)0x0003) /* PD[0] pin */
#define AFIO_EXTICR_EXTI1_PA                    ((uint16_t)0x0000) /* PA[1] pin */
#define AFIO_EXTICR_EXTI1_PC                    ((uint16_t)0x0008) /* PC[1] pin */
#define AFIO_EXTICR_EXTI1_PD                    ((uint16_t)0x000C) /* PD[1] pin */
#define AFIO_EXTICR_EXTI2_PA                    ((uint16_t)0x0000) /* PA[2] pin */
#define AFIO_EXTICR_EXTI2_PC                    ((uint16_t)0x0020) /* PC[2] pin */
#define AFIO_EXTICR_EXTI2_PD                    ((uint16_t)0x0030) /* PD[2] pin */
#define AFIO_EXTICR_EXTI3_PC                    ((uint16_t)0x0080) /* PC[3] pin */
#define AFIO_EXTICR_EXTI3_PD                    ((uint16_t)0x00C0) /* PD[3] pin */
#define AFIO_EXTICR_EXTI4_PC                    ((uint16_t)0x0200) /* PC[4] pin */
#define AFIO_EXTICR_EXTI4_PD                    ((uint16_t)0x0300) /* PD[4] pin */
#define AFIO_EXTICR_EXTI5_PC                    ((uint16_t)0x0800) /* PC[5] pin */
#define AFIO_EXTICR_EXTI5_PD                    ((uint16_t)0x0C00) /* PD[5] pin */
#define AFIO_EXTICR_EXTI6_PC                    ((uint16_t)0x2000) /* PC[6] pin */
#define AFIO_EXTICR_EXTI6_PD                    ((uint16_t)0x3000) /* PD[6] pin */
#define AFIO_EXTICR_EXTI7_PC                    ((uint16_t)0x8000) /* PC[7] pin */
#define AFIO_EXTICR_EXTI7_PD                    ((uint16_t)0xC000) /* PD[7] pin */
#endif

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/*****************  Bit definition for AFIO_EXTICR1 register  *****************/
#define AFIO_EXTICR1_EXTI0                      ((uint16_t)0x000F) /* EXTI 0 configuration */
#define AFIO_EXTICR1_EXTI1                      ((uint16_t)0x00F0) /* EXTI 1 configuration */
#define AFIO_EXTICR1_EXTI2                      ((uint16_t)0x0F00) /* EXTI 2 configuration */
#define AFIO_EXTICR1_EXTI3                      ((uint16_t)0xF000) /* EXTI 3 configuration */

#define AFIO_EXTICR1_EXTI0_PA                   ((uint16_t)0x0000) /* PA[0] pin */
#define AFIO_EXTICR1_EXTI0_PB                   ((uint16_t)0x0001) /* PB[0] pin */
#define AFIO_EXTICR1_EXTI0_PC                   ((uint16_t)0x0002) /* PC[0] pin */
#define AFIO_EXTICR1_EXTI0_PD                   ((uint16_t)0x0003) /* PD[0] pin */
#define AFIO_EXTICR1_EXTI0_PE                   ((uint16_t)0x0004) /* PE[0] pin */
#define AFIO_EXTICR1_EXTI0_PF                   ((uint16_t)0x0005) /* PF[0] pin */
#define AFIO_EXTICR1_EXTI0_PG                   ((uint16_t)0x0006) /* PG[0] pin */

#define AFIO_EXTICR1_EXTI1_PA                   ((uint16_t)0x0000) /* PA[1] pin */
#define AFIO_EXTICR1_EXTI1_PB                   ((uint16_t)0x0010) /* PB[1] pin */
#define AFIO_EXTICR1_EXTI1_PC                   ((uint16_t)0x0020) /* PC[1] pin */
#define AFIO_EXTICR1_EXTI1_PD                   ((uint16_t)0x0030) /* PD[1] pin */
#define AFIO_EXTICR1_EXTI1_PE                   ((uint16_t)0x0040) /* PE[1] pin */
#define AFIO_EXTICR1_EXTI1_PF                   ((uint16_t)0x0050) /* PF[1] pin */
#define AFIO_EXTICR1_EXTI1_PG                   ((uint16_t)0x0060) /* PG[1] pin */

#define AFIO_EXTICR1_EXTI2_PA                   ((uint16_t)0x0000) /* PA[2] pin */
#define AFIO_EXTICR1_EXTI2_PB                   ((uint16_t)0x0100) /* PB[2] pin */
#define AFIO_EXTICR1_EXTI2_PC                   ((uint16_t)0x0200) /* PC[2] pin */
#define AFIO_EXTICR1_EXTI2_PD                   ((uint16_t)0x0300) /* PD[2] pin */
#define AFIO_EXTICR1_EXTI2_PE                   ((uint16_t)0x0400) /* PE[2] pin */
#define AFIO_EXTICR1_EXTI2_PF                   ((uint16_t)0x0500) /* PF[2] pin */
#define AFIO_EXTICR1_EXTI2_PG                   ((uint16_t)0x0600) /* PG[2] pin */

#define AFIO_EXTICR1_EXTI3_PA                   ((uint16_t)0x0000) /* PA[3] pin */
#define AFIO_EXTICR1_EXTI3_PB                   ((uint16_t)0x1000) /* PB[3] pin */
#define AFIO_EXTICR1_EXTI3_PC                   ((uint16_t)0x2000) /* PC[3] pin */
#define AFIO_EXTICR1_EXTI3_PD                   ((uint16_t)0x3000) /* PD[3] pin */
#define AFIO_EXTICR1_EXTI3_PE                   ((uint16_t)0x4000) /* PE[3] pin */
#define AFIO_EXTICR1_EXTI3_PF                   ((uint16_t)0x5000) /* PF[3] pin */
#define AFIO_EXTICR1_EXTI3_PG                   ((uint16_t)0x6000) /* PG[3] pin */

/*****************  Bit definition for AFIO_EXTICR2 register  *****************/
#define AFIO_EXTICR2_EXTI4                      ((uint16_t)0x000F) /* EXTI 4 configuration */
#define AFIO_EXTICR2_EXTI5                      ((uint16_t)0x00F0) /* EXTI 5 configuration */
#define AFIO_EXTICR2_EXTI6                      ((uint16_t)0x0F00) /* EXTI 6 configuration */
#define AFIO_EXTICR2_EXTI7                      ((uint16_t)0xF000) /* EXTI 7 configuration */

#define AFIO_EXTICR2_EXTI4_PA                   ((uint16_t)0x0000) /* PA[4] pin */
#define AFIO_EXTICR2_EXTI4_PB                   ((uint16_t)0x0001) /* PB[4] pin */
#define AFIO_EXTICR2_EXTI4_PC                   ((uint16_t)0x0002) /* PC[4] pin */
#define AFIO_EXTICR2_EXTI4_PD                   ((uint16_t)0x0003) /* PD[4] pin */
#define AFIO_EXTICR2_EXTI4_PE                   ((uint16_t)0x0004) /* PE[4] pin */
#define AFIO_EXTICR2_EXTI4_PF                   ((uint16_t)0x0005) /* PF[4] pin */
#define AFIO_EXTICR2_EXTI4_PG                   ((uint16_t)0x0006) /* PG[4] pin */

#define AFIO_EXTICR2_EXTI5_PA                   ((uint16_t)0x0000) /* PA[5] pin */
#define AFIO_EXTICR2_EXTI5_PB                   ((uint16_t)0x0010) /* PB[5] pin */
#define AFIO_EXTICR2_EXTI5_PC                   ((uint16_t)0x0020) /* PC[5] pin */
#define AFIO_EXTICR2_EXTI5_PD                   ((uint16_t)0x0030) /* PD[5] pin */
#define AFIO_EXTICR2_EXTI5_PE                   ((uint16_t)0x0040) /* PE[5] pin */
#define AFIO_EXTICR2_EXTI5_PF                   ((uint16_t)0x0050) /* PF[5] pin */
#define AFIO_EXTICR2_EXTI5_PG                   ((uint16_t)0x0060) /* PG[5] pin */

#define AFIO_EXTICR2_EXTI6_PA                   ((uint16_t)0x0000) /* PA[6] pin */
#define AFIO_EXTICR2_EXTI6_PB                   ((uint16_t)0x0100) /* PB[6] pin */
#define AFIO_EXTICR2_EXTI6_PC                   ((uint16_t)0x0200) /* PC[6] pin */
#define AFIO_EXTICR2_EXTI6_PD                   ((uint16_t)0x0300) /* PD[6] pin */
#define AFIO_EXTICR2_EXTI6_PE                   ((uint16_t)0x0400) /* PE[6] pin */
#define AFIO_EXTICR2_EXTI6_PF                   ((uint16_t)0x0500) /* PF[6] pin */
#define AFIO_EXTICR2_EXTI6_PG                   ((uint16_t)0x0600) /* PG[6] pin */

#define AFIO_EXTICR2_EXTI7_PA                   ((uint16_t)0x0000) /* PA[7] pin */
#define AFIO_EXTICR2_EXTI7_PB                   ((uint16_t)0x1000) /* PB[7] pin */
#define AFIO_EXTICR2_EXTI7_PC                   ((uint16_t)0x2000) /* PC[7] pin */
#define AFIO_EXTICR2_EXTI7_PD                   ((uint16_t)0x3000) /* PD[7] pin */
#define AFIO_EXTICR2_EXTI7_PE                   ((uint16_t)0x4000) /* PE[7] pin */
#define AFIO_EXTICR2_EXTI7_PF                   ((uint16_t)0x5000) /* PF[7] pin */
#define AFIO_EXTICR2_EXTI7_PG                   ((uint16_t)0x6000) /* PG[7] pin */

/*****************  Bit definition for AFIO_EXTICR3 register  *****************/
#define AFIO_EXTICR3_EXTI8                      ((uint16_t)0x000F) /* EXTI 8 configuration */
#define AFIO_EXTICR3_EXTI9                      ((uint16_t)0x00F0) /* EXTI 9 configuration */
#define AFIO_EXTICR3_EXTI10                     ((uint16_t)0x0F00) /* EXTI 10 configuration */
#define AFIO_EXTICR3_EXTI11                     ((uint16_t)0xF000) /* EXTI 11 configuration */

#define AFIO_EXTICR3_EXTI8_PA                   ((uint16_t)0x0000) /* PA[8] pin */
#define AFIO_EXTICR3_EXTI8_PB                   ((uint16_t)0x0001) /* PB[8] pin */
#define AFIO_EXTICR3_EXTI8_PC                   ((uint16_t)0x0002) /* PC[8] pin */
#define AFIO_EXTICR3_EXTI8_PD                   ((uint16_t)0x0003) /* PD[8] pin */
#define AFIO_EXTICR3_EXTI8_PE                   ((uint16_t)0x0004) /* PE[8] pin */
#define AFIO_EXTICR3_EXTI8_PF                   ((uint16_t)0x0005) /* PF[8] pin */
#define AFIO_EXTICR3_EXTI8_PG                   ((uint16_t)0x0006) /* PG[8] pin */

#define AFIO_EXTICR3_EXTI9_PA                   ((uint16_t)0x0000) /* PA[9] pin */
#define AFIO_EXTICR3_EXTI9_PB                   ((uint16_t)0x0010) /* PB[9] pin */
#define AFIO_EXTICR3_EXTI9_PC                   ((uint16_t)0x0020) /* PC[9] pin */
#define AFIO_EXTICR3_EXTI9_PD                   ((uint16_t)0x0030) /* PD[9] pin */
#define AFIO_EXTICR3_EXTI9_PE                   ((uint16_t)0x0040) /* PE[9] pin */
#define AFIO_EXTICR3_EXTI9_PF                   ((uint16_t)0x0050) /* PF[9] pin */
#define AFIO_EXTICR3_EXTI9_PG                   ((uint16_t)0x0060) /* PG[9] pin */

#define AFIO_EXTICR3_EXTI10_PA                  ((uint16_t)0x0000) /* PA[10] pin */
#define AFIO_EXTICR3_EXTI10_PB                  ((uint16_t)0x0100) /* PB[10] pin */
#define AFIO_EXTICR3_EXTI10_PC                  ((uint16_t)0x0200) /* PC[10] pin */
#define AFIO_EXTICR3_EXTI10_PD                  ((uint16_t)0x0300) /* PD[10] pin */
#define AFIO_EXTICR3_EXTI10_PE                  ((uint16_t)0x0400) /* PE[10] pin */
#define AFIO_EXTICR3_EXTI10_PF                  ((uint16_t)0x0500) /* PF[10] pin */
#define AFIO_EXTICR3_EXTI10_PG                  ((uint16_t)0x0600) /* PG[10] pin */

#define AFIO_EXTICR3_EXTI11_PA                  ((uint16_t)0x0000) /* PA[11] pin */
#define AFIO_EXTICR3_EXTI11_PB                  ((uint16_t)0x1000) /* PB[11] pin */
#define AFIO_EXTICR3_EXTI11_PC                  ((uint16_t)0x2000) /* PC[11] pin */
#define AFIO_EXTICR3_EXTI11_PD                  ((uint16_t)0x3000) /* PD[11] pin */
#define AFIO_EXTICR3_EXTI11_PE                  ((uint16_t)0x4000) /* PE[11] pin */
#define AFIO_EXTICR3_EXTI11_PF                  ((uint16_t)0x5000) /* PF[11] pin */
#define AFIO_EXTICR3_EXTI11_PG                  ((uint16_t)0x6000) /* PG[11] pin */

/*****************  Bit definition for AFIO_EXTICR4 register  *****************/
#define AFIO_EXTICR4_EXTI12                     ((uint16_t)0x000F) /* EXTI 12 configuration */
#define AFIO_EXTICR4_EXTI13                     ((uint16_t)0x00F0) /* EXTI 13 configuration */
#define AFIO_EXTICR4_EXTI14                     ((uint16_t)0x0F00) /* EXTI 14 configuration */
#define AFIO_EXTICR4_EXTI15                     ((uint16_t)0xF000) /* EXTI 15 configuration */

#define AFIO_EXTICR4_EXTI12_PA                  ((uint16_t)0x0000) /* PA[12] pin */
#define AFIO_EXTICR4_EXTI12_PB                  ((uint16_t)0x0001) /* PB[12] pin */
#define AFIO_EXTICR4_EXTI12_PC                  ((uint16_t)0x0002) /* PC[12] pin */
#define AFIO_EXTICR4_EXTI12_PD                  ((uint16_t)0x0003) /* PD[12] pin */
#define AFIO_EXTICR4_EXTI12_PE                  ((uint16_t)0x0004) /* PE[12] pin */
#define AFIO_EXTICR4_EXTI12_PF                  ((uint16_t)0x0005) /* PF[12] pin */
#define AFIO_EXTICR4_EXTI12_PG                  ((uint16_t)0x0006) /* PG[12] pin */

#define AFIO_EXTICR4_EXTI13_PA                  ((uint16_t)0x0000) /* PA[13] pin */
#define AFIO_EXTICR4_EXTI13_PB                  ((uint16_t)0x0010) /* PB[13] pin */
#define AFIO_EXTICR4_EXTI13_PC                  ((uint16_t)0x0020) /* PC[13] pin */
#define AFIO_EXTICR4_EXTI13_PD                  ((uint16_t)0x0030) /* PD[13] pin */
#define AFIO_EXTICR4_EXTI13_PE                  ((uint16_t)0x0040) /* PE[13] pin */
#define AFIO_EXTICR4_EXTI13_PF                  ((uint16_t)0x0050) /* PF[13] pin */
#define AFIO_EXTICR4_EXTI13_PG                  ((uint16_t)0x0060) /* PG[13] pin */

#define AFIO_EXTICR4_EXTI14_PA                  ((uint16_t)0x0000) /* PA[14] pin */
#define AFIO_EXTICR4_EXTI14_PB                  ((uint16_t)0x0100) /* PB[14] pin */
#define AFIO_EXTICR4_EXTI14_PC                  ((uint16_t)0x0200) /* PC[14] pin */
#define AFIO_EXTICR4_EXTI14_PD                  ((uint16_t)0x0300) /* PD[14] pin */
#define AFIO_EXTICR4_EXTI14_PE                  ((uint16_t)0x0400) /* PE[14] pin */
#define AFIO_EXTICR4_EXTI14_PF                  ((uint16_t)0x0500) /* PF[14] pin */
#define AFIO_EXTICR4_EXTI14_PG                  ((uint16_t)0x0600) /* PG[14] pin */

#define AFIO_EXTICR4_EXTI15_PA                  ((uint16_t)0x0000) /* PA[15] pin */
#define AFIO_EXTICR4_EXTI15_PB                  ((uint16_t)0x1000) /* PB[15] pin */
#define AFIO_EXTICR4_EXTI15_PC                  ((uint16_t)0x2000) /* PC[15] pin */
#define AFIO_EXTICR4_EXTI15_PD                  ((uint16_t)0x3000) /* PD[15] pin */
#define AFIO_EXTICR4_EXTI15_PE                  ((uint16_t)0x4000) /* PE[15] pin */
#define AFIO_EXTICR4_EXTI15_PF                  ((uint16_t)0x5000) /* PF[15] pin */
#define AFIO_EXTICR4_EXTI15_PG                  ((uint16_t)0x6000) /* PG[15] pin */
#endif

/******************************************************************************/
/*                           Independent WATCHDOG                             */
/******************************************************************************/

/*******************  Bit definition for IWDG_CTLR register  ********************/
#define IWDG_KEY                                ((uint16_t)0xFFFF) /* Key value (write only, read 0000h) */

/*******************  Bit definition for IWDG_PSCR register  ********************/
#define IWDG_PR                                 ((uint8_t)0x07) /* PR[2:0] (Prescaler divider) */
#define IWDG_PR_0                               ((uint8_t)0x01) /* Bit 0 */
#define IWDG_PR_1                               ((uint8_t)0x02) /* Bit 1 */
#define IWDG_PR_2                               ((uint8_t)0x04) /* Bit 2 */

/*******************  Bit definition for IWDG_RLDR register  *******************/
#define IWDG_RL                                 ((uint16_t)0x0FFF) /* Watchdog counter reload value */

/*******************  Bit definition for IWDG_STATR register  ********************/
#define IWDG_PVU                                ((uint8_t)0x01) /* Watchdog prescaler value update */
#define IWDG_RVU                                ((uint8_t)0x02) /* Watchdog counter reload value update */

/******************************************************************************/
/*                      Inter-integrated Circuit Interface                    */
/******************************************************************************/

/*******************  Bit definition for I2C_CTLR1 register  ********************/
#define I2C_CTLR1_PE                            ((uint16_t)0x0001) /* Peripheral Enable */
#define I2C_CTLR1_SMBUS                         ((uint16_t)0x0002) /* SMBus Mode */
#define I2C_CTLR1_SMBTYPE                       ((uint16_t)0x0008) /* SMBus Type */
#define I2C_CTLR1_ENARP                         ((uint16_t)0x0010) /* ARP Enable */
#define I2C_CTLR1_ENPEC                         ((uint16_t)0x0020) /* PEC Enable */
#define I2C_CTLR1_ENGC                          ((uint16_t)0x0040) /* General Call Enable */
#define I2C_CTLR1_NOSTRETCH                     ((uint16_t)0x0080) /* Clock Stretching Disable (Slave mode) */
#define I2C_CTLR1_START                         ((uint16_t)0x0100) /* Start Generation */
#define I2C_CTLR1_STOP                          ((uint16_t)0x0200) /* Stop Generation */
#define I2C_CTLR1_ACK                           ((uint16_t)0x0400) /* Acknowledge Enable */
#define I2C_CTLR1_POS                           ((uint16_t)0x0800) /* Acknowledge/PEC Position (for data reception) */
#define I2C_CTLR1_PEC                           ((uint16_t)0x1000) /* Packet Error Checking */
#define I2C_CTLR1_ALERT                         ((uint16_t)0x2000) /* SMBus Alert */
#define I2C_CTLR1_SWRST                         ((uint16_t)0x8000) /* Software Reset */

/*******************  Bit definition for I2C_CTLR2 register  ********************/
#define I2C_CTLR2_FREQ                          ((uint16_t)0x003F) /* FREQ[5:0] bits (Peripheral Clock Frequency) */
#define I2C_CTLR2_FREQ_0                        ((uint16_t)0x0001) /* Bit 0 */
#define I2C_CTLR2_FREQ_1                        ((uint16_t)0x0002) /* Bit 1 */
#define I2C_CTLR2_FREQ_2                        ((uint16_t)0x0004) /* Bit 2 */
#define I2C_CTLR2_FREQ_3                        ((uint16_t)0x0008) /* Bit 3 */
#define I2C_CTLR2_FREQ_4                        ((uint16_t)0x0010) /* Bit 4 */
#define I2C_CTLR2_FREQ_5                        ((uint16_t)0x0020) /* Bit 5 */

#define I2C_CTLR2_ITERREN                       ((uint16_t)0x0100) /* Error Interrupt Enable */
#define I2C_CTLR2_ITEVTEN                       ((uint16_t)0x0200) /* Event Interrupt Enable */
#define I2C_CTLR2_ITBUFEN                       ((uint16_t)0x0400) /* Buffer Interrupt Enable */
#define I2C_CTLR2_DMAEN                         ((uint16_t)0x0800) /* DMA Requests Enable */
#define I2C_CTLR2_LAST                          ((uint16_t)0x1000) /* DMA Last Transfer */

/*******************  Bit definition for I2C_OADDR1 register  *******************/
#define I2C_OADDR1_ADD1_7                       ((uint16_t)0x00FE) /* Interface Address */
#define I2C_OADDR1_ADD8_9                       ((uint16_t)0x0300) /* Interface Address */

#define I2C_OADDR1_ADD0                         ((uint16_t)0x0001) /* Bit 0 */
#define I2C_OADDR1_ADD1                         ((uint16_t)0x0002) /* Bit 1 */
#define I2C_OADDR1_ADD2                         ((uint16_t)0x0004) /* Bit 2 */
#define I2C_OADDR1_ADD3                         ((uint16_t)0x0008) /* Bit 3 */
#define I2C_OADDR1_ADD4                         ((uint16_t)0x0010) /* Bit 4 */
#define I2C_OADDR1_ADD5                         ((uint16_t)0x0020) /* Bit 5 */
#define I2C_OADDR1_ADD6                         ((uint16_t)0x0040) /* Bit 6 */
#define I2C_OADDR1_ADD7                         ((uint16_t)0x0080) /* Bit 7 */
#define I2C_OADDR1_ADD8                         ((uint16_t)0x0100) /* Bit 8 */
#define I2C_OADDR1_ADD9                         ((uint16_t)0x0200) /* Bit 9 */

#define I2C_OADDR1_ADDMODE                      ((uint16_t)0x8000) /* Addressing Mode (Slave mode) */

/*******************  Bit definition for I2C_OADDR2 register  *******************/
#define I2C_OADDR2_ENDUAL                       ((uint8_t)0x01) /* Dual addressing mode enable */
#define I2C_OADDR2_ADD2                         ((uint8_t)0xFE) /* Interface address */

/********************  Bit definition for I2C_DATAR register  ********************/
#define I2C_DR_DATAR                            ((uint8_t)0xFF) /* 8-bit Data Register */

/*******************  Bit definition for I2C_STAR1 register  ********************/
#define I2C_STAR1_SB                            ((uint16_t)0x0001) /* Start Bit (Master mode) */
#define I2C_STAR1_ADDR                          ((uint16_t)0x0002) /* Address sent (master mode)/matched (slave mode) */
#define I2C_STAR1_BTF                           ((uint16_t)0x0004) /* Byte Transfer Finished */
#define I2C_STAR1_ADD10                         ((uint16_t)0x0008) /* 10-bit header sent (Master mode) */
#define I2C_STAR1_STOPF                         ((uint16_t)0x0010) /* Stop detection (Slave mode) */
#define I2C_STAR1_RXNE                          ((uint16_t)0x0040) /* Data Register not Empty (receivers) */
#define I2C_STAR1_TXE                           ((uint16_t)0x0080) /* Data Register Empty (transmitters) */
#define I2C_STAR1_BERR                          ((uint16_t)0x0100) /* Bus Error */
#define I2C_STAR1_ARLO                          ((uint16_t)0x0200) /* Arbitration Lost (master mode) */
#define I2C_STAR1_AF                            ((uint16_t)0x0400) /* Acknowledge Failure */
#define I2C_STAR1_OVR                           ((uint16_t)0x0800) /* Overrun/Underrun */
#define I2C_STAR1_PECERR                        ((uint16_t)0x1000) /* PEC Error in reception */
#define I2C_STAR1_TIMEOUT                       ((uint16_t)0x4000) /* Timeout or Tlow Error */
#define I2C_STAR1_SMBALERT                      ((uint16_t)0x8000) /* SMBus Alert */

/*******************  Bit definition for I2C_STAR2 register  ********************/
#define I2C_STAR2_MSL                           ((uint16_t)0x0001) /* Master/Slave */
#define I2C_STAR2_BUSY                          ((uint16_t)0x0002) /* Bus Busy */
#define I2C_STAR2_TRA                           ((uint16_t)0x0004) /* Transmitter/Receiver */
#define I2C_STAR2_GENCALL                       ((uint16_t)0x0010) /* General Call Address (Slave mode) */
#define I2C_STAR2_SMBDEFAULT                    ((uint16_t)0x0020) /* SMBus Device Default Address (Slave mode) */
#define I2C_STAR2_SMBHOST                       ((uint16_t)0x0040) /* SMBus Host Header (Slave mode) */
#define I2C_STAR2_DUALF                         ((uint16_t)0x0080) /* Dual Flag (Slave mode) */
#define I2C_STAR2_PEC                           ((uint16_t)0xFF00) /* Packet Error Checking Register */

/*******************  Bit definition for I2C_CKCFGR register  ********************/
#define I2C_CKCFGR_CCR                          ((uint16_t)0x0FFF) /* Clock Control Register in Fast/Standard mode (Master mode) */
#define I2C_CKCFGR_DUTY                         ((uint16_t)0x4000) /* Fast Mode Duty Cycle */
#define I2C_CKCFGR_FS                           ((uint16_t)0x8000) /* I2C Master Mode Selection */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/******************  Bit definition for I2C_RTR register  *******************/
#define I2C_RTR_TRISE                           ((uint8_t)0x3F) /* Maximum Rise Time in Fast/Standard mode (Master mode) */
#endif

/******************************************************************************/
/*                             Power Control                                  */
/******************************************************************************/

/********************  Bit definition for PWR_CTLR register  ********************/
#define PWR_CTLR_LPDS                           ((uint16_t)0x0001) /* Low-Power Deepsleep */
#define PWR_CTLR_PDDS                           ((uint16_t)0x0002) /* Power Down Deepsleep */
#define PWR_CTLR_CWUF                           ((uint16_t)0x0004) /* Clear Wakeup Flag */
#define PWR_CTLR_CSBF                           ((uint16_t)0x0008) /* Clear Standby Flag */
#define PWR_CTLR_PVDE                           ((uint16_t)0x0010) /* Power Voltage Detector Enable */

#define PWR_CTLR_PLS                            ((uint16_t)0x00E0) /* PLS[2:0] bits (PVD Level Selection) */
#define PWR_CTLR_PLS_0                          ((uint16_t)0x0020) /* Bit 0 */
#define PWR_CTLR_PLS_1                          ((uint16_t)0x0040) /* Bit 1 */
#define PWR_CTLR_PLS_2                          ((uint16_t)0x0080) /* Bit 2 */

#define PWR_CTLR_PLS_2V2                        ((uint16_t)0x0000) /* PVD level 2.2V */
#define PWR_CTLR_PLS_2V3                        ((uint16_t)0x0020) /* PVD level 2.3V */
#define PWR_CTLR_PLS_2V4                        ((uint16_t)0x0040) /* PVD level 2.4V */
#define PWR_CTLR_PLS_2V5                        ((uint16_t)0x0060) /* PVD level 2.5V */
#define PWR_CTLR_PLS_2V6                        ((uint16_t)0x0080) /* PVD level 2.6V */
#define PWR_CTLR_PLS_2V7                        ((uint16_t)0x00A0) /* PVD level 2.7V */
#define PWR_CTLR_PLS_2V8                        ((uint16_t)0x00C0) /* PVD level 2.8V */
#define PWR_CTLR_PLS_2V9                        ((uint16_t)0x00E0) /* PVD level 2.9V */

#define PWR_CTLR_DBP                            ((uint16_t)0x0100) /* Disable Backup Domain write protection */

/*******************  Bit definition for PWR_CSR register  ********************/
#define PWR_CSR_WUF                             ((uint16_t)0x0001) /* Wakeup Flag */
#define PWR_CSR_SBF                             ((uint16_t)0x0002) /* Standby Flag */
#define PWR_CSR_PVDO                            ((uint16_t)0x0004) /* PVD Output */
#define PWR_CSR_EWUP                            ((uint16_t)0x0100) /* Enable WKUP pin */

/******************************************************************************/
/*                         Reset and Clock Control                            */
/******************************************************************************/

/********************  Bit definition for RCC_CTLR register  ********************/
#define RCC_HSION                               ((uint32_t)0x00000001) /* Internal High Speed clock enable */
#define RCC_HSIRDY                              ((uint32_t)0x00000002) /* Internal High Speed clock ready flag */
#define RCC_HSITRIM                             ((uint32_t)0x000000F8) /* Internal High Speed clock trimming */
#define RCC_HSICAL                              ((uint32_t)0x0000FF00) /* Internal High Speed clock Calibration */
#define RCC_HSEON                               ((uint32_t)0x00010000) /* External High Speed clock enable */
#define RCC_HSERDY                              ((uint32_t)0x00020000) /* External High Speed clock ready flag */
#define RCC_HSEBYP                              ((uint32_t)0x00040000) /* External High Speed clock Bypass */
#define RCC_CSSON                               ((uint32_t)0x00080000) /* Clock Security System enable */
#define RCC_PLLON                               ((uint32_t)0x01000000) /* PLL enable */
#define RCC_PLLRDY                              ((uint32_t)0x02000000) /* PLL clock ready flag */

/*******************  Bit definition for RCC_CFGR0 register  *******************/
#define RCC_SW                                  ((uint32_t)0x00000003) /* SW[1:0] bits (System clock Switch) */
#define RCC_SW_0                                ((uint32_t)0x00000001) /* Bit 0 */
#define RCC_SW_1                                ((uint32_t)0x00000002) /* Bit 1 */

#define RCC_SW_HSI                              ((uint32_t)0x00000000) /* HSI selected as system clock */
#define RCC_SW_HSE                              ((uint32_t)0x00000001) /* HSE selected as system clock */
#define RCC_SW_PLL                              ((uint32_t)0x00000002) /* PLL selected as system clock */

#define RCC_SWS                                 ((uint32_t)0x0000000C) /* SWS[1:0] bits (System Clock Switch Status) */
#define RCC_SWS_0                               ((uint32_t)0x00000004) /* Bit 0 */
#define RCC_SWS_1                               ((uint32_t)0x00000008) /* Bit 1 */

#define RCC_SWS_HSI                             ((uint32_t)0x00000000) /* HSI oscillator used as system clock */
#define RCC_SWS_HSE                             ((uint32_t)0x00000004) /* HSE oscillator used as system clock */
#define RCC_SWS_PLL                             ((uint32_t)0x00000008) /* PLL used as system clock */

#define RCC_HPRE                                ((uint32_t)0x000000F0) /* HPRE[3:0] bits (AHB prescaler) */
#define RCC_HPRE_0                              ((uint32_t)0x00000010) /* Bit 0 */
#define RCC_HPRE_1                              ((uint32_t)0x00000020) /* Bit 1 */
#define RCC_HPRE_2                              ((uint32_t)0x00000040) /* Bit 2 */
#define RCC_HPRE_3                              ((uint32_t)0x00000080) /* Bit 3 */

#if defined(CH32V003) || defined(CH32X03x)
#define RCC_HPRE_DIV1                           ((uint32_t)0x00000000) /* SYSCLK not divided */
#define RCC_HPRE_DIV2                           ((uint32_t)0x00000010) /* SYSCLK divided by 2 */
#define RCC_HPRE_DIV3                           ((uint32_t)0x00000020) /* SYSCLK divided by 3 */
#define RCC_HPRE_DIV4                           ((uint32_t)0x00000030) /* SYSCLK divided by 4 */
#define RCC_HPRE_DIV5                           ((uint32_t)0x00000040) /* SYSCLK divided by 5 */
#define RCC_HPRE_DIV6                           ((uint32_t)0x00000050) /* SYSCLK divided by 6 */
#define RCC_HPRE_DIV7                           ((uint32_t)0x00000060) /* SYSCLK divided by 7 */
#define RCC_HPRE_DIV8                           ((uint32_t)0x00000070) /* SYSCLK divided by 8 */
#define RCC_HPRE_DIV16                          ((uint32_t)0x000000B0) /* SYSCLK divided by 16 */
#define RCC_HPRE_DIV32                          ((uint32_t)0x000000C0) /* SYSCLK divided by 32 */
#define RCC_HPRE_DIV64                          ((uint32_t)0x000000D0) /* SYSCLK divided by 64 */
#define RCC_HPRE_DIV128                         ((uint32_t)0x000000E0) /* SYSCLK divided by 128 */
#define RCC_HPRE_DIV256                         ((uint32_t)0x000000F0) /* SYSCLK divided by 256 */
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RCC_HPRE_DIV1                   		((uint32_t)0x00000000) /* SYSCLK not divided */
#define RCC_HPRE_DIV2                   		((uint32_t)0x00000080) /* SYSCLK divided by 2 */
#define RCC_HPRE_DIV4                   		((uint32_t)0x00000090) /* SYSCLK divided by 4 */
#define RCC_HPRE_DIV8                   		((uint32_t)0x000000A0) /* SYSCLK divided by 8 */
#define RCC_HPRE_DIV16                  		((uint32_t)0x000000B0) /* SYSCLK divided by 16 */
#define RCC_HPRE_DIV64                  		((uint32_t)0x000000C0) /* SYSCLK divided by 64 */
#define RCC_HPRE_DIV128                 		((uint32_t)0x000000D0) /* SYSCLK divided by 128 */
#define RCC_HPRE_DIV256                 		((uint32_t)0x000000E0) /* SYSCLK divided by 256 */
#define RCC_HPRE_DIV512                 		((uint32_t)0x000000F0) /* SYSCLK divided by 512 */
#endif

#define RCC_PPRE1                               ((uint32_t)0x00000700) /* PRE1[2:0] bits (APB1 prescaler) */
#define RCC_PPRE1_0                             ((uint32_t)0x00000100) /* Bit 0 */
#define RCC_PPRE1_1                             ((uint32_t)0x00000200) /* Bit 1 */
#define RCC_PPRE1_2                             ((uint32_t)0x00000400) /* Bit 2 */

#define RCC_PPRE1_DIV1                          ((uint32_t)0x00000000) /* HCLK not divided */
#define RCC_PPRE1_DIV2                          ((uint32_t)0x00000400) /* HCLK divided by 2 */
#define RCC_PPRE1_DIV4                          ((uint32_t)0x00000500) /* HCLK divided by 4 */
#define RCC_PPRE1_DIV8                          ((uint32_t)0x00000600) /* HCLK divided by 8 */
#define RCC_PPRE1_DIV16                         ((uint32_t)0x00000700) /* HCLK divided by 16 */

#define RCC_PPRE2                               ((uint32_t)0x00003800) /* PRE2[2:0] bits (APB2 prescaler) */
#define RCC_PPRE2_0                             ((uint32_t)0x00000800) /* Bit 0 */
#define RCC_PPRE2_1                             ((uint32_t)0x00001000) /* Bit 1 */
#define RCC_PPRE2_2                             ((uint32_t)0x00002000) /* Bit 2 */

#define RCC_PPRE2_DIV1                          ((uint32_t)0x00000000) /* HCLK not divided */
#define RCC_PPRE2_DIV2                          ((uint32_t)0x00002000) /* HCLK divided by 2 */
#define RCC_PPRE2_DIV4                          ((uint32_t)0x00002800) /* HCLK divided by 4 */
#define RCC_PPRE2_DIV8                          ((uint32_t)0x00003000) /* HCLK divided by 8 */
#define RCC_PPRE2_DIV16                         ((uint32_t)0x00003800) /* HCLK divided by 16 */

#define RCC_ADCPRE                              ((uint32_t)0x0000C000) /* ADCPRE[1:0] bits (ADC prescaler) */
#define RCC_ADCPRE_0                            ((uint32_t)0x00004000) /* Bit 0 */
#define RCC_ADCPRE_1                            ((uint32_t)0x00008000) /* Bit 1 */

#define RCC_ADCPRE_DIV2                         ((uint32_t)0x00000000) /* PCLK2 divided by 2 */
#define RCC_ADCPRE_DIV4                         ((uint32_t)0x00004000) /* PCLK2 divided by 4 */
#define RCC_ADCPRE_DIV6                         ((uint32_t)0x00008000) /* PCLK2 divided by 6 */
#define RCC_ADCPRE_DIV8                         ((uint32_t)0x0000C000) /* PCLK2 divided by 8 */

#define RCC_PLLSRC                              ((uint32_t)0x00010000) /* PLL entry clock source */

#define RCC_PLLXTPRE                            ((uint32_t)0x00020000) /* HSE divider for PLL entry */

#define RCC_PLLMULL                             ((uint32_t)0x003C0000) /* PLLMUL[3:0] bits (PLL multiplication factor) */
#define RCC_PLLMULL_0                           ((uint32_t)0x00040000) /* Bit 0 */
#define RCC_PLLMULL_1                           ((uint32_t)0x00080000) /* Bit 1 */
#define RCC_PLLMULL_2                           ((uint32_t)0x00100000) /* Bit 2 */
#define RCC_PLLMULL_3                           ((uint32_t)0x00200000) /* Bit 3 */

#ifdef CH32V003
#define RCC_PLLSRC_HSI_Mul2                     ((uint32_t)0x00000000) /* HSI clock*2 selected as PLL entry clock source */
#define RCC_PLLSRC_HSE_Mul2                     ((uint32_t)0x00010000) /* HSE clock*2 selected as PLL entry clock source */
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RCC_PLLSRC_HSI_Div2                     ((uint32_t)0x00000000) /* HSI clock divided by 2 selected as PLL entry clock source */
#define RCC_PLLSRC_HSE                          ((uint32_t)0x00010000) /* HSE clock selected as PLL entry clock source */
#endif

#define RCC_PLLXTPRE_HSE                        ((uint32_t)0x00000000) /* HSE clock not divided for PLL entry */
#define RCC_PLLXTPRE_HSE_Div2                   ((uint32_t)0x00020000) /* HSE clock divided by 2 for PLL entry */

#define RCC_PLLMULL2                            ((uint32_t)0x00000000) /* PLL input clock*2 */
#define RCC_PLLMULL3                            ((uint32_t)0x00040000) /* PLL input clock*3 */
#define RCC_PLLMULL4                            ((uint32_t)0x00080000) /* PLL input clock*4 */
#define RCC_PLLMULL5                            ((uint32_t)0x000C0000) /* PLL input clock*5 */
#define RCC_PLLMULL6                            ((uint32_t)0x00100000) /* PLL input clock*6 */
#define RCC_PLLMULL7                            ((uint32_t)0x00140000) /* PLL input clock*7 */
#define RCC_PLLMULL8                            ((uint32_t)0x00180000) /* PLL input clock*8 */
#define RCC_PLLMULL9                            ((uint32_t)0x001C0000) /* PLL input clock*9 */
#define RCC_PLLMULL10                           ((uint32_t)0x00200000) /* PLL input clock10 */
#define RCC_PLLMULL11                           ((uint32_t)0x00240000) /* PLL input clock*11 */
#define RCC_PLLMULL12                           ((uint32_t)0x00280000) /* PLL input clock*12 */
#define RCC_PLLMULL13                           ((uint32_t)0x002C0000) /* PLL input clock*13 */
#define RCC_PLLMULL14                           ((uint32_t)0x00300000) /* PLL input clock*14 */
#define RCC_PLLMULL15                           ((uint32_t)0x00340000) /* PLL input clock*15 */
#define RCC_PLLMULL16                           ((uint32_t)0x00380000) /* PLL input clock*16 */
#if defined(CH32V20x)
#define RCC_PLLMULL18                           ((uint32_t)0x003C0000) /* PLL input clock*18 */
#endif

#if defined(CH32V30x)
/* for CH32V307 */
#define  RCC_PLLMULL18_EXTEN             		((uint32_t)0x00000000) /* PLL input clock*18 */
#define  RCC_PLLMULL3_EXTEN              		((uint32_t)0x00040000) /* PLL input clock*3 */
#define  RCC_PLLMULL4_EXTEN              		((uint32_t)0x00080000) /* PLL input clock*4 */
#define  RCC_PLLMULL5_EXTEN              		((uint32_t)0x000C0000) /* PLL input clock*5 */
#define  RCC_PLLMULL6_EXTEN              		((uint32_t)0x00100000) /* PLL input clock*6 */
#define  RCC_PLLMULL7_EXTEN              		((uint32_t)0x00140000) /* PLL input clock*7 */
#define  RCC_PLLMULL8_EXTEN              		((uint32_t)0x00180000) /* PLL input clock*8 */
#define  RCC_PLLMULL9_EXTEN              		((uint32_t)0x001C0000) /* PLL input clock*9 */
#define  RCC_PLLMULL10_EXTEN             		((uint32_t)0x00200000) /* PLL input clock10 */
#define  RCC_PLLMULL11_EXTEN             		((uint32_t)0x00240000) /* PLL input clock*11 */
#define  RCC_PLLMULL12_EXTEN             		((uint32_t)0x00280000) /* PLL input clock*12 */
#define  RCC_PLLMULL13_EXTEN             		((uint32_t)0x002C0000) /* PLL input clock*13 */
#define  RCC_PLLMULL14_EXTEN             		((uint32_t)0x00300000) /* PLL input clock*14 */
#define  RCC_PLLMULL6_5_EXTEN            		((uint32_t)0x00340000) /* PLL input clock*6.5 */
#define  RCC_PLLMULL15_EXTEN             		((uint32_t)0x00380000) /* PLL input clock*15 */
#define  RCC_PLLMULL16_EXTEN             		((uint32_t)0x003C0000) /* PLL input clock*16 */
#endif

#define RCC_USBPRE                              ((uint32_t)0x00400000) /* USB Device prescaler */

#define RCC_CFGR0_MCO                           ((uint32_t)0x07000000) /* MCO[2:0] bits (Microcontroller Clock Output) */
#define RCC_MCO_0                               ((uint32_t)0x01000000) /* Bit 0 */
#define RCC_MCO_1                               ((uint32_t)0x02000000) /* Bit 1 */
#define RCC_MCO_2                               ((uint32_t)0x04000000) /* Bit 2 */

#define RCC_MCO_NOCLOCK                         ((uint32_t)0x00000000) /* No clock */
#define RCC_CFGR0_MCO_SYSCLK                    ((uint32_t)0x04000000) /* System clock selected as MCO source */
#define RCC_CFGR0_MCO_HSI                       ((uint32_t)0x05000000) /* HSI clock selected as MCO source */
#define RCC_CFGR0_MCO_HSE                       ((uint32_t)0x06000000) /* HSE clock selected as MCO source  */
#define RCC_CFGR0_MCO_PLL                       ((uint32_t)0x07000000) /* PLL clock divided by 2 selected as MCO source */

/*******************  Bit definition for RCC_INTR register  ********************/
#define RCC_LSIRDYF                             ((uint32_t)0x00000001) /* LSI Ready Interrupt flag */
#define RCC_LSERDYF                             ((uint32_t)0x00000002) /* LSE Ready Interrupt flag */
#define RCC_HSIRDYF                             ((uint32_t)0x00000004) /* HSI Ready Interrupt flag */
#define RCC_HSERDYF                             ((uint32_t)0x00000008) /* HSE Ready Interrupt flag */
#define RCC_PLLRDYF                             ((uint32_t)0x00000010) /* PLL Ready Interrupt flag */
#define RCC_CSSF                                ((uint32_t)0x00000080) /* Clock Security System Interrupt flag */
#define RCC_LSIRDYIE                            ((uint32_t)0x00000100) /* LSI Ready Interrupt Enable */
#define RCC_LSERDYIE                            ((uint32_t)0x00000200) /* LSE Ready Interrupt Enable */
#define RCC_HSIRDYIE                            ((uint32_t)0x00000400) /* HSI Ready Interrupt Enable */
#define RCC_HSERDYIE                            ((uint32_t)0x00000800) /* HSE Ready Interrupt Enable */
#define RCC_PLLRDYIE                            ((uint32_t)0x00001000) /* PLL Ready Interrupt Enable */
#define RCC_LSIRDYC                             ((uint32_t)0x00010000) /* LSI Ready Interrupt Clear */
#define RCC_LSERDYC                             ((uint32_t)0x00020000) /* LSE Ready Interrupt Clear */
#define RCC_HSIRDYC                             ((uint32_t)0x00040000) /* HSI Ready Interrupt Clear */
#define RCC_HSERDYC                             ((uint32_t)0x00080000) /* HSE Ready Interrupt Clear */
#define RCC_PLLRDYC                             ((uint32_t)0x00100000) /* PLL Ready Interrupt Clear */
#define RCC_CSSC                                ((uint32_t)0x00800000) /* Clock Security System Interrupt Clear */

/*****************  Bit definition for RCC_APB2PRSTR register  *****************/
#define RCC_AFIORST                             ((uint32_t)0x00000001) /* Alternate Function I/O reset */
#define RCC_IOPARST                             ((uint32_t)0x00000004) /* I/O port A reset */
#define RCC_IOPBRST                             ((uint32_t)0x00000008) /* I/O port B reset */
#define RCC_IOPCRST                             ((uint32_t)0x00000010) /* I/O port C reset */
#define RCC_IOPDRST                             ((uint32_t)0x00000020) /* I/O port D reset */
#define RCC_ADC1RST                             ((uint32_t)0x00000200) /* ADC 1 interface reset */

#define RCC_ADC2RST                             ((uint32_t)0x00000400) /* ADC 2 interface reset */

#define RCC_TIM1RST                             ((uint32_t)0x00000800) /* TIM1 Timer reset */
#define RCC_SPI1RST                             ((uint32_t)0x00001000) /* SPI 1 reset */
#define RCC_USART1RST                           ((uint32_t)0x00004000) /* USART1 reset */

#define RCC_IOPERST                             ((uint32_t)0x00000040) /* I/O port E reset */

/*****************  Bit definition for RCC_APB1PRSTR register  *****************/
#define RCC_TIM2RST                             ((uint32_t)0x00000001) /* Timer 2 reset */
#define RCC_TIM3RST                             ((uint32_t)0x00000002) /* Timer 3 reset */
#define RCC_WWDGRST                             ((uint32_t)0x00000800) /* Window Watchdog reset */
#define RCC_USART2RST                           ((uint32_t)0x00020000) /* USART 2 reset */
#define RCC_I2C1RST                             ((uint32_t)0x00200000) /* I2C 1 reset */

#define RCC_CAN1RST                             ((uint32_t)0x02000000) /* CAN1 reset */

#define RCC_BKPRST                              ((uint32_t)0x08000000) /* Backup interface reset */
#define RCC_PWRRST                              ((uint32_t)0x10000000) /* Power interface reset */

#define RCC_TIM4RST                             ((uint32_t)0x00000004) /* Timer 4 reset */
#define RCC_SPI2RST                             ((uint32_t)0x00004000) /* SPI 2 reset */
#define RCC_USART3RST                           ((uint32_t)0x00040000) /* USART 3 reset */
#define RCC_I2C2RST                             ((uint32_t)0x00400000) /* I2C 2 reset */

#define RCC_USBRST                              ((uint32_t)0x00800000) /* USB Device reset */

/******************  Bit definition for RCC_AHBPCENR register  ******************/
#define RCC_DMA1EN                              ((uint16_t)0x0001) /* DMA1 clock enable */
#define RCC_SRAMEN                              ((uint16_t)0x0004) /* SRAM interface clock enable */
#define RCC_FLITFEN                             ((uint16_t)0x0010) /* FLITF clock enable */
#define RCC_CRCEN                               ((uint16_t)0x0040) /* CRC clock enable */
#define RCC_USBHD                               ((uint16_t)0x1000)
#define RCC_USBFS                               ((uint16_t)0x1000)
#define RCC_USBPD                               ((uint16_t)0x20000)

/******************  Bit definition for RCC_APB2PCENR register  *****************/
#define RCC_AFIOEN                              ((uint32_t)0x00000001) /* Alternate Function I/O clock enable */
#define RCC_IOPAEN                              ((uint32_t)0x00000004) /* I/O port A clock enable */
#define RCC_IOPBEN                              ((uint32_t)0x00000008) /* I/O port B clock enable */
#define RCC_IOPCEN                              ((uint32_t)0x00000010) /* I/O port C clock enable */
#define RCC_IOPDEN                              ((uint32_t)0x00000020) /* I/O port D clock enable */
#define RCC_ADC1EN                              ((uint32_t)0x00000200) /* ADC 1 interface clock enable */

#define RCC_ADC2EN                              ((uint32_t)0x00000400) /* ADC 2 interface clock enable */

#define RCC_TIM1EN                              ((uint32_t)0x00000800) /* TIM1 Timer clock enable */
#define RCC_SPI1EN                              ((uint32_t)0x00001000) /* SPI 1 clock enable */
#define RCC_USART1EN                            ((uint32_t)0x00004000) /* USART1 clock enable */

/*****************  Bit definition for RCC_APB1PCENR register  ******************/
#define RCC_TIM2EN                              ((uint32_t)0x00000001) /* Timer 2 clock enabled*/
#define RCC_TIM3EN                              ((uint32_t)0x00000002) /* Timer 3 clock enable */
#define RCC_WWDGEN                              ((uint32_t)0x00000800) /* Window Watchdog clock enable */
#define RCC_USART2EN                            ((uint32_t)0x00020000) /* USART 2 clock enable */
#define RCC_I2C1EN                              ((uint32_t)0x00200000) /* I2C 1 clock enable */

#define RCC_BKPEN                               ((uint32_t)0x08000000) /* Backup interface clock enable */
#define RCC_PWREN                               ((uint32_t)0x10000000) /* Power interface clock enable */

#define RCC_USBEN                               ((uint32_t)0x00800000) /* USB Device clock enable */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/*******************  Bit definition for RCC_BDCTLR register  *******************/
#define RCC_LSEON                               ((uint32_t)0x00000001) /* External Low Speed oscillator enable */
#define RCC_LSERDY                              ((uint32_t)0x00000002) /* External Low Speed oscillator Ready */
#define RCC_LSEBYP                              ((uint32_t)0x00000004) /* External Low Speed oscillator Bypass */

#define RCC_RTCSEL                              ((uint32_t)0x00000300) /* RTCSEL[1:0] bits (RTC clock source selection) */
#define RCC_RTCSEL_0                            ((uint32_t)0x00000100) /* Bit 0 */
#define RCC_RTCSEL_1                            ((uint32_t)0x00000200) /* Bit 1 */

#define RCC_RTCSEL_NOCLOCK                      ((uint32_t)0x00000000) /* No clock */
#define RCC_RTCSEL_LSE                          ((uint32_t)0x00000100) /* LSE oscillator clock used as RTC clock */
#define RCC_RTCSEL_LSI                          ((uint32_t)0x00000200) /* LSI oscillator clock used as RTC clock */
#define RCC_RTCSEL_HSE                          ((uint32_t)0x00000300) /* HSE oscillator clock divided by 128 used as RTC clock */

#define RCC_RTCEN                               ((uint32_t)0x00008000) /* RTC clock enable */
#define RCC_BDRST                               ((uint32_t)0x00010000) /* Backup domain software reset  */
#endif

/*******************  Bit definition for RCC_RSTSCKR register  ********************/
#define RCC_LSION                               ((uint32_t)0x00000001) /* Internal Low Speed oscillator enable */
#define RCC_LSIRDY                              ((uint32_t)0x00000002) /* Internal Low Speed oscillator Ready */
#define RCC_RMVF                                ((uint32_t)0x01000000) /* Remove reset flag */
#define RCC_PINRSTF                             ((uint32_t)0x04000000) /* PIN reset flag */
#define RCC_PORRSTF                             ((uint32_t)0x08000000) /* POR/PDR reset flag */
#define RCC_SFTRSTF                             ((uint32_t)0x10000000) /* Software Reset flag */
#define RCC_IWDGRSTF                            ((uint32_t)0x20000000) /* Independent Watchdog reset flag */
#define RCC_WWDGRSTF                            ((uint32_t)0x40000000) /* Window watchdog reset flag */
#define RCC_LPWRRSTF                            ((uint32_t)0x80000000) /* Low-Power reset flag */

#if defined(CH32V30x)
/******************************************************************************/
/*                                    RNG                                     */
/******************************************************************************/
/********************  Bit definition for RNG_CR register  *******************/
#define  RNG_CR_RNGEN                         ((uint32_t)0x00000004)
#define  RNG_CR_IE                            ((uint32_t)0x00000008)

/********************  Bit definition for RNG_SR register  *******************/
#define  RNG_SR_DRDY                          ((uint32_t)0x00000001)
#define  RNG_SR_CECS                          ((uint32_t)0x00000002)
#define  RNG_SR_SECS                          ((uint32_t)0x00000004)
#define  RNG_SR_CEIS                          ((uint32_t)0x00000020)
#define  RNG_SR_SEIS                          ((uint32_t)0x00000040)
#endif

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/******************************************************************************/
/*                             Real-Time Clock                                */
/******************************************************************************/

/*******************  Bit definition for RTC_CTLRH register  ********************/
#define RTC_CTLRH_SECIE                         ((uint8_t)0x01) /* Second Interrupt Enable */
#define RTC_CTLRH_ALRIE                         ((uint8_t)0x02) /* Alarm Interrupt Enable */
#define RTC_CTLRH_OWIE                          ((uint8_t)0x04) /* OverfloW Interrupt Enable */

/*******************  Bit definition for RTC_CTLRL register  ********************/
#define RTC_CTLRL_SECF                          ((uint8_t)0x01) /* Second Flag */
#define RTC_CTLRL_ALRF                          ((uint8_t)0x02) /* Alarm Flag */
#define RTC_CTLRL_OWF                           ((uint8_t)0x04) /* OverfloW Flag */
#define RTC_CTLRL_RSF                           ((uint8_t)0x08) /* Registers Synchronized Flag */
#define RTC_CTLRL_CNF                           ((uint8_t)0x10) /* Configuration Flag */
#define RTC_CTLRL_RTOFF                         ((uint8_t)0x20) /* RTC operation OFF */

/*******************  Bit definition for RTC_PSCH register  *******************/
#define RTC_PSCH_PRL                            ((uint16_t)0x000F) /* RTC Prescaler Reload Value High */

/*******************  Bit definition for RTC_PRLL register  *******************/
#define RTC_PSCL_PRL                            ((uint16_t)0xFFFF) /* RTC Prescaler Reload Value Low */

/*******************  Bit definition for RTC_DIVH register  *******************/
#define RTC_DIVH_RTC_DIV                        ((uint16_t)0x000F) /* RTC Clock Divider High */

/*******************  Bit definition for RTC_DIVL register  *******************/
#define RTC_DIVL_RTC_DIV                        ((uint16_t)0xFFFF) /* RTC Clock Divider Low */

/*******************  Bit definition for RTC_CNTH register  *******************/
#define RTC_CNTH_RTC_CNT                        ((uint16_t)0xFFFF) /* RTC Counter High */

/*******************  Bit definition for RTC_CNTL register  *******************/
#define RTC_CNTL_RTC_CNT                        ((uint16_t)0xFFFF) /* RTC Counter Low */

/*******************  Bit definition for RTC_ALRMH register  *******************/
#define RTC_ALRMH_RTC_ALRM                      ((uint16_t)0xFFFF) /* RTC Alarm High */

/*******************  Bit definition for RTC_ALRML register  *******************/
#define RTC_ALRML_RTC_ALRM                      ((uint16_t)0xFFFF) /* RTC Alarm Low */
#endif

/******************************************************************************/
/*                        Serial Peripheral Interface                         */
/******************************************************************************/

/*******************  Bit definition for SPI_CTLR1 register  ********************/
#define SPI_CTLR1_CPHA                          ((uint16_t)0x0001) /* Clock Phase */
#define SPI_CTLR1_CPOL                          ((uint16_t)0x0002) /* Clock Polarity */
#define SPI_CTLR1_MSTR                          ((uint16_t)0x0004) /* Master Selection */

#define SPI_CTLR1_BR                            ((uint16_t)0x0038) /* BR[2:0] bits (Baud Rate Control) */
#define SPI_CTLR1_BR_0                          ((uint16_t)0x0008) /* Bit 0 */
#define SPI_CTLR1_BR_1                          ((uint16_t)0x0010) /* Bit 1 */
#define SPI_CTLR1_BR_2                          ((uint16_t)0x0020) /* Bit 2 */

#define SPI_CTLR1_SPE                           ((uint16_t)0x0040) /* SPI Enable */
#define SPI_CTLR1_SSI                           ((uint16_t)0x0100) /* Internal slave select */
#define SPI_CTLR1_SSM                           ((uint16_t)0x0200) /* Software slave management */
#define SPI_CTLR1_RXONLY                        ((uint16_t)0x0400) /* Receive only */
#define SPI_CTLR1_DFF                           ((uint16_t)0x0800) /* Data Frame Format */
#define SPI_CTLR1_CRCNEXT                       ((uint16_t)0x1000) /* Transmit CRC next */
#define SPI_CTLR1_CRCEN                         ((uint16_t)0x2000) /* Hardware CRC calculation enable */
#define SPI_CTLR1_BIDIOE                        ((uint16_t)0x4000) /* Output enable in bidirectional mode */
#define SPI_CTLR1_BIDIMODE                      ((uint16_t)0x8000) /* Bidirectional data mode enable */

/*******************  Bit definition for SPI_CTLR2 register  ********************/
#define SPI_CTLR2_RXDMAEN                       ((uint8_t)0x01) /* Rx Buffer DMA Enable */
#define SPI_CTLR2_TXDMAEN                       ((uint8_t)0x02) /* Tx Buffer DMA Enable */
#define SPI_CTLR2_SSOE                          ((uint8_t)0x04) /* SS Output Enable */
#define SPI_CTLR2_ERRIE                         ((uint8_t)0x20) /* Error Interrupt Enable */
#define SPI_CTLR2_RXNEIE                        ((uint8_t)0x40) /* RX buffer Not Empty Interrupt Enable */
#define SPI_CTLR2_TXEIE                         ((uint8_t)0x80) /* Tx buffer Empty Interrupt Enable */

/********************  Bit definition for SPI_STATR register  ********************/
#define SPI_STATR_RXNE                          ((uint8_t)0x01) /* Receive buffer Not Empty */
#define SPI_STATR_TXE                           ((uint8_t)0x02) /* Transmit buffer Empty */
#define SPI_STATR_CHSIDE                        ((uint8_t)0x04) /* Channel side */
#define SPI_STATR_UDR                           ((uint8_t)0x08) /* Underrun flag */
#define SPI_STATR_CRCERR                        ((uint8_t)0x10) /* CRC Error flag */
#define SPI_STATR_MODF                          ((uint8_t)0x20) /* Mode fault */
#define SPI_STATR_OVR                           ((uint8_t)0x40) /* Overrun flag */
#define SPI_STATR_BSY                           ((uint8_t)0x80) /* Busy flag */

/********************  Bit definition for SPI_DATAR register  ********************/
#define SPI_DATAR_DR                            ((uint16_t)0xFFFF) /* Data Register */

/*******************  Bit definition for SPI_CRCR register  ******************/
#define SPI_CRCR_CRCPOLY                        ((uint16_t)0xFFFF) /* CRC polynomial register */

/******************  Bit definition for SPI_RCRCR register  ******************/
#define SPI_RCRCR_RXCRC                         ((uint16_t)0xFFFF) /* Rx CRC Register */

/******************  Bit definition for SPI_TCRCR register  ******************/
#define SPI_TCRCR_TXCRC                         ((uint16_t)0xFFFF) /* Tx CRC Register */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/******************  Bit definition for SPI_I2SCFGR register  *****************/
#define SPI_I2SCFGR_CHLEN                       ((uint16_t)0x0001) /* Channel length (number of bits per audio channel) */

#define SPI_I2SCFGR_DATLEN                      ((uint16_t)0x0006) /* DATLEN[1:0] bits (Data length to be transferred) */
#define SPI_I2SCFGR_DATLEN_0                    ((uint16_t)0x0002) /* Bit 0 */
#define SPI_I2SCFGR_DATLEN_1                    ((uint16_t)0x0004) /* Bit 1 */

#define SPI_I2SCFGR_CKPOL                       ((uint16_t)0x0008) /* steady state clock polarity */

#define SPI_I2SCFGR_I2SSTD                      ((uint16_t)0x0030) /* I2SSTD[1:0] bits (I2S standard selection) */
#define SPI_I2SCFGR_I2SSTD_0                    ((uint16_t)0x0010) /* Bit 0 */
#define SPI_I2SCFGR_I2SSTD_1                    ((uint16_t)0x0020) /* Bit 1 */

#define SPI_I2SCFGR_PCMSYNC                     ((uint16_t)0x0080) /* PCM frame synchronization */

#define SPI_I2SCFGR_I2SCFG                      ((uint16_t)0x0300) /* I2SCFG[1:0] bits (I2S configuration mode) */
#define SPI_I2SCFGR_I2SCFG_0                    ((uint16_t)0x0100) /* Bit 0 */
#define SPI_I2SCFGR_I2SCFG_1                    ((uint16_t)0x0200) /* Bit 1 */

#define SPI_I2SCFGR_I2SE                        ((uint16_t)0x0400) /* I2S Enable */
#define SPI_I2SCFGR_I2SMOD                      ((uint16_t)0x0800) /* I2S mode selection */

/******************  Bit definition for SPI_I2SPR register  *******************/
#define SPI_I2SPR_I2SDIV                        ((uint16_t)0x00FF) /* I2S Linear prescaler */
#define SPI_I2SPR_ODD                           ((uint16_t)0x0100) /* Odd factor for the prescaler */
#define SPI_I2SPR_MCKOE                         ((uint16_t)0x0200) /* Master Clock Output Enable */
#endif

/******************************************************************************/
/*                                    TIM                                     */
/******************************************************************************/

/*******************  Bit definition for TIM_CTLR1 register  ********************/
#define TIM_CEN                                 ((uint16_t)0x0001) /* Counter enable */
#define TIM_UDIS                                ((uint16_t)0x0002) /* Update disable */
#define TIM_URS                                 ((uint16_t)0x0004) /* Update request source */
#define TIM_OPM                                 ((uint16_t)0x0008) /* One pulse mode */
#define TIM_DIR                                 ((uint16_t)0x0010) /* Direction */

#define TIM_CMS                                 ((uint16_t)0x0060) /* CMS[1:0] bits (Center-aligned mode selection) */
#define TIM_CMS_0                               ((uint16_t)0x0020) /* Bit 0 */
#define TIM_CMS_1                               ((uint16_t)0x0040) /* Bit 1 */

#define TIM_ARPE                                ((uint16_t)0x0080) /* Auto-reload preload enable */

#define TIM_CTLR1_CKD                           ((uint16_t)0x0300) /* CKD[1:0] bits (clock division) */
#define TIM_CKD_0                               ((uint16_t)0x0100) /* Bit 0 */
#define TIM_CKD_1                               ((uint16_t)0x0200) /* Bit 1 */

/*******************  Bit definition for TIM_CTLR2 register  ********************/
#define TIM_CCPC                                ((uint16_t)0x0001) /* Capture/Compare Preloaded Control */
#define TIM_CCUS                                ((uint16_t)0x0004) /* Capture/Compare Control Update Selection */
#define TIM_CCDS                                ((uint16_t)0x0008) /* Capture/Compare DMA Selection */

#define TIM_MMS                                 ((uint16_t)0x0070) /* MMS[2:0] bits (Master Mode Selection) */
#define TIM_MMS_0                               ((uint16_t)0x0010) /* Bit 0 */
#define TIM_MMS_1                               ((uint16_t)0x0020) /* Bit 1 */
#define TIM_MMS_2                               ((uint16_t)0x0040) /* Bit 2 */

#define TIM_TI1S                                ((uint16_t)0x0080) /* TI1 Selection */
#define TIM_OIS1                                ((uint16_t)0x0100) /* Output Idle state 1 (OC1 output) */
#define TIM_OIS1N                               ((uint16_t)0x0200) /* Output Idle state 1 (OC1N output) */
#define TIM_OIS2                                ((uint16_t)0x0400) /* Output Idle state 2 (OC2 output) */
#define TIM_OIS2N                               ((uint16_t)0x0800) /* Output Idle state 2 (OC2N output) */
#define TIM_OIS3                                ((uint16_t)0x1000) /* Output Idle state 3 (OC3 output) */
#define TIM_OIS3N                               ((uint16_t)0x2000) /* Output Idle state 3 (OC3N output) */
#define TIM_OIS4                                ((uint16_t)0x4000) /* Output Idle state 4 (OC4 output) */

/*******************  Bit definition for TIM_SMCFGR register  *******************/
#define TIM_SMS                                 ((uint16_t)0x0007) /* SMS[2:0] bits (Slave mode selection) */
#define TIM_SMS_0                               ((uint16_t)0x0001) /* Bit 0 */
#define TIM_SMS_1                               ((uint16_t)0x0002) /* Bit 1 */
#define TIM_SMS_2                               ((uint16_t)0x0004) /* Bit 2 */

#define TIM_TS                                  ((uint16_t)0x0070) /* TS[2:0] bits (Trigger selection) */
#define TIM_TS_0                                ((uint16_t)0x0010) /* Bit 0 */
#define TIM_TS_1                                ((uint16_t)0x0020) /* Bit 1 */
#define TIM_TS_2                                ((uint16_t)0x0040) /* Bit 2 */

#define TIM_MSM                                 ((uint16_t)0x0080) /* Master/slave mode */

#define TIM_ETF                                 ((uint16_t)0x0F00) /* ETF[3:0] bits (External trigger filter) */
#define TIM_ETF_0                               ((uint16_t)0x0100) /* Bit 0 */
#define TIM_ETF_1                               ((uint16_t)0x0200) /* Bit 1 */
#define TIM_ETF_2                               ((uint16_t)0x0400) /* Bit 2 */
#define TIM_ETF_3                               ((uint16_t)0x0800) /* Bit 3 */

#define TIM_ETPS                                ((uint16_t)0x3000) /* ETPS[1:0] bits (External trigger prescaler) */
#define TIM_ETPS_0                              ((uint16_t)0x1000) /* Bit 0 */
#define TIM_ETPS_1                              ((uint16_t)0x2000) /* Bit 1 */

#define TIM_ECE                                 ((uint16_t)0x4000) /* External clock enable */
#define TIM_ETP                                 ((uint16_t)0x8000) /* External trigger polarity */

/*******************  Bit definition for TIM_DMAINTENR register  *******************/
#define TIM_UIE                                 ((uint16_t)0x0001) /* Update interrupt enable */
#define TIM_CC1IE                               ((uint16_t)0x0002) /* Capture/Compare 1 interrupt enable */
#define TIM_CC2IE                               ((uint16_t)0x0004) /* Capture/Compare 2 interrupt enable */
#define TIM_CC3IE                               ((uint16_t)0x0008) /* Capture/Compare 3 interrupt enable */
#define TIM_CC4IE                               ((uint16_t)0x0010) /* Capture/Compare 4 interrupt enable */
#define TIM_COMIE                               ((uint16_t)0x0020) /* COM interrupt enable */
#define TIM_TIE                                 ((uint16_t)0x0040) /* Trigger interrupt enable */
#define TIM_BIE                                 ((uint16_t)0x0080) /* Break interrupt enable */
#define TIM_UDE                                 ((uint16_t)0x0100) /* Update DMA request enable */
#define TIM_CC1DE                               ((uint16_t)0x0200) /* Capture/Compare 1 DMA request enable */
#define TIM_CC2DE                               ((uint16_t)0x0400) /* Capture/Compare 2 DMA request enable */
#define TIM_CC3DE                               ((uint16_t)0x0800) /* Capture/Compare 3 DMA request enable */
#define TIM_CC4DE                               ((uint16_t)0x1000) /* Capture/Compare 4 DMA request enable */
#define TIM_COMDE                               ((uint16_t)0x2000) /* COM DMA request enable */
#define TIM_TDE                                 ((uint16_t)0x4000) /* Trigger DMA request enable */

/********************  Bit definition for TIM_INTFR register  ********************/
#define TIM_UIF                                 ((uint16_t)0x0001) /* Update interrupt Flag */
#define TIM_CC1IF                               ((uint16_t)0x0002) /* Capture/Compare 1 interrupt Flag */
#define TIM_CC2IF                               ((uint16_t)0x0004) /* Capture/Compare 2 interrupt Flag */
#define TIM_CC3IF                               ((uint16_t)0x0008) /* Capture/Compare 3 interrupt Flag */
#define TIM_CC4IF                               ((uint16_t)0x0010) /* Capture/Compare 4 interrupt Flag */
#define TIM_COMIF                               ((uint16_t)0x0020) /* COM interrupt Flag */
#define TIM_TIF                                 ((uint16_t)0x0040) /* Trigger interrupt Flag */
#define TIM_BIF                                 ((uint16_t)0x0080) /* Break interrupt Flag */
#define TIM_CC1OF                               ((uint16_t)0x0200) /* Capture/Compare 1 Overcapture Flag */
#define TIM_CC2OF                               ((uint16_t)0x0400) /* Capture/Compare 2 Overcapture Flag */
#define TIM_CC3OF                               ((uint16_t)0x0800) /* Capture/Compare 3 Overcapture Flag */
#define TIM_CC4OF                               ((uint16_t)0x1000) /* Capture/Compare 4 Overcapture Flag */

/*******************  Bit definition for TIM_SWEVGR register  ********************/
#define TIM_UG                                  ((uint8_t)0x01) /* Update Generation */
#define TIM_CC1G                                ((uint8_t)0x02) /* Capture/Compare 1 Generation */
#define TIM_CC2G                                ((uint8_t)0x04) /* Capture/Compare 2 Generation */
#define TIM_CC3G                                ((uint8_t)0x08) /* Capture/Compare 3 Generation */
#define TIM_CC4G                                ((uint8_t)0x10) /* Capture/Compare 4 Generation */
#define TIM_COMG                                ((uint8_t)0x20) /* Capture/Compare Control Update Generation */
#define TIM_TG                                  ((uint8_t)0x40) /* Trigger Generation */
#define TIM_BG                                  ((uint8_t)0x80) /* Break Generation */

/******************  Bit definition for TIM_CHCTLR1 register  *******************/
#define TIM_CC1S                                ((uint16_t)0x0003) /* CC1S[1:0] bits (Capture/Compare 1 Selection) */
#define TIM_CC1S_0                              ((uint16_t)0x0001) /* Bit 0 */
#define TIM_CC1S_1                              ((uint16_t)0x0002) /* Bit 1 */

#define TIM_OC1FE                               ((uint16_t)0x0004) /* Output Compare 1 Fast enable */
#define TIM_OC1PE                               ((uint16_t)0x0008) /* Output Compare 1 Preload enable */

#define TIM_OC1M                                ((uint16_t)0x0070) /* OC1M[2:0] bits (Output Compare 1 Mode) */
#define TIM_OC1M_0                              ((uint16_t)0x0010) /* Bit 0 */
#define TIM_OC1M_1                              ((uint16_t)0x0020) /* Bit 1 */
#define TIM_OC1M_2                              ((uint16_t)0x0040) /* Bit 2 */

#define TIM_OC1CE                               ((uint16_t)0x0080) /* Output Compare 1Clear Enable */

#define TIM_CC2S                                ((uint16_t)0x0300) /* CC2S[1:0] bits (Capture/Compare 2 Selection) */
#define TIM_CC2S_0                              ((uint16_t)0x0100) /* Bit 0 */
#define TIM_CC2S_1                              ((uint16_t)0x0200) /* Bit 1 */

#define TIM_OC2FE                               ((uint16_t)0x0400) /* Output Compare 2 Fast enable */
#define TIM_OC2PE                               ((uint16_t)0x0800) /* Output Compare 2 Preload enable */

#define TIM_OC2M                                ((uint16_t)0x7000) /* OC2M[2:0] bits (Output Compare 2 Mode) */
#define TIM_OC2M_0                              ((uint16_t)0x1000) /* Bit 0 */
#define TIM_OC2M_1                              ((uint16_t)0x2000) /* Bit 1 */
#define TIM_OC2M_2                              ((uint16_t)0x4000) /* Bit 2 */

#define TIM_OC2CE                               ((uint16_t)0x8000) /* Output Compare 2 Clear Enable */

#define TIM_IC1PSC                              ((uint16_t)0x000C) /* IC1PSC[1:0] bits (Input Capture 1 Prescaler) */
#define TIM_IC1PSC_0                            ((uint16_t)0x0004) /* Bit 0 */
#define TIM_IC1PSC_1                            ((uint16_t)0x0008) /* Bit 1 */

#define TIM_IC1F                                ((uint16_t)0x00F0) /* IC1F[3:0] bits (Input Capture 1 Filter) */
#define TIM_IC1F_0                              ((uint16_t)0x0010) /* Bit 0 */
#define TIM_IC1F_1                              ((uint16_t)0x0020) /* Bit 1 */
#define TIM_IC1F_2                              ((uint16_t)0x0040) /* Bit 2 */
#define TIM_IC1F_3                              ((uint16_t)0x0080) /* Bit 3 */

#define TIM_IC2PSC                              ((uint16_t)0x0C00) /* IC2PSC[1:0] bits (Input Capture 2 Prescaler) */
#define TIM_IC2PSC_0                            ((uint16_t)0x0400) /* Bit 0 */
#define TIM_IC2PSC_1                            ((uint16_t)0x0800) /* Bit 1 */

#define TIM_IC2F                                ((uint16_t)0xF000) /* IC2F[3:0] bits (Input Capture 2 Filter) */
#define TIM_IC2F_0                              ((uint16_t)0x1000) /* Bit 0 */
#define TIM_IC2F_1                              ((uint16_t)0x2000) /* Bit 1 */
#define TIM_IC2F_2                              ((uint16_t)0x4000) /* Bit 2 */
#define TIM_IC2F_3                              ((uint16_t)0x8000) /* Bit 3 */

/******************  Bit definition for TIM_CHCTLR2 register  *******************/
#define TIM_CC3S                                ((uint16_t)0x0003) /* CC3S[1:0] bits (Capture/Compare 3 Selection) */
#define TIM_CC3S_0                              ((uint16_t)0x0001) /* Bit 0 */
#define TIM_CC3S_1                              ((uint16_t)0x0002) /* Bit 1 */

#define TIM_OC3FE                               ((uint16_t)0x0004) /* Output Compare 3 Fast enable */
#define TIM_OC3PE                               ((uint16_t)0x0008) /* Output Compare 3 Preload enable */

#define TIM_OC3M                                ((uint16_t)0x0070) /* OC3M[2:0] bits (Output Compare 3 Mode) */
#define TIM_OC3M_0                              ((uint16_t)0x0010) /* Bit 0 */
#define TIM_OC3M_1                              ((uint16_t)0x0020) /* Bit 1 */
#define TIM_OC3M_2                              ((uint16_t)0x0040) /* Bit 2 */

#define TIM_OC3CE                               ((uint16_t)0x0080) /* Output Compare 3 Clear Enable */

#define TIM_CC4S                                ((uint16_t)0x0300) /* CC4S[1:0] bits (Capture/Compare 4 Selection) */
#define TIM_CC4S_0                              ((uint16_t)0x0100) /* Bit 0 */
#define TIM_CC4S_1                              ((uint16_t)0x0200) /* Bit 1 */

#define TIM_OC4FE                               ((uint16_t)0x0400) /* Output Compare 4 Fast enable */
#define TIM_OC4PE                               ((uint16_t)0x0800) /* Output Compare 4 Preload enable */

#define TIM_OC4M                                ((uint16_t)0x7000) /* OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_OC4M_0                              ((uint16_t)0x1000) /* Bit 0 */
#define TIM_OC4M_1                              ((uint16_t)0x2000) /* Bit 1 */
#define TIM_OC4M_2                              ((uint16_t)0x4000) /* Bit 2 */

#define TIM_OC4CE                               ((uint16_t)0x8000) /* Output Compare 4 Clear Enable */

#define TIM_IC3PSC                              ((uint16_t)0x000C) /* IC3PSC[1:0] bits (Input Capture 3 Prescaler) */
#define TIM_IC3PSC_0                            ((uint16_t)0x0004) /* Bit 0 */
#define TIM_IC3PSC_1                            ((uint16_t)0x0008) /* Bit 1 */

#define TIM_IC3F                                ((uint16_t)0x00F0) /* IC3F[3:0] bits (Input Capture 3 Filter) */
#define TIM_IC3F_0                              ((uint16_t)0x0010) /* Bit 0 */
#define TIM_IC3F_1                              ((uint16_t)0x0020) /* Bit 1 */
#define TIM_IC3F_2                              ((uint16_t)0x0040) /* Bit 2 */
#define TIM_IC3F_3                              ((uint16_t)0x0080) /* Bit 3 */

#define TIM_IC4PSC                              ((uint16_t)0x0C00) /* IC4PSC[1:0] bits (Input Capture 4 Prescaler) */
#define TIM_IC4PSC_0                            ((uint16_t)0x0400) /* Bit 0 */
#define TIM_IC4PSC_1                            ((uint16_t)0x0800) /* Bit 1 */

#define TIM_IC4F                                ((uint16_t)0xF000) /* IC4F[3:0] bits (Input Capture 4 Filter) */
#define TIM_IC4F_0                              ((uint16_t)0x1000) /* Bit 0 */
#define TIM_IC4F_1                              ((uint16_t)0x2000) /* Bit 1 */
#define TIM_IC4F_2                              ((uint16_t)0x4000) /* Bit 2 */
#define TIM_IC4F_3                              ((uint16_t)0x8000) /* Bit 3 */

/*******************  Bit definition for TIM_CCER register  *******************/
#define TIM_CC1E                                ((uint16_t)0x0001) /* Capture/Compare 1 output enable */
#define TIM_CC1P                                ((uint16_t)0x0002) /* Capture/Compare 1 output Polarity */
#define TIM_CC1NE                               ((uint16_t)0x0004) /* Capture/Compare 1 Complementary output enable */
#define TIM_CC1NP                               ((uint16_t)0x0008) /* Capture/Compare 1 Complementary output Polarity */
#define TIM_CC2E                                ((uint16_t)0x0010) /* Capture/Compare 2 output enable */
#define TIM_CC2P                                ((uint16_t)0x0020) /* Capture/Compare 2 output Polarity */
#define TIM_CC2NE                               ((uint16_t)0x0040) /* Capture/Compare 2 Complementary output enable */
#define TIM_CC2NP                               ((uint16_t)0x0080) /* Capture/Compare 2 Complementary output Polarity */
#define TIM_CC3E                                ((uint16_t)0x0100) /* Capture/Compare 3 output enable */
#define TIM_CC3P                                ((uint16_t)0x0200) /* Capture/Compare 3 output Polarity */
#define TIM_CC3NE                               ((uint16_t)0x0400) /* Capture/Compare 3 Complementary output enable */
#define TIM_CC3NP                               ((uint16_t)0x0800) /* Capture/Compare 3 Complementary output Polarity */
#define TIM_CC4E                                ((uint16_t)0x1000) /* Capture/Compare 4 output enable */
#define TIM_CC4P                                ((uint16_t)0x2000) /* Capture/Compare 4 output Polarity */
#define TIM_CC4NP                               ((uint16_t)0x8000) /* Capture/Compare 4 Complementary output Polarity */

/*******************  Bit definition for TIM_CNT register  ********************/
#define TIM_CNT                                 ((uint16_t)0xFFFF) /* Counter Value */

/*******************  Bit definition for TIM_PSC register  ********************/
#define TIM_PSC                                 ((uint16_t)0xFFFF) /* Prescaler Value */

/*******************  Bit definition for TIM_ATRLR register  ********************/
#define TIM_ARR                                 ((uint16_t)0xFFFF) /* actual auto-reload Value */

/*******************  Bit definition for TIM_RPTCR register  ********************/
#define TIM_REP                                 ((uint8_t)0xFF) /* Repetition Counter Value */

/*******************  Bit definition for TIM_CH1CVR register  *******************/
#define TIM_CCR1                                ((uint16_t)0xFFFF) /* Capture/Compare 1 Value */

/*******************  Bit definition for TIM_CH2CVR register  *******************/
#define TIM_CCR2                                ((uint16_t)0xFFFF) /* Capture/Compare 2 Value */

/*******************  Bit definition for TIM_CH3CVR register  *******************/
#define TIM_CCR3                                ((uint16_t)0xFFFF) /* Capture/Compare 3 Value */

/*******************  Bit definition for TIM_CH4CVR register  *******************/
#define TIM_CCR4                                ((uint16_t)0xFFFF) /* Capture/Compare 4 Value */

/*******************  Bit definition for TIM_BDTR register  *******************/
#define TIM_DTG                                 ((uint16_t)0x00FF) /* DTG[0:7] bits (Dead-Time Generator set-up) */
#define TIM_DTG_0                               ((uint16_t)0x0001) /* Bit 0 */
#define TIM_DTG_1                               ((uint16_t)0x0002) /* Bit 1 */
#define TIM_DTG_2                               ((uint16_t)0x0004) /* Bit 2 */
#define TIM_DTG_3                               ((uint16_t)0x0008) /* Bit 3 */
#define TIM_DTG_4                               ((uint16_t)0x0010) /* Bit 4 */
#define TIM_DTG_5                               ((uint16_t)0x0020) /* Bit 5 */
#define TIM_DTG_6                               ((uint16_t)0x0040) /* Bit 6 */
#define TIM_DTG_7                               ((uint16_t)0x0080) /* Bit 7 */

#define TIM_LOCK                                ((uint16_t)0x0300) /* LOCK[1:0] bits (Lock Configuration) */
#define TIM_LOCK_0                              ((uint16_t)0x0100) /* Bit 0 */
#define TIM_LOCK_1                              ((uint16_t)0x0200) /* Bit 1 */

#define TIM_OSSI                                ((uint16_t)0x0400) /* Off-State Selection for Idle mode */
#define TIM_OSSR                                ((uint16_t)0x0800) /* Off-State Selection for Run mode */
#define TIM_BKE                                 ((uint16_t)0x1000) /* Break enable */
#define TIM_BKP                                 ((uint16_t)0x2000) /* Break Polarity */
#define TIM_AOE                                 ((uint16_t)0x4000) /* Automatic Output enable */
#define TIM_MOE                                 ((uint16_t)0x8000) /* Main Output enable */

/*******************  Bit definition for TIM_DMACFGR register  ********************/
#define TIM_DBA                                 ((uint16_t)0x001F) /* DBA[4:0] bits (DMA Base Address) */
#define TIM_DBA_0                               ((uint16_t)0x0001) /* Bit 0 */
#define TIM_DBA_1                               ((uint16_t)0x0002) /* Bit 1 */
#define TIM_DBA_2                               ((uint16_t)0x0004) /* Bit 2 */
#define TIM_DBA_3                               ((uint16_t)0x0008) /* Bit 3 */
#define TIM_DBA_4                               ((uint16_t)0x0010) /* Bit 4 */

#define TIM_DBL                                 ((uint16_t)0x1F00) /* DBL[4:0] bits (DMA Burst Length) */
#define TIM_DBL_0                               ((uint16_t)0x0100) /* Bit 0 */
#define TIM_DBL_1                               ((uint16_t)0x0200) /* Bit 1 */
#define TIM_DBL_2                               ((uint16_t)0x0400) /* Bit 2 */
#define TIM_DBL_3                               ((uint16_t)0x0800) /* Bit 3 */
#define TIM_DBL_4                               ((uint16_t)0x1000) /* Bit 4 */

/*******************  Bit definition for TIM_DMAADR register  *******************/
#define TIM_DMAR_DMAB                           ((uint16_t)0xFFFF) /* DMA register for burst accesses */

/******************************************************************************/
/*         Universal Synchronous Asynchronous Receiver Transmitter            */
/******************************************************************************/

/*******************  Bit definition for USART_STATR register  *******************/
#define USART_STATR_PE                          ((uint16_t)0x0001) /* Parity Error */
#define USART_STATR_FE                          ((uint16_t)0x0002) /* Framing Error */
#define USART_STATR_NE                          ((uint16_t)0x0004) /* Noise Error Flag */
#define USART_STATR_ORE                         ((uint16_t)0x0008) /* OverRun Error */
#define USART_STATR_IDLE                        ((uint16_t)0x0010) /* IDLE line detected */
#define USART_STATR_RXNE                        ((uint16_t)0x0020) /* Read Data Register Not Empty */
#define USART_STATR_TC                          ((uint16_t)0x0040) /* Transmission Complete */
#define USART_STATR_TXE                         ((uint16_t)0x0080) /* Transmit Data Register Empty */
#define USART_STATR_LBD                         ((uint16_t)0x0100) /* LIN Break Detection Flag */
#define USART_STATR_CTS                         ((uint16_t)0x0200) /* CTS Flag */

/*******************  Bit definition for USART_DATAR register  *******************/
#define USART_DATAR_DR                          ((uint16_t)0x01FF) /* Data value */

/******************  Bit definition for USART_BRR register  *******************/
#define USART_BRR_DIV_Fraction                  ((uint16_t)0x000F) /* Fraction of USARTDIV */
#define USART_BRR_DIV_Mantissa                  ((uint16_t)0xFFF0) /* Mantissa of USARTDIV */

/******************  Bit definition for USART_CTLR1 register  *******************/
#define USART_CTLR1_SBK                         ((uint16_t)0x0001) /* Send Break */
#define USART_CTLR1_RWU                         ((uint16_t)0x0002) /* Receiver wakeup */
#define USART_CTLR1_RE                          ((uint16_t)0x0004) /* Receiver Enable */
#define USART_CTLR1_TE                          ((uint16_t)0x0008) /* Transmitter Enable */
#define USART_CTLR1_IDLEIE                      ((uint16_t)0x0010) /* IDLE Interrupt Enable */
#define USART_CTLR1_RXNEIE                      ((uint16_t)0x0020) /* RXNE Interrupt Enable */
#define USART_CTLR1_TCIE                        ((uint16_t)0x0040) /* Transmission Complete Interrupt Enable */
#define USART_CTLR1_TXEIE                       ((uint16_t)0x0080) /* PE Interrupt Enable */
#define USART_CTLR1_PEIE                        ((uint16_t)0x0100) /* PE Interrupt Enable */
#define USART_CTLR1_PS                          ((uint16_t)0x0200) /* Parity Selection */
#define USART_CTLR1_PCE                         ((uint16_t)0x0400) /* Parity Control Enable */
#define USART_CTLR1_WAKE                        ((uint16_t)0x0800) /* Wakeup method */
#define USART_CTLR1_M                           ((uint16_t)0x1000) /* Word length */
#define USART_CTLR1_UE                          ((uint16_t)0x2000) /* USART Enable */
#define USART_CTLR1_OVER8                       ((uint16_t)0x8000) /* USART Oversmapling 8-bits */

/******************  Bit definition for USART_CTLR2 register  *******************/
#define USART_CTLR2_ADD                         ((uint16_t)0x000F) /* Address of the USART node */
#define USART_CTLR2_LBDL                        ((uint16_t)0x0020) /* LIN Break Detection Length */
#define USART_CTLR2_LBDIE                       ((uint16_t)0x0040) /* LIN Break Detection Interrupt Enable */
#define USART_CTLR2_LBCL                        ((uint16_t)0x0100) /* Last Bit Clock pulse */
#define USART_CTLR2_CPHA                        ((uint16_t)0x0200) /* Clock Phase */
#define USART_CTLR2_CPOL                        ((uint16_t)0x0400) /* Clock Polarity */
#define USART_CTLR2_CLKEN                       ((uint16_t)0x0800) /* Clock Enable */

#define USART_CTLR2_STOP                        ((uint16_t)0x3000) /* STOP[1:0] bits (STOP bits) */
#define USART_CTLR2_STOP_0                      ((uint16_t)0x1000) /* Bit 0 */
#define USART_CTLR2_STOP_1                      ((uint16_t)0x2000) /* Bit 1 */

#define USART_CTLR2_LINEN                       ((uint16_t)0x4000) /* LIN mode enable */

/******************  Bit definition for USART_CTLR3 register  *******************/
#define USART_CTLR3_EIE                         ((uint16_t)0x0001) /* Error Interrupt Enable */
#define USART_CTLR3_IREN                        ((uint16_t)0x0002) /* IrDA mode Enable */
#define USART_CTLR3_IRLP                        ((uint16_t)0x0004) /* IrDA Low-Power */
#define USART_CTLR3_HDSEL                       ((uint16_t)0x0008) /* Half-Duplex Selection */
#define USART_CTLR3_NACK                        ((uint16_t)0x0010) /* Smartcard NACK enable */
#define USART_CTLR3_SCEN                        ((uint16_t)0x0020) /* Smartcard mode enable */
#define USART_CTLR3_DMAR                        ((uint16_t)0x0040) /* DMA Enable Receiver */
#define USART_CTLR3_DMAT                        ((uint16_t)0x0080) /* DMA Enable Transmitter */
#define USART_CTLR3_RTSE                        ((uint16_t)0x0100) /* RTS Enable */
#define USART_CTLR3_CTSE                        ((uint16_t)0x0200) /* CTS Enable */
#define USART_CTLR3_CTSIE                       ((uint16_t)0x0400) /* CTS Interrupt Enable */
#define USART_CTLR3_ONEBIT                      ((uint16_t)0x0800) /* One Bit method */

/******************  Bit definition for USART_GPR register  ******************/
#define USART_GPR_PSC                           ((uint16_t)0x00FF) /* PSC[7:0] bits (Prescaler value) */
#define USART_GPR_PSC_0                         ((uint16_t)0x0001) /* Bit 0 */
#define USART_GPR_PSC_1                         ((uint16_t)0x0002) /* Bit 1 */
#define USART_GPR_PSC_2                         ((uint16_t)0x0004) /* Bit 2 */
#define USART_GPR_PSC_3                         ((uint16_t)0x0008) /* Bit 3 */
#define USART_GPR_PSC_4                         ((uint16_t)0x0010) /* Bit 4 */
#define USART_GPR_PSC_5                         ((uint16_t)0x0020) /* Bit 5 */
#define USART_GPR_PSC_6                         ((uint16_t)0x0040) /* Bit 6 */
#define USART_GPR_PSC_7                         ((uint16_t)0x0080) /* Bit 7 */

#define USART_GPR_GT                            ((uint16_t)0xFF00) /* Guard time value */

/******************************************************************************/
/*                            Window WATCHDOG                                 */
/******************************************************************************/

/*******************  Bit definition for WWDG_CTLR register  ********************/
#define WWDG_CTLR_T                             ((uint8_t)0x7F) /* T[6:0] bits (7-Bit counter (MSB to LSB)) */
#define WWDG_CTLR_T0                            ((uint8_t)0x01) /* Bit 0 */
#define WWDG_CTLR_T1                            ((uint8_t)0x02) /* Bit 1 */
#define WWDG_CTLR_T2                            ((uint8_t)0x04) /* Bit 2 */
#define WWDG_CTLR_T3                            ((uint8_t)0x08) /* Bit 3 */
#define WWDG_CTLR_T4                            ((uint8_t)0x10) /* Bit 4 */
#define WWDG_CTLR_T5                            ((uint8_t)0x20) /* Bit 5 */
#define WWDG_CTLR_T6                            ((uint8_t)0x40) /* Bit 6 */

#define WWDG_CTLR_WDGA                          ((uint8_t)0x80) /* Activation bit */

/*******************  Bit definition for WWDG_CFGR register  *******************/
#define WWDG_CFGR_W                             ((uint16_t)0x007F) /* W[6:0] bits (7-bit window value) */
#define WWDG_CFGR_W0                            ((uint16_t)0x0001) /* Bit 0 */
#define WWDG_CFGR_W1                            ((uint16_t)0x0002) /* Bit 1 */
#define WWDG_CFGR_W2                            ((uint16_t)0x0004) /* Bit 2 */
#define WWDG_CFGR_W3                            ((uint16_t)0x0008) /* Bit 3 */
#define WWDG_CFGR_W4                            ((uint16_t)0x0010) /* Bit 4 */
#define WWDG_CFGR_W5                            ((uint16_t)0x0020) /* Bit 5 */
#define WWDG_CFGR_W6                            ((uint16_t)0x0040) /* Bit 6 */

#define WWDG_CFGR_WDGTB                         ((uint16_t)0x0180) /* WDGTB[1:0] bits (Timer Base) */
#define WWDG_CFGR_WDGTB0                        ((uint16_t)0x0080) /* Bit 0 */
#define WWDG_CFGR_WDGTB1                        ((uint16_t)0x0100) /* Bit 1 */

#define WWDG_CFGR_EWI                           ((uint16_t)0x0200) /* Early Wakeup Interrupt */

/*******************  Bit definition for WWDG_STATR register  ********************/
#define WWDG_STATR_EWIF                         ((uint8_t)0x01) /* Early Wakeup Interrupt Flag */

/******************************************************************************/
/*                          ENHANCED FUNNCTION                                */
/******************************************************************************/

/****************************  Enhanced register  *****************************/
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define EXTEN_USBD_LS                           ((uint32_t)0x00000001) /* Bit 0 */
#define EXTEN_USBD_PU_EN                        ((uint32_t)0x00000002) /* Bit 1 */
#define EXTEN_ETH_10M_EN                        ((uint32_t)0x00000004) /* Bit 2 */
#define EXTEN_PLL_HSI_PRE                       ((uint32_t)0x00000010) /* Bit 4 */
#endif
#define EXTEN_LOCKUP_EN                         ((uint32_t)0x00000040) /* Bit 5 */
#define EXTEN_LOCKUP_RSTF                       ((uint32_t)0x00000080) /* Bit 7 */

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define EXTEN_ULLDO_TRIM                        ((uint32_t)0x00000300) /* ULLDO_TRIM[1:0] bits */
#define EXTEN_ULLDO_TRIM0                       ((uint32_t)0x00000100) /* Bit 0 */
#define EXTEN_ULLDO_TRIM1                       ((uint32_t)0x00000200) /* Bit 1 */
#endif

#if !defined(CH32V10x)

#define EXTEN_LDO_TRIM                          ((uint32_t)0x00000400) /* Bit 10 */
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTEN_LDO_TRIM0                         ((uint32_t)0x00000400) /* Bit 0 */
#define EXTEN_LDO_TRIM1                         ((uint32_t)0x00000800) /* Bit 1 */
#endif

#else

#define EXTEN_IDO_TRIM                          ((uint32_t)0x00000400) /* Bit 10 */
#define EXTEN_WRITE_EN                          ((uint32_t)0x00004000) /* Bit 14 */
#define EXTEN_SHORT_WAKE                        ((uint32_t)0x00008000) /* Bit 15 */

#define EXTEN_FLASH_CLK_TRIM                    ((uint32_t)0x00070000) /* FLASH_CLK_TRIM[2:0] bits */
#define EXTEN_FLASH_CLK_TRIM0                   ((uint32_t)0x00010000) /* Bit 0 */
#define EXTEN_FLASH_CLK_TRIM1                   ((uint32_t)0x00020000) /* Bit 1 */
#define EXTEN_FLASH_CLK_TRIM2                   ((uint32_t)0x00040000) /* Bit 2 */

#endif

#if defined(CH32V003)
#define EXTEN_OPA_EN                            ((uint32_t)0x00010000)
#define EXTEN_OPA_NSEL                          ((uint32_t)0x00020000)
#define EXTEN_OPA_PSEL                          ((uint32_t)0x00040000)
#endif

#if defined(CH32V20x) || defined(CH32V30x)
/******************************************************************************/
/*                                  DVP                                       */
/******************************************************************************/

/*******************  Bit definition for DVP_CR0 register  ********************/
#define RB_DVP_ENABLE                           0x01  // RW, DVP enable
#define RB_DVP_V_POLAR                          0x02  // RW, DVP VSYNC polarity control: 1 = invert, 0 = not invert
#define RB_DVP_H_POLAR                          0x04  // RW, DVP HSYNC polarity control: 1 = invert, 0 = not invert
#define RB_DVP_P_POLAR                          0x08  // RW, DVP PCLK polarity control: 1 = invert, 0 = not invert
#define RB_DVP_MSK_DAT_MOD                      0x30
#define RB_DVP_D8_MOD                           0x00  // RW, DVP 8bits data mode
#define RB_DVP_D10_MOD                          0x10  // RW, DVP 10bits data mode
#define RB_DVP_D12_MOD                          0x20  // RW, DVP 12bits data mode
#define RB_DVP_JPEG                             0x40  // RW, DVP JPEG mode

/*******************  Bit definition for DVP_CR1 register  ********************/
#define RB_DVP_DMA_EN                           0x01  // RW, DVP dma enable
#define RB_DVP_ALL_CLR                          0x02  // RW, DVP all clear, high action
#define RB_DVP_RCV_CLR                          0x04  // RW, DVP receive logic clear, high action
#define RB_DVP_BUF_TOG                          0x08  // RW, DVP bug toggle by software, write 1 to toggle, ignored writing 0
#define RB_DVP_CM                               0x10  // RW, DVP capture mode
#define RB_DVP_CROP                             0x20  // RW, DVP Crop feature enable
#define RB_DVP_FCRC                             0xC0  // RW, DVP frame capture rate control:
#define DVP_RATE_100P                           0x00  //00 = every frame captured (100%)
#define DVP_RATE_50P                            0x40  //01 = every alternate frame captured (50%)
#define DVP_RATE_25P                            0x80  //10 = one frame in four frame captured (25%)

/*******************  Bit definition for DVP_IER register  ********************/
#define RB_DVP_IE_STR_FRM                       0x01  // RW, DVP frame start interrupt enable
#define RB_DVP_IE_ROW_DONE                      0x02  // RW, DVP row received done interrupt enable
#define RB_DVP_IE_FRM_DONE                      0x04  // RW, DVP frame received done interrupt enable
#define RB_DVP_IE_FIFO_OV                       0x08  // RW, DVP receive fifo overflow interrupt enable
#define RB_DVP_IE_STP_FRM                       0x10  // RW, DVP frame stop interrupt enable

/*******************  Bit definition for DVP_IFR register  ********************/
#define RB_DVP_IF_STR_FRM                       0x01  // RW1, interrupt flag for DVP frame start
#define RB_DVP_IF_ROW_DONE                      0x02  // RW1, interrupt flag for DVP row receive done
#define RB_DVP_IF_FRM_DONE                      0x04  // RW1, interrupt flag for DVP frame receive done
#define RB_DVP_IF_FIFO_OV                       0x08  // RW1, interrupt flag for DVP receive fifo overflow
#define RB_DVP_IF_STP_FRM                       0x10  // RW1, interrupt flag for DVP frame stop

/*******************  Bit definition for DVP_STATUS register  ********************/
#define RB_DVP_FIFO_RDY                         0x01  // RO, DVP receive fifo ready
#define RB_DVP_FIFO_FULL                        0x02  // RO, DVP receive fifo full
#define RB_DVP_FIFO_OV                          0x04  // RO, DVP receive fifo overflow
#define RB_DVP_MSK_FIFO_CNT                     0x70  // RO, DVP receive fifo count

#if defined(CH32V20x)
/******************************************************************************/
/*                                  ETH10M                                    */
/******************************************************************************/
/* ETH register */
#define R8_ETH_EIE              (*((volatile uint8_t *)(0x40028000+3))) /* Interrupt Enable Register */
#define  RB_ETH_EIE_INTIE       0x80                  /* RW interrupt enable*/
#define  RB_ETH_EIE_RXIE        0x40                  /* RW Receive complete interrupt enable */
#define  RB_ETH_EIE_LINKIE      0x10                  /* RW Link Change Interrupt Enable */
#define  RB_ETH_EIE_TXIE        0x08                  /* RW send complete interrupt enable */
#define  RB_ETH_EIE_R_EN50      0x04                  /* RW TX 50�� resistor adjustment. 1: On-chip 50�� connected 0: On-chip 50�� disconnected */
#define  RB_ETH_EIE_TXERIE      0x02                  /* RW Transmit Error Interrupt Enable */
#define  RB_ETH_EIE_RXERIE      0x01                  /* RW1 receive error flag */
#define R8_ETH_EIR              (*((volatile uint8_t *)(0x40028000+4))) /* Interrupt Flag Register */
#define  RB_ETH_EIR_RXIF        0x40                  /* RW1 Receive complete flag */
#define  RB_ETH_EIR_LINKIF      0x10                  /* RW1 Link Change Flag */
#define  RB_ETH_EIR_TXIF        0x08                  /* RW1 Link Change Flag */
#define  RB_ETH_EIR_TXERIF      0x02                  /* RW1 send error flag */
#define  RB_ETH_EIR_RXERIF      0x01                  /* RW1 receive error flag */
#define R8_ETH_ESTAT            (*((volatile uint8_t *)(0x40028000+5))) /* status register */
#define  RB_ETH_ESTAT_INT       0x80                  /* RW1 interrupt */
#define  RB_ETH_ESTAT_BUFER     0x40                  /* RW1 Buffer error */
#define  RB_ETH_ESTAT_RXCRCER   0x20                  /* RO receive crc error */
#define  RB_ETH_ESTAT_RXNIBBLE  0x10                  /* RO receives nibble error */
#define  RB_ETH_ESTAT_RXMORE    0x08                  /* RO receives more than maximum packets */
#define  RB_ETH_ESTAT_RXBUSY    0x04                  /* RO receive busy */
#define  RB_ETH_ESTAT_TXABRT    0x02                  /* RO send interrupted by mcu */
#define R8_ETH_ECON2            (*((volatile uint8_t *)(0x40028000+6))) /* ETH PHY Analog Block Control Register */
#define  RB_ETH_ECON2_RX        0x0E                  /* 011b must be written */
#define  RB_ETH_ECON2_TX        0x01
#define  RB_ETH_ECON2_MUST      0x06                  /* 011b must be written */
#define R8_ETH_ECON1            (*((volatile uint8_t *)(0x40028000+7))) /* Transceiver Control Register */
#define  RB_ETH_ECON1_TXRST     0x80                  /* RW Send module reset */
#define  RB_ETH_ECON1_RXRST     0x40                  /* RW Receiver module reset */
#define  RB_ETH_ECON1_TXRTS     0x08                  /* RW The transmission starts, and it is automatically cleared after the transmission is completed. */
#define  RB_ETH_ECON1_RXEN      0x04                  /* RW Receive is enabled, when cleared, the error flag RXERIF will change to 1 if it is receiving */

#define R32_ETH_TX              (*((volatile uint32_t *)(0x40028000+8))) /* send control */
#define R16_ETH_ETXST           (*((volatile uint16_t *)(0x40028000+8))) /* RW Send DMA buffer start address */
#define R16_ETH_ETXLN           (*((volatile uint16_t *)(0x40028000+0xA))) /* RW send length */
#define R32_ETH_RX              (*((volatile uint32_t *)(0x40028000+0xC))) /* receive control */
#define R16_ETH_ERXST           (*((volatile uint16_t *)(0x40028000+0xC))) /* RW Receive DMA buffer start address */
#define R16_ETH_ERXLN           (*((volatile uint16_t *)(0x40028000+0xE))) /* RO receive length */

#define R32_ETH_HTL             (*((volatile uint32_t *)(0x40028000+0x10)))
#define R8_ETH_EHT0             (*((volatile uint8_t *)(0x40028000+0x10))) /* RW Hash Table Byte0 */
#define R8_ETH_EHT1             (*((volatile uint8_t *)(0x40028000+0x11))) /* RW Hash Table Byte1 */
#define R8_ETH_EHT2             (*((volatile uint8_t *)(0x40028000+0x12))) /* RW Hash Table Byte2 */
#define R8_ETH_EHT3             (*((volatile uint8_t *)(0x40028000+0x13))) /* RW Hash Table Byte3 */
#define R32_ETH_HTH             (*((volatile uint32_t *)(0x40028000+0x14)))
#define R8_ETH_EHT4             (*((volatile uint8_t *)(0x40028000+0x14))) /* RW Hash Table Byte4 */
#define R8_ETH_EHT5             (*((volatile uint8_t *)(0x40028000+0x15))) /* RW Hash Table Byte5 */
#define R8_ETH_EHT6             (*((volatile uint8_t *)(0x40028000+0x16))) /* RW Hash Table Byte6 */
#define R8_ETH_EHT7             (*((volatile uint8_t *)(0x40028000+0x17))) /* RW Hash Table Byte7 */

#define R32_ETH_MACON           (*((volatile uint32_t *)(0x40028000+0x18)))
#define R8_ETH_ERXFCON          (*((volatile uint8_t *)(0x40028000+0x18))) /* Received Packet Filtering Control Register */
/* RW 0=Do not enable this filter condition, 1=When ANDOR=1,
target address mismatch will be filtered, when ANDOR=0, target address match will be accepted */
#define  RB_ETH_ERXFCON_UCEN    0x80
#define  RB_ETH_ERXFCON_CRCEN   0x20
#define  RB_ETH_ERXFCON_EN      0x10
#define  RB_ETH_ERXFCON_MPEN    0x08
#define  RB_ETH_ERXFCON_HTEN    0x04
#define  RB_ETH_ERXFCON_MCEN    0x02
#define  RB_ETH_ERXFCON_BCEN    0x01
#define R8_ETH_MACON1           (*((volatile uint8_t *)(0x40028000+0x19))) /* Mac flow control registers */
/* RW When FULDPX=0 is invalid, when FULDPX=1, 11=send 0 timer pause frame,
then stop sending, 10=send pause frame periodically, 01=send pause frame once, then stop sending, 00=stop sending pause frame */
#define  RB_ETH_MACON1_FCEN     0x30
#define  RB_ETH_MACON1_TXPAUS   0x08                  /* RW Send pause frame enable*/
#define  RB_ETH_MACON1_RXPAUS   0x04                  /* RW Receive pause frame enable */
#define  RB_ETH_MACON1_PASSALL  0x02                  /* RW 1=Unfiltered control frames will be written to the buffer, 0=Control frames will be filtered */
#define  RB_ETH_MACON1_MARXEN   0x01                  /* RW MAC layer receive enable */
#define R8_ETH_MACON2           (*((volatile uint8_t *)(0x40028000+0x1A))) /* Mac Layer Packet Control Register */
#define  RB_ETH_MACON2_PADCFG   0xE0                  /* RW Short Packet Padding Settings */
#define  RB_ETH_MACON2_TXCRCEN  0x10                  /* RW Send to add crc, if you need to add crc in PADCFG, this position is 1 */
#define  RB_ETH_MACON2_PHDREN   0x08                  /* RW Special 4 bytes do not participate in crc check */
#define  RB_ETH_MACON2_HFRMEN   0x04                  /* RW Allow jumbo frames */
#define  RB_ETH_MACON2_FULDPX   0x01                  /* RW full duplex */
#define R8_ETH_MABBIPG          (*((volatile uint8_t *)(0x40028000+0x1B))) /* Minimum Interpacket Interval Register */
#define  RB_ETH_MABBIPG_MABBIPG 0x7F                  /* RW Minimum number of bytes between packets */

#define R32_ETH_TIM             (*((volatile uint32_t *)(0x40028000+0x1C)))
#define R16_ETH_EPAUS           (*((volatile uint16_t *)(0x40028000+0x1C))) /* RW Flow Control Pause Frame Time Register */
#define R16_ETH_MAMXFL          (*((volatile uint16_t *)(0x40028000+0x1E))) /* RW Maximum Received Packet Length Register */
#define R16_ETH_MIRD            (*((volatile uint16_t *)(0x40028000+0x20))) /* RW MII read data register */

#define R32_ETH_MIWR            (*((volatile uint32_t *)(0x40028000+0x24)))
#define R8_ETH_MIREGADR         (*((volatile uint8_t *)(0x40028000+0x24))) /* MII address register*/
#define  RB_ETH_MIREGADR_MASK   0x1F                  /* RW PHY register address mask */
#define R8_ETH_MISTAT           (*((volatile uint8_t *)(0x40028000+0x25))) /* RW PHY register address mask */
//#define  RB_ETH_MIREGADR_MIIWR  0x20                  /* WO MII write command */
#define R16_ETH_MIWR            (*((volatile uint16_t *)(0x40028000+0x26))) /* WO MII Write Data Register */
#define R32_ETH_MAADRL          (*((volatile uint32_t *)(0x40028000+0x28))) /* RW MAC 1-4 */
#define R8_ETH_MAADRL1          (*((volatile uint8_t *)(0x40028000+0x28))) /* RW MAC 1 */
#define R8_ETH_MAADRL2          (*((volatile uint8_t *)(0x40028000+0x29))) /* RW MAC 2 */
#define R8_ETH_MAADRL3          (*((volatile uint8_t *)(0x40028000+0x2A))) /* RW MAC 3 */
#define R8_ETH_MAADRL4          (*((volatile uint8_t *)(0x40028000+0x2B))) /* RW MAC 4 */
#define R16_ETH_MAADRH          (*((volatile uint16_t *)(0x40028000+0x2C))) /* RW MAC 5-6 */
#define R8_ETH_MAADRL5          (*((volatile uint8_t *)(0x40028000+0x2C))) /* RW MAC 4 */
#define R8_ETH_MAADRL6          (*((volatile uint8_t *)(0x40028000+0x2D))) /* RW MAC 4 */

//PHY address
#define PHY_BMCR                0x00                                            /* Control Register */
#define PHY_BMSR                0x01                                            /* Status Register */
#define PHY_ANAR                0x04                                            /* Auto-Negotiation Advertisement Register */
#define PHY_ANLPAR              0x05                                            /* Auto-Negotiation Link Partner Base  Page Ability Register*/
#define PHY_ANER                0x06                                            /* Auto-Negotiation Expansion Register */
#define PHY_MDIX                0x1e                                            /* Custom MDIX Mode Register */
//Custom MDIX Mode Register  @PHY_MDIX
#define PN_NORMAL               0x04                                            /* Analog p, n polarity selection */
#define MDIX_MODE_MASK          0x03                                            /* mdix settings */
#define MDIX_MODE_AUTO          0x00                                            /*  */
#define MDIX_MODE_MDIX          0x01
#define MDIX_MODE_MDI           0x02
//ECON2 test mode, to be determined
#define RX_VCM_MODE_0
#define RX_VCM_MODE_1
#define RX_VCM_MODE_2
#define RX_VCM_MODE_3
//RX reference voltage value setting  @RX_REF
#define RX_REF_25mV             (0<<2)                                          /* 25mV */
#define RX_REF_49mV             (1<<2)                                          /* 49mV */
#define RX_REF_74mV             (2<<2)                                          /* 74mV */
#define RX_REF_98mV             (3<<2)                                          /* 98mV */
#define RX_REF_123mV            (4<<2)                                          /* 123mV */
#define RX_REF_148mV            (5<<2)                                          /* 148mV */
#define RX_REF_173mV            (6<<2)                                          /* 173mV */
#define RX_REF_198mV            (7<<2)                                          /* 198mV */
//TX DRIVER Bias Current  @TX_AMP
#define TX_AMP_0                (0<<0)                                          /* 43mA   / 14.5mA   (1.4V/0.7V) */
#define TX_AMP_1                (1<<0)                                          /* 53.1mA / 18mA     (1.8V/0.9V) */
#define TX_AMP_2                (2<<0)                                          /* 75.6mA / 25.6mA   (2.6V/1.3V) */
#define TX_AMP_3                (3<<0)                                          /* 122mA  / 41.45mA  (4.1V/2.3V) */
//FCEN pause frame control      @FCEN
#define FCEN_0_TIMER            (3<<4)                                          /* Send a 0 timer pause frame, then stop sending */
#define FCEN_CYCLE              (2<<4)                                          /* Periodically send pause frames */
#define FCEN_ONCE               (1<<4)                                          /* Send pause frame once, then stop sending */
#define FCEN_STOP               (0<<4)                                          /* Stop sending pause frames */
//PADCFG short packet control  @PADCFG
#define PADCFG_AUTO_0           (7<<5)                                          /* All short packets are filled with 00h to 64 bytes, then 4 bytes crc */
#define PADCFG_NO_ACT_0         (6<<5)                                          /* No padding for short packets */
/* The detected VLAN network packet whose field is 8100h is automatically filled
with 00h to 64 bytes, otherwise the short packet is filled with 60 bytes of 0, and then 4 bytes of crc after filling */
#define PADCFG_DETE_AUTO        (5<<5)
#define PADCFG_NO_ACT_1         (4<<5)                                          /* No padding for short packets */
#define PADCFG_AUTO_1           (3<<5)                                          /* All short packets are filled with 00h to 64 bytes, then 4 bytes crc */
#define PADCFG_NO_ACT_2         (2<<5)                                          /* No padding for short packets */
#define PADCFG_AUTO_3           (1<<5)                                          /* All short packets are filled with 00h to 60 bytes, and then 4 bytes crc */
#define PADCFG_NO_ACT_3         (0<<5)                                          /* No padding for short packets */

/* Bit or field definition for PHY basic status register */
#define PHY_Linked_Status       ((uint16_t)0x0004)      /* Valid link established */

#define PHY_Reset                               ((uint16_t)0x8000)      /* PHY Reset */

#define PHY_AutoNego_Complete                   ((uint16_t)0x0020)      /* Auto-Negotioation process completed */

//MII control
#define  RB_ETH_MIREGADR_MIIWR  0x20                                            /* WO MII write command */
#define  RB_ETH_MIREGADR_MIRDL  0x1f                                            /* RW PHY register address */

#endif // defined(CH32V20x)
#endif // defined(CH32V20x) || defined(CH32V30x)

#ifdef __cplusplus
}
#endif

#endif /* __CH32V00x_H */

/*
 * This file contains the contents of various parts of the evt.
 *
 * The collection of this file was generated by cnlohr, 2023-02-18
 * and AlexanderMandera, 2023-06-23
 *
 * Contents subject to below copyright where applicable by law.
 *
 * (IANAL, BUT Because it is an interface, it is unlikely protected by copyright)
 *
 *********************************** (C) COPYRIGHT *******************************
 * File Name          : ------------------
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2020/08/08
 * Description        : Library configuration file.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __CH32V00x_CONF_H
#define __CH32V00x_CONF_H


#ifdef __cplusplus
extern "C" {
#endif



/* ch32v00x_gpio.c -----------------------------------------------------------*/
/* MASK */
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ECR_PORTPINCONFIG_MASK    ((uint16_t)0xFF80)
#endif
#define LSB_MASK                  ((uint16_t)0xFFFF)
#define DBGAFR_POSITION_MASK      ((uint32_t)0x000F0000)
#define DBGAFR_SDI_MASK           ((uint32_t)0xF8FFFFFF)
#define DBGAFR_LOCATION_MASK      ((uint32_t)0x00200000)
#define DBGAFR_NUMBITS_MASK       ((uint32_t)0x00100000)


/* ch32v00x_adc.c ------------------------------------------------------------*/

/* ADC DISCNUM mask */
#define CTLR1_DISCNUM_Reset              ((uint32_t)0xFFFF1FFF)

/* ADC DISCEN mask */
#define CTLR1_DISCEN_Set                 ((uint32_t)0x00000800)
#define CTLR1_DISCEN_Reset               ((uint32_t)0xFFFFF7FF)

/* ADC JAUTO mask */
#define CTLR1_JAUTO_Set                  ((uint32_t)0x00000400)
#define CTLR1_JAUTO_Reset                ((uint32_t)0xFFFFFBFF)

/* ADC JDISCEN mask */
#define CTLR1_JDISCEN_Set                ((uint32_t)0x00001000)
#define CTLR1_JDISCEN_Reset              ((uint32_t)0xFFFFEFFF)

/* ADC AWDCH mask */
#define CTLR1_AWDCH_Reset                ((uint32_t)0xFFFFFFE0)

/* ADC Analog watchdog enable mode mask */
#define CTLR1_AWDMode_Reset              ((uint32_t)0xFF3FFDFF)

///* CTLR1 register Mask */
//Editor's Note: Overloaded Definition
#define ADC_CTLR1_CLEAR_Mask                 ((uint32_t)0xFFF0FEFF)

/* ADC ADON mask */
#define CTLR2_ADON_Set                   ((uint32_t)0x00000001)
#define CTLR2_ADON_Reset                 ((uint32_t)0xFFFFFFFE)

/* ADC DMA mask */
#define CTLR2_DMA_Set                    ((uint32_t)0x00000100)
#define CTLR2_DMA_Reset                  ((uint32_t)0xFFFFFEFF)

/* ADC RSTCAL mask */
#define CTLR2_RSTCAL_Set                 ((uint32_t)0x00000008)

/* ADC CAL mask */
#define CTLR2_CAL_Set                    ((uint32_t)0x00000004)

/* ADC SWSTART mask */
#define CTLR2_SWSTART_Set                ((uint32_t)0x00400000)

/* ADC EXTTRIG mask */
#define CTLR2_EXTTRIG_Set                ((uint32_t)0x00100000)
#define CTLR2_EXTTRIG_Reset              ((uint32_t)0xFFEFFFFF)

/* ADC Software start mask */
#define CTLR2_EXTTRIG_SWSTART_Set        ((uint32_t)0x00500000)
#define CTLR2_EXTTRIG_SWSTART_Reset      ((uint32_t)0xFFAFFFFF)

/* ADC JEXTSEL mask */
#define CTLR2_JEXTSEL_Reset              ((uint32_t)0xFFFF8FFF)

/* ADC JEXTTRIG mask */
#define CTLR2_JEXTTRIG_Set               ((uint32_t)0x00008000)
#define CTLR2_JEXTTRIG_Reset             ((uint32_t)0xFFFF7FFF)

/* ADC JSWSTART mask */
#define CTLR2_JSWSTART_Set               ((uint32_t)0x00200000)

/* ADC injected software start mask */
#define CTLR2_JEXTTRIG_JSWSTART_Set      ((uint32_t)0x00208000)
#define CTLR2_JEXTTRIG_JSWSTART_Reset    ((uint32_t)0xFFDF7FFF)

/* ADC TSPD mask */
#define CTLR2_TSVREFE_Set                ((uint32_t)0x00800000)
#define CTLR2_TSVREFE_Reset              ((uint32_t)0xFF7FFFFF)

/* CTLR2 register Mask */
#define CTLR2_CLEAR_Mask                 ((uint32_t)0xFFF1F7FD)

/* ADC SQx mask */
#define RSQR3_SQ_Set                     ((uint32_t)0x0000001F)
#define RSQR2_SQ_Set                     ((uint32_t)0x0000001F)
#define RSQR1_SQ_Set                     ((uint32_t)0x0000001F)

/* RSQR1 register Mask */
#define RSQR1_CLEAR_Mask                 ((uint32_t)0xFF0FFFFF)

/* ADC JSQx mask */
#define ISQR_JSQ_Set                     ((uint32_t)0x0000001F)

/* ADC JL mask */
#define ISQR_JL_Set                      ((uint32_t)0x00300000)
#define ISQR_JL_Reset                    ((uint32_t)0xFFCFFFFF)

/* ADC SMPx mask */
#define SAMPTR1_SMP_Set                  ((uint32_t)0x00000007)
#define SAMPTR2_SMP_Set                  ((uint32_t)0x00000007)

/* ADC IDATARx registers offset */
#define IDATAR_Offset                    ((uint8_t)0x28)

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* ADC1 RDATAR register base address */
#define RDATAR_ADDRESS                   ((uint32_t)0x4001244C)

/* ch32v20x_bkp.c ------------------------------------------------------------*/
#define OCTLR_CAL_MASK    ((uint16_t)0xFF80)
#define OCTLR_MASK        ((uint16_t)0xFC7F)

#endif

#if defined(CH32V20x) || defined(CH32V30x)
/* ch32v20x_can.c ------------------------------------------------------------*/
/* CAN CTLR Register bits */
#define CTLR_DBF                     ((uint32_t)0x00010000)

/* CAN Mailbox Transmit Request */
#define TMIDxR_TXRQ                  ((uint32_t)0x00000001)

/* CAN FCTLR Register bits */
#define FCTLR_FINIT                  ((uint32_t)0x00000001)

/* Time out for INAK bit */
#define INAK_TIMEOUT                 ((uint32_t)0x0000FFFF)
/* Time out for SLAK bit */
#define SLAK_TIMEOUT                 ((uint32_t)0x0000FFFF)


/* Flags in TSTATR register */
#define CAN_FLAGS_TSTATR             ((uint32_t)0x08000000)
/* Flags in RFIFO1 register */
#define CAN_FLAGS_RFIFO1             ((uint32_t)0x04000000)
/* Flags in RFIFO0 register */
#define CAN_FLAGS_RFIFO0             ((uint32_t)0x02000000)
/* Flags in STATR register */
#define CAN_FLAGS_STATR              ((uint32_t)0x01000000)
/* Flags in ERRSR register */
#define CAN_FLAGS_ERRSR              ((uint32_t)0x00F00000)

/* Mailboxes definition */
#define CAN_TXMAILBOX_0              ((uint8_t)0x00)
#define CAN_TXMAILBOX_1              ((uint8_t)0x01)
#define CAN_TXMAILBOX_2              ((uint8_t)0x02)


#define CAN_MODE_MASK                ((uint32_t) 0x00000003)

#endif

#if defined(CH32V30x)
/* ch32v30x_dac.c ------------------------------------------------------------*/
/* CTLR register Mask */
// Editor's note: Overloaded Definition.
#define DAC_CTLR_CLEAR_MASK    ((uint32_t)0x00000FFE)

/* DAC Dual Channels SWTR masks */
#define DUAL_SWTR_SET      		((uint32_t)0x00000003)
#define DUAL_SWTR_RESET    		((uint32_t)0xFFFFFFFC)

/* DHR registers offsets */
#define DHR12R1_OFFSET     		((uint32_t)0x00000008)
#define DHR12R2_OFFSET     		((uint32_t)0x00000014)
#define DHR12RD_OFFSET     		((uint32_t)0x00000020)

/* DOR register offset */
#define DOR_OFFSET         		((uint32_t)0x0000002C)
#endif

/* ch32v00x_dbgmcu.c ---------------------------------------------------------*/
#define IDCODE_DEVID_MASK    ((uint32_t)0x0000FFFF)


/* ch32v00x_dma.c ------------------------------------------------------------*/

/* DMA1 Channelx interrupt pending bit masks */
#define DMA1_Channel1_IT_Mask    ((uint32_t)(DMA_GIF1 | DMA_TCIF1 | DMA_HTIF1 | DMA_TEIF1))
#define DMA1_Channel2_IT_Mask    ((uint32_t)(DMA_GIF2 | DMA_TCIF2 | DMA_HTIF2 | DMA_TEIF2))
#define DMA1_Channel3_IT_Mask    ((uint32_t)(DMA_GIF3 | DMA_TCIF3 | DMA_HTIF3 | DMA_TEIF3))
#define DMA1_Channel4_IT_Mask    ((uint32_t)(DMA_GIF4 | DMA_TCIF4 | DMA_HTIF4 | DMA_TEIF4))
#define DMA1_Channel5_IT_Mask    ((uint32_t)(DMA_GIF5 | DMA_TCIF5 | DMA_HTIF5 | DMA_TEIF5))
#define DMA1_Channel6_IT_Mask    ((uint32_t)(DMA_GIF6 | DMA_TCIF6 | DMA_HTIF6 | DMA_TEIF6))
#define DMA1_Channel7_IT_Mask    ((uint32_t)(DMA_GIF7 | DMA_TCIF7 | DMA_HTIF7 | DMA_TEIF7))
#if defined(CH32V20x)
#define DMA1_Channel8_IT_Mask    ((uint32_t)(DMA_GIF8 | DMA_TCIF8 | DMA_HTIF8 | DMA_TEIF8))
#endif

#if defined(CH32V30x)
/* DMA2 Channelx interrupt pending bit masks */
#define DMA2_Channel1_IT_Mask     ((uint32_t)(DMA_GIF1 | DMA_TCIF1 | DMA_HTIF1 | DMA_TEIF1))
#define DMA2_Channel2_IT_Mask     ((uint32_t)(DMA_GIF2 | DMA_TCIF2 | DMA_HTIF2 | DMA_TEIF2))
#define DMA2_Channel3_IT_Mask     ((uint32_t)(DMA_GIF3 | DMA_TCIF3 | DMA_HTIF3 | DMA_TEIF3))
#define DMA2_Channel4_IT_Mask     ((uint32_t)(DMA_GIF4 | DMA_TCIF4 | DMA_HTIF4 | DMA_TEIF4))
#define DMA2_Channel5_IT_Mask     ((uint32_t)(DMA_GIF5 | DMA_TCIF5 | DMA_HTIF5 | DMA_TEIF5))
#endif
#if defined(CH32V10x) || defined(CH32V30x)
#define DMA2_Channel6_IT_Mask     ((uint32_t)(DMA_GIF6 | DMA_TCIF6 | DMA_HTIF6 | DMA_TEIF6))
#define DMA2_Channel7_IT_Mask     ((uint32_t)(DMA_GIF7 | DMA_TCIF7 | DMA_HTIF7 | DMA_TEIF7))
#define DMA2_Channel8_IT_Mask     ((uint32_t)(DMA_GIF8 | DMA_TCIF8 | DMA_HTIF8 | DMA_TEIF8))
#define DMA2_Channel9_IT_Mask     ((uint32_t)(DMA_GIF9 | DMA_TCIF9 | DMA_HTIF9 | DMA_TEIF9))
#define DMA2_Channel10_IT_Mask    ((uint32_t)(DMA_GIF10 | DMA_TCIF10 | DMA_HTIF10 | DMA_TEIF10))
#define DMA2_Channel11_IT_Mask    ((uint32_t)(DMA_GIF11 | DMA_TCIF11 | DMA_HTIF11 | DMA_TEIF11))
#endif

/* DMA2 FLAG mask */
// Editor's note: Overloaded Definition.
#define DMA2_FLAG_Mask            ((uint32_t)0x10000000)
#if defined(CH32V30x)
#define DMA2_EXTEN_FLAG_Mask      ((uint32_t)0x20000000)
#endif

/* DMA registers Masks */
#define CFGR_CLEAR_Mask          ((uint32_t)0xFFFF800F)

/* ch32v00x_exti.c -----------------------------------------------------------*/


/* No interrupt selected */
#define EXTI_LINENONE    ((uint32_t)0x00000)

/* ch32v00x_flash.c ----------------------------------------------------------*/

#if defined(CH32V003) || defined(CH32V10x)
/* Flash Access Control Register bits */
#define ACR_LATENCY_Mask           ((uint32_t)0x00000038)
#endif

#if defined(CH32V10x)
#define ACR_HLFCYA_Mask            ((uint32_t)0xFFFFFFF7)
#define ACR_PRFTBE_Mask            ((uint32_t)0xFFFFFFEF)
#define ACR_PRFTBS_Mask            ((uint32_t)0x00000020)
#endif

/* Flash Control Register bits */
#define CR_PG_Set                  ((uint32_t)0x00000001)
#define CR_PG_Reset                ((uint32_t)0xFFFFFFFE)
#define CR_PER_Set                 ((uint32_t)0x00000002)
#define CR_PER_Reset               ((uint32_t)0xFFFFFFFD)
#define CR_MER_Set                 ((uint32_t)0x00000004)
#define CR_MER_Reset               ((uint32_t)0xFFFFFFFB)
#define CR_OPTPG_Set               ((uint32_t)0x00000010)
#define CR_OPTPG_Reset             ((uint32_t)0xFFFFFFEF)
#define CR_OPTER_Set               ((uint32_t)0x00000020)
#define CR_OPTER_Reset             ((uint32_t)0xFFFFFFDF)
#define CR_STRT_Set                ((uint32_t)0x00000040)
#define CR_LOCK_Set                ((uint32_t)0x00000080)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define CR_FAST_LOCK_Set           ((uint32_t)0x00008000)
#endif
#define CR_PAGE_PG                 ((uint32_t)0x00010000)
#define CR_PAGE_ER                 ((uint32_t)0x00020000)
#if defined(CH32V003) || defined(CH32V10x)
#define CR_BUF_LOAD                ((uint32_t)0x00040000)
#define CR_BUF_RST                 ((uint32_t)0x00080000)
#elif defined(CH32V20x) || defined(CH32V30x)
#define CR_BER32                   ((uint32_t)0x00040000)
#define CR_BER64                   ((uint32_t)0x00080000)
#define CR_PG_STRT                 ((uint32_t)0x00200000)
#endif

/* FLASH Status Register bits */
#define SR_BSY                     ((uint32_t)0x00000001)
#if defined(CH32V20x) || defined(CH32V30x)
#define SR_WR_BSY                  ((uint32_t)0x00000002)
#endif
#if defined(CH32V10x)
#define SR_PGERR                   ((uint32_t)0x00000004)
#endif
#define SR_WRPRTERR                ((uint32_t)0x00000010)
#define SR_EOP                     ((uint32_t)0x00000020)

/* FLASH Mask */
#define RDPRT_Mask                 ((uint32_t)0x00000002)
#define WRP0_Mask                  ((uint32_t)0x000000FF)
#define WRP1_Mask                  ((uint32_t)0x0000FF00)
#define WRP2_Mask                  ((uint32_t)0x00FF0000)
#define WRP3_Mask                  ((uint32_t)0xFF000000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define OB_USER_BFB2               ((uint16_t)0x0008)
#endif

/* FLASH Keys */
#define RDP_Key                    ((uint16_t)0x00A5)
#define FLASH_KEY1                 ((uint32_t)0x45670123)
#define FLASH_KEY2                 ((uint32_t)0xCDEF89AB)

/* FLASH BANK address */
#define FLASH_BANK1_END_ADDRESS    ((uint32_t)0x807FFFF)

#if defined(CH32V20x)
/* EEPROM address */
#define EEPROM_ADDRESS             ((uint32_t)0x8070000)
#endif

/* Delay definition */
#define EraseTimeout               ((uint32_t)0x000B0000)
#if defined(CH32V003) || defined(CH32V20x) || defined(CH32V10x)
#define ProgramTimeout             ((uint32_t)0x00002000)
#elif defined(CH32V30x)
#define ProgramTimeout             ((uint32_t)0x00005000)
#endif

/* Flash Program Valid Address */
#define ValidAddrStart             (FLASH_BASE)
#if !defined(CH32V10x)
#define ValidAddrEnd               (FLASH_BASE + 0x4000)
#else
#define ValidAddrEnd               (FLASH_BASE + 0x10000)
#endif

#if defined(CH32V30x)

/* ch32v30x_fsmc.c -----------------------------------------------------------*/

/* FSMC BCRx Mask */
#define BCR_MBKEN_Set          ((uint32_t)0x00000001)
#define BCR_MBKEN_Reset        ((uint32_t)0x000FFFFE)
#define BCR_FACCEN_Set         ((uint32_t)0x00000040)

/* FSMC PCRx Mask */
#define PCR_PBKEN_Set          ((uint32_t)0x00000004)
#define PCR_PBKEN_Reset        ((uint32_t)0x000FFFFB)
#define PCR_ECCEN_Set          ((uint32_t)0x00000040)
#define PCR_ECCEN_Reset        ((uint32_t)0x000FFFBF)
#define PCR_MemoryType_NAND    ((uint32_t)0x00000008)

#endif

/* ch32v00x_i2c.c ------------------------------------------------------------*/


/* I2C SPE mask */
#define CTLR1_PE_Set             ((uint16_t)0x0001)
#define CTLR1_PE_Reset           ((uint16_t)0xFFFE)

/* I2C START mask */
#define CTLR1_START_Set          ((uint16_t)0x0100)
#define CTLR1_START_Reset        ((uint16_t)0xFEFF)

/* I2C STOP mask */
#define CTLR1_STOP_Set           ((uint16_t)0x0200)
#define CTLR1_STOP_Reset         ((uint16_t)0xFDFF)

/* I2C ACK mask */
#define CTLR1_ACK_Set            ((uint16_t)0x0400)
#define CTLR1_ACK_Reset          ((uint16_t)0xFBFF)

/* I2C ENGC mask */
#define CTLR1_ENGC_Set           ((uint16_t)0x0040)
#define CTLR1_ENGC_Reset         ((uint16_t)0xFFBF)

/* I2C SWRST mask */
#define CTLR1_SWRST_Set          ((uint16_t)0x8000)
#define CTLR1_SWRST_Reset        ((uint16_t)0x7FFF)

/* I2C PEC mask */
#define CTLR1_PEC_Set            ((uint16_t)0x1000)
#define CTLR1_PEC_Reset          ((uint16_t)0xEFFF)

/* I2C ENPEC mask */
#define CTLR1_ENPEC_Set          ((uint16_t)0x0020)
#define CTLR1_ENPEC_Reset        ((uint16_t)0xFFDF)

/* I2C ENARP mask */
#define CTLR1_ENARP_Set          ((uint16_t)0x0010)
#define CTLR1_ENARP_Reset        ((uint16_t)0xFFEF)

/* I2C NOSTRETCH mask */
#define CTLR1_NOSTRETCH_Set      ((uint16_t)0x0080)
#define CTLR1_NOSTRETCH_Reset    ((uint16_t)0xFF7F)

////* I2C registers Masks */
// Editor's note: Overloaded Definition.
#define I2C_CTLR1_CLEAR_Mask         ((uint16_t)0xFBF5)

/* I2C DMAEN mask */
#define CTLR2_DMAEN_Set          ((uint16_t)0x0800)
#define CTLR2_DMAEN_Reset        ((uint16_t)0xF7FF)

/* I2C LAST mask */
#define CTLR2_LAST_Set           ((uint16_t)0x1000)
#define CTLR2_LAST_Reset         ((uint16_t)0xEFFF)

/* I2C FREQ mask */
#define CTLR2_FREQ_Reset         ((uint16_t)0xFFC0)

/* I2C ADD0 mask */
#define OADDR1_ADD0_Set          ((uint16_t)0x0001)
#define OADDR1_ADD0_Reset        ((uint16_t)0xFFFE)

/* I2C ENDUAL mask */
#define OADDR2_ENDUAL_Set        ((uint16_t)0x0001)
#define OADDR2_ENDUAL_Reset      ((uint16_t)0xFFFE)

/* I2C ADD2 mask */
#define OADDR2_ADD2_Reset        ((uint16_t)0xFF01)

/* I2C F/S mask */
#define CKCFGR_FS_Set            ((uint16_t)0x8000)

/* I2C CCR mask */
#define CKCFGR_CCR_Set           ((uint16_t)0x0FFF)

/* I2C FLAG mask */
//Editor's Note: Overloaded Definition
#define I2c_FLAG_Mask            ((uint32_t)0x00FFFFFF)

/* I2C Interrupt Enable mask */
#define ITEN_Mask                ((uint32_t)0x07000000)

/* ch32v00x_iwdg.c -----------------------------------------------------------*/

/* CTLR register bit mask */
#define CTLR_KEY_Reload    ((uint16_t)0xAAAA)
#define CTLR_KEY_Enable    ((uint16_t)0xCCCC)

#if defined(CH32V20x) || defined(CH32V30x)

/* ch32v20x_opa.c ------------------------------------------------------------*/
#define OPA_MASK         ((uint32_t)0x000F)
#define OPA_Total_NUM    4

#endif

/* ch32v00x_pwr.c ------------------------------------------------------------*/


/* PWR registers bit mask */
/* CTLR register bit mask */
#if !defined(CH32V10x)
#define CTLR_DS_MASK     ((uint32_t)0xFFFFFFFD)
#else
#define CTLR_DS_MASK     ((uint32_t)0xFFFFFFFC)
#endif
#define CTLR_PLS_MASK    ((uint32_t)0xFFFFFF1F)
#if defined(CH32V003)
#define AWUPSC_MASK      ((uint32_t)0xFFFFFFF0)
#define AWUWR_MASK       ((uint32_t)0xFFFFFFC0)
#endif

/* ch32v00x_rcc.c ------------------------------------------------------------*/

/* RCC registers bit address in the alias region */
#define RCC_OFFSET                 (RCC_BASE - PERIPH_BASE)

/* BDCTLR Register */
#define BDCTLR_OFFSET              (RCC_OFFSET + 0x20)

/* RCC registers bit mask */

/* CTLR register bit mask */
#define CTLR_HSEBYP_Reset          ((uint32_t)0xFFFBFFFF)
#define CTLR_HSEBYP_Set            ((uint32_t)0x00040000)
#define CTLR_HSEON_Reset           ((uint32_t)0xFFFEFFFF)
#define CTLR_HSEON_Set             ((uint32_t)0x00010000)
#define CTLR_HSITRIM_Mask          ((uint32_t)0xFFFFFF07)

#define CFGR0_PLL_Mask             ((uint32_t)0xFFC0FFFF)
#define CFGR0_PLLMull_Mask         ((uint32_t)0x003C0000)
#define CFGR0_PLLSRC_Mask          ((uint32_t)0x00010000)
#define CFGR0_PLLXTPRE_Mask        ((uint32_t)0x00020000)
#define CFGR0_SWS_Mask             ((uint32_t)0x0000000C)
#define CFGR0_SW_Mask              ((uint32_t)0xFFFFFFFC)
#define CFGR0_HPRE_Reset_Mask      ((uint32_t)0xFFFFFF0F)
#define CFGR0_HPRE_Set_Mask        ((uint32_t)0x000000F0)
#define CFGR0_PPRE1_Reset_Mask     ((uint32_t)0xFFFFF8FF)
#define CFGR0_PPRE1_Set_Mask       ((uint32_t)0x00000700)
#define CFGR0_PPRE2_Reset_Mask     ((uint32_t)0xFFFFC7FF)
#define CFGR0_PPRE2_Set_Mask       ((uint32_t)0x00003800)
#define CFGR0_ADCPRE_Reset_Mask    ((uint32_t)0xFFFF07FF)
#define CFGR0_ADCPRE_Set_Mask      ((uint32_t)0x0000F800)

/* RSTSCKR register bit mask */
#define RSTSCKR_RMVF_Set           ((uint32_t)0x01000000)

#if defined(CH32V30x)
/* CFGR2 register bit mask */
#define CFGR2_PREDIV1SRC           ((uint32_t)0x00010000)
#define CFGR2_PREDIV1              ((uint32_t)0x0000000F)
#define CFGR2_PREDIV2              ((uint32_t)0x000000F0)
#define CFGR2_PLL2MUL              ((uint32_t)0x00000F00)
#define CFGR2_PLL3MUL              ((uint32_t)0x0000F000)
#endif

/* RCC Flag Mask */
//Editor's Note: Overloaded Definition
#define RCC_FLAG_Mask                  ((uint8_t)0x1F)

/* INTR register byte 2 (Bits[15:8]) base address */
#define INTR_BYTE2_ADDRESS         ((uint32_t)0x40021009)

/* INTR register byte 3 (Bits[23:16]) base address */
#define INTR_BYTE3_ADDRESS         ((uint32_t)0x4002100A)

/* CFGR0 register byte 4 (Bits[31:24]) base address */
#define CFGR0_BYTE4_ADDRESS        ((uint32_t)0x40021007)

/* BDCTLR register base address */
#define BDCTLR_ADDRESS             (PERIPH_BASE + BDCTLR_OFFSET)

#ifndef __ASSEMBLER__
#ifdef CH32V003
static __I uint8_t APBAHBPrescTable[16] = {1, 2, 3, 4, 5, 6, 7, 8, 1, 2, 3, 4, 5, 6, 7, 8};
static __I uint8_t ADCPrescTable[20] = {2, 4, 6, 8, 4, 8, 12, 16, 8, 16, 24, 32, 16, 32, 48, 64, 32, 64, 96, 128};
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
static __I uint8_t APBAHBPrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};
static __I uint8_t ADCPrescTable[4] = {2, 4, 6, 8};
#endif
#endif

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* ch32v20x_rtc.c ------------------------------------------------------------*/

/* RTC_Private_Defines */
#define RTC_LSB_MASK     ((uint32_t)0x0000FFFF) /* RTC LSB Mask */
#define PRLH_MSB_MASK    ((uint32_t)0x000F0000) /* RTC Prescaler MSB Mask */

#endif

#if defined(CH32V30x)

/* ch32v00x_sdio.c -----------------------------------------------------------*/

#define SDIO_OFFSET         (SDIO_BASE - PERIPH_BASE)

/* CLKCR register clear mask */
#define CLKCR_CLEAR_MASK    ((uint32_t)0xFFFF8100)

/* SDIO PWRCTRL Mask */
#define PWR_PWRCTRL_MASK    ((uint32_t)0xFFFFFFFC)

/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK    ((uint32_t)0xFFFFFF08)

/* CMD Register clear mask */
#define CMD_CLEAR_MASK      ((uint32_t)0xFFFFF800)

/* SDIO RESP Registers Address */
#define SDIO_RESP_ADDR      ((uint32_t)(SDIO_BASE + 0x14))

#endif // defined(CH32V30x)

/* ch32v00x_spi.c ------------------------------------------------------------*/

/* SPI SPE mask */
#define CTLR1_SPE_Set         ((uint16_t)0x0040)
#define CTLR1_SPE_Reset       ((uint16_t)0xFFBF)

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/* I2S I2SE mask */
#define I2SCFGR_I2SE_Set      ((uint16_t)0x0400)
#define I2SCFGR_I2SE_Reset    ((uint16_t)0xFBFF)
#endif

/* SPI CRCNext mask */
#define CTLR1_CRCNext_Set     ((uint16_t)0x1000)

/* SPI CRCEN mask */
#define CTLR1_CRCEN_Set       ((uint16_t)0x2000)
#define CTLR1_CRCEN_Reset     ((uint16_t)0xDFFF)

/* SPI SSOE mask */
#define CTLR2_SSOE_Set        ((uint16_t)0x0004)
#define CTLR2_SSOE_Reset      ((uint16_t)0xFFFB)

/* SPI registers Masks */
//Editor's Note: Overloaded Definition
#define SPI_CTLR1_CLEAR_Mask  ((uint16_t)0x3040)
#define I2SCFGR_CLEAR_Mask    ((uint16_t)0xF040)

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
/* SPI or I2S mode selection masks */
#define SPI_Mode_Select       ((uint16_t)0xF7FF)
#define I2S_Mode_Select       ((uint16_t)0x0800)

/* I2S clock source selection masks */
#define I2S2_CLOCK_SRC        ((uint32_t)(0x00020000))
#define I2S3_CLOCK_SRC        ((uint32_t)(0x00040000))
#define I2S_MUL_MASK          ((uint32_t)(0x0000F000))
#define I2S_DIV_MASK          ((uint32_t)(0x000000F0))
#endif


/* ch32v00x_tim.c ------------------------------------------------------------*/

/* TIM registers bit mask */
#define SMCFGR_ETR_Mask    ((uint16_t)0x00FF)
#define CHCTLR_Offset      ((uint16_t)0x0018)
#define CCER_CCE_Set       ((uint16_t)0x0001)
#define CCER_CCNE_Set      ((uint16_t)0x0004)

/* ch32v00x_usart.c ----------------------------------------------------------*/

/* USART_Private_Defines */
#define CTLR1_UE_Set              ((uint16_t)0x2000) /* USART Enable Mask */
#define CTLR1_UE_Reset            ((uint16_t)0xDFFF) /* USART Disable Mask */

#define CTLR1_WAKE_Mask           ((uint16_t)0xF7FF) /* USART WakeUp Method Mask */

#define CTLR1_RWU_Set             ((uint16_t)0x0002) /* USART mute mode Enable Mask */
#define CTLR1_RWU_Reset           ((uint16_t)0xFFFD) /* USART mute mode Enable Mask */
#define CTLR1_SBK_Set             ((uint16_t)0x0001) /* USART Break Character send Mask */
//Editor's Note: Overloaded Definition
#define USART_CTLR1_CLEAR_Mask          ((uint16_t)0xE9F3) /* USART CR1 Mask */
#define CTLR2_Address_Mask        ((uint16_t)0xFFF0) /* USART address Mask */

#define CTLR2_LINEN_Set           ((uint16_t)0x4000) /* USART LIN Enable Mask */
#define CTLR2_LINEN_Reset         ((uint16_t)0xBFFF) /* USART LIN Disable Mask */

#define CTLR2_LBDL_Mask           ((uint16_t)0xFFDF) /* USART LIN Break detection Mask */
#define CTLR2_STOP_CLEAR_Mask     ((uint16_t)0xCFFF) /* USART CR2 STOP Bits Mask */
#define CTLR2_CLOCK_CLEAR_Mask    ((uint16_t)0xF0FF) /* USART CR2 Clock Mask */

#define CTLR3_SCEN_Set            ((uint16_t)0x0020) /* USART SC Enable Mask */
#define CTLR3_SCEN_Reset          ((uint16_t)0xFFDF) /* USART SC Disable Mask */

#define CTLR3_NACK_Set            ((uint16_t)0x0010) /* USART SC NACK Enable Mask */
#define CTLR3_NACK_Reset          ((uint16_t)0xFFEF) /* USART SC NACK Disable Mask */

#define CTLR3_HDSEL_Set           ((uint16_t)0x0008) /* USART Half-Duplex Enable Mask */
#define CTLR3_HDSEL_Reset         ((uint16_t)0xFFF7) /* USART Half-Duplex Disable Mask */

#define CTLR3_IRLP_Mask           ((uint16_t)0xFFFB) /* USART IrDA LowPower mode Mask */
#define CTLR3_CLEAR_Mask          ((uint16_t)0xFCFF) /* USART CR3 Mask */

#define CTLR3_IREN_Set            ((uint16_t)0x0002) /* USART IrDA Enable Mask */
#define CTLR3_IREN_Reset          ((uint16_t)0xFFFD) /* USART IrDA Disable Mask */
#define GPR_LSB_Mask              ((uint16_t)0x00FF) /* Guard Time Register LSB Mask */
#define GPR_MSB_Mask              ((uint16_t)0xFF00) /* Guard Time Register MSB Mask */
#define IT_Mask                   ((uint16_t)0x001F) /* USART Interrupt Mask */

/* USART OverSampling-8 Mask */
#define CTLR1_OVER8_Set           ((uint16_t)0x8000) /* USART OVER8 mode Enable Mask */
#define CTLR1_OVER8_Reset         ((uint16_t)0x7FFF) /* USART OVER8 mode Disable Mask */

/* USART One Bit Sampling Mask */
#define CTLR3_ONEBITE_Set         ((uint16_t)0x0800) /* USART ONEBITE mode Enable Mask */
#define CTLR3_ONEBITE_Reset       ((uint16_t)0xF7FF) /* USART ONEBITE mode Disable Mask */

/* ch32v00x_wwdg.c ------------------------------------------------------------*/

/* CTLR register bit mask */
#define CTLR_WDGA_Set      ((uint32_t)0x00000080)

/* CFGR register bit mask */
#define CFGR_WDGTB_Mask    ((uint32_t)0xFFFFFE7F)
#define CFGR_W_Mask        ((uint32_t)0xFFFFFF80)
#define BIT_Mask           ((uint8_t)0x7F)


/* ch32v00x_adc.h ------------------------------------------------------------*/

/* ADC_mode */
#define ADC_Mode_Independent                           ((uint32_t)0x00000000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC_Mode_RegInjecSimult                        ((uint32_t)0x00010000)
#define ADC_Mode_RegSimult_AlterTrig                   ((uint32_t)0x00020000)
#define ADC_Mode_InjecSimult_FastInterl                ((uint32_t)0x00030000)
#define ADC_Mode_InjecSimult_SlowInterl                ((uint32_t)0x00040000)
#define ADC_Mode_InjecSimult                           ((uint32_t)0x00050000)
#define ADC_Mode_RegSimult                             ((uint32_t)0x00060000)
#define ADC_Mode_FastInterl                            ((uint32_t)0x00070000)
#define ADC_Mode_SlowInterl                            ((uint32_t)0x00080000)
#define ADC_Mode_AlterTrig                             ((uint32_t)0x00090000)
#endif

/* ADC_external_trigger_sources_for_regular_channels_conversion */
#ifdef CH32V003

#define ADC_ExternalTrigConv_T1_TRGO                   ((uint32_t)0x00000000)
#define ADC_ExternalTrigConv_T1_CC1                    ((uint32_t)0x00020000)
#define ADC_ExternalTrigConv_T1_CC2                    ((uint32_t)0x00040000)
#define ADC_ExternalTrigConv_T2_TRGO                   ((uint32_t)0x00060000)
#define ADC_ExternalTrigConv_T2_CC1                    ((uint32_t)0x00080000)
#define ADC_ExternalTrigConv_T2_CC2                    ((uint32_t)0x000A0000)
#define ADC_ExternalTrigConv_Ext_PD3_PC2               ((uint32_t)0x000C0000)
#define ADC_ExternalTrigConv_None                      ((uint32_t)0x000E0000)

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

#define ADC_ExternalTrigConv_T1_CC1                    ((uint32_t)0x00000000)
#define ADC_ExternalTrigConv_T1_CC2                    ((uint32_t)0x00020000)
#define ADC_ExternalTrigConv_T2_CC2                    ((uint32_t)0x00060000)
#define ADC_ExternalTrigConv_T3_TRGO                   ((uint32_t)0x00080000)
#define ADC_ExternalTrigConv_T4_CC4                    ((uint32_t)0x000A0000)
#define ADC_ExternalTrigConv_Ext_IT11_TIM8_TRGO        ((uint32_t)0x000C0000)

#define ADC_ExternalTrigConv_T1_CC3                    ((uint32_t)0x00040000)
#define ADC_ExternalTrigConv_None                      ((uint32_t)0x000E0000)

#endif

#if defined(CH32V20x) || defined(CH32V30x)

#define ADC_ExternalTrigConv_T3_CC1                    ((uint32_t)0x00000000)
#define ADC_ExternalTrigConv_T2_CC3                    ((uint32_t)0x00020000)
#define ADC_ExternalTrigConv_T8_CC1                    ((uint32_t)0x00060000)
#define ADC_ExternalTrigConv_T8_TRGO                   ((uint32_t)0x00080000)
#define ADC_ExternalTrigConv_T5_CC1                    ((uint32_t)0x000A0000)
#define ADC_ExternalTrigConv_T5_CC3                    ((uint32_t)0x000C0000)

#endif

/* ADC_data_align */
#define ADC_DataAlign_Right                            ((uint32_t)0x00000000)
#define ADC_DataAlign_Left                             ((uint32_t)0x00000800)

/* ADC_channels */
#define ADC_Channel_0                                  ((uint8_t)0x00)
#define ADC_Channel_1                                  ((uint8_t)0x01)
#define ADC_Channel_2                                  ((uint8_t)0x02)
#define ADC_Channel_3                                  ((uint8_t)0x03)
#define ADC_Channel_4                                  ((uint8_t)0x04)
#define ADC_Channel_5                                  ((uint8_t)0x05)
#define ADC_Channel_6                                  ((uint8_t)0x06)
#define ADC_Channel_7                                  ((uint8_t)0x07)
#define ADC_Channel_8                                  ((uint8_t)0x08)
#define ADC_Channel_9                                  ((uint8_t)0x09)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC_Channel_10                                 ((uint8_t)0x0A)
#define ADC_Channel_11                                 ((uint8_t)0x0B)
#define ADC_Channel_12                                 ((uint8_t)0x0C)
#define ADC_Channel_13                                 ((uint8_t)0x0D)
#define ADC_Channel_14                                 ((uint8_t)0x0E)
#define ADC_Channel_15                                 ((uint8_t)0x0F)
#define ADC_Channel_16                                 ((uint8_t)0x10)
#define ADC_Channel_17                                 ((uint8_t)0x11)
#endif

#ifdef CH32V003
#define ADC_Channel_Vrefint                            ((uint8_t)ADC_Channel_8)
#define ADC_Channel_Vcalint                            ((uint8_t)ADC_Channel_9)
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC_Channel_TempSensor                         ((uint8_t)ADC_Channel_16)
#define ADC_Channel_Vrefint                            ((uint8_t)ADC_Channel_17)
#endif

#if defined(CH32V20x) || defined(CH32V30x)
/*ADC_output_buffer*/
#define ADC_OutputBuffer_Enable                        ((uint32_t)0x04000000)
#define ADC_OutputBuffer_Disable                       ((uint32_t)0x00000000)

/*ADC_pga*/
#define ADC_Pga_1                                      ((uint32_t)0x00000000)
#define ADC_Pga_4                                      ((uint32_t)0x08000000)
#define ADC_Pga_16                                     ((uint32_t)0x10000000)
#define ADC_Pga_64                                     ((uint32_t)0x18000000)
#endif

/* ADC_sampling_time */
#ifdef CH32V003
#define ADC_SampleTime_3Cycles                         ((uint8_t)0x00)
#define ADC_SampleTime_9Cycles                         ((uint8_t)0x01)
#define ADC_SampleTime_15Cycles                        ((uint8_t)0x02)
#define ADC_SampleTime_30Cycles                        ((uint8_t)0x03)
#define ADC_SampleTime_43Cycles                        ((uint8_t)0x04)
#define ADC_SampleTime_57Cycles                        ((uint8_t)0x05)
#define ADC_SampleTime_73Cycles                        ((uint8_t)0x06)
#define ADC_SampleTime_241Cycles                       ((uint8_t)0x07)
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC_SampleTime_1Cycles5                        ((uint8_t)0x00)
#define ADC_SampleTime_7Cycles5                        ((uint8_t)0x01)
#define ADC_SampleTime_13Cycles5                       ((uint8_t)0x02)
#define ADC_SampleTime_28Cycles5                       ((uint8_t)0x03)
#define ADC_SampleTime_41Cycles5                       ((uint8_t)0x04)
#define ADC_SampleTime_55Cycles5                       ((uint8_t)0x05)
#define ADC_SampleTime_71Cycles5                       ((uint8_t)0x06)
#define ADC_SampleTime_239Cycles5                      ((uint8_t)0x07)
#endif

/* ADC_external_trigger_sources_for_injected_channels_conversion */
#ifdef CH32V003
#define ADC_ExternalTrigInjecConv_T1_CC3               ((uint32_t)0x00000000)
#define ADC_ExternalTrigInjecConv_T1_CC4               ((uint32_t)0x00001000)
#define ADC_ExternalTrigInjecConv_T2_CC3               ((uint32_t)0x00002000)
#define ADC_ExternalTrigInjecConv_T2_CC4               ((uint32_t)0x00003000)
#define ADC_ExternalTrigInjecConv_Ext_PD1_PA2          ((uint32_t)0x00006000)
#define ADC_ExternalTrigInjecConv_None                 ((uint32_t)0x00007000)
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define ADC_ExternalTrigInjecConv_T2_TRGO              ((uint32_t)0x00002000)
#define ADC_ExternalTrigInjecConv_T2_CC1               ((uint32_t)0x00003000)
#define ADC_ExternalTrigInjecConv_T3_CC4               ((uint32_t)0x00004000)
#define ADC_ExternalTrigInjecConv_T4_TRGO              ((uint32_t)0x00005000)
#define ADC_ExternalTrigInjecConv_Ext_IT15_TIM8_CC4    ((uint32_t)0x00006000)

#define ADC_ExternalTrigInjecConv_T1_TRGO              ((uint32_t)0x00000000)
#define ADC_ExternalTrigInjecConv_T1_CC4               ((uint32_t)0x00001000)
#define ADC_ExternalTrigInjecConv_None                 ((uint32_t)0x00007000)
#endif

#if defined(CH32V20x) || defined(CH32V30x)
#define ADC_ExternalTrigInjecConv_T4_CC3               ((uint32_t)0x00002000)
#define ADC_ExternalTrigInjecConv_T8_CC2               ((uint32_t)0x00003000)
#define ADC_ExternalTrigInjecConv_T8_CC4               ((uint32_t)0x00004000)
#define ADC_ExternalTrigInjecConv_T5_TRGO              ((uint32_t)0x00005000)
#define ADC_ExternalTrigInjecConv_T5_CC4               ((uint32_t)0x00006000)
#endif

/* ADC_injected_channel_selection */
#define ADC_InjectedChannel_1                          ((uint8_t)0x14)
#define ADC_InjectedChannel_2                          ((uint8_t)0x18)
#define ADC_InjectedChannel_3                          ((uint8_t)0x1C)
#define ADC_InjectedChannel_4                          ((uint8_t)0x20)

/* ADC_analog_watchdog_selection */
#define ADC_AnalogWatchdog_SingleRegEnable             ((uint32_t)0x00800200)
#define ADC_AnalogWatchdog_SingleInjecEnable           ((uint32_t)0x00400200)
#define ADC_AnalogWatchdog_SingleRegOrInjecEnable      ((uint32_t)0x00C00200)
#define ADC_AnalogWatchdog_AllRegEnable                ((uint32_t)0x00800000)
#define ADC_AnalogWatchdog_AllInjecEnable              ((uint32_t)0x00400000)
#define ADC_AnalogWatchdog_AllRegAllInjecEnable        ((uint32_t)0x00C00000)
#define ADC_AnalogWatchdog_None                        ((uint32_t)0x00000000)

/* ADC_interrupts_definition */
#define ADC_IT_EOC                                     ((uint16_t)0x0220)
#define ADC_IT_AWD                                     ((uint16_t)0x0140)
#define ADC_IT_JEOC                                    ((uint16_t)0x0480)

/* ADC_flags_definition */
#define ADC_FLAG_AWD                                   ((uint8_t)0x01)
#define ADC_FLAG_EOC                                   ((uint8_t)0x02)
#define ADC_FLAG_JEOC                                  ((uint8_t)0x04)
#define ADC_FLAG_JSTRT                                 ((uint8_t)0x08)
#define ADC_FLAG_STRT                                  ((uint8_t)0x10)

#if defined(CH32V003)
/* ADC_calibration_voltage_definition */
#define ADC_CALVOL_50PERCENT                           ((uint32_t)0x02000000)
#define ADC_CALVOL_75PERCENT                           ((uint32_t)0x04000000)

/* ADC_external_trigger_sources_delay_channels_definition */
#define ADC_ExternalTrigRegul_DLY                      ((uint32_t)0x00000000)
#define ADC_ExternalTrigInjec_DLY                      ((uint32_t)0x00020000)
#endif

#if defined(CH32V10x) || defined(CH32V20x)

/* ch32v20x_bkp.h ------------------------------------------------------------*/

/* Tamper_Pin_active_level */
#define BKP_TamperPinLevel_High           ((uint16_t)0x0000)
#define BKP_TamperPinLevel_Low            ((uint16_t)0x0001)

/* RTC_output_source_to_output_on_the_Tamper_pin */
#define BKP_RTCOutputSource_None          ((uint16_t)0x0000)
#define BKP_RTCOutputSource_CalibClock    ((uint16_t)0x0080)
#define BKP_RTCOutputSource_Alarm         ((uint16_t)0x0100)
#define BKP_RTCOutputSource_Second        ((uint16_t)0x0300)

/* Data_Backup_Register */
#define BKP_DR1                           ((uint16_t)0x0004)
#define BKP_DR2                           ((uint16_t)0x0008)
#define BKP_DR3                           ((uint16_t)0x000C)
#define BKP_DR4                           ((uint16_t)0x0010)
#define BKP_DR5                           ((uint16_t)0x0014)
#define BKP_DR6                           ((uint16_t)0x0018)
#define BKP_DR7                           ((uint16_t)0x001C)
#define BKP_DR8                           ((uint16_t)0x0020)
#define BKP_DR9                           ((uint16_t)0x0024)
#define BKP_DR10                          ((uint16_t)0x0028)
#define BKP_DR11                          ((uint16_t)0x0040)
#define BKP_DR12                          ((uint16_t)0x0044)
#define BKP_DR13                          ((uint16_t)0x0048)
#define BKP_DR14                          ((uint16_t)0x004C)
#define BKP_DR15                          ((uint16_t)0x0050)
#define BKP_DR16                          ((uint16_t)0x0054)
#define BKP_DR17                          ((uint16_t)0x0058)
#define BKP_DR18                          ((uint16_t)0x005C)
#define BKP_DR19                          ((uint16_t)0x0060)
#define BKP_DR20                          ((uint16_t)0x0064)
#define BKP_DR21                          ((uint16_t)0x0068)
#define BKP_DR22                          ((uint16_t)0x006C)
#define BKP_DR23                          ((uint16_t)0x0070)
#define BKP_DR24                          ((uint16_t)0x0074)
#define BKP_DR25                          ((uint16_t)0x0078)
#define BKP_DR26                          ((uint16_t)0x007C)
#define BKP_DR27                          ((uint16_t)0x0080)
#define BKP_DR28                          ((uint16_t)0x0084)
#define BKP_DR29                          ((uint16_t)0x0088)
#define BKP_DR30                          ((uint16_t)0x008C)
#define BKP_DR31                          ((uint16_t)0x0090)
#define BKP_DR32                          ((uint16_t)0x0094)
#define BKP_DR33                          ((uint16_t)0x0098)
#define BKP_DR34                          ((uint16_t)0x009C)
#define BKP_DR35                          ((uint16_t)0x00A0)
#define BKP_DR36                          ((uint16_t)0x00A4)
#define BKP_DR37                          ((uint16_t)0x00A8)
#define BKP_DR38                          ((uint16_t)0x00AC)
#define BKP_DR39                          ((uint16_t)0x00B0)
#define BKP_DR40                          ((uint16_t)0x00B4)
#define BKP_DR41                          ((uint16_t)0x00B8)
#define BKP_DR42                          ((uint16_t)0x00BC)

#endif

#if defined(CH32V20x)

/* ch32v20x_can.h ------------------------------------------------------------*/

/* CAN_sleep_constants */
#define CAN_InitStatus_Failed               ((uint8_t)0x00) /* CAN initialization failed */
#define CAN_InitStatus_Success              ((uint8_t)0x01) /* CAN initialization OK */

/* CAN_Mode */
#define CAN_Mode_Normal                     ((uint8_t)0x00) /* normal mode */
#define CAN_Mode_LoopBack                   ((uint8_t)0x01) /* loopback mode */
#define CAN_Mode_Silent                     ((uint8_t)0x02) /* silent mode */
#define CAN_Mode_Silent_LoopBack            ((uint8_t)0x03) /* loopback combined with silent mode */

/* CAN_Operating_Mode */
#define CAN_OperatingMode_Initialization    ((uint8_t)0x00) /* Initialization mode */
#define CAN_OperatingMode_Normal            ((uint8_t)0x01) /* Normal mode */
#define CAN_OperatingMode_Sleep             ((uint8_t)0x02) /* sleep mode */

/* CAN_Mode_Status */
#define CAN_ModeStatus_Failed               ((uint8_t)0x00)                   /* CAN entering the specific mode failed */
#define CAN_ModeStatus_Success              ((uint8_t)!CAN_ModeStatus_Failed) /* CAN entering the specific mode Succeed */

/* CAN_synchronisation_jump_width */
#define CAN_SJW_1tq                         ((uint8_t)0x00) /* 1 time quantum */
#define CAN_SJW_2tq                         ((uint8_t)0x01) /* 2 time quantum */
#define CAN_SJW_3tq                         ((uint8_t)0x02) /* 3 time quantum */
#define CAN_SJW_4tq                         ((uint8_t)0x03) /* 4 time quantum */

/* CAN_time_quantum_in_bit_segment_1 */
#define CAN_BS1_1tq                         ((uint8_t)0x00) /* 1 time quantum */
#define CAN_BS1_2tq                         ((uint8_t)0x01) /* 2 time quantum */
#define CAN_BS1_3tq                         ((uint8_t)0x02) /* 3 time quantum */
#define CAN_BS1_4tq                         ((uint8_t)0x03) /* 4 time quantum */
#define CAN_BS1_5tq                         ((uint8_t)0x04) /* 5 time quantum */
#define CAN_BS1_6tq                         ((uint8_t)0x05) /* 6 time quantum */
#define CAN_BS1_7tq                         ((uint8_t)0x06) /* 7 time quantum */
#define CAN_BS1_8tq                         ((uint8_t)0x07) /* 8 time quantum */
#define CAN_BS1_9tq                         ((uint8_t)0x08) /* 9 time quantum */
#define CAN_BS1_10tq                        ((uint8_t)0x09) /* 10 time quantum */
#define CAN_BS1_11tq                        ((uint8_t)0x0A) /* 11 time quantum */
#define CAN_BS1_12tq                        ((uint8_t)0x0B) /* 12 time quantum */
#define CAN_BS1_13tq                        ((uint8_t)0x0C) /* 13 time quantum */
#define CAN_BS1_14tq                        ((uint8_t)0x0D) /* 14 time quantum */
#define CAN_BS1_15tq                        ((uint8_t)0x0E) /* 15 time quantum */
#define CAN_BS1_16tq                        ((uint8_t)0x0F) /* 16 time quantum */

/* CAN_time_quantum_in_bit_segment_2 */
#define CAN_BS2_1tq                         ((uint8_t)0x00) /* 1 time quantum */
#define CAN_BS2_2tq                         ((uint8_t)0x01) /* 2 time quantum */
#define CAN_BS2_3tq                         ((uint8_t)0x02) /* 3 time quantum */
#define CAN_BS2_4tq                         ((uint8_t)0x03) /* 4 time quantum */
#define CAN_BS2_5tq                         ((uint8_t)0x04) /* 5 time quantum */
#define CAN_BS2_6tq                         ((uint8_t)0x05) /* 6 time quantum */
#define CAN_BS2_7tq                         ((uint8_t)0x06) /* 7 time quantum */
#define CAN_BS2_8tq                         ((uint8_t)0x07) /* 8 time quantum */

/* CAN_filter_mode */
#define CAN_FilterMode_IdMask               ((uint8_t)0x00) /* identifier/mask mode */
#define CAN_FilterMode_IdList               ((uint8_t)0x01) /* identifier list mode */

/* CAN_filter_scale */
#define CAN_FilterScale_16bit               ((uint8_t)0x00) /* Two 16-bit filters */
#define CAN_FilterScale_32bit               ((uint8_t)0x01) /* One 32-bit filter */

/* CAN_filter_FIFO */
#define CAN_Filter_FIFO0                    ((uint8_t)0x00) /* Filter FIFO 0 assignment for filter x */
#define CAN_Filter_FIFO1                    ((uint8_t)0x01) /* Filter FIFO 1 assignment for filter x */

/* CAN_identifier_type */
#define CAN_Id_Standard                     ((uint32_t)0x00000000) /* Standard Id */
#define CAN_Id_Extended                     ((uint32_t)0x00000004) /* Extended Id */

/* CAN_remote_transmission_request */
#define CAN_RTR_Data                        ((uint32_t)0x00000000) /* Data frame */
#define CAN_RTR_Remote                      ((uint32_t)0x00000002) /* Remote frame */

/* CAN_transmit_constants */
#define CAN_TxStatus_Failed                 ((uint8_t)0x00) /* CAN transmission failed */
#define CAN_TxStatus_Ok                     ((uint8_t)0x01) /* CAN transmission succeeded */
#define CAN_TxStatus_Pending                ((uint8_t)0x02) /* CAN transmission pending */
#define CAN_TxStatus_NoMailBox              ((uint8_t)0x04) /* CAN cell did not provide an empty mailbox */

/* CAN_receive_FIFO_number_constants */
#define CAN_FIFO0                           ((uint8_t)0x00) /* CAN FIFO 0 used to receive */
#define CAN_FIFO1                           ((uint8_t)0x01) /* CAN FIFO 1 used to receive */

/* CAN_sleep_constants */
#define CAN_Sleep_Failed                    ((uint8_t)0x00) /* CAN did not enter the sleep mode */
#define CAN_Sleep_Ok                        ((uint8_t)0x01) /* CAN entered the sleep mode */

/* CAN_wake_up_constants */
#define CAN_WakeUp_Failed                   ((uint8_t)0x00) /* CAN did not leave the sleep mode */
#define CAN_WakeUp_Ok                       ((uint8_t)0x01) /* CAN leaved the sleep mode */

/* CAN_Error_Code_constants */
#define CAN_ErrorCode_NoErr                 ((uint8_t)0x00) /* No Error */
#define CAN_ErrorCode_StuffErr              ((uint8_t)0x10) /* Stuff Error */
#define CAN_ErrorCode_FormErr               ((uint8_t)0x20) /* Form Error */
#define CAN_ErrorCode_ACKErr                ((uint8_t)0x30) /* Acknowledgment Error */
#define CAN_ErrorCode_BitRecessiveErr       ((uint8_t)0x40) /* Bit Recessive Error */
#define CAN_ErrorCode_BitDominantErr        ((uint8_t)0x50) /* Bit Dominant Error */
#define CAN_ErrorCode_CRCErr                ((uint8_t)0x60) /* CRC Error  */
#define CAN_ErrorCode_SoftwareSetErr        ((uint8_t)0x70) /* Software Set Error */

/* CAN_flags */
/* Transmit Flags */
/* If the flag is 0x3XXXXXXX, it means that it can be used with CAN_GetFlagStatus()
 * and CAN_ClearFlag() functions.
 * If the flag is 0x1XXXXXXX, it means that it can only be used with CAN_GetFlagStatus() function.
*/
#define CAN_FLAG_RQCP0                      ((uint32_t)0x38000001) /* Request MailBox0 Flag */
#define CAN_FLAG_RQCP1                      ((uint32_t)0x38000100) /* Request MailBox1 Flag */
#define CAN_FLAG_RQCP2                      ((uint32_t)0x38010000) /* Request MailBox2 Flag */

/* Receive Flags */
#define CAN_FLAG_FMP0                       ((uint32_t)0x12000003) /* FIFO 0 Message Pending Flag */
#define CAN_FLAG_FF0                        ((uint32_t)0x32000008) /* FIFO 0 Full Flag            */
#define CAN_FLAG_FOV0                       ((uint32_t)0x32000010) /* FIFO 0 Overrun Flag         */
#define CAN_FLAG_FMP1                       ((uint32_t)0x14000003) /* FIFO 1 Message Pending Flag */
#define CAN_FLAG_FF1                        ((uint32_t)0x34000008) /* FIFO 1 Full Flag            */
#define CAN_FLAG_FOV1                       ((uint32_t)0x34000010) /* FIFO 1 Overrun Flag         */

/* Operating Mode Flags */
#define CAN_FLAG_WKU                        ((uint32_t)0x31000008) /* Wake up Flag */
#define CAN_FLAG_SLAK                       ((uint32_t)0x31000012) /* Sleep acknowledge Flag */
/* Note:
 *When SLAK intterupt is disabled (SLKIE=0), no polling on SLAKI is possible.
 *In this case the SLAK bit can be polled.
*/


/* Error Flags */
#define CAN_FLAG_EWG                        ((uint32_t)0x10F00001) /* Error Warning Flag   */
#define CAN_FLAG_EPV                        ((uint32_t)0x10F00002) /* Error Passive Flag   */
#define CAN_FLAG_BOF                        ((uint32_t)0x10F00004) /* Bus-Off Flag         */
#define CAN_FLAG_LEC                        ((uint32_t)0x30F00070) /* Last error code Flag */

/* CAN_interrupts */
#define CAN_IT_TME                          ((uint32_t)0x00000001) /* Transmit mailbox empty Interrupt*/

/* Receive Interrupts */
#define CAN_IT_FMP0                         ((uint32_t)0x00000002) /* FIFO 0 message pending Interrupt*/
#define CAN_IT_FF0                          ((uint32_t)0x00000004) /* FIFO 0 full Interrupt*/
#define CAN_IT_FOV0                         ((uint32_t)0x00000008) /* FIFO 0 overrun Interrupt*/
#define CAN_IT_FMP1                         ((uint32_t)0x00000010) /* FIFO 1 message pending Interrupt*/
#define CAN_IT_FF1                          ((uint32_t)0x00000020) /* FIFO 1 full Interrupt*/
#define CAN_IT_FOV1                         ((uint32_t)0x00000040) /* FIFO 1 overrun Interrupt*/

/* Operating Mode Interrupts */
#define CAN_IT_WKU                          ((uint32_t)0x00010000) /* Wake-up Interrupt*/
#define CAN_IT_SLK                          ((uint32_t)0x00020000) /* Sleep acknowledge Interrupt*/

/* Error Interrupts */
#define CAN_IT_EWG                          ((uint32_t)0x00000100) /* Error warning Interrupt*/
#define CAN_IT_EPV                          ((uint32_t)0x00000200) /* Error passive Interrupt*/
#define CAN_IT_BOF                          ((uint32_t)0x00000400) /* Bus-off Interrupt*/
#define CAN_IT_LEC                          ((uint32_t)0x00000800) /* Last error code Interrupt*/
#define CAN_IT_ERR                          ((uint32_t)0x00008000) /* Error Interrupt*/

/* Flags named as Interrupts : kept only for FW compatibility */
#define CAN_IT_RQCP0                        CAN_IT_TME
#define CAN_IT_RQCP1                        CAN_IT_TME
#define CAN_IT_RQCP2                        CAN_IT_TME

/* CAN_Legacy */
#define CANINITFAILED                       CAN_InitStatus_Failed
#define CANINITOK                           CAN_InitStatus_Success
#define CAN_FilterFIFO0                     CAN_Filter_FIFO0
#define CAN_FilterFIFO1                     CAN_Filter_FIFO1
#define CAN_ID_STD                          CAN_Id_Standard
#define CAN_ID_EXT                          CAN_Id_Extended
#define CAN_RTR_DATA                        CAN_RTR_Data
#define CAN_RTR_REMOTE                      CAN_RTR_Remote
#define CANTXFAILE                          CAN_TxStatus_Failed
#define CANTXOK                             CAN_TxStatus_Ok
#define CANTXPENDING                        CAN_TxStatus_Pending
#define CAN_NO_MB                           CAN_TxStatus_NoMailBox
#define CANSLEEPFAILED                      CAN_Sleep_Failed
#define CANSLEEPOK                          CAN_Sleep_Ok
#define CANWAKEUPFAILED                     CAN_WakeUp_Failed
#define CANWAKEUPOK                         CAN_WakeUp_Ok

#endif

#if defined(CH32V20x)
/* ch32v00x_dac.h ------------------------------------------------------------*/

/* DAC_trigger_selection */
#define DAC_Trigger_None                   ((uint32_t)0x00000000) /* Conversion is automatic once the DAC1_DHRxxxx register
                                                                     has been loaded, and not by external trigger */
#define DAC_Trigger_T6_TRGO                ((uint32_t)0x00000004) /* TIM6 TRGO selected as external conversion trigger for DAC channel */
#define DAC_Trigger_T8_TRGO                ((uint32_t)0x0000000C) /* TIM8 TRGO selected as external conversion trigger for DAC channel
                                                                     only in High-density devices*/
#define DAC_Trigger_T7_TRGO                ((uint32_t)0x00000014) /* TIM7 TRGO selected as external conversion trigger for DAC channel */
#define DAC_Trigger_T5_TRGO                ((uint32_t)0x0000001C) /* TIM5 TRGO selected as external conversion trigger for DAC channel */
#define DAC_Trigger_T2_TRGO                ((uint32_t)0x00000024) /* TIM2 TRGO selected as external conversion trigger for DAC channel */
#define DAC_Trigger_T4_TRGO                ((uint32_t)0x0000002C) /* TIM4 TRGO selected as external conversion trigger for DAC channel */
#define DAC_Trigger_Ext_IT9                ((uint32_t)0x00000034) /* EXTI Line9 event selected as external conversion trigger for DAC channel */
#define DAC_Trigger_Software               ((uint32_t)0x0000003C) /* Conversion started by software trigger for DAC channel */

/* DAC_wave_generation */
#define DAC_WaveGeneration_None            ((uint32_t)0x00000000)
#define DAC_WaveGeneration_Noise           ((uint32_t)0x00000040)
#define DAC_WaveGeneration_Triangle        ((uint32_t)0x00000080)


/* DAC_lfsrunmask_triangleamplitude */
#define DAC_LFSRUnmask_Bit0                ((uint32_t)0x00000000) /* Unmask DAC channel LFSR bit0 for noise wave generation */
#define DAC_LFSRUnmask_Bits1_0             ((uint32_t)0x00000100) /* Unmask DAC channel LFSR bit[1:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits2_0             ((uint32_t)0x00000200) /* Unmask DAC channel LFSR bit[2:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits3_0             ((uint32_t)0x00000300) /* Unmask DAC channel LFSR bit[3:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits4_0             ((uint32_t)0x00000400) /* Unmask DAC channel LFSR bit[4:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits5_0             ((uint32_t)0x00000500) /* Unmask DAC channel LFSR bit[5:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits6_0             ((uint32_t)0x00000600) /* Unmask DAC channel LFSR bit[6:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits7_0             ((uint32_t)0x00000700) /* Unmask DAC channel LFSR bit[7:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits8_0             ((uint32_t)0x00000800) /* Unmask DAC channel LFSR bit[8:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits9_0             ((uint32_t)0x00000900) /* Unmask DAC channel LFSR bit[9:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits10_0            ((uint32_t)0x00000A00) /* Unmask DAC channel LFSR bit[10:0] for noise wave generation */
#define DAC_LFSRUnmask_Bits11_0            ((uint32_t)0x00000B00) /* Unmask DAC channel LFSR bit[11:0] for noise wave generation */
#define DAC_TriangleAmplitude_1            ((uint32_t)0x00000000) /* Select max triangle amplitude of 1 */
#define DAC_TriangleAmplitude_3            ((uint32_t)0x00000100) /* Select max triangle amplitude of 3 */
#define DAC_TriangleAmplitude_7            ((uint32_t)0x00000200) /* Select max triangle amplitude of 7 */
#define DAC_TriangleAmplitude_15           ((uint32_t)0x00000300) /* Select max triangle amplitude of 15 */
#define DAC_TriangleAmplitude_31           ((uint32_t)0x00000400) /* Select max triangle amplitude of 31 */
#define DAC_TriangleAmplitude_63           ((uint32_t)0x00000500) /* Select max triangle amplitude of 63 */
#define DAC_TriangleAmplitude_127          ((uint32_t)0x00000600) /* Select max triangle amplitude of 127 */
#define DAC_TriangleAmplitude_255          ((uint32_t)0x00000700) /* Select max triangle amplitude of 255 */
#define DAC_TriangleAmplitude_511          ((uint32_t)0x00000800) /* Select max triangle amplitude of 511 */
#define DAC_TriangleAmplitude_1023         ((uint32_t)0x00000900) /* Select max triangle amplitude of 1023 */
#define DAC_TriangleAmplitude_2047         ((uint32_t)0x00000A00) /* Select max triangle amplitude of 2047 */
#define DAC_TriangleAmplitude_4095         ((uint32_t)0x00000B00) /* Select max triangle amplitude of 4095 */

/* DAC_output_buffer */
#define DAC_OutputBuffer_Enable            ((uint32_t)0x00000000)
#define DAC_OutputBuffer_Disable           ((uint32_t)0x00000002)

/* DAC_Channel_selection */
#define DAC_Channel_1                      ((uint32_t)0x00000000)
#define DAC_Channel_2                      ((uint32_t)0x00000010)

/* DAC_data_alignment */
#define DAC_Align_12b_R                    ((uint32_t)0x00000000)
#define DAC_Align_12b_L                    ((uint32_t)0x00000004)
#define DAC_Align_8b_R                     ((uint32_t)0x00000008)

/* DAC_wave_generation */
#define DAC_Wave_Noise                     ((uint32_t)0x00000040)
#define DAC_Wave_Triangle                  ((uint32_t)0x00000080)
#endif

/* ch32v00x_dbgmcu.h ---------------------------------------------------------*/

/* CFGR0 Register */
#ifdef CH32V003
#define DBGMCU_IWDG_STOP             ((uint32_t)0x00000001)
#define DBGMCU_WWDG_STOP             ((uint32_t)0x00000002)
#define DBGMCU_TIM1_STOP             ((uint32_t)0x00000010)
#define DBGMCU_TIM2_STOP             ((uint32_t)0x00000020)
#elif defined(CH32V20x) || defined(CH32V30x)
#define DBGMCU_SLEEP                 ((uint32_t)0x00000001)
#define DBGMCU_STOP                  ((uint32_t)0x00000002)
#define DBGMCU_STANDBY               ((uint32_t)0x00000004)
#define DBGMCU_IWDG_STOP             ((uint32_t)0x00000100)
#define DBGMCU_WWDG_STOP             ((uint32_t)0x00000200)
#define DBGMCU_I2C1_SMBUS_TIMEOUT    ((uint32_t)0x00000400)
#define DBGMCU_I2C2_SMBUS_TIMEOUT    ((uint32_t)0x00000800)
#define DBGMCU_TIM1_STOP             ((uint32_t)0x00001000)
#define DBGMCU_TIM2_STOP             ((uint32_t)0x00002000)
#define DBGMCU_TIM3_STOP             ((uint32_t)0x00004000)
#define DBGMCU_TIM4_STOP             ((uint32_t)0x00008000)
#define DBGMCU_TIM5_STOP             ((uint32_t)0x00010000)
#define DBGMCU_TIM6_STOP             ((uint32_t)0x00020000)
#define DBGMCU_TIM7_STOP             ((uint32_t)0x00040000)
#define DBGMCU_TIM8_STOP             ((uint32_t)0x00080000)
#define DBGMCU_CAN1_STOP             ((uint32_t)0x00100000)
#define DBGMCU_CAN2_STOP             ((uint32_t)0x00200000)
#define DBGMCU_TIM9_STOP             ((uint32_t)0x00400000)
#define DBGMCU_TIM10_STOP            ((uint32_t)0x00800000)
#elif defined(CH32V10x)
#define DBGMCU_IWDG_STOP             ((uint32_t)0x00000001)
#define DBGMCU_WWDG_STOP             ((uint32_t)0x00000002)
#define DBGMCU_I2C1_SMBUS_TIMEOUT    ((uint32_t)0x00000004)
#define DBGMCU_I2C2_SMBUS_TIMEOUT    ((uint32_t)0x00000008)
#define DBGMCU_TIM1_STOP             ((uint32_t)0x00000010)
#define DBGMCU_TIM2_STOP             ((uint32_t)0x00000020)
#define DBGMCU_TIM3_STOP             ((uint32_t)0x00000040)
#define DBGMCU_TIM4_STOP             ((uint32_t)0x00000080)
#define DBGMCU_SLEEP                 ((uint32_t)0x00000001)
#define DBGMCU_STOP                  ((uint32_t)0x00000002)
#define DBGMCU_STANDBY               ((uint32_t)0x00000004)
#endif

/* ch32v00x_dma.h ------------------------------------------------------------*/

/* DMA_data_transfer_direction */
#define DMA_DIR_PeripheralDST              ((uint32_t)0x00000010)
#define DMA_DIR_PeripheralSRC              ((uint32_t)0x00000000)

/* DMA_peripheral_incremented_mode */
#define DMA_PeripheralInc_Enable           ((uint32_t)0x00000040)
#define DMA_PeripheralInc_Disable          ((uint32_t)0x00000000)

/* DMA_memory_incremented_mode */
#define DMA_MemoryInc_Enable               ((uint32_t)0x00000080)
#define DMA_MemoryInc_Disable              ((uint32_t)0x00000000)

/* DMA_peripheral_data_size */
#define DMA_PeripheralDataSize_Byte        ((uint32_t)0x00000000)
#define DMA_PeripheralDataSize_HalfWord    ((uint32_t)0x00000100)
#define DMA_PeripheralDataSize_Word        ((uint32_t)0x00000200)

/* DMA_memory_data_size */
#define DMA_MemoryDataSize_Byte            ((uint32_t)0x00000000)
#define DMA_MemoryDataSize_HalfWord        ((uint32_t)0x00000400)
#define DMA_MemoryDataSize_Word            ((uint32_t)0x00000800)

/* DMA_circular_normal_mode */
#define DMA_Mode_Circular                  ((uint32_t)0x00000020)
#define DMA_Mode_Normal                    ((uint32_t)0x00000000)

/* DMA_priority_level */
#define DMA_Priority_VeryHigh              ((uint32_t)0x00003000)
#define DMA_Priority_High                  ((uint32_t)0x00002000)
#define DMA_Priority_Medium                ((uint32_t)0x00001000)
#define DMA_Priority_Low                   ((uint32_t)0x00000000)

/* DMA_memory_to_memory */
#define DMA_M2M_Enable                     ((uint32_t)0x00004000)
#define DMA_M2M_Disable                    ((uint32_t)0x00000000)

/* DMA_interrupts_definition */
#define DMA_IT_TC                          ((uint32_t)0x00000002)
#define DMA_IT_HT                          ((uint32_t)0x00000004)
#define DMA_IT_TE                          ((uint32_t)0x00000008)

#define DMA1_IT_GL1                        ((uint32_t)0x00000001)
#define DMA1_IT_TC1                        ((uint32_t)0x00000002)
#define DMA1_IT_HT1                        ((uint32_t)0x00000004)
#define DMA1_IT_TE1                        ((uint32_t)0x00000008)
#define DMA1_IT_GL2                        ((uint32_t)0x00000010)
#define DMA1_IT_TC2                        ((uint32_t)0x00000020)
#define DMA1_IT_HT2                        ((uint32_t)0x00000040)
#define DMA1_IT_TE2                        ((uint32_t)0x00000080)
#define DMA1_IT_GL3                        ((uint32_t)0x00000100)
#define DMA1_IT_TC3                        ((uint32_t)0x00000200)
#define DMA1_IT_HT3                        ((uint32_t)0x00000400)
#define DMA1_IT_TE3                        ((uint32_t)0x00000800)
#define DMA1_IT_GL4                        ((uint32_t)0x00001000)
#define DMA1_IT_TC4                        ((uint32_t)0x00002000)
#define DMA1_IT_HT4                        ((uint32_t)0x00004000)
#define DMA1_IT_TE4                        ((uint32_t)0x00008000)
#define DMA1_IT_GL5                        ((uint32_t)0x00010000)
#define DMA1_IT_TC5                        ((uint32_t)0x00020000)
#define DMA1_IT_HT5                        ((uint32_t)0x00040000)
#define DMA1_IT_TE5                        ((uint32_t)0x00080000)
#define DMA1_IT_GL6                        ((uint32_t)0x00100000)
#define DMA1_IT_TC6                        ((uint32_t)0x00200000)
#define DMA1_IT_HT6                        ((uint32_t)0x00400000)
#define DMA1_IT_TE6                        ((uint32_t)0x00800000)
#define DMA1_IT_GL7                        ((uint32_t)0x01000000)
#define DMA1_IT_TC7                        ((uint32_t)0x02000000)
#define DMA1_IT_HT7                        ((uint32_t)0x04000000)
#define DMA1_IT_TE7                        ((uint32_t)0x08000000)
#if defined(CH32V20x)
#define DMA1_IT_GL8                        ((uint32_t)0x10000000)
#define DMA1_IT_TC8                        ((uint32_t)0x20000000)
#define DMA1_IT_HT8                        ((uint32_t)0x40000000)
#define DMA1_IT_TE8                        ((uint32_t)0x80000000)
#endif

#if defined(CH32V10x) || defined(CH32V30x)
#define DMA2_IT_GL1                        ((uint32_t)0x10000001)
#define DMA2_IT_TC1                        ((uint32_t)0x10000002)
#define DMA2_IT_HT1                        ((uint32_t)0x10000004)
#define DMA2_IT_TE1                        ((uint32_t)0x10000008)
#define DMA2_IT_GL2                        ((uint32_t)0x10000010)
#define DMA2_IT_TC2                        ((uint32_t)0x10000020)
#define DMA2_IT_HT2                        ((uint32_t)0x10000040)
#define DMA2_IT_TE2                        ((uint32_t)0x10000080)
#define DMA2_IT_GL3                        ((uint32_t)0x10000100)
#define DMA2_IT_TC3                        ((uint32_t)0x10000200)
#define DMA2_IT_HT3                        ((uint32_t)0x10000400)
#define DMA2_IT_TE3                        ((uint32_t)0x10000800)
#define DMA2_IT_GL4                        ((uint32_t)0x10001000)
#define DMA2_IT_TC4                        ((uint32_t)0x10002000)
#define DMA2_IT_HT4                        ((uint32_t)0x10004000)
#define DMA2_IT_TE4                        ((uint32_t)0x10008000)
#define DMA2_IT_GL5                        ((uint32_t)0x10010000)
#define DMA2_IT_TC5                        ((uint32_t)0x10020000)
#define DMA2_IT_HT5                        ((uint32_t)0x10040000)
#define DMA2_IT_TE5                        ((uint32_t)0x10080000)
#endif
#if defined(CH32V30x)
#define DMA2_IT_GL6                        ((uint32_t)0x10100000)
#define DMA2_IT_TC6                        ((uint32_t)0x10200000)
#define DMA2_IT_HT6                        ((uint32_t)0x10400000)
#define DMA2_IT_TE6                        ((uint32_t)0x10800000)
#define DMA2_IT_GL7                        ((uint32_t)0x11000000)
#define DMA2_IT_TC7                        ((uint32_t)0x12000000)
#define DMA2_IT_HT7                        ((uint32_t)0x14000000)
#define DMA2_IT_TE7                        ((uint32_t)0x18000000)

#define DMA2_IT_GL8                        ((uint32_t)0x20000001)
#define DMA2_IT_TC8                        ((uint32_t)0x20000002)
#define DMA2_IT_HT8                        ((uint32_t)0x20000004)
#define DMA2_IT_TE8                        ((uint32_t)0x20000008)
#define DMA2_IT_GL9                        ((uint32_t)0x20000010)
#define DMA2_IT_TC9                        ((uint32_t)0x20000020)
#define DMA2_IT_HT9                        ((uint32_t)0x20000040)
#define DMA2_IT_TE9                        ((uint32_t)0x20000080)
#define DMA2_IT_GL10                       ((uint32_t)0x20000100)
#define DMA2_IT_TC10                       ((uint32_t)0x20000200)
#define DMA2_IT_HT10                       ((uint32_t)0x20000400)
#define DMA2_IT_TE10                       ((uint32_t)0x20000800)
#define DMA2_IT_GL11                       ((uint32_t)0x20001000)
#define DMA2_IT_TC11                       ((uint32_t)0x20002000)
#define DMA2_IT_HT11                       ((uint32_t)0x20004000)
#define DMA2_IT_TE11                       ((uint32_t)0x20008000)
#endif

/* DMA_flags_definition */
#define DMA1_FLAG_GL1                      ((uint32_t)0x00000001)
#define DMA1_FLAG_TC1                      ((uint32_t)0x00000002)
#define DMA1_FLAG_HT1                      ((uint32_t)0x00000004)
#define DMA1_FLAG_TE1                      ((uint32_t)0x00000008)
#define DMA1_FLAG_GL2                      ((uint32_t)0x00000010)
#define DMA1_FLAG_TC2                      ((uint32_t)0x00000020)
#define DMA1_FLAG_HT2                      ((uint32_t)0x00000040)
#define DMA1_FLAG_TE2                      ((uint32_t)0x00000080)
#define DMA1_FLAG_GL3                      ((uint32_t)0x00000100)
#define DMA1_FLAG_TC3                      ((uint32_t)0x00000200)
#define DMA1_FLAG_HT3                      ((uint32_t)0x00000400)
#define DMA1_FLAG_TE3                      ((uint32_t)0x00000800)
#define DMA1_FLAG_GL4                      ((uint32_t)0x00001000)
#define DMA1_FLAG_TC4                      ((uint32_t)0x00002000)
#define DMA1_FLAG_HT4                      ((uint32_t)0x00004000)
#define DMA1_FLAG_TE4                      ((uint32_t)0x00008000)
#define DMA1_FLAG_GL5                      ((uint32_t)0x00010000)
#define DMA1_FLAG_TC5                      ((uint32_t)0x00020000)
#define DMA1_FLAG_HT5                      ((uint32_t)0x00040000)
#define DMA1_FLAG_TE5                      ((uint32_t)0x00080000)
#define DMA1_FLAG_GL6                      ((uint32_t)0x00100000)
#define DMA1_FLAG_TC6                      ((uint32_t)0x00200000)
#define DMA1_FLAG_HT6                      ((uint32_t)0x00400000)
#define DMA1_FLAG_TE6                      ((uint32_t)0x00800000)
#define DMA1_FLAG_GL7                      ((uint32_t)0x01000000)
#define DMA1_FLAG_TC7                      ((uint32_t)0x02000000)
#define DMA1_FLAG_HT7                      ((uint32_t)0x04000000)
#define DMA1_FLAG_TE7                      ((uint32_t)0x08000000)
#if defined(CH32V20x)
#define DMA1_FLAG_GL8                      ((uint32_t)0x10000000)
#define DMA1_FLAG_TC8                      ((uint32_t)0x20000000)
#define DMA1_FLAG_HT8                      ((uint32_t)0x40000000)
#define DMA1_FLAG_TE8                      ((uint32_t)0x80000000)
#endif

#if defined(CH32V10x) || defined(CH32V30x)
#define DMA2_FLAG_GL1                      ((uint32_t)0x10000001)
#define DMA2_FLAG_TC1                      ((uint32_t)0x10000002)
#define DMA2_FLAG_HT1                      ((uint32_t)0x10000004)
#define DMA2_FLAG_TE1                      ((uint32_t)0x10000008)
#define DMA2_FLAG_GL2                      ((uint32_t)0x10000010)
#define DMA2_FLAG_TC2                      ((uint32_t)0x10000020)
#define DMA2_FLAG_HT2                      ((uint32_t)0x10000040)
#define DMA2_FLAG_TE2                      ((uint32_t)0x10000080)
#define DMA2_FLAG_GL3                      ((uint32_t)0x10000100)
#define DMA2_FLAG_TC3                      ((uint32_t)0x10000200)
#define DMA2_FLAG_HT3                      ((uint32_t)0x10000400)
#define DMA2_FLAG_TE3                      ((uint32_t)0x10000800)
#define DMA2_FLAG_GL4                      ((uint32_t)0x10001000)
#define DMA2_FLAG_TC4                      ((uint32_t)0x10002000)
#define DMA2_FLAG_HT4                      ((uint32_t)0x10004000)
#define DMA2_FLAG_TE4                      ((uint32_t)0x10008000)
#define DMA2_FLAG_GL5                      ((uint32_t)0x10010000)
#define DMA2_FLAG_TC5                      ((uint32_t)0x10020000)
#define DMA2_FLAG_HT5                      ((uint32_t)0x10040000)
#define DMA2_FLAG_TE5                      ((uint32_t)0x10080000)
#endif

#if defined(CH32V30x)
#define DMA2_FLAG_GL6                      ((uint32_t)0x10100000)
#define DMA2_FLAG_TC6                      ((uint32_t)0x10200000)
#define DMA2_FLAG_HT6                      ((uint32_t)0x10400000)
#define DMA2_FLAG_TE6                      ((uint32_t)0x10800000)
#define DMA2_FLAG_GL7                      ((uint32_t)0x11000000)
#define DMA2_FLAG_TC7                      ((uint32_t)0x12000000)
#define DMA2_FLAG_HT7                      ((uint32_t)0x14000000)
#define DMA2_FLAG_TE7                      ((uint32_t)0x18000000)

#define DMA2_FLAG_GL8                      ((uint32_t)0x20000001)
#define DMA2_FLAG_TC8                      ((uint32_t)0x20000002)
#define DMA2_FLAG_HT8                      ((uint32_t)0x20000004)
#define DMA2_FLAG_TE8                      ((uint32_t)0x20000008)
#define DMA2_FLAG_GL9                      ((uint32_t)0x20000010)
#define DMA2_FLAG_TC9                      ((uint32_t)0x20000020)
#define DMA2_FLAG_HT9                      ((uint32_t)0x20000040)
#define DMA2_FLAG_TE9                      ((uint32_t)0x20000080)
#define DMA2_FLAG_GL10                     ((uint32_t)0x20000100)
#define DMA2_FLAG_TC10                     ((uint32_t)0x20000200)
#define DMA2_FLAG_HT10                     ((uint32_t)0x20000400)
#define DMA2_FLAG_TE10                     ((uint32_t)0x20000800)
#define DMA2_FLAG_GL11                     ((uint32_t)0x20001000)
#define DMA2_FLAG_TC11                     ((uint32_t)0x20002000)
#define DMA2_FLAG_HT11                     ((uint32_t)0x20004000)
#define DMA2_FLAG_TE11                     ((uint32_t)0x20008000)
#endif

#if defined(CH32V30x)
/* ch32v00x_eth.h ------------------------------------------------------------*/

#define PHY_10BASE_T_LINKED   1
#define PHY_10BASE_T_NOT_LINKED   0

#define DMA_TPS_Mask      ((uint32_t)0x00700000)
#define DMA_RPS_Mask      ((uint32_t)0x000E0000)

/* ETH delay.Just for Ethernet */
#define _eth_delay_    ETH_Delay       /* Default _eth_delay_ function with less precise timing */

/* definition for Ethernet frame */
#define ETH_MAX_PACKET_SIZE    1536    /* ETH_HEADER + ETH_EXTRA + MAX_ETH_PAYLOAD + ETH_CRC */
#define ETH_HEADER               14    /* 6 byte Dest addr, 6 byte Src addr, 2 byte length/type */
#define ETH_CRC                   4    /* Ethernet CRC */
#define ETH_EXTRA                 2    /* Extra bytes in some cases */
#define VLAN_TAG                  4    /* optional 802.1q VLAN Tag */
#define MIN_ETH_PAYLOAD          46    /* Minimum Ethernet payload size */
#define MAX_ETH_PAYLOAD        1500    /* Maximum Ethernet payload size */
#define JUMBO_FRAME_PAYLOAD    9000    /* Jumbo frame payload size */

/**
   DMA Tx Desciptor
  -----------------------------------------------------------------------------------------------
  TDES0 | OWN(31) | CTRL[30:26] | Reserved[25:24] | CTRL[23:20] | Reserved[19:17] | Status[16:0] |
  -----------------------------------------------------------------------------------------------
  TDES1 | Reserved[31:29] | Buffer2 ByteCount[28:16] | Reserved[15:13] | Buffer1 ByteCount[12:0] |
  -----------------------------------------------------------------------------------------------
  TDES2 |                         Buffer1 Address [31:0]                                         |
  -----------------------------------------------------------------------------------------------
  TDES3 |                   Buffer2 Address [31:0] / Next Desciptor Address [31:0]               |
  ------------------------------------------------------------------------------------------------
*/


/* Bit or field definition of TDES0 register (DMA Tx descriptor status register)*/
#define ETH_DMATxDesc_OWN                     ((uint32_t)0x80000000)  /* OWN bit: descriptor is owned by DMA engine */
#define ETH_DMATxDesc_IC                      ((uint32_t)0x40000000)  /* Interrupt on Completion */
#define ETH_DMATxDesc_LS                      ((uint32_t)0x20000000)  /* Last Segment */
#define ETH_DMATxDesc_FS                      ((uint32_t)0x10000000)  /* First Segment */
#define ETH_DMATxDesc_DC                      ((uint32_t)0x08000000)  /* Disable CRC */
#define ETH_DMATxDesc_DP                      ((uint32_t)0x04000000)  /* Disable Padding */
#define ETH_DMATxDesc_TTSE                    ((uint32_t)0x02000000)  /* Transmit Time Stamp Enable */
#define ETH_DMATxDesc_CIC                     ((uint32_t)0x00C00000)  /* Checksum Insertion Control: 4 cases */
#define ETH_DMATxDesc_CIC_ByPass              ((uint32_t)0x00000000)  /* Do Nothing: Checksum Engine is bypassed */
#define ETH_DMATxDesc_CIC_IPV4Header          ((uint32_t)0x00400000)  /* IPV4 header Checksum Insertion */
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Segment  ((uint32_t)0x00800000)  /* TCP/UDP/ICMP Checksum Insertion calculated over segment only */
#define ETH_DMATxDesc_CIC_TCPUDPICMP_Full     ((uint32_t)0x00C00000)  /* TCP/UDP/ICMP Checksum Insertion fully calculated */
#define ETH_DMATxDesc_TER                     ((uint32_t)0x00200000)  /* Transmit End of Ring */
#define ETH_DMATxDesc_TCH                     ((uint32_t)0x00100000)  /* Second Address Chained */
#define ETH_DMATxDesc_TTSS                    ((uint32_t)0x00020000)  /* Tx Time Stamp Status */
#define ETH_DMATxDesc_IHE                     ((uint32_t)0x00010000)  /* IP Header Error */
#define ETH_DMATxDesc_ES                      ((uint32_t)0x00008000)  /* Error summary: OR of the following bits: UE || ED || EC || LCO || NC || LCA || FF || JT */
#define ETH_DMATxDesc_JT                      ((uint32_t)0x00004000)  /* Jabber Timeout */
#define ETH_DMATxDesc_FF                      ((uint32_t)0x00002000)  /* Frame Flushed: DMA/MTL flushed the frame due to SW flush */
#define ETH_DMATxDesc_PCE                     ((uint32_t)0x00001000)  /* Payload Checksum Error */
#define ETH_DMATxDesc_LCA                     ((uint32_t)0x00000800)  /* Loss of Carrier: carrier lost during tramsmission */
#define ETH_DMATxDesc_NC                      ((uint32_t)0x00000400)  /* No Carrier: no carrier signal from the tranceiver */
#define ETH_DMATxDesc_LCO                     ((uint32_t)0x00000200)  /* Late Collision: transmission aborted due to collision */
#define ETH_DMATxDesc_EC                      ((uint32_t)0x00000100)  /* Excessive Collision: transmission aborted after 16 collisions */
#define ETH_DMATxDesc_VF                      ((uint32_t)0x00000080)  /* VLAN Frame */
#define ETH_DMATxDesc_CC                      ((uint32_t)0x00000078)  /* Collision Count */
#define ETH_DMATxDesc_ED                      ((uint32_t)0x00000004)  /* Excessive Deferral */
#define ETH_DMATxDesc_UF                      ((uint32_t)0x00000002)  /* Underflow Error: late data arrival from the memory */
#define ETH_DMATxDesc_DB                      ((uint32_t)0x00000001)  /* Deferred Bit */

/* Field definition of TDES1 register */
#define ETH_DMATxDesc_TBS2  ((uint32_t)0x1FFF0000)  /* Transmit Buffer2 Size */
#define ETH_DMATxDesc_TBS1  ((uint32_t)0x00001FFF)  /* Transmit Buffer1 Size */

/* Field definition of TDES2 register */
#define ETH_DMATxDesc_B1AP  ((uint32_t)0xFFFFFFFF)  /* Buffer1 Address Pointer */

/* Field definition of TDES3 register */
#define ETH_DMATxDesc_B2AP  ((uint32_t)0xFFFFFFFF)  /* Buffer2 Address Pointer */

/**
  DMA Rx Desciptor
  ---------------------------------------------------------------------------------------------------------------------
  RDES0 | OWN(31) |                                             Status [30:0]                                          |
  ---------------------------------------------------------------------------------------------------------------------
  RDES1 | CTRL(31) | Reserved[30:29] | Buffer2 ByteCount[28:16] | CTRL[15:14] | Reserved(13) | Buffer1 ByteCount[12:0] |
  ---------------------------------------------------------------------------------------------------------------------
  RDES2 |                                       Buffer1 Address [31:0]                                                 |
  ---------------------------------------------------------------------------------------------------------------------
  RDES3 |                          Buffer2 Address [31:0] / Next Desciptor Address [31:0]                              |
  ----------------------------------------------------------------------------------------------------------------------
*/

/* Bit or field definition of RDES0 register (DMA Rx descriptor status register) */
#define ETH_DMARxDesc_OWN         ((uint32_t)0x80000000)  /* OWN bit: descriptor is owned by DMA engine  */
#define ETH_DMARxDesc_AFM         ((uint32_t)0x40000000)  /* DA Filter Fail for the rx frame  */
#define ETH_DMARxDesc_FL          ((uint32_t)0x3FFF0000)  /* Receive descriptor frame length  */
#define ETH_DMARxDesc_ES          ((uint32_t)0x00008000)  /* Error summary: OR of the following bits: DE || OE || IPC || LC || RWT || RE || CE */
#define ETH_DMARxDesc_DE          ((uint32_t)0x00004000)  /* Desciptor error: no more descriptors for receive frame  */
#define ETH_DMARxDesc_SAF         ((uint32_t)0x00002000)  /* SA Filter Fail for the received frame */
#define ETH_DMARxDesc_LE          ((uint32_t)0x00001000)  /* Frame size not matching with length field */
#define ETH_DMARxDesc_OE          ((uint32_t)0x00000800)  /* Overflow Error: Frame was damaged due to buffer overflow */
#define ETH_DMARxDesc_VLAN        ((uint32_t)0x00000400)  /* VLAN Tag: received frame is a VLAN frame */
#define ETH_DMARxDesc_FS          ((uint32_t)0x00000200)  /* First descriptor of the frame  */
#define ETH_DMARxDesc_LS          ((uint32_t)0x00000100)  /* Last descriptor of the frame  */
#define ETH_DMARxDesc_IPV4HCE     ((uint32_t)0x00000080)  /* IPC Checksum Error: Rx Ipv4 header checksum error   */
#define ETH_DMARxDesc_LC          ((uint32_t)0x00000040)  /* Late collision occurred during reception   */
#define ETH_DMARxDesc_FT          ((uint32_t)0x00000020)  /* Frame type - Ethernet, otherwise 802.3    */
#define ETH_DMARxDesc_RWT         ((uint32_t)0x00000010)  /* Receive Watchdog Timeout: watchdog timer expired during reception    */
#define ETH_DMARxDesc_RE          ((uint32_t)0x00000008)  /* Receive error: error reported by MII interface  */
#define ETH_DMARxDesc_DBE         ((uint32_t)0x00000004)  /* Dribble bit error: frame contains non int multiple of 8 bits  */
#define ETH_DMARxDesc_CE          ((uint32_t)0x00000002)  /* CRC error */
#define ETH_DMARxDesc_MAMPCE      ((uint32_t)0x00000001)  /* Rx MAC Address/Payload Checksum Error: Rx MAC address matched/ Rx Payload Checksum Error */

/* Bit or field definition of RDES1 register */
#define ETH_DMARxDesc_DIC   ((uint32_t)0x80000000)  /* Disable Interrupt on Completion */
#define ETH_DMARxDesc_RBS2  ((uint32_t)0x1FFF0000)  /* Receive Buffer2 Size */
#define ETH_DMARxDesc_RER   ((uint32_t)0x00008000)  /* Receive End of Ring */
#define ETH_DMARxDesc_RCH   ((uint32_t)0x00004000)  /* Second Address Chained */
#define ETH_DMARxDesc_RBS1  ((uint32_t)0x00001FFF)  /* Receive Buffer1 Size */

/* Field definition of RDES2 register */
#define ETH_DMARxDesc_B1AP  ((uint32_t)0xFFFFFFFF)  /* Buffer1 Address Pointer */

/* Field definition of RDES3 register */
#define ETH_DMARxDesc_B2AP  ((uint32_t)0xFFFFFFFF)  /* Buffer2 Address Pointer */

/* Timeout threshold of Reading or writing PHY registers */
#define PHY_READ_TO                     ((uint32_t)0x004FFFFF)
#define PHY_WRITE_TO                    ((uint32_t)0x0004FFFF)

/* Delay time after reset PHY */
#define PHY_ResetDelay                  ((uint32_t)0x000FFFFF)

/* Delay time after configure PHY */
#define PHY_ConfigDelay                 ((uint32_t)0x00FFFFFF)

/* PHY basic register */
#define PHY_BCR                          0x0           /*PHY transceiver Basic Control Register */
#define PHY_BSR                          0x01          /*PHY transceiver Basic Status Register*/
#define PHY_ANAR                         0x04          /* Auto-Negotiation Advertisement Register */
#define PHY_ANLPAR                       0x05          /* Auto-Negotiation Link Partner Base  Page Ability Register*/
#define PHY_ANER                         0x06          /* Auto-Negotiation Expansion Register */
#define PHY_BMCR                         PHY_BCR
#define PHY_BMSR                         PHY_BSR
#define PHY_STATUS                       0x10
#define PHY_MDIX                         0x1E

/* Bit or field definition for PHY basic control register */
#define PHY_Reset                       ((uint16_t)0x8000)      /* PHY Reset */
#define PHY_Loopback                    ((uint16_t)0x4000)      /* Select loop-back mode */
#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100)      /* Set the full-duplex mode at 100 Mb/s */
#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000)      /* Set the half-duplex mode at 100 Mb/s */
#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100)      /* Set the full-duplex mode at 10 Mb/s */
#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000)      /* Set the half-duplex mode at 10 Mb/s */
#define PHY_AutoNegotiation             ((uint16_t)0x1000)      /* Enable auto-negotiation function */
#define PHY_Restart_AutoNegotiation     ((uint16_t)0x0200)      /* Restart auto-negotiation function */
#define PHY_Powerdown                   ((uint16_t)0x0800)      /* Select the power down mode */
#define PHY_Isolate                     ((uint16_t)0x0400)      /* Isolate PHY from MII */

/* Bit or field definition for PHY basic status register */
#define PHY_AutoNego_Complete           ((uint16_t)0x0020)      /* Auto-Negotioation process completed */
#define PHY_Linked_Status               ((uint16_t)0x0004)      /* Valid link established */
#define PHY_Jabber_detection            ((uint16_t)0x0002)      /* Jabber condition detected */
#define PHY_RMII_Mode                   ((uint16_t)0x0020)      /* RMII */


/* Internal 10BASE-T PHY 50R*4 pull-up resistance enable or disable */
#define ETH_Internal_Pull_Up_Res_Enable     ((uint32_t)0x00100000)
#define ETH_Internal_Pull_Up_Res_Disable    ((uint32_t)0x00000000)

/* MAC autoNegotiation enable or disable */
#define ETH_AutoNegotiation_Enable     ((uint32_t)0x00000001)
#define ETH_AutoNegotiation_Disable    ((uint32_t)0x00000000)

/* MAC watchdog enable or disable */
#define ETH_Watchdog_Enable       ((uint32_t)0x00000000)
#define ETH_Watchdog_Disable      ((uint32_t)0x00800000)

/* Bit description - MAC jabber enable or disable */
#define ETH_Jabber_Enable    ((uint32_t)0x00000000)
#define ETH_Jabber_Disable   ((uint32_t)0x00400000)

/* Value of minimum IFG between frames during transmission */
#define ETH_InterFrameGap_96Bit   ((uint32_t)0x00000000)  /* minimum IFG between frames during transmission is 96Bit */
#define ETH_InterFrameGap_88Bit   ((uint32_t)0x00020000)  /* minimum IFG between frames during transmission is 88Bit */
#define ETH_InterFrameGap_80Bit   ((uint32_t)0x00040000)  /* minimum IFG between frames during transmission is 80Bit */
#define ETH_InterFrameGap_72Bit   ((uint32_t)0x00060000)  /* minimum IFG between frames during transmission is 72Bit */
#define ETH_InterFrameGap_64Bit   ((uint32_t)0x00080000)  /* minimum IFG between frames during transmission is 64Bit */
#define ETH_InterFrameGap_56Bit   ((uint32_t)0x000A0000)  /* minimum IFG between frames during transmission is 56Bit */
#define ETH_InterFrameGap_48Bit   ((uint32_t)0x000C0000)  /* minimum IFG between frames during transmission is 48Bit */
#define ETH_InterFrameGap_40Bit   ((uint32_t)0x000E0000)  /* minimum IFG between frames during transmission is 40Bit */

/* MAC carrier sense enable or disable */
#define ETH_CarrierSense_Enable   ((uint32_t)0x00000000)
#define ETH_CarrierSense_Disable  ((uint32_t)0x00010000)

/* MAC speed */
#define ETH_Speed_10M        ((uint32_t)0x00000000)
#define ETH_Speed_100M       ((uint32_t)0x00004000)
#define ETH_Speed_1000M      ((uint32_t)0x00008000)

/* MAC receive own enable or disable */
#define ETH_ReceiveOwn_Enable     ((uint32_t)0x00000000)
#define ETH_ReceiveOwn_Disable    ((uint32_t)0x00002000)

/* MAC Loopback mode enable or disable */
#define ETH_LoopbackMode_Enable        ((uint32_t)0x00001000)
#define ETH_LoopbackMode_Disable       ((uint32_t)0x00000000)

/* MAC fullDuplex or halfDuplex */
#define ETH_Mode_FullDuplex       ((uint32_t)0x00000800)
#define ETH_Mode_HalfDuplex       ((uint32_t)0x00000000)

/* MAC offload checksum enable or disable */
#define ETH_ChecksumOffload_Enable     ((uint32_t)0x00000400)
#define ETH_ChecksumOffload_Disable    ((uint32_t)0x00000000)

/* MAC transmission retry enable or disable */
#define ETH_RetryTransmission_Enable   ((uint32_t)0x00000000)
#define ETH_RetryTransmission_Disable  ((uint32_t)0x00000200)

/* MAC automatic pad CRC strip enable or disable */
#define ETH_AutomaticPadCRCStrip_Enable     ((uint32_t)0x00000080)
#define ETH_AutomaticPadCRCStrip_Disable    ((uint32_t)0x00000000)

/* MAC backoff limitation */
#define ETH_BackOffLimit_10  ((uint32_t)0x00000000)
#define ETH_BackOffLimit_8   ((uint32_t)0x00000020)
#define ETH_BackOffLimit_4   ((uint32_t)0x00000040)
#define ETH_BackOffLimit_1   ((uint32_t)0x00000060)

/* MAC deferral check enable or disable */
#define ETH_DeferralCheck_Enable       ((uint32_t)0x00000010)
#define ETH_DeferralCheck_Disable      ((uint32_t)0x00000000)

/* Bit description  : MAC receive all frame enable or disable */
#define ETH_ReceiveAll_Enable     ((uint32_t)0x80000000)
#define ETH_ReceiveAll_Disable    ((uint32_t)0x00000000)

/* MAC backoff limitation */
#define ETH_SourceAddrFilter_Normal_Enable       ((uint32_t)0x00000200)
#define ETH_SourceAddrFilter_Inverse_Enable      ((uint32_t)0x00000300)
#define ETH_SourceAddrFilter_Disable             ((uint32_t)0x00000000)

/* MAC Pass control frames */
#define ETH_PassControlFrames_BlockAll                ((uint32_t)0x00000040)  /* MAC filters all control frames from reaching the application */
#define ETH_PassControlFrames_ForwardAll              ((uint32_t)0x00000080)  /* MAC forwards all control frames to application even if they fail the Address Filter */
#define ETH_PassControlFrames_ForwardPassedAddrFilter ((uint32_t)0x000000C0)  /* MAC forwards control frames that pass the Address Filter. */

/* MAC broadcast frames reception */
#define ETH_BroadcastFramesReception_Enable      ((uint32_t)0x00000000)
#define ETH_BroadcastFramesReception_Disable     ((uint32_t)0x00000020)

/* MAC destination address filter */
#define ETH_DestinationAddrFilter_Normal    ((uint32_t)0x00000000)
#define ETH_DestinationAddrFilter_Inverse   ((uint32_t)0x00000008)

/* MAC Promiscuous mode enable or disable */
#define ETH_PromiscuousMode_Enable     ((uint32_t)0x00000001)
#define ETH_PromiscuousMode_Disable    ((uint32_t)0x00000000)

/* MAC multicast frames filter */
#define ETH_MulticastFramesFilter_PerfectHashTable    ((uint32_t)0x00000404)
#define ETH_MulticastFramesFilter_HashTable           ((uint32_t)0x00000004)
#define ETH_MulticastFramesFilter_Perfect             ((uint32_t)0x00000000)
#define ETH_MulticastFramesFilter_None                ((uint32_t)0x00000010)

/* MAC unicast frames filter */
#define ETH_UnicastFramesFilter_PerfectHashTable ((uint32_t)0x00000402)
#define ETH_UnicastFramesFilter_HashTable        ((uint32_t)0x00000002)
#define ETH_UnicastFramesFilter_Perfect          ((uint32_t)0x00000000)

/* Bit description  : MAC zero quanta pause */
#define ETH_ZeroQuantaPause_Enable     ((uint32_t)0x00000000)
#define ETH_ZeroQuantaPause_Disable    ((uint32_t)0x00000080)

/* Field description  : MAC pause low threshold */
#define ETH_PauseLowThreshold_Minus4        ((uint32_t)0x00000000)  /* Pause time minus 4 slot times */
#define ETH_PauseLowThreshold_Minus28       ((uint32_t)0x00000010)  /* Pause time minus 28 slot times */
#define ETH_PauseLowThreshold_Minus144      ((uint32_t)0x00000020)  /* Pause time minus 144 slot times */
#define ETH_PauseLowThreshold_Minus256      ((uint32_t)0x00000030)  /* Pause time minus 256 slot times */

/* MAC unicast pause frame detect enable or disable*/
#define ETH_UnicastPauseFrameDetect_Enable  ((uint32_t)0x00000008)
#define ETH_UnicastPauseFrameDetect_Disable ((uint32_t)0x00000000)

/* MAC receive flow control frame enable or disable */
#define ETH_ReceiveFlowControl_Enable       ((uint32_t)0x00000004)
#define ETH_ReceiveFlowControl_Disable      ((uint32_t)0x00000000)

/* MAC transmit flow control enable or disable */
#define ETH_TransmitFlowControl_Enable      ((uint32_t)0x00000002)
#define ETH_TransmitFlowControl_Disable     ((uint32_t)0x00000000)

/* MAC VLAN tag comparison */
#define ETH_VLANTagComparison_12Bit    ((uint32_t)0x00010000)
#define ETH_VLANTagComparison_16Bit    ((uint32_t)0x00000000)

/* MAC flag */
#define ETH_MAC_FLAG_TST     ((uint32_t)0x00000200)  /* Time stamp trigger flag (on MAC) */
#define ETH_MAC_FLAG_MMCT    ((uint32_t)0x00000040)  /* MMC transmit flag  */
#define ETH_MAC_FLAG_MMCR    ((uint32_t)0x00000020)  /* MMC receive flag */
#define ETH_MAC_FLAG_MMC     ((uint32_t)0x00000010)  /* MMC flag (on MAC) */
#define ETH_MAC_FLAG_PMT     ((uint32_t)0x00000008)  /* PMT flag (on MAC) */

/* MAC interrupt */
#define ETH_MAC_IT_TST       ((uint32_t)0x00000200)  /* Time stamp trigger interrupt (on MAC) */
#define ETH_MAC_IT_MMCT      ((uint32_t)0x00000040)  /* MMC transmit interrupt */
#define ETH_MAC_IT_MMCR      ((uint32_t)0x00000020)  /* MMC receive interrupt */
#define ETH_MAC_IT_MMC       ((uint32_t)0x00000010)  /* MMC interrupt (on MAC) */
#define ETH_MAC_IT_PMT       ((uint32_t)0x00000008)  /* PMT interrupt (on MAC) */

/* MAC address */
#define ETH_MAC_Address0     ((uint32_t)0x00000000)
#define ETH_MAC_Address1     ((uint32_t)0x00000008)
#define ETH_MAC_Address2     ((uint32_t)0x00000010)
#define ETH_MAC_Address3     ((uint32_t)0x00000018)

/* MAC address filter select */
#define ETH_MAC_AddressFilter_SA       ((uint32_t)0x00000000)
#define ETH_MAC_AddressFilter_DA       ((uint32_t)0x00000008)

/* MAC address mask */
#define ETH_MAC_AddressMask_Byte6      ((uint32_t)0x20000000)  /* Mask MAC Address high reg bits [15:8] */
#define ETH_MAC_AddressMask_Byte5      ((uint32_t)0x10000000)  /* Mask MAC Address high reg bits [7:0] */
#define ETH_MAC_AddressMask_Byte4      ((uint32_t)0x08000000)  /* Mask MAC Address low reg bits [31:24] */
#define ETH_MAC_AddressMask_Byte3      ((uint32_t)0x04000000)  /* Mask MAC Address low reg bits [23:16] */
#define ETH_MAC_AddressMask_Byte2      ((uint32_t)0x02000000)  /* Mask MAC Address low reg bits [15:8] */
#define ETH_MAC_AddressMask_Byte1      ((uint32_t)0x01000000)  /* Mask MAC Address low reg bits [70] */


/******************************************************************************/
/*                                                                            */
/*                          MAC Descriptor Register                                                                                                                                     */
/*                                                                            */
/******************************************************************************/

/* DMA descriptor segment */
#define ETH_DMATxDesc_LastSegment      ((uint32_t)0x40000000)  /* Last Segment */
#define ETH_DMATxDesc_FirstSegment     ((uint32_t)0x20000000)  /* First Segment */

/* DMA descriptor checksum setting */
#define ETH_DMATxDesc_ChecksumByPass             ((uint32_t)0x00000000)   /* Checksum engine bypass */
#define ETH_DMATxDesc_ChecksumIPV4Header         ((uint32_t)0x00400000)   /* IPv4 header checksum insertion  */
#define ETH_DMATxDesc_ChecksumTCPUDPICMPSegment  ((uint32_t)0x00800000)   /* TCP/UDP/ICMP checksum insertion. Pseudo header checksum is assumed to be present */
#define ETH_DMATxDesc_ChecksumTCPUDPICMPFull     ((uint32_t)0x00C00000)   /* TCP/UDP/ICMP checksum fully in hardware including pseudo header */

/* DMA RX & TX buffer */
#define ETH_DMARxDesc_Buffer1     ((uint32_t)0x00000000)  /* DMA Rx Desc Buffer1 */
#define ETH_DMARxDesc_Buffer2     ((uint32_t)0x00000001)  /* DMA Rx Desc Buffer2 */


/******************************************************************************/
/*                                                                            */
/*                          ETH DMA Register                                                                                                                                    */
/*                                                                            */
/******************************************************************************/

/* DMA drop TCPIP checksum error frame enable or disable */
#define ETH_DropTCPIPChecksumErrorFrame_Enable   ((uint32_t)0x00000000)
#define ETH_DropTCPIPChecksumErrorFrame_Disable  ((uint32_t)0x04000000)

/* DMA receive store forward enable or disable */
#define ETH_ReceiveStoreForward_Enable      ((uint32_t)0x02000000)
#define ETH_ReceiveStoreForward_Disable     ((uint32_t)0x00000000)

/* DMA flush received frame enable or disable */
#define ETH_FlushReceivedFrame_Enable       ((uint32_t)0x00000000)
#define ETH_FlushReceivedFrame_Disable      ((uint32_t)0x01000000)

/* DMA transmit store forward enable or disable */
#define ETH_TransmitStoreForward_Enable     ((uint32_t)0x00200000)
#define ETH_TransmitStoreForward_Disable    ((uint32_t)0x00000000)

/* DMA transmit threshold control */
#define ETH_TransmitThresholdControl_64Bytes     ((uint32_t)0x00000000)  /* threshold level of the MTL Transmit FIFO is 64 Bytes */
#define ETH_TransmitThresholdControl_128Bytes    ((uint32_t)0x00004000)  /* threshold level of the MTL Transmit FIFO is 128 Bytes */
#define ETH_TransmitThresholdControl_192Bytes    ((uint32_t)0x00008000)  /* threshold level of the MTL Transmit FIFO is 192 Bytes */
#define ETH_TransmitThresholdControl_256Bytes    ((uint32_t)0x0000C000)  /* threshold level of the MTL Transmit FIFO is 256 Bytes */
#define ETH_TransmitThresholdControl_40Bytes     ((uint32_t)0x00010000)  /* threshold level of the MTL Transmit FIFO is 40 Bytes */
#define ETH_TransmitThresholdControl_32Bytes     ((uint32_t)0x00014000)  /* threshold level of the MTL Transmit FIFO is 32 Bytes */
#define ETH_TransmitThresholdControl_24Bytes     ((uint32_t)0x00018000)  /* threshold level of the MTL Transmit FIFO is 24 Bytes */
#define ETH_TransmitThresholdControl_16Bytes     ((uint32_t)0x0001C000)  /* threshold level of the MTL Transmit FIFO is 16 Bytes */

/* DMA forward error frames */
#define ETH_ForwardErrorFrames_Enable       ((uint32_t)0x00000080)
#define ETH_ForwardErrorFrames_Disable      ((uint32_t)0x00000000)

/* DMA forward undersized good frames enable or disable */
#define ETH_ForwardUndersizedGoodFrames_Enable   ((uint32_t)0x00000040)
#define ETH_ForwardUndersizedGoodFrames_Disable  ((uint32_t)0x00000000)

/* DMA receive threshold control */
#define ETH_ReceiveThresholdControl_64Bytes      ((uint32_t)0x00000000)  /* threshold level of the MTL Receive FIFO is 64 Bytes */
#define ETH_ReceiveThresholdControl_32Bytes      ((uint32_t)0x00000008)  /* threshold level of the MTL Receive FIFO is 32 Bytes */
#define ETH_ReceiveThresholdControl_96Bytes      ((uint32_t)0x00000010)  /* threshold level of the MTL Receive FIFO is 96 Bytes */
#define ETH_ReceiveThresholdControl_128Bytes     ((uint32_t)0x00000018)  /* threshold level of the MTL Receive FIFO is 128 Bytes */

/* DMA second frame operate enable or disable */
#define ETH_SecondFrameOperate_Enable       ((uint32_t)0x00000004)
#define ETH_SecondFrameOperate_Disable      ((uint32_t)0x00000000)

/* Address aligned beats enable or disable */
#define ETH_AddressAlignedBeats_Enable      ((uint32_t)0x02000000)
#define ETH_AddressAlignedBeats_Disable     ((uint32_t)0x00000000)

/* DMA Fixed burst enable or disable */
#define ETH_FixedBurst_Enable     ((uint32_t)0x00010000)
#define ETH_FixedBurst_Disable    ((uint32_t)0x00000000)


/* RX DMA burst length */
#define ETH_RxDMABurstLength_1Beat          ((uint32_t)0x00020000)  /* maximum number of beats to be transferred in one RxDMA transaction is 1 */
#define ETH_RxDMABurstLength_2Beat          ((uint32_t)0x00040000)  /* maximum number of beats to be transferred in one RxDMA transaction is 2 */
#define ETH_RxDMABurstLength_4Beat          ((uint32_t)0x00080000)  /* maximum number of beats to be transferred in one RxDMA transaction is 4 */
#define ETH_RxDMABurstLength_8Beat          ((uint32_t)0x00100000)  /* maximum number of beats to be transferred in one RxDMA transaction is 8 */
#define ETH_RxDMABurstLength_16Beat         ((uint32_t)0x00200000)  /* maximum number of beats to be transferred in one RxDMA transaction is 16 */
#define ETH_RxDMABurstLength_32Beat         ((uint32_t)0x00400000)  /* maximum number of beats to be transferred in one RxDMA transaction is 32 */
#define ETH_RxDMABurstLength_4xPBL_4Beat    ((uint32_t)0x01020000)  /* maximum number of beats to be transferred in one RxDMA transaction is 4 */
#define ETH_RxDMABurstLength_4xPBL_8Beat    ((uint32_t)0x01040000)  /* maximum number of beats to be transferred in one RxDMA transaction is 8 */
#define ETH_RxDMABurstLength_4xPBL_16Beat   ((uint32_t)0x01080000)  /* maximum number of beats to be transferred in one RxDMA transaction is 16 */
#define ETH_RxDMABurstLength_4xPBL_32Beat   ((uint32_t)0x01100000)  /* maximum number of beats to be transferred in one RxDMA transaction is 32 */
#define ETH_RxDMABurstLength_4xPBL_64Beat   ((uint32_t)0x01200000)  /* maximum number of beats to be transferred in one RxDMA transaction is 64 */
#define ETH_RxDMABurstLength_4xPBL_128Beat  ((uint32_t)0x01400000)  /* maximum number of beats to be transferred in one RxDMA transaction is 128 */


/* TX DMA burst length */
#define ETH_TxDMABurstLength_1Beat          ((uint32_t)0x00000100)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 1 */
#define ETH_TxDMABurstLength_2Beat          ((uint32_t)0x00000200)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 2 */
#define ETH_TxDMABurstLength_4Beat          ((uint32_t)0x00000400)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
#define ETH_TxDMABurstLength_8Beat          ((uint32_t)0x00000800)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
#define ETH_TxDMABurstLength_16Beat         ((uint32_t)0x00001000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
#define ETH_TxDMABurstLength_32Beat         ((uint32_t)0x00002000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
#define ETH_TxDMABurstLength_4xPBL_4Beat    ((uint32_t)0x01000100)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
#define ETH_TxDMABurstLength_4xPBL_8Beat    ((uint32_t)0x01000200)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
#define ETH_TxDMABurstLength_4xPBL_16Beat   ((uint32_t)0x01000400)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
#define ETH_TxDMABurstLength_4xPBL_32Beat   ((uint32_t)0x01000800)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
#define ETH_TxDMABurstLength_4xPBL_64Beat   ((uint32_t)0x01001000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 64 */
#define ETH_TxDMABurstLength_4xPBL_128Beat  ((uint32_t)0x01002000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 128 */

/* DMA arbitration_round robin */
#define ETH_DMAArbitration_RoundRobin_RxTx_1_1   ((uint32_t)0x00000000)
#define ETH_DMAArbitration_RoundRobin_RxTx_2_1   ((uint32_t)0x00004000)
#define ETH_DMAArbitration_RoundRobin_RxTx_3_1   ((uint32_t)0x00008000)
#define ETH_DMAArbitration_RoundRobin_RxTx_4_1   ((uint32_t)0x0000C000)
#define ETH_DMAArbitration_RxPriorTx             ((uint32_t)0x00000002)

/* DMA interrupt FALG */
#define ETH_DMA_FLAG_TST               ((uint32_t)0x20000000)  /* Time-stamp trigger interrupt (on DMA) */
#define ETH_DMA_FLAG_PMT               ((uint32_t)0x10000000)  /* PMT interrupt (on DMA) */
#define ETH_DMA_FLAG_MMC               ((uint32_t)0x08000000)  /* MMC interrupt (on DMA) */
#define ETH_DMA_FLAG_DataTransferError ((uint32_t)0x00800000)  /* Error bits 0-Rx DMA, 1-Tx DMA */
#define ETH_DMA_FLAG_ReadWriteError    ((uint32_t)0x01000000)  /* Error bits 0-write trnsf, 1-read transfr */
#define ETH_DMA_FLAG_AccessError       ((uint32_t)0x02000000)  /* Error bits 0-data buffer, 1-desc. access */
#define ETH_DMA_FLAG_NIS               ((uint32_t)0x00010000)  /* Normal interrupt summary flag */
#define ETH_DMA_FLAG_AIS               ((uint32_t)0x00008000)  /* Abnormal interrupt summary flag */
#define ETH_DMA_FLAG_ER                ((uint32_t)0x00004000)  /* Early receive flag */
#define ETH_DMA_FLAG_FBE               ((uint32_t)0x00002000)  /* Fatal bus error flag */
#define ETH_DMA_FLAG_ET                ((uint32_t)0x00000400)  /* Early transmit flag */
#define ETH_DMA_FLAG_RWT               ((uint32_t)0x00000200)  /* Receive watchdog timeout flag */
#define ETH_DMA_FLAG_RPS               ((uint32_t)0x00000100)  /* Receive process stopped flag */
#define ETH_DMA_FLAG_RBU               ((uint32_t)0x00000080)  /* Receive buffer unavailable flag */
#define ETH_DMA_FLAG_R                 ((uint32_t)0x00000040)  /* Receive flag */
#define ETH_DMA_FLAG_TU                ((uint32_t)0x00000020)  /* Underflow flag */
#define ETH_DMA_FLAG_RO                ((uint32_t)0x00000010)  /* Overflow flag */
#define ETH_DMA_FLAG_TJT               ((uint32_t)0x00000008)  /* Transmit jabber timeout flag */
#define ETH_DMA_FLAG_TBU               ((uint32_t)0x00000004)  /* Transmit buffer unavailable flag */
#define ETH_DMA_FLAG_TPS               ((uint32_t)0x00000002)  /* Transmit process stopped flag */
#define ETH_DMA_FLAG_T                 ((uint32_t)0x00000001)  /* Transmit flag */

/* DMA interrupt */
#define ETH_DMA_IT_PHYLINK   ((uint32_t)0x80000000)  /* Internal PHY link status change interrupt */
#define ETH_DMA_IT_TST       ((uint32_t)0x20000000)  /* Time-stamp trigger interrupt (on DMA) */
#define ETH_DMA_IT_PMT       ((uint32_t)0x10000000)  /* PMT interrupt (on DMA) */
#define ETH_DMA_IT_MMC       ((uint32_t)0x08000000)  /* MMC interrupt (on DMA) */
#define ETH_DMA_IT_NIS       ((uint32_t)0x00010000)  /* Normal interrupt summary */
#define ETH_DMA_IT_AIS       ((uint32_t)0x00008000)  /* Abnormal interrupt summary */
#define ETH_DMA_IT_ER        ((uint32_t)0x00004000)  /* Early receive interrupt */
#define ETH_DMA_IT_FBE       ((uint32_t)0x00002000)  /* Fatal bus error interrupt */
#define ETH_DMA_IT_ET        ((uint32_t)0x00000400)  /* Early transmit interrupt */
#define ETH_DMA_IT_RWT       ((uint32_t)0x00000200)  /* Receive watchdog timeout interrupt */
#define ETH_DMA_IT_RPS       ((uint32_t)0x00000100)  /* Receive process stopped interrupt */
#define ETH_DMA_IT_RBU       ((uint32_t)0x00000080)  /* Receive buffer unavailable interrupt */
#define ETH_DMA_IT_R         ((uint32_t)0x00000040)  /* Receive interrupt */
#define ETH_DMA_IT_TU        ((uint32_t)0x00000020)  /* Underflow interrupt */
#define ETH_DMA_IT_RO        ((uint32_t)0x00000010)  /* Overflow interrupt */
#define ETH_DMA_IT_TJT       ((uint32_t)0x00000008)  /* Transmit jabber timeout interrupt */
#define ETH_DMA_IT_TBU       ((uint32_t)0x00000004)  /* Transmit buffer unavailable interrupt */
#define ETH_DMA_IT_TPS       ((uint32_t)0x00000002)  /* Transmit process stopped interrupt */
#define ETH_DMA_IT_T         ((uint32_t)0x00000001)  /* Transmit interrupt */

/* DMA transmit process */
#define ETH_DMA_TransmitProcess_Stopped     ((uint32_t)0x00000000)  /* Stopped - Reset or Stop Tx Command issued */
#define ETH_DMA_TransmitProcess_Fetching    ((uint32_t)0x00100000)  /* Running - fetching the Tx descriptor */
#define ETH_DMA_TransmitProcess_Waiting     ((uint32_t)0x00200000)  /* Running - waiting for status */
#define ETH_DMA_TransmitProcess_Reading     ((uint32_t)0x00300000)  /* Running - reading the data from host memory */
#define ETH_DMA_TransmitProcess_Suspended   ((uint32_t)0x00600000)  /* Suspended - Tx Desciptor unavailabe */
#define ETH_DMA_TransmitProcess_Closing     ((uint32_t)0x00700000)  /* Running - closing Rx descriptor */

/* DMA receive Process */
#define ETH_DMA_ReceiveProcess_Stopped      ((uint32_t)0x00000000)  /* Stopped - Reset or Stop Rx Command issued */
#define ETH_DMA_ReceiveProcess_Fetching     ((uint32_t)0x00020000)  /* Running - fetching the Rx descriptor */
#define ETH_DMA_ReceiveProcess_Waiting      ((uint32_t)0x00060000)  /* Running - waiting for packet */
#define ETH_DMA_ReceiveProcess_Suspended    ((uint32_t)0x00080000)  /* Suspended - Rx Desciptor unavailable */
#define ETH_DMA_ReceiveProcess_Closing      ((uint32_t)0x000A0000)  /* Running - closing descriptor */
#define ETH_DMA_ReceiveProcess_Queuing      ((uint32_t)0x000E0000)  /* Running - queuing the recieve frame into host memory */

/* DMA overflow */
#define ETH_DMA_Overflow_RxFIFOCounter      ((uint32_t)0x10000000)  /* Overflow bit for FIFO overflow counter */
#define ETH_DMA_Overflow_MissedFrameCounter ((uint32_t)0x00010000)  /* Overflow bit for missed frame counter */


/*********************************************************************************
*                            Ethernet PMT defines
**********************************************************************************/

/* PMT flag */
#define ETH_PMT_FLAG_WUFFRPR      ((uint32_t)0x80000000)  /* Wake-Up Frame Filter Register Poniter Reset */
#define ETH_PMT_FLAG_WUFR         ((uint32_t)0x00000040)  /* Wake-Up Frame Received */
#define ETH_PMT_FLAG_MPR          ((uint32_t)0x00000020)  /* Magic Packet Received */

/*********************************************************************************
*                            Ethernet MMC defines
**********************************************************************************/

/* MMC TX interrupt flag */
#define ETH_MMC_IT_TGF       ((uint32_t)0x00200000)  /* When Tx good frame counter reaches half the maximum value */
#define ETH_MMC_IT_TGFMSC    ((uint32_t)0x00008000)  /* When Tx good multi col counter reaches half the maximum value */
#define ETH_MMC_IT_TGFSC     ((uint32_t)0x00004000)  /* When Tx good single col counter reaches half the maximum value */

/* MMC RX interrupt flag */
#define ETH_MMC_IT_RGUF      ((uint32_t)0x10020000)  /* When Rx good unicast frames counter reaches half the maximum value */
#define ETH_MMC_IT_RFAE      ((uint32_t)0x10000040)  /* When Rx alignment error counter reaches half the maximum value */
#define ETH_MMC_IT_RFCE      ((uint32_t)0x10000020)  /* When Rx crc error counter reaches half the maximum value */


/* MMC description */
#define ETH_MMCCR            ((uint32_t)0x00000100)  /* MMC CR register */
#define ETH_MMCRIR           ((uint32_t)0x00000104)  /* MMC RIR register */
#define ETH_MMCTIR           ((uint32_t)0x00000108)  /* MMC TIR register */
#define ETH_MMCRIMR          ((uint32_t)0x0000010C)  /* MMC RIMR register */
#define ETH_MMCTIMR          ((uint32_t)0x00000110)  /* MMC TIMR register */
#define ETH_MMCTGFSCCR       ((uint32_t)0x0000014C)  /* MMC TGFSCCR register */
#define ETH_MMCTGFMSCCR      ((uint32_t)0x00000150)  /* MMC TGFMSCCR register */
#define ETH_MMCTGFCR         ((uint32_t)0x00000168)  /* MMC TGFCR register */
#define ETH_MMCRFCECR        ((uint32_t)0x00000194)  /* MMC RFCECR register */
#define ETH_MMCRFAECR        ((uint32_t)0x00000198)  /* MMC RFAECR register */
#define ETH_MMCRGUFCR        ((uint32_t)0x000001C4)  /* MMC RGUFCR register */


/*********************************************************************************
*                            Ethernet PTP defines
**********************************************************************************/

/* PTP fine update method or coarse Update method */
#define ETH_PTP_FineUpdate        ((uint32_t)0x00000001)  /* Fine Update method */
#define ETH_PTP_CoarseUpdate      ((uint32_t)0x00000000)  /* Coarse Update method */


/* PTP time stamp control */
#define ETH_PTP_FLAG_TSARU        ((uint32_t)0x00000020)  /* Addend Register Update */
#define ETH_PTP_FLAG_TSITE        ((uint32_t)0x00000010)  /* Time Stamp Interrupt Trigger */
#define ETH_PTP_FLAG_TSSTU        ((uint32_t)0x00000008)  /* Time Stamp Update */
#define ETH_PTP_FLAG_TSSTI        ((uint32_t)0x00000004)  /* Time Stamp Initialize */

/* PTP positive/negative time value */
#define ETH_PTP_PositiveTime      ((uint32_t)0x00000000)  /* Positive time value */
#define ETH_PTP_NegativeTime      ((uint32_t)0x80000000)  /* Negative time value */


/******************************************************************************/
/*                                                                            */
/*                                PTP Register                                                                                                                           */
/*                                                                            */
/******************************************************************************/
#define ETH_PTPTSCR     ((uint32_t)0x00000700)  /* PTP TSCR register */
#define ETH_PTPSSIR     ((uint32_t)0x00000704)  /* PTP SSIR register */
#define ETH_PTPTSHR     ((uint32_t)0x00000708)  /* PTP TSHR register */
#define ETH_PTPTSLR     ((uint32_t)0x0000070C)  /* PTP TSLR register */
#define ETH_PTPTSHUR    ((uint32_t)0x00000710)  /* PTP TSHUR register */
#define ETH_PTPTSLUR    ((uint32_t)0x00000714)  /* PTP TSLUR register */
#define ETH_PTPTSAR     ((uint32_t)0x00000718)  /* PTP TSAR register */
#define ETH_PTPTTHR     ((uint32_t)0x0000071C)  /* PTP TTHR register */
#define ETH_PTPTTLR     ((uint32_t)0x00000720)  /* PTP TTLR register */

#define ETH_DMASR_TSTS       ((unsigned int)0x20000000)  /* Time-stamp trigger status */
#define ETH_DMASR_PMTS       ((unsigned int)0x10000000)  /* PMT status */
#define ETH_DMASR_MMCS       ((unsigned int)0x08000000)  /* MMC status */
#define ETH_DMASR_EBS        ((unsigned int)0x03800000)  /* Error bits status */
  #define ETH_DMASR_EBS_DescAccess      ((unsigned int)0x02000000)  /* Error bits 0-data buffer, 1-desc. access */
  #define ETH_DMASR_EBS_ReadTransf      ((unsigned int)0x01000000)  /* Error bits 0-write trnsf, 1-read transfr */
  #define ETH_DMASR_EBS_DataTransfTx    ((unsigned int)0x00800000)  /* Error bits 0-Rx DMA, 1-Tx DMA */
#define ETH_DMASR_TPS         ((unsigned int)0x00700000)  /* Transmit process state */
  #define ETH_DMASR_TPS_Stopped         ((unsigned int)0x00000000)  /* Stopped - Reset or Stop Tx Command issued  */
  #define ETH_DMASR_TPS_Fetching        ((unsigned int)0x00100000)  /* Running - fetching the Tx descriptor */
  #define ETH_DMASR_TPS_Waiting         ((unsigned int)0x00200000)  /* Running - waiting for status */
  #define ETH_DMASR_TPS_Reading         ((unsigned int)0x00300000)  /* Running - reading the data from host memory */
  #define ETH_DMASR_TPS_Suspended       ((unsigned int)0x00600000)  /* Suspended - Tx Descriptor unavailabe */
  #define ETH_DMASR_TPS_Closing         ((unsigned int)0x00700000)  /* Running - closing Rx descriptor */
#define ETH_DMASR_RPS         ((unsigned int)0x000E0000)  /* Receive process state */
  #define ETH_DMASR_RPS_Stopped         ((unsigned int)0x00000000)  /* Stopped - Reset or Stop Rx Command issued */
  #define ETH_DMASR_RPS_Fetching        ((unsigned int)0x00020000)  /* Running - fetching the Rx descriptor */
  #define ETH_DMASR_RPS_Waiting         ((unsigned int)0x00060000)  /* Running - waiting for packet */
  #define ETH_DMASR_RPS_Suspended       ((unsigned int)0x00080000)  /* Suspended - Rx Descriptor unavailable */
  #define ETH_DMASR_RPS_Closing         ((unsigned int)0x000A0000)  /* Running - closing descriptor */
  #define ETH_DMASR_RPS_Queuing         ((unsigned int)0x000E0000)  /* Running - queuing the recieve frame into host memory */
#define ETH_DMASR_NIS        ((unsigned int)0x00010000)  /* Normal interrupt summary */
#define ETH_DMASR_AIS        ((unsigned int)0x00008000)  /* Abnormal interrupt summary */
#define ETH_DMASR_ERS        ((unsigned int)0x00004000)  /* Early receive status */
#define ETH_DMASR_FBES       ((unsigned int)0x00002000)  /* Fatal bus error status */
#define ETH_DMASR_ETS        ((unsigned int)0x00000400)  /* Early transmit status */
#define ETH_DMASR_RWTS       ((unsigned int)0x00000200)  /* Receive watchdog timeout status */
#define ETH_DMASR_RPSS       ((unsigned int)0x00000100)  /* Receive process stopped status */
#define ETH_DMASR_RBUS       ((unsigned int)0x00000080)  /* Receive buffer unavailable status */
#define ETH_DMASR_RS         ((unsigned int)0x00000040)  /* Receive status */
#define ETH_DMASR_TUS        ((unsigned int)0x00000020)  /* Transmit underflow status */
#define ETH_DMASR_ROS        ((unsigned int)0x00000010)  /* Receive overflow status */
#define ETH_DMASR_TJTS       ((unsigned int)0x00000008)  /* Transmit jabber timeout status */
#define ETH_DMASR_TBUS       ((unsigned int)0x00000004)  /* Transmit buffer unavailable status */
#define ETH_DMASR_TPSS       ((unsigned int)0x00000002)  /* Transmit process stopped status */
#define ETH_DMASR_TS         ((unsigned int)0x00000001)  /* Transmit status */


/******************************************************************************/
/*                                                                            */
/*                          ETH MAC Register                                                                                                                               */
/*                                                                            */
/******************************************************************************/
#define ETH_MACCR_WD      ((unsigned int)0x00800000)  /* Watchdog disable */
#define ETH_MACCR_JD      ((unsigned int)0x00400000)  /* Jabber disable */
#define ETH_MACCR_IFG     ((unsigned int)0x000E0000)  /* Inter-frame gap */
#define ETH_MACCR_IFG_96Bit     ((unsigned int)0x00000000)  /* Minimum IFG between frames during transmission is 96Bit */
   #define ETH_MACCR_IFG_88Bit     ((unsigned int)0x00020000)  /* Minimum IFG between frames during transmission is 88Bit */
   #define ETH_MACCR_IFG_80Bit     ((unsigned int)0x00040000)  /* Minimum IFG between frames during transmission is 80Bit */
   #define ETH_MACCR_IFG_72Bit     ((unsigned int)0x00060000)  /* Minimum IFG between frames during transmission is 72Bit */
   #define ETH_MACCR_IFG_64Bit     ((unsigned int)0x00080000)  /* Minimum IFG between frames during transmission is 64Bit */
   #define ETH_MACCR_IFG_56Bit     ((unsigned int)0x000A0000)  /* Minimum IFG between frames during transmission is 56Bit */
   #define ETH_MACCR_IFG_48Bit     ((unsigned int)0x000C0000)  /* Minimum IFG between frames during transmission is 48Bit */
   #define ETH_MACCR_IFG_40Bit     ((unsigned int)0x000E0000)  /* Minimum IFG between frames during transmission is 40Bit */
#define ETH_MACCR_CSD     ((unsigned int)0x00010000)  /* Carrier sense disable (during transmission) */
#define ETH_MACCR_FES     ((unsigned int)0x00004000)  /* Fast ethernet speed */
#define ETH_MACCR_ROD     ((unsigned int)0x00002000)  /* Receive own disable */
#define ETH_MACCR_LM      ((unsigned int)0x00001000)  /* loopback mode */
#define ETH_MACCR_DM      ((unsigned int)0x00000800)  /* Duplex mode */
#define ETH_MACCR_IPCO    ((unsigned int)0x00000400)  /* IP Checksum offload */
#define ETH_MACCR_RD      ((unsigned int)0x00000200)  /* Retry disable */
#define ETH_MACCR_APCS    ((unsigned int)0x00000080)  /* Automatic Pad/CRC stripping */
#define ETH_MACCR_BL      ((unsigned int)0x00000060)  /* Back-off limit: random integer number (r) of slot time delays before reschedulinga transmission attempt during retries after a collision: 0 =< r <2^k */
   #define ETH_MACCR_BL_10    ((unsigned int)0x00000000)  /* k = min (n, 10) */
   #define ETH_MACCR_BL_8     ((unsigned int)0x00000020)  /* k = min (n, 8) */
   #define ETH_MACCR_BL_4     ((unsigned int)0x00000040)  /* k = min (n, 4) */
   #define ETH_MACCR_BL_1     ((unsigned int)0x00000060)  /* k = min (n, 1) */
#define ETH_MACCR_DC      ((unsigned int)0x00000010)  /* Defferal check */
#define ETH_MACCR_TE      ((unsigned int)0x00000008)  /* Transmitter enable */
#define ETH_MACCR_RE      ((unsigned int)0x00000004)  /* Receiver enable */

#define ETH_MACFFR_RA     ((unsigned int)0x80000000)  /* Receive all */
#define ETH_MACFFR_HPF    ((unsigned int)0x00000400)  /* Hash or perfect filter */
#define ETH_MACFFR_SAF    ((unsigned int)0x00000200)  /* Source address filter enable */
#define ETH_MACFFR_SAIF   ((unsigned int)0x00000100)  /* SA inverse filtering */
#define ETH_MACFFR_PCF    ((unsigned int)0x000000C0)  /* Pass control frames: 3 cases */
   #define ETH_MACFFR_PCF_BlockAll                ((unsigned int)0x00000040)  /* MAC filters all control frames from reaching the application */
   #define ETH_MACFFR_PCF_ForwardAll              ((unsigned int)0x00000080)  /* MAC forwards all control frames to application even if they fail the Address Filter */
   #define ETH_MACFFR_PCF_ForwardPassedAddrFilter ((unsigned int)0x000000C0)  /* MAC forwards control frames that pass the Address Filter. */
#define ETH_MACFFR_BFD    ((unsigned int)0x00000020)  /* Broadcast frame disable */
#define ETH_MACFFR_PAM    ((unsigned int)0x00000010)  /* Pass all mutlicast */
#define ETH_MACFFR_DAIF   ((unsigned int)0x00000008)  /* DA Inverse filtering */
#define ETH_MACFFR_HM     ((unsigned int)0x00000004)  /* Hash multicast */
#define ETH_MACFFR_HU     ((unsigned int)0x00000002)  /* Hash unicast */
#define ETH_MACFFR_PM     ((unsigned int)0x00000001)  /* Promiscuous mode */

#define ETH_MACHTHR_HTH   ((unsigned int)0xFFFFFFFF)  /* Hash table high */
#define ETH_MACHTLR_HTL   ((unsigned int)0xFFFFFFFF)  /* Hash table low */

#define ETH_MACMIIAR_PA   ((unsigned int)0x0000F800)  /* Physical layer address */
#define ETH_MACMIIAR_MR   ((unsigned int)0x000007C0)  /* MII register in the selected PHY */
#define ETH_MACMIIAR_CR   ((unsigned int)0x0000001C)  /* CR clock range: 6 cases */
   #define ETH_MACMIIAR_CR_Div42   ((unsigned int)0x00000000)  /* HCLK:60-100 MHz; MDC clock= HCLK/42 */
   #define ETH_MACMIIAR_CR_Div16   ((unsigned int)0x00000008)  /* HCLK:20-35 MHz; MDC clock= HCLK/16 */
   #define ETH_MACMIIAR_CR_Div26   ((unsigned int)0x0000000C)  /* HCLK:35-60 MHz; MDC clock= HCLK/26 */
#define ETH_MACMIIAR_MW   ((unsigned int)0x00000002)  /* MII write */
#define ETH_MACMIIAR_MB   ((unsigned int)0x00000001)  /* MII busy */
#define ETH_MACMIIDR_MD   ((unsigned int)0x0000FFFF)  /* MII data: read/write data from/to PHY */
#define ETH_MACFCR_PT     ((unsigned int)0xFFFF0000)  /* Pause time */
#define ETH_MACFCR_ZQPD   ((unsigned int)0x00000080)  /* Zero-quanta pause disable */
#define ETH_MACFCR_PLT    ((unsigned int)0x00000030)  /* Pause low threshold: 4 cases */
   #define ETH_MACFCR_PLT_Minus4   ((unsigned int)0x00000000)  /* Pause time minus 4 slot times */
   #define ETH_MACFCR_PLT_Minus28  ((unsigned int)0x00000010)  /* Pause time minus 28 slot times */
   #define ETH_MACFCR_PLT_Minus144 ((unsigned int)0x00000020)  /* Pause time minus 144 slot times */
   #define ETH_MACFCR_PLT_Minus256 ((unsigned int)0x00000030)  /* Pause time minus 256 slot times */
#define ETH_MACFCR_UPFD   ((unsigned int)0x00000008)  /* Unicast pause frame detect */
#define ETH_MACFCR_RFCE   ((unsigned int)0x00000004)  /* Receive flow control enable */
#define ETH_MACFCR_TFCE   ((unsigned int)0x00000002)  /* Transmit flow control enable */
#define ETH_MACFCR_FCBBPA ((unsigned int)0x00000001)  /* Flow control busy/backpressure activate */

#define ETH_MACVLANTR_VLANTC ((unsigned int)0x00010000)  /* 12-bit VLAN tag comparison */
#define ETH_MACVLANTR_VLANTI ((unsigned int)0x0000FFFF)  /* VLAN tag identifier (for receive frames) */

#define ETH_MACRWUFFR_D   ((unsigned int)0xFFFFFFFF)  /* Wake-up frame filter register data */
/* Eight sequential Writes to this address (offset 0x28) will write all Wake-UpFrame Filter Registers.
Eight sequential Reads from this address (offset 0x28) will read all Wake-UpFrame Filter Registers. */

/*
Wake-UpFrame Filter Reg0 : Filter 0 Byte Mask
Wake-UpFrame Filter Reg1 : Filter 1 Byte Mask
Wake-UpFrame Filter Reg2 : Filter 2 Byte Mask
Wake-UpFrame Filter Reg3 : Filter 3 Byte Mask
Wake-UpFrame Filter Reg4 : RSVD - Filter3 Command - RSVD - Filter2 Command -
                           RSVD - Filter1 Command - RSVD - Filter0 Command
Wake-UpFrame Filter Re5 : Filter3 Offset - Filter2 Offset - Filter1 Offset - Filter0 Offset
Wake-UpFrame Filter Re6 : Filter1 CRC16 - Filter0 CRC16
Wake-UpFrame Filter Re7 : Filter3 CRC16 - Filter2 CRC16 */

#define ETH_MACPMTCSR_WFFRPR ((unsigned int)0x80000000)  /* Wake-Up Frame Filter Register Pointer Reset */
#define ETH_MACPMTCSR_GU     ((unsigned int)0x00000200)  /* Global Unicast */
#define ETH_MACPMTCSR_WFR    ((unsigned int)0x00000040)  /* Wake-Up Frame Received */
#define ETH_MACPMTCSR_MPR    ((unsigned int)0x00000020)  /* Magic Packet Received */
#define ETH_MACPMTCSR_WFE    ((unsigned int)0x00000004)  /* Wake-Up Frame Enable */
#define ETH_MACPMTCSR_MPE    ((unsigned int)0x00000002)  /* Magic Packet Enable */
#define ETH_MACPMTCSR_PD     ((unsigned int)0x00000001)  /* Power Down */

#define ETH_MACSR_TSTS      ((unsigned int)0x00000200)  /* Time stamp trigger status */
#define ETH_MACSR_MMCTS     ((unsigned int)0x00000040)  /* MMC transmit status */
#define ETH_MACSR_MMMCRS    ((unsigned int)0x00000020)  /* MMC receive status */
#define ETH_MACSR_MMCS      ((unsigned int)0x00000010)  /* MMC status */
#define ETH_MACSR_PMTS      ((unsigned int)0x00000008)  /* PMT status */

#define ETH_MACIMR_TSTIM     ((unsigned int)0x00000200)  /* Time stamp trigger interrupt mask */
#define ETH_MACIMR_PMTIM     ((unsigned int)0x00000008)  /* PMT interrupt mask */

#define ETH_MACA0HR_MACA0H   ((unsigned int)0x0000FFFF)  /* MAC address0 high */

#define ETH_MACA0LR_MACA0L   ((unsigned int)0xFFFFFFFF)  /* MAC address0 low */

#define ETH_MACA1HR_AE       ((unsigned int)0x80000000)  /* Address enable */
#define ETH_MACA1HR_SA       ((unsigned int)0x40000000)  /* Source address */
#define ETH_MACA1HR_MBC      ((unsigned int)0x3F000000)  /* Mask byte control: bits to mask for comparison of the MAC Address bytes */
   #define ETH_MACA1HR_MBC_HBits15_8    ((unsigned int)0x20000000)  /* Mask MAC Address high reg bits [15:8] */
   #define ETH_MACA1HR_MBC_HBits7_0     ((unsigned int)0x10000000)  /* Mask MAC Address high reg bits [7:0] */
   #define ETH_MACA1HR_MBC_LBits31_24   ((unsigned int)0x08000000)  /* Mask MAC Address low reg bits [31:24] */
   #define ETH_MACA1HR_MBC_LBits23_16   ((unsigned int)0x04000000)  /* Mask MAC Address low reg bits [23:16] */
   #define ETH_MACA1HR_MBC_LBits15_8    ((unsigned int)0x02000000)  /* Mask MAC Address low reg bits [15:8] */
   #define ETH_MACA1HR_MBC_LBits7_0     ((unsigned int)0x01000000)  /* Mask MAC Address low reg bits [7:0] */
#define ETH_MACA1HR_MACA1H   ((unsigned int)0x0000FFFF)  /* MAC address1 high */

#define ETH_MACA1LR_MACA1L   ((unsigned int)0xFFFFFFFF)  /* MAC address1 low */

#define ETH_MACA2HR_AE       ((unsigned int)0x80000000)  /* Address enable */
#define ETH_MACA2HR_SA       ((unsigned int)0x40000000)  /* Source address */
#define ETH_MACA2HR_MBC      ((unsigned int)0x3F000000)  /* Mask byte control */
   #define ETH_MACA2HR_MBC_HBits15_8    ((unsigned int)0x20000000)  /* Mask MAC Address high reg bits [15:8] */
   #define ETH_MACA2HR_MBC_HBits7_0     ((unsigned int)0x10000000)  /* Mask MAC Address high reg bits [7:0] */
   #define ETH_MACA2HR_MBC_LBits31_24   ((unsigned int)0x08000000)  /* Mask MAC Address low reg bits [31:24] */
   #define ETH_MACA2HR_MBC_LBits23_16   ((unsigned int)0x04000000)  /* Mask MAC Address low reg bits [23:16] */
   #define ETH_MACA2HR_MBC_LBits15_8    ((unsigned int)0x02000000)  /* Mask MAC Address low reg bits [15:8] */
   #define ETH_MACA2HR_MBC_LBits7_0     ((unsigned int)0x01000000)  /* Mask MAC Address low reg bits [70] */

#define ETH_MACA2HR_MACA2H   ((unsigned int)0x0000FFFF)  /* MAC address1 high */
#define ETH_MACA2LR_MACA2L   ((unsigned int)0xFFFFFFFF)  /* MAC address2 low */

#define ETH_MACA3HR_AE       ((unsigned int)0x80000000)  /* Address enable */
#define ETH_MACA3HR_SA       ((unsigned int)0x40000000)  /* Source address */
#define ETH_MACA3HR_MBC      ((unsigned int)0x3F000000)  /* Mask byte control */
   #define ETH_MACA3HR_MBC_HBits15_8    ((unsigned int)0x20000000)  /* Mask MAC Address high reg bits [15:8] */
   #define ETH_MACA3HR_MBC_HBits7_0     ((unsigned int)0x10000000)  /* Mask MAC Address high reg bits [7:0] */
   #define ETH_MACA3HR_MBC_LBits31_24   ((unsigned int)0x08000000)  /* Mask MAC Address low reg bits [31:24] */
   #define ETH_MACA3HR_MBC_LBits23_16   ((unsigned int)0x04000000)  /* Mask MAC Address low reg bits [23:16] */
   #define ETH_MACA3HR_MBC_LBits15_8    ((unsigned int)0x02000000)  /* Mask MAC Address low reg bits [15:8] */
   #define ETH_MACA3HR_MBC_LBits7_0     ((unsigned int)0x01000000)  /* Mask MAC Address low reg bits [70] */
#define ETH_MACA3HR_MACA3H   ((unsigned int)0x0000FFFF)  /* MAC address3 high */
#define ETH_MACA3LR_MACA3L   ((unsigned int)0xFFFFFFFF)  /* MAC address3 low */

/******************************************************************************
 *                          ETH MMC Register
 ******************************************************************************/
#define ETH_MMCCR_MCFHP      ((unsigned int)0x00000020)  /* MMC counter Full-Half preset */
#define ETH_MMCCR_MCP        ((unsigned int)0x00000010)  /* MMC counter preset */
#define ETH_MMCCR_MCF        ((unsigned int)0x00000008)  /* MMC Counter Freeze */
#define ETH_MMCCR_ROR        ((unsigned int)0x00000004)  /* Reset on Read */
#define ETH_MMCCR_CSR        ((unsigned int)0x00000002)  /* Counter Stop Rollover */
#define ETH_MMCCR_CR         ((unsigned int)0x00000001)  /* Counters Reset */

#define ETH_MMCRIR_RGUFS     ((unsigned int)0x00020000)  /* Set when Rx good unicast frames counter reaches half the maximum value */
#define ETH_MMCRIR_RFAES     ((unsigned int)0x00000040)  /* Set when Rx alignment error counter reaches half the maximum value */
#define ETH_MMCRIR_RFCES     ((unsigned int)0x00000020)  /* Set when Rx crc error counter reaches half the maximum value */

#define ETH_MMCTIR_TGFS      ((unsigned int)0x00200000)  /* Set when Tx good frame count counter reaches half the maximum value */
#define ETH_MMCTIR_TGFMSCS   ((unsigned int)0x00008000)  /* Set when Tx good multi col counter reaches half the maximum value */
#define ETH_MMCTIR_TGFSCS    ((unsigned int)0x00004000)  /* Set when Tx good single col counter reaches half the maximum value */

#define ETH_MMCRIMR_RGUFM    ((unsigned int)0x00020000)  /* Mask the interrupt when Rx good unicast frames counter reaches half the maximum value */
#define ETH_MMCRIMR_RFAEM    ((unsigned int)0x00000040)  /* Mask the interrupt when when Rx alignment error counter reaches half the maximum value */
#define ETH_MMCRIMR_RFCEM    ((unsigned int)0x00000020)  /* Mask the interrupt when Rx crc error counter reaches half the maximum value */

#define ETH_MMCTIMR_TGFM     ((unsigned int)0x00200000)  /* Mask the interrupt when Tx good frame count counter reaches half the maximum value */
#define ETH_MMCTIMR_TGFMSCM  ((unsigned int)0x00008000)  /* Mask the interrupt when Tx good multi col counter reaches half the maximum value */
#define ETH_MMCTIMR_TGFSCM   ((unsigned int)0x00004000)  /* Mask the interrupt when Tx good single col counter reaches half the maximum value */

#define ETH_MMCTGFSCCR_TGFSCC     ((unsigned int)0xFFFFFFFF)  /* Number of successfully transmitted frames after a single collision in Half-duplex mode. */

#define ETH_MMCTGFMSCCR_TGFMSCC   ((unsigned int)0xFFFFFFFF)  /* Number of successfully transmitted frames after more than a single collision in Half-duplex mode. */

#define ETH_MMCTGFCR_TGFC    ((unsigned int)0xFFFFFFFF)  /* Number of good frames transmitted. */

#define ETH_MMCRFCECR_RFCEC  ((unsigned int)0xFFFFFFFF)  /* Number of frames received with CRC error. */

#define ETH_MMCRFAECR_RFAEC  ((unsigned int)0xFFFFFFFF)  /* Number of frames received with alignment (dribble) error */

#define ETH_MMCRGUFCR_RGUFC  ((unsigned int)0xFFFFFFFF)  /* Number of good unicast frames received. */


/******************************************************************************
 *                          ETH Precise Clock Protocol Register
 ******************************************************************************/
#define ETH_PTPTSCR_TSCNT       ((unsigned int)0x00030000)  /* Time stamp clock node type */
#define ETH_PTPTSSR_TSSMRME     ((unsigned int)0x00008000)  /* Time stamp snapshot for message relevant to master enable */
#define ETH_PTPTSSR_TSSEME      ((unsigned int)0x00004000)  /* Time stamp snapshot for event message enable */
#define ETH_PTPTSSR_TSSIPV4FE   ((unsigned int)0x00002000)  /* Time stamp snapshot for IPv4 frames enable */
#define ETH_PTPTSSR_TSSIPV6FE   ((unsigned int)0x00001000)  /* Time stamp snapshot for IPv6 frames enable */
#define ETH_PTPTSSR_TSSPTPOEFE  ((unsigned int)0x00000800)  /* Time stamp snapshot for PTP over ethernet frames enable */
#define ETH_PTPTSSR_TSPTPPSV2E  ((unsigned int)0x00000400)  /* Time stamp PTP packet snooping for version2 format enable */
#define ETH_PTPTSSR_TSSSR       ((unsigned int)0x00000200)  /* Time stamp Sub-seconds rollover */
#define ETH_PTPTSSR_TSSARFE     ((unsigned int)0x00000100)  /* Time stamp snapshot for all received frames enable */

#define ETH_PTPTSCR_TSARU    ((unsigned int)0x00000020)  /* Addend register update */
#define ETH_PTPTSCR_TSITE    ((unsigned int)0x00000010)  /* Time stamp interrupt trigger enable */
#define ETH_PTPTSCR_TSSTU    ((unsigned int)0x00000008)  /* Time stamp update */
#define ETH_PTPTSCR_TSSTI    ((unsigned int)0x00000004)  /* Time stamp initialize */
#define ETH_PTPTSCR_TSFCU    ((unsigned int)0x00000002)  /* Time stamp fine or coarse update */
#define ETH_PTPTSCR_TSE      ((unsigned int)0x00000001)  /* Time stamp enable */

#define ETH_PTPSSIR_STSSI    ((unsigned int)0x000000FF)  /* System time Sub-second increment value */

#define ETH_PTPTSHR_STS      ((unsigned int)0xFFFFFFFF)  /* System Time second */

#define ETH_PTPTSLR_STPNS    ((unsigned int)0x80000000)  /* System Time Positive or negative time */
#define ETH_PTPTSLR_STSS     ((unsigned int)0x7FFFFFFF)  /* System Time sub-seconds */

#define ETH_PTPTSHUR_TSUS    ((unsigned int)0xFFFFFFFF)  /* Time stamp update seconds */

#define ETH_PTPTSLUR_TSUPNS  ((unsigned int)0x80000000)  /* Time stamp update Positive or negative time */
#define ETH_PTPTSLUR_TSUSS   ((unsigned int)0x7FFFFFFF)  /* Time stamp update sub-seconds */

#define ETH_PTPTSAR_TSA      ((unsigned int)0xFFFFFFFF)  /* Time stamp addend */

#define ETH_PTPTTHR_TTSH     ((unsigned int)0xFFFFFFFF)  /* Target time stamp high */

#define ETH_PTPTTLR_TTSL     ((unsigned int)0xFFFFFFFF)  /* Target time stamp low */

#define ETH_PTPTSSR_TSTTR    ((unsigned int)0x00000020)  /* Time stamp target time reached */
#define ETH_PTPTSSR_TSSO     ((unsigned int)0x00000010)  /* Time stamp seconds overflow */

/******************************************************************************
 *                       ETH DMA Register
 ******************************************************************************/
#define ETH_DMABMR_AAB       ((unsigned int)0x02000000)  /* Address-Aligned beats */
#define ETH_DMABMR_FPM        ((unsigned int)0x01000000)  /* 4xPBL mode */
#define ETH_DMABMR_USP       ((unsigned int)0x00800000)  /* Use separate PBL */
#define ETH_DMABMR_RDP       ((unsigned int)0x007E0000)  /* RxDMA PBL */
   #define ETH_DMABMR_RDP_1Beat    ((unsigned int)0x00020000)  /* maximum number of beats to be transferred in one RxDMA transaction is 1 */
   #define ETH_DMABMR_RDP_2Beat    ((unsigned int)0x00040000)  /* maximum number of beats to be transferred in one RxDMA transaction is 2 */
   #define ETH_DMABMR_RDP_4Beat    ((unsigned int)0x00080000)  /* maximum number of beats to be transferred in one RxDMA transaction is 4 */
   #define ETH_DMABMR_RDP_8Beat    ((unsigned int)0x00100000)  /* maximum number of beats to be transferred in one RxDMA transaction is 8 */
   #define ETH_DMABMR_RDP_16Beat   ((unsigned int)0x00200000)  /* maximum number of beats to be transferred in one RxDMA transaction is 16 */
   #define ETH_DMABMR_RDP_32Beat   ((unsigned int)0x00400000)  /* maximum number of beats to be transferred in one RxDMA transaction is 32 */
   #define ETH_DMABMR_RDP_4xPBL_4Beat   ((unsigned int)0x01020000)  /* maximum number of beats to be transferred in one RxDMA transaction is 4 */
   #define ETH_DMABMR_RDP_4xPBL_8Beat   ((unsigned int)0x01040000)  /* maximum number of beats to be transferred in one RxDMA transaction is 8 */
   #define ETH_DMABMR_RDP_4xPBL_16Beat  ((unsigned int)0x01080000)  /* maximum number of beats to be transferred in one RxDMA transaction is 16 */
   #define ETH_DMABMR_RDP_4xPBL_32Beat  ((unsigned int)0x01100000)  /* maximum number of beats to be transferred in one RxDMA transaction is 32 */
   #define ETH_DMABMR_RDP_4xPBL_64Beat  ((unsigned int)0x01200000)  /* maximum number of beats to be transferred in one RxDMA transaction is 64 */
   #define ETH_DMABMR_RDP_4xPBL_128Beat ((unsigned int)0x01400000)  /* maximum number of beats to be transferred in one RxDMA transaction is 128 */
#define ETH_DMABMR_FB        ((unsigned int)0x00010000)  /* Fixed Burst */
#define ETH_DMABMR_RTPR      ((unsigned int)0x0000C000)  /* Rx Tx priority ratio */
   #define ETH_DMABMR_RTPR_1_1     ((unsigned int)0x00000000)  /* Rx Tx priority ratio */
   #define ETH_DMABMR_RTPR_2_1     ((unsigned int)0x00004000)  /* Rx Tx priority ratio */
   #define ETH_DMABMR_RTPR_3_1     ((unsigned int)0x00008000)  /* Rx Tx priority ratio */
   #define ETH_DMABMR_RTPR_4_1     ((unsigned int)0x0000C000)  /* Rx Tx priority ratio */
#define ETH_DMABMR_PBL    ((unsigned int)0x00003F00)  /* Programmable burst length */
   #define ETH_DMABMR_PBL_1Beat    ((unsigned int)0x00000100)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 1 */
   #define ETH_DMABMR_PBL_2Beat    ((unsigned int)0x00000200)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 2 */
   #define ETH_DMABMR_PBL_4Beat    ((unsigned int)0x00000400)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
   #define ETH_DMABMR_PBL_8Beat    ((unsigned int)0x00000800)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
   #define ETH_DMABMR_PBL_16Beat   ((unsigned int)0x00001000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
   #define ETH_DMABMR_PBL_32Beat   ((unsigned int)0x00002000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
   #define ETH_DMABMR_PBL_4xPBL_4Beat   ((unsigned int)0x01000100)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 4 */
   #define ETH_DMABMR_PBL_4xPBL_8Beat   ((unsigned int)0x01000200)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 8 */
   #define ETH_DMABMR_PBL_4xPBL_16Beat  ((unsigned int)0x01000400)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 16 */
   #define ETH_DMABMR_PBL_4xPBL_32Beat  ((unsigned int)0x01000800)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 32 */
   #define ETH_DMABMR_PBL_4xPBL_64Beat  ((unsigned int)0x01001000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 64 */
   #define ETH_DMABMR_PBL_4xPBL_128Beat ((unsigned int)0x01002000)  /* maximum number of beats to be transferred in one TxDMA (or both) transaction is 128 */
#define ETH_DMABMR_EDE       ((unsigned int)0x00000080)  /* Enhanced Descriptor Enable */
#define ETH_DMABMR_DSL       ((unsigned int)0x0000007C)  /* Descriptor Skip Length */
#define ETH_DMABMR_DA        ((unsigned int)0x00000002)  /* DMA arbitration scheme */
#define ETH_DMABMR_SR        ((unsigned int)0x00000001)  /* Software reset */

#define ETH_DMATPDR_TPD      ((unsigned int)0xFFFFFFFF)  /* Transmit poll demand */

#define ETH_DMARPDR_RPD      ((unsigned int)0xFFFFFFFF)  /* Receive poll demand  */

#define ETH_DMARDLAR_SRL     ((unsigned int)0xFFFFFFFF)  /* Start of receive list */

#define ETH_DMATDLAR_STL     ((unsigned int)0xFFFFFFFF)  /* Start of transmit list */

#define ETH_DMASR_TSTS       ((unsigned int)0x20000000)  /* Time-stamp trigger status */
#define ETH_DMASR_PMTS       ((unsigned int)0x10000000)  /* PMT status */
#define ETH_DMASR_MMCS       ((unsigned int)0x08000000)  /* MMC status */
#define ETH_DMASR_EBS        ((unsigned int)0x03800000)  /* Error bits status */
   #define ETH_DMASR_EBS_DescAccess      ((unsigned int)0x02000000)  /* Error bits 0-data buffer, 1-desc. access */
   #define ETH_DMASR_EBS_ReadTransf      ((unsigned int)0x01000000)  /* Error bits 0-write trnsf, 1-read transfr */
   #define ETH_DMASR_EBS_DataTransfTx    ((unsigned int)0x00800000)  /* Error bits 0-Rx DMA, 1-Tx DMA */
#define ETH_DMASR_TPS         ((unsigned int)0x00700000)  /* Transmit process state */
   #define ETH_DMASR_TPS_Stopped         ((unsigned int)0x00000000)  /* Stopped - Reset or Stop Tx Command issued  */
   #define ETH_DMASR_TPS_Fetching        ((unsigned int)0x00100000)  /* Running - fetching the Tx descriptor */
   #define ETH_DMASR_TPS_Waiting         ((unsigned int)0x00200000)  /* Running - waiting for status */
   #define ETH_DMASR_TPS_Reading         ((unsigned int)0x00300000)  /* Running - reading the data from host memory */
   #define ETH_DMASR_TPS_Suspended       ((unsigned int)0x00600000)  /* Suspended - Tx Descriptor unavailabe */
   #define ETH_DMASR_TPS_Closing         ((unsigned int)0x00700000)  /* Running - closing Rx descriptor */
#define ETH_DMASR_RPS         ((unsigned int)0x000E0000)  /* Receive process state */
   #define ETH_DMASR_RPS_Stopped         ((unsigned int)0x00000000)  /* Stopped - Reset or Stop Rx Command issued */
   #define ETH_DMASR_RPS_Fetching        ((unsigned int)0x00020000)  /* Running - fetching the Rx descriptor */
   #define ETH_DMASR_RPS_Waiting         ((unsigned int)0x00060000)  /* Running - waiting for packet */
   #define ETH_DMASR_RPS_Suspended       ((unsigned int)0x00080000)  /* Suspended - Rx Descriptor unavailable */
   #define ETH_DMASR_RPS_Closing         ((unsigned int)0x000A0000)  /* Running - closing descriptor */
   #define ETH_DMASR_RPS_Queuing         ((unsigned int)0x000E0000)  /* Running - queuing the recieve frame into host memory */
#define ETH_DMASR_NIS        ((unsigned int)0x00010000)  /* Normal interrupt summary */
#define ETH_DMASR_AIS        ((unsigned int)0x00008000)  /* Abnormal interrupt summary */
#define ETH_DMASR_ERS        ((unsigned int)0x00004000)  /* Early receive status */
#define ETH_DMASR_FBES       ((unsigned int)0x00002000)  /* Fatal bus error status */
#define ETH_DMASR_ETS        ((unsigned int)0x00000400)  /* Early transmit status */
#define ETH_DMASR_RWTS       ((unsigned int)0x00000200)  /* Receive watchdog timeout status */
#define ETH_DMASR_RPSS       ((unsigned int)0x00000100)  /* Receive process stopped status */
#define ETH_DMASR_RBUS       ((unsigned int)0x00000080)  /* Receive buffer unavailable status */
#define ETH_DMASR_RS         ((unsigned int)0x00000040)  /* Receive status */
#define ETH_DMASR_TUS        ((unsigned int)0x00000020)  /* Transmit underflow status */
#define ETH_DMASR_ROS        ((unsigned int)0x00000010)  /* Receive overflow status */
#define ETH_DMASR_TJTS       ((unsigned int)0x00000008)  /* Transmit jabber timeout status */
#define ETH_DMASR_TBUS       ((unsigned int)0x00000004)  /* Transmit buffer unavailable status */
#define ETH_DMASR_TPSS       ((unsigned int)0x00000002)  /* Transmit process stopped status */
#define ETH_DMASR_TS         ((unsigned int)0x00000001)  /* Transmit status */

#define ETH_DMAOMR_DTCEFD    ((unsigned int)0x04000000)  /* Disable Dropping of TCP/IP checksum error frames */
#define ETH_DMAOMR_RSF       ((unsigned int)0x02000000)  /* Receive store and forward */
#define ETH_DMAOMR_DFRF      ((unsigned int)0x01000000)  /* Disable flushing of received frames */
#define ETH_DMAOMR_TSF       ((unsigned int)0x00200000)  /* Transmit store and forward */
#define ETH_DMAOMR_FTF       ((unsigned int)0x00100000)  /* Flush transmit FIFO */
#define ETH_DMAOMR_TTC       ((unsigned int)0x0001C000)  /* Transmit threshold control */
   #define ETH_DMAOMR_TTC_64Bytes       ((unsigned int)0x00000000)  /* threshold level of the MTL Transmit FIFO is 64 Bytes */
   #define ETH_DMAOMR_TTC_128Bytes      ((unsigned int)0x00004000)  /* threshold level of the MTL Transmit FIFO is 128 Bytes */
   #define ETH_DMAOMR_TTC_192Bytes      ((unsigned int)0x00008000)  /* threshold level of the MTL Transmit FIFO is 192 Bytes */
   #define ETH_DMAOMR_TTC_256Bytes      ((unsigned int)0x0000C000)  /* threshold level of the MTL Transmit FIFO is 256 Bytes */
   #define ETH_DMAOMR_TTC_40Bytes       ((unsigned int)0x00010000)  /* threshold level of the MTL Transmit FIFO is 40 Bytes */
   #define ETH_DMAOMR_TTC_32Bytes       ((unsigned int)0x00014000)  /* threshold level of the MTL Transmit FIFO is 32 Bytes */
   #define ETH_DMAOMR_TTC_24Bytes       ((unsigned int)0x00018000)  /* threshold level of the MTL Transmit FIFO is 24 Bytes */
   #define ETH_DMAOMR_TTC_16Bytes       ((unsigned int)0x0001C000)  /* threshold level of the MTL Transmit FIFO is 16 Bytes */
#define ETH_DMAOMR_ST        ((unsigned int)0x00002000)  /* Start/stop transmission command */
#define ETH_DMAOMR_FEF       ((unsigned int)0x00000080)  /* Forward error frames */
#define ETH_DMAOMR_FUGF      ((unsigned int)0x00000040)  /* Forward undersized good frames */
#define ETH_DMAOMR_RTC       ((unsigned int)0x00000018)  /* receive threshold control */
   #define ETH_DMAOMR_RTC_64Bytes       ((unsigned int)0x00000000)  /* threshold level of the MTL Receive FIFO is 64 Bytes */
   #define ETH_DMAOMR_RTC_32Bytes       ((unsigned int)0x00000008)  /* threshold level of the MTL Receive FIFO is 32 Bytes */
   #define ETH_DMAOMR_RTC_96Bytes       ((unsigned int)0x00000010)  /* threshold level of the MTL Receive FIFO is 96 Bytes */
   #define ETH_DMAOMR_RTC_128Bytes      ((unsigned int)0x00000018)  /* threshold level of the MTL Receive FIFO is 128 Bytes */
#define ETH_DMAOMR_OSF       ((unsigned int)0x00000004)  /* operate on second frame */
#define ETH_DMAOMR_SR        ((unsigned int)0x00000002)  /* Start/stop receive */

#define ETH_DMAIER_NISE      ((unsigned int)0x00010000)  /* Normal interrupt summary enable */
#define ETH_DMAIER_AISE      ((unsigned int)0x00008000)  /* Abnormal interrupt summary enable */
#define ETH_DMAIER_ERIE      ((unsigned int)0x00004000)  /* Early receive interrupt enable */
#define ETH_DMAIER_FBEIE     ((unsigned int)0x00002000)  /* Fatal bus error interrupt enable */
#define ETH_DMAIER_ETIE      ((unsigned int)0x00000400)  /* Early transmit interrupt enable */
#define ETH_DMAIER_RWTIE     ((unsigned int)0x00000200)  /* Receive watchdog timeout interrupt enable */
#define ETH_DMAIER_RPSIE     ((unsigned int)0x00000100)  /* Receive process stopped interrupt enable */
#define ETH_DMAIER_RBUIE     ((unsigned int)0x00000080)  /* Receive buffer unavailable interrupt enable */
#define ETH_DMAIER_RIE       ((unsigned int)0x00000040)  /* Receive interrupt enable */
#define ETH_DMAIER_TUIE      ((unsigned int)0x00000020)  /* Transmit Underflow interrupt enable */
#define ETH_DMAIER_ROIE      ((unsigned int)0x00000010)  /* Receive Overflow interrupt enable */
#define ETH_DMAIER_TJTIE     ((unsigned int)0x00000008)  /* Transmit jabber timeout interrupt enable */
#define ETH_DMAIER_TBUIE     ((unsigned int)0x00000004)  /* Transmit buffer unavailable interrupt enable */
#define ETH_DMAIER_TPSIE     ((unsigned int)0x00000002)  /* Transmit process stopped interrupt enable */
#define ETH_DMAIER_TIE       ((unsigned int)0x00000001)  /* Transmit interrupt enable */

#define ETH_DMAMFBOCR_OFOC   ((unsigned int)0x10000000)  /* Overflow bit for FIFO overflow counter */
#define ETH_DMAMFBOCR_MFA    ((unsigned int)0x0FFE0000)  /* Number of frames missed by the application */
#define ETH_DMAMFBOCR_OMFC   ((unsigned int)0x00010000)  /* Overflow bit for missed frame counter */
#define ETH_DMAMFBOCR_MFC    ((unsigned int)0x0000FFFF)  /* Number of frames missed by the controller */

#define ETH_DMACHTDR_HTDAP   ((unsigned int)0xFFFFFFFF)  /* Host transmit descriptor address pointer */
#define ETH_DMACHRDR_HRDAP   ((unsigned int)0xFFFFFFFF)  /* Host receive descriptor address pointer */
#define ETH_DMACHTBAR_HTBAP  ((unsigned int)0xFFFFFFFF)  /* Host transmit buffer address pointer */
#define ETH_DMACHRBAR_HRBAP  ((unsigned int)0xFFFFFFFF)  /* Host receive buffer address pointer */


#define ETH_MAC_ADDR_HBASE   (ETH_MAC_BASE + 0x40)   /* ETHERNET MAC address high offset */
#define ETH_MAC_ADDR_LBASE    (ETH_MAC_BASE + 0x44)  /* ETHERNET MAC address low offset */

/* ETHERNET MACMIIAR register Mask */
#define MACMIIAR_CR_MASK    ((uint32_t)0xFFFFFFE3)

/* ETHERNET MACCR register Mask */
#define MACCR_CLEAR_MASK    ((uint32_t)0xFF20810F)

/* ETHERNET MACFCR register Mask */
#define MACFCR_CLEAR_MASK   ((uint32_t)0x0000FF41)

/* ETHERNET DMAOMR register Mask */
#define DMAOMR_CLEAR_MASK   ((uint32_t)0xF8DE3F23)

/* ETHERNET Remote Wake-up frame register length */
#define ETH_WAKEUP_REGISTER_LENGTH      8

/* ETHERNET Missed frames counter Shift */
#define  ETH_DMA_RX_OVERFLOW_MISSEDFRAMES_COUNTERSHIFT     17

/* ETHERNET DMA Tx descriptors Collision Count Shift */
#define  ETH_DMATXDESC_COLLISION_COUNTSHIFT        3

/* ETHERNET DMA Tx descriptors Buffer2 Size Shift */
#define  ETH_DMATXDESC_BUFFER2_SIZESHIFT           16

/* ETHERNET DMA Rx descriptors Frame Length Shift */
#define  ETH_DMARXDESC_FRAME_LENGTHSHIFT           16

/* ETHERNET DMA Rx descriptors Buffer2 Size Shift */
#define  ETH_DMARXDESC_BUFFER2_SIZESHIFT           16

/* ETHERNET errors */
#define  ETH_ERROR              ((uint32_t)0)
#define  ETH_SUCCESS            ((uint32_t)1)
#endif

/* ch32v00x_exti.h -----------------------------------------------------------*/

#ifndef __ASSEMBLER__

/* EXTI mode enumeration */
typedef enum
{
    EXTI_Mode_Interrupt = 0x00,
    EXTI_Mode_Event = 0x04
} EXTIMode_TypeDef;

/* EXTI Trigger enumeration */
typedef enum
{
    EXTI_Trigger_Rising = 0x08,
    EXTI_Trigger_Falling = 0x0C,
    EXTI_Trigger_Rising_Falling = 0x10
} EXTITrigger_TypeDef;

#endif

/* EXTI_Lines */
#define EXTI_Line0     ((uint32_t)0x00001) /* External interrupt line 0 */
#define EXTI_Line1     ((uint32_t)0x00002) /* External interrupt line 1 */
#define EXTI_Line2     ((uint32_t)0x00004) /* External interrupt line 2 */
#define EXTI_Line3     ((uint32_t)0x00008) /* External interrupt line 3 */
#define EXTI_Line4     ((uint32_t)0x00010) /* External interrupt line 4 */
#define EXTI_Line5     ((uint32_t)0x00020) /* External interrupt line 5 */
#define EXTI_Line6     ((uint32_t)0x00040) /* External interrupt line 6 */
#define EXTI_Line7     ((uint32_t)0x00080) /* External interrupt line 7 */
#define EXTI_Line8     ((uint32_t)0x00100) /* External interrupt line 8 Connected to the PVD Output */
#define EXTI_Line9     ((uint32_t)0x00200) /* External interrupt line 9 Connected to the PWR Auto Wake-up event*/
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_Line10    ((uint32_t)0x00400)  /* External interrupt line 10 */
#define EXTI_Line11    ((uint32_t)0x00800)  /* External interrupt line 11 */
#define EXTI_Line12    ((uint32_t)0x01000)  /* External interrupt line 12 */
#define EXTI_Line13    ((uint32_t)0x02000)  /* External interrupt line 13 */
#define EXTI_Line14    ((uint32_t)0x04000)  /* External interrupt line 14 */
#define EXTI_Line15    ((uint32_t)0x08000)  /* External interrupt line 15 */
#endif
#if defined(CH32V20x) || defined(CH32V30x)
#define EXTI_Line16    ((uint32_t)0x10000)  /* External interrupt line 16 Connected to the PVD Output */
#define EXTI_Line17    ((uint32_t)0x20000)  /* External interrupt line 17 Connected to the RTC Alarm event */
#define EXTI_Line18    ((uint32_t)0x40000)  /* External interrupt line 18 Connected to the USBD Device \
                                               Wakeup from suspend event */
#define EXTI_Line19    ((uint32_t)0x80000)  /* External interrupt line 19 Connected to the Ethernet Wakeup event */
#define EXTI_Line20    ((uint32_t)0x100000) /* External interrupt line 20 Connected to the USBFS Wakeup event */

#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
  #define EXTI_Line21    ((uint32_t)0x200000) /* External interrupt line 21 Connected to the OSCCAL Wakeup event */
#endif

#elif defined(CH32V10x)
#define EXTI_Line16    ((uint32_t)0x10000) /* External interrupt line 16 Connected to the PVD Output */
#define EXTI_Line17    ((uint32_t)0x20000) /* External interrupt line 17 Connected to the RTC Alarm event */
#define EXTI_Line18    ((uint32_t)0x40000)
#define EXTI_Line19    ((uint32_t)0x80000) /* External interrupt line 19 Connected to the USBHD Wakeup event */
#endif

/* ch32v00x_flash.h ----------------------------------------------------------*/


#ifndef __ASSEMBLER__
/* FLASH Status */
typedef enum
{
    FLASH_BUSY = 1,
    FLASH_ERROR_PG,
    FLASH_ERROR_WRP,
    FLASH_COMPLETE,
    FLASH_TIMEOUT
} FLASH_Status;
#endif

#if defined(CH32V003) || defined(CH32V10x)
/* Flash_Latency */
#define FLASH_Latency_0                  ((uint32_t)0x00000000) /* FLASH Zero Latency cycle */
#define FLASH_Latency_1                  ((uint32_t)0x00000001) /* FLASH One Latency cycle */
#define FLASH_Latency_2                  ((uint32_t)0x00000002) /* FLASH Two Latency cycles */
#endif

#if defined(CH32V10x)
/* Half_Cycle_Enable_Disable */
#define FLASH_HalfCycleAccess_Enable     ((uint32_t)0x00000008) /* FLASH Half Cycle Enable */
#define FLASH_HalfCycleAccess_Disable    ((uint32_t)0x00000000) /* FLASH Half Cycle Disable */

/* Prefetch_Buffer_Enable_Disable */
#define FLASH_PrefetchBuffer_Enable      ((uint32_t)0x00000010) /* FLASH Prefetch Buffer Enable */
#define FLASH_PrefetchBuffer_Disable     ((uint32_t)0x00000000) /* FLASH Prefetch Buffer Disable */
#endif

#ifdef CH32V003
/* Values to be used with CH32V00x devices (1page = 64Byte) */
#define FLASH_WRProt_Pages0to15          ((uint32_t)0x00000001) /* CH32 Low and Medium density devices: Write protection of page 0 to 15 */
#define FLASH_WRProt_Pages16to31         ((uint32_t)0x00000002) /* CH32 Low and Medium density devices: Write protection of page 16 to 31 */
#define FLASH_WRProt_Pages32to47         ((uint32_t)0x00000004) /* CH32 Low and Medium density devices: Write protection of page 32 to 47 */
#define FLASH_WRProt_Pages48to63         ((uint32_t)0x00000008) /* CH32 Low and Medium density devices: Write protection of page 48 to 63 */
#define FLASH_WRProt_Pages64to79         ((uint32_t)0x00000010) /* CH32 Low and Medium density devices: Write protection of page 64 to 79 */
#define FLASH_WRProt_Pages80to95         ((uint32_t)0x00000020) /* CH32 Low and Medium density devices: Write protection of page 80 to 95 */
#define FLASH_WRProt_Pages96to111        ((uint32_t)0x00000040) /* CH32 Low and Medium density devices: Write protection of page 96 to 111 */
#define FLASH_WRProt_Pages112to127       ((uint32_t)0x00000080) /* CH32 Low and Medium density devices: Write protection of page 112 to 127 */
#define FLASH_WRProt_Pages128to143       ((uint32_t)0x00000100) /* CH32 Medium-density devices: Write protection of page 128 to 143 */
#define FLASH_WRProt_Pages144to159       ((uint32_t)0x00000200) /* CH32 Medium-density devices: Write protection of page 144 to 159 */
#define FLASH_WRProt_Pages160to175       ((uint32_t)0x00000400) /* CH32 Medium-density devices: Write protection of page 160 to 175 */
#define FLASH_WRProt_Pages176to191       ((uint32_t)0x00000800) /* CH32 Medium-density devices: Write protection of page 176 to 191 */
#define FLASH_WRProt_Pages192to207       ((uint32_t)0x00001000) /* CH32 Medium-density devices: Write protection of page 192 to 207 */
#define FLASH_WRProt_Pages208to223       ((uint32_t)0x00002000) /* CH32 Medium-density devices: Write protection of page 208 to 223 */
#define FLASH_WRProt_Pages224to239       ((uint32_t)0x00004000) /* CH32 Medium-density devices: Write protection of page 224 to 239 */
#define FLASH_WRProt_Pages240to255       ((uint32_t)0x00008000) /* CH32 Medium-density devices: Write protection of page 240 to 255 */

#define FLASH_WRProt_AllPages            ((uint32_t)0x0000FFFF) /* Write protection of all Pages */

#elif defined(CH32V20x) || defined(CH32V30x)
/* Write Protect */
#define FLASH_WRProt_Sectors0          ((uint32_t)0x00000001) /* Write protection of setor 0  */
#define FLASH_WRProt_Sectors1          ((uint32_t)0x00000002) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors2          ((uint32_t)0x00000004) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors3          ((uint32_t)0x00000008) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors4          ((uint32_t)0x00000010) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors5          ((uint32_t)0x00000020) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors6          ((uint32_t)0x00000040) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors7          ((uint32_t)0x00000080) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors8          ((uint32_t)0x00000100) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors9          ((uint32_t)0x00000200) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors10         ((uint32_t)0x00000400) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors11         ((uint32_t)0x00000800) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors12         ((uint32_t)0x00001000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors13         ((uint32_t)0x00002000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors14         ((uint32_t)0x00004000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors15         ((uint32_t)0x00008000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors16         ((uint32_t)0x00010000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors17         ((uint32_t)0x00020000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors18         ((uint32_t)0x00040000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors19         ((uint32_t)0x00080000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors20         ((uint32_t)0x00100000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors21         ((uint32_t)0x00200000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors22         ((uint32_t)0x00400000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors23         ((uint32_t)0x00800000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors24         ((uint32_t)0x01000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors25         ((uint32_t)0x02000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors26         ((uint32_t)0x04000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors27         ((uint32_t)0x08000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors28         ((uint32_t)0x10000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors29         ((uint32_t)0x20000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors30         ((uint32_t)0x40000000) /* Write protection of setor 0 */
#define FLASH_WRProt_Sectors31to127    ((uint32_t)0x80000000) /* Write protection of page 62 to 255 */

#define FLASH_WRProt_AllSectors        ((uint32_t)0xFFFFFFFF) /* Write protection of all Sectors */

#elif defined(CH32V10x)

/* Values to be used with CH32V10x Low and Medium density devices */
#define FLASH_WRProt_Pages0to3           ((uint32_t)0x00000001) /* CH32 Low and Medium density devices: Write protection of page 0 to 3 */
#define FLASH_WRProt_Pages4to7           ((uint32_t)0x00000002) /* CH32 Low and Medium density devices: Write protection of page 4 to 7 */
#define FLASH_WRProt_Pages8to11          ((uint32_t)0x00000004) /* CH32 Low and Medium density devices: Write protection of page 8 to 11 */
#define FLASH_WRProt_Pages12to15         ((uint32_t)0x00000008) /* CH32 Low and Medium density devices: Write protection of page 12 to 15 */
#define FLASH_WRProt_Pages16to19         ((uint32_t)0x00000010) /* CH32 Low and Medium density devices: Write protection of page 16 to 19 */
#define FLASH_WRProt_Pages20to23         ((uint32_t)0x00000020) /* CH32 Low and Medium density devices: Write protection of page 20 to 23 */
#define FLASH_WRProt_Pages24to27         ((uint32_t)0x00000040) /* CH32 Low and Medium density devices: Write protection of page 24 to 27 */
#define FLASH_WRProt_Pages28to31         ((uint32_t)0x00000080) /* CH32 Low and Medium density devices: Write protection of page 28 to 31 */

/* Values to be used with CH32V10x Medium-density devices */
#define FLASH_WRProt_Pages32to35         ((uint32_t)0x00000100) /* CH32 Medium-density devices: Write protection of page 32 to 35 */
#define FLASH_WRProt_Pages36to39         ((uint32_t)0x00000200) /* CH32 Medium-density devices: Write protection of page 36 to 39 */
#define FLASH_WRProt_Pages40to43         ((uint32_t)0x00000400) /* CH32 Medium-density devices: Write protection of page 40 to 43 */
#define FLASH_WRProt_Pages44to47         ((uint32_t)0x00000800) /* CH32 Medium-density devices: Write protection of page 44 to 47 */
#define FLASH_WRProt_Pages48to51         ((uint32_t)0x00001000) /* CH32 Medium-density devices: Write protection of page 48 to 51 */
#define FLASH_WRProt_Pages52to55         ((uint32_t)0x00002000) /* CH32 Medium-density devices: Write protection of page 52 to 55 */
#define FLASH_WRProt_Pages56to59         ((uint32_t)0x00004000) /* CH32 Medium-density devices: Write protection of page 56 to 59 */
#define FLASH_WRProt_Pages60to63         ((uint32_t)0x00008000) /* CH32 Medium-density devices: Write protection of page 60 to 63 */
#define FLASH_WRProt_Pages64to67         ((uint32_t)0x00010000) /* CH32 Medium-density devices: Write protection of page 64 to 67 */
#define FLASH_WRProt_Pages68to71         ((uint32_t)0x00020000) /* CH32 Medium-density devices: Write protection of page 68 to 71 */
#define FLASH_WRProt_Pages72to75         ((uint32_t)0x00040000) /* CH32 Medium-density devices: Write protection of page 72 to 75 */
#define FLASH_WRProt_Pages76to79         ((uint32_t)0x00080000) /* CH32 Medium-density devices: Write protection of page 76 to 79 */
#define FLASH_WRProt_Pages80to83         ((uint32_t)0x00100000) /* CH32 Medium-density devices: Write protection of page 80 to 83 */
#define FLASH_WRProt_Pages84to87         ((uint32_t)0x00200000) /* CH32 Medium-density devices: Write protection of page 84 to 87 */
#define FLASH_WRProt_Pages88to91         ((uint32_t)0x00400000) /* CH32 Medium-density devices: Write protection of page 88 to 91 */
#define FLASH_WRProt_Pages92to95         ((uint32_t)0x00800000) /* CH32 Medium-density devices: Write protection of page 92 to 95 */
#define FLASH_WRProt_Pages96to99         ((uint32_t)0x01000000) /* CH32 Medium-density devices: Write protection of page 96 to 99 */
#define FLASH_WRProt_Pages100to103       ((uint32_t)0x02000000) /* CH32 Medium-density devices: Write protection of page 100 to 103 */
#define FLASH_WRProt_Pages104to107       ((uint32_t)0x04000000) /* CH32 Medium-density devices: Write protection of page 104 to 107 */
#define FLASH_WRProt_Pages108to111       ((uint32_t)0x08000000) /* CH32 Medium-density devices: Write protection of page 108 to 111 */
#define FLASH_WRProt_Pages112to115       ((uint32_t)0x10000000) /* CH32 Medium-density devices: Write protection of page 112 to 115 */
#define FLASH_WRProt_Pages116to119       ((uint32_t)0x20000000) /* CH32 Medium-density devices: Write protection of page 115 to 119 */
#define FLASH_WRProt_Pages120to123       ((uint32_t)0x40000000) /* CH32 Medium-density devices: Write protection of page 120 to 123 */
#define FLASH_WRProt_Pages124to127       ((uint32_t)0x80000000) /* CH32 Medium-density devices: Write protection of page 124 to 127 */

#define FLASH_WRProt_Pages62to255        ((uint32_t)0x80000000) /* CH32 Medium-density devices: Write protection of page 62 to 255 */

#define FLASH_WRProt_AllPages            ((uint32_t)0xFFFFFFFF) /* Write protection of all Pages */

#endif // defined(CH32V10x)

/* Option_Bytes_IWatchdog */
#define OB_IWDG_SW                       ((uint16_t)0x0001) /* Software IWDG selected */
#define OB_IWDG_HW                       ((uint16_t)0x0000) /* Hardware IWDG selected */

/* Option_Bytes_nRST_STOP */
#define OB_STOP_NoRST                    ((uint16_t)0x0002) /* No reset generated when entering in STOP */
#define OB_STOP_RST                      ((uint16_t)0x0000) /* Reset generated when entering in STOP */

/* Option_Bytes_nRST_STDBY */
#define OB_STDBY_NoRST                   ((uint16_t)0x0004) /* No reset generated when entering in STANDBY */
#define OB_STDBY_RST                     ((uint16_t)0x0000) /* Reset generated when entering in STANDBY */

#ifdef CH32V003
/* Option_Bytes_RST_ENandDT */
#define OB_RST_NoEN                      ((uint16_t)0x0018) /* Reset IO disable (PD7)*/
#define OB_RST_EN_DT12ms                 ((uint16_t)0x0010) /* Reset IO enable (PD7) and  Ignore delay time 12ms */
#define OB_RST_EN_DT1ms                  ((uint16_t)0x0008) /* Reset IO enable (PD7) and  Ignore delay time 1ms */
#define OB_RST_EN_DT128ms                ((uint16_t)0x0000) /* Reset IO enable (PD7) and  Ignore delay time 128ms */
#endif

/* FLASH_Interrupts */
#define FLASH_IT_ERROR                   ((uint32_t)0x00000400) /* FPEC error interrupt source */
#define FLASH_IT_EOP                     ((uint32_t)0x00001000) /* End of FLASH Operation Interrupt source */
#define FLASH_IT_BANK1_ERROR             FLASH_IT_ERROR         /* FPEC BANK1 error interrupt source */
#define FLASH_IT_BANK1_EOP               FLASH_IT_EOP           /* End of FLASH BANK1 Operation Interrupt source */

/* FLASH_Flags */
#define FLASH_FLAG_BSY                   ((uint32_t)0x00000001) /* FLASH Busy flag */
#define FLASH_FLAG_EOP                   ((uint32_t)0x00000020) /* FLASH End of Operation flag */
#if defined(CH32V10x)
#define FLASH_FLAG_PGERR                 ((uint32_t)0x00000004) /* FLASH Program error flag */
#endif
#define FLASH_FLAG_WRPRTERR              ((uint32_t)0x00000010) /* FLASH Write protected error flag */
#define FLASH_FLAG_OPTERR                ((uint32_t)0x00000001) /* FLASH Option Byte error flag */

#define FLASH_FLAG_BANK1_BSY             FLASH_FLAG_BSY       /* FLASH BANK1 Busy flag*/
#define FLASH_FLAG_BANK1_EOP             FLASH_FLAG_EOP       /* FLASH BANK1 End of Operation flag */
#if defined(CH32V10x)
#define FLASH_FLAG_BANK1_PGERR           FLASH_FLAG_PGERR     /* FLASH BANK1 Program error flag */
#endif
#define FLASH_FLAG_BANK1_WRPRTERR        FLASH_FLAG_WRPRTERR  /* FLASH BANK1 Write protected error flag */

#if defined(CH32V20x) || defined(CH32V30x)
/* FLASH_Access_CLK */
#define FLASH_Access_SYSTEM_HALF      ((uint32_t)0x00000000) /* FLASH Enhance Clock = SYSTEM */
#define FLASH_Access_SYSTEM           ((uint32_t)0x02000000) /* Enhance_CLK = SYSTEM/2 */
#endif

#if defined(CH32V003)
/* System_Reset_Start_Mode */
#define Start_Mode_USER                  ((uint32_t)0x00000000)
#define Start_Mode_BOOT                  ((uint32_t)0x00004000)
#endif

#if defined(CH32V30x)

/* ch32v30x_fsmc.h ------------------------------------------------------------*/

/* FSMC_NORSRAM_Bank */
#define FSMC_Bank1_NORSRAM1                             ((uint32_t)0x00000000)

/* FSMC_NAND_Bank */
#define FSMC_Bank2_NAND                                 ((uint32_t)0x00000010)

/* FSMC_Data_Address_Bus_Multiplexing */
#define FSMC_DataAddressMux_Disable                     ((uint32_t)0x00000000)
#define FSMC_DataAddressMux_Enable                      ((uint32_t)0x00000002)

/* FSMC_Memory_Type */
#define FSMC_MemoryType_SRAM                            ((uint32_t)0x00000000)
#define FSMC_MemoryType_PSRAM                           ((uint32_t)0x00000004)
#define FSMC_MemoryType_NOR                             ((uint32_t)0x00000008)

/* FSMC_Data_Width */
#define FSMC_MemoryDataWidth_8b                         ((uint32_t)0x00000000)
#define FSMC_MemoryDataWidth_16b                        ((uint32_t)0x00000010)

/* FSMC_Burst_Access_Mode */
#define FSMC_BurstAccessMode_Disable                    ((uint32_t)0x00000000)
#define FSMC_BurstAccessMode_Enable                     ((uint32_t)0x00000100)

/* FSMC_AsynchronousWait */
#define FSMC_AsynchronousWait_Disable                   ((uint32_t)0x00000000)
#define FSMC_AsynchronousWait_Enable                    ((uint32_t)0x00008000)

/* FSMC_Wait_Signal_Polarity */
#define FSMC_WaitSignalPolarity_Low                     ((uint32_t)0x00000000)
#define FSMC_WaitSignalPolarity_High                    ((uint32_t)0x00000200)

/* FSMC_Wrap_Mode */
#define FSMC_WrapMode_Disable                           ((uint32_t)0x00000000)
#define FSMC_WrapMode_Enable                            ((uint32_t)0x00000400)

/* FSMC_Wait_Timing */
#define FSMC_WaitSignalActive_BeforeWaitState           ((uint32_t)0x00000000)
#define FSMC_WaitSignalActive_DuringWaitState           ((uint32_t)0x00000800)

/* FSMC_Write_Operation */
#define FSMC_WriteOperation_Disable                     ((uint32_t)0x00000000)
#define FSMC_WriteOperation_Enable                      ((uint32_t)0x00001000)

/* FSMC_Wait_Signal */
#define FSMC_WaitSignal_Disable                         ((uint32_t)0x00000000)
#define FSMC_WaitSignal_Enable                          ((uint32_t)0x00002000)

/* FSMC_Extended_Mode */
#define FSMC_ExtendedMode_Disable                       ((uint32_t)0x00000000)
#define FSMC_ExtendedMode_Enable                        ((uint32_t)0x00004000)

/* FSMC_Write_Burst */
#define FSMC_WriteBurst_Disable                         ((uint32_t)0x00000000)
#define FSMC_WriteBurst_Enable                          ((uint32_t)0x00080000)

/* FSMC_Access_Mode */
#define FSMC_AccessMode_A                               ((uint32_t)0x00000000)
#define FSMC_AccessMode_B                               ((uint32_t)0x10000000)
#define FSMC_AccessMode_C                               ((uint32_t)0x20000000)
#define FSMC_AccessMode_D                               ((uint32_t)0x30000000)

/* FSMC_Wait_feature */
#define FSMC_Waitfeature_Disable                        ((uint32_t)0x00000000)
#define FSMC_Waitfeature_Enable                         ((uint32_t)0x00000002)

/* FSMC_ECC */
#define FSMC_ECC_Disable                                ((uint32_t)0x00000000)
#define FSMC_ECC_Enable                                 ((uint32_t)0x00000040)

/* FSMC_ECC_Page_Size */
#define FSMC_ECCPageSize_256Bytes                       ((uint32_t)0x00000000)
#define FSMC_ECCPageSize_512Bytes                       ((uint32_t)0x00020000)
#define FSMC_ECCPageSize_1024Bytes                      ((uint32_t)0x00040000)
#define FSMC_ECCPageSize_2048Bytes                      ((uint32_t)0x00060000)
#define FSMC_ECCPageSize_4096Bytes                      ((uint32_t)0x00080000)
#define FSMC_ECCPageSize_8192Bytes                      ((uint32_t)0x000A0000)

/* FSMC_Interrupt_sources */
#define FSMC_IT_RisingEdge                              ((uint32_t)0x00000008)
#define FSMC_IT_Level                                   ((uint32_t)0x00000010)
#define FSMC_IT_FallingEdge                             ((uint32_t)0x00000020)

/* FSMC_Flags */
#define FSMC_FLAG_RisingEdge                            ((uint32_t)0x00000001)
#define FSMC_FLAG_Level                                 ((uint32_t)0x00000002)
#define FSMC_FLAG_FallingEdge                           ((uint32_t)0x00000004)
#define FSMC_FLAG_FEMPT                                 ((uint32_t)0x00000040)

#endif

/* ch32v00x_gpio.h ------------------------------------------------------------*/

#ifndef __ASSEMBLER__

/* Output Maximum frequency selection */
typedef enum
{
	GPIO_Speed_In,
	GPIO_Speed_10MHz,
	GPIO_Speed_2MHz,
	GPIO_Speed_50MHz
} GPIOSpeed_TypeDef;

#endif

#define GPIO_CNF_IN_ANALOG   0
#define GPIO_CNF_IN_FLOATING 4
#define GPIO_CNF_IN_PUPD     8
#define GPIO_CNF_OUT_PP      0
#define GPIO_CNF_OUT_OD      4
#define GPIO_CNF_OUT_PP_AF   8
#define GPIO_CNF_OUT_OD_AF   12

/* Configuration Mode enumeration */
/*
typedef enum
{
    GPIO_Mode_AIN = 0x0,
    GPIO_Mode_IN_FLOATING = 0x04,
    GPIO_Mode_IPD = 0x28,
    GPIO_Mode_IPU = 0x48,
    GPIO_Mode_Out_OD = 0x14,
    GPIO_Mode_Out_PP = 0x10,
    GPIO_Mode_AF_OD = 0x1C,
    GPIO_Mode_AF_PP = 0x18
} GPIOMode_TypeDef;
*/

#ifndef __ASSEMBLER__

/* Bit_SET and Bit_RESET enumeration */
typedef enum
{
    Bit_RESET = 0,
    Bit_SET
} BitAction;

#endif

/* GPIO_pins_define */
#define GPIO_Pin_0                     ((uint16_t)0x0001) /* Pin 0 selected */
#define GPIO_Pin_1                     ((uint16_t)0x0002) /* Pin 1 selected */
#define GPIO_Pin_2                     ((uint16_t)0x0004) /* Pin 2 selected */
#define GPIO_Pin_3                     ((uint16_t)0x0008) /* Pin 3 selected */
#define GPIO_Pin_4                     ((uint16_t)0x0010) /* Pin 4 selected */
#define GPIO_Pin_5                     ((uint16_t)0x0020) /* Pin 5 selected */
#define GPIO_Pin_6                     ((uint16_t)0x0040) /* Pin 6 selected */
#define GPIO_Pin_7                     ((uint16_t)0x0080) /* Pin 7 selected */
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define GPIO_Pin_8                      ((uint16_t)0x0100) /* Pin 8 selected */
#define GPIO_Pin_9                      ((uint16_t)0x0200) /* Pin 9 selected */
#define GPIO_Pin_10                     ((uint16_t)0x0400) /* Pin 10 selected */
#define GPIO_Pin_11                     ((uint16_t)0x0800) /* Pin 11 selected */
#define GPIO_Pin_12                     ((uint16_t)0x1000) /* Pin 12 selected */
#define GPIO_Pin_13                     ((uint16_t)0x2000) /* Pin 13 selected */
#define GPIO_Pin_14                     ((uint16_t)0x4000) /* Pin 14 selected */
#define GPIO_Pin_15                     ((uint16_t)0x8000) /* Pin 15 selected */
#endif
#define GPIO_Pin_All                   ((uint16_t)0xFFFF) /* All pins selected */

/* GPIO_Remap_define */
#ifdef CH32V003

#define GPIO_Remap_SPI1                ((uint32_t)0x00000001) /* SPI1 Alternate Function mapping */
#define GPIO_PartialRemap_I2C1         ((uint32_t)0x10000002) /* I2C1 Partial Alternate Function mapping */
#define GPIO_FullRemap_I2C1            ((uint32_t)0x10400002) /* I2C1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_USART1      ((uint32_t)0x80000004) /* USART1 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_USART1      ((uint32_t)0x80200000) /* USART1 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_USART1          ((uint32_t)0x80200004) /* USART1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_TIM1        ((uint32_t)0x00160040) /* TIM1 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_TIM1        ((uint32_t)0x00160080) /* TIM1 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_TIM1            ((uint32_t)0x001600C0) /* TIM1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_TIM2        ((uint32_t)0x00180100) /* TIM2 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_TIM2        ((uint32_t)0x00180200) /* TIM2 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_TIM2            ((uint32_t)0x00180300) /* TIM2 Full Alternate Function mapping */
#define GPIO_Remap_PA1_2               ((uint32_t)0x00008000) /* PA1 and PA2 Alternate Function mapping */
#define GPIO_Remap_ADC1_ETRGINJ        ((uint32_t)0x00200002) /* ADC1 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC1_ETRGREG        ((uint32_t)0x00200004) /* ADC1 External Trigger Regular Conversion remapping */
#define GPIO_Remap_LSI_CAL             ((uint32_t)0x00200080) /* LSI calibration Alternate Function mapping */
#define GPIO_Remap_SDI_Disable         ((uint32_t)0x00300400) /* SDI Disabled */

#elif defined(CH32V20x) || defined(CH32V30x)

/* PCFR1 */
#define GPIO_Remap_SPI1                 ((uint32_t)0x00000001) /* SPI1 Alternate Function mapping */
#define GPIO_Remap_I2C1                 ((uint32_t)0x00000002) /* I2C1 Alternate Function mapping */
#define GPIO_Remap_USART1               ((uint32_t)0x00000004) /* USART1 Alternate Function mapping low bit */
#define GPIO_Remap_USART2               ((uint32_t)0x00000008) /* USART2 Alternate Function mapping */
#define GPIO_PartialRemap_USART3        ((uint32_t)0x00140010) /* USART3 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART3           ((uint32_t)0x00140030) /* USART3 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM1          ((uint32_t)0x00160040) /* TIM1 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM1             ((uint32_t)0x001600C0) /* TIM1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_TIM2         ((uint32_t)0x00180100) /* TIM2 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_TIM2         ((uint32_t)0x00180200) /* TIM2 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_TIM2             ((uint32_t)0x00180300) /* TIM2 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM3          ((uint32_t)0x001A0800) /* TIM3 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM3             ((uint32_t)0x001A0C00) /* TIM3 Full Alternate Function mapping */
#define GPIO_Remap_TIM4                 ((uint32_t)0x00001000) /* TIM4 Alternate Function mapping */
#define GPIO_Remap1_CAN1                ((uint32_t)0x001D4000) /* CAN1 Alternate Function mapping */
#define GPIO_Remap2_CAN1                ((uint32_t)0x001D6000) /* CAN1 Alternate Function mapping */
#define GPIO_Remap_PD01                 ((uint32_t)0x00008000) /* PD01 Alternate Function mapping */
#define GPIO_Remap_TIM5CH4_LSI          ((uint32_t)0x00200001) /* LSI connected to TIM5 Channel4 input capture for calibration */
#define GPIO_Remap_ADC1_ETRGINJ         ((uint32_t)0x00200002) /* ADC1 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC1_ETRGREG         ((uint32_t)0x00200004) /* ADC1 External Trigger Regular Conversion remapping */
#define GPIO_Remap_ADC2_ETRGINJ         ((uint32_t)0x00200008) /* ADC2 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC2_ETRGREG         ((uint32_t)0x00200010) /* ADC2 External Trigger Regular Conversion remapping */
#define GPIO_Remap_ETH                  ((uint32_t)0x00200020) /* Ethernet remapping (only for Connectivity line devices) */
#define GPIO_Remap_CAN2                 ((uint32_t)0x00200040) /* CAN2 remapping (only for Connectivity line devices) */
#define GPIO_Remap_MII_RMII_SEL         ((uint32_t)0x00200080) /* MII or RMII selection */
#define GPIO_Remap_SWJ_Disable          ((uint32_t)0x00300400) /* Full SWJ Disabled (JTAG-DP + SW-DP) */
#define GPIO_Remap_SPI3                 ((uint32_t)0x00201000) /* SPI3/I2S3 Alternate Function mapping (only for Connectivity line devices) */
#define GPIO_Remap_TIM2ITR1_PTP_SOF     ((uint32_t)0x00202000) /* Ethernet PTP output or USB OTG SOF (Start of Frame) connected \
                                                                  to TIM2 Internal Trigger 1 for calibration                    \
                                                                  (only for Connectivity line devices) */
#define GPIO_Remap_PTP_PPS              ((uint32_t)0x00204000) /* Ethernet MAC PPS_PTS output on PB05 (only for Connectivity line devices) */

/* PCFR2 */
#define GPIO_Remap_TIM8                 ((uint32_t)0x80000004) /* TIM8 Alternate Function mapping */
#define GPIO_PartialRemap_TIM9          ((uint32_t)0x80130008) /* TIM9 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM9             ((uint32_t)0x80130010) /* TIM9 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM10         ((uint32_t)0x80150020) /* TIM10 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM10            ((uint32_t)0x80150040) /* TIM10 Full Alternate Function mapping */
#define GPIO_Remap_FSMC_NADV            ((uint32_t)0x80000400) /* FSMC_NADV Alternate Function mapping */
#define GPIO_PartialRemap_USART4        ((uint32_t)0x80300001) /* USART4 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART4           ((uint32_t)0x80300002) /* USART4 Full Alternate Function mapping */
#define GPIO_PartialRemap_USART5        ((uint32_t)0x80320004) /* USART5 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART5           ((uint32_t)0x80320008) /* USART5 Full Alternate Function mapping */
#define GPIO_PartialRemap_USART6        ((uint32_t)0x80340010) /* USART6 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART6           ((uint32_t)0x80340020) /* USART6 Full Alternate Function mapping */
#define GPIO_PartialRemap_USART7        ((uint32_t)0x80360040) /* USART7 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART7           ((uint32_t)0x80360080) /* USART7 Full Alternate Function mapping */
#define GPIO_PartialRemap_USART8        ((uint32_t)0x80380100) /* USART8 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART8           ((uint32_t)0x80380200) /* USART8 Full Alternate Function mapping */
#define GPIO_Remap_USART1_HighBit       ((uint32_t)0x80200400) /* USART1 Alternate Function mapping high bit */

#elif defined(CH32V10x)

/* GPIO_Remap_define */
#define GPIO_Remap_SPI1                ((uint32_t)0x00000001) /* SPI1 Alternate Function mapping */
#define GPIO_Remap_I2C1                ((uint32_t)0x00000002) /* I2C1 Alternate Function mapping */
#define GPIO_Remap_USART1              ((uint32_t)0x00000004) /* USART1 Alternate Function mapping */
#define GPIO_Remap_USART2              ((uint32_t)0x00000008) /* USART2 Alternate Function mapping */
#define GPIO_PartialRemap_USART3       ((uint32_t)0x00140010) /* USART3 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART3          ((uint32_t)0x00140030) /* USART3 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM1         ((uint32_t)0x00160040) /* TIM1 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM1            ((uint32_t)0x001600C0) /* TIM1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_TIM2        ((uint32_t)0x00180100) /* TIM2 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_TIM2        ((uint32_t)0x00180200) /* TIM2 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_TIM2            ((uint32_t)0x00180300) /* TIM2 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM3         ((uint32_t)0x001A0800) /* TIM3 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM3            ((uint32_t)0x001A0C00) /* TIM3 Full Alternate Function mapping */
#define GPIO_Remap_TIM4                ((uint32_t)0x00001000) /* TIM4 Alternate Function mapping */
#define GPIO_Remap1_CAN1               ((uint32_t)0x001D4000) /* CAN1 Alternate Function mapping */
#define GPIO_Remap2_CAN1               ((uint32_t)0x001D6000) /* CAN1 Alternate Function mapping */
#define GPIO_Remap_PD01                ((uint32_t)0x00008000) /* PD01 Alternate Function mapping */
#define GPIO_Remap_ADC1_ETRGINJ        ((uint32_t)0x00200002) /* ADC1 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC1_ETRGREG        ((uint32_t)0x00200004) /* ADC1 External Trigger Regular Conversion remapping */
#define GPIO_Remap_SWJ_Disable         ((uint32_t)0x00300400) /* Full SWJ Disabled (JTAG-DP + SW-DP) */
#define GPIO_Remap_TIM2ITR1_PTP_SOF    ((uint32_t)0x00202000) /* Ethernet PTP output or USB OTG SOF (Start of Frame) connected \
                                                                 to TIM2 Internal Trigger 1 for calibration                    \
                                                                 (only for Connectivity line devices) */
#define GPIO_Remap_TIM1_DMA            ((uint32_t)0x80000010) /* TIM1 DMA requests mapping (only for Value line devices) */
#define GPIO_Remap_TIM67_DAC_DMA       ((uint32_t)0x80000800) /* TIM6/TIM7 and DAC DMA requests remapping (only for High density Value line devices) */
#define GPIO_Remap_MISC                ((uint32_t)0x80002000) /* Miscellaneous Remap (DMA2 Channel5 Position and DAC Trigger remapping, \
                                                                   only for High density Value line devices) */

#endif // defined(CH32V10x)

/* GPIO_Port_Sources */
#define GPIO_PortSourceGPIOA           ((uint8_t)0x00)
#define GPIO_PortSourceGPIOC           ((uint8_t)0x02)
#define GPIO_PortSourceGPIOD           ((uint8_t)0x03)
#if defined(CH32X03x)
#define GPIO_PortSourceGPIOB            ((uint8_t)0x01)
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define GPIO_PortSourceGPIOB            ((uint8_t)0x01)
#define GPIO_PortSourceGPIOD            ((uint8_t)0x03)
#define GPIO_PortSourceGPIOE            ((uint8_t)0x04)
#define GPIO_PortSourceGPIOF            ((uint8_t)0x05)
#define GPIO_PortSourceGPIOG            ((uint8_t)0x06)
#endif

/* GPIO_Pin_sources */
#define GPIO_PinSource0                ((uint8_t)0x00)
#define GPIO_PinSource1                ((uint8_t)0x01)
#define GPIO_PinSource2                ((uint8_t)0x02)
#define GPIO_PinSource3                ((uint8_t)0x03)
#define GPIO_PinSource4                ((uint8_t)0x04)
#define GPIO_PinSource5                ((uint8_t)0x05)
#define GPIO_PinSource6                ((uint8_t)0x06)
#define GPIO_PinSource7                ((uint8_t)0x07)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined(CH32X03x)
#define GPIO_PinSource8                 ((uint8_t)0x08)
#define GPIO_PinSource9                 ((uint8_t)0x09)
#define GPIO_PinSource10                ((uint8_t)0x0A)
#define GPIO_PinSource11                ((uint8_t)0x0B)
#define GPIO_PinSource12                ((uint8_t)0x0C)
#define GPIO_PinSource13                ((uint8_t)0x0D)
#define GPIO_PinSource14                ((uint8_t)0x0E)
#define GPIO_PinSource15                ((uint8_t)0x0F)
#endif

#if defined(CH32V20x) || defined(CH32V30x)
/* Ethernet_Media_Interface */
#define GPIO_ETH_MediaInterface_MII     ((u32)0x00000000)
#define GPIO_ETH_MediaInterface_RMII    ((u32)0x00000001)
#endif

/* ch32v00x_i2c.h ------------------------------------------------------------*/

/* I2C_mode */
#define I2C_Mode_I2C                                         ((uint16_t)0x0000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C_Mode_SMBusDevice                                 ((uint16_t)0x0002)
#define I2C_Mode_SMBusHost                                   ((uint16_t)0x000A)
#endif

/* I2C_duty_cycle_in_fast_mode */
#define I2C_DutyCycle_16_9                                   ((uint16_t)0x4000) /* I2C fast mode Tlow/Thigh = 16/9 */
#define I2C_DutyCycle_2                                      ((uint16_t)0xBFFF) /* I2C fast mode Tlow/Thigh = 2 */

/* I2C_acknowledgement */
#define I2C_Ack_Enable                                       ((uint16_t)0x0400)
#define I2C_Ack_Disable                                      ((uint16_t)0x0000)

/* I2C_transfer_direction */
#define I2C_Direction_Transmitter                            ((uint8_t)0x00)
#define I2C_Direction_Receiver                               ((uint8_t)0x01)

/* I2C_acknowledged_address */
#define I2C_AcknowledgedAddress_7bit                         ((uint16_t)0x4000)
#define I2C_AcknowledgedAddress_10bit                        ((uint16_t)0xC000)

/* I2C_registers */
#define I2C_Register_CTLR1                                   ((uint8_t)0x00)
#define I2C_Register_CTLR2                                   ((uint8_t)0x04)
#define I2C_Register_OADDR1                                  ((uint8_t)0x08)
#define I2C_Register_OADDR2                                  ((uint8_t)0x0C)
#define I2C_Register_DATAR                                   ((uint8_t)0x10)
#define I2C_Register_STAR1                                   ((uint8_t)0x14)
#define I2C_Register_STAR2                                   ((uint8_t)0x18)
#define I2C_Register_CKCFGR                                  ((uint8_t)0x1C)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C_Register_RTR                                     ((uint8_t)0x20)

/* I2C_SMBus_alert_pin_level */
#define I2C_SMBusAlert_Low                                   ((uint16_t)0x2000)
#define I2C_SMBusAlert_High                                  ((uint16_t)0xDFFF)
#endif

/* I2C_PEC_position */
#define I2C_PECPosition_Next                                 ((uint16_t)0x0800)
#define I2C_PECPosition_Current                              ((uint16_t)0xF7FF)

/* I2C_NACK_position */
#define I2C_NACKPosition_Next                                ((uint16_t)0x0800)
#define I2C_NACKPosition_Current                             ((uint16_t)0xF7FF)

/* I2C_interrupts_definition */
#define I2C_IT_BUF                                           ((uint16_t)0x0400)
#define I2C_IT_EVT                                           ((uint16_t)0x0200)
#define I2C_IT_ERR                                           ((uint16_t)0x0100)

/* I2C_interrupts_definition */
#define I2C_IT_PECERR                                        ((uint32_t)0x01001000)
#define I2C_IT_OVR                                           ((uint32_t)0x01000800)
#define I2C_IT_AF                                            ((uint32_t)0x01000400)
#define I2C_IT_ARLO                                          ((uint32_t)0x01000200)
#define I2C_IT_BERR                                          ((uint32_t)0x01000100)
#define I2C_IT_TXE                                           ((uint32_t)0x06000080)
#define I2C_IT_RXNE                                          ((uint32_t)0x06000040)
#define I2C_IT_STOPF                                         ((uint32_t)0x02000010)
#define I2C_IT_ADD10                                         ((uint32_t)0x02000008)
#define I2C_IT_BTF                                           ((uint32_t)0x02000004)
#define I2C_IT_ADDR                                          ((uint32_t)0x02000002)
#define I2C_IT_SB                                            ((uint32_t)0x02000001)

/* SR2 register flags  */
#define I2C_FLAG_DUALF                                       ((uint32_t)0x00800000)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C_FLAG_SMBHOST                                     ((uint32_t)0x00400000)
#define I2C_FLAG_SMBDEFAULT                                  ((uint32_t)0x00200000)
#endif
#define I2C_FLAG_GENCALL                                     ((uint32_t)0x00100000)
#define I2C_FLAG_TRA                                         ((uint32_t)0x00040000)
#define I2C_FLAG_BUSY                                        ((uint32_t)0x00020000)
#define I2C_FLAG_MSL                                         ((uint32_t)0x00010000)

/* SR1 register flags */
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define I2C_FLAG_SMBALERT                                    ((uint32_t)0x10008000)
#define I2C_FLAG_TIMEOUT                                     ((uint32_t)0x10004000)
#endif
#define I2C_FLAG_PECERR                                      ((uint32_t)0x10001000)
#define I2C_FLAG_OVR                                         ((uint32_t)0x10000800)
#define I2C_FLAG_AF                                          ((uint32_t)0x10000400)
#define I2C_FLAG_ARLO                                        ((uint32_t)0x10000200)
#define I2C_FLAG_BERR                                        ((uint32_t)0x10000100)
#define I2C_FLAG_TXE                                         ((uint32_t)0x10000080)
#define I2C_FLAG_RXNE                                        ((uint32_t)0x10000040)
#define I2C_FLAG_STOPF                                       ((uint32_t)0x10000010)
#define I2C_FLAG_ADD10                                       ((uint32_t)0x10000008)
#define I2C_FLAG_BTF                                         ((uint32_t)0x10000004)
#define I2C_FLAG_ADDR                                        ((uint32_t)0x10000002)
#define I2C_FLAG_SB                                          ((uint32_t)0x10000001)

/****************I2C Master Events (Events grouped in order of communication)********************/

/********************************************************************************************************************
  * @brief  Start communicate
  *
  * After master use I2C_GenerateSTART() function sending the START condition,the master
  * has to wait for event 5(the Start condition has been correctly
  * released on the I2C bus ).
  *
  */
/* EVT5 */
#define  I2C_EVENT_MASTER_MODE_SELECT                      ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */

/********************************************************************************************************************
  * @brief  Address Acknowledge
  *
  * When start condition correctly released on the bus(check EVT5), the
  * master use I2C_Send7bitAddress() function sends the address of the slave(s) with which it will communicate
  * it also determines master as transmitter or Receiver. Then the master has to wait that a slave acknowledges
  * his address. If an acknowledge is sent on the bus, one of the following events will be set:
  *
  *
  *
  *  1) In case of Master Receiver (7-bit addressing): the I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED
  *     event is set.
  *
  *  2) In case of Master Transmitter (7-bit addressing): the I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
  *     is set
  *
  *  3) In case of 10-Bit addressing mode, the master (after generating the START
  *  and checking on EVT5) use I2C_SendData() function send the header of 10-bit addressing mode.
  *  Then master wait EVT9. EVT9 means that the 10-bit addressing header has been correctly sent
  *  on the bus. Then master should use the function I2C_Send7bitAddress() to send the second part
  *  of the 10-bit address (LSB) . Then master should wait for event 6.
  *
  *
  */

/* EVT6 */
#define  I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED        ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define  I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED           ((uint32_t)0x00030002)  /* BUSY, MSL and ADDR flags */
/*EVT9 */
#define  I2C_EVENT_MASTER_MODE_ADDRESS10                   ((uint32_t)0x00030008)  /* BUSY, MSL and ADD10 flags */

/********************************************************************************************************************
  * @brief Communication events
  *
  * If START condition has generated and slave address
  * been acknowledged. then the master has to check one of the following events for
  * communication procedures:
  *
  * 1) Master Receiver mode: The master has to wait on the event EVT7 then use
  *   I2C_ReceiveData() function to read the data received from the slave .
  *
  * 2) Master Transmitter mode: The master use I2C_SendData() function to send data
  *     then to wait on event EVT8 or EVT8_2.
  *    These two events are similar:
  *     - EVT8 means that the data has been written in the data register and is
  *       being shifted out.
  *     - EVT8_2 means that the data has been physically shifted out and output
  *       on the bus.
  *     In most cases, using EVT8 is sufficient for the application.
  *     Using EVT8_2  will leads to a slower communication  speed but will more reliable .
  *     EVT8_2 is also more suitable than EVT8 for testing on the last data transmission
  *
  *
  *  Note:
  *  In case the  user software does not guarantee that this event EVT7 is managed before
  *  the current byte end of transfer, then user may check on I2C_EVENT_MASTER_BYTE_RECEIVED
  *  and I2C_FLAG_BTF flag at the same time .But in this case the communication may be slower.
  *
  *
  */

/* Master Receive mode */
/* EVT7 */
#define  I2C_EVENT_MASTER_BYTE_RECEIVED                    ((uint32_t)0x00030040)  /* BUSY, MSL and RXNE flags */

/* Master Transmitter mode*/
/* EVT8 */
#define I2C_EVENT_MASTER_BYTE_TRANSMITTING                 ((uint32_t)0x00070080) /* TRA, BUSY, MSL, TXE flags */
/* EVT8_2 */
#define  I2C_EVENT_MASTER_BYTE_TRANSMITTED                 ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */

/******************I2C Slave Events (Events grouped in order of communication)******************/

/********************************************************************************************************************
  * @brief  Start Communicate events
  *
  * Wait on one of these events at the start of the communication. It means that
  * the I2C peripheral detected a start condition of master device generate on the bus.
  * If the acknowledge feature is enabled through function I2C_AcknowledgeConfig()),The peripheral generates an ACK condition on the bus.
  *
  *
  *
  * a) In normal case (only one address managed by the slave), when the address
  *   sent by the master matches the own address of the peripheral (configured by
  *   I2C_OwnAddress1 field) the I2C_EVENT_SLAVE_XXX_ADDRESS_MATCHED event is set
  *   (where XXX could be TRANSMITTER or RECEIVER).
  *
  * b) In case the address sent by the master matches the second address of the
  *   peripheral (configured by the function I2C_OwnAddress2Config() and enabled
  *   by the function I2C_DualAddressCmd()) the events I2C_EVENT_SLAVE_XXX_SECONDADDRESS_MATCHED
  *   (where XXX could be TRANSMITTER or RECEIVER) are set.
  *
  * c) In case the address sent by the master is General Call (address 0x00) and
  *   if the General Call is enabled for the peripheral (using function I2C_GeneralCallCmd())
  *   the following event is set I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED.
  *
  */

/* EVT1 */
/* a) Case of One Single Address managed by the slave */
#define  I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED          ((uint32_t)0x00020002) /* BUSY and ADDR flags */
#define  I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED       ((uint32_t)0x00060082) /* TRA, BUSY, TXE and ADDR flags */

/* b) Case of Dual address managed by the slave */
#define  I2C_EVENT_SLAVE_RECEIVER_SECONDADDRESS_MATCHED    ((uint32_t)0x00820000)  /* DUALF and BUSY flags */
#define  I2C_EVENT_SLAVE_TRANSMITTER_SECONDADDRESS_MATCHED ((uint32_t)0x00860080)  /* DUALF, TRA, BUSY and TXE flags */

/* c) Case of General Call enabled for the slave */
#define  I2C_EVENT_SLAVE_GENERALCALLADDRESS_MATCHED        ((uint32_t)0x00120000)  /* GENCALL and BUSY flags */

/********************************************************************************************************************
  * @brief  Communication events
  *
  * Wait on one of these events when EVT1 has already been checked :
  *
  * - Slave Receiver mode:
  *     - EVT2--The device is expecting to receive a data byte .
  *     - EVT4--The device is expecting the end of the communication: master
  *       sends a stop condition and data transmission is stopped.
  *
  * - Slave Transmitter mode:
  *    - EVT3--When a byte has been transmitted by the slave and the Master is expecting
  *      the end of the byte transmission. The two events I2C_EVENT_SLAVE_BYTE_TRANSMITTED and
  *      I2C_EVENT_SLAVE_BYTE_TRANSMITTING are similar. If the user software doesn't guarantee
  *      the EVT3 is managed before the current byte end of transfer The second one can optionally
  *      be used.
  *    - EVT3_2--When the master sends a NACK  to tell slave device that data transmission
  *      shall end . The slave device has to stop sending
  *      data bytes and wait a Stop condition from bus.
  *
  *  Note:
  *  If the  user software does not guarantee that the event 2 is
  *  managed before the current byte end of transfer, User may check on I2C_EVENT_SLAVE_BYTE_RECEIVED
  *  and I2C_FLAG_BTF flag at the same time .
  *  In this case the communication will be slower.
  *
  */

/* Slave Receiver mode*/
/* EVT2 */
#define  I2C_EVENT_SLAVE_BYTE_RECEIVED                     ((uint32_t)0x00020040)  /* BUSY and RXNE flags */
/* EVT4  */
#define  I2C_EVENT_SLAVE_STOP_DETECTED                     ((uint32_t)0x00000010)  /* STOPF flag */

/* Slave Transmitter mode -----------------------*/
/* EVT3 */
#define  I2C_EVENT_SLAVE_BYTE_TRANSMITTED                  ((uint32_t)0x00060084)  /* TRA, BUSY, TXE and BTF flags */
#define  I2C_EVENT_SLAVE_BYTE_TRANSMITTING                 ((uint32_t)0x00060080)  /* TRA, BUSY and TXE flags */
/*EVT3_2 */
#define  I2C_EVENT_SLAVE_ACK_FAILURE                       ((uint32_t)0x00000400)  /* AF flag */


/* ch32v00x_iwdg.h -----------------------------------------------------------*/

/* IWDG_WriteAccess */
#define IWDG_WriteAccess_Enable     ((uint16_t)0x5555)
#define IWDG_WriteAccess_Disable    ((uint16_t)0x0000)

/* IWDG_prescaler */
#define IWDG_Prescaler_4            ((uint8_t)0x00)
#define IWDG_Prescaler_8            ((uint8_t)0x01)
#define IWDG_Prescaler_16           ((uint8_t)0x02)
#define IWDG_Prescaler_32           ((uint8_t)0x03)
#define IWDG_Prescaler_64           ((uint8_t)0x04)
#define IWDG_Prescaler_128          ((uint8_t)0x05)
#define IWDG_Prescaler_256          ((uint8_t)0x06)

/* IWDG_Flag */
#define IWDG_FLAG_PVU               ((uint16_t)0x0001)
#define IWDG_FLAG_RVU               ((uint16_t)0x0002)


/* ch32v00x_misc.h -----------------------------------------------------------*/

/* Preemption_Priority_Group */
#define NVIC_PriorityGroup_0           ((uint32_t)0x00)
#define NVIC_PriorityGroup_1           ((uint32_t)0x01)
#define NVIC_PriorityGroup_2           ((uint32_t)0x02)
#define NVIC_PriorityGroup_3           ((uint32_t)0x03)
#define NVIC_PriorityGroup_4           ((uint32_t)0x04)

#if !defined(CH32V10x)
/* ch32v00x_opa.h ------------------------------------------------------------*/

/* Editor's note: I don't know if this is actually useful */
#ifndef __ASSEMBLER__

#if defined(CH32V20x) || defined(CH32V30x)
/* OPA member enumeration */
typedef enum
{
    OPA1 = 0,
    OPA2,
    OPA3,
    OPA4
} OPA_Num_TypeDef;
#endif

/* OPA PSEL enumeration */
typedef enum
{
    CHP0 = 0,
    CHP1
} OPA_PSEL_TypeDef;

/* OPA NSEL enumeration */
typedef enum
{
    CHN0 = 0,
    CHN1
} OPA_NSEL_TypeDef;

#if defined(CH32V20x) || defined(CH32V30x)
/* OPA out channel enumeration */
typedef enum
{
    OUT_IO_OUT0 = 0,
    OUT_IO_OUT1
} OPA_Mode_TypeDef;
#endif


/* OPA Init Structure definition */
#ifdef CH32V003
typedef struct
{
    OPA_PSEL_TypeDef PSEL;    /* Specifies the positive channel of OPA */
    OPA_NSEL_TypeDef NSEL;    /* Specifies the negative channel of OPA */
} OPA_InitTypeDef;
#elif defined(CH32V20x) || defined(CH32V30x)
typedef struct
{
    OPA_Num_TypeDef  OPA_NUM; /* Specifies the members of OPA */
    OPA_PSEL_TypeDef PSEL;    /* Specifies the positive channel of OPA */
    OPA_NSEL_TypeDef NSEL;    /* Specifies the negative channel of OPA */
    OPA_Mode_TypeDef Mode;    /* Specifies the mode of OPA */
} OPA_InitTypeDef;
#endif

#endif

/* ch32v00x_pwr.h ------------------------------------------------------------*/

#endif

#ifdef CH32V003
/* PVD_detection_level  */

#define PWR_PVDLevel_2V9          ((uint32_t)0x00000000)
#define PWR_PVDLevel_3V1          ((uint32_t)0x00000020)
#define PWR_PVDLevel_3V3          ((uint32_t)0x00000040)
#define PWR_PVDLevel_3V5          ((uint32_t)0x00000060)
#define PWR_PVDLevel_3V7          ((uint32_t)0x00000080)
#define PWR_PVDLevel_3V9          ((uint32_t)0x000000A0)
#define PWR_PVDLevel_4V1          ((uint32_t)0x000000C0)
#define PWR_PVDLevel_4V4          ((uint32_t)0x000000E0)

/* PWR_AWU_Prescaler */
#define PWR_AWU_Prescaler_1       ((uint32_t)0x00000000)
#define PWR_AWU_Prescaler_2       ((uint32_t)0x00000002)
#define PWR_AWU_Prescaler_4       ((uint32_t)0x00000003)
#define PWR_AWU_Prescaler_8       ((uint32_t)0x00000004)
#define PWR_AWU_Prescaler_16      ((uint32_t)0x00000005)
#define PWR_AWU_Prescaler_32      ((uint32_t)0x00000006)
#define PWR_AWU_Prescaler_64      ((uint32_t)0x00000007)
#define PWR_AWU_Prescaler_128     ((uint32_t)0x00000008)
#define PWR_AWU_Prescaler_256     ((uint32_t)0x00000009)
#define PWR_AWU_Prescaler_512     ((uint32_t)0x0000000A)
#define PWR_AWU_Prescaler_1024    ((uint32_t)0x0000000B)
#define PWR_AWU_Prescaler_2048    ((uint32_t)0x0000000C)
#define PWR_AWU_Prescaler_4096    ((uint32_t)0x0000000D)
#define PWR_AWU_Prescaler_10240   ((uint32_t)0x0000000E)
#define PWR_AWU_Prescaler_61440   ((uint32_t)0x0000000F)

/* STOP_mode_entry */
#define PWR_STANDBYEntry_WFI      ((uint8_t)0x01)
#define PWR_STANDBYEntry_WFE      ((uint8_t)0x02)

/* PWR_Flag */
#define PWR_FLAG_PVDO             ((uint32_t)0x00000004)

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* PVD_detection_level  */
#define PWR_PVDLevel_2V2          ((uint32_t)0x00000000)
#define PWR_PVDLevel_2V3          ((uint32_t)0x00000020)
#define PWR_PVDLevel_2V4          ((uint32_t)0x00000040)
#define PWR_PVDLevel_2V5          ((uint32_t)0x00000060)
#define PWR_PVDLevel_2V6          ((uint32_t)0x00000080)
#define PWR_PVDLevel_2V7          ((uint32_t)0x000000A0)
#define PWR_PVDLevel_2V8          ((uint32_t)0x000000C0)
#define PWR_PVDLevel_2V9          ((uint32_t)0x000000E0)

/* Regulator_state_is_STOP_mode */
#define PWR_Regulator_ON          ((uint32_t)0x00000000)
#define PWR_Regulator_LowPower    ((uint32_t)0x00000001)

/* STOP_mode_entry */
#define PWR_STOPEntry_WFI         ((uint8_t)0x01)
#define PWR_STOPEntry_WFE         ((uint8_t)0x02)

/* PWR_Flag */
#define PWR_FLAG_WU               ((uint32_t)0x00000001)
#define PWR_FLAG_SB               ((uint32_t)0x00000002)
#define PWR_FLAG_PVDO             ((uint32_t)0x00000004)

#endif


/* ch32v00x_rcc.h ------------------------------------------------------------*/


/* HSE_configuration */
#define RCC_HSE_OFF                      ((uint32_t)0x00000000)
#define RCC_HSE_ON                       ((uint32_t)0x00010000)
#define RCC_HSE_Bypass                   ((uint32_t)0x00040000)

#ifdef CH32V003

/* PLL_entry_clock_source */
#define RCC_PLLSource_HSI_MUL2           ((uint32_t)0x00000000)
#define RCC_PLLSource_HSE_MUL2           ((uint32_t)0x00030000)

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* PLL_entry_clock_source */
#define RCC_PLLSource_HSI_Div2          ((uint32_t)0x00000000)

#if defined(CH32V20x) || defined(CH32V30x_D8) || defined(CH32V10x)

#define RCC_PLLSource_HSE_Div1          ((uint32_t)0x00010000)
#define RCC_PLLSource_HSE_Div2          ((uint32_t)0x00030000)

#else

#define RCC_PLLSource_PREDIV1            ((uint32_t)0x00010000)

#endif

#if defined(CH32V20x) || defined(CH32V30x_D8) || defined(CH32V10x)

/* PLL_multiplication_factor for other CH32V20x  */
#define RCC_PLLMul_2                    ((uint32_t)0x00000000)
#define RCC_PLLMul_3                    ((uint32_t)0x00040000)
#define RCC_PLLMul_4                    ((uint32_t)0x00080000)
#define RCC_PLLMul_5                    ((uint32_t)0x000C0000)
#define RCC_PLLMul_6                    ((uint32_t)0x00100000)
#define RCC_PLLMul_7                    ((uint32_t)0x00140000)
#define RCC_PLLMul_8                    ((uint32_t)0x00180000)
#define RCC_PLLMul_9                    ((uint32_t)0x001C0000)
#define RCC_PLLMul_10                   ((uint32_t)0x00200000)
#define RCC_PLLMul_11                   ((uint32_t)0x00240000)
#define RCC_PLLMul_12                   ((uint32_t)0x00280000)
#define RCC_PLLMul_13                   ((uint32_t)0x002C0000)
#define RCC_PLLMul_14                   ((uint32_t)0x00300000)
#define RCC_PLLMul_15                   ((uint32_t)0x00340000)
#define RCC_PLLMul_16                   ((uint32_t)0x00380000)

#if !defined(CH32V10x)
#define RCC_PLLMul_18                   ((uint32_t)0x003C0000)
#endif

#else

#define RCC_PLLMul_18_EXTEN              ((uint32_t)0x00000000)
#define RCC_PLLMul_3_EXTEN               ((uint32_t)0x00040000)
#define RCC_PLLMul_4_EXTEN               ((uint32_t)0x00080000)
#define RCC_PLLMul_5_EXTEN               ((uint32_t)0x000C0000)
#define RCC_PLLMul_6_EXTEN               ((uint32_t)0x00100000)
#define RCC_PLLMul_7_EXTEN               ((uint32_t)0x00140000)
#define RCC_PLLMul_8_EXTEN               ((uint32_t)0x00180000)
#define RCC_PLLMul_9_EXTEN               ((uint32_t)0x001C0000)
#define RCC_PLLMul_10_EXTEN              ((uint32_t)0x00200000)
#define RCC_PLLMul_11_EXTEN              ((uint32_t)0x00240000)
#define RCC_PLLMul_12_EXTEN              ((uint32_t)0x00280000)
#define RCC_PLLMul_13_EXTEN              ((uint32_t)0x002C0000)
#define RCC_PLLMul_14_EXTEN              ((uint32_t)0x00300000)
#define RCC_PLLMul_6_5_EXTEN             ((uint32_t)0x00340000)
#define RCC_PLLMul_15_EXTEN              ((uint32_t)0x00380000)
#define RCC_PLLMul_16_EXTEN              ((uint32_t)0x003C0000)

#endif // defined(CH32V20x) || defined(CH32V30x_D8) || defined(CH32V10x)

/* PREDIV1_division_factor */
#ifdef CH32V30x_D8C
#define RCC_PREDIV1_Div1                 ((uint32_t)0x00000000)
#define RCC_PREDIV1_Div2                 ((uint32_t)0x00000001)
#define RCC_PREDIV1_Div3                 ((uint32_t)0x00000002)
#define RCC_PREDIV1_Div4                 ((uint32_t)0x00000003)
#define RCC_PREDIV1_Div5                 ((uint32_t)0x00000004)
#define RCC_PREDIV1_Div6                 ((uint32_t)0x00000005)
#define RCC_PREDIV1_Div7                 ((uint32_t)0x00000006)
#define RCC_PREDIV1_Div8                 ((uint32_t)0x00000007)
#define RCC_PREDIV1_Div9                 ((uint32_t)0x00000008)
#define RCC_PREDIV1_Div10                ((uint32_t)0x00000009)
#define RCC_PREDIV1_Div11                ((uint32_t)0x0000000A)
#define RCC_PREDIV1_Div12                ((uint32_t)0x0000000B)
#define RCC_PREDIV1_Div13                ((uint32_t)0x0000000C)
#define RCC_PREDIV1_Div14                ((uint32_t)0x0000000D)
#define RCC_PREDIV1_Div15                ((uint32_t)0x0000000E)
#define RCC_PREDIV1_Div16                ((uint32_t)0x0000000F)

#endif

/* PREDIV1_clock_source */
#ifdef CH32V30x_D8C
#define RCC_PREDIV1_Source_HSE           ((uint32_t)0x00000000)
#define RCC_PREDIV1_Source_PLL2          ((uint32_t)0x00010000)
#endif

/* PREDIV2_division_factor */
#ifdef CH32V30x_D8C
#define RCC_PREDIV2_Div1                 ((uint32_t)0x00000000)
#define RCC_PREDIV2_Div2                 ((uint32_t)0x00000010)
#define RCC_PREDIV2_Div3                 ((uint32_t)0x00000020)
#define RCC_PREDIV2_Div4                 ((uint32_t)0x00000030)
#define RCC_PREDIV2_Div5                 ((uint32_t)0x00000040)
#define RCC_PREDIV2_Div6                 ((uint32_t)0x00000050)
#define RCC_PREDIV2_Div7                 ((uint32_t)0x00000060)
#define RCC_PREDIV2_Div8                 ((uint32_t)0x00000070)
#define RCC_PREDIV2_Div9                 ((uint32_t)0x00000080)
#define RCC_PREDIV2_Div10                ((uint32_t)0x00000090)
#define RCC_PREDIV2_Div11                ((uint32_t)0x000000A0)
#define RCC_PREDIV2_Div12                ((uint32_t)0x000000B0)
#define RCC_PREDIV2_Div13                ((uint32_t)0x000000C0)
#define RCC_PREDIV2_Div14                ((uint32_t)0x000000D0)
#define RCC_PREDIV2_Div15                ((uint32_t)0x000000E0)
#define RCC_PREDIV2_Div16                ((uint32_t)0x000000F0)
#endif

/* PLL2_multiplication_factor */
#ifdef CH32V30x_D8C
#define RCC_PLL2Mul_2_5                  ((uint32_t)0x00000000)
#define RCC_PLL2Mul_12_5                 ((uint32_t)0x00000100)
#define RCC_PLL2Mul_4                    ((uint32_t)0x00000200)
#define RCC_PLL2Mul_5                    ((uint32_t)0x00000300)
#define RCC_PLL2Mul_6                    ((uint32_t)0x00000400)
#define RCC_PLL2Mul_7                    ((uint32_t)0x00000500)
#define RCC_PLL2Mul_8                    ((uint32_t)0x00000600)
#define RCC_PLL2Mul_9                    ((uint32_t)0x00000700)
#define RCC_PLL2Mul_10                   ((uint32_t)0x00000800)
#define RCC_PLL2Mul_11                   ((uint32_t)0x00000900)
#define RCC_PLL2Mul_12                   ((uint32_t)0x00000A00)
#define RCC_PLL2Mul_13                   ((uint32_t)0x00000B00)
#define RCC_PLL2Mul_14                   ((uint32_t)0x00000C00)
#define RCC_PLL2Mul_15                   ((uint32_t)0x00000D00)
#define RCC_PLL2Mul_16                   ((uint32_t)0x00000E00)
#define RCC_PLL2Mul_20                   ((uint32_t)0x00000F00)
#endif

/* PLL3_multiplication_factor */
#ifdef CH32V30x_D8C
#define RCC_PLL3Mul_2_5                  ((uint32_t)0x00000000)
#define RCC_PLL3Mul_12_5                 ((uint32_t)0x00001000)
#define RCC_PLL3Mul_4                    ((uint32_t)0x00002000)
#define RCC_PLL3Mul_5                    ((uint32_t)0x00003000)
#define RCC_PLL3Mul_6                    ((uint32_t)0x00004000)
#define RCC_PLL3Mul_7                    ((uint32_t)0x00005000)
#define RCC_PLL3Mul_8                    ((uint32_t)0x00006000)
#define RCC_PLL3Mul_9                    ((uint32_t)0x00007000)
#define RCC_PLL3Mul_10                   ((uint32_t)0x00008000)
#define RCC_PLL3Mul_11                   ((uint32_t)0x00009000)
#define RCC_PLL3Mul_12                   ((uint32_t)0x0000A000)
#define RCC_PLL3Mul_13                   ((uint32_t)0x0000B000)
#define RCC_PLL3Mul_14                   ((uint32_t)0x0000C000)
#define RCC_PLL3Mul_15                   ((uint32_t)0x0000D000)
#define RCC_PLL3Mul_16                   ((uint32_t)0x0000E000)
#define RCC_PLL3Mul_20                   ((uint32_t)0x0000F000)
#endif

#endif

/* System_clock_source */
#define RCC_SYSCLKSource_HSI             ((uint32_t)0x00000000)
#define RCC_SYSCLKSource_HSE             ((uint32_t)0x00000001)
#define RCC_SYSCLKSource_PLLCLK          ((uint32_t)0x00000002)

#ifdef CH32V003

/* AHB_clock_source */
#define RCC_SYSCLK_Div1                  ((uint32_t)0x00000000)
#define RCC_SYSCLK_Div2                  ((uint32_t)0x00000010)
#define RCC_SYSCLK_Div3                  ((uint32_t)0x00000020)
#define RCC_SYSCLK_Div4                  ((uint32_t)0x00000030)
#define RCC_SYSCLK_Div5                  ((uint32_t)0x00000040)
#define RCC_SYSCLK_Div6                  ((uint32_t)0x00000050)
#define RCC_SYSCLK_Div7                  ((uint32_t)0x00000060)
#define RCC_SYSCLK_Div8                  ((uint32_t)0x00000070)
#define RCC_SYSCLK_Div16                 ((uint32_t)0x000000B0)
#define RCC_SYSCLK_Div32                 ((uint32_t)0x000000C0)
#define RCC_SYSCLK_Div64                 ((uint32_t)0x000000D0)
#define RCC_SYSCLK_Div128                ((uint32_t)0x000000E0)
#define RCC_SYSCLK_Div256                ((uint32_t)0x000000F0)

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* AHB_clock_source */
#define RCC_SYSCLK_Div1                 ((uint32_t)0x00000000)
#define RCC_SYSCLK_Div2                 ((uint32_t)0x00000080)
#define RCC_SYSCLK_Div4                 ((uint32_t)0x00000090)
#define RCC_SYSCLK_Div8                 ((uint32_t)0x000000A0)
#define RCC_SYSCLK_Div16                ((uint32_t)0x000000B0)
#define RCC_SYSCLK_Div64                ((uint32_t)0x000000C0)
#define RCC_SYSCLK_Div128               ((uint32_t)0x000000D0)
#define RCC_SYSCLK_Div256               ((uint32_t)0x000000E0)
#define RCC_SYSCLK_Div512               ((uint32_t)0x000000F0)

/* APB1_APB2_clock_source */
#define RCC_HCLK_Div1                   ((uint32_t)0x00000000)
#define RCC_HCLK_Div2                   ((uint32_t)0x00000400)
#define RCC_HCLK_Div4                   ((uint32_t)0x00000500)
#define RCC_HCLK_Div8                   ((uint32_t)0x00000600)
#define RCC_HCLK_Div16                  ((uint32_t)0x00000700)

#endif

/* RCC_Interrupt_source */
#define RCC_IT_LSIRDY                    ((uint8_t)0x01)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RCC_IT_LSERDY                   ((uint8_t)0x02)
#endif
#define RCC_IT_HSIRDY                    ((uint8_t)0x04)
#define RCC_IT_HSERDY                    ((uint8_t)0x08)
#define RCC_IT_PLLRDY                    ((uint8_t)0x10)
#define RCC_IT_CSS                       ((uint8_t)0x80)

#ifdef CH32V30x_D8C
#define RCC_IT_PLL2RDY                   ((uint8_t)0x20)
#define RCC_IT_PLL3RDY                   ((uint8_t)0x40)
#endif

#if defined(CH32V20x)

/* USB_Device_clock_source */
#define RCC_USBCLKSource_PLLCLK_Div1    ((uint8_t)0x00)
#define RCC_USBCLKSource_PLLCLK_Div2    ((uint8_t)0x01)
#define RCC_USBCLKSource_PLLCLK_Div3    ((uint8_t)0x02)

#ifdef CH32V20x_D8W
  #define RCC_USBCLKSource_PLLCLK_Div5    ((uint8_t)0x03)
#endif

#endif

#if defined(CH32V10x)
/* USB_Device_clock_source */
#define RCC_USBCLKSource_PLLCLK_1Div5    ((uint8_t)0x00)
#define RCC_USBCLKSource_PLLCLK_Div1     ((uint8_t)0x01)
#endif

#if defined(CH32V30x)

/* USB_OTG_FS_clock_source */
#define RCC_OTGFSCLKSource_PLLCLK_Div1   ((uint8_t)0x00)
#define RCC_OTGFSCLKSource_PLLCLK_Div2   ((uint8_t)0x01)
#define RCC_OTGFSCLKSource_PLLCLK_Div3   ((uint8_t)0x02)

/* I2S2_clock_source */
#ifdef CH32V30x_D8C
#define RCC_I2S2CLKSource_SYSCLK         ((uint8_t)0x00)
#define RCC_I2S2CLKSource_PLL3_VCO       ((uint8_t)0x01)
#endif

/* I2S3_clock_source */
#ifdef CH32V30x_D8C
#define RCC_I2S3CLKSource_SYSCLK         ((uint8_t)0x00)
#define RCC_I2S3CLKSource_PLL3_VCO       ((uint8_t)0x01)
#endif

#endif

#ifdef CH32V003

/* ADC_clock_source */
#define RCC_PCLK2_Div2                   ((uint32_t)0x00000000)
#define RCC_PCLK2_Div4                   ((uint32_t)0x00004000)
#define RCC_PCLK2_Div6                   ((uint32_t)0x00008000)
#define RCC_PCLK2_Div8                   ((uint32_t)0x0000C000)
#define RCC_PCLK2_Div12                  ((uint32_t)0x0000A000)
#define RCC_PCLK2_Div16                  ((uint32_t)0x0000E000)
#define RCC_PCLK2_Div24                  ((uint32_t)0x0000A800)
#define RCC_PCLK2_Div32                  ((uint32_t)0x0000E800)
#define RCC_PCLK2_Div48                  ((uint32_t)0x0000B000)
#define RCC_PCLK2_Div64                  ((uint32_t)0x0000F000)
#define RCC_PCLK2_Div96                  ((uint32_t)0x0000B800)
#define RCC_PCLK2_Div128                 ((uint32_t)0x0000F800)

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* ADC_clock_source */
#define RCC_PCLK2_Div2                 ((uint32_t)0x00000000)
#define RCC_PCLK2_Div4                 ((uint32_t)0x00004000)
#define RCC_PCLK2_Div6                 ((uint32_t)0x00008000)
#define RCC_PCLK2_Div8                 ((uint32_t)0x0000C000)

/* LSE_configuration */
#define RCC_LSE_OFF                    ((uint8_t)0x00)
#define RCC_LSE_ON                     ((uint8_t)0x01)
#define RCC_LSE_Bypass                 ((uint8_t)0x04)

/* RTC_clock_source */
#define RCC_RTCCLKSource_LSE           ((uint32_t)0x00000100)
#define RCC_RTCCLKSource_LSI           ((uint32_t)0x00000200)
#define RCC_RTCCLKSource_HSE_Div128    ((uint32_t)0x00000300)

#endif

#if defined(CH32V003) || defined(CH32X03x)

/* AHB_peripheral */
#define RCC_AHBPeriph_DMA1               ((uint32_t)0x00000001)
#define RCC_AHBPeriph_SRAM               ((uint32_t)0x00000004)

/* APB2_peripheral */
#define RCC_APB2Periph_AFIO              ((uint32_t)0x00000001)
#define RCC_APB2Periph_GPIOA             ((uint32_t)0x00000004)
#ifdef CH32X03x
#define RCC_APB2Periph_GPIOB             ((uint32_t)0x00000008)
#endif
#define RCC_APB2Periph_GPIOC             ((uint32_t)0x00000010)
#define RCC_APB2Periph_GPIOD             ((uint32_t)0x00000020)
#define RCC_APB2Periph_ADC1              ((uint32_t)0x00000200)
#define RCC_APB2Periph_TIM1              ((uint32_t)0x00000800)
#define RCC_APB2Periph_SPI1              ((uint32_t)0x00001000)
#define RCC_APB2Periph_USART1            ((uint32_t)0x00004000)

/* APB1_peripheral */
#define RCC_APB1Periph_TIM2              ((uint32_t)0x00000001)
#define RCC_APB1Periph_WWDG              ((uint32_t)0x00000800)
#define RCC_APB1Periph_I2C1              ((uint32_t)0x00200000)
#define RCC_APB1Periph_PWR               ((uint32_t)0x10000000)

#if defined(CH32X03x)

/* APB2_peripheral */
#define RCC_APB2Periph_GPIOB             ((uint32_t)0x00000008)

#define RCC_APB1Periph_USART2            ((uint32_t)0x00020000)
#define RCC_APB1Periph_USART3            ((uint32_t)0x00040000)
#define RCC_APB1Periph_UART4             ((uint32_t)0x00080000)

#endif

#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* AHB_peripheral */
#define RCC_AHBPeriph_DMA1             ((uint32_t)0x00000001)
#define RCC_AHBPeriph_DMA2             ((uint32_t)0x00000002)
#define RCC_AHBPeriph_SRAM             ((uint32_t)0x00000004)

#if defined(CH32V10x)
#define RCC_AHBPeriph_FLITF            ((uint32_t)0x00000010)
#endif

#define RCC_AHBPeriph_CRC              ((uint32_t)0x00000040)
#define RCC_AHBPeriph_FSMC             ((uint32_t)0x00000100)

#if !defined(CH32V10x)
#define RCC_AHBPeriph_RNG              ((uint32_t)0x00000200)
#endif

#define RCC_AHBPeriph_SDIO             ((uint32_t)0x00000400)

#if !defined(CH32V10x)
#define RCC_AHBPeriph_USBHS            ((uint32_t)0x00000800)
#define RCC_AHBPeriph_OTG_FS           ((uint32_t)0x00001000)
#else
#define RCC_AHBPeriph_USBHD            ((uint32_t)0x00001000)
#endif

#if defined(CH32V30x)
#define RCC_AHBPeriph_DVP                ((uint32_t)0x00002000)
#define RCC_AHBPeriph_ETH_MAC            ((uint32_t)0x00004000)
#define RCC_AHBPeriph_ETH_MAC_Tx         ((uint32_t)0x00008000)
#define RCC_AHBPeriph_ETH_MAC_Rx         ((uint32_t)0x00010000)
#endif

#ifdef CH32V20x_D8W
#define RCC_AHBPeriph_BLE_CRC          ((uint32_t)0x00030040)
#endif

/* APB2_peripheral */
#define RCC_APB2Periph_AFIO            ((uint32_t)0x00000001)
#define RCC_APB2Periph_GPIOA           ((uint32_t)0x00000004)
#define RCC_APB2Periph_GPIOB           ((uint32_t)0x00000008)
#define RCC_APB2Periph_GPIOC           ((uint32_t)0x00000010)
#define RCC_APB2Periph_GPIOD           ((uint32_t)0x00000020)
#define RCC_APB2Periph_GPIOE           ((uint32_t)0x00000040)
#define RCC_APB2Periph_ADC1            ((uint32_t)0x00000200)
#define RCC_APB2Periph_ADC2            ((uint32_t)0x00000400)
#define RCC_APB2Periph_TIM1            ((uint32_t)0x00000800)
#define RCC_APB2Periph_SPI1            ((uint32_t)0x00001000)
#define RCC_APB2Periph_TIM8            ((uint32_t)0x00002000)
#define RCC_APB2Periph_USART1          ((uint32_t)0x00004000)
#if !defined(CH32V10x)
#define RCC_APB2Periph_TIM9            ((uint32_t)0x00080000)
#define RCC_APB2Periph_TIM10           ((uint32_t)0x00100000)
#else
#define RCC_APB2Periph_ADC3              ((uint32_t)0x00008000)
#define RCC_APB2Periph_TIM15             ((uint32_t)0x00010000)
#define RCC_APB2Periph_TIM16             ((uint32_t)0x00020000)
#define RCC_APB2Periph_TIM17             ((uint32_t)0x00040000)
#define RCC_APB2Periph_TIM9              ((uint32_t)0x00080000)
#define RCC_APB2Periph_TIM10             ((uint32_t)0x00100000)
#define RCC_APB2Periph_TIM11             ((uint32_t)0x00200000)
#endif

/* APB1_peripheral */
#define RCC_APB1Periph_TIM2            ((uint32_t)0x00000001)
#define RCC_APB1Periph_TIM3            ((uint32_t)0x00000002)
#define RCC_APB1Periph_TIM4            ((uint32_t)0x00000004)
#define RCC_APB1Periph_TIM5            ((uint32_t)0x00000008)
#define RCC_APB1Periph_TIM6            ((uint32_t)0x00000010)
#define RCC_APB1Periph_TIM7            ((uint32_t)0x00000020)
#if !defined(CH32V10x)
#define RCC_APB1Periph_UART6           ((uint32_t)0x00000040)
#define RCC_APB1Periph_UART7           ((uint32_t)0x00000080)
#define RCC_APB1Periph_UART8           ((uint32_t)0x00000100)
#else
#define RCC_APB1Periph_TIM12             ((uint32_t)0x00000040)
#define RCC_APB1Periph_TIM13             ((uint32_t)0x00000080)
#define RCC_APB1Periph_TIM14             ((uint32_t)0x00000100)
#endif
#define RCC_APB1Periph_WWDG            ((uint32_t)0x00000800)
#define RCC_APB1Periph_SPI2            ((uint32_t)0x00004000)
#define RCC_APB1Periph_SPI3            ((uint32_t)0x00008000)
#define RCC_APB1Periph_USART2          ((uint32_t)0x00020000)
#define RCC_APB1Periph_USART3          ((uint32_t)0x00040000)
#define RCC_APB1Periph_UART4           ((uint32_t)0x00080000)
#define RCC_APB1Periph_UART5           ((uint32_t)0x00100000)
#define RCC_APB1Periph_I2C1            ((uint32_t)0x00200000)
#define RCC_APB1Periph_I2C2            ((uint32_t)0x00400000)
#define RCC_APB1Periph_USB             ((uint32_t)0x00800000)
#define RCC_APB1Periph_CAN1            ((uint32_t)0x02000000)
#define RCC_APB1Periph_CAN2            ((uint32_t)0x04000000)
#define RCC_APB1Periph_BKP             ((uint32_t)0x08000000)
#define RCC_APB1Periph_PWR             ((uint32_t)0x10000000)
#define RCC_APB1Periph_DAC             ((uint32_t)0x20000000)
#if defined(CH32V10x)
#define RCC_APB1Periph_CEC               ((uint32_t)0x40000000)
#endif

#endif

/* Clock_source_to_output_on_MCO_pin */
#define RCC_MCO_NoClock                  ((uint8_t)0x00)
#define RCC_MCO_SYSCLK                   ((uint8_t)0x04)
#define RCC_MCO_HSI                      ((uint8_t)0x05)
#define RCC_MCO_HSE                      ((uint8_t)0x06)
#ifdef CH32V003
#define RCC_MCO_PLLCLK                   ((uint8_t)0x07)
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RCC_MCO_PLLCLK_Div2            	 ((uint8_t)0x07)
#endif

#ifdef CH32V30x_D8C
#define RCC_MCO_PLL2CLK                  ((uint8_t)0x08)
#define RCC_MCO_PLL3CLK_Div2             ((uint8_t)0x09)
#define RCC_MCO_XT1                      ((uint8_t)0x0A)
#define RCC_MCO_PLL3CLK                  ((uint8_t)0x0B)
#endif

/* RCC_Flag */
#define RCC_FLAG_HSIRDY                  ((uint8_t)0x21)
#define RCC_FLAG_HSERDY                  ((uint8_t)0x31)
#define RCC_FLAG_PLLRDY                  ((uint8_t)0x39)
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define RCC_FLAG_LSERDY                	 ((uint8_t)0x41)
#endif
#define RCC_FLAG_LSIRDY                  ((uint8_t)0x61)
#define RCC_FLAG_PINRST                  ((uint8_t)0x7A)
#define RCC_FLAG_PORRST                  ((uint8_t)0x7B)
#define RCC_FLAG_SFTRST                  ((uint8_t)0x7C)
#define RCC_FLAG_IWDGRST                 ((uint8_t)0x7D)
#define RCC_FLAG_WWDGRST                 ((uint8_t)0x7E)
#define RCC_FLAG_LPWRRST                 ((uint8_t)0x7F)

#ifdef CH32V30x_D8C
#define RCC_FLAG_PLL2RDY                 ((uint8_t)0x3B)
#define RCC_FLAG_PLL3RDY                 ((uint8_t)0x3D)
#endif

/* SysTick_clock_source */
#define SysTick_CLKSource_HCLK_Div8      ((uint32_t)0xFFFFFFFB)
#define SysTick_CLKSource_HCLK           ((uint32_t)0x00000004)

/* RNG_clock_source */
#ifdef CH32V30x_D8C
#define RCC_RNGCLKSource_SYSCLK          ((uint32_t)0x00)
#define RCC_RNGCLKSource_PLL3_VCO        ((uint32_t)0x01)
#endif

/* ETH1G_clock_source */
#ifdef CH32V30x_D8C
#define RCC_ETH1GCLKSource_PLL2_VCO      ((uint32_t)0x00)
#define RCC_ETH1GCLKSource_PLL3_VCO      ((uint32_t)0x01)
#define RCC_ETH1GCLKSource_PB1_IN        ((uint32_t)0x02)
#endif

#if defined(CH32V20x)

/* USBFS_clock_source */
#define RCC_USBPLL_Div1                ((uint32_t)0x00)
#define RCC_USBPLL_Div2                ((uint32_t)0x01)
#define RCC_USBPLL_Div3                ((uint32_t)0x02)
#define RCC_USBPLL_Div4                ((uint32_t)0x03)
#define RCC_USBPLL_Div5                ((uint32_t)0x04)
#define RCC_USBPLL_Div6                ((uint32_t)0x05)
#define RCC_USBPLL_Div7                ((uint32_t)0x06)
#define RCC_USBPLL_Div8                ((uint32_t)0x07)

/* ETH_clock_source */
#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
  #define RCC_ETHCLK_Div1    ((uint32_t)0x00)
  #define RCC_ETHCLK_Div2    ((uint32_t)0x01)
#endif

#endif // defined(CH32V20x)

#if defined(CH32V30x)

/* USBFS_clock_source */
#ifdef CH32V30x_D8C
#define RCC_USBPLL_Div1                  ((uint32_t)0x00)
#define RCC_USBPLL_Div2                  ((uint32_t)0x01)
#define RCC_USBPLL_Div3                  ((uint32_t)0x02)
#define RCC_USBPLL_Div4                  ((uint32_t)0x03)
#define RCC_USBPLL_Div5                  ((uint32_t)0x04)
#define RCC_USBPLL_Div6                  ((uint32_t)0x05)
#define RCC_USBPLL_Div7                  ((uint32_t)0x06)
#define RCC_USBPLL_Div8                  ((uint32_t)0x07)

#endif

/* USBHSPLL_clock_source */
#ifdef CH32V30x_D8C
#define RCC_HSBHSPLLCLKSource_HSE        ((uint32_t)0x00)
#define RCC_HSBHSPLLCLKSource_HSI        ((uint32_t)0x01)

#endif

/* USBHSPLLCKREF_clock_select */
#ifdef CH32V30x_D8C
#define RCC_USBHSPLLCKREFCLK_3M          ((uint32_t)0x00)
#define RCC_USBHSPLLCKREFCLK_4M          ((uint32_t)0x01)
#define RCC_USBHSPLLCKREFCLK_8M          ((uint32_t)0x02)
#define RCC_USBHSPLLCKREFCLK_5M          ((uint32_t)0x03)

#endif

/* OTGUSBCLK48M_clock_source */
#define RCC_USBCLK48MCLKSource_PLLCLK    ((uint32_t)0x00)
#define RCC_USBCLK48MCLKSource_USBPHY    ((uint32_t)0x01)

#endif

#if defined(CH32V30x)

/* ch32v00x_rng.h ------------------------------------------------------------*/

/* RNG_flags_definition*/
#define RNG_FLAG_DRDY               ((uint8_t)0x0001) /* Data ready */
#define RNG_FLAG_CECS               ((uint8_t)0x0002) /* Clock error current status */
#define RNG_FLAG_SECS               ((uint8_t)0x0004) /* Seed error current status */

/* RNG_interrupts_definition */
#define RNG_IT_CEI                  ((uint8_t)0x20) /* Clock error interrupt */
#define RNG_IT_SEI                  ((uint8_t)0x40) /* Seed error interrupt */

#endif

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/* ch32v00x_rtc.h ------------------------------------------------------------*/
/* RTC_interrupts_define */
#define RTC_IT_OW         ((uint16_t)0x0004) /* Overflow interrupt */
#define RTC_IT_ALR        ((uint16_t)0x0002) /* Alarm interrupt */
#define RTC_IT_SEC        ((uint16_t)0x0001) /* Second interrupt */

/* RTC_interrupts_flags */
#define RTC_FLAG_RTOFF    ((uint16_t)0x0020) /* RTC Operation OFF flag */
#define RTC_FLAG_RSF      ((uint16_t)0x0008) /* Registers Synchronized flag */
#define RTC_FLAG_OW       ((uint16_t)0x0004) /* Overflow flag */
#define RTC_FLAG_ALR      ((uint16_t)0x0002) /* Alarm flag */
#define RTC_FLAG_SEC      ((uint16_t)0x0001) /* Second flag */

#if defined(CH32V20x_D8) || defined(CH32V20x_D8W)
#define RB_OSC32K_HTUNE       (0x1FE0)
#define RB_OSC32K_LTUNE       (0x1F)

#define RB_OSC_CAL_HALT       (0x80)
#define RB_OSC_CAL_EN         (0x02)
#define RB_OSC_CAL_INT_EN     (0x01)

#define RB_OSC_CAL_OV_CNT     (0xFF)

#define RB_OSC_CAL_IF_END     (1 << 15)
#define RB_OSC_CAL_CNT_OV     (1 << 14)
#define RB_OSC_CAL_CNT        (0x3FFF)

#define RB_CAL_LP_EN          (1 << 6)
#define RB_CAL_WKUP_EN        (1 << 5)
#define RB_OSC_HALT_MD        (1 << 4)
#define RB_OSC_CNT_VLU        (0x0F)


#ifdef CLK_OSC32K
#if ( CLK_OSC32K == 1 )
#define CAB_LSIFQ       32000
#else
#define CAB_LSIFQ       32768
#endif
#else
#define CAB_LSIFQ       32000
#endif
#endif // defined(CH32V20x_D8) || defined(CH32V20x_D8W)

#endif // defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

#if defined(CH32V30x)
/* ch32v00x_sdio.h -----------------------------------------------------------*/

/* SDIO_Clock_Edge */
#define SDIO_ClockEdge_Rising               ((uint32_t)0x00000000)
#define SDIO_ClockEdge_Falling              ((uint32_t)0x00002000)

/* SDIO_Clock_Bypass */
#define SDIO_ClockBypass_Disable             ((uint32_t)0x00000000)
#define SDIO_ClockBypass_Enable              ((uint32_t)0x00000400)

/* SDIO_Clock_Power_Save */
#define SDIO_ClockPowerSave_Disable         ((uint32_t)0x00000000)
#define SDIO_ClockPowerSave_Enable          ((uint32_t)0x00000200)

/* SDIO_Bus_Wide */
#define SDIO_BusWide_1b                     ((uint32_t)0x00000000)
#define SDIO_BusWide_4b                     ((uint32_t)0x00000800)
#define SDIO_BusWide_8b                     ((uint32_t)0x00001000)

/* SDIO_Hardware_Flow_Control */
#define SDIO_HardwareFlowControl_Disable    ((uint32_t)0x00000000)
#define SDIO_HardwareFlowControl_Enable     ((uint32_t)0x00004000)

/* SDIO_Power_State */
#define SDIO_PowerState_OFF                 ((uint32_t)0x00000000)
#define SDIO_PowerState_ON                  ((uint32_t)0x00000003)

/* SDIO_Interrupt_sources */
#define SDIO_IT_CCRCFAIL                    ((uint32_t)0x00000001)
#define SDIO_IT_DCRCFAIL                    ((uint32_t)0x00000002)
#define SDIO_IT_CTIMEOUT                    ((uint32_t)0x00000004)
#define SDIO_IT_DTIMEOUT                    ((uint32_t)0x00000008)
#define SDIO_IT_TXUNDERR                    ((uint32_t)0x00000010)
#define SDIO_IT_RXOVERR                     ((uint32_t)0x00000020)
#define SDIO_IT_CMDREND                     ((uint32_t)0x00000040)
#define SDIO_IT_CMDSENT                     ((uint32_t)0x00000080)
#define SDIO_IT_DATAEND                     ((uint32_t)0x00000100)
#define SDIO_IT_STBITERR                    ((uint32_t)0x00000200)
#define SDIO_IT_DBCKEND                     ((uint32_t)0x00000400)
#define SDIO_IT_CMDACT                      ((uint32_t)0x00000800)
#define SDIO_IT_TXACT                       ((uint32_t)0x00001000)
#define SDIO_IT_RXACT                       ((uint32_t)0x00002000)
#define SDIO_IT_TXFIFOHE                    ((uint32_t)0x00004000)
#define SDIO_IT_RXFIFOHF                    ((uint32_t)0x00008000)
#define SDIO_IT_TXFIFOF                     ((uint32_t)0x00010000)
#define SDIO_IT_RXFIFOF                     ((uint32_t)0x00020000)
#define SDIO_IT_TXFIFOE                     ((uint32_t)0x00040000)
#define SDIO_IT_RXFIFOE                     ((uint32_t)0x00080000)
#define SDIO_IT_TXDAVL                      ((uint32_t)0x00100000)
#define SDIO_IT_RXDAVL                      ((uint32_t)0x00200000)
#define SDIO_IT_SDIOIT                      ((uint32_t)0x00400000)
#define SDIO_IT_CEATAEND                    ((uint32_t)0x00800000)

/* SDIO_Response_Type */
#define SDIO_Response_No                    ((uint32_t)0x00000000)
#define SDIO_Response_Short                 ((uint32_t)0x00000040)
#define SDIO_Response_Long                  ((uint32_t)0x000000C0)

/* SDIO_Wait_Interrupt_State */
#define SDIO_Wait_No                        ((uint32_t)0x00000000)
#define SDIO_Wait_IT                        ((uint32_t)0x00000100)
#define SDIO_Wait_Pend                      ((uint32_t)0x00000200)

/* SDIO_CPSM_State */
#define SDIO_CPSM_Disable                    ((uint32_t)0x00000000)
#define SDIO_CPSM_Enable                     ((uint32_t)0x00000400)

/* SDIO_Response_Registers */
#define SDIO_RESP1                          ((uint32_t)0x00000000)
#define SDIO_RESP2                          ((uint32_t)0x00000004)
#define SDIO_RESP3                          ((uint32_t)0x00000008)
#define SDIO_RESP4                          ((uint32_t)0x0000000C)

/* SDIO_Data_Block_Size */
#define SDIO_DataBlockSize_1b               ((uint32_t)0x00000000)
#define SDIO_DataBlockSize_2b               ((uint32_t)0x00000010)
#define SDIO_DataBlockSize_4b               ((uint32_t)0x00000020)
#define SDIO_DataBlockSize_8b               ((uint32_t)0x00000030)
#define SDIO_DataBlockSize_16b              ((uint32_t)0x00000040)
#define SDIO_DataBlockSize_32b              ((uint32_t)0x00000050)
#define SDIO_DataBlockSize_64b              ((uint32_t)0x00000060)
#define SDIO_DataBlockSize_128b             ((uint32_t)0x00000070)
#define SDIO_DataBlockSize_256b             ((uint32_t)0x00000080)
#define SDIO_DataBlockSize_512b             ((uint32_t)0x00000090)
#define SDIO_DataBlockSize_1024b            ((uint32_t)0x000000A0)
#define SDIO_DataBlockSize_2048b            ((uint32_t)0x000000B0)
#define SDIO_DataBlockSize_4096b            ((uint32_t)0x000000C0)
#define SDIO_DataBlockSize_8192b            ((uint32_t)0x000000D0)
#define SDIO_DataBlockSize_16384b           ((uint32_t)0x000000E0)

/* SDIO_Transfer_Direction */
#define SDIO_TransferDir_ToCard             ((uint32_t)0x00000000)
#define SDIO_TransferDir_ToSDIO             ((uint32_t)0x00000002)

/* SDIO_Transfer_Type */
#define SDIO_TransferMode_Block             ((uint32_t)0x00000000)
#define SDIO_TransferMode_Stream            ((uint32_t)0x00000004)

/* SDIO_DPSM_State */
#define SDIO_DPSM_Disable                    ((uint32_t)0x00000000)
#define SDIO_DPSM_Enable                     ((uint32_t)0x00000001)

/* SDIO_Flags */
#define SDIO_FLAG_CCRCFAIL                  ((uint32_t)0x00000001)
#define SDIO_FLAG_DCRCFAIL                  ((uint32_t)0x00000002)
#define SDIO_FLAG_CTIMEOUT                  ((uint32_t)0x00000004)
#define SDIO_FLAG_DTIMEOUT                  ((uint32_t)0x00000008)
#define SDIO_FLAG_TXUNDERR                  ((uint32_t)0x00000010)
#define SDIO_FLAG_RXOVERR                   ((uint32_t)0x00000020)
#define SDIO_FLAG_CMDREND                   ((uint32_t)0x00000040)
#define SDIO_FLAG_CMDSENT                   ((uint32_t)0x00000080)
#define SDIO_FLAG_DATAEND                   ((uint32_t)0x00000100)
#define SDIO_FLAG_STBITERR                  ((uint32_t)0x00000200)
#define SDIO_FLAG_DBCKEND                   ((uint32_t)0x00000400)
#define SDIO_FLAG_CMDACT                    ((uint32_t)0x00000800)
#define SDIO_FLAG_TXACT                     ((uint32_t)0x00001000)
#define SDIO_FLAG_RXACT                     ((uint32_t)0x00002000)
#define SDIO_FLAG_TXFIFOHE                  ((uint32_t)0x00004000)
#define SDIO_FLAG_RXFIFOHF                  ((uint32_t)0x00008000)
#define SDIO_FLAG_TXFIFOF                   ((uint32_t)0x00010000)
#define SDIO_FLAG_RXFIFOF                   ((uint32_t)0x00020000)
#define SDIO_FLAG_TXFIFOE                   ((uint32_t)0x00040000)
#define SDIO_FLAG_RXFIFOE                   ((uint32_t)0x00080000)
#define SDIO_FLAG_TXDAVL                    ((uint32_t)0x00100000)
#define SDIO_FLAG_RXDAVL                    ((uint32_t)0x00200000)
#define SDIO_FLAG_SDIOIT                    ((uint32_t)0x00400000)
#define SDIO_FLAG_CEATAEND                  ((uint32_t)0x00800000)

/* SDIO_Read_Wait_Mode */
#define SDIO_ReadWaitMode_CLK               ((uint32_t)0x00000001)
#define SDIO_ReadWaitMode_DATA2             ((uint32_t)0x00000000)

#endif

/* ch32v00x_spi.h ------------------------------------------------------------*/


/* SPI_data_direction */
#define SPI_Direction_2Lines_FullDuplex    ((uint16_t)0x0000)
#define SPI_Direction_2Lines_RxOnly        ((uint16_t)0x0400)
#define SPI_Direction_1Line_Rx             ((uint16_t)0x8000)
#define SPI_Direction_1Line_Tx             ((uint16_t)0xC000)

/* SPI_mode */
#define SPI_Mode_Master                    ((uint16_t)0x0104)
#define SPI_Mode_Slave                     ((uint16_t)0x0000)

/* SPI_data_size */
#define SPI_DataSize_16b                   ((uint16_t)0x0800)
#define SPI_DataSize_8b                    ((uint16_t)0x0000)

/* SPI_Clock_Polarity */
#define SPI_CPOL_Low                       ((uint16_t)0x0000)
#define SPI_CPOL_High                      ((uint16_t)0x0002)

/* SPI_Clock_Phase */
#define SPI_CPHA_1Edge                     ((uint16_t)0x0000)
#define SPI_CPHA_2Edge                     ((uint16_t)0x0001)

/* SPI_Slave_Select_management */
#define SPI_NSS_Soft                       ((uint16_t)0x0200)
#define SPI_NSS_Hard                       ((uint16_t)0x0000)

/* SPI_BaudRate_Prescaler */
#define SPI_BaudRatePrescaler_2            ((uint16_t)0x0000)
#define SPI_BaudRatePrescaler_4            ((uint16_t)0x0008)
#define SPI_BaudRatePrescaler_8            ((uint16_t)0x0010)
#define SPI_BaudRatePrescaler_16           ((uint16_t)0x0018)
#define SPI_BaudRatePrescaler_32           ((uint16_t)0x0020)
#define SPI_BaudRatePrescaler_64           ((uint16_t)0x0028)
#define SPI_BaudRatePrescaler_128          ((uint16_t)0x0030)
#define SPI_BaudRatePrescaler_256          ((uint16_t)0x0038)

/* SPI_MSB transmission */
#define SPI_FirstBit_MSB                   ((uint16_t)0x0000)

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define SPI_FirstBit_LSB                   ((uint16_t)0x0080)

/* I2S_Mode */
#define I2S_Mode_SlaveTx                   ((uint16_t)0x0000)
#define I2S_Mode_SlaveRx                   ((uint16_t)0x0100)
#define I2S_Mode_MasterTx                  ((uint16_t)0x0200)
#define I2S_Mode_MasterRx                  ((uint16_t)0x0300)

/* I2S_Standard */
#define I2S_Standard_Phillips              ((uint16_t)0x0000)
#define I2S_Standard_MSB                   ((uint16_t)0x0010)
#define I2S_Standard_LSB                   ((uint16_t)0x0020)
#define I2S_Standard_PCMShort              ((uint16_t)0x0030)
#define I2S_Standard_PCMLong               ((uint16_t)0x00B0)

/* I2S_Data_Format */
#define I2S_DataFormat_16b                 ((uint16_t)0x0000)
#define I2S_DataFormat_16bextended         ((uint16_t)0x0001)
#define I2S_DataFormat_24b                 ((uint16_t)0x0003)
#define I2S_DataFormat_32b                 ((uint16_t)0x0005)

/* I2S_MCLK_Output */
#define I2S_MCLKOutput_Enable              ((uint16_t)0x0200)
#define I2S_MCLKOutput_Disable             ((uint16_t)0x0000)

/* I2S_Audio_Frequency */
#define I2S_AudioFreq_192k                 ((uint32_t)192000)
#define I2S_AudioFreq_96k                  ((uint32_t)96000)
#define I2S_AudioFreq_48k                  ((uint32_t)48000)
#define I2S_AudioFreq_44k                  ((uint32_t)44100)
#define I2S_AudioFreq_32k                  ((uint32_t)32000)
#define I2S_AudioFreq_22k                  ((uint32_t)22050)
#define I2S_AudioFreq_16k                  ((uint32_t)16000)
#define I2S_AudioFreq_11k                  ((uint32_t)11025)
#define I2S_AudioFreq_8k                   ((uint32_t)8000)
#define I2S_AudioFreq_Default              ((uint32_t)2)

/* I2S_Clock_Polarity */
#define I2S_CPOL_Low                       ((uint16_t)0x0000)
#define I2S_CPOL_High                      ((uint16_t)0x0008)

#endif

/* SPI_I2S_DMA_transfer_requests */
#define SPI_I2S_DMAReq_Tx                  ((uint16_t)0x0002)
#define SPI_I2S_DMAReq_Rx                  ((uint16_t)0x0001)

/* SPI_NSS_internal_software_management */
#define SPI_NSSInternalSoft_Set            ((uint16_t)0x0100)
#define SPI_NSSInternalSoft_Reset          ((uint16_t)0xFEFF)

/* SPI_CRC_Transmit_Receive */
#define SPI_CRC_Tx                         ((uint8_t)0x00)
#define SPI_CRC_Rx                         ((uint8_t)0x01)

/* SPI_direction_transmit_receive */
#define SPI_Direction_Rx                   ((uint16_t)0xBFFF)
#define SPI_Direction_Tx                   ((uint16_t)0x4000)

/* SPI_I2S_interrupts_definition */
#define SPI_I2S_IT_TXE                     ((uint8_t)0x71)
#define SPI_I2S_IT_RXNE                    ((uint8_t)0x60)
#define SPI_I2S_IT_ERR                     ((uint8_t)0x50)
#define SPI_I2S_IT_OVR                     ((uint8_t)0x56)
#define SPI_IT_MODF                        ((uint8_t)0x55)
#define SPI_IT_CRCERR                      ((uint8_t)0x54)
#define I2S_IT_UDR                         ((uint8_t)0x53)

/* SPI_I2S_flags_definition */
#define SPI_I2S_FLAG_RXNE                  ((uint16_t)0x0001)
#define SPI_I2S_FLAG_TXE                   ((uint16_t)0x0002)
#define I2S_FLAG_CHSIDE                    ((uint16_t)0x0004)
#define I2S_FLAG_UDR                       ((uint16_t)0x0008)
#define SPI_FLAG_CRCERR                    ((uint16_t)0x0010)
#define SPI_FLAG_MODF                      ((uint16_t)0x0020)
#define SPI_I2S_FLAG_OVR                   ((uint16_t)0x0040)
#define SPI_I2S_FLAG_BSY                   ((uint16_t)0x0080)


/* ch32v00x_tim.h ------------------------------------------------------------*/

/* TIM_Output_Compare_and_PWM_modes */
#define TIM_OCMode_Timing                  ((uint16_t)0x0000)
#define TIM_OCMode_Active                  ((uint16_t)0x0010)
#define TIM_OCMode_Inactive                ((uint16_t)0x0020)
#define TIM_OCMode_Toggle                  ((uint16_t)0x0030)
#define TIM_OCMode_PWM1                    ((uint16_t)0x0060)
#define TIM_OCMode_PWM2                    ((uint16_t)0x0070)

/* TIM_One_Pulse_Mode */
#define TIM_OPMode_Single                  ((uint16_t)0x0008)
#define TIM_OPMode_Repetitive              ((uint16_t)0x0000)

/* TIM_Channel */
#define TIM_Channel_1                      ((uint16_t)0x0000)
#define TIM_Channel_2                      ((uint16_t)0x0004)
#define TIM_Channel_3                      ((uint16_t)0x0008)
#define TIM_Channel_4                      ((uint16_t)0x000C)

/* TIM_Clock_Division_CKD */
#define TIM_CKD_DIV1                       ((uint16_t)0x0000)
#define TIM_CKD_DIV2                       ((uint16_t)0x0100)
#define TIM_CKD_DIV4                       ((uint16_t)0x0200)

/* TIM_Counter_Mode */
#define TIM_CounterMode_Up                 ((uint16_t)0x0000)
#define TIM_CounterMode_Down               ((uint16_t)0x0010)
#define TIM_CounterMode_CenterAligned1     ((uint16_t)0x0020)
#define TIM_CounterMode_CenterAligned2     ((uint16_t)0x0040)
#define TIM_CounterMode_CenterAligned3     ((uint16_t)0x0060)

/* TIM_Output_Compare_Polarity */
#define TIM_OCPolarity_High                ((uint16_t)0x0000)
#define TIM_OCPolarity_Low                 ((uint16_t)0x0002)

/* TIM_Output_Compare_N_Polarity */
#define TIM_OCNPolarity_High               ((uint16_t)0x0000)
#define TIM_OCNPolarity_Low                ((uint16_t)0x0008)

/* TIM_Output_Compare_state */
#define TIM_OutputState_Disable            ((uint16_t)0x0000)
#define TIM_OutputState_Enable             ((uint16_t)0x0001)

/* TIM_Output_Compare_N_state */
#define TIM_OutputNState_Disable           ((uint16_t)0x0000)
#define TIM_OutputNState_Enable            ((uint16_t)0x0004)

/* TIM_Capture_Compare_state */
#define TIM_CCx_Enable                     ((uint16_t)0x0001)
#define TIM_CCx_Disable                    ((uint16_t)0x0000)

/* TIM_Capture_Compare_N_state */
#define TIM_CCxN_Enable                    ((uint16_t)0x0004)
#define TIM_CCxN_Disable                   ((uint16_t)0x0000)

/* Break_Input_enable_disable */
#define TIM_Break_Enable                   ((uint16_t)0x1000)
#define TIM_Break_Disable                  ((uint16_t)0x0000)

/* Break_Polarity */
#define TIM_BreakPolarity_Low              ((uint16_t)0x0000)
#define TIM_BreakPolarity_High             ((uint16_t)0x2000)

/* TIM_AOE_Bit_Set_Reset */
#define TIM_AutomaticOutput_Enable         ((uint16_t)0x4000)
#define TIM_AutomaticOutput_Disable        ((uint16_t)0x0000)

/* Lock_level */
#define TIM_LOCKLevel_OFF                  ((uint16_t)0x0000)
#define TIM_LOCKLevel_1                    ((uint16_t)0x0100)
#define TIM_LOCKLevel_2                    ((uint16_t)0x0200)
#define TIM_LOCKLevel_3                    ((uint16_t)0x0300)

/* OSSI_Off_State_Selection_for_Idle_mode_state */
#define TIM_OSSIState_Enable               ((uint16_t)0x0400)
#define TIM_OSSIState_Disable              ((uint16_t)0x0000)

/* OSSR_Off_State_Selection_for_Run_mode_state */
#define TIM_OSSRState_Enable               ((uint16_t)0x0800)
#define TIM_OSSRState_Disable              ((uint16_t)0x0000)

/* TIM_Output_Compare_Idle_State */
#define TIM_OCIdleState_Set                ((uint16_t)0x0100)
#define TIM_OCIdleState_Reset              ((uint16_t)0x0000)

/* TIM_Output_Compare_N_Idle_State */
#define TIM_OCNIdleState_Set               ((uint16_t)0x0200)
#define TIM_OCNIdleState_Reset             ((uint16_t)0x0000)

/* TIM_Input_Capture_Polarity */
#define TIM_ICPolarity_Rising              ((uint16_t)0x0000)
#define TIM_ICPolarity_Falling             ((uint16_t)0x0002)
#define TIM_ICPolarity_BothEdge            ((uint16_t)0x000A)

/* TIM_Input_Capture_Selection */
#define TIM_ICSelection_DirectTI           ((uint16_t)0x0001) /* TIM Input 1, 2, 3 or 4 is selected to be \
                                                                 connected to IC1, IC2, IC3 or IC4, respectively */
#define TIM_ICSelection_IndirectTI         ((uint16_t)0x0002) /* TIM Input 1, 2, 3 or 4 is selected to be \
                                                                 connected to IC2, IC1, IC4 or IC3, respectively. */
#define TIM_ICSelection_TRC                ((uint16_t)0x0003) /* TIM Input 1, 2, 3 or 4 is selected to be connected to TRC. */

/* TIM_Input_Capture_Prescaler */
#define TIM_ICPSC_DIV1                     ((uint16_t)0x0000) /* Capture performed each time an edge is detected on the capture input. */
#define TIM_ICPSC_DIV2                     ((uint16_t)0x0004) /* Capture performed once every 2 events. */
#define TIM_ICPSC_DIV4                     ((uint16_t)0x0008) /* Capture performed once every 4 events. */
#define TIM_ICPSC_DIV8                     ((uint16_t)0x000C) /* Capture performed once every 8 events. */

/* TIM_interrupt_sources */
#define TIM_IT_Update                      ((uint16_t)0x0001)
#define TIM_IT_CC1                         ((uint16_t)0x0002)
#define TIM_IT_CC2                         ((uint16_t)0x0004)
#define TIM_IT_CC3                         ((uint16_t)0x0008)
#define TIM_IT_CC4                         ((uint16_t)0x0010)
#define TIM_IT_COM                         ((uint16_t)0x0020)
#define TIM_IT_Trigger                     ((uint16_t)0x0040)
#define TIM_IT_Break                       ((uint16_t)0x0080)

/* TIM_DMA_Base_address */
#define TIM_DMABase_CR1                    ((uint16_t)0x0000)
#define TIM_DMABase_CR2                    ((uint16_t)0x0001)
#define TIM_DMABase_SMCR                   ((uint16_t)0x0002)
#define TIM_DMABase_DIER                   ((uint16_t)0x0003)
#define TIM_DMABase_SR                     ((uint16_t)0x0004)
#define TIM_DMABase_EGR                    ((uint16_t)0x0005)
#define TIM_DMABase_CCMR1                  ((uint16_t)0x0006)
#define TIM_DMABase_CCMR2                  ((uint16_t)0x0007)
#define TIM_DMABase_CCER                   ((uint16_t)0x0008)
#define TIM_DMABase_CNT                    ((uint16_t)0x0009)
#define TIM_DMABase_PSC                    ((uint16_t)0x000A)
#define TIM_DMABase_ARR                    ((uint16_t)0x000B)
#define TIM_DMABase_RCR                    ((uint16_t)0x000C)
#define TIM_DMABase_CCR1                   ((uint16_t)0x000D)
#define TIM_DMABase_CCR2                   ((uint16_t)0x000E)
#define TIM_DMABase_CCR3                   ((uint16_t)0x000F)
#define TIM_DMABase_CCR4                   ((uint16_t)0x0010)
#define TIM_DMABase_BDTR                   ((uint16_t)0x0011)
#define TIM_DMABase_DCR                    ((uint16_t)0x0012)

/* TIM_DMA_Burst_Length */
#define TIM_DMABurstLength_1Transfer       ((uint16_t)0x0000)
#define TIM_DMABurstLength_2Transfers      ((uint16_t)0x0100)
#define TIM_DMABurstLength_3Transfers      ((uint16_t)0x0200)
#define TIM_DMABurstLength_4Transfers      ((uint16_t)0x0300)
#define TIM_DMABurstLength_5Transfers      ((uint16_t)0x0400)
#define TIM_DMABurstLength_6Transfers      ((uint16_t)0x0500)
#define TIM_DMABurstLength_7Transfers      ((uint16_t)0x0600)
#define TIM_DMABurstLength_8Transfers      ((uint16_t)0x0700)
#define TIM_DMABurstLength_9Transfers      ((uint16_t)0x0800)
#define TIM_DMABurstLength_10Transfers     ((uint16_t)0x0900)
#define TIM_DMABurstLength_11Transfers     ((uint16_t)0x0A00)
#define TIM_DMABurstLength_12Transfers     ((uint16_t)0x0B00)
#define TIM_DMABurstLength_13Transfers     ((uint16_t)0x0C00)
#define TIM_DMABurstLength_14Transfers     ((uint16_t)0x0D00)
#define TIM_DMABurstLength_15Transfers     ((uint16_t)0x0E00)
#define TIM_DMABurstLength_16Transfers     ((uint16_t)0x0F00)
#define TIM_DMABurstLength_17Transfers     ((uint16_t)0x1000)
#define TIM_DMABurstLength_18Transfers     ((uint16_t)0x1100)

/* TIM_DMA_sources */
#define TIM_DMA_Update                     ((uint16_t)0x0100)
#define TIM_DMA_CC1                        ((uint16_t)0x0200)
#define TIM_DMA_CC2                        ((uint16_t)0x0400)
#define TIM_DMA_CC3                        ((uint16_t)0x0800)
#define TIM_DMA_CC4                        ((uint16_t)0x1000)
#define TIM_DMA_COM                        ((uint16_t)0x2000)
#define TIM_DMA_Trigger                    ((uint16_t)0x4000)

/* TIM_External_Trigger_Prescaler */
#define TIM_ExtTRGPSC_OFF                  ((uint16_t)0x0000)
#define TIM_ExtTRGPSC_DIV2                 ((uint16_t)0x1000)
#define TIM_ExtTRGPSC_DIV4                 ((uint16_t)0x2000)
#define TIM_ExtTRGPSC_DIV8                 ((uint16_t)0x3000)

/* TIM_Internal_Trigger_Selection */
#define TIM_TS_ITR0                        ((uint16_t)0x0000)
#define TIM_TS_ITR1                        ((uint16_t)0x0010)
#define TIM_TS_ITR2                        ((uint16_t)0x0020)
#define TIM_TS_ITR3                        ((uint16_t)0x0030)
#define TIM_TS_TI1F_ED                     ((uint16_t)0x0040)
#define TIM_TS_TI1FP1                      ((uint16_t)0x0050)
#define TIM_TS_TI2FP2                      ((uint16_t)0x0060)
#define TIM_TS_ETRF                        ((uint16_t)0x0070)

/* TIM_TIx_External_Clock_Source */
#define TIM_TIxExternalCLK1Source_TI1      ((uint16_t)0x0050)
#define TIM_TIxExternalCLK1Source_TI2      ((uint16_t)0x0060)
#define TIM_TIxExternalCLK1Source_TI1ED    ((uint16_t)0x0040)

/* TIM_External_Trigger_Polarity */
#define TIM_ExtTRGPolarity_Inverted        ((uint16_t)0x8000)
#define TIM_ExtTRGPolarity_NonInverted     ((uint16_t)0x0000)

/* TIM_Prescaler_Reload_Mode */
#define TIM_PSCReloadMode_Update           ((uint16_t)0x0000)
#define TIM_PSCReloadMode_Immediate        ((uint16_t)0x0001)

/* TIM_Forced_Action */
#define TIM_ForcedAction_Active            ((uint16_t)0x0050)
#define TIM_ForcedAction_InActive          ((uint16_t)0x0040)

/* TIM_Encoder_Mode */
#define TIM_EncoderMode_TI1                ((uint16_t)0x0001)
#define TIM_EncoderMode_TI2                ((uint16_t)0x0002)
#define TIM_EncoderMode_TI12               ((uint16_t)0x0003)

/* TIM_Event_Source */
#define TIM_EventSource_Update             ((uint16_t)0x0001)
#define TIM_EventSource_CC1                ((uint16_t)0x0002)
#define TIM_EventSource_CC2                ((uint16_t)0x0004)
#define TIM_EventSource_CC3                ((uint16_t)0x0008)
#define TIM_EventSource_CC4                ((uint16_t)0x0010)
#define TIM_EventSource_COM                ((uint16_t)0x0020)
#define TIM_EventSource_Trigger            ((uint16_t)0x0040)
#define TIM_EventSource_Break              ((uint16_t)0x0080)

/* TIM_Update_Source */
#define TIM_UpdateSource_Global            ((uint16_t)0x0000) /* Source of update is the counter overflow/underflow \
                                                                 or the setting of UG bit, or an update generation  \
                                                                 through the slave mode controller. */
#define TIM_UpdateSource_Regular           ((uint16_t)0x0001) /* Source of update is counter overflow/underflow. */

/* TIM_Output_Compare_Preload_State */
#define TIM_OCPreload_Enable               ((uint16_t)0x0008)
#define TIM_OCPreload_Disable              ((uint16_t)0x0000)

/* TIM_Output_Compare_Fast_State */
#define TIM_OCFast_Enable                  ((uint16_t)0x0004)
#define TIM_OCFast_Disable                 ((uint16_t)0x0000)

/* TIM_Output_Compare_Clear_State */
#define TIM_OCClear_Enable                 ((uint16_t)0x0080)
#define TIM_OCClear_Disable                ((uint16_t)0x0000)

/* TIM_Trigger_Output_Source */
#define TIM_TRGOSource_Reset               ((uint16_t)0x0000)
#define TIM_TRGOSource_Enable              ((uint16_t)0x0010)
#define TIM_TRGOSource_Update              ((uint16_t)0x0020)
#define TIM_TRGOSource_OC1                 ((uint16_t)0x0030)
#define TIM_TRGOSource_OC1Ref              ((uint16_t)0x0040)
#define TIM_TRGOSource_OC2Ref              ((uint16_t)0x0050)
#define TIM_TRGOSource_OC3Ref              ((uint16_t)0x0060)
#define TIM_TRGOSource_OC4Ref              ((uint16_t)0x0070)

/* TIM_Slave_Mode */
#define TIM_SlaveMode_Reset                ((uint16_t)0x0004)
#define TIM_SlaveMode_Gated                ((uint16_t)0x0005)
#define TIM_SlaveMode_Trigger              ((uint16_t)0x0006)
#define TIM_SlaveMode_External1            ((uint16_t)0x0007)

/* TIM_Master_Slave_Mode */
#define TIM_MasterSlaveMode_Enable         ((uint16_t)0x0080)
#define TIM_MasterSlaveMode_Disable        ((uint16_t)0x0000)

/* TIM_Flags */
#define TIM_FLAG_Update                    ((uint16_t)0x0001)
#define TIM_FLAG_CC1                       ((uint16_t)0x0002)
#define TIM_FLAG_CC2                       ((uint16_t)0x0004)
#define TIM_FLAG_CC3                       ((uint16_t)0x0008)
#define TIM_FLAG_CC4                       ((uint16_t)0x0010)
#define TIM_FLAG_COM                       ((uint16_t)0x0020)
#define TIM_FLAG_Trigger                   ((uint16_t)0x0040)
#define TIM_FLAG_Break                     ((uint16_t)0x0080)
#define TIM_FLAG_CC1OF                     ((uint16_t)0x0200)
#define TIM_FLAG_CC2OF                     ((uint16_t)0x0400)
#define TIM_FLAG_CC3OF                     ((uint16_t)0x0800)
#define TIM_FLAG_CC4OF                     ((uint16_t)0x1000)

/* TIM_Legacy */
#define TIM_DMABurstLength_1Byte           TIM_DMABurstLength_1Transfer
#define TIM_DMABurstLength_2Bytes          TIM_DMABurstLength_2Transfers
#define TIM_DMABurstLength_3Bytes          TIM_DMABurstLength_3Transfers
#define TIM_DMABurstLength_4Bytes          TIM_DMABurstLength_4Transfers
#define TIM_DMABurstLength_5Bytes          TIM_DMABurstLength_5Transfers
#define TIM_DMABurstLength_6Bytes          TIM_DMABurstLength_6Transfers
#define TIM_DMABurstLength_7Bytes          TIM_DMABurstLength_7Transfers
#define TIM_DMABurstLength_8Bytes          TIM_DMABurstLength_8Transfers
#define TIM_DMABurstLength_9Bytes          TIM_DMABurstLength_9Transfers
#define TIM_DMABurstLength_10Bytes         TIM_DMABurstLength_10Transfers
#define TIM_DMABurstLength_11Bytes         TIM_DMABurstLength_11Transfers
#define TIM_DMABurstLength_12Bytes         TIM_DMABurstLength_12Transfers
#define TIM_DMABurstLength_13Bytes         TIM_DMABurstLength_13Transfers
#define TIM_DMABurstLength_14Bytes         TIM_DMABurstLength_14Transfers
#define TIM_DMABurstLength_15Bytes         TIM_DMABurstLength_15Transfers
#define TIM_DMABurstLength_16Bytes         TIM_DMABurstLength_16Transfers
#define TIM_DMABurstLength_17Bytes         TIM_DMABurstLength_17Transfers
#define TIM_DMABurstLength_18Bytes         TIM_DMABurstLength_18Transfers


/* ch32v00x_usart.h ----------------------------------------------------------*/

/* USART_Word_Length */
#define USART_WordLength_8b                  ((uint16_t)0x0000)
#define USART_WordLength_9b                  ((uint16_t)0x1000)

/* USART_Stop_Bits */
#define USART_StopBits_1                     ((uint16_t)0x0000)
#define USART_StopBits_0_5                   ((uint16_t)0x1000)
#define USART_StopBits_2                     ((uint16_t)0x2000)
#define USART_StopBits_1_5                   ((uint16_t)0x3000)

/* USART_Parity */
#define USART_Parity_No                      ((uint16_t)0x0000)
#define USART_Parity_Even                    ((uint16_t)0x0400)
#define USART_Parity_Odd                     ((uint16_t)0x0600)

/* USART_Mode */
#define USART_Mode_Rx                        ((uint16_t)0x0004)
#define USART_Mode_Tx                        ((uint16_t)0x0008)

/* USART_Hardware_Flow_Control */
#define USART_HardwareFlowControl_None       ((uint16_t)0x0000)
#define USART_HardwareFlowControl_RTS        ((uint16_t)0x0100)
#define USART_HardwareFlowControl_CTS        ((uint16_t)0x0200)
#define USART_HardwareFlowControl_RTS_CTS    ((uint16_t)0x0300)

/* USART_Clock */
#define USART_Clock_Disable                  ((uint16_t)0x0000)
#define USART_Clock_Enable                   ((uint16_t)0x0800)

/* USART_Clock_Polarity */
#define USART_CPOL_Low                       ((uint16_t)0x0000)
#define USART_CPOL_High                      ((uint16_t)0x0400)

/* USART_Clock_Phase */
#define USART_CPHA_1Edge                     ((uint16_t)0x0000)
#define USART_CPHA_2Edge                     ((uint16_t)0x0200)

/* USART_Last_Bit */
#define USART_LastBit_Disable                ((uint16_t)0x0000)
#define USART_LastBit_Enable                 ((uint16_t)0x0100)

/* USART_Interrupt_definition */
#define USART_IT_PE                          ((uint16_t)0x0028)
#define USART_IT_TXE                         ((uint16_t)0x0727)
#define USART_IT_TC                          ((uint16_t)0x0626)
#define USART_IT_RXNE                        ((uint16_t)0x0525)
#define USART_IT_ORE_RX                      ((uint16_t)0x0325)
#define USART_IT_IDLE                        ((uint16_t)0x0424)
#define USART_IT_LBD                         ((uint16_t)0x0846)
#define USART_IT_CTS                         ((uint16_t)0x096A)
#define USART_IT_ERR                         ((uint16_t)0x0060)
#define USART_IT_ORE_ER                      ((uint16_t)0x0360)
#define USART_IT_NE                          ((uint16_t)0x0260)
#define USART_IT_FE                          ((uint16_t)0x0160)

#define USART_IT_ORE                         USART_IT_ORE_ER

/* USART_DMA_Requests */
#define USART_DMAReq_Tx                      ((uint16_t)0x0080)
#define USART_DMAReq_Rx                      ((uint16_t)0x0040)

/* USART_WakeUp_methods */
#define USART_WakeUp_IdleLine                ((uint16_t)0x0000)
#define USART_WakeUp_AddressMark             ((uint16_t)0x0800)

/* USART_LIN_Break_Detection_Length */
#define USART_LINBreakDetectLength_10b       ((uint16_t)0x0000)
#define USART_LINBreakDetectLength_11b       ((uint16_t)0x0020)

/* USART_IrDA_Low_Power */
#define USART_IrDAMode_LowPower              ((uint16_t)0x0004)
#define USART_IrDAMode_Normal                ((uint16_t)0x0000)

/* USART_Flags */
#define USART_FLAG_CTS                       ((uint16_t)0x0200)
#define USART_FLAG_LBD                       ((uint16_t)0x0100)
#define USART_FLAG_TXE                       ((uint16_t)0x0080)
#define USART_FLAG_TC                        ((uint16_t)0x0040)
#define USART_FLAG_RXNE                      ((uint16_t)0x0020)
#define USART_FLAG_IDLE                      ((uint16_t)0x0010)
#define USART_FLAG_ORE                       ((uint16_t)0x0008)
#define USART_FLAG_NE                        ((uint16_t)0x0004)
#define USART_FLAG_FE                        ((uint16_t)0x0002)
#define USART_FLAG_PE                        ((uint16_t)0x0001)

// While not truly CH32X035, we can re-use some of the USB register defs.
#if defined(CH32V10x) | defined(CH32X03x)
/* ch32v10x_usb.h ------------------------------------------------------------*/

#ifndef NULL
  #define NULL    0
#endif

#ifndef VOID
  #define VOID    void
#endif
#ifndef CONST
  #define CONST    const
#endif
#ifndef BOOL
typedef unsigned char BOOL;
#endif
#ifndef BOOLEAN
typedef unsigned char BOOLEAN;
#endif
#ifndef CHAR
typedef char CHAR;
#endif
#ifndef INT8
typedef char INT8;
#endif
#ifndef INT16
typedef short INT16;
#endif
#ifndef INT32
typedef long INT32;
#endif
#ifndef UINT8
typedef unsigned char UINT8;
#endif
#ifndef UINT16
typedef unsigned short UINT16;
#endif
#ifndef UINT32
typedef unsigned long UINT32;
#endif
#ifndef UINT8V
typedef unsigned char volatile UINT8V;
#endif
#ifndef UINT16V
typedef unsigned short volatile UINT16V;
#endif
#ifndef UINT32V
typedef unsigned long volatile UINT32V;
#endif

#ifndef PVOID
typedef void *PVOID;
#endif
#ifndef PCHAR
typedef char *PCHAR;
#endif
#ifndef PCHAR
typedef const char *PCCHAR;
#endif
#ifndef PINT8
typedef char *PINT8;
#endif
#ifndef PINT16
typedef short *PINT16;
#endif
#ifndef PINT32
typedef long *PINT32;
#endif
#ifndef PUINT8
typedef unsigned char *PUINT8;
#endif
#ifndef PUINT16
typedef unsigned short *PUINT16;
#endif
#ifndef PUINT32
typedef unsigned long *PUINT32;
#endif
#ifndef PUINT8V
typedef volatile unsigned char *PUINT8V;
#endif
#ifndef PUINT16V
typedef volatile unsigned short *PUINT16V;
#endif
#ifndef PUINT32V
typedef volatile unsigned long *PUINT32V;
#endif

/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/
/*       USB  */
#define R32_USB_CONTROL      (*((PUINT32V)(0x40023400))) // USB control & interrupt enable & device address
#define R8_USB_CTRL          (*((PUINT8V)(0x40023400)))  // USB base control
#define RB_UC_HOST_MODE      0x80                        // enable USB host mode: 0=device mode, 1=host mode
#define RB_UC_LOW_SPEED      0x40                        // enable USB low speed: 0=12Mbps, 1=1.5Mbps
#define RB_UC_DEV_PU_EN      0x20                        // USB device enable and internal pullup resistance enable
#define RB_UC_SYS_CTRL1      0x20                        // USB system control high bit
#define RB_UC_SYS_CTRL0      0x10                        // USB system control low bit
#define MASK_UC_SYS_CTRL     0x30                        // bit mask of USB system control
// bUC_HOST_MODE & bUC_SYS_CTRL1 & bUC_SYS_CTRL0: USB system control
//   0 00: disable USB device and disable internal pullup resistance
//   0 01: enable USB device and disable internal pullup resistance, need external pullup resistance
//   0 1x: enable USB device and enable internal pullup resistance
//   1 00: enable USB host and normal status
//   1 01: enable USB host and force UDP/UDM output SE0 state
//   1 10: enable USB host and force UDP/UDM output J state
//   1 11: enable USB host and force UDP/UDM output resume or K state
#define RB_UC_INT_BUSY       0x08           // enable automatic responding busy for device mode or automatic pause for host mode during interrupt flag UIF_TRANSFER valid
#define RB_UC_RESET_SIE      0x04           // force reset USB SIE, need software clear
#define RB_UC_CLR_ALL        0x02           // force clear FIFO and count of USB
#define RB_UC_DMA_EN         0x01           // DMA enable and DMA interrupt enable for USB

#define R8_UDEV_CTRL         (*((PUINT8V)(0x40023401))) // USB device physical prot control
#define RB_UD_PD_DIS         0x80                       // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define RB_UD_DP_PIN         0x20                       // ReadOnly: indicate current UDP pin level
#define RB_UD_DM_PIN         0x10                       // ReadOnly: indicate current UDM pin level
#define RB_UD_LOW_SPEED      0x04                       // enable USB physical port low speed: 0=full speed, 1=low speed
#define RB_UD_GP_BIT         0x02                       // general purpose bit
#define RB_UD_PORT_EN        0x01                       // enable USB physical port I/O: 0=disable, 1=enable

#define R8_UHOST_CTRL        R8_UDEV_CTRL   // USB host physical prot control
#define RB_UH_PD_DIS         0x80           // disable USB UDP/UDM pulldown resistance: 0=enable pulldown, 1=disable
#define RB_UH_DP_PIN         0x20           // ReadOnly: indicate current UDP pin level
#define RB_UH_DM_PIN         0x10           // ReadOnly: indicate current UDM pin level
#define RB_UH_LOW_SPEED      0x04           // enable USB port low speed: 0=full speed, 1=low speed
#define RB_UH_BUS_RESET      0x02           // control USB bus reset: 0=normal, 1=force bus reset
#define RB_UH_PORT_EN        0x01           // enable USB port: 0=disable, 1=enable port, automatic disabled if USB device detached

#define R8_USB_INT_EN        (*((PUINT8V)(0x40023402))) // USB interrupt enable
#define RB_UIE_DEV_SOF       0x80                       // enable interrupt for SOF received for USB device mode
#define RB_UIE_DEV_NAK       0x40                       // enable interrupt for NAK responded for USB device mode
#define RB_UIE_FIFO_OV       0x10                       // enable interrupt for FIFO overflow
#define RB_UIE_HST_SOF       0x08                       // enable interrupt for host SOF timer action for USB host mode
#define RB_UIE_SUSPEND       0x04                       // enable interrupt for USB suspend or resume event
#define RB_UIE_TRANSFER      0x02                       // enable interrupt for USB transfer completion
#define RB_UIE_DETECT        0x01                       // enable interrupt for USB device detected event for USB host mode
#define RB_UIE_BUS_RST       0x01                       // enable interrupt for USB bus reset event for USB device mode

#define R8_USB_DEV_AD        (*((PUINT8V)(0x40023403))) // USB device address
#define RB_UDA_GP_BIT        0x80                       // general purpose bit
#define MASK_USB_ADDR        0x7F                       // bit mask for USB device address

#define R32_USB_STATUS       (*((PUINT32V)(0x40023404))) // USB miscellaneous status & interrupt flag & interrupt status
#define R8_USB_MIS_ST        (*((PUINT8V)(0x40023405)))  // USB miscellaneous status
#define RB_UMS_SOF_PRES      0x80                        // RO, indicate host SOF timer presage status
#define RB_UMS_SOF_ACT       0x40                        // RO, indicate host SOF timer action status for USB host
#define RB_UMS_SIE_FREE      0x20                        // RO, indicate USB SIE free status
#define RB_UMS_R_FIFO_RDY    0x10                        // RO, indicate USB receiving FIFO ready status (not empty)
#define RB_UMS_BUS_RESET     0x08                        // RO, indicate USB bus reset status
#define RB_UMS_SUSPEND       0x04                        // RO, indicate USB suspend status
#define RB_UMS_DM_LEVEL      0x02                        // RO, indicate UDM level saved at device attached to USB host
#define RB_UMS_DEV_ATTACH    0x01                        // RO, indicate device attached status on USB host

#define R8_USB_INT_FG        (*((PUINT8V)(0x40023406))) // USB interrupt flag
#define RB_U_IS_NAK          0x80                       // RO, indicate current USB transfer is NAK received
#define RB_U_TOG_OK          0x40                       // RO, indicate current USB transfer toggle is OK
#define RB_U_SIE_FREE        0x20                       // RO, indicate USB SIE free status
#define RB_UIF_FIFO_OV       0x10                       // FIFO overflow interrupt flag for USB, direct bit address clear or write 1 to clear
#define RB_UIF_HST_SOF       0x08                       // host SOF timer interrupt flag for USB host, direct bit address clear or write 1 to clear
#define RB_UIF_SUSPEND       0x04                       // USB suspend or resume event interrupt flag, direct bit address clear or write 1 to clear
#define RB_UIF_TRANSFER      0x02                       // USB transfer completion interrupt flag, direct bit address clear or write 1 to clear
#define RB_UIF_DETECT        0x01                       // device detected event interrupt flag for USB host mode, direct bit address clear or write 1 to clear
#define RB_UIF_BUS_RST       0x01                       // bus reset event interrupt flag for USB device mode, direct bit address clear or write 1 to clear

#define R8_USB_INT_ST        (*((PUINT8V)(0x40023407))) // USB interrupt status
#define RB_UIS_IS_NAK        0x80                       // RO, indicate current USB transfer is NAK received for USB device mode
#define RB_UIS_TOG_OK        0x40                       // RO, indicate current USB transfer toggle is OK
#define RB_UIS_TOKEN1        0x20                       // RO, current token PID code bit 1 received for USB device mode
#define RB_UIS_TOKEN0        0x10                       // RO, current token PID code bit 0 received for USB device mode
#define MASK_UIS_TOKEN       0x30                       // RO, bit mask of current token PID code received for USB device mode
#define UIS_TOKEN_OUT        0x00
#define UIS_TOKEN_SOF        0x10
#define UIS_TOKEN_IN         0x20
#define UIS_TOKEN_SETUP      0x30
// bUIS_TOKEN1 & bUIS_TOKEN0: current token PID code received for USB device mode
//   00: OUT token PID received
//   01: SOF token PID received
//   10: IN token PID received
//   11: SETUP token PID received
#define MASK_UIS_ENDP        0x0F           // RO, bit mask of current transfer endpoint number for USB device mode
#define MASK_UIS_H_RES       0x0F           // RO, bit mask of current transfer handshake response for USB host mode: 0000=no response, time out from device, others=handshake response PID received

#define R16_USB_RX_LEN       (*((PUINT16V)(0x40023408))) // USB receiving length
#define MASK_UIS_RX_LEN      0x3FF                       // RO, bit mask of current receive length(10 bits for ch32v10x)
#define R32_USB_BUF_MODE     (*((PUINT32V)(0x4002340c))) // USB endpoint buffer mode
#define R8_UEP4_1_MOD        (*((PUINT8V)(0x4002340c)))  // endpoint 4/1 mode
#define RB_UEP1_RX_EN        0x80                        // enable USB endpoint 1 receiving (OUT)
#define RB_UEP1_TX_EN        0x40                        // enable USB endpoint 1 transmittal (IN)
#define RB_UEP1_BUF_MOD      0x10                        // buffer mode of USB endpoint 1
// bUEPn_RX_EN & bUEPn_TX_EN & bUEPn_BUF_MOD: USB endpoint 1/2/3 buffer mode, buffer start address is UEPn_DMA
//   0 0 x:  disable endpoint and disable buffer
//   1 0 0:  64 bytes buffer for receiving (OUT endpoint)
//   1 0 1:  dual 64 bytes buffer by toggle bit bUEP_R_TOG selection for receiving (OUT endpoint), total=128bytes
//   0 1 0:  64 bytes buffer for transmittal (IN endpoint)
//   0 1 1:  dual 64 bytes buffer by toggle bit bUEP_T_TOG selection for transmittal (IN endpoint), total=128bytes
//   1 1 0:  64 bytes buffer for receiving (OUT endpoint) + 64 bytes buffer for transmittal (IN endpoint), total=128bytes
//   1 1 1:  dual 64 bytes buffer by bUEP_R_TOG selection for receiving (OUT endpoint) + dual 64 bytes buffer by bUEP_T_TOG selection for transmittal (IN endpoint), total=256bytes
#define RB_UEP4_RX_EN        0x08           // enable USB endpoint 4 receiving (OUT)
#define RB_UEP4_TX_EN        0x04           // enable USB endpoint 4 transmittal (IN)
// bUEP4_RX_EN & bUEP4_TX_EN: USB endpoint 4 buffer mode, buffer start address is UEP0_DMA
//   0 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//   1 0:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 receiving (OUT endpoint), total=128bytes
//   0 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=128bytes
//   1 1:  single 64 bytes buffer for endpoint 0 receiving & transmittal (OUT & IN endpoint)
//           + 64 bytes buffer for endpoint 4 receiving (OUT endpoint) + 64 bytes buffer for endpoint 4 transmittal (IN endpoint), total=192bytes

#define R8_UEP2_3_MOD        (*((PUINT8V)(0x4002340d))) // endpoint 2/3 mode
#define RB_UEP3_RX_EN        0x80                       // enable USB endpoint 3 receiving (OUT)
#define RB_UEP3_TX_EN        0x40                       // enable USB endpoint 3 transmittal (IN)
#define RB_UEP3_BUF_MOD      0x10                       // buffer mode of USB endpoint 3
#define RB_UEP2_RX_EN        0x08                       // enable USB endpoint 2 receiving (OUT)
#define RB_UEP2_TX_EN        0x04                       // enable USB endpoint 2 transmittal (IN)
#define RB_UEP2_BUF_MOD      0x01                       // buffer mode of USB endpoint 2

#define R8_UH_EP_MOD         R8_UEP2_3_MOD  //host endpoint mode
#define RB_UH_EP_TX_EN       0x40           // enable USB host OUT endpoint transmittal
#define RB_UH_EP_TBUF_MOD    0x10           // buffer mode of USB host OUT endpoint
// bUH_EP_TX_EN & bUH_EP_TBUF_MOD: USB host OUT endpoint buffer mode, buffer start address is UH_TX_DMA
//   0 x:  disable endpoint and disable buffer
//   1 0:  64 bytes buffer for transmittal (OUT endpoint)
//   1 1:  dual 64 bytes buffer by toggle bit bUH_T_TOG selection for transmittal (OUT endpoint), total=128bytes
#define RB_UH_EP_RX_EN       0x08           // enable USB host IN endpoint receiving
#define RB_UH_EP_RBUF_MOD    0x01           // buffer mode of USB host IN endpoint
// bUH_EP_RX_EN & bUH_EP_RBUF_MOD: USB host IN endpoint buffer mode, buffer start address is UH_RX_DMA
//   0 x:  disable endpoint and disable buffer
//   1 0:  64 bytes buffer for receiving (IN endpoint)
//   1 1:  dual 64 bytes buffer by toggle bit bUH_R_TOG selection for receiving (IN endpoint), total=128bytes

#define R8_UEP5_6_MOD        (*((PUINT8V)(0x4002340e))) // endpoint 5/6 mode
#define RB_UEP6_RX_EN        0x80                       // enable USB endpoint 6 receiving (OUT)
#define RB_UEP6_TX_EN        0x40                       // enable USB endpoint 6 transmittal (IN)
#define RB_UEP6_BUF_MOD      0x10                       // buffer mode of USB endpoint 6
#define RB_UEP5_RX_EN        0x08                       // enable USB endpoint 5 receiving (OUT)
#define RB_UEP5_TX_EN        0x04                       // enable USB endpoint 5 transmittal (IN)
#define RB_UEP5_BUF_MOD      0x01                       // buffer mode of USB endpoint 5

#define R8_UEP7_MOD          (*((PUINT8V)(0x4002340f))) // endpoint 7 mode
#define RB_UEP7_RX_EN        0x08                       // enable USB endpoint 7 receiving (OUT)
#define RB_UEP7_TX_EN        0x04                       // enable USB endpoint 7 transmittal (IN)
#define RB_UEP7_BUF_MOD      0x01                       // buffer mode of USB endpoint 7

#define R16_UEP0_DMA         (*((PUINT16V)(0x40023410))) // endpoint 0 DMA buffer address
#define R16_UEP1_DMA         (*((PUINT16V)(0x40023414))) // endpoint 1 DMA buffer address
#define R16_UEP2_DMA         (*((PUINT16V)(0x40023418))) // endpoint 2 DMA buffer address
#define R16_UH_RX_DMA        R16_UEP2_DMA                // host rx endpoint buffer high address
#define R16_UEP3_DMA         (*((PUINT16V)(0x4002341c))) // endpoint 3 DMA buffer address

#define R16_UEP4_DMA         (*((PUINT16V)(0x40023420))) // endpoint 4 DMA buffer address
#define R16_UEP5_DMA         (*((PUINT16V)(0x40023424))) // endpoint 5 DMA buffer address
#define R16_UEP6_DMA         (*((PUINT16V)(0x40023428))) // endpoint 6 DMA buffer address
#define R16_UEP7_DMA         (*((PUINT16V)(0x4002342c))) // endpoint 7 DMA buffer address

#define R16_UH_TX_DMA        R16_UEP3_DMA                // host tx endpoint buffer high address
#define R32_USB_EP0_CTRL     (*((PUINT32V)(0x40023430))) // endpoint 0 control & transmittal length
#define R8_UEP0_T_LEN        (*((PUINT8V)(0x40023430)))  // endpoint 0 transmittal length
#define R8_UEP0_CTRL         (*((PUINT8V)(0x40023432)))  // endpoint 0 control
#define R32_USB_EP1_CTRL     (*((PUINT32V)(0x40023434))) // endpoint 1 control & transmittal length
#define R16_UEP1_T_LEN       (*((PUINT16V)(0x40023434))) // endpoint 1 transmittal length(16-bits for ch32v10x)
#define R8_UEP1_CTRL         (*((PUINT8V)(0x40023436)))  // endpoint 1 control
#define RB_UEP_R_TOG         0x80                        // expected data toggle flag of USB endpoint X receiving (OUT): 0=DATA0, 1=DATA1
#define RB_UEP_T_TOG         0x40                        // prepared data toggle flag of USB endpoint X transmittal (IN): 0=DATA0, 1=DATA1
#define RB_UEP_AUTO_TOG      0x10                        // enable automatic toggle after successful transfer completion on endpoint 1/2/3: 0=manual toggle, 1=automatic toggle
#define RB_UEP_R_RES1        0x08                        // handshake response type high bit for USB endpoint X receiving (OUT)
#define RB_UEP_R_RES0        0x04                        // handshake response type low bit for USB endpoint X receiving (OUT)
#define MASK_UEP_R_RES       0x0C                        // bit mask of handshake response type for USB endpoint X receiving (OUT)
#define UEP_R_RES_ACK        0x00
#define UEP_R_RES_TOUT       0x04
#define UEP_R_RES_NAK        0x08
#define UEP_R_RES_STALL      0x0C
// RB_UEP_R_RES1 & RB_UEP_R_RES0: handshake response type for USB endpoint X receiving (OUT)
//   00: ACK (ready)
//   01: no response, time out to host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)
#define RB_UEP_T_RES1        0x02           // handshake response type high bit for USB endpoint X transmittal (IN)
#define RB_UEP_T_RES0        0x01           // handshake response type low bit for USB endpoint X transmittal (IN)
#define MASK_UEP_T_RES       0x03           // bit mask of handshake response type for USB endpoint X transmittal (IN)
#define UEP_T_RES_ACK        0x00
#define UEP_T_RES_TOUT       0x01
#define UEP_T_RES_NAK        0x02
#define UEP_T_RES_STALL      0x03
// bUEP_T_RES1 & bUEP_T_RES0: handshake response type for USB endpoint X transmittal (IN)
//   00: DATA0 or DATA1 then expecting ACK (ready)
//   01: DATA0 or DATA1 then expecting no response, time out from host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)

#define R8_UH_SETUP          R8_UEP1_CTRL   // host aux setup
#define RB_UH_PRE_PID_EN     0x80           // USB host PRE PID enable for low speed device via hub
#define RB_UH_SOF_EN         0x40           // USB host automatic SOF enable

#define R32_USB_EP2_CTRL     (*((PUINT32V)(0x40023438))) // endpoint 2 control & transmittal length
#define R16_UEP2_T_LEN       (*((PUINT16V)(0x40023438))) // endpoint 2 transmittal length(16-bits for ch32v10x)
#define R8_UEP2_CTRL         (*((PUINT8V)(0x4002343a)))  // endpoint 2 control

#define R8_UH_EP_PID         (*((PUINT8V)(0x40023438)))  // host endpoint and PID
#define MASK_UH_TOKEN        0xF0           // bit mask of token PID for USB host transfer
#define MASK_UH_ENDP         0x0F           // bit mask of endpoint number for USB host transfer

#define R8_UH_RX_CTRL        R8_UEP2_CTRL   // host receiver endpoint control
#define RB_UH_R_TOG          0x80           // expected data toggle flag of host receiving (IN): 0=DATA0, 1=DATA1
#define RB_UH_R_AUTO_TOG     0x10           // enable automatic toggle after successful transfer completion: 0=manual toggle, 1=automatic toggle
#define RB_UH_R_RES          0x04           // prepared handshake response type for host receiving (IN): 0=ACK (ready), 1=no response, time out to device, for isochronous transactions

#define R32_USB_EP3_CTRL     (*((PUINT32V)(0x4002343c))) // endpoint 3 control & transmittal length
#define R16_UEP3_T_LEN       (*((PUINT16V)(0x4002343c))) // endpoint 3 transmittal length(16-bits for ch32v10x)
#define R8_UEP3_CTRL         (*((PUINT8V)(0x4002343e)))  // endpoint 3 control
#define R8_UH_TX_LEN         (*((PUINT16V)(0x4002343c))) //R8_UEP3_T_LEN				// host transmittal endpoint transmittal length

#define R8_UH_TX_CTRL        R8_UEP3_CTRL   // host transmittal endpoint control
#define RB_UH_T_TOG          0x40           // prepared data toggle flag of host transmittal (SETUP/OUT): 0=DATA0, 1=DATA1
#define RB_UH_T_AUTO_TOG     0x10           // enable automatic toggle after successful transfer completion: 0=manual toggle, 1=automatic toggle
#define RB_UH_T_RES          0x01           // expected handshake response type for host transmittal (SETUP/OUT): 0=ACK (ready), 1=no response, time out from device, for isochronous transactions

#define R32_USB_EP4_CTRL     (*((PUINT32V)(0x40023440))) // endpoint 4 control & transmittal length
#define R16_UEP4_T_LEN       (*((PUINT16V)(0x40023440))) // endpoint 4 transmittal length(16-bits for ch32v10x)
#define R8_UEP4_CTRL         (*((PUINT8V)(0x40023442)))  // endpoint 4 control

#define R32_USB_EP5_CTRL     (*((PUINT32V)(0x40023444))) // endpoint 5 control & transmittal length
#define R16_UEP5_T_LEN       (*((PUINT16V)(0x40023444))) // endpoint 5 transmittal length(16-bits for ch32v10x)
#define R8_UEP5_CTRL         (*((PUINT8V)(0x40023446)))  // endpoint 5 control

#define R32_USB_EP6_CTRL     (*((PUINT32V)(0x40023448))) // endpoint 6 control & transmittal length
#define R16_UEP6_T_LEN       (*((PUINT16V)(0x40023448))) // endpoint 6 transmittal length(16-bits for ch32v10x)
#define R8_UEP6_CTRL         (*((PUINT8V)(0x4002344a)))  // endpoint 6 control

#define R32_USB_EP7_CTRL     (*((PUINT32V)(0x4002344c))) // endpoint 7 control & transmittal length
#define R16_UEP7_T_LEN       (*((PUINT16V)(0x4002344c))) // endpoint 7 transmittal length(16-bits for ch32v10x)
#define R8_UEP7_CTRL         (*((PUINT8V)(0x4002344e)))  // endpoint 7 control

/* ch32v10x_usb_host.h -----------------------------------------------------------*/

#define ERR_SUCCESS            0x00
#define ERR_USB_CONNECT        0x15
#define ERR_USB_DISCON         0x16
#define ERR_USB_BUF_OVER       0x17
#define ERR_USB_DISK_ERR       0x1F
#define ERR_USB_TRANSFER       0x20
#define ERR_USB_UNSUPPORT      0xFB
#define ERR_USB_UNKNOWN        0xFE
#define ERR_AOA_PROTOCOL       0x41

#define ROOT_DEV_DISCONNECT    0
#define ROOT_DEV_CONNECTED     1
#define ROOT_DEV_FAILED        2
#define ROOT_DEV_SUCCESS       3
#define DEV_TYPE_KEYBOARD      (USB_DEV_CLASS_HID | 0x20)
#define DEV_TYPE_MOUSE         (USB_DEV_CLASS_HID | 0x30)
#define DEF_AOA_DEVICE         0xF0
#define DEV_TYPE_UNKNOW        0xFF

#define HUB_MAX_PORTS          4
#define WAIT_USB_TOUT_200US    3000

#endif

/* ch32v00x_wwdg.h -----------------------------------------------------------*/


/* WWDG_Prescaler */
#define WWDG_Prescaler_1    ((uint32_t)0x00000000)
#define WWDG_Prescaler_2    ((uint32_t)0x00000080)
#define WWDG_Prescaler_4    ((uint32_t)0x00000100)
#define WWDG_Prescaler_8    ((uint32_t)0x00000180)

#ifdef __cplusplus
};
#endif

#endif /* __CH32V00x_CONF_H */



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CORE_RISCV_H__
#define __CORE_RISCV_H__

/* define compiler specific symbols */
#if defined(__CC_ARM)
  #define __ASM       __asm     /*!< asm keyword for ARM Compiler          */
  #define __INLINE    __inline  /*!< inline keyword for ARM Compiler       */

#elif defined(__ICCARM__)
  #define __ASM       __asm   /*!< asm keyword for IAR Compiler          */
  #define __INLINE    inline  /*!< inline keyword for IAR Compiler. Only avaiable in High optimization mode! */

#elif defined(__GNUC__)
  #define __ASM       __asm   /*!< asm keyword for GNU Compiler          */
  #define __INLINE    inline  /*!< inline keyword for GNU Compiler       */

#elif defined(__TASKING__)
  #define __ASM       __asm   /*!< asm keyword for TASKING Compiler      */
  #define __INLINE    inline  /*!< inline keyword for TASKING Compiler   */

#endif

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef __ASSEMBLER__

/* Standard Peripheral Library old types (maintained for legacy purpose) */
typedef __I uint32_t vuc32;  /* Read Only */
typedef __I uint16_t vuc16;  /* Read Only */
typedef __I uint8_t vuc8;   /* Read Only */

typedef const uint32_t uc32;  /* Read Only */
typedef const uint16_t uc16;  /* Read Only */
typedef const uint8_t uc8;   /* Read Only */

typedef __I int32_t vsc32;  /* Read Only */
typedef __I int16_t vsc16;  /* Read Only */
typedef __I int8_t vsc8;   /* Read Only */

typedef const int32_t sc32;  /* Read Only */
typedef const int16_t sc16;  /* Read Only */
typedef const int8_t sc8;   /* Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

#if defined(CH32V20x) || defined(CH32V30x)
typedef __I uint64_t vuc64;  /* Read Only */
typedef const uint64_t uc64;  /* Read Only */
typedef __I int64_t vsc64;  /* Read Only */
typedef const int64_t sc64;  /* Read Only */
typedef __IO uint64_t  vu64;
typedef uint64_t  u64;
typedef __IO int64_t  vs64;
typedef int64_t  s64;
#endif

typedef enum {NoREADY = 0, READY = !NoREADY} ErrorStatus;

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

#define   RV_STATIC_INLINE  static  inline

/* memory mapped structure for Program Fast Interrupt Controller (PFIC) */
typedef struct{
    __I  uint32_t ISR[8];
    __I  uint32_t IPR[8];
    __IO uint32_t ITHRESDR;
    __IO uint32_t RESERVED;
    __IO uint32_t CFGR;
    __I  uint32_t GISR;
    __IO uint8_t VTFIDR[4];
    uint8_t RESERVED0[12];
    __IO uint32_t VTFADDR[4];
    uint8_t RESERVED1[0x90];
    __O  uint32_t IENR[8];
    uint8_t RESERVED2[0x60];
    __O  uint32_t IRER[8];
    uint8_t RESERVED3[0x60];
    __O  uint32_t IPSR[8];
    uint8_t RESERVED4[0x60];
    __O  uint32_t IPRR[8];
    uint8_t RESERVED5[0x60];
    __IO uint32_t IACTR[8];
    uint8_t RESERVED6[0xE0];
    __IO uint8_t IPRIOR[256];
    uint8_t RESERVED7[0x810];
    __IO uint32_t SCTLR;
}PFIC_Type;

#if defined (CH32V003)

/* memory mapped structure for SysTick */
typedef struct
{
    __IO uint32_t CTLR;
    __IO uint32_t SR;
    __IO uint32_t CNT;
    uint32_t RESERVED0;
    __IO uint32_t CMP;
    uint32_t RESERVED1;
} SysTick_Type;

#elif defined(CH32V20x) || defined(CH32V30x)

/* memory mapped structure for SysTick */
typedef struct
{
	__IO uint32_t CTLR;
	__IO uint32_t SR;
	__IO uint64_t CNT;
	__IO uint64_t CMP;
} SysTick_Type;

#elif defined(CH32X03x)

/* memory mapped structure for SysTick */
typedef struct
{
  __IO uint32_t CTLR;
  __IO uint32_t SR;
  __IO uint32_t CNTL;
  __IO uint32_t CNTH;
  __IO uint32_t CMPL;
  __IO uint32_t CMPH;
} SysTick_Type;

#elif defined(CH32V10x)

/* memory mapped structure for SysTick */
typedef struct
{
  __IO uint32_t CTLR;
  __IO uint32_t CNTL;
  __IO uint32_t CNTH;
  __IO uint32_t CMPL;
  __IO uint32_t CMPH;
} SysTick_Type;

#endif


#endif

#define PFIC            ((PFIC_Type *) PFIC_BASE )
#define NVIC            PFIC
#define NVIC_KEY1       ((uint32_t)0xFA050000)
#define	NVIC_KEY2	    ((uint32_t)0xBCAF0000)
#define	NVIC_KEY3		((uint32_t)0xBEEF0000)


#define SysTick         ((SysTick_Type *) SysTick_BASE)

#ifndef __ASSEMBLER__

/*********************************************************************
 * @fn      __enable_irq
 *
 * @brief   Enable Global Interrupt
 *
 * @return  none
 */
RV_STATIC_INLINE void __enable_irq()
{
  uint32_t result;

    __ASM volatile(
#if __GNUC__ > 10
		".option arch, +zicsr\n"
#endif
		"csrr %0," "mstatus": "=r"(result));
  result |= 0x88;
  __ASM volatile ("csrw mstatus, %0" : : "r" (result) );
}

/*********************************************************************
 * @fn      __disable_irq
 *
 * @brief   Disable Global Interrupt
 *
 * @return  none
 */
RV_STATIC_INLINE void __disable_irq()
{
  uint32_t result;

    __ASM volatile(
#if __GNUC__ > 10
		".option arch, +zicsr\n"
#endif
		"csrr %0," "mstatus": "=r"(result));
  result &= ~0x88;
  __ASM volatile ("csrw mstatus, %0" : : "r" (result) );
}

/*********************************************************************
 * @fn      __isenabled_irq
 *
 * @brief   Is Global Interrupt enabled
 *
 * @return  1: yes, 0: no
 */
RV_STATIC_INLINE uint8_t __isenabled_irq(void)
{
    uint32_t result;

    __ASM volatile(
#if __GNUC__ > 10
    ".option arch, +zicsr\n"
#endif
    "csrr %0," "mstatus": "=r"(result));
    return (result & 0x08) != 0u;
}

/*********************************************************************
 * @fn      __get_cpu_sp
 *
 * @brief   Get stack pointer
 *
 * @return  stack pointer
 */
RV_STATIC_INLINE uint32_t __get_cpu_sp(void);
RV_STATIC_INLINE uint32_t __get_cpu_sp(void)
{
  uint32_t result;

  __ASM volatile(
#if __GNUC__ > 10
    ".option arch, +zicsr\n"
#endif
  "mv %0, sp" : "=r"(result));
  return result;
}

/*********************************************************************
 * @fn      __NOP
 *
 * @brief   nop
 *
 * @return  none
 */
RV_STATIC_INLINE void __NOP()
{
  __ASM volatile ("nop");
}

/*********************************************************************
 * @fn       NVIC_EnableIRQ
 *
 * @brief   Disable Interrupt
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_EnableIRQ(IRQn_Type IRQn)
{
  NVIC->IENR[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
}

/*********************************************************************
 * @fn       NVIC_DisableIRQ
 *
 * @brief   Disable Interrupt
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_DisableIRQ(IRQn_Type IRQn)
{
  NVIC->IRER[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
}

/*********************************************************************
 * @fn       NVIC_GetStatusIRQ
 *
 * @brief   Get Interrupt Enable State
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  1 - 1: Interrupt Pending Enable
 *                0 - Interrupt Pending Disable
 */
RV_STATIC_INLINE uint32_t NVIC_GetStatusIRQ(IRQn_Type IRQn)
{
  return((uint32_t) ((NVIC->ISR[(uint32_t)(IRQn) >> 5] & (1 << ((uint32_t)(IRQn) & 0x1F)))?1:0));
}

/*********************************************************************
 * @fn      NVIC_GetPendingIRQ
 *
 * @brief   Get Interrupt Pending State
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  1 - 1: Interrupt Pending Enable
 *                0 - Interrupt Pending Disable
 */
RV_STATIC_INLINE uint32_t NVIC_GetPendingIRQ(IRQn_Type IRQn)
{
  return((uint32_t) ((NVIC->IPR[(uint32_t)(IRQn) >> 5] & (1 << ((uint32_t)(IRQn) & 0x1F)))?1:0));
}

/*********************************************************************
 * @fn      NVIC_SetPendingIRQ
 *
 * @brief   Set Interrupt Pending
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_SetPendingIRQ(IRQn_Type IRQn)
{
  NVIC->IPSR[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
}

/*********************************************************************
 * @fn      NVIC_ClearPendingIRQ
 *
 * @brief   Clear Interrupt Pending
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_ClearPendingIRQ(IRQn_Type IRQn)
{
  NVIC->IPRR[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F));
}

/*********************************************************************
 * @fn      NVIC_GetActive
 *
 * @brief   Get Interrupt Active State
 *
 * @param   IRQn - Interrupt Numbers
 *
 * @return  1 - Interrupt Active
 *                0 - Interrupt No Active
 */
RV_STATIC_INLINE uint32_t NVIC_GetActive(IRQn_Type IRQn)
{
  return((uint32_t)((NVIC->IACTR[(uint32_t)(IRQn) >> 5] & (1 << ((uint32_t)(IRQn) & 0x1F)))?1:0));
}

/*********************************************************************
 * @fn      NVIC_SetPriority
 *
 * @brief   Set Interrupt Priority
 *
 * @param   IRQn - Interrupt Numbers
 *          priority: bit7 - pre-emption priority
 *                    bit6 - subpriority
 *                    bit[5-0] - reserved
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_SetPriority(IRQn_Type IRQn, uint8_t priority)
{
  NVIC->IPRIOR[(uint32_t)(IRQn)] = priority;
}

/*********************************************************************
 * SUSPEND ALL INTERRUPTS EXCEPT
 * The following 3 functions serve to suspend all interrupts, except for the one you momentarily need.
 * The purpose of this is to not disturb the one interrupt of interest and let it run unimpeded.
 * procedure:
 * 1. save the enabled IRQs: uint32_t IRQ_backup = NVIC_get_enabled_IRQs();
 * 2. disable all IRQs: NVIC_clear_all_IRQs_except(IRQ_of_interest);
 * 3. restore the previously enabled IRQs: NVIC_restore_IRQs(IRQ_backup);
 *
 * bit layout of the IRQ backup
 * bit		0 | 1 | 2  |  3  | 4  |  5  | 6  .. 22 | 23 .. 28
 * IRQn		2 | 3 | 12 | res | 14 | res | 16 .. 31 | 32 .. 38
 * IRQn 2 and 3 aren't actually user-settable (see RM).
 *
 * Specifying an invalid IRQn_to_keep like 0 will disable all interrupts.
 */

RV_STATIC_INLINE uint32_t NVIC_get_enabled_IRQs()
{
	return ( ((NVIC->ISR[0] >> 2) & 0b11) | ((NVIC->ISR[0] >> 12) << 2) | ((NVIC->ISR[1] & 0b1111111) << 23) );
}

RV_STATIC_INLINE void NVIC_clear_all_IRQs_except(uint8_t IRQn_to_keep)
{
	if (!(IRQn_to_keep >> 5)) {		// IRQn_to_keep < 32
		NVIC->IRER[0] = (~0) & (~(1 << IRQn_to_keep));
		NVIC->IRER[1] = (~0);
	}
	else {
		IRQn_to_keep = IRQn_to_keep >> 5;
		NVIC->IRER[0] = (~0);
		NVIC->IRER[1] = (~0) & (~(1 << IRQn_to_keep));
	}
}

RV_STATIC_INLINE void NVIC_restore_IRQs(uint32_t old_state)
{
	NVIC->IENR[0] = (old_state >> 2) << 12;
	NVIC->IENR[1] = old_state >> 23;
}

/*********************************************************************
 * @fn       __WFI
 *
 * @brief   Wait for Interrupt
 *
 * @return  none
 */
__attribute__( ( always_inline ) ) RV_STATIC_INLINE void __WFI(void)
{
  NVIC->SCTLR &= ~(1<<3);   // wfi
  __ASM volatile ("wfi");
}

/*********************************************************************
 * @fn       __WFE
 *
 * @brief   Wait for Events
 *
 * @return  none
 */
__attribute__( ( always_inline ) ) RV_STATIC_INLINE void __WFE(void)
{
  uint32_t t;

  t = NVIC->SCTLR;
  NVIC->SCTLR |= (1<<3)|(1<<5);     // (wfi->wfe)+(__sev)
  NVIC->SCTLR = (NVIC->SCTLR & ~(1<<5)) | ( t & (1<<5));
  __ASM volatile ("wfi");
  __ASM volatile ("wfi");
}

/*********************************************************************
 * @fn      SetVTFIRQ
 *
 * @brief   Set VTF Interrupt
 *
 * @param   addr - VTF interrupt service function base address.
 *                  IRQn - Interrupt Numbers
 *                  num - VTF Interrupt Numbers
 *                  NewState -  DISABLE or ENABLE
 *
 * @return  none
 */
RV_STATIC_INLINE void SetVTFIRQ(uint32_t addr, IRQn_Type IRQn, uint8_t num, FunctionalState NewState){
  if(num > 1)  return ;

  if (NewState != DISABLE)
  {
      NVIC->VTFIDR[num] = IRQn;
      NVIC->VTFADDR[num] = ((addr&0xFFFFFFFE)|0x1);
  }
  else{
      NVIC->VTFIDR[num] = IRQn;
      NVIC->VTFADDR[num] = ((addr&0xFFFFFFFE)&(~0x1));
  }
}

/*********************************************************************
 * @fn       NVIC_SystemReset
 *
 * @brief   Initiate a system reset request
 *
 * @return  none
 */
RV_STATIC_INLINE void NVIC_SystemReset(void)
{
  NVIC->CFGR = NVIC_KEY3|(1<<7);
}

// For configuring INTSYSCR, for interrupt nesting + hardware stack enable.
static inline uint32_t __get_INTSYSCR(void)
{
    uint32_t result;
    __ASM volatile("csrr %0, 0x804": "=r"(result));
    return (result);
}

static inline void __set_INTSYSCR( uint32_t value )
{
    __ASM volatile("csrw 0x804, %0" : : "r"(value));
}

#if defined(CH32V30x)
/*********************************************************************
 * @fn      __get_FFLAGS
 *
 * @brief   Return the Floating-Point Accrued Exceptions
 *
 * @return  fflags value
 */
static inline uint32_t __get_FFLAGS(void)
{
  uint32_t result;

  __ASM volatile ( "csrr %0," "fflags" : "=r" (result) );
  return (result);
}

/*********************************************************************
 * @fn      __set_FFLAGS
 *
 * @brief   Set the Floating-Point Accrued Exceptions
 *
 * @param   value  - set FFLAGS value
 *
 * @return  none
 */
static inline void __set_FFLAGS(uint32_t value)
{
  __ASM volatile ("csrw fflags, %0" : : "r" (value) );
}

/*********************************************************************
 * @fn      __get_FRM
 *
 * @brief   Return the Floating-Point Dynamic Rounding Mode
 *
 * @return  frm value
 */
static inline uint32_t __get_FRM(void)
{
  uint32_t result;

  __ASM volatile ( "csrr %0," "frm" : "=r" (result) );
  return (result);
}

/*********************************************************************
 * @fn      __set_FRM
 *
 * @brief   Set the Floating-Point Dynamic Rounding Mode
 *
 * @param   value  - set frm value
 *
 * @return  none
 */
static inline void __set_FRM(uint32_t value)
{
  __ASM volatile ("csrw frm, %0" : : "r" (value) );
}

/*********************************************************************
 * @fn      __get_FCSR
 *
 * @brief   Return the Floating-Point Control and Status Register
 *
 * @return  fcsr value
 */
static inline uint32_t __get_FCSR(void)
{
  uint32_t result;

  __ASM volatile ( "csrr %0," "fcsr" : "=r" (result) );
  return (result);
}

/*********************************************************************
 * @fn      __set_FCSR
 *
 * @brief   Set the Floating-Point Dynamic Rounding Mode
 *
 * @param   value  - set fcsr value
 *
 * @return  none
 */
static inline void __set_FCSR(uint32_t value)
{
  __ASM volatile ("csrw fcsr, %0" : : "r" (value) );
}
#endif

/*********************************************************************
 * @fn      __get_MSTATUS
 *
 * @brief   Return the Machine Status Register
 *
 * @return  mstatus value
 */
static inline uint32_t __get_MSTATUS(void)
{
    uint32_t result;

    __ASM volatile("csrr %0," "mstatus": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MSTATUS
 *
 * @brief   Set the Machine Status Register
 *
 * @param   value  - set mstatus value
 *
 * @return  none
 */
static inline void __set_MSTATUS(uint32_t value)
{
    __ASM volatile("csrw mstatus, %0" : : "r"(value));
}

/*********************************************************************
 * @fn      __get_MISA
 *
 * @brief   Return the Machine ISA Register
 *
 * @return  misa value
 */
static inline uint32_t __get_MISA(void)
{
    uint32_t result;

    __ASM volatile("csrr %0,""misa" : "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MISA
 *
 * @brief   Set the Machine ISA Register
 *
 * @param   value  - set misa value
 *
 * @return  none
 */
static inline void __set_MISA(uint32_t value)
{
    __ASM volatile("csrw misa, %0" : : "r"(value));
}

/*********************************************************************
 * @fn      __get_MTVEC
 *
 * @brief   Return the Machine Trap-Vector Base-Address Register
 *
 * @return  mtvec value
 */
static inline uint32_t __get_MTVEC(void)
{
    uint32_t result;

    __ASM volatile("csrr %0," "mtvec": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MTVEC
 *
 * @brief   Set the Machine Trap-Vector Base-Address Register
 *
 * @param   value  - set mtvec value
 *
 * @return  none
 */
static inline void __set_MTVEC(uint32_t value)
{
    __ASM volatile("csrw mtvec, %0":: "r"(value));
}

/*********************************************************************
 * @fn      __get_MSCRATCH
 *
 * @brief   Return the Machine Seratch Register
 *
 * @return  mscratch value
 */
static inline uint32_t __get_MSCRATCH(void)
{
    uint32_t result;

    __ASM volatile("csrr %0," "mscratch" : "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MSCRATCH
 *
 * @brief   Set the Machine Seratch Register
 *
 * @param   value  - set mscratch value
 *
 * @return  none
 */
static inline void __set_MSCRATCH(uint32_t value)
{
    __ASM volatile("csrw mscratch, %0" : : "r"(value));
}

/*********************************************************************
 * @fn      __get_MEPC
 *
 * @brief   Return the Machine Exception Program Register
 *
 * @return  mepc value
 */
static inline uint32_t __get_MEPC(void)
{
    uint32_t result;

    __ASM volatile("csrr %0," "mepc" : "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MEPC
 *
 * @brief   Set the Machine Exception Program Register
 *
 * @return  mepc value
 */
static inline void __set_MEPC(uint32_t value)
{
    __ASM volatile("csrw mepc, %0" : : "r"(value));
}

/*********************************************************************
 * @fn      __get_MCAUSE
 *
 * @brief   Return the Machine Cause Register
 *
 * @return  mcause value
 */
static inline uint32_t __get_MCAUSE(void)
{
    uint32_t result;

    __ASM volatile("csrr %0," "mcause": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __set_MCAUSE
 *
 * @brief   Set the Machine Cause Register
 *
 * @return  mcause value
 */
static inline void __set_MCAUSE(uint32_t value)
{
    __ASM volatile("csrw mcause, %0":: "r"(value));
}

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)

/*********************************************************************
 * @fn      __get_MTVAL
 *
 * @brief   Return the Machine Trap Value Register
 *
 * @return  mtval value
 */
static inline uint32_t __get_MTVAL(void)
{
	uint32_t result;

	__ASM volatile ( "csrr %0," "mtval" : "=r" (result) );
	return (result);
}

/*********************************************************************
 * @fn      __set_MTVAL
 *
 * @brief   Set the Machine Trap Value Register
 *
 * @return  mtval value
 */
static inline void __set_MTVAL(uint32_t value)
{
	__ASM volatile ("csrw mtval, %0" : : "r" (value) );
}

#endif

/*********************************************************************
 * @fn      __get_MVENDORID
 *
 * @brief   Return Vendor ID Register
 *
 * @return  mvendorid value
 */
static inline uint32_t __get_MVENDORID(void)
{
    uint32_t result;

    __ASM volatile("csrr %0,""mvendorid": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __get_MARCHID
 *
 * @brief   Return Machine Architecture ID Register
 *
 * @return  marchid value
 */
static inline uint32_t __get_MARCHID(void)
{
    uint32_t result;

    __ASM volatile("csrr %0,""marchid": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __get_MIMPID
 *
 * @brief   Return Machine Implementation ID Register
 *
 * @return  mimpid value
 */
static inline uint32_t __get_MIMPID(void)
{
    uint32_t result;

    __ASM volatile("csrr %0,""mimpid": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __get_MHARTID
 *
 * @brief   Return Hart ID Register
 *
 * @return  mhartid value
 */
static inline uint32_t __get_MHARTID(void)
{
    uint32_t result;

    __ASM volatile("csrr %0,""mhartid": "=r"(result));
    return (result);
}

/*********************************************************************
 * @fn      __get_SP
 *
 * @brief   Return SP Register
 *
 * @return  SP value
 */
static inline uint32_t __get_SP(void)
{
    uint32_t result;

    __ASM volatile("mv %0,""sp": "=r"(result):);
    return (result);
}

// Depending on a LOT of factors, it's about 6 cycles per n.
// **DO NOT send it zero or less.**
#ifndef __MACOSX__
static inline void Delay_Tiny( int n ) {
	__ASM volatile( "\
		mv a5, %[n]\n\
		1: \
		c.addi a5, -1\n\
		c.bnez a5, 1b" : : [n]"r"(n) : "a5" );
}
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif/* __CORE_RISCV_H__ */


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/* SYSTICK info
 * time on the ch32v003 is kept by the SysTick counter (32bit)
 * by default, it will operate at (FUNCONF_SYSTEM_CORE_CLOCK / 8) = 6MHz
 * more info at https://github.com/cnlohr/ch32v003fun/wiki/Time
*/

#if defined( FUNCONF_SYSTICK_USE_HCLK ) && FUNCONF_SYSTICK_USE_HCLK && !defined(CH32V10x)
#define DELAY_US_TIME ((FUNCONF_SYSTEM_CORE_CLOCK)/1000000)
#define DELAY_MS_TIME ((FUNCONF_SYSTEM_CORE_CLOCK)/1000)
#else // Use systick = hclk/8
#define DELAY_US_TIME ((FUNCONF_SYSTEM_CORE_CLOCK)/8000000)
#define DELAY_MS_TIME ((FUNCONF_SYSTEM_CORE_CLOCK)/8000)
#endif

#define Delay_Us(n) DelaySysTick( (n) * DELAY_US_TIME )
#define Delay_Ms(n) DelaySysTick( (n) * DELAY_MS_TIME )

#define Ticks_from_Us(n)	(n * DELAY_US_TIME)
#define Ticks_from_Ms(n)	(n * DELAY_MS_TIME)

// Add a certain number of nops.  Note: These are usually executed in pairs
// and take two cycles, so you typically would use 0, 2, 4, etc.
#define ADD_N_NOPS( n ) asm volatile( ".rept " #n "\nc.nop\n.endr" );

// Arduino-like GPIO Functionality
#define GpioOf( pin ) ((GPIO_TypeDef *)(GPIOA_BASE + 0x400 * ((pin)>>4)))

#define FUN_HIGH 0x1
#define FUN_LOW 0x0
#define FUN_OUTPUT (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)
#define FUN_INPUT (GPIO_CNF_IN_FLOATING)

#define PA1 1
#define PA2 2
#define PC0 32
#define PC1 33
#define PC2 34
#define PC3 35
#define PC4 36
#define PC5 37
#define PC6 38
#define PC7 39
#define PD0 48
#define PD1 49
#define PD2 50
#define PD3 51
#define PD4 52
#define PD5 53
#define PD6 54
#define PD7 55

#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x) || defined( CH32X03x )
#define PA0 0
#define PA3 3
#define PA4 4
#define PA5 5
#define PA6 6
#define PA7 7
#define PA8 8
#define PA9 9
#define PA10 10
#define PA11 11
#define PA12 12
#define PA13 13
#define PA14 14
#define PA15 15
#define PB0 16
#define PB1 17
#define PB2 18
#define PB3 19
#define PB4 20
#define PB5 21
#define PB6 22
#define PB7 23
#define PB8 24
#define PB9 25
#define PB10 26
#define PB11 27
#define PB12 28
#define PB13 29
#define PB14 30
#define PB15 31
#define PC8 40
#define PC9 41
#define PC10 42
#define PC11 43
#define PC12 44
#define PC13 45
#define PC14 46
#define PC15 47
#endif


// For pins, use things like PA8, PB15
// For configuration, use things like GPIO_CFGLR_OUT_10Mhz_PP

#define funDigitalWrite( pin, value ) { GpioOf( pin )->BSHR = 1<<((!(value))*16 + ((pin) & 0xf)); }

#if defined(CH32X03x)
#define funGpioInitAll() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC ); }
#define funPinMode( pin, mode ) { *((&GpioOf(pin)->CFGLR)+((pin&0x8)>>3)) = ( (*((&GpioOf(pin)->CFGLR)+((pin&0x8)>>3))) & (~(0xf<<(4*((pin)&0x7))))) | ((mode)<<(4*((pin)&0x7))); }
#elif defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
#define funGpioInitAll() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD ); }
#define funPinMode( pin, mode ) { *((&GpioOf(pin)->CFGLR)+((pin&0x8)>>3)) = ( (*((&GpioOf(pin)->CFGLR)+((pin&0x8)>>3))) & (~(0xf<<(4*((pin)&0x7))))) | ((mode)<<(4*((pin)&0x7))); }
#define funGpioInitB() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB ); }
#else
#define funGpioInitAll() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD ); }
#define funPinMode( pin, mode ) { GpioOf(pin)->CFGLR = (GpioOf(pin)->CFGLR & (~(0xf<<(4*((pin)&0xf))))) | ((mode)<<(4*((pin)&0xf))); }
#endif

#define funGpioInitA() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA ); }
#define funGpioInitC() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC ); }
#define funGpioInitD() { RCC->APB2PCENR |= ( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD ); }
#define funDigitalRead( pin ) ((GpioOf(pin)->INDR >> ((pin)&0xf)) & 1)



#if defined(__riscv) || defined(__riscv__) || defined( CH32V003FUN_BASE )

// Stuff that can only be compiled on device (not for the programmer, or other host programs)

#ifndef __ASSEMBLER__
void handle_reset()            __attribute__((naked)) __attribute((section(".text.handle_reset"))) __attribute__((used));
void DefaultIRQHandler( void ) __attribute__((section(".text.vector_handler"))) __attribute__((naked)) __attribute__((used));
// used to clear the CSS flag in case of clock fail switch
#if defined(FUNCONF_USE_CLK_SEC) && FUNCONF_USE_CLK_SEC
	void NMI_RCC_CSS_IRQHandler( void ) __attribute__((section(".text.vector_handler"))) __attribute__((naked)) __attribute__((used));
#endif
#endif

// For debug writing to the debug interface.
#if defined(CH32V003)
	#define DMDATA0 ((volatile uint32_t*)0xe00000f4)
	#define DMDATA1 ((volatile uint32_t*)0xe00000f8)
#else
	#define DMDATA0 ((volatile uint32_t*)0xe0000380)
	#define DMDATA1 ((volatile uint32_t*)0xe0000384)
#endif

#endif

// Determination of PLL multiplication factor for non-V003 chips
#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x)
	#if !defined(FUNCONF_SYSTEM_CORE_CLOCK)
		#define PLL_MULTIPLICATION ((uint32_t)0)
	#else
		#if defined(CH32V10x) || defined(CH32V20x) || defined(CH32V30x_D8)
			#if FUNCONF_PLL_MULTIPLIER == 2
				#define PLL_MULTIPLICATION RCC_PLLMULL2
			#elif FUNCONF_PLL_MULTIPLIER == 3
				#define PLL_MULTIPLICATION RCC_PLLMULL3
			#elif FUNCONF_PLL_MULTIPLIER == 4
				#define PLL_MULTIPLICATION RCC_PLLMULL4
			#elif FUNCONF_PLL_MULTIPLIER == 5
				#define PLL_MULTIPLICATION RCC_PLLMULL5
			#elif FUNCONF_PLL_MULTIPLIER == 6
				#define PLL_MULTIPLICATION RCC_PLLMULL6
			#elif FUNCONF_PLL_MULTIPLIER == 7
				#define PLL_MULTIPLICATION RCC_PLLMULL7
			#elif FUNCONF_PLL_MULTIPLIER == 8
				#define PLL_MULTIPLICATION RCC_PLLMULL8
			#elif FUNCONF_PLL_MULTIPLIER == 9
				#define PLL_MULTIPLICATION RCC_PLLMULL9
			#elif FUNCONF_PLL_MULTIPLIER == 10
				#define PLL_MULTIPLICATION RCC_PLLMULL10
			#elif FUNCONF_PLL_MULTIPLIER == 11
				#define PLL_MULTIPLICATION RCC_PLLMULL11
			#elif FUNCONF_PLL_MULTIPLIER == 12
				#define PLL_MULTIPLICATION RCC_PLLMULL12
			#elif FUNCONF_PLL_MULTIPLIER == 13
				#define PLL_MULTIPLICATION RCC_PLLMULL13
			#elif FUNCONF_PLL_MULTIPLIER == 14
				#define PLL_MULTIPLICATION RCC_PLLMULL14
			#elif FUNCONF_PLL_MULTIPLIER == 15
				#define PLL_MULTIPLICATION RCC_PLLMULL15
			#elif FUNCONF_PLL_MULTIPLIER == 16
				#define PLL_MULTIPLICATION RCC_PLLMULL16
			#elif defined(CH32V20x) && FUNCONF_PLL_MULTIPLIER == 18
				#define PLL_MULTIPLICATION RCC_PLLMULL18
			#else
				#error "Invalid PLL multiplier"
			#endif
		#else
			#if FUNCONF_PLL_MULTIPLIER == 3
				#define PLL_MULTIPLICATION RCC_PLLMULL3_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 4
				#define PLL_MULTIPLICATION RCC_PLLMULL4_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 5
				#define PLL_MULTIPLICATION RCC_PLLMULL5_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 6
				#define PLL_MULTIPLICATION RCC_PLLMULL6_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 7
				#define PLL_MULTIPLICATION RCC_PLLMULL7_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 8
				#define PLL_MULTIPLICATION RCC_PLLMULL8_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 9
				#define PLL_MULTIPLICATION RCC_PLLMULL9_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 10
				#define PLL_MULTIPLICATION RCC_PLLMULL10_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 11
				#define PLL_MULTIPLICATION RCC_PLLMULL11_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 12
				#define PLL_MULTIPLICATION RCC_PLLMULL12_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 13
				#define PLL_MULTIPLICATION RCC_PLLMULL13_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 14
				#define PLL_MULTIPLICATION RCC_PLLMULL14_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 15
				#define PLL_MULTIPLICATION RCC_PLLMULL15_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 16
				#define PLL_MULTIPLICATION RCC_PLLMULL16_EXTEN
			#elif FUNCONF_PLL_MULTIPLIER == 18
				#define PLL_MULTIPLICATION RCC_PLLMULL18_EXTEN
			#else
				#error "Invalid PLL multiplier"
			#endif
		#endif
	#endif
#endif

#ifndef __ASSEMBLER__

void DelaySysTick( uint32_t n );

// Tricky: We need to make sure main and SystemInit() are preserved.
int main() __attribute__((used));
void SystemInit(void);

#ifdef FUNCONF_UART_PRINTF_BAUD
	#define UART_BAUD_RATE FUNCONF_UART_PRINTF_BAUD
#else
	#define UART_BAUD_RATE 115200
#endif
// Debug UART baud rate register calculation. Works assuming HCLK prescaler is off.
// Computes UART_BRR = CORE_CLOCK / BAUD_RATE with rounding to closest integer
#define UART_BRR (((FUNCONF_SYSTEM_CORE_CLOCK) + (UART_BAUD_RATE)/2) / (UART_BAUD_RATE))
// Put an output debug UART on Pin D5.
// You can write to this with printf(...) or puts(...)

void SetupUART( int uartBRR );

void WaitForDebuggerToAttach();

// Just a definition to the internal _write function.
int _write(int fd, const char *buf, int size);

// Call this to busy-wait the polling of input.
void poll_input();

// Receiving bytes from host.  Override if you wish.
void handle_debug_input( int numbytes, uint8_t * data );

#endif

#ifdef CH32V003 // CH32V003-only

// xw_ext.inc, thanks to @macyler, @jnk0le, @duk for this reverse engineering.

/*
Encoder for some of the proprietary 'XW' RISC-V instructions present on the QingKe RV32 processor.
Examples:
	XW_C_LBU(a3, a1, 27); // c.xw.lbu a3, 27(a1)
	XW_C_SB(a0, s0, 13);  // c.xw.sb a0, 13(s0)

	XW_C_LHU(a5, a5, 38); // c.xw.lhu a5, 38(a5)
	XW_C_SH(a2, s1, 14);  // c.xw.sh a2, 14(s1)
*/

// Let us do some compile-time error checking.
#define ASM_ASSERT(COND) .if (!(COND)); .err; .endif

// Integer encodings of the possible compressed registers.
#define C_s0 0
#define C_s1 1
#define C_a0 2
#define C_a1 3
#define C_a2 4
#define C_a3 5
#define C_a4 6
#define C_a5 7

// register to encoding
#define REG2I(X) (C_ ## X)

// XW opcodes
#define XW_OP_LBUSP 0b1000000000000000
#define XW_OP_STSP  0b1000000001000000

#define XW_OP_LHUSP 0b1000000000100000
#define XW_OP_SHSP  0b1000000001100000

#define XW_OP_LBU   0b0010000000000000
#define XW_OP_SB    0b1010000000000000

#define XW_OP_LHU   0b0010000000000010
#define XW_OP_SH    0b1010000000000010

// The two different XW encodings supported at the moment.
#define XW_ENCODE1(OP, R1, R2, IMM) ASM_ASSERT((IMM) >= 0 && (IMM) < 32); .2byte ((OP) | (REG2I(R1) << 2) | (REG2I(R2) << 7) | \
	(((IMM) & 0b1) << 12) | (((IMM) & 0b110) << (5 - 1)) | (((IMM) & 0b11000) << (10 - 3)))

#define XW_ENCODE2(OP, R1, R2, IMM) ASM_ASSERT((IMM) >= 0 && (IMM) < 32); .2byte ((OP) | (REG2I(R1) << 2) | (REG2I(R2) << 7) | \
	(((IMM) & 0b11) << 5) | (((IMM) & 0b11100) << (10 - 2))

// Compressed load byte, zero-extend result
#define XW_C_LBU(RD, RS, IMM) XW_ENCODE1(XW_OP_LBU, RD, RS, IMM)

// Compressed store byte
#define XW_C_SB(RS1, RS2, IMM) XW_ENCODE1(XW_OP_SB, RS1, RS2, IMM)

// Compressed load half, zero-extend result
#define XW_C_LHU(RD, RS, IMM) ASM_ASSERT(((IMM) & 1) == 0); XW_ENCODE2(XW_OP_LHU, RD, RS, ((IMM) >> 1)))

// Compressed store half
#define XW_C_SH(RS1, RS2, IMM)  ASM_ASSERT(((IMM) & 1) == 0); XW_ENCODE2(XW_OP_SH, RS1, RS2, ((IMM) >> 1)))

#endif

/* Copyright notice from original EVT.
 ********************************** (C) COPYRIGHT  *******************************
 * File Name          : core_riscv.h + ch32v00x.h
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/08/08
 * Description        : RISC-V Core Peripheral Access Layer Header File
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/


#ifdef __cplusplus
};
#endif
