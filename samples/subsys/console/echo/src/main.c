#include <zephyr.h>
#include <console.h>

void main(void)
{
	console_init();

	while (1) {
		u8_t c = console_getchar();

		console_putchar(c);
		if (c == '\r') {
			console_putchar('\n');
		}
	}
}
