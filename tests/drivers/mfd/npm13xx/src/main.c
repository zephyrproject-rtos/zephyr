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

#define MAIN_SIZE   0x26U
#define CHG_EVENTS  0x0BU
#define VBUS_EVENTS 0x17U

static const struct device *const pmic = DEVICE_DT_GET(DT_NODELABEL(pmic));
static const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(DT_NODELABEL(pmic), host_int_gpios);
static const uint8_t event_offsets[] = {0x0BU, 0x0FU, 0x13U, 0x17U, 0x23U};

static struct {
	uint8_t pending[MAIN_SIZE];
	uint8_t enabled[MAIN_SIZE];
	uint32_t reads;
	uint32_t clears;
	bool injecting;
	bool fail_read;
	bool fail_clear;
	bool failed_clear_lands;
} registers;

static K_SEM_DEFINE(idle, 0, 1);

static bool irq_pending(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(event_offsets); i++) {
		uint8_t offset = event_offsets[i];

		if ((registers.pending[offset] & registers.enabled[offset]) != 0U) {
			return true;
		}
	}
	return false;
}

static int transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{
	ARG_UNUSED(target);
	ARG_UNUSED(addr);

	if (num_msgs == 2 && msgs[0].len == 2U && msgs[0].buf[0] == 0U && msgs[0].buf[1] == 0U &&
	    msgs[1].len == MAIN_SIZE && (msgs[1].flags & I2C_MSG_READ) != 0U) {
		registers.reads++;
		if (registers.fail_read) {
			registers.fail_read = false;
			return -EIO;
		}
		memcpy(msgs[1].buf, registers.pending, MAIN_SIZE);
		if (registers.injecting && !irq_pending()) {
			k_sem_give(&idle);
		}
		return 0;
	}

	if (num_msgs != 1 || msgs[0].len != 3U || (msgs[0].flags & I2C_MSG_READ) != 0U) {
		return -EIO;
	}

	/* The other banks only receive device-initialization writes in this fixture. */
	if (msgs[0].buf[0] != 0U) {
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(event_offsets); i++) {
		uint8_t offset = event_offsets[i];

		if (msgs[0].buf[1] == offset + 1U) {
			registers.enabled[offset] |= msgs[0].buf[2];
			return 0;
		}
		if (msgs[0].buf[1] == offset) {
			bool fail = registers.fail_clear;

			registers.clears++;
			registers.fail_clear = false;
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
	struct mfd_npm13xx_event_callback cb;
	npm13xx_event_t events[4];
	uint32_t calls;
	uint32_t min_clears;
	bool remove_self;
};

static struct subscriber subscribers[3];

static void handler(const struct device *dev, struct mfd_npm13xx_event_callback *cb,
		    npm13xx_event_t events)
{
	struct subscriber *sub = CONTAINER_OF(cb, struct subscriber, cb);

	zassert_equal_ptr(dev, pmic);
	zassert_equal_ptr(k_current_get(), k_work_queue_thread_get(&k_sys_work_q));
	zassert_not_equal(events, 0U);
	zassert_equal(events & ~cb->event_mask, 0U);
	zassert_true(registers.clears >= sub->min_clears, "Callback ran before acknowledgment");
	zassert_true(sub->calls < ARRAY_SIZE(sub->events));
	sub->events[sub->calls++] = events;
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
	registers.injecting = true;
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 1));
	zassert_ok(k_sem_take(&idle, K_SECONDS(1)), "Event processing did not reach idle");
	/* The emulator signals before the driver finishes invoking handlers. */
	zassert_true(k_work_submit(&barrier) >= 0);
	(void)k_work_flush(&barrier, &sync);
	registers.injecting = false;
}

static void subscribe(size_t index, npm13xx_event_t mask)
{
	subscribers[index].cb.handler = handler;
	subscribers[index].cb.event_mask = mask;
	zassert_ok(mfd_npm13xx_add_callback(pmic, &subscribers[index].cb));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(pmic));
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		int ret = mfd_npm13xx_remove_callback(pmic, &subscribers[i].cb);

		zassert_true(ret == 0 || ret == -EINVAL);
	}
	zassert_ok(gpio_emul_input_set(irq.port, irq.pin, 0));
	memset(&registers, 0, sizeof(registers));
	memset(subscribers, 0, sizeof(subscribers));
	k_sem_reset(&idle);
}

ZTEST(npm13xx_callbacks, test_aggregate_matching_events_after_acknowledgment)
{
	npm13xx_event_t charge = BIT(NPM13XX_EVENT_CHG_COMPLETED);
	npm13xx_event_t vbus = BIT(NPM13XX_EVENT_VBUS_DETECTED);

	subscribe(0, charge | vbus);
	subscribe(1, vbus);
	subscribe(2, BIT(NPM13XX_EVENT_CHG_ERROR));
	subscribers[0].min_clears = 2U;
	subscribers[1].min_clears = 2U;
	registers.pending[CHG_EVENTS] = 0x10U;
	registers.pending[VBUS_EVENTS] = 0x01U;
	dispatch();

	zassert_equal(subscribers[0].calls, 1U);
	zassert_equal(subscribers[0].events[0], charge | vbus);
	zassert_equal(subscribers[1].calls, 1U);
	zassert_equal(subscribers[1].events[0], vbus);
	zassert_equal(subscribers[2].calls, 0U);
	zassert_equal(registers.clears, 2U);
}

ZTEST(npm13xx_callbacks, test_self_removal_preserves_next_callback)
{
	npm13xx_event_t mask = BIT(NPM13XX_EVENT_VBUS_DETECTED);

	subscribe(0, mask);
	subscribe(1, mask);
	subscribers[1].remove_self = true;
	registers.pending[VBUS_EVENTS] = 0x01U;
	dispatch();
	zassert_equal(subscribers[0].calls, 1U);
	zassert_equal(subscribers[1].calls, 1U);

	registers.pending[VBUS_EVENTS] = 0x01U;
	dispatch();
	zassert_equal(subscribers[0].calls, 2U);
	zassert_equal(subscribers[1].calls, 1U);
}

static void clear_error(bool lands)
{
	npm13xx_event_t mask = BIT(NPM13XX_EVENT_VBUS_DETECTED);

	subscribe(0, mask);
	registers.pending[VBUS_EVENTS] = 0x01U;
	registers.fail_clear = true;
	registers.failed_clear_lands = lands;
	dispatch();
	zassert_equal(registers.reads, 2U);
	zassert_equal(registers.clears, lands ? 1U : 2U);
	zassert_equal(subscribers[0].calls, lands ? 1U : 2U);
	for (size_t i = 0; i < subscribers[0].calls; i++) {
		zassert_equal(subscribers[0].events[i], mask);
	}
}

ZTEST(npm13xx_callbacks, test_failed_clear_retries_pending_event)
{
	clear_error(false);
}

ZTEST(npm13xx_callbacks, test_failed_clear_that_lands_still_notifies)
{
	clear_error(true);
}

ZTEST(npm13xx_callbacks, test_read_failure_retries)
{
	subscribe(0, BIT(NPM13XX_EVENT_VBUS_DETECTED));
	registers.pending[VBUS_EVENTS] = 0x01U;
	registers.fail_read = true;
	dispatch();
	zassert_equal(registers.reads, 2U);
	zassert_equal(subscribers[0].calls, 1U);
	zassert_equal(subscribers[0].events[0], BIT(NPM13XX_EVENT_VBUS_DETECTED));
}

ZTEST(npm13xx_callbacks, test_registration_validation)
{
	struct mfd_npm13xx_event_callback *cb = &subscribers[0].cb;

	zassert_equal(mfd_npm13xx_add_callback(pmic, NULL), -EINVAL);
	cb->event_mask = BIT(NPM13XX_EVENT_VBUS_DETECTED);
	zassert_equal(mfd_npm13xx_add_callback(pmic, cb), -EINVAL);
	cb->handler = handler;
	cb->event_mask = 0U;
	zassert_equal(mfd_npm13xx_add_callback(pmic, cb), -EINVAL);
	cb->event_mask = BIT(NPM13XX_EVENT_MAX);
	zassert_equal(mfd_npm13xx_add_callback(pmic, cb), -EINVAL);
	cb->event_mask = BIT(NPM13XX_EVENT_VBUS_DETECTED);
	zassert_ok(mfd_npm13xx_add_callback(pmic, cb));
	zassert_equal(mfd_npm13xx_add_callback(pmic, cb), -EINVAL);
	zassert_ok(mfd_npm13xx_remove_callback(pmic, cb));
	zassert_equal(mfd_npm13xx_remove_callback(pmic, cb), -EINVAL);
	zassert_equal(mfd_npm13xx_remove_callback(pmic, NULL), -EINVAL);
}

ZTEST_SUITE(npm13xx_callbacks, NULL, NULL, before, NULL, NULL);
