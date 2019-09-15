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

/* resource IDs */
#define BUTTON_DIGITAL_STATE_ID		5500
#define BUTTON_DIGITAL_INPUT_COUNTER_ID	5501
#define BUTTON_APPLICATION_TYPE_ID	5750

#define BUTTON_MAX_ID			3

#define MAX_INSTANCE_COUNT	CONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT

/*
 * Calculate resource instances as follows:
 * start with BUTTON_MAX_ID
 */
#define RESOURCE_INSTANCE_COUNT        (BUTTON_MAX_ID)

struct sw {
	struct gpio_callback button_cb;
	struct k_timer button_timer;
	struct k_work button_work;
	struct device *gpio;
	u8_t sw_num;
	u8_t pin;
};

static struct sw sw;
static u32_t time, last_time;

/* resource state */
struct ipso_button_data {
	u64_t counter;
	u16_t obj_inst_id;
	bool last_state;
	bool state;
};

static struct ipso_button_data button_data[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj onoff_switch;
static struct lwm2m_engine_obj_field fields[] = {
	OBJ_FIELD_DATA(BUTTON_DIGITAL_STATE_ID, R, BOOL),
	OBJ_FIELD_DATA(BUTTON_DIGITAL_INPUT_COUNTER_ID, R_OPT, U64),
	OBJ_FIELD_DATA(BUTTON_APPLICATION_TYPE_ID, RW_OPT, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][BUTTON_MAX_ID];
static struct lwm2m_engine_res_inst
			res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static int get_button_index(u16_t obj_inst_id)
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

static int state_post_write_cb(u16_t obj_inst_id,
			       u16_t res_id, u16_t res_inst_id,
			       u8_t *data, u16_t data_len,
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

static struct lwm2m_engine_obj_inst *button_create(u16_t obj_inst_id)
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

	inst[avail].resources = res[avail];
	inst[avail].resource_count = i;

	LOG_DBG("Create IPSO Button instance: %d", obj_inst_id);

	return &inst[avail];
}

#define BUTTON_DEBOUNCE_DELAY_MS 250

static uint8_t pin_pos_to_pin(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(DT_ALIAS_SW0_GPIOS_PIN):
		return DT_ALIAS_SW0_GPIOS_PIN;
	case BIT(DT_ALIAS_SW1_GPIOS_PIN):
		return DT_ALIAS_SW1_GPIOS_PIN;
	case BIT(DT_ALIAS_SW2_GPIOS_PIN):
		return DT_ALIAS_SW2_GPIOS_PIN;
	case BIT(DT_ALIAS_SW3_GPIOS_PIN):
		return DT_ALIAS_SW3_GPIOS_PIN;
	}

	LOG_ERR("No match for GPIO pin 0x%08x\n", pin_pos);

	return 0;
}

static uint8_t pin_pos_to_sw_num(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(DT_ALIAS_SW0_GPIOS_PIN): return 0;
	case BIT(DT_ALIAS_SW1_GPIOS_PIN): return 1;
	case BIT(DT_ALIAS_SW2_GPIOS_PIN): return 2;
	case BIT(DT_ALIAS_SW3_GPIOS_PIN): return 3;
	}

	LOG_ERR("No match for GPIO pin 0x%08x\n", pin_pos);

	return 0;
}

static void button_off_timer(struct k_timer *work)
{
	struct sw *sw = CONTAINER_OF(work, struct sw, button_timer);
	char pathstr[15];
	u32_t val = 0U;

	gpio_pin_read(sw->gpio, sw->pin, &val);
	LOG_DBG("%s: switch=%d, val=%d", __func__, sw->sw_num, val);

	if (val == 0) {
		k_timer_start(&sw->button_timer, K_SECONDS(1), 0);
		return;
	}

	snprintk(pathstr, sizeof(pathstr), "/%u/%u/%u",
			IPSO_OBJECT_PUSH_BUTTON_ID, sw->sw_num,
			BUTTON_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(pathstr, !val);
}

static void button_pressed_worker(struct k_work *work)
{
	struct sw *sw = CONTAINER_OF(work, struct sw, button_work);
	char pathstr[15];
	u32_t val = 0U;

	LOG_DBG("%s: switch=%d", __func__, sw->sw_num);

	gpio_pin_read(sw->gpio, sw->pin, &val);

	snprintk(pathstr, sizeof(pathstr), "/%u/%u/%u",
			IPSO_OBJECT_PUSH_BUTTON_ID, sw->sw_num,
			BUTTON_DIGITAL_STATE_ID);
	lwm2m_engine_set_bool(pathstr, !val);

	k_timer_stop(&sw->button_timer);
	k_timer_start(&sw->button_timer, K_MSEC(500), 0);
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
		u32_t pin_pos)
{
	struct sw *sw = CONTAINER_OF(cb, struct sw, button_cb);
	int i;

	time = k_uptime_get_32();

	/* debounce the switch */
	if (time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		last_time = time;
		return;
	}

	LOG_DBG("%s: pin_pos=0x%08X", __func__, pin_pos);

	/* If any button is already pressed, just return */
	for (i = 0; i < ARRAY_SIZE(inst); i++) {
		if (button_data[i].state) {
			LOG_ERR("%s: button %d is still pressed", __func__, i);
			return;
		}
	}

	sw->pin = pin_pos_to_pin(pin_pos);
	sw->sw_num = pin_pos_to_sw_num(pin_pos);

	k_work_submit(&sw->button_work);

	last_time = time;
}

static void configure_buttons(void)
{
	/* Initialize the button debouncer */
	last_time = k_uptime_get_32();

	/* Initialize button worker task */
	k_work_init(&sw.button_work, button_pressed_worker);

	/* Initialize button off timer */
	k_timer_init(&sw.button_timer, button_off_timer, NULL);

	sw.gpio = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	if (!sw.gpio) {
		LOG_ERR("Can't get gpio device");
		return;
	}

	gpio_pin_configure(sw.gpio, DT_ALIAS_SW0_GPIOS_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			 GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
#ifdef DT_ALIAS_SW1_GPIOS_PIN
	gpio_pin_configure(sw.gpio, DT_ALIAS_SW1_GPIOS_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			 GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
#endif
#ifdef DT_ALIAS_SW2_GPIOS_PIN
	gpio_pin_configure(sw.gpio, DT_ALIAS_SW2_GPIOS_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			 GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
#endif
#ifdef DT_ALIAS_SW3_GPIOS_PIN
	gpio_pin_configure(sw.gpio, DT_ALIAS_SW3_GPIOS_PIN,
			(GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			 GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
#endif

	gpio_init_callback(&sw.button_cb, button_pressed,
			BIT(DT_ALIAS_SW0_GPIOS_PIN)
#ifdef DT_ALIAS_SW1_GPIOS_PIN
			| BIT(DT_ALIAS_SW1_GPIOS_PIN)
#endif
#ifdef DT_ALIAS_SW2_GPIOS_PIN
			| BIT(DT_ALIAS_SW2_GPIOS_PIN)
#endif
#ifdef DT_ALIAS_SW3_GPIOS_PIN
			| BIT(DT_ALIAS_SW3_GPIOS_PIN)
#endif
			);

	gpio_add_callback(sw.gpio, &sw.button_cb);

	gpio_pin_enable_callback(sw.gpio, DT_ALIAS_SW0_GPIOS_PIN);
#ifdef DT_ALIAS_SW1_GPIOS_PIN
	gpio_pin_enable_callback(sw.gpio, DT_ALIAS_SW1_GPIOS_PIN);
#endif
#ifdef DT_ALIAS_SW2_GPIOS_PIN
	gpio_pin_enable_callback(sw.gpio, DT_ALIAS_SW2_GPIOS_PIN);
#endif
#ifdef DT_ALIAS_SW3_GPIOS_PIN
	gpio_pin_enable_callback(sw.gpio, DT_ALIAS_SW3_GPIOS_PIN);
#endif
}

static int ipso_button_init(struct device *dev)
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
