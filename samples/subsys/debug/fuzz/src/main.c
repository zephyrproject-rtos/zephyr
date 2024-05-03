/* Copyright (c) 2022 Google, LLC.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/irq.h>
#include <irq_ctrl.h>
#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <nsi_cpu_if.h>
#include <nsi_main_semipublic.h>
#elif defined(CONFIG_BOARD_NATIVE_POSIX)
/* Note: native_posix will be deprecated soon */
extern void posix_init(int argc, char *argv[]);
extern void posix_exec_for(uint64_t us);
#define nsi_init posix_init
#define nsi_exec_for posix_exec_for
#else
#error "Platform not supported"
#endif

/* Fuzz testing is coverage-based, so we want to hide a failure case
 * (a write through a null pointer in this case) down inside a call
 * tree in such a way that it would be very unlikely to be found by
 * randomly-selected input.  But the fuzzer can still find it in
 * linear(-ish) time by discovering each new function along the way
 * and then probing that new space.  The 1 in 2^56 case here would
 * require months-to-years of work for a large datacenter, but the
 * fuzzer gets it in 20 seconds or so. This requires that the code for
 * each case be distinguishable/instrumentable though, which is why we
 * generate the recursive handler functions this way and disable
 * inlining to prevent optimization.
 */
int *global_null_ptr;
static const uint8_t key[] = { 0x9e, 0x21, 0x0c, 0x18, 0x9d, 0xd1, 0x7d };
bool found[ARRAY_SIZE(key)];

#define LASTKEY (ARRAY_SIZE(key) - 1)

#define GEN_CHECK(cur, nxt)                                                                        \
	void check##nxt(const uint8_t *data, size_t sz);                                           \
	void __attribute__((noinline)) check##cur(const uint8_t *data, size_t sz)                  \
	{                                                                                          \
		if (cur < sz && data[cur] == key[cur]) {                                           \
			if (!found[cur]) {                                                         \
				printk("#\n# Found key %d\n#\n", cur);                             \
				found[cur] = true;                                                 \
			}                                                                          \
			if (cur == LASTKEY) {                                                      \
				*global_null_ptr = 0; /* boom! */                                  \
			} else {                                                                   \
				check##nxt(data, sz);                                              \
			}                                                                          \
		}                                                                                  \
	}

GEN_CHECK(0, 1)
GEN_CHECK(1, 2)
GEN_CHECK(2, 3)
GEN_CHECK(3, 4)
GEN_CHECK(4, 5)
GEN_CHECK(5, 6)
GEN_CHECK(6, 0)

/* Fuzz input received from LLVM via "interrupt" */
static const uint8_t *fuzz_buf;
static size_t fuzz_sz;

K_SEM_DEFINE(fuzz_sem, 0, K_SEM_MAX_LIMIT);

static void fuzz_isr(const void *arg)
{
	/* We could call check0() to execute the fuzz case here, but
	 * pass it through to the main thread instead to get more OS
	 * coverage.
	 */
	k_sem_give(&fuzz_sem);
}

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);

		/* Execute the fuzz case we got from LLVM and passed
		 * through an interrupt to this thread.
		 */
		check0(fuzz_buf, fuzz_sz);
	}
	return 0;
}

/**
 * Entry point for fuzzing. Works by placing the data
 * into two known symbols, triggering an app-visible interrupt, and
 * then letting the simulator run for a fixed amount of time (intended to be
 * "long enough" to handle the event and reach a quiescent state
 * again)
 */
#if defined(CONFIG_BOARD_NATIVE_SIM)
NATIVE_SIMULATOR_IF /* We expose this function to the final runner link stage*/
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t sz)
{
	static bool runner_initialized;

	if (!runner_initialized) {
		nsi_init(0, NULL);
		runner_initialized = true;
	}

	/* Provide the fuzz data to the embedded OS as an interrupt, with
	 * "DMA-like" data placed into native_fuzz_buf/sz
	 */
	fuzz_buf = (void *)data;
	fuzz_sz = sz;

	hw_irq_ctrl_set_irq(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	/* Give the OS time to process whatever happened in that
	 * interrupt and reach an idle state.
	 */
	nsi_exec_for(k_ticks_to_us_ceil64(CONFIG_ARCH_POSIX_FUZZ_TICKS));

	return 0;
}
