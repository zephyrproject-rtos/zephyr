/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define DEVICE_ID    0x01U
#define DEFAULT      0x09U
#define PULL_ENABLE  0x0BU
#define INPUT        0x0FU
#define MASK         0x11U
#define STATUS       0x13U
#define REGISTER_END 0x14U

static const struct device *const devices[] = {
	DEVICE_DT_GET(DT_NODELABEL(expander)),
	DEVICE_DT_GET(DT_NODELABEL(polled)),
};
static const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(DT_NODELABEL(expander), int_gpios);
static const struct i2c_dt_spec buses[] = {
	I2C_DT_SPEC_GET(DT_NODELABEL(expander)),
	I2C_DT_SPEC_GET(DT_NODELABEL(polled)),
};

struct chip {
	uint8_t regs[REGISTER_END];
	uint8_t input;
	uint8_t pending;
	uint8_t fail_reg;
	uint8_t first_pull;
	uint32_t failures;
	uint32_t id_reads;
	uint32_t acknowledgements;
	int64_t first_failure;
	int64_t last_failure;
	bool fail_read;
	bool lands;
	bool reset;
	bool pull_written;
};

static struct chip chips[2];
static K_MUTEX_DEFINE(model_lock);
static K_SEM_DEFINE(activity, 0, 1);
static struct gpio_callback callbacks[2];
static atomic_t calls[2][8];

static int update_irq(void)
{
	bool active = chips[0].reset || (chips[0].pending & ~chips[0].regs[MASK]) != 0U;

	return gpio_emul_input_set(irq.port, irq.pin, !active);
}

static int transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs, int addr)
{
	struct chip *chip = &chips[addr == 0x43 ? 0 : 1];
	bool read = num_msgs == 2;
	uint8_t reg = msgs[0].buf[0];
	bool fail;
	int ret;

	ARG_UNUSED(target);
	if ((addr != 0x43 && addr != 0x44) || reg >= REGISTER_END ||
	    (read &&
	     (msgs[0].len != 1U || msgs[1].len != 1U || (msgs[1].flags & I2C_MSG_READ) == 0U)) ||
	    (!read && (num_msgs != 1 || msgs[0].len != 2U))) {
		return -EIO;
	}

	k_mutex_lock(&model_lock, K_FOREVER);
	fail = chip->failures > 0U && chip->fail_reg == reg && chip->fail_read == read;
	if (fail) {
		if (chip->first_failure == 0) {
			chip->first_failure = k_uptime_get();
		}
		chip->last_failure = k_uptime_get();
		chip->failures--;
	}
	if (!fail || chip->lands) {
		if (read) {
			switch (reg) {
			case DEVICE_ID:
				msgs[1].buf[0] = 0xA0U | (chip->reset ? BIT(1) : 0U);
				chip->reset = false;
				chip->id_reads++;
				break;
			case INPUT:
				msgs[1].buf[0] = chip->input;
				break;
			case STATUS:
				msgs[1].buf[0] = chip->pending;
				chip->pending = 0U;
				break;
			default:
				msgs[1].buf[0] = chip->regs[reg];
				break;
			}
		} else if (reg == DEVICE_ID && (msgs[0].buf[1] & BIT(0)) != 0U) {
			memset(chip->regs, 0, sizeof(chip->regs));
			chip->regs[0x07U] = 0xFFU;
			chip->regs[PULL_ENABLE] = 0xFFU;
			chip->reset = true;
		} else {
			chip->regs[reg] = msgs[0].buf[1];
			if (reg == PULL_ENABLE && !chip->pull_written) {
				chip->first_pull = msgs[0].buf[1];
				chip->pull_written = true;
			}
		}
	}
	if (read && reg == STATUS && !fail) {
		chip->acknowledgements++;
	}
	ret = update_irq();
	k_mutex_unlock(&model_lock);
	k_sem_give(&activity);
	return ret < 0 ? ret : (fail ? -EIO : 0);
}

static int emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);
	return gpio_pin_configure_dt(&irq, GPIO_INPUT);
}

static const struct i2c_emul_api emul_api = {.transfer = transfer};

EMUL_DT_DEFINE(DT_NODELABEL(expander), emul_init, NULL, NULL, &emul_api, NULL);
EMUL_DT_DEFINE(DT_NODELABEL(polled), emul_init, NULL, NULL, &emul_api, NULL);

static void callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	size_t index = dev == devices[0] ? 0U : 1U;

	ARG_UNUSED(cb);
	zassert_equal_ptr(k_current_get(), k_work_queue_thread_get(&k_sys_work_q));
	zassert_not_equal(pins, 0U);
	for (size_t i = 0; i < 8U; i++) {
		if ((pins & BIT(i)) != 0U) {
			atomic_inc(&calls[index][i]);
		}
	}
	k_sem_give(&activity);
}

static void barrier_handler(struct k_work *work)
{
	ARG_UNUSED(work);
}

static K_WORK_DEFINE(barrier, barrier_handler);

static void flush(void)
{
	struct k_work_sync sync;

	zassert_true(k_work_submit(&barrier) >= 0);
	(void)k_work_flush(&barrier, &sync);
}

static void input_set(size_t index, uint8_t input)
{
	struct chip *chip = &chips[index];
	uint8_t changed;

	k_mutex_lock(&model_lock, K_FOREVER);
	changed = chip->input ^ input;
	chip->input = input;
	chip->pending |= changed & (input ^ chip->regs[DEFAULT]);
	zassert_ok(update_irq());
	k_mutex_unlock(&model_lock);
}

static uint32_t acknowledgements(void)
{
	uint32_t value;

	k_mutex_lock(&model_lock, K_FOREVER);
	value = chips[0].acknowledgements;
	k_mutex_unlock(&model_lock);
	return value;
}

static void wait_ack(uint32_t previous)
{
	int64_t deadline = k_uptime_get() + 1000;

	while (acknowledgements() <= previous) {
		zassert_true(k_uptime_get() < deadline, "Interrupt service did not recover");
		(void)k_sem_take(&activity, K_MSEC(5));
	}
	flush();
}

static void wait_calls(size_t index, size_t pin, atomic_val_t expected)
{
	int64_t deadline = k_uptime_get() + 1000;

	while (atomic_get(&calls[index][pin]) < expected) {
		zassert_true(k_uptime_get() < deadline, "Missing pin %zu callback", pin);
		(void)k_sem_take(&activity, K_MSEC(5));
	}
	flush();
	zassert_equal(atomic_get(&calls[index][pin]), expected);
}

static void fail_at(uint8_t reg, bool read, bool lands, uint32_t count)
{
	k_mutex_lock(&model_lock, K_FOREVER);
	chips[0].fail_reg = reg;
	chips[0].fail_read = read;
	chips[0].lands = lands;
	chips[0].failures = count;
	chips[0].first_failure = 0;
	chips[0].last_failure = 0;
	k_mutex_unlock(&model_lock);
}

static void *setup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]));
		gpio_init_callback(&callbacks[i], callback, 0xFFU);
		zassert_ok(gpio_add_callback(devices[i], &callbacks[i]));
	}
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	fail_at(0U, false, false, 0U);
	flush();
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		uint8_t scratch;

		for (size_t pin = 0; pin < 8U; pin++) {
			zassert_ok(gpio_pin_interrupt_configure(devices[i], pin, GPIO_INT_DISABLE));
		}
		input_set(i, 0U);
		for (size_t pin = 0; pin < 8U; pin++) {
			zassert_ok(gpio_pin_configure(devices[i], pin, GPIO_INPUT));
			zassert_ok(
				gpio_pin_interrupt_configure(devices[i], pin, GPIO_INT_EDGE_BOTH));
		}
		if (i == 1U) {
			k_msleep(10);
		}
		for (size_t pin = 0; pin < 8U; pin++) {
			zassert_ok(gpio_pin_interrupt_configure(devices[i], pin, GPIO_INT_DISABLE));
		}
		zassert_ok(i2c_reg_read_byte_dt(&buses[i], STATUS, &scratch));
	}
	flush();
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		for (size_t pin = 0; pin < 8U; pin++) {
			atomic_clear(&calls[i][pin]);
		}
	}
}

ZTEST(pi4ioe5v6408, test_reset_and_initial_pulls)
{
	for (size_t i = 0; i < ARRAY_SIZE(chips); i++) {
		zassert_equal(chips[i].id_reads, 1U);
		zassert_false(chips[i].reset);
		zassert_equal(chips[i].first_pull, 0xFEU);
	}
}

ZTEST(pi4ioe5v6408, test_all_pins_and_edge_modes)
{
	const gpio_flags_t modes[] = {GPIO_INT_EDGE_RISING, GPIO_INT_EDGE_FALLING,
				      GPIO_INT_EDGE_BOTH};

	for (size_t pin = 0; pin < 8U; pin++) {
		for (size_t mode = 0; mode < ARRAY_SIZE(modes); mode++) {
			atomic_clear(&calls[0][pin]);
			zassert_ok(gpio_pin_interrupt_configure(devices[0], pin, modes[mode]));
			for (size_t edge = 0; edge < 8U; edge++) {
				uint32_t previous = acknowledgements();

				input_set(0U, (edge & 1U) == 0U ? BIT(pin) : 0U);
				wait_ack(previous);
			}
			zassert_equal(atomic_get(&calls[0][pin]), mode == 2U ? 8 : 4);
			zassert_ok(gpio_pin_interrupt_configure(devices[0], pin, GPIO_INT_DISABLE));
		}
	}
}

ZTEST(pi4ioe5v6408, test_unsubscribed_interrupt_is_acknowledged)
{
	uint32_t previous = acknowledgements();

	k_mutex_lock(&model_lock, K_FOREVER);
	chips[0].regs[MASK] = 0U;
	k_mutex_unlock(&model_lock);
	input_set(0U, 1U);
	wait_ack(previous);
	zassert_equal(atomic_get(&calls[0][0]), 0);
	zassert_equal(gpio_pin_get_dt(&irq), 0);
}

static void recovery(uint8_t reg, bool read, bool lands, uint32_t failures)
{
	uint32_t previous = acknowledgements();

	zassert_ok(gpio_pin_interrupt_configure(devices[0], 0, GPIO_INT_EDGE_BOTH));
	fail_at(reg, read, lands, failures);
	input_set(0U, 1U);
	wait_ack(previous);
	wait_calls(0U, 0U, 1);
	zassert_equal(chips[0].regs[DEFAULT] & 1U, 1U);
	zassert_equal(gpio_pin_get_dt(&irq), 0);
	previous = acknowledgements();
	input_set(0U, 0U);
	wait_ack(previous);
	wait_calls(0U, 0U, 2);
}

ZTEST(pi4ioe5v6408, test_input_read_failure_recovers)
{
	recovery(INPUT, true, false, 1U);
}

ZTEST(pi4ioe5v6408, test_default_write_failure_recovers)
{
	recovery(DEFAULT, false, false, 1U);
}

ZTEST(pi4ioe5v6408, test_default_write_error_after_effect_recovers)
{
	recovery(DEFAULT, false, true, 1U);
}

ZTEST(pi4ioe5v6408, test_status_read_failure_recovers)
{
	recovery(STATUS, true, false, 1U);
}

ZTEST(pi4ioe5v6408, test_status_read_error_after_effect_recovers)
{
	recovery(STATUS, true, true, 1U);
}

ZTEST(pi4ioe5v6408, test_retries_are_delayed)
{
	recovery(INPUT, true, false, 3U);
	zassert_true(chips[0].last_failure - chips[0].first_failure >= 10,
		     "Repeated I2C errors caused immediate retries");
}

ZTEST(pi4ioe5v6408, test_polling_edges)
{
	zassert_ok(gpio_pin_interrupt_configure(devices[1], 0, GPIO_INT_EDGE_BOTH));
	for (size_t edge = 0; edge < 8U; edge++) {
		input_set(1U, (edge & 1U) == 0U ? 1U : 0U);
		wait_calls(1U, 0U, edge + 1U);
	}
}

ZTEST(pi4ioe5v6408, test_input_defaults_follow_each_subscribed_pin)
{
	const uint8_t levels[] = {0x01U, 0x81U, 0x80U, 0x00U};
	uint32_t previous = acknowledgements();

	zassert_ok(gpio_pin_interrupt_configure(devices[0], 0, GPIO_INT_EDGE_BOTH));
	input_set(0U, levels[0]);
	wait_ack(previous);
	zassert_ok(gpio_pin_interrupt_configure(devices[0], 7, GPIO_INT_EDGE_BOTH));
	zassert_equal(chips[0].regs[DEFAULT] & 0x81U, levels[0]);
	for (size_t i = 1; i < ARRAY_SIZE(levels); i++) {
		previous = acknowledgements();

		input_set(0U, levels[i]);
		wait_ack(previous);
		zassert_equal(chips[0].regs[DEFAULT] & 0x81U, levels[i]);
	}
	zassert_equal(atomic_get(&calls[0][0]), 2);
	zassert_equal(atomic_get(&calls[0][7]), 2);
}

ZTEST(pi4ioe5v6408, test_failed_configuration_keeps_previous_subscription)
{
	const uint8_t fail_regs[] = {INPUT, DEFAULT, MASK};

	for (size_t i = 0; i < ARRAY_SIZE(fail_regs); i++) {
		for (size_t lands = 0; lands < 2U; lands++) {
			uint32_t previous;

			before(NULL);
			zassert_ok(gpio_pin_interrupt_configure(devices[0], 7, GPIO_INT_EDGE_BOTH));
			fail_at(fail_regs[i], fail_regs[i] == INPUT, lands != 0U, 1U);
			zassert_equal(
				gpio_pin_interrupt_configure(devices[0], 0, GPIO_INT_EDGE_BOTH),
				-EIO);
			previous = acknowledgements();
			input_set(0U, 0x81U);
			wait_ack(previous);
			zassert_equal(atomic_get(&calls[0][0]), 0);
			zassert_equal(atomic_get(&calls[0][7]), 1);
		}
	}
}

ZTEST(pi4ioe5v6408, test_unsubscribed_status_failure_recovers)
{
	for (size_t lands = 0; lands < 2U; lands++) {
		uint32_t previous;

		before(NULL);
		previous = acknowledgements();
		fail_at(STATUS, true, lands != 0U, 1U);
		k_mutex_lock(&model_lock, K_FOREVER);
		chips[0].regs[MASK] = 0U;
		k_mutex_unlock(&model_lock);
		input_set(0U, 1U);
		wait_ack(previous);
		zassert_equal(atomic_get(&calls[0][0]), 0);
		zassert_equal(gpio_pin_get_dt(&irq), 0);
	}
}

ZTEST(pi4ioe5v6408, test_captured_callback_is_not_repeated)
{
	recovery(DEFAULT, false, false, 3U);
}

ZTEST_SUITE(pi4ioe5v6408, NULL, setup, before, NULL, NULL);
