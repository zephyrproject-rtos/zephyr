/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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
#ifndef _FSL_MPU_H_
#define _FSL_MPU_H_

#include "fsl_common.h"

/*!
 * @addtogroup mpu
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief MPU driver version 2.1.0. */
#define FSL_MPU_DRIVER_VERSION (MAKE_VERSION(2, 1, 1))
/*@}*/

/*! @brief MPU the bit shift for masters with privilege rights: read write and execute. */
#define MPU_REGION_RWXRIGHTS_MASTER_SHIFT(n) (n * 6)

/*! @brief MPU masters with read, write and execute rights bit mask. */
#define MPU_REGION_RWXRIGHTS_MASTER_MASK(n) (0x1Fu << MPU_REGION_RWXRIGHTS_MASTER_SHIFT(n))

/*! @brief MPU masters with read, write and execute rights bit width. */
#define MPU_REGION_RWXRIGHTS_MASTER_WIDTH 5

/*! @brief MPU masters with read, write and execute rights priority setting. */
#define MPU_REGION_RWXRIGHTS_MASTER(n, x) \
    (((uint32_t)(((uint32_t)(x)) << MPU_REGION_RWXRIGHTS_MASTER_SHIFT(n))) & MPU_REGION_RWXRIGHTS_MASTER_MASK(n))

/*! @brief MPU masters with read, write and execute rights process enable bit shift. */
#define MPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n) (n * 6 + MPU_REGION_RWXRIGHTS_MASTER_WIDTH)

/*! @brief MPU masters with read, write and execute rights process enable bit mask. */
#define MPU_REGION_RWXRIGHTS_MASTER_PE_MASK(n) (0x1u << MPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n))

/*! @brief MPU masters with read, write and execute rights process enable setting. */
#define MPU_REGION_RWXRIGHTS_MASTER_PE(n, x) \
    (((uint32_t)(((uint32_t)(x)) << MPU_REGION_RWXRIGHTS_MASTER_PE_SHIFT(n))) & MPU_REGION_RWXRIGHTS_MASTER_PE_MASK(n))

/*! @brief MPU masters with normal read write permission bit shift. */
#define MPU_REGION_RWRIGHTS_MASTER_SHIFT(n) ((n - FSL_FEATURE_MPU_PRIVILEGED_RIGHTS_MASTER_COUNT) * 2 + 24)

/*! @brief MPU masters with normal read write rights bit mask. */
#define MPU_REGION_RWRIGHTS_MASTER_MASK(n) (0x3u << MPU_REGION_RWRIGHTS_MASTER_SHIFT(n))

/*! @brief MPU masters with normal read write rights priority setting. */
#define MPU_REGION_RWRIGHTS_MASTER(n, x) \
    (((uint32_t)(((uint32_t)(x)) << MPU_REGION_RWRIGHTS_MASTER_SHIFT(n))) & MPU_REGION_RWRIGHTS_MASTER_MASK(n))

/*! @brief the Slave port numbers. */
#define MPU_SLAVE_PORT_NUM  (4u)
/*! @brief define the maximum index of master with privileged rights. */
#define MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX (3)

/*! @brief Describes the number of MPU regions. */
typedef enum _mpu_region_total_num
{
    kMPU_8Regions = 0x0U,  /*!< MPU supports 8 regions.  */
    kMPU_12Regions = 0x1U, /*!< MPU supports 12 regions. */
    kMPU_16Regions = 0x2U  /*!< MPU supports 16 regions. */
} mpu_region_total_num_t;

/*! @brief MPU slave port number. */
typedef enum _mpu_slave
{
    kMPU_Slave0 = 0U, /*!< MPU slave port 0. */
    kMPU_Slave1 = 1U, /*!< MPU slave port 1. */
    kMPU_Slave2 = 2U, /*!< MPU slave port 2. */
    kMPU_Slave3 = 3U, /*!< MPU slave port 3. */
    kMPU_Slave4 = 4U  /*!< MPU slave port 4. */
} mpu_slave_t;

/*! @brief MPU error access control detail. */
typedef enum _mpu_err_access_control
{
    kMPU_NoRegionHit = 0U,        /*!< No region hit error. */
    kMPU_NoneOverlappRegion = 1U, /*!< Access single region error. */
    kMPU_OverlappRegion = 2U      /*!< Access overlapping region error. */
} mpu_err_access_control_t;

/*! @brief MPU error access type. */
typedef enum _mpu_err_access_type
{
    kMPU_ErrTypeRead = 0U, /*!< MPU error access type --- read.  */
    kMPU_ErrTypeWrite = 1U /*!< MPU error access type --- write. */
} mpu_err_access_type_t;

/*! @brief MPU access error attributes.*/
typedef enum _mpu_err_attributes
{
    kMPU_InstructionAccessInUserMode = 0U,       /*!< Access instruction error in user mode. */
    kMPU_DataAccessInUserMode = 1U,              /*!< Access data error in user mode. */
    kMPU_InstructionAccessInSupervisorMode = 2U, /*!< Access instruction error in supervisor mode. */
    kMPU_DataAccessInSupervisorMode = 3U         /*!< Access data error in supervisor mode. */
} mpu_err_attributes_t;

/*! @brief MPU access rights in supervisor mode for bus master 0 ~ 3. */
typedef enum _mpu_supervisor_access_rights
{
    kMPU_SupervisorReadWriteExecute = 0U, /*!< Read write and execute operations are allowed in supervisor mode. */
    kMPU_SupervisorReadExecute = 1U,      /*!< Read and execute operations are allowed in supervisor mode. */
    kMPU_SupervisorReadWrite = 2U,        /*!< Read write operations are allowed in supervisor mode. */
    kMPU_SupervisorEqualToUsermode = 3U   /*!< Access permission equal to user mode. */
} mpu_supervisor_access_rights_t;

/*! @brief MPU access rights in user mode for bus master 0 ~ 3. */
typedef enum _mpu_user_access_rights
{
    kMPU_UserNoAccessRights = 0U,  /*!< No access allowed in user mode.  */
    kMPU_UserExecute = 1U,         /*!< Execute operation is allowed in user mode. */
    kMPU_UserWrite = 2U,           /*!< Write operation is allowed in user mode. */
    kMPU_UserWriteExecute = 3U,    /*!< Write and execute operations are allowed in user mode. */
    kMPU_UserRead = 4U,            /*!< Read is allowed in user mode. */
    kMPU_UserReadExecute = 5U,     /*!< Read and execute operations are allowed in user mode. */
    kMPU_UserReadWrite = 6U,       /*!< Read and write operations are allowed in user mode. */
    kMPU_UserReadWriteExecute = 7U /*!< Read write and execute operations are allowed in user mode. */
} mpu_user_access_rights_t;

/*! @brief MPU hardware basic information. */
typedef struct _mpu_hardware_info
{
    uint8_t hardwareRevisionLevel;         /*!< Specifies the MPU's hardware and definition reversion level. */
    uint8_t slavePortsNumbers;             /*!< Specifies the number of slave ports connected to MPU. */
    mpu_region_total_num_t regionsNumbers; /*!< Indicates the number of region descriptors implemented. */
} mpu_hardware_info_t;

/*! @brief MPU detail error access information. */
typedef struct _mpu_access_err_info
{
    uint32_t master;                        /*!< Access error master. */
    mpu_err_attributes_t attributes;        /*!< Access error attributes. */
    mpu_err_access_type_t accessType;       /*!< Access error type. */
    mpu_err_access_control_t accessControl; /*!< Access error control. */
    uint32_t address;                       /*!< Access error address. */
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    uint8_t processorIdentification; /*!< Access error processor identification. */
#endif                               /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */
} mpu_access_err_info_t;

/*! @brief MPU read/write/execute rights control for bus master 0 ~ 3. */
typedef struct _mpu_rwxrights_master_access_control
{
    mpu_supervisor_access_rights_t superAccessRights; /*!< Master access rights in supervisor mode. */
    mpu_user_access_rights_t userAccessRights;        /*!< Master access rights in user mode. */
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    bool processIdentifierEnable; /*!< Enables or disables process identifier. */
#endif                            /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */
} mpu_rwxrights_master_access_control_t;

/*! @brief MPU read/write access control for bus master 4 ~ 7. */
typedef struct _mpu_rwrights_master_access_control
{
    bool writeEnable; /*!< Enables or disables write permission. */
    bool readEnable;  /*!< Enables or disables read permission.  */
} mpu_rwrights_master_access_control_t;

/*!
 * @brief MPU region configuration structure.
 *
 * This structure is used to configure the regionNum region.
 * The accessRights1[0] ~ accessRights1[3] are used to configure the bus master
 * 0 ~ 3 with the privilege rights setting. The accessRights2[0] ~ accessRights2[3]
 * are used to configure the high master 4 ~ 7 with the normal read write permission.
 * The master port assignment is the chip configuration. Normally, the core is the
 * master 0, debugger is the master 1.
 * Note that the MPU assigns a priority scheme where the debugger is treated as the highest
 * priority master followed by the core and then all the remaining masters.
 * MPU protection does not allow writes from the core to affect the "regionNum 0" start
 * and end address nor the permissions associated with the debugger. It can only write
 * the permission fields associated with the other masters. This protection guarantees that
 * the debugger always has access to the entire address space and those rights can't
 * be changed by the core or any other bus master. Prepare
 * the region configuration when regionNum is 0.
 */
typedef struct _mpu_region_config
{
    uint32_t regionNum;    /*!< MPU region number, range form 0 ~ FSL_FEATURE_MPU_DESCRIPTOR_COUNT - 1. */
    uint32_t startAddress; /*!< Memory region start address. Note: bit0 ~ bit4 always be marked as 0 by MPU. The actual
                              start address is 0-modulo-32 byte address.  */
    uint32_t endAddress; /*!< Memory region end address. Note: bit0 ~ bit4 always be marked as 1 by MPU. The actual end
                          address is 31-modulo-32 byte address. */
    mpu_rwxrights_master_access_control_t accessRights1[4]; /*!< Masters with read, write and execute rights setting. */
    mpu_rwrights_master_access_control_t accessRights2[4];  /*!< Masters with normal read write rights setting. */
#if FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER
    uint8_t processIdentifier; /*!< Process identifier used when "processIdentifierEnable" set with true. */
    uint8_t
        processIdMask; /*!< Process identifier mask. The setting bit will ignore the same bit in process identifier. */
#endif                 /* FSL_FEATURE_MPU_HAS_PROCESS_IDENTIFIER */
} mpu_region_config_t;

/*!
 * @brief The configuration structure for the MPU initialization.
 *
 * This structure is used when calling the MPU_Init function.
 */
typedef struct _mpu_config
{
    mpu_region_config_t regionConfig; /*!< Region access permission. */
    struct _mpu_config *next;         /*!< Pointer to the next structure. */
} mpu_config_t;

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
 * @brief Initializes the MPU with the user configuration structure.
 *
 * This function configures the MPU module with the user-defined configuration.
 *
 * @param base     MPU peripheral base address.
 * @param config   The pointer to the configuration structure.
 */
void MPU_Init(MPU_Type *base, const mpu_config_t *config);

/*!
 * @brief Deinitializes the MPU regions.
 *
 * @param base     MPU peripheral base address.
 */
void MPU_Deinit(MPU_Type *base);

/* @}*/

/*!
 * @name Basic Control Operations
 * @{
 */

/*!
 * @brief Enables/disables the MPU globally.
 *
 * Call this API to enable or disable the MPU module.
 *
 * @param base     MPU peripheral base address.
 * @param enable   True enable MPU, false disable MPU.
 */
static inline void MPU_Enable(MPU_Type *base, bool enable)
{
    if (enable)
    {
        /* Enable the MPU globally. */
        base->CESR |= MPU_CESR_VLD_MASK;
    }
    else
    { /* Disable the MPU globally. */
        base->CESR &= ~MPU_CESR_VLD_MASK;
    }
}

/*!
 * @brief Enables/disables the MPU for a special region.
 *
 * When MPU is enabled, call this API to disable an unused region
 * of an enabled MPU. Call this API to minimize the power dissipation.
 *
 * @param base     MPU peripheral base address.
 * @param number   MPU region number.
 * @param enable   True enable the special region MPU, false disable the special region MPU.
 */
static inline void MPU_RegionEnable(MPU_Type *base, uint32_t number, bool enable)
{
    if (enable)
    {
        /* Enable the #number region MPU. */
        base->WORD[number][3] |= MPU_WORD_VLD_MASK;
    }
    else
    { /* Disable the #number region MPU. */
        base->WORD[number][3] &= ~MPU_WORD_VLD_MASK;
    }
}

/*!
 * @brief Gets the MPU basic hardware information.
 *
 * @param base           MPU peripheral base address.
 * @param hardwareInform The pointer to the MPU hardware information structure. See "mpu_hardware_info_t".
 */
void MPU_GetHardwareInfo(MPU_Type *base, mpu_hardware_info_t *hardwareInform);

/*!
 * @brief Sets the MPU region.
 *
 * Note: Due to the MPU protection, the region number 0 does not allow writes from
 * core to affect the start and end address nor the permissions associated with
 * the debugger. It can only write the permission fields associated
 * with the other masters.
 *
 * @param base          MPU peripheral base address.
 * @param regionConfig  The pointer to the MPU user configuration structure. See "mpu_region_config_t".
 */
void MPU_SetRegionConfig(MPU_Type *base, const mpu_region_config_t *regionConfig);

/*!
 * @brief Sets the region start and end address.
 *
 * Memory region start address. Note: bit0 ~ bit4 is always marked as 0 by MPU.
 * The actual start address by MPU is 0-modulo-32 byte address.
 * Memory region end address. Note: bit0 ~ bit4 always be marked as 1 by MPU.
 * The end address used by the MPU is 31-modulo-32 byte address.
 * Note: Due to the MPU protection, the startAddr and endAddr can't be
 * changed by the core when regionNum is 0.
 *
 * @param base          MPU peripheral base address.
 * @param regionNum     MPU region number. The range is from 0 to
 * FSL_FEATURE_MPU_DESCRIPTOR_COUNT - 1.
 * @param startAddr     Region start address.
 * @param endAddr       Region end address.
 */
void MPU_SetRegionAddr(MPU_Type *base, uint32_t regionNum, uint32_t startAddr, uint32_t endAddr);

/*!
 * @brief Sets the MPU region access rights for masters with read, write, and execute rights.
 * The MPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The privilege rights masters have the read, write, and execute access rights.
 * Except the normal read and write rights, the execute rights are also
 * allowed for these masters. The privilege rights masters normally range from
 * bus masters 0 - 3. However, the maximum master number is device-specific.
 * See the "MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX".
 * The normal rights masters access rights control see
 * "MPU_SetRegionRwMasterAccessRights()".
 *
 * @param base          MPU peripheral base address.
 * @param regionNum     MPU region number. Should range from 0 to
 * FSL_FEATURE_MPU_DESCRIPTOR_COUNT - 1.
 * @param masterNum     MPU bus master number. Should range from 0 to
 * MPU_PRIVILEGED_RIGHTS_MASTER_MAX_INDEX.
 * @param accessRights  The pointer to the MPU access rights configuration. See "mpu_rwxrights_master_access_control_t".
 */
void MPU_SetRegionRwxMasterAccessRights(MPU_Type *base,
                                        uint32_t regionNum,
                                        uint32_t masterNum,
                                        const mpu_rwxrights_master_access_control_t *accessRights);
#if FSL_FEATURE_MPU_HAS_MASTER_4_7
/*!
 * @brief Sets the MPU region access rights for masters with read and write rights.
 * The MPU access rights depend on two board classifications of bus masters.
 * The privilege rights masters and the normal rights masters.
 * The normal rights masters only have the read and write access permissions.
 * The privilege rights access control see "MPU_SetRegionRwxMasterAccessRights".
 *
 * @param base          MPU peripheral base address.
 * @param regionNum     MPU region number. The range is from 0 to
 * FSL_FEATURE_MPU_DESCRIPTOR_COUNT - 1.
 * @param masterNum     MPU bus master number. Should range from FSL_FEATURE_MPU_PRIVILEGED_RIGHTS_MASTER_COUNT
 * to ~ FSL_FEATURE_MPU_MASTER_MAX_INDEX.
 * @param accessRights  The pointer to the MPU access rights configuration. See "mpu_rwrights_master_access_control_t".
 */
void MPU_SetRegionRwMasterAccessRights(MPU_Type *base,
                                       uint32_t regionNum,
                                       uint32_t masterNum,
                                       const mpu_rwrights_master_access_control_t *accessRights);
#endif  /* FSL_FEATURE_MPU_HAS_MASTER_4_7 */
/*!
 * @brief Gets the numbers of slave ports where errors occur.
 *
 * @param base       MPU peripheral base address.
 * @param slaveNum   MPU slave port number.
 * @return The slave ports error status.
 *         true  - error happens in this slave port.
 *         false - error didn't happen in this slave port.
 */
bool MPU_GetSlavePortErrorStatus(MPU_Type *base, mpu_slave_t slaveNum);

/*!
 * @brief Gets the MPU detailed error access information.
 *
 * @param base       MPU peripheral base address.
 * @param slaveNum   MPU slave port number.
 * @param errInform  The pointer to the MPU access error information. See "mpu_access_err_info_t".
 */
void MPU_GetDetailErrorAccessInfo(MPU_Type *base, mpu_slave_t slaveNum, mpu_access_err_info_t *errInform);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_MPU_H_ */
