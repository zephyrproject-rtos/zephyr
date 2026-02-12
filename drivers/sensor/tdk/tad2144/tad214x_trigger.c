/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "tad214x_drv.h"

LOG_MODULE_DECLARE(TAD214X, CONFIG_SENSOR_LOG_LEVEL);

static void tad214x_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct tad214x_data *drv_data = CONTAINER_OF(cb, struct tad214x_data, gpio_cb);
	const struct tad214x_config *cfg = (const struct tad214x_config *)drv_data->dev->config;

	ARG_UNUSED(pins);
    if (cfg->if_mode == IF_ENC) {
        gpio_pin_interrupt_configure_dt(&cfg->mosi_gpio, GPIO_INT_DISABLE);
        gpio_pin_interrupt_configure_dt(&cfg->miso_gpio, GPIO_INT_DISABLE);
        gpio_pin_interrupt_configure_dt(&cfg->sck_gpio,  GPIO_INT_DISABLE);
    } else {
        gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
    }

#if defined(CONFIG_TAD2144_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_TAD2144_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void tad214x_thread_cb(const struct device *dev)
{
	struct tad214x_data *drv_data = dev->data;
	const struct tad214x_config *cfg = dev->config;
	uint8_t i_status;

	tad214x_mutex_lock(dev);
    if (cfg->if_mode == IF_ENC) {
        gpio_pin_interrupt_configure_dt(&cfg->mosi_gpio, GPIO_INT_DISABLE);
        gpio_pin_interrupt_configure_dt(&cfg->miso_gpio, GPIO_INT_DISABLE);
        gpio_pin_interrupt_configure_dt(&cfg->sck_gpio,  GPIO_INT_DISABLE);        
    }else{
	    gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
    }
    
	if (drv_data->drdy_handler) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}

    if (cfg->if_mode == IF_ENC) {
        gpio_pin_interrupt_configure_dt(&cfg->mosi_gpio, GPIO_INT_EDGE_BOTH);
        gpio_pin_interrupt_configure_dt(&cfg->miso_gpio, GPIO_INT_EDGE_BOTH);
        gpio_pin_interrupt_configure_dt(&cfg->sck_gpio,  GPIO_INT_EDGE_BOTH);        
    }else{
	    gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_LEVEL_LOW);
    }
    
	tad214x_mutex_unlock(dev);
}

#if defined(CONFIG_TAD2144_TRIGGER_OWN_THREAD)
static void tad214x_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct device *dev = (struct device *)p1;
	struct tad214x_data *drv_data = (struct tad214x_data *)dev->data;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		tad214x_thread_cb(dev);
	}
}
#elif defined(CONFIG_TAD2144_TRIGGER_GLOBAL_THREAD)
static void tad214x_work_handler(struct k_work *work)
{
	struct tad214x_data *data = CONTAINER_OF(work, struct tad214x_data, work);

	tad214x_thread_cb(data->dev);
}
#endif

int tad214x_trigger_init(const struct device *dev)
{
	struct tad214x_data *drv_data = (struct tad214x_data *)dev->data;
	const struct tad214x_config *cfg = (struct tad214x_config *)dev->config;
	int result = 0;
    
    if(cfg->if_mode == IF_ENC) {
        const struct gpio_dt_spec *gpios[] = {&cfg->mosi_gpio, &cfg->miso_gpio, &cfg->sck_gpio};
        uint32_t enc_pin_mask  = 0;

        for (int i = 0; i < 3; i++) {
            if (!gpio_is_ready_dt(gpios[i])) return -ENODEV;

            gpio_pin_configure_dt(gpios[i], GPIO_INPUT);
            enc_pin_mask |= BIT(gpios[i]->pin);
        }

        gpio_init_callback(&drv_data->gpio_cb, tad214x_gpio_callback, enc_pin_mask);
        result = gpio_add_callback(cfg->mosi_gpio.port, &drv_data->gpio_cb);
        
    } else {
    	if (!cfg->gpio_int.port) {
    		LOG_ERR("trigger enabled but no interrupt gpio supplied");
    		return -ENODEV;
    	}

    	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
    		LOG_ERR("gpio_int gpio not ready");
    		return -ENODEV;
    	}

    	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
    	gpio_init_callback(&drv_data->gpio_cb, tad214x_gpio_callback, BIT(cfg->gpio_int.pin));
    	result = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);
    }

    drv_data->dev = dev;
    
	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	k_mutex_init(&drv_data->mutex);

#if defined(CONFIG_TAD2144_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_TAD2144_THREAD_STACK_SIZE, tad214x_thread, (struct device *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_TAD2144_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&drv_data->thread, "tad214x");
#elif defined(CONFIG_TAD2144_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = tad214x_work_handler;
#endif

	return 0;
}

static void tad214x_handle_enc(const struct device *dev, const struct sensor_trigger *trig)
{
	struct tad214x_data *drv_data = dev->data;
	const struct tad214x_config *cfg = dev->config;

	if (trig->type == SENSOR_TRIG_DATA_READY) {
        static volatile uint8_t last_encoder_state = 0;
        
        int val_a = gpio_pin_get_dt(&cfg->mosi_gpio); // encA
        int val_b = gpio_pin_get_dt(&cfg->miso_gpio); // encB
        int val_z = gpio_pin_get_dt(&cfg->sck_gpio);  // encZ

        uint8_t current_state = (val_a << 1) | val_b;
        // Check for index pulse (Z channel)
        if (val_z)
          drv_data->encoder_position = 0; // Reset position on index pulse
        else if (current_state != last_encoder_state) {
          if (((last_encoder_state == 0x00) && (current_state == 0x02)) ||
              ((last_encoder_state == 0x01) && (current_state == 0x00)) ||
              ((last_encoder_state == 0x02) && (current_state == 0x03)) ||
              ((last_encoder_state == 0x03) && (current_state == 0x01))) {
            drv_data->encoder_position++;
          } else {
            drv_data->encoder_position--;
          }
        }
        
        drv_data->encoder_position = drv_data->encoder_position%16384;
        last_encoder_state = current_state;
	}
}


int tad214x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	int rc = 0;
	struct tad214x_data *drv_data = dev->data;
	const struct tad214x_config *cfg = dev->config;

	tad214x_mutex_lock(dev);
    
    if(trig->type == SENSOR_TRIG_DATA_READY){
        if(cfg->if_mode == IF_ENC) {
            gpio_pin_interrupt_configure_dt(&cfg->mosi_gpio, GPIO_INT_DISABLE);
            gpio_pin_interrupt_configure_dt(&cfg->miso_gpio, GPIO_INT_DISABLE);
            gpio_pin_interrupt_configure_dt(&cfg->sck_gpio,  GPIO_INT_DISABLE);
            
            drv_data->drdy_handler = tad214x_handle_enc;
            drv_data->drdy_trigger = trig;

            gpio_pin_interrupt_configure_dt(&cfg->mosi_gpio, GPIO_INT_EDGE_BOTH);
            gpio_pin_interrupt_configure_dt(&cfg->miso_gpio, GPIO_INT_EDGE_BOTH);
            gpio_pin_interrupt_configure_dt(&cfg->sck_gpio,  GPIO_INT_EDGE_BOTH);            
        } else {
            if (handler == NULL) {
                tad214x_mutex_unlock(dev);
        		return -1;
        	}
            
        	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
       		drv_data->drdy_handler = handler;
       		drv_data->drdy_trigger = trig;
        	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_LEVEL_LOW);
        }
    }else {
        rc = -1;
    }
    
	tad214x_mutex_unlock(dev);

	return rc;
}
