#include "common.h"

struct nvs_fs fs = {
	.sector_size = NVS_SECTOR_SIZE,
	.sector_count = NVS_SECTOR_COUNT,
	.offset = NVS_STORAGE_OFFSET,
	.max_len = NVS_MAX_ELEM_SIZE,
};

struct device *led_device[4];

struct device *button_device[4];

static struct k_work button_work;

static void button_pressed(struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

void gpio_init(void)
{
	static struct gpio_callback button_cb[4]; 

	/*------------------------------LEDs configiuratin & setting-----------------------------*/

	//NRF_P0->DIR |= 0x0001E000;
	//NRF_P0->OUT |= 0x0001E000;

	led_device[0] = device_get_binding(LED0_GPIO_PORT);
	gpio_pin_configure(led_device[0], LED0_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);

	led_device[1] = device_get_binding(LED1_GPIO_PORT);
	gpio_pin_configure(led_device[1], LED1_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[1], LED1_GPIO_PIN, 1);

	led_device[2] = device_get_binding(LED2_GPIO_PORT);
	gpio_pin_configure(led_device[2], LED2_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);

	led_device[3] = device_get_binding(LED3_GPIO_PORT);
	gpio_pin_configure(led_device[3], LED3_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);

	/*-------------------------------------------------------Buttons configiuratin & setting----------------------------------------------------------------*/

	k_work_init(&button_work, publish);
	
	button_device[0] = device_get_binding(SW0_GPIO_NAME);
	gpio_pin_configure(button_device[0], SW0_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], SW0_GPIO_PIN);

	button_device[1] = device_get_binding(SW1_GPIO_NAME);
	gpio_pin_configure(button_device[1], SW1_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[1], button_pressed, BIT(SW1_GPIO_PIN));
	gpio_add_callback(button_device[1], &button_cb[1]);
	gpio_pin_enable_callback(button_device[1], SW1_GPIO_PIN);

	button_device[2] = device_get_binding(SW2_GPIO_NAME);
	gpio_pin_configure(button_device[2], SW2_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[2], button_pressed, BIT(SW2_GPIO_PIN));
	gpio_add_callback(button_device[2], &button_cb[2]);
	gpio_pin_enable_callback(button_device[2], SW2_GPIO_PIN);

	button_device[3] = device_get_binding(SW3_GPIO_NAME);
	gpio_pin_configure(button_device[3], SW3_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[3], button_pressed, BIT(SW3_GPIO_PIN));
	gpio_add_callback(button_device[3], &button_cb[3]);
	gpio_pin_enable_callback(button_device[3], SW3_GPIO_PIN);
}

int NVS_read(uint16_t id, void *data_buffer, size_t len)
{
	int rc, ret = -1;

	if(nvs_read_hist(&fs, id, data_buffer, len, 0) ==  len)
	{
		printk("\n\r(NVS_Variable):%d found\n\r", id);

		rc = nvs_read(&fs, id, data_buffer, len);
		if (rc > 0) 
		{
			ret = 0;
		} 
	}
	else
	{
		printk("\n\r(NVS_Variable):%d not found\n\r", id);
	}

	return ret;
}

int NVS_write(uint16_t id, void *data_buffer, size_t len)
{
	int ret = -1;

	if(nvs_write(&fs, id, data_buffer, len) == len)
	{	
		ret = 0;
	}

	return ret;
}



struct light_state_t light_state_current;

void update_light_state(void)
{
	int power = 100 * (light_state_current.power + 32768)/65535;

	if(light_state_current.OnOff == 0x01)
	{
		gpio_pin_write(led_device[0], LED0_GPIO_PIN, 0);	//LED1 On
	}
	else
	{ 
		gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);	//LED1 Off
	}
	
	if(power < 50)
	{	
		gpio_pin_write(led_device[2], LED2_GPIO_PIN, 0);	//LED3 On
		gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);	//LED4 Off
	}
	else
	{
		gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);	//LED3 Off
		gpio_pin_write(led_device[3], LED3_GPIO_PIN, 0);	//LED4 On
	}
}

void nvs_light_state_save(void)
{
	if(NVS_write(NVS_LED_STATE_ID, &light_state_current, sizeof(struct light_state_t)) == 0)
	{
		printk("\n\rLight state has saved !!\n\r");
	}
	else
	{
		printk("\n\rLight state has not saved !!\n\r");
	}
}
