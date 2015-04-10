/* linker-tool-diab.h - diab toolchain linker defs */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
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


/*
DESCRIPTION
This header file defines the necessary macros used by the linker script for
use with the Diab linker.
*/

#ifndef __LINKER_TOOL_DIAB_H
#define __LINKER_TOOL_DIAB_H

/* see linker-tool-gcc.h for description of these macros */

#define GROUP_START(where)    GROUP : {
#define GROUP_END(where)      } > where

#define GROUP_LINK_IN(where)

#define GROUP_FOLLOWS_AT(where)

#define SECTION_PROLOGUE(name, options, align) \
	name options align :

#define SECTION_AT_PROLOGUE(name, options, align, addr) \
	name LOAD(addr) options align :

/* GCC-isms */
#define LONG(x)   STORE(x, 4)
#define LENGTH(x) SIZEOF(x)

/*
 * Diab linker does not drop empty sections, so we have to make sure the
 * pgalign section is not aligned on 4K when the MMU is off, and thus the
 * section is unused.
 */
  #define PGALIGN_ALIGN(x)

#define COMMON_SYMBOLS *[COMMON]

#endif /* !__LINKER_TOOL_DIAB_H */
