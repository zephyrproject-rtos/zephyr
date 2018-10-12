#include "interrupt.h"

#define DURATION 5
struct k_timer timer;

#define ISR0_OFFSET 1
#define ISR1_OFFSET 2

volatile u32_t new_val;
u32_t old_val = 0xDEAD;
volatile u32_t check_lock_new;
u32_t check_lock_old = 0xBEEF;

void isr1(void *param)
{
	ARG_UNUSED(param);
	new_val = 0xDEAD;
	printk("%s ran !!\n", __func__);
}

/**
 *
 * triggering interrupt from the timer expiry function while isr0
 * is in busy wait
 */
#ifndef NO_TRIGGER_FROM_SW
static void handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	irq_enable(IRQ_LINE(ISR1_OFFSET));
	trigger_irq(IRQ_LINE(ISR1_OFFSET));
}
#else
void handler(void)
{
	ztest_test_skip();
}
#endif

void isr0(void *param)
{
	ARG_UNUSED(param);
	printk("%s running !!\n", __func__);
	k_busy_wait(K_SECONDS(10));
	printk("%s execution completed !!\n", __func__);
	zassert_equal(new_val, old_val, "Nested interrupt is not working\n");
}

/**
 *
 * Interrupt nesting feature allows an ISR to be preempted in mid-execution
 * if a higher priority interrupt is signaled. The lower priority ISR resumes
 * execution once the higher priority ISR has completed its processing.
 */
#ifndef NO_TRIGGER_FROM_SW
void test_nested_isr(void)
{
	IRQ_CONNECT(IRQ_LINE(ISR0_OFFSET), 1, isr0, NULL, 0);
	IRQ_CONNECT(IRQ_LINE(ISR1_OFFSET), 0, isr1, NULL, 0);

	k_timer_init(&timer, handler, NULL);
	k_timer_start(&timer, DURATION, 0);
	irq_enable(IRQ_LINE(ISR0_OFFSET));
	trigger_irq(IRQ_LINE(ISR0_OFFSET));

}
#else
void test_nested_isr(void)
{
	ztest_test_skip();
}
#endif

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	check_lock_new = 0xBEEF;
}

static void offload_function(void *param)
{
	ARG_UNUSED(param);

	zassert_true(_is_in_isr(), "Not in IRQ context!");
	k_timer_init(&timer, timer_handler, NULL);
	k_busy_wait(K_SECONDS(1));
	k_timer_start(&timer, DURATION, 0);
	zassert_not_equal(check_lock_new, check_lock_old,
		"Interrupt locking didn't work properly");
}

void test_prevent_interruption(void)
{
	irq_offload(offload_function, NULL);
	k_timer_stop(&timer);
}
