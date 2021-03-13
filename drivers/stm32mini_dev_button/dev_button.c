#include "dev_button.h"

static struct gpio_callback button_cb_data;

extern struct k_msgq input_key_message;

struct input_manager_info
{
    uint8_t click_num;
    struct k_delayed_work work_item;
    int64_t report_stamp;
    uint8_t keycode;
    uint8_t keytype;
    uint16_t prev_stable_keycode;
}input_manager;



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
    return KEY_RESERVED;
}

void report_key_event(struct k_work *work)
{
    uint16_t key_value;
    printk("report_key_event keycode:%d key_type:%d\n", input_manager.keycode,input_manager.keytype);

    //超过一定时间应该把clicknum清空
    input_manager.prev_stable_keycode=KEY_RESERVED;//按键释放
    key_value=input_manager.keycode<<8|input_manager.keytype;
    k_msgq_put(&input_key_message, &key_value, K_NO_WAIT);

}

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
    uint16_t keycode=0;

    keycode=pin_to_keycode(pins);

    if(keycode!=input_manager.prev_stable_keycode)
    {
        if(input_manager.prev_stable_keycode!=KEY_RESERVED)
        {
            //连着按了两种以上的键值
            report_key_event(NULL);
        }
        input_manager.prev_stable_keycode=keycode;
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

    k_delayed_work_submit(&input_manager.work_item, K_MSEC(QUICKLY_CLICK_DURATION));
	// printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
}



static int button_init(const struct device *dev)
{
    const struct device *button0;//button0指向的值不可以修改
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
    return 0;
}




DEVICE_DEFINE(button_input_init, "button_driver", button_init, NULL, NULL,
	      NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      NULL);

