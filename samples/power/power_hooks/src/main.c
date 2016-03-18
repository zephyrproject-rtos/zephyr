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

#include <zephyr.h>
#include <power.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

void main(void)
{
	PRINT("Power management template!\n");
}

/**
 * Enter suspend or low power idle state
 *
 * This routine checks if the upcoming kernel idle interval is sufficient to
 * justify entering soc suspend mode or turn off peripherals. If it is, then
 * the routine performs the operations necessary to conserve power; if not,
 * then the routine returns immediately without suspending.
 *
 * param: upcoming kernel idle time ticks
 *
 * return: non-zero value if handled; zero to let kernel do the idling
 */

int _sys_soc_suspend(int32_t ticks)
{
	return 0;
}

/**
 * Exit suspend or low power idle state
 *
 * This routine is called after a suspend state is exited.  This would
 * resume execution at the point system suspended if the soc was suspended.
 * If soc was not suspended, then this function will return and kernel will
 * continue its cold boot process.
 *
 * If any states were altered at an earlier call to suspend, then they
 * should be restored in this function.
 *
 * Will not return to caller if soc was suspended
 */
void _sys_soc_resume(void)
{
}
