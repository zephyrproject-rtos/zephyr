/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
uint8_t data=23;
uint8_t data1=34;
const uint32_t data2=45;
int main(void)
{
	uint8_t local=90;
	printf("Hello World! %s\n", CONFIG_BOARD);
	return 0;
}