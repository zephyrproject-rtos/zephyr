/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RPMSG_DEFAULT_CONFIG_H
#define _RPMSG_DEFAULT_CONFIG_H

#define RL_USE_CUSTOM_CONFIG (1)

#if RL_USE_CUSTOM_CONFIG
#include "rpmsg_config.h"
#endif

/*!
 * @addtogroup config
 * @{
 * @file
 */

//! @name Configuration options
//@{

//! @def RL_MS_PER_INTERVAL
//!
//! Delay in milliseconds used in non-blocking API functions for polling.
//! The default value is 1.
#ifndef RL_MS_PER_INTERVAL
#define RL_MS_PER_INTERVAL (1)
#endif

//! @def RL_BUFFER_PAYLOAD_SIZE
//!
//! Size of the buffer payload, it must be equal to (240, 496, 1008, ...) 
//! [2^n - 16].
//! The default value is 496.
#ifndef RL_BUFFER_PAYLOAD_SIZE
#define RL_BUFFER_PAYLOAD_SIZE (496)
#endif

//! @def RL_BUFFER_COUNT
//!
//! Number of the buffers, it must be power of two (2, 4, ...).
//! The default value is 2.
#ifndef RL_BUFFER_COUNT
#define RL_BUFFER_COUNT (2)
#endif

//! @def RL_API_HAS_ZEROCOPY
//!
//! Zero-copy API functions enabled/disabled.
//! The default value is 1 (enabled).
#ifndef RL_API_HAS_ZEROCOPY
#define RL_API_HAS_ZEROCOPY (1)
#endif

//! @def RL_USE_STATIC_API
//!
//! Static API functions (no dynamic allocation) enabled/disabled.
//! The default value is 0 (static API disabled).
#ifndef RL_USE_STATIC_API
#define RL_USE_STATIC_API (0)
#endif

//! @def RL_CLEAR_USED_BUFFERS
//!
//! Clearing used buffers before returning back to the pool of free buffers 
//! enabled/disabled.
//! The default value is 0 (disabled).
#ifndef RL_CLEAR_USED_BUFFERS
#define RL_CLEAR_USED_BUFFERS (0)
#endif

//! @def RL_USE_MCMGR_IPC_ISR_HANDLER
//!
//! When enabled IPC interrupts are managed by the Multicore Manager (IPC 
//! interrupts router), when disabled RPMsg-Lite manages IPC interrupts 
//! by itself.
//! The default value is 0 (no MCMGR IPC ISR handler used).
#ifndef RL_USE_MCMGR_IPC_ISR_HANDLER
#define RL_USE_MCMGR_IPC_ISR_HANDLER (0)
#endif

//! @def RL_USE_ENVIRONMENT_CONTEXT
//!
//! When enabled the environment layer uses its own context.
//! Added for QNX port mainly, but can be used if required.
//! The default value is 0 (no context, saves some RAM).
#ifndef RL_USE_ENVIRONMENT_CONTEXT
#define RL_USE_ENVIRONMENT_CONTEXT (0)
#endif


//! @def RL_DEBUG_CHECK_BUFFERS
//!
//! Do not use in RPMsg-Lite to Linux configuration
#ifndef RL_DEBUG_CHECK_BUFFERS
#define RL_DEBUG_CHECK_BUFFERS (0)
#endif



//! @def RL_HANG
//!
//! Default implementation of hang assert function
 static inline void RL_HANG(void)
 {
     while(1)
     {

     }
 }


//! @def RL_ASSERT
//!
//! Assert implementation.
#ifndef RL_ASSERT
#define RL_ASSERT_BOOL(b)  \
    do                \
    {                 \
        if (!(b))       \
        { \
            RL_HANG(); \
        } \
    } while (0);
#define RL_ASSERT(x) RL_ASSERT_BOOL((x)!=0)




#endif
//@}

#endif /* _RPMSG_DEFAULT_CONFIG_H */
