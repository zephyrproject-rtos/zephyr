/*
 * Copyright (c) 2016 Linaro Limited
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
  * @brief Test to verify the behavior of CONFIG_RUNTIME_NMI at runtime.
  */

#include <zephyr.h>
#include <misc/printk.h>
#include <misc/reboot.h>

#include <tc_util.h>

extern void _NmiHandlerSet(void (*pHandler)(void));

static void nmi_test_isr(void)
{
	printk("NMI received (test_handler_isr)! Rebooting...\n");
	/* ISR triggered correctly: test passed! */
	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}

void main(void)
{
	uint32_t i = 0;

	/* Configure the NMI isr */
	_NmiHandlerSet(nmi_test_isr);

	for (i = 0; i < 10; i++) {
		printk("Trigger NMI in 10s: %d s\n", i);
		k_sleep(1000);
	}

	/* Trigger NMI: Should fire immediately */
	_ScbNmiPend();
}
