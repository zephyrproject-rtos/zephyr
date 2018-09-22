/*  main.h */

/*
 *   SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_H
#define MAIN_H

#include <zephyr.h>
#include <misc/printk.h>
#include <kernel_structs.h>
#include <string.h>
#include <stdlib.h>

#include <app_memory/app_memdomain.h>
#include <misc/util.h>

#if defined(CONFIG_ARC)
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#endif

void enc(void);
void pt(void);
void ct(void);

#define _app_user_d _app_dmem(part0)
#define _app_user_b _app_bmem(part0)

#define _app_red_d _app_dmem(part1)
#define _app_red_b _app_bmem(part1)

#define _app_enc_d _app_dmem(part2)
#define _app_enc_b _app_bmem(part2)

#define _app_blk_d _app_dmem(part3)
#define _app_blk_b _app_bmem(part3)

#define _app_ct_d _app_dmem(part4)
#define _app_ct_b _app_bmem(part4)

/*
 * Constant
 */

#define STACKSIZE 1024

#define PRIORITY 7

#define BYTE unsigned char


#define START_WHEEL {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, \
	12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25}
#define START_WHEEL2 {6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, \
	17, 18, 19, 20, 21, 22, 23, 24, 25, 5, 0, 4, 1, 3, 2}
#define REFLECT {1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, \
	15, 14, 17, 16, 19, 18, 21, 20, 23, 22, 25, 24}


#endif
