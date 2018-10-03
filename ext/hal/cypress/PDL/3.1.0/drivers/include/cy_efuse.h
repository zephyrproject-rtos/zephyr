/***************************************************************************//**
* \file cy_efuse.h
* \version 1.10
*
* Provides the API declarations of the eFuse driver.
*
********************************************************************************
* \copyright
* Copyright 2017-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#if !defined(CY_EFUSE_H)
#define CY_EFUSE_H

/**
* \defgroup group_efuse Electronic Fuses (eFuse)
* \{
* 
* Electronic Fuses (eFuses) - non-volatile memory whose
* each bit is one-time programmable (OTP). One eFuse macro consists of
* 256 bits (32 * 8). The PSoC devices have up to 16 eFuse macros; consult the
* device-specific datasheet to determine how many macros for a particular device.
* These are implemented as a regular Advanced High-performance Bus (AHB)
* peripheral with the following characteristics:
* - eFuses are used to control the device life-cycle stage (NORMAL, SECURE,
* and SECURE_WITH_DEBUG) and the protection settings;
* - eFuse memory can be programmed (eFuse bit value changed from '0' to '1')
* only once; if an eFuse bit is blown, it cannot be cleared again; 
* - programming fuses requires the associated I/O supply to be at a specific
* level: the VDDIO0 (or VDDIO if only one VDDIO is present in the package)
* supply of the device should be set to 2.5 V (&plusmn;5%);
* - fuses are programmed via the PSoC Programmer tool that parses the hex file
* and extracts the necessary information; the fuse data must be located at the
* dedicated section in the hex file. For more details see
* [PSoC 6 Programming Specifications](http://www.cypress.com/documentation/programming-specifications/psoc-6-programming-specifications)
*
* \section group_efuse_configuration Configuration Considerations
*
* Efuse memory can have different organization depending on the selected device.
* Consult the device TRM to determine the efuse memory organization and
* registers bitmap on the selected device.
*
* To read fuse data use the driver [functions] (\ref group_efuse_functions).
*
* To blow fuses, define a data structure of \ref cy_stc_efuse_data_t type in the
* firmware. The structure must be placed in the special memory section, for
* this use a compiler attribute.
* Each byte in the structure corresponds to the one fuse bit in the
* device. It allows the PSoC Programmer tool to distinguish bytes that are
* being set from bytes we don't care about or with unknown values. Fill the
* structure with the following values:
* - 0x00 - Not blown;
* - 0x01 - Blown;
* - 0xFF - Ignore.
*
* After the structure is defined and the values are set, build the project and
* download the firmware. To blow fuses, the firmware must be downloaded by the
* PSoC Programmer tool. Before you download firmware, ensure that the
* conditions from the PSoC 6 Programming Specification are met.
*
* The code below shows an example of the efuse data structure
* definition to blow SECURE bit of the life-cycle stage register. 
* The bits to blow are set to the EFUSE_STATE_SET value.
* \snippet eFuse_v1_0_sut_00.cydsn/main_cm0p.c SNIPPET_EFUSE_DATA_STC
*
* \section group_efuse_more_information More Information
*
* Refer to the technical reference manual (TRM) and the device datasheet.
*
* \section group_efuse_MISRA MISRA-C Compliance
*
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th>Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>2.3</td>
*     <td>R</td>
*     <td>The character sequence // shall not be used within a comment.</td>
*     <td>The comments provide a useful WEB link to the documentation.</td>
*   </tr>
*   <tr>
*     <td>11.5</td>
*     <td>R</td>
*     <td>Dangerous pointer cast results in loss of volatile qualification.</td>
*     <td>The removal of the volatile qualification inside the function has no
*         side effects.</td>
*   </tr>
* </table>
*
* \section group_efuse_changelog Changelog
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.10</td>
*     <td>Flattened the driver source code organization into the single
*         source directory and the single include directory
*     </td>
*     <td>Simplified the driver library directory structure</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_efuse_macros Macros
* \defgroup group_efuse_functions Functions
* \defgroup group_efuse_data_structures Data Structures
* \defgroup group_efuse_enumerated_types Enumerated Types
*/

#include "cy_device_headers.h"
#include "cy_syslib.h"

/***************************************
* Macro Definitions
***************************************/
/**
* \addtogroup group_efuse_macros
* \{
*/

/** The driver major version */
#define CY_EFUSE_DRV_VERSION_MAJOR          1
/** The driver minor version */
#define CY_EFUSE_DRV_VERSION_MINOR          10
/** The eFuse driver identifier */
#define CY_EFUSE_ID                         (CY_PDL_DRV_ID(0x1AUL))
/** The number of bits in the byte */
#define CY_EFUSE_BITS_PER_BYTE              (8UL)
/** \} group_efuse_macros */

/***************************************
* Enumerated Types
***************************************/
/**
* \addtogroup group_efuse_enumerated_types
* \{
*/
/** This enum has the return values of the eFuse driver */
typedef enum 
{
    CY_EFUSE_SUCCESS               = 0x00UL,  /**< Success */
    CY_EFUSE_INVALID_PROTECTION    = CY_EFUSE_ID | CY_PDL_STATUS_ERROR | 0x01UL, /**< Invalid access in the current protection state */
    CY_EFUSE_INVALID_FUSE_ADDR     = CY_EFUSE_ID | CY_PDL_STATUS_ERROR | 0x02UL, /**< Invalid eFuse address */
    CY_EFUSE_BAD_PARAM             = CY_EFUSE_ID | CY_PDL_STATUS_ERROR | 0x03UL, /**< One or more invalid parameters */
    CY_EFUSE_IPC_BUSY              = CY_EFUSE_ID | CY_PDL_STATUS_ERROR | 0x04UL, /**< The IPC structure is already locked by another process */
    CY_EFUSE_ERR_UNC               = CY_EFUSE_ID | CY_PDL_STATUS_ERROR | 0xFFUL  /**< Unknown error code. See Cy_EFUSE_GetExternalStatus() */
} cy_en_efuse_status_t;

/** \} group_efuse_data_structure */

#if defined(__cplusplus)
extern "C" {
#endif
/***************************************
* Function Prototypes
***************************************/

/**
* \addtogroup group_efuse_functions
* \{
*/
cy_en_efuse_status_t Cy_EFUSE_GetEfuseBit(uint32_t bitNum, bool *bitVal);
cy_en_efuse_status_t Cy_EFUSE_GetEfuseByte(uint32_t offset, uint8_t *byteVal);
uint32_t Cy_EFUSE_GetExternalStatus(void);
/** \} group_efuse_functions */

#if defined(__cplusplus)
}
#endif


#endif /* #if !defined(CY_EFUSE_H) */

/** \} group_efuse */


/* [] END OF FILE */
