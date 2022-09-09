/* Copyright (c) 2022 Google, LLC.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>

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

#define GEN_CHECK(cur, nxt)						\
	void check##nxt(uint8_t *data, size_t sz);			\
	void __attribute__((noinline)) check##cur(uint8_t *data, size_t sz) \
	{								\
		if (cur < sz && data[cur] == key[cur]) {		\
			if (!found[cur]) {				\
				printk("#\n# Found key %d\n#\n", cur);	\
				found[cur] = true;			\
			}						\
			if (cur == LASTKEY) {				\
				*global_null_ptr = 0; /* boom! */	\
			} else {					\
				check##nxt(data, sz);			\
			}						\
		}							\
	}

GEN_CHECK(0, 1)
GEN_CHECK(1, 2)
GEN_CHECK(2, 3)
GEN_CHECK(3, 4)
GEN_CHECK(4, 5)
GEN_CHECK(5, 6)
GEN_CHECK(6, 0)

/* Fuzz input received from LLVM via "interrupt" */
extern uint8_t *posix_fuzz_buf, posix_fuzz_sz;

K_SEM_DEFINE(fuzz_sem, 0, K_SEM_MAX_LIMIT);

static void fuzz_isr(const void *arg)
{
	/* We could call check0() to execute the fuzz case here, but
	 * pass it through to the main thread instead to get more OS
	 * coverage.
	 */
	k_sem_give(&fuzz_sem);
}

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	IRQ_CONNECT(CONFIG_ARCH_POSIX_FUZZ_IRQ, 0, fuzz_isr, NULL, 0);
	irq_enable(CONFIG_ARCH_POSIX_FUZZ_IRQ);

	while (true) {
		k_sem_take(&fuzz_sem, K_FOREVER);

		/* Execute the fuzz case we got from LLVM and passed
		 * through an interrupt to this thread.
		 */
		check0(posix_fuzz_buf, posix_fuzz_sz);
	}
}
