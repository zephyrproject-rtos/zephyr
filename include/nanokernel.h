/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Public APIs for the nanokernel.
 */

#ifndef __NANOKERNEL_H__
#define __NANOKERNEL_H__

/* fundamental include files */

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <sections.h>

/* generic kernel public APIs */

#include <kernel_version.h>
#include <sys_clock.h>
#include <drivers/rand32.h>
#include <misc/slist.h>
#include <misc/dlist.h>

#include <kernel.h>

/* architecture-specific nanokernel public APIs */
#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */
