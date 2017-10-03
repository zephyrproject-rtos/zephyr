/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
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
 */
 /** ============================================================================
 *  @file       EMAC.h
 *
 *  @brief      EMAC driver interface
 *
 *  The EMAC header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/EMAC.h>
 *  @endcode
 *
 *  # Implementation #
 *
 *  The EMAC driver interface module is joined (at link time) to a
 *  NULL-terminated array of EMAC_Config data structures named *EMAC_config*.
 *  *EMAC_config* is implemented in the application with each entry being an
 *  instance of a EMAC peripheral. Each entry in *EMAC_config* contains a:
 *  - (EMAC_FxnTable *) to a set of functions that implement a EMAC peripheral
 *  - (void *) data object that is associated with the EMAC_FxnTable
 *  - (void *) hardware attributes that are associated to the EMAC_FxnTable
 *
 *  # Instrumentation #
 *  The EMAC driver interface produces log statements if instrumentation is
 *  enabled.
 *
 *  Diagnostics Mask | Log details |
 *  ---------------- | ----------- |
 *  Diags_USER1      | basic operations |
 *  Diags_USER2      | detailed operations |
 *
 *  ============================================================================
 */

#ifndef ti_drivers_EMAC__include
#define ti_drivers_EMAC__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 *  @defgroup EMAC_CONTROL EMAC_control command and status codes
 *  These EMAC macros are reservations for EMAC.h
 *  @{
 */

/*!
 * Common EMAC_control command code reservation offset.
 * EMAC driver implementations should offset command codes with
 * EMAC_CMD_RESERVED growing positively
 *
 * Example implementation specific command codes:
 * @code
 * #define EMACXYZ_CMD_COMMAND0     EMAC_CMD_RESERVED + 0
 * #define EMACXYZ_CMD_COMMAND1     EMAC_CMD_RESERVED + 1
 * @endcode
 */
#define EMAC_CMD_RESERVED            32

/*!
 * Common EMAC_control status code reservation offset.
 * EMAC driver implementations should offset status codes with
 * EMAC_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define EMACXYZ_STATUS_ERROR0    EMAC_STATUS_RESERVED - 0
 * #define EMACXYZ_STATUS_ERROR1    EMAC_STATUS_RESERVED - 1
 * #define EMACXYZ_STATUS_ERROR2    EMAC_STATUS_RESERVED - 2
 * @endcode
 */
#define EMAC_STATUS_RESERVED        -32

/**
 *  @defgroup EMAC_STATUS Status Codes
 *  EMAC_STATUS_* macros are general status codes returned by EMAC_control()
 *  @{
 *  @ingroup EMAC_CONTROL
 */

/*!
 * @brief   Successful status code returned by EMAC_control().
 *
 * EMAC_control() returns EMAC_STATUS_SUCCESS if the control code was executed
 * successfully.
 */
#define EMAC_STATUS_SUCCESS         0

/*!
 * @brief   Generic error status code returned by EMAC_control().
 *
 * EMAC_control() returns EMAC_STATUS_ERROR if the control code was not executed
 * successfully.
 */
#define EMAC_STATUS_ERROR          -1

/*!
 * @brief   An error status code returned by EMAC_control() for undefined
 * command codes.
 *
 * EMAC_control() returns EMAC_STATUS_UNDEFINEDCMD if the control code is not
 * recognized by the driver implementation.
 */
#define EMAC_STATUS_UNDEFINEDCMD   -2
/** @}*/

/**
 *  @defgroup EMAC_CMD Command Codes
 *  EMAC_CMD_* macros are general command codes for EMAC_control(). Not all EMAC
 *  driver implementations support these command codes.
 *  @{
 *  @ingroup EMAC_CONTROL
 */

/* Add Camera_CMD_<commands> here */

/** @}*/

/** @}*/

/* Prototypes of EMAC interface */
typedef void (*EMAC_InitFxn)(uint32_t);
typedef bool (*EMAC_isLinkUpFxn)(uint32_t);

/*!
 *  @brief      EMAC Function table structure
 */
typedef struct EMAC_FxnTable {
    EMAC_InitFxn           emacInit;
    EMAC_isLinkUpFxn       emacIsLinkUp;
} EMAC_FxnTable;

/*!
 *  @brief      EMAC Global configuration
 *
 *  The EMAC_Config structure contains a set of pointers used to characterize
 *  the EMAC driver implementation.
 *
 *  This structure needs to be defined before calling EMAC_init() and it must
 *  not be changed thereafter.
 *
 *  @sa     EMAC_init()
 */
typedef struct EMAC_Config {
    /*! Pointer to a table of driver-specific implementations of EMAC APIs */
    EMAC_FxnTable const *fxnTablePtr;
    /*! Pointer to a driver specific data object */
    void                *object;
    /*! Pointer to a driver specific hardware attributes structure */
    void          const *hwAttrs;
} EMAC_Config;

/*!
 *  @brief  This function initializes the EMAC driver
 *
 *  This function must be called by the application before the NDK stack
 *  thread is started.
 *
 *  @pre    The EMAC_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other EMAC driver APIs. This function call does not modify any
 *          peripheral registers.
 */
extern void EMAC_init(void);

/*!
 *  @brief  This function returns if the link is up
 *
 *  @return true is the link is up. false if it is down.
 */
extern bool EMAC_isLinkUp(unsigned int index);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_EMAC__include */
