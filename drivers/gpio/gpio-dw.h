/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <gpio.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#define CONFIG_GPIO_DW_BITS 32
extern int gpio_initialize_dw(struct device *port);
typedef void (*gpio_config_irq_t)(struct device *port);

struct gpio_config_dw {
	uint32_t base_addr;
	uint32_t bits;
	uint32_t irq_num;
#ifdef CONFIG_PCI
	struct pci_dev_info  pci_dev;
#endif /* CONFIG_PCI */
	gpio_config_irq_t config_func;

#ifdef CONFIG_GPIO_DW_SHARED_IRQ
	char *shared_irq_dev_name;
#endif /* CONFIG_GPIO_DW_SHARED_IRQ */
};

struct gpio_runtime_dw {
	gpio_callback_t callback;
	uint32_t enabled_callbacks;
	uint8_t port_callback;
};
