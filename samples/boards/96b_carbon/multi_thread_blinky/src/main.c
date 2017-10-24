#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <misc/printk.h>
#include <shell/shell.h>

/* size of stack area used by each thread */
#define STACKSIZE 128

/* scheduling priority used by each thread */
#define PRIORITY 7

#define MY_SHELL_MODULE "sample_module"

/*-----shell code fom <zephyr_base>/samples/subsys/shell/shell/------*/
static int shell_cmd_ping(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printk("pong\n");

	return 0;
}

static int shell_cmd_params(int argc, char *argv[])
{
	int cnt;

	printk("argc = %d\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		printk("  argv[%d] = %s\n", cnt, argv[cnt]);
	}
	return 0;
}

static struct shell_cmd commands[] = {
	{ "ping", shell_cmd_ping, NULL },
	{ "params", shell_cmd_params, "print argc" },
	{ NULL, NULL, NULL }
};

void main(void)
{
	SHELL_REGISTER(MY_SHELL_MODULE, commands);
}
/*-------------------------------------------------------------------*/


void blink1(void)
{
	int cnt = 0;
	struct device *gpioa;

	gpioa = device_get_binding("GPIOA");
	gpio_pin_configure(gpioa, 15, GPIO_DIR_OUT);
	while (1) {
		gpio_pin_write(gpioa, 15, (cnt + 1) % 2);
		k_sleep(100);
		cnt++;
	}
}

void blink2(void)
{
	int cnt = 0;
	struct device *gpiod;

	gpiod = device_get_binding("GPIOD");
	gpio_pin_configure(gpiod, 2, GPIO_DIR_OUT);
	while (1) {
		gpio_pin_write(gpiod, 2, cnt % 2);
		k_sleep(1000);
		cnt++;
	}
}

void blink3(void)
{
	int cnt = 0, cnt2 = 0, sleep = 100;
	struct device *gpiob;

	gpiob = device_get_binding("GPIOB");
	gpio_pin_configure(gpiob, 5, GPIO_DIR_OUT);
	while (1) {
		while (cnt2 != 5) {
			gpio_pin_write(gpiob, 5, cnt2 % 2);
			k_sleep(sleep);
			sleep += 100;
			cnt2++;
		}
		cnt2 = 0;
		sleep = 100;
	}
}

K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(blink2_id, STACKSIZE, blink2, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(blink3_id, STACKSIZE, blink3, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);
