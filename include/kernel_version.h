/* kernel version support */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

#ifndef _kernel_version__h_
#define _kernel_version__h_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The kernel version has been converted from a string to a four-byte
 * quantity that is divided into two parts.
 *
 * Part 1: The three most significant bytes represent the kernel's
 * numeric version, x.y.z. These fields denote:
 *       x -- major release
 *       y -- minor release
 *       z -- patchlevel release
 * Each of these elements must therefore be in the range 0 to 255, inclusive.
 *
 * Part 2: The least significant byte is reserved for future use.
 */
#define SYS_KERNEL_VER_MAJOR(ver) ((ver >> 24) & 0xFF)
#define SYS_KERNEL_VER_MINOR(ver) ((ver >> 16) & 0xFF)
#define SYS_KERNEL_VER_PATCHLEVEL(ver) ((ver >> 8) & 0xFF)

/* kernel version routines */

extern uint32_t sys_kernel_version_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _kernel_version__h_ */
