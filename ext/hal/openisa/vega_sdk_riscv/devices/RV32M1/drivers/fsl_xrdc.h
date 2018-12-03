/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_XRDC_H_
#define _FSL_XRDC_H_

#include "fsl_common.h"

/*!
 * @addtogroup xrdc
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/
#define FSL_XRDC_DRIVER_VERSION (MAKE_VERSION(2, 0, 3)) /*!< Version 2.0.3. */

#ifndef FSL_CLOCK_XRDC_GATE_COUNT
#define FSL_CLOCK_XRDC_GATE_COUNT 1U
#endif

/*! @brief XRDC status.  */
enum _xrdc_status
{
    kStatus_XRDC_NoError = MAKE_STATUS(kStatusGroup_XRDC, 0) /*!< No error captured. */
};

/*!
 * @brief XRDC hardware configuration.
 */
typedef struct _xrdc_hardware_config
{
    uint8_t masterNumber; /*!< Number of bus masters. */
    uint8_t domainNumber; /*!< Number of domains.     */
    uint8_t pacNumber;    /*!< Number of PACs.        */
    uint8_t mrcNumber;    /*!< Number of MRCs.        */
} xrdc_hardware_config_t;

/*!
 * @brief XRDC PID enable mode, the register bit XRDC_MDA_Wx[PE], used for domain hit evaluation.
 */
typedef enum _xrdc_pid_enable
{
    kXRDC_PidDisable,  /*!< PID is not used in domain hit evalution. */
    kXRDC_PidDisable1, /*!< PID is not used in domain hit evalution. */
    kXRDC_PidExp0,     /*!<  ((XRDC_MDA_W[PID] & ~XRDC_MDA_W[PIDM]) == (XRDC_PID[PID] & ~XRDC_MDA_W[PIDM])). */
    kXRDC_PidExp1      /*!< ~((XRDC_MDA_W[PID] & ~XRDC_MDA_W[PIDM]) == (XRDC_PID[PID] & ~XRDC_MDA_W[PIDM])). */
} xrdc_pid_enable_t;

/*!
 * @brief XRDC domain ID select method, the register bit XRDC_MDA_Wx[DIDS], used for
 * domain hit evaluation.
 */
typedef enum _xrdc_did_sel
{
    kXRDC_DidMda,         /*!< Use MDAn[3:0] as DID. */
    kXRDC_DidInput,       /*!< Use the input DID (DID_in) as DID. */
    kXRDC_DidMdaAndInput, /*!< Use MDAn[3:2] concatenated with DID_in[1:0] as DID. */
    kXRDC_DidReserved     /*!< Reserved. */
} xrdc_did_sel_t;

/*!
 * @brief XRDC secure attribute, the register bit XRDC_MDA_Wx[SA], used for non-processor
 * bus master domain assignment.
 */
typedef enum _xrdc_secure_attr
{
    kXRDC_ForceSecure,    /*!< Force the bus attribute for this master to secure.        */
    kXRDC_ForceNonSecure, /*!< Force the bus attribute for this master to non-secure.    */
    kXRDC_MasterSecure,   /*!< Use the bus master's secure/nonsecure attribute directly. */
    kXRDC_MasterSecure1,  /*!< Use the bus master's secure/nonsecure attribute directly. */
} xrdc_secure_attr_t;

/*!
 * @brief XRDC privileged attribute, the register bit XRDC_MDA_Wx[PA], used for non-processor
 * bus master domain assignment.
 */
typedef enum _xrdc_privilege_attr
{
    kXRDC_ForceUser,        /*!< Force the bus attribute for this master to user.       */
    kXRDC_ForcePrivilege,   /*!< Force the bus attribute for this master to privileged. */
    kXRDC_MasterPrivilege,  /*!< Use the bus master's attribute directly. */
    kXRDC_MasterPrivilege1, /*!< Use the bus master's attribute directly. */
} xrdc_privilege_attr_t;

/*!
 * @brief Domain assignment for the processor bus master.
 */
typedef struct _xrdc_processor_domain_assignment
{
    uint32_t domainId : 4U;       /*!< Domain ID.            */
    uint32_t domainIdSelect : 2U; /*!< Domain ID select method, see @ref xrdc_did_sel_t. */
    uint32_t pidEnable : 2U;      /*!< PId enable method, see @ref xrdc_pid_enable_t. */
    uint32_t pidMask : 6U;        /*!< PId mask.              */
    uint32_t : 2U;                /*!< Reserved. */
    uint32_t pid : 6U;            /*!< PId value.            */
    uint32_t : 2U;                /*!< Reserved. */
#if !(defined(FSL_FEATURE_XRDC_NO_MDA_LPID) && FSL_FEATURE_XRDC_NO_MDA_LPID)
    uint32_t logicPartId : 4U; /*!< Logical partition ID. */
#else
    uint32_t : 4U;  /*!< Reserved. */
#endif
#if !(defined(FSL_FEATURE_XRDC_NO_MDA_LPE) && FSL_FEATURE_XRDC_NO_MDA_LPE)
    uint32_t enableLogicPartId : 1U; /*!< Logical partition ID. */
#else
    uint32_t : 1U;  /*!< Reserved. */
#endif                  /* FSL_FEATURE_XRDC_NO_MDA_LPE */
    uint32_t : 1U;      /*!< Reserved. */
    uint32_t lock : 1U; /*!< Lock the register.    */
    uint32_t : 1U;      /*!< Reserved. */
} xrdc_processor_domain_assignment_t;

/*!
 * @brief Domain assignment for the non-processor bus master.
 */
typedef struct _xrdc_non_processor_domain_assignment
{
    uint32_t domainId : 4U;       /*!< Domain ID.            */
    uint32_t privilegeAttr : 2U;  /*!< Privileged attribute, see @ref xrdc_privilege_attr_t. */
    uint32_t secureAttr : 2U;     /*!< Secure attribute, see @ref xrdc_secure_attr_t. */
    uint32_t bypassDomainId : 1U; /*!< Bypass domain ID.     */
    uint32_t : 15U;               /*!< Reserved. */
#if !(defined(FSL_FEATURE_XRDC_NO_MDA_LPID) && FSL_FEATURE_XRDC_NO_MDA_LPID)
    uint32_t logicPartId : 4U; /*!< Logical partition ID. */
#else
    uint32_t : 4U;  /*!< Reserved. */
#endif
#if !(defined(FSL_FEATURE_XRDC_NO_MDA_LPE) && FSL_FEATURE_XRDC_NO_MDA_LPE)
    uint32_t enableLogicPartId : 1U; /*!< Enable logical partition ID. */
#else
    uint32_t : 1U;  /*!< Reserved. */
#endif
    uint32_t : 1U;      /*!< Reserved. */
    uint32_t lock : 1U; /*!< Lock the register.    */
    uint32_t : 1U;      /*!< Reserved. */
} xrdc_non_processor_domain_assignment_t;

/*!
 * @brief XRDC PID LK2 definition XRDC_PIDn[LK2]
 */
typedef enum _xrdc_pid_lock
{
    kXRDC_PidLockSecurePrivilegeWritable = 0U,  /*!< Writable by any secure privileged write. */
    kXRDC_PidLockSecurePrivilegeWritable1 = 1U, /*!< Writable by any secure privileged write. */
    kXRDC_PidLockMasterXOnly = 2U,              /*!< PIDx is only writable by master x.       */
    kXRDC_PidLockLocked = 3U                    /*!< Read-only until the next reset.          */
} xrdc_pid_lock_t;

/*!
 * @brief XRDC process identifier (PID) configuration.
 */
typedef struct _xrdc_pid_config
{
    uint32_t pid : 6U; /*!< PID value, PIDn[PID].           */
#if (defined(FSL_FEATURE_XRDC_HAS_PID_SP4SM) && FSL_FEATURE_XRDC_HAS_PID_SP4SM)
    uint32_t : 21U;            /*!< Reserved. */
    uint32_t sp4smEnable : 1U; /*!< Enable special 4-state model.       */
#else
    uint32_t : 22U; /*!< Reserved. */
#endif
    uint32_t tsmEnable : 1U; /*!< Enable three-state model.       */
    uint32_t lockMode : 2U;  /*!< PIDn configuration lock mode, see @ref xrdc_pid_lock_t. */
    uint32_t : 1U;           /*!< Reserved. */
} xrdc_pid_config_t;

/*!
 * @brief XRDC domain access control policy.
 */
typedef enum _xrdc_access_policy
{
    /* policy SecurePriv  SecureUser  NonSecurePriv  NonSecureUsr*/
    kXRDC_AccessPolicyNone = 0U,      /*  000      none        none        none            none    */
    kXRDC_AccessPolicySpuR = 1U,      /*  001      r             r         none            none    */
    kXRDC_AccessPolicySpRw = 2U,      /*  010      r,w         none        none            none    */
    kXRDC_AccessPolicySpuRw = 3U,     /*  011      r,w          r,w        none            none    */
    kXRDC_AccessPolicySpuRwNpR = 4U,  /*  100      r,w          r,w         r              none    */
    kXRDC_AccessPolicySpuRwNpuR = 5U, /*  101      r,w          r,w         r                r     */
    kXRDC_AccessPolicySpuRwNpRw = 6U, /*  110      r,w          r,w         r,w            none    */
    kXRDC_AccessPolicyAll = 7U        /*  111      r,w          r,w         r,w             r,w    */
} xrdc_access_policy_t;

/*!
 * @brief Access configuration lock mode, the register field PDAC and MRGD LK2.
 */
typedef enum _xrdc_access_config_lock
{
    kXRDC_AccessConfigLockWritable = 0U,    /*!< Entire PDACn/MRGDn can be written.       */
    kXRDC_AccessConfigLockWritable1 = 1U,   /*!< Entire PDACn/MRGDn can be written.       */
    kXRDC_AccessConfigLockDomainXOnly = 2U, /*!< Domain x only write the DxACP field.     */
    kXRDC_AccessConfigLockLocked = 3U       /*!< PDACn is read-only until the next reset. */
} xrdc_access_config_lock_t;

#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL) || \
    (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
/*!
 * @brief Exclusive access lock mode configuration, the register field PDAC and MRGD EAL.
 */
typedef enum _xrdc_excl_access_lock_config
{
    kXRDC_ExclAccessLockDisabled = 0U,             /*!< Lock disabled.       */
    kXRDC_ExclAccessLockDisabledUntilNextRst = 1U, /*!< Lock disabled until next reset.       */
    kXRDC_ExclAccessLockEnabledStateAvail = 2U,    /*!< Lock enabled, lock state = available.     */
    kXRDC_ExclAccessLockEnabledStateNotAvail = 3U  /*!< Lock enabled, lock state = not available. */
} xrdc_excl_access_lock_config_t;
#endif

/*!
 * @brief XRDC peripheral domain access control configuration.
 */
typedef struct _xrdc_periph_access_config
{
    xrdc_periph_t periph;               /*!< Peripheral name.               */
    xrdc_access_config_lock_t lockMode; /*!< PDACn lock configuration.      */
#if !(defined(FSL_FEATURE_XRDC_NO_PDAC_SE) && FSL_FEATURE_XRDC_NO_PDAC_SE)
    bool enableSema;  /*!< Enable semaphore or not.       */
    uint32_t semaNum; /*!< Semaphore number.              */
#endif
#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
    xrdc_excl_access_lock_config_t exclAccessLockMode; /*!< Exclusive access lock configuration.       */
#endif
    xrdc_access_policy_t policy[FSL_FEATURE_XRDC_DOMAIN_COUNT]; /*!< Access policy for each domain. */
} xrdc_periph_access_config_t;

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SZ) && FSL_FEATURE_XRDC_NO_MRGD_SZ)
/*!
 * @brief XRDC memory size definition.
 */
typedef enum _xrdc_mem_size
{
    kXRDC_MemSizeNone = 0U,  /*!< None size.          */
    kXRDC_MemSize32B = 4U,   /*!< 2^(4+1)    = 32     */
    kXRDC_MemSize64B = 5U,   /*!< 2^(5+1)    = 64     */
    kXRDC_MemSize128B = 6U,  /*!< 2^(6+1)    = 128    */
    kXRDC_MemSize256B = 7U,  /*!< 2^(7+1)    = 256    */
    kXRDC_MemSize512B = 8U,  /*!< 2^(8+1)    = 512    */
    kXRDC_MemSize1K = 9U,    /*!< 2^(9+1)    = 1kB    */
    kXRDC_MemSize2K = 10U,   /*!< 2^(10+1)   = 2kB    */
    kXRDC_MemSize4K = 11U,   /*!< 2^(11+1)   = 4kB    */
    kXRDC_MemSize8K = 12U,   /*!< 2^(12+1)   = 8kB    */
    kXRDC_MemSize16K = 13U,  /*!< 2^(13+1)   = 16kB   */
    kXRDC_MemSize32K = 14U,  /*!< 2^(14+1)   = 32kB   */
    kXRDC_MemSize64K = 15U,  /*!< 2^(15+1)   = 64kB   */
    kXRDC_MemSize128K = 16U, /*!< 2^(16+1)   = 128kB  */
    kXRDC_MemSize256K = 17U, /*!< 2^(17+1)   = 256kB  */
    kXRDC_MemSize512K = 18U, /*!< 2^(18+1)   = 512kB  */
    kXRDC_MemSize1M = 19U,   /*!< 2^(19+1)   = 1MB    */
    kXRDC_MemSize2M = 20U,   /*!< 2^(20+1)   = 2MB    */
    kXRDC_MemSize4M = 21U,   /*!< 2^(21+1)   = 4MB    */
    kXRDC_MemSize8M = 22U,   /*!< 2^(22+1)   = 8MB    */
    kXRDC_MemSize16M = 23U,  /*!< 2^(23+1)   = 16MB   */
    kXRDC_MemSize32M = 24U,  /*!< 2^(24+1)   = 32MB   */
    kXRDC_MemSize64M = 25U,  /*!< 2^(25+1)   = 64MB   */
    kXRDC_MemSize128M = 26U, /*!< 2^(26+1)   = 128MB  */
    kXRDC_MemSize256M = 27U, /*!< 2^(27+1)   = 256MB  */
    kXRDC_MemSize512M = 28U, /*!< 2^(28+1)   = 512MB  */
    kXRDC_MemSize1G = 29U,   /*!< 2^(29+1)   = 1GB    */
    kXRDC_MemSize2G = 30U,   /*!< 2^(30+1)   = 2GB    */
    kXRDC_MemSize4G = 31U    /*!< 2^(31+1)   = 4GB    */
} xrdc_mem_size_t;
#endif /* FSL_FEATURE_XRDC_NO_MRGD_SZ */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
/*!
 * @brief XRDC memory ACCSET (SET of programmable access flags).
 */
typedef enum _xrdc_mem_accset
{
    kXRDC_MemAccset1 = 0U, /*!< Memory region SET 1 of programmable access flags.     */
    kXRDC_MemAccset2 = 1U, /*!< Memory region SET 2 of programmable access flags.     */
} xrdc_mem_accset_t;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ACCSET */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_CR) && FSL_FEATURE_XRDC_HAS_MRGD_CR)
/*!
 * @brief XRDC memory code region indicator.
 */
typedef enum _xrdc_mem_code_region
{
    kXRDC_MemCodeRegion0 = 0U, /*!< Code region indicator 0=data.     */
    kXRDC_MemCodeRegion1 = 1U, /*!< Code region indicator 1=code.     */
} xrdc_mem_code_region_t;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_CR */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_DXSEL) && FSL_FEATURE_XRDC_HAS_MRGD_DXSEL)
/*!
 * @brief XRDC domain access flags/policy select.
 *
 * Policy:
 * {R,W,X} Read, write, execute flags.
 * flag = 0 : inhibits access, flag = 1 : allows access.
 * policy => SecurePriv_NonSecurePriv_SecureUser_NonSecureUsr
 * xxx_xxx_xxx_xxx => PS{R,W,X}_PN{R,W,X}_US{R,W,X}_UN{R,W,X}
 *
 *           PS > PN > US > UN          PS > PN > US > UN
 * DxSEL       CodeRegion = 0            CodeRegion = 1
 * 000    000_000_000_000 = 0x000     000_000_000_000 = 0x000
 * 001                         ACCSET1
 * 010                         ACCSET2
 * 011    110_000_000_000 = 0xC00     001_001_001_001 = 0x249
 * 100    110_110_000_000 = 0xD80     111_000_000_000 = 0xE00
 * 101    110_110_100_100 = 0xDA4     110_111_000_000 = 0xDC0
 * 110    110_110_110_000 = 0xDB0     110_110_111_000 = 0xDB8
 * 111    110_110_110_110 = 0xDB6     110_110_111_111 = 0xDBF
 */
typedef enum _xrdc_access_flags_select
{
    kXRDC_AccessFlagsNone = 0U, /* Select predefined constant {r,w,x} flags. See the table above. */
    kXRDC_AccessFlagsAlt1 = 1U, /* Select ACCSET1 4*{r,w,x} fully‐programmable access flags. */
    kXRDC_AccessFlagsAlt2 = 2U, /* Select ACCSET2 4*{r,w,x} fully‐programmable access flags. */
    kXRDC_AccessFlagsAlt3 = 3U, /* Select predefined constant {r,w,x} flags. See the table above. */
    kXRDC_AccessFlagsAlt4 = 4U, /* Select predefined constant {r,w,x} flags. See the table above. */
    kXRDC_AccessFlagsAlt5 = 5U, /* Select predefined constant {r,w,x} flags. See the table above. */
    kXRDC_AccessFlagsAlt6 = 6U, /* Select predefined constant {r,w,x} flags. See the table above. */
    kXRDC_AccessFlagsAlt7 = 7U  /* Select predefined constant {r,w,x} flags. See the table above. */
} xrdc_access_flags_select_t;
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_DXSEL */

/*!
 * @brief XRDC memory region domain access control configuration.
 */
typedef struct _xrdc_mem_access_config
{
    xrdc_mem_t mem; /*!< Memory region descriptor name. */
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SE) && FSL_FEATURE_XRDC_NO_MRGD_SE)
    bool enableSema; /*!< Enable semaphore or not.       */
    uint8_t semaNum; /*!< Semaphore number.              */
#endif
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SZ) && FSL_FEATURE_XRDC_NO_MRGD_SZ)
    xrdc_mem_size_t size; /*!< Memory region size.            */
#endif
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_SRD) && FSL_FEATURE_XRDC_NO_MRGD_SRD)
    uint8_t subRegionDisableMask; /*!< Sub-region disable mask.       */
#endif

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
    bool enableAccset1Lock; /*!< Enable ACCSET1 access lock or not.       */
    bool enableAccset2Lock; /*!< Enable ACCSET2 access lock or not.       */
    uint16_t accset1;       /*!< SET 1 of Programmable access flags.
                              xxx_xxx_xxx_xxx => PS{R,W,X}_PN{R,W,X}_US{R,W,X}_UN{R,W,X}.
                              flag = 0 : inhibits access, flag = 1 : allows access. */
    uint16_t accset2;       /*!< SET 2 of Programmable access flags.
                              xxx_xxx_xxx_xxx => PS{R,W,X}_PN{R,W,X}_US{R,W,X}_UN{R,W,X}.
                              flag = 0 : inhibits access, flag = 1 : allows access. */
#endif
    xrdc_access_config_lock_t lockMode; /*!< MRGDn lock configuration.      */

#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_DXACP) && FSL_FEATURE_XRDC_NO_MRGD_DXACP)
    xrdc_access_policy_t policy[FSL_FEATURE_XRDC_DOMAIN_COUNT]; /*!< Access policy for each domain. */
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_DXSEL) && FSL_FEATURE_XRDC_HAS_MRGD_DXSEL)
    xrdc_access_flags_select_t
        policy[FSL_FEATURE_XRDC_DOMAIN_COUNT]; /*!< Access policy/flags select for each domain. */
#endif

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_CR) && FSL_FEATURE_XRDC_HAS_MRGD_CR)
    xrdc_mem_code_region_t codeRegion; /*!< Code region select. @ref xrdc_mem_code_region_t.  */
#endif

    uint32_t baseAddress; /*!< Memory region base/start address.    */
#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR) && FSL_FEATURE_XRDC_HAS_MRGD_ENDADDR)
    uint32_t endAddress; /*!< Memory region end address. The 5 LSB of end address
                              is ignored and forced to 0x1F by hardware. */
#endif
#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
    xrdc_excl_access_lock_config_t exclAccessLockMode; /*!< Exclusive access lock configuration.       */
#endif
} xrdc_mem_access_config_t;

/*!
 * @brief XRDC controller definition for domain error check.
 */
typedef enum _xrdc_controller
{
    kXRDC_MemController0 = 0U,     /*!< Memory region controller 0.     */
    kXRDC_MemController1 = 1U,     /*!< Memory region controller 1.     */
    kXRDC_MemController2 = 2U,     /*!< Memory region controller 2.     */
    kXRDC_MemController3 = 3U,     /*!< Memory region controller 3.     */
    kXRDC_MemController4 = 4U,     /*!< Memory region controller 4.     */
    kXRDC_MemController5 = 5U,     /*!< Memory region controller 5.     */
    kXRDC_MemController6 = 6U,     /*!< Memory region controller 6.     */
    kXRDC_MemController7 = 7U,     /*!< Memory region controller 7.     */
    kXRDC_MemController8 = 8U,     /*!< Memory region controller 8.     */
    kXRDC_MemController9 = 9U,     /*!< Memory region controller 9.     */
    kXRDC_MemController10 = 10U,   /*!< Memory region controller 10.    */
    kXRDC_MemController11 = 11U,   /*!< Memory region controller 11.    */
    kXRDC_MemController12 = 12U,   /*!< Memory region controller 12.    */
    kXRDC_MemController13 = 13U,   /*!< Memory region controller 13.    */
    kXRDC_MemController14 = 14U,   /*!< Memory region controller 14.    */
    kXRDC_MemController15 = 15U,   /*!< Memory region controller 15.    */
    kXRDC_PeriphController0 = 16U, /*!< Peripheral access controller 0. */
    kXRDC_PeriphController1 = 17U, /*!< Peripheral access controller 1. */
    kXRDC_PeriphController2 = 18U, /*!< Peripheral access controller 2. */
    kXRDC_PeriphController3 = 19U, /*!< Peripheral access controller 3. */
} xrdc_controller_t;

/*!
 * @brief XRDC domain error state definition XRDC_DERR_W1_n[EST].
 */
typedef enum _xrdc_error_state
{
    kXRDC_ErrorStateNone = 0x00U,   /*!< No access violation detected.       */
    kXRDC_ErrorStateNone1 = 0x01U,  /*!< No access violation detected.       */
    kXRDC_ErrorStateSingle = 0x02U, /*!< Single access violation detected.   */
    kXRDC_ErrorStateMulti = 0x03U   /*!< Multiple access violation detected. */
} xrdc_error_state_t;

/*!
 * @brief XRDC domain error attribute definition XRDC_DERR_W1_n[EATR].
 */
typedef enum _xrdc_error_attr
{
    kXRDC_ErrorSecureUserInst = 0x00U,         /*!< Secure user mode, instruction fetch access.           */
    kXRDC_ErrorSecureUserData = 0x01U,         /*!< Secure user mode, data access.                        */
    kXRDC_ErrorSecurePrivilegeInst = 0x02U,    /*!< Secure privileged mode, instruction fetch access.     */
    kXRDC_ErrorSecurePrivilegeData = 0x03U,    /*!< Secure privileged mode, data access.                  */
    kXRDC_ErrorNonSecureUserInst = 0x04U,      /*!< NonSecure user mode, instruction fetch access.        */
    kXRDC_ErrorNonSecureUserData = 0x05U,      /*!< NonSecure user mode, data access.                     */
    kXRDC_ErrorNonSecurePrivilegeInst = 0x06U, /*!< NonSecure privileged mode, instruction fetch access.  */
    kXRDC_ErrorNonSecurePrivilegeData = 0x07U  /*!< NonSecure privileged mode, data access.               */
} xrdc_error_attr_t;

/*!
 * @brief XRDC domain error access type definition XRDC_DERR_W1_n[ERW].
 */
typedef enum _xrdc_error_type
{
    kXRDC_ErrorTypeRead = 0x00U, /*!< Error occurs on read reference.  */
    kXRDC_ErrorTypeWrite = 0x01U /*!< Error occurs on write reference. */
} xrdc_error_type_t;

/*!
 * @brief XRDC domain error definition.
 */
typedef struct _xrdc_error
{
    xrdc_controller_t controller;  /*!< Which controller captured access violation.     */
    uint32_t address;              /*!< Access address that generated access violation. */
    xrdc_error_state_t errorState; /*!< Error state.                                    */
    xrdc_error_attr_t errorAttr;   /*!< Error attribute.                                */
    xrdc_error_type_t errorType;   /*!< Error type.                                     */
    uint8_t errorPort;             /*!< Error port.                                     */
    uint8_t domainId;              /*!< Domain ID.                                      */
} xrdc_error_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Initializes the XRDC module.
 *
 * This function enables the XRDC clock.
 *
 * @param base XRDC peripheral base address.
 */
void XRDC_Init(XRDC_Type *base);

/*!
 * @brief De-initializes the XRDC module.
 *
 * This function disables the XRDC clock.
 *
 * @param base XRDC peripheral base address.
 */
void XRDC_Deinit(XRDC_Type *base);

/*!
 * @name XRDC manager (XRDC_MGR)
 * @{
 */

/*!
 * @brief Gets the XRDC hardware configuration.
 *
 * This function gets the XRDC hardware configurations, including number of bus
 * masters, number of domains, number of MRCs and number of PACs.
 *
 * @param base XRDC peripheral base address.
 * @param config Pointer to the structure to get the configuration.
 */
void XRDC_GetHardwareConfig(XRDC_Type *base, xrdc_hardware_config_t *config);

/*!
 * @brief Locks the XRDC global control register XRDC_CR.
 *
 * This function locks the XRDC_CR register. After it is locked, the register is
 * read-only until the next reset.
 *
 * @param base XRDC peripheral base address.
 */
static inline void XRDC_LockGlobalControl(XRDC_Type *base)
{
    base->CR |= XRDC_CR_LK1_MASK;
}

/*!
 * @brief Sets the XRDC global valid.
 *
 * This function sets the XRDC global valid or invalid. When the XRDC is global
 * invalid, all accesses from all bus masters to all slaves are allowed.
 *
 * @param base XRDC peripheral base address.
 * @param valid True to valid XRDC.
 */
static inline void XRDC_SetGlobalValid(XRDC_Type *base, bool valid)
{
    if (valid)
    {
#if (defined(FSL_FEATURE_XRDC_HAS_NO_CR_GVLD) && FSL_FEATURE_XRDC_HAS_NO_CR_GVLD)
        base->CR |= (XRDC_CR_GVLDM_MASK | XRDC_CR_GVLDC_MASK | XRDC_CR_GVLDP_MASK);
#else
        base->CR |= XRDC_CR_GVLD_MASK;
#endif
    }
    else
    {
#if (defined(FSL_FEATURE_XRDC_HAS_NO_CR_GVLD) && FSL_FEATURE_XRDC_HAS_NO_CR_GVLD)
        base->CR &= ~(XRDC_CR_GVLDM_MASK | XRDC_CR_GVLDC_MASK | XRDC_CR_GVLDP_MASK);
#else
        base->CR &= ~XRDC_CR_GVLD_MASK;
#endif
    }
}

/*!
 * @brief Gets the domain ID of the current bus master.
 *
 * This function returns the domain ID of the current bus master.
 *
 * @param base XRDC peripheral base address.
 * @return Domain ID of current bus master.
 */
static inline uint8_t XRDC_GetCurrentMasterDomainId(XRDC_Type *base)
{
    return (uint8_t)((base->HWCFG1 & XRDC_HWCFG1_DID_MASK) >> XRDC_HWCFG1_DID_SHIFT);
}

/*!
 * @brief Gets and clears the first domain error of the current domain.
 *
 * This function gets the first access violation information for the current domain
 * and clears the pending flag. There might be multiple access violations pending
 * for the current domain. This function only processes the first error.
 *
 * @param base XRDC peripheral base address.
 * @param error Pointer to the error information.
 * @return If the access violation is captured, this function returns the kStatus_Success.
 *         The error information can be obtained from the parameter error. If no
 *         access violation is captured, this function returns the kStatus_XRDC_NoError.
 */
status_t XRDC_GetAndClearFirstDomainError(XRDC_Type *base, xrdc_error_t *error);

/*!
 * @brief Gets and clears the first domain error of the specific domain.
 *
 * This function gets the first access violation information for the specific domain
 * and clears the pending flag. There might be multiple access violations pending
 * for the current domain. This function only processes the first error.
 *
 * @param base XRDC peripheral base address.
 * @param error Pointer to the error information.
 * @param domainId The error of which domain to get and clear.
 * @return If the access violation is captured, this function returns the kStatus_Success.
 *         The error information can be obtained from the parameter error. If no
 *         access violation is captured, this function returns the kStatus_XRDC_NoError.
 */
status_t XRDC_GetAndClearFirstSpecificDomainError(XRDC_Type *base, xrdc_error_t *error, uint8_t domainId);

/*@}*/

/*!
 * @name XRDC Master Domain Assignment Controller (XRDC_MDAC).
 * @{
 */

/*!
 * @brief Gets the default PID configuration structure.
 *
 * This function initializes the configuration structure to default values. The default
 * values are:
 *
 * @code
 * config->pid       = 0U;
 * config->tsmEnable = 0U;
 * config->sp4smEnable = 0U;
 * config->lockMode  = kXRDC_PidLockSecurePrivilegeWritable;
 * @endcode
 *
 * @param config Pointer to the configuration structure.
 */
static inline void XRDC_GetPidDefaultConfig(xrdc_pid_config_t *config)
{
    assert(config);

    (*((uint32_t *)config)) = 0U;
}

/*!
 * @brief Configures the PID for a specific bus master.
 *
 * This function configures the PID for a specific bus master. Do not use this
 * function for non-processor bus masters.
 *
 * @param base XRDC peripheral base address.
 * @param master Which bus master to configure.
 * @param config Pointer to the configuration structure.
 */
static inline void XRDC_SetPidConfig(XRDC_Type *base, xrdc_master_t master, const xrdc_pid_config_t *config)
{
    assert(config);

    base->PID[master] = *((const uint32_t *)config);
}

/*!
 * @brief Sets the PID configuration register lock mode.
 *
 * This function sets the PID configuration register lock XRDC_PIDn[LK2].
 *
 * @param base XRDC peripheral base address.
 * @param master Which master's PID to lock.
 * @param lockMode Lock mode to set.
 */
static inline void XRDC_SetPidLockMode(XRDC_Type *base, xrdc_master_t master, xrdc_pid_lock_t lockMode)
{
    uint32_t reg = base->PID[master];

    reg = ((reg & ~XRDC_PID_LK2_MASK) | XRDC_PID_LK2(lockMode));

    base->PID[master] = reg;
}

/*!
 * @brief Gets the default master domain assignment for non-processor bus master.
 *
 * This function gets the default master domain assignment for non-processor bus master.
 * It should only be used for the no-processor bus masters, such as DMA. This function
 * sets the assignment as follows:
 *
 * @code
 * assignment->domainId            = 0U;
 * assignment->privilegeAttr       = kXRDC_ForceUser;
 * assignment->privilegeAttr       = kXRDC_ForceSecure;
 * assignment->bypassDomainId      = 0U;
 * assignment->blogicPartId        = 0U;
 * assignment->benableLogicPartId  = 0U;
 * assignment->lock                = 0U;
 * @endcode
 *
 * @param assignment Pointer to the assignment structure.
 */
static inline void XRDC_GetDefaultNonProcessorDomainAssignment(xrdc_non_processor_domain_assignment_t *assignment)
{
    assert(assignment);

    *(uint32_t *)assignment = 0U;
}

/*!
 * @brief Gets the default master domain assignment for the processor bus master.
 *
 * This function gets the default master domain assignment for the processor bus master.
 * It should only be used for the processor bus masters, such as CORE0. This function
 * sets the assignment as follows:
 *
 * @code
 * assignment->domainId           = 0U;
 * assignment->domainIdSelect     = kXRDC_DidMda;
 * assignment->dpidEnable         = kXRDC_PidDisable;
 * assignment->pidMask            = 0U;
 * assignment->pid                = 0U;
 * assignment->logicPartId        = 0U;
 * assignment->enableLogicPartId  = 0U;
 * assignment->lock               = 0U;
 * @endcode
 *
 * @param assignment Pointer to the assignment structure.
 */
static inline void XRDC_GetDefaultProcessorDomainAssignment(xrdc_processor_domain_assignment_t *assignment)
{
    assert(assignment);

    *(uint32_t *)assignment = 0U;
}

/*!
 * @brief Sets the non-processor bus master domain assignment.
 *
 * This function sets the non-processor master domain assignment as valid.
 * One bus master might have multiple domain assignment registers. The parameter
 * \p assignIndex specifies which assignment register to set.
 *
 * Example: Set domain assignment for DMA0.
 * @code
 * xrdc_non_processor_domain_assignment_t nonProcessorAssignment;
 *
 * XRDC_GetDefaultNonProcessorDomainAssignment(&nonProcessorAssignment); // Get default assignment
 * // Modify necessary members.
 * nonProcessorAssignment.domainId = 1;
 * nonProcessorAssignment.xxx      = xxx; // Other modifications.
 *
 * // Set the domain assignment. Only 1 assignment register for DMA0. Pass in 0U as assignIndex;
 * XRDC_SetMasterDomainAssignment(XRDC, kXrdcMasterDma0, 0U, &nonProcessorAssignment);
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param master Which master to configure.
 * @param assignIndex Which assignment register to set.
 * @param assignment Pointer to the assignment structure.
 */
static inline void XRDC_SetNonProcessorDomainAssignment(XRDC_Type *base,
                                                        xrdc_master_t master,
                                                        uint8_t assignIndex,
                                                        const xrdc_non_processor_domain_assignment_t *assignment)
{
    /* Make sure the master is a non-CPU/non-processor master */
    assert(base->MDACFG[master] & XRDC_MDACFG_NCM_MASK);
    /* Make sure the master has the assignment register. */
    assert(assignIndex < ((base->MDACFG[master] & XRDC_MDACFG_NMDAR_MASK) >> XRDC_MDACFG_NMDAR_SHIFT));
    assert(assignment);

    base->MDA[master].MDA_W[assignIndex] = ((*(const uint32_t *)assignment) | XRDC_MDA_W_VLD_MASK);
}

/*!
 * @brief Sets the processor bus master domain assignment.
 *
 * This function sets the processor master domain assignment as valid.
 * One bus master might have multiple domain assignment registers. The parameter
 * \p assignIndex specifies which assignment register to set.
 *
 * Example: Set domain assignment for core 0.
 * @code
 * xrdc_processor_domain_assignment_t processorAssignment;
 *
 * XRDC_GetDefaultProcessorDomainAssignment(&processorAssignment); // Get default assignment
 *
 * // Set the domain assignment. There are 3 assignment registers for core 0.
 * // Set assignment register 0.
 * processorAssignment.domainId = 1;
 * processorAssignment.xxx      = xxx; // Other modifications.
 * XRDC_SetMasterDomainAssignment(XRDC, kXrdcMasterCpu0, 0U, &processorAssignment);
 *
 * // Set assignment register 1.
 * processorAssignment.domainId = 2;
 * processorAssignment.xxx      = xxx; // Other modifications.
 * XRDC_SetMasterDomainAssignment(XRDC, kXrdcMasterCpu0, 1U, &processorAssignment);
 *
 * // Set assignment register 2.
 * processorAssignment.domainId = 0;
 * processorAssignment.xxx      = xxx; // Other modifications.
 * XRDC_SetMasterDomainAssignment(XRDC, kXrdcMasterCpu0, 2U, &processorAssignment);
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param master Which master to configure.
 * @param assignIndex Which assignment register to set.
 * @param assignment Pointer to the assignment structure.
 */
static inline void XRDC_SetProcessorDomainAssignment(XRDC_Type *base,
                                                     xrdc_master_t master,
                                                     uint8_t assignIndex,
                                                     const xrdc_processor_domain_assignment_t *assignment)
{
    /* Make sure the master is a CPU/processor master */
    assert(!(base->MDACFG[master] & XRDC_MDACFG_NCM_MASK));
    /* Make sure the master has the assignment register. */
    assert(assignIndex < ((base->MDACFG[master] & XRDC_MDACFG_NMDAR_MASK) >> XRDC_MDACFG_NMDAR_SHIFT));
    assert(assignment);

    base->MDA[master].MDA_W[assignIndex] = ((*(const uint32_t *)assignment) | XRDC_MDA_W_VLD_MASK);
}

/*!
 * @brief Locks the bus master domain assignment register.
 *
 * This function locks the master domain assignment. One bus master might have
 * multiple domain assignment registers. The parameter \p assignIndex specifies
 * which assignment register to lock. After it is locked, the register can't be changed
 * until next reset.
 *
 * @param base XRDC peripheral base address.
 * @param master Which master to configure.
 * @param assignIndex Which assignment register to lock.
 */
static inline void XRDC_LockMasterDomainAssignment(XRDC_Type *base, xrdc_master_t master, uint8_t assignIndex)
{
    /* Make sure the master has the assignment register. */
    assert(assignIndex < ((base->MDACFG[master] & XRDC_MDACFG_NMDAR_MASK) >> XRDC_MDACFG_NMDAR_SHIFT));

    base->MDA[master].MDA_W[assignIndex] |= XRDC_MDA_W_LK1_MASK;
}

/*!
 * @brief Sets the master domain assignment as valid or invalid.
 *
 * This function sets the master domain assignment as valid or invalid. One bus master might have
 * multiple domain assignment registers. The parameter \p assignIndex specifies
 * which assignment register to configure.
 *
 * @param base XRDC peripheral base address.
 * @param master Which master to configure.
 * @param assignIndex Index for the domain assignment register.
 * @param valid True to set valid, false to set invalid.
 */
static inline void XRDC_SetMasterDomainAssignmentValid(XRDC_Type *base,
                                                       xrdc_master_t master,
                                                       uint8_t assignIndex,
                                                       bool valid)
{
    /* Make sure the master has the assignment register. */
    assert(assignIndex < ((base->MDACFG[master] & XRDC_MDACFG_NMDAR_MASK) >> XRDC_MDACFG_NMDAR_SHIFT));

    if (valid)
    {
        base->MDA[master].MDA_W[assignIndex] |= XRDC_MDA_W_VLD_MASK;
    }
    else
    {
        base->MDA[master].MDA_W[assignIndex] &= ~XRDC_MDA_W_VLD_MASK;
    }
}

/*@}*/

/*!
 * @name XRDC Memory Region Controller (XRDC_MRC)
 * @{
 */

/*!
 * @brief Gets the default memory region access policy.
 *
 * This function gets the default memory region access policy.
 * It sets the policy as follows:
 * @code
 * config->enableSema            = false;
 * config->semaNum               = 0U;
 * config->subRegionDisableMask  = 0U;
 * config->size                  = kXrdcMemSizeNone;
 * config->lockMode              = kXRDC_AccessConfigLockWritable;
 * config->baseAddress           = 0U;
 * config->policy[0]             = kXRDC_AccessPolicyNone;
 * config->policy[1]             = kXRDC_AccessPolicyNone;
 * ...
 * config->policy[15]            = kXRDC_AccessPolicyNone;
 * @endcode
 *
 * @param config Pointer to the configuration structure.
 */
void XRDC_GetMemAccessDefaultConfig(xrdc_mem_access_config_t *config);

/*!
 * @brief Sets the memory region access policy.
 *
 * This function sets the memory region access configuration as valid.
 * There are two methods to use it:
 *
 * Example 1: Set one configuration run time.
 * @code
 * // Set memory region 0x20000000 ~ 0x20000400 accessible by domain 0, use MRC0_1.
 * xrdc_mem_access_config_t config =
 * {
 *     .mem         = kXRDC_MemMrc0_1,
 *     .baseAddress = 0x20000000U,
 *     .size        = kXRDC_MemSize1K,
 *     .policy[0]   = kXRDC_AccessPolicyAll
 * };
 * XRDC_SetMemAccessConfig(XRDC, &config);
 * @endcode
 *
 * Example 2: Set multiple configurations during startup.
 * @code
 * // Set memory region 0x20000000 ~ 0x20000400 accessible by domain 0, use MRC0_1.
 * // Set memory region 0x1FFF0000 ~ 0x1FFF0800 accessible by domain 0, use MRC0_2.
 * xrdc_mem_access_config_t configs[] =
 * {
 *     {
 *         .mem         = kXRDC_MemMrc0_1,
 *         .baseAddress = 0x20000000U,
 *         .size        = kXRDC_MemSize1K,
 *         .policy[0]   = kXRDC_AccessPolicyAll
 *     },
 *     {
 *         .mem         = kXRDC_MemMrc0_2,
 *         .baseAddress = 0x1FFF0000U,
 *         .size        = kXRDC_MemSize2K,
 *         .policy[0]   = kXRDC_AccessPolicyAll
 *     }
 * };
 *
 * // Set the configurations.
 * for (i=0U; i<((sizeof(configs)/sizeof(configs[0]))); i++)
 * {
 *     XRDC_SetMemAccessConfig(XRDC, &configs[i]);
 * }
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param config Pointer to the access policy configuration structure.
 */
void XRDC_SetMemAccessConfig(XRDC_Type *base, const xrdc_mem_access_config_t *config);

/*!
 * @brief Sets the memory region descriptor register lock mode.
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region descriptor to lock.
 * @param lockMode The lock mode to set.
 */
static inline void XRDC_SetMemAccessLockMode(XRDC_Type *base, xrdc_mem_t mem, xrdc_access_config_lock_t lockMode)
{
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_W3_LK2) && FSL_FEATURE_XRDC_NO_MRGD_W3_LK2)
    uint32_t reg = base->MRGD[mem].MRGD_W[3];

    reg = ((reg & ~XRDC_MRGD_W_LK2_MASK) | XRDC_MRGD_W_LK2(lockMode));

    base->MRGD[mem].MRGD_W[3] = reg;
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_W4_LK2) && FSL_FEATURE_XRDC_HAS_MRGD_W4_LK2)
    uint32_t reg = base->MRGD[mem].MRGD_W[4];

    reg = ((reg & ~XRDC_MRGD_W_LK2_MASK) | XRDC_MRGD_W_LK2(lockMode));

    base->MRGD[mem].MRGD_W[4] = reg;
#endif
}

/*!
 * @brief Sets the memory region descriptor as valid or invalid.
 *
 * This function sets the memory region access configuration dynamically. For example:
 *
 * @code
 * // Set memory region 0x20000000 ~ 0x20000400 accessible by domain 0, use MRC0_1.
 * xrdc_mem_access_config_t config =
 * {
 *     .mem         = kXRDC_MemMrc0_1,
 *     .baseAddress = 0x20000000U,
 *     .size        = kXRDC_MemSize1K,
 *     .policy[0]   = kXRDC_AccessPolicyAll
 * };
 * XRDC_SetMemAccessConfig(XRDC, &config);
 *
 * // Set the memory access configuration invalid.
 * XRDC_SetMemAccessValid(kXRDC_MemMrc0_1, false);
 *
 * // Set the memory access configuration valid, the region 0x20000000 ~ 0x20000400 accessible by domain 0
 * XRDC_SetMemAccessValid(kXRDC_MemMrc0_1, true);
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region descriptor to set.
 * @param valid True to set valid, false to set invalid.
 */
static inline void XRDC_SetMemAccessValid(XRDC_Type *base, xrdc_mem_t mem, bool valid)
{
#if !(defined(FSL_FEATURE_XRDC_NO_MRGD_W3_VLD) && FSL_FEATURE_XRDC_NO_MRGD_W3_VLD)
    if (valid)
    {
        base->MRGD[mem].MRGD_W[3] |= XRDC_MRGD_W_VLD_MASK;
    }
    else
    {
        base->MRGD[mem].MRGD_W[3] &= ~XRDC_MRGD_W_VLD_MASK;
    }
#elif(defined(FSL_FEATURE_XRDC_HAS_MRGD_W4_VLD) && FSL_FEATURE_XRDC_HAS_MRGD_W4_VLD)
    if (valid)
    {
        base->MRGD[mem].MRGD_W[4] |= XRDC_MRGD_W_VLD_MASK;
    }
    else
    {
        base->MRGD[mem].MRGD_W[4] &= ~XRDC_MRGD_W_VLD_MASK;
    }
#endif
}

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_EAL) && FSL_FEATURE_XRDC_HAS_MRGD_EAL)
/*!
 * @brief Sets the memory region exclusive access lock mode configuration.
 *
 * Note: Any write to MRGD_W[0-3]_n clears the MRGD_W4_n[VLD] indicator so a coherent register state can be supported.
 *       It is indispensable to re-assert the valid bit when dynamically changing the EAL in the MRGD, which is done in
 * this API.
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region's exclusive access lock mode to configure.
 * @param lockMode The exclusive access lock mode to set.
 */
void XRDC_SetMemExclAccessLockMode(XRDC_Type *base, xrdc_mem_t mem, xrdc_excl_access_lock_config_t lockMode);

/*!
 * @brief Forces the release of the memory region exclusive access lock.
 *
 * A lock can be forced to the available state (EAL=10) by a domain that does not own the
 * lock through the forced lock release procedure:
 * The procedure to force a exclusive access lock release is as follows:
 * 1. Write 0x02000046 to W1 register (PAC/MSC) or W3 register (MRC)
 * 2. Write 0x02000052 to W1 register (PAC/MSC) or W3 register (MRC)
 *
 * Note: The two writes must be consecutive, any intervening write to the register resets the sequence.
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region's exclusive access lock to force release.
 */
void XRDC_ForceMemExclAccessLockRelease(XRDC_Type *base, xrdc_mem_t mem);

/*!
 * @brief Gets the exclusive access lock domain owner of the memory region.
 *
 * This function returns the domain ID of the exclusive access lock owner of the memory region.
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region's exclusive access lock domain owner to get.
 * @return Domain ID of the memory region exclusive access lock owner.
 */
static inline uint8_t XRDC_GetMemExclAccessLockDomainOwner(XRDC_Type *base, xrdc_mem_t mem)
{
    return (uint8_t)((base->MRGD[mem].MRGD_W[2U] & XRDC_MRGD_W_EALO_MASK) >> XRDC_MRGD_W_EALO_SHIFT);
}
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_EAL */

#if (defined(FSL_FEATURE_XRDC_HAS_MRGD_ACCSET) && FSL_FEATURE_XRDC_HAS_MRGD_ACCSET)
/*!
 * @brief Sets the memory region ACCSET (programmable access flags) lock.
 *
 * @param base XRDC peripheral base address.
 * @param mem Which memory region descriptor to lock.
 * @param mem Which set/index of ACCSET to lock.
 * @param lock True to set lock, false to set unlock.
 */
void XRDC_SetMemAccsetLock(XRDC_Type *base, xrdc_mem_t mem, xrdc_mem_accset_t accset, bool lock);
#endif /* FSL_FEATURE_XRDC_HAS_MRGD_ACCSET */

/*@}*/

/*!
 * @name XRDC Peripheral Access Controller (XRDC_PAC)
 * @{
 */

/*!
 * @brief Gets the default peripheral access configuration.
 *
 * The default configuration is set as follows:
 * @code
 * config->enableSema        = false;
 * config->semaNum           = 0U;
 * config->lockMode          = kXRDC_AccessConfigLockWritable;
 * config->policy[0]         = kXRDC_AccessPolicyNone;
 * config->policy[1]         = kXRDC_AccessPolicyNone;
 * ...
 * config->policy[15]        = kXRDC_AccessPolicyNone;
 * @endcode
 *
 * @param config Pointer to the configuration structure.
 */
void XRDC_GetPeriphAccessDefaultConfig(xrdc_periph_access_config_t *config);

/*!
 * @brief Sets the peripheral access configuration.
 *
 * This function sets the peripheral access configuration as valid. Two
 * methods to use it:
 * Method 1: Set for one peripheral, which is used for runtime settings.
 * @code
 * xrdc_periph_access_config_t config;
 *
 * // Set LPTMR0 accessible by domain 0
 * config.periph    = kXRDC_PeriphLptmr0;
 * config.policy[0] = kXRDC_AccessPolicyAll;
 * XRDC_SetPeriphAccessConfig(XRDC, &config);
 * @endcode
 *
 * Method 2: Set for multiple peripherals, which is used for initialization settings.
 * @code
 * // Prepare the configurations
 * xrdc_periph_access_config_t configs[] =
 * {
 *     {
 *         .periph    = kXRDC_PeriphLptmr0,
 *         .policy[0] = kXRDC_AccessPolicyAll,
 *         .policy[1] = kXRDC_AccessPolicyAll
 *     },
 *     {
 *         .periph    = kXRDC_PeriphLpuart0,
 *         .policy[0] = kXRDC_AccessPolicyAll,
 *         .policy[1] = kXRDC_AccessPolicyAll
 *     }
 * };
 *
 * // Set the configurations.
 * for (i=0U; i<(sizeof(configs)/sizeof(configs[0])), i++)
 * {
 *     XRDC_SetPeriphAccessConfig(XRDC, &config[i]);
 * }
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param config Pointer to the configuration structure.
 */
void XRDC_SetPeriphAccessConfig(XRDC_Type *base, const xrdc_periph_access_config_t *config);

/*!
 * @brief Sets the peripheral access configuration register lock mode.
 *
 * @param base XRDC peripheral base address.
 * @param periph Which peripheral access configuration register to lock.
 * @param lockMode The lock mode to set.
 */
static inline void XRDC_SetPeriphAccessLockMode(XRDC_Type *base,
                                                xrdc_periph_t periph,
                                                xrdc_access_config_lock_t lockMode)
{
    uint32_t reg = base->PDAC_W[periph][1];

#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
    /* If there is PDAC_W1[EAL], should not write non-zero value to EAL. */
    reg = ((reg & ~(XRDC_PDAC_W_LK2_MASK | XRDC_PDAC_W_EAL_MASK)) | XRDC_PDAC_W_LK2(lockMode));
#else
    reg = ((reg & ~XRDC_PDAC_W_LK2_MASK) | XRDC_PDAC_W_LK2(lockMode));
#endif

    base->PDAC_W[periph][1] = reg;
}

/*!
 * @brief Sets the peripheral access as valid or invalid.
 *
 * This function sets the peripheral access configuration dynamically. For example:
 *
 * @code
 * // Set LPTMR0 accessible by domain 0.
 * xrdc_periph_access_config_t config =
 * {
 *     .periph    = kXRDC_PeriphLptmr0;
 *     .policy[0] = kXRDC_AccessPolicyAll;
 * };
 * XRDC_SetPeriphAccessConfig(XRDC, &config);
 *
 * // Set the LPTMR0 access configuration invalid.
 * XRDC_SetPeriphAccessValid(kXrdcPeriLptmr0, false);
 *
 * // Set the LPTMR0 access configuration valid, the LPTMR0 accessible by domain 0.
 * XRDC_SetPeriphAccessValid(kXrdcPeriLptmr0, true);
 * @endcode
 *
 * @param base XRDC peripheral base address.
 * @param periph Which peripheral access configuration to set.
 * @param valid True to set valid, false to set invalid.
 */
static inline void XRDC_SetPeriphAccessValid(XRDC_Type *base, xrdc_periph_t periph, bool valid)
{
#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
    /* If there is PDAC_W1[EAL], should not write non-zero value to EAL. */
    uint32_t reg = base->PDAC_W[periph][1] & ~XRDC_PDAC_W_EAL_MASK;

    if (valid)
    {
        base->PDAC_W[periph][1] = reg | XRDC_PDAC_W_VLD_MASK;
    }
    else
    {
        base->PDAC_W[periph][1] = reg & ~XRDC_PDAC_W_VLD_MASK;
    }
#else
    if (valid)
    {
        base->PDAC_W[periph][1] |= XRDC_PDAC_W_VLD_MASK;
    }
    else
    {
        base->PDAC_W[periph][1] &= ~XRDC_PDAC_W_VLD_MASK;
    }
#endif
}

#if (defined(FSL_FEATURE_XRDC_HAS_PDAC_EAL) && FSL_FEATURE_XRDC_HAS_PDAC_EAL)
/*!
 * @brief Sets the peripheral exclusive access lock mode configuration.
 *
 * @param base XRDC peripheral base address.
 * @param periph Which peripheral's exclusive access lock mode to configure.
 * @param lockMode The exclusive access lock mode to set.
 */
static inline void XRDC_SetPeriphExclAccessLockMode(XRDC_Type *base,
                                                    xrdc_periph_t periph,
                                                    xrdc_excl_access_lock_config_t lockMode)
{
    /* Write kXRDC_ExclAccessLockDisabled is not allowed. */
    assert(kXRDC_ExclAccessLockDisabled != lockMode);

    uint32_t reg = base->PDAC_W[periph][1];

    reg = ((reg & ~XRDC_PDAC_W_EAL_MASK) | XRDC_PDAC_W_EAL(lockMode));

    base->PDAC_W[periph][1] = reg;
}

/*!
 * @brief Forces the release of the peripheral exclusive access lock.
 *
 * A lock can be forced to the available state (EAL=10) by a domain that does not own the
 * lock through the forced lock release procedure:
 * The procedure to force a exclusive access lock release is as follows:
 * 1. Write 0x02000046 to W1 register (PAC/MSC) or W3 register (MRC)
 * 2. Write 0x02000052 to W1 register (PAC/MSC) or W3 register (MRC)
 *
 * Note: The two writes must be consecutive, any intervening write to the register resets the sequence.
 *
 * @param base XRDC peripheral base address.
 * @param periph Which peripheral's exclusive access lock to force release.
 */
void XRDC_ForcePeriphExclAccessLockRelease(XRDC_Type *base, xrdc_periph_t periph);

/*!
 * @brief Gets the exclusive access lock domain owner of the peripheral.
 *
 * This function returns the domain ID of the exclusive access lock owner of the peripheral.
 *
 * @param base XRDC peripheral base address.
 * @param periph Which peripheral's exclusive access lock domain owner to get.
 * @return Domain ID of the peripheral exclusive access lock owner.
 */
static inline uint8_t XRDC_GetPeriphExclAccessLockDomainOwner(XRDC_Type *base, xrdc_periph_t periph)
{
    return (uint8_t)((base->PDAC_W[periph][0U] & XRDC_PDAC_W_EALO_MASK) >> XRDC_PDAC_W_EALO_SHIFT);
}
#endif /* FSL_FEATURE_XRDC_HAS_PDAC_EAL */
/*@}*/

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* _FSL_XRDC_H_ */
