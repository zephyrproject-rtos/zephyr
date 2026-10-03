/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/mfd/npm2100.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define EVENT_REGS 5U
#define EVENTS_CLR 0x05U
#define INTEN_SET  0x0AU

static const struct device *const pmic = DEVICE_DT_GET(DT_NODELABEL(pmic));
static const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(DT_NODELABEL(pmic), host_int_gpios);

static struct {
	uint8_t pending[EVENT_REGS];
	uint8_t enabled[EVENT_REGS];
	uint8_t after_clear[EVENT_REGS];
	uint32_t reads;
	uint32_t clears;
	bool fail_read;
	bool fail_clear;
	bool failed_clear_lands;
	bool fail_enable;
} registers;

static K_SEM_DEFINE(cleared, 0, 1);

static bool irq_pending(void)
{
	for (size_t i = 0; i < EVENT_REGS; i++) {
		if ((registers.pending[i] & registers.enabled[i]) != 0U) {
			return true;
		}
	}
	return false;
}

/* Only the event register transactions need emulation; accept the init writes. */
static int transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{
	uint8_t reg;

	ARG_UNUSED(target);
	ARG_UNUSED(addr);

	if (num_msgs == 2 && msgs[0].len == 1U && msgs[0].buf[0] == 0U &&
	    msgs[1].len == EVENT_REGS && (msgs[1].flags & I2C_MSG_READ) != 0U) {
		registers.reads++;
		if (registers.fail_read) {
			registers.fail_read = false;
			return -EIO;
		}
		memcpy(msgs[1].buf, registers.pending, EVENT_REGS);
		return 0;
	}

	if (num_msgs != 1 || msgs[0].len < 2U || (msgs[0].flags & I2C_MSG_READ) != 0U) {
		return -EIO;
	}

	reg = msgs[0].buf[0];

	if (reg >= INTEN_SET && reg < INTEN_SET + EVENT_REGS && msgs[0].len == 2U) {
		if (registers.fail_enable) {
			registers.fail_enable = false;
			return -EIO;
		}
		registers.enabled[reg - INTEN_SET] |= msgs[0].buf[1];
	} else if (reg >= EVENTS_CLR && reg < EVENTS_CLR + EVENT_REGS) {
		bool dispatch_clear = msgs[0].len == EVENT_REGS + 1U;
		bool fail = dispatch_clear && registers.fail_clear;
		bool active = false;

		if (dispatch_clear) {
			registers.clears++;
			registers.fail_clear = false;
		}
		if (!fail || registers.failed_clear_lands) {
			for (size_t i = 1U; i < msgs[0].len; i++) {
				size_t offset = reg - EVENTS_CLR + i - 1U;

				if (offset >= EVENT_REGS) {
					return -EIO;
				}
				registers.pending[offset] &= ~msgs[0].buf[i];
			}
			if (dispatch_clear) {
				int ret;

				for (size_t i = 0; i < EVENT_REGS; i++) {
					registers.pending[i] |= registers.after_clear[i];
					registers.after_clear[i] = 0U;
				}
				active = irq_pending();
				ret = gpio_emul_input_set(irq.port, irq.pin, active);

				if (ret < 0) {
					return ret;
				}
			}
		}
		if (fail) {
			return -EIO;
		}
		if (dispatch_clear && !active) {
			k_sem_give(&cleared);
		}
	}

	return 0;
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
	struct mfd_npm2100_event_callback cb;
	npm2100_event_t events[8];
	uint32_t calls;
	bool remove_self;
};

static struct subscriber subscribers[3];

static void handler(const struct device *dev, struct mfd_npm2100_event_callback *cb,
		    npm2100_event_t events)
{
	struct subscriber *sub = CONTAINER_OF(cb, struct subscriber, cb);

	zassert_equal_ptr(dev, pmic);
	zassert_equal_ptr(k_current_get(), k_work_queue_thread_get(&k_sys_work_q));
	zassert_not_equal(events, 0U);
	zassert_equal(events & (events - 1U), 0U, "Expected one event per call");
	zassert_equal(events & ~cb->event_mask, 0U);
	zassert_true(sub->calls < ARRAY_SIZE(sub->events));
	sub->events[sub->calls++] = events;
	if (sub->remove_self) {
		zassert_ok(mfd_npm2100_remove_callback(dev, cb));
	}
}

static void before_clear_handler(const struct device *dev, struct mfd_npm2100_event_callback *cb,
				 npm2100_event_t events)
{
	zassert_equal(registers.clears, 0U, "Events cleared before callback dispatch");
	handler(dev, cb, events);
}

static void barrier_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

static K_WORK_DEFINE(barrier, barrier_handler);

static void wait_for_dispatch(void)
{
	struct k_work_sync sync;

	zassert_ok(k_sem_take(&cleared, K_SECONDS(1)), "No successful event clear");
	/* The emulator signals before the driver work item returns. */
	zassert_true(k_work_submit(&barrier) >= 0);
	(void)k_work_flush(&barrier, &sync);
}

static void fire(uint8_t offset, uint8_t mask)
{
	registers.pending[offset] |= mask;
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 1));
	wait_for_dispatch();
}

static void subscribe(size_t index, npm2100_event_t mask)
{
	subscribers[index].cb.handler = handler;
	subscribers[index].cb.event_mask = mask;
	zassert_ok(mfd_npm2100_add_callback(pmic, &subscribers[index].cb));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(pmic));
	zassert_true(gpio_is_ready_dt(&irq));
	memset(&registers, 0, sizeof(registers));
	memset(subscribers, 0, sizeof(subscribers));
	k_sem_reset(&cleared);
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 0));
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		int ret = mfd_npm2100_remove_callback(pmic, &subscribers[i].cb);

		zassert_true(ret == 0 || ret == -EINVAL);
	}
}

ZTEST(npm2100, test_event_mapping_and_filtering)
{
	subscribe(0, BIT(NPM2100_EVENT_ADC_VBAT_READY) | BIT(NPM2100_EVENT_LDOSW_VINTFAIL));
	subscribe(1, BIT(NPM2100_EVENT_BOOST_VBAT_WARN));

	zassert_equal(registers.enabled[1], BIT(0));
	zassert_equal(registers.enabled[3], BIT(0));
	zassert_equal(registers.enabled[4], BIT(1));
	registers.pending[1] = BIT(0);
	registers.pending[3] = BIT(0);
	registers.pending[4] = BIT(1);
	fire(0, BIT(0)); /* This event has no subscriber. */

	zassert_equal(subscribers[0].calls, 2U);
	zassert_equal(subscribers[0].events[0], BIT(NPM2100_EVENT_ADC_VBAT_READY));
	zassert_equal(subscribers[0].events[1], BIT(NPM2100_EVENT_LDOSW_VINTFAIL));
	zassert_equal(subscribers[1].calls, 1U);
	zassert_equal(subscribers[1].events[0], BIT(NPM2100_EVENT_BOOST_VBAT_WARN));
	zassert_equal(registers.reads, 1U);
	zassert_equal(registers.clears, 1U);
}

ZTEST(npm2100, test_callbacks_precede_clear)
{
	subscribers[0].cb.event_mask =
		BIT(NPM2100_EVENT_SYS_DIETEMP_WARN) | BIT(NPM2100_EVENT_ADC_VBAT_READY);
	subscribers[1].cb.event_mask = BIT(NPM2100_EVENT_ADC_VBAT_READY);
	for (size_t i = 0; i < 2U; i++) {
		subscribers[i].cb.handler = before_clear_handler;
		zassert_ok(mfd_npm2100_add_callback(pmic, &subscribers[i].cb));
	}
	registers.pending[1] = BIT(0);
	fire(0, BIT(0));

	zassert_equal(subscribers[0].calls, 2U);
	zassert_equal(subscribers[0].events[0], BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	zassert_equal(subscribers[0].events[1], BIT(NPM2100_EVENT_ADC_VBAT_READY));
	zassert_equal(subscribers[1].calls, 1U);
	zassert_equal(subscribers[1].events[0], BIT(NPM2100_EVENT_ADC_VBAT_READY));
	zassert_equal(registers.reads, 1U);
	zassert_equal(registers.clears, 1U);
	zassert_equal(registers.pending[0], 0U);
	zassert_equal(registers.pending[1], 0U);
}

ZTEST(npm2100, test_self_removal_preserves_next_subscriber)
{
	npm2100_event_t mask = BIT(NPM2100_EVENT_SYS_DIETEMP_WARN);

	subscribe(0, mask);
	subscribe(1, mask);
	subscribers[1].remove_self = true; /* Newest subscriber is visited first. */
	fire(0, BIT(0));
	zassert_equal(subscribers[1].calls, 1U);
	zassert_equal(subscribers[0].calls, 1U);
	fire(0, BIT(0));
	zassert_equal(subscribers[1].calls, 1U);
	zassert_equal(subscribers[0].calls, 2U);
}

ZTEST(npm2100, test_event_after_clear_is_delivered)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN) | BIT(NPM2100_EVENT_ADC_VBAT_READY));
	registers.after_clear[1] = BIT(0);
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 2U);
	zassert_equal(subscribers[0].events[0], BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	zassert_equal(subscribers[0].events[1], BIT(NPM2100_EVENT_ADC_VBAT_READY));
	zassert_equal(registers.reads, 2U);
	zassert_equal(registers.clears, 2U);
}

ZTEST(npm2100, test_duplicate_registration)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	zassert_ok(mfd_npm2100_add_callback(pmic, &subscribers[0].cb));
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 1U);
	zassert_ok(mfd_npm2100_remove_callback(pmic, &subscribers[0].cb));
	zassert_equal(mfd_npm2100_remove_callback(pmic, &subscribers[0].cb), -EINVAL);
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 1U);
}

ZTEST(npm2100, test_null_callback_and_handler)
{
	zassert_equal(mfd_npm2100_add_callback(pmic, NULL), -EINVAL);
	zassert_equal(mfd_npm2100_add_callback(pmic, &subscribers[0].cb), -EINVAL);
	zassert_equal(mfd_npm2100_remove_callback(pmic, NULL), -EINVAL);
}

ZTEST(npm2100, test_unmapped_mask_bits_do_not_match)
{
	subscribe(0, 0U);
	subscribe(1, BIT(31));
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 0U);
	zassert_equal(subscribers[1].calls, 0U);
}

ZTEST(npm2100, test_failed_reregistration_keeps_callback)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	registers.fail_enable = true;
	zassert_equal(mfd_npm2100_add_callback(pmic, &subscribers[0].cb), -EIO);
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 1U);
}

ZTEST(npm2100, test_read_failure_retries)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	registers.fail_read = true;
	fire(0, BIT(0));
	zassert_equal(registers.reads, 2U);
	zassert_equal(subscribers[0].calls, 1U);
}

ZTEST(npm2100, test_clear_failure_retries_notification)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	registers.fail_clear = true;
	fire(0, BIT(0));
	zassert_equal(registers.clears, 2U);
	zassert_equal(subscribers[0].calls, 2U);
}

ZTEST(npm2100, test_clear_error_after_write_does_not_lose_event)
{
	subscribe(0, BIT(NPM2100_EVENT_SYS_DIETEMP_WARN));
	registers.fail_clear = true;
	registers.failed_clear_lands = true;
	fire(0, BIT(0));
	zassert_equal(registers.clears, 2U);
	zassert_equal(subscribers[0].calls, 1U);
}

ZTEST(npm2100, test_enable_failure_does_not_register_callback)
{
	subscribers[0].cb.handler = handler;
	subscribers[0].cb.event_mask = BIT(NPM2100_EVENT_SYS_DIETEMP_WARN);
	registers.fail_enable = true;
	zassert_equal(mfd_npm2100_add_callback(pmic, &subscribers[0].cb), -EIO);
	zassert_equal(mfd_npm2100_remove_callback(pmic, &subscribers[0].cb), -EINVAL);
	fire(0, BIT(0));
	zassert_equal(subscribers[0].calls, 0U);
}

ZTEST_SUITE(npm2100, NULL, NULL, before, after, NULL);
