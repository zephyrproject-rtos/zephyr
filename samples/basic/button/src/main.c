/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <drivers/watchdog.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	1000

#define WDT_TOUT_MS 5000U
#define WDT_FEED_TOUT K_MSEC(WDT_TOUT_MS * 2 / 3)

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;
const struct device *wdt;
struct k_work_delayable wdtWorker;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}

static void onWdtTout(const struct device *dev, int channel_id) {
    printk("Watchdog timedout!");
}

static void onWdtWork(struct k_work *work) {
    struct k_work_delayable *dw = CONTAINER_OF(work, struct k_work_delayable, work);
    wdt_feed(wdt, 0);
    k_work_reschedule(dw, WDT_FEED_TOUT);
}

static void initWdt(void) {

    static const struct wdt_timeout_cfg cfg = {
        .window.min = 0U,
        .window.max = WDT_TOUT_MS,
        .callback = onWdtTout,
        .flags = WDT_FLAG_RESET_SOC,
    };

    wdt = device_get_binding(DT_LABEL(DT_ALIAS(watchdog)));
    if (!wdt) {
        printk("No wdt\n");
        return;
    }

    int rv = wdt_install_timeout(wdt, &cfg);
    if (rv < 0) {
        printk("Init %d\n", rv);
        return;
    }

    rv = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (rv) {
        printk("setup %d\n", rv);
        return;
    }
    printk("ok\n");
    k_work_init_delayable(&wdtWorker, onWdtWork);
    k_work_reschedule(&wdtWorker, WDT_FEED_TOUT);
}

void main(void)
{
	int ret;

	initWdt();

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	printk("Press the button\n");
	while (1) {
		k_msleep(SLEEP_TIME_MS);
		printk("alive\n");
	}
}
