#include <zephyr.h>
#include <misc/printk.h>
#include <console.h>

void main(void)
{
	console_getchar_init();

	while (1) {
		uint8_t c = console_getchar();

		printk("char: [0x%x] %c\n", c, c);
	}
}
