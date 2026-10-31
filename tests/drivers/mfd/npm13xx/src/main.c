/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define MAIN_SIZE 0x26U
#define CHARGE    BIT(NPM13XX_EVENT_CHG_COMPLETED)
#define VBUS      BIT(NPM13XX_EVENT_VBUS_DETECTED)

static const struct device *const pmic = DEVICE_DT_GET(DT_NODELABEL(pmic));
static const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(DT_NODELABEL(pmic), host_int_gpios);
static const uint8_t offsets[] = {0x0BU, 0x0FU, 0x13U, 0x17U, 0x23U};
static const struct {
	uint8_t offset;
	uint8_t mask;
} event_registers[] = {
	{0x0BU, 0x10U}, {0x0BU, 0x20U}, {0x0FU, 0x01U}, {0x0FU, 0x02U}, {0x13U, 0x01U},
	{0x13U, 0x02U}, {0x13U, 0x08U}, {0x17U, 0x01U}, {0x17U, 0x02U}, {0x23U, 0x01U},
	{0x23U, 0x02U}, {0x23U, 0x04U}, {0x23U, 0x08U}, {0x23U, 0x10U},
};

static struct {
	uint8_t pending[MAIN_SIZE];
	uint8_t enabled[MAIN_SIZE];
	uint32_t reads;
	uint32_t clears;
	uint32_t fail_clear_at;
	bool failed_clear_lands;
	bool fail_read;
	bool injecting;
} registers;

struct call {
	uint32_t subscriber;
	uint32_t events;
	uint32_t earlier_clears;
	uint32_t pending_events;
};

static struct call calls[3 * ARRAY_SIZE(event_registers)];
static uint32_t call_count;
static K_SEM_DEFINE(idle, 0, 1);

static bool irq_pending(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(offsets); i++) {
		if ((registers.pending[offsets[i]] & registers.enabled[offsets[i]]) != 0U) {
			return true;
		}
	}
	return false;
}

static int transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{
	ARG_UNUSED(target);
	ARG_UNUSED(addr);

	if (num_msgs == 2 && msgs[0].len == 2U && msgs[0].buf[0] == 0U &&
	    (msgs[1].flags & I2C_MSG_READ) != 0U) {
		uint8_t offset = msgs[0].buf[1];

		if (offset >= MAIN_SIZE || msgs[1].len > MAIN_SIZE - offset) {
			return -EIO;
		}
		registers.reads++;
		if (registers.fail_read) {
			registers.fail_read = false;
			return -EIO;
		}
		memcpy(msgs[1].buf, &registers.pending[offset], msgs[1].len);
		if (registers.injecting && !irq_pending()) {
			k_sem_give(&idle);
		}
		return 0;
	}

	if (num_msgs != 1 || msgs[0].len != 3U || (msgs[0].flags & I2C_MSG_READ) != 0U) {
		return -EIO;
	}
	if (msgs[0].buf[0] != 0U) {
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(offsets); i++) {
		uint8_t offset = offsets[i];

		if (msgs[0].buf[1] == offset + 1U) {
			registers.enabled[offset] |= msgs[0].buf[2];
			return 0;
		}
		if (msgs[0].buf[1] == offset) {
			bool fail = ++registers.clears == registers.fail_clear_at;

			if (!fail || registers.failed_clear_lands) {
				registers.pending[offset] &= ~msgs[0].buf[2];
				if (registers.injecting) {
					int ret = gpio_emul_input_set(irq.port, irq.pin,
								      irq_pending());

					if (ret < 0) {
						return ret;
					}
				}
			}
			if (fail) {
				return -EIO;
			}
			if (registers.injecting && !irq_pending()) {
				k_sem_give(&idle);
			}
			return 0;
		}
	}
	return -EIO;
}

static const struct i2c_emul_api emul_api = {
	.transfer = transfer,
};

static int emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);
	return 0;
}

EMUL_DT_DEFINE(DT_NODELABEL(pmic), emul_init, NULL, NULL, &emul_api, NULL);

struct subscriber {
	struct gpio_callback cb;
	uint32_t index;
	bool remove_self;
};

static struct subscriber subscribers[3];

static void handler(const struct device *dev, struct gpio_callback *cb, uint32_t events)
{
	struct subscriber *sub = CONTAINER_OF(cb, struct subscriber, cb);
	uint32_t pending = 0U;

	zassert_equal_ptr(dev, pmic);
	zassert_equal_ptr(k_current_get(), k_work_queue_thread_get(&k_sys_work_q));
	zassert_not_equal(events, 0U);
	zassert_equal(events & (events - 1U), 0U, "Expected one event per call");
	zassert_equal(events & ~cb->pin_mask, 0U);
	zassert_true(call_count < ARRAY_SIZE(calls));
	for (size_t i = 0; i < ARRAY_SIZE(event_registers); i++) {
		uint8_t latched = registers.pending[event_registers[i].offset];

		if ((latched & event_registers[i].mask) != 0U) {
			pending |= BIT(i);
		}
	}
	calls[call_count++] = (struct call){sub->index, events, registers.clears, pending};
	if (sub->remove_self) {
		zassert_ok(mfd_npm13xx_remove_callback(dev, cb));
	}
}

static void barrier_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

static K_WORK_DEFINE(barrier, barrier_handler);

static void dispatch(void)
{
	struct k_work_sync sync;

	registers.reads = 0U;
	registers.clears = 0U;
	zassert_true(irq_pending(), "Pending event interrupt is not enabled");
	registers.injecting = true;
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 1));
	zassert_ok(k_sem_take(&idle, K_SECONDS(1)), "Event processing did not reach idle");
	zassert_true(k_work_submit(&barrier) >= 0);
	(void)k_work_flush(&barrier, &sync);
	registers.injecting = false;
}

static void subscribe(size_t index, uint32_t mask)
{
	subscribers[index].cb.handler = handler;
	subscribers[index].cb.pin_mask = mask;
	subscribers[index].index = index;
	zassert_ok(mfd_npm13xx_add_callback(pmic, &subscribers[index].cb));
	for (size_t i = 0; i < ARRAY_SIZE(event_registers); i++) {
		if ((mask & BIT(i)) != 0U) {
			uint8_t enabled = registers.enabled[event_registers[i].offset];

			zassert_equal(enabled & event_registers[i].mask, event_registers[i].mask,
				      "Subscribed event interrupt is not enabled");
		}
	}
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(pmic));
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		subscribers[i].cb.handler = handler;
		int ret = mfd_npm13xx_remove_callback(pmic, &subscribers[i].cb);

		zassert_true(ret == 0 || ret == -EINVAL);
	}
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 0));
	memset(&registers, 0, sizeof(registers));
	memset(subscribers, 0, sizeof(subscribers));
	memset(calls, 0, sizeof(calls));
	call_count = 0U;
	k_sem_reset(&idle);
}

static void expect_call(uint32_t position, uint32_t subscriber, uint32_t event,
			uint32_t earlier_clears)
{
	zassert_true(position < call_count);
	zassert_equal(calls[position].subscriber, subscriber);
	zassert_equal(calls[position].events, event);
	zassert_equal(calls[position].earlier_clears, earlier_clears);
	zassert_equal(calls[position].pending_events & event, event,
		      "Event cleared before notification");
}

ZTEST(npm13xx_callbacks, test_two_events_notify_before_each_clear)
{
	subscribe(0, CHARGE | VBUS);
	subscribe(1, CHARGE | VBUS);
	registers.pending[0x0B] = 0x10U;
	registers.pending[0x17] = 0x01U;

	dispatch();
	zassert_equal(call_count, 4U);
	expect_call(0, 1, CHARGE, 0);
	expect_call(1, 0, CHARGE, 0);
	expect_call(2, 1, VBUS, 1);
	expect_call(3, 0, VBUS, 1);
	zassert_equal(registers.clears, 2U);
}

ZTEST(npm13xx_callbacks, test_filters_subscribers)
{
	subscribe(0, CHARGE);
	subscribe(1, VBUS);
	subscribe(2, BIT(NPM13XX_EVENT_CHG_ERROR));
	registers.pending[0x0B] = 0x10U;
	registers.pending[0x17] = 0x01U;
	dispatch();
	zassert_equal(call_count, 2U);
	expect_call(0, 0, CHARGE, 0);
	expect_call(1, 1, VBUS, 1);
}

ZTEST(npm13xx_callbacks, test_self_removal_preserves_other_subscriber)
{
	subscribe(0, CHARGE | VBUS);
	subscribe(1, CHARGE | VBUS);
	subscribers[1].remove_self = true;
	registers.pending[0x0B] = 0x10U;
	registers.pending[0x17] = 0x01U;
	dispatch();
	zassert_equal(call_count, 3U);
	expect_call(0, 1, CHARGE, 0);
	expect_call(1, 0, CHARGE, 0);
	expect_call(2, 0, VBUS, 1);
}

ZTEST(npm13xx_callbacks, test_repeated_add_moves_callback_without_duplication)
{
	subscribe(0, VBUS);
	subscribe(1, VBUS);
	zassert_ok(mfd_npm13xx_add_callback(pmic, &subscribers[0].cb));
	registers.pending[0x17] = 0x01U;
	dispatch();
	zassert_equal(call_count, 2U);
	expect_call(0, 0, VBUS, 0);
	expect_call(1, 1, VBUS, 0);
}

static void check_clear_error(uint32_t failure, bool lands)
{
	subscribe(0, CHARGE | VBUS);
	registers.pending[0x0B] = 0x10U;
	registers.pending[0x17] = 0x01U;
	registers.fail_clear_at = failure;
	registers.failed_clear_lands = lands;
	dispatch();
	zassert_true(registers.reads >= 2U);
	zassert_equal(registers.clears, lands ? 2U : 3U);
	zassert_equal(call_count, lands ? 2U : 3U);
	expect_call(0, 0, CHARGE, 0);
	if (failure == 1U && !lands) {
		expect_call(1, 0, CHARGE, 1);
		expect_call(2, 0, VBUS, 2);
	} else {
		expect_call(1, 0, VBUS, 1);
		if (!lands) {
			expect_call(2, 0, VBUS, 2);
		}
	}
}

ZTEST(npm13xx_callbacks, test_clear_errors_retry_captured_events)
{
	for (uint32_t failure = 1U; failure <= 2U; failure++) {
		for (uint32_t lands = 0U; lands < 2U; lands++) {
			before(NULL);
			check_clear_error(failure, lands != 0U);
		}
	}
}

ZTEST(npm13xx_callbacks, test_read_failure_retries)
{
	subscribe(0, VBUS);
	registers.pending[0x17] = 0x01U;
	registers.fail_read = true;
	dispatch();
	zassert_true(registers.reads >= 2U);
	zassert_equal(call_count, 1U);
	expect_call(0, 0, VBUS, 0);
}

ZTEST(npm13xx_callbacks, test_remove_registered_and_unregistered_callback)
{
	subscribers[0].cb.handler = handler;
	zassert_equal(mfd_npm13xx_remove_callback(pmic, &subscribers[0].cb), -EINVAL);
	subscribe(0, VBUS);
	zassert_ok(mfd_npm13xx_remove_callback(pmic, &subscribers[0].cb));
	zassert_equal(mfd_npm13xx_remove_callback(pmic, &subscribers[0].cb), -EINVAL);
	registers.pending[0x17] = 0x01U;
	dispatch();
	zassert_equal(call_count, 0U);
	zassert_equal(registers.clears, 1U);
}

ZTEST(npm13xx_callbacks, test_all_event_bit_mappings)
{
	zassert_equal(ARRAY_SIZE(event_registers), NPM13XX_EVENT_MAX);
	for (size_t i = 0; i < ARRAY_SIZE(event_registers); i++) {
		subscribe(0, BIT(i));
		registers.pending[event_registers[i].offset] = event_registers[i].mask;
		dispatch();
		zassert_equal(call_count, 1U);
		expect_call(0, 0, BIT(i), 0);
		zassert_ok(mfd_npm13xx_remove_callback(pmic, &subscribers[0].cb));
		call_count = 0U;
	}
}

ZTEST(npm13xx_callbacks, test_same_bank_events_remain_pending_until_notification)
{
	uint32_t first = BIT(NPM13XX_EVENT_GPIO1_EDGE);
	uint32_t second = BIT(NPM13XX_EVENT_GPIO2_EDGE);

	subscribe(0, first | second);
	registers.pending[0x23] = 0x06U;
	dispatch();
	zassert_equal(call_count, 2U);
	expect_call(0, 0, first, 0);
	expect_call(1, 0, second, 1);
}

ZTEST_SUITE(npm13xx_callbacks, NULL, NULL, before, NULL, NULL);
