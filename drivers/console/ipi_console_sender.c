/* ipi_console_send.c - Console messages to another processor */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <nanokernel.h>
#include <misc/printk.h>
#include <ipi.h>
#include <console/ipi_console.h>

static struct device *ipi_console_device;

static int consoleOut(int character)
{
	if (character == '\r') {
		return character;
	}

	/* We just stash the character into the id field and don't supply
	 * any extra data */
	ipi_send(ipi_console_device, 1, character, NULL, 0);

	return character;
}

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

int ipi_console_sender_init(struct device *d)
{
	struct ipi_console_sender_config_info *config_info;

	config_info = d->config->config_info;
	ipi_console_device = device_get_binding(config_info->bind_to);

	if (!ipi_console_device) {
		printk("unable to bind IPI console sender to '%s'\n",
		       config_info->bind_to);
		return DEV_INVALID_CONF;
	}

	if (config_info->flags & IPI_CONSOLE_STDOUT) {
		__stdout_hook_install(consoleOut);
	}
	if (config_info->flags & IPI_CONSOLE_PRINTK) {
		__printk_hook_install(consoleOut);
	}

	return DEV_OK;
}

