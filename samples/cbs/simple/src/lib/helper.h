/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

typedef struct {
	char msg[50];
	uint32_t counter;
} job_t;

void report_cbs_settings(void)
{
	printf("\n/////////////////////////////////////////////////////////////////////////////////"
	       "/////\n");
	printf("\nBoard:\t\t%s\n", CONFIG_BOARD_TARGET);
#ifdef CONFIG_CBS
	printf("[CBS]\t\tCBS enabled.\n");
#ifdef CONFIG_CBS_LOG
	printf("[CBS]\t\tCBS events logging: enabled.\n");
#else
	printf("[CBS]\t\tCBS events logging: disabled.\n");
#endif
#ifdef CONFIG_TIMEOUT_64BIT
	printf("[CBS]\t\tSYS 64-bit timeouts: supported.\n");
#else
	printf("[CBS]\t\tSYS 64-bit timeouts: not supported. using 32-bit API instead.\n");
#endif
#else
	printf("[CBS]\t\tCBS disabled.\n");
#endif
	printf("\n/////////////////////////////////////////////////////////////////////////////////"
	       "/////\n\n");
}

#endif
