/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This generates the following output on qemu_riscv64:
 *
 *     *** Booting Zephyr OS build v3.6.0-677-g999a2edd1f1e ***
 *     Hello World! qemu_riscv64
 *     func1
 *     err
 *     E:
 *     E:  mcause: 2, Illegal instruction
 *     E:   mtval: 77777777
 *     E:      a0: 0000000000000000    t0: 0000000000000000
 *     E:      a1: 0000000010000000    t1: 0000000000000000
 *     E:      a2: 000000000000000a    t2: 0000000000000000
 *     E:      a3: 000000008000c210    t3: 0000000000000000
 *     E:      a4: 000000008000c210    t4: 0000000000000000
 *     E:      a5: 000000008000c210    t5: 0000000000000000
 *     E:      a6: 0000000000000000    t6: 0000000000000000
 *     E:      a7: 0000000000000000
 *     E:      ra: 0000000080000446
 *     E:    mepc: 0000000080000446
 *     E: mstatus: 0000000a00021880
 *     E:
 *     E: Call trace begin:
 *     E:       0: fp: 000000008000eb40   ra: 000000008000047a
 *     E:       1: fp: 000000008000eb60   ra: 000000008000049e
 *     E:       2: fp: 000000008000eb70   ra: 00000000800023b0
 *     E:       3: fp: 000000008000eb80   ra: 000000008000082e
 *     E: Call trace end
 *     E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
 *     E: Current thread: 0x8000c210 (main)
 *     E: Halting system
 */

#include <stdio.h>
#include <stdbool.h>

static void err(void)
{
	printf("%s\n", __func__);
	__asm__ volatile (".word 0x77777777");
}

static void func1(bool a)
{
	printf("%s\n", __func__);
	if (a) {
		err();
	}
}

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

	func1(true);

	return 0;
}
