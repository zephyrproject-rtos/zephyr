/* ipi_console.c - Console messages to/from another processor */

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

#ifndef _IPI_CONSOLE_H_
#define _IPI_CONSOLE_H_

#include <nanokernel.h>
#include <device.h>
#include <misc/ring_buffer.h>

#define IPI_CONSOLE_STDOUT	(1 << 0)
#define IPI_CONSOLE_PRINTK	(1 << 1)

/* Good way to determine these numbers other than trial-and-error?
 * using printf() in the fiber seems to require a lot more stack space */
#define IPI_CONSOLE_STACK_SIZE		512
#define IPI_CONSOLE_PRI			2

struct ipi_console_receiver_config_info {
	/** Name of the low-level IPI driver to bind to */
	char *bind_to;

	/** Stack for the receiver's fiber, which prints out messages as
	 * they come in. Should be sized IPI_CONSOLE_STACK_SIZE */
	char *fiber_stack;

	/** Ring buffer data area for stashing characters from the interrupt
	 * callback */
	uint32_t *ring_buf_data;

	/** Size of ring_buf_data in 32-bit chunks */
	unsigned int rb_size32;

	/** Line buffer for incoming messages, characters accumulate here
	 * and then are sent to printk() once full (including a trailing NULL)
	 * or a carriage return seen */
	char *line_buf;

	/** Size in bytes of the line buffer. Must be at least 2 */
	unsigned int lb_size;

	/** Destination for received console messages, one of
	 * IPI_CONSOLE_STDOUT or IPI_CONSOLE_PRINTK */
	unsigned int flags;
};

struct ipi_console_receiver_runtime_data {
	/** Buffer for received bytes from the low-level IPI device */
	struct ring_buf rb;

	/** Semaphore to wake up the fiber to print out messages */
	struct nano_sem sem;

	/** pointer to the bound low-level IPI device */
	struct device *ipi_device;
};

struct ipi_console_sender_config_info {
	/** Name of the low-level driver to bind to */
	char *bind_to;

	/** Source of messages to forward, hooks will be installed.
	 * Can be IPI_CONSOLE_STDOUT, IPI_CONSOLE_PRINTK, or both */
	int flags;
};

#if CONFIG_IPI_CONSOLE_RECEIVER
int ipi_console_receiver_init(struct device *d);
#endif

#if CONFIG_IPI_CONSOLE_SENDER
int ipi_console_sender_init(struct device *d);
#endif

#endif /* _IPI_CONSOLE_H_ */
