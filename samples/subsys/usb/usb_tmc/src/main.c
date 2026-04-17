#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/* Device instance */
USBD_DEVICE_DEFINE(tmc_dev,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x1234, 0x5678);

/* Configuration */
USBD_CONFIGURATION_DEFINE(tmc_fs_config, 0, 50, NULL);

int main(void)
{
	int ret;

	/* Add configuration */
	ret = usbd_add_configuration(&tmc_dev,
				     USBD_SPEED_FS,
				     &tmc_fs_config);
	if (ret) {
		printk("Failed to add configuration: %d\n", ret);
		return ret;
	}

	/* Register your TMC class instance */
	ret = usbd_register_class(&tmc_dev,
				  "tmc_0",   /* must match USBD_DEFINE_CLASS */
				  USBD_SPEED_FS,
				  1);        /* configuration index */
	if (ret) {
		printk("Failed to register class: %d\n", ret);
		return ret;
	}

	/* Optional: device class codes (keep 0 for now) */
	usbd_device_set_code_triple(&tmc_dev,
				    USBD_SPEED_FS,
				    0, 0, 0);

	/* Initialize USB stack */
	ret = usbd_init(&tmc_dev);
	if (ret) {
		printk("USB init failed: %d\n", ret);
		return ret;
	}

	/* Enable USB */
	ret = usbd_enable(&tmc_dev);
	if (ret) {
		printk("USB enable failed: %d\n", ret);
		return ret;
	}

	printk("USBTMC device started\n");

	while (1) {
		k_sleep(K_FOREVER);
	}
}