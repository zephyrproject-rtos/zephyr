/*
 * Copyright (c) 2026 Zephyr SNMP Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_button.h"
#include "app_led.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/snmp/snmp.h>
#include <zephyr/net/snmp/snmp_trap.h>

LOG_MODULE_REGISTER(app_button, LOG_LEVEL_INF);

#ifndef CONFIG_SNMP_SAMPLE_ENTERPRISE_ID
#define CONFIG_SNMP_SAMPLE_ENTERPRISE_ID 54321
#endif

static uint32_t s_button_counter;

/* Agent to notify through; set by app_button_init() from main.c's storage. */
static struct snmp_agent *s_trap_agent;

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
static const struct gpio_dt_spec s_btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback s_btn_cb_data;
static bool s_hardware_available;
static int64_t s_last_press_time;

static void button_work_handler(struct k_work *work);
static K_WORK_DEFINE(s_button_work, button_work_handler);

static void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();
	if (now - s_last_press_time > 200) {

		s_last_press_time = now;
		k_work_submit(&s_button_work);
	}
}

static void button_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	s_button_counter++;
	LOG_INF("==================================================");
	LOG_INF(">>> User Button Pressed! Total Count: %u <<<", s_button_counter);
	LOG_INF("==================================================");

#if defined(CONFIG_SNMP_TRAP_ENABLED)
	app_button_trigger_notification();
#else
	LOG_INF("SNMP Traps disabled in Kconfig (CONFIG_SNMP_TRAP_ENABLED=n)");
#endif
}
#endif

int app_button_init(struct snmp_agent *agent)
{
#if !defined(CONFIG_SNMP_TRAP_ENABLED)
	ARG_UNUSED(agent);
#endif
	s_trap_agent = agent;

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	if (!gpio_is_ready_dt(&s_btn)) {
		LOG_WRN("User Button device %s not ready", s_btn.port->name);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&s_btn, GPIO_INPUT);
	if (ret < 0) {

		LOG_ERR("Failed to configure button pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&s_btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure button interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&s_btn_cb_data, button_isr, BIT(s_btn.pin));
	ret = gpio_add_callback(s_btn.port, &s_btn_cb_data);
	if (ret < 0) {
		LOG_ERR("Failed to add button callback: %d", ret);
		return ret;
	}

	s_hardware_available = true;
	LOG_INF("User Button initialized on %s pin %d (Interrupt active edge)", s_btn.port->name,
		s_btn.pin);
	return 0;
#else
	LOG_WRN("No 'sw0' alias found in DeviceTree; manual triggering available");
	return 0;
#endif
}

uint32_t app_button_get_counter(void)
{
	return s_button_counter;
}

int app_button_trigger_notification(void)
{
#if defined(CONFIG_SNMP_TRAP_ENABLED)
	/* Notification OID: 1.3.6.1.4.1.<enterprise>.1.2.1 (buttonPressed) */
	struct snmp_oid trap_oid = {
		.len = 10, .ids = {1, 3, 6, 1, 4, 1, CONFIG_SNMP_SAMPLE_ENTERPRISE_ID, 1, 2, 1}};

	struct snmp_varbind varbinds[2];

	/* Varbind 1: buttonCounter (1.3.6.1.4.1.<enterprise>.1.1.3.0) */
	varbinds[0].oid.len = 10;
	varbinds[0].oid.ids[0] = 1;
	varbinds[0].oid.ids[1] = 3;
	varbinds[0].oid.ids[2] = 6;
	varbinds[0].oid.ids[3] = 1;
	varbinds[0].oid.ids[4] = 4;
	varbinds[0].oid.ids[5] = 1;
	varbinds[0].oid.ids[6] = CONFIG_SNMP_SAMPLE_ENTERPRISE_ID;
	varbinds[0].oid.ids[7] = 1;
	varbinds[0].oid.ids[8] = 1;
	varbinds[0].oid.ids[9] = 3;
	varbinds[0].oid.ids[10] = 0;
	varbinds[0].oid.len = 11;
	varbinds[0].type = SNMP_TAG_COUNTER32;
	varbinds[0].val.uint_val = s_button_counter;

	/* Varbind 2: ledState (1.3.6.1.4.1.<enterprise>.1.1.2.0) */
	varbinds[1].oid.len = 10;
	varbinds[1].oid.ids[0] = 1;
	varbinds[1].oid.ids[1] = 3;
	varbinds[1].oid.ids[2] = 6;
	varbinds[1].oid.ids[3] = 1;
	varbinds[1].oid.ids[4] = 4;
	varbinds[1].oid.ids[5] = 1;
	varbinds[1].oid.ids[6] = CONFIG_SNMP_SAMPLE_ENTERPRISE_ID;
	varbinds[1].oid.ids[7] = 1;
	varbinds[1].oid.ids[8] = 1;
	varbinds[1].oid.ids[9] = 2;
	varbinds[1].oid.ids[10] = 0;
	varbinds[1].oid.len = 11;
	varbinds[1].type = ASN1_TAG_INTEGER;
	varbinds[1].val.int_val = app_led_get_state();

	int ret = snmp_trap_send(s_trap_agent, &trap_oid, varbinds, 2, false);
	if (ret < 0) {

		LOG_WRN("Failed to send button SNMP notification: %d", ret);
		return ret;
	}

	LOG_INF("SNMP Notification dispatched successfully (buttonCounter=%u, ledState=%d)",
		s_button_counter, varbinds[1].val.int_val);
	return 0;
#else
	LOG_WRN("SNMP Trap engine is disabled");
	return -ENOTSUP;
#endif
}

int app_button_simulate_press(void)
{
	s_button_counter++;
	LOG_INF(">>> Simulated Button Press! Total Count: %u <<<", s_button_counter);
	return app_button_trigger_notification();
}
