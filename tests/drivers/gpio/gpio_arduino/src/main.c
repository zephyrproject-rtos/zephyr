/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys_clock.h>

#if CONFIG_GPIO_EMUL
void gpio_emul_loopback(const struct device *port_in, uint32_t pin_in,
			const struct device *port_out, uint32_t pin_out);
#endif


#define DEV_OUT DT_GPIO_CTLR(DT_INST(0, test_arduino_gpio), out_gpios)
#define DEV_IN DT_GPIO_CTLR(DT_INST(0, test_arduino_gpio), in_gpios)
#define PIN_OUT DT_GPIO_PIN(DT_INST(0, test_arduino_gpio), out_gpios)
#define PIN_OUT_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_arduino_gpio), out_gpios)
#define PIN_IN DT_GPIO_PIN(DT_INST(0, test_arduino_gpio), in_gpios)
#define PIN_IN_FLAGS DT_GPIO_FLAGS(DT_INST(0, test_arduino_gpio), in_gpios)

#define _STACK_SIZE (1024U)
#define INIT_RX_PRIO 1
#define INIT_TX_PRIO 2
#define INIT_RX_OPTION (K_USER | K_INHERIT_PERMS)
#define INIT_TX_OPTION (K_USER | K_INHERIT_PERMS)
#define INIT_TX_DELAY 10
#define INIT_RX_DELAY 0

#define BAUD_RATE 1200
#define DELAY_US_PER_BIT (USEC_PER_SEC/BAUD_RATE)
#define SAMPLE_PERIOD (DELAY_US_PER_BIT>>1u)
#define HOLD_TIME (10u)

enum gpio_test_phase {
	eIDLE = 0,
	eSTART,
	eDATA,
	ePARITY,
	eSTOP,
};

static struct gpio_callback cb_data;
static struct k_thread thread_tx;
static struct k_thread thread_rx;

static K_THREAD_STACK_DEFINE(stack_tx, _STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_rx, _STACK_SIZE);

volatile char tx_data = 0x55;
volatile char rx_data;
volatile char tx_status;
bool received;
bool send;

volatile enum gpio_test_phase status = eIDLE;

static void callback(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(gpio_cb);
	ARG_UNUSED(pins);

	/* process input here*/
	if (status == eIDLE) {
		status = eSTART;
		zassert_ok(gpio_pin_interrupt_configure(DEVICE_DT_GET(DEV_IN),
				PIN_IN, GPIO_INT_DISABLE));
	} else {
		printk("should not be here, as we disable interrupt in other status %d\n", status);
		status = eIDLE;
		zassert_ok(gpio_pin_interrupt_configure(DEVICE_DT_GET(DEV_IN),
				PIN_IN, GPIO_INT_DISABLE));
	}
}

static void *gpio_arduino_setup(void)
{
	int ret = 0;

	zassert_true(device_is_ready(DEVICE_DT_GET(DEV_OUT)));
	zassert_true(device_is_ready(DEVICE_DT_GET(DEV_IN)));
	ret = gpio_pin_configure(DEVICE_DT_GET(DEV_OUT), PIN_OUT,
				PIN_OUT_FLAGS | GPIO_OUTPUT);
	if (ret != 0) {
		printk("pin configure is %d, 0x%x\n", PIN_OUT,
			PIN_OUT_FLAGS | GPIO_INPUT);
		zassert_ok(ret, "pin configure error\n");
	}
	ret = gpio_pin_configure(DEVICE_DT_GET(DEV_IN), PIN_IN, PIN_IN_FLAGS | GPIO_INPUT);
	if (ret != 0) {
		printk("pin configure is %d, 0x%x\n", PIN_IN, PIN_IN_FLAGS | GPIO_INPUT);
		zassert_ok(ret, "pin configure error\n");
	}

	zassert_ok(gpio_pin_interrupt_configure(DEVICE_DT_GET(DEV_IN), PIN_IN, GPIO_INT_DISABLE));
	gpio_init_callback(&cb_data, callback, BIT(PIN_IN));
	zassert_ok(gpio_add_callback(DEVICE_DT_GET(DEV_IN), &cb_data), "failed to add callback");
	return 0;
}

static void gpio_arduino_after(void *arg)
{
	zassert_ok(gpio_remove_callback(DEVICE_DT_GET(DEV_IN), &cb_data),
		   "failed to remove callback");
}

/*entry routines*/
static void thread_tx_entry(void *p1, void *p2, void *p3)
{
	int i = 0;
	int timeout = DELAY_US_PER_BIT * 2;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* send data */
	for (i = 0; i < 10; i++) {
		if (i == 0) {
			/* start bit */
			gpio_pin_set(DEVICE_DT_GET(DEV_OUT), PIN_OUT, 0);
			k_usleep(DELAY_US_PER_BIT);
			/* enable RX interrupt */
			zassert_ok(gpio_pin_interrupt_configure(DEVICE_DT_GET(DEV_IN),
				PIN_IN, GPIO_INT_LEVEL_LOW),
				"failed to enable interrupt");
			printk("tx start\n");
			continue;
		}
		timeout = DELAY_US_PER_BIT * 2;
		while (!received) {
			timeout--;
			if (timeout < 1) {
				tx_status = 1;
				break;
			}
			k_usleep(SAMPLE_PERIOD);
		}
		received = false;
		if (i >= 1 && i <= 7) {
			printk("tx data\n");
			/* data bit big indian*/
			if ((tx_data>>(i - 1)) & 0x1) {
				gpio_pin_set(DEVICE_DT_GET(DEV_OUT), PIN_OUT, 1);
			} else {
				gpio_pin_set(DEVICE_DT_GET(DEV_OUT), PIN_OUT, 0);
			}
			k_usleep(HOLD_TIME);
			send = true;
			k_usleep(DELAY_US_PER_BIT);
			continue;
		}
		if (i == 8) {
			/* parity bit */
			printk("tx parity\n");
			gpio_pin_set(DEVICE_DT_GET(DEV_OUT), PIN_OUT, 0);
			k_usleep(HOLD_TIME);
			send = true;
			k_usleep(DELAY_US_PER_BIT);
			continue;
		}
		if (i == 9) {
			printk("stop bit\n");
			/* stop bit */
			gpio_pin_set(DEVICE_DT_GET(DEV_OUT), PIN_OUT, 1);
			k_usleep(HOLD_TIME);
			send = true;
			k_usleep(DELAY_US_PER_BIT);
			continue;
		}
	}
}

static void thread_rx_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	char in_data = 0;
	int data_count = 0;
	int timeout = DELAY_US_PER_BIT * 2;

	while (true) {
		if (status == eIDLE) {
			k_usleep(SAMPLE_PERIOD);
			continue;
		}
		if (status == eSTART) {
			bool in_bit = 0;

			in_data = 0;
			k_usleep(SAMPLE_PERIOD);
			in_bit = gpio_pin_get(DEVICE_DT_GET(DEV_IN), PIN_IN);
			if (in_bit == 0) {
				status = eDATA;
				printk("rx start\n");
				received = true;
				k_usleep(DELAY_US_PER_BIT);
			} else {
				/* noise */
				status = eIDLE;
				k_usleep(SAMPLE_PERIOD);
			}
			continue;
		}
		timeout = DELAY_US_PER_BIT * 2;
		while (!send) {
			timeout--;
			if (timeout < 1) {
				break;
			}
			k_usleep(SAMPLE_PERIOD);
		}
		send = false;
		if (status == eDATA) {
			int in_bit = 0;

			printk("rx data\n");
			in_bit = gpio_pin_get(DEVICE_DT_GET(DEV_IN), PIN_IN);
			in_data = (in_data << 1) | in_bit;
			data_count++;
			received = true;
			k_usleep(DELAY_US_PER_BIT);
			if (data_count == 7) {
				status = ePARITY;
			}
			continue;
		}
		if (status == ePARITY) {
			/* parity bit not care */
			status = eSTOP;
			received = true;
			k_usleep(DELAY_US_PER_BIT);
		}
		if (status == eSTOP) {
			int in_bit = 0;

			k_usleep(DELAY_US_PER_BIT);
			in_bit = gpio_pin_get(DEVICE_DT_GET(DEV_IN), PIN_IN);
			while (in_bit == 0) {
				k_usleep(DELAY_US_PER_BIT);
			}
			received = true;
			/* get stop bit */
			rx_data = in_data;
			tx_status = 1;
			return;
		}
	}
}


ZTEST(gpio_arduino, test_arduino_gpio_transfer)
{
	k_tid_t pthread_rx = k_thread_create(&thread_rx, stack_rx,
				  _STACK_SIZE, thread_rx_entry, NULL,
				  NULL, NULL, INIT_RX_PRIO,
				  INIT_RX_OPTION,
				  K_MSEC(INIT_RX_DELAY));

	k_tid_t pthread_tx = k_thread_create(&thread_tx, stack_tx,
				  _STACK_SIZE, thread_tx_entry, NULL,
				  NULL, NULL, INIT_TX_PRIO,
				  INIT_TX_OPTION,
				  K_MSEC(INIT_TX_DELAY));

	zassert_not_null(pthread_rx, "thread_rx creation failed");
	zassert_not_null(pthread_tx, "thread_tx creation failed");

	while (tx_status == 0) {
#if CONFIG_GPIO_EMUL
		gpio_emul_loopback(DEVICE_DT_GET(DEV_IN), PIN_IN,
				DEVICE_DT_GET(DEV_OUT), PIN_OUT);
#endif
		k_usleep((SAMPLE_PERIOD >> 1));
	}
	printk("rx = %x\n", rx_data);
	printk("tx = %x\n", tx_data);
	zassert_equal(rx_data, tx_data);
}

ZTEST_SUITE(gpio_arduino, NULL, gpio_arduino_setup, NULL, gpio_arduino_after, NULL);
