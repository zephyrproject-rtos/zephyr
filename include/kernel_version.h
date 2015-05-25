/* kernel version support */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

#ifndef _kernel_version__h_
#define _kernel_version__h_

/*
 * The kernel version has been converted from a string to a four-byte
 * quantity that is divided into three parts.
 *
 * Part 1: The two most significant bytes are sub-divided into four nibbles,
 * representing the kernel's numeric version, w.x.y.z. These fields denote:
 *       w -- generation release number
 *       x -- major release
 *       y -- minor release
 *       z -- servicepack release
 * Each of these elements must therefore be in the range 0 to 15, inclusive.
 *
 * Part 2: The next most significant byte is used for a variety of flags and is
 * broken down as follows:
 *     Bits 7..0 - Cleared as the are currently unused.
 *
 * Part 3: The least significant byte is reserved for future use.
 */
#define KERNEL_VER_GENERATION(ver) ((ver >> 28) & 0x0F)
#define KERNEL_VER_MAJOR(ver) ((ver >> 24) & 0x0F)
#define KERNEL_VER_MINOR(ver) ((ver >> 20) & 0x0F)
#define KERNEL_VER_SERVICEPACK(ver) ((ver >> 16) & 0x0F)

/* return 8-bit flags */
#define KERNEL_VER_FLAGS(ver) ((ver >> 8) & 0xFF)

/* kernel version routines */

extern uint32_t kernel_version_get(void);

#endif /* _kernel_version__h_ */
