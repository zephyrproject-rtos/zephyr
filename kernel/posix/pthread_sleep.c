/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
/*
 *  ======== sleep ========
 */
unsigned int sleep(unsigned int seconds)
{
	k_sleep(seconds * MSEC_PER_SEC);
	return 0;
}

/*
 *  ======== sleep ========
 */
unsigned int msleep(unsigned int mseconds)
{
	k_sleep(mseconds);
	return 0;
}


/*
 *  ======== usleep ========
 */
int usleep(useconds_t useconds)
{
	k_busy_wait(useconds);
	return 0;
}

