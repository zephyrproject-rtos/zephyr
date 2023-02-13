/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEC_SOC_H
#define __MEC_SOC_H

#ifndef _ASMLANGUAGE

/*
 * MEC172x includes the ARM single precision FPU and the ARM MPU with
 * eight regions. Zephyr has an in-tree CMSIS header located in the arch
 * include hierarchy that includes the correct ARM CMSIS core_xxx header
 * from hal_cmsis based on the k-config CPU selection.
 * The Zephyr in-tree header does not provide all the symbols ARM CMSIS
 * requires. Zephyr does not define CMSIS FPU present and defaults CMSIS
 * MPU present to 0. We define these two symbols here based on our k-config
 * selections. NOTE: Zephyr in-tree CMSIS defines the Cortex-M4 hardware
 * revision to 0. At this time ARM CMSIS does not appear to use the hardware
 * revision in any macros.
 */
#define __FPU_PRESENT  CONFIG_CPU_HAS_FPU
#define __MPU_PRESENT  CONFIG_CPU_HAS_ARM_MPU

#define __CM4_REV 0x0201                /*!< Core Revision r2p1 */

#define __VTOR_PRESENT 1                /*!< Set to 1 if VTOR is present */
#define __NVIC_PRIO_BITS 3              /*!< Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig 0        /*!< 0 use default SysTick HW */
#define __FPU_DP 0                      /*!< Set to 1 if FPU is double precision */
#define __ICACHE_PRESENT 0              /*!< Set to 1 if I-Cache is present */
#define __DCACHE_PRESENT 0              /*!< Set to 1 if D-Cache is present */
#define __DTCM_PRESENT 0                /*!< Set to 1 if DTCM is present */

/** @brief ARM Cortex-M4 NVIC Interrupt Numbers
 * CM4 NVIC implements 16 internal interrupt sources. CMSIS macros use
 * negative numbers [-15, -1]. Lower numerical value indicates higher
 * priority.
 * -15 = Reset Vector invoked on POR or any CPU reset.
 * -14 = NMI
 * -13 = Hard Fault. At POR or CPU reset all faults map to Hard Fault.
 * -12 = Memory Management Fault. If enabled Hard Faults caused by access
 *       violations, no address match, or MPU mismatch.
 * -11 = Bus Fault. If enabled pre-fetch, AHB access faults.
 * -10 = Usage Fault. If enabled Undefined instructions, illegal state
 *       transition (Thumb -> ARM mode), unaligned, etc.
 * -9 through -6 are not implemented (reserved).
 * -5 System call via SVC instruction.
 * -4 Debug Monitor.
 * -3 not implemented (reserved).
 * -2 PendSV for system service.
 * -1 SysTick NVIC system timer.
 * Numbers >= 0 are external peripheral interrupts.
 */
typedef enum {
	/* ========== ARM Cortex-M4 Specific Interrupt Numbers ============ */

	Reset_IRQn              = -15,  /*!< POR/CPU Reset Vector */
	NonMaskableInt_IRQn     = -14,  /*!< NMI */
	HardFault_IRQn          = -13,  /*!< Hard Faults */
	MemoryManagement_IRQn   = -12,  /*!< Memory Management faults */
	BusFault_IRQn           = -11,  /*!< Bus Access faults */
	UsageFault_IRQn         = -10,  /*!< Usage/instruction faults */
	SVCall_IRQn             = -5,   /*!< SVC */
	DebugMonitor_IRQn       = -4,   /*!< Debug Monitor */
	PendSV_IRQn             = -2,   /*!< PendSV */
	SysTick_IRQn            = -1,   /*!< SysTick */

	/* ==============  MEC172x Specific Interrupt Numbers ============ */

	GIRQ08_IRQn             = 0,    /*!< GPIO 0140 - 0176 */
	GIRQ09_IRQn             = 1,    /*!< GPIO 0100 - 0136 */
	GIRQ10_IRQn             = 2,    /*!< GPIO 0040 - 0076 */
	GIRQ11_IRQn             = 3,    /*!< GPIO 0000 - 0036 */
	GIRQ12_IRQn             = 4,    /*!< GPIO 0200 - 0236 */
	GIRQ13_IRQn             = 5,    /*!< SMBus Aggregated */
	GIRQ14_IRQn             = 6,    /*!< DMA Aggregated */
	GIRQ15_IRQn             = 7,
	GIRQ16_IRQn             = 8,
	GIRQ17_IRQn             = 9,
	GIRQ18_IRQn             = 10,
	GIRQ19_IRQn             = 11,
	GIRQ20_IRQn             = 12,
	GIRQ21_IRQn             = 13,
	/* GIRQ22(peripheral clock wake) is not connected to NVIC */
	GIRQ23_IRQn             = 14,
	GIRQ24_IRQn             = 15,
	GIRQ25_IRQn             = 16,
	GIRQ26_IRQn             = 17, /*!< GPIO 0240 - 0276 */
	/* Reserved 18-19 */
	/* GIRQ's 8 - 12, 24 - 26 no direct connections */
	I2C_SMB_0_IRQn          = 20,   /*!< GIRQ13 b[0] */
	I2C_SMB_1_IRQn          = 21,   /*!< GIRQ13 b[1] */
	I2C_SMB_2_IRQn          = 22,   /*!< GIRQ13 b[2] */
	I2C_SMB_3_IRQn          = 23,   /*!< GIRQ13 b[3] */
	DMA0_IRQn               = 24,   /*!< GIRQ14 b[0] */
	DMA1_IRQn               = 25,   /*!< GIRQ14 b[1] */
	DMA2_IRQn               = 26,   /*!< GIRQ14 b[2] */
	DMA3_IRQn               = 27,   /*!< GIRQ14 b[3] */
	DMA4_IRQn               = 28,   /*!< GIRQ14 b[4] */
	DMA5_IRQn               = 29,   /*!< GIRQ14 b[5] */
	DMA6_IRQn               = 30,   /*!< GIRQ14 b[6] */
	DMA7_IRQn               = 31,   /*!< GIRQ14 b[7] */
	DMA8_IRQn               = 32,   /*!< GIRQ14 b[8] */
	DMA9_IRQn               = 33,   /*!< GIRQ14 b[9] */
	DMA10_IRQn              = 34,   /*!< GIRQ14 b[10] */
	DMA11_IRQn              = 35,   /*!< GIRQ14 b[11] */
	DMA12_IRQn              = 36,   /*!< GIRQ14 b[12] */
	DMA13_IRQn              = 37,   /*!< GIRQ14 b[13] */
	DMA14_IRQn              = 38,   /*!< GIRQ14 b[14] */
	DMA15_IRQn              = 39,   /*!< GIRQ14 b[15] */
	UART0_IRQn              = 40,   /*!< GIRQ15 b[0] */
	UART1_IRQn              = 41,   /*!< GIRQ15 b[1] */
	EMI0_IRQn               = 42,   /*!< GIRQ15 b[2] */
	EMI1_IRQn               = 43,   /*!< GIRQ15 b[3] */
	EMI2_IRQn               = 44,   /*!< GIRQ15 b[4] */
	ACPI_EC0_IBF_IRQn       = 45,   /*!< GIRQ15 b[5] */
	ACPI_EC0_OBE_IRQn       = 46,   /*!< GIRQ15 b[6] */
	ACPI_EC1_IBF_IRQn       = 47,   /*!< GIRQ15 b[7] */
	ACPI_EC1_OBE_IRQn       = 48,   /*!< GIRQ15 b[8] */
	ACPI_EC2_IBF_IRQn       = 49,   /*!< GIRQ15 b[9] */
	ACPI_EC2_OBE_IRQn       = 50,   /*!< GIRQ15 b[10] */
	ACPI_EC3_IBF_IRQn       = 51,   /*!< GIRQ15 b[11] */
	ACPI_EC3_OBE_IRQn       = 52,   /*!< GIRQ15 b[12] */
	ACPI_EC4_IBF_IRQn       = 53,   /*!< GIRQ15 b[13] */
	ACPI_EC4_OBE_IRQn       = 54,   /*!< GIRQ15 b[14] */
	ACPI_PM1_CTL_IRQn       = 55,   /*!< GIRQ15 b[15] */
	ACPI_PM1_EN_IRQn        = 56,   /*!< GIRQ15 b[16] */
	ACPI_PM1_STS_IRQn       = 57,   /*!< GIRQ15 b[17] */
	KBC_OBE_IRQn            = 58,   /*!< GIRQ15 b[18] */
	KBC_IBF_IRQn            = 59,   /*!< GIRQ15 b[19] */
	MBOX_IRQn               = 60,   /*!< GIRQ15 b[20] */
	/* reserved 61 */
	P80BD_0_IRQn            = 62,   /*!< GIRQ15 b[22] */
	/* reserved 63-64 */
	PKE_IRQn                = 65,   /*!< GIRQ16 b[0] */
	/* reserved 66 */
	RNG_IRQn                = 67,   /*!< GIRQ16 b[2] */
	AESH_IRQn               = 68,   /*!< GIRQ16 b[3] */
	/* reserved 69 */
	PECI_IRQn               = 70,   /*!< GIRQ17 b[0] */
	TACH_0_IRQn             = 71,   /*!< GIRQ17 b[1] */
	TACH_1_IRQn             = 72,   /*!< GIRQ17 b[2] */
	TACH_2_IRQn             = 73,   /*!< GIRQ17 b[3] */
	RPMFAN_0_FAIL_IRQn      = 74,   /*!< GIRQ17 b[20] */
	RPMFAN_0_STALL_IRQn     = 75,   /*!< GIRQ17 b[21] */
	RPMFAN_1_FAIL_IRQn      = 76,   /*!< GIRQ17 b[22] */
	RPMFAN_1_STALL_IRQn     = 77,   /*!< GIRQ17 b[23] */
	ADC_SNGL_IRQn           = 78,   /*!< GIRQ17 b[8] */
	ADC_RPT_IRQn            = 79,   /*!< GIRQ17 b[9] */
	RCID_0_IRQn             = 80,   /*!< GIRQ17 b[10] */
	RCID_1_IRQn             = 81,   /*!< GIRQ17 b[11] */
	RCID_2_IRQn             = 82,   /*!< GIRQ17 b[12] */
	LED_0_IRQn              = 83,   /*!< GIRQ17 b[13] */
	LED_1_IRQn              = 84,   /*!< GIRQ17 b[14] */
	LED_2_IRQn              = 85,   /*!< GIRQ17 b[15] */
	LED_3_IRQn              = 86,   /*!< GIRQ17 b[16] */
	PHOT_IRQn               = 87,   /*!< GIRQ17 b[17] */
	/* reserved 88-89 */
	SPIP_0_IRQn             = 90,   /*!< GIRQ18 b[0] */
	QMSPI_0_IRQn            = 91,   /*!< GIRQ18 b[1] */
	GPSPI_0_TXBE_IRQn       = 92,   /*!< GIRQ18 b[2] */
	GPSPI_0_RXBF_IRQn       = 93,   /*!< GIRQ18 b[3] */
	GPSPI_1_TXBE_IRQn       = 94,   /*!< GIRQ18 b[4] */
	GPSPI_1_RXBF_IRQn       = 95,   /*!< GIRQ18 b[5] */
	BCL_0_ERR_IRQn          = 96,   /*!< GIRQ18 b[7] */
	BCL_0_BCLR_IRQn         = 97,   /*!< GIRQ18 b[6] */
	/* reserved 98-99 */
	PS2_0_ACT_IRQn          = 100,  /*!< GIRQ18 b[10] */
	/* reserved 101-102 */
	ESPI_PC_IRQn            = 103,  /*!< GIRQ19 b[0] */
	ESPI_BM1_IRQn           = 104,  /*!< GIRQ19 b[1] */
	ESPI_BM2_IRQn           = 105,  /*!< GIRQ19 b[2] */
	ESPI_LTR_IRQn           = 106,  /*!< GIRQ19 b[3] */
	ESPI_OOB_UP_IRQn        = 107,  /*!< GIRQ19 b[4] */
	ESPI_OOB_DN_IRQn        = 108,  /*!< GIRQ19 b[5] */
	ESPI_FLASH_IRQn         = 109,  /*!< GIRQ19 b[6] */
	ESPI_RESET_IRQn         = 110,  /*!< GIRQ19 b[7] */
	RTMR_IRQn               = 111,  /*!< GIRQ23 b[10] */
	HTMR_0_IRQn             = 112,  /*!< GIRQ23 b[16] */
	HTMR_1_IRQn             = 113,  /*!< GIRQ23 b[17] */
	WK_IRQn                 = 114,  /*!< GIRQ21 b[3] */
	WKSUB_IRQn              = 115,  /*!< GIRQ21 b[4] */
	WKSEC_IRQn              = 116,  /*!< GIRQ21 b[5] */
	WKSUBSEC_IRQn           = 117,  /*!< GIRQ21 b[6] */
	WKSYSPWR_IRQn           = 118,  /*!< GIRQ21 b[7] */
	RTC_IRQn                = 119,  /*!< GIRQ21 b[8] */
	RTC_ALARM_IRQn          = 120,  /*!< GIRQ21 b[9] */
	VCI_OVRD_IN_IRQn        = 121,  /*!< GIRQ21 b[10] */
	VCI_IN0_IRQn            = 122,  /*!< GIRQ21 b[11] */
	VCI_IN1_IRQn            = 123,  /*!< GIRQ21 b[12] */
	VCI_IN2_IRQn            = 124,  /*!< GIRQ21 b[13] */
	VCI_IN3_IRQn            = 125,  /*!< GIRQ21 b[14] */
	VCI_IN4_IRQn            = 126,  /*!< GIRQ21 b[15] */
	/* reserved 127-128 */
	PS2_0A_WAKE_IRQn        = 129,  /*!< GIRQ21 b[18] */
	PS2_0B_WAKE_IRQn        = 130,  /*!< GIRQ21 b[19] */
	/* reserved 131-134 */
	KEYSCAN_IRQn            = 135,  /*!< GIRQ21 b[25] */
	B16TMR_0_IRQn           = 136,  /*!< GIRQ23 b[0] */
	B16TMR_1_IRQn           = 137,  /*!< GIRQ23 b[1] */
	B16TMR_2_IRQn           = 138,  /*!< GIRQ23 b[2] */
	B16TMR_3_IRQn           = 139,  /*!< GIRQ23 b[3] */
	B32TMR_0_IRQn           = 140,  /*!< GIRQ23 b[4] */
	B32TMR_1_IRQn           = 141,  /*!< GIRQ23 b[5] */
	CTMR_0_IRQn             = 142,  /*!< GIRQ23 b[6] */
	CTMR_1_IRQn             = 143,  /*!< GIRQ23 b[7] */
	CTMR_2_IRQn             = 144,  /*!< GIRQ23 b[8] */
	CTMR_3_IRQn             = 145,  /*!< GIRQ23 b[9] */
	CCT_IRQn                = 146,  /*!< GIRQ18 b[20] */
	CCT_CAP0_IRQn           = 147,  /*!< GIRQ18 b[21] */
	CCT_CAP1_IRQn           = 148,  /*!< GIRQ18 b[22] */
	CCT_CAP2_IRQn           = 149,  /*!< GIRQ18 b[23] */
	CCT_CAP3_IRQn           = 150,  /*!< GIRQ18 b[24] */
	CCT_CAP4_IRQn           = 151,  /*!< GIRQ18 b[25] */
	CCT_CAP5_IRQn           = 152,  /*!< GIRQ18 b[26] */
	CCT_CMP0_IRQn           = 153,  /*!< GIRQ18 b[27] */
	CCT_CMP1_IRQn           = 154,  /*!< GIRQ18 b[28] */
	EEPROMC_IRQn            = 155,  /*!< GIRQ18 b[13] */
	ESPI_VWIRE_IRQn         = 156,  /*!< GIRQ19 b[8] */
	/* reserved 157 */
	I2C_SMB_4_IRQn          = 158,  /*!< GIRQ13 b[4] */
	TACH_3_IRQn             = 159,  /*!< GIRQ17 b[4] */
	/* reserved 160-165 */
	SAF_DONE_IRQn           = 166,  /*!< GIRQ19 b[9] */
	SAF_ERR_IRQn            = 167,  /*!< GIRQ19 b[10] */
	/* reserved 168 */
	SAF_CACHE_IRQn          = 169,  /*!< GIRQ19 b[11] */
	/* reserved 170 */
	WDT_0_IRQn              = 171,  /*!< GIRQ21 b[2] */
	GLUE_IRQn               = 172,  /*!< GIRQ21 b[26] */
	OTP_RDY_IRQn            = 173,  /*!< GIRQ20 b[3] */
	CLK32K_MON_IRQn         = 174,  /*!< GIRQ20 b[9] */
	ACPI_EC0_IRQn           = 175,  /* ACPI EC OBE and IBF combined into one */
	ACPI_EC1_IRQn           = 176,  /* No GIRQ connection. Status in ACPI blocks */
	ACPI_EC2_IRQn           = 177,  /* Code uses level bits and NVIC bits */
	ACPI_EC3_IRQn           = 178,
	ACPI_EC4_IRQn           = 179,
	ACPI_PM1_IRQn           = 180,
	MAX_IRQn
} IRQn_Type;

#include <zephyr/sys/util.h>

/* chip specific register defines */
#include "reg/mec172x_defs.h"
#include "reg/mec172x_ecia.h"
#include "reg/mec172x_ecs.h"
#include "reg/mec172x_espi_iom.h"
#include "reg/mec172x_espi_saf.h"
#include "reg/mec172x_espi_vw.h"
#include "reg/mec172x_gpio.h"
#include "reg/mec172x_i2c_smb.h"
#include "reg/mec172x_p80bd.h"
#include "reg/mec172x_pcr.h"
#include "reg/mec172x_qspi.h"
#include "reg/mec172x_vbat.h"
#include "reg/mec172x_emi.h"

/* common peripheral register defines */
#include "../common/reg/mec_acpi_ec.h"
#include "../common/reg/mec_adc.h"
#include "../common/reg/mec_global_cfg.h"
#include "../common/reg/mec_kbc.h"
#include "../common/reg/mec_keyscan.h"
#include "../common/reg/mec_peci.h"
#include "../common/reg/mec_ps2.h"
#include "../common/reg/mec_pwm.h"
#include "../common/reg/mec_tach.h"
#include "../common/reg/mec_tfdp.h"
#include "../common/reg/mec_timers.h"
#include "../common/reg/mec_uart.h"
#include "../common/reg/mec_vci.h"
#include "../common/reg/mec_wdt.h"
#include "../common/reg/mec_gpio.h"

/* common SoC API */
#include "../common/soc_dt.h"
#include "../common/soc_gpio.h"
#include "../common/soc_pcr.h"
#include "../common/soc_pins.h"
#include "../common/soc_espi_channels.h"
#include "../common/soc_i2c.h"

/* MEC172x SAF V2 */
#include "soc_espi_saf_v2.h"

#endif

#endif
