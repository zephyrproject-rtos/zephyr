/*
 * Copyright (c) Intel Corporation
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

#ifndef _SHARED_MEM_H_
#define _SHARED_MEM_H_

/* Start of the shared 80K RAM */
#define SHARED_ADDR_START 0xA8000000

struct shared_mem {
	uint32_t arc_start;
	uint32_t flags;
};

#define ARC_READY	(1 << 0)

#define shared_data ((volatile struct shared_mem *) SHARED_ADDR_START)

#endif
