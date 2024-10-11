/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_1), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_2), okay)
#define I2C_DEV_NODE	DT_ALIAS(i2c_2)
#else
#error "Please set the correct I2C device"
#endif

#define MAX_I2C_BUFFER_SIZE 16
#define I2C_TARGET_FLAGS_ERROR_TIMEOUT	BIT(10)

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

uint8_t txBuffer[MAX_I2C_BUFFER_SIZE] = {0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
						0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};
uint32_t txBufIndex = 0;

uint8_t rxBuffer[MAX_I2C_BUFFER_SIZE];
uint32_t rxBufIndex = 0;

struct k_sem my_sem;

int timeoutFlag = 0;

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);

/* This callback is overloaded to be notified of a timeout if it occurs
 * on the device due to hanging */
static int i2c_stop_callback(struct i2c_target_config * cfg){
	/* used to control test flow and notify thread that transaction has
	 * completed */
	if(cfg->flags & I2C_TARGET_FLAGS_ERROR_TIMEOUT){
		timeoutFlag = 1;
	}
	k_sem_give(&my_sem);
	return 0;
}

static int i2c_read_requested(struct i2c_target_config * cfg, uint8_t *initVal){
	if(txBufIndex < MAX_I2C_BUFFER_SIZE){
		*initVal = txBuffer[txBufIndex];
		txBufIndex++;
		return 0;
	} else {
		return -1;
	}
}

static int i2c_read_processed(struct i2c_target_config *cfg, uint8_t *nextByte){
	if(txBufIndex < MAX_I2C_BUFFER_SIZE) {
		*nextByte = txBuffer[txBufIndex];
		txBufIndex++;
		return 0;
	} else {
		return -1;
	}
}

static int i2c_write_requested(struct i2c_target_config *cfg){
	if(rxBufIndex < MAX_I2C_BUFFER_SIZE) {
		return 0;
	} else {
		return -1;
	}
}

static int i2c_write_received(struct i2c_target_config *cfg, uint8_t nextByte){
	if(rxBufIndex < MAX_I2C_BUFFER_SIZE){
		rxBuffer[rxBufIndex] = nextByte;
		rxBufIndex++;
		return 0;
	} else {
		return -1;
	}
}

struct i2c_target_callbacks i2c_callbacks_data = {
	.write_requested = i2c_write_requested, /* callback for target to begin recieving */
	.read_requested = i2c_read_requested,  	/* callback for target to begin recieving */
	.write_received = i2c_write_received,   /* callback for target to receive */
	.read_processed = i2c_read_processed,   /* callback for target to transmit */
	.stop = i2c_stop_callback, 			    /* callback to mark the end of a transaction */
};

struct i2c_target_config i2c_target_cfg_data = {
	.flags = 0x00, /* doesn't support 10-bit addressing. */
	.address = 0x48,
	.callbacks = &i2c_callbacks_data,
};

int main(void)
{
	uint32_t i2c_cfg_tmp;
	k_sem_init(&my_sem, 0, 1);

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	if (!device_is_ready(i2c_dev)) {
		printf("I2C device is not ready\n");
		return -EBUSY;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		printf("I2C config failed\n");
		return -1;
	}

	/* 2. Verify i2c_get_config() */
	if (i2c_get_config(i2c_dev, &i2c_cfg_tmp)) {
		printf("I2C get_config failed\n");
		return -1;
	}

	if (i2c_cfg != i2c_cfg_tmp) {
		printf("I2C get_config returned invalid config\n");
		return -1;
	}

	/* 3. verify i2c_write() */
	if (i2c_target_register(i2c_dev, &i2c_target_cfg_data)) {
		printf("Fail to convert to target\n");
		return -1;
	}
	k_sleep(K_MSEC(1));

	int j = 0;

	while(1)
	{
		j++;
		if(j == 5){
			printf("should hang on next master read\n");
			DL_I2C_disableInterrupt((I2C_Regs*)0x400F2000, DL_I2C_INTERRUPT_TARGET_TXFIFO_EMPTY);
		}
		printf("Waiting to recieve/transmit data from/to master.\n");

		k_sem_take(&my_sem, K_FOREVER);

		printf("Transaction completed.\n");

		if(timeoutFlag != 0){
			printf("Timeout Occurred During Sleep!\n");
			gpio_pin_set_dt(&led, 1);
			timeoutFlag = 0;
			return -ETIMEDOUT;
		}

		/* print and reset buffers for the next transaction */
		if(txBufIndex > 0){
			printf("Device transmitted: ");
			for(int j = 0; j < txBufIndex; j++){
				printf("%d, ", txBuffer[j]);
			}
			printf("during transmission.\n");
			txBufIndex = 0;
		}
		if(rxBufIndex > 0){
			printf("Device received: ");
			for(int j = 0; j < rxBufIndex; j++){
				printf("%d, ", rxBuffer[j]);
			}
			printf("during transmission.\n");
			rxBufIndex = 0;
		}

	}
	return 0;
}
