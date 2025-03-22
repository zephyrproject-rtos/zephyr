#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

#include "adsp.h"

static const struct device *mbox = DEVICE_DT_GET(DT_ALIAS(mbox));
static mbox_channel_id_t chmbox = 0;

static uint32_t mboxData = 0x12345678;

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	if(!device_is_ready(mbox))
	{
		return 0;
	}
	mbox_set_enabled(mbox, chmbox, true);

	printk("[CM33] Starting HiFi4 DSP... \n");
	dspStart();
	k_msleep(1000);

	struct mbox_msg msg = {
		.data = &mboxData,
		.size = sizeof(mboxData)
	};
	printk("[CM33] Sending mbox message... ");
	if(mbox_send(mbox, chmbox, &msg) < 0)
	{
		printk("failed!");
	}
	printk("\n");
    
	return 0;
}
