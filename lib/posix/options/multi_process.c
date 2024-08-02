/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/unistd.h>

pid_t getpid(void)
{
	/*
	 * To maintain compatibility with some other POSIX operating systems,
	 * a PID of zero is used to indicate that the process exists in another namespace.
	 * PID zero is also used by the scheduler in some cases.
	 * PID one is usually reserved for the init process.
	 * Also note, that negative PIDs may be used by kill()
	 * to send signals to process groups in some implementations.
	 *
	 * At the moment, getpid just returns an arbitrary number >= 2
	 */

	return 42;
}
