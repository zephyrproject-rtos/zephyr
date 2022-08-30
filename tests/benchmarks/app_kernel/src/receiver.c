/* receiver.c */

/*
 * Copyright (c) 1997-2010,2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *	File Naming information.
 *	------------------------
 *	Files that end with:
 *		_B : Is a file that contains a benchmark function
 *		_R : Is a file that contains the receiver task
 *			 of a benchmark function
 */

#include "receiver.h"

char data_recv[MESSAGE_SIZE] = { 0 };

void dequtask(void);
void waittask(void);
void mailrecvtask(void);
void piperecvtask(void);

/**
 *
 * @brief Main function of the task that receives data in the test
 *
 */
void recvtask(void *p1, void *p2, void *p3)
{
	/* order must be compatible with master.c ! */
#ifdef FIFO_BENCH
	k_sem_take(&STARTRCV, K_FOREVER);
	dequtask();
#endif
#ifdef SEMA_BENCH
	k_sem_take(&STARTRCV, K_FOREVER);
	waittask();
#endif
#ifdef MAILBOX_BENCH
	k_sem_take(&STARTRCV, K_FOREVER);
	mailrecvtask();
#endif
#ifdef PIPE_BENCH
	k_sem_take(&STARTRCV, K_FOREVER);
	piperecvtask();
#endif
}
