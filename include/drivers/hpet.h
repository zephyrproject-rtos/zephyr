/* hpet.h - High Precision Event Timer */

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

#ifndef __INChpeth
#define __INChpeth

#include <drivers/ioapic.h>

#if defined(CONFIG_HPET_TIMER_FALLING_EDGE)
#define HPET_IOAPIC_FLAGS  (IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_HPET_TIMER_RISING_EDGE)
#define HPET_IOAPIC_FLAGS  (IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_HPET_TIMER_LEVEL_HIGH)
#define HPET_IOAPIC_FLAGS  (IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_HPET_TIMER_LEVEL_LOW)
#define HPET_IOAPIC_FLAGS  (IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#endif /* __INChpeth */
