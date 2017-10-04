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
#ifndef __PMAP_H__
#define __PMAP_H__

//*****************************************************************************
//
//! \addtogroup pmap_api
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

#include <stdint.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

//*****************************************************************************
//
//The following are values that can be passed to the PMAP_configurePorts() API
//as the portMapReconfigure parameter.
//
//*****************************************************************************
#define PMAP_ENABLE_RECONFIGURATION     PMAP_CTL_PRECFG
#define PMAP_DISABLE_RECONFIGURATION    0x00

//*****************************************************************************
//
//The following are values that can be passed to the PMAP_configurePorts() API
//as the pxMAPy parameter.
//
//*****************************************************************************
#define PMAP_P1MAP ((uint32_t)P1MAP - PMAP_BASE)
#define PMAP_P2MAP ((uint32_t)P2MAP - PMAP_BASE)
#define PMAP_P3MAP ((uint32_t)P3MAP - PMAP_BASE)
#define PMAP_P4MAP ((uint32_t)P4MAP - PMAP_BASE)
#define PMAP_P5MAP ((uint32_t)P5MAP - PMAP_BASE)
#define PMAP_P6MAP ((uint32_t)P6MAP - PMAP_BASE)
#define PMAP_P7MAP ((uint32_t)P7MAP - PMAP_BASE)


//*****************************************************************************
//
//Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! This function configures the MSP432 Port Mapper
//!
//! \param portMapping is the pointer to init Data
//! \param pxMAPy is the Port Mapper to initialize
//! \param numberOfPorts is the number of Ports to initialize
//! \param portMapReconfigure is used to enable/disable reconfiguration
//!             Valid values are
//!             \b PMAP_ENABLE_RECONFIGURATION
//!             \b PMAP_DISABLE_RECONFIGURATION [Default value]
//! Modified registers are \b PMAPKEYID, \b PMAPCTL
//!
//! \return None
//
//*****************************************************************************
extern void PMAP_configurePorts(const uint8_t *portMapping, uint8_t pxMAPy,
        uint8_t numberOfPorts, uint8_t portMapReconfigure);

/* Defines for future devices that might have multiple instances */
#define PMAP_configurePortsMultipleInstance(a,b,c,d,e) PMAP_configurePorts(b,c,d,e)

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

#endif
