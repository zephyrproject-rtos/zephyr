/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>

#include "cts.h"

#define BT_EVAL_UUID_TEST BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x5155, 0x5678, 0x1234, 0x56789abcdef3))
#define BT_EVAL_UUID_TEST_notify BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x12345678, 0x5566, 0x5678, 0x1234, 0x56789abcdef3))

/*SPI*/
#include "nrfx_spis.h"

#define PIN_SCK 29
#define PIN_MOSI 31
#define PIN_MISO 30
#define PIN_CSN 28
#define SPIS_NR 0


#include <devicetree.h>
#include <drivers/gpio.h>
#define LED0_NODE DT_ALIAS(led2)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
//#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define PIN	3
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
const struct device *led;
bool low = true;
bool high =false;

nrfx_spis_t spis_t = NRFX_SPIS_INSTANCE(SPIS_NR);
nrfx_spis_config_t spis_config_t = NRFX_SPIS_DEFAULT_CONFIG(PIN_SCK,PIN_MOSI,PIN_MISO,PIN_CSN);

uint8_t readend=0;

uint8_t rx_buffer[8];
uint8_t tx_buffer[8];
uint8_t tmpbuffer[8] = {0};
K_MSGQ_DEFINE(rx_msgq, sizeof(rx_buffer),8, 0);
K_MSGQ_DEFINE(tx_msgq, sizeof(tx_buffer),8, 0);
/*spi end*/
uint8_t blerecv=0;
K_SEM_DEFINE(SPI_EVENT, 0, 1);
uint32_t start_time;
uint32_t stop_time;
uint32_t cycles_spent;
uint32_t nanoseconds_spent;
uint32_t delay_array[100]={0};
uint8_t delay_count=0;
uint8_t end_flag=0;

uint8_t connecting=0;

void spis_event_handler_t(nrfx_spis_evt_t const *p_event, void *p_context){
    int err;
    switch(p_event->evt_type){
        case NRFX_SPIS_XFER_DONE:
		//printk("zzz\n");
		if(rx_buffer[0]!= 255)
		{	
			//printk("zzz\n");
			//k_msgq_cleanup(&rx_msgq);
			//k_msgq_put(&rx_msgq, &rx_buffer, K_NO_WAIT);
			memcpy(tmpbuffer, rx_buffer, sizeof(rx_buffer));
			//printk("read ");
			//for(int i=0;i<8;i++){
			//	printk("%d ",rx_buffer[i]);
			//}
			//printk("\r\n");	
			k_sem_give(&SPI_EVENT);
			//readend=1;

		}
            err = nrfx_spis_buffers_set(&spis_t, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
            if(err != NRFX_SUCCESS){
                printk("Error with setting.\n");
            }
		

            break;
        default:
            ;
    }
	
}

/* Custom Service Variables */
static struct bt_uuid_128 vnd_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
	0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t vnd_value[] = { 'V', 'e', 'n', 'd', 'o', 'r' };

static ssize_t read_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	printk("read_vnd: %u %u %u\n", value[0],value[1],value[2]);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;
	printk("write_vnd: %u %u %u\n", value[0],value[1],value[2]);
	if (offset + len > sizeof(vnd_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static uint8_t simulate_vnd;
static uint8_t indicating;
static struct bt_gatt_indicate_params ind_params;

static void vnd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_vnd = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
	indicating = 0U;
}

//#define MAX_DATA 74
/*
static uint8_t vnd_long_value[] = {
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '1',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '2',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '3',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '4',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '5',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '6',
		  '.', ' ' };
*/
#define MAX_DATA 16
static uint8_t vnd_long_value[] = {
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', 'd', 'a', 't', 'a', '1',
		  'V', 'e', 'n', 'd', 'o', 'r', ' ', '66', '88', '77' };

static ssize_t read_long_vnd(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	printk("channel %u\r\n",lll_chan_get());
	printk("read_long_vnd: %c %c %c\n", value[0],value[1],value[2]);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(vnd_long_value));
}

static ssize_t write_long_vnd(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      uint16_t len, uint16_t offset, uint8_t flags)
{
	uint8_t *value = attr->user_data;

	stop_time = k_uptime_get_32();
	cycles_spent = stop_time - start_time;
	//nanoseconds_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent);
/*	
	if(delay_count<100){	
	delay_array[delay_count]=cycles_spent;
	delay_count++;}
	else{
		for(int i=0;i<100;i++){
			printk("%u\n",delay_array[i]);
			end_flag=1;
		}	
	}
*/
	//printk("send --> recv %u ms %u\n",cycles_spent,delay_count);

	//printk("write_long_vnd: %u %u %u %u %u %u %u %u \n", value[0],value[1],value[2], value[3],value[4],value[5], value[6],value[7]);

	if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
		return 0;
	}

	if (offset + len > sizeof(vnd_long_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	//printk("b peripheral len %u \n",buf->len);
	
	printk("b peripheral  DATA ");
	for(int i=0;i<8;i++){
		printk("%u ",((uint8_t *)buf)[i]);
	}
	printk("\n");

	//memcpy(value + offset, buf, len);
	memcpy(tx_buffer, buf, len);
	blerecv=1;
/*
	printk("copy to spi tx data ");
	gpio_pin_set(led, PIN, (int)low);
	
	for(int i = 0 ; i< 8 ; i++)
		printk("%d ",tx_buffer[i]);
	printk("\r\n");
*/
	gpio_pin_set(led, PIN, (int)high);	
	gpio_pin_set(led, PIN, (int)low);

	readend=0;


	return len;
}

static const struct bt_uuid_128 vnd_long_uuid = BT_UUID_INIT_128(
	0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x55, 0x15, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_cep vnd_long_cep = {
	.properties = BT_GATT_CEP_RELIABLE_WRITE,
};

static int signed_value;

static ssize_t read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;
	printk("read_signed: %u %u %u\n", value[0],value[1],value[2]);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 sizeof(signed_value));
}

static ssize_t write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset,
			    uint8_t flags)
{
	uint8_t *value = attr->user_data;
	printk("write_signed: %u %u %u\n", value[0],value[1],value[2]);
	if (offset + len > sizeof(signed_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static const struct bt_uuid_128 vnd_signed_uuid = BT_UUID_INIT_128(
	0xf3, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x13,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x13);

static const struct bt_uuid_128 vnd_write_cmd_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static ssize_t write_without_rsp_vnd(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len, uint16_t offset,
				     uint8_t flags)
{
	uint8_t *value = attr->user_data;
	
	printk("Value was written: %u %u %u\n", value[0],value[1],value[2]);
	/* Write request received. Reject it since this char only accepts
	 * Write Commands.
	 */
	if (!(flags & BT_GATT_WRITE_FLAG_CMD)) {
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_REQ_REJECTED);
	}

	if (offset + len > sizeof(vnd_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(vnd_svc,
	BT_GATT_PRIMARY_SERVICE(&vnd_uuid),
	BT_GATT_CHARACTERISTIC(BT_EVAL_UUID_TEST_notify, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CHARACTERISTIC(&vnd_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       read_vnd, write_vnd, vnd_value),
	BT_GATT_CCC(vnd_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(&vnd_auth_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       read_vnd, write_vnd, vnd_value),
	BT_GATT_CHARACTERISTIC(&vnd_long_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_EXT_PROP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_long_vnd, write_long_vnd, &vnd_long_value),
	BT_GATT_CEP(&vnd_long_cep),
	BT_GATT_CHARACTERISTIC(&vnd_signed_uuid.uuid, BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_AUTH,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_signed, write_signed, &signed_value),
	BT_GATT_CHARACTERISTIC(&vnd_write_cmd_uuid.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL,
			       write_without_rsp_vnd, &vnd_value),
	

);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf9, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x0d, 0x18, 0x0f, 0x18,  0x05, 0x18),
/*	
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
*/
	//BT_DATA_BYTES(BT_DATA_UUID128_ALL,BT_EVAL_UUID_TEST),

/*	
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_CTS_VAL)),

	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
*/


};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected!!! channel %u\r\n",lll_chan_get());
		connecting=1;
		//printk("channel %u\r\n",lll_chan_get());
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
	connecting=0;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	cts_init();

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}
	//printk("heartrate %d \r\n",heartrate);
	bt_hrs_notify(heartrate);
}





static void test_notify(void)
{
	static uint8_t testcount[8] = {0};

	/* Heartrate measurements simulation */
	//k_msgq_get(&rx_msgq, &rx_buffer, K_NO_WAIT);
	//testcount[1]++;
	//if (testcount[1] == 100) {
	//	testcount[1] = 0;
	//}
	memcpy(testcount, tmpbuffer, sizeof(tmpbuffer));
	//testcount[0] = rx_buffer[0];
	//testcount[1] = rx_buffer[1];
	//testcount[2] = rx_buffer[2];
	//testcount[3] = rx_buffer[3];
	//testcount[4] = rx_buffer[4];
	//testcount[5] = rx_buffer[5];
	printk("heartrate %d %d %d %d %d %d\r\n",testcount[0],testcount[1],testcount[2],testcount[3],testcount[4],testcount[5]);
	start_time = k_uptime_get_32();
	bt_test_notify(testcount);

}
static void manual_isr_setup()
{
    //IRQ_DIRECT_CONNECT(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn, 0, nrfx_spis_0_irq_handler, 0);
    //irq_enable(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
      IRQ_CONNECT(DT_IRQN(DT_NODELABEL(spi0)), 
                           DT_IRQ(DT_NODELABEL(spi0), priority), 
                            nrfx_isr, nrfx_spis_0_irq_handler, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(spi0)));
}

void init_spis(){
    int err;
    err = nrfx_spis_init(&spis_t,&spis_config_t,spis_event_handler_t,NULL);
    if(err != NRFX_SUCCESS){
        printk("Error with init.\n");
    } else {
        printk("SPIS started.\n");
    }
}

void main(void)
{
	int err,ret;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	bt_addr_le_t addr_array[CONFIG_BT_ID_MAX];
	size_t size = CONFIG_BT_ID_MAX;
	bt_id_get(addr_array, &size);
	for (int i = 0; i < size; i++) {
		char dev[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(&addr_array[i], dev, sizeof(dev));
		printk("%s\n", dev);
	}
	k_msleep(1000);
	
	/*ec:e4:cd:dc:1b:f0*/

	bt_conn_cb_register(&conn_callbacks);
	//bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	
	/*spi led*/
	init_spis();

	
	led = device_get_binding(LED0);
	if (led == NULL) {
		return;
	}

	ret = gpio_pin_configure(led, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}
        gpio_pin_set(led, PIN, (int)low);

	manual_isr_setup();

	err = nrfx_spis_buffers_set(&spis_t, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));
	if(err != NRFX_SUCCESS){
		printk("Error with setting.\n");
	}
	



	while (1) {
		//k_sleep(K_SECONDS(1));

		/* Current Time Service updates only when time is changed */
		cts_notify();

		/* Heartrate measurements simulation */
		//hrs_notify();

		
		/* Battery level simulat+ion */
		//bas_notify();
		
		//test_notify();
		//if(readend==1){
		if(k_sem_take(&SPI_EVENT, K_FOREVER)==0){
			test_notify();
			readend=0;
				


		}
		
		

		/* Vendor indication simulation */
		if (simulate_vnd) {
			if (indicating) {
				continue;
			}

			ind_params.attr = &vnd_svc.attrs[2];
			ind_params.func = indicate_cb;
			ind_params.data = &indicating;
			ind_params.len = sizeof(indicating);

			if (bt_gatt_indicate(NULL, &ind_params) == 0) {
				indicating = 1U;
			}
		}
	}
}
