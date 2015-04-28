/* kmail.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
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
 * 3) Neither the name of Wind River Systems nor the names of its contributors
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

#ifndef KMAIL_H
#define KMAIL_H

/* includes */

#include <microkernel/k_struct.h>

/* defines */

/*******************************************************************************
*
* ISASYNCMSG - determines if mailbox message is synchronous or asynchronous
*
* Returns a non-zero value if the specified message contains a valid pool ID,
* indicating that it is an asynchronous message.
*/

#define ISASYNCMSG(message) ((message)->tx_block.poolid != 0)

/* externs */

extern void _k_mbox_send_reply(struct k_args *Writer);
extern void _k_mbox_send_request(struct k_args *Writer);
extern void _k_mbox_send_ack(struct k_args *Writer);
extern void K_recvack(struct k_args *Reader);
extern void K_recvrpl(struct k_args *Reader);
extern void K_recvreq(struct k_args *Reader);
extern void K_recvdata(struct k_args *Starter);

#endif
