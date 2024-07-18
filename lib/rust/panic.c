#include <zephyr/kernel.h>

void k_panic__extern()
{
    k_panic();
}
