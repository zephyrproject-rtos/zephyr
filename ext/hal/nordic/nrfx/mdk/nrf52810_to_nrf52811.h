/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef NRF52810_TO_NRF52811_H
#define NRF52810_TO_NRF52811_H

/*lint ++flb "Enter library region */

/* This file is given to prevent your SW from not compiling with the name changes between nRF52810 and nRF52811 devices.
 * It redefines the old nRF52810 names into the new ones as long as the functionality is still supported. If the
 * functionality is gone, there old names are not defined, so compilation will fail. */
 
/* Differences between latest nRF52810 headers and nRF52811 headers. */

/* Interrupt service routines handlers. */
#ifndef TWIM0_TWIS0_IRQHandler
    #define TWIM0_TWIS0_IRQHandler      TWIM0_TWIS0_SPIM1_SPIS1_IRQHandler
#endif


/* Interrupt service routines index. */
#ifndef TWIM0_TWIS0_IRQn
    #define TWIM0_TWIS0_IRQn            TWIM0_TWIS0_SPIM1_SPIS1_IRQn
#endif


/*lint --flb "Leave library region" */

#endif /* NRF52810_TO_NRF52811_H */

