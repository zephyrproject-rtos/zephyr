/*
 * Copyright (c) 2019 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>

/* Amount of execution threads to create and run */
#define THREADS_NUM	16

/*
 * Amount of digits of Pi to calculate, must be a multiple of 4,
 * as used algorithm spits 4 digits on every iteration.
 */
#define DIGITS_NUM	240

#define LENGTH		((DIGITS_NUM / 4) * 14)
#define STACK_SIZE	((LENGTH * sizeof(int) + 1280))

#ifdef CONFIG_SMP
#define CORES_NUM	arch_num_cpus()
#else
#define CORES_NUM	1
#endif

static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);
static struct k_thread tthread[THREADS_NUM];
static char th_buffer[THREADS_NUM][DIGITS_NUM + 1];
static atomic_t th_counter = THREADS_NUM;

K_SEM_DEFINE(main_sem, 0, 1);

void test_thread(void *arg1, void *arg2, void *arg3)
{
	atomic_t *counter = (atomic_t *)arg1;
	char *buffer = (char *)arg2;

	ARG_UNUSED(arg3);

	/*
	 * Adapted and improved (for random number of digits) version of Pi
	 * calculation program initially proposed by Dik T. Winter as:
	 * -------------------------------->8--------------------------------
	 * int a=10000,b,c=2800,d,e,f[2801],g;main(){for(;b-c;)f[b++]=a/5;
	 * for(;d=0,g=c*2;c-=14,printf("%.4d",e+d/a),e=d%a)for(b=c;d+=f[b]*a,
	 * f[b]=d%--g,d/=g--,--b;d*=b);}
	 * -------------------------------->8--------------------------------
	 */
	#define NEW_BASE	10000
	#define ARRAY_INIT	2000

	int array[LENGTH + 1] = {};
	int carry = 0;
	int i, j;

	for (i = 0; i < LENGTH; i++) {
		array[i] = ARRAY_INIT;
	}

	for (i = LENGTH; i > 0; i -= 14) {
		int sum = 0, value;

		for (j = i; j > 0; --j) {
			sum = sum * j + NEW_BASE * array[j];
			array[j] = sum % (j * 2 - 1);
			sum /= j * 2 - 1;
		}

		value = carry + sum / NEW_BASE;
		carry = sum % NEW_BASE;

		/* Convert 4-digit int to string */
		sprintf(buffer, "%.4d", value);
		buffer += 4;
	}

	if (atomic_dec(counter) == 1) {
		k_sem_give(&main_sem);
	}
}

int main(void)
{
	uint32_t start_time, stop_time, cycles_spent, nanoseconds_spent;
	int i;

	printk("Calculate first %d digits of Pi independently by %d threads.\n",
	       DIGITS_NUM, THREADS_NUM);

	/* Capture initial time stamp */
	start_time = k_cycle_get_32();

	for (i = 0; i < THREADS_NUM; i++) {
		k_thread_create(&tthread[i], tstack[i], STACK_SIZE,
			       test_thread,
			       (void *)&th_counter, (void *)th_buffer[i], NULL,
			       K_PRIO_COOP(10), 0, K_NO_WAIT);
	}

	/* Wait for all workers to finish their calculations */
	k_sem_take(&main_sem, K_FOREVER);

	/* Capture final time stamp */
	stop_time = k_cycle_get_32();

	cycles_spent = stop_time - start_time;
	nanoseconds_spent = (uint32_t)k_cyc_to_ns_floor64(cycles_spent);

	for (i = 0; i < THREADS_NUM; i++) {
		printk("Pi value calculated by thread #%d: %s\n", i, th_buffer[i]);
	}

	printk("All %d threads executed by %d cores in %d msec\n", THREADS_NUM,
	       CORES_NUM, nanoseconds_spent / 1000 / 1000);
	return 0;
}
