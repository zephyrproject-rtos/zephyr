#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display_tm1637.h>



#define TM1637_NODE DT_NODELABEL(tm1637)

void main(void)
{
	const struct device *tm1637 = DEVICE_DT_GET(TM1637_NODE);

	if (!device_is_ready(tm1637)) {
		printk("TM1637 device not ready\n");
		return;
	}

	
		
	
	      
	      tm1637_display_raw_segments(tm1637,1,0x3f);
	     //tm1637_display_number(tm1637,8000,false);
	     
	      
		
	
}

