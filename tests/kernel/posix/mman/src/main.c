/*
 * Copyright (c) 2018 Juan Manuel Torres Palma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include <kernel.h>
#include <sys/mman.h>
#include <misc/printk.h>

#define NTH         2
#define STACKSZ     256
#define SHARED_STR  "Hello thread"
#define BUF_SZ      (sizeof(SHARED_STR))

/*
 * One is used to hold reader from reading before writing, and another is to
 * prevent writer from removing it before reader could access.
 */
K_SEM_DEFINE(lock_reader, 0, NTH);
K_SEM_DEFINE(lock_writer, 0, NTH);

static const char mem_name[] = "/shrd_reg";
static int result = 0;
static int th_status[NTH];

void *read_thread(void *vid)
{
	int id = (int)vid;
	int fd;
	char *buff;
	const char expected_str[] = SHARED_STR;

	fd = shm_open(mem_name, O_CREAT | O_RDONLY, S_IRWXU);
	buff = mmap(0, BUF_SZ, PROT_READ, MAP_SHARED, fd, 0);

	k_sem_take(&lock_reader, K_FOREVER); /* Wait for writer thread */
	result = strncmp(expected_str, buff, BUF_SZ);
	k_sem_give(&lock_writer);

	munmap(buff, BUF_SZ);
	close(fd);

	th_status[id] = 1;

	return NULL;
}

void *write_thread(void *vid)
{
	int id = (int)vid;
	int fd;
	char *buff;
	const char str[] = SHARED_STR;

	fd = shm_open(mem_name, O_CREAT | O_RDWR, S_IRWXU);
	buff = mmap(0, BUF_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(buff, str, BUF_SZ);

	k_sem_give(&lock_reader); /* Data ready */
	munmap(buff, BUF_SZ);
	close(fd);

	k_sem_take(&lock_writer, K_FOREVER); /* Wait for reader to open memory region */
	shm_unlink(mem_name);

	th_status[id] = 1;

	return NULL;
}

int wait_for_threads(void)
{
	int i;

	for (i = 0; i < NTH; i++) {
		if (!th_status[i]) {
			return 1;
		}
	}

	return 0;
}

/*
 * Returns 0 if strings are equal
 */
int check_result(void)
{
	return result;
}

void main(void)
{
	int status = TC_FAIL;

	TC_START("POSIX shared memory API\n");

	while (wait_for_threads()) {
		k_sleep(100);
	}

	if (check_result()) {
		TC_END_REPORT(status);
	}

	printk("Test finished\n");

	status = TC_PASS;
}

K_THREAD_DEFINE(thread0, STACKSZ, write_thread, (void *)0, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

K_THREAD_DEFINE(thread1, STACKSZ, read_thread, (void *)1, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
