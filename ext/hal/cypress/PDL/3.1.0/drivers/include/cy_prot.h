/***************************************************************************//**
* \file cy_prot.h
* \version 1.20
*
* \brief
* Provides an API declaration of the Protection Unit driver
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

/**
* \defgroup group_prot Protection Unit (Prot)
* \{
*
* The Protection Unit driver provides an API to configure the Memory Protection
* Units (MPU), Shared Memory Protection Units (SMPU), and Peripheral Protection
* Units (PPU). These are separate from the ARM Core MPUs and provide additional
* mechanisms for securing resource accesses. The Protection units address the 
* following concerns in an embedded design:
* - <b>Security requirements:</b> This includes the prevention of malicious attacks
*   to access secure memory or peripherals.
* - <b>Safety requirements:</b> This includes detection of accidental (non-malicious)
*   SW errors and random HW errors. It is important to enable failure analysis
*   to investigate the root cause of a safety violation.
* 
* \section group_prot_protection_type Protection Types
*
* Protection units are hardware configuration structures that control bus accesses 
* to the resources that they protect. By combining these individual configuration 
* structures, a system is built to allow strict restrictions on the capabilities
* of individual bus masters (e.g. CM0+, CM4, Crypt) and their operating modes.
* This architecture can then be integrated into the overall security system
* of the end application. To build this system, 3 main protection unit types
* are available; MPU, SMPU and PPU. When a resource is accessed (memory/register),
* it must pass the evaluation performed for each category. These access evaluations
* are prioritized, where MPU has the highest priority, followed by SMPU, followed
* by PPU. i.e. if an SMPU and a PPU protect the same resource and if access is
* denied by the SMPU, then the PPU access evaluation is skipped. This can lead to a 
* denial-of-service scenario and the application should pay special attention in
* taking ownership of the protection unit configurations.
*
* \subsection group_prot_memory_protection Memory Protection
*
* Memory access control for a bus master is controlled using an MPU. These are
* most often used to distinguish user and privileged accesses from a single bus 
* master such as task switching in an OS/kernel. For ARM cores (CM0+, CM4), the 
* core MPUs are used to perform this task. For other non-ARM bus masters such 
* as Crypto, MPU structs are available, which can be used in a similar manner
* as the ARM core MPUs. These MPUs however must be configured by the ARM cores.
* Other bus masters that do not have an MPU, such as DMA (DW), inherit the access
* control attributes of the bus master that configured the channel. Also note
* that unlike other protection units, MPUs do not support protection context
* evaluation. MPU structs have a descending priority, where larger index struct
* has higher priority access evaluation over lower index structs. E.g. MPU_STRUCT15
* has higher priority than MPU_STRUCT14 and its access will be evaluated before
* MPU_STRUCT14. If both target the same memory, then the higher index (MPU_STRUCT15)
* will be used, and the lower index (MPU_STRUCT14) will be ignored.
*
* \subsection group_prot_shared_memory_protection Shared Memory Protection
*
* In order to protect a region of memory from all bus masters, an SMPU is used.
* This protection effectively allows only those with correct bus master access
* settings to read/write/execute the memory region. This type of protection
* is used in general memory such as Flash and SRAM. Peripheral registers are
* best configured using the peripheral protection units instead. SMPU structs 
* have a descending priority, where larger index struct has higher priority 
* access evaluation over lower index structs. E.g. SMPU_STRUCT15 has higher priority
* than SMPU_STRUCT14 and its access will be evaluated before SMPU_STRUCT14. 
* If both target the same memory, then the higher index (MPU_STRUCT15) will be
* used, and the lower index (SMPU_STRUCT14) will be ignored.
*
* \subsection group_prot_peripheral_protection Peripheral Protection
*
* Peripheral protection is provided by PPUs and allow control of peripheral
* register accesses by bus masters. Four types of PPUs are available.
* - <b>Fixed Group (GR) PPUs</b> are used to protect an entire peripheral MMIO group
*   from invalid bus master accesses. The MMIO grouping information and which
*   resource belongs to which group is device specific and can be obtained 
*   from the device technical reference manual (TRM). Group PPUs have the highest
*   priority in the PPU category. Therefore their access evaluations take precedence
*   over the other types of PPUs.
* - <b>Programmable (PROG) PPUs</b> are used to protect any peripheral memory region
*   in a device from invalid bus master accesses. It is the most versatile
*   type of peripheral protection unit. Programmable PPUs have the second highest
*   priority and take precedence over Region PPUs and Slave PPUs. Similar to SMPUs,
*   higher index PROG PPUs have higher priority than lower indexes PROG PPUs.
* - <b>Fixed Region (RG) PPUs</b> are used to protect an entire peripheral slave 
*   instance from invalid bus master accesses. For example, TCPWM0, TCPWM1, 
*   SCB0, and SCB1, etc. Region PPUs have the third highest priority and take precedence
*   over Slave PPUs.
* - <b>Fixed Slave (SL) PPUs</b> are used to protect specified regions of peripheral
*   instances. For example, individual DW channel structs, SMPU structs, and 
*   IPC structs, etc. Slave PPUs have the lowest priority in the PPU category and
*   therefore are evaluated last.
*
* \section group_prot_protection_context Protection Context
* 
* Protection context (PC) attribute is present in all bus masters and is evaluated
* when accessing memory protected by an SMPU or a PPU. There are no limitations
* to how the PC values are allocated to the bus masters and this makes it
* possible for multiple bus masters to essentially share protection context
* values. The exception to this rule is the PC value 0.
*
* \subsection group_prot_pc0 PC=0
*
* Protection context 0 is a hardware controlled protection context update
* mechanism that allows only a single entry point for transitioning into PC=0
* value. This mechanism is only present for the secure CM0+ core and is a
* fundamental feature in defining a security solution. While all bus masters
* are configured to PC=0 at device boot, it is up to the security solution
* to transition these bus masters to PC!=0 values. Once this is done, those
* bus masters can no longer revert back to PC=0 and can no longer access
* resources protected at PC=0.
*
* In order to enter PC=0, the CM0+ core must assign an interrupt vector or
* an exception handler address to the CPUSS.CM0_PC0_HANDLER register. This
* allows the hardware to check whether the executing code address matches the
* value in this register. If they match, the current PC value is saved and
* the CM0+ bus master automatically transitions to PC=0. It is then up to
* the executing code to decide if and when it will revert to a PC!=0 value.
* At that point, the only way to re-transition to PC=0 is through the defined
* exception/interrupt handler.
*
* \section group_prot_access_evaluation Access Evaluation
*
* Each protection unit is capable of evaluating several access types. These can
* be used to build a system of logical evaluations for different kinds of
* bus master modes of operations. These access types can be divided into
* three broad access categories.
*
* - <b>User/Privileged access:</b> The ARM convention of user mode versus privileged
*   mode is applied in the protection units. For ARM cores, switching between
*   user and privileged modes is handled by updating its Control register or
*   by exception entries. Other bus masters such as Crypto have their own
*   user/privileged settings bit in the bus master control register. This is
*   then controlled by the ARM cores. Bus masters that do not have 
*   user/privileged access controls, such as DMA, inherit their attributes
*   from the bus master that configured it. The user/privileged distinction
*   is used mainly in the MPUs for single bus master accesses but they can
*   also be used in all other protection units.
* - <b>Secure/Non-secure access:</b> The secure/non-secure attribute is another 
*   identifier to distinguish between two separate modes of operations. Much
*   like the user/privileged access, the secure/non-secure mode flag is present
*   in the bus master control register. The ARM core does not have this
*   attribute in its control register and must use the bus master control
*   register instead. Bus masters that inherit their attributes, such as DMA,
*   inherit the secure/non-secure attribute. The primary use-case for this
*   access evaluation is to define a region to be secure or non-secure using
*   an SMPU or a PPU. A bus master with a secure attribute can access
*   both secure and non-secure regions, whereas a bus master with non-secure
*   attribute can only access non-secure regions.
* - <b>Protection Context access:</b> Protection Context is an attribute
*   that serves two purposes; To enter the hardware controlled secure PC=0 
*   mode of operation from non-secure modes and to provide finer granularity
*   to the bus master access definitions. It is used in SMPU and PPU configuration
*   to control which bus master protection context can access the resources 
*   that they protect.
*
* \section group_prot_protection_structure Protection Structure
*
* Each protection unit is comprised of a master struct and a slave struct pair.
* The exception to this rule is MPU structs, which only have the slave struct
* equivalent. The protection units apply their access evaluations in a decreasing
* index order. For example, if SMPU1 and SMPU2 both protect a specific memory region,
* the the higher index (SMPU2) will be evaluated first. In a secure system, the 
* higher index protection structs would then provide the high level of security
* and the lower indexes would provide the lower level of security. Refer to the
* \ref group_prot_protection_type section for more information.
*
* \subsection group_prot_slave_struct Slave Struct
*
* The slave struct is used to configure the protection settings for the resource
* of interest (memory/registers). Depending on the type of protection unit,
* the available attributes differ. However all Slave protection units have the
* following general format.
*
* \subsubsection group_prot_slave_addr Slave Struct Address Definition
*
* - Address: For MPU, SMPU and PROG PPU, the address field is used to define
*   the base memory region to apply the protection. This field has a dependency 
*   on the region size, which dictates the alignment of the protection unit. E.g.
*   if the region size is 64KB, the address field is aligned to 64KB. Hence
*   the lowest bits [15:0] are ignored. For instance, if the address is defined 
*   at 0x0800FFFF, the protection unit would apply its protection settings from
*   0x08000000. Thus alignment must be checked before defining the protection 
*   address. The address field for other PPUs are not used, as they are bound 
*   to their respective peripheral memory locations. 
* - Region Size: For MPU, SMPU and PROG PPU, the region size is used to define
*   the memory block size to apply the protection settings, starting from the
*   defined base address. It is also used to define the 8 sub-regions for the
*   chosen memory block. E.g. If the region size is 64KB, each subregion would
*   be 8KB. This information can then be used to disable the protection
*   settings for select subregions, which gives finer granularity to the 
*   memory regions. PPUs do not have region size definitions as they are bound 
*   to their respective peripheral memory locations. 
* - Subregions: The memory block defined by the address and region size fields
*   is divided into 8 (0 to 7) equally spaced subregions. The protection settings
*   of the protection unit can be disabled for these subregions. E.g. for a 
*   given 64KB of memory block starting from address 0x08000000, disabling 
*   subregion 0 would result in the protection settings not affecting the memory
*   located between 0x08000000 to 0x08001FFF. PPUs do not have subregion 
*   definitions as they are bound to their respective peripheral memory locations. 
*
* \subsubsection group_prot_slave_attr Slave Struct Attribute Definition
*
* - User Permission: Protection units can control the access restrictions
*   of the read (R), write (W) and execute (X) (subject to their availability 
*   depending on the type of protection unit) operations on the memory block 
*   when the bus master is operating in user mode. PPU structs do not provide
*   execute attributes.
* - Privileged Permission: Similar to the user permission, protection units can
*   control the access restrictions of the read (R), write (W) and execute (X) 
*   (subject to their availability depending on the type of protection unit) 
*   operations on the memory block when the bus master is operating in 
*   privileged mode. PPU structs do not provide execute attributes.
* - Secure/Non-secure: Applies the secure/non-secure protection settings to
*   the defined memory region. Secure protection allows only bus masters that
*   access the memory with secure attribute. Non-secure protection allows
*   bus masters that have either secure or non-secure attributes.
* - PC match: This attribute allows the protection unit to either apply the
*   3 access evaluations (user/privileged, secure/non-secure, protection context)
*   or to only provide an address range match. This is useful when multiple
*   protection units protect an overlapping memory region and it's desirable
*   to only have access evaluations applied from only one of these protection
*   units. For example, SMPU1 protects memory A and SMPU2 protects memory B.
*   There exists a region where A and B intersect and this is accessed by a
*   bus master. Both SMPU1 and SMPU2 are configured to operate in "match" mode.
*   In this scenario, the access evaluation will only be applied by the higher
*   index protection unit (i.e. SMPU2) and the access attributes of SMPU1 will
*   be ignored. If the bus master then tries to access a memory region A (that
*   does not intersect with B), the access evaluation from SMPU1 will be used.
*   Note that the PC match functionality is only available in SMPUs.
* - PC mask: Defines the allowed protection context values that can access the
*   protected memory. The bus master attribute must be operating in one of the
*   protection context values allowed by the protection unit. E.g. If SMPU1 is
*   configured to allow only PC=1 and PC=5, a bus master (such as CM4) must
*   be operating at PC=1 or PC=5 when accessing the protected memory region.
* 
* \subsection group_prot_master_struct Master Struct
*
* The master struct protects its slave struct in the protection unit. This
* architecture makes possible for the slave configuration to be protected from
* reconfiguration by an unauthorized bus master. The configuration attributes
* and the format are similar to that of the slave structs.
*
* \subsubsection group_prot_master_addr Master Struct Address Definition
*
* - Address: The address definition for master struct is fixed to the slave
*   struct that it protects.
* - Region Size: The region size is fixed to 256B region.
* - Subregion: This value is fixed to only enable the first 64B subregions,
*   which applies the protection settings to the entire protection unit.
*
* \subsubsection group_prot_master_attr Master Struct Attribute Definition
*
* - User Permission: Only the write (W) access attribute is allowed for 
*   master structs, which controls whether a bus master operating in user
*   mode has the write access.
* - Privileged Permission: Only the write (W) access attribute is allowed for 
*   master structs, which controls whether a bus master operating in privileged
*   mode has the write access.
* - Secure/Non-Secure: Same behavior as slave struct.
* - PC match: Same behavior as slave struct.
* - PC mask: Same behavior as slave struct.
*
* \section group_prot_driver_usage Driver Usage
*
* Setting up and using protection units can be summed up in four stages:
*
* - Configure the bus master attributes. This defines the capabilities of
*   the bus master when trying to access the protected resources.
* - Configure the slave struct of a given protection unit. This defines
*   the protection attributes to be applied to the bus master accessing
*   the protected resource and also defines the size and location of the
*   memory block to protect.
* - Configure the master struct of the protection unit. This defines the
*   attributes to be checked against the bus master that is trying to
*   reconfigure the slave struct.
* - Set the active PC value of the bus master and place it in the correct
*   mode of operation (user/privileged, secure/non-secure). Then access
*   the protected memory.
*
* For example, by configuring the CM0+ bus master configuration to allow
* only protection contexts 2 and 3, the bus master will be able to 
* set its protection context only to 2 or 3. During runtime, the CM0+ core 
* can set its protection context to 2 by calling Cy_Prot_SetActivePC() 
* and access all regions of protected memory that allow PC=2. A fault will
* be triggered if a resource is protected with different protection settings.
*
* Note that each protection unit is distinguished by its type (e.g. 
* PROT_MPU_MPU_STRUCT_Type). The list of supported protection units can be
* obtained from the device definition header file. Choose a protection unit 
* of interest, and call its corresponding Cy_Prot_Config<X>Struct() function
* with its software protection unit configuration structure populated. Then
* enable the protection unit by calling the Cy_Prot_Enable<X>Struct() function.
*
* Note that the bus master ID (en_prot_master_t) is defined in the device
* config header file.
*
* \section group_prot_configuration Configuration Considerations
*
* When a resource (memory/register) is accessed, it must pass evaluation of
* all three protection unit categories in the following order: MPU->SMPU->PPU.
* The application should ensure that a denial-of-service attack cannot be
* made on the PPU by the SMPU. For this reason, it is recommended that the 
* application's security policy limit the ability for the non-secure client
* from configuring the SMPUs.
* 
* Within each category, the priority hierarchy must be carefully considered
* to ensure that a higher priority protection unit cannot be configured to
* override the security configuration of a lower index protection unit.
* Therefore if a lower index protection unit is configured, relevant higher
* priority indexes should be configured (or protected from unwanted 
* reconfiguration). E.g. If a PPU_SL is configured, PPU_RG and PPU_GR that
* overlaps with the protected registers should also be configured. SImilar 
* to SMPUs, it is recommended that the configuration of PPU_PROG be limited.
* Otherwise they can be used to override the protection settings of PPU_RG
* and PPU_SL structs.
*
* All bus masters are set to PC=0 value at device reset and therefore have full
* access to all resources. It is up to the security solution to implement
* what privileges each bus master has. Once transitioned to a PC!=0 value,
* only the CM0+ core is capable of re-entering the PC=0 via the user-defined
* exception entry in the CPUSS.CM0_PC0_HANDLER register.
*
* - SMPU 15 and 14 are configured and enabled to only allow PC=0 accesses at
*   device boot. 
* - PROG PPU 15, 14, 13 and 12 are configured to only allow PC=0 accesses at 
*   device boot.
* - GR PPU 0 and 2 are configured to only allow PC=0 accesses at device boot.
*
* \section group_prot_more_information More Information
*
* Refer to Technical Reference Manual (TRM) and the device datasheet.
*
* \section group_prot_MISRA MISRA-C Compliance]
* The Prot driver does not have any driver-specific deviations.
*
* \section group_prot_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.20</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td rowspan="2">1.10</td>
*     <td>Added input parameter validation to the API functions.<br>
*     cy_en_prot_pcmask_t, cy_en_prot_subreg_t and cy_en_prot_pc_t 
*     types are set to typedef enum</td>
*     <td>Improved debugging capability</td>
*   </tr>
*   <tr>
*     <td>Expanded documentation</td>
*     <td></td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_prot_macros Macros
* \defgroup group_prot_functions Functions
* \{
*   \defgroup group_prot_functions_busmaster    Bus Master and PC Functions
*   \defgroup group_prot_functions_mpu          MPU Functions
*   \defgroup group_prot_functions_smpu         SMPU Functions
*   \defgroup group_prot_functions_ppu_prog_v2  PPU Programmable (PROG) v2 Functions
*   \defgroup group_prot_functions_ppu_fixed_v2 PPU Fixed (FIXED) v2 Functions
*   \defgroup group_prot_functions_ppu_prog     PPU Programmable (PROG) Functions
*   \defgroup group_prot_functions_ppu_gr       PPU Group (GR) Functions
*   \defgroup group_prot_functions_ppu_sl       PPU Slave (SL) Functions
*   \defgroup group_prot_functions_ppu_rg       PPU Region (RG) Functions
* \}
* \defgroup group_prot_data_structures Data Structures
* \defgroup group_prot_enums Enumerated Types
*/

#if !defined(__CY_PROT_H__)
#define __CY_PROT_H__

#include <stdbool.h>
#include <stddef.h>
#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_prot_macros
* \{
*/

/** Driver major version */
#define CY_PROT_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_PROT_DRV_VERSION_MINOR       20

/** Prot driver ID */
#define CY_PROT_ID CY_PDL_DRV_ID(0x30u)

/** \} group_prot_macros */

/**
* \addtogroup group_prot_enums
* \{
*/

/**
* Prot Driver error codes
*/
typedef enum 
{
    CY_PROT_SUCCESS   = 0x00u,                                    /**< Returned successful */
    CY_PROT_BAD_PARAM = CY_PROT_ID | CY_PDL_STATUS_ERROR | 0x01u, /**< Bad parameter was passed */
    CY_PROT_FAILURE   = CY_PROT_ID | CY_PDL_STATUS_ERROR | 0x03u  /**< The resource is locked */
} cy_en_prot_status_t;

/**
* User/Privileged permission
*/
typedef enum 
{
    CY_PROT_PERM_DISABLED = 0x00u, /**< Read, Write and Execute disabled */
    CY_PROT_PERM_R        = 0x01u, /**< Read enabled */
    CY_PROT_PERM_W        = 0x02u, /**< Write enabled */
    CY_PROT_PERM_RW       = 0x03u, /**< Read and Write enabled */
    CY_PROT_PERM_X        = 0x04u, /**< Execute enabled */
    CY_PROT_PERM_RX       = 0x05u, /**< Read and Execute enabled */
    CY_PROT_PERM_WX       = 0x06u, /**< Write and Execute enabled */
    CY_PROT_PERM_RWX      = 0x07u  /**< Read, Write and Execute enabled */
}cy_en_prot_perm_t;

/**
* Memory region size
*/
typedef enum 
{
    CY_PROT_SIZE_4B    = 1u,  /**< 4 bytes */
    CY_PROT_SIZE_8B    = 2u,  /**< 8 bytes */
    CY_PROT_SIZE_16B   = 3u,  /**< 16 bytes */
    CY_PROT_SIZE_32B   = 4u,  /**< 32 bytes */
    CY_PROT_SIZE_64B   = 5u,  /**< 64 bytes */
    CY_PROT_SIZE_128B  = 6u,  /**< 128 bytes */

    CY_PROT_SIZE_256B  = 7u,  /**< 256 bytes */
    CY_PROT_SIZE_512B  = 8u,  /**< 512 bytes */
    CY_PROT_SIZE_1KB   = 9u,  /**< 1 Kilobyte */
    CY_PROT_SIZE_2KB   = 10u, /**< 2 Kilobytes */
    CY_PROT_SIZE_4KB   = 11u, /**< 4 Kilobytes */
    CY_PROT_SIZE_8KB   = 12u, /**< 8 Kilobytes */
    CY_PROT_SIZE_16KB  = 13u, /**< 16 Kilobytes */
    CY_PROT_SIZE_32KB  = 14u, /**< 32 Kilobytes */
    CY_PROT_SIZE_64KB  = 15u, /**< 64 Kilobytes */
    CY_PROT_SIZE_128KB = 16u, /**< 128 Kilobytes */
    CY_PROT_SIZE_256KB = 17u, /**< 256 Kilobytes */
    CY_PROT_SIZE_512KB = 18u, /**< 512 Kilobytes */
    CY_PROT_SIZE_1MB   = 19u, /**< 1 Megabyte */
    CY_PROT_SIZE_2MB   = 20u, /**< 2 Megabytes */
    CY_PROT_SIZE_4MB   = 21u, /**< 4 Megabytes */
    CY_PROT_SIZE_8MB   = 22u, /**< 8 Megabytes */
    CY_PROT_SIZE_16MB  = 23u, /**< 16 Megabytes */
    CY_PROT_SIZE_32MB  = 24u, /**< 32 Megabytes */
    CY_PROT_SIZE_64MB  = 25u, /**< 64 Megabytes */
    CY_PROT_SIZE_128MB = 26u, /**< 128 Megabytes */
    CY_PROT_SIZE_256MB = 27u, /**< 256 Megabytes */
    CY_PROT_SIZE_512MB = 28u, /**< 512 Megabytes */
    CY_PROT_SIZE_1GB   = 29u, /**< 1 Gigabyte */
    CY_PROT_SIZE_2GB   = 30u, /**< 2 Gigabytes */
    CY_PROT_SIZE_4GB   = 31u  /**< 4 Gigabytes */
}cy_en_prot_size_t;

/**
* Protection Context (PC)
*/
enum cy_en_prot_pc_t
{
    CY_PROT_PC1  = 1u,  /**< PC = 1 */
    CY_PROT_PC2  = 2u,  /**< PC = 2 */
    CY_PROT_PC3  = 3u,  /**< PC = 3 */
    CY_PROT_PC4  = 4u,  /**< PC = 4 */
    CY_PROT_PC5  = 5u,  /**< PC = 5 */
    CY_PROT_PC6  = 6u,  /**< PC = 6 */
    CY_PROT_PC7  = 7u,  /**< PC = 7 */
    CY_PROT_PC8  = 8u,  /**< PC = 8 */
    CY_PROT_PC9  = 9u,  /**< PC = 9 */
    CY_PROT_PC10 = 10u, /**< PC = 10 */
    CY_PROT_PC11 = 11u, /**< PC = 11 */
    CY_PROT_PC12 = 12u, /**< PC = 12 */
    CY_PROT_PC13 = 13u, /**< PC = 13 */
    CY_PROT_PC14 = 14u, /**< PC = 14 */
    CY_PROT_PC15 = 15u  /**< PC = 15 */
};

/**
* Subregion disable (0-7)
*/
enum cy_en_prot_subreg_t
{
    CY_PROT_SUBREGION_DIS0 = 0x01u,  /**< Disable subregion 0 */
    CY_PROT_SUBREGION_DIS1 = 0x02u,  /**< Disable subregion 1 */
    CY_PROT_SUBREGION_DIS2 = 0x04u,  /**< Disable subregion 2 */
    CY_PROT_SUBREGION_DIS3 = 0x08u,  /**< Disable subregion 3 */
    CY_PROT_SUBREGION_DIS4 = 0x10u,  /**< Disable subregion 4 */
    CY_PROT_SUBREGION_DIS5 = 0x20u,  /**< Disable subregion 5 */
    CY_PROT_SUBREGION_DIS6 = 0x40u,  /**< Disable subregion 6 */
    CY_PROT_SUBREGION_DIS7 = 0x80u   /**< Disable subregion 7 */
};

/**
* Protection context mask (PC_MASK) 
*/
enum cy_en_prot_pcmask_t
{
    CY_PROT_PCMASK1  = 0x0001u,  /**< Mask to allow PC = 1 */
    CY_PROT_PCMASK2  = 0x0002u,  /**< Mask to allow PC = 2 */
    CY_PROT_PCMASK3  = 0x0004u,  /**< Mask to allow PC = 3 */
    CY_PROT_PCMASK4  = 0x0008u,  /**< Mask to allow PC = 4 */
    CY_PROT_PCMASK5  = 0x0010u,  /**< Mask to allow PC = 5 */
    CY_PROT_PCMASK6  = 0x0020u,  /**< Mask to allow PC = 6 */
    CY_PROT_PCMASK7  = 0x0040u,  /**< Mask to allow PC = 7 */
    CY_PROT_PCMASK8  = 0x0080u,  /**< Mask to allow PC = 8 */
    CY_PROT_PCMASK9  = 0x0100u,  /**< Mask to allow PC = 9 */
    CY_PROT_PCMASK10 = 0x0200u,  /**< Mask to allow PC = 10 */
    CY_PROT_PCMASK11 = 0x0400u,  /**< Mask to allow PC = 11 */
    CY_PROT_PCMASK12 = 0x0800u,  /**< Mask to allow PC = 12 */
    CY_PROT_PCMASK13 = 0x1000u,  /**< Mask to allow PC = 13 */
    CY_PROT_PCMASK14 = 0x2000u,  /**< Mask to allow PC = 14 */
    CY_PROT_PCMASK15 = 0x4000u   /**< Mask to allow PC = 15 */
};

/** \} group_prot_enums */


/***************************************
*        Constants
***************************************/

/** \cond INTERNAL */

//#if (1U == CY_IP_MXPERI_VERSION)    
#if !defined(PERI_MS_PPU_PR_Type)
typedef PERI_MS_PPU_PR_V2_Type PERI_MS_PPU_PR_Type;
typedef PERI_MS_PPU_FX_V2_Type PERI_MS_PPU_FX_Type;
#endif /* defined(PERI_MS_PPU_PR_Type) */
//#else
#if !defined(PERI_PPU_PR_Type)
typedef PERI_PPU_PR_V1_Type PERI_PPU_PR_Type;
typedef PERI_PPU_GR_V1_Type PERI_PPU_GR_Type;
typedef PERI_GR_PPU_SL_V1_Type PERI_GR_PPU_SL_Type;
typedef PERI_GR_PPU_RG_V1_Type PERI_GR_PPU_RG_Type;
#endif /* defined(PERI_PPU_PR_Type) */
//#endif /* CY_PERI_V1 */

/* General Masks and shifts */
#define CY_PROT_MSX_CTL_SHIFT                   (0x02UL) /**< Shift for MSx_CTL register */
#define CY_PROT_STRUCT_ENABLE                   (0x01UL) /**< Enable protection unit struct */
#define CY_PROT_STRUCT_DISABLE                  (0x00UL) /**< Disable protection unit struct */
#define CY_PROT_ADDR_SHIFT                      (8UL)    /**< Address shift for MPU, SMPU and PROG PPU structs */
#define CY_PROT_PCMASK_CHECK                    (0x01UL) /**< Shift and mask for pcMask check */

/* Permission masks and shifts */
#define CY_PROT_ATT_PERMISSION_MASK             (0x07UL) /**< Protection Unit attribute permission mask */
#define CY_PROT_ATT_PRIV_PERMISSION_SHIFT       (0x03UL) /**< Protection Unit priviliged attribute permission shift */

#define CY_PROT_ATT_PERI_USER_PERM_Pos          (0UL)    /**< PERI v2 priviliged attribute permission shift */
#define CY_PROT_ATT_PERI_USER_PERM_Msk          (0x03UL) /**< PERI v2 attribute permission mask */
#define CY_PROT_ATT_PERI_PRIV_PERM_Pos          (2UL)    /**< PERI v2 priviliged attribute permission shift */
#define CY_PROT_ATT_PERI_PRIV_PERM_Msk          ((uint32_t)(0x03UL << CY_PROT_ATT_PERI_PRIV_PERM_Pos)) /**< PERI v2 attribute permission mask */

#define CY_PROT_ATT_PC_MAX                      (4U)     /**< Maximum PC value per ATT reg */

/* BWC macros */
#define CY_PROT_ATT_PERI_PERM_MASK              (0x03UL)
#define CY_PROT_ATT_PERI_PRIV_PERM_SHIFT        (0x02UL)
/* End of BWC macros */

#define CY_PROT_PC_MAX                          (8u) /* TODO replace this with the device struct member */
#define CY_PROT_SMPU_PC_LIMIT_MASK              (0xFFFFFFFFUL << CY_PROT_PC_MAX)
#define CY_PROT_PPU_PROG_PC_LIMIT_MASK          (0xFFFFFFFFUL << CY_PROT_PC_MAX)
#define CY_PROT_PPU_FIXED_PC_LIMIT_MASK         (0xFFFFFFFFUL << CY_PROT_PC_MAX)
    
#define CY_PROT_SMPU_ATT0_MASK                  ((uint32_t)~(PROT_SMPU_SMPU_STRUCT_ATT0_PC_MASK_0_Msk))
#define CY_PROT_SMPU_ATT1_MASK                  ((uint32_t)~(PROT_SMPU_SMPU_STRUCT_ATT1_UX_Msk \
                                                       | PROT_SMPU_SMPU_STRUCT_ATT1_PX_Msk \
                                                       | PROT_SMPU_SMPU_STRUCT_ATT1_PC_MASK_0_Msk \
                                                       | PROT_SMPU_SMPU_STRUCT_ATT1_REGION_SIZE_Msk \
                                                ))

#define CY_PROT_PPU_PROG_ATT0_MASK              ((uint32_t)~(PERI_PPU_PR_ATT0_UX_Msk \
                                                       | PERI_PPU_PR_ATT0_PX_Msk \
                                                       | PERI_PPU_PR_ATT0_PC_MASK_0_Msk \
                                                ))
#define CY_PROT_PPU_PROG_ATT1_MASK              ((uint32_t)~(PERI_PPU_PR_ATT1_UX_Msk \
                                                       | PERI_PPU_PR_ATT1_PX_Msk \
                                                       | PERI_PPU_PR_ATT1_PC_MASK_0_Msk \
                                                       | PERI_PPU_PR_ATT1_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_GR_ATT0_MASK                ((uint32_t)~(PERI_PPU_GR_ATT0_UX_Msk \
                                                       | PERI_PPU_GR_ATT0_PX_Msk \
                                                       | PERI_PPU_GR_ATT0_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT0_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_GR_ATT1_MASK                ((uint32_t)~(PERI_PPU_GR_ATT1_UX_Msk \
                                                       | PERI_PPU_GR_ATT1_PX_Msk \
                                                       | PERI_PPU_GR_ATT1_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT1_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_SL_ATT0_MASK                ((uint32_t)~(PERI_PPU_GR_ATT0_UX_Msk \
                                                       | PERI_PPU_GR_ATT0_PX_Msk \
                                                       | PERI_PPU_GR_ATT0_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT0_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_SL_ATT1_MASK                ((uint32_t)~(PERI_PPU_GR_ATT1_UX_Msk \
                                                       | PERI_PPU_GR_ATT1_PX_Msk \
                                                       | PERI_PPU_GR_ATT1_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT1_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_RG_ATT0_MASK                ((uint32_t)~(PERI_PPU_GR_ATT0_UX_Msk \
                                                       | PERI_PPU_GR_ATT0_PX_Msk \
                                                       | PERI_PPU_GR_ATT0_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT0_REGION_SIZE_Msk \
                                                ))
#define CY_PROT_PPU_RG_ATT1_MASK                ((uint32_t)~(PERI_PPU_GR_ATT1_UX_Msk \
                                                       | PERI_PPU_GR_ATT1_PX_Msk \
                                                       | PERI_PPU_GR_ATT1_PC_MASK_0_Msk \
                                                       | PERI_PPU_GR_ATT1_REGION_SIZE_Msk \
                                                ))

/* Parameter check macros */
#define CY_PROT_BUS_MASTER_FIELD                (0x0000FFFFUL) /* TODO Replace this with bitfield value from device config struct */
#define CY_PROT_IS_BUS_MASTER_VALID(busMaster)  ((CY_PROT_BUS_MASTER_FIELD && (1UL << (uint32_t)(busMaster))) != 0UL)

#define CY_PROT_IS_MPU_PERM_VALID(permission)   (((permission) == CY_PROT_PERM_DISABLED) || \
                                                 ((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_W) || \
                                                 ((permission) == CY_PROT_PERM_RW) || \
                                                 ((permission) == CY_PROT_PERM_X) || \
                                                 ((permission) == CY_PROT_PERM_RX) || \
                                                 ((permission) == CY_PROT_PERM_WX) || \
                                                 ((permission) == CY_PROT_PERM_RWX))

#define CY_PROT_IS_SMPU_MS_PERM_VALID(permission) (((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_RW))

#define CY_PROT_IS_SMPU_SL_PERM_VALID(permission) (((permission) == CY_PROT_PERM_DISABLED) || \
                                                 ((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_W) || \
                                                 ((permission) == CY_PROT_PERM_RW) || \
                                                 ((permission) == CY_PROT_PERM_X) || \
                                                 ((permission) == CY_PROT_PERM_RX) || \
                                                 ((permission) == CY_PROT_PERM_WX) || \
                                                 ((permission) == CY_PROT_PERM_RWX))

#define CY_PROT_IS_PROG_MS_PERM_VALID(permission) (((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_RW))

#define CY_PROT_IS_PROG_SL_PERM_VALID(permission) (((permission) == CY_PROT_PERM_DISABLED) || \
                                                 ((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_W) || \
                                                 ((permission) == CY_PROT_PERM_RW))

#define CY_PROT_IS_FIXED_MS_PERM_VALID(permission) (((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_RW))

#define CY_PROT_IS_FIXED_SL_PERM_VALID(permission) (((permission) == CY_PROT_PERM_DISABLED) || \
                                                 ((permission) == CY_PROT_PERM_R) || \
                                                 ((permission) == CY_PROT_PERM_W) || \
                                                 ((permission) == CY_PROT_PERM_RW))

#define CY_PROT_IS_REGION_SIZE_VALID(regionSize) (((regionSize) == CY_PROT_SIZE_256B) || \
                                                  ((regionSize) == CY_PROT_SIZE_512B) || \
                                                  ((regionSize) == CY_PROT_SIZE_1KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_8KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_16KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_32KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_64KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_128KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_256KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_512KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_1MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_8MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_16MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_32MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_64MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_128MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_256MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_512MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_1GB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2GB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4GB))

#define CY_PROT_IS_PPU_V2_SIZE_VALID(regionSize)  (((regionSize) == CY_PROT_SIZE_4B) || \
                                                  ((regionSize) == CY_PROT_SIZE_8B) || \
                                                  ((regionSize) == CY_PROT_SIZE_16B) || \
                                                  ((regionSize) == CY_PROT_SIZE_32B) || \
                                                  ((regionSize) == CY_PROT_SIZE_64B) || \
                                                  ((regionSize) == CY_PROT_SIZE_128B) || \
                                                  ((regionSize) == CY_PROT_SIZE_256B) || \
                                                  ((regionSize) == CY_PROT_SIZE_512B) || \
                                                  ((regionSize) == CY_PROT_SIZE_1KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_8KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_16KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_32KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_64KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_128KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_256KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_512KB) || \
                                                  ((regionSize) == CY_PROT_SIZE_1MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_8MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_16MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_32MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_64MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_128MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_256MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_512MB) || \
                                                  ((regionSize) == CY_PROT_SIZE_1GB) || \
                                                  ((regionSize) == CY_PROT_SIZE_2GB) || \
                                                  ((regionSize) == CY_PROT_SIZE_4GB))
/** \endcond */


/***************************************
*       Configuration Structures
***************************************/

/**
* \addtogroup group_prot_data_structures
* \{
*/

/** Configuration structure for MPU Struct initialization */
typedef struct 
{
    uint32_t*         address;          /**< Base address of the memory region */
    cy_en_prot_size_t regionSize;       /**< Size of the memory region */
    uint8_t           subregions;       /**< Mask of the 8 subregions to disable */
    cy_en_prot_perm_t userPermission;   /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;   /**< Privileged permissions for the region */
    bool              secure;           /**< Non Secure = 0, Secure = 1 */
} cy_stc_mpu_cfg_t;

/** Configuration structure for SMPU struct initialization */
typedef struct 
{
    uint32_t*         address;          /**< Base address of the memory region (Only applicable to slave) */
    cy_en_prot_size_t regionSize;       /**< Size of the memory region (Only applicable to slave) */
    uint8_t           subregions;       /**< Mask of the 8 subregions to disable (Only applicable to slave) */
    cy_en_prot_perm_t userPermission;   /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;   /**< Privileged permissions for the region */
    bool              secure;           /**< Non Secure = 0, Secure = 1 */
    bool              pcMatch;          /**< Access evaluation = 0, Matching = 1  */
    uint16_t          pcMask;           /**< Mask of allowed protection context(s) */
} cy_stc_smpu_cfg_t;

/** Configuration structure for Programmable (PROG) PPU (PPU_PR) struct initialization */
typedef struct 
{
    uint32_t*         address;          /**< Base address of the memory region (Only applicable to slave) */
    cy_en_prot_size_t regionSize;       /**< Size of the memory region (Only applicable to slave) */
    uint8_t           subregions;       /**< Mask of the 8 subregions to disable (Only applicable to slave) */
    cy_en_prot_perm_t userPermission;   /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;   /**< Privileged permissions for the region */
    bool              secure;           /**< Non Secure = 0, Secure = 1 */
    bool              pcMatch;          /**< Access evaluation = 0, Matching = 1  */
    uint16_t          pcMask;           /**< Mask of allowed protection context(s) */
} cy_stc_ppu_prog_cfg_t;

/** Configuration structure for Fixed Group (GR) PPU (PPU_GR) struct initialization */
typedef struct 
{
    cy_en_prot_perm_t userPermission;   /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;   /**< Privileged permissions for the region */
    bool              secure;           /**< Non Secure = 0, Secure = 1 */
    bool              pcMatch;          /**< Access evaluation = 0, Matching = 1  */
    uint16_t          pcMask;           /**< Mask of allowed protection context(s) */
} cy_stc_ppu_gr_cfg_t;

/** Configuration structure for Fixed Slave (SL) PPU (PPU_SL) struct initialization */
typedef struct 
{
    cy_en_prot_perm_t userPermission;   /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;   /**< Privileged permissions for the region */
    bool              secure;           /**< Non Secure = 0, Secure = 1 */
    bool              pcMatch;          /**< Access evaluation = 0, Matching = 1  */
    uint16_t          pcMask;           /**< Mask of allowed protection context(s) */
} cy_stc_ppu_sl_cfg_t;

/** Configuration structure for Fixed Region (RG) PPU (PPU_RG) struct initialization */
typedef struct 
{
    cy_en_prot_perm_t userPermission;  /**< User permissions for the region */
    cy_en_prot_perm_t privPermission;  /**< Privileged permissions for the region */
    bool             secure;           /**< Non Secure = 0, Secure = 1 */
    bool             pcMatch;          /**< Access evaluation = 0, Matching = 1  */
    uint16_t         pcMask;           /**< Mask of allowed protection context(s) */
} cy_stc_ppu_rg_cfg_t;

/** \} group_prot_data_structures */


/***************************************
*        Function Prototypes
***************************************/

/**
* \addtogroup group_prot_functions
* \{
*/

/**
* \addtogroup group_prot_functions_busmaster
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigBusMaster(en_prot_master_t busMaster, bool privileged, bool secure, uint32_t pcMask);
cy_en_prot_status_t Cy_Prot_SetActivePC(en_prot_master_t busMaster, uint32_t pc);
uint32_t Cy_Prot_GetActivePC(en_prot_master_t busMaster);
/** \} group_prot_functions_busmaster */

/**
* \addtogroup group_prot_functions_mpu
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigMpuStruct(PROT_MPU_MPU_STRUCT_Type* base, const cy_stc_mpu_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnableMpuStruct(PROT_MPU_MPU_STRUCT_Type* base);
cy_en_prot_status_t Cy_Prot_DisableMpuStruct(PROT_MPU_MPU_STRUCT_Type* base);
/** \} group_prot_functions_mpu */

/**
* \addtogroup group_prot_functions_smpu
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigSmpuMasterStruct(PROT_SMPU_SMPU_STRUCT_Type* base, const cy_stc_smpu_cfg_t* config);
cy_en_prot_status_t Cy_Prot_ConfigSmpuSlaveStruct(PROT_SMPU_SMPU_STRUCT_Type* base, const cy_stc_smpu_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnableSmpuMasterStruct(PROT_SMPU_SMPU_STRUCT_Type* base);
cy_en_prot_status_t Cy_Prot_DisableSmpuMasterStruct(PROT_SMPU_SMPU_STRUCT_Type* base);
cy_en_prot_status_t Cy_Prot_EnableSmpuSlaveStruct(PROT_SMPU_SMPU_STRUCT_Type* base);
cy_en_prot_status_t Cy_Prot_DisableSmpuSlaveStruct(PROT_SMPU_SMPU_STRUCT_Type* base);
/** \} group_prot_functions_smpu */



/**
* \addtogroup group_prot_functions_ppu_prog_v2
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuProgMasterAtt(PERI_MS_PPU_PR_Type* base, uint16_t pcMask, cy_en_prot_perm_t userPermission, cy_en_prot_perm_t privPermission, bool secure);
cy_en_prot_status_t Cy_Prot_ConfigPpuProgSlaveCore(PERI_MS_PPU_PR_Type* base, uint32_t address, cy_en_prot_size_t regionSize);
cy_en_prot_status_t Cy_Prot_ConfigPpuProgSlaveAtt(PERI_MS_PPU_PR_Type* base, uint16_t pcMask, cy_en_prot_perm_t userPermission, cy_en_prot_perm_t privPermission, bool secure);
cy_en_prot_status_t Cy_Prot_EnablePpuProgSlaveRegion(PERI_MS_PPU_PR_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuProgSlaveRegion(PERI_MS_PPU_PR_Type* base);
/** \} group_prot_functions_ppu_prog_v2 */

/**
* \addtogroup group_prot_functions_ppu_fixed_v2
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedMasterAtt(PERI_MS_PPU_FX_Type* base, uint16_t pcMask, cy_en_prot_perm_t userPermission, cy_en_prot_perm_t privPermission, bool secure);
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedSlaveAtt(PERI_MS_PPU_FX_Type* base, uint16_t pcMask, cy_en_prot_perm_t userPermission, cy_en_prot_perm_t privPermission, bool secure);
/** \} group_prot_functions_ppu_fixed_v2 */


/**
* \addtogroup group_prot_functions_ppu_prog
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuProgMasterStruct(PERI_PPU_PR_Type* base, const cy_stc_ppu_prog_cfg_t* config);
cy_en_prot_status_t Cy_Prot_ConfigPpuProgSlaveStruct(PERI_PPU_PR_Type* base, const cy_stc_ppu_prog_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnablePpuProgMasterStruct(PERI_PPU_PR_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuProgMasterStruct(PERI_PPU_PR_Type* base);
cy_en_prot_status_t Cy_Prot_EnablePpuProgSlaveStruct(PERI_PPU_PR_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuProgSlaveStruct(PERI_PPU_PR_Type* base);
/** \} group_prot_functions_ppu_prog */

/**
* \addtogroup group_prot_functions_ppu_gr
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedGrMasterStruct(PERI_PPU_GR_Type* base, const cy_stc_ppu_gr_cfg_t* config);
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedGrSlaveStruct(PERI_PPU_GR_Type* base, const cy_stc_ppu_gr_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedGrMasterStruct(PERI_PPU_GR_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedGrMasterStruct(PERI_PPU_GR_Type* base);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedGrSlaveStruct(PERI_PPU_GR_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedGrSlaveStruct(PERI_PPU_GR_Type* base);
/** \} group_prot_functions_ppu_gr */

/**
* \addtogroup group_prot_functions_ppu_sl
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedSlMasterStruct(PERI_GR_PPU_SL_Type* base, const cy_stc_ppu_sl_cfg_t* config);
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedSlSlaveStruct(PERI_GR_PPU_SL_Type* base, const cy_stc_ppu_sl_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedSlMasterStruct(PERI_GR_PPU_SL_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedSlMasterStruct(PERI_GR_PPU_SL_Type* base);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedSlSlaveStruct(PERI_GR_PPU_SL_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedSlSlaveStruct(PERI_GR_PPU_SL_Type* base);
/** \} group_prot_functions_ppu_sl */

/**
* \addtogroup group_prot_functions_ppu_rg
* \{
*/
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedRgMasterStruct(PERI_GR_PPU_RG_Type* base, const cy_stc_ppu_rg_cfg_t* config);
cy_en_prot_status_t Cy_Prot_ConfigPpuFixedRgSlaveStruct(PERI_GR_PPU_RG_Type* base, const cy_stc_ppu_rg_cfg_t* config);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedRgMasterStruct(PERI_GR_PPU_RG_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedRgMasterStruct(PERI_GR_PPU_RG_Type* base);
cy_en_prot_status_t Cy_Prot_EnablePpuFixedRgSlaveStruct(PERI_GR_PPU_RG_Type* base);
cy_en_prot_status_t Cy_Prot_DisablePpuFixedRgSlaveStruct(PERI_GR_PPU_RG_Type* base);
/** \} group_prot_functions_ppu_rg */

/** \} group_prot_functions */

/** \} group_prot */

#if defined(__cplusplus)
}
#endif

#endif /* CY_PROT_H */
