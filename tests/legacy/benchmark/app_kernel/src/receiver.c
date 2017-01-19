/* receiver.c */

/*
 * Copyright (c) 1997-2010,2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
 */
void recvtask(void)
{
	/* order must be compatible with master.c ! */
#ifdef FIFO_BENCH
	task_sem_take(STARTRCV, TICKS_UNLIMITED);
	dequtask();
#endif
#ifdef SEMA_BENCH
	task_sem_take(STARTRCV, TICKS_UNLIMITED);
	waittask();
#endif
#ifdef MAILBOX_BENCH
	task_sem_take(STARTRCV, TICKS_UNLIMITED);
	mailrecvtask();
#endif
#ifdef PIPE_BENCH
	task_sem_take(STARTRCV, TICKS_UNLIMITED);
	piperecvtask();
#endif
}
