#include <ztest.h>
#include <sys_clock.h>

#define  STACK_SIZE 512

static struct k_sem sema;

static K_THREAD_STACK_DEFINE(test_thread_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(test_thread_stack1, STACK_SIZE);
static struct k_thread test_thread_d;

#define ISR0_OFFSET 10
#define ISR2_OFFSET 11

#define IRQ_LINE(offset) (CONFIG_NUM_IRQS - ((offset) + 1))

volatile u32_t val;
volatile u32_t handler_executed;
u32_t VAL = 0xDEAD;
int key;


#if defined(CONFIG_ARM)
#include <arch/arm/cortex_m/cmsis.h>
void trigger_irq(int irq)
{
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU) || defined(CONFIG_CPU_CORTEX_M0) \
	|| defined(CONFIG_CPU_CORTEX_M0PLUS)
	NVIC_SetPendingIRQ(irq);
#else
	NVIC->STIR = irq;
#endif
}

#elif defined(CONFIG_CPU_ARCV2)
void trigger_irq(int irq)
{
	_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, irq);
}
#else
/* For not supported architecture */
#define NO_TRIGGER_FROM_SW
#endif

void isr2(void)
{
	printk("%s is executing\n", __func__);
	handler_executed++;
}

#ifndef NO_TRIGGER_FROM_SW
static void new_thread2(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	irq_enable(IRQ_LINE(ISR2_OFFSET));
	trigger_irq(IRQ_LINE(ISR2_OFFSET));
	k_sleep(SECONDS(1));
	zassert_equal(handler_executed, 1,
		"irq_lock is not working as expected\n");
	k_sem_give(&sema);
}
#else
static void new_thread2(void *arg1, void *arg2, void *arg3)
{
	k_sem_give(&sema);
	ztest_test_skip();
}
#endif

static void new_thread1(void *arg1, void *arg2, void *arg3)
{
	u32_t key1;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	key1 = irq_lock();
	IRQ_CONNECT(IRQ_LINE(ISR2_OFFSET), 2, isr2, NULL, 0);
}

void test_thread_specific_irq_prevention(void)
{
	k_sem_init(&sema, 0, 1);
	k_tid_t tid = k_thread_create(&test_thread_d, test_thread_stack,
					STACK_SIZE, new_thread1, NULL, NULL,
					NULL, K_PRIO_PREEMPT(1), 0, 0);

	k_tid_t tid1 = k_thread_create(&test_thread_d, test_thread_stack1,
					STACK_SIZE, new_thread2, NULL, NULL,
					NULL, K_PRIO_PREEMPT(2), 0, 0);
	k_sem_take(&sema, K_FOREVER);
	k_thread_abort(tid);
	k_thread_abort(tid1);
}

void isr1(void *param)
{
	ARG_UNUSED(param);
	printk("%s is executing\n", __func__);
	val = 0xDEAD;
}

void call_to_irq_unlock(int key)
{
	irq_unlock(key);
	k_sleep(SECONDS(1));
	zassert_equal(val, VAL, "irq_unlock is not working as expected\n");
}

#ifndef NO_TRIGGER_FROM_SW
void test_prevent_interruption(void)
{
	IRQ_CONNECT(IRQ_LINE(ISR0_OFFSET), 1, isr1, NULL, 0);
	key = irq_lock();
	irq_enable(IRQ_LINE(ISR0_OFFSET));
	trigger_irq(IRQ_LINE(ISR0_OFFSET));
	zassert_not_equal(val, VAL, "irq_lock is not working as expected\n");
	call_to_irq_unlock(key);
}
#else
void test_prevent_interruption(void)
{
	 ztest_test_skip();
}
#endif

void test_main(void)
{
	ztest_test_suite(test_irq_lock,
			ztest_unit_test(test_prevent_interruption),
			ztest_unit_test(test_thread_specific_irq_prevention)
			);
	ztest_run_test_suite(test_irq_lock);
}
