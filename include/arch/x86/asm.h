/* asm.h - x86 tool dependent headers */

/*
 * Copyright (c) 2007-2014 Wind River Systems, Inc.
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

#ifndef __INCsysX86Asmh
#define __INCsysX86Asmh


#include <toolchain.h>
#include <sections.h>

/* offsets from stack pointer to function arguments */

#define SP_ARG0         0
#define SP_ARG1         4
#define SP_ARG2         8
#define SP_ARG3         12
#define SP_ARG4         16
#define SP_ARG5         20
#define SP_ARG6         24
#define SP_ARG7         28
#define SP_ARG8         32

#endif /* __INCsysX86Asmh */
