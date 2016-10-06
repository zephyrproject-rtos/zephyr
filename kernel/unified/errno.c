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

/** @file
 *
 * @brief Per-thread errno accessor function
 *
 * Allow accessing the errno for the current thread without involving the
 * context switching.
 */

#include <nano_private.h>

#ifdef CONFIG_KERNEL_V2
/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 */
const int _k_neg_eagain = -EAGAIN;
#endif

#ifdef CONFIG_ERRNO
int *_get_errno(void)
{
	return &_nanokernel.current->errno_var;
}
#endif
