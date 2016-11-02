/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <toolchain.h>

#if !defined(ALIGNED)
#define ALIGNED(x) __aligned(x)
#endif

#define ALIGN4(x)     (((uint32_t)(x)+3) & (~((uint32_t)3)))

#define DOUBLE_BUFFER_SIZE 2
#define TRIPLE_BUFFER_SIZE 3

#define BDADDR_SIZE 6

#endif /* _DEFINES_H_ */
