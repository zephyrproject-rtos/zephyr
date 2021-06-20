/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __CORE_FEATURE_ECLIC__
#define __CORE_FEATURE_ECLIC__
/*!
 * @file     core_feature_eclic.h
 * @brief    ECLIC feature API header file for Nuclei N/NX Core
 */
/*
 * ECLIC Feature Configuration Macro:
 * 1. __ECLIC_PRESENT:  Define whether Enhanced Core Local Interrupt Controller (ECLIC) Unit is present or not
 *   * 0: Not present
 *   * 1: Present
 * 2. __ECLIC_BASEADDR:  Base address of the ECLIC unit.
 * 3. ECLIC_GetInfoCtlbits():  Define the number of hardware bits are actually implemented in the clicintctl registers.
 *   Valid number is 1 - 8.
 * 4. __ECLIC_INTNUM  : Define the external interrupt number of ECLIC Unit
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__ECLIC_PRESENT) && (__ECLIC_PRESENT == 1)
/**
 * \defgroup NMSIS_Core_ECLIC_Registers     Register Define and Type Definitions Of ECLIC
 * \ingroup NMSIS_Core_Registers
 * \brief   Type definitions and defines for eclic registers.
 *
 * @{
 */

/**
 * \brief  Union type to access CLICFG configure register.
 */
typedef union {
	struct {
		uint8_t _reserved0 : 1;         /*!< bit:     0   Overflow condition code flag */
		uint8_t nlbits : 4;             /*!< bit:     29  Carry condition code flag */
		uint8_t _reserved1 : 2;         /*!< bit:     30  Zero condition code flag */
		uint8_t _reserved2 : 1;         /*!< bit:     31  Negative condition code flag */
	} b;                                    /*!< Structure used for bit  access */
	uint8_t w;                              /*!< Type      used for byte access */
} CLICCFG_Type;

/**
 * \brief  Union type to access CLICINFO information register.
 */
typedef union {
	struct {
		uint32_t numint : 13;           /*!< bit:  0..12   number of maximum interrupt inputs supported */
		uint32_t version : 8;           /*!< bit:  13..20  20:17 for architecture version,16:13 for implementation version */
		uint32_t intctlbits : 4;        /*!< bit:  21..24  specifies how many hardware bits are actually implemented in the clicintctl registers */
		uint32_t _reserved0 : 7;        /*!< bit:  25..31  Reserved */
	} b;                                    /*!< Structure used for bit  access */
	uint32_t w;                             /*!< Type      used for word access */
} CLICINFO_Type;

/**
 * \brief Access to the structure of a vector interrupt controller.
 */
typedef struct {
	__IOM uint8_t INTIP;                    /*!< Offset: 0x000 (R/W)  Interrupt set pending register */
	__IOM uint8_t INTIE;                    /*!< Offset: 0x001 (R/W)  Interrupt set enable register */
	__IOM uint8_t INTATTR;                  /*!< Offset: 0x002 (R/W)  Interrupt set attributes register */
	__IOM uint8_t INTCTRL;                  /*!< Offset: 0x003 (R/W)  Interrupt configure register */
} CLIC_CTRL_Type;

typedef struct {
	__IOM uint8_t CFG;                      /*!< Offset: 0x000 (R/W)  CLIC configuration register */
	uint8_t RESERVED0[3];
	__IM uint32_t INFO;                     /*!< Offset: 0x004 (R/ )  CLIC information register */
	uint8_t RESERVED1[3];
	__IOM uint8_t MTH;                      /*!< Offset: 0x00B (R/W)  CLIC machine mode threshold register */
	uint32_t RESERVED2[0x3FD];
	CLIC_CTRL_Type CTRL[4096];              /*!< Offset: 0x1000 (R/W) CLIC register structure for INTIP, INTIE, INTATTR, INTCTL */
} CLIC_Type;

#define CLIC_CLICCFG_NLBIT_Pos                 1U                                       /*!< CLIC CLICCFG: NLBIT Position */
#define CLIC_CLICCFG_NLBIT_Msk                 (0xFUL << CLIC_CLICCFG_NLBIT_Pos)        /*!< CLIC CLICCFG: NLBIT Mask */

#define CLIC_CLICINFO_CTLBIT_Pos                21U                                     /*!< CLIC INTINFO: __ECLIC_GetInfoCtlbits() Position */
#define CLIC_CLICINFO_CTLBIT_Msk                (0xFUL << CLIC_CLICINFO_CTLBIT_Pos)     /*!< CLIC INTINFO: __ECLIC_GetInfoCtlbits() Mask */

#define CLIC_CLICINFO_VER_Pos                  13U                                      /*!< CLIC CLICINFO: VERSION Position */
#define CLIC_CLICINFO_VER_Msk                  (0xFFUL << CLIC_CLICCFG_NLBIT_Pos)       /*!< CLIC CLICINFO: VERSION Mask */

#define CLIC_CLICINFO_NUM_Pos                  0U                                       /*!< CLIC CLICINFO: NUM Position */
#define CLIC_CLICINFO_NUM_Msk                  (0xFFFUL << CLIC_CLICINFO_NUM_Pos)       /*!< CLIC CLICINFO: NUM Mask */

#define CLIC_INTIP_IP_Pos                      0U                                       /*!< CLIC INTIP: IP Position */
#define CLIC_INTIP_IP_Msk                      (0x1UL << CLIC_INTIP_IP_Pos)             /*!< CLIC INTIP: IP Mask */

#define CLIC_INTIE_IE_Pos                      0U                                       /*!< CLIC INTIE: IE Position */
#define CLIC_INTIE_IE_Msk                      (0x1UL << CLIC_INTIE_IE_Pos)             /*!< CLIC INTIE: IE Mask */

#define CLIC_INTATTR_TRIG_Pos                  1U                                       /*!< CLIC INTATTR: TRIG Position */
#define CLIC_INTATTR_TRIG_Msk                  (0x3UL << CLIC_INTATTR_TRIG_Pos)         /*!< CLIC INTATTR: TRIG Mask */

#define CLIC_INTATTR_SHV_Pos                   0U                                       /*!< CLIC INTATTR: SHV Position */
#define CLIC_INTATTR_SHV_Msk                   (0x1UL << CLIC_INTATTR_SHV_Pos)          /*!< CLIC INTATTR: SHV Mask */

#define ECLIC_MAX_NLBITS                       8U                                       /*!< Max nlbit of the CLICINTCTLBITS */
#define ECLIC_MODE_MTVEC_Msk                   3U                                       /*!< ECLIC Mode mask for MTVT CSR Register */

#define ECLIC_NON_VECTOR_INTERRUPT             0x0                                      /*!< Non-Vector Interrupt Mode of ECLIC */
#define ECLIC_VECTOR_INTERRUPT                 0x1                                      /*!< Vector Interrupt Mode of ECLIC */

/**\brief ECLIC Trigger Enum for different Trigger Type */
typedef enum ECLIC_TRIGGER {
	ECLIC_LEVEL_TRIGGER             = 0x0,  /*!< Level Triggerred, trig[0] = 0 */
	ECLIC_POSTIVE_EDGE_TRIGGER      = 0x1,  /*!< Postive/Rising Edge Triggered, trig[1] = 1, trig[0] = 0 */
	ECLIC_NEGTIVE_EDGE_TRIGGER      = 0x3,  /*!< Negtive/Falling Edge Triggered, trig[1] = 1, trig[0] = 0 */
	ECLIC_MAX_TRIGGER               = 0x3   /*!< MAX Supported Trigger Mode */
} ECLIC_TRIGGER_Type;

#ifndef __ECLIC_BASEADDR
/* Base address of ECLIC(__ECLIC_BASEADDR) should be defined in <Device.h> */
#error "__ECLIC_BASEADDR is not defined, please check!"
#endif

#ifndef __ECLIC_INTCTLBITS
/* Define __ECLIC_INTCTLBITS to get via ECLIC->INFO if not defined */
#define __ECLIC_INTCTLBITS                  (__ECLIC_GetInfoCtlbits())
#endif

/* ECLIC Memory mapping of Device */
#define ECLIC_BASE                          __ECLIC_BASEADDR                            /*!< ECLIC Base Address */
#define ECLIC                               ((CLIC_Type *) ECLIC_BASE)                  /*!< CLIC configuration struct */

/** @} */ /* end of group NMSIS_Core_ECLIC_Registers */

/* ##########################   ECLIC functions  #################################### */
/**
 * \defgroup   NMSIS_Core_IntExc        Interrupts and Exceptions
 * \brief Functions that manage interrupts and exceptions via the ECLIC.
 *
 * @{
 */

/**
 * \brief  Definition of IRQn numbers
 * \details
 * The core interrupt enumeration names for IRQn values are defined in the file <b><Device>.h</b>.
 * - Interrupt ID(IRQn) from 0 to 18 are reserved for core internal interrupts.
 * - Interrupt ID(IRQn) start from 19 represent device-specific external interrupts.
 * - The first device-specific interrupt has the IRQn value 19.
 *
 * The table below describes the core interrupt names and their availability in various Nuclei Cores.
 */
/* The following enum IRQn definition in this file
 * is only used for doxygen documentation generation,
 * The <Device>.h is the real file to define it by vendor
 */
#if defined(__ONLY_FOR_DOXYGEN_DOCUMENT_GENERATION__)
typedef enum IRQn {
	/* ========= Nuclei N/NX Core Specific Interrupt Numbers  =========== */
	/* Core Internal Interrupt IRQn definitions */
	Reserved0_IRQn                          =   0,  /*!<  Internal reserved */
	Reserved1_IRQn                          =   1,  /*!<  Internal reserved */
	Reserved2_IRQn                          =   2,  /*!<  Internal reserved */
	SysTimerSW_IRQn                         =   3,  /*!<  System Timer SW interrupt */
	Reserved3_IRQn                          =   4,  /*!<  Internal reserved */
	Reserved4_IRQn                          =   5,  /*!<  Internal reserved */
	Reserved5_IRQn                          =   6,  /*!<  Internal reserved */
	SysTimer_IRQn                           =   7,  /*!<  System Timer Interrupt */
	Reserved6_IRQn                          =   8,  /*!<  Internal reserved */
	Reserved7_IRQn                          =   9,  /*!<  Internal reserved */
	Reserved8_IRQn                          =  10,  /*!<  Internal reserved */
	Reserved9_IRQn                          =  11,  /*!<  Internal reserved */
	Reserved10_IRQn                         =  12,  /*!<  Internal reserved */
	Reserved11_IRQn                         =  13,  /*!<  Internal reserved */
	Reserved12_IRQn                         =  14,  /*!<  Internal reserved */
	Reserved13_IRQn                         =  15,  /*!<  Internal reserved */
	Reserved14_IRQn                         =  16,  /*!<  Internal reserved */
	Reserved15_IRQn                         =  17,  /*!<  Internal reserved */
	Reserved16_IRQn                         =  18,  /*!<  Internal reserved */

	/* ========= Device Specific Interrupt Numbers  =================== */
	/* ToDo: add here your device specific external interrupt numbers.
	 * 19~max(NUM_INTERRUPT, 1023) is reserved number for user.
	 * Maxmum interrupt supported could get from clicinfo.NUM_INTERRUPT.
	 * According the interrupt handlers defined in startup_Device.S
	 * eg.: Interrupt for Timer#1       eclic_tim1_handler   ->   TIM1_IRQn */
	FirstDeviceSpecificInterrupt_IRQn       = 19,   /*!< First Device Specific Interrupt */
	SOC_INT_MAX,                                    /*!< Number of total interrupts */
} IRQn_Type;
#endif /* __ONLY_FOR_DOXYGEN_DOCUMENT_GENERATION__ */

#ifdef NMSIS_ECLIC_VIRTUAL
    #ifndef NMSIS_ECLIC_VIRTUAL_HEADER_FILE
	#define NMSIS_ECLIC_VIRTUAL_HEADER_FILE "nmsis_eclic_virtual.h"
    #endif
    #include NMSIS_ECLIC_VIRTUAL_HEADER_FILE
#else
    #define ECLIC_SetCfgNlbits            __ECLIC_SetCfgNlbits
    #define ECLIC_GetCfgNlbits            __ECLIC_GetCfgNlbits
    #define ECLIC_GetInfoVer              __ECLIC_GetInfoVer
    #define ECLIC_GetInfoCtlbits          __ECLIC_GetInfoCtlbits
    #define ECLIC_GetInfoNum              __ECLIC_GetInfoNum
    #define ECLIC_SetMth                  __ECLIC_SetMth
    #define ECLIC_GetMth                  __ECLIC_GetMth
    #define ECLIC_EnableIRQ               __ECLIC_EnableIRQ
    #define ECLIC_GetEnableIRQ            __ECLIC_GetEnableIRQ
    #define ECLIC_DisableIRQ              __ECLIC_DisableIRQ
    #define ECLIC_SetPendingIRQ           __ECLIC_SetPendingIRQ
    #define ECLIC_GetPendingIRQ           __ECLIC_GetPendingIRQ
    #define ECLIC_ClearPendingIRQ         __ECLIC_ClearPendingIRQ
    #define ECLIC_SetTrigIRQ              __ECLIC_SetTrigIRQ
    #define ECLIC_GetTrigIRQ              __ECLIC_GetTrigIRQ
    #define ECLIC_SetShvIRQ               __ECLIC_SetShvIRQ
    #define ECLIC_GetShvIRQ               __ECLIC_GetShvIRQ
    #define ECLIC_SetCtrlIRQ              __ECLIC_SetCtrlIRQ
    #define ECLIC_GetCtrlIRQ              __ECLIC_GetCtrlIRQ
    #define ECLIC_SetLevelIRQ             __ECLIC_SetLevelIRQ
    #define ECLIC_GetLevelIRQ             __ECLIC_GetLevelIRQ
    #define ECLIC_SetPriorityIRQ          __ECLIC_SetPriorityIRQ
    #define ECLIC_GetPriorityIRQ          __ECLIC_GetPriorityIRQ

#endif /* NMSIS_ECLIC_VIRTUAL */

#ifdef NMSIS_VECTAB_VIRTUAL
    #ifndef NMSIS_VECTAB_VIRTUAL_HEADER_FILE
	#define NMSIS_VECTAB_VIRTUAL_HEADER_FILE "nmsis_vectab_virtual.h"
    #endif
    #include NMSIS_VECTAB_VIRTUAL_HEADER_FILE
#else
    #define ECLIC_SetVector              __ECLIC_SetVector
    #define ECLIC_GetVector              __ECLIC_GetVector
#endif  /* (NMSIS_VECTAB_VIRTUAL) */

/**
 * \brief  Set nlbits value
 * \details
 * This function set the nlbits value of CLICCFG register.
 * \param [in]    nlbits    nlbits value
 * \remarks
 * - nlbits is used to set the width of level in the CLICINTCTL[i].
 * \sa
 * - \ref ECLIC_GetCfgNlbits
 */
__STATIC_FORCEINLINE void __ECLIC_SetCfgNlbits(uint32_t nlbits)
{
	ECLIC->CFG &= ~CLIC_CLICCFG_NLBIT_Msk;
	ECLIC->CFG |= (uint8_t)((nlbits << CLIC_CLICCFG_NLBIT_Pos) & CLIC_CLICCFG_NLBIT_Msk);
}

/**
 * \brief  Get nlbits value
 * \details
 * This function get the nlbits value of CLICCFG register.
 * \return   nlbits value of CLICCFG register
 * \remarks
 * - nlbits is used to set the width of level in the CLICINTCTL[i].
 * \sa
 * - \ref ECLIC_SetCfgNlbits
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetCfgNlbits(void)
{
	return ((uint32_t)((ECLIC->CFG & CLIC_CLICCFG_NLBIT_Msk) >> CLIC_CLICCFG_NLBIT_Pos));
}

/**
 * \brief  Get the ECLIC version number
 * \details
 * This function gets the hardware version information from CLICINFO register.
 * \return   hardware version number in CLICINFO register.
 * \remarks
 * - This function gets harware version information from CLICINFO register.
 * - Bit 20:17 for architecture version, bit 16:13 for implementation version.
 * \sa
 * - \ref ECLIC_GetInfoNum
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetInfoVer(void)
{
	return ((uint32_t)((ECLIC->INFO & CLIC_CLICINFO_VER_Msk) >> CLIC_CLICINFO_VER_Pos));
}

/**
 * \brief  Get CLICINTCTLBITS
 * \details
 * This function gets CLICINTCTLBITS from CLICINFO register.
 * \return  CLICINTCTLBITS from CLICINFO register.
 * \remarks
 * - In the CLICINTCTL[i] registers, with 2 <= CLICINTCTLBITS <= 8.
 * - The implemented bits are kept left-justified in the most-significant bits of each 8-bit
 *   CLICINTCTL[I] register, with the lower unimplemented bits treated as hardwired to 1.
 * \sa
 * - \ref ECLIC_GetInfoNum
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetInfoCtlbits(void)
{
	return ((uint32_t)((ECLIC->INFO & CLIC_CLICINFO_CTLBIT_Msk) >> CLIC_CLICINFO_CTLBIT_Pos));
}

/**
 * \brief  Get number of maximum interrupt inputs supported
 * \details
 * This function gets number of maximum interrupt inputs supported from CLICINFO register.
 * \return  number of maximum interrupt inputs supported from CLICINFO register.
 * \remarks
 * - This function gets number of maximum interrupt inputs supported from CLICINFO register.
 * - The num_interrupt field specifies the actual number of maximum interrupt inputs supported in this implementation.
 * \sa
 * - \ref ECLIC_GetInfoCtlbits
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetInfoNum(void)
{
	return ((uint32_t)((ECLIC->INFO & CLIC_CLICINFO_NUM_Msk) >> CLIC_CLICINFO_NUM_Pos));
}

/**
 * \brief  Set Machine Mode Interrupt Level Threshold
 * \details
 * This function sets machine mode interrupt level threshold.
 * \param [in]  mth       Interrupt Level Threshold.
 * \sa
 * - \ref ECLIC_GetMth
 */
__STATIC_FORCEINLINE void __ECLIC_SetMth(uint8_t mth)
{
	ECLIC->MTH = mth;
}

/**
 * \brief  Get Machine Mode Interrupt Level Threshold
 * \details
 * This function gets machine mode interrupt level threshold.
 * \return       Interrupt Level Threshold.
 * \sa
 * - \ref ECLIC_SetMth
 */
__STATIC_FORCEINLINE uint8_t __ECLIC_GetMth(void)
{
	return (ECLIC->MTH);
}


/**
 * \brief  Enable a specific interrupt
 * \details
 * This function enables the specific interrupt \em IRQn.
 * \param [in]  IRQn  Interrupt number
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_DisableIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_EnableIRQ(IRQn_Type IRQn)
{
	ECLIC->CTRL[IRQn].INTIE |= CLIC_INTIE_IE_Msk;
}

/**
 * \brief  Get a specific interrupt enable status
 * \details
 * This function returns the interrupt enable status for the specific interrupt \em IRQn.
 * \param [in]  IRQn  Interrupt number
 * \returns
 * - 0  Interrupt is not enabled
 * - 1  Interrupt is pending
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_EnableIRQ
 * - \ref ECLIC_DisableIRQ
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetEnableIRQ(IRQn_Type IRQn)
{
	return((uint32_t) (ECLIC->CTRL[IRQn].INTIE) & CLIC_INTIE_IE_Msk);
}

/**
 * \brief  Disable a specific interrupt
 * \details
 * This function disables the specific interrupt \em IRQn.
 * \param [in]  IRQn  Number of the external interrupt to disable
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_EnableIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_DisableIRQ(IRQn_Type IRQn)
{
	ECLIC->CTRL[IRQn].INTIE &= ~CLIC_INTIE_IE_Msk;
}

/**
 * \brief  Get the pending specific interrupt
 * \details
 * This function returns the pending status of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \returns
 * - 0  Interrupt is not pending
 * - 1  Interrupt is pending
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetPendingIRQ
 * - \ref ECLIC_ClearPendingIRQ
 */
__STATIC_FORCEINLINE int32_t __ECLIC_GetPendingIRQ(IRQn_Type IRQn)
{
	return((uint32_t)(ECLIC->CTRL[IRQn].INTIP) & CLIC_INTIP_IP_Msk);
}

/**
 * \brief  Set a specific interrupt to pending
 * \details
 * This function sets the pending bit for the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_GetPendingIRQ
 * - \ref ECLIC_ClearPendingIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetPendingIRQ(IRQn_Type IRQn)
{
	ECLIC->CTRL[IRQn].INTIP |= CLIC_INTIP_IP_Msk;
}

/**
 * \brief  Clear a specific interrupt from pending
 * \details
 * This function removes the pending state of the specific interrupt \em IRQn.
 * \em IRQn cannot be a negative number.
 * \param [in]      IRQn  Interrupt number
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetPendingIRQ
 * - \ref ECLIC_GetPendingIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_ClearPendingIRQ(IRQn_Type IRQn)
{
	ECLIC->CTRL[IRQn].INTIP &= ~CLIC_INTIP_IP_Msk;
}

/**
 * \brief  Set trigger mode and polarity for a specific interrupt
 * \details
 * This function set trigger mode and polarity of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      trig
 *                   - 00  level trigger, \ref ECLIC_LEVEL_TRIGGER
 *                   - 01  positive edge trigger, \ref ECLIC_POSTIVE_EDGE_TRIGGER
 *                   - 02  level trigger, \ref ECLIC_LEVEL_TRIGGER
 *                   - 03  negative edge trigger, \ref ECLIC_NEGTIVE_EDGE_TRIGGER
 * \remarks
 * - IRQn must not be negative.
 *
 * \sa
 * - \ref ECLIC_GetTrigIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetTrigIRQ(IRQn_Type IRQn, uint32_t trig)
{
	ECLIC->CTRL[IRQn].INTATTR &= ~CLIC_INTATTR_TRIG_Msk;
	ECLIC->CTRL[IRQn].INTATTR |= (uint8_t)(trig << CLIC_INTATTR_TRIG_Pos);
}

/**
 * \brief  Get trigger mode and polarity for a specific interrupt
 * \details
 * This function get trigger mode and polarity of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return
 *                 - 00  level trigger, \ref ECLIC_LEVEL_TRIGGER
 *                 - 01  positive edge trigger, \ref ECLIC_POSTIVE_EDGE_TRIGGER
 *                 - 02  level trigger, \ref ECLIC_LEVEL_TRIGGER
 *                 - 03  negative edge trigger, \ref ECLIC_NEGTIVE_EDGE_TRIGGER
 * \remarks
 *     - IRQn must not be negative.
 * \sa
 *     - \ref ECLIC_SetTrigIRQ
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetTrigIRQ(IRQn_Type IRQn)
{
	return ((int32_t)(((ECLIC->CTRL[IRQn].INTATTR) & CLIC_INTATTR_TRIG_Msk) >> CLIC_INTATTR_TRIG_Pos));
}

/**
 * \brief  Set interrupt working mode for a specific interrupt
 * \details
 * This function set selective hardware vector or non-vector working mode of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      shv
 *                        - 0  non-vector mode, \ref ECLIC_NON_VECTOR_INTERRUPT
 *                        - 1  vector mode, \ref ECLIC_VECTOR_INTERRUPT
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_GetShvIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetShvIRQ(IRQn_Type IRQn, uint32_t shv)
{
	ECLIC->CTRL[IRQn].INTATTR &= ~CLIC_INTATTR_SHV_Msk;
	ECLIC->CTRL[IRQn].INTATTR |= (uint8_t)(shv << CLIC_INTATTR_SHV_Pos);
}

/**
 * \brief  Get interrupt working mode for a specific interrupt
 * \details
 * This function get selective hardware vector or non-vector working mode of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return       shv
 *                        - 0  non-vector mode, \ref ECLIC_NON_VECTOR_INTERRUPT
 *                        - 1  vector mode, \ref ECLIC_VECTOR_INTERRUPT
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetShvIRQ
 */
__STATIC_FORCEINLINE uint32_t __ECLIC_GetShvIRQ(IRQn_Type IRQn)
{
	return ((int32_t)(((ECLIC->CTRL[IRQn].INTATTR) & CLIC_INTATTR_SHV_Msk) >> CLIC_INTATTR_SHV_Pos));
}

/**
 * \brief  Modify ECLIC Interrupt Input Control Register for a specific interrupt
 * \details
 * This function modify ECLIC Interrupt Input Control(CLICINTCTL[i]) register of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      intctrl  Set value for CLICINTCTL[i] register
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_GetCtrlIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetCtrlIRQ(IRQn_Type IRQn, uint8_t intctrl)
{
	ECLIC->CTRL[IRQn].INTCTRL = intctrl;
}

/**
 * \brief  Get ECLIC Interrupt Input Control Register value for a specific interrupt
 * \details
 * This function modify ECLIC Interrupt Input Control register of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return       value of ECLIC Interrupt Input Control register
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetCtrlIRQ
 */
__STATIC_FORCEINLINE uint8_t __ECLIC_GetCtrlIRQ(IRQn_Type IRQn)
{
	return (ECLIC->CTRL[IRQn].INTCTRL);
}

/**
 * \brief  Set ECLIC Interrupt level of a specific interrupt
 * \details
 * This function set interrupt level of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      lvl_abs   Interrupt level
 * \remarks
 * - IRQn must not be negative.
 * - If lvl_abs to be set is larger than the max level allowed, it will be force to be max level.
 * - When you set level value you need use clciinfo.nlbits to get the width of level.
 *   Then we could know the maximum of level. CLICINTCTLBITS is how many total bits are
 *   present in the CLICINTCTL register.
 * \sa
 * - \ref ECLIC_GetLevelIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetLevelIRQ(IRQn_Type IRQn, uint8_t lvl_abs)
{
	uint8_t nlbits = __ECLIC_GetCfgNlbits();
	uint8_t intctlbits = (uint8_t)__ECLIC_INTCTLBITS;

	if (nlbits == 0) {
		return;
	}

	if (nlbits > intctlbits) {
		nlbits = intctlbits;
	}
	uint8_t maxlvl = ((1 << nlbits) - 1);
	if (lvl_abs > maxlvl) {
		lvl_abs = maxlvl;
	}
	uint8_t lvl = lvl_abs << (ECLIC_MAX_NLBITS - nlbits);
	uint8_t cur_ctrl = __ECLIC_GetCtrlIRQ(IRQn);
	cur_ctrl = cur_ctrl << nlbits;
	cur_ctrl = cur_ctrl >> nlbits;
	__ECLIC_SetCtrlIRQ(IRQn, (cur_ctrl | lvl));
}

/**
 * \brief  Get ECLIC Interrupt level of a specific interrupt
 * \details
 * This function get interrupt level of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return         Interrupt level
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetLevelIRQ
 */
__STATIC_FORCEINLINE uint8_t __ECLIC_GetLevelIRQ(IRQn_Type IRQn)
{
	uint8_t nlbits = __ECLIC_GetCfgNlbits();
	uint8_t intctlbits = (uint8_t)__ECLIC_INTCTLBITS;

	if (nlbits == 0) {
		return 0;
	}

	if (nlbits > intctlbits) {
		nlbits = intctlbits;
	}
	uint8_t intctrl = __ECLIC_GetCtrlIRQ(IRQn);
	uint8_t lvl_abs = intctrl >> (ECLIC_MAX_NLBITS - nlbits);
	return lvl_abs;
}

/**
 * \brief  Get ECLIC Interrupt priority of a specific interrupt
 * \details
 * This function get interrupt priority of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      pri   Interrupt priority
 * \remarks
 * - IRQn must not be negative.
 * - If pri to be set is larger than the max priority allowed, it will be force to be max priority.
 * - Priority width is CLICINTCTLBITS minus clciinfo.nlbits if clciinfo.nlbits
 *   is less than CLICINTCTLBITS. Otherwise priority width is 0.
 * \sa
 * - \ref ECLIC_GetPriorityIRQ
 */
__STATIC_FORCEINLINE void __ECLIC_SetPriorityIRQ(IRQn_Type IRQn, uint8_t pri)
{
	uint8_t nlbits = __ECLIC_GetCfgNlbits();
	uint8_t intctlbits = (uint8_t)__ECLIC_INTCTLBITS;

	if (nlbits < intctlbits) {
		uint8_t maxpri = ((1 << (intctlbits - nlbits)) - 1);
		if (pri > maxpri) {
			pri = maxpri;
		}
		pri = pri << (ECLIC_MAX_NLBITS - intctlbits);
		uint8_t mask = ((uint8_t)(-1)) >> intctlbits;
		pri = pri | mask;
		uint8_t cur_ctrl = __ECLIC_GetCtrlIRQ(IRQn);
		cur_ctrl = cur_ctrl >> (ECLIC_MAX_NLBITS - nlbits);
		cur_ctrl = cur_ctrl << (ECLIC_MAX_NLBITS - nlbits);
		__ECLIC_SetCtrlIRQ(IRQn, (cur_ctrl | pri));
	}
}

/**
 * \brief  Get ECLIC Interrupt priority of a specific interrupt
 * \details
 * This function get interrupt priority of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return   Interrupt priority
 * \remarks
 * - IRQn must not be negative.
 * \sa
 * - \ref ECLIC_SetPriorityIRQ
 */
__STATIC_FORCEINLINE uint8_t __ECLIC_GetPriorityIRQ(IRQn_Type IRQn)
{
	uint8_t nlbits = __ECLIC_GetCfgNlbits();
	uint8_t intctlbits = (uint8_t)__ECLIC_INTCTLBITS;

	if (nlbits < intctlbits) {
		uint8_t cur_ctrl = __ECLIC_GetCtrlIRQ(IRQn);
		uint8_t pri = cur_ctrl << nlbits;
		pri = pri >> nlbits;
		pri = pri >> (ECLIC_MAX_NLBITS - intctlbits);
		return pri;
	} else {
		return 0;
	}
}

/**
 * \brief  Set Interrupt Vector of a specific interrupt
 * \details
 * This function set interrupt handler address of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \param [in]      vector   Interrupt handler address
 * \remarks
 * - IRQn must not be negative.
 * - You can set the \ref CSR_CSR_MTVT to set interrupt vector table entry address.
 * - If your vector table is placed in readonly section, the vector for IRQn will not be modified.
 *   For this case, you need to use the correct irq handler name defined in your vector table as
 *   your irq handler function name.
 * - This function will only work correctly when the vector table is placed in an read-write enabled section.
 * \sa
 * - \ref ECLIC_GetVector
 */
__STATIC_FORCEINLINE void __ECLIC_SetVector(IRQn_Type IRQn, rv_csr_t vector)
{
#if __RISCV_XLEN == 32
	volatile uint32_t vec_base;
	vec_base = ((uint32_t)__RV_CSR_READ(CSR_MTVT));
	(*(unsigned long *) (vec_base + ((int32_t)IRQn) * 4)) = vector;
#elif __RISCV_XLEN == 64
	volatile uint64_t vec_base;
	vec_base = ((uint64_t)__RV_CSR_READ(CSR_MTVT));
	(*(unsigned long *) (vec_base + ((int32_t)IRQn) * 8)) = vector;
#else // TODO Need cover for XLEN=128 case in future
	volatile uint64_t vec_base;
	vec_base = ((uint64_t)__RV_CSR_READ(CSR_MTVT));
	(*(unsigned long *) (vec_base + ((int32_t)IRQn) * 8)) = vector;
#endif
}

/**
 * \brief  Get Interrupt Vector of a specific interrupt
 * \details
 * This function get interrupt handler address of the specific interrupt \em IRQn.
 * \param [in]      IRQn  Interrupt number
 * \return        Interrupt handler address
 * \remarks
 * - IRQn must not be negative.
 * - You can read \ref CSR_CSR_MTVT to get interrupt vector table entry address.
 * \sa
 *     - \ref ECLIC_SetVector
 */
__STATIC_FORCEINLINE rv_csr_t __ECLIC_GetVector(IRQn_Type IRQn)
{
#if __RISCV_XLEN == 32
	return (*(uint32_t *)(__RV_CSR_READ(CSR_MTVT) + IRQn * 4));
#elif __RISCV_XLEN == 64
	return (*(uint64_t *)(__RV_CSR_READ(CSR_MTVT) + IRQn * 8));
#else // TODO Need cover for XLEN=128 case in future
	return (*(uint64_t *)(__RV_CSR_READ(CSR_MTVT) + IRQn * 8));
#endif
}

/**
 * \brief  Set Exception entry address
 * \details
 * This function set exception handler address to 'CSR_MTVEC'.
 * \param [in]      addr  Exception handler address
 * \remarks
 * - This function use to set exception handler address to 'CSR_MTVEC'. Address is 4 bytes align.
 * \sa
 * - \ref __get_exc_entry
 */
__STATIC_FORCEINLINE void __set_exc_entry(rv_csr_t addr)
{
	addr &= (rv_csr_t)(~0x3F);
	addr |= ECLIC_MODE_MTVEC_Msk;
	__RV_CSR_WRITE(CSR_MTVEC, addr);
}

/**
 * \brief  Get Exception entry address
 * \details
 * This function get exception handler address from 'CSR_MTVEC'.
 * \return       Exception handler address
 * \remarks
 * - This function use to get exception handler address from 'CSR_MTVEC'. Address is 4 bytes align
 * \sa
 * - \ref __set_exc_entry
 */
__STATIC_FORCEINLINE rv_csr_t __get_exc_entry(void)
{
	unsigned long addr = __RV_CSR_READ(CSR_MTVEC);

	return (addr & ~ECLIC_MODE_MTVEC_Msk);
}

/**
 * \brief  Set Non-vector interrupt entry address
 * \details
 * This function set Non-vector interrupt address.
 * \param [in]      addr  Non-vector interrupt entry address
 * \remarks
 * - This function use to set non-vector interrupt entry address to 'CSR_MTVT2' if
 * - CSR_MTVT2 bit0 is 1. If 'CSR_MTVT2' bit0 is 0 then set address to 'CSR_MTVEC'
 * \sa
 * - \ref __get_nonvec_entry
 */
__STATIC_FORCEINLINE void __set_nonvec_entry(rv_csr_t addr)
{
	if (__RV_CSR_READ(CSR_MTVT2) & 0x1) {
		__RV_CSR_WRITE(CSR_MTVT2, addr | 0x01);
	} else {
		addr &= (rv_csr_t)(~0x3F);
		addr |= ECLIC_MODE_MTVEC_Msk;
		__RV_CSR_WRITE(CSR_MTVEC, addr);
	}
}

/**
 * \brief  Get Non-vector interrupt entry address
 * \details
 * This function get Non-vector interrupt address.
 * \return      Non-vector interrupt handler address
 * \remarks
 * - This function use to get non-vector interrupt entry address from 'CSR_MTVT2' if
 * - CSR_MTVT2 bit0 is 1. If 'CSR_MTVT2' bit0 is 0 then get address from 'CSR_MTVEC'.
 * \sa
 * - \ref __set_nonvec_entry
 */
__STATIC_FORCEINLINE rv_csr_t __get_nonvec_entry(void)
{
	if (__RV_CSR_READ(CSR_MTVT2) & 0x1) {
		return __RV_CSR_READ(CSR_MTVT2) & (~(rv_csr_t)(0x1));
	} else {
		rv_csr_t addr = __RV_CSR_READ(CSR_MTVEC);
		return (addr & ~ECLIC_MODE_MTVEC_Msk);
	}
}

/**
 * \brief  Get NMI interrupt entry from 'CSR_MNVEC'
 * \details
 * This function get NMI interrupt address from 'CSR_MNVEC'.
 * \return      NMI interrupt handler address
 * \remarks
 * - This function use to get NMI interrupt handler address from 'CSR_MNVEC'. If CSR_MMISC_CTL[9] = 1 'CSR_MNVEC'
 * - will be equal as mtvec. If CSR_MMISC_CTL[9] = 0 'CSR_MNVEC' will be equal as reset vector.
 * - NMI entry is defined via \ref CSR_MMISC_CTL, writing to \ref CSR_MNVEC will be ignored.
 */
__STATIC_FORCEINLINE rv_csr_t __get_nmi_entry(void)
{
	return __RV_CSR_READ(CSR_MNVEC);
}

/**
 * \brief   Save necessary CSRs into variables for vector interrupt nesting
 * \details
 * This macro is used to declare variables which are used for saving
 * CSRs(MCAUSE, MEPC, MSUB), and it will read these CSR content into
 * these variables, it need to be used in a vector-interrupt if nesting
 * is required.
 * \remarks
 * - Interrupt will be enabled after this macro is called
 * - It need to be used together with \ref RESTORE_IRQ_CSR_CONTEXT
 * - Don't use variable names __mcause, __mpec, __msubm in your ISR code
 * - If you want to enable interrupt nesting feature for vector interrupt,
 * you can do it like this:
 * \code
 * // __INTERRUPT attribute will generates function entry and exit sequences suitable
 * // for use in an interrupt handler when this attribute is present
 * __INTERRUPT void eclic_mtip_handler(void)
 * {
 *     // Must call this to save CSRs
 *     SAVE_IRQ_CSR_CONTEXT();
 *     // !!!Interrupt is enabled here!!!
 *     // !!!Higher priority interrupt could nest it!!!
 *
 *     // put you own interrupt handling code here
 *
 *     // Must call this to restore CSRs
 *     RESTORE_IRQ_CSR_CONTEXT();
 * }
 * \endcode
 */
#define SAVE_IRQ_CSR_CONTEXT()			       \
	rv_csr_t __mcause = __RV_CSR_READ(CSR_MCAUSE); \
	rv_csr_t __mepc = __RV_CSR_READ(CSR_MEPC);     \
	rv_csr_t __msubm = __RV_CSR_READ(CSR_MSUBM);   \
	__enable_irq();

/**
 * \brief   Restore necessary CSRs from variables for vector interrupt nesting
 * \details
 * This macro is used restore CSRs(MCAUSE, MEPC, MSUB) from pre-defined variables
 * in \ref SAVE_IRQ_CSR_CONTEXT macro.
 * \remarks
 * - Interrupt will be disabled after this macro is called
 * - It need to be used together with \ref SAVE_IRQ_CSR_CONTEXT
 */
#define RESTORE_IRQ_CSR_CONTEXT()	    \
	__disable_irq();		    \
	__RV_CSR_WRITE(CSR_MSUBM, __msubm); \
	__RV_CSR_WRITE(CSR_MEPC, __mepc);   \
	__RV_CSR_WRITE(CSR_MCAUSE, __mcause);

/** @} */ /* End of Doxygen Group NMSIS_Core_IntExc */

#endif /* defined(__ECLIC_PRESENT) && (__ECLIC_PRESENT == 1) */

#ifdef __cplusplus
}
#endif
#endif /** __CORE_FEATURE_ECLIC__ */
