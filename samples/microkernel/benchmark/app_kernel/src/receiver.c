/* receiver.c */

/*
 * Copyright (c) 1997-2010,2013-2014 Wind River Systems, Inc.
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

/*
	File Naming information.
	------------------------
	Files that end with:
		_B : Is a file that contains a benchmark function
		_R : Is a file that contains the receiver task
			 of a benchmark function
 */

#include "receiver.h"

char data_recv[OCTET_TO_SIZEOFUNIT(MESSAGE_SIZE)] = { 0 };

void dequtask(void);
void waittask(void);
void mailrecvtask(void);
void piperecvtask(void);

/**
 *
 * @brief Main function of the task that receives data in the test
 *
 * @return N/A
 *
 * \NOMANUAL
 */

void recvtask(void)
{
	/* order must be compatible with master.c ! */
#ifdef FIFO_BENCH
	task_sem_take_wait(STARTRCV);
	dequtask();
#endif
#ifdef SEMA_BENCH
	task_sem_take_wait(STARTRCV);
	waittask();
#endif
#ifdef MAILBOX_BENCH
	task_sem_take_wait(STARTRCV);
	mailrecvtask();
#endif
#ifdef PIPE_BENCH
	task_sem_take_wait(STARTRCV);
	piperecvtask();
#endif
}
