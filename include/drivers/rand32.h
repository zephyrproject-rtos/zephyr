/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Random number generator header file
 *
 * This header file declares prototypes for the kernel's random number generator
 * APIs.
 *
 * Typically, a platform enables the hidden CUSTOM_RANDOM_GENERATOR
 * configuration option and provide its own driver that implements both
 * sys_rand32_init() and sys_rand32_get(). If it does not do this, then for
 * projects that require random numbers, the project must either implement
 * those routines, or (for testing purposes only) enable the
 * TEST_RANDOM_GENERATOR configuration option.
 */

#ifndef __INCrand32h
#define __INCrand32h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void sys_rand32_init(void);
extern uint32_t sys_rand32_get(void);

#ifdef __cplusplus
}
#endif

#endif /* __INCrand32h */
