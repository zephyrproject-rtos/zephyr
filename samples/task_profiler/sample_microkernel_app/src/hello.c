/* hello.c - Hello World demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

#include <zephyr.h>
#include <stdio.h>

#ifdef CONFIG_MICROKERNEL

/*
 * Microkernel version of hello world demo has two tasks that utilize
 * semaphores and sleeps to take turns printing a greeting message at
 * a controlled rate.
 */


/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)


#define MYFIBER_STACK_SIZE 500
char __stack myfiber_stack[MYFIBER_STACK_SIZE];

void myfiber(int param1, int param2)
{
	volatile int i;

	printf("Hello fiber %d\n", param1);
	for (i = 0; i < 300; i++) {
	}
}

/*
 *
 * @param taskname    task identification string
 * @param mySem       task's own semaphore
 * @param otherSem    other task's semaphore
 *
 */
void helloLoop(const char *taskname, ksem_t mySem, ksem_t otherSem)
{
	int count = 0;

	while (1) {
		task_sem_take(mySem, TICKS_UNLIMITED);

		if (mySem == TASKASEM) {
			task_fiber_start(&myfiber_stack[0], MYFIBER_STACK_SIZE, myfiber, count,
					 0, 1, 0);
			count++;
		}

		/* say "hello" */
		printf("%s: Hello World!\n", taskname);

		/* wait a while, then let other task have a turn */
		task_sleep(SLEEPTICKS);
		task_sem_give(otherSem);
	}
}

void taskA(void)
{
	/* taskA gives its own semaphore, allowing it to say hello right away */
	task_sem_give(TASKASEM);

	/* invoke routine that allows task to ping-pong hello messages with taskB */
	helloLoop(__func__, TASKASEM, TASKBSEM);
}

void taskB(void)
{
	/* invoke routine that allows task to ping-pong hello messages with taskA */
	helloLoop(__func__, TASKBSEM, TASKASEM);
}


#endif /* CONFIG_MICROKERNEL */
