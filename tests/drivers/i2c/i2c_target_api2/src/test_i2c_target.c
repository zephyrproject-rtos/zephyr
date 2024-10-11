/*
 * Copyright (c) 2023 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_i2c_basic
 * @{
 * @defgroup t_i2c_target test_i2c_target_api2
 * @brief TestPurpose: verify I2C target can be registered and
 *  	  function correctly. (requires an external controller)
 * @}
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

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

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

uint8_t txBuffer[MAX_I2C_BUFFER_SIZE] = {0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
						0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8};
uint32_t txBufIndex = 0;

uint8_t rxBuffer[MAX_I2C_BUFFER_SIZE];
uint32_t rxBufIndex = 0;

struct k_sem my_sem;

static int i2c_stop_callback(struct i2c_target_config * cfg){
	/* used to control test flow and notify thread that transaction has
	 * completed */
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

static int test_i2c_data(void)
{
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	uint32_t i2c_cfg_tmp;

	k_sem_init(&my_sem, 0, 1);

	if (!device_is_ready(i2c_dev)) {
		TC_PRINT("I2C device is not ready\n");
		return TC_FAIL;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* 2. Verify i2c_get_config() */
	if (i2c_get_config(i2c_dev, &i2c_cfg_tmp)) {
		TC_PRINT("I2C get_config failed\n");
		return TC_FAIL;
	}

	if (i2c_cfg != i2c_cfg_tmp) {
		TC_PRINT("I2C get_config returned invalid config\n");
		return TC_FAIL;
	}

	/* 3. verify i2c_write() */
	if (i2c_target_register(i2c_dev, &i2c_target_cfg_data)) {
		TC_PRINT("Fail to convert to target\n");
		return TC_FAIL;
	}
	k_sleep(K_MSEC(1));

	for(int j = 4; j > 0; j--)
	{
		TC_PRINT("Waiting to recieve data from master. Transactions remaining: %d\n", j);

		k_sem_take(&my_sem, K_FOREVER);

		TC_PRINT("Master data received.\n");
		/* print and reset buffers for the next transaction */
		if(txBufIndex > 0){
			TC_PRINT("Device transmitted: ");
			for(int j = 0; j < txBufIndex; j++){
				TC_PRINT("%d, ", txBuffer[j]);
			}
			TC_PRINT("during transmission.\n");
			txBufIndex = 0;
		}
		if(rxBufIndex > 0){
			TC_PRINT("Device received: ");
			for(int j = 0; j < rxBufIndex; j++){
				TC_PRINT("%d, ", rxBuffer[j]);
			}
			TC_PRINT("during transmission.\n");
			rxBufIndex = 0;
		}
	}

	if(i2c_target_unregister(i2c_dev, &i2c_target_cfg_data)){
		TC_PRINT("Fail to unregister\n");
		return TC_FAIL;
	}

	return TC_PASS;
}


static int test_i2c_errors(void) {
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);

	k_sem_init(&my_sem, 0, 1);

	if (!device_is_ready(i2c_dev)) {
		TC_PRINT("I2C device is not ready\n");
		return TC_FAIL;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* Test common errors in the configuration of the I2C peripheral */

	if (i2c_target_register(i2c_dev, &i2c_target_cfg_data)) {
		TC_PRINT("Fail to convert to target\n");
		return TC_FAIL;
	}
	/* attempting to re-register as a target. This is allowed.  */
	if (i2c_target_register(i2c_dev, &i2c_target_cfg_data)) {
		TC_PRINT("Fail to convert to target\n");
		return TC_FAIL;
	}

	uint8_t test_array[] = {0x81, 0x82, 0x83, 0x84};
	/* starting a controller transfer as a target. Should fail */
	if (i2c_burst_write(i2c_dev, 0x1E, 0x00, test_array, 4) != -EBUSY) {
		TC_PRINT("Fail to fail\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

ZTEST(I2C_DEV_NODE, test_i2c_data)
{
	zassert_true(test_i2c_data() == TC_PASS);
}

ZTEST(I2C_DEV_NODE, test_i2c_errors)
{
	zassert_true(test_i2c_errors() == TC_PASS);
}

ZTEST_SUITE(I2C_DEV_NODE, NULL, NULL, NULL, NULL, NULL);
