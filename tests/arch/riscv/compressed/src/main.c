/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/arch/riscv/exception.h>

#if CONFIG_RISCV_ISA_EXT_ZCA
ZTEST(riscv_compressed, test_zca_load_store)
{
	uint32_t a = 5;
	uint32_t b = 0;

	/* c.swsp and c.lwsp */
	__asm__ volatile("addi sp, sp, -0x4;"
			 "c.swsp %1, 0x0(sp);"
			 "c.lwsp %0, 0x0(sp);"
			 "addi sp, sp, 0x4;"
			: "=&cr"(b)
			: "cr"(a)
			: "memory");

	zexpect_equal(b, 5, "c.swsp and c.lwsp");

	b = 0;
	a = 7;
	uint32_t * c = &a;

	/* c.lw */
	__asm__ volatile("c.lw %0, 0x0(%1);"
			: "=cr"(b)
			: "cr"(c)
			: "memory");

	zexpect_equal(b, 7, "c.lw");

	b = 3;
	/* c.sw */
	__asm__ volatile("c.sw %0, 0x0(%1);"
			:
			: "cr"(b), "cr"(c)
			: "memory");

	zexpect_equal(a, 3, "c.sw");
}

ZTEST(riscv_compressed, test_zca_control)
{
	volatile bool skipped = true;
	/* c.j */
	__asm__ goto("c.j %l[cj];"
		    :
		    :
		    :
		    : cj);

	skipped = false;
cj:
	zexpect_equal(skipped, true, "c.j");

#if CONFIG_RISCV_ISA_RV32E || CONFIG_RISCV_ISA_RV32I
	/* c.jal */
	__asm__ volatile goto("c.jal %l[cjal];"
			     :
			     :
			     : "ra"
			     : cjal);

	skipped = false;
cjal:
	zexpect_equal(skipped, true, "c.jal");
#endif

	/* c.jr */
	__asm__ volatile goto("la s0, %l[cjr];"
			      "c.jr s0;"
			     :
			     :
			     : "s0"
			     : cjr);

	skipped = false;
cjr:
	zexpect_equal(skipped, true, "c.jr");

	/* c.jalr */
	__asm__ volatile goto("la s0, %l[cjalr];"
			      "c.jalr s0;"
			     :
			     :
			     : "s0", "ra"
			     : cjalr);

	skipped = false;
cjalr:
	zexpect_equal(skipped, true, "c.jalr");

	/* c.bnez */
	__asm__ volatile goto("c.bnez %0, %l[cbnez];"
			     :
			     : "cr"(skipped)
			     :
			     : cbnez);

	skipped = false;
cbnez:
	zexpect_equal(skipped, true, "c.bnez");

	/* c.beqz */
	__asm__ volatile goto("c.beqz %0, %l[cbeqz];"
			     :
			     : "cr"(skipped)
			     :
			     : cbeqz);

	skipped = false;
cbeqz:
	zexpect_equal(skipped, false, "c.beqz");
}

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	unsigned long mcause = csr_read(mcause);

	zexpect_equal(mcause & RISCV_MCAUSE_EXC_MASK, RISCV_EXC_BREAK, "c.ebreak");

	zexpect_equal(reason, K_ERR_CPU_EXCEPTION, "c.ebreak");

	ztest_set_fault_valid(false);
}

ZTEST(riscv_compressed, test_zca_integer)
{
	unsigned long a = 0;
	unsigned long b = 0;
	unsigned long * c = &a;

	/* c.li */
	__asm__ volatile("c.li %0, 0xF;"
			: "=cr"(a)
			:);

	zexpect_equal(a, 0xF, "c.li");

	/* c.lui */
	__asm__ volatile("c.lui %0, 0xF;"
			: "=cr"(a)
			:);

	zexpect_equal(a, 0xFUL << 12, "c.lui");

	a = 2;
	/* c.addi */
	__asm__ volatile("c.addi %0, 1;"
			: "+cr"(a)
			: "cr"(a));

	zexpect_equal(a, 3, "c.addi");

	a = 0xF;

	/* c.slli */
	__asm__ volatile("c.slli %0, 1;"
			: "+cr"(a)
			: "cr"(a));

	zexpect_equal(a, 0xF << 1, "c.slli");

	a = ULONG_MAX;

	/* c.srli */
	__asm__ volatile("c.srli %0, 1;"
			: "+cr"(a)
			: "cr"(a));

	zexpect_equal(a, ULONG_MAX >> 1UL, "c.srli");

	a = ULONG_MAX;

	/* c.srai */
	__asm__ volatile("c.srai %0, 1;"
			: "+cr"(a)
			: "cr"(a));

	zexpect_equal(a, ULONG_MAX, "c.srai");

	/* c.andi */
	__asm__ volatile("c.andi %0, 0;"
			: "+cr"(a)
			: "cr"(a));

	zexpect_equal(a, 0, "c.andi");

	b = 5;
	/* c.mv */
	__asm__ volatile("c.mv %0, %1;"
			: "=cr"(a)
			: "cr"(b));

	zexpect_equal(a, 5, "c.mv");

	/* c.add */
	__asm__ volatile("c.add %0, %1;"
			: "+cr"(a)
			: "cr"(b));

	zexpect_equal(a, 10, "c.add");

	__asm__ volatile("sw sp, 0x0(%0);"
			:
			: "cr"(c)
			: "memory");

	b = a;

	/* c.addi16sp */
	__asm__ volatile("c.addi16sp sp, -16;"
			 "sw sp, 0x0(%0);"
			 "c.addi16sp sp, 16;"
			 :
			 : "cr"(c)
			 : "memory");

	zexpect_equal(a, b - 16, "c.addi16sp");

	__asm__ volatile("sw sp, 0x0(%0);"
			:
			: "cr"(c)
			: "memory");

	/* c.addi4spn */
	__asm__ volatile("c.addi4spn %0, sp, 64;"
			 : "=cr"(b));

	zexpect_equal(a + 64, b, "c.addi4spn");

	a = 0xFF00;
	b = 0x0FF0;
	/* c.and */
	__asm__ volatile("c.and %0, %1;"
			: "+cr"(a)
			: "cr"(b));

	zexpect_equal(a, 0xF00, "c.and");

	/* c.or */
	__asm__ volatile("c.or %0, %1;"
			: "+cr"(a)
			: "cr"(b));

	zexpect_equal(a, 0xFF0, "c.or");

	/* c.xor */
	__asm__ volatile("c.xor %0, %1;"
			: "+cr"(a)
			: "cr"(b));

	zexpect_equal(a, 0, "c.xor");

	a = 0xFF0;
	/* c.sub */
	__asm__ volatile("c.sub %0, %1;"
			: "+cr"(a)
			: "cr"(b));

	zexpect_equal(a, 0, "c.sub");

	/* c.nop */
	__asm__ volatile("c.nop");

#ifndef CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL
	/* TODO: investigate ebreak handling in ALWAYS_SWITCH_TROUGH_ECALL */

	ztest_set_fault_valid(true);
	/* c.ebreak */
	__asm__ volatile("c.ebreak");
#endif
}

#if CONFIG_RISCV_ISA_RV64I
ZTEST(riscv_compressed, test_zca_64i)
{
	uint64_t i = UINT64_MAX;
	uint64_t j = 0;

	/* c.sdsp and c.ldsp */
	__asm__ volatile("addi sp, sp, -0x8;"
			 "c.sdsp %1, 0x0(sp);"
			 "c.ldsp %0, 0x0(sp);"
			 "addi sp, sp, 0x8;"
			: "=&cr"(j)
			: "cr"(i)
			: "memory");

	zexpect_equal(j, UINT64_MAX, "c.sdsp and c.ldsp");

	j = 0;
	i = UINT64_MAX - 5;
	uint64_t * k = &i;

	/* c.ld */
	__asm__ volatile("c.ld %0, 0x0(%1);"
			: "=cr"(j)
			: "cr"(k)
			: "memory");

	zexpect_equal(j, UINT64_MAX - 5, "c.ld");

	j = UINT64_MAX - 3;

	/* c.sd */
	__asm__ volatile("c.sd %0, 0x0(%1);"
			:
			: "cr"(j), "cr"(k)
			: "memory");

	zexpect_equal(i, UINT64_MAX - 3, "c.sd");

	j = 5;
	/* c.addiw */
	__asm__ volatile("c.addiw %0, 1;"
			: "+cr"(j));

	zexpect_equal(j, 6, "c.addiw");

	i = 5;
	/* c.addw */
	__asm__ volatile("c.addw %0, %1;"
			: "+cr"(j)
			: "cr"(i));

	zexpect_equal(j, 11, "c.addw");

	/* c.subw */
	__asm__ volatile("c.sub %0, %1;"
			: "+cr"(j)
			: "cr"(i));

	zexpect_equal(j, 6, "c.subw");
}
#endif
#endif

#if CONFIG_RISCV_ISA_EXT_ZCF
ZTEST(riscv_compressed, test_zcf)
{
	float a = 5.1f;
	float b = 0.0f;

	/* c.fswsp and c.flwsp */
	__asm__ volatile("addi sp, sp, -0x4;"
			 "c.fswsp %1, 0x0(sp);"
			 "c.flwsp %0, 0x0(sp);"
			 "addi sp, sp, 0x4;"
			: "=&cf"(b)
			: "cf"(a)
			: "memory");

	zexpect_equal(b, 5.1f, "c.fswsp and c.flwsp");


	b = 0.0f;
	a = 7.2f;
	float * c = &a;

	/* c.flw */
	__asm__ volatile("c.flw %0, 0x0(%1);"
			: "=cf"(b)
			: "cr"(c)
			: "memory");

	zexpect_equal(b, 7.2f, "c.flw");
	b = 3.3f;

	/* c.fsw */
	__asm__ volatile("c.fsw %0, 0x0(%1);"
			:
			: "cf"(b), "cr"(c)
			: "memory");

	zexpect_equal(a, 3.3f, "c.fsw");
}
#endif

#if CONFIG_RISCV_ISA_EXT_ZCD
ZTEST(riscv_compressed, test_zcd)
{
	double a = 5.1;
	double b = 0;

	/* c.fsdsp and c.fldsp */
	__asm__ volatile("addi sp, sp, -0x8;"
			 "c.fsdsp %1, 0x0(sp);"
			 "c.fldsp %0, 0x0(sp);"
			 "addi sp, sp, 0x8;"
			: "=&cf"(b)
			: "cf"(a)
			: "memory");

	zexpect_equal(b, 5.1, "c.fsdsp and c.fldsp");


	b = 0;
	a = 7.2;
	double * c = &a;

	/* c.fld */
	__asm__ volatile("c.fld %0, 0x0(%1);"
			: "=cf"(b)
			: "cr"(c)
			: "memory");

	zexpect_equal(b, 7.2, "c.fld");
	b = 3.3;

	/* c.fsd */
	__asm__ volatile("c.fsd %0, 0x0(%1);"
			:
			: "cf"(b), "cr"(c)
			: "memory");

	zexpect_equal(a, 3.3, "c.fsd");
}
#endif

ZTEST_SUITE(riscv_compressed, NULL, NULL, NULL, NULL, NULL);
