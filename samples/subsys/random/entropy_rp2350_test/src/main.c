#include <stdio.h>
#include <zephyr/random/random.h>

int main(void)
{
	printf("Random: 0x%08x\n", sys_rand32_get());
	return 0;
}
