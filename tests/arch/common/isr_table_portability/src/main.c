/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/interrupt_util.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_BOARD_QEMU_CORTEX_A53)
/* SGIs 0..2 are reserved for IPIs; only SGIs 3..7 are raised here. */
#define OBJECT_IRQ 3U
#define NULL_IRQ 4U
#define INTEGER_IRQ 5U
#define SHARED_IRQ 6U
#define DYNAMIC_IRQ 7U
#define DEFAULT_IRQ 13U
#define TEST_IRQ_PRIORITY IRQ_DEFAULT_PRIORITY
#elif defined(CONFIG_BOARD_QEMU_CORTEX_M3)
/* The board's disabled Ethernet device owns IRQ 42 when networking is enabled. */
#define OBJECT_IRQ 42U
#define NULL_IRQ 41U
#define INTEGER_IRQ 40U
#define SHARED_IRQ 39U
#define DYNAMIC_IRQ 38U
#define DEFAULT_IRQ 37U
#define TEST_IRQ_PRIORITY 2U
#else
#error "Select a platform with reserved, hardware-triggerable test IRQs"
#endif

#define TEST_IRQ_FLAGS 0U
#define INTEGER_ARGUMENT_VALUE 0x51U
#define SECOND_SHARED_ARGUMENT_VALUE 0xa2U
#define IRQ_TIMEOUT_MS 1000
#define REPEATED_TRIGGERS 2U
#define MAX_COMPLETIONS 3U

BUILD_ASSERT(DEFAULT_IRQ < CONFIG_NUM_IRQS);
BUILD_ASSERT(OBJECT_IRQ >= CONFIG_GEN_IRQ_START_VECTOR);
BUILD_ASSERT(DEFAULT_IRQ >= CONFIG_GEN_IRQ_START_VECTOR);

enum handler_id {
	OBJECT_HANDLER,
	NULL_HANDLER,
	INTEGER_HANDLER,
#if defined(CONFIG_SHARED_INTERRUPTS)
	SHARED_FIRST_HANDLER,
	SHARED_SECOND_HANDLER,
#endif
#if defined(CONFIG_DYNAMIC_INTERRUPTS)
	DYNAMIC_FIRST_HANDLER,
#if defined(CONFIG_SHARED_INTERRUPTS)
	DYNAMIC_SECOND_HANDLER,
#endif
#endif
	HANDLER_COUNT,
};

struct observation {
	atomic_t calls;
	const void *argument;
};

static struct observation observations[HANDLER_COUNT];
static const uint32_t object_argument = 0x12345678U;
#if defined(CONFIG_SHARED_INTERRUPTS)
static const uint32_t shared_argument = 0x87654321U;
#endif
#if defined(CONFIG_DYNAMIC_INTERRUPTS)
static const uint32_t dynamic_argument = 0x11223344U;
#endif
K_SEM_DEFINE(completed, 0, MAX_COMPLETIONS);

static void record_call(enum handler_id id, const void *argument)
{
	observations[id].argument = argument;
	atomic_inc(&observations[id].calls);
	k_sem_give(&completed);
}

#define DEFINE_HANDLER(name, id) \
	static void name(const void *argument) \
	{ \
		record_call(id, argument); \
	}

DEFINE_HANDLER(object_isr, OBJECT_HANDLER)
DEFINE_HANDLER(null_isr, NULL_HANDLER)
DEFINE_HANDLER(integer_isr, INTEGER_HANDLER)
#if defined(CONFIG_SHARED_INTERRUPTS)
DEFINE_HANDLER(shared_first_isr, SHARED_FIRST_HANDLER)
DEFINE_HANDLER(shared_second_isr, SHARED_SECOND_HANDLER)
#endif
#if defined(CONFIG_DYNAMIC_INTERRUPTS)
DEFINE_HANDLER(dynamic_first_isr, DYNAMIC_FIRST_HANDLER)
#if defined(CONFIG_SHARED_INTERRUPTS)
DEFINE_HANDLER(dynamic_second_isr, DYNAMIC_SECOND_HANDLER)
#endif
#endif

static uint32_t table_index(uint32_t irq)
{
	return irq - CONFIG_GEN_IRQ_START_VECTOR;
}

static struct _isr_table_entry read_entry(uint32_t irq)
{
#if defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
	return _sw_isr_table[table_index(irq)];
#else
	struct _isr_table_entry entry;

	get_isr_entry(table_index(irq), &entry);
	return entry;
#endif
}

static void assert_entry(uint32_t irq, void (*handler)(const void *), const void *argument)
{
	struct _isr_table_entry entry = read_entry(irq);

	zassert_true(entry.isr == handler, "Wrong handler for IRQ %u", irq);
	zassert_true(entry.arg == argument, "Wrong argument for IRQ %u", irq);
}

static void assert_calls(enum handler_id first, uint32_t first_count,
			 enum handler_id second, uint32_t second_count)
{
	for (uint32_t id = 0U; id < HANDLER_COUNT; ++id) {
		uint32_t expected = 0U;

		if (id == first) {
			expected = first_count;
		} else if (id == second) {
			expected = second_count;
		}
		zassert_equal(atomic_get(&observations[id].calls), (atomic_val_t)expected,
			      "Wrong call count for handler %u", id);
	}
}

static void wait_for_handlers(uint32_t count)
{
	for (uint32_t index = 0U; index < count; ++index) {
		zassert_ok(k_sem_take(&completed, K_MSEC(IRQ_TIMEOUT_MS)),
			   "IRQ did not reach its handler");
	}
}

static void raise_repeatedly(uint32_t irq, uint32_t clients)
{
	irq_enable(irq);
	for (uint32_t count = 0U; count < REPEATED_TRIGGERS; ++count) {
		trigger_irq(irq);
		wait_for_handlers(clients);
	}
	irq_disable(irq);
}

static void assert_observed_argument(enum handler_id id, const void *argument)
{
	zassert_true(observations[id].argument == argument,
		     "Handler %u received the wrong argument", id);
}

#if defined(CONFIG_SHARED_INTERRUPTS)
static const struct z_shared_isr_table_entry *shared_slot(uint32_t irq)
{
	return &z_shared_sw_isr_table[table_index(irq)];
}

static void assert_empty_shared_slot(uint32_t irq)
{
	const struct z_shared_isr_table_entry *slot = shared_slot(irq);

	zassert_equal(slot->client_num, 0U, "Unexpected shared clients for IRQ %u", irq);
	for (uint32_t index = 0U; index < CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS; ++index) {
		zassert_is_null(slot->clients[index].arg);
		zassert_is_null(slot->clients[index].isr);
	}
}

static void assert_shared_pair(uint32_t irq, void (*first)(const void *), const void *first_arg,
			       void (*second)(const void *), const void *second_arg)
{
	const struct z_shared_isr_table_entry *slot = shared_slot(irq);
	bool found_first = false;
	bool found_second = false;

	assert_entry(irq, z_shared_isr, slot);
	zassert_equal(slot->client_num, 2U);
	for (uint32_t index = 0U; index < slot->client_num; ++index) {
		const struct _isr_table_entry *client = &slot->clients[index];

		found_first |= client->isr == first && client->arg == first_arg;
		found_second |= client->isr == second && client->arg == second_arg;
	}
	zassert_true(found_first, "First shared client was not retained");
	zassert_true(found_second, "Second shared client was not retained");
	for (uint32_t index = slot->client_num;
	     index < CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS; ++index) {
		zassert_is_null(slot->clients[index].arg);
		zassert_is_null(slot->clients[index].isr);
	}
}
#endif

static void *setup(void)
{
	IRQ_CONNECT(OBJECT_IRQ, TEST_IRQ_PRIORITY, object_isr, &object_argument, TEST_IRQ_FLAGS);
	IRQ_CONNECT(NULL_IRQ, TEST_IRQ_PRIORITY, null_isr, NULL, TEST_IRQ_FLAGS);
	IRQ_CONNECT(INTEGER_IRQ, TEST_IRQ_PRIORITY, integer_isr,
		    UINT_TO_POINTER(INTEGER_ARGUMENT_VALUE), TEST_IRQ_FLAGS);
#if defined(CONFIG_SHARED_INTERRUPTS)
	IRQ_CONNECT(SHARED_IRQ, TEST_IRQ_PRIORITY, shared_first_isr,
		    &shared_argument, TEST_IRQ_FLAGS);
	IRQ_CONNECT(SHARED_IRQ, TEST_IRQ_PRIORITY, shared_second_isr,
		    UINT_TO_POINTER(SECOND_SHARED_ARGUMENT_VALUE), TEST_IRQ_FLAGS);
#endif
	return NULL;
}

static void disable_test_irqs(void)
{
	irq_disable(OBJECT_IRQ);
	irq_disable(NULL_IRQ);
	irq_disable(INTEGER_IRQ);
#if defined(CONFIG_SHARED_INTERRUPTS)
	irq_disable(SHARED_IRQ);
#endif
#if defined(CONFIG_DYNAMIC_INTERRUPTS)
	irq_disable(DYNAMIC_IRQ);
#endif
}

static void before(void *data)
{
	ARG_UNUSED(data);
	disable_test_irqs();
	k_sem_reset(&completed);
	for (uint32_t id = 0U; id < HANDLER_COUNT; ++id) {
		atomic_clear(&observations[id].calls);
		observations[id].argument = NULL;
	}
}

static void after(void *data)
{
	ARG_UNUSED(data);
	disable_test_irqs();
}

ZTEST(isr_table_portability, test_static_object_parameter)
{
	assert_entry(OBJECT_IRQ, object_isr, &object_argument);
#if defined(CONFIG_SHARED_INTERRUPTS)
	assert_empty_shared_slot(OBJECT_IRQ);
#endif
	raise_repeatedly(OBJECT_IRQ, 1U);
	assert_calls(OBJECT_HANDLER, REPEATED_TRIGGERS, HANDLER_COUNT, 0U);
	assert_observed_argument(OBJECT_HANDLER, &object_argument);
}

ZTEST(isr_table_portability, test_static_null_parameter)
{
	assert_entry(NULL_IRQ, null_isr, NULL);
	raise_repeatedly(NULL_IRQ, 1U);
	assert_calls(NULL_HANDLER, REPEATED_TRIGGERS, HANDLER_COUNT, 0U);
	assert_observed_argument(NULL_HANDLER, NULL);
}

ZTEST(isr_table_portability, test_static_integer_parameter)
{
	const void *argument = UINT_TO_POINTER(INTEGER_ARGUMENT_VALUE);

	assert_entry(INTEGER_IRQ, integer_isr, argument);
	raise_repeatedly(INTEGER_IRQ, 1U);
	assert_calls(INTEGER_HANDLER, REPEATED_TRIGGERS, HANDLER_COUNT, 0U);
	assert_observed_argument(INTEGER_HANDLER, argument);
}

ZTEST(isr_table_portability, test_default_entry)
{
	/* Invoking the default handler is fatal; inspect it without enabling the IRQ. */
	assert_entry(DEFAULT_IRQ, z_irq_spurious, NULL);
#if defined(CONFIG_SHARED_INTERRUPTS)
	assert_empty_shared_slot(DEFAULT_IRQ);
#endif
	assert_calls(HANDLER_COUNT, 0U, HANDLER_COUNT, 0U);
}

#if defined(CONFIG_SHARED_INTERRUPTS)
ZTEST(isr_table_portability, test_static_shared_clients)
{
	const void *second_argument = UINT_TO_POINTER(SECOND_SHARED_ARGUMENT_VALUE);

	assert_shared_pair(SHARED_IRQ, shared_first_isr, &shared_argument,
			   shared_second_isr, second_argument);
	raise_repeatedly(SHARED_IRQ, 2U);
	assert_calls(SHARED_FIRST_HANDLER, REPEATED_TRIGGERS,
		     SHARED_SECOND_HANDLER, REPEATED_TRIGGERS);
	assert_observed_argument(SHARED_FIRST_HANDLER, &shared_argument);
	assert_observed_argument(SHARED_SECOND_HANDLER, second_argument);
}
#endif

#if defined(CONFIG_DYNAMIC_INTERRUPTS)
ZTEST(isr_table_portability, test_dynamic_single_client)
{
	int connected;

	assert_entry(DYNAMIC_IRQ, z_irq_spurious, NULL);
	connected = irq_connect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_first_isr,
					&dynamic_argument, TEST_IRQ_FLAGS);
	zassert_equal(connected, DYNAMIC_IRQ);
	assert_entry(DYNAMIC_IRQ, dynamic_first_isr, &dynamic_argument);
#if defined(CONFIG_SHARED_INTERRUPTS)
	assert_empty_shared_slot(DYNAMIC_IRQ);
#endif
	raise_repeatedly(DYNAMIC_IRQ, 1U);
	assert_calls(DYNAMIC_FIRST_HANDLER, REPEATED_TRIGGERS, HANDLER_COUNT, 0U);
	assert_observed_argument(DYNAMIC_FIRST_HANDLER, &dynamic_argument);
#if defined(CONFIG_SHARED_INTERRUPTS)
	zassert_ok(irq_disconnect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_first_isr,
					  &dynamic_argument, TEST_IRQ_FLAGS));
	assert_entry(DYNAMIC_IRQ, z_irq_spurious, NULL);
	assert_empty_shared_slot(DYNAMIC_IRQ);
#endif
}

#if defined(CONFIG_SHARED_INTERRUPTS)
ZTEST(isr_table_portability, test_dynamic_shared_transitions)
{
	const void *second_argument = UINT_TO_POINTER(SECOND_SHARED_ARGUMENT_VALUE);
	int connected;

	assert_entry(DYNAMIC_IRQ, z_irq_spurious, NULL);
	assert_empty_shared_slot(DYNAMIC_IRQ);
	connected = irq_connect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_first_isr,
					&dynamic_argument, TEST_IRQ_FLAGS);
	zassert_equal(connected, DYNAMIC_IRQ);
	assert_entry(DYNAMIC_IRQ, dynamic_first_isr, &dynamic_argument);
	assert_empty_shared_slot(DYNAMIC_IRQ);
	connected = irq_connect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_second_isr,
					second_argument, TEST_IRQ_FLAGS);
	zassert_equal(connected, DYNAMIC_IRQ);
	assert_shared_pair(DYNAMIC_IRQ, dynamic_first_isr, &dynamic_argument,
			   dynamic_second_isr, second_argument);
	raise_repeatedly(DYNAMIC_IRQ, 2U);
	assert_calls(DYNAMIC_FIRST_HANDLER, REPEATED_TRIGGERS,
		     DYNAMIC_SECOND_HANDLER, REPEATED_TRIGGERS);
	assert_observed_argument(DYNAMIC_FIRST_HANDLER, &dynamic_argument);
	assert_observed_argument(DYNAMIC_SECOND_HANDLER, second_argument);

	zassert_ok(irq_disconnect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_second_isr,
					  second_argument, TEST_IRQ_FLAGS));
	assert_entry(DYNAMIC_IRQ, dynamic_first_isr, &dynamic_argument);
	assert_empty_shared_slot(DYNAMIC_IRQ);
	irq_enable(DYNAMIC_IRQ);
	trigger_irq(DYNAMIC_IRQ);
	wait_for_handlers(1U);
	irq_disable(DYNAMIC_IRQ);
	assert_calls(DYNAMIC_FIRST_HANDLER, REPEATED_TRIGGERS + 1U,
		     DYNAMIC_SECOND_HANDLER, REPEATED_TRIGGERS);
	zassert_ok(irq_disconnect_dynamic(DYNAMIC_IRQ, TEST_IRQ_PRIORITY, dynamic_first_isr,
					  &dynamic_argument, TEST_IRQ_FLAGS));
	assert_entry(DYNAMIC_IRQ, z_irq_spurious, NULL);
	assert_empty_shared_slot(DYNAMIC_IRQ);
}
#endif
#endif

ZTEST_SUITE(isr_table_portability, NULL, setup, before, after, NULL);
