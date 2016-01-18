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

/**
 * @file x86-specific reboot functionalities
 *
 * @details Implements the required 'arch' sub-APIs.
 */

#include <nanokernel.h>
#include <misc/reboot.h>

static inline void cold_reboot(void)
{
	uint8_t reset_value = SYS_X86_RST_CNT_CPU_RST | SYS_X86_RST_CNT_SYS_RST |
							SYS_X86_RST_CNT_FULL_RST;
	sys_out8(reset_value, SYS_X86_RST_CNT_REG);
}

void sys_arch_reboot(int type)
{
	switch (type) {
	case SYS_REBOOT_COLD:
		cold_reboot();
		break;
	default:
		/* do nothing */
		break;
	}
}
