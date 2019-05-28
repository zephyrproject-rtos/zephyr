#include <device.h>
#include <i2c.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "icm20948.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(ICM20948);


int icm20948_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
    struct icm20948_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

    gpio_pin_disable_callback(drv_data->gpio, DT_TDK_ICM20948_0_IRQ_GPIOS_PIN);

    drv_data->data_ready_handler = handler;
    if(handler == NULL){
        return 0;
    }

    drv_data->data_ready_trigger = *trig;


	gpio_pin_enable_callback(drv_data->gpio, DT_TDK_ICM20948_0_IRQ_GPIOS_PIN);
    
    return 0;
}

static void icm20948_gpio_callback(struct device *dev, struct gipio_callback * cb, u32_t pins){
    struct icm20948_data *drv_data = CONTAINER_OF(cb, struct icm20948_data, gpio_cb);

    ARG_UNUSED(pins);

    gpio_pin_disable_callback(dev, DT_TDK_ICM20948_0_IRQ_GPIOS_PIN);

#if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}


#ifdef CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD
static void lsm6dsl_work_cb(struct k_work *work)
{
	struct icm20948_data *drv_data =
		CONTAINER_OF(work, struct icm20948_data, work);

	lsm6dsl_thread_cb(drv_data->dev);
}
#endif


int icm20948_init_interrupt(struct device *dev) {
    struct icm20948_data  *drv_data = dev->driver_data;

    /* setup data ready gpio interrupt */
    drv_data->gpio = device_get_binding(DT_TDK_ICM20948_0_IRQ_GPIOS_PIN);
    if(drv_data->gpio == NULL) {
        LOG_ERR("Cannot get pointer to %s device.",
			    DT_TDK_ICM20948_0_IRQ_GPIOS_PIN);
		return -EINVAL;
    }

    gpio_pin_configure(drv_data->gpio, DT_TDK_ICM20948_0_IRQ_GPIOS_PIN, GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

    gpio_init_callback(&drv_data->gpio_cb, icm20948_gpio_callback, BIT(DT_TDK_ICM20948_0_IRQ_GPIOS_PIN));

    if(gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0){
        LOG_ERR("Could not set gpio callback.");
		return -EIO;
    }

    /* enable data-ready interrupt */
	if(drv_data->hw_tf->write_reg(drv_data, ICM20948_REG_INT_ENABLE,
        ICM20948_ENABLE_FSYNC 
        
    );

    #if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	    k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	    k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ICM20948_THREAD_STACK_SIZE,
			(k_thread_entry_t)icm20948_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_ICM20948_THREAD_PRIORITY),
			0, 0);
    #elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	    drv_data->work.handler = icm20948_work_cb;
	    drv_data->dev = dev;
    #endif

    gpio_pin_enable_callback(drv_data->gpio, DT_ST_LSM6DSL_0_IRQ_GPIOS_PIN);

	return 0;

}