/***************************************************************************//**
* \file cy_ipc_drv.h
* \version 1.30
*
* Provides an API declaration of the IPC driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef CY_IPC_DRV_H
#define CY_IPC_DRV_H


/**
* \defgroup group_ipc Inter Process Communication (IPC)
* \{
* The inter-processor communication (IPC) driver provides a safe and reliable
* method to transfer data between CPUs. Hardware locking ensures that only one
* device can acquire and transfer data at a time so no data is lost or
* overwritten by asynchronous processes or CPUs.
*
* There are three parts to the API:
*     - Driver-level (DRV) API - used internally by Semaphore and Pipe levels
*     - Pipe-level (PIPE) API - establishes a communication channel between
*       processors
*     - Semaphore-level (SEMA) API - enables users to set and clear flags to
*       synchronize operations.
*
* Firmware does not need to use the DRV API. It can implement IPC functionality
* entirely with the PIPE and SEMA APIs.
*
* \section group_ipc_background Background
*
* IPC is implemented in hardware as a collection of individual communication
* channels, each with a set of 32-bit registers. The IPC design implements a set
* of interrupts that enable each processor to notify the other that data is
* available, or has been processed. There is also a locking mechanism that
* allows only one CPU to gain access at a time.
*
* The Driver-level API manages each channel's registers to implement IPC
* functionality. For information on the IPC registers, see the IPC chapter of
* the Technical Reference Manual (TRM).
*
* At the hardware level, communication is a five-step process.
*     -#  The sending processor acquires a channel
*     -#  It puts data into the channel
*     -#  The sender generates a notify event (interrupt)
*     -#  The receiving processor identifies the sender and retrieves the data
*     -#  The receiving processor generates a release event (interrupt)
*
* \image html ipc_driver.png
*
* These transactions are handled transparently by the DRV-level API. Use the
* PIPE and SEMA layers of the API to implement communication in your application.
* The data transferred is limited to a single 32-bit value. As implemented by
* the PIPE API, that value is a pointer to a data structure of arbitrary size
* and complexity.
*
* \section group_ipc_overview Overview
*
* The Pipe is the key element in the PDL design. A pipe is typically a
* full-duplex communication channel between CPU cores. A pipe allows a single
* conduit to transfer messages or data to and from multiple processes or CPUs.
*
* A pipe has two endpoints, one on each core. Each endpoint contains a dedicated
* IPC channel and an interrupt. IPC channels 0-7 and IPC interrupts 0-7 are
* reserved for system use.
*
* The pipe also contains the number of clients it supports, and for each client
* a callback function. So the pipe can service a number of clients, each with a
* separate callback function, on either endpoint. The number of clients a pipe
* supports is the sum of each endpoint's clients.
*
* This design enables any number of processes on the sending core to put
* arbitrary data into a single pipe. The first element of that data is the
* client ID of the client that should handle the data.
*
* An interrupt notifies the receiving core that data is available. The receiving
* core parses the data to identify the client, and then dispatches the event to
* the appropriate client via the client callback function. An interrupt notifies
* the sending core that the receiver is finished. In this way a single pipe can
* manage arbitrary data transfers between cores with data flowing in either
* direction.
*
* \image html ipc_ints.png
*
* The application can use semaphores to control access to shared resources, as
* required by the application's logic.
*
* The PDL provides two specific files that set up default IPC functionality.
* They are cy_ipc_config.h and cy_ipc_config.c. You can modify these files based
* on the requirements of your design. If you use PSoC Creator as a development
* environment, it will not overwrite your changes when you generate the
* application or build your code.
*
* \section group_ipc_pipe_layer PIPE layer
*
* A pipe is a communication channel between two endpoints. PSoC 6 devices support
* 16 IPC channels, and 16 IPC interrupts, each numbered 0-15. IPC Channels 0-7
* and IPC interrupts 0-7 are reserved for system use. Channels 8-15 and
* interrupts 8-15 are available for application use.
*
* A full duplex pipe uses two IPC channels, one per endpoint. Each endpoint
* specifies all the information required to process a message (either sent or
* received). Each endpoint is configured to use an IPC channel, and an IPC
* interrupt. Common practice is to use the interrupt with the same number as
* the IPC channel. However, IPC Interrupts are not directly associated with the
* IPC channels, so any channel can use any interrupt.  Any IPC channel can
* trigger 0, 1 or all the IPC interrupts at once, depending on the Notify or
* Release masks used.
*
* It is also possible to set up a one-directional pipe, using a single IPC
* channel. In this design one processor is always the sender, and the other is
* always the receiver. However, there are still two endpoints.
*
* A pipe supports an arbitrary number of clients with an array of callback
* functions, one per client. The client ID is the index number into the array
* for the client. After a pipe is configured and initialized, the application
* calls Cy_IPC_Pipe_RegisterCallback() once per client to register each client's
* callback function. Multiple clients can use the same callback function. The
* endpoints in a pipe share the callback array.
*
* Use Cy_IPC_Pipe_SendMessage() to send data. You specify both the "to" and
* "from" endpoints, and a callback function to be used when the data transfer is
* complete. The data is a 32-bit void pointer. The data pointed to is arbitrary,
* and can be an array, a structure, or a location in memory. The only limitation
* is that the first element of the data must be a 32-bit unsigned word containing
* a client ID number. The ID number is the index into the callback array.
*
* When a message is sent, the receiving endpoint's interrupt handler is called.
* The ISR can perform any task required by the design. However, as part of its
* function it calls \ref Cy_IPC_Pipe_ExecCallback. This function retrieves the
* client ID from the data and calls the associated callback function.
* The user-supplied callback function handles the data in whatever way is
* appropriate based on the application logic.
*
* After the callback function is returned by the receiver, it invokes the release
* callback function defined by the sender of the message.
*
* \section group_ipc_sema_layer SEMA Layer
*
* A semaphore is a flag the application uses to control access to a shared
* resource. The SEMA-level API uses an IPC channel to implement
* semaphores. Startup code sets up a default semaphore system. The
* default system creates an array of 128 semaphores (four 32-bit values).
* Semaphores 0-15 are reserved for system use. See
* Configuration Considerations - SEMA.
*
* Functions are available to initialize the semaphore system, to set or
* clear a semaphore, or to get the semaphore's current status. Application
* logic uses SEMA functions to relate a particular semaphore to a particular
* shared resource, and set, clear, or check the flag when accessing the
* shared resource.
*
* \section group_ipc_configuration_cypipe Configuration Considerations - CYPIPE
*
* There are none. The cy_ipc_config files set up the required CYPIPE for system
* use. Do not modify the CYPIPE. It uses IPC channels 5 and 6 to implement full
* duplex communication between cores. On the CM0+ the notify interrupt is
* assigned to NVIC IRQn 27. See System Interrupt (SysInt) for background.
*
* To create your own pipe (<b>USRPIPE</b>) you should edit cy_ipc_config files
* and take 4 steps:
*   -# Define a pipe callbacks processing interrupt handler
*      (similar to <b>Cy_IPC_SystemPipeIsr</b>)
*   -# Define a callbacks array (similar to <b>cy_ipc_pipe_sysCbArray</b>)
*   -# Define your pipe configuration with a cy_stc_ipc_pipe_config_t type structure
*      (similar to <b>systemPipeConfig</b> and <b>CY_CYPIPE_DEFAULT_CONFIG</b>)
*   -# Call Cy_IPC_Pipe_Init() from each core to initialize your pipe (similar
*      to call in the <b>Cy_IPC_SystemPipeInit</b>)
*
* \section group_ipc_configuration_sema Configuration Considerations - SEMA
*
* Startup code calls Cy_IPC_SystemSemaInit() (in cy_ipc_config.c) to set up
* semaphore functionality. This function calls the PDL init function
* Cy_IPC_Sema_Init() with default values. By default the semaphore system
* uses IPC channel 4, and creates 128 semaphores. Do <b>not</b> change the IPC
* channel. You can change the number of semaphores.
*
* To change the number of semaphores, modify this line of code in cy_ipc_config.h.
*
* \code
* #define CY_IPC_SEMA_COUNT               (uint32_t)(128u)
* \endcode
*
* The file cy_ipc_config.c declares array ipcSemaArray to hold the semaphore
* flags based on the size defined for this symbol. Use increments of 32. You
* must have at least 32 semaphores. Semaphores 0-15 are reserved for
* system use. Your application can use semaphores greater than 15.
*
* \section group_ipc_more_information More Information
*
* Cy_IPC_SystemSemaInit() and Cy_IPC_SystemPipeInit() functions are called in the
* SystemInit function. If the default startup file is not used, or SystemInit is
* not called in your project, call the following three functions prior to
* executing any flash or EmEEPROM write or erase operation. For example:
*  -# Cy_IPC_SystemSemaInit()
*  -# Cy_IPC_SystemPipeInit()
*  -# Cy_Flash_Init()
*
* Also Cy_IPC_SystemPipeInit function is called to support BLE host/controller
* communication.
*
* See the technical reference manual(TRM) for more information on the IPC.
*
* \section group_ipc_MISRA MISRA-C Compliance
*
* <table class="doxtable">
*   <tr>
*     <th>MISRA Rule</th>
*     <th>Rule Class (Required/Advisory)</th>
*     <th>Rule Description</th>
*     <th style="width: 50%;">Description of Deviation(s)</th>
*   </tr>
*   <tr>
*     <td>10.3</td>
*     <td>R</td>
*     <td>The value of a complex expression of integer type shall be cast
*         only to a type of the same signedness that is no wider than the underlying
*         type of the expression.</td>
*     <td>The cast from integer to enumeration value is used to calculate
*         the interrupt vector source from the integer number of the IPC interrupt
*         structure, so there is no way to avoid this cast.</td>
*   </tr>
*   <tr>
*     <td>11.4</td>
*     <td>A</td>
*     <td>A cast should not be performed between a pointer to the void to a
*         pointer to the object type.</td>
*     <td>The cast from the void to pointer and vice versa is used to transmit
*         data via the \ref group_ipc channel by exchanging the pointer. We
*         exchange only one pointer, so there is no way to avoid this cast.</td>
*   </tr>
* </table>
*
* \section group_ipc_changelog Changelog
*
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td>1.20</td>
*     <td>Added \ref Cy_IPC_Pipe_ExecuteCallback function.
*         Updated documentation about user pipe initialization.
*     </td>
*     <td>Interface improvement, documentation update</td>
*   </tr>
*   <tr>
*     <td>1.10.1</td>
*     <td>Updated description of the \ref Cy_IPC_Pipe_Init,
*         \ref Cy_IPC_Pipe_EndpointInit, \ref Cy_IPC_Sema_Set functions.
*         Added / updated code snippets.
*     </td>
*     <td>Documentation update and clarification</td>
*   </tr>
*   <tr>
*     <td>1.10</td>
*     <td>Added support for more IPC structures</td>
*     <td>New device support</td>
*   </tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
* \defgroup group_ipc_drv IPC driver layer (IPC_DRV)
* \{
*   The functions of this layer are used in the higher IPC levels
*   (Semaphores and Pipes).
*   Users should not call any of these IPC functions directly.
*
*   \defgroup group_ipc_macros Macros
*       Macro definitions are used in the driver
*
*   \defgroup group_ipc_functions Functions
*       Functions are used in the driver
*
*   \defgroup group_ipc_data_structures Data Structures
*       Data structures are used in the driver
*
*   \defgroup group_ipc_enums Enumerated Types
*       Enumerations are used in the driver
* \}
*
* \defgroup group_ipc_sema IPC semaphores layer (IPC_SEMA)
* \defgroup group_ipc_pipe IPC pipes layer (IPC_PIPE)
*
*/

/******************************************************************************/
/* Include files                                                              */
/******************************************************************************/

#include "cy_device.h"
#include "cy_device_headers.h"

#include "cy_ipc_config.h"
#include "cy_syslib.h"
#include <stddef.h>


/**
* \addtogroup group_ipc_macros
* \{
*/

/** Driver major version */
#define CY_IPC_DRV_VERSION_MAJOR       1

/** Driver minor version */
#define CY_IPC_DRV_VERSION_MINOR       30

/** Defines a value to indicate that no notification events are needed */
#define CY_IPC_NO_NOTIFICATION         (uint32_t)(0x00000000ul)

/* Error Code constants */
#define CY_IPC_ID CY_PDL_DRV_ID(0x22u)  /**< Software PDL driver ID for IPC */

/** Return prefix for IPC driver function status codes */
#define CY_IPC_ID_INFO    (uint32_t)( CY_IPC_ID | CY_PDL_STATUS_INFO )
/** Return prefix for IPC driver function warning return values */
#define CY_IPC_ID_WARNING (uint32_t)( CY_IPC_ID | CY_PDL_STATUS_WARNING)
/** Return prefix for IPC driver function error return values */
#define CY_IPC_ID_ERROR   (uint32_t)( CY_IPC_ID | CY_PDL_STATUS_ERROR)

/** Converts the IPC interrupt channel number to interrupt vector */
#define CY_IPC_INTR_NUM_TO_VECT(x)         ((int32_t)cpuss_interrupts_ipc_0_IRQn + (x))

/** \} group_ipc_macros */

/* end of definition in device.h */


/**
* \addtogroup group_ipc_enums
* \{
*/

/**
* This is a list of ENUMs used for function return status.
*/
typedef enum
{
    /** Function was successfully executed */
    CY_IPC_DRV_SUCCESS      = (0x00u),
    /** Function was not executed due to an error.
        Typical conditions for the error explained
        in the function description */
    CY_IPC_DRV_ERROR        = ( CY_IPC_ID_ERROR + 1ul),
} cy_en_ipcdrv_status_t;

/** \} group_ipc_enums */


#ifdef __cplusplus
extern "C" {
#endif

/** \cond INTERNAL */

__STATIC_INLINE void     Cy_IPC_Drv_WriteDataValue (IPC_STRUCT_Type* base, uint32_t dataValue);
__STATIC_INLINE uint32_t Cy_IPC_Drv_ReadDataValue (IPC_STRUCT_Type const * base);

__STATIC_INLINE uint32_t Cy_IPC_Drv_ExtractAcquireMask (uint32_t intMask);
__STATIC_INLINE uint32_t Cy_IPC_Drv_ExtractReleaseMask (uint32_t intMask);

/** \endcond */

/**
* \addtogroup group_ipc_functions
* \{
*/

__STATIC_INLINE IPC_STRUCT_Type*       Cy_IPC_Drv_GetIpcBaseAddress (uint32_t ipcIndex);
__STATIC_INLINE IPC_INTR_STRUCT_Type*  Cy_IPC_Drv_GetIntrBaseAddr (uint32_t ipcIntrIndex);

__STATIC_INLINE void     Cy_IPC_Drv_AcquireNotify (IPC_STRUCT_Type * base, uint32_t notifyEventIntr);
__STATIC_INLINE void     Cy_IPC_Drv_ReleaseNotify (IPC_STRUCT_Type * base, uint32_t notifyEventIntr);

__STATIC_INLINE  cy_en_ipcdrv_status_t Cy_IPC_Drv_LockAcquire (IPC_STRUCT_Type const * base);
cy_en_ipcdrv_status_t    Cy_IPC_Drv_LockRelease (IPC_STRUCT_Type * base, uint32_t releaseEventIntr);
__STATIC_INLINE bool     Cy_IPC_Drv_IsLockAcquired (IPC_STRUCT_Type const * base);
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetLockStatus (IPC_STRUCT_Type const * base);

cy_en_ipcdrv_status_t    Cy_IPC_Drv_SendMsgWord (IPC_STRUCT_Type * base, uint32_t notifyEventIntr, uint32_t message);
cy_en_ipcdrv_status_t    Cy_IPC_Drv_ReadMsgWord (IPC_STRUCT_Type const * base, uint32_t * message);
__STATIC_INLINE cy_en_ipcdrv_status_t Cy_IPC_Drv_SendMsgPtr (IPC_STRUCT_Type* base, uint32_t notifyEventIntr, void const * msgPtr);
__STATIC_INLINE cy_en_ipcdrv_status_t Cy_IPC_Drv_ReadMsgPtr (IPC_STRUCT_Type const * base, void ** msgPtr);

__STATIC_INLINE void     Cy_IPC_Drv_SetInterruptMask (IPC_INTR_STRUCT_Type * base,
                                                      uint32_t ipcReleaseMask, uint32_t ipcAcquireMask);
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptMask (IPC_INTR_STRUCT_Type const * base);
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptStatusMasked (IPC_INTR_STRUCT_Type const * base);
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptStatus (IPC_INTR_STRUCT_Type const * base);
__STATIC_INLINE void     Cy_IPC_Drv_SetInterrupt (IPC_INTR_STRUCT_Type * base,
                                                      uint32_t ipcReleaseMask, uint32_t ipcAcquireMask);
__STATIC_INLINE void     Cy_IPC_Drv_ClearInterrupt (IPC_INTR_STRUCT_Type * base,
                                                      uint32_t ipcReleaseMask, uint32_t ipcAcquireMask);

/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetIpcBaseAddress
****************************************************************************//**
*
* This function takes an IPC channel index as a parameter and returns the base
* address the IPC registers corresponding to the IPC channel.
*
* \note The user is responsible for ensuring that ipcIndex does not exceed the
* limits.
*
* \param ipcIndex
* Represents the number of IPC structure. This is converted to the base address of
* the IPC channel registers.
*
* \return
* Returns a pointer to the base of the IPC registers.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_SendMsgWord
*
*******************************************************************************/
__STATIC_INLINE IPC_STRUCT_Type* Cy_IPC_Drv_GetIpcBaseAddress (uint32_t ipcIndex)
{
    CY_ASSERT_L1((uint32_t)(cy_device->cpussIpcNr) > ipcIndex);
    return ( (IPC_STRUCT_Type*) ( &IPC->STRUCT[ipcIndex] ) );
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetIntrBaseAddr
****************************************************************************//**
*
* This function takes an IPC interrupt structure index and returns the base
* address of the IPC interrupt registers corresponding to the IPC Interrupt.
*
* \note The user is responsible for ensuring that ipcIntrIndex does not exceed the
* limits.
*
* \param ipcIntrIndex
* Represents the number of IPC interrupt structure. This is converted to the
* base address of the IPC interrupt registers.
*
* \return
* Returns a pointer to the base of the IPC interrupt registers.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatus
*
*******************************************************************************/
__STATIC_INLINE IPC_INTR_STRUCT_Type* Cy_IPC_Drv_GetIntrBaseAddr (uint32_t ipcIntrIndex)
{
    CY_ASSERT_L1((uint32_t)(cy_device->cpussIpcIrqNr) > ipcIntrIndex);
    return ( (IPC_INTR_STRUCT_Type*) ( &IPC->INTR_STRUCT[ipcIntrIndex] ) );
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_SetInterruptMask
****************************************************************************//**
*
* This function is used to set the interrupt mask for an IPC Interrupt.
* The mask sets release or acquire notification events for all IPC channels.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from the
* IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \param ipcReleaseMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* release event.
*
* \param ipcAcquireMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* notify event.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE void  Cy_IPC_Drv_SetInterruptMask (IPC_INTR_STRUCT_Type* base,
                                              uint32_t ipcReleaseMask, uint32_t ipcAcquireMask)
{
    CY_ASSERT_L1(0ul == (ipcAcquireMask  & ~(uint32_t)(IPC_STRUCT_NOTIFY_INTR_NOTIFY_Msk)));
    CY_ASSERT_L1(0ul == (ipcReleaseMask & ~(uint32_t)(IPC_STRUCT_RELEASE_INTR_RELEASE_Msk)));
    REG_IPC_INTR_STRUCT_INTR_MASK(base) = _VAL2FLD( IPC_INTR_STRUCT_INTR_MASK_NOTIFY,  ipcAcquireMask) |
                      _VAL2FLD( IPC_INTR_STRUCT_INTR_MASK_RELEASE, ipcReleaseMask);
}


/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetInterruptMask
****************************************************************************//**
*
* This function is used to read the interrupt mask.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from
* the IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \return
*   The return value is encoded as follows
*   <table>
*   <tr><th>Interrupt sources   <th>Value
*   <tr><td>Ipc_PORTX_RELEASE   <td>Xth bit set
*   <tr><td>Ipc_PORTX_NOTIFY    <td>X+16th bit set
*   </table>
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptMask(IPC_INTR_STRUCT_Type const * base)
{
    return REG_IPC_INTR_STRUCT_INTR_MASK(base);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetInterruptStatusMasked
****************************************************************************//**
*
* This function is used to read the active unmasked interrupt. This function
* can be used in the interrupt service routine to find which source triggered
* the interrupt.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from the
* IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \return
*   The return value is encoded as follows
*   <table>
*   <tr><th>Interrupt sources   <th>Value
*   <tr><td>Ipc_PORTX_RELEASE   <td>Xth bit set
*   <tr><td>Ipc_PORTX_NOTIFY    <td>X+16th bit set
*   </table>
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptStatusMasked (IPC_INTR_STRUCT_Type const * base)
{
    return REG_IPC_INTR_STRUCT_INTR_MASKED(base);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetInterruptStatus
****************************************************************************//**
*
* This function is used to read the pending interrupts. Note that this read is
* an unmasked read of the interrupt status. Interrupt sources read as active by
* this function would generate interrupts only if they were not masked.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from the
* IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \return
*   The return value is encoded as follows
*   <table>
*   <tr><th>Interrupt sources   <th>Value
*   <tr><td>Ipc_PORTX_RELEASE   <td>Xth bit set
*   <tr><td>Ipc_PORTX_NOTIFY    <td>X+16th bit set
*   </table>
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetInterruptStatus(IPC_INTR_STRUCT_Type const * base)
{
    return REG_IPC_INTR_STRUCT_INTR(base);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_SetInterrupt
****************************************************************************//**
*
* This function is used to set the interrupt source. This function can be used
* to activate interrupts through software.
* \note That interrupt sources set using this interrupt would generate interrupts
* only if they are not masked.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from the
* IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \param ipcReleaseMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* release event.
*
* \param ipcAcquireMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* notify event.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_SetInterrupt
*
*******************************************************************************/
__STATIC_INLINE void  Cy_IPC_Drv_SetInterrupt(IPC_INTR_STRUCT_Type* base, uint32_t ipcReleaseMask, uint32_t ipcAcquireMask)
{
    CY_ASSERT_L1(0ul == (ipcAcquireMask  & ~(uint32_t)(IPC_STRUCT_NOTIFY_INTR_NOTIFY_Msk)));
    CY_ASSERT_L1(0ul == (ipcReleaseMask & ~(uint32_t)(IPC_STRUCT_RELEASE_INTR_RELEASE_Msk)));
    REG_IPC_INTR_STRUCT_INTR_SET(base) =  _VAL2FLD( IPC_INTR_STRUCT_INTR_NOTIFY,  ipcAcquireMask )  |
                      _VAL2FLD( IPC_INTR_STRUCT_INTR_RELEASE, ipcReleaseMask );
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ClearInterrupt
****************************************************************************//**
*
* This function is used to clear the interrupt source. Use this function to clear
* a pending interrupt source in the interrupt status.
*
* \param base
* This is a handle to the IPC interrupt. This handle can be calculated from the
* IPC interrupt number using \ref Cy_IPC_Drv_GetIntrBaseAddr.
*
* \param ipcReleaseMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* release event.
*
* \param ipcAcquireMask
* An encoded list of all IPC channels that can trigger the interrupt on a
* notify event.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetInterruptStatusMasked
*
*******************************************************************************/
__STATIC_INLINE void  Cy_IPC_Drv_ClearInterrupt(IPC_INTR_STRUCT_Type* base, uint32_t ipcReleaseMask, uint32_t ipcAcquireMask)
{
    CY_ASSERT_L1(0ul == (ipcAcquireMask  & ~(uint32_t)(IPC_STRUCT_NOTIFY_INTR_NOTIFY_Msk)));
    CY_ASSERT_L1(0ul == (ipcReleaseMask & ~(uint32_t)(IPC_STRUCT_RELEASE_INTR_RELEASE_Msk)));
    REG_IPC_INTR_STRUCT_INTR(base) =  _VAL2FLD(IPC_INTR_STRUCT_INTR_NOTIFY,  ipcAcquireMask) |
                  _VAL2FLD(IPC_INTR_STRUCT_INTR_RELEASE, ipcReleaseMask);
    (void)REG_IPC_INTR_STRUCT_INTR(base);  /* Read the register to flush the cache */
}

/** \} group_ipc_functions */

/** \} group_ipc */

/*******************************************************************************
* Function Name: Cy_IPC_Drv_AcquireNotify
****************************************************************************//**
*
* The function generates a aquire notification event by IPC interrupt structure.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param notifyEventIntr
* Bit encoded list of IPC interrupt structures that are triggered
* by a notification. Bit number correspond to number of the IPC interrupt
* structure.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_LockAcquire
*
*******************************************************************************/
__STATIC_INLINE void  Cy_IPC_Drv_AcquireNotify (IPC_STRUCT_Type* base, uint32_t notifyEventIntr)
{
    CY_ASSERT_L1(0ul == (notifyEventIntr  & ~(uint32_t)(IPC_STRUCT_NOTIFY_INTR_NOTIFY_Msk)));
    REG_IPC_STRUCT_NOTIFY(base) = _VAL2FLD(IPC_STRUCT_NOTIFY_INTR_NOTIFY, notifyEventIntr);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ReleaseNotify
****************************************************************************//**
*
* The function generates a release notification event by IPC interrupt structure.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param notifyEventIntr
* Bit encoded list of IPC interrupt lines that are triggered by a notification.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_ReadMsgWord
*
*******************************************************************************/
__STATIC_INLINE void  Cy_IPC_Drv_ReleaseNotify (IPC_STRUCT_Type* base, uint32_t notifyEventIntr)
{
    CY_ASSERT_L1(0ul == (notifyEventIntr  & ~(uint32_t)(IPC_INTR_STRUCT_INTR_RELEASE_Msk)));
    REG_IPC_STRUCT_RELEASE(base) = _VAL2FLD(IPC_INTR_STRUCT_INTR_RELEASE, notifyEventIntr);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_WriteDataValue
****************************************************************************//**
*
* The function writes a value to the DATA register of the IPC channel.
*
* This function is internal and should not be called directly by user
* software.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param dataValue
* Value to be written.
*
*******************************************************************************/
__STATIC_INLINE void     Cy_IPC_Drv_WriteDataValue (IPC_STRUCT_Type* base, uint32_t dataValue)
{
    REG_IPC_STRUCT_DATA(base) = dataValue;
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ReadDataValue
****************************************************************************//**
*
* The function reads a value from the DATA register of the IPC channel.
*
* This function is internal and should not be called directly by user
* software.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \return
* Value from DATA register.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_ReadDataValue (IPC_STRUCT_Type const * base)
{
    return REG_IPC_STRUCT_DATA(base);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_IsLockAcquired
****************************************************************************//**
*
* The function is used to test the status of an IPC channel. The function
* tells the reader if the IPC channel was in the locked or released state.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \return
*   Status for the function:
*   true:  The IPC channel is in the Locked state.
*   false: The IPC channel is in the Released state.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_LockAcquire
*
*******************************************************************************/
__STATIC_INLINE bool Cy_IPC_Drv_IsLockAcquired (IPC_STRUCT_Type const * base)
{
    return ( 0u != _FLD2VAL(IPC_STRUCT_ACQUIRE_SUCCESS, REG_IPC_STRUCT_LOCK_STATUS(base)) );
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_GetLockStatus
****************************************************************************//**
*
* The function is used to get the status of an IPC channel.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \return
* Value from LOCK_STATUS register.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_GetLockStatus
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_GetLockStatus (IPC_STRUCT_Type const * base)
{
    return REG_IPC_STRUCT_LOCK_STATUS(base);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ExtractAcquireMask
****************************************************************************//**
*
* The function extracts an Acquire mask part from full interrupt mask value.
*
* This function is internal and should not be called directly by user
* software.
*
* \param intMask
* Interrupt mask value to be processed.
*
* \return
* Acquire mask value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_ExtractAcquireMask (uint32_t intMask)
{
    return _FLD2VAL(IPC_INTR_STRUCT_INTR_MASK_NOTIFY, intMask);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ExtractReleaseMask
****************************************************************************//**
*
* The function extracts a Release mask part from full interrupt mask value.
*
* This function is internal and should not be called directly by user
* software.
*
* \param intMask
* Interrupt mask value to be processed.
*
* \return
* Release mask value.
*
*******************************************************************************/
__STATIC_INLINE uint32_t Cy_IPC_Drv_ExtractReleaseMask (uint32_t intMask)
{
    return _FLD2VAL(IPC_INTR_STRUCT_INTR_MASK_RELEASE, intMask);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_SendMsgPtr
****************************************************************************//**
*
* This function is used to send a message pointer through an IPC channel.
* The message structure may hold a generic pointer that may contain the address
* of any user data type or structure. This parameter could be a pointer to a 32-bit
* integer, an array, or even a data structure defined in the user code. This
* function acts as a transfer engine for sending the pointer. Any memory
* management of the pointer allocation and deallocation is up to the application
* code.
* The function also has an associated notification field that will let the
* message notify one or multiple interrupts.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param notifyEventIntr
* Bit encoded list of IPC interrupt lines that are triggered during the release
* action.
*
* \param msgPtr
* The message pointer that is being sent over the IPC channel.
*
* \return   Status of the operation:
*   \retval CY_IPC_DRV_SUCCESS: The send operation was successful.
*   \retval CY_IPC_DRV_ERROR:   The IPC channel is unavailable because
*                               it is already locked.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_SendMsgPtr
*
*******************************************************************************/
__STATIC_INLINE cy_en_ipcdrv_status_t  Cy_IPC_Drv_SendMsgPtr(IPC_STRUCT_Type* base, uint32_t notifyEventIntr, void const * msgPtr)
{
    CY_ASSERT_L1(NULL != msgPtr);
    return Cy_IPC_Drv_SendMsgWord(base, notifyEventIntr, (uint32_t)msgPtr);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_ReadMsgPtr
****************************************************************************//**
*
* This function is used to read a 32-bit pointer message through an IPC channel.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress.
*
* \param msgPtr
* Pointer variable to hold the data pointer that is being read from the IPC
* channel.
*
*
* \return Status of the operation
*   \retval CY_IPC_DRV_SUCCESS: The function executed successfully and the IPC
*                       was acquired.
*   \retval CY_IPC_DRV_ERROR:   The function encountered an error because the IPC
*                       channel was already in a released state meaning the data
*                       in it is invalid.
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_ReadMsgPtr
*
*******************************************************************************/
__STATIC_INLINE  cy_en_ipcdrv_status_t  Cy_IPC_Drv_ReadMsgPtr (IPC_STRUCT_Type const * base, void ** msgPtr)
{
    CY_ASSERT_L1(NULL != msgPtr);
    return Cy_IPC_Drv_ReadMsgWord(base, (uint32_t *)msgPtr);
}

/*******************************************************************************
* Function Name: Cy_IPC_Drv_LockAcquire
****************************************************************************//**
*
* This function is used to acquire the IPC channel.
*
* \param base
* This parameter is a handle that represents the base address of the registers
* of the IPC channel.
* The parameter is generally returned from a call to the \ref
* Cy_IPC_Drv_GetIpcBaseAddress
*
* \return   Status of the operation
*   \retval CY_IPC_DRV_SUCCESS: The IPC was successfully acquired
*   \retval CY_IPC_DRV_ERROR:   The IPC was not acquired because it was already acquired
*                       by another master
*
* \funcusage
* \snippet IPC_sut_01.cydsn/main_cm4.c snippet_Cy_IPC_Drv_LockAcquire
*
*******************************************************************************/
__STATIC_INLINE cy_en_ipcdrv_status_t Cy_IPC_Drv_LockAcquire (IPC_STRUCT_Type const * base)
{
    return ( 0ul != _FLD2VAL(IPC_STRUCT_ACQUIRE_SUCCESS, REG_IPC_STRUCT_ACQUIRE(base))) ? CY_IPC_DRV_SUCCESS : CY_IPC_DRV_ERROR;
}

#ifdef __cplusplus
}
#endif

#endif /* !defined(CY_IPC_DRV_H) */

/* [] END OF FILE */
