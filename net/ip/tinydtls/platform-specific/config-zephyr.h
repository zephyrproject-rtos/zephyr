/*
 * Copyright (c) 2016 Intel Corporation
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

#include <misc/__assert.h>

/* Do not include assert.h as that does not exists */
#undef HAVE_ASSERT_H

#ifndef NDEBUG
#ifndef assert
#define assert(...) __ASSERT(...)
#endif
#else
#ifndef assert
#define assert(...)
#endif
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BYTE_ORDER LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER BIG_ENDIAN
#else
#error "Unknown byte order"
#endif

#ifndef MAY_ALIAS
#define MAY_ALIAS __attribute__((__may_alias__))
#endif
