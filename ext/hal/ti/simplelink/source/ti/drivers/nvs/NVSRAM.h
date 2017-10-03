/*
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
 */
/** ============================================================================
 *  @file       NVSRAM.h
 *
 *  @brief      RAM implementation of the NVS driver
 *
 *  This NVS driver implementation makes use of RAM instead of FLASH memory.
 *  It can be used for developing code which relies the NVS driver without
 *  wearing down FLASH memory.
 *
 *  The NVS header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/NVS.h>
 *  #include <ti/drivers/nvs/NVSRAM.h>
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef ti_drivers_nvs_NVSRAM__include
#define ti_drivers_nvs_NVSRAM__include

#include <stdint.h>
#include <stdbool.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*!
 *  @internal @brief NVS function pointer table
 *
 *  'NVSRAM_fxnTable' is a fully populated function pointer table
 *  that can be referenced in the NVS_config[] array entries.
 *
 *  Users can minimize their application code size by providing their
 *  own custom NVS function pointer table that contains only those APIs
 *  used by the application.
 *
 *  An example of a custom NVS function table is shown below:
 *  @code
 *  //
 *  // Since the application does not use the
 *  // NVS_control(), NVS_lock(), and NVS_unlock() APIs,
 *  // these APIs are removed from the function
 *  // pointer table and replaced with NULL
 *  //
 *  const NVS_FxnTable myNVS_fxnTable = {
 *      NVSRAM_close,
 *      NULL,     // remove NVSRAM_control(),
 *      NVSRAM_erase,
 *      NVSRAM_getAttrs,
 *      NVSRAM_init,
 *      NULL,     // remove NVSRAM_lock(),
 *      NVSRAM_open,
 *      NVSRAM_read,
 *      NULL,     // remove NVSRAM_unlock(),
 *      NVSRAM_write
 *  };
 *  @endcode
 */
extern const NVS_FxnTable NVSRAM_fxnTable;

/*!
 *  @brief      NVSRAM Hardware Attributes
 *
 *  The 'sectorSize' is the minimal amount of data to that is cleared on an
 *  erase operation.  Devices which feature internal FLASH memory usually
 *  have a 4096 byte sector size (refer to device specific documentation).  It
 *  is recommended that the 'sectorSize' used match the FLASH memory sector
 *  size.
 *
 *  The 'regionBase' field must point to the base address of the region
 *  to be managed.  It is also required that the region be aligned on a
 *  sectorSize boundary (example below to demonstrate how to do this).
 *
 *  The 'regionSize' must be an integer multiple of the 'sectorSize'.
 *
 *  Defining and reserving RAM memory regions can be done entirely within the
 *  Board.c file.
 *
 *  The example below defines a char array, 'ramBuf' and uses compiler
 *  pragmas to place 'ramBuf' at an aligned address within RAM.
 *
 *  @code
 *  #define SECTORSIZE (4096)
 *
 *  #if defined(__TI_COMPILER_VERSION__)
 *  #pragma DATA_ALIGN(ramBuf, 4096)
 *  #elif defined(__IAR_SYSTEMS_ICC__)
 *  #pragma data_alignment=4096
 *  #elif defined(__GNUC__)
 *  __attribute__ ((aligned (4096)))
 *  #endif
 *  static char ramBuf[SECTORSIZE * 4];
 *
 *  NVSRAM_HWAttrs NVSRAMHWAttrs[1] = {
 *      {
 *          .regionBase = (void *) ramBuf,
 *          .regionSize = SECTORSIZE * 4,
 *          .sectorSize = SECTORSIZE
 *      }
 *  };
 *
 *
 *  @endcode
 */
typedef struct NVSRAM_HWAttrs {
    void   *regionBase;    /*!< Base address of RAM region */
    size_t  regionSize;    /*!< The size of the region in bytes */
    size_t  sectorSize;    /*!< Sector size in bytes */
} NVSRAM_HWAttrs;

/*
 *  @brief      NVSRAM Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct NVSRAM_Object {
    size_t sectorBaseMask;
    bool   isOpen;
} NVSRAM_Object;

/*
 *  @cond NODOC
 *  NVSRAM driver public APIs
 */

extern void NVSRAM_close(NVS_Handle handle);
extern int_fast16_t NVSRAM_control(NVS_Handle handle, uint_fast16_t cmd,
    uintptr_t arg);
extern int_fast16_t NVSRAM_erase(NVS_Handle handle, size_t offset,
    size_t size);
extern void NVSRAM_getAttrs(NVS_Handle handle, NVS_Attrs *attrs);
extern void NVSRAM_init();
extern int_fast16_t NVSRAM_lock(NVS_Handle handle, uint32_t timeout);
extern NVS_Handle NVSRAM_open(uint_least8_t index, NVS_Params *params);
extern int_fast16_t NVSRAM_read(NVS_Handle handle, size_t offset,
    void *buffer, size_t bufferSize);
extern void NVSRAM_unlock(NVS_Handle handle);
extern int_fast16_t NVSRAM_write(NVS_Handle handle, size_t offset,
    void *buffer, size_t bufferSize, uint_fast16_t flags);

/*! @endcond */

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

/*@}*/
#endif /* ti_drivers_nvs_NVSRAM__include */
