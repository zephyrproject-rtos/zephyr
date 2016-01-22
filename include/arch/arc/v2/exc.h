/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * @brief ARCv2 public exception handling
 *
 * ARC-specific nanokernel exception handling interface. Included by ARC/arch.h.
 */

#ifndef _ARCH_ARC_V2_EXC_H_
#define _ARCH_ARC_V2_EXC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
#else
struct __esf {
	/* XXX - not defined yet */
	int placeholder;
};

typedef struct __esf NANO_ESF;
extern const NANO_ESF _default_esf;
#endif

#ifdef __cplusplus
}
#endif


#endif /* _ARCH_ARC_V2_EXC_H_ */
