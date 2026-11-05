/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include "sw_isr_common.h"

#if defined(TEST_LOCAL_ISR_TABLE_DEFAULT)
BUILD_ASSERT(IS_ENABLED(CONFIG_ISR_TABLES_LOCAL_DECLARATION),
	     "Local ISR table generation must be enabled by default");
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#include <zephyr/drivers/uart.h>
#endif

/* These slots are inspected, never enabled or raised by the test. */
#if defined(CONFIG_MIPS)
/* Malta's CP0 timer uses IRQ 7; software IRQs 0 and 1 are unclaimed. */
#define FIRST_TEST_IRQ 0
#define SECOND_TEST_IRQ 1
#elif defined(CONFIG_RISCV)
/* Local CPU IRQs avoid interpreting a flat table index as a PLIC child IRQ. */
#define FIRST_TEST_IRQ 1
#define SECOND_TEST_IRQ 2
#else
#define FIRST_TEST_IRQ (CONFIG_NUM_IRQS - 2)
#define SECOND_TEST_IRQ (CONFIG_NUM_IRQS - 1)
#endif
#if defined(CONFIG_RISCV)
#define REGISTRATION_IRQ(irq) \
	((irq) + CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET)
#else
#define REGISTRATION_IRQ(irq) (irq)
#endif
#define TIMER_EXPIRATIONS 3
#define IRQ_TIMEOUT_MS 2000
#define TIMER_INTERVAL_MS 20
#define INTEGER_ARGUMENT 0x51U

BUILD_ASSERT(REGISTRATION_IRQ(FIRST_TEST_IRQ) >= CONFIG_GEN_IRQ_START_VECTOR);

static const uint32_t object_argument = 0x12345678U;
#if defined(CONFIG_SHARED_INTERRUPTS)
static const uint32_t second_argument = 0x87654321U;
#endif

static void object_handler(const void *argument)
{
	ARG_UNUSED(argument);
}

static void second_handler(const void *argument)
{
	ARG_UNUSED(argument);
}

static struct _isr_table_entry read_entry(unsigned int irq)
{
	unsigned int index = z_get_sw_isr_table_idx(REGISTRATION_IRQ(irq));

#if defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
	struct _isr_table_entry entry;

	get_isr_entry(index, &entry);
	return entry;
#else
	return _sw_isr_table[index];
#endif
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_SHARED_INTERRUPTS) && \
	(defined(CONFIG_RISCV) || defined(CONFIG_SPARC) || \
	 defined(CONFIG_OPENRISC))
#define UART_SHARED_OBSERVER
static atomic_t uart_observer_calls;
static atomic_t invalid_uart_observer;

static void uart_shared_observer(const void *argument)
{
	if (!k_is_in_isr() || argument != &object_argument) {
		atomic_set(&invalid_uart_observer, 1);
	}
	atomic_inc(&uart_observer_calls);
}
#endif

static void *setup(void)
{
	Z_ISR_DECLARE(REGISTRATION_IRQ(FIRST_TEST_IRQ), 0, object_handler,
		      &object_argument);
#if defined(CONFIG_SHARED_INTERRUPTS)
	Z_ISR_DECLARE(REGISTRATION_IRQ(FIRST_TEST_IRQ), 0, second_handler,
		      &second_argument);
#else
	Z_ISR_DECLARE(REGISTRATION_IRQ(SECOND_TEST_IRQ), 0, second_handler,
		      UINT_TO_POINTER(INTEGER_ARGUMENT));
#endif
#if defined(UART_SHARED_OBSERVER)
	Z_ISR_DECLARE(REGISTRATION_IRQ(DT_IRQN(DT_CHOSEN(zephyr_console))), 0,
		      uart_shared_observer, &object_argument);
#endif
	return NULL;
}

ZTEST(isr_table_runtime, test_original_registration_references)
{
	struct _isr_table_entry entry = read_entry(FIRST_TEST_IRQ);

#if defined(CONFIG_SHARED_INTERRUPTS)
	unsigned int slot_index =
		z_get_sw_isr_table_idx(REGISTRATION_IRQ(FIRST_TEST_IRQ));
	const struct z_shared_isr_table_entry *slot =
		&z_shared_sw_isr_table[slot_index];
	bool first_found = false;
	bool second_found = false;

	zassert_equal(entry.isr, z_shared_isr);
	zassert_equal(entry.arg, slot);
	zassert_equal(slot->client_num, 2);
	for (size_t index = 0; index < slot->client_num; ++index) {
		const struct _isr_table_entry *client = &slot->clients[index];

		first_found |= client->isr == object_handler &&
			       client->arg == &object_argument;
		second_found |= client->isr == second_handler &&
				client->arg == &second_argument;
	}
	zassert_true(first_found);
	zassert_true(second_found);
#else
	zassert_equal(entry.isr, object_handler);
	zassert_equal(entry.arg, &object_argument);
	entry = read_entry(SECOND_TEST_IRQ);
	zassert_equal(entry.isr, second_handler);
	zassert_equal(entry.arg, UINT_TO_POINTER(INTEGER_ARGUMENT));
#endif
}

#if defined(CONFIG_DYNAMIC_INTERRUPTS)
static const uint32_t dynamic_argument = 0x11223344U;

static void dynamic_handler(const void *argument)
{
	ARG_UNUSED(argument);
}

ZTEST(isr_table_runtime, test_dynamic_entry_update)
{
	struct _isr_table_entry entry;

	irq_connect_dynamic(FIRST_TEST_IRQ, 1, dynamic_handler,
			    &dynamic_argument, 0);
	entry = read_entry(FIRST_TEST_IRQ);
	zassert_equal(entry.isr, dynamic_handler);
	zassert_equal(entry.arg, &dynamic_argument);
	irq_connect_dynamic(FIRST_TEST_IRQ, 1, object_handler,
			    &object_argument, 0);
	entry = read_entry(FIRST_TEST_IRQ);
	zassert_equal(entry.isr, object_handler);
	zassert_equal(entry.arg, &object_argument);
}
#endif

static atomic_t timer_calls;
static atomic_t invalid_timer_context;
static K_SEM_DEFINE(timer_completed, 0, 1);
static struct k_timer hardware_timer;

static void timer_expired(struct k_timer *timer)
{
	if (!k_is_in_isr() || timer != &hardware_timer ||
	    k_timer_user_data_get(timer) != &object_argument) {
		atomic_set(&invalid_timer_context, 1);
	}
	if (atomic_inc(&timer_calls) + 1 == TIMER_EXPIRATIONS) {
		k_timer_stop(timer);
		k_sem_give(&timer_completed);
	}
}

ZTEST(isr_table_runtime, test_hardware_timer_interrupt)
{
	int result;

	atomic_clear(&timer_calls);
	atomic_clear(&invalid_timer_context);
	k_sem_reset(&timer_completed);
	k_timer_init(&hardware_timer, timer_expired, NULL);
	k_timer_user_data_set(&hardware_timer, (void *)&object_argument);
	k_timer_start(&hardware_timer, K_MSEC(TIMER_INTERVAL_MS),
		      K_MSEC(TIMER_INTERVAL_MS));
	result = k_sem_take(&timer_completed, K_MSEC(IRQ_TIMEOUT_MS));

	k_timer_stop(&hardware_timer);
	zassert_ok(result, "Hardware timer interrupt did not complete");
	zassert_equal(atomic_get(&timer_calls), TIMER_EXPIRATIONS);
	zassert_equal(atomic_get(&invalid_timer_context), 0);
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static const struct device *const uart =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static atomic_t uart_calls;
static atomic_t invalid_uart_context;
static K_SEM_DEFINE(uart_completed, 0, 1);

static void uart_interrupt(const struct device *dev, void *argument)
{
	/* A driver may notify its client before the first hardware edge. */
	if (!k_is_in_isr()) {
		return;
	}
	uart_irq_tx_disable(dev);
	if (dev != uart || argument != &object_argument) {
		atomic_set(&invalid_uart_context, 1);
	}
	atomic_inc(&uart_calls);
	k_sem_give(&uart_completed);
}

ZTEST(isr_table_runtime, test_hardware_uart_interrupt)
{
	int result;

	zassert_true(device_is_ready(uart));
	atomic_clear(&uart_calls);
	atomic_clear(&invalid_uart_context);
#if defined(UART_SHARED_OBSERVER)
	atomic_clear(&uart_observer_calls);
	atomic_clear(&invalid_uart_observer);
#endif
	k_sem_reset(&uart_completed);
	result = uart_irq_callback_user_data_set(uart, uart_interrupt,
					    (void *)&object_argument);
	zassert_ok(result);
	/* TX-empty level or this byte's TX edge raises the device IRQ. */
	uart_irq_tx_enable(uart);
	uart_poll_out(uart, '.');
	result = k_sem_take(&uart_completed, K_MSEC(IRQ_TIMEOUT_MS));
	uart_irq_tx_disable(uart);
	uart_irq_callback_user_data_set(uart, NULL, NULL);
	zassert_ok(result, "UART controller interrupt did not complete");
	/* A TX-empty level may already be pending when TX IRQs are disabled. */
	printk("UART IRQ callbacks: %ld\n", (long)atomic_get(&uart_calls));
	zassert_true(atomic_get(&uart_calls) >= 1,
		     "No UART IRQ callback observed");
	zassert_equal(atomic_get(&invalid_uart_context), 0);
#if defined(UART_SHARED_OBSERVER)
	printk("Shared UART IRQ observations: %ld\n",
	       (long)atomic_get(&uart_observer_calls));
	zassert_true(atomic_get(&uart_observer_calls) >= 1,
		     "Shared UART IRQ observer was not called");
	zassert_equal(atomic_get(&invalid_uart_observer), 0);
#endif
}
#endif

ZTEST_SUITE(isr_table_runtime, NULL, setup, NULL, NULL, NULL);
