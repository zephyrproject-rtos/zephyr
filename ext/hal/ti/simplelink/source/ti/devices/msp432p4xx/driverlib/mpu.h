/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef __MPU_H__
#define __MPU_H__

//*****************************************************************************
//
//! \addtogroup mpu_api
//! @{
//
//*****************************************************************************


//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <stdint.h>

//*****************************************************************************
//
// Flags that can be passed to MPU_enableModule.
//
//*****************************************************************************
#define MPU_CONFIG_PRIV_DEFAULT     MPU_CTRL_PRIVDEFENA_Msk
#define MPU_CONFIG_HARDFLT_NMI      MPU_CTRL_HFNMIENA_Msk
#define MPU_CONFIG_NONE             0

//*****************************************************************************
//
// Flags for the region size to be passed to MPU_setRegion.
//
//*****************************************************************************
#define MPU_RGN_SIZE_32B            (4 << 1)
#define MPU_RGN_SIZE_64B            (5 << 1)
#define MPU_RGN_SIZE_128B           (6 << 1)
#define MPU_RGN_SIZE_256B           (7 << 1)
#define MPU_RGN_SIZE_512B           (8 << 1)

#define MPU_RGN_SIZE_1K             (9 << 1)
#define MPU_RGN_SIZE_2K             (10 << 1)
#define MPU_RGN_SIZE_4K             (11 << 1)
#define MPU_RGN_SIZE_8K             (12 << 1)
#define MPU_RGN_SIZE_16K            (13 << 1)
#define MPU_RGN_SIZE_32K            (14 << 1)
#define MPU_RGN_SIZE_64K            (15 << 1)
#define MPU_RGN_SIZE_128K           (16 << 1)
#define MPU_RGN_SIZE_256K           (17 << 1)
#define MPU_RGN_SIZE_512K           (18 << 1)

#define MPU_RGN_SIZE_1M             (19 << 1)
#define MPU_RGN_SIZE_2M             (20 << 1)
#define MPU_RGN_SIZE_4M             (21 << 1)
#define MPU_RGN_SIZE_8M             (22 << 1)
#define MPU_RGN_SIZE_16M            (23 << 1)
#define MPU_RGN_SIZE_32M            (24 << 1)
#define MPU_RGN_SIZE_64M            (25 << 1)
#define MPU_RGN_SIZE_128M           (26 << 1)
#define MPU_RGN_SIZE_256M           (27 << 1)
#define MPU_RGN_SIZE_512M           (28 << 1)

#define MPU_RGN_SIZE_1G             (29 << 1)
#define MPU_RGN_SIZE_2G             (30 << 1)
#define MPU_RGN_SIZE_4G             (31 << 1)

//*****************************************************************************
//
// Flags for the permissions to be passed to MPU_setRegion.
//
//*****************************************************************************
#define MPU_RGN_PERM_EXEC           0x00000000
#define MPU_RGN_PERM_NOEXEC         0x10000000
#define MPU_RGN_PERM_PRV_NO_USR_NO  0x00000000
#define MPU_RGN_PERM_PRV_RW_USR_NO  0x01000000
#define MPU_RGN_PERM_PRV_RW_USR_RO  0x02000000
#define MPU_RGN_PERM_PRV_RW_USR_RW  0x03000000
#define MPU_RGN_PERM_PRV_RO_USR_NO  0x05000000
#define MPU_RGN_PERM_PRV_RO_USR_RO  0x06000000

//*****************************************************************************
//
// Flags for the sub-region to be passed to MPU_setRegion.
//
//*****************************************************************************
#define MPU_SUB_RGN_DISABLE_0       0x00000100
#define MPU_SUB_RGN_DISABLE_1       0x00000200
#define MPU_SUB_RGN_DISABLE_2       0x00000400
#define MPU_SUB_RGN_DISABLE_3       0x00000800
#define MPU_SUB_RGN_DISABLE_4       0x00001000
#define MPU_SUB_RGN_DISABLE_5       0x00002000
#define MPU_SUB_RGN_DISABLE_6       0x00004000
#define MPU_SUB_RGN_DISABLE_7       0x00008000

//*****************************************************************************
//
// Flags to enable or disable a region, to be passed to MPU_setRegion.
//
//*****************************************************************************
#define MPU_RGN_ENABLE              1
#define MPU_RGN_DISABLE             0

#define NVIC_MPU_TYPE_DREGION_S     8

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! Enables and configures the MPU for use.
//!
//! \param mpuConfig is the logical OR of the possible configurations.
//!
//! This function enables the Cortex-M memory protection unit.  It also
//! configures the default behavior when in privileged mode and while handling
//! a hard fault or NMI.  Prior to enabling the MPU, at least one region must
//! be set by calling MPU_setRegion() or else by enabling the default region for
//! privileged mode by passing the \b MPU_CONFIG_PRIV_DEFAULT flag to
//! MPU_enableModule().  Once the MPU is enabled, a memory management fault is
//! generated for memory access violations.
//!
//! The \e mpuConfig parameter should be the logical OR of any of the
//! following:
//!
//! - \b MPU_CONFIG_PRIV_DEFAULT enables the default memory map when in
//! privileged mode and when no other regions are defined.  If this option
//! is not enabled, then there must be at least one valid region already
//! defined when the MPU is enabled.
//! - \b MPU_CONFIG_HARDFLT_NMI enables the MPU while in a hard fault or NMI
//! exception handler.  If this option is not enabled, then the MPU is
//! disabled while in one of these exception handlers and the default
//! memory map is applied.
//! - \b MPU_CONFIG_NONE chooses none of the above options.  In this case,
//! no default memory map is provided in privileged mode, and the MPU is
//! not enabled in the fault handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_enableModule(uint32_t mpuConfig);

//*****************************************************************************
//
//! Disables the MPU for use.
//!
//! This function disables the Cortex-M memory protection unit.  When the
//! MPU is disabled, the default memory map is used and memory management
//! faults are not generated.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_disableModule(void);

//*****************************************************************************
//
//! Gets the count of regions supported by the MPU.
//!
//! This function is used to get the total number of regions that are supported
//! by the MPU, including regions that are already programmed.
//!
//! \return The number of memory protection regions that are available
//! for programming using MPU_setRegion().
//
//*****************************************************************************
extern uint32_t MPU_getRegionCount(void);

//*****************************************************************************
//
//! Enables a specific region.
//!
//! \param region is the region number to enable. Valid values are between
//!  0 and 7 inclusively.
//!
//! This function is used to enable a memory protection region.  The region
//! should already be configured with the MPU_setRegion() function.  Once
//! enabled, the memory protection rules of the region are applied and access
//! violations cause a memory management fault.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_enableRegion(uint32_t region);

//*****************************************************************************
//
//! Disables a specific region.
//!
//! \param region is the region number to disable. Valid values are between
//!  0 and 7 inclusively.
//!
//! This function is used to disable a previously enabled memory protection
//! region.  The region remains configured if it is not overwritten with
//! another call to MPU_setRegion(), and can be enabled again by calling
//! MPU_enableRegion().
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_disableRegion(uint32_t region);

//*****************************************************************************
//
//! Sets up the access rules for a specific region.
//!
//! \param region is the region number to set up.
//! \param addr is the base address of the region.  It must be aligned
//! according to the size of the region specified in flags.
//! \param flags is a set of flags to define the attributes of the region.
//!
//! This function sets up the protection rules for a region.  The region has
//! a base address and a set of attributes including the size. The base
//! address parameter, \e addr, must be aligned according to the size, and
//! the size must be a power of 2.
//!
//! \param region is the region number to set. Valid values are between
//!  0 and 7 inclusively.
//!
//! The \e flags parameter is the logical OR of all of the attributes
//! of the region.  It is a combination of choices for region size,
//! execute permission, read/write permissions, disabled sub-regions,
//! and a flag to determine if the region is enabled.
//!
//! The size flag determines the size of a region and must be one of the
//! following:
//!
//! - \b MPU_RGN_SIZE_32B
//! - \b MPU_RGN_SIZE_64B
//! - \b MPU_RGN_SIZE_128B
//! - \b MPU_RGN_SIZE_256B
//! - \b MPU_RGN_SIZE_512B
//! - \b MPU_RGN_SIZE_1K
//! - \b MPU_RGN_SIZE_2K
//! - \b MPU_RGN_SIZE_4K
//! - \b MPU_RGN_SIZE_8K
//! - \b MPU_RGN_SIZE_16K
//! - \b MPU_RGN_SIZE_32K
//! - \b MPU_RGN_SIZE_64K
//! - \b MPU_RGN_SIZE_128K
//! - \b MPU_RGN_SIZE_256K
//! - \b MPU_RGN_SIZE_512K
//! - \b MPU_RGN_SIZE_1M
//! - \b MPU_RGN_SIZE_2M
//! - \b MPU_RGN_SIZE_4M
//! - \b MPU_RGN_SIZE_8M
//! - \b MPU_RGN_SIZE_16M
//! - \b MPU_RGN_SIZE_32M
//! - \b MPU_RGN_SIZE_64M
//! - \b MPU_RGN_SIZE_128M
//! - \b MPU_RGN_SIZE_256M
//! - \b MPU_RGN_SIZE_512M
//! - \b MPU_RGN_SIZE_1G
//! - \b MPU_RGN_SIZE_2G
//! - \b MPU_RGN_SIZE_4G
//!
//! The execute permission flag must be one of the following:
//!
//! - \b MPU_RGN_PERM_EXEC enables the region for execution of code
//! - \b MPU_RGN_PERM_NOEXEC disables the region for execution of code
//!
//! The read/write access permissions are applied separately for the
//! privileged and user modes.  The read/write access flags must be one
//! of the following:
//!
//! - \b MPU_RGN_PERM_PRV_NO_USR_NO - no access in privileged or user mode
//! - \b MPU_RGN_PERM_PRV_RW_USR_NO - privileged read/write, user no access
//! - \b MPU_RGN_PERM_PRV_RW_USR_RO - privileged read/write, user read-only
//! - \b MPU_RGN_PERM_PRV_RW_USR_RW - privileged read/write, user read/write
//! - \b MPU_RGN_PERM_PRV_RO_USR_NO - privileged read-only, user no access
//! - \b MPU_RGN_PERM_PRV_RO_USR_RO - privileged read-only, user read-only
//!
//! The region is automatically divided into 8 equally-sized sub-regions by
//! the MPU.  Sub-regions can only be used in regions of size 256 bytes
//! or larger.  Any of these 8 sub-regions can be disabled, allowing for
//! creation of ``holes'' in a region which can be left open, or overlaid
//! by another region with different attributes.  Any of the 8 sub-regions
//! can be disabled with a logical OR of any of the following flags:
//!
//! - \b MPU_SUB_RGN_DISABLE_0
//! - \b MPU_SUB_RGN_DISABLE_1
//! - \b MPU_SUB_RGN_DISABLE_2
//! - \b MPU_SUB_RGN_DISABLE_3
//! - \b MPU_SUB_RGN_DISABLE_4
//! - \b MPU_SUB_RGN_DISABLE_5
//! - \b MPU_SUB_RGN_DISABLE_6
//! - \b MPU_SUB_RGN_DISABLE_7
//!
//! Finally, the region can be initially enabled or disabled with one of
//! the following flags:
//!
//! - \b MPU_RGN_ENABLE
//! - \b MPU_RGN_DISABLE
//!
//! As an example, to set a region with the following attributes: size of
//! 32 KB, execution enabled, read-only for both privileged and user, one
//! sub-region disabled, and initially enabled; the \e flags parameter would
//! have the following value:
//!
//! <code>
//! (MPU_RGN_SIZE_32K | MPU_RGN_PERM_EXEC | MPU_RGN_PERM_PRV_RO_USR_RO |
//!  MPU_SUB_RGN_DISABLE_2 | MPU_RGN_ENABLE)
//! </code>
//!
//! \note This function writes to multiple registers and is not protected
//! from interrupts.  It is possible that an interrupt which accesses a
//! region may occur while that region is in the process of being changed.
//! The safest way to handle this is to disable a region before changing it.
//! Refer to the discussion of this in the API Detailed Description section.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_setRegion(uint32_t region, uint32_t addr, uint32_t flags);

//*****************************************************************************
//
//! Gets the current settings for a specific region.
//!
//! \param region is the region number to get. Valid values are between
//!  0 and 7 inclusively.
//! \param addr points to storage for the base address of the region.
//! \param pflags points to the attribute flags for the region.
//!
//! This function retrieves the configuration of a specific region.  The
//! meanings and format of the parameters is the same as that of the
//! MPU_setRegion() function.
//!
//! This function can be used to save the configuration of a region for later
//! use with the MPU_setRegion() function.  The region's enable state is
//! preserved in the attributes that are saved.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_getRegion(uint32_t region, uint32_t *addr, uint32_t *pflags);

//*****************************************************************************
//
//! Registers an interrupt handler for the memory management fault.
//!
//! \param intHandler is a pointer to the function to be called when the
//! memory management fault occurs.
//!
//! This function sets and enables the handler to be called when the MPU
//! generates a memory management fault due to a protection region access
//! violation.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters an interrupt handler for the memory management fault.
//!
//! This function disables and clears the handler to be called when a
//! memory management fault occurs.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_unregisterInterrupt(void);

//*****************************************************************************
//
//! Enables the interrupt for the memory management fault.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_enableInterrupt(void);

//*****************************************************************************
//
//! Disables the interrupt for the memory management fault.
//!
//! \return None.
//
//*****************************************************************************
extern void MPU_disableInterrupt(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif //  __MPU_H__
