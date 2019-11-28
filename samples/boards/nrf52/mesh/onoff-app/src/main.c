/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This application is specific to the Nordic nRF52840-PDK board.
 *
 * It supports the 4 buttons and 4 LEDs as mesh clients and servers.
 *
 * Prior to provisioning, a button inverts the state of the
 * corresponding LED.
 *
 * The unprovisioned beacon uses the device address set by Nordic
 * in the FICR as its UUID and is presumed unique.
 *
 * Button and LED 1 are in the root node.
 * The 3 remaining button/LED pairs are in element 1 through 3.
 * Assuming the provisioner assigns 0x100 to the root node,
 * the secondary elements will appear at 0x101, 0x102 and 0x103.
 *
 * It's anticipated that after provisioning, the button clients would
 * be configured to publish and the LED servers to subscribe.
 *
 * If a LED server is provided with a publish address, it will
 * also publish its status on a state change.
 *
 * Messages from a button to its corresponding LED are ignored as
 * the LED's state has already been changed locally by the button client.
 *
 * The buttons are debounced at a nominal 250ms. That value can be
 * changed as needed.
 *
 */

#include <sys/printk.h>
#include <settings/settings.h>
#include <sys/byteorder.h>
#include <device.h>
#include <drivers/gpio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/mesh.h>
#include <stdio.h>

/* Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET		BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET		BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS	BT_MESH_MODEL_OP_2(0x82, 0x04)

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);

static void gen_onoff_set_unack(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);

static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);

static void gen_onoff_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);

/*
 * Server Configuration Declaration
 */

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_ENABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

/*
 * Client Configuration Declaration
 */

static struct bt_mesh_cfg_cli cfg_cli = {
};

/*
 * Health Server Declaration
 */

static struct bt_mesh_health_srv health_srv = {
};

/*
 * Publication Declarations
 *
 * The publication messages are initialized to the
 * the size of the opcode + content
 *
 * For publication, the message must be in static or global as
 * it is re-transmitted several times. This occurs
 * after the function that called bt_mesh_model_publish() has
 * exited and the stack is no longer valid.
 *
 * Note that the additional 4 bytes for the AppMIC is not needed
 * because it is added to a stack variable at the time a
 * transmission occurs.
 *
 */

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv_s_0, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli_s_0, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv_s_1, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli_s_1, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv_s_2, NULL, 2 + 2);
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli_s_2, NULL, 2 + 2);

/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */

/*
 * OnOff Model Server Op Dispatch Table
 *
 */

static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

/*
 * OnOff Model Client Op Dispatch Table
 */

static const struct bt_mesh_model_op gen_onoff_cli_op[] = {
	{ BT_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, gen_onoff_status },
	BT_MESH_MODEL_OP_END,
};

struct onoff_state {
	u8_t current;
	u8_t previous;
	u8_t led_gpio_pin;
	struct device *led_device;
};

/*
 * Declare and Initialize Element Contexts
 * Change to select different GPIO output pins
 */

static struct onoff_state onoff_state[] = {
	{ .led_gpio_pin = DT_ALIAS_LED0_GPIOS_PIN },
	{ .led_gpio_pin = DT_ALIAS_LED1_GPIOS_PIN },
	{ .led_gpio_pin = DT_ALIAS_LED2_GPIOS_PIN },
	{ .led_gpio_pin = DT_ALIAS_LED3_GPIOS_PIN },
};

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op,
		      &gen_onoff_pub_srv, &onoff_state[0]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op,
		      &gen_onoff_pub_cli, &onoff_state[0]),
};

/*
 * Element 1 Models
 */

static struct bt_mesh_model secondary_0_models[] = {
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op,
		      &gen_onoff_pub_srv_s_0, &onoff_state[1]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op,
		      &gen_onoff_pub_cli_s_0, &onoff_state[1]),
};

/*
 * Element 2 Models
 */

static struct bt_mesh_model secondary_1_models[] = {
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op,
		      &gen_onoff_pub_srv_s_1, &onoff_state[2]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op,
		      &gen_onoff_pub_cli_s_1, &onoff_state[2]),
};

/*
 * Element 3 Models
 */

static struct bt_mesh_model secondary_2_models[] = {
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op,
		      &gen_onoff_pub_srv_s_2, &onoff_state[3]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, gen_onoff_cli_op,
		      &gen_onoff_pub_cli_s_2, &onoff_state[3]),
};

/*
 * Button to Client Model Assignments
 */

struct bt_mesh_model *mod_cli_sw[] = {
		&root_models[4],
		&secondary_0_models[1],
		&secondary_1_models[1],
		&secondary_2_models[1],
};

/*
 * LED to Server Model Assigmnents
 */

struct bt_mesh_model *mod_srv_sw[] = {
		&root_models[3],
		&secondary_0_models[0],
		&secondary_1_models[0],
		&secondary_2_models[0],
};

/*
 * Root and Secondary Element Declarations
 */

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(0, secondary_0_models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(0, secondary_1_models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(0, secondary_2_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

struct device *sw_device;

struct sw {
	u8_t sw_num;
	u8_t onoff_state;
	struct k_work button_work;
	struct k_timer button_timer;
};


static u8_t button_press_cnt;
static struct sw sw;

static struct gpio_callback button_cb;

static u8_t trans_id;
static u32_t time, last_time;
static u16_t primary_addr;
static u16_t primary_net_idx;

/*
 * Generic OnOff Model Server Message Handlers
 *
 * Mesh Model Specification 3.1.1
 *
 */

static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
	struct onoff_state *onoff_state = model->user_data;

	printk("addr 0x%04x onoff 0x%02x\n",
	       bt_mesh_model_elem(model)->addr, onoff_state->current);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(&msg, onoff_state->current);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		printk("Unable to send On Off Status response\n");
	}
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct net_buf_simple *msg = model->pub->msg;
	struct onoff_state *onoff_state = model->user_data;
	int err;

	onoff_state->current = net_buf_simple_pull_u8(buf);
	printk("addr 0x%02x state 0x%02x\n",
	       bt_mesh_model_elem(model)->addr, onoff_state->current);

	/* Pin set low turns on LED's on the nrf52840-pca10056 board */
	gpio_pin_write(onoff_state->led_device,
		       onoff_state->led_gpio_pin,
		       onoff_state->current ? 0 : 1);

	/*
	 * If a server has a publish address, it is required to
	 * publish status on a state change
	 *
	 * See Mesh Profile Specification 3.7.6.1.2
	 *
	 * Only publish if there is an assigned address
	 */

	if (onoff_state->previous != onoff_state->current &&
	    model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
		printk("publish last 0x%02x cur 0x%02x\n",
		       onoff_state->previous, onoff_state->current);
		onoff_state->previous = onoff_state->current;
		bt_mesh_model_msg_init(msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
		net_buf_simple_add_u8(msg, onoff_state->current);
		err = bt_mesh_model_publish(model);
		if (err) {
			printk("bt_mesh_model_publish err %d\n", err);
		}
	}
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	printk("gen_onoff_set\n");

	gen_onoff_set_unack(model, ctx, buf);
	gen_onoff_get(model, ctx, buf);
}

static void gen_onoff_status(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	u8_t	state;

	state = net_buf_simple_pull_u8(buf);

	printk("Node 0x%04x OnOff status from 0x%04x with state 0x%02x\n",
	       bt_mesh_model_elem(model)->addr, ctx->addr, state);
}

static int output_number(bt_mesh_output_action_t action, u32_t number)
{
	printk("OOB Number %06u\n", number);
	return 0;
}

static int output_string(const char *str)
{
	printk("OOB String %s\n", str);
	return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	printk("provisioning complete for net_idx 0x%04x addr 0x%04x\n",
	       net_idx, addr);
	primary_addr = addr;
	primary_net_idx = net_idx;
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static u8_t dev_uuid[16] = { 0xdd, 0xdd };

#define BUTTON_DEBOUNCE_DELAY_MS 250

/*
 * Map GPIO pins to button number
 * Change to select different GPIO input pins
 */

static uint8_t pin_to_sw(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(DT_ALIAS_SW0_GPIOS_PIN): return 0;
	case BIT(DT_ALIAS_SW1_GPIOS_PIN): return 1;
	case BIT(DT_ALIAS_SW2_GPIOS_PIN): return 2;
	case BIT(DT_ALIAS_SW3_GPIOS_PIN): return 3;
	}

	printk("No match for GPIO pin 0x%08x\n", pin_pos);
	return 0;
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pin_pos)
{
	/*
	 * One button press within a 1 second interval sends an on message
	 * More than one button press sends an off message
	 */

	time = k_uptime_get_32();

	/* debounce the switch */
	if (time < last_time + BUTTON_DEBOUNCE_DELAY_MS) {
		last_time = time;
		return;
	}

	if (button_press_cnt == 0U) {
		k_timer_start(&sw.button_timer, K_SECONDS(1), K_NO_WAIT);
	}

	printk("button_press_cnt 0x%02x\n", button_press_cnt);
	button_press_cnt++;

	/* The variable pin_pos is the pin position in the GPIO register,
	 * not the pin number. It's assumed that only one bit is set.
	 */

	sw.sw_num = pin_to_sw(pin_pos);
	last_time = time;
}

/*
 * Button Count Timer Worker
 */

static void button_cnt_timer(struct k_timer *work)
{
	struct sw *button_sw = CONTAINER_OF(work, struct sw, button_timer);

	button_sw->onoff_state = button_press_cnt == 1U ? 1 : 0;
	printk("button_press_cnt 0x%02x onoff_state 0x%02x\n",
	       button_press_cnt, button_sw->onoff_state);
	button_press_cnt = 0U;
	k_work_submit(&sw.button_work);
}

/*
 * Button Pressed Worker Task
 */

static void button_pressed_worker(struct k_work *work)
{
	struct bt_mesh_model *mod_cli, *mod_srv;
	struct bt_mesh_model_pub *pub_cli, *pub_srv;
	struct sw *sw = CONTAINER_OF(work, struct sw, button_work);
	int err;
	u8_t sw_idx = sw->sw_num;

	mod_cli = mod_cli_sw[sw_idx];
	pub_cli = mod_cli->pub;

	mod_srv = mod_srv_sw[sw_idx];
	pub_srv = mod_srv->pub;

	/* If unprovisioned, just call the set function.
	 * The intent is to have switch-like behavior
	 * prior to provisioning. Once provisioned,
	 * the button and its corresponding led are no longer
	 * associated and act independently. So, if a button is to
	 * control its associated led after provisioning, the button
	 * must be configured to either publish to the led's unicast
	 * address or a group to which the led is subscribed.
	 */

	if (primary_addr == BT_MESH_ADDR_UNASSIGNED) {
		NET_BUF_SIMPLE_DEFINE(msg, 1);
		struct bt_mesh_msg_ctx ctx = {
			.addr = sw_idx + primary_addr,
		};

		/* This is a dummy message sufficient
		 * for the led server
		 */

		net_buf_simple_add_u8(&msg, sw->onoff_state);
		gen_onoff_set_unack(mod_srv, &ctx, &msg);
		return;
	}

	if (pub_cli->addr == BT_MESH_ADDR_UNASSIGNED) {
		return;
	}

	printk("publish to 0x%04x onoff 0x%04x sw_idx 0x%04x\n",
	       pub_cli->addr, sw->onoff_state, sw_idx);
	bt_mesh_model_msg_init(pub_cli->msg,
			       BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_u8(pub_cli->msg, sw->onoff_state);
	net_buf_simple_add_u8(pub_cli->msg, trans_id++);
	err = bt_mesh_model_publish(mod_cli);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}
}

/* Disable OOB security for SILabs Android app */

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
#if 1
	.output_size = 6,
	.output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
	.output_number = output_number,
	.output_string = output_string,
#else
	.output_size = 0,
	.output_actions = 0,
	.output_number = 0,
#endif
	.complete = prov_complete,
	.reset = prov_reset,
};

/*
 * Bluetooth Ready Callback
 */

static void bt_ready(int err)
{
	struct bt_le_oob oob;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* Use identity address as device UUID */
	if (bt_le_oob_get_local(BT_ID_DEFAULT, &oob)) {
		printk("Identity Address unavailable\n");
	} else {
		memcpy(dev_uuid, oob.addr.a.val, 6);
	}

	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

	printk("Mesh initialized\n");
}

void init_led(u8_t dev, const char *port, u32_t pin_num)
{
	onoff_state[dev].led_device = device_get_binding(port);
	gpio_pin_configure(onoff_state[dev].led_device,
			   pin_num, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(onoff_state[dev].led_device, pin_num, 1);
}

void main(void)
{
	int err;

	printk("Initializing...\n");

	/* Initialize the button debouncer */
	last_time = k_uptime_get_32();

	/* Initialize button worker task*/
	k_work_init(&sw.button_work, button_pressed_worker);

	/* Initialize button count timer */
	k_timer_init(&sw.button_timer, button_cnt_timer, NULL);

	sw_device = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	gpio_pin_configure(sw_device, DT_ALIAS_SW0_GPIOS_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, DT_ALIAS_SW1_GPIOS_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, DT_ALIAS_SW2_GPIOS_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_pin_configure(sw_device, DT_ALIAS_SW3_GPIOS_PIN,
			  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
	gpio_init_callback(&button_cb, button_pressed,
			   BIT(DT_ALIAS_SW0_GPIOS_PIN) | BIT(DT_ALIAS_SW1_GPIOS_PIN) |
			   BIT(DT_ALIAS_SW2_GPIOS_PIN) | BIT(DT_ALIAS_SW3_GPIOS_PIN));
	gpio_add_callback(sw_device, &button_cb);
	gpio_pin_enable_callback(sw_device, DT_ALIAS_SW0_GPIOS_PIN);
	gpio_pin_enable_callback(sw_device, DT_ALIAS_SW1_GPIOS_PIN);
	gpio_pin_enable_callback(sw_device, DT_ALIAS_SW2_GPIOS_PIN);
	gpio_pin_enable_callback(sw_device, DT_ALIAS_SW3_GPIOS_PIN);

	/* Initialize LED's */
	init_led(0, DT_ALIAS_LED0_GPIOS_CONTROLLER, DT_ALIAS_LED0_GPIOS_PIN);
	init_led(1, DT_ALIAS_LED1_GPIOS_CONTROLLER, DT_ALIAS_LED1_GPIOS_PIN);
	init_led(2, DT_ALIAS_LED2_GPIOS_CONTROLLER, DT_ALIAS_LED2_GPIOS_PIN);
	init_led(3, DT_ALIAS_LED3_GPIOS_CONTROLLER, DT_ALIAS_LED3_GPIOS_PIN);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
