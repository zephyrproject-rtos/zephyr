/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This sample application is roughly based on the sample code in Linux
 * manpage for eventfd().
 */
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define fatal(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* As Zephyr doesn't provide command-line args, emulate them. */
char *input_argv[] = {"argv0", "1", "2", "3", "4"};

int efd;
int g_argc;
char **g_argv;

void writer(void)
{
	int j;
	uint64_t u;
	ssize_t s;

	for (j = 1; j < g_argc; j++) {
		printf("Writing %s to efd\n", g_argv[j]);
		u = strtoull(g_argv[j], NULL, 0);
		s = write(efd, &u, sizeof(uint64_t));
		if (s != sizeof(uint64_t)) {
			fatal("write");
		}
	}
	printf("Completed write loop\n");
}

void reader(void)
{
	uint64_t u;
	ssize_t s;

	sleep(1);

	printf("About to read\n");
	s = read(efd, &u, sizeof(uint64_t));
	if (s != sizeof(uint64_t)) {
		fatal("read");
	}
	printf("Read %llu (0x%llx) from efd\n",
	       (unsigned long long)u, (unsigned long long)u);
}

int main(int argc, char *argv[])
{
	argv = input_argv;
	argc = sizeof(input_argv) / sizeof(input_argv[0]);

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <num>...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	g_argc = argc;
	g_argv = argv;

	efd = eventfd(0, 0);
	if (efd == -1) {
		fatal("eventfd");
	}

	writer();
	reader();

	printf("Finished\n");

	return 0;
}
