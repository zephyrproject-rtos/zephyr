/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <getopt.h>

#ifdef CONFIG_POSIX_API

#include <arpa/inet.h>
#include <dirent.h>
#include <mqueue.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#else

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/dirent.h>
#include <zephyr/posix/mqueue.h>
#include <zephyr/posix/net/if.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/posix/netinet/tcp.h>
#include <zephyr/posix/poll.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/sched.h>
#include <zephyr/posix/semaphore.h>
#include <zephyr/posix/signal.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/posix/sys/ioctl.h>
#include <zephyr/posix/sys/select.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/unistd.h>

#endif
