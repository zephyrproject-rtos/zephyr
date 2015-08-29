/* microkernel.h - public API for microkernel */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

#ifndef _MICROKERNEL_H
#define _MICROKERNEL_H

/* nanokernel and generic kernel public APIs */

#include <nanokernel.h>

/*
 * microkernel private APIs that are exposed via the public API
 *
 * THESE ITEMS SHOULD NOT BE REFERENCED EXCEPT BY THE KERNEL ITSELF!
 */

#define _USE_CURRENT_SEM (-1)

/* end of private APIs */

/**
 * @brief Microkernel Public APIs
 * @defgroup microkernel_services Microkernel Services
 * @{
 */

#include <microkernel/base_api.h>

#include <microkernel/task.h>
#include <microkernel/ticks.h>
#include <microkernel/memory_map.h>
#include <microkernel/mutex.h>
#include <microkernel/mailbox.h>
#include <microkernel/fifo.h>
#include <microkernel/semaphore.h>
#include <microkernel/event.h>
#include <microkernel/memory_pool.h>
#include <microkernel/pipe.h>
#include <microkernel/task_irq.h>

/**
 * @}
 */

#endif /* _MICROKERNEL_H */
