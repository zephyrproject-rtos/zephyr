/*
 * Copyright (c) 2018 Juan Manuel Torres Palma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <semaphore.h>

/* TODO: Global data structure to keep track of named semaphores */

int sem_init(sem_t *sem, int pshared, unsigned value)
{
	ARG_UNUSED(pshared); /* In Zephyr, process is not defined */
	k_sem_init(sem, value, SEM_VALUE_MAX);
	return 0;
}

int sem_destroy(sem_t *sem)
{
	if (k_sem_count_get(sem) != 0) {
		errno = EBUSY;
		return -1;
	}
	return 0;
}

int sem_getvalue(sem_t *sem, int *sval)
{
	*sval = (int)k_sem_count_get(sem);
	return 0;
}

int sem_post(sem_t *sem)
{
	/* TODO: Care about process scheduling when unlocking */
	k_sem_give(sem);
	return 0;
}

int sem_wait(sem_t *sem)
{
	k_sem_take(sem, K_FOREVER);
	return 0;
}

int sem_trywait(sem_t *sem)
{
	if (k_sem_take(sem, K_NO_WAIT) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}

int sem_timedwait(sem_t *sem, const struct timespec *tv)
{
	return 0;
}

sem_t *sem_open(const char *name, int oflag, ...)
{
	return 0;
}

int sem_close(sem_t *sem)
{
	return 0;
}

int sem_unlink(const char *name)
{
	return 0;
}
