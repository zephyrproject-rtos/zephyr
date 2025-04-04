/**
 * Project                           : Smartwatch
 * Name of the file                  : spo2_polling.cpp
 * Brief Description of file         : This is a Baremetal SpO2 sensor (using polling) Driver file for Mindgrove Silicon.
 * Name of Author                    : Harini Sree.S
 * Email ID                          : harini@mindgrovetech.in
 *
 * @file spo2_polling.cpp
 * @author Harini Sree. S (harini@mindgrovetech.in)
 * @brief This is a Baremetal SpO2 sensor Driver file for Mindgrove Silicon.This file contains the code to print the temperature using the SpO2 module.The module compatible for this code is MAX30100.
 * @date 2024-01-29
 *
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 *
 */

#include "spo2.h"

#define REPORTING_PERIOD_MS     1000
#define DELAY_FREQ_BASE 		40000000

/** 
 * @fn spo2_write
 * 
 * @brief Writes data to a register of the MAX30100 sensor.
 * 
 * @details This function writes data to a register of the MAX30100 sensor via I2C communication.
 * 
 * @param i2c_number The I2C number used for communication
 * @param slave_address The address of the module MAX30100
 * @param reg_addr The address of the register to read.
 * @param data The data to write to the register.
 */
void spo2_write(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr, uint8_t data)
{
	// printf("\nBegin");
	uint8_t msg_arr[2];
	struct i2c_msg msg_obj;
	msg_arr[0] = reg_addr;
	msg_arr[1] = data;
	msg_obj.buf = msg_arr;
	msg_obj.len = 2;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
}

/** 
 * @fn spo2_read
 * 
 * @brief Reads a register from the MAX30100 sensor.
 * 
 * @details This function reads a register from the MAX30100 sensor via I2C communication.
 * 
 * @param i2c_number The I2C number used for communication
 * @param slave_address The address of the module MAX30100
 * @param reg_addr The address of the register to read.
 * 
 * @return uint8_t The value read from the specified register.
 */
uint8_t spo2_read(const struct device *dev, uint8_t i2c_number, uint8_t slave_address, uint8_t reg_addr)
{
	uint8_t msg_arr[1];
	struct i2c_msg msg_obj;

	msg_arr[0] = reg_addr;
	msg_obj.buf = msg_arr;
	msg_obj.len = 1;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);

	msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	return msg_arr[0];
}

/** 
 * @fn getPartId
 * 
 * @brief Retrieves the part ID of the MAX30100 sensor.
 * 
 * @details This function retrieves the part ID of the MAX30100 sensor by reading 
 * the corresponding register via I2C communication.
 * 
 * @return uint8_t The part ID of the MAX30100 sensor.
 */
uint8_t getPartId(const struct device *dev) {
    // Read the part ID register and return the value
    return spo2_read(dev, I2C0, 0X57, 0xff);
}

/** 
 * @fn begin
 * 
 * @brief Initializes the MAX30100 sensor.
 * 
 * @details This function initializes the MAX30100 sensor by performing the following steps:
 * - Initializes the I2C communication.
 * - Sets the I2C clock speed.
 * - Verifies the part ID of the sensor.
 * - Sets the default mode of operation.
 * - Sets the default pulse width for LEDs.
 * - Sets the default sampling rate.
 * - Sets the default current for IR and red LEDs.
 * - Enables high-resolution mode.
 * 
 * @return bool True if the initialization is successful, false otherwise.
 */
bool hrm_begin(const struct device *dev)
{
	// Verify part ID
	if (getPartId(dev) != EXPECTED_PART_ID) 
	{
		return false;
	}

	// Set Mode
	uint8_t modeConfig = MAX30100_MC_TEMP_EN | DEFAULT_MODE;	// To initialise temperature detection
	// uint8_t modeConfig = DEFAULT_MODE;	// Default mode
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION, modeConfig);

	// uint8_t x = spo2_read(dev, I2C0, 0X57, MAX30100_REG_MODE_CONFIGURATION);

	// Set LedsPulseWidth
	uint8_t previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xfc) | DEFAULT_PULSE_WIDTH);

	// Set SamplingRate
	previous = spo2_read(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION);
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, (previous & 0xe3) | (DEFAULT_SAMPLING_RATE << 2));

	// Set LedsCurrent
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_LED_CONFIGURATION, DEFAULT_RED_LED_CURRENT << 4 | DEFAULT_RED_LED_CURRENT);

	// Set HighresMode
	spo2_write(dev, I2C0, 0X57, MAX30100_REG_SPO2_CONFIGURATION, previous | MAX30100_SPC_SPO2_HI_RES_EN);

	return true;
}

/** 
 * @fn begin
 * 
 * @brief Initializes the PulseOximeter object and the underlying hardware.
 * 
 * @details This function initializes the PulseOximeter object and the underlying
 * hardware components.
 * 
 * @param debuggingMode_ The debugging mode to be set for the PulseOximeter object.
 * 
 * @return bool True if initialization is successful, otherwise false.
 */
bool begin(const struct device *dev)
{
	// debuggingMode = debuggingMode_;

	bool ready = hrm_begin(dev);
	
	spo2_write(dev, I2C0, 0x57, MAX30100_REG_MODE_CONFIGURATION, MAX30100_MODE_SPO2_HR);

	return true;
}

/** 
 * @fn retrieveTemperature
 * 
 * @brief Retrieves the temperature reading from the MAX30100 sensor.
 * 
 * @details @details This function retrieves the temperature reading from the MAX30100 sensor
 * by reading the temperature data registers and combining the integer and 
 * fractional parts to calculate the temperature in degrees Celsius.
 * 
 * @return float The temperature reading in degrees Celsius.
 */
float retrieveTemperature(const struct device *dev) 
{
    // Read the integer part of the temperature
    int8_t tempInteger = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_INT);
    // Read the fractional part of the temperature
    float tempFrac = spo2_read(dev, I2C0, 0X57, MAX30100_REG_TEMPERATURE_DATA_FRAC);

    // Calculate the temperature in degrees Celsius and return it
    return tempFrac * 0.0625 + tempInteger;
}

int main()
{
   const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

	spo2_write(dev, I2C0, 0X57, MAX30100_REG_FIFO_DATA, DEFAULT_MODE);

	// Initialize the PulseOximeter instance
	// Failures are generally due to an improper I2C wiring, missing power supply
	// or wrong target chip
	if (!begin(dev)) 
	{
		printf("\nFAILED");
		for(;;);
	} 
	else 
	{
		printf("\nSUCCESS");
	}

	printf("\nTemp %.0f",retrieveTemperature(dev));
}