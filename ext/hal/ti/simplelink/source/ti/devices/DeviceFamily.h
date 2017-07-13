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
 *  @file       DeviceFamily.h
 *
 *  @brief      Infrastructure to select correct driverlib path and identify devices
 *
 *  This module enables the selection of the correct driverlib path for the current
 *  device. It also facilitates the use of per-device conditional compilation
 *  to enable minor variations in drivers between devices.
 *
 *  In order to use this functionality, DeviceFamily_XYZ must be defined as one of
 *  the supported values. The DeviceFamily_ID and DeviceFamily_DIRECTORY defines
 *  are set based on DeviceFamily_XYZ.
 */

#ifndef ti_devices_DeviceFamily__include
#define ti_devices_DeviceFamily__include

#ifdef __cplusplus
extern "C" {
#endif

/* List of DeviceFamily_ID_XYZ values.
 * DeviceFamily_ID may be used in the preprocessor for conditional compilation.
 * DeviceFamily_ID is set as one of these IDs is set based on the top
 * level DeviceFamily_XYZ define.
 */
#define DeviceFamily_ID_CC26X0         0x00000001
#define DeviceFamily_ID_CC26X0R2       0x00000002
#define DeviceFamily_ID_CC26X2         0x00000004
#define DeviceFamily_ID_CC13X0         0x00000100
#define DeviceFamily_ID_CC13X2         0x00000200
#define DeviceFamily_ID_CC3200         0x00010000
#define DeviceFamily_ID_CC3220         0x00020000
#define DeviceFamily_ID_MSP432P401x    0x01000000
#define DeviceFamily_ID_MSP432P4x1xI   0x02000000
#define DeviceFamily_ID_MSP432P4x1xT   0x04000000

/* Lookup table that sets DeviceFamily_ID and DeviceFamily_DIRECTORY based on the existence
 * of a DeviceFamily_XYZ define.
 * If DeviceFamily_XYZ is undefined, a compiler error is thrown. If multiple DeviceFamily_XYZ
 * are defined, the first one encountered is used.
 *
 * Currently supported values of DeviceFamily_XYZ are:
 * - DeviceFamily_CC26X0
 * - DeviceFamily_CC26X0R2
 * - DeviceFamily_CC26X2
 * - DeviceFamily_CC13X0
 * - DeviceFamily_CC13X2
 * - DeviceFamily_CC3200
 * - DeviceFamily_CC3220
 * - DeviceFamily_MSP432P401x
 * - DeviceFamily_MSP432P4x1
 */
#if defined(DeviceFamily_CC26X0)
    #define DeviceFamily_ID             DeviceFamily_ID_CC26X0
    #define DeviceFamily_DIRECTORY      cc26x0
#elif defined(DeviceFamily_CC26X0R2)
    #define DeviceFamily_ID             DeviceFamily_ID_CC26X0R2
    #define DeviceFamily_DIRECTORY      cc26x0r2
#elif defined(DeviceFamily_CC26X2)
    #define DeviceFamily_ID             DeviceFamily_ID_CC26X2
    #define DeviceFamily_DIRECTORY      cc26x2
#elif defined(DeviceFamily_CC13X0)
    #define DeviceFamily_ID             DeviceFamily_ID_CC13X0
    #define DeviceFamily_DIRECTORY      cc13x0
#elif defined(DeviceFamily_CC13X2)
    #define DeviceFamily_ID             DeviceFamily_ID_CC13X2
    #define DeviceFamily_DIRECTORY      cc26x2
#elif defined(DeviceFamily_CC3200)
    #define DeviceFamily_ID             DeviceFamily_ID_CC3200
    #define DeviceFamily_DIRECTORY      cc32xx
#elif defined(DeviceFamily_CC3220)
    #define DeviceFamily_ID             DeviceFamily_ID_CC3220
    #define DeviceFamily_DIRECTORY      cc32xx
#elif defined(DeviceFamily_MSP432P401x) || defined(__MSP432P401R__)
    #define DeviceFamily_ID             DeviceFamily_ID_MSP432P401x
    #define DeviceFamily_DIRECTORY      msp432p4xx
    #if !defined(__MSP432P401R__)
        #define __MSP432P401R__
    #endif
#elif defined(DeviceFamily_MSP432P4x1xI)
    #define DeviceFamily_ID             DeviceFamily_ID_MSP432P4x1xI
    #define DeviceFamily_DIRECTORY      msp432p4xx
    #if !defined(__MSP432P4111__)
        #define __MSP432P4111__
    #endif
#elif defined(DeviceFamily_MSP432P4x1xT)
    #define DeviceFamily_ID             DeviceFamily_ID_MSP432P4x1xT
    #define DeviceFamily_DIRECTORY      msp432p4xx
    #if !defined(__MSP432P4111__)
        #define __MSP432P4111__
    #endif
#else
    #error "DeviceFamily_XYZ undefined. You must defined DeviceFamily_XYZ!"
#endif

/*!
 *  @brief  Macro to include correct driverlib path.
 *
 *  @pre    DeviceFamily_XYZ which sets DeviceFamily_DIRECTORY must be defined first.
 *
 *  @param  x   A token containing the path of the file to include based on the root
 *              device folder. The preceding forward slash must be omitted.
 *              For example:
 *                  - #include DeviceFamily_constructPath(inc/hw_memmap.h)
 *                  - #include DeviceFamily_constructPath(driverlib/ssi.h)
 *
 *  @return Returns an include path.
 *
 */
#define DeviceFamily_constructPath(x) <ti/devices/DeviceFamily_DIRECTORY/x>

#ifdef __cplusplus
}
#endif

#endif /* ti_devices_DeviceFamily__include */
