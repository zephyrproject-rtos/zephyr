/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
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

/* default values */
/* START { */
#ifndef RL_MS_PER_INTERVAL
#define RL_MS_PER_INTERVAL (1)
#endif

#ifndef RL_BUFFER_PAYLOAD_SIZE
#define RL_BUFFER_PAYLOAD_SIZE (496)
#endif

#ifndef RL_BUFFER_COUNT
#define RL_BUFFER_COUNT (2)
#endif

#ifndef RL_API_HAS_ZEROCOPY
#define RL_API_HAS_ZEROCOPY (1)
#endif

#ifndef RL_USE_STATIC_API
#define RL_USE_STATIC_API (0)
#endif

#ifndef RL_CLEAR_USED_BUFFERS
#define RL_CLEAR_USED_BUFFERS (0)
#endif

#ifndef RL_USE_MCMGR_IPC_ISR_HANDLER
#define RL_USE_MCMGR_IPC_ISR_HANDLER (0)
#endif

#ifndef RL_ASSERT
#define RL_ASSERT_BOOL(b)  \
    do                \
    {                 \
        if (!(b))       \
            while (1) \
                ;     \
    } while (0);
#define RL_ASSERT(x) RL_ASSERT_BOOL((x)!=0)
#endif
/* } END */

#endif /* _RPMSG_DEFAULT_CONFIG_H */
