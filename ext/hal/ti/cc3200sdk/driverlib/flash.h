//*****************************************************************************
//
//  flash.h
//
//  Prototypes for the flash driver.
//
//  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#ifndef __FLASH_H__
#define __FLASH_H__

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

//*****************************************************************************
//
// Values that can be passed to FlashProtectSet(), and returned by
// FlashProtectGet().
//
//*****************************************************************************
typedef enum
{
    FlashReadWrite,                         // Flash can be read and written
    FlashReadOnly,                          // Flash can only be read
    FlashExecuteOnly                        // Flash can only be executed
}
tFlashProtection;

//*****************************************************************************
//
// Values passed to FlashIntEnable(), FlashIntDisable() and FlashIntClear() and
// returned from FlashIntStatus().
//
//*****************************************************************************
#define FLASH_INT_PROGRAM     0x00000002 // Programming Interrupt Mask
#define FLASH_INT_ACCESS      0x00000001 // Access Interrupt Mask
#define FLASH_INT_EEPROM      0x00000004 // EEPROM Interrupt Mask
#define FLASH_INT_VOLTAGE_ERR 0x00000200 // Voltage Error Interrupt Mask
#define FLASH_INT_DATA_ERR    0x00000400 // Invalid Data Interrupt Mask
#define FLASH_INT_ERASE_ERR   0x00000800 // Erase Error Interrupt Mask
#define FLASH_INT_PROGRAM_ERR 0x00002000 // Program Verify Error Interrupt Mask


//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void FlashDisable(void);
extern long FlashErase(unsigned long ulAddress);
extern void FlashEraseNonBlocking(unsigned long ulAddress);
extern long FlashMassErase(void);
extern void FlashMassEraseNonBlocking(void);
extern long FlashProgram(unsigned long *pulData, unsigned long ulAddress,
                         unsigned long ulCount);
extern long FlashProgramNonBlocking(unsigned long *pulData,
                                    unsigned long ulAddress,
                                    unsigned long ulCount);
extern void FlashIntRegister(void (*pfnHandler)(void));
extern void FlashIntUnregister(void);
extern void FlashIntEnable(unsigned long ulIntFlags);
extern void FlashIntDisable(unsigned long ulIntFlags);
extern unsigned long FlashIntStatus(tBoolean bMasked);
extern void FlashIntClear(unsigned long ulIntFlags);
extern tFlashProtection FlashProtectGet(unsigned long ulAddress);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __FLASH_H__
