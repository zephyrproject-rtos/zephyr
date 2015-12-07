/* receiver.c */

/*
 * Copyright (c) 1997-2010,2013-2014 Wind River Systems, Inc.
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
