/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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

#ifndef __INC_assert_h__
#define __INC_assert_h__

#include <misc/__assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
#ifndef assert
#define assert(test) __ASSERT_NO_MSG(test)
#endif
#else
#ifndef assert
#define assert(test)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif  /* __INC_assert_h__ */
