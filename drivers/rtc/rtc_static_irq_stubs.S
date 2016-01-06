/*
 * Copyright (c) 2015, Intel Corportation.
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


#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>

#if defined(CONFIG_RTC_DW)
#if defined(CONFIG_IOAPIC) || defined(CONFIG_MVIC)
	ioapic_mkstub rtc rtc_dw_isr rtc_dw_isr_dev
#endif
#endif

#if defined(CONFIG_RTC_QMSI)
#if defined(CONFIG_IOAPIC) || defined(CONFIG_MVIC)
	ioapic_mkstub rtc qm_rtc_isr_0 0
#endif
#endif /* CONFIG_RTC_QMSI */

/* externs (internal APIs) */

GTEXT(_IntEnt)
GTEXT(_IntExit)
