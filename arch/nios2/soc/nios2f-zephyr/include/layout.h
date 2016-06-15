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

#include <system.h>

#define _RESET_VECTOR		ALT_CPU_RESET_ADDR
#define	_EXC_VECTOR		ALT_CPU_EXCEPTION_ADDR

#define _ROM_ADDR		ONCHIP_FLASH_0_DATA_BASE
#define _ROM_SIZE		ONCHIP_FLASH_0_DATA_SPAN

#define _RAM_ADDR		ONCHIP_MEMORY2_0_BASE
#define _RAM_SIZE		ONCHIP_MEMORY2_0_SPAN
