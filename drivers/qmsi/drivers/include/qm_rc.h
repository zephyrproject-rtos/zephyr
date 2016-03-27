/*
 * Copyright (c) 2015, Intel Corporation
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *     @(#)errno.h   7.1 (Berkeley) 6/4/86
 */

#ifndef __QM_RC_H__
#define __QM_RC_H__

/* Return codes */
typedef enum {
	QM_RC_OK = 0,
	QM_RC_ERROR,       /* Unknown/unclassified error */
	QM_RC_EINVAL = 22, /* Invalid argument, matches Berkeley equivalent */
	/* UART */
	QM_RC_UART_RX_OE = 0x80, /* Receiver overrun */
	QM_RC_UART_RX_FE,	/* Framing error */
	QM_RC_UART_RX_PE,	/* Parity error */
	QM_RC_UART_RX_BI,	/* Break interrupt */
	/* I2C */
	QM_RC_I2C_ARB_LOST = 0x100, /* Arbitration lost */
	QM_RC_I2C_NAK,		    /* Missing acknowledge */
	/* SPI */
	QM_RC_SPI_RX_OE = 0x120,    /* RX Fifo Overflow error */
} qm_rc_t;

#endif /* __QM_RC_H__ */
