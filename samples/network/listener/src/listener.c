/* listener.c - Networking demo */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <net/net_core.h>
#include <net/net_socket.h>


#ifdef CONFIG_MICROKERNEL

/*
 * Microkernel version of hello world demo has two tasks that utilize
 * semaphores and sleeps to take turns printing a greeting message at
 * a controlled rate.
 */

#include <zephyr.h>

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define TASK_IPADDR { { { 0xaa,0xaa,0,0,0,0,0,0,0,0,0,0,0,0,0,0x2 } } }

static struct net_context *get_context(const struct net_addr *addr)
{
	struct net_context *ctx;
	static struct net_addr any_addr;
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	ctx = net_context_get(IPPROTO_UDP,
			      &any_addr, 0,
			      addr, 4242);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return NULL;
	}

	return ctx;
}

/*
*
* \param taskname    task identification string
* \param mySem       task's own semaphore
* \param otherSem    other task's semaphore
*
*/
void helloLoop(const char *taskname, ksem_t mySem, ksem_t otherSem)
{
	static struct in6_addr in6addr_my = TASK_IPADDR;  /* aaaa::2 */
	static struct net_addr my_addr;
	static struct net_context *ctx;
	struct net_buf *buf;

	net_init();

	if (!ctx) {
		my_addr.in6_addr = in6addr_my;
		my_addr.family = AF_INET6;

		ctx = get_context(&my_addr);
	}

	while (1) {
		task_sem_take_wait(mySem);

		buf = net_receive(ctx);
		if (buf) {
			PRINT("%s: received %d bytes\n", taskname,
			      uip_appdatalen(buf));
			net_buf_put(buf);
		}

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
	helloLoop(__FUNCTION__, TASKASEM, TASKBSEM);
}

void taskB(void)
{
	/* invoke routine that allows task to ping-pong hello messages with taskA */
	helloLoop(__FUNCTION__, TASKBSEM, TASKASEM);
}

#else /*  CONFIG_NANOKERNEL */

/*
 * Nanokernel version of hello world demo has a task and a fiber that utilize
 * semaphores and timers to take turns printing a greeting message at
 * a controlled rate.
 */

#include <nanokernel.h>

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  500
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)

#define STACKSIZE 2000

char fiberStack[STACKSIZE];

struct nano_sem nanoSemTask;
struct nano_sem nanoSemFiber;

static struct net_addr any_addr;
static struct net_addr my_addr;

void fiberEntry(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};
	struct net_context *ctx;
	struct net_buf *buf;

#define MYADDR { { { 0xaa,0xaa,0,0,0,0,0,0,0,0,0,0,0,0,0,2 } } }
	static struct in6_addr in6addr_my = MYADDR;  /* aaaa::2 */
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;

	ctx = net_context_get(IPPROTO_UDP,
			      &any_addr, 0,
			      &my_addr, 4242);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	nano_sem_init (&nanoSemFiber);
	nano_timer_init (&timer, data);

	while (1) {
		/* wait for task to let us have a turn */
		nano_fiber_sem_take_wait (&nanoSemFiber);

		buf = net_receive(ctx);
		if (buf) {
			PRINT("%s: received %d bytes\n", __FUNCTION__,
				uip_appdatalen(buf));
			net_buf_put(buf);
		}

		/* wait a while, then let task have a turn */
		nano_fiber_timer_start (&timer, SLEEPTICKS);
		nano_fiber_timer_wait (&timer);
		nano_fiber_sem_give (&nanoSemTask);
	}
}

void main(void)
{
	struct nano_timer timer;
	uint32_t data[2] = {0, 0};

	net_init();

	PRINT("%s: run listener\n", __FUNCTION__);

	task_fiber_start (&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) fiberEntry, 0, 0, 7, 0);

	nano_sem_init(&nanoSemTask);
	nano_timer_init(&timer, data);

	while (1) {
		/* wait a while, then let fiber have a turn */
		nano_task_timer_start(&timer, SLEEPTICKS);
		nano_task_timer_wait(&timer);
		nano_task_sem_give(&nanoSemFiber);

		/* now wait for fiber to let us have a turn */
		nano_task_sem_take_wait(&nanoSemTask);
	}
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
