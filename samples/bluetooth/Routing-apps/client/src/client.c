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


#define CID_INTEL 0x0002 /*Company identifier assigned by the Bluetooth SIG*/
#define NODE_ADDR 0x0001 /*Unicast Address*/
#define GROUP_ADDR 0x9999 /*The Address to use for pub and sub*/
#define STACKSIZE 1024
#define ALLIGN 4
#define QSIZE 1024 //XXX
#define AGGREGATION_PRIORITY 5
#define AGGREGATION_INTERVAL 10 *1000 //in ms
#define NODES_NUM					3
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
struct k_timer aggregation_timer;
unsigned int bytes_recvd=0;
//u8_t	sensor_state[2];
struct sensors{
	u16_t unicast;
	u32_t temp;
	u32_t pressure;
	u32_t X;
};
//XXX
struct sensors sensor_data[NODES_NUM]={
	{0x0002,0,0,0},{0x0003,0,0,0},{0x0004,0,0,0}
};
K_SEM_DEFINE(sem, 1, 1);
K_MSGQ_DEFINE(msgQ, sizeof(struct sensors), QSIZE, ALLIGN); /*Initialize the Message Queue*/
K_THREAD_STACK_DEFINE(stack_area, STACKSIZE);
struct k_thread aggregation_thread;


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
static void sen_descriptor_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf);
static void sen_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf);
static void sen_column_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf);
static void sen_series_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf);


//static int periodic_update(struct bt_mesh_model *mod);

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
BT_MESH_MODEL_PUB_DEFINE(sensor_pub_cli, NULL, 1+0);  //client doesn't publish data


/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */

/*
 * Sensor Model Server Client Dispatch Table
 *
 */

 static const struct bt_mesh_model_op sensor_cli_op[] = {
 	{ BT_MESH_MODEL_OP_SENSOR_DESCRIPTOR_STATUS, 1, sen_descriptor_status },
   { BT_MESH_MODEL_OP_SENSOR_STATUS, 1, sen_status }, //minimum data to receive
   { BT_MESH_MODEL_OP_SENSOR_COLUMN_STATUS, 1, sen_column_status },
   { BT_MESH_MODEL_OP_SENSOR_SERIES_STATUS, 1, sen_series_status },
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
  	/*              ID-OPCODE - PUBLICATION - USER_DATA */
	BT_MESH_MODEL(BT_MESH_MODEL_ID_SENSOR_CLI, sensor_cli_op,
		      &sensor_pub_cli, NULL),
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
		printk("Button 1 pressed - Client No Action\n");
		break;

		case 1:
		printk("Button 2 pressed - Client No Action\n");
		break;

		case 2:
		printk("Button 3 pressed - Client No Action \n");
		break;

		case 3:
		printk("Button 4 pressed - Client No Action\n");
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
static void sen_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct sensors recvd_data;
	printk("[GUI] %04x-endE2E\n",ctx->addr);
	printk("[GUI] %04x-PktActual-%d\n",ctx->addr,buf->len);
	bytes_recvd+=buf->len;
	/*skipping the formalities, assuming we know format, size and did ID mapping*/
net_buf_simple_pull_le16(buf);
recvd_data.temp = net_buf_simple_pull_le16(buf);
net_buf_simple_pull_le16(buf);
recvd_data.pressure = net_buf_simple_pull_le16(buf);
//net_buf_simple_pull_le16(buf);
recvd_data.X = net_buf_simple_pull_le16(buf);
recvd_data.unicast = ctx->addr;
printk("[%04x] status: Temperature: %i, Pressure: %i \n",recvd_data.unicast,recvd_data.temp,recvd_data.pressure);
k_msgq_put(&msgQ, &recvd_data, K_NO_WAIT);
}

static void sen_descriptor_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
				{

				}

static void sen_column_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf)
	{

	}
static void sen_series_status(struct bt_mesh_model *model,struct bt_mesh_msg_ctx *ctx,struct net_buf_simple *buf)
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

/*Aggregation Timer*/
void aggregation_timer_expiry_fn(struct k_timer *timer_id)
{
	k_sem_give(&sem);
}

K_TIMER_DEFINE(aggregation_timer, aggregation_timer_expiry_fn, NULL);

void aggregator()
{
	u32_t counter[NODES_NUM+1]={0};
	u32_t msg_num;
	int i;
	struct sensors sensor_recvd;
	k_sem_take(&sem, K_FOREVER);
	while(1)
	{
		k_sem_take(&sem, K_FOREVER);
		/*reads data from Queue and */

		printk("[GUI] %04x-Throughput-%i\n",NODE_ADDR,((bytes_recvd*100)/(AGGREGATION_INTERVAL/1000)));
		bytes_recvd=0;
		msg_num= k_msgq_num_used_get(&msgQ);
		if(!msg_num)
				{
					printk("WARNING: ALL NODES ARE DOWN!\n");
					continue;
				}
		while(counter[0]<=msg_num && k_msgq_get(&msgQ, &sensor_recvd, K_NO_WAIT) == 0)
		{
			for(i=0;i<NODES_NUM;i++)
			{
				if(sensor_recvd.unicast==sensor_data[i].unicast)
					{
						sensor_data[i].temp+= sensor_recvd.temp;
						sensor_data[i].pressure+=sensor_recvd.pressure;
						counter[i+1]++;
						break;
					}

			}
		counter[0]++;
		}
		for(i=0;i<NODES_NUM;i++)
		{
			if (counter[i+1]!=0)
			{
			printk("[GUI] %04x-Temperature-%i\n",sensor_data[i].unicast,sensor_data[i].temp/counter[i+1]);
			printk("[GUI] %04x-Pressure-%i\n",sensor_data[i].unicast,sensor_data[i].pressure/counter[i+1]);
			}
			sensor_data[i].temp=0;
			sensor_data[i].pressure=0;
			counter[i]=0;
		}
		counter[i]=0;
	}
}


/*
 * Bluetooth Ready Callback
 */

 static void configure(void)
 {
 	printk("Configuring...\n");
 	/* Add Application Key */
 	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);
	/*Bind the App key to BT_MESH_MODEL_ID_GEN_ONOFF_SRV (ONOFF Server Model) */
 	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx, BT_MESH_MODEL_ID_SENSOR_CLI, NULL);
	/* publish periodicaly to a remote address */
//	bt_mesh_cfg_mod_sub_add(net_idx, addr, addr, 0x0001, BT_MESH_MODEL_ID_SENSOR_CLI, NULL);
	printk("Configuration complete\n");

	k_thread_create(&aggregation_thread, stack_area, K_THREAD_STACK_SIZEOF(stack_area), aggregator, NULL, NULL, NULL, AGGREGATION_PRIORITY, 0, K_NO_WAIT);
  k_timer_start(&aggregation_timer, AGGREGATION_INTERVAL, AGGREGATION_INTERVAL);
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
	//printk("[%04x] %s", primary_addr, buf);
}

void board_init(u16_t *addr, u32_t *seq)
{
	*addr = NODE_ADDR;
	*seq = 0;
}

void main(void)
{
	printk("1\n");
	int err;

	/*
	 * Initialize the logger hook
	 */
	syslog_hook_install(log_cbuf_put);
	SYS_LOG_INF("Initializing...");
	/* configuring buttons */

	sw_device = device_get_binding(SW0_GPIO_NAME);
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
	printk("2\n");
	gpio_add_callback(sw_device, &button_cb);
	printk("3\n");
	gpio_pin_enable_callback(sw_device, SW0_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW1_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW2_GPIO_PIN);
	gpio_pin_enable_callback(sw_device, SW3_GPIO_PIN);
	printk("4\n");
	board_init(&addr, &seq);
		printk("5\n");
	err = bt_enable(bt_ready);
	printk("6\n");
	if (err) {
		SYS_LOG_ERR("Bluetooth init failed (err %d)", err);
	}
}
