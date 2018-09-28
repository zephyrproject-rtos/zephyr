/***************************************************************************//**
* \file cy_device_common.h
*
* \brief
* This file provides types and common device definitions that do not changed
* between different products.
*
********************************************************************************
* \copyright
* Copyright 2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CY_DEVICE_COMMON_H_
#define _CY_DEVICE_COMMON_H_

#include <stdint.h>

/*******************************************************************************
*                         Interrupt Number Definition
*******************************************************************************/

typedef enum {
#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))
  /* ARM Cortex-M0+ Core Interrupt Numbers */
  Reset_IRQn                        = -15,      /*!< -15 Reset Vector, invoked on Power up and warm reset */
  NonMaskableInt_IRQn               = -14,      /*!< -14 Non maskable Interrupt, cannot be stopped or preempted */
  HardFault_IRQn                    = -13,      /*!< -13 Hard Fault, all classes of Fault */
  SVCall_IRQn                       =  -5,      /*!<  -5 System Service Call via SVC instruction */
  PendSV_IRQn                       =  -2,      /*!<  -2 Pendable request for system service */
  SysTick_IRQn                      =  -1      /*!<  -1 System Tick Timer */

#else
  /* ARM Cortex-M4 Core Interrupt Numbers */
  Reset_IRQn                        = -15,      /*!< -15 Reset Vector, invoked on Power up and warm reset */
  NonMaskableInt_IRQn               = -14,      /*!< -14 Non maskable Interrupt, cannot be stopped or preempted */
  HardFault_IRQn                    = -13,      /*!< -13 Hard Fault, all classes of Fault */
  MemoryManagement_IRQn             = -12,      /*!< -12 Memory Management, MPU mismatch, including Access Violation and No Match */
  BusFault_IRQn                     = -11,      /*!< -11 Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault */
  UsageFault_IRQn                   = -10,      /*!< -10 Usage Fault, i.e. Undef Instruction, Illegal State Transition */
  SVCall_IRQn                       =  -5,      /*!<  -5 System Service Call via SVC instruction */
  DebugMonitor_IRQn                 =  -4,      /*!<  -4 Debug Monitor */
  PendSV_IRQn                       =  -2,      /*!<  -2 Pendable request for system service */
  SysTick_IRQn                      =  -1,      /*!<  -1 System Tick Timer */

#endif
} IRQn_Type;


#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))

typedef enum {
  disconnected_IRQn                 = 240       /*!< 240 Disconnected */
} cy_en_intr_t;

#endif

/*******************************************************************************
*                    Processor and Core Peripheral Section
*******************************************************************************/

#if ((defined(__GNUC__)        && (__ARM_ARCH == 6) && (__ARM_ARCH_6M__ == 1)) || \
     (defined(__ICCARM__)      && (__CORE__ == __ARM6M__)) || \
     (defined(__ARMCC_VERSION) && (__TARGET_ARCH_THUMB == 3)) || \
     (defined(__ghs__)         && defined(__CORE_CORTEXM0PLUS__)))

/* Configuration of the ARM Cortex-M0+ Processor and Core Peripherals */
#define __CM0PLUS_REV                   0x0001U /*!< CM0PLUS Core Revision */
#define __NVIC_PRIO_BITS                2       /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig          0       /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT                  1       /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT                   1       /*!< MPU present or not */

/** \} Configuration_of_CMSIS */

#include "core_cm0plus.h"                       /*!< ARM Cortex-M0+ processor and core peripherals */

#else

/* Configuration of the ARM Cortex-M4 Processor and Core Peripherals */
#define __CM4_REV                       0x0001U /*!< CM4 Core Revision */
#define __NVIC_PRIO_BITS                3       /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig          0       /*!< Set to 1 if different SysTick Config is used */
#define __VTOR_PRESENT                  1       /*!< Set to 1 if CPU supports Vector Table Offset Register */
#define __MPU_PRESENT                   1       /*!< MPU present or not */
#define __FPU_PRESENT                   1       /*!< FPU present or not */
#define __CM0P_PRESENT                  1       /*!< CM0P present or not */

/** \} Configuration_of_CMSIS */

#include "core_cm4.h"                           /*!< ARM Cortex-M4 processor and core peripherals */

#endif

/*******************************************************************************
*         Product-specific enums describing platform resources
*******************************************************************************/
/* For the target device these enums are defined in product-specific
* configuration files.
*/

typedef int          en_clk_dst_t;        /* SysClk */
typedef int          en_ep_mon_sel_t;     /* Profile */
typedef int          en_hsiom_sel_t;      /* GPIO */

typedef enum
{
    TRIGGER_TYPE_LEVEL = 0u,
    TRIGGER_TYPE_EDGE = 1u
} en_trig_type_t;

typedef enum
{
    CPUSS_MS_ID_CM0                 =  0,
    CPUSS_MS_ID_CM4                 = 14,
} en_prot_master_t;


/*******************************************************************************
*      Platform and peripheral definitions
*******************************************************************************/

#define CY_IP_MXTCPWM                   1u
#define CY_IP_MXCSDV2                   1u
#define CY_IP_MXLCD                     1u
#define CY_IP_MXS40SRSS                 1u
#define CY_IP_MXS40SRSS_RTC             1u
#define CY_IP_MXS40SRSS_MCWDT           1u
#define CY_IP_MXSCB                     1u
#define CY_IP_MXPERI                    1u
#define CY_IP_MXPERI_TR                 1u
#define CY_IP_M4CPUSS                   1u
#define CY_IP_M4CPUSS_DMA               1u
#define CY_IP_MXCRYPTO                  1u
#define CY_IP_MXBLESS                   1u
#define CY_IP_MXSDHC                    1u
#define CY_IP_MXAUDIOSS                 1u
#define CY_IP_MXLPCOMP                  1u
#define CY_IP_MXS40PASS                 1u
#define CY_IP_MXS40PASS_SAR             1u
#define CY_IP_MXS40PASS_CTDAC           1u
#define CY_IP_MXS40PASS_CTB             1u
#define CY_IP_MXSMIF                    1u
#define CY_IP_MXUSBFS                   1u
#define CY_IP_MXS40IOSS                 1u
#define CY_IP_MXEFUSE                   1u
#define CY_IP_MXUDB                     1u
#define CY_IP_MXPROFILE                 1u

/* Include IP definitions */
#include "ip/cyip_sflash.h"
#include "ip/cyip_peri.h"
#include "ip/cyip_peri_v2.h"
#include "ip/cyip_peri_ms_v2.h"
#include "ip/cyip_crypto.h"
#include "ip/cyip_crypto_v2.h"
#include "ip/cyip_cpuss.h"
#include "ip/cyip_cpuss_v2.h"
#include "ip/cyip_fault.h"
#include "ip/cyip_fault_v2.h"
#include "ip/cyip_ipc.h"
#include "ip/cyip_ipc_v2.h"
#include "ip/cyip_prot.h"
#include "ip/cyip_prot_v2.h"
#include "ip/cyip_flashc.h"
#include "ip/cyip_flashc_v2.h"
#include "ip/cyip_srss.h"
#include "ip/cyip_backup.h"
#include "ip/cyip_dw.h"
#include "ip/cyip_dw_v2.h"
#include "ip/cyip_dmac_v2.h"
#include "ip/cyip_efuse.h"
#include "ip/cyip_efuse_data.h"
#include "ip/cyip_profile.h"
#include "ip/cyip_hsiom.h"
#include "ip/cyip_hsiom_v2.h"
#include "ip/cyip_gpio.h"
#include "ip/cyip_gpio_v2.h"
#include "ip/cyip_smartio.h"
#include "ip/cyip_smartio_v2.h"
#include "ip/cyip_udb.h"
#include "ip/cyip_lpcomp.h"
#include "ip/cyip_csd.h"
#include "ip/cyip_tcpwm.h"
#include "ip/cyip_lcd.h"
#include "ip/cyip_ble.h"
#include "ip/cyip_usbfs.h"
#include "ip/cyip_smif.h"
#include "ip/cyip_sdhc.h"
#include "ip/cyip_scb.h"
#include "ip/cyip_ctbm.h"
#include "ip/cyip_ctdac.h"
#include "ip/cyip_sar.h"
#include "ip/cyip_pass.h"
#include "ip/cyip_i2s.h"
#include "ip/cyip_pdm.h"

/* IP type definitions */
typedef SFLASH_V1_Type SFLASH_Type;
typedef PERI_GR_V2_Type PERI_GR_Type;
typedef PERI_TR_GR_V2_Type PERI_TR_GR_Type;
typedef PERI_TR_1TO1_GR_V2_Type PERI_TR_1TO1_GR_Type;
typedef PERI_PPU_PR_V1_Type PERI_PPU_PR_Type;
typedef PERI_PPU_GR_V1_Type PERI_PPU_GR_Type;
typedef PERI_GR_PPU_SL_V1_Type PERI_GR_PPU_SL_Type;
typedef PERI_GR_PPU_RG_V1_Type PERI_GR_PPU_RG_Type;
typedef PERI_V2_Type PERI_Type;
typedef PERI_MS_PPU_PR_V2_Type PERI_MS_PPU_PR_Type;
typedef PERI_MS_PPU_FX_V2_Type PERI_MS_PPU_FX_Type;
typedef PERI_MS_V2_Type PERI_MS_Type;
typedef CRYPTO_V2_Type CRYPTO_Type;
typedef CPUSS_V2_Type CPUSS_Type;
typedef FAULT_STRUCT_V2_Type FAULT_STRUCT_Type;
typedef FAULT_V2_Type FAULT_Type;
typedef IPC_STRUCT_V2_Type IPC_STRUCT_Type;
typedef IPC_INTR_STRUCT_V2_Type IPC_INTR_STRUCT_Type;
typedef IPC_V2_Type IPC_Type;
typedef PROT_SMPU_SMPU_STRUCT_V2_Type PROT_SMPU_SMPU_STRUCT_Type;
typedef PROT_SMPU_V2_Type PROT_SMPU_Type;
typedef PROT_MPU_MPU_STRUCT_V2_Type PROT_MPU_MPU_STRUCT_Type;
typedef PROT_MPU_V2_Type PROT_MPU_Type;
typedef PROT_V2_Type PROT_Type;
typedef FLASHC_FM_CTL_V2_Type FLASHC_FM_CTL_Type;
typedef FLASHC_V2_Type FLASHC_Type;
typedef MCWDT_STRUCT_V1_Type MCWDT_STRUCT_Type;
typedef SRSS_V1_Type SRSS_Type;
typedef BACKUP_V1_Type BACKUP_Type;
typedef DW_CH_STRUCT_V2_Type DW_CH_STRUCT_Type;
typedef DW_V2_Type DW_Type;
typedef DMAC_CH_V2_Type DMAC_CH_Type;
typedef DMAC_V2_Type DMAC_Type;
typedef EFUSE_V1_Type EFUSE_Type;
typedef PROFILE_CNT_STRUCT_V1_Type PROFILE_CNT_STRUCT_Type;
typedef PROFILE_V1_Type PROFILE_Type;
typedef HSIOM_PRT_V2_Type HSIOM_PRT_Type;
typedef HSIOM_V2_Type HSIOM_Type;
typedef GPIO_PRT_V2_Type GPIO_PRT_Type;
typedef GPIO_V2_Type GPIO_Type;
typedef SMARTIO_PRT_V2_Type SMARTIO_PRT_Type;
typedef SMARTIO_V2_Type SMARTIO_Type;
typedef UDB_WRKONE_V1_Type UDB_WRKONE_Type;
typedef UDB_WRKMULT_V1_Type UDB_WRKMULT_Type;
typedef UDB_UDBPAIR_UDBSNG_V1_Type UDB_UDBPAIR_UDBSNG_Type;
typedef UDB_UDBPAIR_ROUTE_V1_Type UDB_UDBPAIR_ROUTE_Type;
typedef UDB_UDBPAIR_V1_Type UDB_UDBPAIR_Type;
typedef UDB_DSI_V1_Type UDB_DSI_Type;
typedef UDB_PA_V1_Type UDB_PA_Type;
typedef UDB_BCTL_V1_Type UDB_BCTL_Type;
typedef UDB_UDBIF_V1_Type UDB_UDBIF_Type;
typedef UDB_V1_Type UDB_Type;
typedef LPCOMP_V1_Type LPCOMP_Type;
typedef CSD_V1_Type CSD_Type;
typedef TCPWM_CNT_V1_Type TCPWM_CNT_Type;
typedef TCPWM_V1_Type TCPWM_Type;
typedef LCD_V1_Type LCD_Type;
typedef BLE_RCB_RCBLL_V1_Type BLE_RCB_RCBLL_Type;
typedef BLE_RCB_V1_Type BLE_RCB_Type;
typedef BLE_BLELL_V1_Type BLE_BLELL_Type;
typedef BLE_BLESS_V1_Type BLE_BLESS_Type;
typedef BLE_V1_Type BLE_Type;
typedef USBFS_USBDEV_V1_Type USBFS_USBDEV_Type;
typedef USBFS_USBLPM_V1_Type USBFS_USBLPM_Type;
typedef USBFS_USBHOST_V1_Type USBFS_USBHOST_Type;
typedef USBFS_V1_Type USBFS_Type;
typedef SMIF_DEVICE_V1_Type SMIF_DEVICE_Type;
typedef SMIF_V1_Type SMIF_Type;
typedef SDHC_WRAP_V1_Type SDHC_WRAP_Type;
typedef SDHC_CORE_V1_Type SDHC_CORE_Type;
typedef SDHC_V1_Type SDHC_Type;
typedef CySCB_V1_Type CySCB_Type;
typedef CTBM_V1_Type CTBM_Type;
typedef CTDAC_V1_Type CTDAC_Type;
typedef SAR_V1_Type SAR_Type;
typedef PASS_AREF_V1_Type PASS_AREF_Type;
typedef PASS_V1_Type PASS_Type;
typedef PDM_V1_Type PDM_Type;
typedef I2S_V1_Type I2S_Type;

/*******************************************************************************
*               Symbols with external linkage
*******************************************************************************/

extern uint32_t cy_PeriClkFreqHz;
extern uint32_t cy_BleEcoClockFreqHz;

/*******************************************************************************
* The remaining part is temporary here to enable library build and will
* go away once all drivers are updated.
*******************************************************************************/
 /* Number of IPC structures. Set the max allowed for platform */
 #define CPUSS_IPC_IPC_NR                16u
 /* Number of IPC interrupt structures. Set the max allowed for platform */
 #define CPUSS_IPC_IPC_IRQ_NR            16u

/* This is problematic. The drivers MUST NOT access interrupts directly.
 * cpuss_interrupts_ipc_0_IRQn differs between BLE and 2M and the difference
 * makes sense. The drivers cannot rely on device interrupt numbers.
 *
 * ipc_intr_cypipeConfig.intrSrc = (IRQn_Type)(cpuss_interrupts_ipc_0_IRQn + epConfigDataA.ipcNotifierNumber);
 *
 * The driver has to be changed to not access the interrupt vectors directly.
 * This is a temporary workaround.
 */
#define cpuss_interrupts_ipc_0_IRQn     23

/*************** SYSCLK *****************/

/* Number of clock paths. Must be > 0 */
#define SRSS_NUM_CLKPATH                5u
/* Number of PLLs present. Must be <= NUM_CLKPATH */
#define SRSS_NUM_PLL                    1u
/* Number of HFCLK roots present. Must be > 0 */
#define SRSS_NUM_HFROOT                 5u
/* Number of 8.0 dividers */
#define PERI_DIV_8_NR                   8u
/* Number of 16.0 dividers */
#define PERI_DIV_16_NR                  16u
/* Number of 16.5 (fractional) dividers */
#define PERI_DIV_16_5_NR                4u
/* Number of 24.5 (fractional) dividers */
#define PERI_DIV_24_5_NR                1u


/**************** SMIF *****************/

/* SMIF_DEVICE_NR is used by Cy_SMIF_DeInit() to set all device CTL
 * register to 0. Not sure how it behaves if the FW attempts to access
 * nonexistent device registers */

/* Number of external devices supported ([1,4]) */
#define SMIF_DEVICE_NR                  4u

/**************** SAR ******************/

/* Number of SAR channels */
#define PASS_SAR_SAR_CHANNELS           16u

/*************** PROFILE ****************/

/* This is used to define an internal static variable that keeps
 * control and status information for each counter.
 * static cy_stc_profile_ctr_t cy_ep_ctrs[PROFILE_PRFL_CNT_NR];
 * The size of cy_stc_profile_ctr_t is 36 bytes. To be device
 * independent the driver must assume the max number of counters
 * for the platform. This will waste (32-8)*36=864 bytes of RAM.
 */

/* Number of profiling counters. Legal range [1, 32] */
#define PROFILE_PRFL_CNT_NR             8u

/* Total count of Energy Profiler monitor signal connections */
#define EP_MONITOR_COUNT                28u

/**************** GPIO *****************/

/* Number of ports in device */
#define IOSS_GPIO_GPIO_PORT_NR          15u

/**************** FLASH *****************/

/* Page size in # of 32-bit words (1: 4 bytes, 2: 8 bytes, ... */
#define CPUSS_FLASHC_PA_SIZE            128u

/* For now this is only used by Flash to update cy_Hfclk0FreqHz
 * variable in case the system clock settings were changed in FW
 * after startup.
 */
extern void SystemCoreClockUpdate(void);

/* This is frustrating... See Cy_Flash_Init() in cy_flash.c
 * The flash driver installs ISR into the vector table for silicon workaround.
 * We need to understand if the workaround is needed for other products
 * and then define how to handle this.
 *
 * This is a temporary hack to get a clean build.
 */
#define cpuss_interrupt_fm_IRQn         ((IRQn_Type)85)      /*!<  85 [Active] FLASH Macro Interrupt */

/* Flash also uses cy_Hfclk0FreqHz variable and SystemCoreClockUpdate()
 * function defined in system_psoc63_cm0plus.c that is project-specific startup
 * code.
 */
extern uint32_t cy_Hfclk0FreqHz;

#define CY_EM_EEPROM_BASE               0x14000000UL
#define CY_EM_EEPROM_SIZE               0x00008000UL
#define CY_SFLASH_BASE                  0x16000000UL
#define CY_SFLASH_SIZE                  0x00008000UL

/* This is from the CM0+ System Startup. Used for flash workaround.
 * This must be either moved to the library or not used by the drivers. */
#define CY_SYS_CM4_STATUS_ENABLED       3U
extern uint32_t Cy_SysGetCM4Status(void);

/**************** CTDAC *****************/

/* CTDAC uses PCLK_PASS_CLOCK_CTDAC which is the element of a product-specific
 * enum (en_clk_dst_t) that describes clock connections for the device.
 * Why does CTDAC attempts to configure clocks? The drivers are not allowed to
 * configure platform resources!!! */

#define PCLK_PASS_CLOCK_CTDAC           55

/*************** CRYPTO *****************/

/* Crypto uses below defines to conditionally exclude the function
 * prototypes for unsupported features of the Crypto IP.
 * (by )of public header files. This check needs to be removed from the driver. */

/* Cryptography IP present or not (0=No, 1=Yes) */
#define CPUSS_CRYPTO_PRESENT            1u/* AES cipher support (0 = no support, 1 = support */
/* (Tripple) DES cipher support (0 = no support, 1 = support */
#define CPUSS_CRYPTO_DES                1u
/* Pseudo random number generation support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_PR                 1u
/* SHA support included */
#define CPUSS_CRYPTO_SHA                1u
/* SHA1 hash support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_SHA1               1u
/* SHA256 hash support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_SHA256             1u
/* SHA512 hash support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_SHA512             1u
/* Cyclic Redundancy Check support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_CRC                1u
/* Vector unit support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_VU                 1u
/* True random number generation support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_TR                 1u
/* String support (0 = no support, 1 = support) */
#define CPUSS_CRYPTO_STR                1u

/* We have to rethink the usage of config files for our drivers and
 * forbid any LLD to configure platform resources.
 */
#define cpuss_interrupt_crypto_IRQn     84
#define NvicMux2_IRQn                   2
#define NvicMux30_IRQn                  30
#define NvicMux31_IRQn                  31


/*************** DW *****************/

/* DataWire 0 present or not (0=No, 1=Yes) */
#define CPUSS_DW0_PRESENT               1u
/* Number of DataWire 0 channels (8, 16 or 32) */
#define CPUSS_DW0_CH_NR                 16u
/* DataWire 1 present or not (0=No, 1=Yes) */
#define CPUSS_DW1_PRESENT               1u
/* Number of DataWire 1 channels (8, 16 or 32) */
#define CPUSS_DW1_CH_NR                 16u

/*************** SCB *****************/

/* I don't know what to do about that!!!
 * Probably generate SCB descriptor structure in Flash
 * This however will require to convert the SCB address
 * to index. The SCB base addresses changes between
 * different products, so we will have to store the address
 * of SCB0 for each product as well. This is a kind of
 * overengineering... */

#define SCB_GET_EZ_DATA_NR(base)        256u
#define SCB_IS_I2C_SLAVE_CAPABLE(base)  true
#define SCB_IS_I2C_MASTER_CAPABLE(base) ((base) != SCB8)
#define SCB_IS_I2C_DS_CAPABLE(base)     ((base) == SCB8)
#define SCB_IS_SPI_SLAVE_CAPABLE(base)  true
#define SCB_IS_SPI_MASTER_CAPABLE(base) ((base) != SCB8)
#define SCB_IS_SPI_DS_CAPABLE(base)     ((base) == SCB8)
#define SCB_IS_UART_CAPABLE(base)       ((base) != SCB8)

/*************** SYSPM *****************/

#define CY_MMIO_UDB_GROUP_NR            3u
#define CY_MMIO_UDB_SLAVE_NR            4u

/*************** SYSLIB *****************/

/* Ideally the SYSLIB must be changed to eliminate the
 * dependencies on the system startup variables.
 */
extern uint32_t cy_delayFreqKhz;
extern uint8_t cy_delayFreqMhz;
extern uint32_t cy_delay32kMs;

/************** SYSINT ******************/
#define CY_IP_M4CPUSS_VERSION            1u


/*******************************************************************************
*                 MPN-specific parameters
*******************************************************************************/
/* This comes from 001-91989 Marketing Part Definitions and is SW wounding
 * thing. Some MPNs have max frequency limit of 50 MHz. I don't think this
 * can be supported in a reasonable way and should always be set to max
 * supported by the platform.
 */
#define CY_HF_CLK_MAX_FREQ              150000000UL

/* Flash capacity is different per MPN */
#define CY_FLASH_BASE                   0x10000000UL
#define CY_FLASH_SIZE                   0x00100000UL

/* This seems to use wrong assumption. Need to clarify if it makes sense */
#define CPUSS_FLASHC_PA_SIZE_LOG2       0x7UL



/*******************************************************************************
*                                    SFLASH
*******************************************************************************/

#define SFLASH_BASE                             0x16000000UL
#define SFLASH                                  ((SFLASH_Type*) SFLASH_BASE)  // Used by syslib, syspm, flash

/*******************************************************************************
*                                     PERI
*******************************************************************************/

#define PERI_BASE                               0x40010000UL
#define PERI                                    ((PERI_Type*) PERI_BASE) // Used by trigmux, sysclk, sysPm

/*******************************************************************************
*                                    CPUSS
*******************************************************************************/

#define CPUSS_BASE                              0x40210000UL
#define CPUSS                                   ((CPUSS_Type*) CPUSS_BASE)    //Used by systick, syspm, syslib, sysint, sysclk, flash

/*******************************************************************************
*                                     IPC
*******************************************************************************/

#define IPC_BASE                                0x40230000UL
#define IPC                                     ((IPC_Type*) IPC_BASE)
#define IPC_STRUCT0                             ((IPC_STRUCT_Type*) &IPC->STRUCT[0])  // Used by syslib
#define IPC_STRUCT7                             ((IPC_STRUCT_Type*) &IPC->STRUCT[7])  // Used by syspm

/*******************************************************************************
*                                     PROT
*******************************************************************************/

#define PROT_BASE                               0x40240000UL
#define PROT                                    ((PROT_Type*) PROT_BASE)     //Used by prot

/*******************************************************************************
*                                    FLASHC
*******************************************************************************/

#define FLASHC_BASE                             0x40250000UL
#define FLASHC                                  ((FLASHC_Type*) FLASHC_BASE)            //used by syslib, flash
#define FLASHC_FM_CTL                           ((FLASHC_FM_CTL_Type*) &FLASHC->FM_CTL) //used by syspm

/*******************************************************************************
*                                     SRSS
*******************************************************************************/

#define SRSS_BASE                               0x40260000UL
#define SRSS                                    ((SRSS_Type*) SRSS_BASE)    //used by wdt, syspm, syslib, sysclk, lvd, lpcomp, flash

/*******************************************************************************
*                                    BACKUP
*******************************************************************************/

#define BACKUP_BASE                             0x40270000UL
#define BACKUP                                  ((BACKUP_Type*) BACKUP_BASE)   //used by syspm, syslib, sysclk, rtc

/*******************************************************************************
*                                      DW
*******************************************************************************/

#define DW0_BASE                                0x40280000UL
#define DW1_BASE                                0x40281000UL
#define DW0                                     ((DW_Type*) DW0_BASE)         //used by dma
#define DW1                                     ((DW_Type*) DW1_BASE)         //used by dma

/*******************************************************************************
*                                   PROFILE
*******************************************************************************/

#define PROFILE_BASE                            0x402D0000UL
#define PROFILE                                 ((PROFILE_Type*) PROFILE_BASE) //used by profile


/*******************************************************************************
*                                    HSIOM
*******************************************************************************/

#define HSIOM_BASE                              0x40310000UL
#define HSIOM                                   ((HSIOM_Type*) HSIOM_BASE)   //used by lpcomp, gpio

/*******************************************************************************
*                                     GPIO
*******************************************************************************/

#define GPIO_BASE                               0x40320000UL
#define GPIO                                    ((GPIO_Type*) GPIO_BASE)     //used by GPIO

#define GPIO_PRT0                               ((GPIO_PRT_Type*) &GPIO->PRT[0])                                  /* 0x40320000 */
#define GPIO_PRT1                               ((GPIO_PRT_Type*) &GPIO->PRT[1])                                  /* 0x40320080 */
#define GPIO_PRT2                               ((GPIO_PRT_Type*) &GPIO->PRT[2])                                  /* 0x40320100 */
#define GPIO_PRT3                               ((GPIO_PRT_Type*) &GPIO->PRT[3])                                  /* 0x40320180 */
#define GPIO_PRT4                               ((GPIO_PRT_Type*) &GPIO->PRT[4])                                  /* 0x40320200 */
#define GPIO_PRT5                               ((GPIO_PRT_Type*) &GPIO->PRT[5])                                  /* 0x40320280 */
#define GPIO_PRT6                               ((GPIO_PRT_Type*) &GPIO->PRT[6])                                  /* 0x40320300 */
#define GPIO_PRT7                               ((GPIO_PRT_Type*) &GPIO->PRT[7])                                  /* 0x40320380 */
#define GPIO_PRT8                               ((GPIO_PRT_Type*) &GPIO->PRT[8])                                  /* 0x40320400 */
#define GPIO_PRT9                               ((GPIO_PRT_Type*) &GPIO->PRT[9])                                  /* 0x40320480 */
#define GPIO_PRT10                              ((GPIO_PRT_Type*) &GPIO->PRT[10])                                 /* 0x40320500 */
#define GPIO_PRT11                              ((GPIO_PRT_Type*) &GPIO->PRT[11])                                 /* 0x40320580 */
#define GPIO_PRT12                              ((GPIO_PRT_Type*) &GPIO->PRT[12])                                 /* 0x40320600 */
#define GPIO_PRT13                              ((GPIO_PRT_Type*) &GPIO->PRT[13])                                 /* 0x40320680 */
#define GPIO_PRT14                              ((GPIO_PRT_Type*) &GPIO->PRT[14])                                 /* 0x40320700 */

/*******************************************************************************
*                                     UDB
*******************************************************************************/

#define UDB_BASE                                0x40340000UL
#define UDB                                     ((UDB_Type*) UDB_BASE)          //used by syspm

/*******************************************************************************
*                                    LPCOMP
*******************************************************************************/

#define LPCOMP_BASE                             0x40350000UL
#define LPCOMP                                  ((LPCOMP_Type*) LPCOMP_BASE)  //used by lpcomp (CDT302132)

/*******************************************************************************
*                                     SCB
*******************************************************************************/

#define SCB8_BASE                               0x40690000UL
#define SCB8                                    ((CySCB_Type*) SCB8_BASE)     //used by scb parameter validation

/*******************************************************************************
*                                     PASS
*******************************************************************************/

#define PASS_BASE                               0x411F0000UL
#define PASS                                    ((PASS_Type*) PASS_BASE)         //used by sysanalog
#define PASS_AREF                               ((PASS_AREF_Type*) &PASS->AREF)  //used by sysanalog, CTB

/*******************************************************************************
*                                     BLE
*******************************************************************************/

#define BLE_BASE                                0x403C0000UL
#define BLE                                     ((BLE_Type*) BLE_BASE)                                            /* 0x403C0000 */
#define BLE_RCB_RCBLL                           ((BLE_RCB_RCBLL_Type*) &BLE->RCB.RCBLL)                           /* 0x403C0100 */
#define BLE_RCB                                 ((BLE_RCB_Type*) &BLE->RCB)                                       /* 0x403C0000 */
#define BLE_BLELL                               ((BLE_BLELL_Type*) &BLE->BLELL)                                   /* 0x403C1000 */
#define BLE_BLESS                               ((BLE_BLESS_Type*) &BLE->BLESS)                                   /* 0x403DF000 */

#endif /* _CY_DEVICE_COMMON_H_ */


/* [] END OF FILE */
