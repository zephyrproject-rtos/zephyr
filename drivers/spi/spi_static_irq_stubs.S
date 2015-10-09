/*
 * Copyright (c) 2012-2015, Wind River Systems, Inc.
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

#ifdef CONFIG_X86
#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#endif

#if defined(CONFIG_SPI_INTEL_PORT_0)
	ioapic_mkstub spi_intel_irq_port_0 spi_intel_isr spi_intel_isr_port_0
#endif /* CONFIG_SPI_INTEL_PORT_0 */
#if defined(CONFIG_SPI_INTEL_PORT_1)
	ioapic_mkstub spi_intel_irq_port_1 spi_intel_isr spi_intel_isr_port_1
#endif /* CONFIG_SPI_INTEL_PORT_1 */

#if defined(CONFIG_SPI_DW_PORT_0)
#ifdef CONFIG_IOAPIC
	ioapic_mkstub spi_dw_irq_port_0 spi_dw_isr spi_dw_isr_port_0
#endif
#endif
#if defined(CONFIG_SPI_DW_PORT_1)
#ifdef CONFIG_IOAPIC
	ioapic_mkstub spi_dw_irq_port_1 spi_dw_isr spi_dw_isr_port_1
#endif
#endif

