/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/ztest.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_MULTITHREADING
#error "Test does not work with CONFIG_MULTITHREADING=n"
#endif

char stack[4096];

void *main_sh, *my_sh;

/* Aforementioned dirty trick: this value becomes the "next switch
 * handle" examined by the interrupt exit code in lieu of whatever the
 * scheduler would return.
 */
void *next_sh;

int sum;

bool my_fn_ran;

extern void *arm_m_last_switch_handle;

void *z_get_next_switch_handle(void *interrupted)
{
	return next_sh;
}

void my_fn(void *a, void *b, void *c, void *d)
{
	void *psplim;

	__asm__ volatile("mrs %0, psplim" : "=r"(psplim));

	printk("%s: PSPLIM = %p\n", __func__, psplim);

	zassert_equal((int)a, 0);
	zassert_equal((int)b, 1);
	zassert_equal((int)c, 2);

	register int A = 11;
	register int B = 12;
	register int C = 13;
	register int D = 14;
	register int E = 15;

#ifdef CONFIG_CPU_HAS_FPU
	register float FA = 11.0f;
	register float FB = 12.0f;
	register float FC = 13.0f;
#endif

	for (int n = 0; /**/; n++) {
		printk("%s:%d iter %d\n", __func__, __LINE__, n);

		if (arm_m_last_switch_handle) {
			printk("Using exception handle @ %p\n", arm_m_last_switch_handle);
			main_sh = arm_m_last_switch_handle;
			arm_m_last_switch_handle = NULL;
		}

		my_fn_ran = true;

		arm_m_switch(main_sh, &my_sh);

		zassert_equal(A, 11);
		zassert_equal(B, 12);
		zassert_equal(C, 13);
		zassert_equal(D, 14);
		zassert_equal(E, 15);
#ifdef CONFIG_CPU_HAS_FPU
		zassert_equal(FA, 11.0f);
		zassert_equal(FB, 12.0f);
		zassert_equal(FC, 13.0f);
#endif
	}
}

/* Nothing in particular to do here except exercise the interrupt exit hook */
void my_svc(void)
{
	printk("%s:%d\n", __func__, __LINE__);
	arm_m_exc_tail();

	/* Validate that the tail hook doesn't need to be last */
	printk("   arm_m_exc_tail() has been called\n");
}

ZTEST(arm_m_switch, test_smoke)
{
	void *psplim;

	__asm__ volatile("mrs %0, psplim" : "=r"(psplim));
	printk("In main, PSPLIM = %p\n", psplim);

	/* "register" variables don't strictly force the compiler not
	 * to spill them, but inspecting the generated code shows it's
	 * leaving them in place across the switch.
	 */
	register int A = 1;
	register int B = 2;
	register int C = 3;
	register int D = 4;
	register int E = 5;

#ifdef CONFIG_CPU_HAS_FPU
	/* Prime all the FPU registers with something I can recognize in a debugger */
	uint32_t sregs[32];

	for (int i = 0; i < 32; i++) {
		sregs[i] = 0x3f800000 + i;
	}
	__asm__ volatile("vldm %0, {s0-s31}" ::"r"(sregs));

	register float FA = 1.0f;
	register float FB = 2.0f;
	register float FC = 3.0f;
#endif

	sum += A + B + C + D + E;

	/* Hit an interrupt and make sure CPU state doesn't get messed up */
	printk("Invoking SVC\n");
	__asm__ volatile("svc 0");
	printk("...back\n");

	zassert_equal(A, 1);
	zassert_equal(B, 2);
	zassert_equal(C, 3);
	zassert_equal(D, 4);
	zassert_equal(E, 5);
#ifdef CONFIG_CPU_HAS_FPU
	zassert_equal(FA, 1.0f);
	zassert_equal(FB, 2.0f);
	zassert_equal(FC, 3.0f);
#endif

	/* Now likewise switch to and from a foreign stack and check */
	my_sh = arm_m_new_stack(stack, sizeof(stack), my_fn, (void *)0, (void *)1, (void *)2, NULL);

	int cycles = 16;

	for (int n = 0; n < cycles; n++) {
		printk("main() switching to my_fn() (iter %d)...\n", n);
		my_fn_ran = false;
		arm_m_switch(my_sh, &main_sh);
		printk("...and back\n");

		zassert_true(my_fn_ran);
		zassert_equal(A, 1);
		zassert_equal(B, 2);
		zassert_equal(C, 3);
		zassert_equal(D, 4);
		zassert_equal(E, 5);
#ifdef CONFIG_CPU_HAS_FPU
		zassert_equal(FA, 1.0f);
		zassert_equal(FB, 2.0f);
		zassert_equal(FC, 3.0f);
#endif
	}
}

/* Makes a copy of the vector table in writable RAM (it's generally in
 * a ROM section), redirects it, and hooks the SVC interrupt with our
 * own code above so we can catch direct interrupts.
 */
void *vector_hijack(void)
{
	static uint32_t __aligned(1024) vectors[256];
	uint32_t *vtor_p = (void *)0xe000ed08;
	uint32_t *vtor = (void *)*vtor_p;

	printk("VTOR @%p\n", vtor);
	if (vtor == NULL) {
		/* mps2/an385 doesn't set this up, don't know why */
		printk("VTOR not set up by SOC, skipping case\n");
		ztest_test_skip();
		return NULL;
	}

	/* Vector count: _vector_start/end set by the linker. */
	int nv = (&_vector_end[0] - &_vector_start[0]) / sizeof(uint32_t);

	for (int i = 0; i < nv; i++) {
		vectors[i] = vtor[i];
	}
	*vtor_p = (uint32_t)&vectors[0];
	vtor = (void *)*vtor_p;
	printk("VTOR now @%p\n", vtor);

	/* And hook the SVC call with our own function above, allowing
	 * us direct access to interrupt entry
	 */
	vtor[11] = (int)my_svc;
	printk("vtor[11] == %p (my_svc == %p)\n", (void *)vtor[11], my_svc);

	return NULL;
}

ZTEST_SUITE(arm_m_switch, NULL, vector_hijack, NULL, NULL, NULL);
