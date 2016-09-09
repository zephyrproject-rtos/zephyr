/*
 * Copyright (c) 2016 Intel Corporation.
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

#include <zephyr.h>
#include <sys_io.h>
#include <misc/__assert.h>
#include <power.h>
#include <soc_power.h>

#define _DEEP_SLEEP_MODE		0xDEEBDEEB
#define _LOW_POWER_MODE			0xD02ED02E
#define _DEVICE_SUSPEND_ONLY_MODE	0x1D1E1D1E

/* GPS1 is reserved for PM use */
#define _GPS1	0xb0800104

/* Variables used to save CPU state */
uint64_t _pm_save_gdtr;
uint64_t _pm_save_idtr;
uint32_t _pm_save_esp;

#if (defined(CONFIG_SYS_POWER_LOW_POWER_STATE) || \
	defined(CONFIG_SYS_POWER_DEEP_SLEEP) || \
	defined(CONFIG_DEVICE_POWER_MANAGEMENT))
void _sys_soc_set_power_policy(uint32_t pm_policy)
{
	switch (pm_policy) {
	case SYS_PM_DEEP_SLEEP:
		sys_write32(_DEEP_SLEEP_MODE, _GPS1);
		break;
	case SYS_PM_LOW_POWER_STATE:
		sys_write32(_LOW_POWER_MODE, _GPS1);
		break;
	case SYS_PM_DEVICE_SUSPEND_ONLY:
		sys_write32(_DEVICE_SUSPEND_ONLY_MODE, _GPS1);
		break;
	case SYS_PM_NOT_HANDLED:
		sys_write32(0, _GPS1);
		break;
	default:
		__ASSERT(0, "Unknown PM policy");
		break;
	}
}

int _sys_soc_get_power_policy(void)
{
	uint32_t mode;

	mode = sys_read32(_GPS1);

	switch (mode) {
	case _DEEP_SLEEP_MODE:
		return SYS_PM_DEEP_SLEEP;
	case _LOW_POWER_MODE:
		return SYS_PM_LOW_POWER_STATE;
	case _DEVICE_SUSPEND_ONLY_MODE:
		return SYS_PM_DEVICE_SUSPEND_ONLY;
	}

	return SYS_PM_NOT_HANDLED;
}
#endif
