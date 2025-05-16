/*!
 * SPDX-License-Identifier: Apache 2.0
 * Copyright (c) 2025 TQ-Systems GmbH <oss@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Isaac L. L. Yuki
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "PF5020.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/platform/hooks.h>
#include "fsl_clock.h"
#include <zephyr/drivers/pwm.h>

#if (defined(DISPLAY_TM070) && (DISPLAY_TM070 == 1))
#include "tm070.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Check if devicetree node identifier is defined */
#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_peripheral_6), okay)
#define I2C_NODE DT_ALIAS(i2c_peripheral_6)
#else
#error "I2C isn't set."
#endif

#define PF5020_ADDRESS 0x08

#if (defined(DISPLAY_TM070) && (DISPLAY_TM070 == 1))
#define PWM_NODE    DT_NODELABEL(flexpwm2_pwm2)
#define PWM_CHANNEL 0
#define PWM_PERIOD  PWM_MSEC(20)
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

static const struct device *i2c_dev;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static uint32_t PF5020_transfer(const void *peripheral, uint8_t regAddress, size_t regAddressSize,
				uint8_t *buffer, uint8_t dataSize,
				PF5020_TransferDirection_t transferDirection);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * \brief  This is the main application entry point where system is
 * initialized and all system components are started.
 */
void soc_late_init_hook()
{
	float voltage;

	uint32_t sysClock = CLOCK_GetRootClockFreq(kCLOCK_Root_M7);
	printk("\r\nCore Clock Frequency: %u Hz\r\n", sysClock);
	sysClock = CLOCK_GetRootClockFreq(kCLOCK_Root_Semc);
	printk("SEMC Clock Frequency: %u Hz\r\n", sysClock);

	i2c_dev = DEVICE_DT_GET(I2C_NODE);

	PF5020_Handle_t PMIC = {.transfer = PF5020_transfer, .peripheral = i2c_dev};

	PF5020_setCoreVoltage(PMIC_VDD_SOC_1V100, &PMIC);

	PF5020_readCoreVoltage(&voltage, &PMIC);
	printk("PMIC set to: %fV\r\n", (double)voltage);
}

void board_late_init_hook()
{
#if (defined(DISPLAY_TM070) && (DISPLAY_TM070 == 1))

	const struct device *pwm_dev = DEVICE_DT_GET(PWM_NODE);
	if (!pwm_dev) {
		printk("Error: PWM device not found.\n");
		return;
	}
	int ret = pwm_set(pwm_dev, PWM_CHANNEL, PWM_PERIOD, PWM_PERIOD, PWM_POLARITY_NORMAL);
	if (ret) {
		printk("Error %d: failed to set PWM\n", ret);
	}

	BOARD_PrepareDisplayController();
	printk("Initialized LVDS bridge\r\n");

#endif
}

/*!
 * \brief This is the transfer function that is used by the driver.
 * \note Parameters are described in the drivers function.
 */
static uint32_t PF5020_transfer(const void *peripheral, uint8_t regAddress, size_t regAddressSize,
				uint8_t *buffer, uint8_t dataSize,
				PF5020_TransferDirection_t transferDirection)
{
	status_t status = kStatus_NoTransferInProgress;
	struct i2c_msg msg;
	switch (transferDirection) {
	case PF5020_READ:
		msg.flags = I2C_MSG_WRITE;
		msg.buf = &regAddress;
		msg.len = regAddressSize;
		status = i2c_transfer(peripheral, &msg, 1, PF5020_ADDRESS);
		msg.buf = buffer;
		msg.len = dataSize;
		msg.flags = I2C_MSG_READ | I2C_MSG_STOP;
		status = i2c_transfer(peripheral, &msg, 1, PF5020_ADDRESS);
		break;
	case PF5020_WRITE:
		status = i2c_burst_write(peripheral, PF5020_ADDRESS, regAddress, buffer, dataSize);
		break;
	default:
		status = 1;
	}
	return (uint32_t)status;
}
