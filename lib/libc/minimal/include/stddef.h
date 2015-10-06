/* stddef.h */

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

#ifndef __INC_stddef_h__
#define __INC_stddef_h__

#if !defined(__size_t_defined)
#define __size_t_defined
typedef unsigned int  size_t;
#endif

#if !defined(__ptrdiff_t_defined)
#define __ptrdiff_t_defined
typedef int  ptrdiff_t;
#endif

#if !defined(NULL)
#define NULL (void *) 0
#endif

#define offsetof(type, member) ((size_t) (&((type *) NULL)->member))

#endif /* __INC_stddef_h__ */
