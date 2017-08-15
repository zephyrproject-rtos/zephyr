#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <misc/printk.h>

/* define LED and Button pin */
#define LED_PORT LED0_GPIO_PORT
#define LED_PIN LED0_GPIO_PIN
#define LED_FLASH_TIME 100//ms

#define BTN_PORT SW0_GPIO_NAME
#define BTN_PIN SW0_GPIO_PIN

/* define semaphore */
K_SEM_DEFINE(semPress, 0, 1);
K_SEM_DEFINE(semHold, 0, 1);
K_SEM_DEFINE(semBtnIntr, 0, 1);

/* define global device */
struct device *gDevLed;
struct device *gDevBtn;

static struct gpio_callback gpio_cb;

/* define thread parameter */
#define STACKSIZE 1024
#define PRIORITY_EVENT 7
#define PRIORITY_INTR 6

void ledOnOff(u32_t on)
{
	gpio_pin_write(gDevLed, LED_PIN, !on);
}
void ledFlash(u32_t times)
{
	while(times) {
		ledOnOff(1);
		k_sleep(LED_FLASH_TIME);
		ledOnOff(0);
 
 		if(--times){
 			k_sleep(LED_FLASH_TIME);
		}
 	}
}
u32_t isKeyDown(void)
{
 	u32_t val;
 	gpio_pin_read(gDevBtn, BTN_PIN, &val);
 	return !val;
}

void threadPress(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	while(1) {
 		k_sem_take(&semPress, K_FOREVER);
 		printk("btn pressed\n");
 		ledFlash(1);
 	}
}
void threadHold(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	while(1) {
		k_sem_take(&semHold, K_FOREVER);
		printk("btn hold\n");
		ledFlash(2);
 	}
}
void threadBtnIntr(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);
	u32_t count;
	bool bHold;
	while(1) {
		k_sem_take(&semBtnIntr, K_FOREVER);
		count = 0;
		bHold = false;
 		do {
			k_sleep(50);
			count++;
 			if(count > 6) {
				bHold = true;
				break;
 			}
		}while(isKeyDown());
 
		if(bHold){
			k_sem_give(&semHold);
		}
		else{
			k_sem_give(&semPress);
		}
	}
}


K_THREAD_DEFINE(threadPress_id, STACKSIZE, threadPress, NULL, NULL, NULL, 
		PRIORITY_EVENT, 0, K_NO_WAIT);
K_THREAD_DEFINE(threadHold_id, STACKSIZE, threadHold, NULL, NULL, NULL, 
		PRIORITY_EVENT, 0, K_NO_WAIT);
K_THREAD_DEFINE(threadBtnIntr_id, STACKSIZE, threadBtnIntr, NULL, NULL, NULL, 
		PRIORITY_INTR, 0, K_NO_WAIT);

void btnIntrCb(struct device *gpio, struct gpio_callback *cb, u32_t pin)
{
	k_sem_give(&semBtnIntr);
}

void helloMsg(void)
{
	printk("\n************************************************\n");
	printk("**\n");
	printk("** Welcome to visit http://blog.csdn.net/veabol\n");
	printk("** Thread program start!\n");
	printk("**\n");
	printk("************************************************\n");
}

void main (void)
{
 
	helloMsg();
 
	printk("configure led\n");
	gDevLed = device_get_binding(LED_PORT);
	if (!gDevLed) {
		printk("error\n");
		return;
 	}
	gpio_pin_configure(gDevLed, LED_PIN, GPIO_DIR_OUT);
	ledOnOff(0);

	printk("configure button\n");
	gDevBtn = device_get_binding(BTN_PORT);
 		
	if (!gDevBtn) {
		printk("error\n");
		return;
 	}
	gpio_pin_configure(gDevBtn, BTN_PIN, 
	GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP);
	gpio_init_callback(&gpio_cb, btnIntrCb, BIT(BTN_PIN));
	gpio_add_callback(gDevBtn, &gpio_cb);
	gpio_pin_enable_callback(gDevBtn, BTN_PIN);
}
