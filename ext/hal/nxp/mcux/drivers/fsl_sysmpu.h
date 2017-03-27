/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_SYSMPU_H_
#define _FSL_SYSMPU_H_

#include "fsl_common.h"

/*!
 * @addtogroup sysmpu
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief SYSMPU driver version 2.2.0. */
#define FSL_SYSMPU_DRIVER_VERSION (MAKE_VERSION(2, 2, 0))
/*@}*/

/*! @brief define the start master port with read and write attributes. */
#define SYSMPU_MASTER_RWATTRIBUTE_START_PORT (4)

/*! @brief SYSMPU the bit shift for masters with privilege rights: read write and execute. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_SHIFT(n) (n * 6)

/*! @brief SYSMPU masters with read, write and execute rights bit mask. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_MASK(n) (0x1Fu << SYSMPU_REGION_RWXRIGHTS_MASTER_SHIFT(n))

/*! @brief SYSMPU masters with read, write and execute rights bit width. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_WIDTH 5

/*! @brief SYSMPU masters with read, write and execute rights priority setting. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER(n, x) \
    (((uint32_t)(((uint32_t)(x)) << SYSMPU_REGION_RWXRIGHTS_MASTER_SHIFT(n))) & SYSMPU_REGION_RWXRIGHTS_MASTER_MASK(n))

/*! @brief SYSMPU masters with read, write and execute rights process enable bit shift. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n) (n * 6 + SYSMPU_REGION_RWXRIGHTS_MASTER_WIDTH)

/*! @brief SYSMPU masters with read, write and execute rights process enable bit mask. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_PE_MASK(n) (0x1u << SYSMPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n))

/*! @brief SYSMPU masters with read, write and execute rights process enable setting. */
#define SYSMPU_REGION_RWXRIGHTS_MASTER_PE(n, x) \
    (((uint32_t)(((uint32_t)(x)) << SYSMPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n))) & SYSMPU_REGION_RWXRIGHTS_MASTER_PE_MASK(n))

/*! @brief SYSMPU masters with normal read write permission bit shift. */
#define SYSMPU_REGION_RWRIGHTS_MASTER_SHIFT(n) ((n - SYSMPU_MASTER_RWATTRIBUTE_START_PORT) * 2 + 24)

/*! @brief SYSMPU masters with normal read write rights bit mask. */
#define SYSMPU_REGION_RWRIGHTS_MASTER_MASK(n) (0x3u << SYSMPU_REGION_RWRIGHTS_MASTER_SHIFT(n))

/*! @brief SYSMPU masters with normal read write rights priority setting. */
#define SYSMPU_REGION_RWRIGHTS_MASTER(n, x) \
    (((uint32_t)(((uint32_t)(x)) << SYSMPU_REGION_RWRIGHTS_MASTER_SHIFT(n))) & SYSMPU_REGION_RWRIGHTS_MASTER_MASK(n))


/*! @brief Describes the number of SYSMPU regions. */
typedef enum _sysmpu_region_total_num
{
    kSYSMPU_8Regions = 0x0U,  /*!< SYSMPU supports 8 regions.  */
    kSYSMPU_12Regions = 0x1U, /*!< SYSMPU supports 12 regions. */
    kSYSMPU_16Regions = 0x2U  /*!< SYSMPU supports 16 regions. */
} sysmpu_region_total_num_t;

/*! @brief SYSMPU slave port number. */
typedef enum _sysmpu_slave
{
    kSYSMPU_Slave0 = 0U, /*!< SYSMPU slave port 0. */
    kSYSMPU_Slave1 = 1U, /*!< SYSMPU slave port 1. */
    kSYSMPU_Slave2 = 2U, /*!< SYSMPU slave port 2. */
    kSYSMPU_Slave3 = 3U, /*!< SYSMPU slave port 3. */
    kSYSMPU_Slave4 = 4U, /*!< SYSMPU slave port 4. */
#if FSL_FEATURE_SYSMPU_SLAVE_COUNT > 5
    kSYSMPU_Slave5 = 5U, /*!< SYSMPU slave port 5. */
#endif
#if FSL_FEATURE_SYSMPU_SLAVE_COUNT > 6
    kSYSMPU_Slave6 = 6U, /*!< SYSMPU slave port 6. */
#endif
#if FSL_FEATURE_SYSMPU_SLAVE_COUNT > 7
    kSYSMPU_Slave7 = 7U, /*!< SYSMPU slave port 7. */
#endif
} sysmpu_slave_t;

/*! @brief SYSMPU error access control detail. */
typedef enum _sysmpu_err_access_control
{
    kSYSMPU_NoRegionHit = 0U,        /*!< No region hit error. */
    kSYSMPU_NoneOverlappRegion = 1U, /*!< Access single region error. */
    kSYSMPU_OverlappRegion = 2U      /*!< Access overlapping region error. */
} sysmpu_err_access_control_t;

/*! @brief SYSMPU error access type. */
typedef enum _sysmpu_err_access_type
{
    kSYSMPU_ErrTypeRead = 0U, /*!< SYSMPU error access type --- read.  */
    kSYSMPU_ErrTypeWrite = 1U /*!< SYSMPU error access type --- write. */
} sysmpu_err_access_type_t;

/*! @brief SYSMPU access error attributes.*/
typedef enum _sysmpu_err_attributes
{
    kSYSMPU_InstructionAccessInUserMode = 0U,       /*!< Access instruction error in user mode. */
    kSYSMPU_DataAccessInUserMode = 1U,              /*!< Access data error in user mode. */
    kSYSMPU_InstructionAccessInSupervisorMode = 2U, /*!< Access instruction error in supervisor mode. */
    kSYSMPU_DataAccessInSupervisorMode = 3U         /*!< Access data error in supervisor mode. */
} sysmpu_err_attributes_t;

/*! @brief SYSMPU access rights in supervisor mode for bus master 0 ~ 3. */
typedef enum _sysmpu_supervisor_access_rights
{
    kSYSMPU_SupervisorReadWriteExecute = 0U, /*!< Read write and execute operations are allowed in supervisor mode. */
    kSYSMPU_SupervisorReadExecute = 1U,      /*!< Read and execute operations are allowed in supervisor mode. */
    kSYSMPU_SupervisorReadWrite = 2U,        /*!< Read write operations are allowed in supervisor mode. */
    kSYSMPU_SupervisorEqualToUsermode = 3U   /*!< Access permission equal to user mode. */
} sysmpu_supervisor_access_rights_t;

/*! @brief SYSMPU access rights in user mode for bus master 0 ~ 3. */
typedef enum _sysmpu_user_access_rights
{
    kSYSMPU_UserNoAccessRights = 0U,  /*!< No access allowed in user mode.  */
    kSYSMPU_UserExecute = 1U,         /*!< Execute operation is allowed in user mode. */
    kSYSMPU_UserWrite = 2U,           /*!< Write operation is allowed in user mode. */
    kSYSMPU_UserWriteExecute = 3U,    /*!< Write and execute operations are allowed in user mode. */
    kSYSMPU_UserRead = 4U,            /*!< Read is allowed in user mode. */
    kSYSMPU_UserReadExecute = 5U,     /*!< Read and execute operations are allowed in user mode. */
    kSYSMPU_UserReadWrite = 6U,       /*!< Read and write operations are allowed in user mode. */
    kSYSMPU_UserReadWriteExecute = 7U /*!< Read write and execute operations are allowed in user mode. */
} sysmpu_user_access_rights_t;

/*! @brief SYSMPU hardware basic information. */
typedef struct _sysmpu_hardware_info
{
    uint8_t hardwareRevisionLevel;         /*!< Specifies the SYSMPU's hardware and definition reversion level. */
    uint8_t slavePortsNumbers;             /*!< Specifies the number of slave ports connected to SYSMPU. */
    sysmpu_region_total_num_t regionsNumbers; /*!< Indicates the number of region descriptors implemented. */
} sysmpu_hardware_info_t;

/*! @brief SYSMPU detail error access information. */
typedef struct _sysmpu_access_err_info
{
    uint32_t master;                        /*!< Access error master. */
    sysmpu_err_attributes_t attributes;        /*!< Access error attributes. */
    sysmpu_err_access_type_t accessType;       /*!< Access error type. */
    sysmpu_err_access_control_t accessControl; /*!< Access error control. */
    uint32_t address;                       /*!< Access error address. */
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    uint8_t processorIdentification; /*!< Access error processor identification. */
#endif                               /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */
} sysmpu_access_err_info_t;

/*! @brief SYSMPU read/write/execute rights control for bus master 0 ~ 3. */
typedef struct _sysmpu_rwxrights_master_access_control
{
    sysmpu_supervisor_access_rights_t superAccessRights; /*!< Master access rights in supervisor mode. */
    sysmpu_user_access_rights_t userAccessRights;        /*!< Master access rights in user mode. */
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    bool processIdentifierEnable; /*!< Enables or disables process identifier. */
#endif                            /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */
} sysmpu_rwxrights_master_access_control_t;

/*! @brief SYSMPU read/write access control for bus master 4 ~ 7. */
typedef struct _sysmpu_rwrights_master_access_control
{
    bool writeEnable; /*!< Enables or disables write permission. */
    bool readEnable;  /*!< Enables or disables read permission.  */
} sysmpu_rwrights_master_access_control_t;

/*!
 * @brief SYSMPU region configuration structure.
 *
 * This structure is used to configure the regionNum region.
 * The accessRights1[0] ~ accessRights1[3] are used to configure the bus master
 * 0 ~ 3 with the privilege rights setting. The accessRights2[0] ~ accessRights2[3]
 * are used to configure the high master 4 ~ 7 with the normal read write permission.
 * The master port assignment is the chip configuration. Normally, the core is the
 * master 0, debugger is the master 1.
 * Note that the SYSMPU assigns a priority scheme where the debugger is treated as the highest
 * priority master followed by the core and then all the remaining masters.
 * SYSMPU protection does not allow writes from the core to affect the "regionNum 0" start
 * and end address nor the permissions associated with the debugger. It can only write
 * the permission fields associated with the other masters. This protection guarantees that
 * the debugger always has access to the entire address space and those rights can't
 * be changed by the core or any other bus master. Prepare
 * the region configuration when regionNum is 0.
 */
typedef struct _sysmpu_region_config
{
    uint32_t regionNum;    /*!< SYSMPU region number, range form 0 ~ FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1. */
    uint32_t startAddress; /*!< Memory region start address. Note: bit0 ~ bit4 always be marked as 0 by SYSMPU. The actual
                              start address is 0-modulo-32 byte address.  */
    uint32_t endAddress; /*!< Memory region end address. Note: bit0 ~ bit4 always be marked as 1 by SYSMPU. The actual end
                          address is 31-modulo-32 byte address. */
    sysmpu_rwxrights_master_access_control_t accessRights1[4]; /*!< Masters with read, write and execute rights setting. */
    sysmpu_rwrights_master_access_control_t accessRights2[4];  /*!< Masters with normal read write rights setting. */
#if FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER
    uint8_t processIdentifier; /*!< Process identifier used when "processIdentifierEnable" set with true. */
    uint8_t
        processIdMask; /*!< Process identifier mask. The setting bit will ignore the same bit in process identifier. */
#endif                 /* FSL_FEATURE_SYSMPU_HAS_PROCESS_IDENTIFIER */
} sysmpu_region_config_t;

/*!
 * @brief The configuration structure for the SYSMPU initialization.
 *
 * This structure is used when calling the SYSMPU_Init function.
 */
typedef struct _sysmpu_config
{
    sysmpu_region_config_t regionConfig; /*!< Region access permission. */
    struct _sysmpu_config *next;         /*!< Pointer to the next structure. */
} sysmpu_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes the SYSMPU with the user configuration structure.
 *
 * This function configures the SYSMPU module with the user-defined configuration.
 *
 * @param base     SYSMPU peripheral base address.
 * @param config   The pointer to the configuration structure.
 */
void SYSMPU_Init(SYSMPU_Type *base, const sysmpu_config_t *config);

/*!
 * @brief Deinitializes the SYSMPU regions.
 *
 * @param base     SYSMPU peripheral base address.
 */
void SYSMPU_Deinit(SYSMPU_Type *base);

/* @}*/

/*!
 * @name Basic Control Operations
 * @{
 */

/*!
 * @brief Enables/disables the SYSMPU globally.
 *
 * Call this API to enable or disable the SYSMPU module.
 *
 * @param base     SYSMPU peripheral base address.
 * @param enable   True enable SYSMPU, false disable SYSMPU.
 */
static inline void SYSMPU_Enable(SYSMPU_Type *base, bool enable)
{
    if (enable)
    {
        /* Enable the SYSMPU globally. */
        base->CESR |= SYSMPU_CESR_VLD_MASK;
    }
    else
    { /* Disable the SYSMPU globally. */
        base->CESR &= ~SYSMPU_CESR_VLD_MASK;
    }
}

/*!
 * @brief Enables/disables the SYSMPU for a special region.
 *
 * When SYSMPU is enabled, call this API to disable an unused region
 * of an enabled SYSMPU. Call this API to minimize the power dissipation.
 *
 * @param base     SYSMPU peripheral base address.
 * @param number   SYSMPU region number.
 * @param enable   True enable the special region SYSMPU, false disable the special region SYSMPU.
 */
static inline void SYSMPU_RegionEnable(SYSMPU_Type *base, uint32_t number, bool enable)
{
    if (enable)
    {
        /* Enable the #number region SYSMPU. */
        base->WORD[number][3] |= SYSMPU_WORD_VLD_MASK;
    }
    else
    { /* Disable the #number region SYSMPU. */
        base->WORD[number][3] &= ~SYSMPU_WORD_VLD_MASK;
    }
}

/*!
 * @brief Gets the SYSMPU basic hardware information.
 *
 * @param base           SYSMPU peripheral base address.
 * @param hardwareInform The pointer to the SYSMPU hardware information structure. See "sysmpu_hardware_info_t".
 */
void SYSMPU_GetHardwareInfo(SYSMPU_Type *base, sysmpu_hardware_info_t *hardwareInform);

/*!
 * @brief Sets the SYSMPU region.
 *
 * Note: Due to the SYSMPU protection, the region number 0 does not allow writes from
 * core to affect the start and end address nor the permissions associated with
 * the debugger. It can only write the permission fields associated
 * with the other masters.
 *
 * @param base          SYSMPU peripheral base address.
 * @param regionConfig  The pointer to the SYSMPU user configuration structure. See "sysmpu_region_config_t".
 */
void SYSMPU_SetRegionConfig(SYSMPU_Type *base, const sysmpu_region_config_t *regionConfig);

/*!
 * @brief Sets the region start and end address.
 *
 * Memory region start address. Note: bit0 ~ bit4 is always marked as 0 by SYSMPU.
 * The actual start address by SYSMPU is 0-modulo-32 byte address.
 * Memory region end address. Note: bit0 ~ bit4 always be marked as 1 by SYSMPU.
 * The end address used by the SYSMPU is 31-modulo-32 byte address.
 * Note: Due to the SYSMPU protection, the startAddr and endAddr can't be
 * changed by the core when regionNum is 0.
 *
 * @param base          SYSMPU peripheral base address.
 * @param regionNum     SYSMPU region number. The range is from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * @param startAddr     Region start address.
 * @param endAddr       Region end address.
 */
void SYSMPU_SetRegionAddr(SYSMPU_Type *base, uint32_t regionNum, uint32_t startAddr, uint32_t endAddr);

/*!
 * @brief Sets the SYSMPU region access rights for masters with read, write, and execute rights.
 * The SYSMPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The privilege rights masters have the read, write, and execute access rights.
 * Except the normal read and write rights, the execute rights are also
 * allowed for these masters. The privilege rights masters normally range from
 * bus masters 0 - 3. However, the maximum master number is device-specific.
 * See the "SYSMPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX".
 * The normal rights masters access rights control see
 * "SYSMPU_SetRegionRwMasterAccessRights()".
 *
 * @param base          SYSMPU peripheral base address.
 * @param regionNum     SYSMPU region number. Should range from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * @param masterNum     SYSMPU bus master number. Should range from 0 to
 * SYSMPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX.
 * @param accessRights  The pointer to the SYSMPU access rights configuration. See "sysmpu_rwxrights_master_access_control_t".
 */
void SYSMPU_SetRegionRwxMasterAccessRights(SYSMPU_Type *base,
                                        uint32_t regionNum,
                                        uint32_t masterNum,
                                        const sysmpu_rwxrights_master_access_control_t *accessRights);
#if FSL_FEATURE_SYSMPU_MASTER_COUNT > 4
/*!
 * @brief Sets the SYSMPU region access rights for masters with read and write rights.
 * The SYSMPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The normal rights masters only have the read and write access permissions.
 * The privilege rights access control see "SYSMPU_SetRegionRwxMasterAccessRights".
 *
 * @param base          SYSMPU peripheral base address.
 * @param regionNum     SYSMPU region number. The range is from 0 to
 * FSL_FEATURE_SYSMPU_DESCRIPTOR_COUNT - 1.
 * @param masterNum     SYSMPU bus master number. Should range from SYSMPU_MASTER_RWATTRIBUTE_START_PORT
 * to ~ FSL_FEATURE_SYSMPU_MASTER_COUNT - 1.
 * @param accessRights  The pointer to the SYSMPU access rights configuration. See "sysmpu_rwrights_master_access_control_t".
 */
void SYSMPU_SetRegionRwMasterAccessRights(SYSMPU_Type *base,
                                       uint32_t regionNum,
                                       uint32_t masterNum,
                                       const sysmpu_rwrights_master_access_control_t *accessRights);
#endif  /* FSL_FEATURE_SYSMPU_MASTER_COUNT > 4 */
/*!
 * @brief Gets the numbers of slave ports where errors occur.
 *
 * @param base       SYSMPU peripheral base address.
 * @param slaveNum   SYSMPU slave port number.
 * @return The slave ports error status.
 *         true  - error happens in this slave port.
 *         false - error didn't happen in this slave port.
 */
bool SYSMPU_GetSlavePortErrorStatus(SYSMPU_Type *base, sysmpu_slave_t slaveNum);

/*!
 * @brief Gets the SYSMPU detailed error access information.
 *
 * @param base       SYSMPU peripheral base address.
 * @param slaveNum   SYSMPU slave port number.
 * @param errInform  The pointer to the SYSMPU access error information. See "sysmpu_access_err_info_t".
 */
void SYSMPU_GetDetailErrorAccessInfo(SYSMPU_Type *base, sysmpu_slave_t slaveNum, sysmpu_access_err_info_t *errInform);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_SYSMPU_H_ */
