/*
 * Copyright (c) 2015, Intel Corporation
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
 * @file
 * @brief Ethernet interrupt stubs
 * This module contains the static interrupt stubs for Ethernet drivers.
 */

#define _ASMLANGUAGE

#ifdef CONFIG_X86
#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#endif

#if defined(CONFIG_ETH_DW)
#if CONFIG_ETH_DW_0
	ioapic_mkstub eth_dw_0 eth_dw_isr eth_dw_isr_0
#endif /* CONFIG_ETH_DW_0 */
#endif /* CONFIG_ETH_DW */
