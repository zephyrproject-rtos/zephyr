/* microkernel.h - public API for microkernel */

/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
