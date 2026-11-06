/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/drivers/tlc6c59xx.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest.h>

#define WIDTH DT_PROP(DT_NODELABEL(chip0), ngpios)
#define PORT_MASK BIT_MASK(WIDTH)
#define FRAME_SIZE DIV_ROUND_UP(3U * WIDTH, 8U)

struct chain_emul_data {
	uint8_t frame[FRAME_SIZE];
	uint32_t calls;
	int next_error;
	bool delay;
};

static int chain_io(const struct emul *emul, const struct spi_config *config,
		    const struct spi_buf_set *tx, const struct spi_buf_set *rx)
{
	struct chain_emul_data *data = emul->data;
	int ret = data->next_error;

	zassert_equal(SPI_WORD_SIZE_GET(config->operation), 8U);
	zassert_equal(config->operation & SPI_TRANSFER_LSB, 0U);
	zassert_is_null(rx);
	zassert_not_null(tx);
	zassert_equal(tx->count, 1U);
	zassert_equal(tx->buffers[0].len, FRAME_SIZE);
	data->calls++;
	data->next_error = 0;
	if (data->delay) {
		/* Let a second writer run while the first is waiting for SPI. */
		k_sleep(K_MSEC(1));
	}
	if (ret == 0) {
		memcpy(data->frame, tx->buffers[0].buf, FRAME_SIZE);
	}
	return ret;
}

static const struct spi_emul_api chain_api = {
	.io = chain_io,
};

static int chain_init(const struct emul *emul, const struct device *bus)
{
	ARG_UNUSED(emul);
	ARG_UNUSED(bus);
	return 0;
}

static struct chain_emul_data chain_data;
EMUL_DT_DEFINE(DT_NODELABEL(chain), chain_init, &chain_data, NULL, &chain_api, NULL);

struct tlc6c59xx_fixture {
	const struct device *parent;
	const struct device *chips[3];
	struct chain_emul_data *emul;
};

static void *setup(void)
{
	static struct tlc6c59xx_fixture fixture = {
		.parent = DEVICE_DT_GET(DT_NODELABEL(chain)),
		.chips = {
			DEVICE_DT_GET(DT_NODELABEL(chip0)),
			DEVICE_DT_GET(DT_NODELABEL(chip1)),
			DEVICE_DT_GET(DT_NODELABEL(chip2)),
		},
		.emul = &chain_data,
	};

	zassert_true(device_is_ready(fixture.parent));
	for (size_t i = 0U; i < ARRAY_SIZE(fixture.chips); i++) {
		zassert_true(device_is_ready(fixture.chips[i]));
	}
	/* Parent initialization must send one all-zero chain frame. */
	zassert_equal(fixture.emul->calls, 1U);
	for (size_t i = 0U; i < FRAME_SIZE; i++) {
		zassert_equal(fixture.emul->frame[i], 0U);
	}
	return &fixture;
}

static void before(void *arg)
{
	struct tlc6c59xx_fixture *fixture = arg;

	fixture->emul->next_error = 0;
	fixture->emul->delay = false;
	for (size_t i = 0U; i < ARRAY_SIZE(fixture->chips); i++) {
		zassert_ok(gpio_port_clear_bits_raw(fixture->chips[i], PORT_MASK));
	}
}

/* Decode the wire frame as an integer, independently of the driver's bit loop. */
static void expect_words(struct tlc6c59xx_fixture *fixture, uint16_t near, uint16_t middle,
			 uint16_t far)
{
	uint64_t frame = 0U;
	uint64_t expected = ((uint64_t)far << (2U * WIDTH)) |
			    ((uint64_t)middle << WIDTH) | near;

	for (size_t i = 0U; i < FRAME_SIZE; i++) {
		frame = (frame << 8U) | fixture->emul->frame[i];
	}
	zassert_equal(frame, expected, "incorrect chain order, state or leading padding");
}

ZTEST_F(tlc6c59xx, test_packing)
{
	zassert_ok(gpio_port_set_masked_raw(fixture->chips[0], PORT_MASK, 0x123U & PORT_MASK));
	zassert_ok(gpio_port_set_masked_raw(fixture->chips[1], PORT_MASK, 0x456U & PORT_MASK));
	zassert_ok(gpio_port_set_masked_raw(fixture->chips[2], PORT_MASK, 0x789U & PORT_MASK));
	expect_words(fixture, 0x123U & PORT_MASK, 0x456U & PORT_MASK, 0x789U & PORT_MASK);
}

ZTEST_F(tlc6c59xx, test_every_output)
{
	uint16_t words[3] = {0};

	for (size_t chip = 0U; chip < ARRAY_SIZE(fixture->chips); chip++) {
		for (size_t pin = 0U; pin < WIDTH; pin++) {
			words[chip] |= BIT(pin);
			zassert_ok(gpio_port_set_bits_raw(fixture->chips[chip], BIT(pin)));
			expect_words(fixture, words[0], words[1], words[2]);
		}
	}
	for (size_t chip = 0U; chip < ARRAY_SIZE(fixture->chips); chip++) {
		for (size_t pin = 0U; pin < WIDTH; pin++) {
			words[chip] &= ~BIT(pin);
			zassert_ok(gpio_port_clear_bits_raw(fixture->chips[chip], BIT(pin)));
			expect_words(fixture, words[0], words[1], words[2]);
		}
	}
}

ZTEST_F(tlc6c59xx, test_gpio_operations)
{
	const struct device *dev = fixture->chips[0];

	zassert_ok(gpio_pin_configure(dev, 0U, GPIO_OUTPUT_HIGH));
	zassert_ok(gpio_pin_configure(dev, 1U, GPIO_OUTPUT_HIGH));
	zassert_ok(gpio_pin_configure(dev, 0U, GPIO_OUTPUT_LOW));
	expect_words(fixture, 2U, 0U, 0U);
	zassert_ok(gpio_port_set_masked_raw(dev, 3U, 0xffU));
	expect_words(fixture, 3U, 0U, 0U);
	zassert_ok(gpio_port_toggle_bits(dev, 5U));
	expect_words(fixture, 6U, 0U, 0U);
	zassert_ok(gpio_port_clear_bits_raw(dev, 2U));
	zassert_ok(gpio_port_set_bits_raw(dev, BIT(WIDTH - 1U)));
	expect_words(fixture, BIT(WIDTH - 1U) | 4U, 0U, 0U);
	zassert_ok(gpio_pin_configure(dev, 2U, GPIO_OUTPUT));
	expect_words(fixture, BIT(WIDTH - 1U) | 4U, 0U, 0U);
}

ZTEST_F(tlc6c59xx, test_active_low)
{
	const struct device *dev = fixture->chips[1];

	zassert_ok(gpio_pin_configure(dev, 0U, GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW));
	expect_words(fixture, 0U, 0U, 0U);
	zassert_ok(gpio_pin_set(dev, 0U, 0));
	expect_words(fixture, 0U, 1U, 0U);
	zassert_ok(gpio_pin_set(dev, 0U, 1));
	expect_words(fixture, 0U, 0U, 0U);
	zassert_ok(gpio_pin_configure(dev, 0U, GPIO_OUTPUT_LOW));
}

ZTEST_F(tlc6c59xx, test_spi_failure_rollback)
{
	const struct device *dev = fixture->chips[0];

	for (size_t operation = 0U; operation < 6U; operation++) {
		int ret;

		zassert_ok(gpio_port_set_masked_raw(dev, PORT_MASK, 5U));
		fixture->emul->next_error = -EIO;
		switch (operation) {
		case 0:
			ret = gpio_port_set_masked_raw(dev, PORT_MASK, 2U);
			break;
		case 1:
			ret = gpio_port_set_bits_raw(dev, 2U);
			break;
		case 2:
			ret = gpio_port_clear_bits_raw(dev, 1U);
			break;
		case 3:
			ret = gpio_port_toggle_bits(dev, 3U);
			break;
		case 4:
			ret = gpio_pin_configure(dev, 0U, GPIO_OUTPUT_LOW);
			break;
		default:
			ret = gpio_pin_configure(dev, 1U, GPIO_OUTPUT_HIGH);
			break;
		}
		zassert_equal(ret, -EIO);
		/* Another child must not replay the failed parent-cache update. */
		zassert_ok(gpio_port_set_bits_raw(fixture->chips[1], 8U));
		expect_words(fixture, 5U, 8U, 0U);
		/* Updating this child must also use its last successful state. */
		zassert_ok(gpio_port_toggle_bits(dev, 1U));
		expect_words(fixture, 4U, 8U, 0U);
	}
}

ZTEST_F(tlc6c59xx, test_rejected_requests)
{
	const struct device *dev = fixture->chips[0];
	const struct gpio_driver_api *api = DEVICE_API_GET(gpio, dev);
	const gpio_flags_t unsupported[] = {
		GPIO_INPUT, GPIO_DISCONNECTED, GPIO_OUTPUT | GPIO_PULL_UP,
		GPIO_OUTPUT | GPIO_PULL_DOWN, GPIO_OUTPUT | GPIO_OPEN_DRAIN,
		GPIO_OUTPUT | GPIO_OPEN_SOURCE,
	};
	uint32_t calls = fixture->emul->calls;
	gpio_port_value_t value;

	for (size_t i = 0U; i < ARRAY_SIZE(unsupported); i++) {
		zassert_equal(gpio_pin_configure(dev, 0U, unsupported[i]), -ENOTSUP);
	}
	/* Bypass public pin assertions to exercise the driver's bounds checks. */
	zassert_equal(api->pin_configure(dev, WIDTH, GPIO_OUTPUT), -EINVAL);
	zassert_equal(gpio_port_set_bits_raw(dev, BIT(WIDTH)), -EINVAL);
	zassert_equal(gpio_port_clear_bits_raw(dev, BIT(WIDTH)), -EINVAL);
	zassert_equal(gpio_port_toggle_bits(dev, BIT(WIDTH)), -EINVAL);
	zassert_equal(gpio_port_set_masked_raw(dev, BIT(WIDTH), 0U), -EINVAL);
	zassert_equal(gpio_port_get_raw(dev, &value), -ENOTSUP);
	zassert_equal(gpio_pin_interrupt_configure(dev, 0U, GPIO_INT_EDGE_RISING), -ENOTSUP);
	zassert_equal(tlc6c59xx_write_state(fixture->parent, 3U, 0U), -EINVAL);
	zassert_equal(tlc6c59xx_write_state(fixture->parent, 0U, BIT(WIDTH)), -EINVAL);
	zassert_equal(fixture->emul->calls, calls);
}

static void isr_requests(const void *arg)
{
	const struct tlc6c59xx_fixture *fixture = arg;
	const struct device *dev = fixture->chips[0];

	zassert_equal(gpio_pin_configure(dev, 0U, GPIO_OUTPUT), -EWOULDBLOCK);
	zassert_equal(gpio_port_set_bits_raw(dev, 1U), -EWOULDBLOCK);
	zassert_equal(gpio_port_clear_bits_raw(dev, 1U), -EWOULDBLOCK);
	zassert_equal(gpio_port_set_masked_raw(dev, 1U, 1U), -EWOULDBLOCK);
	zassert_equal(gpio_port_toggle_bits(dev, 1U), -EWOULDBLOCK);
	zassert_equal(tlc6c59xx_write_state(fixture->parent, 0U, 1U), -EWOULDBLOCK);
}

ZTEST_F(tlc6c59xx, test_isr_rejection)
{
	uint32_t calls = fixture->emul->calls;

	irq_offload(isr_requests, fixture);
	zassert_equal(fixture->emul->calls, calls);
}

static void toggle_worker(void *dev, void *mask, void *unused)
{
	ARG_UNUSED(unused);
	for (size_t i = 0U; i < 101U; i++) {
		zassert_ok(gpio_port_toggle_bits(dev, (uintptr_t)mask));
	}
}

static void concurrent_updates(struct tlc6c59xx_fixture *fixture, bool separate_chips)
{
	static K_THREAD_STACK_ARRAY_DEFINE(stacks, 2, 2048);
	struct k_thread threads[2];

	fixture->emul->delay = true;
	for (size_t i = 0U; i < ARRAY_SIZE(threads); i++) {
		k_thread_create(&threads[i], stacks[i], K_THREAD_STACK_SIZEOF(stacks[i]),
				toggle_worker, (void *)fixture->chips[separate_chips ? i : 0U],
				(void *)BIT(i), NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	for (size_t i = 0U; i < ARRAY_SIZE(threads); i++) {
		zassert_ok(k_thread_join(&threads[i], K_SECONDS(5)));
	}
	fixture->emul->delay = false;
	expect_words(fixture, separate_chips ? 1U : 3U, separate_chips ? 2U : 0U, 0U);
}

ZTEST_F(tlc6c59xx, test_concurrent_same_chip)
{
	concurrent_updates(fixture, false);
}

ZTEST_F(tlc6c59xx, test_concurrent_different_chips)
{
	concurrent_updates(fixture, true);
}

ZTEST_SUITE(tlc6c59xx, NULL, setup, before, NULL, NULL);
