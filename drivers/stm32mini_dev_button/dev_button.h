
#ifndef __DEV_BUTTON_H__
#define __DEV_BUTTON_H__

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

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



#endif 