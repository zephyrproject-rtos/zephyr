/**
1) define message opcodes for get set and status page 298 models specs
2) declare model functions
3) declare and assign cfg server,cfg cli, health_srv
4) Define a model publication context BT_MESH_MODEL_PUB_DEFINE(_name, _update, _msg_len)
5) declare array of struct bt_mesh_model_op for model functions
6) declare array of bt_mesh_model for root element (including server and client)
7) declare element using bt_mesh_elem array
8) declare bt_mesh_comp for node (elements+elements count +CID)
**/

#include <misc/printk.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <nrf.h>
#include <device.h>
#include <gpio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <errno.h>
#include <bluetooth/mesh.h>
#include <stdio.h>
#include <board.h>


#define CID_INTEL 	 0x0002   /*Company identifier assigned by the Bluetooth SIG*/
#define NODE_ADDR  	 0x0003   /*Unicast Address*/
#define GROUP_ADDR 	 0x9999  /*The Address to use for pub and sub*/
#define START     	 0x20      /* Start of dummy data */
#define DATA_LEN   	 12       /*length of status (more than 8 == segmented) */
#define TEMP_ID		 	 0b00000000001
#define PRESSURE_ID  0b00000000010
#define X_ID 	 0b00000000011



/*For Provisioning and Configurations*/

static const u8_t net_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};


static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
static u16_t addr = NODE_ADDR;
static u32_t seq;
static u16_t primary_addr;
static u16_t primary_net_idx;

/*
 * The include must follow the define for it to take effect.
 * If it isn't, the domain defaults to "general"
 */

#define SYS_LOG_DOMAIN "sensor"
#include <logging/sys_log.h>


/* 1) Defining server messages opcodes */
#define BT_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET	   	BT_MESH_MODEL_OP_2(0x82, 0x30)
#define BT_MESH_MODEL_OP_SENSOR_DESCRIPTOR_STATUS		BT_MESH_MODEL_OP_1(0x51)
#define BT_MESH_MODEL_OP_SENSOR_GET                	BT_MESH_MODEL_OP_2(0x82, 0x31)
#define BT_MESH_MODEL_OP_SENSOR_STATUS	            BT_MESH_MODEL_OP_1(0x52)
#define BT_MESH_MODEL_OP_SENSOR_COLUMN_GET	        BT_MESH_MODEL_OP_2(0x82,0x32)
#define BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS	      BT_MESH_MODEL_OP_1(0x53)
#define BT_MESH_MODEL_OP_SENSOR_SERIES_GET	        BT_MESH_MODEL_OP_2(0x82,0x33)
#define BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS	      BT_MESH_MODEL_OP_1(0x54)

/* 2) Declare model functions*/
static void sen_descriptor_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf);
static void sen_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf);
static void sen_column_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf);
static void sen_series_get(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf);
static int periodic_update(struct bt_mesh_model *mod);

/*
 * Server Configuration Declaration
 */

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_ENABLED,
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
 * 4) Publication Declarations
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
BT_MESH_MODEL_PUB_DEFINE(sensor_pub_srv, periodic_update, 2+DATA_LEN);
u16_t sensors[]={200,150,0};
bool dir[]={true,true};

int periodic_update (struct bt_mesh_model *mod)
{
	printk("******* updating *********\n");
	bt_mesh_model_msg_init(sensor_pub_srv.msg, BT_MESH_MODEL_OP_SENSOR_STATUS);

	/*Temp_simulation*/
	net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+TEMP_ID));
	if (sensors[0]==250 || sensors[0] ==150)
		dir[0] = !dir[0];

	if (dir[0])
			sensors[0]++;
	else
			sensors[0]--;
	net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[0]);
	printk("TVL: %04x,DATA: %i\n",((0b0<<7)+(0b1000<<3)+TEMP_ID), sensors[0]);

	/*Pressure simulation*/

	net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+PRESSURE_ID));
	if (sensors[1]==0 || sensors[1] ==200)
	dir[1] = !dir[1];
	if (dir[1])
			sensors[1]+=5;
	else
			sensors[1]-=5;
	net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[1]);
	printk("TVL: %04x,DATA: %i\n",((0b0<<7)+(0b1000<<3)+PRESSURE_ID), sensors[1]);

	/*X simulation*/
	//net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+X_ID)); //uncomment=>12 bytes
	//sensors[2]=(rand()%(255+1));

	net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[2]);
	printk("TVL: %04x,DATA: %04x\n",((0b0<<7)+(0b1000<<3)+X_ID), sensors[2]);
	printk("[GUI] %04x-startE2E\n",NODE_ADDR);

	return 0;
}

/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */

/*
 * Sensor Model Server Op Dispatch Table
 *
 */

static const struct bt_mesh_model_op sensor_srv_op[] = {
	{ BT_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET, 0, sen_descriptor_get },
	{ BT_MESH_MODEL_OP_SENSOR_GET, 0, sen_get },
	{ BT_MESH_MODEL_OP_SENSOR_COLUMN_GET, 0, sen_column_get },
  { BT_MESH_MODEL_OP_SENSOR_SERIES_GET, 0,sen_series_get },
	BT_MESH_MODEL_OP_END,
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
	BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_SRV, sensor_srv_op,
		      &sensor_pub_srv,NULL),
};
/* Declaring root element */
static struct bt_mesh_elem root = BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE);

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = &root,
	.elem_count = 1,
};


struct device *sw_device;
static struct gpio_callback button_cb;  //button call back
static u32_t time, last_time;
#define BUTTON_DEBOUNCE_DELAY_MS 250
struct bt_mesh_cfg_mod_pub pub;

/*
 * Map GPIO pins to button number
 * Change to select different GPIO input pins
 */

static uint8_t pin_to_sw(uint32_t pin_pos)
{
	switch (pin_pos) {
	case BIT(SW0_GPIO_PIN): return 0;
	case BIT(SW1_GPIO_PIN): return 1;
	case BIT(SW2_GPIO_PIN): return 2;
	case BIT(SW3_GPIO_PIN): return 3;
	}
	SYS_LOG_ERR("No match for GPIO pin 0x%08x", pin_pos);
	return 0;
}
static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   uint32_t pin_pos)
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



	/* The variable pin_pos is the pin position in the GPIO register,
	 * not the pin number. It's assumed that only one bit is set.
	 */
	switch (pin_to_sw(pin_pos))
	{
		case 0:
		printk("[GUI] starting Publishing \n");
		pub.period=0b01000010;
		printk("Period is 0x%04x \n",pub.period );
		bt_mesh_cfg_mod_pub_set(net_idx, addr, addr ,BT_MESH_MODEL_ID_SENSOR_SRV, &pub, NULL);
		break;

		case 1:
		printk("[GUI] Stopping Publishing \n");
		struct bt_mesh_cfg_mod_pub pub2 = {
		.period =0,
		};
		bt_mesh_cfg_mod_pub_set(net_idx, addr, addr ,BT_MESH_MODEL_ID_SENSOR_SRV, &pub2, NULL);
		break;

		case 2:
		printk("Button 3 pressed - No action\n");
		break;

		case 3:
		printk("Button 4 pressed - DEC \n");
		if ((pub.period & BIT_MASK(6))>=0x01)
				pub.period --;
		printk("Period is 0x%04x DEC \n",pub.period );
		bt_mesh_cfg_mod_pub_set(net_idx, addr, addr ,BT_MESH_MODEL_ID_SENSOR_SRV, &pub, NULL);
		break;

		default:
		printk("BUTTON ERROR \n");
		break;
	}
	last_time = time;
}



/*
 * Sensor Server Message Handlers
 *
 * Mesh Model Specification 3.1.1
 *
 */

static void sen_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	// 2 opcode + 15 message + 4 AppMIC
	NET_BUF_SIMPLE_DEFINE(msg, 2 + DATA_LEN + 4);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_SENSOR_STATUS);
		/*Temp_simulation*/
		net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+TEMP_ID));
		net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[0]);
		/*Pressure simulation*/
		net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+PRESSURE_ID));
		net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[1]);
		/*X simulation*/

		//net_buf_simple_add_le16(sensor_pub_srv.msg,((0b0<<7)+(0b1000<<3)+X_ID));
		net_buf_simple_add_le16(sensor_pub_srv.msg,sensors[2]);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		SYS_LOG_ERR("Unable to Status response");
	}
}
void sen_descriptor_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
				{

				}

void sen_column_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
				{

				}
void sen_series_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
				{

				}



static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	SYS_LOG_INF("OOB Number %u", number);
	return 0;
}

static int output_string(const char *str)
{
	SYS_LOG_INF("OOB String %s", str);
	return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	SYS_LOG_INF("provisioning complete for net_idx 0x%04x addr 0x%04x",
		    net_idx, addr);
	primary_addr = addr;
	primary_net_idx = net_idx;
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static u8_t dev_uuid[16] = { 0xdd, 0xdd };

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

 static void configure(void)
 {
 	printk("Configuring...\n");
 	/* Add Application Key */
 	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);
	/*Bind the App key Server Model */
 	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx, BT_MESH_MODEL_ID_SENSOR_SRV, NULL);

 /* publish periodicaly to a remote address */
	pub.addr = 0x0001;
	pub.app_idx=app_idx;
	pub.ttl = 0x07;
	pub.period =0b01000010;
	printk("Configuration complete\n");
 }


static void bt_ready(int err)
{
	struct bt_le_oob oob;

	if (err) {
		SYS_LOG_ERR("Bluetooth init failed (err %d", err);
		return;
	}

	SYS_LOG_INF("Bluetooth initialized");

	/* Use identity address as device UUID */
if (bt_le_oob_get_local(&oob)) {
		SYS_LOG_ERR("Identity Address unavailable");
	} else {
		memcpy(dev_uuid, oob.addr.a.val, 6);
	}
	err = bt_mesh_init(&prov, &comp);
	if (err) {
		SYS_LOG_ERR("Initializing mesh failed (err %d)", err);
		return;
	}

	//bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
	SYS_LOG_INF("Mesh initialized");

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
				dev_key);
	if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	}

	printk("Provisioning completed\n");
	configure();
}


void log_cbuf_put(const char *format, ...)
{
	va_list args;
	char buf[250];

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	printk("[%04x] %s", primary_addr, buf);
}

void board_init(u16_t *addr, u32_t *seq)
{
	*addr = NODE_ADDR;
	*seq = 0;
}

void main(void)
{
		int err;

		/*
		 * Initialize the logger hook
		 */
		syslog_hook_install(log_cbuf_put);
		SYS_LOG_INF("Initializing...");

		/* Initialize the button debouncer */
		last_time = k_uptime_get_32();


		sw_device = device_get_binding(SW0_GPIO_NAME);
		/* configuring buttons */
		gpio_pin_configure(sw_device, SW0_GPIO_PIN,
				  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
		gpio_pin_configure(sw_device, SW1_GPIO_PIN,
				  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
		gpio_pin_configure(sw_device, SW2_GPIO_PIN,
				  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
		gpio_pin_configure(sw_device, SW3_GPIO_PIN,
				  (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				   GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP));
		gpio_init_callback(&button_cb, button_pressed,
				   BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN) |
				   BIT(SW2_GPIO_PIN) | BIT(SW3_GPIO_PIN));
		gpio_add_callback(sw_device, &button_cb);
		gpio_pin_enable_callback(sw_device, SW0_GPIO_PIN);
		gpio_pin_enable_callback(sw_device, SW1_GPIO_PIN);
		gpio_pin_enable_callback(sw_device, SW2_GPIO_PIN);
		gpio_pin_enable_callback(sw_device, SW3_GPIO_PIN);
		board_init(&addr, &seq);
		err = bt_enable(bt_ready);
		if (err) {
			SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
		}
}
