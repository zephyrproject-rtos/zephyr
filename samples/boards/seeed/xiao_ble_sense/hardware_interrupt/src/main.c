#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#include <zephyr/../../drivers/sensor/st/lsm6dsl/lsm6dsl.h>
#include "lsm6ds3.h"


#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;

static struct gpio_callback tap_cb;
static uint8_t interrupt_count = 0;

/*LED Declaration*/
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

/* initialize LSM6DS3 */
lsm6ds3_data_t lsm_data;

/* initialize dev for writing/reading to registers */
static const struct device *lsm6ds3_dev =
	DEVICE_DT_GET_OR_NULL(DT_ALIAS(accel0));

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Double Tap detected at %" PRIu32 "\n", k_cycle_get_32());
}

void setupDoubleTapInterrupt(){

    struct lsm6dsl_data *lsm_data = lsm6ds3_dev->data;
    /*
     * For more detailed descriptions of each register please refer to the LSM6DSL
     * datasheet found at http://www.st.com/resource/en/datasheet/lsm6dsl.pdf
     */

    /* CTRL1_XL(10h): Linear acceleration sensor control register, sets ODR */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x10, 0xFF, 0x60);

    /* TAP_CFG(58h): Enables interrupts and inactivity functions, configuration of filtering and tap recognition functions */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x58, 0xFF, 0x8E);

    /* TAP_THS_6D(59h): Portrait/landscape position and tap function threshold register */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x59, 0xFF, 0x85);

    /* INT_DUR2(5Ah): Tap recognition function setting, set "Duration", "Quiet" and "Shock." */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5A, 0xFF, 0x7F);

    /* WAKE_UP_THS(5Bh): Single/double-tap event enable, leaves threshold for wakeup at default */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5B, 0xFF, 0x80);

    /* MD1_CFG(5Eh): Enables INT1_DOUBLE_TAP, routing of 6D event on INT1 pin */
    lsm_data->hw_tf->update_reg(lsm6ds3_dev, 0x5E, 0xFF, 0x08);

}

int main(void){

    int ret;

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return 0;
	}

    setupDoubleTapInterrupt();

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	if (led0.port) {
		ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led0.port->name, led0.pin);
		} else {
			printk("Set up LED at %s pin %d\n", led0.port->name, led0.pin);
		}
	}

    printk("Double tap to interrupt\n");
	if (led0.port) {
		while (1) {
			/* If we have an LED, match its state to the button's. */
			int val = gpio_pin_get_dt(&button);

			if (val >= 0) {
				gpio_pin_set_dt(&led0, val);
			}
			k_msleep(1);
		}
	}
	return 0;

}