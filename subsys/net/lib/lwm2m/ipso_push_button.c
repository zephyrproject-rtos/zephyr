/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Source material for IPSO On/Off Switch object (3347):
 * http://www.openmobilealliance.org/tech/profiles/lwm2m/3347.xml
 */

#define LOG_MODULE_NAME net_ipso_button
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <init.h>

#include <drivers/gpio.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"

/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */
#define SW0_NODE        DT_ALIAS(sw0)
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL  DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN    DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS  (GPIO_INPUT | GPIO_INT_EDGE_FALLING | GPIO_PULL_UP)
#else
#define SW0_GPIO_LABEL  ""
#define SW0_GPIO_PIN    0
#define SW0_GPIO_FLAGS  0
#endif

#define SW1_NODE        DT_ALIAS(sw1)
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
#define SW1_GPIO_LABEL  DT_GPIO_LABEL(SW1_NODE, gpios)
#define SW1_GPIO_PIN    DT_GPIO_PIN(SW1_NODE, gpios)
#define SW1_GPIO_FLAGS  (GPIO_INPUT | GPIO_INT_EDGE_FALLING | GPIO_PULL_UP)
#else
#define SW1_GPIO_LABEL  ""
#define SW1_GPIO_PIN    0
#define SW1_GPIO_FLAGS  0
#endif

#define SW2_NODE        DT_ALIAS(sw2)
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
#define SW2_GPIO_LABEL  DT_GPIO_LABEL(SW2_NODE, gpios)
#define SW2_GPIO_PIN    DT_GPIO_PIN(SW2_NODE, gpios)
#define SW2_GPIO_FLAGS  (GPIO_INPUT | GPIO_INT_EDGE_FALLING | GPIO_PULL_UP)
#else
#define SW2_GPIO_LABEL  ""
#define SW2_GPIO_PIN    0
#define SW2_GPIO_FLAGS  0
#endif

#define SW3_NODE        DT_ALIAS(sw3)
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
#define SW3_GPIO_LABEL  DT_GPIO_LABEL(SW3_NODE, gpios)
#define SW3_GPIO_PIN    DT_GPIO_PIN(SW3_NODE, gpios)
#define SW3_GPIO_FLAGS  (GPIO_INPUT | GPIO_INT_EDGE_FALLING | GPIO_PULL_UP)
#else
#define SW3_GPIO_LABEL  ""
#define SW3_GPIO_PIN    0
#define SW3_GPIO_FLAGS  0
#endif

#ifdef CONFIG_LWM2M_IPSO_PUSH_BUTTON_TIMESTAMP
#define ADD_TIMESTAMPS 1
#else
#define ADD_TIMESTAMPS 0
#endif

/* resource IDs */
#define BUTTON_DIGITAL_STATE_ID		5500
#define BUTTON_DIGITAL_INPUT_COUNTER_ID	5501
#define BUTTON_APPLICATION_TYPE_ID	5750
#if ADD_TIMESTAMPS
#define BUTTON_TIMESTAMP_ID		5518

#define BUTTON_MAX_ID			4
#else
#define BUTTON_MAX_ID			3
#endif

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUTTON_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUTTON_MAX_ID)

struct sw_inst {
	uint8_t pin;
	bool pressed;
	uint32_t last_time;
};

struct sw {
	struct gpio_callback button_cb;
	struct k_timer button_timer;
	struct k_work button_work;
	const struct device *gpio;
	struct sw_inst swi[MAX_INSTANCE_COUNT];
};

static struct sw sw;
static uint32_t time, last_time;

/* resource state */
struct ipso_button_data {
	uint64_t counter;
	uint16_t obj_inst_id;
	bool last_state;
	bool state;
};

static struct ipso_button_data button_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj onoff_switch;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(BUTTON_DIGITAL_STATE_ID, R, BOOL),
	OBJ_FIELD_DATA(BUTTON_DIGITAL_INPUT_COUNTER_ID, R_OPT, U64),
	OBJ_FIELD_DATA(BUTTON_APPLICATION_TYPE_ID, RW_OPT, STRING),
#if ADD_TIMESTAMPS
	OBJ_FIELD_DATA(BUTTON_TIMESTAMP_ID, RW_OPT, TIME),
#endif
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUTTON_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int get_button_index(uint16_t obj_inst_id)
{
	int i, ret = -ENOENT;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (!inst[i].obj || inst[i].obj_inst_id != obj_inst_id) {
			continue;
		}

		ret = i;
		break;
	}

	return ret;
}

static int state_post_write_cb(uint16_t obj_inst_id,
			       uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len,
			       bool last_block, size_t total_size)
{
	int i;

	i = get_button_index(obj_inst_id);
	if (i < 0) {
		return i;
	}

	if (button_data[i].state && !button_data[i].last_state) {
		/* off to on transition */
		button_data[i].counter++;
	}

	button_data[i].last_state = button_data[i].state;
	return 0;
}

static struct lwm2m_engine_obj_inst *button_create(uint16_t obj_inst_id)
{
	int index, avail = -1, i = 0, j = 0;

	/* Check that there is no other instance with this ID */
	for (index = 0; index < ARRAY_SIZE(inst); index++) {
		if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
			LOG_ERR("Can not create instance - "
				"already existing: %u", obj_inst_id);
			return NULL;
		}

		/* Save first available slot index */
		if (avail < 0 && !inst[index].obj) {
			avail = index;
		}
	}

	if (avail < 0) {
		LOG_ERR("Can not create instance - no more room: %u",
			obj_inst_id);
		return NULL;
	}

	/* Set default values */
	(void)memset(&button_data[avail], 0, sizeof(button_data[avail]));
	button_data[avail].obj_inst_id = obj_inst_id;

	(void)memset(res[avail], 0,
		     sizeof(res[avail][0]) * ARRAY_SIZE(res[avail]));
	init_res_instance(res_inst[avail], ARRAY_SIZE(res_inst[avail]));

	/* initialize instance resource data */
	INIT_OBJ_RES(BUTTON_DIGITAL_STATE_ID, res[avail], i,
		     res_inst[avail], j, 1, true,
		     &button_data[avail].state,
		     sizeof(button_data[avail].state),
		     NULL, NULL, state_post_write_cb, NULL);
	INIT_OBJ_RES_DATA(BUTTON_DIGITAL_INPUT_COUNTER_ID, res[avail], i,
			  res_inst[avail], j,
			  &button_data[avail].counter,
			  sizeof(button_data[avail].counter));
	INIT_OBJ_RES_OPTDATA(BUTTON_APPLICATION_TYPE_ID, res[avail], i,
			     res_inst[avail], j);
#if ADD_TIMESTAMPS
	INIT_OBJ_RES_OPTDATA(BUTTON_TIMESTAMP_ID, res[avail], i,
			     res_inst[avail], j);
#endif

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Button instance: %d", obj_inst_id);

	return &inst[avail];
}

#define BUTTON_DEBOUNCE_DELAY_MS 250

static uint8_t pin_pos_to_pin(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(SW0_GPIO_PIN):
		return SW0_GPIO_PIN;
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	case BIT(SW1_GPIO_PIN):
		return SW1_GPIO_PIN;
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	case BIT(SW2_GPIO_PIN):
		return SW2_GPIO_PIN;
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	case BIT(SW3_GPIO_PIN):
		return SW3_GPIO_PIN;
#endif
	}

	LOG_ERR("No match for GPIO pin 0x%08x\n", pin_pos);

	return -EINVAL;
}

static int8_t pin_pos_to_sw_num(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(SW0_GPIO_PIN): return 0;
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	case BIT(SW1_GPIO_PIN): return 1;
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	case BIT(SW2_GPIO_PIN): return 2;
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	case BIT(SW3_GPIO_PIN): return 3;
#endif
	}

	LOG_ERR("No match for GPIO pin 0x%08x\n", pin_pos);

	return -EINVAL;
}

static void button_off_timer(struct k_timer *work)
{
	struct sw *sw = CONTAINER_OF(work, struct sw, button_timer);
	char pathstr[15];
	uint32_t val = 0U;
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (!sw->swi[i].pressed)
			continue;

		val = gpio_pin_get(sw->gpio, sw->swi[i].pin);
		if (val < 0) {
			LOG_ERR("get_pin_value, %d", val);
			k_timer_stop(&sw->button_timer);
			continue;
		}
		LOG_DBG("switch: %d, val: %d", i, val);

		/* check if button is stil pressed */
		if (val == 0) {
			/* restart the timer to sample pin again */
			k_timer_start(&sw->button_timer, K_SECONDS(1),
					K_NO_WAIT);
			continue;
		}

		sw->swi[i].pressed = false;

		LOG_DBG("switch: %d, pressed: false", i);
		snprintk(pathstr, sizeof(pathstr), "/%u/%u/%u",
				IPSO_OBJECT_PUSH_BUTTON_ID, i,
				BUTTON_DIGITAL_STATE_ID);
		lwm2m_engine_set_bool(pathstr, !val);
	}
}

static void button_pressed_worker(struct k_work *work)
{
	struct sw *sw = CONTAINER_OF(work, struct sw, button_work);
	char pathstr[15];
	uint32_t val = 0U;
	int i;

	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (!sw->swi[i].pressed)
			continue;

		val = gpio_pin_get(sw->gpio, sw->swi[i].pin);
		if (val < 0) {
			LOG_ERR("get_pin_value, %d", val);
			k_timer_stop(&sw->button_timer);
			sw->swi[i].pressed = false;
			continue;
		}

		LOG_DBG("switch: %d, val: %d, pressed: true", i, val);
		snprintk(pathstr, sizeof(pathstr), "/%u/%u/%u",
				IPSO_OBJECT_PUSH_BUTTON_ID, i,
				BUTTON_DIGITAL_STATE_ID);
		lwm2m_engine_set_bool(pathstr, !val);

		k_timer_stop(&sw->button_timer);
		k_timer_start(&sw->button_timer, K_MSEC(500), K_NO_WAIT);
	}
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
		uint32_t pin_pos)
{
	struct sw *sw = CONTAINER_OF(cb, struct sw, button_cb);
	int sw_num = pin_pos_to_sw_num(pin_pos);
	int res;

	LOG_DBG("pin_pos: 0x%08X, sw_num: %d", pin_pos, sw_num);

	if (sw_num < 0) {
		return;
	}

	if (sw_num >= ARRAY_SIZE(inst)) {
		LOG_INF("No available instaces for sw_num %d", sw_num);
		return;
	}

	time = k_uptime_get_32();

	res = pin_pos_to_pin(pin_pos);
	if (res < 0) {
		return;
	}
	sw->swi[sw_num].pin = (uint8_t)res;
	sw->swi[sw_num].pressed = true;

	/* debounce the switch */
	if (time < sw->swi[sw_num].last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		sw->swi[sw_num].last_time = time;
		LOG_DBG("switch: %d, debounce", sw_num);
		return;
	}

	k_work_submit(&sw->button_work);
	sw->swi[sw_num].last_time = time;
}

static void configure_buttons(void)
{
	int i;

	sw.gpio = device_get_binding(SW0_GPIO_LABEL);
	if (!sw.gpio) {
		LOG_ERR("Can't get gpio device");
		return;
	}

	gpio_pin_configure(sw.gpio, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	gpio_pin_configure(sw.gpio, SW1_GPIO_PIN, SW1_GPIO_FLAGS);
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	gpio_pin_configure(sw.gpio, SW2_GPIO_PIN, SW2_GPIO_FLAGS);
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	gpio_pin_configure(sw.gpio, SW3_GPIO_PIN, SW3_GPIO_FLAGS);
#endif

	gpio_init_callback(&sw.button_cb, button_pressed,
			BIT(SW0_GPIO_PIN)
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
			| BIT(SW1_GPIO_PIN)
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
			| BIT(SW2_GPIO_PIN)
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
			| BIT(SW3_GPIO_PIN)
#endif
			);

	gpio_add_callback(sw.gpio, &sw.button_cb);

	gpio_pin_interrupt_configure(sw.gpio, SW0_GPIO_PIN,
			GPIO_INT_EDGE_FALLING);
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	gpio_pin_interrupt_configure(sw.gpio, SW1_GPIO_PIN,
			GPIO_INT_EDGE_FALLING);
#endif
#if DT_NODE_HAS_STATUS(SW2_NODE, okay)
	gpio_pin_interrupt_configure(sw.gpio, SW2_GPIO_PIN,
			GPIO_INT_EDGE_FALLING);
#endif
#if DT_NODE_HAS_STATUS(SW3_NODE, okay)
	gpio_pin_interrupt_configure(sw.gpio, SW3_GPIO_PIN,
			GPIO_INT_EDGE_FALLING);
#endif

	/* Initialize the button debouncer */
	last_time = k_uptime_get_32();

	/* Initialize button worker task */
	k_work_init(&sw.button_work, button_pressed_worker);

	/* Initialize button off timer */
	k_timer_init(&sw.button_timer, button_off_timer, NULL);

	/* init button state to non-pressed */
	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		sw.swi[i].pressed = false;
	}
}

static int ipso_button_init(const struct device *dev)
{
	onoff_switch.obj_id = IPSO_OBJECT_PUSH_BUTTON_ID;
	onoff_switch.fields = fields;
	onoff_switch.field_count = ARRAY_SIZE(fields);
	onoff_switch.max_instance_count = ARRAY_SIZE(inst);
	onoff_switch.create_cb = button_create;
	lwm2m_register_obj(&onoff_switch);

	configure_buttons();

	return 0;
}

SYS_INIT(ipso_button_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
