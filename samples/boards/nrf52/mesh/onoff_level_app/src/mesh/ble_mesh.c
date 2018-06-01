#include "common.h"

/*
static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	printk("OOB Number: %u\n", number);

	//board_output_number(action, number);

	return 0;
}

static int output_string(const char *str)
{
	printk("OOB String: %s\n", str);

	return 0;
}
*/

static void prov_complete(u16_t net_idx, u16_t addr)
{
	//board_prov_complete();
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

//static const uint8_t dev_uuid[16] = { 0x33, 0x33 };
static uint8_t dev_uuid[16];

static const struct bt_mesh_prov prov = 
{
	.uuid = dev_uuid,

	//.static_val = static_oob,
	//.static_val_len = 16,

	//.output_size = 6,
	//.output_actions = BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING,
	
	//.output_number = output_number,
	//.output_string = output_string,

	.complete = prov_complete,
	.reset = prov_reset,
};

void bt_ready(int err)
{
	struct bt_le_oob oob;

	if (err) 
        {
		printk("Bluetooth init failed (err %d", err);
		return;
	}

	printk("Bluetooth initialized");

	/* Use identity address as device UUID */
	if (bt_le_oob_get_local(&oob)) 
	{
		printk("Identity Address unavailable");
	} 
	else 
	{
		memcpy(dev_uuid, oob.addr.a.val, 6);
	}

        //board_init();

	err = bt_mesh_init(&prov, &comp);

	if (err) {
		printk("Initializing mesh failed (err %d)\n\r", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}
        
	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

	printk("Mesh initialized\n\r");
}
