/**************************************************************************//**
 * @file     KB1200.h
 * @brief    CMSIS Core Peripheral Access Layer Header File for
 *           KB1200 Device
 * @version  V1.0.0
 * @date     02. July 2023
 ******************************************************************************/

#ifndef KB1200_H
#define KB1200_H

#ifdef __cplusplus
extern "C" {
#endif


/* -------------------------  Interrupt Number Definition  ------------------------ */

//typedef 
enum IRQn
{
/* -------------------  Processor Exceptions Numbers  ----------------------------- */
//  NonMaskableInt_IRQn           = -14,     /*  2 Non Maskable Interrupt */
//  HardFault_IRQn                = -13,     /*  3 HardFault Interrupt */
//  MemoryManagement_IRQn         = -12,     /*  4 Memory Management Interrupt */
//  BusFault_IRQn                 = -11,     /*  5 Bus Fault Interrupt */
//  UsageFault_IRQn               = -10,     /*  6 Usage Fault Interrupt */
//  SVCall_IRQn                   =  -5,     /* 11 SV Call Interrupt */
//  DebugMonitor_IRQn             =  -4,     /* 12 Debug Monitor Interrupt */
//  PendSV_IRQn                   =  -2,     /* 14 Pend SV Interrupt */
//  SysTick_IRQn                  =  -1,     /* 15 System Tick Interrupt */

/* -------------------  Processor Interrupt Numbers  ------------------------------ */
  WDT_IRQn                      =   0,
  POWERLOST_IRQn                =   1,
  GPIO0xE_IRQn                  =   2,
  GPIO1x_IRQn                   =   3,
  GPIO2x_IRQn                   =   4,
  GPIO3x_IRQn                   =   5,
  GPIO4x_IRQn                   =   6,
  GPIO5x_IRQn                   =   7,
  GPIO6x_IRQn                   =   8,
  GPIO7x_IRQn                   =   9,
  HBNT_IRQn                     =  10,
  MTC_IRQn                      =  11,
  ITIM_IRQn                     =  12,
  GPT_IRQn                      =  13,
  OMST_IRQn                     =  14,
  TACHO_IRQn                    =  15,
  IKB_IRQn                      =  16,
  FSMBM_IRQn                    =  17,
  I2CS_IRQn                     =  18,
  I2CD32_IRQn                   =  19,
  SER_IRQn                      =  20,
  PS2_IRQn                      =  21,
  PECI_IRQn                     =  22,
  UART_IRQn                     =  23,
  DMA_IRQn                      =  24,
  EXTCMD_IRQn                   =  25,
  KBC_IRQn                      =  26,
  ECI_IRQn                      =  27,
  DBI_IRQn                      =  28,
  LEGI_IRQn                     =  29,
  MBX_IRQn                      =  30,
  MEMS_IOS_IRQn                 =  31,
  ESPICH1_IRQn                  =  32,
  ESPICH2_IRQn                  =  33,
  ESPICH3_IRQn                  =  34,
  SPIS_IRQn                     =  35,
  SA_IRQn                       =  36,
  /* Interrupts 10 .. 224 are left out */
}; // IRQn_Type;


/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* -------  Start of section using anonymous unions and disabling warnings  ------- */
#if   defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


/* --------  Configuration of Core Peripherals  ----------------------------------- */
//#define __CM4_REV                 0x0001U   /* Core revision r0p1 */
//#define __MPU_PRESENT             1U        /* MPU present */
//#define __VTOR_PRESENT            1U        /* VTOR present */
//#define __NVIC_PRIO_BITS          3U        /* Number of Bits used for Priority Levels */
//#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */
//#define __FPU_PRESENT             1U        /* FPU present */

//#include "core_cm4.h"                       /* Processor and core peripherals */
//#include "system_kb1200.h"                  /* System Header */

/* Flash and SRAM, Peripheral base address */
//#define FLASH_BASE          (0x00000000UL)
//#define SRAM_BASE           (0x20000000UL)
#define AHB_P_BASE          (0x50000000UL)  
#define APB_P_BASE          (0x40000000UL) 
/* APB Peripherals address */
#define GCFG_BASE           (0x40000000UL)
#define PMU_BASE            (0x40010000UL)
#define AFAN_BASE           (0x40030000UL)
#define VCC0_BASE           (0x40040000UL)
#define HBNT_BASE           (0x40040040UL)
#define VBAT_BASE           (0x40041000UL)
#define MTC_BASE            (0x400410A0UL)
#define WDT_BASE            (0x40060000UL)
#define GPT_BASE            (0x40070000UL)
#define ITIM_BASE           (0x40080000UL)
#define OMST_BASE           (0x40090000UL)
#define TACHO_BASE          (0x40100000UL)
#define IKB_BASE            (0x40110000UL)
#define VC_BASE             (0x40120000UL)
#define ADC_BASE            (0x40130000UL)
#define FANPWM_BASE         (0x40200000UL)
#define PWM_BASE            (0x40210000UL)
#define PWMLED_BASE         (0x40220000UL)
//#define DAC_BASE            (0x40230000UL)
#define PS2_BASE            (0x40300000UL)
#define SER_BASE            (0x40310000UL)
#define SER1_BASE           (0x40310000UL)
#define SER2_BASE           (0x40310020UL)
#define SER3_BASE           (0x40310040UL)
#define SPIH_BASE           (0x40320000UL)
#define PECI_BASE           (0x40330000UL)
#define FSMBM0_BASE         (0x40340000UL)
#define FSMBM1_BASE         (0x40341000UL)
#define FSMBM2_BASE         (0x40342000UL)
#define FSMBM3_BASE         (0x40343000UL)
#define FSMBM4_BASE         (0x40344000UL)
#define FSMBM5_BASE         (0x40345000UL)
#define FSMBM6_BASE         (0x40346000UL)
#define FSMBM7_BASE         (0x40347000UL)
#define FSMBM8_BASE         (0x40348000UL)
#define FSMBM9_BASE         (0x40349000UL)
#define I2CS0_BASE          (0x40350000UL)
#define I2CS1_BASE          (0x40351000UL)
#define I2CS2_BASE          (0x40352000UL)
#define I2CS3_BASE          (0x40353000UL)
#define OTP_BASE            (0x40360000UL)
//#define ESB_BASE            (0x40360000UL)
//#define ESB_00FF_BASE       (0x40370000UL)
//#define ESB_B0BF_BASE       (0x403800B0UL)
//#define ESB_C0CF_BASE       (0x403900C0UL)
//#define CIR_BASE            (0x403A0000UL)
//#define OWM_BASE            (0x403B0000UL)
/* AHB Peripherals address */
#define GPIO_BASE           (0x50000000UL)
//#define EGPIO_BASE          (0x50000100UL)
#define GPTD_BASE           (0x50010000UL)
//#define EGPTD_BASE          (0x50010100UL)
//#define BNA_BASE            (0x50020000UL)
//#define SHA_BASE            (0x50030000UL)
#define SA_BASE             (0x50020000UL)
#define XBI_BASE            (0x50100000UL)
//#define EFP_BASE            (0x50101000UL)
#define ISPI_BASE           (0x50101000UL)
#define ISPI_BUF_BASE       (0x50101100UL)
#define ISPIP_BASE          (0x50101200UL)
#define DMA_BASE            (0x50110000UL)
#define EDI32_BASE          (0x50120000UL)
#define I2CD32_BASE         (0x50130000UL)
#define SPIS_BASE           (0x50150000UL)
//#define USB_BASE            (0x50150000UL)
//#define USBEP1_BASE         (0x50150100UL)
//#define USBEP2_BASE         (0x50150200UL)
//#define USBEP3_BASE         (0x50150300UL)
//#define USBEP4_BASE         (0x50150400UL)
#define ESPI_BASE           (0x50200000UL)
#define ESPIPHER_BASE       (0x50201000UL)
#define ESPIVW_BASE         (0x50202000UL)
#define ESPIOOB_BASE        (0x50203000UL)
#define ESPIFA_BASE         (0x50204000UL)
#define HIF_BASE            (0x50210000UL)
#define KBC_BASE            (0x50211000UL)
#define ECI_BASE            (0x50212000UL)
#define LEGI0_BASE          (0x50213000UL)
#define LEGI1_BASE          (0x50213020UL)
#define DBI0_BASE           (0x50214000UL)
#define DBI1_BASE           (0x50214020UL)
#define MBX1_BASE           (0x50215000UL)
#define MBX2_BASE           (0x50215100UL)
#define UART_BASE           (0x50216000UL)

/* --------  End of section using anonymous unions and disabling warnings  -------- */
#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif


#ifdef __cplusplus
}
#endif

#endif  /* KB1200_H */
