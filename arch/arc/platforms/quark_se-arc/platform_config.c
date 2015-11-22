/*
 * Copyright (c) 2015 Intel Corporation
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

#include <device.h>
#include <init.h>
#include "board.h"

#if CONFIG_IPI_QUARK_SE
#include <ipi.h>
#include <ipi/ipi_quark_se.h>

IRQ_CONNECT_STATIC(quark_se_ipi, QUARK_SE_IPI_INTERRUPT,
		   QUARK_SE_IPI_INTERRUPT_PRI, quark_se_ipi_isr, NULL);

static int arc_quark_se_ipi_init(void)
{
	IRQ_CONFIG(quark_se_ipi, QUARK_SE_IPI_INTERRUPT,
		   QUARK_SE_IPI_INTERRUPT_PRI);
	irq_enable(QUARK_SE_IPI_INTERRUPT);
	return DEV_OK;
}

static struct quark_se_ipi_controller_config_info ipi_controller_config = {
	.controller_init = arc_quark_se_ipi_init
};
DECLARE_DEVICE_INIT_CONFIG(quark_se_ipi, "", quark_se_ipi_controller_initialize,
			   &ipi_controller_config);
pre_kernel_late_init(quark_se_ipi, NULL);

#if CONFIG_IPI_CONSOLE_SENDER
#include <console/ipi_console.h>
QUARK_SE_IPI_DEFINE(quark_se_ipi4, 4, QUARK_SE_IPI_OUTBOUND);

struct ipi_console_sender_config_info quark_se_ipi_sender_config = {
	.bind_to = "quark_se_ipi4",
	.flags = IPI_CONSOLE_PRINTK | IPI_CONSOLE_STDOUT,
};
DECLARE_DEVICE_INIT_CONFIG(ipi_console, "ipi_console",
			   ipi_console_sender_init,
			   &quark_se_ipi_sender_config);
nano_early_init(ipi_console, NULL);

#endif /* CONFIG_IPI_CONSOLE_SENDER */
#endif /* CONFIG_IPI_QUARK_SE */
