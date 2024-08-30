/*
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX - License - Identifier : Apache-2.0
 */
#include <cv64a6.h>
#include <stdio.h>


int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	z_cv64a6_finish_test(0);

	return 0;
}
