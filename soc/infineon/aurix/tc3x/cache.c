/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void arch_dcache_enable(void)
{
	__asm__ volatile("   mtcr 0x9040, %0\n"
			 "   isync\n"
			 :
			 : "d"(0));
}

void arch_dcache_disable(void)
{
	__asm__ volatile("   mtcr 0x9040, %0\n"
			 "   isync\n"
			 :
			 : "d"(2));
}

void arch_icache_enable(void)
{
	__asm__ volatile("   mtcr 0x920C, %0\n"
			 "   isync\n"
			 :
			 : "d"(0));
}

void arch_icache_disable(void)
{
	__asm__ volatile("   mtcr 0x920C, %0\n"
			 "   isync\n"
			 :
			 : "d"(2));
}

int arch_icache_invd_all(void)
{
	unsigned int pcon;

	__asm__ volatile("   mtcr 0x9204, %0\n"
			 "   isync\n"
			 :
			 : "d"(1));
	do {
		__asm__ volatile("   mfcr %0, 0x9204\n" : "=d"(pcon) : :);
	} while (pcon & 0x1);

	return 0;
}

void arch_cache_init(void)
{
}

