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
#ifndef __FPU_H__
#define __FPU_H__

//*****************************************************************************
//
//!
//! \addtogroup fpu_api
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
// Values that can be passed to FPUHalfPrecisionSet as the mode parameter.
//
//*****************************************************************************
#define FPU_HALF_IEEE           0x00000000
#define FPU_HALF_ALTERNATE      0x04000000

//*****************************************************************************
//
// Values that can be passed to FPU_setNaNMode as the mode parameter.
//
//*****************************************************************************
#define FPU_NAN_PROPAGATE       0x00000000
#define FPU_NAN_DEFAULT         0x02000000

//*****************************************************************************
//
// Values that can be passed to FPU_setFlushToZeroMode as the mode parameter.
//
//*****************************************************************************
#define FPU_FLUSH_TO_ZERO_DIS   0x00000000
#define FPU_FLUSH_TO_ZERO_EN    0x01000000

//*****************************************************************************
//
// Values that can be passed to FPU_setRoundingMode as the mode parameter.
//
//*****************************************************************************
#define FPU_ROUND_NEAREST       0x00000000
#define FPU_ROUND_POS_INF       0x00400000
#define FPU_ROUND_NEG_INF       0x00800000
#define FPU_ROUND_ZERO          0x00c00000

//*****************************************************************************
//
//! Enables the floating-point unit.
//!
//! This function enables the floating-point unit, allowing the floating-point
//! instructions to be executed.  This function must be called prior to
//! performing any hardware floating-point operations; failure to do so results
//! in a NOCP usage fault.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_enableModule(void);

//*****************************************************************************
//
//! Disables the floating-point unit.
//!
//! This function disables the floating-point unit, preventing floating-point
//! instructions from executing (generating a NOCP usage fault instead).
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_disableModule(void);

//*****************************************************************************
//
//! Enables the stacking of floating-point registers.
//!
//! This function enables the stacking of floating-point registers s0-s15 when
//! an interrupt is handled.  When enabled, space is reserved on the stack for
//! the floating-point context and the floating-point state is saved into this
//! stack space.  Upon return from the interrupt, the floating-point context is
//! restored.
//!
//! If the floating-point registers are not stacked, floating-point
//! instructions cannot be safely executed in an interrupt handler because the
//! values of s0-s15 are not likely to be preserved for the interrupted code.
//! On the other hand, stacking the floating-point registers increases the
//! stacking operation from 8 words to 26 words, also increasing the interrupt
//! response latency.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_enableStacking(void);

//*****************************************************************************
//
//! Enables the lazy stacking of floating-point registers.
//!
//! This function enables the lazy stacking of floating-point registers s0-s15
//! when an interrupt is handled.  When lazy stacking is enabled, space is
//! reserved on the stack for the floating-point context, but the
//! floating-point state is not saved.  If a floating-point instruction is
//! executed from within the interrupt context, the floating-point context is
//! first saved into the space reserved on the stack.  On completion of the
//! interrupt handler, the floating-point context is only restored if it was
//! saved (as the result of executing a floating-point instruction).
//!
//! This method provides a compromise between fast interrupt response (because
//! the floating-point state is not saved on interrupt entry) and the ability
//! to use floating-point in interrupt handlers (because the floating-point
//! state is saved if floating-point instructions are used).
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_enableLazyStacking(void);

//*****************************************************************************
//
//! Disables the stacking of floating-point registers.
//!
//! This function disables the stacking of floating-point registers s0-s15 when
//! an interrupt is handled.  When floating-point context stacking is disabled,
//! floating-point operations performed in an interrupt handler destroy the
//! floating-point context of the main thread of execution.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_disableStacking(void);

//*****************************************************************************
//
//! Selects the format of half-precision floating-point values.
//!
//! \param mode is the format for half-precision floating-point value, which
//! is either \b FPU_HALF_IEEE or \b FPU_HALF_ALTERNATE.
//!
//! This function selects between the IEEE half-precision floating-point
//! representation and the Cortex-M processor alternative representation.  The
//! alternative representation has a larger range but does not have a way to
//! encode infinity (positive or negative) or NaN (quiet or signalling).  The
//! default setting is the IEEE format.
//!
//! \note Unless this function is called prior to executing any floating-point
//! instructions, the default mode is used.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_setHalfPrecisionMode(uint32_t mode);

//*****************************************************************************
//
//! Selects the NaN mode.
//!
//! \param mode is the mode for NaN results; which is
//! either \b FPU_NAN_PROPAGATE or \b FPU_NAN_DEFAULT.
//!
//! This function selects the handling of NaN results during floating-point
//! computations.  NaNs can either propagate (the default), or they can return
//! the default NaN.
//!
//! \note Unless this function is called prior to executing any floating-point
//! instructions, the default mode is used.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_setNaNMode(uint32_t mode);

//*****************************************************************************
//
//! Selects the flush-to-zero mode.
//!
//! \param mode is the flush-to-zero mode; which is either
//! \b FPU_FLUSH_TO_ZERO_DIS or \b FPU_FLUSH_TO_ZERO_EN.
//!
//! This function enables or disables the flush-to-zero mode of the
//! floating-point unit.  When disabled (the default), the floating-point unit
//! is fully IEEE compliant.  When enabled, values close to zero are treated as
//! zero, greatly improving the execution speed at the expense of some accuracy
//! (as well as IEEE compliance).
//!
//! \note Unless this function is called prior to executing any floating-point
//! instructions, the default mode is used.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_setFlushToZeroMode(uint32_t mode);

//*****************************************************************************
//
//! Selects the rounding mode for floating-point results.
//!
//! \param mode is the rounding mode.
//!
//! This function selects the rounding mode for floating-point results.  After
//! a floating-point operation, the result is rounded toward the specified
//! value.  The default mode is \b FPU_ROUND_NEAREST.
//!
//! The following rounding modes are available (as specified by \e mode):
//!
//! - \b FPU_ROUND_NEAREST - round toward the nearest value
//! - \b FPU_ROUND_POS_INF - round toward positive infinity
//! - \b FPU_ROUND_NEG_INF - round toward negative infinity
//! - \b FPU_ROUND_ZERO - round toward zero
//!
//! \note Unless this function is called prior to executing any floating-point
//! instructions, the default mode is used.
//!
//! \return None.
//
//*****************************************************************************
extern void FPU_setRoundingMode(uint32_t mode);

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


#endif // __FPU_H__
