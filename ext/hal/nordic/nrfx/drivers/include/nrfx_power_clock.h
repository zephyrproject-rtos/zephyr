/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_POWER_CLOCK_H__
#define NRFX_POWER_CLOCK_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif


__STATIC_INLINE void nrfx_power_clock_irq_init(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE void nrfx_power_clock_irq_init(void)
{
    uint8_t priority;
#if NRFX_CHECK(NRFX_POWER_ENABLED) && NRFX_CHECK(NRFX_CLOCK_ENABLED)
    #if NRFX_POWER_CONFIG_IRQ_PRIORITY != NRFX_CLOCK_CONFIG_IRQ_PRIORITY
    #error "IRQ priority for POWER and CLOCK have to be the same. Check <nrfx_config.h>."
    #endif
    priority = NRFX_POWER_CONFIG_IRQ_PRIORITY;
#elif NRFX_CHECK(NRFX_POWER_ENABLED)
    priority = NRFX_POWER_CONFIG_IRQ_PRIORITY;
#elif NRFX_CHECK(NRFX_CLOCK_ENABLED)
    priority = NRFX_CLOCK_CONFIG_IRQ_PRIORITY;
#endif

    if (!NRFX_IRQ_IS_ENABLED(POWER_CLOCK_IRQn))
    {
        NRFX_IRQ_PRIORITY_SET(POWER_CLOCK_IRQn, priority);
        NRFX_IRQ_ENABLE(POWER_CLOCK_IRQn);
    }
}
#endif // SUPPRESS_INLINE_IMPLEMENTATION


#if NRFX_CHECK(NRFX_POWER_ENABLED) && NRFX_CHECK(NRFX_CLOCK_ENABLED)
void nrfx_power_clock_irq_handler(void);
#elif NRFX_CHECK(NRFX_POWER_ENABLED)
#define nrfx_power_irq_handler  nrfx_power_clock_irq_handler
#elif NRFX_CHECK(NRFX_CLOCK_ENABLED)
#define nrfx_clock_irq_handler  nrfx_power_clock_irq_handler
#endif


#ifdef __cplusplus
}
#endif

#endif // NRFX_POWER_CLOCK_H__
