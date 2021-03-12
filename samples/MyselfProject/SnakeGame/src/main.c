#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "drivers/lcd_display.h"

#include <stdlib.h>
// #include <malloc.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)



//按键相关
#define SW0_NODE    DT_ALIAS(sw0)
#define SW1_NODE    DT_ALIAS(sw1)
#define SW2_NODE    DT_ALIAS(sw2)

#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))

#define SW1_GPIO_LABEL	DT_GPIO_LABEL(SW1_NODE, gpios)
#define SW1_GPIO_PIN	DT_GPIO_PIN(SW1_NODE, gpios)
#define SW1_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW1_NODE, gpios))

#define SW2_GPIO_LABEL	DT_GPIO_LABEL(SW2_NODE, gpios)
#define SW2_GPIO_PIN	DT_GPIO_PIN(SW2_NODE, gpios)
#define SW2_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW2_NODE, gpios))



#define BUTTON_DEBOUNCE_DELAY_MS 250

#define QUICKLY_CLICK_DURATION 200

#define KEY_RESERVED		0
#define KEY_LEFT			1
#define KEY_MIDDLE          2
#define KEY_RIGHT           3

enum{
    KEY_TYPE_SHORT_UP,
    KEY_TYPE_DOUBLE_CLICK,
    KEY_TYPE_TRIPLE_CLICK,
};




static struct gpio_callback button_cb_data;

struct input_manager_info
{
    uint8_t click_num;
    struct k_delayed_work work_item;
    int64_t report_stamp;
    uint8_t keycode;
    uint8_t keytype;
}input_manager;



//一个全局的input,val_code 更新

struct key_report
{
	uint16_t prev_stable_keycode;
}key_manager;

static uint8_t pin_to_keycode(uint32_t pin_pos)
{
    switch (pin_pos)
    {
    case BIT(SW0_GPIO_PIN):
        return KEY_LEFT;
    case BIT(SW1_GPIO_PIN):
        return KEY_MIDDLE;
    case BIT(SW2_GPIO_PIN):
        return KEY_RIGHT;
    default:
        break;
    }
}
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
    uint16_t keycode=0;

    keycode=pin_to_keycode(pins);


    if(keycode!=key_manager.prev_stable_keycode)
    {
        key_manager.prev_stable_keycode=keycode;
        input_manager.click_num=1;
        input_manager.keycode=keycode;
        
    }else{
        input_manager.click_num++;
    }

    switch (input_manager.click_num)
    {
    case 1:
        input_manager.keytype=KEY_TYPE_SHORT_UP;
        break;
    case 2:
        input_manager.keytype=KEY_TYPE_DOUBLE_CLICK;
        break;
    case 3:
        input_manager.keytype=KEY_TYPE_TRIPLE_CLICK;
        break;
    default:
        break;
    }

    //printk("button_pressed keycode:%d key_type:%d\n", input_manager.keycode,input_manager.keytype);
    k_delayed_work_submit(&input_manager.work_item, K_MSEC(QUICKLY_CLICK_DURATION));
	// printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}



void report_key_event(struct k_work *work)
{
    printk("report_key_event keycode:%d key_type:%d\n", input_manager.keycode,input_manager.keytype);

    //超过一定时间应该把clicknum清空
    key_manager.prev_stable_keycode=KEY_RESERVED;//按键释放

}






void main(void)
{
    #if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7735), okay)
    #define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))
    #else 
    #define DISPLAY_DEV_NAME ""
    #pragma message("sitronix_st7735 not find")
    #endif

    // const struct device *dev;
    
	// dev = device_get_binding(LED0);
	// gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	// gpio_pin_set(dev, PIN, 1);


    const struct device *button0;
    const struct device *button1;
    const struct device *button2;

    k_delayed_work_init(&input_manager.work_item, report_key_event);


    
    button0 = device_get_binding(SW0_GPIO_LABEL);
    button1 = device_get_binding(SW1_GPIO_LABEL);
    button2 = device_get_binding(SW2_GPIO_LABEL);



    gpio_pin_configure(button0, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
    gpio_pin_interrupt_configure(button0,SW0_GPIO_PIN,GPIO_INT_EDGE_TO_ACTIVE);
    
    gpio_add_callback(button0, &button_cb_data);

    gpio_pin_configure(button1, SW1_GPIO_PIN, SW1_GPIO_FLAGS);
    gpio_pin_interrupt_configure(button1,SW1_GPIO_PIN,GPIO_INT_EDGE_TO_INACTIVE);

    gpio_add_callback(button1, &button_cb_data);

    gpio_pin_configure(button2, SW2_GPIO_PIN, SW2_GPIO_FLAGS);
    gpio_pin_interrupt_configure(button2,SW2_GPIO_PIN,GPIO_INT_EDGE_TO_INACTIVE);

    gpio_add_callback(button2, &button_cb_data);



    gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN)|BIT(SW1_GPIO_PIN)|BIT(SW2_GPIO_PIN));


    




    #ifdef KEY_INPUT_SUPPOR











    #endif


    const struct device* display_dev=device_get_binding(DISPLAY_DEV_NAME);
    st7735_LcdInit(display_dev);
    while(1)
    {
        // gpio_pin_set(dev, PIN, 1);
        // k_msleep(20);
        // gpio_pin_set(dev, PIN, 0);
        // k_msleep(20);
    }
}