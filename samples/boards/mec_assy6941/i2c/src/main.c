/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2022 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#define I2C_TARGET_NOT_PRESENT_ADDR 0x56

/* LTC2489 ADC conversion time using internal clock.
 * If a converstion is in progress LTC2489 will NACK its address.
 */
#define LTC2489_ADC_CONV_TIME_MS 150
#define LTC2489_ADC_READ_RETRIES 10

static const struct device *i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2c0));

/* Access devices on an I2C bus using Device Tree child nodes of the I2C controller */
static const struct i2c_dt_spec pca9555_dts = I2C_DT_SPEC_GET(DT_NODELABEL(pca9555_evb));
static const struct i2c_dt_spec ltc2489_dts = I2C_DT_SPEC_GET(DT_NODELABEL(ltc2489_evb));
static const struct i2c_dt_spec mb85rc256v_dts = I2C_DT_SPEC_GET(DT_NODELABEL(mb85rc256v));

static volatile uint32_t spin_val;
static volatile int ret_val;

static void spin_on(uint32_t id, int rval);

uint8_t buf1[256];
uint8_t buf2[256];
uint8_t buf3[256];

int main(void)
{
	int ret = 0;
	uint32_t i2c_dev_config = 0;
	uint32_t adc_retry_count = 0;
	uint32_t temp = 0;
	uint32_t fram_nbytes = 0;
	uint16_t fram_mem_addr = 0;
	uint8_t nmsgs = 0;
	uint8_t target_addr = 0;
	struct i2c_msg msgs[4];

	LOG_INF("MEC5 I2C sample: board: %s", DT_N_P_compatible_IDX_0);

	memset(msgs, 0, sizeof(msgs));
	memset(buf1, 0x55, sizeof(buf1));
	memset(buf2, 0x55, sizeof(buf2));

	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C device is not ready! (%d)", -3);
		spin_on(2u, -3);
	}

	i2c_dev_config = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	ret = i2c_configure(i2c_dev, i2c_dev_config);
	if (ret) {
		LOG_ERR("I2C configuration error (%d)", ret);
		spin_on(3u, ret);
	}

	ret = i2c_get_config(i2c_dev, &temp);
	if (ret) {
		LOG_ERR("I2C get configuration error (%d)", ret);
		spin_on(4u, ret);
	}

	if (temp != i2c_dev_config) {
		LOG_ERR("I2C configuration does not match: orig(0x%08x) returned(0x%08x)\n",
			i2c_dev_config, temp);
		spin_on(5u, ret);
	}

	/* I2C write to non-existent device: check error return and if
	 * future I2C accesses to a real device work.
	 */
	LOG_INF("Attempt I2C transaction to a non-existent address");
	target_addr = I2C_TARGET_NOT_PRESENT_ADDR;
	nmsgs = 1u;
	buf1[0] = 2u;
	buf1[1] = 0x55u;
	buf1[2] = 0xaa;
	msgs[0].buf = buf1;
	msgs[0].len = 3u;
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer(i2c_dev, msgs, nmsgs, target_addr);
	if (ret != 0) {
		LOG_INF("PASS: Expected API to return an error (%d)", ret);
	} else {
		LOG_ERR("FAIL: I2C API should have returned an error!");
		spin_on(6u, ret);
	}

	for (int i = 0; i < 3; i++) {
		LOG_INF("I2C Write 3 bytes to PCA9555 target device");

		nmsgs = 1u;
		buf1[0] = 2u; /* PCA9555 cmd=2 is output port 0 */
		buf1[1] = 0x55u;
		buf1[2] = 0xaau;
		msgs[0].buf = buf1;
		msgs[0].len = 3u;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

		ret = i2c_transfer_dt(&pca9555_dts, msgs, nmsgs);
		if (ret) {
			LOG_ERR("Loop %d: I2C write to PCA9555 error (%d)", i, ret);
			spin_on(7u, ret);
		}
	}

	LOG_INF("Write 3 bytes to PCA9555 target using multiple write buffers");
	nmsgs = 3u;
	buf1[0] = 2u;
	buf1[1] = 0x33u;
	buf1[2] = 0xcc;
	msgs[0].buf = buf1;
	msgs[0].len = 1u;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = &buf1[1];
	msgs[1].len = 1u;
	msgs[1].flags = I2C_MSG_WRITE;
	msgs[2].buf = &buf1[2];
	msgs[2].len = 1u;
	msgs[2].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&pca9555_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C write multiple buffers to PCA9555 error (%d)", ret);
		spin_on(7u, ret);
	}

	LOG_INF("Read both PCA9555 input ports");

	/* PCA9555 read protocol: START TX[wrAddr, cmd],
	 * Rpt-START TX[rdAddr],  RX[data,...], STOP
	 */
	nmsgs = 2u;
	buf1[0] = 0u; /* PCA9555 cmd=0 is input port 0 */
	buf2[0] = 0x55u; /* receive buffer */
	buf2[1] = 0x55u;

	msgs[0].buf = buf1;
	msgs[0].len = 1u;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf2;
	msgs[1].len = 2u;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&pca9555_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C transfer error: write cmd byte, read 2 data bytes: (%d)", ret);
		spin_on(8u, ret);
	}

	LOG_INF("PCA9555 Port 0 input = 0x%02x  Port 1 input = 0x%02x", buf2[0], buf2[1]);

	LOG_INF("Read both PCA9555 input ports using different buffers for each port");
	nmsgs = 3u;

	buf1[0] = 0u; /* PCA9555 cmd=0 is input port 0 */
	buf2[0] = 0x55u; /* first receive buffer */
	buf2[1] = 0x55u;
	buf2[8] = 0x55u; /* second receive buffer */
	buf2[9] = 0x55u;

	msgs[0].buf = buf1;
	msgs[0].len = 1u;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf2;
	msgs[1].len = 1u;
	msgs[1].flags = I2C_MSG_READ;

	msgs[2].buf = &buf2[8];
	msgs[2].len = 1u;
	msgs[2].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&pca9555_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C transfer error: 3 messages: (%d)", ret);
		spin_on(9u, ret);
	}

	LOG_INF("PCA9555 Port 0 input = 0x%02x  Port 1 input = 0x%02x", buf2[0], buf2[8]);

	LOG_INF("Select ADC channel 0 in LTC2489. This triggers a conversion!");
	nmsgs = 1;
	buf1[0] = 0xb0u; /* 1011_0000 selects channel 0 as single ended */
	msgs[0].buf = buf1;
	msgs[0].len = 1u;
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&ltc2489_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C write to set LTC2489 channel failed: (%d)", ret);
		spin_on(10u, ret);
	}

	/* wait for LTC2489 to finish a conversion */
	k_sleep(K_MSEC(LTC2489_ADC_CONV_TIME_MS));

	/* LTC2489 ADC read protocol is I2C read: START TX[rdAddr]
	 * RX[data[7:0], data[15:8], data[23:16]] STOP
	 */
	LOG_INF("Read 24-bit ADC reading from LTC2489");
	adc_retry_count = 0;
	do {
		buf2[0] = 0x55;
		buf2[1] = 0x55;
		buf2[2] = 0x55;

		nmsgs = 1;
		msgs[0].buf = buf2;
		msgs[0].len = 3u;
		msgs[0].flags = I2C_MSG_READ | I2C_MSG_STOP;

		ret = i2c_transfer_dt(&ltc2489_dts, msgs, nmsgs);
		if (ret) {
			adc_retry_count++;
			LOG_INF("LTC2489 read error (%d)", ret);
		}
	} while ((ret != 0) && (adc_retry_count < LTC2489_ADC_READ_RETRIES));

	if (ret == 0) {
		temp = ((uint32_t)(buf2[0]) + (((uint32_t)(buf2[1])) << 8)
				+ (((uint32_t)(buf2[2])) << 16));
		LOG_INF("LTC2489 reading = 0x%08x", temp);
	}

	/* I2C FRAM tests */
	fram_nbytes = 64u;
	fram_mem_addr = 0x1234u;

	LOG_INF("MB85RC256V FRAM write %u bytes to offset 0x%x", fram_nbytes, fram_mem_addr);

	for (temp = 0; temp < fram_nbytes; temp++) {
		buf2[temp] = (uint8_t)((temp + 1u) & 0xffu);
	}

	nmsgs = 2;
	buf1[0] = (uint8_t)((fram_mem_addr >> 8) & 0xffu); /* address b[15:8] */
	buf1[1] = (uint8_t)((fram_mem_addr) & 0xffu); /* address b[7:0] */

	msgs[0].buf = buf1;
	msgs[0].len = 2u;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = buf2;
	msgs[1].len = fram_nbytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&mb85rc256v_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C API for FRAM write returned error %d", ret);
		spin_on(13, ret);
	}

	LOG_INF("MB85RC256V FRAM read %u bytes from offset 0x%x", fram_nbytes, fram_mem_addr);
	memset(buf3, 0x55, sizeof(buf3));

	nmsgs = 2;
	buf1[0] = (uint8_t)((fram_mem_addr >> 8) & 0xffu); /* address b[15:8] */
	buf1[1] = (uint8_t)((fram_mem_addr) & 0xffu); /* address b[7:0] */

	msgs[0].buf = buf1;
	msgs[0].len = 2u;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = buf3;
	msgs[1].len = fram_nbytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	ret = i2c_transfer_dt(&mb85rc256v_dts, msgs, nmsgs);
	if (ret) {
		LOG_ERR("I2C API for FRAM read returned error %d", ret);
		spin_on(13, ret);
	}

	ret = memcmp(buf2, buf3, fram_nbytes);
	if (ret == 0) {
		LOG_INF("Compare read of data written to FRAM matches: PASS");
	} else {
		LOG_ERR("Compare read of data written to FRAM has mismatch: FAIL");
	}

	LOG_INF("Application Done (%d)", ret);
	spin_on(256, 0);

	return 0;
}

static void spin_on(uint32_t id, int rval)
{
	spin_val = id;
	ret_val = rval;

	log_panic(); /* flush log buffers */

	while (spin_val) {
		;
	}
}
