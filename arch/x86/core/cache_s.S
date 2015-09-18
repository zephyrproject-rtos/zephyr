/* cache_s.S - cache manipulation */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
/*
DESCRIPTION
This module contains functions for manipulating caches.
 */

#ifndef CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED

#define _ASMLANGUAGE
#include <arch/x86/asm.h>

	/* externs (internal APIs) */
	GTEXT(_SysCacheFlush)

/**
 *
 * @brief Flush a page to main memory
 *
 * This implementation flushes the whole cache.
 *
 * C signature:
 *
 *   void _SysCacheFlush (vaddr_t virt, size_t size)
 *
 * Both parameters are ignored in this implementation.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _SysCacheFlush)
	wbinvd
	ret

#endif /* !CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED */
